// uc_orig: memory.cpp (fallen/Source/memory.cpp)
// Game-level memory management and level save/load system.
// First chunk: globals init, level memory table, pointer/index conversion, anim serialization.

#include "things/core/thing.h" // pool types (Vehicle, Person, etc.)
#include "game/game_types.h" // Game struct, TICK_RATIO, PEOPLE, VEHICLES, etc.
#include "buildings/prim.h" // calc_prim_normals, calc_prim_info, mark_prim_objects_as_unloaded, etc.
#include "assets/formats/anim_globals.h" // game_chunk, anim_chunk, next_game_chunk, next_anim_chunk, next_prim_*
#include "assets/texture.h"
#include "engine/graphics/pipeline/aeng.h"
#include "camera/fc.h"
#include "navigation/wmove.h"
#include "navigation/wmove_globals.h"
#include "map/supermap.h"
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "things/items/barrel.h"
#include "things/items/barrel_globals.h"
#include "missions/eway_globals.h"
#include "effects/environment/tracks.h"
#include "things/animals/bat.h"
#include "missions/playcuts.h"
#include "missions/playcuts_globals.h"

#include "things/core/statedef.h"
#include "world_objects/plat_globals.h"
#include "engine/audio/sound_globals.h"

#include "missions/memory.h"
#include "missions/memory_globals.h"

extern void BAT_normal(Thing* p_thing);

// TEXTURE_set — texture set index, defined in assets/texture_globals.cpp.
extern SLONG TEXTURE_set;

// EWAY scripting time counters — defined in eway.cpp, used by load_whole_game.
extern SLONG EWAY_count_up;
extern UBYTE EWAY_count_up_add_penalties;
extern SWORD EWAY_count_up_num_penalties;
extern UWORD EWAY_count_up_penalty_timer;

// EWAY fake wander text indices — defined in eway.cpp.
extern UWORD EWAY_fake_wander_text_normal_index;
extern UWORD EWAY_fake_wander_text_normal_number;
extern UWORD EWAY_fake_wander_text_guilty_index;
extern UWORD EWAY_fake_wander_text_guilty_number;
extern UWORD EWAY_fake_wander_text_annoyed_index;
extern UWORD EWAY_fake_wander_text_annoyed_number;

// EWAY time and tick counters — defined in eway.cpp.
extern SLONG EWAY_time_accurate;
extern SLONG EWAY_time;
extern SLONG EWAY_tick;

// EWAY camera override state — defined in eway.cpp.
extern SLONG EWAY_cam_active;
extern SLONG EWAY_cam_x;
extern SLONG EWAY_cam_y;
extern SLONG EWAY_cam_z;
extern SLONG EWAY_cam_dx;
extern SLONG EWAY_cam_dy;
extern SLONG EWAY_cam_dz;
extern SLONG EWAY_cam_yaw;
extern SLONG EWAY_cam_pitch;
extern SLONG EWAY_cam_waypoint;
extern SLONG EWAY_cam_target;
extern SLONG EWAY_cam_delay;
extern SLONG EWAY_cam_speed;
extern SLONG EWAY_cam_freeze;
extern SLONG EWAY_cam_cant_interrupt;

// uc_orig: init_memory (fallen/Source/memory.cpp)
void init_memory(void)
{
    SLONG c0 = 0;
    SLONG mem_size, mem_cumlative = 0;
    struct MemTable* p_tab;
    UBYTE* p_all;

    // Override runtime maxima based on engine constants.
    save_table[SAVE_TABLE_PEOPLE].Maximum = RMAX_PEOPLE;
    save_table[SAVE_TABLE_VEHICLE].Maximum = RMAX_VEHICLES;
    save_table[SAVE_TABLE_SPECIAL].Maximum = RMAX_SPECIALS;
    save_table[SAVE_TABLE_BAT].Maximum = RBAT_MAX_BATS;
    save_table[SAVE_TABLE_DTWEEN].Maximum = RMAX_DRAW_TWEENS;
    save_table[SAVE_TABLE_DMESH].Maximum = RMAX_DRAW_MESHES;

    extern UBYTE music_max_gain;

    // Sum up total memory needed for all array slots.
    while (save_table[c0].Point) {
        p_tab = &save_table[c0];
        mem_size = p_tab->StructSize * p_tab->Maximum;
        mem_cumlative += mem_size;
        c0++;
    }

    c0 = 0;
    mem_cumlative += 1024;

    if (mem_all) {
        MemFree(mem_all);
        mem_all = NULL;
    }

    mem_all = MemAlloc(mem_cumlative);
    ASSERT(mem_all);
    mem_all_size = mem_cumlative;

    p_all = (UBYTE*)mem_all;

    // Partition the flat block among all entries, aligning each to 4 bytes.
    while (save_table[c0].Point) {
        p_tab = &save_table[c0];
        mem_size = p_tab->StructSize * p_tab->Maximum;

        mem_size += 3;
        mem_size &= 0xfffffffc;
        *p_tab->Point = p_all;
        p_all += mem_size;
        c0++;
    }

    anim_mids = (PrimPoint*)MemAlloc(256 * sizeof(PrimPoint));
    // Furniture isn't in the save table, so allocate separately.
    the_game.Furnitures = (Furniture*)MemAlloc(sizeof(Furniture) * MAX_FURNITURE);
    prim_normal = (PrimNormal*)MemAlloc(sizeof(PrimNormal) * MAX_PRIM_POINTS);
}

