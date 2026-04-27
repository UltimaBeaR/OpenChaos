// Level and prim loader — binary resource loading for the game world.
// Handles .iam map files, .prm static prim objects, .all animated prim objects,
// and .tma texture style tables.

#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "assets/formats/mapthing.h"
#include "map/pap_globals.h"
#include "map/ob.h"
#include "map/ob_globals.h"
#include "map/supermap.h"
#include "missions/eway.h"
#include "missions/memory_globals.h"
#include "buildings/building_globals.h" // textures_xy, textures_flags, building_list, etc.
#include "buildings/prim_globals.h"     // prim_names[]
#include "buildings/prim.h"             // clear_prims
#include "engine/audio/sound.h"
#include "engine/audio/sound_globals.h"
#include "assets/texture.h"               // TEXTURE_choose_set

#include "assets/formats/level_loader.h"
#include "assets/formats/level_loader_globals.h"
#include "engine/io/file.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Forward declaration for load_anim_system (in anim_loader.cpp, not yet migrated).
SLONG load_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG type);

// uc_orig: PRIM_DIR (fallen/Source/io.cpp)
CBYTE PRIM_DIR[100] = "server/prims";
// uc_orig: DATA_DIR (fallen/Source/io.cpp)
CBYTE DATA_DIR[100] = "";
// uc_orig: TEXTURE_WORLD_DIR (fallen/Source/io.cpp)
CBYTE TEXTURE_WORLD_DIR[100] = "";

// uc_orig: record_prim_status (fallen/Source/io.cpp)
void record_prim_status(void)
{
    local_next_prim_point = next_prim_point;
    local_next_prim_face4 = next_prim_face4;
    local_next_prim_face3 = next_prim_face3;
    local_next_prim_object = next_prim_object;
    local_next_prim_multi_object = next_prim_multi_object;
}

// Replace the extension in 'name' (up to the first '.') with 'add' (3-char string).
// Writes the result into 'new_name'. If no '.' found, appends extension.
// uc_orig: change_extension (fallen/Source/io.cpp)
void change_extension(CBYTE* name, CBYTE* add, CBYTE* new_name)
{
    SLONG c0 = 0;
    while (name[c0]) {
        new_name[c0] = name[c0];
        if (name[c0] == '.') {
            new_name[c0 + 1] = add[0];
            new_name[c0 + 2] = add[1];
            new_name[c0 + 3] = add[2];
            new_name[c0 + 4] = 0;
            return;
        }
        c0++;
    }
    new_name[c0] = '.';
    new_name[c0 + 1] = add[0];
    new_name[c0 + 2] = add[1];
    new_name[c0 + 3] = add[2];
    new_name[c0 + 4] = 0;
}

// Load instyle.tma: binary table of interior tile UV indices for building interiors.
// Format: [4] save_type, [2] count_x, [2] count_y, [count_x*count_y] inside_tex[][], [name block skipped].
// uc_orig: load_texture_instyles (fallen/Source/io.cpp)
void load_texture_instyles(UBYTE editor, UBYTE world)
{
    UWORD temp, temp2;
    SLONG save_type = 1;
    MFFileHandle handle = FILE_OPEN_ERROR;
    CBYTE fname[MAX_PATH];

    sprintf(fname, "%sinstyle.tma", TEXTURE_WORLD_DIR);

    handle = FileOpen(fname);

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&save_type, 4);

        FileRead(handle, (UBYTE*)&temp, 2);
        FileRead(handle, (UBYTE*)&temp2, 2);
        FileRead(handle, (UBYTE*)&inside_tex[0][0], sizeof(UBYTE) * temp * temp2);
        FileRead(handle, (UBYTE*)&temp, 2);
        FileRead(handle, (UBYTE*)&temp2, 2);

        if (editor) {
        } else
            FileSeek(handle, SEEK_MODE_CURRENT, temp * temp2);

        FileClose(handle);
    }
}

