// uc_orig: controls.cpp (fallen/Source/Controls.cpp)
// Per-frame game-controller: inventory management, music context selection,
// danger level calculation, debug console parser, and the process_controls() dispatcher.

#include <stdio.h>
#include <string.h>

#include "game/game_types.h" // NET_PERSON, NET_PLAYER, GAME_STATE, GAME_TURN, the_game
#include "ai/pcom.h"
#include "assets/sound_id.h"
#include "engine/audio/music.h"
#include "engine/audio/sound.h"
#include "engine/audio/mfx.h"
#include "engine/console/console.h"
#include "engine/debug/debug_help/debug_help.h" // F1 debug hotkey legend
#include "engine/graphics/lighting/night.h"
#include "engine/graphics/lighting/night_globals.h"
#include "engine/graphics/render_interp.h"
#include "engine/graphics/geometry/bind_palette.h"
#include "engine/graphics/geometry/figure.h"
#include "map/pap.h"
#include "map/road.h"
#include "missions/eway.h"
#include "missions/eway_globals.h"
#include "effects/combat/pyro.h"
#include "things/characters/person.h"
#include "things/characters/person_globals.h" // anim_type[], mesh_type[] (TEMP model cycler)
#include "things/characters/anim_ids.h" // ANIM_STAND_HIP (TEMP model cycler reset anim)
#include "things/core/thing.h"
#include "things/core/statedef.h"
// ANIM_TYPE_DARCI, ANIM_TYPE_ROPER come from fallen/Headers/Person.h via actors/characters/person.h
#include "assets/formats/anim_globals.h"
#include "buildings/ware.h"
#include "buildings/ware_globals.h"
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "assets/formats/tga.h"

#include "engine/io/file.h"
#include "engine/platform/sdl3_bridge.h" // sdl3_get_ticks (SPARK_process wall-clock gate)
#include "map/level_pools.h"
#include "map/supermap.h"
#include "map/supermap_globals.h"
#include "things/items/special.h"
#include "assets/formats/elev_globals.h"
#include "camera/fc.h"
#include "camera/fc_globals.h"
#include "things/characters/anim_ids.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "game/action_map/act_dev_console.h" // ACT_CONS_*
#include "game/action_map/act_bangunsnotgames.h" // ACT_BANG_*
#include "game/action_map/act_foot.h" // ACT_FOOT_*
#include "game/action_map/act_car.h" // ACT_CAR_*
#include "engine/input/input_frame.h"
#include "game/game_globals.h" // g_frame_dt_ms (wall-clock per-render-frame delta)

#include "world_objects/dirt.h"
#include "effects/weather/mist.h"
#include "effects/combat/spark.h"
#include "effects/combat/glitter.h"
#include "effects/environment/ribbon.h"
#include "things/items/hook.h"
#include "things/characters/snipe.h"
#include "things/items/barrel.h"
#include "things/vehicles/vehicle.h"
#include "things/vehicles/chopper.h"
#include "navigation/wmove.h"
#include "navigation/wand.h"
#include "map/ob.h"
#include "engine/effects/psystem.h"
#include "engine/physics/collide.h"
#include "engine/input/mouse.h"
#include "ai/mav.h"
#include "assets/formats/anim_tmap.h"
#include "ui/hud/panel.h"

#include "ui/menus/widget.h" // Form type for form_leave_map extern decl
#include "game/game_tick.h"
#include "game/game_tick_globals.h"
#include "combat/combat_test_mode.h" // debug combat-testing harness (bangunsnotgames)
#include "engine/graphics/pipeline/aeng.h" // AENG_world_line, AENG_raytraced_position

// Forward declarations for functions not yet migrated from old/
// Scene FBO dimensions (defined in d3d/display_globals.cpp).
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;

extern SLONG am_i_a_thug(Thing* p_person);
extern SLONG analogue;
extern UWORD fade_black;
extern void reload_level(void);
extern SLONG plant_feet(Thing* p_person);
extern UWORD count_gang(Thing* p_target);
extern UBYTE aeng_draw_cloud_flag; // defined in aeng.cpp

// tga[] is file-local, used only by tga_dump and plan_view_shot.
// uc_orig: tga (fallen/Source/Controls.cpp)
static TGA_Pixel tga[480][640];

// uc_orig: INVENTORY_FADE_SPEED (fallen/Source/Controls.cpp)
// Inventory panel fade speed in opacity units per frame.
#define INVENTORY_FADE_SPEED (32)

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
        if (!oc_stricmp(cmd, cmd_list[i])) {
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
                    sprintf(cmd, "%d", way->id);
                    CONSOLE_text(cmd);
                } else
                    CONSOLE_text("wpt not found");
                break;

            case 7: // vtx - dump vertex buffer information
            {
                // uc-abs-path: was "C:\VertexBufferInfo.txt"
                FILE* fd = MF_Fopen("VertexBufferInfo.txt", "w");
                if (fd) {
                    ge_dump_vpool_info(fd);
                    MF_Fclose(fd);
                    CONSOLE_text("Info dumped at VertexBufferInfo.txt");
                } else
                    CONSOLE_text("Can't open VertexBufferInfo.txt");
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
                    ge_set_gamma(val, 256);
                } else {
                    CONSOLE_text("Gamma 0-7");
                }
                break;

            case 10: // dkeys -- debug keys en/disable
                allow_debug_keys ^= 1;
                if (allow_debug_keys) {
                    CONSOLE_text("debug mode on");
                    // Pop up the hotkey legend for 5 seconds so the user
                    // immediately sees what debug mode unlocks (F1 to
                    // redisplay).
                    debug_help_notify_bangunsnotgames_on();
                } else
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

            // Cheat commands (no allow_debug_keys gate — these are full
            // gameplay cheats, equivalents of the gamepad combo).
            case 26: // bloodofkings — toggle immortality
                cheat_apply_immortal_toggle();
                break;
            case 27: // shieldofsteel — full health
                cheat_apply_full_health();
                break;
            case 28: // weneedguns — spawn weapons ring around player
                cheat_apply_spawn_weapons();
                break;
            case 29: // losttrack — max ammo for all weapon types
                cheat_apply_max_ammo();
                break;
            }
            return;
        }
    }
    CONSOLE_text("huh?", 3000);
}

// uc_orig: tga_dump (fallen/Source/Controls.cpp)
// Captures the current frame to a TGA file at actual screen resolution.
void tga_dump(void)
{
    SLONG i;

    CBYTE fname[64];

    FILE* handle;

    // Create output directory if needed.
    oc_mkdir("debug_screenshots");

    for (i = 0; i < 10000; i++) {
        sprintf(fname, "debug_screenshots/shot%04d.tga", i);

        handle = MF_Fopen(fname, "rb");

        if (handle) {
            MF_Fclose(handle);
        } else {
            goto found_file;
        }
    }

    return;

found_file:;

    SLONG w = ge_get_screen_width();
    SLONG h = ge_get_screen_height();

    // One-shot read: glReadPixels into a scratch RGBA buffer, then strip the
    // alpha channel into the TGA_Pixel packing. No lock/unlock writeback path
    // — that was the WDDM throttle trigger (see startup_hang_investigation).
    uint8_t* rgba = (uint8_t*)malloc((size_t)w * (size_t)h * 4);
    if (!rgba)
        return;
    ge_read_framebuffer_rgba(rgba, w, h);

    TGA_Pixel* pixels = (TGA_Pixel*)malloc(w * h * sizeof(TGA_Pixel));
    if (!pixels) {
        free(rgba);
        return;
    }

    for (SLONG p = 0, n = w * h; p < n; ++p) {
        pixels[p].red = rgba[p * 4 + 0];
        pixels[p].green = rgba[p * 4 + 1];
        pixels[p].blue = rgba[p * 4 + 2];
    }
    free(rgba);

    TGA_save(fname, w, h, pixels, UC_FALSE);
    free(pixels);
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
                if (*ch == '\\' || *ch == '/') {
                    ch += 1;

                    break;
                }

                ch--;
            }

            mapname = ch;

        } else {
            mapname = "shot.tga";
        }

        // uc-abs-path: was "c:\shot\%s"
        sprintf(fname, "shot/%s", mapname);

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

// uc_orig: CONTROLS_new_inventory (fallen/Source/Controls.cpp)
// Opens the inventory panel (begins fade-in). Initialises ItemFocus if not set.
// Always returns 0.
SLONG CONTROLS_new_inventory(Thing* darci, Thing* player)
{
    UWORD temp = player->Genus.Player->PopupFade;

    // Wall-clock fade-in. Original `temp += INVENTORY_FADE_SPEED` was per
    // render frame, so on high FPS the popup fade-in finished in tens of
    // ms (visually instant). Scaling by frame_dt / design_tick gives a
    // constant fade duration regardless of render rate. Static float
    // accumulator preserves sub-unit precision at non-integer ratios
    // (e.g. 100 FPS gives non-integer units/frame — truncation alone
    // would lose precision each frame, accumulating to ~5% error over
    // the fade). Single-player only — static is safe since CNET multi-
    // player is dead.
    //
    // Reference rate: 20 Hz (UC_PHYSICS_DESIGN_HZ). See game_types.h —
    // 30 Hz is a PS1-only rate; PC retail ran at ~22 Hz, and most
    // engine tick arithmetic is built around 20 Hz.
    constexpr float DESIGN_TICK_MS = 1000.0f / float(UC_PHYSICS_DESIGN_HZ);
    static float fade_in_accum = 0.0f;
    if (!temp) {
        player->Genus.Player->ItemFocus = -1;
        fade_in_accum = 0.0f; // reset accumulator on fade restart
    }

    const float fade_step_f = (float)INVENTORY_FADE_SPEED * g_frame_dt_ms / DESIGN_TICK_MS
        + fade_in_accum;
    const SLONG fade_step_int = (SLONG)fade_step_f;
    fade_in_accum = fade_step_f - (float)fade_step_int;

    temp += fade_step_int;

    if (temp < 256)
        player->Genus.Player->PopupFade = (UBYTE)temp;

    if (player->Genus.Player->ItemFocus == -1) {

        {
            player->Genus.Player->ItemFocus = CONTROLS_get_selected_item(darci, player);
        }
    }
    return (0);
}