// Restore keyframe offsets back to pointers (post-load).
// uc_orig: convert_keyframe_to_pointer (fallen/Source/memory.cpp)
void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;

    ASSERT((((uintptr_t)p) & 3) == 0);
    ASSERT((((uintptr_t)p_ele) & 3) == 0);
    ASSERT((((uintptr_t)p_fight) & 3) == 0);

    // Pointer fields contain 32-bit index values loaded via _Disk structs.
    // Must sign-extend via int32_t: -1 sentinel = 0xFFFFFFFF which zero-extends
    // to 0x00000000FFFFFFFF on x64, appearing positive to (intptr_t).
    for (c0 = 0; c0 < count; c0++) {
        int32_t idx;

        idx = (int32_t)(uintptr_t)p[c0].FirstElement;
        p[c0].FirstElement = (idx < 0) ? NULL : &p_ele[idx];

        idx = (int32_t)(uintptr_t)p[c0].PrevFrame;
        p[c0].PrevFrame = (idx < 0) ? NULL : &p[idx];

        idx = (int32_t)(uintptr_t)p[c0].NextFrame;
        p[c0].NextFrame = (idx < 0) ? NULL : &p[idx];

        idx = (int32_t)(uintptr_t)p[c0].Fight;
        p[c0].Fight = (idx < 0) ? NULL : &p_fight[idx];
    }
}

// uc_orig: convert_animlist_to_pointer (fallen/Source/memory.cpp)
void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        int32_t idx = (int32_t)(uintptr_t)p[c0];
        p[c0] = &p_anim[idx];
    }
}

// uc_orig: convert_fightcol_to_pointer (fallen/Source/memory.cpp)
void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        int32_t idx = (int32_t)(uintptr_t)p[c0].Next;
        p[c0].Next = (idx < 0) ? 0 : &p_fight[idx];
    }
}

// Stub: save_whole_game was present in source but had an empty body.
// uc_orig: save_whole_game (fallen/Source/memory.cpp)
void save_whole_game(CBYTE* gamename)
{
}

// Restore the draw pointer for a single thing from an integer offset, by draw type.
// uc_orig: convert_drawtype_to_pointer (fallen/Source/memory.cpp)
void convert_drawtype_to_pointer(Thing* p_thing, SLONG meshtype)
{
    switch (meshtype) {
    case DT_MESH: {
        DrawMesh* drawtype;
        drawtype = TO_DRAW_MESH((uintptr_t)p_thing->Draw.Mesh);
        p_thing->Draw.Mesh = drawtype;
        drawtype->Cache = 0;
    } break;
    case DT_ROT_MULTI:
    case DT_ANIM_PRIM:
    case DT_BIKE: {
        DrawTween* drawtype;
        SLONG chunk;

        drawtype = TO_DRAW_TWEEN((uintptr_t)p_thing->Draw.Tweened);
        ASSERT((uintptr_t)(p_thing->Draw.Tweened) < RMAX_DRAW_TWEENS);
        p_thing->Draw.Tweened = drawtype;

        chunk = (intptr_t)drawtype->TheChunk;
        switch (chunk >> 16) {
        case 1:
            drawtype->TheChunk = &game_chunk[chunk & 0xffff];
            break;
        case 2:
            drawtype->TheChunk = &anim_chunk[chunk & 0xffff];
            break;
        default:
            ASSERT(0);
            break;
        }

        switch (p_thing->Class) {
        case CLASS_PERSON:
            drawtype->QueuedFrame = 0;
            drawtype->InterruptFrame = 0;
            if (p_thing->Genus.Person->PersonType == PERSON_CIV && (drawtype->CurrentAnim > 100 && drawtype->CurrentAnim < 140) && drawtype->CurrentAnim != 109) {
                drawtype->CurrentFrame = game_chunk[ANIM_TYPE_CIV].AnimList[drawtype->CurrentAnim];
            } else if (p_thing->Genus.Person->PersonType == PERSON_COP && (drawtype->CurrentAnim > 200 && drawtype->CurrentAnim < 220)) {
                drawtype->CurrentFrame = game_chunk[ANIM_TYPE_ROPER].AnimList[drawtype->CurrentAnim];
            } else
                drawtype->CurrentFrame = global_anim_array[p_thing->Genus.Person->AnimType][drawtype->CurrentAnim];

            if (drawtype->CurrentFrame)
                drawtype->NextFrame = drawtype->CurrentFrame->NextFrame;
            else
                drawtype->NextFrame = 0;

            if (p_thing->State == STATE_DEAD) {
                SLONG c0;
                SLONG old_tick;

                old_tick = TICK_RATIO;

                the_game.TickRatio = 1 << TICK_SHIFT;
                extern SLONG person_normal_animate(Thing * p_thing);
                for (c0 = 0; c0 < 40; c0++)
                    person_normal_animate(p_thing);

                the_game.TickRatio = old_tick;
            }
            break;
        case CLASS_BAT:

        case CLASS_BIKE:
        case CLASS_ANIM_PRIM:
            drawtype->QueuedFrame = 0;
            drawtype->InterruptFrame = 0;

            drawtype->CurrentFrame = anim_chunk[p_thing->Index].AnimList[drawtype->CurrentAnim];
            if (drawtype->CurrentFrame)
                drawtype->NextFrame = drawtype->CurrentFrame->NextFrame;
            else
                drawtype->NextFrame = 0;

            break;

        case CLASS_ANIMAL:
            drawtype->QueuedFrame = 0;
            drawtype->InterruptFrame = 0;
            drawtype->CurrentFrame = game_chunk[6].AnimList[1];
            if (drawtype->CurrentFrame)
                drawtype->NextFrame = drawtype->CurrentFrame->NextFrame;
            else
                drawtype->NextFrame = 0;
            break;

        default:
            ASSERT(0);
            drawtype->QueuedFrame = 0;
            drawtype->InterruptFrame = 0;
            drawtype->CurrentFrame = 0;
            drawtype->NextFrame = 0;
            break;
        }
    } break;
    }
}