// Load style.tma: exterior city tile texture UV + polygon-type flag table.
// 200 styles x 5 slots; each slot has a TXTY (atlas page + UV) and a flags byte.
// uc_orig: load_texture_styles (fallen/Source/io.cpp)
void load_texture_styles(UBYTE editor, UBYTE world)
{
    UWORD temp, temp2;
    SLONG save_type = 1;
    MFFileHandle handle = FILE_OPEN_ERROR;
    CBYTE fname[MAX_PATH];

    sprintf(fname, "%sstyle.tma", TEXTURE_WORLD_DIR);

    handle = FileOpen(fname);

    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&save_type, 4);
        if (save_type > 1) {

            if (save_type < 4) {
                FileRead(handle, (UBYTE*)&temp, 2);
                FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct TextureInfo) * 8 * 8 * temp);
            }

            FileRead(handle, (UBYTE*)&temp, 2);
            FileRead(handle, (UBYTE*)&temp2, 2);
            ASSERT(temp == 200);
            if (save_type < 5) {
                SLONG c0;
                ASSERT(temp2 == 8);
                for (c0 = 0; c0 < temp; c0++) {
                    FileSeek(handle, SEEK_MODE_CURRENT, sizeof(struct TXTY) * 3);
                    FileRead(handle, (UBYTE*)&textures_xy[c0][0], sizeof(struct TXTY) * (temp2 - 3));
                }
            } else {
                ASSERT(temp2 == 5);
                FileRead(handle, (UBYTE*)&textures_xy[0][0], sizeof(struct TXTY) * temp * temp2);
            }

            FileRead(handle, (UBYTE*)&temp, 2);
            FileRead(handle, (UBYTE*)&temp2, 2);
            ASSERT(temp == 200);
            ASSERT(temp2 == 21);
            if (editor) {
            } else
                FileSeek(handle, SEEK_MODE_CURRENT, temp * temp2);

            ASSERT(save_type > 2);
            FileRead(handle, (UBYTE*)&temp, 2);
            FileRead(handle, (UBYTE*)&temp2, 2);
            if (save_type < 5) {
                SLONG c0;
                for (c0 = 0; c0 < temp; c0++) {
                    ASSERT(temp2 == 8);
                    FileSeek(handle, SEEK_MODE_CURRENT, 3);
                    FileRead(handle, (UBYTE*)&textures_flags[c0][0], sizeof(UBYTE) * (temp2 - 3));
                }

            } else {
                if (temp2 != 5) {
                    SLONG c0, c1;
                    FileClose(handle);
                    for (c0 = 0; c0 < temp; c0++)
                        for (c1 = 0; c1 < 5; c1++) {
                            textures_flags[c0][c1] = POLY_GT;
                        }
                    return;
                }
                FileRead(handle, (UBYTE*)&textures_flags[0][0], sizeof(UBYTE) * temp * temp2);
            }
        }
        FileClose(handle);
    }
}

// Load animNNN.all for one animated world object slot if not already loaded.
// 'prim' indexes anim_chunk[]; animated prims are placed entities like balrog, bane, bats.
// uc_orig: load_anim_prim_object (fallen/Source/io.cpp)
SLONG load_anim_prim_object(SLONG prim)
{
    CBYTE fname[130];
    ASSERT(WITHIN(prim, 0, 255));

    if (anim_chunk[prim].MultiObject[0])
        return (0);

    sprintf(fname, "anim%03d.all", prim);

    if (prim >= next_anim_chunk)
        next_anim_chunk = prim + 1;

    return (load_anim_system(&anim_chunk[prim], fname, 0));
}

// Post-map-load: scan all Things and load anim prims for CLASS_ANIM_PRIM entries.
// Also loads balrog (3) and bane (4) if their per-level flags are set.
// uc_orig: load_needed_anim_prims (fallen/Source/io.cpp)
static void load_needed_anim_prims()
{
    SLONG c0;

    for (c0 = 1; c0 < MAX_PRIMARY_THINGS; c0++) {
        if (TO_THING(c0)->Class == CLASS_ANIM_PRIM) {
            load_anim_prim_object(TO_THING(c0)->Index);
        }
    }

    extern UBYTE this_level_has_the_balrog;
    extern UBYTE this_level_has_bane;

    if (this_level_has_the_balrog) {
        load_anim_prim_object(3);
    }

    if (this_level_has_bane) {
        load_anim_prim_object(4);
    }
}

