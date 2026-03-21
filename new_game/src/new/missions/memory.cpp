// uc_orig: memory.cpp (fallen/Source/memory.cpp)
// Game-level memory management and level save/load system.
// First chunk: globals init, level memory table, pointer/index conversion, anim serialization.

#include "fallen/Headers/Game.h"      // Temporary: Game struct, Thing system types, macro helpers
#include "fallen/Headers/ob.h"        // Temporary: OB_ob, OB_mapwho, OB_ob_upto
#include "fallen/Headers/fc.h"        // Temporary: FC_look_at, FC_force_camera_behind
#include "fallen/Headers/wmove.h"     // Temporary: WMOVE_Face, WMOVE_face, WMOVE_face_upto, RWMOVE_MAX_FACES
#include "fallen/Headers/supermap.h"  // Temporary: DBuilding/DFacet/DWalkable types and counters
#include "fallen/Headers/night.h"     // Temporary: NIGHT_slight, NIGHT_smap, NIGHT_dlight types and counts
#include "fallen/Headers/barrel.h"    // Temporary: BARREL_sphere, BARREL_barrel, barrel types
#include "fallen/Headers/eway.h"      // Temporary: EWAY_mess, EWAY_way, EWAY_cond, EWAY_edef, EWAY_timer
#include "fallen/Headers/pap.h"       // Temporary: PAP_hi, PAP_lo, PAP_SIZE_HI, PAP_SIZE_LO
#include "fallen/Headers/mav.h"       // Temporary: MAV_opt, MAV_nav, MAV_MAX_OPTS
#include "fallen/Headers/road.h"      // Temporary: ROAD_node, ROAD_edge types
#include "fallen/Headers/balloon.h"   // Temporary: BALLOON_balloon
#include "fallen/Headers/tracks.h"    // Temporary: tracks, TRACK_BUFFER_LENGTH, Track type
#include "fallen/Headers/ware.h"      // Temporary: WARE_ware, WARE_nav, WARE_height, WARE_rooftex
#include "fallen/Headers/trip.h"      // Temporary: TRIP_wire
#include "fallen/Headers/psystem.h"   // Temporary: PYROS, MAX_PYROS, Pyro type
#include "fallen/Headers/env.h"       // Temporary: ENV types
#include "fallen/Headers/bat.h"       // Temporary: BATS, BAT types
#include "fallen/Headers/door.h"      // Temporary: DOOR_door
#include "fallen/Headers/spark.h"     // Temporary: spark types
#include "fallen/Headers/playcuts.h"  // Temporary: PLAYCUTS_cutscenes, CPData, CPChannel, CPPacket types
#include "fallen/Headers/statedef.h"  // Temporary: STATE_DEAD, set_generic_person_state_function, set_state_function
#include "fallen/DDEngine/Headers/poly.h" // Temporary: POLY_PAGE_BLOODSPLAT
#include "fallen/Headers/sound.h"     // Temporary: SOUND_FXMapping, SOUND_FXGroups

#include "missions/memory.h"
#include "missions/memory_globals.h"

extern ULONG level_index;
extern void BAT_normal(Thing* p_thing);

// uc_orig: release_memory (fallen/Source/memory.cpp)
void release_memory(void)
{
}

