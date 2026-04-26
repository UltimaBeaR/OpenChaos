#include "engine/platform/uc_common.h"              // MFFileHandle, UBYTE etc.
#include "game/game_types.h"   // Game struct, ENGINE_palette macro
#include "engine/animation/anim_types.h"  // KeyFrameChunk, GameKeyFrameChunk, KeyFrameElement, etc.
#include "buildings/prim_types.h"  // PrimObject, PrimFace3/4, PrimPoint, PRIM_OBJ_*
#include "buildings/prim_globals.h" // prim_names[]
#include "map/level_pools.h"

#include "assets/formats/anim_loader.h"
#include "assets/formats/anim_loader_globals.h"
#include "assets/formats/anim_globals.h"           // the_elements, game_chunk, anim_chunk
#include "assets/formats/anim.h"                   // free_game_chunk
#include "assets/formats/level_loader.h"           // change_extension, DATA_DIR, read_object_name
#include "assets/formats/level_loader_globals.h"
#include "map/supermap_globals.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

// These functions are defined in memory.cpp (not yet migrated).
extern void convert_keyframe_to_pointer(GameKeyFrame* p, GameKeyFrameElement* p_ele, GameFightCol* p_fight, SLONG count);
extern void convert_animlist_to_pointer(GameKeyFrame** p, GameKeyFrame* p_anim, SLONG count);
extern void convert_fightcol_to_pointer(GameFightCol* p, GameFightCol* p_fight, SLONG count);
extern CBYTE* body_part_names[];
extern void free_game_chunk(GameKeyFrameChunk* the_chunk);

// uc_orig: read_a_prim (fallen/Source/io.cpp)
void read_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;
    SLONG dp;

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&prim_names[next_prim_object], 32);

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        ASSERT(ep - sp >= 0 && ep - sp < 10000);

        dp = next_prim_point - sp;

        // Read vertex data: old format uses 32-bit SLONG coords; newer uses 16-bit SWORD.
        if (save_type <= 3) {
            struct OldPrimPoint {
                SLONG X, Y, Z;
            };
            for (c0 = sp; c0 < ep; c0++) {
                OldPrimPoint op;
                FileRead(handle, (UBYTE*)&op, sizeof(OldPrimPoint));
                prim_points[next_prim_point].X = (SWORD)op.X;
                prim_points[next_prim_point].Y = (SWORD)op.Y;
                prim_points[next_prim_point].Z = (SWORD)op.Z;
                next_prim_point++;
            }
        } else {
            for (c0 = sp; c0 < ep; c0++) {
                FileRead(handle, (UBYTE*)&prim_points[next_prim_point], sizeof(struct PrimPoint));
                next_prim_point++;
            }
        }

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        ASSERT(ef3 - sf3 >= 0 && ef3 - sf3 < 10000);
        for (c0 = sf3; c0 < ef3; c0++) {
            SLONG c1;
            FileRead(handle, (UBYTE*)&prim_faces3[next_prim_face3], sizeof(struct PrimFace3));
            for (c1 = 0; c1 < 3; c1++)
                prim_faces3[next_prim_face3].Points[c1] += dp;
            next_prim_face3++;
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        ASSERT(ef4 - sf4 >= 0 && ef4 - sf4 < 10000);
        for (c0 = sf4; c0 < ef4; c0++) {
            SLONG c1;
            FileRead(handle, (UBYTE*)&prim_faces4[next_prim_face4], sizeof(struct PrimFace4));
            for (c1 = 0; c1 < 4; c1++)
                prim_faces4[next_prim_face4].Points[c1] += dp;
            // Animated mesh sub-objects are never walkable.
            prim_faces4[next_prim_face4].FaceFlags &= ~FACE_FLAG_WALKABLE;
            next_prim_face4++;
        }

        prim_objects[next_prim_object].StartPoint = next_prim_point - (ep - sp);
        prim_objects[next_prim_object].EndPoint = next_prim_point;
        prim_objects[next_prim_object].StartFace3 = next_prim_face3 - (ef3 - sf3);
        prim_objects[next_prim_object].EndFace3 = next_prim_face3;
        prim_objects[next_prim_object].StartFace4 = next_prim_face4 - (ef4 - sf4);
        prim_objects[next_prim_object].EndFace4 = next_prim_face4;
        next_prim_object++;
    }
}