// Load anim prims demanded by EWAY waypoints (EWAY_DO_CREATE_ANIMAL events in the level script).
// uc_orig: load_level_anim_prims (fallen/Source/io.cpp)
void load_level_anim_prims(void)
{
    SLONG i;
    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        if (ew->ed.type == EWAY_DO_CREATE_ANIMAL) {
            SLONG anim;
            anim = 0;
            switch (ew->ed.subtype) {

            case EWAY_SUBTYPE_ANIMAL_BAT:
                anim = 1;
                break;
            case EWAY_SUBTYPE_ANIMAL_GARGOYLE:
                anim = 2;
                break;
            case EWAY_SUBTYPE_ANIMAL_BALROG:
                anim = 3;
                break;
            case EWAY_SUBTYPE_ANIMAL_BANE:
                anim = 4;
                break;
            }
            if (anim_chunk[anim].AnimList == 0)
                load_anim_prim_object(anim);
        }
    }
}

// Load the .iam map file: PAP_Hi grid, anim prim Thing placements, OB objects, super map.
// After loading: loads static prims via OB_load_needed_prims() and anim prims.
// texture_set (save_type >= 20) selects which city texture palette (0=city, 1=forest, 5=snow).
// uc_orig: load_game_map (fallen/Source/io.cpp)
void load_game_map(CBYTE* name)
{
    UWORD i;
    UWORD temp;
    SLONG save_type = 1, ob_size;
    SWORD x, z;
    SWORD c0;
    struct MapThingPSX* t_mthing;

    CBYTE fname[100];

    sprintf(fname, "%s%s", DATA_DIR, name);

    MFFileHandle handle = FILE_OPEN_ERROR;
    handle = FileOpen(fname);
    if (handle != FILE_OPEN_ERROR) {
        FileRead(handle, (UBYTE*)&save_type, 4);

        if (save_type > 23) {
            FileRead(handle, (UBYTE*)&ob_size, 4);
        }

        FileRead(handle, (UBYTE*)&PAP_2HI(0, 0), sizeof(PAP_Hi) * PAP_SIZE_HI * PAP_SIZE_HI);

        extern UWORD WARE_roof_tex[PAP_SIZE_HI][PAP_SIZE_HI];

        memset((UBYTE*)WARE_roof_tex, 0, sizeof(UWORD) * PAP_SIZE_HI * PAP_SIZE_HI);

        for (x = 0; x < PAP_SIZE_LO; x++)
            for (z = 0; z < PAP_SIZE_LO; z++) {
                PAP_2LO(x, z).MapWho = 0;
            }

        if (save_type == 18) {
            FileRead(handle, (UBYTE*)&temp, sizeof(temp));
            for (c0 = 0; c0 < temp; c0++) {
                struct MapThingPSX map_thing;
                t_mthing = &map_thing;
                FileRead(handle, (UBYTE*)&map_thing, sizeof(struct MapThingPSX));

                switch (t_mthing->Type) {
                case MAP_THING_TYPE_ANIM_PRIM:
                    extern void create_anim_prim(SLONG x, SLONG y, SLONG z, SLONG prim, SLONG yaw);

                    load_anim_prim_object(t_mthing->IndexOther);
                    create_anim_prim(t_mthing->X, t_mthing->Y, t_mthing->Z, t_mthing->IndexOther, t_mthing->AngleY);
                    break;
                }
            }
        } else if (save_type > 18) {
            FileRead(handle, (UBYTE*)&temp, sizeof(temp));
            for (c0 = 0; c0 < temp; c0++) {
                struct LoadGameThing map_thing;
                FileRead(handle, (UBYTE*)&map_thing, sizeof(struct LoadGameThing));

                switch (map_thing.Type) {
                case MAP_THING_TYPE_ANIM_PRIM:
                    extern void create_anim_prim(SLONG x, SLONG y, SLONG z, SLONG prim, SLONG yaw);

                    load_anim_prim_object(map_thing.IndexOther);
                    create_anim_prim(map_thing.X, map_thing.Y, map_thing.Z, map_thing.IndexOther, map_thing.AngleY);
                    break;
                }
            }
        }

        // OB data (lampposts, bins, etc.) is embedded in save_type < 23 maps only.
        // Newer maps build OB data from the super_map instead.
        if (save_type < 23) {
            FileRead(handle, (UBYTE*)&OB_ob_upto, sizeof(OB_ob_upto));
            FileRead(handle, (UBYTE*)&OB_ob[0], sizeof(OB_Ob) * OB_ob_upto);
            FileRead(handle, (UBYTE*)&OB_mapwho[0][0], sizeof(OB_Mapwho) * OB_SIZE * OB_SIZE);
        }

        for (i = 1; i < OB_ob_upto; i++) {
            OB_ob[i].flags &= ~OB_FLAG_DAMAGED;
            OB_ob[i].flags &= ~OB_FLAG_RESERVED1;
            OB_ob[i].flags &= ~OB_FLAG_RESERVED2;
        }

        load_super_map(handle, save_type);

        OB_load_needed_prims();
        load_needed_anim_prims();

        if (save_type >= 20) {
            SLONG texture_set;

            FileRead(handle, (UBYTE*)&texture_set, sizeof(texture_set));

            ASSERT(WITHIN(texture_set, 0, 21));

            TEXTURE_choose_set(texture_set);
            TEXTURE_SET = texture_set;
            world_type = 0;
            if (texture_set == 5) {
                world_type = WORLD_TYPE_SNOW;
            } else if (texture_set == 1) {
                world_type = WORLD_TYPE_FOREST;
            }
        } else {
            TEXTURE_choose_set(1);
        }
        if (save_type >= 25) {
            FileRead(handle, (UBYTE*)psx_textures_xy, 2 * 200 * 5);
        }

        FileClose(handle);
    }
}