// Restore all pointer fields of a single Thing from integer offsets (post-load).
// uc_orig: convert_thing_to_pointer (fallen/Source/memory.cpp)
void convert_thing_to_pointer(Thing* p_thing)
{
    extern void process_hardware_level_input_for_player(Thing * p_thing);
    extern void fn_anim_prim_normal(Thing * p_thing);

    switch (p_thing->Class) {

    case CLASS_NONE:
        break;
    case CLASS_PLAYER:
        p_thing->Genus.Player = (Player*)TO_PLAYER((intptr_t)p_thing->Genus.Player);
        p_thing->Genus.Player->PlayerPerson = (Thing*)TO_THING((intptr_t)p_thing->Genus.Player->PlayerPerson);
        p_thing->StateFn = process_hardware_level_input_for_player;

        break;
    case CLASS_CAMERA:
        break;
    case CLASS_PROJECTILE:
        p_thing->Genus.Projectile = (Projectile*)TO_PROJECTILE((intptr_t)p_thing->Genus.Projectile);
        break;
    case CLASS_BUILDING:
        break;
    case CLASS_PERSON:
        p_thing->Genus.Person = (Person*)TO_PERSON((intptr_t)p_thing->Genus.Person);
        set_generic_person_state_function(p_thing, p_thing->State);
        break;
    case CLASS_ANIMAL:
        p_thing->Genus.Animal = (Animal*)TO_ANIMAL((intptr_t)p_thing->Genus.Animal);

        set_state_function(p_thing, p_thing->State);

        break;
    case CLASS_FURNITURE:
        p_thing->Genus.Furniture = (Furniture*)TO_FURNITURE((intptr_t)p_thing->Genus.Furniture);
        break;
    case CLASS_SWITCH:
        p_thing->Genus.Switch = (Switch*)TO_SWITCH((intptr_t)p_thing->Genus.Switch);
        break;
    case CLASS_VEHICLE:
        p_thing->Genus.Vehicle = (Vehicle*)TO_VEHICLE((intptr_t)p_thing->Genus.Vehicle);
        set_state_function(p_thing, p_thing->State);
        break;
    case CLASS_SPECIAL:
        p_thing->Genus.Special = (Special*)TO_SPECIAL((intptr_t)p_thing->Genus.Special);
        void special_normal(Thing * s_thing);
        p_thing->StateFn = special_normal;

        break;
    case CLASS_CHOPPER:
        void CHOPPER_fn_normal(Thing*);
        p_thing->Genus.Chopper = (Chopper*)TO_CHOPPER((intptr_t)p_thing->Genus.Chopper);
        p_thing->StateFn = CHOPPER_fn_normal;

        break;
    case CLASS_PYRO:
        p_thing->Genus.Pyro = (Pyro*)TO_PYRO((intptr_t)p_thing->Genus.Pyro);
        set_state_function(p_thing, p_thing->State);
        break;
    case CLASS_TRACK:
        p_thing->Genus.Track = (Track*)TO_TRACK((intptr_t)p_thing->Genus.Track);
        break;
    case CLASS_PLAT:
        p_thing->Genus.Plat = (Plat*)TO_PLAT((intptr_t)p_thing->Genus.Plat);
        p_thing->StateFn = PLAT_process;
        break;
    case CLASS_BARREL:
        void BARREL_process_normal(Thing * p_barrel);
        p_thing->Genus.Barrel = (Barrel*)TO_BARREL((intptr_t)p_thing->Genus.Barrel);
        p_thing->StateFn = BARREL_process_normal;

        break;

    case CLASS_BAT:
        p_thing->StateFn = BAT_normal;
        p_thing->Genus.Bat = (Bat*)TO_BAT((intptr_t)p_thing->Genus.Bat);
        break;

    default:
        ASSERT(0);
        break;
    }
    switch (p_thing->DrawType) {
    case DT_MESH:
    case DT_CHOPPER:
        convert_drawtype_to_pointer(p_thing, DT_MESH);
        break;
    case DT_ROT_MULTI:
        convert_drawtype_to_pointer(p_thing, DT_ROT_MULTI);
        break;
    case DT_ANIM_PRIM:
    case DT_BIKE:
        convert_drawtype_to_pointer(p_thing, DT_ANIM_PRIM);
        break;
    }
}

// Restore all pointer fields for all Things, players, EWAY messages, pyros, and cutscenes.
// uc_orig: convert_index_to_pointers (fallen/Source/memory.cpp)
void convert_index_to_pointers(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_THINGS; c0++) {
        convert_thing_to_pointer(TO_THING(c0));
    }

    for (c0 = 0; c0 < MAX_PLAYERS; c0++) {
        NET_PERSON(c0) = TO_THING((intptr_t)NET_PERSON(c0));
        NET_PLAYER(c0) = TO_THING((intptr_t)NET_PLAYER(c0));
    }

    for (c0 = 0; c0 < EWAY_mess_upto; c0++) {
        EWAY_mess[c0] = (CBYTE*)&EWAY_mess_buffer[(intptr_t)EWAY_mess[c0]];
    }

    for (c0 = 0; c0 < MAX_PYROS; c0++) {
        if (TO_PYRO(c0)->thing)
            TO_PYRO(c0)->thing = TO_THING((uintptr_t)TO_PYRO(c0)->thing);

        if (TO_PYRO(c0)->victim)
            TO_PYRO(c0)->victim = TO_THING((uintptr_t)TO_PYRO(c0)->victim);
    }
    for (c0 = 0; c0 < PLAYCUTS_cutscene_ctr; c0++) {
        PLAYCUTS_cutscenes[c0].channels = PLAYCUTS_tracks + (intptr_t)PLAYCUTS_cutscenes[c0].channels;
    }
    for (c0 = 0; c0 < PLAYCUTS_track_ctr; c0++) {
        PLAYCUTS_tracks[c0].packets = PLAYCUTS_packets + (intptr_t)PLAYCUTS_tracks[c0].packets;
    }
    // PLAYCUTS text packets: pos.X already stores offset into PLAYCUTS_text_data (not a pointer),
    // so no conversion needed for save/load.
}