// uc_orig: load_palette (fallen/Source/io.cpp)
void load_palette(CBYTE* palette)
{
    FILE* handle;

    handle = MF_Fopen(palette, "rb");

    if (handle == NULL) {
        goto file_error;
    }

    if (fread(ENGINE_palette, 1, sizeof(ENGINE_palette), handle) != sizeof(ENGINE_palette)) {
        MF_Fclose(handle);
        goto file_error;
    }

    MF_Fclose(handle);

    return;

file_error:;

    SLONG i;

    for (i = 0; i < 256; i++) {
        ENGINE_palette[i].red = rand();
        ENGINE_palette[i].green = rand();
        ENGINE_palette[i].blue = rand();
    }
}

// uc_orig: load_insert_game_chunk (fallen/Source/io.cpp)
SLONG load_insert_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk)
{
    SLONG save_type = 0, c0;
    ULONG addr1, addr2;  // original 32-bit addresses read from file
    ULONG addr3;
    UWORD check;

    struct GameKeyFrameElementBig* temp_mem;

    FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

    ASSERT(save_type > 1);

    FileRead(handle, (UBYTE*)&p_chunk->ElementCount, sizeof(p_chunk->ElementCount));

    FileRead(handle, (UBYTE*)&p_chunk->MaxPeopleTypes, sizeof(p_chunk->MaxPeopleTypes));
    FileRead(handle, (UBYTE*)&p_chunk->MaxAnimFrames, sizeof(p_chunk->MaxAnimFrames));
    FileRead(handle, (UBYTE*)&p_chunk->MaxElements, sizeof(p_chunk->MaxElements));
    FileRead(handle, (UBYTE*)&p_chunk->MaxKeyFrames, sizeof(p_chunk->MaxKeyFrames));
    FileRead(handle, (UBYTE*)&p_chunk->MaxFightCols, sizeof(p_chunk->MaxFightCols));

    if (p_chunk->MaxPeopleTypes)
        p_chunk->PeopleTypes = (struct BodyDef*)MemAlloc(MAX_PEOPLE_TYPES * sizeof(struct BodyDef));

    if (p_chunk->MaxKeyFrames)
        p_chunk->AnimKeyFrames = (struct GameKeyFrame*)MemAlloc((p_chunk->MaxKeyFrames + 500) * sizeof(struct GameKeyFrame));

    if (p_chunk->MaxAnimFrames)
        p_chunk->AnimList = (struct GameKeyFrame**)MemAlloc((p_chunk->MaxAnimFrames + 200) * sizeof(struct GameKeyFrame*));

    if (p_chunk->MaxElements)
        p_chunk->TheElements = (struct GameKeyFrameElement*)MemAlloc((p_chunk->MaxElements + 500 * 15) * sizeof(struct GameKeyFrameElement));

    temp_mem = (struct GameKeyFrameElementBig*)MemAlloc(p_chunk->MaxElements * sizeof(struct GameKeyFrameElementBig));

    if (p_chunk->MaxFightCols)
        p_chunk->FightCols = (struct GameFightCol*)MemAlloc((p_chunk->MaxFightCols + 50) * sizeof(struct GameFightCol));

    // Load the people types
    FileRead(handle, (UBYTE*)&p_chunk->PeopleTypes[0], sizeof(struct BodyDef) * p_chunk->MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the keyframe linked lists — read fixed-size disk format, then expand to runtime struct.
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));
    {
        GameKeyFrame_Disk* disk_kf = (GameKeyFrame_Disk*)MemAlloc(sizeof(GameKeyFrame_Disk) * p_chunk->MaxKeyFrames);
        FileRead(handle, (UBYTE*)disk_kf, sizeof(GameKeyFrame_Disk) * p_chunk->MaxKeyFrames);
        for (SLONG i = 0; i < p_chunk->MaxKeyFrames; i++) {
            p_chunk->AnimKeyFrames[i].XYZIndex     = disk_kf[i].XYZIndex;
            p_chunk->AnimKeyFrames[i].TweenStep    = disk_kf[i].TweenStep;
            p_chunk->AnimKeyFrames[i].Flags         = disk_kf[i].Flags;
            p_chunk->AnimKeyFrames[i].FirstElement  = (GameKeyFrameElement*)(uintptr_t)disk_kf[i].FirstElement;
            p_chunk->AnimKeyFrames[i].PrevFrame     = (GameKeyFrame*)(uintptr_t)disk_kf[i].PrevFrame;
            p_chunk->AnimKeyFrames[i].NextFrame     = (GameKeyFrame*)(uintptr_t)disk_kf[i].NextFrame;
            p_chunk->AnimKeyFrames[i].Fight         = (GameFightCol*)(uintptr_t)disk_kf[i].Fight;
        }
        MemFree(disk_kf);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the anim elements
    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[0], sizeof(struct GameKeyFrameElement) * p_chunk->MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Load the anim list — file stores 32-bit pointer values, expand to pointer-sized entries.
    {
        uint32_t* disk_list = (uint32_t*)MemAlloc(sizeof(uint32_t) * p_chunk->MaxAnimFrames);
        FileRead(handle, (UBYTE*)disk_list, sizeof(uint32_t) * p_chunk->MaxAnimFrames);
        for (SLONG i = 0; i < p_chunk->MaxAnimFrames; i++) {
            p_chunk->AnimList[i] = (GameKeyFrame*)(uintptr_t)disk_list[i];
        }
        MemFree(disk_list);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    // Load fight cols — read fixed-size disk format, then expand.
    {
        GameFightCol_Disk* disk_fc = (GameFightCol_Disk*)MemAlloc(sizeof(GameFightCol_Disk) * p_chunk->MaxFightCols);
        FileRead(handle, (UBYTE*)disk_fc, sizeof(GameFightCol_Disk) * p_chunk->MaxFightCols);
        for (SLONG i = 0; i < p_chunk->MaxFightCols; i++) {
            p_chunk->FightCols[i].Dist1        = disk_fc[i].Dist1;
            p_chunk->FightCols[i].Dist2        = disk_fc[i].Dist2;
            p_chunk->FightCols[i].Angle        = disk_fc[i].Angle;
            p_chunk->FightCols[i].Priority     = disk_fc[i].Priority;
            p_chunk->FightCols[i].Damage       = disk_fc[i].Damage;
            p_chunk->FightCols[i].Tween        = disk_fc[i].Tween;
            p_chunk->FightCols[i].AngleHitFrom = disk_fc[i].AngleHitFrom;
            p_chunk->FightCols[i].Height       = disk_fc[i].Height;
            p_chunk->FightCols[i].Width        = disk_fc[i].Width;
            p_chunk->FightCols[i].Dummy        = disk_fc[i].Dummy;
            p_chunk->FightCols[i].Next         = (GameFightCol*)(uintptr_t)disk_fc[i].Next;
        }
        MemFree(disk_fc);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type < 3) {
        // Convert old tween step counts to step sizes.
        for (c0 = 0; c0 < p_chunk->MaxKeyFrames; c0++) {
            p_chunk->AnimKeyFrames[c0].TweenStep = (256 / (p_chunk->AnimKeyFrames[c0].TweenStep + 1)) >> 1;
        }
    }

    if (save_type > 4) {
        // Newer format: pointers stored as index offsets, use helpers to fix them up.
        convert_keyframe_to_pointer(p_chunk->AnimKeyFrames, p_chunk->TheElements, p_chunk->FightCols, p_chunk->MaxKeyFrames);
        convert_animlist_to_pointer(p_chunk->AnimList, p_chunk->AnimKeyFrames, p_chunk->MaxAnimFrames);
        convert_fightcol_to_pointer(p_chunk->FightCols, p_chunk->FightCols, p_chunk->MaxFightCols);
    } else {
        // Older format: stored pointers are original 32-bit runtime addresses.
        // Relocate by converting old address → element index → new pointer.
        // Cannot use byte-offset relocation because sizeof changed on x64:
        //   GameKeyFrame: 20 (x86) → 36 (x64), GameFightCol: 16 → 20.
        for (c0 = 0; c0 < p_chunk->MaxKeyFrames; c0++) {
            uintptr_t old_ptr;

            // NextFrame
            old_ptr = (uintptr_t)p_chunk->AnimKeyFrames[c0].NextFrame;
            if (old_ptr) {
                uintptr_t idx = (old_ptr - addr1) / sizeof(GameKeyFrame_Disk);
                p_chunk->AnimKeyFrames[c0].NextFrame = &p_chunk->AnimKeyFrames[idx];
            }

            // PrevFrame
            old_ptr = (uintptr_t)p_chunk->AnimKeyFrames[c0].PrevFrame;
            if (old_ptr) {
                uintptr_t idx = (old_ptr - addr1) / sizeof(GameKeyFrame_Disk);
                p_chunk->AnimKeyFrames[c0].PrevFrame = &p_chunk->AnimKeyFrames[idx];
            }

            // FirstElement (sizeof unchanged — no pointers in GameKeyFrameElement)
            old_ptr = (uintptr_t)p_chunk->AnimKeyFrames[c0].FirstElement;
            if (old_ptr) {
                uintptr_t idx = (old_ptr - addr2) / sizeof(GameKeyFrameElement);
                p_chunk->AnimKeyFrames[c0].FirstElement = &p_chunk->TheElements[idx];
            }

            // Fight
            old_ptr = (uintptr_t)p_chunk->AnimKeyFrames[c0].Fight;
            if (old_ptr && old_ptr >= addr3) {
                uintptr_t idx = (old_ptr - addr3) / sizeof(GameFightCol_Disk);
                if (idx < (uintptr_t)p_chunk->MaxFightCols) {
                    p_chunk->AnimKeyFrames[c0].Fight = &p_chunk->FightCols[idx];
                } else {
                    p_chunk->AnimKeyFrames[c0].Fight = 0;
                }
            } else {
                p_chunk->AnimKeyFrames[c0].Fight = 0;
            }
        }

        // Relocate FightCol->Next chain
        for (c0 = 0; c0 < p_chunk->MaxFightCols; c0++) {
            uintptr_t old_ptr = (uintptr_t)p_chunk->FightCols[c0].Next;
            if (old_ptr) {
                uintptr_t idx = (old_ptr - addr3) / sizeof(GameFightCol_Disk);
                if (idx < (uintptr_t)p_chunk->MaxFightCols) {
                    p_chunk->FightCols[c0].Next = &p_chunk->FightCols[idx];
                } else {
                    p_chunk->FightCols[c0].Next = 0;
                }
            }
        }

        // AnimList: old 32-bit keyframe pointers → index → new pointer
        for (c0 = 0; c0 < p_chunk->MaxAnimFrames; c0++) {
            uintptr_t old_ptr = (uintptr_t)p_chunk->AnimList[c0];
            if (old_ptr) {
                uintptr_t idx = (old_ptr - addr1) / sizeof(GameKeyFrame_Disk);
                p_chunk->AnimList[c0] = &p_chunk->AnimKeyFrames[idx];
            }
        }
    }

    MemFree((void*)temp_mem);
    return (1);
}

// uc_orig: load_insert_a_multi_prim (fallen/Source/io.cpp)
SLONG load_insert_a_multi_prim(MFFileHandle handle)
{
    SLONG c0;
    SLONG save_type = 0;
    SLONG so, eo;

    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        FileRead(handle, (UBYTE*)&so, sizeof(so));
        FileRead(handle, (UBYTE*)&eo, sizeof(eo));

        prim_multi_objects[next_prim_multi_object].StartObject = next_prim_object;
        prim_multi_objects[next_prim_multi_object].EndObject = next_prim_object + (eo - so);
        for (c0 = so; c0 < eo; c0++)
            read_a_prim(c0, handle, save_type);

        next_prim_multi_object++;
        return (next_prim_multi_object - 1);
    } else
        return (0);
}