// Load a single static prim object (.prm file) by prim ID (1-265) into the global prim database.
// Supports new format (nprimNNN.prm, file_type=1) and old format (primNNN.prm, file_type=0).
// Adjusts face point indices from file-relative to global-array offsets after loading.
// uc_orig: load_prim_object (fallen/Source/io.cpp)
SLONG load_prim_object(SLONG prim)
{
    SLONG i;
    SLONG j;

    SLONG num_points;
    SLONG num_faces3;
    SLONG num_faces4;

    PrimObject* po;
    PrimFace3* f3;
    PrimFace4* f4;

    CBYTE fname[156];
    UWORD save_type;
    UWORD file_type = 1;

    MFFileHandle handle;

    ASSERT(WITHIN(prim, 0, 265));

    po = &prim_objects[prim];

    if (po->StartPoint || prim == 238) {
        return UC_TRUE;
    }

    sprintf(fname, "%s/nprim%03d.prm", PRIM_DIR, prim);

    handle = FileOpen(fname);
    if (handle == FILE_OPEN_ERROR) {

        sprintf(fname, "%s/prim%03d.prm", PRIM_DIR, prim);

        handle = FileOpen(fname);
        if (handle == FILE_OPEN_ERROR) {
            po->StartPoint = 0;
            po->EndPoint = 0;
            po->StartFace3 = 0;
            po->EndFace3 = 0;
            po->StartFace4 = 0;
            po->EndFace4 = 0;
            return UC_FALSE;
        }
        file_type = 0;
    }

    if (file_type == 1) {
        FileRead(handle, &save_type, sizeof(save_type));
        FileRead(handle, &prim_names[prim], 32);
        if (FileRead(handle, &prim_objects[prim], sizeof(PrimObject)) != sizeof(PrimObject))
            goto file_error;
    } else {
        struct PrimObjectOld oldprim;
        if (FileRead(handle, &oldprim, sizeof(PrimObjectOld)) != sizeof(PrimObjectOld))
            goto file_error;
        prim_objects[prim].coltype = oldprim.coltype;
        prim_objects[prim].damage = oldprim.damage;
        prim_objects[prim].EndFace3 = oldprim.EndFace3;
        prim_objects[prim].EndFace4 = oldprim.EndFace4;
        prim_objects[prim].StartFace3 = oldprim.StartFace3;
        prim_objects[prim].StartFace4 = oldprim.StartFace4;
        prim_objects[prim].EndPoint = oldprim.EndPoint;
        prim_objects[prim].StartPoint = oldprim.StartPoint;
        prim_objects[prim].shadowtype = oldprim.shadowtype;
        prim_objects[prim].flag = oldprim.flag;
        memcpy(prim_names[prim], oldprim.ObjectName, 32);
        save_type = oldprim.Dummy[3];
    }

    num_points = po->EndPoint - po->StartPoint;
    num_faces3 = po->EndFace3 - po->StartFace3;
    num_faces4 = po->EndFace4 - po->StartFace4;
    ASSERT(num_points >= 0);
    ASSERT(num_faces3 >= 0);
    ASSERT(num_faces4 >= 0);

    if (next_prim_point + num_points > MAX_PRIM_POINTS || next_prim_face3 + num_faces3 > MAX_PRIM_FACES3 || next_prim_face4 + num_faces4 > MAX_PRIM_FACES4) {
        FileClose(handle);

        po->StartPoint = po->EndPoint = 0;
        po->StartFace3 = po->EndFace3 = 0;
        po->StartFace4 = po->EndFace4 = 0;
        ASSERT(0);

        return UC_FALSE;
    }

    if (save_type - PRIM_START_SAVE_TYPE == 1) {
        if (FileRead(handle, &prim_points[next_prim_point], sizeof(PrimPoint) * num_points) != sizeof(PrimPoint) * num_points)
            goto file_error;
    } else {
        SLONG c0;
        struct OldPrimPoint pp;
        for (c0 = 0; c0 < num_points; c0++) {
            if (FileRead(handle, &pp, sizeof(OldPrimPoint)) != sizeof(OldPrimPoint))
                goto file_error;

            prim_points[c0 + next_prim_point].X = (SWORD)pp.X;
            prim_points[c0 + next_prim_point].Y = (SWORD)pp.Y;
            prim_points[c0 + next_prim_point].Z = (SWORD)pp.Z;
        }
    }
    if (FileRead(handle, &prim_faces3[next_prim_face3], sizeof(PrimFace3) * num_faces3) != sizeof(PrimFace3) * num_faces3)
        goto file_error;
    if (FileRead(handle, &prim_faces4[next_prim_face4], sizeof(PrimFace4) * num_faces4) != sizeof(PrimFace4) * num_faces4)
        goto file_error;

    FileClose(handle);

    // Rebase face point indices from file-relative to global prim_points[] offset.
    for (i = 0; i < num_faces3; i++) {
        f3 = &prim_faces3[next_prim_face3 + i];

        for (j = 0; j < 3; j++) {
            f3->Points[j] += next_prim_point - po->StartPoint;
        }
    }

    for (i = 0; i < num_faces4; i++) {
        f4 = &prim_faces4[next_prim_face4 + i];

        for (j = 0; j < 4; j++) {
            f4->Points[j] += next_prim_point - po->StartPoint;
        }
    }

    po->StartPoint = next_prim_point;
    po->StartFace3 = next_prim_face3;
    po->StartFace4 = next_prim_face4;

    po->EndPoint = po->StartPoint + num_points;
    po->EndFace3 = po->StartFace3 + num_faces3;
    po->EndFace4 = po->StartFace4 + num_faces4;

    next_prim_point += num_points;
    next_prim_face3 += num_faces3;
    next_prim_face4 += num_faces4;

    // Hard-code PRIM_FLAG_ITEM for collectible item prims (health, gun, key).
    switch (prim) {
    case PRIM_OBJ_ITEM_HEALTH:
    case PRIM_OBJ_ITEM_GUN:
    case PRIM_OBJ_ITEM_KEY:
        po->flag |= PRIM_FLAG_ITEM;
        break;
    }

    return UC_TRUE;

file_error:;
    FileClose(handle);
    return UC_FALSE;
}

