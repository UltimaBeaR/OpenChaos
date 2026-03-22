// uc_orig: controls.cpp (fallen/Source/Controls.cpp)
// Per-frame game-controller: inventory management, music context selection,
// danger level calculation, debug console parser, and the process_controls() dispatcher.

#include <stdio.h>
#include <string.h>

#include "fallen/Headers/Game.h"       // Temporary: NET_PERSON, NET_PLAYER, GAME_STATE, GAME_TURN, game_chunk
#include "ai/pcom.h"                   // Temporary: PCOM_MOVE_WANDER
#include "assets/sound_id.h"           // Temporary: S_NULL, S_TUNE_DANGER_RED, S_TUNE_DANGER_GREEN (Waves enum)
#include "engine/audio/music.h"        // Temporary: MUSIC_mode, MUSIC_bodge_code, MUSIC_MODE_*
#include "engine/audio/sound.h"       // Temporary: MUSIC_REF
#include "engine/audio/mfx.h"          // Temporary: MFX_play_ambient, MFX_OVERLAP, MFX_load_wave_list
#include "engine/graphics/resources/console.h" // Temporary: CONSOLE_text
#include "engine/lighting/night.h"     // Temporary: NIGHT_cache_recalc, NIGHT_dfcache_recalc, NIGHT_generate_walkable_lighting
#include "engine/lighting/night_globals.h" // Temporary: NIGHT_amb_red/green/blue
#include "world/map/pap.h"             // Temporary: PAP_calc_height_at, PAP_2HI
#include "world/map/road.h"            // Temporary: ROAD_is_road
#include "missions/eway.h"             // Temporary: EWAY_Way, EWAY_Edef
#include "missions/eway_globals.h"    // Temporary: EWAY_way, EWAY_way_upto, EWAY_edef, EWAY_edef_upto, EWAY_DO_*
#include "effects/pyro.h"              // Temporary: PYRO_create, PYRO_GAMEOVER
#include "actors/characters/person.h"  // Temporary: set_person_idle, set_person_item_away, set_person_draw_item, set_person_draw_gun
#include "actors/core/thing.h"         // Temporary: move_thing_on_map, THING_NUMBER, THING_find_sphere, THING_array, THING_ARRAY_SIZE, TO_THING
#include "fallen/Headers/statedef.h"   // Temporary: SUB_STATE_CRAWLING, PERSON_MODE_*, FLAG_PERSON_*, FLAGS_HAS_GUN, FLAG_PERSON_GUN_OUT
// ANIM_TYPE_DARCI, ANIM_TYPE_ROPER come from fallen/Headers/Person.h via actors/characters/person.h
#include "assets/anim_globals.h"      // Temporary: game_chunk
#include "world/environment/ware.h"    // Temporary: WARE_Ware type
#include "world/environment/ware_globals.h" // Temporary: WARE_ware
#include "engine/graphics/pipeline/polypage.h"   // Temporary: PolyPage::EnableAlphaSort, DisableAlphaSort, AlphaSortEnabled, SetGreenScreen
#include "engine/graphics/pipeline/vertex_buffer.h"   // Temporary: TheVPool, VertexBufferPool
#include "engine/graphics/graphics_api/gd_display.h"  // Temporary: the_display, Display
#include "assets/tga.h"                          // Temporary: TGA_save, TGA_Pixel
#include "MFStdLib/Headers/StdFile.h"             // Temporary: MF_Fopen, MF_Fclose
#include "missions/memory_globals.h"             // Temporary: roof_faces4, next_roof_face4, dfacets, next_dfacet
#include "world/map/supermap.h"                  // Temporary: DFacet, RoofFace4, STOREY_TYPE_*
#include "world/map/supermap_globals.h"          // Temporary: supermap globals
#include "actors/items/special.h"                // Temporary: SPECIAL_info, SPECIAL_GROUP_*, SPECIAL_SHOTGUN, SPECIAL_AK47, SPECIAL_GRENADE, SPECIAL_BASEBALLBAT, SPECIAL_KNIFE, SPECIAL_EXPLOSIVES, SPECIAL_WIRE_CUTTER
#include "missions/elev_globals.h"               // Temporary: ELEV_fname_level, ELEV_fname_map
#include "ui/camera/fc.h"                        // Temporary: FC_force_camera_behind
#include "ui/menus/cnet_globals.h"               // Temporary: CNET_player_id, CNET_num_players (for PLAYER_ID/NO_PLAYERS macros)

#include "ui/controls.h"
#include "ui/controls_globals.h"

// Forward declarations for functions not yet migrated from old/
extern SLONG am_i_a_thug(Thing* p_person);
extern void drop_current_gun(Thing* p_person, SLONG change_anim);
extern SLONG analogue;
extern UWORD fade_black;
extern void reload_level(void);
extern SLONG plant_feet(Thing* p_person);
extern UWORD count_gang(Thing* p_target);