// uc_orig: init_memory (fallen/Source/memory.cpp)
void init_memory(void)
{
    SLONG c0 = 0;
    SLONG mem_size, mem_cumlative = 0;
    struct MemTable* p_tab;
    UBYTE* p_all;
    SLONG temp;

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
        void* ptr;
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
        void* ptr;
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

// uc_orig: set_darci_normals (fallen/Source/memory.cpp)
void set_darci_normals(void)
{
    SLONG count_vertex;
    SLONG c0, c1, index;
    SLONG sp, ep;
    SLONG last_point = 0, first_point = 0x7fffffff;
    SLONG start_object;
    for (c0 = 1; c0 < darci_normal_count; c0++) {
        SLONG nx, ny, nz, c;

        nx = prim_normal[c0].X;
        ny = prim_normal[c0].Y;
        nz = prim_normal[c0].Z;
        SATURATE(nx, -256, 255);
        SATURATE(ny, -256, 255);
        SATURATE(nz, -256, 255);

        nx >>= 4;
        nx += 16;
        ASSERT(nx >= 0 && nx <= 31);

        ny >>= 4;
        ny += 16;
        ASSERT(ny >= 0 && ny <= 31);

        nz >>= 4;
        nz += 16;
        ASSERT(nz >= 0 && nz <= 31);

        nx &= 31;
        ny &= 31;
        nz &= 31;

        c = nx << 10;
        c |= ny << 5;
        c |= nz;
        darci_normal[c0] = c;
    }
    return;
}

// Convert a draw pointer for a single thing to an integer offset, by draw type.
// uc_orig: convert_drawtype_to_index (fallen/Source/memory.cpp)
void convert_drawtype_to_index(Thing* p_thing, SLONG meshtype)
{
    switch (meshtype) {
    case DT_MESH:
        if (p_thing->Draw.Mesh) {
            ULONG drawtype;
            drawtype = (p_thing->Draw.Mesh - DRAW_MESHES);
            p_thing->Draw.Mesh = (DrawMesh*)drawtype;
        }
        break;
    case DT_ROT_MULTI:
    case DT_ANIM_PRIM:
    case DT_BIKE:
        if (p_thing->Draw.Tweened) {
            ULONG drawtype;
            SLONG chunk;

            switch (p_thing->Class) {
            case CLASS_BIKE:
            case CLASS_PERSON:
            case CLASS_VEHICLE:
            case CLASS_ANIMAL:
                chunk = (ULONG)(p_thing->Draw.Tweened->TheChunk - &game_chunk[0]);
                ASSERT(chunk >= 0);
                chunk |= 1 << 16;
                p_thing->Draw.Tweened->TheChunk = (GameKeyFrameChunk*)chunk;
                break;
            case CLASS_BAT:
            case CLASS_ANIM_PRIM:
                chunk = (ULONG)(p_thing->Draw.Tweened->TheChunk - &anim_chunk[0]);
                ASSERT(chunk >= 0);
                chunk |= 2 << 16;
                p_thing->Draw.Tweened->TheChunk = (GameKeyFrameChunk*)chunk;
                break;
            }

            drawtype = (p_thing->Draw.Tweened - DRAW_TWEENS);
            p_thing->Draw.Tweened = (DrawTween*)drawtype;
        }
        break;
    }
}

// Convert all pointer fields of a single Thing to integer offsets (pre-save).
// uc_orig: convert_thing_to_index (fallen/Source/memory.cpp)
void convert_thing_to_index(Thing* p_thing)
{
    switch (p_thing->DrawType) {
    case DT_MESH:
    case DT_CHOPPER:
        convert_drawtype_to_index(p_thing, DT_MESH);
        break;
    case DT_ROT_MULTI:
        convert_drawtype_to_index(p_thing, DT_ROT_MULTI);
        break;
    case DT_ANIM_PRIM:
    case DT_BIKE:
        convert_drawtype_to_index(p_thing, DT_ANIM_PRIM);
        break;
    }

    switch (p_thing->Class) {
    case CLASS_NONE:
        break;
    case CLASS_PLAYER:
        p_thing->Genus.Player->PlayerPerson = (Thing*)THING_NUMBER(p_thing->Genus.Player->PlayerPerson);
        p_thing->Genus.Player = (Player*)PLAYER_NUMBER(p_thing->Genus.Player);
        break;
    case CLASS_CAMERA:
        break;
    case CLASS_PROJECTILE:
        p_thing->Genus.Projectile = (Projectile*)PROJECTILE_NUMBER(p_thing->Genus.Projectile);
        break;
    case CLASS_BUILDING:
        break;
    case CLASS_PERSON:
        p_thing->Genus.Person = (Person*)PERSON_NUMBER(p_thing->Genus.Person);
        break;
    case CLASS_ANIMAL:
        p_thing->Genus.Animal = (Animal*)ANIMAL_NUMBERb(p_thing->Genus.Animal);
        break;
    case CLASS_FURNITURE:
        p_thing->Genus.Furniture = (Furniture*)FURNITURE_NUMBER(p_thing->Genus.Furniture);
        break;
    case CLASS_SWITCH:
        p_thing->Genus.Switch = (Switch*)SWITCH_NUMBER(p_thing->Genus.Switch);
        break;
    case CLASS_VEHICLE:
        p_thing->Genus.Vehicle = (Vehicle*)VEHICLE_NUMBER(p_thing->Genus.Vehicle);
        break;
    case CLASS_SPECIAL:
        p_thing->Genus.Special = (Special*)SPECIAL_NUMBER(p_thing->Genus.Special);
        break;
    case CLASS_ANIM_PRIM:
        break;
    case CLASS_CHOPPER:
        p_thing->Genus.Chopper = (Chopper*)CHOPPER_NUMBER(p_thing->Genus.Chopper);
        break;
    case CLASS_PYRO:
        p_thing->Genus.Pyro = (Pyro*)PYRO_NUMBER(p_thing->Genus.Pyro);
        break;
    case CLASS_TRACK:
        if (p_thing->Genus.Track->flags == TRACK_FLAGS_SPLUTTING)
            p_thing->Genus.Track->page = POLY_PAGE_BLOODSPLAT;
        p_thing->Genus.Track = (Track*)TRACK_NUMBER(p_thing->Genus.Track);
        break;
    case CLASS_PLAT:
        p_thing->Genus.Plat = (Plat*)PLAT_NUMBER(p_thing->Genus.Plat);
        break;
    case CLASS_BARREL:
        p_thing->Genus.Barrel = (Barrel*)BARREL_NUMBER(p_thing->Genus.Barrel);
        break;
    case CLASS_BAT:
        p_thing->Genus.Bat = (Bat*)BAT_NUMBER(p_thing->Genus.Bat);
        break;
    default:
        ASSERT(0);
        break;
    }
}

// uc_orig: convert_pointers_to_index (fallen/Source/memory.cpp)
void convert_pointers_to_index(void)
{
    SLONG c0, i;
    static int max_people = 0, max_car = 0, max_mesh = 0, max_tween = 0, max_anim = 0, max_special = 0, max_bat = 0;
    SLONG count_people = 0, count_car = 0, count_mesh = 0, count_tween = 0, count_anim = 0, count_special = 0, count_bat = 0;
    SLONG gap = 0;

    for (c0 = 0; c0 < MAX_THINGS; c0++) {
        convert_thing_to_index(TO_THING(c0));
    }
    for (i = 0; i < RMAX_DRAW_MESHES; i++) {
        if (DRAW_MESHES[i].Angle != 0xfafa) {
            if (i > count_mesh) {
                count_mesh = i;
            }
        }
    }
    for (c0 = 0; c0 < RMAX_DRAW_TWEENS; c0++) {
        if (!(DRAW_TWEENS[c0].Flags & DT_FLAG_UNUSED)) {
            if (c0 > count_tween)
                count_tween = c0;
        }
    }
    for (i = 0; i < RMAX_VEHICLES; i++) {
        if (TO_VEHICLE(i)->Spring[0].Compression != VEH_NULL) {
            if (i > count_car)
                count_car = i;
        }
    }
    for (c0 = 0; c0 < RMAX_PEOPLE; c0++) {
        if (PEOPLE[c0].AnimType != PERSON_NONE) {
            if (c0 > count_people)
                count_people = c0;
        }
    }
    for (c0 = 1; c0 < RMAX_SPECIALS; c0++) {
        if (SPECIALS[c0].SpecialType != SPECIAL_NONE) {
            if (c0 > count_special)
                count_special = c0;
        }
    }
    for (i = 0; i < RBAT_MAX_BATS; i++) {
        if (TO_BAT(i)->type != BAT_TYPE_UNUSED) {
            if (i > count_bat)
                count_bat = i;
        }
    }

    save_table[SAVE_TABLE_PEOPLE].Maximum = MIN(save_table[SAVE_TABLE_PEOPLE].Extra + count_people, RMAX_PEOPLE);
    save_table[SAVE_TABLE_VEHICLE].Maximum = MIN(save_table[SAVE_TABLE_VEHICLE].Extra + count_car, RMAX_VEHICLES);
    save_table[SAVE_TABLE_SPECIAL].Maximum = MIN(save_table[SAVE_TABLE_SPECIAL].Extra + count_special, RMAX_SPECIALS);
    save_table[SAVE_TABLE_BAT].Maximum = MIN(save_table[SAVE_TABLE_BAT].Extra + count_bat, RBAT_MAX_BATS);
    save_table[SAVE_TABLE_DTWEEN].Maximum = MIN(save_table[SAVE_TABLE_DTWEEN].Extra + count_tween, RMAX_DRAW_TWEENS);
    save_table[SAVE_TABLE_DMESH].Maximum = MIN(save_table[SAVE_TABLE_DMESH].Extra + count_mesh, RMAX_DRAW_MESHES);

    extern CBYTE ELEV_fname_level[];
    if (level_index == 0) {
        if (strstr(ELEV_fname_level, "FTutor")) {
        } else {
        }
    }
    // Cop killers, semtex, estate map, stern revenge: boost headroom for extra NPCs.
    if (level_index == 20 || level_index == 19 || level_index == 26 || level_index == 24 || strstr(ELEV_fname_level, "Album1")) {
        save_table[SAVE_TABLE_PEOPLE].Maximum = MIN(save_table[SAVE_TABLE_PEOPLE].Extra + count_people + 30, RMAX_PEOPLE);
        save_table[SAVE_TABLE_DTWEEN].Maximum = MIN(save_table[SAVE_TABLE_DTWEEN].Extra + count_tween + 30, RMAX_DRAW_TWEENS);
        save_table[SAVE_TABLE_DMESH].Maximum = MIN(save_table[SAVE_TABLE_DMESH].Extra + count_mesh + 30, RMAX_DRAW_MESHES);
    }

    if (count_special > max_special)
        max_special = count_special;
    if (count_bat > max_bat)
        max_bat = count_bat;
    if (count_mesh > max_mesh)
        max_mesh = count_mesh;
    if (count_tween > max_tween)
        max_tween = count_tween;
    if (count_anim > max_anim)
        max_anim = count_anim;
    if (count_car > max_car)
        max_car = count_car;
    if (count_people > max_people)
        max_people = count_people;

    for (c0 = 0; c0 < MAX_PLAYERS; c0++) {
        NET_PERSON(c0) = (Thing*)THING_NUMBER(NET_PERSON(c0));
        NET_PLAYER(c0) = (Thing*)THING_NUMBER(NET_PLAYER(c0));
    }
    for (c0 = 0; c0 < EWAY_mess_upto; c0++) {
        EWAY_mess[c0] = (CBYTE*)((SLONG)(EWAY_mess[c0] - EWAY_mess_buffer));
    }
    for (c0 = 0; c0 < PYRO_COUNT; c0++) {
        if (TO_PYRO(c0)->thing)
            TO_PYRO(c0)->thing = (Thing*)THING_NUMBER(TO_PYRO(c0)->thing);
        if (TO_PYRO(c0)->victim)
            TO_PYRO(c0)->victim = (Thing*)THING_NUMBER(TO_PYRO(c0)->victim);
    }
    // Cutscene: convert channel/packet pointers to integer offsets.
    for (c0 = 0; c0 < PLAYCUTS_cutscene_ctr; c0++) {
        PLAYCUTS_cutscenes[c0].channels = (CPChannel*)(PLAYCUTS_cutscenes[c0].channels - PLAYCUTS_tracks);
    }
    for (c0 = 0; c0 < PLAYCUTS_track_ctr; c0++) {
        PLAYCUTS_tracks[c0].packets = (CPPacket*)(PLAYCUTS_tracks[c0].packets - PLAYCUTS_packets);
    }
    for (c0 = 0; c0 < PLAYCUTS_packet_ctr; c0++) {
        if (PLAYCUTS_packets[c0].type == 5)
            PLAYCUTS_packets[c0].pos.X -= (ULONG)PLAYCUTS_text_data;
    }
}

#define STORE_DATA(a) \
    FileWrite(handle, (UBYTE*)&a, sizeof(a));

// Convert keyframe pointer fields to integer offsets (for serialization).
// uc_orig: convert_keyframe_to_index (fallen/Source/memory.cpp)
void convert_keyframe_to_index(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        p[c0].FirstElement = (GameKeyFrameElement*)((SLONG)(p[c0].FirstElement - p_ele));
        p[c0].PrevFrame = (GameKeyFrame*)((SLONG)(p[c0].PrevFrame - p));
        p[c0].NextFrame = (GameKeyFrame*)((SLONG)(p[c0].NextFrame - p));
        p[c0].Fight = (GameFightCol*)((SLONG)(p[c0].Fight - p_fight));
    }
}