// uc_orig: load_anim_system (fallen/Source/io.cpp)
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type)
{
    SLONG c0;
    MFFileHandle handle;
    SLONG save_type = 0;
    CBYTE ext_name[100];
    SLONG count;
    UBYTE load;

    CBYTE fname[100];

    sprintf(fname, "%sdata/%s", DATA_DIR, name);

    free_game_chunk(p_chunk);

    change_extension(fname, "all", ext_name);
    handle = FileOpen(ext_name);
    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        if (save_type > 2) {

            FileRead(handle, (UBYTE*)&count, sizeof(count));

            for (c0 = 0; c0 < count; c0++) {
                switch (type) {
                case 0:
                    p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    break;
                case 1:
                    // people.all: always load Darci (slot 0), conditionally load others.
                    switch (c0) {
                    case 0: // darci
                        load = 1;
                        break;
                    case 1: // slag
                        load = !(DONT_load & (1 << PERSON_SLAG_TART));
                        break;
                    case 2: // fat slag
                        load = !(DONT_load & (1 << PERSON_SLAG_FATUGLY));
                        break;
                    case 3: // hostage
                        load = !(DONT_load & (1 << PERSON_HOSTAGE));
                        break;
                    default:
                        load = 1;
                        break;
                    }
                    if (load)
                        p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    else {
                        skip_load_a_multi_prim(handle);
                        p_chunk->MultiObject[c0] = 1;
                    }
                    break;
                case 2:
                    // thug.all: slots 0=rasta 1=grey 2=red 3=miller 4=cop 5=mib 6=tramp 7=civ variety
                    switch (c0) {
                    case 0: // rasta
                    case 1: // grey
                    case 2: // red
                    case 4: // cop
                    case 7: // civ variety
                        load = 1;
                        break;
                    case 3: // miller (mechanic)
                        load = !(DONT_load & (1 << PERSON_MECHANIC));
                        break;
                    case 5: // MIB
                        load = !(DONT_load & (1 << PERSON_MIB1));
                        break;
                    case 6: // tramp
                        load = !(DONT_load & (1 << PERSON_TRAMP));
                        break;
                    default:
                        load = 1;
                        break;
                    }
                    if (load)
                        p_chunk->MultiObject[c0] = load_insert_a_multi_prim(handle);
                    else {
                        skip_load_a_multi_prim(handle);
                        p_chunk->MultiObject[c0] = 1;
                    }
                    break;
                }
            }
            p_chunk->MultiObject[c0] = 0;
        } else {
            p_chunk->MultiObject[0] = load_insert_a_multi_prim(handle);
            p_chunk->MultiObject[1] = 0;
        }
        load_insert_game_chunk(handle, p_chunk);

        FileClose(handle);
        return (1);
    }
    return (0);
}