// OpenChaos: D-pad direct weapon select (stage13_gamepad.md).
// ===========================================================
// Replaces / complements R3 cycle:
//   D-pad ↑ = pistol
//   D-pad ← = AK47 (called "AK47" in source; visually M16 in retail)
//   D-pad → = shotgun
//   D-pad ↓ = melee toggle (non-melee → fist; fist/melee → cycle
//             fist → bat → knife → fist, owned-only)
// R3 cycle path is left untouched.

// Returns the SpecialType currently equipped by Darci. Synthesises
// SPECIAL_GUN for the pistol (which has no Thing in SpecialList — it sits
// in FLAGS_HAS_GUN/FLAG_PERSON_GUN_OUT). Returns SPECIAL_NONE for an empty
// hand (fist / nothing drawn). Used by the D-pad weapon select to detect
// "already equipped" (no-op) and melee-toggle state.
static SLONG dpad_current_weapon_type(Thing* darci)
{
    if (darci->Genus.Person->Flags & FLAG_PERSON_GUN_OUT)
        return SPECIAL_GUN;
    if (!darci->Genus.Person->SpecialUse)
        return SPECIAL_NONE;
    Thing* p_su = TO_THING(darci->Genus.Person->SpecialUse);
    if (!p_su)
        return SPECIAL_NONE;
    return p_su->Genus.Special->SpecialType;
}

// Walks Darci's SpecialList counting drawable items (matching
// can_i_draw_this_special, same predicate CONTROLS_set_inventory and
// CONTROLS_get_selected_item use). Returns the 1-based slot index of the
// first item with the requested SpecialType, or -1 if not in inventory.
// Slot 0 is reserved for fist; pistol (when owned) lives at slot N+1
// where N is the drawable-count returned by this walk — caller handles
// the pistol separately.
static SLONG dpad_find_inventory_slot_by_type(Thing* darci, SLONG target_type)
{
    if (!darci->Genus.Person->SpecialList)
        return -1;
    SLONG slot = 1; // slot 0 = fist
    Thing* p_special = TO_THING(darci->Genus.Person->SpecialList);
    while (p_special) {
        ASSERT(p_special->Class == CLASS_SPECIAL);
        if (can_i_draw_this_special(p_special)) {
            if (p_special->Genus.Special->SpecialType == target_type)
                return slot;
            slot++;
        }
        if (!p_special->Genus.Special->NextSpecial)
            break;
        p_special = TO_THING(p_special->Genus.Special->NextSpecial);
    }
    return -1;
}

// Counts the drawable-special slots in Darci's inventory (slots 1..N).
// The pistol, if owned (FLAGS_HAS_GUN), sits at slot N+1 — this helper is
// used only to compute that pistol slot for the ↑ direct-select path.
static SLONG dpad_count_drawable_specials(Thing* darci)
{
    SLONG count = 0;
    if (!darci->Genus.Person->SpecialList)
        return 0;
    Thing* p_special = TO_THING(darci->Genus.Person->SpecialList);
    while (p_special) {
        ASSERT(p_special->Class == CLASS_SPECIAL);
        if (can_i_draw_this_special(p_special))
            count++;
        if (!p_special->Genus.Special->NextSpecial)
            break;
        p_special = TO_THING(p_special->Genus.Special->NextSpecial);
    }
    return count;
}

// Canonical ordered weapon list — the single source of truth for BOTH the
// inventory popup (PANEL_inventory) and the scroll, so they stay in lockstep.
// Order: pistol, AK47, shotgun, grenade, bat, knife, then any other owned
// drawable special (explosives, wire cutter, ...) in inventory order, then the
// FIST last. Each entry is a token: SPECIAL_GUN for the pistol, the SpecialType
// for specials, SPECIAL_NONE (0) for the fist. Only OWNED items are added.
// Returns the entry count (always >= 1 — the fist is always present).
SLONG CONTROLS_build_weapon_list(Thing* darci, CBYTE* out, SLONG max)
{
    static const SLONG priority[] = {
        SPECIAL_GUN, SPECIAL_AK47, SPECIAL_SHOTGUN, SPECIAL_GRENADE,
        SPECIAL_BASEBALLBAT, SPECIAL_KNIFE,
    };
    const SLONG prio_len = (SLONG)(sizeof(priority) / sizeof(priority[0]));
    SLONG n = 0;

    // 1) Priority weapons in fixed order (owned only).
    for (SLONG i = 0; i < prio_len; ++i) {
        const SLONG t = priority[i];
        const bool owned = (t == SPECIAL_GUN)
            ? ((darci->Flags & FLAGS_HAS_GUN) != 0)
            : (dpad_find_inventory_slot_by_type(darci, t) >= 0);
        if (owned && n < max)
            out[n++] = (CBYTE)t;
    }

    // 2) Any other drawable special not already listed, in inventory order.
    if (darci->Genus.Person->SpecialList) {
        Thing* p = TO_THING(darci->Genus.Person->SpecialList);
        while (p) {
            ASSERT(p->Class == CLASS_SPECIAL);
            if (can_i_draw_this_special(p)) {
                const SLONG t = p->Genus.Special->SpecialType;
                bool listed = false;
                for (SLONG i = 0; i < prio_len; ++i)
                    if (priority[i] == t) { listed = true; break; }
                if (!listed && n < max)
                    out[n++] = (CBYTE)t;
            }
            if (!p->Genus.Special->NextSpecial)
                break;
            p = TO_THING(p->Genus.Special->NextSpecial);
        }
    }

    // 3) Fist last (always present).
    if (n < max)
        out[n++] = (CBYTE)SPECIAL_NONE;

    return n;
}

// Index of a weapon token in the canonical list (the fist token SPECIAL_NONE
// lands on the last index). Returns -1 if not present.
static SLONG controls_weapon_list_index_of(Thing* darci, SLONG type)
{
    CBYTE list[16];
    const SLONG count = CONTROLS_build_weapon_list(darci, list, 16);
    for (SLONG i = 0; i < count; ++i)
        if ((SLONG)list[i] == type)
            return i;
    return -1;
}

// Open / refresh the weapon popup HUD (and reset its display timer). The popup
// always highlights whatever weapon is CURRENTLY in hand (by type), so this on
// its own is the "show the wheel without changing weapon" case.
static void controls_show_popup(Thing* darci, Thing* player)
{
    CONTROLS_new_inventory(darci, player);
    CONTROLS_inventory_mode = 3000;
}

// THE single weapon-equip path. `token` is a canonical-list item: SPECIAL_GUN
// (pistol), a SpecialType, or SPECIAL_NONE (disarm to fists). ALWAYS opens the
// popup so the player sees the result; actually swaps only when the item is
// owned and not already in hand — otherwise it's popup-only feedback ("button
// registered, nothing to swap to"). Every weapon action (D-pad / digits / melee
// / scroll / disarm / arm-last) funnels through here, so the equip + popup
// bookkeeping lives in exactly one place. ItemFocus is left to CONTROLS_new_-
// inventory: the popup highlights by current type, not by ItemFocus.
static void controls_equip(Thing* darci, Thing* player, SLONG token)
{
    controls_show_popup(darci, player);

    if (token == dpad_current_weapon_type(darci))
        return; // already in hand — popup feedback only

    if (token == SPECIAL_NONE) {
        CONTROLS_set_inventory(darci, player, 0); // → fists
        return;
    }

    // Old-slot index for CONTROLS_set_inventory (pistol sits after the specials,
    // outside SpecialList; ownership is the FLAGS_HAS_GUN bit).
    const SLONG slot = (token == SPECIAL_GUN)
        ? ((darci->Flags & FLAGS_HAS_GUN) ? dpad_count_drawable_specials(darci) + 1 : -1)
        : dpad_find_inventory_slot_by_type(darci, token);
    if (slot < 0)
        return; // not owned — popup-only

    CONTROLS_set_inventory(darci, player, slot);
}