// tga[] is file-local, used only by tga_dump and plan_view_shot.
// uc_orig: tga (fallen/Source/Controls.cpp)
static TGA_Pixel tga[480][640];

// Widget pointers used by form_proc — declared here, visible to form_proc only.
// uc_orig: test_form (fallen/Source/Controls.cpp)
static Form* test_form;
// uc_orig: widget_text (fallen/Source/Controls.cpp)
static Widget* widget_text;
// uc_orig: widget_ok (fallen/Source/Controls.cpp)
static Widget* widget_ok;

// uc_orig: INVENTORY_FADE_SPEED (fallen/Source/Controls.cpp)
// Inventory panel fade speed in opacity units per frame.
#define INVENTORY_FADE_SPEED (32)

// Weapon scoring constants used by CONTROLS_get_best_item.
// uc_orig: AK47_SCORE (fallen/Source/Controls.cpp)
#define AK47_SCORE 6
// uc_orig: SHOTGUN_SCORE (fallen/Source/Controls.cpp)
#define SHOTGUN_SCORE 5
// uc_orig: PISTOL_SCORE (fallen/Source/Controls.cpp)
#define PISTOL_SCORE 4
// uc_orig: KNIFE_SCORE (fallen/Source/Controls.cpp)
#define KNIFE_SCORE 3
// uc_orig: BAT_SCORE (fallen/Source/Controls.cpp)
#define BAT_SCORE 2
// uc_orig: GRENADE_SCORE (fallen/Source/Controls.cpp)
#define GRENADE_SCORE 1

// uc_orig: eway_find (fallen/Source/Controls.cpp)
// Linear search for a waypoint by numeric id. Used by the "telw" console command.
EWAY_Way* eway_find(SLONG id)
{
    SLONG i;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        if (ew->id == id)
            return ew;
    }

    return NULL;
}

// uc_orig: eway_find_near (fallen/Source/Controls.cpp)
// Returns the nearest waypoint within 512 map-units of pos. Used by the "wpt" console command.
EWAY_Way* eway_find_near(GameCoord pos)
{
    ULONG i, d = 512, d2;
    SLONG dx, dy, dz, r = -1;
    EWAY_Way* ew;

    pos.X >>= 8;
    pos.Y >>= 8;
    pos.Z >>= 8;

    d *= d;

    for (i = 1; i < (unsigned)EWAY_way_upto; i++) {
        ew = &EWAY_way[i];
        dx = ew->x - pos.X;
        dy = ew->y - pos.Y;
        dz = ew->z - pos.Z;
        d2 = (dx * dx) + (dy * dy) + (dz * dz);
        if (d2 < d) {
            d = d2;
            r = i;
        }
    }

    if (r > 0)
        return &EWAY_way[r];

    return NULL;
}