// Reset dynamic lighting caches so they are recalculated on next use.
// uc_orig: uncache (fallen/Source/memory.cpp)
void uncache(void)
{
    SLONG c0;
    void NIGHT_destroy_all_cached_info();
    NIGHT_destroy_all_cached_info();

    for (c0 = 0; c0 < next_dfacet; c0++) {
        dfacets[c0].Dfcache = 0;
    }
}

// uc_orig: GET_DATA (fallen/Source/memory.cpp)
#define GET_DATA(d)                       \
    memcpy((UBYTE*)&d, p_all, sizeof(d)); \
    p_all += sizeof(d)

// Deserialize all animation chunk data from a raw memory buffer (post-level-load).
// uc_orig: load_whole_anims (fallen/Source/memory.cpp)
void load_whole_anims(UBYTE* p_all)
{
    SLONG c0, c1;
    SLONG dummy;
    SLONG check;

    GET_DATA(next_game_chunk);
    GET_DATA(next_anim_chunk);

    for (c0 = 0; c0 < next_game_chunk; c0++) {
        GET_DATA(dummy);
        if (dummy >= 0)
            ASSERT(dummy == c0);

        if (dummy == c0) {
            struct GameKeyFrameChunk* gc;
            gc = &game_chunk[c0];
            GET_DATA(gc->MaxPeopleTypes);
            GET_DATA(gc->MaxKeyFrames);
            GET_DATA(gc->MaxAnimFrames);
            GET_DATA(gc->MaxFightCols);
            GET_DATA(gc->MaxElements);
            GET_DATA(gc->ElementCount);

            GET_DATA(check);
            ASSERT(check == 666);

            for (c1 = 0; c1 < 10; c1++) {
                GET_DATA(gc->MultiObject[c1]);
            }

            GET_DATA(check);
            ASSERT(check == 666);
            gc->PeopleTypes = (struct BodyDef*)p_all;
            p_all += gc->MaxPeopleTypes * sizeof(struct BodyDef);

            GET_DATA(check);
            ASSERT(check == 666);
            gc->AnimKeyFrames = (GameKeyFrame*)p_all;
            p_all += gc->MaxKeyFrames * sizeof(GameKeyFrame);

            GET_DATA(check);
            ASSERT(check == 666);
            gc->AnimList = (GameKeyFrame**)p_all;
            p_all += gc->MaxAnimFrames * sizeof(GameKeyFrame*);

            GET_DATA(check);
            ASSERT(check == 666);
            gc->TheElements = (GameKeyFrameElement*)p_all;
            p_all += gc->MaxElements * sizeof(GameKeyFrameElement);

            GET_DATA(check);
            ASSERT(check == 666);
            gc->FightCols = (GameFightCol*)p_all;
            p_all += gc->MaxFightCols * sizeof(GameFightCol);

            GET_DATA(check);
            ASSERT(check == 666);

            convert_keyframe_to_pointer(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_pointer(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_pointer(gc->FightCols, gc->FightCols, gc->MaxFightCols);
        }
    }

    for (c0 = 0; c0 < next_anim_chunk; c0++) {
        GET_DATA(dummy);
        if (dummy >= 0)
            ASSERT(dummy == c0);

        if (dummy == c0) {
            struct GameKeyFrameChunk* gc;
            gc = &anim_chunk[c0];
            GET_DATA(gc->MaxPeopleTypes);
            GET_DATA(gc->MaxKeyFrames);
            GET_DATA(gc->MaxAnimFrames);
            GET_DATA(gc->MaxFightCols);
            GET_DATA(gc->MaxElements);
            GET_DATA(gc->ElementCount);

            for (c1 = 0; c1 < 10; c1++) {
                GET_DATA(gc->MultiObject[c1]);
            }

            gc->PeopleTypes = (struct BodyDef*)p_all;
            p_all += gc->MaxPeopleTypes * sizeof(struct BodyDef);

            gc->AnimKeyFrames = (GameKeyFrame*)p_all;
            p_all += gc->MaxKeyFrames * sizeof(GameKeyFrame);

            gc->AnimList = (GameKeyFrame**)p_all;
            p_all += gc->MaxAnimFrames * sizeof(GameKeyFrame*);

            gc->TheElements = (GameKeyFrameElement*)p_all;
            p_all += gc->MaxElements * sizeof(GameKeyFrameElement);

            gc->FightCols = (GameFightCol*)p_all;
            p_all += gc->MaxFightCols * sizeof(GameFightCol);

            convert_keyframe_to_pointer(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_pointer(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_pointer(gc->FightCols, gc->FightCols, gc->MaxFightCols);
        }
    }
    GET_DATA(next_anim_mids);
    anim_mids = (PrimPoint*)p_all;
    p_all += next_anim_mids * sizeof(PrimPoint);
}

// Flag all near-vertical prim faces (where the face normal has very little Y component).
// uc_orig: flag_v_faces (fallen/Source/memory.cpp)
void flag_v_faces(void)
{
    SLONG c0;
    struct PrimFace4* p4;
    struct PrimFace3* p3;
    SLONG vx, vz, wx, wz, ny;

    p4 = &prim_faces4[0];
    for (c0 = 0; c0 < next_prim_face4; c0++) {
        vx = prim_points[p4->Points[1]].X - prim_points[p4->Points[0]].X;
        vz = prim_points[p4->Points[1]].Z - prim_points[p4->Points[0]].Z;

        wx = prim_points[p4->Points[0]].X - prim_points[p4->Points[2]].X;
        wz = prim_points[p4->Points[0]].Z - prim_points[p4->Points[2]].Z;

        ny = ((vz) * (wx)) - (vx * wz) >> 8;

        if (ny < 2)
            p4->FaceFlags |= FACE_FLAG_VERTICAL;

        p4++;
    }

    p3 = &prim_faces3[0];
    for (c0 = 0; c0 < next_prim_face3; c0++) {
        vx = prim_points[p3->Points[1]].X - prim_points[p3->Points[0]].X;
        vz = prim_points[p3->Points[1]].Z - prim_points[p3->Points[0]].Z;

        wx = prim_points[p3->Points[0]].X - prim_points[p3->Points[2]].X;
        wz = prim_points[p3->Points[0]].Z - prim_points[p3->Points[2]].Z;

        ny = ((vz) * (wx)) - (vx * wz) >> 8;

        if (ny < 2)
            p3->FaceFlags |= FACE_FLAG_VERTICAL;

        p3++;
    }
}

// Load full game state from a binary save file (old PC/Dreamcast format).
// Reads all save_table entries, game scalar state, animation chunks, then restores pointers.
// uc_orig: load_whole_game (fallen/Source/memory.cpp)
void load_whole_game(CBYTE* gamename)
{
    SLONG c0 = 0, id;
    UBYTE* p_all;
    SLONG mem_size;
    struct MemTable* ptab;
    MFFileHandle handle = FILE_OPEN_ERROR;
    SLONG count;
    SLONG texture_set_local;
    SLONG save_type;

    SLONG check;
    UBYTE padding_byte;
    UWORD padding_word;

    SetSeed(1234567);
    srand(1234567);

    extern UWORD player_dlight;
    player_dlight = 0;

    if (mem_all)
        MemFree(mem_all);

    handle = FileOpen(gamename);
    if (handle == FILE_OPEN_ERROR) {
        ASSERT(0);
        return;
    }

    mem_size = FileSize(handle);
    p_all = (UBYTE*)MemAlloc(mem_size);
    ASSERT(p_all);
    FileRead(handle, p_all, mem_size);
    FileClose(handle);

    mem_all = p_all;
    mem_all_size = mem_size;

    GET_DATA(save_type);

    while (save_table[c0].Point) {
        SLONG struct_size;
        id = *((SLONG*)p_all);
        ASSERT(id == c0);
        p_all += 4;

        count = *((SLONG*)p_all);
        p_all += 4;

        struct_size = *((SLONG*)p_all);
        p_all += 4;
        ASSERT(struct_size == save_table[c0].StructSize);

        mem_size = *((SLONG*)p_all);
        p_all += 4;

        if (mem_size & 3)
            p_all += 4 - (mem_size & 3);

        ptab = &save_table[c0];

        *ptab->Point = p_all;

        switch (ptab->Type) {
        case 2:
            if (ptab->Extra)
                ptab->Maximum = count;

            break;

        case 1:
            if (ptab->Extra)
                count -= ptab->Extra;

            if (ptab->CountL) {
                *ptab->CountL = count;
            } else {
                if (ptab->CountW)
                    *ptab->CountW = count;
            }
            break;
        }

        p_all += mem_size;

        c0++;
    }

    for (c0 = 0; c0 < EWAY_mess_buffer_upto; c0++)
        if (EWAY_mess_buffer[c0] == 160)
            EWAY_mess_buffer[c0] = 32;

    GET_DATA(PRIMARY_USED);
    GET_DATA(PRIMARY_UNUSED);
    GET_DATA(SECONDARY_USED);
    GET_DATA(SECONDARY_UNUSED);
    GET_DATA(PERSON_COUNT);
    GET_DATA(PERSON_COUNT);
    GET_DATA(ANIMAL_COUNT);
    GET_DATA(CHOPPER_COUNT);
    GET_DATA(PYRO_COUNT);
    GET_DATA(PLAYER_COUNT);
    GET_DATA(PROJECTILE_COUNT);
    GET_DATA(SPECIAL_COUNT);
    GET_DATA(SWITCH_COUNT);
    GET_DATA(PRIMARY_COUNT);
    GET_DATA(SECONDARY_COUNT);
    GET_DATA(GAME_STATE);

    GET_DATA(GAME_TURN);
    GET_DATA(GAME_FLAGS);
    GET_DATA(texture_set_local);
    GET_DATA(WMOVE_face_upto);
    GET_DATA(first_walkable_prim_point);
    GET_DATA(number_of_walkable_prim_points);
    GET_DATA(first_walkable_prim_face4);
    GET_DATA(number_of_walkable_prim_faces4);
    GET_DATA(BARREL_sphere_last);
    GET_DATA(track_head);
    GET_DATA(track_tail);
    GET_DATA(track_eob);
    GET_DATA(NIGHT_dlight_free);
    GET_DATA(NIGHT_dlight_used);

    GET_DATA(EWAY_time_accurate);
    GET_DATA(EWAY_time);
    GET_DATA(EWAY_tick);
    GET_DATA(EWAY_cam_active);
    GET_DATA(EWAY_cam_x);
    GET_DATA(EWAY_cam_y);
    GET_DATA(EWAY_cam_z);
    GET_DATA(EWAY_cam_dx);
    GET_DATA(EWAY_cam_dy);
    GET_DATA(EWAY_cam_dz);
    GET_DATA(EWAY_cam_yaw);
    GET_DATA(EWAY_cam_pitch);
    GET_DATA(EWAY_cam_waypoint);
    GET_DATA(EWAY_cam_target);
    GET_DATA(EWAY_cam_delay);
    GET_DATA(EWAY_cam_speed);
    GET_DATA(EWAY_cam_freeze);
    GET_DATA(EWAY_cam_cant_interrupt);
    GET_DATA(NIGHT_amb_colour);
    GET_DATA(NIGHT_amb_specular);
    GET_DATA(NIGHT_amb_red);
    GET_DATA(NIGHT_amb_green);
    GET_DATA(NIGHT_amb_blue);
    GET_DATA(NIGHT_amb_norm_x);
    GET_DATA(NIGHT_amb_norm_y);
    GET_DATA(NIGHT_amb_norm_z);
    GET_DATA(NIGHT_flag);
    GET_DATA(NIGHT_lampost_radius);
    GET_DATA(NIGHT_lampost_red);
    GET_DATA(NIGHT_lampost_green);
    GET_DATA(NIGHT_lampost_blue);
    GET_DATA(NIGHT_sky_colour);
    GET_DATA(padding_byte);
    GET_DATA(check);
    GET_DATA(CRIME_RATE);
    GET_DATA(CRIME_RATE_SCORE_MUL);
    GET_DATA(MUSIC_WORLD);
    GET_DATA(BOREDOM_RATE);
    GET_DATA(world_type);

    GET_DATA(EWAY_fake_wander_text_normal_index);
    GET_DATA(EWAY_fake_wander_text_normal_number);
    GET_DATA(EWAY_fake_wander_text_guilty_index);
    GET_DATA(EWAY_fake_wander_text_guilty_number);
    GET_DATA(EWAY_fake_wander_text_annoyed_index);
    GET_DATA(EWAY_fake_wander_text_annoyed_number);

    GET_DATA(GAME_FLAGS);

    GET_DATA(semtex);
    GET_DATA(estate);
    GET_DATA(padding_word);

    ASSERT(check == 666);

    EWAY_time_accurate = 0;
    EWAY_time = 0;
    EWAY_count_up = 0;

    EWAY_count_up_add_penalties = 0;
    EWAY_count_up_num_penalties = 0;
    EWAY_count_up_penalty_timer = 0;

    SATURATE(NIGHT_amb_blue, 25, 127);

    load_whole_anims(p_all);

    void setup_global_anim_array(void);
    setup_global_anim_array();
    void init_dead_tween(void);
    init_dead_tween();

    convert_index_to_pointers();

    FC_look_at(0, THING_NUMBER(NET_PERSON(0)));
    FC_force_camera_behind(0);

    uncache();

    {
        TEXTURE_choose_set(texture_set_local);
        ASSERT(0);
    }

    extern void PARTICLE_Reset();
    PARTICLE_Reset();

    extern void POW_init();
    POW_init();

    extern void PCOM_init(void);
    PCOM_init();

    VEH_init_vehinfo();

    extern void init_noises(void);
    extern void init_arrest(void);
    init_noises();
    init_arrest();

    calc_prim_normals();

    AENG_create_dx_prim_points();
    NIGHT_generate_walkable_lighting();

    flag_v_faces();
}

// uc_orig: MEMORY_QUICK_FNAME (fallen/Source/memory.cpp)
#define MEMORY_QUICK_FNAME "data/quicksave.dat"
// uc_orig: MEMORY_QUICK_CHECK (fallen/Source/memory.cpp)
#define MEMORY_QUICK_CHECK 314159265

// Delete any existing quick save so it won't be offered on the load screen.
// uc_orig: MEMORY_quick_init (fallen/Source/memory.cpp)
void MEMORY_quick_init()
{
    FileDelete(MEMORY_QUICK_FNAME);

    MEMORY_quick_avaliable = UC_FALSE;
}

// Write the full game state to the quick save file using stdio.
// On write error the file is deleted and the slot is marked unavailable.
// uc_orig: MEMORY_quick_save (fallen/Source/memory.cpp)
void MEMORY_quick_save()
{
    SLONG i;
    UBYTE padding_byte;
    SLONG check = 666;
    SLONG checksum;

    FILE* handle = MF_Fopen(MEMORY_QUICK_FNAME, "wb");

    if (!handle) {
        return;
    }

    SLONG version = 1;

    if (fwrite(&version, sizeof(SLONG), 1, handle) != 1)
        goto file_error;

    MemTable* mt;

    for (i = 0; save_table[i].Point; i++) {
        mt = &save_table[i];

        if (fwrite(mt->Point, mt->StructSize, mt->Maximum, handle) != (unsigned)mt->Maximum)
            goto file_error;
    }

// uc_orig: QSTORE_DATA (fallen/Source/memory.cpp)
#define QSTORE_DATA(x)                           \
    if (fwrite(&(x), sizeof(x), 1, handle) != 1) \
        goto file_error;

    QSTORE_DATA(PRIMARY_USED);
    QSTORE_DATA(PRIMARY_UNUSED);
    QSTORE_DATA(SECONDARY_USED);
    QSTORE_DATA(SECONDARY_UNUSED);
    QSTORE_DATA(PERSON_COUNT);
    QSTORE_DATA(PERSON_COUNT);
    QSTORE_DATA(ANIMAL_COUNT);
    QSTORE_DATA(CHOPPER_COUNT);
    QSTORE_DATA(PYRO_COUNT);
    QSTORE_DATA(PLAYER_COUNT);
    QSTORE_DATA(PROJECTILE_COUNT);
    QSTORE_DATA(SPECIAL_COUNT);
    QSTORE_DATA(SWITCH_COUNT);
    QSTORE_DATA(PRIMARY_COUNT);
    QSTORE_DATA(SECONDARY_COUNT);
    QSTORE_DATA(GAME_STATE);
    QSTORE_DATA(GAME_TURN);
    QSTORE_DATA(GAME_FLAGS);
    QSTORE_DATA(TEXTURE_set);
    QSTORE_DATA(WMOVE_face_upto)
    QSTORE_DATA(first_walkable_prim_point);
    QSTORE_DATA(number_of_walkable_prim_points);
    QSTORE_DATA(first_walkable_prim_face4);
    QSTORE_DATA(number_of_walkable_prim_faces4);
    QSTORE_DATA(BARREL_sphere_last);
    QSTORE_DATA(track_head);
    QSTORE_DATA(track_tail);
    QSTORE_DATA(track_eob);
    QSTORE_DATA(NIGHT_dlight_free);
    QSTORE_DATA(NIGHT_dlight_used);

    QSTORE_DATA(EWAY_time_accurate);
    QSTORE_DATA(EWAY_time);
    QSTORE_DATA(EWAY_tick);
    QSTORE_DATA(EWAY_cam_active);
    QSTORE_DATA(EWAY_cam_x);
    QSTORE_DATA(EWAY_cam_y);
    QSTORE_DATA(EWAY_cam_z);
    QSTORE_DATA(EWAY_cam_dx);
    QSTORE_DATA(EWAY_cam_dy);
    QSTORE_DATA(EWAY_cam_dz);
    QSTORE_DATA(EWAY_cam_yaw);
    QSTORE_DATA(EWAY_cam_pitch);
    QSTORE_DATA(EWAY_cam_waypoint);
    QSTORE_DATA(EWAY_cam_target);
    QSTORE_DATA(EWAY_cam_delay);
    QSTORE_DATA(EWAY_cam_speed);
    QSTORE_DATA(EWAY_cam_freeze);
    QSTORE_DATA(EWAY_cam_cant_interrupt);

    QSTORE_DATA(NIGHT_amb_colour);
    QSTORE_DATA(NIGHT_amb_specular);
    QSTORE_DATA(NIGHT_amb_red);
    QSTORE_DATA(NIGHT_amb_green);
    QSTORE_DATA(NIGHT_amb_blue);
    QSTORE_DATA(NIGHT_amb_norm_x);
    QSTORE_DATA(NIGHT_amb_norm_y);
    QSTORE_DATA(NIGHT_amb_norm_z);
    QSTORE_DATA(NIGHT_flag);
    QSTORE_DATA(NIGHT_lampost_radius);
    QSTORE_DATA(NIGHT_lampost_red);
    QSTORE_DATA(NIGHT_lampost_green);
    QSTORE_DATA(NIGHT_lampost_blue);
    QSTORE_DATA(NIGHT_sky_colour);
    QSTORE_DATA(padding_byte);
    QSTORE_DATA(check);
    QSTORE_DATA(CRIME_RATE);
    QSTORE_DATA(CRIME_RATE_SCORE_MUL);
    QSTORE_DATA(MUSIC_WORLD);
    QSTORE_DATA(BOREDOM_RATE);
    QSTORE_DATA(world_type);

    QSTORE_DATA(EWAY_fake_wander_text_normal_index);
    QSTORE_DATA(EWAY_fake_wander_text_normal_number);
    QSTORE_DATA(EWAY_fake_wander_text_guilty_index);
    QSTORE_DATA(EWAY_fake_wander_text_guilty_number);
    QSTORE_DATA(EWAY_fake_wander_text_annoyed_index);
    QSTORE_DATA(EWAY_fake_wander_text_annoyed_number);

    QSTORE_DATA(GAME_FLAGS);

    if (fwrite(NIGHT_square, sizeof(NIGHT_Square), NIGHT_MAX_SQUARES, handle) != NIGHT_MAX_SQUARES)
        goto file_error;
    if (fwrite(&NIGHT_square_free, sizeof(NIGHT_square_free), 1, handle) != 1)
        goto file_error;
    if (fwrite(NIGHT_cache, sizeof(UBYTE), PAP_SIZE_LO * PAP_SIZE_LO, handle) != PAP_SIZE_LO * PAP_SIZE_LO)
        goto file_error;

    if (fwrite(NIGHT_dfcache, sizeof(NIGHT_Dfcache), NIGHT_MAX_DFCACHES, handle) != NIGHT_MAX_DFCACHES)
        goto file_error;
    if (fwrite(&NIGHT_dfcache_used, sizeof(UBYTE), 1, handle) != 1)
        goto file_error;
    if (fwrite(&NIGHT_dfcache_free, sizeof(UBYTE), 1, handle) != 1)
        goto file_error;

    checksum = MEMORY_QUICK_CHECK;

    QSTORE_DATA(checksum);

    MF_Fclose(handle);

    MEMORY_quick_avaliable = UC_TRUE;

    return;

file_error:;

    MF_Fclose(handle);

    FileDelete(MEMORY_QUICK_FNAME);

    MEMORY_quick_avaliable = UC_FALSE;

    return;
}

// Load game state from the quick save file.
// Restores all save_table entries and all scalar game state,
// then recalculates lighting caches. Returns UC_TRUE on success.
// uc_orig: MEMORY_quick_load (fallen/Source/memory.cpp)
SLONG MEMORY_quick_load()
{
    SLONG i;
    UBYTE padding_byte;
    SLONG check = 666;
    SLONG checksum;

    FILE* handle = MF_Fopen(MEMORY_QUICK_FNAME, "rb");

    if (!handle) {
        return UC_FALSE;
    }

    SLONG version;

    if (fread(&version, sizeof(SLONG), 1, handle) != 1)
        goto file_error;

    ASSERT(version == 1);

    MemTable* mt;

    for (i = 0; save_table[i].Point; i++) {
        mt = &save_table[i];

        if (fread(mt->Point, mt->StructSize, mt->Maximum, handle) != (unsigned)mt->Maximum)
            goto file_error;
    }

// uc_orig: QREAD_DATA (fallen/Source/memory.cpp)
#define QREAD_DATA(x)                           \
    if (fread(&(x), sizeof(x), 1, handle) != 1) \
        goto file_error;

    QREAD_DATA(PRIMARY_USED);
    QREAD_DATA(PRIMARY_UNUSED);
    QREAD_DATA(SECONDARY_USED);
    QREAD_DATA(SECONDARY_UNUSED);
    QREAD_DATA(PERSON_COUNT);
    QREAD_DATA(PERSON_COUNT);
    QREAD_DATA(ANIMAL_COUNT);
    QREAD_DATA(CHOPPER_COUNT);
    QREAD_DATA(PYRO_COUNT);
    QREAD_DATA(PLAYER_COUNT);
    QREAD_DATA(PROJECTILE_COUNT);
    QREAD_DATA(SPECIAL_COUNT);
    QREAD_DATA(SWITCH_COUNT);
    QREAD_DATA(PRIMARY_COUNT);
    QREAD_DATA(SECONDARY_COUNT);
    QREAD_DATA(GAME_STATE);

    QREAD_DATA(GAME_TURN);
    QREAD_DATA(GAME_FLAGS);
    QREAD_DATA(TEXTURE_set);
    QREAD_DATA(WMOVE_face_upto)
    QREAD_DATA(first_walkable_prim_point);
    QREAD_DATA(number_of_walkable_prim_points);
    QREAD_DATA(first_walkable_prim_face4);
    QREAD_DATA(number_of_walkable_prim_faces4);
    QREAD_DATA(BARREL_sphere_last);
    QREAD_DATA(track_head);
    QREAD_DATA(track_tail);
    QREAD_DATA(track_eob);
    QREAD_DATA(NIGHT_dlight_free);
    QREAD_DATA(NIGHT_dlight_used);

    QREAD_DATA(EWAY_time_accurate);
    QREAD_DATA(EWAY_time);
    QREAD_DATA(EWAY_tick);
    QREAD_DATA(EWAY_cam_active);
    QREAD_DATA(EWAY_cam_x);
    QREAD_DATA(EWAY_cam_y);
    QREAD_DATA(EWAY_cam_z);
    QREAD_DATA(EWAY_cam_dx);
    QREAD_DATA(EWAY_cam_dy);
    QREAD_DATA(EWAY_cam_dz);
    QREAD_DATA(EWAY_cam_yaw);
    QREAD_DATA(EWAY_cam_pitch);
    QREAD_DATA(EWAY_cam_waypoint);
    QREAD_DATA(EWAY_cam_target);
    QREAD_DATA(EWAY_cam_delay);
    QREAD_DATA(EWAY_cam_speed);
    QREAD_DATA(EWAY_cam_freeze);
    QREAD_DATA(EWAY_cam_cant_interrupt);
    QREAD_DATA(NIGHT_amb_colour);
    QREAD_DATA(NIGHT_amb_specular);
    QREAD_DATA(NIGHT_amb_red);
    QREAD_DATA(NIGHT_amb_green);
    QREAD_DATA(NIGHT_amb_blue);
    QREAD_DATA(NIGHT_amb_norm_x);
    QREAD_DATA(NIGHT_amb_norm_y);
    QREAD_DATA(NIGHT_amb_norm_z);
    QREAD_DATA(NIGHT_flag);
    QREAD_DATA(NIGHT_lampost_radius);
    QREAD_DATA(NIGHT_lampost_red);
    QREAD_DATA(NIGHT_lampost_green);
    QREAD_DATA(NIGHT_lampost_blue);
    QREAD_DATA(NIGHT_sky_colour);
    QREAD_DATA(padding_byte);
    QREAD_DATA(check);
    QREAD_DATA(CRIME_RATE);
    QREAD_DATA(CRIME_RATE_SCORE_MUL);
    QREAD_DATA(MUSIC_WORLD);
    QREAD_DATA(BOREDOM_RATE);
    QREAD_DATA(world_type);

    QREAD_DATA(EWAY_fake_wander_text_normal_index);
    QREAD_DATA(EWAY_fake_wander_text_normal_number);
    QREAD_DATA(EWAY_fake_wander_text_guilty_index);
    QREAD_DATA(EWAY_fake_wander_text_guilty_number);
    QREAD_DATA(EWAY_fake_wander_text_annoyed_index);
    QREAD_DATA(EWAY_fake_wander_text_annoyed_number);

    QREAD_DATA(GAME_FLAGS);

    if (fread(NIGHT_square, sizeof(NIGHT_Square), NIGHT_MAX_SQUARES, handle) != NIGHT_MAX_SQUARES)
        goto file_error;
    if (fread(&NIGHT_square_free, sizeof(NIGHT_square_free), 1, handle) != 1)
        goto file_error;
    if (fread(NIGHT_cache, sizeof(UBYTE), PAP_SIZE_LO * PAP_SIZE_LO, handle) != PAP_SIZE_LO * PAP_SIZE_LO)
        goto file_error;

    if (fread(NIGHT_dfcache, sizeof(NIGHT_Dfcache), NIGHT_MAX_DFCACHES, handle) != NIGHT_MAX_DFCACHES)
        goto file_error;
    if (fread(&NIGHT_dfcache_used, sizeof(UBYTE), 1, handle) != 1)
        goto file_error;
    if (fread(&NIGHT_dfcache_free, sizeof(UBYTE), 1, handle) != 1)
        goto file_error;

    QREAD_DATA(checksum);

    if (checksum != MEMORY_QUICK_CHECK) {
        goto file_error;
    }

    MF_Fclose(handle);

    NIGHT_cache_recalc();
    NIGHT_dfcache_recalc();
    NIGHT_generate_walkable_lighting();

    return UC_TRUE;

file_error:;

    MF_Fclose(handle);

    return UC_FALSE;
}