// uc_orig: convert_animlist_to_index (fallen/Source/memory.cpp)
void convert_animlist_to_index(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        p[c0] = (GameKeyFrame*)((SLONG)(p[c0] - p_anim));
    }
}

// uc_orig: convert_fightcol_to_index (fallen/Source/memory.cpp)
void convert_fightcol_to_index(GameFightCol* p, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        p[c0].Next = (GameFightCol*)((SLONG)(p[c0].Next - p_fight));
    }
}

// Restore keyframe offsets back to pointers (post-load).
// uc_orig: convert_keyframe_to_pointer (fallen/Source/memory.cpp)
void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;

    ASSERT((((ULONG)p) & 3) == 0);
    ASSERT((((ULONG)p_ele) & 3) == 0);
    ASSERT((((ULONG)p_fight) & 3) == 0);

    for (c0 = 0; c0 < count; c0++) {
        if (((SLONG)p[c0].FirstElement) < 0)
            p[c0].FirstElement = NULL;
        else
            p[c0].FirstElement = &p_ele[(SLONG)p[c0].FirstElement];

        if (((SLONG)p[c0].PrevFrame) < 0)
            p[c0].PrevFrame = NULL;
        else
            p[c0].PrevFrame = &p[(SLONG)p[c0].PrevFrame];

        if (((SLONG)p[c0].NextFrame) < 0)
            p[c0].NextFrame = NULL;
        else
            p[c0].NextFrame = &p[(SLONG)p[c0].NextFrame];

        if (((SLONG)p[c0].Fight) < 0)
            p[c0].Fight = NULL;
        else
            p[c0].Fight = &p_fight[(SLONG)p[c0].Fight];
    }
}