// uc_orig: parse_console (fallen/Source/Controls.cpp)
// Executes a single debug-console line (entered via F9 in-game).
// Parses the first whitespace-delimited token as a command name and dispatches via cmd_list[].
void parse_console(CBYTE* str)
{
    CBYTE cmd[20];
    CBYTE *ptr = str, *pt2 = cmd;
    UWORD i;
    Thing* darci = NET_PERSON(0);

    static GameCoord stored_pos = { -1, -1, -1 };
    EWAY_Way* way;

    memset(cmd, 0, 20);
    while ((*ptr) && (*ptr != 32) && (ptr - str < 20)) {
        *pt2++ = *ptr++;
    }
    if (ptr - str == 20)
        return;
    ptr++;

    for (i = 0; cmd_list[i]; i++) {
        if (!stricmp(cmd, cmd_list[i])) {
            switch (i) {
            case 0: // cam -- nothing yet

                break;
            case 1: // echo -- for testing
                CONSOLE_text(ptr, 5000);
                break;
            case 2:
                stored_pos = NET_PERSON(0)->WorldPos;
                CONSOLE_text("stored.", 3000);
                break;
            case 3: // store & restore teleport positions
                if ((stored_pos.X == -1) && (stored_pos.Y == -1) && (stored_pos.Z == -1)) {
                    CONSOLE_text("no tel stored.", 5000);
                    break;
                }
                set_person_idle(NET_PERSON(0));
                move_thing_on_map(NET_PERSON(0), &stored_pos);
                FC_force_camera_behind(0);
                plant_feet(NET_PERSON(0));
                NET_PERSON(0)->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
                CONSOLE_text("restored.", 5000);
                break;
            case 4: // telw -- teleport to numbered waypoint
                i = atoi(ptr);
                way = eway_find(i);
                if (way) {
                    GameCoord pos = { way->x << 8, way->y << 8, way->z << 8 };
                    set_person_idle(NET_PERSON(0));
                    move_thing_on_map(NET_PERSON(0), &pos);
                    FC_force_camera_behind(0);
                    plant_feet(NET_PERSON(0));
                    NET_PERSON(0)->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
                    CONSOLE_text("z-z-zap.", 5000);
                } else
                    CONSOLE_text("wpt not found");
                break;
            case 5: // break
                ASSERT(0);
                break;
            case 6: // wpt? -- find nearest wpt
                way = eway_find_near(NET_PERSON(0)->WorldPos);
                if (way) {
                    itoa(way->id, cmd, 10);
                    CONSOLE_text(cmd);
                } else
                    CONSOLE_text("wpt not found");
                break;

            case 7: // vtx - dump vertex buffer information
            {
                FILE* fd = MF_Fopen("C:\\VertexBufferInfo.txt", "w");
                if (fd) {
                    TheVPool->DumpInfo(fd);
                    MF_Fclose(fd);
                    CONSOLE_text("Info dumped at C:\\VertexBufferInfo.txt");
                } else
                    CONSOLE_text("Can't open C:\\VertexBufferInfo.txt");
            } break;

            case 8: // alpha - set alpha sort type
                if (ptr[0] == '0') {
                    PolyPage::DisableAlphaSort();
                    CONSOLE_text("Alpha sorting OFF");
                } else if (ptr[0] == '1') {
                    PolyPage::EnableAlphaSort();
                    CONSOLE_text("Alpha sorting ON");
                } else {
                    CONSOLE_text((CBYTE*)(PolyPage::AlphaSortEnabled() ? "Alpha sorting is ON" : "Alpha sorting is OFF"));
                }
                break;

            case 9: // gamma - set gamma level
                if ((ptr[0] >= '0') && (ptr[0] <= '7')) {
                    int val = 12 * (ptr[0] - '0');
                    the_display.SetGamma(val, 256);
                } else {
                    CONSOLE_text("Gamma 0-7");
                }
                break;

            case 10: // dkeys -- debug keys en/disable
                allow_debug_keys ^= 1;
                if (allow_debug_keys)
                    CONSOLE_text("debug mode on");
                else
                    CONSOLE_text("debug mode off");

                dkeys_have_been_used = UC_TRUE;

                break;

            case 11: // cctv
                if (ptr[0] == '0')
                    PolyPage::SetGreenScreen(false);
                else
                    PolyPage::SetGreenScreen();
                break;

            case 12: // win
                if (allow_debug_keys)
                    GAME_STATE = GS_LEVEL_WON;
                break;

            case 13: // lose
                GAME_STATE = GS_LEVEL_LOST;
                break;
            case 14:
                //				SAVE_ingame("");
                //				CONSOLE_text("GAME SAVED");
                break;
            case 15:
                //				LOAD_ingame("");
                //				CONSOLE_text("GAME LOADED");
                break;
            case 16:
                if (allow_debug_keys)
                    reload_level();
                break;
            case 17:
                // in-game ambient colour editor
                {
                    CBYTE str[100];
                    SLONG r, g, b;
                    sscanf(ptr, "%d %d %d", &r, &g, &b);
                    CONSOLE_text(ptr);
                    sprintf(str, " red %d green %d blue %d \n", r, g, b);
                    CONSOLE_text(str);

                    NIGHT_amb_red = r;
                    NIGHT_amb_green = g;
                    NIGHT_amb_blue = b;
                    NIGHT_cache_recalc();
                    NIGHT_dfcache_recalc();
                    NIGHT_generate_walkable_lighting();
                }
                break;
            case 18:
                analogue ^= 1;
                break;

            case 19:
                i = atoi(ptr);
                sprintf(str, "loading music world %d...", i);
                CONSOLE_text(str);
                MUSIC_WORLD = i;
                MFX_load_wave_list();
                break;
            case 20:
                i = atoi(ptr);
                if (i == 0)
                    fade_black = 1;
                else
                    fade_black = 0;
                break;
            case 21:
                if (allow_debug_keys) {
                    darci->Genus.Person->PersonType = PERSON_ROPER;
                    darci->Genus.Person->AnimType = ANIM_TYPE_ROPER;
                    darci->Draw.Tweened->TheChunk = &game_chunk[ANIM_TYPE_ROPER];
                    darci->Draw.Tweened->MeshID = 0;
                    darci->Draw.Tweened->PersonID = 0;
                    set_person_idle(darci);
                }
                break;
            case 22:
                if (allow_debug_keys) {
                    darci->Genus.Person->PersonType = PERSON_DARCI;
                    darci->Genus.Person->AnimType = ANIM_TYPE_DARCI;
                    darci->Draw.Tweened->TheChunk = &game_chunk[ANIM_TYPE_DARCI];
                    darci->Draw.Tweened->MeshID = 0;
                    darci->Draw.Tweened->PersonID = 0;
                    set_person_idle(darci);
                }
                break;
            case 23:
                extern int AENG_detail_crinkles;
                AENG_detail_crinkles ^= 1;
                if (AENG_detail_crinkles)
                    CONSOLE_text("crinkles on");
                else
                    CONSOLE_text("crinkles off");
                break;

            case 24:
                //				if(allow_debug_keys)
                //				VIOLENCE=1;
                break;

            case 25:
                PYRO_create(darci->WorldPos, PYRO_GAMEOVER);
                break;
            }
            return;
        }
    }
    CONSOLE_text("huh?", 3000);
}