// D-pad ↓ / Tab melee toggle — SELECTS a melee weapon and cycles between the
// owned ones (bat ↔ knife). The fist is NOT a target (disarm is R3 / middle-
// mouse). No melee owned → popup-only (highlights the current weapon).
static void controls_dpad_select_melee(Thing* darci, Thing* player)
{
    constexpr SLONG cycle[] = { SPECIAL_BASEBALLBAT, SPECIAL_KNIFE };
    constexpr SLONG cycle_len = (SLONG)(sizeof(cycle) / sizeof(cycle[0]));

    const SLONG cur = dpad_current_weapon_type(darci);
    SLONG idx = -1; // position of the current weapon in the cycle (-1 = none)
    for (SLONG i = 0; i < cycle_len; ++i)
        if (cycle[i] == cur) { idx = i; break; }

    // First OWNED melee after the current one (wraps; skips un-owned).
    for (SLONG step = 1; step <= cycle_len; ++step) {
        const SLONG t = cycle[(idx + step) % cycle_len];
        if (dpad_find_inventory_slot_by_type(darci, t) >= 0) {
            controls_equip(darci, player, t);
            return;
        }
    }
    controls_show_popup(darci, player); // no melee owned
}

// --- Disarm / last-weapon + inventory scroll (R3 / MMB / wheel) -------------
// OpenChaos weapon scheme: R3 / middle-mouse toggle disarm ↔ last weapon (the
// old "best weapon" auto-pick is gone); mouse wheel and R3 + D-pad ↑/↓ scroll
// the inventory popup. See act_foot.h.

// Most recently equipped non-fist weapon type (SPECIAL_GUN for the pistol).
// Updated every gameplay frame while something is in hand; consulted to re-arm
// from empty hands. SPECIAL_NONE = nothing equipped yet this session.
static SLONG g_weapon_last_type = SPECIAL_NONE;

// Re-arm: equip the last weapon (or the FIRST weapon in the canonical list —
// pistol if owned — when the last is gone / never set). Opens the popup.
static void weapon_arm_last(Thing* darci, Thing* player)
{
    CBYTE list[16];
    const SLONG count = CONTROLS_build_weapon_list(darci, list, (SLONG)sizeof(list));
    const SLONG weapon_count = count - 1; // trailing entry is the fist
    if (weapon_count <= 0) {
        controls_show_popup(darci, player); // no weapons owned
        return;
    }
    SLONG idx = controls_weapon_list_index_of(darci, g_weapon_last_type);
    if (idx < 0 || idx >= weapon_count)
        idx = 0;
    controls_equip(darci, player, (SLONG)list[idx]);
}

// Middle-mouse click: simple toggle — weapon in hand → disarm; fists → re-arm
// the last weapon. (R3 splits these across press/release — see process_controls:
// fists arm on PRESS, a held weapon disarms on RELEASE.)
static void weapon_disarm_or_last(Thing* darci, Thing* player)
{
    if (dpad_current_weapon_type(darci) != SPECIAL_NONE)
        controls_equip(darci, player, SPECIAL_NONE); // disarm
    else
        weapon_arm_last(darci, player);
}