// uc_orig: load_append_game_chunk (fallen/Source/io.cpp)
SLONG load_append_game_chunk(MFFileHandle handle, struct GameKeyFrameChunk* p_chunk, SLONG start_frame)
{
    SLONG save_type = 0;
    ULONG addr1, addr2;
    ULONG addr3;
    UWORD check;

    SLONG ElementCount;
    SWORD MaxPeopleTypes;
    SWORD MaxKeyFrames;
    SWORD MaxAnimFrames;
    SWORD MaxFightCols;
    SLONG MaxElements;

    FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

    ASSERT(save_type > 1);

    FileRead(handle, (UBYTE*)&ElementCount, sizeof(p_chunk->ElementCount));
    FileRead(handle, (UBYTE*)&MaxPeopleTypes, sizeof(p_chunk->MaxPeopleTypes));
    FileRead(handle, (UBYTE*)&MaxAnimFrames, sizeof(p_chunk->MaxAnimFrames));
    FileRead(handle, (UBYTE*)&MaxElements, sizeof(p_chunk->MaxElements));
    FileRead(handle, (UBYTE*)&MaxKeyFrames, sizeof(p_chunk->MaxKeyFrames));
    FileRead(handle, (UBYTE*)&MaxFightCols, sizeof(p_chunk->MaxFightCols));

    // Skip the PeopleTypes block — already loaded when the .all was first loaded.
    FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct BodyDef) * MaxPeopleTypes);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append keyframes — read fixed-size disk format, expand to runtime struct.
    SLONG kf_start = p_chunk->MaxKeyFrames;
    FileRead(handle, (UBYTE*)&addr1, sizeof(addr1));
    {
        GameKeyFrame_Disk* disk_kf = (GameKeyFrame_Disk*)MemAlloc(sizeof(GameKeyFrame_Disk) * MaxKeyFrames);
        FileRead(handle, (UBYTE*)disk_kf, sizeof(GameKeyFrame_Disk) * MaxKeyFrames);
        for (SLONG i = 0; i < MaxKeyFrames; i++) {
            p_chunk->AnimKeyFrames[kf_start + i].XYZIndex     = disk_kf[i].XYZIndex;
            p_chunk->AnimKeyFrames[kf_start + i].TweenStep    = disk_kf[i].TweenStep;
            p_chunk->AnimKeyFrames[kf_start + i].Flags         = disk_kf[i].Flags;
            p_chunk->AnimKeyFrames[kf_start + i].FirstElement  = (GameKeyFrameElement*)(uintptr_t)disk_kf[i].FirstElement;
            p_chunk->AnimKeyFrames[kf_start + i].PrevFrame     = (GameKeyFrame*)(uintptr_t)disk_kf[i].PrevFrame;
            p_chunk->AnimKeyFrames[kf_start + i].NextFrame     = (GameKeyFrame*)(uintptr_t)disk_kf[i].NextFrame;
            p_chunk->AnimKeyFrames[kf_start + i].Fight         = (GameFightCol*)(uintptr_t)disk_kf[i].Fight;
        }
        MemFree(disk_kf);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append elements (no pointers — sizeof unchanged on x64).
    SLONG el_start = p_chunk->MaxElements;
    FileRead(handle, (UBYTE*)&addr2, sizeof(addr2));
    FileRead(handle, (UBYTE*)&p_chunk->TheElements[el_start], sizeof(struct GameKeyFrameElement) * MaxElements);
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    // Append anim list — file stores 32-bit pointer values, expand to pointer-sized entries.
    {
        uint32_t* disk_list = (uint32_t*)MemAlloc(sizeof(uint32_t) * MaxAnimFrames);
        FileRead(handle, (UBYTE*)disk_list, sizeof(uint32_t) * MaxAnimFrames);
        for (SLONG i = 0; i < MaxAnimFrames; i++) {
            p_chunk->AnimList[start_frame + i] = (GameKeyFrame*)(uintptr_t)disk_list[i];
        }
        MemFree(disk_list);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    SLONG fc_start = p_chunk->MaxFightCols;
    FileRead(handle, (UBYTE*)&addr3, sizeof(addr3));

    // Append fight cols — read fixed-size disk format, expand.
    {
        GameFightCol_Disk* disk_fc = (GameFightCol_Disk*)MemAlloc(sizeof(GameFightCol_Disk) * MaxFightCols);
        FileRead(handle, (UBYTE*)disk_fc, sizeof(GameFightCol_Disk) * MaxFightCols);
        for (SLONG i = 0; i < MaxFightCols; i++) {
            p_chunk->FightCols[fc_start + i].Dist1        = disk_fc[i].Dist1;
            p_chunk->FightCols[fc_start + i].Dist2        = disk_fc[i].Dist2;
            p_chunk->FightCols[fc_start + i].Angle        = disk_fc[i].Angle;
            p_chunk->FightCols[fc_start + i].Priority     = disk_fc[i].Priority;
            p_chunk->FightCols[fc_start + i].Damage       = disk_fc[i].Damage;
            p_chunk->FightCols[fc_start + i].Tween        = disk_fc[i].Tween;
            p_chunk->FightCols[fc_start + i].AngleHitFrom = disk_fc[i].AngleHitFrom;
            p_chunk->FightCols[fc_start + i].Height       = disk_fc[i].Height;
            p_chunk->FightCols[fc_start + i].Width        = disk_fc[i].Width;
            p_chunk->FightCols[fc_start + i].Dummy        = disk_fc[i].Dummy;
            p_chunk->FightCols[fc_start + i].Next         = (GameFightCol*)(uintptr_t)disk_fc[i].Next;
        }
        MemFree(disk_fc);
    }
    FileRead(handle, &check, 2);
    ASSERT(check == 666);

    if (save_type > 4) {
        convert_keyframe_to_pointer(&p_chunk->AnimKeyFrames[kf_start], &p_chunk->TheElements[el_start], &p_chunk->FightCols[fc_start], MaxKeyFrames);
        convert_animlist_to_pointer(&p_chunk->AnimList[start_frame], &p_chunk->AnimKeyFrames[kf_start], MaxAnimFrames);
        convert_fightcol_to_pointer(&p_chunk->FightCols[fc_start], &p_chunk->FightCols[fc_start], MaxFightCols);
    }

    p_chunk->MaxAnimFrames = start_frame + MaxAnimFrames;
    p_chunk->MaxElements += MaxElements;
    p_chunk->MaxKeyFrames += MaxKeyFrames;
    p_chunk->MaxFightCols += MaxFightCols;

    return (1);
}

// uc_orig: skip_a_prim (fallen/Source/io.cpp)
void skip_a_prim(SLONG prim, MFFileHandle handle, SLONG save_type)
{
    SLONG c0;
    SLONG sf3, ef3, sf4, ef4, sp, ep;

    if (handle != FILE_OPEN_ERROR) {
        FileSeek(handle, SEEK_MODE_CURRENT, 32); // skip object name

        FileRead(handle, (UBYTE*)&sp, sizeof(sp));
        FileRead(handle, (UBYTE*)&ep, sizeof(ep));

        ASSERT(ep - sp >= 0 && ep - sp < 10000);

        for (c0 = sp; c0 < ep; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimPoint));
        }

        FileRead(handle, (UBYTE*)&sf3, sizeof(sf3));
        FileRead(handle, (UBYTE*)&ef3, sizeof(ef3));
        ASSERT(ef3 - sf3 >= 0 && ef3 - sf3 < 10000);
        for (c0 = sf3; c0 < ef3; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace3));
        }

        FileRead(handle, (UBYTE*)&sf4, sizeof(sf4));
        FileRead(handle, (UBYTE*)&ef4, sizeof(ef4));
        ASSERT(ef4 - sf4 >= 0 && ef4 - sf4 < 10000);
        for (c0 = sf4; c0 < ef4; c0++) {
            FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct PrimFace4));
        }
    }
}