// uc_orig: tga_dump (fallen/Source/Controls.cpp)
// Captures the current frame to a TGA file. Searches for the first free shot000..9999.tga name.
void tga_dump(void)
{
    SLONG i;
    SLONG x;
    SLONG y;

    UBYTE red;
    UBYTE green;
    UBYTE blue;

    CBYTE fname[32];

    FILE* handle;

    for (i = 0; i < 10000; i++) {
        sprintf(fname, "c:\\tmp\\shot%03d.tga", i);

        handle = MF_Fopen(fname, "rb");

        if (handle) {
            MF_Fclose(handle);
        } else {
            goto found_file;
        }
    }

    return;

found_file:;

    for (y = 0; y < 480; y++) {
        for (x = 0; x < 640; x++) {
            the_display.GetPixel(
                x,
                y,
                &red,
                &green,
                &blue);

            tga[y][x].red = red;
            tga[y][x].green = green;
            tga[y][x].blue = blue;
        }
    }

    TGA_save(
        fname,
        640,
        480,
        &tga[0][0],
        UC_FALSE);
}

// uc_orig: plan_view_shot (fallen/Source/Controls.cpp)
// Renders a top-down map overview and saves it as a TGA file.
// Layers: PAP heightmap → shadow flags → roof_faces4[] → dfacets[] → EWAY waypoints.
// Output size: 128*3 x 128*3 pixels. Saved to c:\shot\<mapname>.tga.
void plan_view_shot()
{
    SLONG i;

    SLONG x;
    SLONG y;

    SLONG dx;
    SLONG dy;

    SLONG px;
    SLONG py;

    SLONG px1;
    SLONG py1;

    SLONG px2;
    SLONG py2;

    SLONG mx;
    SLONG my;
    SLONG mz;
    SLONG height;

    UBYTE red;
    UBYTE green;
    UBYTE blue;

    SLONG shadow;

    RoofFace4* rf;
    DFacet* df;

    UBYTE shad[8][9] = {
        { 4, 4, 4, 4, 4, 4, 4, 4, 4 },

        { 4, 4, 1, // 1
            4, 2, 1,
            3, 2, 1 },

        { 3, 2, 1, // 2
            3, 2, 1,
            3, 2, 1 },

        { 3, 2, 1, // 3
            3, 2, 2,
            4, 3, 3 },

        { 1, 1, 1, // 4
            2, 2, 2,
            3, 3, 3 },

        { 1, 1, 1, // 5
            2, 2, 1,
            3, 2, 1 },

        { 3, 2, 1, // 6
            3, 2, 1,
            3, 2, 1 },

        { 1, 1, 1, // 7
            2, 2, 4,
            3, 4, 4 }
    };

    // Floor layer: colour based on heightmap and road presence.
    for (px = 0; px < 128 * 3; px++)
        for (py = 0; py < 128 * 3; py++) {
            mx = (px << 8) / 3;
            mz = (py << 8) / 3;

            height = PAP_calc_height_at(mx, mz);

            if (ROAD_is_road(mx >> 8, mz >> 8)) {
                red = 64 + (height >> 3);

                SATURATE(red, 0, 255);

                green = red;
                blue = red;
            } else {
                red = 128 + (height >> 4);

                SATURATE(red, 4, 255);

                green = red - 4;
                blue = red - 4;
            }

            tga[px][py].red = red;
            tga[px][py].green = green;
            tga[px][py].blue = blue;
        }

    // Shadow layer: apply directional shadow attenuation from PAP flags.
    for (mx = 0; mx < 128; mx++)
        for (mz = 0; mz < 128; mz++) {
            px = mx * 3;
            py = mz * 3;

            shadow = PAP_2HI(mx, mz).Flags & 0x7;

            for (dx = 0; dx < 3; dx++)
                for (dy = 0; dy < 3; dy++) {
                    tga[px + dx][py + dy].red = tga[px + dx][py + dy].red * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
                    tga[px + dx][py + dy].green = tga[px + dx][py + dy].green * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
                    tga[px + dx][py + dy].blue = tga[px + dx][py + dy].blue * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
                }
        }

    // Roof layer: colour based on height, then shade.
    for (i = 1; i < next_roof_face4; i++) {
        rf = &roof_faces4[i];

        height = rf->Y;

        red = 100 + (height >> 5);
        green = red - 4;
        blue = red + 4;

        px = (rf->RX & 127) * 3;
        py = (rf->RZ & 127) * 3;

        shadow = rf->DrawFlags & 0x7;

        for (dx = 0; dx < 3; dx++)
            for (dy = 0; dy < 3; dy++) {
                tga[px + dx][py + dy].red = red * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
                tga[px + dx][py + dy].green = green * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
                tga[px + dx][py + dy].blue = blue * shad[shadow][(2 - dx) + (2 - dy) * 3] >> 2;
            }
    }

    // Fence layer: draw fence lines in green.
    for (i = 1; i < next_dfacet; i++) {
        df = &dfacets[i];

        if (df->FacetType == STOREY_TYPE_FENCE || df->FacetType == STOREY_TYPE_FENCE_FLAT || df->FacetType == STOREY_TYPE_FENCE_BRICK) {
            px1 = df->x[0] * 3;
            py1 = df->z[0] * 3;

            px2 = df->x[1] * 3;
            py2 = df->z[1] * 3;

            dx = SIGN(px2 - px1);
            dy = SIGN(py2 - py1);

            if (dx && dy) {
                // Ignore diagonal facets.
                continue;
            }

            x = px1;
            y = py1;

            while (1) {
                tga[x][y].red = 50;
                tga[x][y].green = 200;
                tga[x][y].blue = 10;

                x += dx;
                y += dy;

                if (x == px2 && y == py2) {
                    break;
                }
            }
        }
    }

    // Waypoint layer: coloured dots by type (enemies=red, cops=blue, civs=white, etc.).
    UBYTE dot_do;
    UBYTE dot_size;

    EWAY_Way* ew;

    for (i = 1; i < EWAY_way_upto; i++) {
        ew = &EWAY_way[i];

        dot_size = 1;

        switch (ew->ed.type) {
        case EWAY_DO_CREATE_PLAYER:
            dot_do = UC_TRUE;
            red = 0;
            green = 0;
            blue = 0;
            dot_size = 2;
            break;

        case EWAY_DO_CREATE_ITEM:
            dot_do = UC_TRUE;
            red = 210;
            green = 210;
            blue = 40;
            break;

        case EWAY_DO_CREATE_ENEMY:

            switch (ew->ed.subtype) {
            case PERSON_DARCI:
            case PERSON_ROPER:

                dot_do = UC_TRUE;
                red = 55;
                green = 255;
                blue = 55;

                break;

            case PERSON_CIV:
            case PERSON_SLAG_TART:
            case PERSON_SLAG_FATUGLY:
            case PERSON_HOSTAGE:
            case PERSON_MECHANIC:
            case PERSON_TRAMP:

                // Don't show wandering civs.
                EWAY_Edef* ee;

                ASSERT(WITHIN(ew->index, 1, EWAY_edef_upto - 1));

                ee = &EWAY_edef[ew->index];

                if (ee->pcom_move != PCOM_MOVE_WANDER) {
                    dot_do = UC_TRUE;
                } else {
                    dot_do = UC_FALSE;
                }

                red = 255;
                green = 255;
                blue = 255;

                break;

            case PERSON_COP:

                dot_do = UC_TRUE;
                red = 55;
                green = 55;
                blue = 255;

                break;

            default:
            case PERSON_THUG_RASTA:
            case PERSON_THUG_GREY:
            case PERSON_THUG_RED:
            case PERSON_MIB1:
            case PERSON_MIB2:
            case PERSON_MIB3:

                dot_do = UC_TRUE;

                red = 255;
                green = 55;
                blue = 55;

                break;
            }

            break;

        default:
            dot_do = UC_FALSE;
            break;
        }

        if (dot_do) {
            SLONG alpha;

            mx = ew->x * 3 >> 8;
            my = ew->z * 3 >> 8;

            for (dx = -dot_size; dx <= dot_size; dx++)
                for (dy = -dot_size; dy <= dot_size; dy++) {
                    px = mx + dx;
                    py = my + dy;

                    if (WITHIN(px, 0, 128 * 3 - 1) && WITHIN(py, 0, 128 * 3 - 1)) {
                        alpha = abs(dx) + abs(dy);

                        tga[px][py].red = red * (4 - alpha) + tga[px][py].red * alpha >> 2;
                        tga[px][py].green = green * (4 - alpha) + tga[px][py].green * alpha >> 2;
                        tga[px][py].blue = blue * (4 - alpha) + tga[px][py].blue * alpha >> 2;
                    }
                }
        }
    }

    extern CBYTE ELEV_fname_level[];
    extern CBYTE ELEV_fname_map[];

    {
        CBYTE fname[256];
        CBYTE* mapname;
        CBYTE* ch;

        if (ELEV_fname_level[0]) {
            mapname = ELEV_fname_level;
        } else if (ELEV_fname_map[0]) {
            mapname = ELEV_fname_map;
        } else {
            mapname = NULL;
        }

        if (mapname) {

            for (ch = mapname; *ch; ch++)
                ;

            while (ch > mapname) {
                if (*ch == '\\') {
                    ch += 1;

                    break;
                }

                ch--;
            }

            mapname = ch;

        } else {
            mapname = "shot.tga";
        }

        sprintf(fname, "c:\\shot\\%s", mapname);

        for (ch = fname; *ch; ch++)
            ;

        ch[-3] = 't';
        ch[-2] = 'g';
        ch[-1] = 'a';

        TGA_save(
            fname,
            128 * 3,
            128 * 3,
            &tga[0][0],
            UC_FALSE);
    }
}