// Wheel / R3 + D-pad ↑↓ scroll: step through the canonical list (the same order
// the popup draws) and equip as it goes, wrapping at the ends. The FIST is part
// of the cycle — landing on it disarms — so scrolling can reach bare hands too.
static void weapon_scroll(Thing* darci, Thing* player, SBYTE dir)
{
    CBYTE list[16];
    const SLONG count = CONTROLS_build_weapon_list(darci, list, (SLONG)sizeof(list));
    if (count <= 1) {
        controls_show_popup(darci, player); // only the fist — nothing to cycle
        return;
    }

    SLONG idx = controls_weapon_list_index_of(darci, dpad_current_weapon_type(darci));
    if (idx < 0)
        idx = 0;
    idx += dir;
    if (idx < 0)
        idx = count - 1;
    else if (idx >= count)
        idx = 0;
    controls_equip(darci, player, (SLONG)list[idx]);
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

// uc_orig: process_controls (fallen/Source/Controls.cpp)
void process_controls(void)
{
    Thing* darci = NET_PERSON(0);

    //	if (Keys[KKEY_D])

    if (GAME_TURN <= 1) {
        // (Keys[KKEY_D] = 0 init reset removed: input_key_just_pressed below
        // doesn't trigger on a held-from-before key — no rising edge in
        // snapshot — so the protection it gave is now automatic.)

        //
        // Find an MIB...
        //

        SLONG list;
        Thing* p_thing;

        for (list = thing_class_head[CLASS_PERSON]; list; list = p_thing->NextLink) {
            p_thing = TO_THING(list);

            ASSERT(p_thing->Class == CLASS_PERSON);
        }
    }

    if (input_debug_modifier_active() && input_key_just_pressed(ACT_BANG_ROOM_BEHIND_CHECK_KKEY)) {
        SLONG is_there_room_behind_person(Thing * p_person, SLONG hit_from_behind);

        if (is_there_room_behind_person(darci, UC_FALSE)) {
            PANEL_new_text(NULL, 400, "There is room behind Darci");
        }

        // set_person_recoil(darci, ANIM_HIT_FRONT_HI, 0);
        // set_person_dead(darci, NULL, PERSON_DEATH_TYPE_LEG_SWEEP, 0, 0);
    }

    //	PANEL_new_text(NULL, 2000, "abcdefghijk lmnopqr stuvwxyz ABCDEFG HIJKLMNO PQRSTUVWXYZ 0123456789 !\"�$%^ &*(){} []<>\\/:;'@ #~?-=+.,");
    //	PANEL_new_text(NULL, 2000, "a-b-c-d-e-f-g  h-i-j-k-l-m-n");

    if (allow_debug_keys) {
    }

    if ((GAME_TURN & 0x0f) == 0)
        set_danger_level();
    context_music();

    if (!CNET_network_game)
        if (darci->State == STATE_DEAD) {
            if (darci->Genus.Person->Flags & FLAG_PERSON_ARRESTED) {
                //
                // Wait till she is actually arrested!
                //

                if (darci->SubState == SUB_STATE_DEAD_ARRESTED) {
                    GAME_STATE = GS_LEVEL_LOST;
                }
            } else {
                GAME_STATE = GS_LEVEL_LOST;
            }
        }
    // Model cycler — N walks a hardcoded per-PersonType list of
    // (MeshID, PersonID) variants. Each PersonType only supports
    // certain combinations because:
    //   • set_person_idle / set_anim_walking / set_anim_running ASSERT
    //     in person.cpp for CIV with MeshID outside {7,8,9} (and
    //     similar gates for other types).
    //   • Each chunk's MultiObject[] only has meshes for the IDs
    //     gameplay actually uses (mesh_type[] default + pcom.cpp
    //     spawn-time overrides).
    //   • PersonID also picks a BodyDef variant (e.g. CIV uses
    //     PersonID 6..9 for clothing/skin from alloc_person).
    //
    // The list below mirrors the (MeshID, PersonID) pairs the original
    // game produces at spawn (mesh_type[] + pcom.cpp + alloc_person CIV
    // randomisation). Press N walks within a type, wrap to next type.
    struct ModelVariant { int8_t mesh_id; int8_t person_id; };
    struct ModelVariantSet { int n; ModelVariant v[8]; };
    constexpr ModelVariantSet VARIANTS[PERSON_NUM_TYPES] = {
        { 1, {{0,0}} },                                   // DARCI
        { 1, {{0,0}} },                                   // ROPER
        { 1, {{4,0}} },                                   // COP
        { 6, {{7,6},{7,7},{7,8},{7,9},{8,0},{9,0}} },     // CIV — 4 BodyDef × MeshID=7 + MeshID 8/9
        { 3, {{0,0},{1,0},{2,0}} },                       // THUG_RASTA (pcom Random()%3)
        { 1, {{1,0}} },                                   // THUG_GREY
        { 1, {{2,0}} },                                   // THUG_RED
        { 1, {{1,0}} },                                   // SLAG_TART
        { 1, {{2,0}} },                                   // SLAG_FATUGLY
        { 1, {{3,0}} },                                   // HOSTAGE
        { 1, {{3,0}} },                                   // MECHANIC
        { 1, {{6,0}} },                                   // TRAMP
        { 1, {{5,0}} },                                   // MIB1
        { 1, {{5,0}} },                                   // MIB2
        { 1, {{5,0}} },                                   // MIB3
    };
    static int s_model_cycle_type     = PERSON_DARCI;
    static int s_model_cycle_variant  = 0;
    if (input_debug_modifier_active() && input_key_just_pressed(ACT_BANG_CYCLE_PLAYER_MODEL_KKEY)) {
        Thing* darci_thing = NET_PERSON(0);
        if (darci_thing && darci_thing->Genus.Person && darci_thing->Draw.Tweened) {
            // Advance variant within type; wrap to next type when out.
            s_model_cycle_variant++;
            if (s_model_cycle_variant >= VARIANTS[s_model_cycle_type].n) {
                s_model_cycle_variant = 0;
                s_model_cycle_type = (s_model_cycle_type + 1) % PERSON_NUM_TYPES;
            }
            const ModelVariant& mv = VARIANTS[s_model_cycle_type].v[s_model_cycle_variant];

            const UBYTE new_anim_type = anim_type[s_model_cycle_type];
            darci_thing->Genus.Person->PersonType = s_model_cycle_type;
            darci_thing->Genus.Person->AnimType   = new_anim_type;
            darci_thing->Draw.Tweened->TheChunk   = &game_chunk[new_anim_type];
            darci_thing->Draw.Tweened->MeshID     = (UBYTE)mv.mesh_id;
            darci_thing->Draw.Tweened->PersonID   = (UBYTE)mv.person_id;
            set_person_idle(darci_thing);
        }
    }
    // Skeleton overlay — B toggles the per-bone debug skeleton draw
    // (lines + per-bone wireframe spheres at joints). Keeper debug
    // feature, gated by allow_debug_keys, listed in F1 legend /
    // debug_keys.md. When off, the entire render block in figure.cpp
    // is skipped behind a single branch — zero per-frame cost.
    //
    // Originally bound to H, but H already triggers plan_view_shot()
    // (top-down map TGA dump) in the allow_debug_keys block below —
    // pressing H would fire both handlers. B avoids that and reads as
    // "Bones".
    if (input_debug_modifier_active() && input_key_just_pressed(ACT_BANG_TOGGLE_SKELETON_OVERLAY_KKEY)) {
        g_skin_debug_draw_skeleton = !g_skin_debug_draw_skeleton;
        CONSOLE_status(g_skin_debug_draw_skeleton
            ? (CBYTE*)"Skeleton overlay: ON"
            : (CBYTE*)"Skeleton overlay: OFF");
    }

    // Persistent status line. "Model: X" only shown in debug mode and
    // when the current selection isn't the default Darci. T-pose used to
    // also write a label here but it's gone now — the per-frame stomp
    // was overwriting the bone-manipulator status line. When neither
    // applies we clear the status iff we wrote it last tick (CONSOLE_status
    // is a shared slot — only stomp our own message, not someone else's).
    {
        static const char* const k_person_names[PERSON_NUM_TYPES] = {
            "DARCI", "ROPER", "COP", "CIV", "THUG_RASTA", "THUG_GREY",
            "THUG_RED", "SLAG_TART", "SLAG_FATUGLY", "HOSTAGE",
            "MECHANIC", "TRAMP", "MIB1", "MIB2", "MIB3"
        };
        static bool s_wrote_status = false;
        const bool non_default = (s_model_cycle_type != PERSON_DARCI)
                              || (s_model_cycle_variant != 0);
        if (allow_debug_keys && non_default) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Model: %s (var %d)",
                     k_person_names[s_model_cycle_type], s_model_cycle_variant);
            CONSOLE_status((CBYTE*)buf);
            s_wrote_status = true;
        } else if (s_wrote_status) {
            CONSOLE_status((CBYTE*)"");
            s_wrote_status = false;
        }
    }

    if (input_debug_modifier_active()) {
        static SLONG index_cam = 0;
        Thing* p_thing;

        if (input_key_just_pressed(ACT_BANG_CYCLE_CAMERA_PERSON_KKEY)) {
            while (1) {
                index_cam++;
                if (index_cam >= MAX_THINGS)
                    index_cam = 0;
                p_thing = TO_THING(index_cam);
                if (p_thing->Class == CLASS_PERSON && (p_thing->Genus.Person->PersonType != PERSON_CIV)) {
                    FC_look_at(1, index_cam);
                    FC_setup_initial_camera(1);
                    break;
                }
            }
        }
        if (input_key_just_pressed(ACT_BANG_CYCLE_CAMERA_PERSON_REV_KKEY)) {
            while (1) {
                index_cam--;
                if (index_cam < 0)
                    index_cam = MAX_THINGS - 1;
                p_thing = TO_THING(index_cam);
                if (p_thing->Class == CLASS_PERSON && (p_thing->Genus.Person->PersonType != PERSON_CIV)) {
                    FC_look_at(1, index_cam);
                    FC_setup_initial_camera(1);
                    break;
                }
            }
        }

        if (input_key_just_pressed(ACT_BANG_TOGGLE_CAMERA_FOCUS_KKEY)) {
            if (FC_cam[1].focus) {
                FC_cam[1].focus = NULL;
            } else {

                p_thing = TO_THING(index_cam);
                if (p_thing->Class == CLASS_PERSON && (p_thing->Genus.Person->PersonType != PERSON_CIV)) {
                    FC_look_at(1, index_cam);
                    FC_setup_initial_camera(1);
                }
            }
        }
    }

    // this stuff shouldn't even _be_ in process_controls.

    DIRT_set_focus(darci->WorldPos.X >> 8, darci->WorldPos.Z >> 8, 0xc00);
    MIST_process();
    //	WATER_process();
    //	BANG_process();

    // SPARK_process is a pure visual tick — it decrements ss->die by 1 and
    // moves spark points by Pos+=dx>>4 each call. Originally called every
    // render frame (PS1 30 FPS, PC retail ~22-50 FPS) so the cadence was
    // render-rate-bound. With unlimited render that means sparks die in
    // ~0.1 s on 280 FPS vs ~1.5 s on 25 FPS, and the per-call Pos advance
    // makes them visibly "wiggle" much faster on high FPS (visible as MIB
    // destruct lightning bolts running fast/short on unlimited).
    // Stays in the render path (it's a visual effect, not gameplay state)
    // but gated to ~15 Hz wall-clock via sdl3_get_ticks edge-detect (same
    // cadence as the MIB destruct spawn / SPARK noise wiggle, so all
    // visual subsystems share one timeline). See FPS unlock issue #19.
    {
        constexpr SLONG SPARK_TICK_INTERVAL_MS = 67;  // ~15 Hz visual cadence
        static UBYTE last_spark_phase = 0;
        UBYTE cur_spark_phase = UBYTE(sdl3_get_ticks() / SPARK_TICK_INTERVAL_MS);
        if (cur_spark_phase != last_spark_phase) {
            last_spark_phase = cur_spark_phase;
            SPARK_process();
        }
    }
    GLITTER_process();
    //	LIGHT_process();
    HOOK_process();
    //	SM_process();
    SNIPE_process();

    // console processing

    static BOOL is_inputing = 0;
    extern UBYTE InkeyToAscii[];
    extern UBYTE InkeyToAsciiShift[];
    extern void CONSOLE_status(CBYTE * msg);

    if (is_inputing) {
        static CBYTE input_text[MAX_PATH] = "] ";

        const bool console_esc   = input_key_just_pressed(ACT_CONS_CANCEL_KKEY);
        const bool console_enter = input_key_just_pressed(ACT_CONS_SUBMIT_KKEY);
        if (console_esc || console_enter) {
            if (console_enter)
                parse_console(input_text + 2); // +2 to skip the "] "
            is_inputing = 0;
            CONSOLE_status("");
            strcpy(input_text, "] ");
        } else {

            POLY_frame_init(UC_FALSE, UC_FALSE);

            const UBYTE last_key = input_last_key();
            if (last_key) {
                UWORD len = strlen(input_text);
                CBYTE key;
                // ShiftFlag is the level-state Shift modifier mirrored from
                // input_frame's event-tracked KKEY_LEFT_SHIFT || KKEY_RIGHT_SHIFT.
                key = ShiftFlag ? InkeyToAsciiShift[last_key] : InkeyToAscii[last_key];
                if (key == 8) {
                    if (len > 2)
                        input_text[len - 1] = 0;
                } else {
                    if (((key >= 'A') && (key <= 'Z')) || ((key >= 'a') && (key <= 'z')) || ((key >= '0') && (key <= '9')) || (key == ' ')) {
                        input_text[len] = key;
                        input_text[len + 1] = 0;
                    }
                }
                input_last_key_consume();
            }

            CONSOLE_status(input_text);
            POLY_frame_draw(UC_FALSE, UC_FALSE);
        }
        return;
    } else {
        // F9 opens the dev console from gameplay. Same scancode in both foot
        // and car contexts — read both ACT names so the constant change in
        // either context would be picked up correctly.
        if (input_key_just_pressed(ACT_FOOT_OPEN_DEV_CONSOLE_KKEY)
            || input_key_just_pressed(ACT_CAR_OPEN_DEV_CONSOLE_KKEY))
            is_inputing = 1;
    }

    // normal key processing

    //
    // Do keyboard inventory.
    //

    extern Form* form_leave_map;
    extern SLONG can_darci_change_weapon(Thing * p_person);

    if ((!(GAME_FLAGS & GF_PAUSED) && !form_leave_map) && can_darci_change_weapon(darci) && input_gameplay_enabled()
        && !(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING)) { // no weapon switching while driving
        Thing* the_player = NET_PLAYER(0);

        //		if (can_darci_change_weapon(darci))
        {

            // OpenChaos weapon scheme. Edge-detect via input_frame (render-frame
            // snapshot) — Player->Pressed only refreshes per physics tick, so at
            // high render rate it would re-fire across frames (issue #20).
            //
            //   R3 (gamepad) / middle-mouse (KBM): disarm ↔ last weapon.
            //   Mouse wheel / R3 + D-pad ↑↓        : scroll the inventory popup.
            //   D-pad (no R3): ↑ pistol, ← AK, → shotgun, ↓ melee, ↑+→ grenade.
            //   Keyboard 1-4: pistol / AK / shotgun / grenade. Tab: melee.

            // Per-frame: remember the current non-fist weapon for the disarm/
            // re-arm toggle and the empty-hand scroll start.
            {
                const SLONG cur_t = dpad_current_weapon_type(darci);
                if (cur_t != SPECIAL_NONE)
                    g_weapon_last_type = cur_t;
            }

            const bool r3_held = input_btn_held(ACT_FOOT_INVENTORY_GBTN);

            // R3 doubles as a scroll modifier. Track whether a D-pad ↑/↓ scroll
            // happened during this hold — if so, the R3 RELEASE does NOT fire the
            // disarm toggle (the press was "used" for scrolling). Middle-mouse
            // has no such conflict (wheel is independent) → it fires on press.
            static bool s_r3_scroll_used = false;
            static bool s_r3_press_armed = false; // had a weapon in hand at R3 press
            if (input_btn_just_pressed(ACT_FOOT_INVENTORY_GBTN)) {
                s_r3_scroll_used = false;
                s_r3_press_armed = (dpad_current_weapon_type(darci) != SPECIAL_NONE);
                if (s_r3_press_armed) {
                    // Weapon in hand: just show the popup (current highlighted).
                    // The disarm happens on RELEASE (so you can scroll instead).
                    controls_show_popup(darci, the_player);
                } else {
                    // Fists: arm the last weapon immediately so the player sees it
                    // selected and can then scroll from there.
                    weapon_arm_last(darci, the_player);
                }
            }

            const bool dpad_up    = input_btn_just_pressed(ACT_FOOT_WEAPON_PISTOL_GBTN);
            const bool dpad_left  = input_btn_just_pressed(ACT_FOOT_WEAPON_AK47_GBTN);
            const bool dpad_right = input_btn_just_pressed(ACT_FOOT_WEAPON_SHOTGUN_GBTN);
            const bool dpad_down  = input_btn_just_pressed(ACT_FOOT_WEAPON_MELEE_CYCLE_GBTN);

            if (r3_held) {
                // R3 held: D-pad ↑/↓ AND left-stick ↑/↓ scroll the inventory;
                // other D-pad ignored. Stick uses the virtual-direction edge so
                // one push = one step (like the D-pad).
                const bool scroll_up =
                    dpad_up || input_stick_just_pressed(GAXIS_LEFT, GDIR_UP);
                const bool scroll_down =
                    dpad_down || input_stick_just_pressed(GAXIS_LEFT, GDIR_DOWN);
                if (scroll_up)   { weapon_scroll(darci, the_player, -1); s_r3_scroll_used = true; }
                if (scroll_down) { weapon_scroll(darci, the_player, +1); s_r3_scroll_used = true; }
            } else {
                // R3 not held: D-pad direct select + grenade on ↑+→ diagonal.
                // Skipped while the cheat modifier (Select + L1 + L2) is held —
                // the D-pad is the cheat-direction selector then.
                const bool cheat_combo_active =
                    input_btn_held(ACT_FOOT_CHEAT_MOD_SELECT_GBTN)
                    && input_btn_held(ACT_FOOT_CHEAT_MOD_L1_GBTN)
                    && input_btn_held(ACT_FOOT_CHEAT_MOD_L2_BTN_GBTN);

                static bool s_diag_grenade_armed = false;
                const bool up_held    = input_btn_held(ACT_FOOT_WEAPON_PISTOL_GBTN);
                const bool right_held = input_btn_held(ACT_FOOT_WEAPON_SHOTGUN_GBTN);

                if (cheat_combo_active) {
                    s_diag_grenade_armed = false;
                } else if (up_held && right_held) {
                    // ↑+→ together = grenade; fire once per diagonal engagement.
                    if (!s_diag_grenade_armed) {
                        controls_equip(darci, the_player, SPECIAL_GRENADE);
                        s_diag_grenade_armed = true;
                    }
                } else {
                    s_diag_grenade_armed = false;
                    if (dpad_up)         controls_equip(darci, the_player, SPECIAL_GUN);
                    else if (dpad_left)  controls_equip(darci, the_player, SPECIAL_AK47);
                    else if (dpad_right) controls_equip(darci, the_player, SPECIAL_SHOTGUN);
                    else if (dpad_down)  controls_dpad_select_melee(darci, the_player);
                }
            }

            // R3 release → disarm to fists, but ONLY if a weapon was in hand at
            // press and nothing was scrolled during the hold. (Releasing after a
            // press from fists keeps the weapon armed on press; scrolling means
            // the player was browsing, not toggling.)
            if (input_btn_just_released(ACT_FOOT_INVENTORY_GBTN) && !s_r3_scroll_used && s_r3_press_armed)
                controls_equip(darci, the_player, SPECIAL_NONE); // disarm

            // Keyboard direct select (1-4) + Tab melee toggle.
            if (input_key_just_pressed(ACT_FOOT_WEAPON_PISTOL_KKEY))
                controls_equip(darci, the_player, SPECIAL_GUN);
            if (input_key_just_pressed(ACT_FOOT_WEAPON_AK47_KKEY))
                controls_equip(darci, the_player, SPECIAL_AK47);
            if (input_key_just_pressed(ACT_FOOT_WEAPON_SHOTGUN_KKEY))
                controls_equip(darci, the_player, SPECIAL_SHOTGUN);
            if (input_key_just_pressed(ACT_FOOT_WEAPON_GRENADE_KKEY))
                controls_equip(darci, the_player, SPECIAL_GRENADE);
            if (input_key_just_pressed(ACT_FOOT_WEAPON_MELEE_KKEY))
                controls_dpad_select_melee(darci, the_player);

            // Middle mouse = disarm ↔ last weapon (KBM counterpart of R3 click).
            if (input_mouse_btn_just_pressed(ACT_FOOT_WEAPON_DISARM_MBTN))
                weapon_disarm_or_last(darci, the_player);

            // Mouse wheel = scroll inventory. Wheel up (dy>0) = previous,
            // wheel down = next. One step per notch.
            {
                const SLONG wheel = input_mouse_wheel_consume();
                const SBYTE d = (wheel > 0) ? (SBYTE)-1 : (SBYTE)+1;
                for (SLONG i = 0, n = (wheel < 0 ? -wheel : wheel); i < n; ++i)
                    weapon_scroll(darci, the_player, d);
            }

            // Keep the popup open as long as R3 (or middle mouse) is held — the
            // wheel stays up while browsing, then lingers normally after release.
            if (r3_held || input_mouse_btn_held(ACT_FOOT_WEAPON_DISARM_MBTN))
                CONTROLS_inventory_mode = 3000;

            if (CONTROLS_inventory_mode) {
                //				Keys[KKEY_ENTER] = 0;

                // Tick down the panel display in wall-clock milliseconds.
                // process_controls runs per render frame, but TICK_TOCK is
                // the per-physics-tick interval (~50 ms at default 20 Hz,
                // saturated to 25..100 ms range) — using TICK_TOCK here gave
                // a per-frame decrement that scaled with render rate (more
                // FPS → faster popup disappear) AND with physics rate (lower
                // physics Hz → larger TICK_TOCK due to saturation → faster
                // disappear). Wall-clock delta keeps the 3000 ms timeout
                // identical regardless of either rate.
                CONTROLS_inventory_mode -= (SLONG)g_frame_dt_ms;
                if (CONTROLS_inventory_mode < 0)
                    CONTROLS_inventory_mode = 0;

                //
                // does the fade in
                //
                CONTROLS_new_inventory(darci, the_player);

                // OpenChaos: the original "Shift (while the wheel is open) =
                // drop current weapon" branch was removed. It was triple-broken:
                // (1) the ground-drop was dead — set_person_gun_away() clears
                // GUN_OUT/SpecialUse before drop_current_gun() runs, so nothing
                // ever spawned; it only holstered to empty hands; (2) it ran
                // every frame while Shift was held, re-posing Darci → freeze;
                // (3) it skipped the fade-in, leaving a near-invisible wheel.
                // In our layout Shift = ACTION (sprint), so "sprint + open
                // wheel" hit it constantly. Holster/unequip belongs on its own
                // input (gamepad: D-pad down; keyboard: TODO mirror the pad).
            }
        }

        //		if (the_player->Genus.Player->PopupFade&&!(NET_PLAYER(0)->Genus.Player->ThisInput & INPUT_MASK_SELECT)) {

        if (the_player->Genus.Player->PopupFade && !CONTROLS_inventory_mode) {
            // Wall-clock fade-out. Same rationale as the fade-in above —
            // raw `-= INVENTORY_FADE_SPEED` per render frame finished too
            // fast on high FPS (felt abrupt). Scale by frame_dt / design
            // tick (20 Hz). Static float accumulator preserves sub-unit
            // precision at non-integer ratios. Underflow protection on
            // UBYTE PopupFade (raw `-=` could wrap below 0).
            constexpr float DESIGN_TICK_MS = 1000.0f / float(UC_PHYSICS_DESIGN_HZ);
            static float fade_out_accum = 0.0f;
            const float fade_step_f = (float)INVENTORY_FADE_SPEED * g_frame_dt_ms / DESIGN_TICK_MS
                + fade_out_accum;
            const SLONG fade_step_int = (SLONG)fade_step_f;
            fade_out_accum = fade_step_f - (float)fade_step_int;

            const SLONG new_fade = (SLONG)the_player->Genus.Player->PopupFade - fade_step_int;
            the_player->Genus.Player->PopupFade = (UBYTE)(new_fade > 0 ? new_fade : 0);
            if (new_fade <= 0) fade_out_accum = 0.0f; // reset on hitting zero

            if (the_player->Genus.Player->ItemFocus > -1) {
                //				CONTROLS_set_inventory(darci,the_player);
                the_player->Genus.Player->ItemFocus = -1;
            }
        }
    }

    // F1-modifier gate: debug-mode hotkeys below only fire while F1 is held
    // (in addition to allow_debug_keys being on). Returns when modifier is
    // NOT active — same effect as the previous `if (!allow_debug_keys)` but
    // adds the F1-hold requirement that suppresses conflicts with WASD /
    // E / etc. See input_frame::input_debug_modifier_active().
    if (!input_debug_modifier_active())
        return;

    // Shift+F12: toggle cheat mode (prints FPS in the top-left corner).
    // Gated along with the rest of the debug keys.
    if (input_key_just_pressed(ACT_BANG_TOGGLE_CHEAT_OVERLAY_KKEY) && ShiftFlag) {
        extern UBYTE cheat;

        if (cheat) {
            cheat = 0;
        } else {
            cheat = 2;
        }
    }

    // Combat-testing harness (OpenChaos). '-' fewer enemies, '=' more,
    // '\' cycle armament tier. update() keeps the wave topped up.
    if (input_key_just_pressed(ACT_BANG_COMBAT_TEST_INC_KKEY))
        combat_test_inc();
    if (input_key_just_pressed(ACT_BANG_COMBAT_TEST_DEC_KKEY))
        combat_test_dec();
    if (input_key_just_pressed(ACT_BANG_COMBAT_TEST_CYCLE_ARMAMENT_KKEY))
        combat_test_cycle_armament();
    combat_test_update();

    if (input_key_just_pressed(ACT_BANG_SAVE_GAME_KKEY)) {
        void save_whole_game(CBYTE * gamename);
        void load_whole_game(CBYTE * gamename);

        if (ShiftFlag) {
            save_whole_game("poo.sav");
        } else {
            load_whole_game("poo.sav");
        }
    }

    // Cloud toggle was on F11 historically but clashed with the input
    // debug panel (F11 → panel opens, covers the screen, making it
    // impossible to actually see whether clouds disappeared). Moved to
    // F4 — previously unbound.
    if (input_key_just_pressed(ACT_BANG_TOGGLE_CLOUDS_KKEY)) {
        if (aeng_draw_cloud_flag) {
            aeng_draw_cloud_flag = 0;
            CONSOLE_text("clouds off");
        } else {
            aeng_draw_cloud_flag = 1;
            CONSOLE_text("clouds on");
        }
    }

    WARE_debug();

    if (ControlFlag && ShiftFlag) {

        if (yomp_speed < 10)
            yomp_speed = 10;

        if (sprint_speed > 120)
            sprint_speed = 120;

        if (yomp_speed > 120)
            yomp_speed = 120;

        if (yomp_speed > sprint_speed)
            sprint_speed = yomp_speed;
        return;
    }

    if (input_key_just_pressed(ACT_BANG_TOGGLE_DETAIL_LEVEL_KKEY)) {
        if (DETAIL_LEVEL)
            DETAIL_LEVEL = 0;
        else
            DETAIL_LEVEL = 0xffff;
    }

    //
    // Mark's stuff.
    //

    //	Thing *darci = NET_PERSON(0);

    void set_person_idle(Thing * p_person);

    if (input_key_just_pressed(ACT_BANG_QUICK_SAVE_GAME_KKEY)) {
        void save_whole_game(CBYTE * gamename);
        save_whole_game("save.me");
    }

    //	TRACE("danger numbers: %d\n",(NET_PLAYER(0)->Genus.Player->Danger));

    if (!ShiftFlag) {
        // J / I — debug overlay draws (MAV, WAND). Drawn every frame the key
        // is held (continuous overlay), so input_key_held — not just_pressed.
        if (input_key_held(ACT_BANG_DRAW_MAV_OVERLAY_KKEY)) {
            SLONG mx = darci->WorldPos.X >> 16;
            SLONG mz = darci->WorldPos.Z >> 16;

            MAV_draw(
                mx - 5, mz - 5,
                mx + 5, mz + 5);
        }

        if (input_key_held(ACT_BANG_DRAW_WAND_OVERLAY_KKEY)) {
            SLONG mx = darci->WorldPos.X >> 16;
            SLONG mz = darci->WorldPos.Z >> 16;

            WAND_draw(mx, mz);
        }
    }

    if (input_key_just_pressed(ACT_BANG_SPAWN_VEHICLE_KKEY)) {
        SLONG y;
        SLONG index;

        static UBYTE type = 0;

        y = PAP_calc_height_at(darci->WorldPos.X >> 8, darci->WorldPos.Z >> 8);
        index = VEH_create((darci->WorldPos.X) + (400 << 8), (y + 65) << 8, (darci->WorldPos.Z), 0, 0, 0, type, 0, 0);

        WMOVE_create(TO_THING(index));

        type += 1;

        if (type == VEH_TYPE_NUMBER) {
            type = 0;
        }
    }

    //	CLOTH_process();

    //
    // Enter and leave the sewers if Darci does.
    //

    // KKEY_W: continuous water particle spawn while held (no consume in
    // original) — input_key_held, not just_pressed.
    if (input_key_held(ACT_BANG_SPAWN_WATER_KKEY)) {
        SLONG px = darci->WorldPos.X >> 8;
        SLONG py = darci->WorldPos.Y >> 8;
        SLONG pz = darci->WorldPos.Z >> 8;

        px += 0x80;
        py += 0x10;
        pz += 0x80;

        if (ControlFlag) {
            DIRT_new_water(px, py, pz, 1 + (SIN(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12), 14, (COS(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12));
            DIRT_new_water(px, py, pz, 1 + (SIN(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12), 14, -1 + (COS(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12));
            DIRT_new_water(px, py, pz, 1 + (SIN(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12), 14, 1 + (COS(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12));
            DIRT_new_water(px, py, pz, -1 + (SIN(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12), 14, (COS(darci->Draw.Tweened->Angle + ((VISUAL_TURN * 7) & 15)) >> 12));
        } else {
            DIRT_new_water(px + 2, py, pz, -1, 28, 0);
            DIRT_new_water(px, py, pz + 2, 0, 29, -1);
            DIRT_new_water(px, py, pz - 2, 0, 28, +1);
            DIRT_new_water(px - 2, py, pz, +1, 29, 0);
        }
    }

    if (GAME_FLAGS & GF_INDOORS) {
    }

    //
    // Tell the ID module where the player is, so that
    // it can draw the correct rooms.
    //

    //	ID_this_is_where_i_am((darci->WorldPos.X>>8) >> ELE_SHIFT, (darci->WorldPos.Z>>8) >> ELE_SHIFT);

    // pyrotest
    {
        static SLONG ribbon_id = -1;
        static UBYTE which_pyro = 0;
        GameCoord posn;

        if (input_key_just_pressed(ACT_BANG_PYRO_CYCLE_TYPE_KKEY)) {
            CBYTE* names[] = { "flicker", "ribbon", "explosion", "sparklies", "bonfire", "immolate", "testrib", "firewall", "new sploje", "new dome", "whoomph" };
            which_pyro++;
            if (which_pyro == (sizeof(names) / sizeof(names[0])))
                which_pyro = 0;
            CONSOLE_text(names[which_pyro], 1500);
        }

        if (input_key_just_pressed(ACT_BANG_PYRO_SPAWN_KKEY)) {
            static UBYTE line = 0;
            static GameCoord oldposn = { 0, 0, 0 };
            Thing* pyro;

            posn.X = darci->WorldPos.X;
            posn.Z = darci->WorldPos.Z;
            posn.Y = (PAP_calc_height_at(posn.X >> 8, posn.Z >> 8) * 256);

            switch (which_pyro) {
            case 0:
                posn.X -= 32000;
                PYRO_create(posn, PYRO_FLICKER);
                break;
            case 1:
                if (ribbon_id == -1)
                    ribbon_id = RIBBON_alloc(RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE | RIBBON_FLAG_CONVECT, 20, POLY_PAGE_FLAMES3, -1, 6, 4);
                break;
            case 2:
                PYRO_construct(posn, 14, 256);
                PYRO_construct(posn, 1, 400);
                posn.X += rand() >> 1;
                posn.Z += rand() >> 1;
                PYRO_construct(posn, 2, 96 + (rand() & 0x3f));
                posn.X = darci->WorldPos.X;
                posn.Z = darci->WorldPos.Z;
                posn.X -= rand() >> 1;
                posn.Z -= rand() >> 1;
                PYRO_construct(posn, 2, 96 + (rand() & 0x3f));

                PCOM_oscillate_tympanum(
                    PCOM_SOUND_BANG,
                    darci,
                    posn.X >> 8,
                    posn.Y >> 8,
                    posn.Z >> 8);

                break;
            case 3:
                PYRO_construct(posn, 1, 400);
                break;
            case 4:
                posn.X -= 32000;
                PYRO_create(posn, PYRO_BONFIRE);
                break;
            case 5:
                pyro = PYRO_create(posn, PYRO_IMMOLATE);
                pyro->Genus.Pyro->victim = darci;
                pyro->Genus.Pyro->Flags = PYRO_FLAGS_FLICKER;
                darci->Genus.Person->BurnIndex = PYRO_NUMBER(pyro->Genus.Pyro) + 1;
                break;
            case 6:
                posn.X -= 32000;
                pyro = PYRO_create(posn, PYRO_FLICKER);
                extern void PYRO_fn_init(Thing * thing);
                PYRO_fn_init(pyro); // heh heh heh
                pyro->Genus.Pyro->radii[0] = 128;
                pyro->Genus.Pyro->radii[1] = 400;
                pyro->Genus.Pyro->radii[2] = 256;
                break;
            case 7:
                if (line) {
                    pyro = PYRO_create(oldposn, PYRO_FIREWALL);
                    pyro->Genus.Pyro->target = posn;
                    line = 0;
                } else {
                    oldposn = posn;
                    line = 1;
                }
                break;

            case 8:
                // pyro=PYRO_create(posn,PYRO_SPLATTERY);
                pyro = PYRO_create(posn, PYRO_FIREBOMB);
                break;

            case 9:
                pyro = PYRO_create(posn, PYRO_NEWDOME);
                if (pyro) {
                    pyro->Genus.Pyro->scale = 400;
                }
                break;

            case 10:
                posn.X -= 32000;
                PYRO_create(posn, PYRO_WHOOMPH);
                break;
            }
        }
        if (ribbon_id != -1) {
            static int ribbon_tick = 0;
            ribbon_tick++;
            if (ribbon_tick & 1) {
                SLONG dx, dz, ang;
                ang = (-darci->Draw.Tweened->Angle) & 2047;
                dx = COS(ang) / 8;
                dz = SIN(ang) / 8;
                posn.X = darci->WorldPos.X;
                posn.Z = darci->WorldPos.Z;
                //			posn.Y=(PAP_calc_height_at(posn.X>>8,posn.Z>>8)*256)+0x3000;
                posn.Y = darci->WorldPos.Y + 0x3000;
                posn.X -= dx;
                posn.Z -= dz;
                RIBBON_extend(ribbon_id, posn.X >> 8, posn.Y / 256, posn.Z >> 8);
                posn.X += dx * 2;
                posn.Z += dz * 2;
                RIBBON_extend(ribbon_id, posn.X >> 8, posn.Y / 256, posn.Z >> 8);
            }
        }
    }

    {
        static UBYTE dlight = NULL;

        if (input_key_just_pressed(ACT_BANG_TOGGLE_DIRECTIONAL_LIGHT_KKEY)) {
            if (dlight) {
                NIGHT_dlight_destroy(dlight);

                dlight = NULL;
            } else {
                dlight = NIGHT_dlight_create(
                    (darci->WorldPos.X >> 8),
                    (darci->WorldPos.Y >> 8) + 0x80,
                    (darci->WorldPos.Z >> 8),
                    200,
                    40,
                    30,
                    20);
            }
        }

        if (dlight) {
            NIGHT_dlight_move(
                dlight,
                (darci->WorldPos.X >> 8),
                (darci->WorldPos.Y >> 8) + 0x80,
                (darci->WorldPos.Z >> 8));

            if (VISUAL_TURN & 0x20) {
                NIGHT_dlight_colour(
                    dlight,
                    60, 20, 20);
            } else {
                NIGHT_dlight_colour(
                    dlight,
                    20, 20, 60);
            }
        }
    }

    if (input_key_just_pressed(ACT_BANG_IMMOLATE_CHOPPER_KKEY)) {
        // this is to test immolation of a thing
        if (TO_CHOPPER(1)->ChopperType != CHOPPER_NONE) {
            Thing* pyro;
            GameCoord anyoldposn; // dont care, updated by thing

            pyro = PYRO_create(anyoldposn, PYRO_IMMOLATE);
            pyro->Genus.Pyro->victim = TO_CHOPPER(1)->thing;
            //			pyro->Genus.Pyro->Flags=PYRO_FLAGS_FACES; // immolate faces
            pyro->Genus.Pyro->Flags = PYRO_FLAGS_FLICKER;
        }
    }
    if (input_key_just_pressed(ACT_BANG_SPAWN_FIREPOOL_KKEY)) {
        static UBYTE line = 0;
        static GameCoord oldposn = { 0, 0, 0 };
        Thing* pyro;
        GameCoord posn;

        if (line) {
            posn.X = darci->WorldPos.X;
            posn.Z = darci->WorldPos.Z;
            //			posn.Y=(PAP_calc_map_height_at(posn.X>>8,posn.Z>>8)<<8)+0x3000;
            posn.Y = (PAP_calc_map_height_at(posn.X >> 8, posn.Z >> 8)) + 0x3000;
            pyro = PYRO_create(oldposn, PYRO_FIREPOOL);
            pyro->Genus.Pyro->target = posn;
            posn.X -= oldposn.X;
            posn.Z -= oldposn.Z;
            posn.X >>= 8;
            posn.Z >>= 9;
            pyro->Genus.Pyro->radius = Root(posn.X * posn.X + posn.Z * posn.Z);
            //			pyro->Genus.Pyro->Flags|=PYRO_FLAGS_TOUCHPOINT;
            line = 0;
        } else {
            oldposn.X = darci->WorldPos.X;
            oldposn.Z = darci->WorldPos.Z;
            //			oldposn.Y=(PAP_calc_map_height_at(oldposn.X>>8,posn.Z>>8)<<8)+0x3000;
            oldposn.Y = (PAP_calc_map_height_at(oldposn.X >> 8, oldposn.Z >> 8)) + 0x3000;
            line = 1;
        }
    }
    static UBYTE smokin = 0;
    if (input_key_just_pressed(ACT_BANG_TOGGLE_STEALTH_DEBUG_KKEY)) {
        stealth_debug = !stealth_debug;
        if (stealth_debug)
            CONSOLE_text("STEALTH DEBUG MODE ON");
        else
            CONSOLE_text("STEALTH DEBUG MODE OFF");
    }

    // KKEY_PERIOD: continuous smoke spawn while held — no consume in original
    // (level read fires every frame). input_key_held preserves that.
    if (input_key_held(ACT_BANG_SPAWN_SMOKE_KKEY)) {
        PARTICLE_Add(
            darci->WorldPos.X,
            darci->WorldPos.Y + 0x4000,
            darci->WorldPos.Z,
            256 + (Random() & 0xff),
            256 + (Random() & 0xff),
            256 + (Random() & 0xff),
            POLY_PAGE_SMOKECLOUD2,
            2,
            0x7fffffff,
            PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE,
            100,
            40,
            1,
            1,
            1);
    }

    //	if (smokin) psystem.Add(darci->WorldPos.X,darci->WorldPos.Y+0x100,darci->WorldPos.Z,0,0,0,POLY_PAGE_FOG,0x7FFFFFFF,PFLAGS_SMOKE,128,2,1);
    if (smokin) {
        //		if (!psystem.Add(darci->WorldPos.X,darci->WorldPos.Y+0x100,darci->WorldPos.Z,0,0,0,POLY_PAGE_STEAM,0,0x7FFFFFFF,PFLAGS_SMOKE|PFLAG_WANDER,128,4,1,1,3))
        //		if (!psystem.Add(darci->WorldPos.X,darci->WorldPos.Y+0x300,darci->WorldPos.Z,0,0,0,POLY_PAGE_STEAM,1+((rand()&3)<<2),0x7FFFFFFF,PFLAGS_SMOKE|PFLAG_WANDER,128,4,1,2,3))
        //		if (!psystem.Exhaust(darci->WorldPos.X,darci->WorldPos.Y+0x300,darci->WorldPos.Z,2,10))
    }

    static SLONG ma_valid = 0;
    static MAV_Action ma = { 0, 0, 0, 0 };
    {
        // e_draw_3d_mapwho_y(nav_x, MAV_height[nav_x][nav_z] << 6, nav_z);

        if (ma_valid) {
            SLONG x1 = darci->WorldPos.X >> 8;
            SLONG y1 = darci->WorldPos.Y >> 8;
            SLONG z1 = darci->WorldPos.Z >> 8;

            SLONG x2 = (ma.dest_x << 8) + 0x80;
            SLONG y2 = MAVHEIGHT(ma.dest_x, ma.dest_z) << 6; // LAV_height[ma.dest_x][ma.dest_z] << 6;
            SLONG z2 = (ma.dest_z << 8) + 0x80;

            AENG_world_line(
                x1, y1, z1, 32, 0x00ffff00,
                x2, y2, z2, 0, 0x000000ff,
                UC_TRUE);
        }
    }

    if (input_key_just_pressed(ACT_BANG_CREATE_OB_KKEY) && !ShiftFlag) {
        OB_create(
            darci->WorldPos.X >> 8,
            darci->WorldPos.Y >> 8,
            darci->WorldPos.Z >> 8,
            darci->Draw.Tweened->Angle,
            0,
            0,
            5, 0, 0, 0);
    }

    if (ShiftFlag) {
        static int skill = 0;
        if (input_key_just_pressed(ACT_BANG_SPAWN_FIGHT_THUG_KKEY)) {
            UWORD index;

            //
            // Create a fight test dummy.
            //

            index = PCOM_create_person(
                PERSON_THUG_RED, //(GAME_TURN&1)?PERSON_THUG:PERSON_SOLDIER,
                0,
                0,
                PCOM_AI_GANG,
                0,
                skill % 16,
                PCOM_MOVE_WANDER,
                0,
                PCOM_BENT_FIGHT_BACK,
                0,
                0,
                0,
                (darci->WorldPos.X) - (128 << 8) + ((Random() & 0xff) << 11),
                darci->WorldPos.Y,
                darci->WorldPos.Z - (128 << 8) + ((Random() & 0xff) << 11),
                0, 0);

            extern void PCOM_set_person_ai_kill_person(Thing * p_person, Thing * p_target, SLONG alert_gang);
            PCOM_set_person_ai_kill_person(TO_THING(index), NET_PERSON(0), 0);

            skill += 2;
        }

        if (input_key_just_pressed(ACT_BANG_SPAWN_BODYGUARD_KKEY)) {
            //
            // Create a bodyguard. First we must create a DUD waypoint that
            // pretends it created Darci!
            //

            SLONG waypoint = EWAY_find_or_create_waypoint_that_created_person(darci);

            PCOM_create_person(
                PERSON_HOSTAGE,
                0,
                0,
                PCOM_AI_BODYGUARD,
                waypoint,
                Random() % 16,
                PCOM_MOVE_FOLLOW,
                waypoint,
                0,
                0,
                0,
                0,
                darci->WorldPos.X + 0x4000,
                darci->WorldPos.Y,
                darci->WorldPos.Z,
                0,
                0, 0, 0); // FLAG2_PERSON_FAKE_WANDER);

            // remove_thing_from_map(darci);
        }

        if (input_key_just_pressed(ACT_BANG_DARCI_DANCE_KKEY)) {
            MAV_precalculate();

            switch (Random() % 3) {
            case 0:
                set_person_do_a_simple_anim(darci, ANIM_DANCE_BOOGIE);
                break;
            case 1:
                set_person_do_a_simple_anim(darci, ANIM_DANCE_WOOGIE);
                break;
            case 2:
                set_person_do_a_simple_anim(darci, ANIM_DANCE_HEADBANG);
                break;
            }

            darci->Genus.Person->Flags |= FLAG_PERSON_NO_RETURN_TO_NORMAL;
            darci->Genus.Person->Action = ACTION_SIT_BENCH;
        }

        // KKEY_Y: continuous fastnav debug overlay while held — no consume in
        // original (level read fires every frame). input_key_held preserves.
        if (input_key_held(ACT_BANG_DRAW_FASTNAV_KKEY)) {
            COLLIDE_debug_fastnav(
                darci->WorldPos.X >> 8,
                darci->WorldPos.Z >> 8);
        }

        // Note: this block is gated by `if (ShiftFlag)` above, so the
        // `&& !ShiftFlag` here is dead code (always false). Pre-existing
        // bug, preserved verbatim — not addressed by input migration.
        if (input_key_just_pressed(ACT_BANG_CREATE_NIGHT_SLIGHT_KKEY) && !ShiftFlag) {
            NIGHT_slight_create(
                (darci->WorldPos.X >> 8),
                (darci->WorldPos.Y >> 8) + 0x80,
                (darci->WorldPos.Z >> 8),
                200,
                30,
                40,
                50);

            NIGHT_cache_recalc();
            NIGHT_dfcache_recalc();
            NIGHT_generate_walkable_lighting();
        }

        if (input_key_just_pressed(ACT_BANG_DEBUGGER_BREAK_KKEY) && ShiftFlag) {
            //
            // shift D to leap into the debugger
            //
            ASSERT(2 + 2 == 5);
        }

        if (input_key_just_pressed(ACT_BANG_GIVE_GUN_KKEY)) {
            darci->Flags |= FLAGS_HAS_GUN;
        }

        if (input_key_just_pressed(ACT_BANG_PLAN_VIEW_SHOT_KKEY)) {
            plan_view_shot();
        }

        if (input_key_just_pressed(ACT_BANG_SPAWN_CHOPPER_KKEY)) {
            Thing* chopper;
            GameCoord posn;

            posn.X = darci->WorldPos.X;
            posn.Z = darci->WorldPos.Z;
            //			posn.Y=darci->WorldPos.Y+(265<<8)+(PAP_calc_map_height_at(posn.X>>8,posn.Z>>8)<<8);
            posn.Y = darci->WorldPos.Y + (265 << 8) + (PAP_calc_map_height_at(posn.X >> 8, posn.Z >> 8));
            chopper = CHOPPER_create(posn, CHOPPER_CIVILIAN);
            chopper->Draw.Mesh->Angle = darci->Draw.Tweened->Angle;
        }

        if (input_key_just_pressed(ACT_BANG_SPAWN_MINE_AT_MOUSE_KKEY)) {
            //
            // Create a mine at the mouse.
            //

            SLONG world_x;
            SLONG world_y;
            SLONG world_z;

            // MouseX/Y are scene-FBO pixels (Stage 6 FBO refactor) — scale
            // into the 640×480 virtual UI canvas that AENG_raytraced_position
            // expects, mirroring the KKEY_G "teleport Darci to mouse" handler
            // below. Without this, on non-native / non-4:3 windows the mine
            // spawns at the wrong world point.
            float hitx = float(input_mouse_x()) * float(DisplayWidth) / float(ScreenWidth);
            float hity = float(input_mouse_y()) * float(DisplayHeight) / float(ScreenHeight);

            AENG_raytraced_position(
                SLONG(hitx + 0.5f),
                SLONG(hity + 0.5f),
                &world_x,
                &world_y,
                &world_z);

            if (WITHIN(world_x, 0, (PAP_SIZE_HI << PAP_SHIFT_HI) - 1) && WITHIN(world_z, 0, (PAP_SIZE_HI << PAP_SHIFT_HI) - 1)) {
                world_y = PAP_calc_map_height_at(world_x, world_z);

                alloc_special(
                    SPECIAL_MINE,
                    SPECIAL_SUBSTATE_NONE,
                    world_x,
                    world_y,
                    world_z,
                    NULL);
            }
        }

    } else {
        //
        // Shift not held down.
        //

        if (input_key_just_pressed(ACT_BANG_SPAWN_ALL_WEAPONS_KKEY)) {
            SLONG wx, wy, wz, dx, dz;
            SLONG angle;

            wx = darci->WorldPos.X >> 8;
            wy = darci->WorldPos.Y >> 8;
            wz = darci->WorldPos.Z >> 8;

            darci->Genus.Person->Health = 9999;

            wy += 0x20;

            for (angle = 0; angle < 7; angle++) {
                dx = COS(angle * (2047 / 7)) >> 8;
                dz = SIN(angle * (2047 / 7)) >> 8;
                switch (angle) {
                case 0:
                    alloc_special(SPECIAL_HEALTH, SPECIAL_SUBSTATE_NONE, wx + dx, wy + 0x10, wz + dz, 0);
                    break;
                case 1:
                    alloc_special(SPECIAL_BASEBALLBAT, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;
                case 2:
                    alloc_special(SPECIAL_KNIFE, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;
                case 3:
                    alloc_special(SPECIAL_SHOTGUN, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;
                case 4:
                    alloc_special(SPECIAL_GRENADE, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;
                case 5:
                    alloc_special(SPECIAL_AK47, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;
                case 6:
                    alloc_special(SPECIAL_MINE, SPECIAL_SUBSTATE_NONE, wx + dx, wy, wz + dz, 0);
                    break;

                default:
                    ASSERT(0);
                    break;
                }
            }
        }

        if (input_key_just_pressed(ACT_BANG_TELEPORT_TO_MOUSE_KKEY)) {
            SLONG world_x;
            SLONG world_y;
            SLONG world_z;

            //
            // Teleport Darci to where the mouse is.
            //

            // MouseX/Y are in scene-FBO pixels after Stage 6 of the
            // FBO-as-virtual-screen refactor; scale into the 640×480
            // virtual UI canvas that AENG_raytraced_position expects.
            float hitx = float(input_mouse_x()) * float(DisplayWidth) / float(ScreenWidth);
            float hity = float(input_mouse_y()) * float(DisplayHeight) / float(ScreenHeight);

            AENG_raytraced_position(
                SLONG(hitx + 0.5f),
                SLONG(hity + 0.5f),
                //				MouseX,
                //				MouseY,
                &world_x,
                &world_y,
                &world_z);

            if (WITHIN(world_x, 600, (PAP_SIZE_HI << PAP_SHIFT_HI) - 601) && WITHIN(world_z, 600, (PAP_SIZE_HI << PAP_SHIFT_HI) - 601)) {
                world_y += 0x10;

                GameCoord teleport;

                teleport.X = world_x << 8;
                teleport.Y = world_y << 8;
                teleport.Z = world_z << 8;

                set_person_idle(darci);
                move_thing_on_map(darci, &teleport);

                SLONG plant_feet(Thing * p_person);
                plant_feet(darci);
                darci->Genus.Person->Flags &= ~(FLAG_PERSON_KO | FLAG_PERSON_HELPLESS);
            }
        }

        // KKEY_U / KKEY_Q: continuous debug effects while held (no consume in
        // original). input_key_held preserves level-read intent.
        if (input_key_held(ACT_BANG_HIT_BARREL_KKEY)) {
            BARREL_hit_with_sphere(
                darci->WorldPos.X >> 8,
                darci->WorldPos.Y >> 8,
                darci->WorldPos.Z >> 8,
                0x80);
        }

        if (input_key_held(ACT_BANG_DRAW_ROAD_DEBUG_KKEY)) {
            ROAD_debug();
        }
    }

    //
    // Animate the animating texture structure
    //
    animate_texture_maps();
    //	Get user game input.
}