// uc_orig: convert_animlist_to_pointer (fallen/Source/memory.cpp)
void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        p[c0] = &p_anim[(SLONG)p[c0]];
    }
}

// uc_orig: convert_fightcol_to_pointer (fallen/Source/memory.cpp)
void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count)
{
    SLONG c0;
    for (c0 = 0; c0 < count; c0++) {
        if (((SLONG)p[c0].Next) < 0)
            p[c0].Next = 0;
        else
            p[c0].Next = &p_fight[(SLONG)p[c0].Next];
    }
}

// Serialize all animation chunks (game_chunk[] and anim_chunk[]) to an open file handle.
// Converts pointers to indexes, writes, then restores.
// uc_orig: save_whole_anims (fallen/Source/memory.cpp)
void save_whole_anims(MFFileHandle handle)
{
    SLONG c0, c1;
    SLONG blank = -1;
    SLONG check = 666;

    STORE_DATA(next_game_chunk);
    STORE_DATA(next_anim_chunk);

    for (c0 = 0; c0 < next_game_chunk; c0++) {
        if (game_chunk[c0].MultiObject[0]) {
            struct GameKeyFrameChunk* gc;
            gc = &game_chunk[c0];
            STORE_DATA(c0);
            STORE_DATA(gc->MaxPeopleTypes);
            STORE_DATA(gc->MaxKeyFrames);
            STORE_DATA(gc->MaxAnimFrames);
            STORE_DATA(gc->MaxFightCols);
            STORE_DATA(gc->MaxElements);
            STORE_DATA(gc->ElementCount);

            convert_keyframe_to_index(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_index(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_index(gc->FightCols, gc->FightCols, gc->MaxFightCols);

            STORE_DATA(check);
            for (c1 = 0; c1 < 10; c1++) {
                STORE_DATA(gc->MultiObject[c1]);
            }

            STORE_DATA(check);
            FileWrite(handle, (UBYTE*)gc->PeopleTypes, gc->MaxPeopleTypes * sizeof(struct BodyDef));
            STORE_DATA(check);
            FileWrite(handle, (UBYTE*)gc->AnimKeyFrames, gc->MaxKeyFrames * sizeof(GameKeyFrame));
            STORE_DATA(check);
            FileWrite(handle, (UBYTE*)gc->AnimList, gc->MaxAnimFrames * sizeof(GameKeyFrame*));
            STORE_DATA(check);
            FileWrite(handle, (UBYTE*)gc->TheElements, gc->MaxElements * sizeof(GameKeyFrameElement));
            STORE_DATA(check);
            FileWrite(handle, (UBYTE*)gc->FightCols, gc->MaxFightCols * sizeof(GameFightCol));
            STORE_DATA(check);

            convert_keyframe_to_pointer(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_pointer(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_pointer(gc->FightCols, gc->FightCols, gc->MaxFightCols);

        } else
            STORE_DATA(blank);
    }

    for (c0 = 0; c0 < next_anim_chunk; c0++) {
        if (anim_chunk[c0].MultiObject[0]) {
            struct GameKeyFrameChunk* gc;
            gc = &anim_chunk[c0];
            STORE_DATA(c0);
            STORE_DATA(gc->MaxPeopleTypes);
            STORE_DATA(gc->MaxKeyFrames);
            STORE_DATA(gc->MaxAnimFrames);
            STORE_DATA(gc->MaxFightCols);
            STORE_DATA(gc->MaxElements);
            STORE_DATA(gc->ElementCount);

            convert_keyframe_to_index(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_index(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_index(gc->FightCols, gc->FightCols, gc->MaxFightCols);

            for (c1 = 0; c1 < 10; c1++) {
                STORE_DATA(gc->MultiObject[c1]);
            }

            FileWrite(handle, (UBYTE*)gc->PeopleTypes, gc->MaxPeopleTypes * sizeof(struct BodyDef));
            FileWrite(handle, (UBYTE*)gc->AnimKeyFrames, gc->MaxKeyFrames * sizeof(GameKeyFrame));
            FileWrite(handle, (UBYTE*)gc->AnimList, gc->MaxAnimFrames * sizeof(GameKeyFrame*));
            FileWrite(handle, (UBYTE*)gc->TheElements, gc->MaxElements * sizeof(GameKeyFrameElement));
            FileWrite(handle, (UBYTE*)gc->FightCols, gc->MaxFightCols * sizeof(GameFightCol));

            convert_keyframe_to_pointer(gc->AnimKeyFrames, gc->TheElements, gc->FightCols, gc->MaxKeyFrames);
            convert_animlist_to_pointer(gc->AnimList, gc->AnimKeyFrames, gc->MaxAnimFrames);
            convert_fightcol_to_pointer(gc->FightCols, gc->FightCols, gc->MaxFightCols);

        } else
            STORE_DATA(blank);
    }
}

// Older offset-finding method: searches by simple distance to mid-point.
// uc_orig: find_best_anim_offset_old (fallen/Source/memory.cpp)
SLONG find_best_anim_offset_old(SLONG mx, SLONG my, SLONG mz, SLONG bdist)
{
    SLONG c0, dist;

    bdist = 128 - bdist;
    ASSERT(bdist > 0);

    for (c0 = 0; (unsigned)c0 < next_anim_mids; c0++) {
        SLONG dx, dy, dz;
        dx = anim_mids[c0].X - mx;
        dy = anim_mids[c0].Y - my;
        dz = anim_mids[c0].Z - mz;

        dist = Root(dx * dx + dy * dy + dz * dz);
        if (dist < bdist)
            return (c0);
    }

    if (next_anim_mids < 256) {
        anim_mids[next_anim_mids].X = mx;
        anim_mids[next_anim_mids].Y = my;
        anim_mids[next_anim_mids].Z = mz;
        next_anim_mids++;
        return (next_anim_mids - 1);
    }
    ASSERT(0);
    return 0;
}

// Find or create an anim mid-point: checks whether all elements of the given keyframe
// fit within 128 units of an existing mid; if not, creates a new one.
// uc_orig: find_best_anim_offset (fallen/Source/memory.cpp)
SLONG find_best_anim_offset(SLONG mx, SLONG my, SLONG mz, SLONG anim, struct GameKeyFrameChunk* gc)
{
    SLONG c0, dist, bdist;

    if (1) {
        for (c0 = 0; (unsigned)c0 < next_anim_mids; c0++) {
            SLONG cx, cy, cz;
            SLONG dx, dy, dz;
            SLONG ele;

            bdist = 0;
            cx = anim_mids[c0].X;
            cy = anim_mids[c0].Y;
            cz = anim_mids[c0].Z;

            for (ele = 0; ele < gc->ElementCount; ele++) {
                dx = gc->AnimKeyFrames[anim].FirstElement[ele].OffsetX + mx - cx;
                dy = gc->AnimKeyFrames[anim].FirstElement[ele].OffsetY + my - cy;
                dz = gc->AnimKeyFrames[anim].FirstElement[ele].OffsetZ + mz - cz;
                dist = Root(dx * dx + dy * dy + dz * dz);
                if (dist > bdist)
                    bdist = dist;
            }
            if (bdist < 128)
                return (c0);
        }
    } else {
        ASSERT(0);
    }

    if (next_anim_mids < 256) {
        anim_mids[next_anim_mids].X = mx;
        anim_mids[next_anim_mids].Y = my;
        anim_mids[next_anim_mids].Z = mz;
        next_anim_mids++;
        return (next_anim_mids - 1);
    }
    ASSERT(0);
    return 0;
}

// Stub: save_whole_game was present in source but had an empty body.
// uc_orig: save_whole_game (fallen/Source/memory.cpp)
void save_whole_game(CBYTE* gamename)
{
}