// uc_orig: can_i_draw_this_special (fallen/Source/Controls.cpp)
// Returns non-zero if the item type can be shown in the inventory UI.
// Filters out item types that have no visual representation (not weapons/tools).
SLONG can_i_draw_this_special(Thing* p_special)
{
    if (SPECIAL_info[p_special->Genus.Special->SpecialType].group == SPECIAL_GROUP_ONEHANDED_WEAPON || SPECIAL_info[p_special->Genus.Special->SpecialType].group == SPECIAL_GROUP_TWOHANDED_WEAPON || p_special->Genus.Special->SpecialType == SPECIAL_EXPLOSIVES || p_special->Genus.Special->SpecialType == SPECIAL_WIRE_CUTTER)
        return UC_TRUE;
    else
        return UC_FALSE;
}

// uc_orig: CONTROLS_set_inventory (fallen/Source/Controls.cpp)
// Applies the item at slot [count] in Darci's inventory (0=fists, 1+=items, last=gun).
// Puts away the current weapon first, then equips the newly selected item.
void CONTROLS_set_inventory(Thing* darci, Thing* player, SLONG count)
{
    Thing* p_special = NULL;

    if (darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT) {
        darci->Genus.Person->SpecialUse = NULL;
        darci->Draw.Tweened->PersonID &= ~0xe0;
        darci->Genus.Person->Flags &= ~FLAG_PERSON_GUN_OUT;
    }

    if (!count) {
        set_person_item_away(darci);
        return;
    }

    if (darci->Genus.Person->SpecialList != 0)
        p_special = TO_THING(darci->Genus.Person->SpecialList);
    else
        p_special = 0;

    if (p_special && can_i_draw_this_special(p_special))
        count--;

    while (count) {
        if (!p_special)
            break;
        ASSERT(p_special->Class == CLASS_SPECIAL);

        if (p_special->Genus.Special->NextSpecial)
            p_special = TO_THING(p_special->Genus.Special->NextSpecial);
        else
            p_special = NULL;

        if (p_special && can_i_draw_this_special(p_special)) {
            count--;
        }
    }

    if (p_special && can_i_draw_this_special(p_special)) {
        set_person_draw_item(darci, p_special->Genus.Special->SpecialType);
    } else {
        darci->Genus.Person->SpecialUse = NULL;
        darci->Draw.Tweened->PersonID = 0;

        if (darci->Flags & FLAGS_HAS_GUN) {
            set_person_draw_gun(darci);
        } else {
            set_person_idle(darci);
        }
    }
}