// uc_orig: skip_load_a_multi_prim (fallen/Source/io.cpp)
void skip_load_a_multi_prim(MFFileHandle handle)
{
    SLONG c0;
    SLONG save_type = 0;
    SLONG so, eo;

    if (handle != FILE_OPEN_ERROR) {

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));

        FileRead(handle, (UBYTE*)&so, sizeof(so));
        FileRead(handle, (UBYTE*)&eo, sizeof(eo));

        for (c0 = so; c0 < eo; c0++)
            skip_a_prim(c0, handle, save_type);
    }
}

// uc_orig: append_anim_system (fallen/Source/io.cpp)
SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh)
{
    SLONG c0;
    MFFileHandle handle;
    SLONG save_type = 0;
    CBYTE ext_name[100];
    SLONG count;

    CBYTE fname[100];

    sprintf(fname, "%sdata/%s", DATA_DIR, name);

    change_extension(fname, "all", ext_name);
    handle = FileOpen(ext_name);
    if (handle != FILE_OPEN_ERROR) {
        SLONG start_ob = 0;

        while (p_chunk->MultiObject[start_ob]) {
            start_ob++;
        }

        FileRead(handle, (UBYTE*)&save_type, sizeof(save_type));
        FileRead(handle, (UBYTE*)&count, sizeof(count));

        ASSERT(start_ob + count < 11);

        for (c0 = 0; c0 < count; c0++) {
            if (load_mesh)
                p_chunk->MultiObject[start_ob + c0] = load_insert_a_multi_prim(handle);
            else
                skip_load_a_multi_prim(handle);
        }
        if (c0 + start_ob < 10)
            p_chunk->MultiObject[c0 + start_ob] = 0;

        load_append_game_chunk(handle, p_chunk, start_anim);

        FileClose(handle);
        return (1);
    }
    return (0);
}