// uc_orig: form_proc (fallen/Source/Controls.cpp)
// Widget callback for popup dialog forms. Any button push closes the form.
BOOL form_proc(Form* form, Widget* widget, SLONG message)
{
    if (widget && widget->methods == &BUTTON_Methods && message == WBN_PUSH) {
        form->returncode = UC_TRUE;

        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: CONTROLS_get_selected_item (fallen/Source/Controls.cpp)
// Returns the inventory slot index of the currently equipped item (0=fist).
// Also updates player->Genus.Player->ItemCount with the total slot count.
SBYTE CONTROLS_get_selected_item(Thing* darci, Thing* player)
{
    SBYTE count = 1;
    Thing* p_special = NULL;
    SBYTE current_item = 0;

    if (darci->Genus.Person->SpecialList) {
        p_special = TO_THING(darci->Genus.Person->SpecialList);

        while (p_special) {
            ASSERT(p_special->Class == CLASS_SPECIAL);
            if (can_i_draw_this_special(p_special)) {
                if (THING_NUMBER(p_special) == darci->Genus.Person->SpecialUse)
                    current_item = count;
                count++;
            }
            if (p_special->Genus.Special->NextSpecial)
                p_special = TO_THING(p_special->Genus.Special->NextSpecial);
            else
                p_special = NULL;
        }
    }

    if (darci->Flags & FLAGS_HAS_GUN) {
        if (darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
            current_item = count;
        count++;
    }
    player->Genus.Player->ItemCount = count;

    return current_item;
}

// uc_orig: CONTROLS_get_best_item (fallen/Source/Controls.cpp)
// Auto-selects the best available weapon using a fixed priority score.
// Ignores weapons with no ammo. Called when Darci is unarmed and selects inventory.
SBYTE CONTROLS_get_best_item(Thing* darci, Thing* player)
{
    SBYTE count = 1;
    Thing* p_special = NULL;
    SBYTE current_item = 0, current_score = 0;

    if (darci->Genus.Person->SpecialList) {
        p_special = TO_THING(darci->Genus.Person->SpecialList);

        while (p_special) {
            ASSERT(p_special->Class == CLASS_SPECIAL);
            if (can_i_draw_this_special(p_special)) {
                switch (p_special->Genus.Special->SpecialType) {
                case SPECIAL_SHOTGUN:
                    if (p_special->Genus.Special->ammo || darci->Genus.Person->ammo_packs_shotgun)
                        if (current_score < SHOTGUN_SCORE) {
                            current_item = count;
                            current_score = SHOTGUN_SCORE;
                        }
                    break;

                case SPECIAL_AK47:
                    if (p_special->Genus.Special->ammo || darci->Genus.Person->ammo_packs_ak47)
                        if (current_score < AK47_SCORE) {
                            current_item = count;
                            current_score = AK47_SCORE;
                        }
                    break;
                case SPECIAL_GRENADE:
                    if (p_special->Genus.Special->ammo)
                        if (current_score < GRENADE_SCORE) {
                            current_item = count;
                            current_score = GRENADE_SCORE;
                        }
                    break;

                case SPECIAL_BASEBALLBAT:
                    if (current_score < BAT_SCORE) {
                        current_item = count;
                        current_score = BAT_SCORE;
                    }
                    break;
                case SPECIAL_KNIFE:
                    if (current_score < KNIFE_SCORE) {
                        current_item = count;
                        current_score = KNIFE_SCORE;
                    }
                    break;
                }
                count++;
            }
            if (p_special->Genus.Special->NextSpecial)
                p_special = TO_THING(p_special->Genus.Special->NextSpecial);
            else
                p_special = NULL;
        }
    }

    if (darci->Flags & FLAGS_HAS_GUN) {
        if (darci->Genus.Person->ammo_packs_pistol || darci->Genus.Person->Ammo) {
            if (current_score < PISTOL_SCORE) {
                current_item = count;
                current_score = PISTOL_SCORE;
            }
        }

        count++;
    }

    return (current_item);
}

// uc_orig: CONTROLS_new_inventory (fallen/Source/Controls.cpp)
// Opens the inventory panel (begins fade-in). Initialises ItemFocus if not set.
// Always returns 0; CONTROLS_get_best_item branch is commented out in this version.
SLONG CONTROLS_new_inventory(Thing* darci, Thing* player)
{
    UWORD temp = player->Genus.Player->PopupFade;
    if (!temp)
        player->Genus.Player->ItemFocus = -1;

    temp += INVENTORY_FADE_SPEED;

    if (temp < 256)
        player->Genus.Player->PopupFade = temp;

    if (player->Genus.Player->ItemFocus == -1) {
        /*
                        if(darci->Genus.Person->SpecialUse==0 &&  !(darci->Flags & FLAGS_HAS_GUN) )
                        {
                                //
                                // currently unarmed so pick best weapon
                                //

                                player->Genus.Player->ItemFocus = CONTROLS_get_best_item(darci, player);
                                if(player->Genus.Player->ItemFocus)
                                {
                                        CONTROLS_set_inventory(darci,player,player->Genus.Player->ItemFocus);
                                        return(1);
                                }
                        }
                        else
        */

        {
            player->Genus.Player->ItemFocus = CONTROLS_get_selected_item(darci, player);
        }
    }
    return (0);
}

// uc_orig: CONTROLS_rot_inventory (fallen/Source/Controls.cpp)
// Cycles the inventory selection by dir (+1 forward / -1 backward).
// If pull_it_out_ooooerrr is set, immediately equips the newly selected item.
void CONTROLS_rot_inventory(Thing* darci, Thing* player, SBYTE dir, SLONG pull_it_out_ooooerrr)
{
    player->Genus.Player->ItemFocus += dir;
    if (player->Genus.Player->ItemFocus == -1)
        player->Genus.Player->ItemFocus = player->Genus.Player->ItemCount - 1;

    if (player->Genus.Player->ItemFocus >= player->Genus.Player->ItemCount)
        player->Genus.Player->ItemFocus = 0;

    if (pull_it_out_ooooerrr)
        CONTROLS_set_inventory(darci, player, player->Genus.Player->ItemFocus);
}

// uc_orig: context_music (fallen/Source/Controls.cpp)
// Selects background music each frame based on Darci's current mode.
// Priority order: neutral → driving → crawling → fighting → sprinting.
// Special bodge_code values 1-4 override for train/final-level scripted tracks.
void context_music(void)
{
    UBYTE mode = 0;
    Thing* darci;
    static SLONG danger = 0;
    static enum Waves danger_lookup[] = { S_NULL, S_TUNE_DANGER_RED, S_NULL, S_TUNE_DANGER_GREEN };
    UBYTE new_danger;

    if (!NET_PLAYER(PLAYER_ID)) {
        MUSIC_mode(0);
        return;
    } else
        new_danger = NET_PLAYER(PLAYER_ID)->Genus.Player->Danger;

    // The danger 'tunes' are more sfx than music, and can overlap other tunes without clashing.
    if (new_danger != danger) {
        danger = danger_lookup[new_danger];
        if (danger)
            MFX_play_ambient(MUSIC_REF + 1, danger, MFX_OVERLAP);
        danger = new_danger;
    }

    darci = NET_PERSON(PLAYER_ID);

    if (darci->Genus.Person->Flags & FLAG_PERSON_DRIVING)
        mode = MUSIC_MODE_DRIVING;

    if (darci->SubState == SUB_STATE_CRAWLING)
        mode = MUSIC_MODE_CRAWLING;

    if (darci->Genus.Person->Mode == PERSON_MODE_FIGHT)
        mode = MUSIC_MODE_FIGHTING;

    if (darci->Genus.Person->Mode == PERSON_MODE_SPRINT)
        mode = MUSIC_MODE_SPRINTING;

    switch (MUSIC_bodge_code) {
    case 1:
        mode = MUSIC_MODE_TRAIN_COMBAT;
        break;
    case 2:
        mode = MUSIC_MODE_TRAIN_ASSAULT;
        break;
    case 3:
        mode = MUSIC_MODE_TRAIN_DRIVING;
        break;
    case 4:
        mode = MUSIC_MODE_FINAL_RECKONING;
        break;
    }

    if (WARE_ware[darci->Genus.Person->Ware].ambience == 4)
        return;

    MUSIC_mode(mode);
}

// uc_orig: set_danger_level (fallen/Source/Controls.cpp)
// Recalculates Player->Danger (0..3) for each player every 16 frames.
// Scans a sphere of radius 0xC00 for hostile NPCs; danger scales with proximity.
void set_danger_level()
{
    SLONG num_found;
    SLONG i, j;

    SLONG dist;
    SLONG best_dist = UC_INFINITY;
    SLONG best_person = NULL;

    Thing* p_found;

    for (i = 0; i < NO_PLAYERS; i++) {
        Thing* p_person = NET_PERSON(i);
        Thing* p_player = NET_PLAYER(i);

        if (p_person == NULL) {
            continue;
        }

        {
            num_found = THING_find_sphere(
                p_person->WorldPos.X >> 8,
                p_person->WorldPos.Y >> 8,
                p_person->WorldPos.Z >> 8,
                0xc00,
                THING_array,
                THING_ARRAY_SIZE,
                1 << CLASS_PERSON);

            for (j = 0; j < num_found; j++) {
                p_found = TO_THING(THING_array[j]);

                if (p_found == p_person) {
                    continue;
                }

                if (p_found->State == STATE_DEAD) {
                    continue;
                }

                if (am_i_a_thug(p_found)) {

                    SLONG dx = abs(p_found->WorldPos.X - p_person->WorldPos.X >> 8);
                    SLONG dy = abs(p_found->WorldPos.Y - p_person->WorldPos.Y >> 8);
                    SLONG dz = abs(p_found->WorldPos.Z - p_person->WorldPos.Z >> 8);

                    dist = QDIST3(dx, dy, dz);

                    if (dist < 0xc00) {
                        if (best_dist > dist) {
                            best_dist = dist;
                            best_person = THING_array[i];
                        }
                    }
                }
            }

            if (best_person) {
                p_player->Genus.Player->Danger = best_dist / 0x400 + 1;

                SATURATE(p_player->Genus.Player->Danger, 1, 3);
            } else {
                p_player->Genus.Player->Danger = 0;
            }
        }
    }
}
