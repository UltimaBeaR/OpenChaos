// Chunks 1-3: globals, script helpers, screen management, draw helpers,
// kibble particle system, FRONTEND_kibble_process, FRONTEND_fetch_title_from_id,
// FRONTEND_display, FRONTEND_init, FRONTEND_loop, etc.

#include <sys/stat.h>
#include "engine/platform/uc_common.h"
#include "engine/platform/sdl3_bridge.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

// Display globals (defined in d3d/display_globals.cpp).
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;
#include "ui/frontend/frontend.h"
#include "ui/frontend/frontend_globals.h"

#include "assets/xlat_str.h"
#include "game/input_actions.h"
#include "game/input_actions_globals.h"
#include "engine/graphics/text/menufont.h"
#include "engine/graphics/text/font2d.h"
#include "engine/graphics/pipeline/polypage.h"  // PolyPage::UIModeScope
#include "engine/graphics/pipeline/polypage.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/core/fmatrix.h"
// DRAW2D_Box, DRAW2D_Tri migrated to draw2d.h (iteration 136).
#include "engine/graphics/pipeline/draw2d.h"
#include "assets/formats/level_loader.h"
#include "assets/formats/level_loader_globals.h"
#include "assets/formats/anim_loader.h"
#include "assets/formats/anim_loader_globals.h"
#include "engine/audio/music.h"
#include "game/game_types.h"

// Additional headers for chunks 2-3 (FRONTEND_display, FRONTEND_input, FRONTEND_init, FRONTEND_loop)
#include "engine/audio/music.h"               // MUSIC_gain, MUSIC_reset, MUSIC_mode_process, MUSIC_bodge_code
#include "engine/audio/mfx.h"                 // MFX_play_stereo, MFX_stop, MFX_set_volumes, etc.
#include "engine/audio/sound.h"               // WEATHER_REF, SIREN_REF
#include "assets/sound_id.h"                  // S_TUNE_BONUS, S_MENU_CLICK_START, etc.
#include "engine/input/keyboard_globals.h"    // Keys[], LastKey, ControlFlag, ShiftFlag
#include "engine/input/joystick.h"             // ReadInputDevice
#include "engine/input/joystick_globals.h"    // the_state (DIJOYSTATE)
#include "engine/graphics/text/font2d_globals.h" // FONT2D_leftmost_x, FONT2D_rightmost_x
#include "engine/graphics/text/menufont_globals.h" // FontPage
#include "game/input_actions.h"                       // get_hardware_input, INPUT_TYPE_JOY, INPUT_MASK_*
#include "game/input_actions_globals.h"              // g_bPunishMePleaseICheatedOnThisLevel
#include "ui/frontend/startscr_globals.h"          // STARTSCR_mission
#include "ui/frontend/startscr.h"                  // STARTS_START, STARTS_EXIT, STARTS_EDITOR
#include "map/supermap_globals.h"       // DONT_load
#include "game/game_tick_globals.h"              // allow_debug_keys
#include "game/game_globals.h"            // VIOLENCE


// uc_orig: RandStream (fallen/Source/frontend.cpp)
// Pseudo-random stream — used for kibble particle variety and title wibble.
// Seeds from a string character; returns a 16-bit value each call.
#define RandStream(s) ((UWORD)((s = ((s * 69069) + 1)) >> 7))

// ---- Script helpers --------------------------------------------------------

// uc_orig: CacheScriptInMemory (fallen/Source/frontend.cpp)
// Reads the entire mission script text file into loaded_in_script[].
void CacheScriptInMemory(CBYTE* script_fname)
{
    FILE* handle = MF_Fopen(script_fname, "rt");

    ASSERT(handle);

    if (handle) {
        int iLoaded = fread(loaded_in_script, 1, SCRIPT_MEMORY, handle);
        ASSERT(iLoaded < sizeof(loaded_in_script));
        loaded_in_script[iLoaded] = '\0';

        MF_Fclose(handle);
    }
}

// uc_orig: FileOpenScript (fallen/Source/frontend.cpp)
// Resets the read cursor to the start of the cached script.
void FileOpenScript(void)
{
    loaded_in_script_read_upto = loaded_in_script;
}

// uc_orig: LoadStringScript (fallen/Source/frontend.cpp)
// Reads the next line from the cached script buffer into txt.
// Stops on null byte (returns immediately) or newline (0x0a, breaks then null-terminates).
CBYTE* LoadStringScript(CBYTE* txt)
{
    CBYTE* ptr = txt;

    ASSERT(loaded_in_script_read_upto);

    *ptr = 0;
    while (1) {

        *ptr = *loaded_in_script_read_upto++;

        if (*ptr == 0) {
            return txt;
        };
        if (*ptr == 10)
            break;
        ptr++;
    }
    *(++ptr) = 0;
    return txt;
}

// uc_orig: FileCloseScript (fallen/Source/frontend.cpp)
void FileCloseScript(void)
{
    loaded_in_script_read_upto = NULL;
}

// ---- Screenfull surface management ----------------------------------------

// FRONTEND_scr_add and FRONTEND_scr_img_load_into_screenfull moved to
// ge_create_screen_surface / ge_load_screen_surface in the graphics engine.

// uc_orig: FRONTEND_scr_unload_theme (fallen/Source/frontend.cpp)
// Releases all four background DirectDraw surfaces.
void FRONTEND_scr_unload_theme()
{
    ge_set_background_override(GE_SCREEN_SURFACE_NONE);

    ge_destroy_screen_surface(screenfull_back);
    screenfull_back = GE_SCREEN_SURFACE_NONE;
    ge_destroy_screen_surface(screenfull_map);
    screenfull_map = GE_SCREEN_SURFACE_NONE;
    ge_destroy_screen_surface(screenfull_brief);
    screenfull_brief = GE_SCREEN_SURFACE_NONE;
    ge_destroy_screen_surface(screenfull_config);
    screenfull_config = GE_SCREEN_SURFACE_NONE;

    screenfull = GE_SCREEN_SURFACE_NONE;
}

// uc_orig: FRONTEND_scr_new_theme (fallen/Source/frontend.cpp)
// Loads a complete set of four theme backgrounds (back, map, brief, config).
// Preserves the currently-displayed surface across the reload.
void FRONTEND_scr_new_theme(
    CBYTE* fname_back,
    CBYTE* fname_map,
    CBYTE* fname_brief,
    CBYTE* fname_config)
{
    SLONG last = 1;

    stop_all_fx_and_music();

    GEScreenSurface active = ge_get_background_override();
    if (active == screenfull_back)   last = 1;
    if (active == screenfull_map)    last = 2;
    if (active == screenfull_brief)  last = 3;
    if (active == screenfull_config) last = 4;

    FRONTEND_scr_unload_theme();

    screenfull_back   = ge_load_screen_surface(fname_back);
    screenfull_map    = ge_load_screen_surface(fname_map);
    screenfull_brief  = ge_load_screen_surface(fname_brief);
    screenfull_config = ge_load_screen_surface(fname_config);

    switch (last) {
    case 1: ge_set_background_override(screenfull_back);   break;
    case 2: ge_set_background_override(screenfull_map);    break;
    case 3: ge_set_background_override(screenfull_brief);  break;
    case 4: ge_set_background_override(screenfull_config); break;
    default: ASSERT(UC_FALSE); break;
    }

    MUSIC_mode(MUSIC_MODE_FRONTEND);
}

// uc_orig: FRONTEND_restore_screenfull_surfaces (fallen/Source/frontend.cpp)
void FRONTEND_restore_screenfull_surfaces(void)
{
    FRONTEND_scr_new_theme(
        menu_back_names[menu_theme],
        menu_map_names[menu_theme],
        menu_brief_names[menu_theme],
        menu_config_names[menu_theme]);
}

// ---- Mission data parsing --------------------------------------------------

// uc_orig: FRONTEND_ParseMissionData (fallen/Source/frontend.cpp)
// Parses one line from the mission script file into a MissionData struct.
// Versions: 4 = ObjID:GroupID:ParentID:ParentIsGroup:Type:Flags:District:fn:title:brief
//           3 = ObjID:GroupID:ParentID:ParentIsGroup:Type:District:fn:title:brief
//           2 = ObjID:GroupID:ParentID:ParentIsGroup:Type:fn:*n:*n:title:brief
//     default = ObjID:GroupID:ParentID:ParentIsGroup:Type:fn:title:brief
// '@' chars in the brief text are converted to carriage returns.
void FRONTEND_ParseMissionData(CBYTE* text, CBYTE version, MissionData* mdata)
{
    UWORD a, n;
    switch (version) {
    case 2:
        sscanf(text, "%d : %d : %d : %d : %d : %s : *%d : %*d : %[^:] : %*s",
            &mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup,
            &mdata->Type, mdata->fn, mdata->ttl);
        mdata->Flags = 0;
        mdata->District = -1;
        n = 9;
        break;
    case 3:
        sscanf(text, "%d : %d : %d : %d : %d : %d : %s : %[^:] : %*s",
            &mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup,
            &mdata->Type, &mdata->District, mdata->fn, mdata->ttl);
        mdata->Flags = 0;
        n = 8;
        break;
    case 4:
        sscanf(text, "%d : %d : %d : %d : %d : %d : %d : %s : %[^:] : %*s",
            &mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup,
            &mdata->Type, &mdata->Flags, &mdata->District, mdata->fn, mdata->ttl);
        n = 9;
        break;
    default:
        sscanf(text, "%d : %d : %d : %d : %d : %s : %[^:] : %*s",
            &mdata->ObjID, &mdata->GroupID, &mdata->ParentID, &mdata->ParentIsGroup,
            &mdata->Type, mdata->fn, mdata->ttl);
        mdata->Flags = 0;
        mdata->District = -1;
        n = 7;
    }
    for (a = 0; a < n; a++) {
        text = strchr(text, ':') + 1;
    }
    strcpy(mdata->brief, text + 1);

    // Convert '@' to carriage return for multi-line briefing text.
    char* pch = mdata->brief;
    while (*pch != '\0') {
        if (*pch == '@') {
            *pch = '\r';
        }
        pch++;
    }

    text = mdata->brief + strlen(mdata->brief) - 2;
    if (*text == 13)
        *text = 0;
}

// uc_orig: FRONTEND_LoadString (fallen/Source/frontend.cpp)
// Reads one line (terminated by 0x0a) from a binary file handle.
CBYTE* FRONTEND_LoadString(MFFileHandle& file, CBYTE* txt)
{
    CBYTE* ptr = txt;

    *ptr = 0;
    while (1) {
        if (FileRead(file, ptr, 1) == FILE_READ_ERROR) {
            *ptr = 0;
            return txt;
        };
        if (*ptr == 10)
            break;
        ptr++;
    }
    *(++ptr) = 0;
    return txt;
}

// uc_orig: FRONTEND_SaveString (fallen/Source/frontend.cpp)
// Writes a string followed by CRLF to a binary file.
void FRONTEND_SaveString(MFFileHandle& file, CBYTE* txt)
{
    CBYTE crlf[] = { 13, 10 };

    FileWrite(file, txt, strlen(txt));
    FileWrite(file, crlf, 2);
}

// ---- Menu utility ----------------------------------------------------------

// uc_orig: FRONTEND_AlterAlpha (fallen/Source/frontend.cpp)
// Adjusts the alpha channel of an ARGB color:
// shifts the current alpha by 'shift' bits, then adds 'add', clamped to 0-255.
SLONG FRONTEND_AlterAlpha(SLONG rgb, SWORD add, SBYTE shift)
{
    SLONG alpha = rgb >> 24;
    alpha <<= shift;
    alpha += add;
    if (alpha > 0xff)
        alpha = 0xff;
    if (alpha < 0)
        alpha = 0;
    rgb &= 0xffffff;
    return rgb | (alpha << 24);
}

// uc_orig: FRONTEND_recenter_menu (fallen/Source/frontend.cpp)
// Vertically centers all menu items on screen.
// Items are spaced 50px apart; base is clamped to minimum 100.
void FRONTEND_recenter_menu(void)
{
    MenuData* md = menu_data;
    int iY = 0;
    for (int i = 0; i < menu_state.items; i++) {
        md->Y = iY;
        md++;
        iY += 50;
    }

    menu_state.base = (480 - menu_state.items * 50) >> 1;
    if (menu_state.base < 100) {
        menu_state.base = 100;
    }
    menu_state.scroll = 0;
}

// uc_orig: FRONTEND_fix_rgb (fallen/Source/frontend.cpp)
// Returns the current fade color; if sel is true, doubles the alpha (selected item highlight).
ULONG FRONTEND_fix_rgb(ULONG rgb, BOOL sel)
{
    rgb = fade_rgb;
    if (sel)
        rgb = FRONTEND_AlterAlpha(rgb, 0, 1);
    return rgb;
}

// ---- Draw helpers ----------------------------------------------------------

// uc_orig: FRONTEND_draw_title (fallen/Source/frontend.cpp)
// Draws a single-character-at-a-time title string using the menu bitmap font.
// 'wibble' = animated wobble mode (doubled brightness + jitter).
// 'r_to_l' = right-to-left clipping (for the slide-in transition effect).
void FRONTEND_draw_title(SLONG x, SLONG y, SLONG cutx, CBYTE* str, BOOL wibble, BOOL r_to_l)
{
    SLONG rgb = wibble ? (fade_rgb << 1) | 0xffffff : fade_rgb | 0xffffff;
    SLONG seed = *str;
    SWORD xo = 0, yo = 0;

    for (; *str; str++) {
        if (!wibble) {
            xo = (RandStream(seed) & 0x1f) - 0xf;
            yo = 4;
        }

        if (((r_to_l) && (x > cutx)) || ((!r_to_l) && (x < cutx))) {
            MENUFONT_Draw(x + xo, y + yo, BIG_FONT_SCALE + (wibble << 5), str, rgb, 0, 1);
        }
        x += (MENUFONT_CharWidth(*str, BIG_FONT_SCALE)) - 2;
    }
}

// uc_orig: FRONTEND_init_xition (fallen/Source/frontend.cpp)
// Initializes the screen transition effect (wipe/iris).
// Sets MidX/MidY/ScaleX/ScaleY and selects the appropriate screenfull surface.
// Coords are in framed-area pixel space (the wipe/iris animates the 4:3
// framed UI region, not the real screen — see ge_blit_surface_to_backbuffer).
void FRONTEND_init_xition(void)
{
    MidX = SLONG(ui_coords::g_frame_w_px) / 2;
    MidY = SLONG(ui_coords::g_frame_h_px) / 2;
    ScaleX = MidX / 64.0f;
    ScaleY = MidY / 64.0f;
    switch (menu_state.mode) {
    case FE_MAPSCREEN:
        screenfull = screenfull_map;
        break;
    case FE_MAINMENU:
        screenfull = screenfull_back;
        break;
    case FE_LOADSCREEN:
    case FE_SAVESCREEN:
    case FE_CONFIG:
        screenfull = screenfull_config;
        break;
    default:
        if (menu_state.mode >= 100) {
            screenfull = screenfull_brief;
        } else {
            screenfull = screenfull_config;
        }
    }
}

// uc_orig: FRONTEND_show_xition (fallen/Source/frontend.cpp)
// Blits the current transition surface into the back buffer as the wipe animates.
// Mission briefing screens use an iris (rect from center); others use a horizontal wipe.
void FRONTEND_show_xition()
{
    struct { SLONG left, top, right, bottom; } rc;

    bool bDoBlit = UC_FALSE;

    if (menu_state.mode >= 100) {
        rc.top = MidY - (fade_state * ScaleY);
        rc.bottom = MidY + (fade_state * ScaleY);
        rc.left = MidX - (fade_state * ScaleX);
        rc.right = MidX + (fade_state * ScaleX);
        if (rc.left < rc.right) {
            bDoBlit = UC_TRUE;
        }
    } else {
        // Horizontal wipe across the framed UI region (not real screen).
        const SLONG framed_w = SLONG(ui_coords::g_frame_w_px);
        const SLONG framed_h = SLONG(ui_coords::g_frame_h_px);
        rc.top = 0;
        rc.bottom = framed_h;
        rc.left = 0;
        rc.right = fade_state * 10 * framed_w / 640;
        if (rc.right > 0) {
            bDoBlit = UC_TRUE;
        }
    }

    if (bDoBlit) {
        ge_blit_surface_to_backbuffer(screenfull, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    }
}

// uc_orig: FRONTEND_stop_xition (fallen/Source/frontend.cpp)
// Finalizes a transition by making the full background surface active.
void FRONTEND_stop_xition()
{
    switch (menu_state.mode) {
    case FE_QUIT:
    case FE_MAINMENU:
        ge_set_background_override(screenfull_back);
        break;
    case FE_CONFIG:
    case FE_CONFIG_VIDEO:
    case FE_CONFIG_AUDIO:
    case FE_CONFIG_INPUT_KB:
    case FE_CONFIG_INPUT_JP:
    case FE_LOADSCREEN:
    case FE_SAVESCREEN:
        ge_set_background_override(screenfull_config);
        break;
    case FE_MAPSCREEN:
        ge_set_background_override(screenfull_map);
        break;
    default:
        if (menu_state.mode >= 100) {
            ge_set_background_override(screenfull_brief);
        } else {
            ge_set_background_override(screenfull_config);
        }
        break;
    }
}

// uc_orig: FRONTEND_draw_button (fallen/Source/frontend.cpp)
// Draws a controller button icon (quad) using the button texture atlas.
// Buttons 0-3 are 64x64 (big); 4-7 are 32x32 (tiny).
// 'flash' = highlight the button in the current tick's blink cycle.
void FRONTEND_draw_button(SLONG x, SLONG y, UBYTE which, UBYTE flash)
{
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    float u, v, w, h;
    UBYTE size = (which < 4) ? 64 : 32;
    UBYTE grow;

    if (flash) {
        grow = 8;

        if (sdl3_get_ticks() & 0x200) {
            which = 7;
        }
    } else {
        grow = 0;
    }

    switch (which) {
    case 0:
    case 4:
        u = 0.0;
        v = 0.0;
        w = 0.5;
        h = 0.5;
        break;
    case 1:
    case 5:
        u = 0.5;
        v = 0.0;
        w = 1.0;
        h = 0.5;
        break;
    case 2:
    case 6:
        u = 0.0;
        v = 0.5;
        w = 0.5;
        h = 1.0;
        break;
    case 3:
    case 7:
        u = 0.5;
        v = 0.5;
        w = 1.0;
        h = 1.0;
        break;
    default:
        u = v = 0.0;
        w = h = 1.0;
        break;
    }

    pp[0].colour = (which < 4) ? 0xffFFFFFF : (fade_rgb << 1) | 0xffffff;
    pp[0].specular = 0xff000000;
    pp[0].Z = 0.5;
    pp[1] = pp[2] = pp[3] = pp[0];

    pp[0].X = x - (grow >> 1);
    pp[0].Y = y - grow;
    pp[0].u = u;
    pp[0].v = v;

    pp[1].X = x + (grow >> 1) + size;
    pp[1].Y = y - grow;
    pp[1].u = w;
    pp[1].v = v;

    pp[2].X = x;
    pp[2].Y = y + size;
    pp[2].u = u;
    pp[2].v = h;

    pp[3].X = x + size;
    pp[3].Y = y + size;
    pp[3].u = w;
    pp[3].v = h;

    POLY_add_quad(quad, (which < 4) ? POLY_PAGE_BIG_BUTTONS : POLY_PAGE_TINY_BUTTONS, UC_FALSE, UC_TRUE);
}

// uc_orig: FRONTEND_kibble_draw (fallen/Source/frontend.cpp)
// Draws all active kibble (leaf/rain/snow) particles as rotated quads.
// Particles use parent's framed UI scope; the scissor that push_ui_mode
// installs clips them to the 4:3 region so they don't bleed onto bars.
void FRONTEND_kibble_draw()
{
    UWORD c0;
    Kibble* k;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    SLONG matrix[9], x, y, z;

    ASSERT(kibble != NULL);

    for (c0 = 0, k = kibble; c0 < 512; c0++, k++)
        if (k->type > 0) {

            pp[0].colour = (k->rgb) | 0xff000000;
            pp[0].specular = 0xff000000;
            pp[1] = pp[2] = pp[3] = pp[0];

            FMATRIX_calc(matrix, k->r, k->t, k->p);

            x = -k->size;
            y = -k->size;
            z = 0;
            FMATRIX_MUL(matrix, x, y, z);
            pp[0].X = (k->x >> 8) + x;
            pp[0].Y = (k->y >> 8) + y;
            pp[0].Z = KIBBLE_Z;
            pp[0].u = 0;
            pp[0].v = 0;

            x = +k->size;
            y = -k->size;
            z = 0;
            FMATRIX_MUL(matrix, x, y, z);
            pp[1].X = (k->x >> 8) + x;
            pp[1].Y = (k->y >> 8) + y;
            pp[1].Z = KIBBLE_Z;
            pp[1].u = 1;
            pp[1].v = 0;

            x = -k->size;
            y = +k->size;
            z = 0;
            FMATRIX_MUL(matrix, x, y, z);
            pp[2].X = (k->x >> 8) + x;
            pp[2].Y = (k->y >> 8) + y;
            pp[2].Z = KIBBLE_Z;
            pp[2].u = 0;
            pp[2].v = 1;

            x = +k->size;
            y = +k->size;
            z = 0;
            FMATRIX_MUL(matrix, x, y, z);
            pp[3].X = (k->x >> 8) + x;
            pp[3].Y = (k->y >> 8) + y;
            pp[3].Z = KIBBLE_Z;
            pp[3].u = 1;
            pp[3].v = 1;

            POLY_add_quad(quad, k->page, UC_FALSE, UC_TRUE);
        }
}

// uc_orig: FRONTEND_DrawSlider (fallen/Source/frontend.cpp)
// Draws a slider control (volume/etc.) as a horizontal bar with a thumb.
void FRONTEND_DrawSlider(MenuData* md)
{
    SLONG y;
    ULONG rgb = FRONTEND_fix_rgb(fade_rgb, 0);
    y = md->Y + menu_state.base - menu_state.scroll;
    DRAW2D_Box(320, y - 2, 610, y + 2, rgb, 0, 192);
    DRAW2D_Box(337, y - 4, 341, y + 4, rgb, 0, 192);
    DRAW2D_Box(337 + 255, y - 4, 341 + 255, y + 4, rgb, 0, 192);
    DRAW2D_Box(337 + (md->Data), y - 8, 341 + (md->Data), y + 8, rgb, 0, 192);
}

// uc_orig: FRONTEND_DrawMulti (fallen/Source/frontend.cpp)
// Draws a multi-choice item's current selection text, right-justified.
// Non-English builds use font2d instead of the bitmap menu font.
void FRONTEND_DrawMulti(MenuData* md, ULONG rgb)
{
    SLONG x, y, dy, c0;
    CBYTE* str;
    dy = md->Y + menu_state.base - menu_state.scroll;
    str = md->Choices;
    c0 = md->Data & 0xff;

    if (!str)
        return;

    while ((*str) && c0--) {
        str += strlen(str) + 1;
    }
    if (IsEnglish) {
        MENUFONT_Dimensions(str, x, y, -1, BIG_FONT_SCALE);
        if (320 + x > 630) {
            if (320 + (x >> 1) > 630) {
                c0 = MENUFONT_CharFit(str, 310, 128);
                MENUFONT_Draw(320, dy - 15, 128, str, rgb, 0, c0);
                MENUFONT_Draw(320, dy + 15, 128, str + c0, rgb, 0);
            } else {
                MENUFONT_Draw(620 - (x >> 1), dy, 128, str, rgb, 0);
            }
        } else {
            MENUFONT_Draw(620 - x, dy, BIG_FONT_SCALE, str, rgb, 0);
        }
    } else {
        rgb = FRONTEND_fix_rgb(fade_rgb, 1);
        FONT2D_DrawStringRightJustify(str, 620, dy, rgb, SMALL_FONT_SCALE + 64, POLY_PAGE_FONT2D);
    }
}

// uc_orig: FRONTEND_DrawKey (fallen/Source/frontend.cpp)
// Draws the name of the keyboard key assigned to a menu action.
// Draws the name of the keyboard key assigned to a menu action.
void FRONTEND_DrawKey(MenuData* md)
{
    SLONG x, y, dy, rgb;
    CBYTE str[25];
    rgb = FRONTEND_fix_rgb(fade_rgb, (grabbing_key && ((menu_data + menu_state.selected == md) && ((sdl3_get_ticks() & 0x7ff) < 0x3ff))));
    dy = md->Y + menu_state.base - menu_state.scroll;

    sdl3_get_key_name(md->Data, str, 25);

    if (IsEnglish) {
        MENUFONT_Dimensions(str, x, y, -1, BIG_FONT_SCALE);
        MENUFONT_Draw(620 - x, dy, BIG_FONT_SCALE, str, rgb, 0);
    } else {
        FONT2D_DrawStringRightJustify(str, 620, dy, rgb, SMALL_FONT_SCALE, POLY_PAGE_FONT2D);
    }
}

// uc_orig: FRONTEND_DrawPad (fallen/Source/frontend.cpp)
// Draws the name of the joypad button assigned to a menu action.
void FRONTEND_DrawPad(MenuData* md)
{
    SLONG x, y, dy, rgb;
    CBYTE str[20];
    rgb = FRONTEND_fix_rgb(fade_rgb, (grabbing_pad && ((menu_data + menu_state.selected == md) && ((sdl3_get_ticks() & 0x7ff) < 0x3ff))));
    dy = md->Y + menu_state.base - menu_state.scroll;
    if (md->Data < 31)
        sprintf(str, "%s %d", XLAT_str(X_BUTTON), md->Data);
    else
        strcpy(str, "Unused");
    MENUFONT_Dimensions(str, x, y, -1, BIG_FONT_SCALE);
    MENUFONT_Draw(620 - x, dy, BIG_FONT_SCALE, str, rgb, 0);
}

// ---- Kibble particle system ------------------------------------------------

// uc_orig: FRONTEND_kibble_init_one (fallen/Source/frontend.cpp)
// Initializes one kibble particle slot for the current theme.
// type: 1=leaf, 2=rain, 3=snow. Bit 7 = ignore map screen restriction.
// Kibble_off[] can suppress individual slots (for performance throttling).
void FRONTEND_kibble_init_one(Kibble* k, UBYTE type)
{
    SLONG kibble_index = k - kibble;

    ASSERT(kibble != NULL);

    ASSERT(WITHIN(kibble_index, 0, 511));

    if (kibble_off[kibble_index]) {
        return;
    }

    if (!(type & 128)) {
        if ((menu_state.mode == FE_MAPSCREEN) || (menu_state.mode >= 100))
            return;
    }
    switch (type & 0x7f) {
    case 1:
        k->dx = (20 + (Random() & 15)) << 5;
        k->dy = 0;
        k->page = POLY_PAGE_BIG_LEAF;
        k->rgb = FRONTEND_leaf_colours[Random() & 3];
        k->x = (-10 - (Random() & 0x1ff)) << 8;
        k->y = ((Random() % 520) - 20) << 8;
        k->size = 35 + (Random() & 0x1f);
        k->r = Random() & 2047;
        k->t = Random() & 2047;
        k->p = 0;
        k->rd = 1 + (Random() & 7);
        k->td = 1 + (Random() & 7);
        k->pd = 0;
        k->type = type;
        break;
    case 2:
        k->dx = k->dy = (20 + (Random() & 15)) << 5;
        k->page = POLY_PAGE_BIG_RAIN;
        k->rgb = 0x3f7f7fff;
        if (Random() & 1) {
            k->x = (-10 - (Random() & 0x1ff)) << 8;
            k->y = ((Random() % 520) - 20) << 8;
        } else {
            k->x = ((Random() % 680) - 20) << 8;
            k->y = (-10 - (Random() & 0x1ff) - 20) << 8;
        }
        k->size = 25 + (Random() & 0x1f);
        k->r = 0;
        k->t = 0;
        k->p = 1792;
        k->rd = 0;
        k->td = 0;
        k->pd = 0;
        k->type = type;
        break;
    case 3:
        k->dx = (15 + (Random() & 15)) << 5;
        k->dy = (5 + (Random() & 15)) << 5;
        k->page = POLY_PAGE_SNOWFLAKE;
        k->rgb = 0xffafafff;
        if (Random() & 1) {
            k->x = (-10 - (Random() & 0x1ff)) << 8;
            k->y = ((Random() % 520) - 20) << 8;
        } else {
            k->x = ((Random() % 680) - 20) << 8;
            k->y = (-10 - (Random() & 0x1ff) - 20) << 8;
        }
        k->size = 25 + (Random() & 0x1f);
        k->r = 0;
        k->t = 0;
        k->p = 0;
        k->rd = 1;
        k->td = 2;
        k->pd = 0;
        k->type = type;
        break;
    }
}

// uc_orig: FRONTEND_kibble_init (fallen/Source/frontend.cpp)
// Initializes the kibble pool to the density appropriate for the current theme.
// Densities: theme 0 (leaves)=25, theme 1 (rain)=255, theme 2 (snow)=40, theme 3=10.
void FRONTEND_kibble_init()
{
    UWORD c0, densities[] = { 25, 255, 40, 10 };
    Kibble* k;

    densities[0] = 25;
    densities[1] = 255;
    densities[2] = 40;
    densities[3] = 10;

    memset(kibble, 0, sizeof(kibble));

    for (c0 = 0, k = kibble; c0 < densities[menu_theme]; c0++, k++)
        FRONTEND_kibble_init_one(k, menu_theme + 1);
}

// uc_orig: FRONTEND_kibble_flurry (fallen/Source/frontend.cpp)
// Spawns a burst of kibble particles (e.g. on menu transition).
// Uses higher densities than init: leaves=125, rain=512, snow=125.
void FRONTEND_kibble_flurry()
{
    UWORD n, c0, densities[4];
    Kibble* k;

    ASSERT(kibble != NULL);

    densities[0] = 125;
    densities[1] = 512;
    densities[2] = 125;
    densities[3] = 10;

    n = densities[menu_theme];

    for (c0 = 0, k = kibble; c0 < n; c0++, k++)
        if (!k->type) {
            switch (menu_theme) {
            case 0:
                FRONTEND_kibble_init_one(k, 1 | 128);
                k->dx = (25 + (Random() & 15)) << 5;
                k->x = (-60 - (Random() & 0xff)) << 8;
                k->y = (Random() % 480) << 8;
                k->size = 5 + (Random() & 0x1f);
                k->r = Random() & 2047;
                k->t = Random() & 2047;
                k->rd = 1 + (Random() & 7);
                k->td = 1 + (Random() & 7);
                break;
            case 1:
                FRONTEND_kibble_init_one(k, 2 | 128);
                break;
            case 2:
                FRONTEND_kibble_init_one(k, 3 | 128);
                break;
            }
        }
}

// ---- Additional includes for chunk 2 ----------------------------------------

#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/graphics/pipeline/aeng.h"
#include "engine/io/env.h"

// ---- Kibble process ---------------------------------------------------------

// uc_orig: FRONTEND_kibble_process (fallen/Source/frontend.cpp)
// Advances all live kibble particles by elapsed time, throttling particle count
// dynamically based on frame rate (below 10 fps turns off particles; above 20 fps
// turns more on).
void FRONTEND_kibble_process()
{
    SLONG c0;
    Kibble* k;

    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t last = 0;
    static uint64_t now = 0;

    ASSERT(kibble != NULL);

    now = sdl3_get_ticks();

    if (last < now - 250) {
        last = now - 250;
    }

    if (now > last + 100) {
        // Front-end running at less than 10 fps — disable some random particles.
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
        kibble_off[rand() & 0x1ff] = UC_TRUE;
    } else if (now < last + 50) {
        // More than 20 fps — enable some random particles.
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
        kibble_off[rand() & 0x1ff] = UC_FALSE;
    }

    SLONG i;
    SLONG num_on;

    for (i = 0, num_on = 0; i < 512; i++) {
        if (!kibble_off[i]) {
            num_on += 1;
        }
    }

    while (last < now) {
        // Process at 40 frames a second.
        last += 1000 / 40;

        for (c0 = 0, k = kibble; c0 < 512; c0++, k++)
            if (k->type > 0) {
                k->x += k->dx;
                k->y += k->dy;
                k->r += k->rd;
                k->t += k->td;
                k->r &= 2047;
                k->t &= 2047;
                switch (k->type) {
                case 1:
                    k->dy++;
                    k->dx++;
                    if ((k->y >> 8) > 240)
                        k->dy -= Random() % ((k->y - 240) >> 14);
                    if ((k->x >> 8) - k->size > 650)
                        FRONTEND_kibble_init_one(k, 1);
                    break;
                case 129:
                    if ((k->x >> 8) - k->size > 650)
                        k->type = 0;
                case 3:
                case 131: {
                    SWORD x = k->x >> 8, y = k->y >> 8;
                    k->dx++;
                    if ((x > 320) && (x < 480))
                        k->dx -= Random() % ((k->x - 320) >> 14);
                    if ((y > 240) && (y < 280))
                        k->dy -= Random() % ((k->y - 240) >> 14);
                }
                case 2:
                case 130:
                    if ((((k->x >> 8) - k->size) > 640) || (((k->y >> 8) - k->size) > 480))
                        if (k->type < 128)
                            FRONTEND_kibble_init_one(k, k->type & 127);
                        else
                            k->type = 0;
                    break;
                }
            }
    }
}

// ---- Script filing ----------------------------------------------------------

// uc_orig: FRONTEND_fetch_title_from_id (fallen/Source/frontend.cpp)
// Scans the mission script to find the title string for the given mission ObjID.
// Falls back to "Urban Chaos" if not found.
void FRONTEND_fetch_title_from_id(CBYTE* script, CBYTE* ttl, UBYTE id)
{
    CBYTE* text;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();

    *ttl = 0;

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    strcpy(ttl, "Urban Chaos");

    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            break;
        }
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);

            if (mdata->ObjID == id) {
                strcpy(ttl, mdata->ttl);
                break;
            }
        }
    }
    FileCloseScript();

    MemFree(text);
    MFdelete(mdata);

    // Trim trailing spaces.
    char* pEnd = ttl + strlen(ttl) - 1;
    while (*pEnd == ' ') {
        *pEnd = '\0';
        pEnd--;
    }
}

// ---- Savegame helpers -------------------------------------------------------

// uc_orig: init_best_found (fallen/Source/frontend.cpp)
// Zeroes the per-mission best-score table.
void init_best_found(void)
{
    memset(&best_found[0][0], 50 * 4, 0);
}

// uc_orig: FRONTEND_save_savegame (fallen/Source/frontend.cpp)
// Writes the current game state to saves/slot{N}.wag.
// Format: mission_name string, complete_point, version=3, Darci/Roper stats,
// DarciDeadCivWarnings, mission_hierarchy[60], best_found[50][4].
bool FRONTEND_save_savegame(CBYTE* mission_name, UBYTE slot)
{
    CBYTE fn[_MAX_PATH];
    MFFileHandle file;
    UBYTE version = 3;

    oc_mkdir("saves");

    sprintf(fn, "saves/slot%d.wag", slot);
    file = FileCreate(fn, 1);
    FRONTEND_SaveString(file, mission_name);
    FileWrite(file, &complete_point, 1);
    FileWrite(file, &version, 1);
    FileWrite(file, &the_game.DarciStrength, 1);
    FileWrite(file, &the_game.DarciConstitution, 1);
    FileWrite(file, &the_game.DarciSkill, 1);
    FileWrite(file, &the_game.DarciStamina, 1);
    FileWrite(file, &the_game.RoperStrength, 1);
    FileWrite(file, &the_game.RoperConstitution, 1);
    FileWrite(file, &the_game.RoperSkill, 1);
    FileWrite(file, &the_game.RoperStamina, 1);
    FileWrite(file, &the_game.DarciDeadCivWarnings, 1);
    FileWrite(file, mission_hierarchy, 60);
    FileWrite(file, &best_found[0][0], 50 * 4);
    FileClose(file);

    return UC_TRUE;
}

// uc_orig: FRONTEND_load_savegame (fallen/Source/frontend.cpp)
// Loads game state from saves/slot{N}.wag.
// Supports versioned format: version 0 = complete_point only;
// version > 0 = + Darci/Roper stats; version > 1 = + mission_hierarchy[60];
// version > 2 = + best_found[50][4].
bool FRONTEND_load_savegame(UBYTE slot)
{
    CBYTE fn[_MAX_PATH];
    MFFileHandle file;
    UBYTE version = 0;

    sprintf(fn, "saves/slot%d.wag", slot);
    file = FileOpen(fn);
    FRONTEND_LoadString(file, fn);
    FileRead(file, &complete_point, 1);
    FileRead(file, &version, 1);
    if (version) {
        FileRead(file, &the_game.DarciStrength, 1);
        FileRead(file, &the_game.DarciConstitution, 1);
        FileRead(file, &the_game.DarciSkill, 1);
        FileRead(file, &the_game.DarciStamina, 1);
        FileRead(file, &the_game.RoperStrength, 1);
        FileRead(file, &the_game.RoperConstitution, 1);
        FileRead(file, &the_game.RoperSkill, 1);
        FileRead(file, &the_game.RoperStamina, 1);
        FileRead(file, &the_game.DarciDeadCivWarnings, 1);
    }
    if (version > 1) {
        FileRead(file, mission_hierarchy, 60);
    }
    if (version > 2) {
        FileRead(file, &best_found[0][0], 50 * 4);
    }
    FileClose(file);

    return UC_TRUE;
}

// uc_orig: FRONTEND_find_savegames (fallen/Source/frontend.cpp)
// Populates menu_data[] with save slot entries (up to 10 slots).
// Optionally greys out empty slots (bGreyOutEmpties) or checks disk space
// (bCheckSaveSpace, unused in this path). Preselects the most recently
// written slot based on file modification time.
void FRONTEND_find_savegames(bool bGreyOutEmpties, bool bCheckSaveSpace)
{
    CBYTE dir[_MAX_PATH], ttl[_MAX_PATH];
    SLONG c0;
    MenuData* md = menu_data;
    CBYTE* str = menu_buffer;
    SLONG x, y, y2 = 0;
    time_t high_time = 0;

    for (c0 = 1; c0 < 11; c0++) {
        md->Type = OT_BUTTON;
        md->Data = FE_SAVE_CONFIRM;
        md->Choices = (CBYTE*)0;

        MFFileHandle file;
        sprintf(dir, "saves/slot%d.wag", c0);
        file = FileOpen(dir);
        // Get file modification time from the opened handle (avoids path issues).
        struct stat st;
        time_t ftime = 0;
        if (file != FILE_OPEN_ERROR) {
            if (fstat(fileno(file), &st) == 0) ftime = st.st_mtime;
            FRONTEND_LoadString(file, ttl);
            FileClose(file);
        } else {
            strcpy(ttl, XLAT_str(X_EMPTY));
            if (bGreyOutEmpties) {
                md->Choices = (CBYTE*)1;
            }
        }
        sprintf(dir, "%d: %s", c0, ttl);
        md->Label = str;
        strcpy(str, ttl);
        str += strlen(str) + 1;
        MENUFONT_Dimensions(md->Label, x, y, -1, BIG_FONT_SCALE);
        md->X = 320 - (x >> 1);
        md->Y = y2;
        y2 += 50;
        if (ftime > high_time) {
            high_time = ftime;
            menu_state.selected = menu_state.items;
        }
        md++;
        menu_state.items++;
    }
}

// ---- Mission list helpers ---------------------------------------------------

// uc_orig: FRONTEND_MissionFilename (fallen/Source/frontend.cpp)
// Returns the .ucm filename for the i-th available mission in the selected district.
CBYTE* FRONTEND_MissionFilename(CBYTE* script, UBYTE i)
{
    CBYTE *text, *str = menu_buffer;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();

    *str = 0;

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    i++;

    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            break;
        }
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);

            if (mdata->District == districts[district_selected][2]) {
                if (mission_hierarchy[mdata->ObjID] & 4) {
                    i--;
                }
            }
            if (!i) {
                strcpy(str, mdata->fn);
                mission_launch = mdata->ObjID;
                break;
            }
        }
    }
    FileCloseScript();

    MemFree(text);
    MFdelete(mdata);

    return str;
}

// uc_orig: FRONTEND_MissionHierarchy (fallen/Source/frontend.cpp)
// Recomputes which missions are available, complete, and highlighted on the map.
// Called whenever complete_point changes (mission completed).
// Updates menu_theme (0-3) based on complete_point thresholds (8/16/24),
// which drives the background image set and kibble particle type.
// Applies special unlock logic: police1 requires ftutor1+assault1+testdrive1a
// all complete. fight2/testdrive3 gated by menu_theme thresholds.
// Stores results in mission_hierarchy[] (bit 1=exists, 2=complete, 4=available)
// and district_valid[] (bit 1=has missions, 2=has ready missions).
// Updates district_flash/district_selected to the earliest suggest_order mission.
void FRONTEND_MissionHierarchy(CBYTE* script)
{
    SLONG best_score;
    CBYTE* text;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();
    UBYTE j, flag;
    UBYTE newtheme;

    bonus_this_turn = 0;

    newtheme = 3;
    if (complete_point < 24)
        newtheme = 2;
    if (complete_point < 16)
        newtheme = 1;
    if (complete_point < 8)
        newtheme = 0;
    if (newtheme != menu_theme) {

        menu_theme = newtheme;

        FRONTEND_scr_new_theme(
            menu_back_names[menu_theme],
            menu_map_names[menu_theme],
            menu_brief_names[menu_theme],
            menu_config_names[menu_theme]);

        switch (menu_state.mode) {
        case FE_MAINMENU:
            ge_set_background_override(screenfull_back);
            break;
        case FE_CONFIG:
        case FE_CONFIG_VIDEO:
        case FE_CONFIG_AUDIO:
        case FE_CONFIG_OPTIONS:
        case FE_CONFIG_INPUT_KB:
        case FE_CONFIG_INPUT_JP:
        case FE_LOADSCREEN:
        case FE_SAVESCREEN:
            ge_set_background_override(screenfull_config);
            break;
        case FE_MAPSCREEN:
            ge_set_background_override(screenfull_map);
            break;
        default:
            ge_set_background_override(screenfull_brief);
        }
        FRONTEND_kibble_init();
    }

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    memset(district_valid, 0, sizeof(district_valid));
    mission_hierarchy[1] = 3;

    // First pass: locate special mission IDs needed for unlock logic.
    SLONG fightID = -1, assaultID = -1, testdriveID = -1, policeID = -1, fight2ID = -1, testdrive3ID = -1;
    SLONG bonusID1 = -1, bonusID2 = -1, bonusID3 = -1;
    SLONG secretIDbreakout = -1, estateID = -1;

    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[')
            break;
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);

            oc_strlwr(mdata->fn);

            if (strstr(mdata->fn, "ftutor1.ucm"))
                fightID = mdata->ObjID;
            if (strstr(mdata->fn, "assault1.ucm"))
                assaultID = mdata->ObjID;
            if (strstr(mdata->fn, "testdrive1a.ucm"))
                testdriveID = mdata->ObjID;
            if (strstr(mdata->fn, "police1.ucm"))
                policeID = mdata->ObjID;
            if (strstr(mdata->fn, "fight2.ucm"))
                fight2ID = mdata->ObjID;
            if (strstr(mdata->fn, "testdrive3.ucm"))
                testdrive3ID = mdata->ObjID;
            if (strstr(mdata->fn, "gangorder1.ucm"))
                bonusID1 = mdata->ObjID;
            if (strstr(mdata->fn, "gangorder2.ucm"))
                bonusID2 = mdata->ObjID;
            if (strstr(mdata->fn, "bankbomb1.ucm"))
                bonusID3 = mdata->ObjID;
            if (strstr(mdata->fn, "estate2.ucm"))
                estateID = mdata->ObjID;
        }
    }
    FileCloseScript();

    ASSERT(WITHIN(fightID, 0, 39));
    ASSERT(WITHIN(fight2ID, 0, 39));
    ASSERT(WITHIN(assaultID, 0, 39));
    ASSERT(WITHIN(testdriveID, 0, 39));
    ASSERT(WITHIN(testdrive3ID, 0, 39));
    ASSERT(WITHIN(policeID, 0, 39));
    ASSERT(WITHIN(bonusID1, 0, 39));
    ASSERT(WITHIN(bonusID2, 0, 39));
    ASSERT(WITHIN(bonusID3, 0, 39));
    ASSERT(WITHIN(estateID, 0, 39));

    best_score = 1000;
    district_flash = -1;
    district_selected = 0;

    // Second pass: compute flags for each mission.
    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            break;
        }
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);

            flag = mission_hierarchy[mdata->ObjID] | 1; // exists

            if (mission_hierarchy[mdata->ParentID] & 2) {
                if (mdata->ObjID == fight2ID && menu_theme < 1) {
                    // Suppress fight2 until theme 1 (8+ missions complete).
                } else if (mdata->ObjID == testdrive3ID && menu_theme < 2) {
                    // Suppress testdrive3 until theme 2 (16+ missions complete).
                } else {
                    for (j = 0; j < 40; j++) {
                        if (districts[j][2] == mdata->District) {
                            district_valid[j] |= 1;
                            if (!(mission_hierarchy[mdata->ObjID] & 2) && (mission_hierarchy[mdata->ObjID] & 4)) {
                                district_valid[j] |= 2;
                            }
                            break;
                        }
                    }

                    flag |= 4; // available
                }
            }

            if (mdata->ObjID == policeID) {
                if ((mission_hierarchy[fightID] & 2)
                    && (mission_hierarchy[assaultID] & 2)
                    && (mission_hierarchy[testdriveID] & 2))
                    flag |= 4;
                else {
                    flag &= ~4;
                    for (j = 0; j < 40; j++) {
                        if (districts[j][2] == mdata->District) {
                            district_valid[j] = 0;
                        }
                    }
                }
            }

            if (complete_point >= 40)
                flag |= 2; // bodge for endgame missions

            mission_hierarchy[mdata->ObjID] = flag;

            if ((flag & 4) && !(flag & 2)) {
                // Mission is available and not yet complete.
                // Check bonus unlock notifications.
                if ((mdata->ObjID == bonusID1) && !(bonus_state & 1)) {
                    bonus_state |= 1;
                    bonus_this_turn = 1;
                }
                if ((mdata->ObjID == bonusID2) && !(bonus_state & 2)) {
                    bonus_state |= 2;
                    bonus_this_turn = 1;
                }
                if ((mdata->ObjID == bonusID3) && !(bonus_state & 4)) {
                    bonus_state |= 4;
                    bonus_this_turn = 1;
                }
                SLONG order = 0;

                while (1) {
                    if (oc_stricmp(mdata->fn, suggest_order[order]) == 0) {
                        if (order < best_score) {
                            best_score = order;

                            for (j = 0; j < 40; j++) {
                                if (districts[j][2] == mdata->District) {
                                    district_flash = j;
                                    district_selected = j;

                                    goto found_mission;
                                }
                            }
                        }
                    }

                    order += 1;

                    if (suggest_order[order][0] == '!') {
                        break;
                    }

                found_mission:;
                }
            }
        }
    }
    FileCloseScript();

    if (!IsEnglish) {
        // Never show breakout mission in non-English builds.
        mission_hierarchy[secretIDbreakout] = 0;
    }

    MemFree(text);
    MFdelete(mdata);
}

// uc_orig: FRONTEND_MissionBrief (fallen/Source/frontend.cpp)
// Populates menu_data with the briefing text and title for mission i in the
// selected district. Also plays the mission briefing voice-over if available.
void FRONTEND_MissionBrief(CBYTE* script, UBYTE i)
{
    CBYTE *text, *str = menu_buffer;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();

    *str = 0;
    i++;

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            break;
        }
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);

            if (mdata->District == districts[district_selected][2]) {
                if (mission_hierarchy[mdata->ObjID] & 4) {
                    i--;
                }
            }

            if (!i) {
                strcpy(str, mdata->brief);
                str += strlen(str) + 1;
                strcpy(str, mdata->ttl);
                menu_state.title = str;
                break;
            }
        }
    }
    FileCloseScript();

    MemFree(text);

    if ((mdata->ObjID) && (mdata->ObjID < 34) && (0 != strcmp(brief_wav[mdata->ObjID], "none"))) {
        CBYTE path[_MAX_PATH];
        strcpy(path, "./");
        strcat(path, "talk2/");
        strcat(path, brief_wav[mdata->ObjID]);
        MFX_QUICK_play(path);
    }

    MFdelete(mdata);
}

// uc_orig: FRONTEND_MissionList (fallen/Source/frontend.cpp)
// Builds the mission list for the given district from the cached mission table.
// Each entry: first byte = mission ObjID, then the mission name string.
void FRONTEND_MissionList(CBYTE* script, UBYTE district)
{
    UBYTE i = 0;
    CBYTE* str = menu_buffer;

    mission_count = mission_selected = 0;

    while (*mission_cache[i].name) {
        if (mission_cache[i].district == district) {
            if (mission_hierarchy[mission_cache[i].id] & 4) {
                mission_selected = mission_count;
                *str++ = mission_cache[i].id;
                strcpy(str, mission_cache[i].name);
                str += strlen(mission_cache[i].name) + 1;
                mission_count++;
            }
        }

        i++;
    }
}

// uc_orig: FRONTEND_CacheMissionList (fallen/Source/frontend.cpp)
// Reads the mission script and caches all mission entries into mission_cache[].
void FRONTEND_CacheMissionList(CBYTE* script)
{
    CBYTE *text;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();
    UBYTE i = 0;

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);
    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            break;
        }
        if ((text[0] == '/') && (text[1] == '/')) {
            if (strstr(text, "Version"))
                sscanf(text, "%*s Version %d", &ver);
        } else {
            FRONTEND_ParseMissionData(text, ver, mdata);
            mission_cache[i].district = mdata->District;
            mission_cache[i].id = mdata->ObjID;
            strcpy(mission_cache[i].name, mdata->ttl);
            i++;
        }
    }
    FileCloseScript();

    MemFree(text);
    MFdelete(mdata);
}

// uc_orig: FRONTEND_districts (fallen/Source/frontend.cpp)
// Reads district definitions from the mission script and populates the districts[]
// array (sorted by map X coordinate for consistent display order).
void FRONTEND_districts(CBYTE* script)
{
    CBYTE *text, *str = menu_buffer;
    SLONG ver, mapx = 0, mapy = 0;
    SLONG x, y;
    UBYTE i = 0, ct, index = 0;
    SWORD temp_dist[40][3];
    UBYTE crap_remap[640][10];

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    district_count = 0;
    memset(crap_remap, 0, sizeof(crap_remap));

    FileOpenScript();
    while (1) {
        LoadStringScript(text);
        if (*text == 0)
            break;
        if (text[0] == '[') {
            i = 1;
        } else {
            if (i) {
                ct = sscanf(text, "%[^=]=%d,%d", str, &mapx, &mapy);
                if (strlen(str) && mapx && mapy && (ct == 3)) {
                    temp_dist[district_count][0] = mapx - 16;
                    temp_dist[district_count][1] = mapy - 32;
                    temp_dist[district_count][2] = index;
                    crap_remap[mapx][0]++;
                    crap_remap[mapx][crap_remap[mapx][0]] = district_count;
                    district_count++;
                }
                index++;
            } else if ((text[0] == '/') && (text[1] == '/')) {
                if (strstr(text, "Version"))
                    sscanf(text, "%*s Version %d", &ver);
            }
        }
    }
    FileCloseScript();

    if (district_count) {
        i = 0;
        for (x = 0; x < 640; x++)
            for (y = 1; y <= crap_remap[x][0]; y++) {
                districts[i][0] = temp_dist[crap_remap[x][y]][0];
                districts[i][1] = temp_dist[crap_remap[x][y]][1];
                districts[i][2] = temp_dist[crap_remap[x][y]][2];
                i++;
            }
        FRONTEND_MissionList(script, districts[district_selected][2]);
    }

    MemFree(text);
}

// ---- Menu helpers -----------------------------------------------------------

// uc_orig: FRONTEND_gettitle (fallen/Source/frontend.cpp)
// Returns the label string for a given menu mode and item selection.
CBYTE* FRONTEND_gettitle(UBYTE mode, UBYTE selection)
{
    RawMenuData* pt = raw_menu_data;
    while (pt->Menu != mode)
        pt++;
    for (; selection; selection--, pt++)
        ;
    return XLAT_str_ptr(pt->Label);
}

// uc_orig: FRONTEND_easy (fallen/Source/frontend.cpp)
// Populates menu_data[] with items for the given mode from raw_menu_data[].
// Computes X positions by centering button labels; stacks items vertically.
void FRONTEND_easy(UBYTE mode)
{
    SLONG x, y, y2 = 0;
    RawMenuData* pt = raw_menu_data;
    MenuData* md = menu_data + menu_state.items;

    if (menu_state.items)
        y2 = (md - 1)->Y + 50;

    int iBigFontScale = BIG_FONT_SCALE;
    if (!IsEnglish) {
        if ((menu_state.mode == FE_SAVESCREEN) || (menu_state.mode == FE_LOADSCREEN) || (menu_state.mode == FE_SAVE_CONFIRM)) {
            iBigFontScale = BIG_FONT_SCALE_FRENCH;
        }
    }

    while (pt->Menu != mode)
        pt++;
    while ((pt->Menu == mode) || !pt->Menu) {
        md->Type = pt->Type;
        md->LabelID = pt->Label;
        md->Label = XLAT_str_ptr(pt->Label);
        md->Data = pt->Data;
        switch (md->Type) {
        case OT_BUTTON:
            md->Choices = pt->Choices;
            // Fallthrough.
        case OT_LABEL:
            MENUFONT_Dimensions(md->Label, x, y, -1, iBigFontScale);
            md->X = 320 - (x >> 1);
            break;
        case OT_MULTI:
            md->X = 30;
            if (pt->Choices == MC_YN) {
                md->Choices = menu_choice_yesno;
                md->Data |= (2 << 8);
            } else if (pt->Choices == MC_SCANNER) {
                md->Choices = menu_choice_scanner;
                md->Data |= (2 << 8);
            }
            break;
        default:
            md->X = 30;
        }
        md->Y = y2;
        y2 += 50;
        md++;
        pt++;
        menu_state.items++;
    }
    for (md = menu_data; md->Type == OT_LABEL; md++, menu_state.selected++)
        ;

    FRONTEND_recenter_menu();
}

// ---- Video/audio config helpers --------------------------------------------

// uc_orig: LabelToIndex (fallen/Source/frontend.cpp)
// Maps a string ID (X_STARS, X_SHADOWS, etc.) to the detail level index
// used by AENG_get/set_detail_levels.
UBYTE LabelToIndex(SLONG label)
{
    switch (label) {
    case X_STARS:
        return 0;
    case X_SHADOWS:
        return 1;
    case X_PUDDLES:
        return 4;
    case X_DIRT:
        return 5;
    case X_MIST:
        return 6;
    case X_RAIN:
        return 7;
    case X_SKYLINE:
        return 8;
    case X_CRINKLES:
        return 11;
    case X_MOON:
        return 2;
    case X_PEOPLE:
        return 3;
    case X_BILINEAR:
        return 9;
    case X_PERSPECTIVE:
        return 10;
    }
    return 19;
}

// uc_orig: FRONTEND_restore_video_data (fallen/Source/frontend.cpp)
// Reads current engine detail levels and updates menu_data[] sliders/multi
// items to reflect the current settings (called when entering config_video).
void FRONTEND_restore_video_data()
{
    int data[20];
    UBYTE i, j;
    AENG_get_detail_levels(data, data + 1, data + 2, data + 3, data + 4, data + 5, data + 6, data + 7, data + 8, data + 9, data + 10, data + 11);
    for (i = 0; i < menu_state.items; i++)
        switch (menu_data[i].LabelID) {
        case X_STARS:
        case X_SHADOWS:
        case X_PUDDLES:
        case X_DIRT:
        case X_MIST:
        case X_RAIN:
        case X_SKYLINE:
        case X_CRINKLES:
        case X_MOON:
        case X_PEOPLE:
        case X_PERSPECTIVE:
        case X_BILINEAR:
            j = LabelToIndex(menu_data[i].LabelID);
            data[j] |= menu_data[i].Data & 0xff00;
            menu_data[i].Data = data[j];
            break;
        }
}

// uc_orig: FRONTEND_store_video_data (fallen/Source/frontend.cpp)
// Reads menu_data[] items and applies them to the engine detail levels.
void FRONTEND_store_video_data()
{
    int data[20], i, mode, bit_depth;

    AENG_get_detail_levels(data, data + 1, data + 2, data + 3, data + 4, data + 5, data + 6, data + 7, data + 8, data + 9, data + 10, data + 11);

    for (i = 0; i < menu_state.items; i++)
        switch (menu_data[i].LabelID) {
        case X_RESOLUTION:
            mode = menu_data[i].Data & 0xff;
            break;
        case X_COLOUR_DEPTH:
            bit_depth = menu_data[i].Data & 0xff ? 32 : 16;
            break;
        case X_DRIVERS:
            break;
        case X_STARS:
        case X_SHADOWS:
        case X_PUDDLES:
        case X_DIRT:
        case X_MIST:
        case X_RAIN:
        case X_SKYLINE:
        case X_CRINKLES:
        case X_MOON:
        case X_PEOPLE:
        case X_PERSPECTIVE:
        case X_BILINEAR:
            data[LabelToIndex(menu_data[i].LabelID)] = menu_data[i].Data & 0xff;
            break;
        case X_LOW:
            break;
        }
    ENV_set_value_number("Mode", mode, "Video");
    ENV_set_value_number("BitDepth", bit_depth, "Video");
    AENG_set_detail_levels(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11]);
}

// State passed through the driver enumeration callback.
struct DriverEnumState {
    CBYTE* str;
    SLONG count;
    SLONG selected;
};

static void driver_enum_callback(const char* name, bool is_primary, bool is_current, void* ctx)
{
    auto* state = static_cast<DriverEnumState*>(ctx);
    char szBuff[80];
    if (is_primary)
        sprintf(szBuff, "%s (Primary)", name);
    else
        sprintf(szBuff, "%s", name);

    strcpy(state->str, szBuff);
    state->str += strlen(state->str) + 1;

    if (is_current)
        state->selected = state->count;

    state->count++;
}

// uc_orig: FRONTEND_do_drivers (fallen/Source/frontend.cpp)
// Populates the display driver/resolution/bit-depth menus by querying
// the display driver list and current display settings.
void FRONTEND_do_drivers()
{
    CBYTE *str = menu_buffer, *str_tmp;

    switch (ScreenWidth) {
    case 640:
        CurrentVidMode = 0;
        break;
    case 800:
        CurrentVidMode = 1;
        break;
    case 1024:
        CurrentVidMode = 2;
        break;
    case 320:
        CurrentVidMode = 3;
        break;
    case 512:
        CurrentVidMode = 4;
        break;
    default:
        CurrentVidMode = 0;
        break;
    }
    CurrentBitDepth = ge_get_screen_bpp();

    strcpy(str, "640x480");
    str += strlen(str) + 1;
    strcpy(str, "800x600");
    str += strlen(str) + 1;
    strcpy(str, "1024x768");
    str += strlen(str) + 1;
    strcpy(str, "320x240");
    str += strlen(str) + 1;
    strcpy(str, "512x384");
    str += strlen(str) + 1;
    str_tmp = str;
    menu_data[1].Data = CurrentVidMode | (5 << 8);
    menu_data[1].Choices = menu_buffer;

    strcpy(str, "16 bit");
    str += strlen(str) + 1;
    strcpy(str, "32 bit");
    str += strlen(str) + 1;
    menu_data[3].Data = ((UBYTE)(CurrentBitDepth == 32)) | (2 << 8);
    menu_data[3].Choices = str_tmp;
    str_tmp = str;

    DriverEnumState state = { str, 0, 0 };
    ge_enumerate_drivers(driver_enum_callback, &state);

    menu_data[2].Data = state.selected | (state.count << 8);
    menu_data[2].Choices = str_tmp;
}

// uc_orig: FRONTEND_gamma_update (fallen/Source/frontend.cpp)
// Applies the current gamma slider value to the display.
void FRONTEND_gamma_update()
{
    if (menu_state.selected == GammaIndex)
        ge_set_gamma(menu_data[GammaIndex].Data & 0xff, 256);
}

// uc_orig: FRONTEND_do_gamma (fallen/Source/frontend.cpp)
// Inserts a gamma slider item into the config video menu.
void FRONTEND_do_gamma()
{
    SLONG x, y, y2 = 0;
    MenuData keepsafe;
    MenuData* md = menu_data + menu_state.items - 1;

    keepsafe = *md;

    md->Label = XLAT_str_ptr(X_GAMMA);
    MENUFONT_Dimensions(md->Label, x, y, -1, BIG_FONT_SCALE);
    md->Type = OT_LABEL;
    md->X = 320 - (x >> 1);
    y2 = md->Y;
    md++;
    y2 += 50;

    md->Label = XLAT_str_ptr(X_LOW);
    md->LabelID = X_LOW;
    GammaIndex = md - menu_data;
    MENUFONT_Dimensions(md->Label, x, y, -1, BIG_FONT_SCALE);
    md->Type = OT_SLIDER;
    md->X = 30;
    md->Y = y2;
    md++;
    y2 += 50;

    *md = keepsafe;
    md->Y = y2;

    md++;
    y2 += 50;

    menu_state.items += 2;

    int32_t a, b;
    ge_get_gamma(&a, &b);
    menu_data[GammaIndex].Data = a;
}

// ---- Mode switching ---------------------------------------------------------

// uc_orig: FRONTEND_mode (fallen/Source/frontend.cpp)
// Switches the frontend to a new screen/mode. Pushes current mode onto a stack
// so FE_BACK (-2) can return to it. Clears menu_data[] and repopulates it for
// the new mode. mode >= 100 is shorthand for MISSIONBRIEFING for mission (mode-100).
void FRONTEND_mode(SBYTE mode, bool bDoTransition)
{
    dwAutoPlayFMVTimeout = sdl3_get_ticks() + AUTOPLAY_FMV_DELAY;

    fade_mode = 1;
    memset(menu_data, 0, sizeof(menu_data));
    menu_state.items = 0;
    if (mode == -2) {
        if (menu_state.stackpos > 0) {
            menu_state.stackpos--;
            mode = menu_state.stack[menu_state.stackpos].mode;
            menu_state.scroll = menu_state.stack[menu_state.stackpos].scroll;
            menu_state.selected = menu_state.stack[menu_state.stackpos].selected;
            menu_state.title = menu_state.stack[menu_state.stackpos].title;
        } else {
            mode = menu_state.mode;
        }
    } else {
        if (menu_thrash) {
            menu_state.stack[menu_state.stackpos].mode = menu_thrash;
            menu_state.stack[menu_state.stackpos].scroll = 0;
            menu_state.stack[menu_state.stackpos].selected = 0;
            menu_state.stack[menu_state.stackpos].title = 0;
            menu_state.stackpos++;
            menu_thrash = 0;
        } else if (menu_state.mode != -1) {
            menu_state.stack[menu_state.stackpos].mode = menu_state.mode;
            menu_state.stack[menu_state.stackpos].scroll = menu_state.scroll;
            menu_state.stack[menu_state.stackpos].selected = menu_state.selected;
            menu_state.stack[menu_state.stackpos].title = menu_state.title;
            menu_state.stackpos++;
        }
        menu_state.selected = 0;
    }

    switch (mode) {
    case FE_MAPSCREEN:
        menu_state.title = XLAT_str_ptr(X_START);
        break;
    case FE_MAINMENU:
        menu_state.title = NULL;
        break;
    case FE_LOADSCREEN:
        menu_state.title = XLAT_str_ptr(X_LOAD_GAME);
        break;
    case FE_SAVESCREEN:
        menu_state.title = XLAT_str_ptr(X_SAVE_GAME);
        break;
    default:
        if (menu_state.stackpos && (mode < 100)) {
            menu_state.title = FRONTEND_gettitle(menu_state.stack[menu_state.stackpos - 1].mode, menu_state.stack[menu_state.stackpos - 1].selected);
        } else {
            menu_state.title = 0;
        }
        break;
    }

    menu_state.mode = mode;

    if (mode >= 100) {
        FRONTEND_init_xition();
        FRONTEND_MissionBrief(MISSION_SCRIPT, mode - 100);
        return;
    }
    switch (mode) {
    case FE_MAPSCREEN:
        FRONTEND_init_xition();
        FRONTEND_districts(MISSION_SCRIPT);
        break;
    case FE_MAINMENU:
        FRONTEND_init_xition();
        FRONTEND_easy(mode);
        menu_state.stackpos = 0;
        MUSIC_mode(MUSIC_MODE_FRONTEND);
        if (AllowSave)
            menu_data[2].Choices = NULL;
        break;
    case FE_SAVESCREEN:
        AllowSave = 1;
        if (!menu_state.stackpos) {
            ge_set_background_override(screenfull_config);
        }
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_find_savegames(UC_FALSE, UC_TRUE);
        FRONTEND_easy(mode);
        menu_state.title = XLAT_str_ptr(X_SAVE_GAME);
        break;
    case FE_LOADSCREEN:
        FRONTEND_find_savegames(UC_TRUE, UC_FALSE);
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        break;
    case FE_CONFIG:
        FRONTEND_init_xition();
        FRONTEND_easy(mode);
        break;
    case FE_QUIT:
        FRONTEND_easy(mode);
        menu_state.title = XLAT_str_ptr(X_EXIT);
        break;
    case FE_CONFIG_VIDEO: {
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        if (ge_is_gamma_available())
            FRONTEND_do_gamma();
        FRONTEND_restore_video_data();
    } break;
    case FE_CONFIG_AUDIO: {
        SLONG fx, amb, mus;
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        MFX_get_volumes(&fx, &amb, &mus);
        menu_data[0].Data = fx << 1;
        menu_data[1].Data = amb << 1;
        menu_data[2].Data = mus << 1;
    } break;
    case FE_CONFIG_INPUT_KB:
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        menu_data[0].Data = ENV_get_value_number("keyboard_left", 203, "Keyboard");
        menu_data[1].Data = ENV_get_value_number("keyboard_right", 205, "Keyboard");
        menu_data[2].Data = ENV_get_value_number("keyboard_forward", 200, "Keyboard");
        menu_data[3].Data = ENV_get_value_number("keyboard_back", 208, "Keyboard");
        menu_data[4].Data = ENV_get_value_number("keyboard_punch", 44, "Keyboard");
        menu_data[5].Data = ENV_get_value_number("keyboard_kick", 45, "Keyboard");
        menu_data[6].Data = ENV_get_value_number("keyboard_action", 46, "Keyboard");
        menu_data[7].Data = ENV_get_value_number("keyboard_jump", 57, "Keyboard");
        menu_data[8].Data = ENV_get_value_number("keyboard_start", 15, "Keyboard");
        menu_data[9].Data = ENV_get_value_number("keyboard_select", 28, "Keyboard");
        menu_data[11].Data = ENV_get_value_number("keyboard_camera", 207, "Keyboard");
        menu_data[12].Data = ENV_get_value_number("keyboard_cam_left", 211, "Keyboard");
        menu_data[13].Data = ENV_get_value_number("keyboard_cam_right", 209, "Keyboard");
        menu_data[14].Data = ENV_get_value_number("keyboard_1stperson", 30, "Keyboard");
        break;
    case FE_CONFIG_INPUT_JP:
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        menu_data[0].Data = ENV_get_value_number("joypad_kick", 4, "Joypad");
        menu_data[1].Data = ENV_get_value_number("joypad_punch", 3, "Joypad");
        menu_data[2].Data = ENV_get_value_number("joypad_jump", 0, "Joypad");
        menu_data[3].Data = ENV_get_value_number("joypad_action", 1, "Joypad");
        menu_data[4].Data = ENV_get_value_number("joypad_move", 7, "Joypad");
        menu_data[5].Data = ENV_get_value_number("joypad_start", 8, "Joypad");
        menu_data[6].Data = ENV_get_value_number("joypad_select", 2, "Joypad");
        menu_data[8].Data = ENV_get_value_number("joypad_camera", 6, "Joypad");
        menu_data[9].Data = ENV_get_value_number("joypad_cam_left", 9, "Joypad");
        menu_data[10].Data = ENV_get_value_number("joypad_cam_right", 10, "Joypad");
        menu_data[11].Data = ENV_get_value_number("joypad_1stperson", 5, "Joypad");
        break;
    case FE_CONFIG_OPTIONS:
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        menu_data[1].Data |= ENV_get_value_number("scanner_follows", 1, "Game");
        break;
    case FE_SAVE_CONFIRM:
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        menu_state.scroll = 0;
        menu_state.title = XLAT_str_ptr(X_SAVE_GAME);
        break;
    }

    FRONTEND_recenter_menu();
}


// ---- Chunk 3: district/mission display, input, init, main loop -------------

// Forward declarations for internal helpers used before their definitions.
// uc_orig: FRONTEND_draw_districts (fallen/Source/frontend.cpp)
static void FRONTEND_draw_districts(void);
// uc_orig: FRONTEND_input (fallen/Source/frontend.cpp)
static UBYTE FRONTEND_input(void);
// uc_orig: FRONTEND_storedata (fallen/Source/frontend.cpp)
static void FRONTEND_storedata(void);
// uc_orig: FRONTEND_ValidMission (fallen/Source/frontend.cpp)
static BOOL FRONTEND_ValidMission(SWORD sel);
// uc_orig: FRONTEND_playambient3d (fallen/Source/frontend.cpp)
static void FRONTEND_playambient3d(SLONG channel, SLONG wave_id, SLONG flags, UBYTE height = 0);
// uc_orig: FRONTEND_sound (fallen/Source/frontend.cpp)
static void FRONTEND_sound(void);
// uc_orig: FRONTEND_diddle_stats (fallen/Source/frontend.cpp)
static void FRONTEND_diddle_stats(void);
// uc_orig: FRONTEND_diddle_music (fallen/Source/frontend.cpp)
static void FRONTEND_diddle_music(void);

// uc_orig: FRONTEND_draw_districts (fallen/Source/frontend.cpp)
// Draws the district buttons and mission list on the map screen.
// For the selected district: lists missions with strikethrough for completed ones.
static void FRONTEND_draw_districts(void)
{
    UBYTE i, j, id;
    SWORD x, y;
    CBYTE* str;
    UWORD fade;
    ULONG rgb;

    if (bonus_this_turn) {
        if (bonus_this_turn == 1) {
            bonus_this_turn++;
            MFX_play_stereo(0, S_TUNE_BONUS, 0);
        }
        fade = (64 - fade_state) << 2;
        if (IsEnglish) {
            str = "Bonus mission unlocked!"; // XLAT_str(X_BONUS_MISSION_UNLOCKED)
            FONT2D_DrawStringCentred(str, 322, 10, 0x000000, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
            FONT2D_DrawStringCentred(str, 320, 8, 0xffffff, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
        }
    }

    fade = (64 - fade_state) << 2;

    {
        CBYTE str2[200];
        sprintf(str2, "%s: %03d  %s: %03d  %s: %03d  %s: %03d\n", XLAT_str_ptr(X_CON_INCREASED), the_game.DarciConstitution, XLAT_str_ptr(X_STA_INCREASED), the_game.DarciStamina, XLAT_str_ptr(X_STR_INCREASED), the_game.DarciStrength, XLAT_str_ptr(X_REF_INCREASED), the_game.DarciSkill);
        int iYpos;
        if (ge_is_ntsc()) {
            // Move it further up so it's on screen for the yanks.
            iYpos = 434;
        } else {
            iYpos = 454;
        }
        FONT2D_DrawStringCentred(str2, 320 + 2, iYpos + 2, 0x000000, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
        FONT2D_DrawStringCentred(str2, 320, iYpos, 0xffffff, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
    }

    for (i = 0; i < district_count; i++) {
        switch (district_valid[i]) {
        case 1:
            FRONTEND_draw_button(districts[i][0], districts[i][1], (UBYTE)(i == district_selected) | 4, i == district_flash);
            break;
        case 2:
        case 3:
            FRONTEND_draw_button(districts[i][0], districts[i][1], (i == district_selected) ? 5 : 6, i == district_flash);
            break;
        }

        if (i == district_selected) {
            y = districts[i][1];
            x = districts[i][0];

            str = menu_buffer;

            for (j = 0; j < mission_count; j++) {
                id = *str++;

                // What colour do we draw this bit of text?
                if (j == mission_selected) {
                    rgb = 0xffffff;
                } else {
                    rgb = 0x667788;
                }

                fade = (64 - fade_state) << 2;

                if (fade > 255)
                    fade = 255;

                if (mission_hierarchy[id] & 4) // 4 => available.
                {
                    if (x > 320) {
                        FONT2D_DrawStringRightJustify(str, x + 18 + 2, y + 2, 0x000000, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
                        FONT2D_DrawStringRightJustify(str, x + 18, y, rgb, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);

// These are just tweaked values - dunno why they work/don't work. Just accept it.
#define STRIKETHROUGH_HANGOVER_L 5
#define STRIKETHROUGH_HANGOVER_R 25

                        if (mission_hierarchy[id] & 2) // 2 => complete
                        {
                            FONT2D_DrawStrikethrough(
                                FONT2D_leftmost_x + 2 - STRIKETHROUGH_HANGOVER_L,
                                x + 2 + STRIKETHROUGH_HANGOVER_R,
                                y + 2,
                                0x000000,
                                256,
                                POLY_PAGE_FONT2D,
                                fade, UC_FALSE);

                            FONT2D_DrawStrikethrough(
                                FONT2D_leftmost_x + 0 - STRIKETHROUGH_HANGOVER_L,
                                x + 0 + STRIKETHROUGH_HANGOVER_R,
                                y,
                                rgb,
                                256,
                                POLY_PAGE_FONT2D,
                                fade, UC_TRUE);
                        }
                    } else {
                        FONT2D_DrawString(str, x + 32 + 2, y + 2, 0x000000, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);
                        FONT2D_DrawString(str, x + 32, y, rgb, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, fade);

#undef STRIKETHROUGH_HANGOVER_L
#undef STRIKETHROUGH_HANGOVER_R
// These are just tweaked values - dunno why they work/don't work. Just accept it.
#define STRIKETHROUGH_HANGOVER_L (-15)
#define STRIKETHROUGH_HANGOVER_R 30

                        if (mission_hierarchy[id] & 2) // 2 => complete
                        {
                            FONT2D_DrawStrikethrough(
                                x + 2 - STRIKETHROUGH_HANGOVER_L,
                                FONT2D_rightmost_x + 2 + STRIKETHROUGH_HANGOVER_R,
                                y + 2,
                                0x000000,
                                256,
                                POLY_PAGE_FONT2D,
                                fade, UC_FALSE);

                            FONT2D_DrawStrikethrough(
                                x - STRIKETHROUGH_HANGOVER_L,
                                FONT2D_rightmost_x + STRIKETHROUGH_HANGOVER_R,
                                y,
                                rgb,
                                256,
                                POLY_PAGE_FONT2D,
                                fade, UC_TRUE);
                        }
                    }
                }

                str += strlen(str) + 1;
                y += 20;
            }
        }
    }
}


// uc_orig: FRONTEND_display (fallen/Source/frontend.cpp)
// Per-frame frontend BACKGROUND render: clear viewport, background image,
// transition image, kibble particles. The TEXT/UI overlay half (menu items,
// arrows, title, district map markers, mission select, moving panel preview)
// was split off into FRONTEND_display_overlay() which runs in the post-
// composition UI pass — keeps the menu glyphs and map dots crisp at native
// resolution while the background image and the leaf particles still go
// through the scene FBO (the leaves are essentially 3D-style sprites and
// look better with composition-pass antialiasing).
//
// Sets g_frontend_overlay_pending so the next UI pass knows to call
// FRONTEND_display_overlay() — a "dirty bit" that's reliably true ONLY
// in the same frame where this background half ran. Using GAME_STATE bits
// instead would leak into outro / video / other phases that share the
// flip path but shouldn't get the menu overlay.
//
// Use-sites declare with `extern bool g_frontend_overlay_pending;`.
bool g_frontend_overlay_pending = false;

void FRONTEND_display()
{
    PolyPage::UIModeScope _ui_scope(ui_coords::UIAnchor::CENTER_CENTER);

    ge_set_viewport(
        0, 0, ge_get_screen_width(), ge_get_screen_height());
    ge_clear(true, true);

    ge_show_back_image();
    if ((fade_mode & 3) == 1)
        FRONTEND_show_xition();
    POLY_frame_init(UC_FALSE, UC_FALSE);
    FRONTEND_kibble_draw();
    POLY_frame_draw(UC_FALSE, UC_FALSE);

    g_frontend_overlay_pending = true;
}

// Per-frame frontend OVERLAY render: menu items (with scrolling, strikethrough,
// widget rendering), "more" arrows, title wibble, district map markers,
// mission select text, and the in-development moving-panel preview. Runs from
// the post-composition UI pass (split_ui_from_scene_plan.md) so the text and
// map dots land on the default FB at native window pixels untouched by FXAA /
// bilinear upscale.
void FRONTEND_display_overlay()
{
    PolyPage::UIModeScope _ui_scope(ui_coords::UIAnchor::CENTER_CENTER);
    UBYTE i;
    SLONG rgb, x, x2, y;
    MenuData* md = menu_data;
    UBYTE whichmap[] = { 2, 0, 1, 3 };
    UBYTE arrow = 0;

    POLY_frame_init(UC_FALSE, UC_FALSE);

    int iBigFontScale = BIG_FONT_SCALE;
    if (!IsEnglish) {
        if ((menu_state.mode == FE_SAVESCREEN) || (menu_state.mode == FE_LOADSCREEN) || (menu_state.mode == FE_SAVE_CONFIRM)) {
            // Reduce it a bit to fit long French words on screen.
            iBigFontScale = BIG_FONT_SCALE_FRENCH;
        }
    }

    for (i = 0; i < menu_state.items; i++, md++) {
        y = md->Y + menu_state.base - menu_state.scroll;
        if ((y >= 100) && (y <= 400)) {
            rgb = FRONTEND_fix_rgb(fade_rgb, i == menu_state.selected);
            if ((md->Type == OT_BUTTON) && (md->Choices == (CBYTE*)1)) // 'greyed out'
            {
                SLONG rgbtemp = rgb & 0xff000000;
                rgb = (rgb & 0xff) >> 1;
                rgb |= (rgb << 8) | (rgb << 16);
                rgb |= rgbtemp;
            }

            MENUFONT_Draw(md->X, y, iBigFontScale, md->Label, rgb, 0);

            switch (md->Type) {
            case OT_SLIDER:
                FRONTEND_DrawSlider(md);
                break;
            case OT_MULTI:
                FRONTEND_DrawMulti(md, rgb);
                break;
            case OT_KEYPRESS:
                FRONTEND_DrawKey(md);
                break;
            case OT_PADPRESS:
                FRONTEND_DrawPad(md);
                break;
            }
        } else {
            if (i == menu_state.selected) { // better do some scrolling
                if (y < 100)
                    menu_state.scroll -= 10;
                if (y > 400)
                    menu_state.scroll += 10;
            }
            if (y < 100)
                arrow |= 1;
            if (y > 400)
                arrow |= 2;
        }
    }
    if (arrow & 1) // draw a "more..." arrow at the top of the screen
    {
        DRAW2D_Tri(320, 50, 335, 65, 305, 65, fade_rgb, 0);
    }
    if (arrow & 2) // draw a "more..." arrow at the bottom of the screen
    {
        DRAW2D_Tri(320, 430, 335, 415, 305, 415, fade_rgb, 0);
    }
    if (menu_state.title && (menu_state.mode != FE_MAINMENU)) {
        BOOL dir;
        x2 = SIN(fade_state << 3) >> 10;
        switch (menu_state.mode) {
        case FE_MAPSCREEN:
            MENUFONT_Dimensions(menu_state.title, x, y, -1, BIG_FONT_SCALE);
            x = 560 - x;
            x2 = (x2 * 10) - 63;
            dir = 0;
            break;
        default:
            x = 80;
            x2 = 642 - (x2 * 10);
            dir = 1;
            break;
        }
        FontPage = POLY_PAGE_NEWFONT;
        FRONTEND_draw_title(x + 2, 44, x2, menu_state.title, 0, dir);
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        POLY_frame_init(UC_FALSE, UC_FALSE);
        FontPage = POLY_PAGE_NEWFONT_INVERSE;
        FRONTEND_draw_title(x, 40, x2, menu_state.title, 1, dir);
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        POLY_frame_init(UC_FALSE, UC_FALSE);
        FRONTEND_draw_button(x2, 8, whichmap[menu_theme]);
    }
    if ((menu_state.mode == FE_MAPSCREEN) && ((fade_mode == 2) || (fade_state == 63)))
        FRONTEND_draw_districts();
    if ((menu_state.mode >= 100) && *menu_buffer) {
        POLY_frame_draw(UC_FALSE, UC_FALSE);
        POLY_frame_init(UC_FALSE, UC_FALSE);
        x = SIN(fade_state << 3) >> 10;
        FRONTEND_draw_button(642 - (x * 10), 8, whichmap[menu_theme]);
        FONT2D_DrawStringWrapTo(menu_buffer, 20 + 2, 100 + 2, 0x000000, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, 255 - (fade_state << 2), 402);
        FONT2D_DrawStringWrapTo(menu_buffer, 20, 100, fade_rgb, SMALL_FONT_SCALE, POLY_PAGE_FONT2D, 255 - (fade_state << 2), 400);
    }

    if (m_bMovingPanel) {
        // Display the panel at the current position.
        // REMEMBER THAT THESE NUMBERS ARE DIVIDED BY 4!
        int iXPos = ENV_get_value_number("panel_x", 32 / 4, "");
        int iYPos = ENV_get_value_number("panel_y", (480 - 32) / 4, "");

        extern void PANEL_draw_quad(float left, float top, float right, float bottom, SLONG page, ULONG colour,
            float u1, float v1, float u2, float v2);

        PANEL_draw_quad(
            (float)(iXPos * 4 + 0),
            (float)(iYPos * 4 - 165),
            (float)(iXPos * 4 + 212),
            (float)(iYPos * 4 - 0),
            POLY_PAGE_LASTPANEL_ALPHA,
            0xffffffff,
            0.0F,
            90.0F / 256.0F,
            212.0F / 256.0F,
            1.0F);
    }

    POLY_frame_draw(UC_FALSE, UC_FALSE);
}

// uc_orig: FRONTEND_storedata (fallen/Source/frontend.cpp)
// Saves current menu screen's settings back to ENV (video, audio, keyboard, joypad, options).
static void FRONTEND_storedata(void)
{
    switch (menu_state.mode) {
    case FE_CONFIG_VIDEO:
        FRONTEND_store_video_data();
        break;
    case FE_CONFIG_AUDIO:
        MFX_stop(WEATHER_REF, MFX_WAVE_ALL);
        MFX_set_volumes(menu_data[0].Data >> 1, menu_data[1].Data >> 1, menu_data[2].Data >> 1);
        break;

    case FE_CONFIG_INPUT_KB:
        ENV_set_value_number("keyboard_left", menu_data[0].Data, "Keyboard");
        ENV_set_value_number("keyboard_right", menu_data[1].Data, "Keyboard");
        ENV_set_value_number("keyboard_forward", menu_data[2].Data, "Keyboard");
        ENV_set_value_number("keyboard_back", menu_data[3].Data, "Keyboard");
        ENV_set_value_number("keyboard_punch", menu_data[4].Data, "Keyboard");
        ENV_set_value_number("keyboard_kick", menu_data[5].Data, "Keyboard");
        ENV_set_value_number("keyboard_action", menu_data[6].Data, "Keyboard");
        ENV_set_value_number("keyboard_jump", menu_data[7].Data, "Keyboard");
        ENV_set_value_number("keyboard_start", menu_data[8].Data, "Keyboard");
        ENV_set_value_number("keyboard_select", menu_data[9].Data, "Keyboard");
        // gap for label
        ENV_set_value_number("keyboard_camera", menu_data[11].Data, "Keyboard");
        ENV_set_value_number("keyboard_cam_left", menu_data[12].Data, "Keyboard");
        ENV_set_value_number("keyboard_cam_right", menu_data[13].Data, "Keyboard");
        ENV_set_value_number("keyboard_1stperson", menu_data[14].Data, "Keyboard");
        break;

    case FE_CONFIG_INPUT_JP:
        ENV_set_value_number("joypad_kick", menu_data[0].Data, "Joypad");
        ENV_set_value_number("joypad_punch", menu_data[1].Data, "Joypad");
        ENV_set_value_number("joypad_jump", menu_data[2].Data, "Joypad");
        ENV_set_value_number("joypad_action", menu_data[3].Data, "Joypad");
        ENV_set_value_number("joypad_move", menu_data[4].Data, "Joypad");
        ENV_set_value_number("joypad_start", menu_data[5].Data, "Joypad");
        ENV_set_value_number("joypad_select", menu_data[6].Data, "Joypad");
        // gap for label
        ENV_set_value_number("joypad_camera", menu_data[8].Data, "Joypad");
        ENV_set_value_number("joypad_cam_left", menu_data[9].Data, "Joypad");
        ENV_set_value_number("joypad_cam_right", menu_data[10].Data, "Joypad");
        ENV_set_value_number("joypad_1stperson", menu_data[11].Data, "Joypad");
        break;

    case FE_CONFIG_OPTIONS:
        ENV_set_value_number("scanner_follows", menu_data[1].Data & 1, "Game");
        break;
    }
}

// uc_orig: FRONTEND_ValidMission (fallen/Source/frontend.cpp)
// Returns UC_TRUE if the mission at position sel in menu_buffer is available (hierarchy bit 4 set).
static BOOL FRONTEND_ValidMission(SWORD sel)
{
    CBYTE* str = menu_buffer;
    UBYTE id = *str;

    while (sel) {
        sel--;
        str++;
        str += strlen(str) + 1;
        id = *str;
    }
    return (BOOL)(mission_hierarchy[id] & 4);
}

// uc_orig: FRONTEND_input (fallen/Source/frontend.cpp)
// Polls keyboard and joypad state, processes navigation keys (up/down/left/right/enter/esc),
// handles key/pad rebinding modes, and returns 0 normally or a FE_* exit code.
static UBYTE FRONTEND_input(void)
{
    UBYTE scan, any_button = 0;
    static SLONG last_input = 0;
    static UBYTE last_button = 0;
    static UBYTE first_pad = 1;
    static int dir_ticker = 0;
    static int held_x = 0; // hysteresis state: -1=LEFT, 0=center, 1=RIGHT
    static int held_y = 0; // hysteresis state: -1=UP, 0=center, 1=DOWN

    SLONG input = 0;

    if (grabbing_pad && !last_input) {
        UBYTE i, j;
        MenuData* item = menu_data + menu_state.selected;
        ReadInputDevice();
        if (Keys[KB_ESC] || (input & INPUT_MASK_CANCEL)) {
            Keys[KB_ESC] = 0;
            grabbing_pad = 0;
        } else {
            for (i = 0; i < 32; i++) {
                if (the_state.rgbButtons[i] & 0x80) {
                    for (j = 0; j < menu_state.items; j++) {
                        if (menu_data[j].Data == i) {
                            menu_data[j].Data = 31;
                        }
                    }
                    item->Data = i;
                    last_button = 1;
                    grabbing_pad = 0;
                    break;
                }
            }
        }
        return 0;
    } else {
        // Axis reading with hysteresis to prevent stick wobble near threshold.
        // Activation zone is wider, release zone is tighter — once a direction is detected,
        // small oscillations around the threshold don't flip it on/off.
#define AXIS_CENTRE 32768
#define ACTIVATE_ZONE 4096
#define RELEASE_ZONE 2048

        input = get_hardware_input(INPUT_TYPE_JOY);

        input &= ~(INPUT_MASK_LEFT | INPUT_MASK_RIGHT | INPUT_MASK_FORWARDS | INPUT_MASK_BACKWARDS);

        // Gamepad axes — only process if connected (disconnected state has lX/lY=0 which
        // would be misread as far-up-left).
        if (the_state.connected) {
            // X axis hysteresis.
            if (held_x == 0) {
                if (the_state.lX > AXIS_CENTRE + ACTIVATE_ZONE) held_x = 1;
                else if (the_state.lX < AXIS_CENTRE - ACTIVATE_ZONE) held_x = -1;
            } else if (held_x > 0) {
                if (the_state.lX < AXIS_CENTRE + RELEASE_ZONE) held_x = 0;
            } else {
                if (the_state.lX > AXIS_CENTRE - RELEASE_ZONE) held_x = 0;
            }

            // Y axis hysteresis.
            if (held_y == 0) {
                if (the_state.lY > AXIS_CENTRE + ACTIVATE_ZONE) held_y = 1;
                else if (the_state.lY < AXIS_CENTRE - ACTIVATE_ZONE) held_y = -1;
            } else if (held_y > 0) {
                if (the_state.lY < AXIS_CENTRE + RELEASE_ZONE) held_y = 0;
            } else {
                if (the_state.lY > AXIS_CENTRE - RELEASE_ZONE) held_y = 0;
            }

            // No diagonals — dominant axis wins (prevents Y wobble from overriding X).
            int32_t dx = the_state.lX > AXIS_CENTRE ? the_state.lX - AXIS_CENTRE : AXIS_CENTRE - the_state.lX;
            int32_t dy = the_state.lY > AXIS_CENTRE ? the_state.lY - AXIS_CENTRE : AXIS_CENTRE - the_state.lY;

            if (held_y != 0 && (dy >= dx || held_x == 0)) {
                if (held_y > 0) input |= INPUT_MASK_BACKWARDS;
                else input |= INPUT_MASK_FORWARDS;
            } else if (held_x != 0) {
                if (held_x > 0) input |= INPUT_MASK_RIGHT;
                else input |= INPUT_MASK_LEFT;
            } else if (held_y != 0) {
                if (held_y > 0) input |= INPUT_MASK_BACKWARDS;
                else input |= INPUT_MASK_FORWARDS;
            }
        } else {
            held_x = 0;
            held_y = 0;
        }

        {
            // Merge keyboard arrows into the gamepad direction path — one unified repeat.
            // Don't clear Keys[] — it's populated by async keyboard hook, clearing causes
            // gaps between auto-repeat events that make the ticker stutter.
            if (Keys[KB_UP])    input |= INPUT_MASK_FORWARDS;
            if (Keys[KB_DOWN])  input |= INPUT_MASK_BACKWARDS;
            if (Keys[KB_LEFT])  input |= INPUT_MASK_LEFT;
            if (Keys[KB_RIGHT]) input |= INPUT_MASK_RIGHT;

            // Split into directions (need auto-repeat) and buttons (edge-detect only).
            SLONG dir_mask = INPUT_MASK_LEFT | INPUT_MASK_RIGHT | INPUT_MASK_FORWARDS | INPUT_MASK_BACKWARDS;
            SLONG dir_input = input & dir_mask;
            SLONG btn_input = input & ~dir_mask;
            SLONG last_dir = last_input & dir_mask;
            SLONG last_btn = last_input & ~dir_mask;

            // Buttons: edge-detect only — fire once per press.
            any_button = 0;
            if (btn_input != last_btn) {
                // Cross/A (index 0) = confirm. Triangle/Y = cancel (via INPUT_MASK_CANCEL).
                any_button = the_state.rgbButtons[0];
            } else {
                btn_input = 0;
            }

            // Directions: ticker-based auto-repeat. Reset on release for clean quick taps.
            if (dir_input) {
                if (dir_input != last_dir) {
                    // New direction or first press — fire immediately.
                    dir_ticker = 12;
                } else if (dir_ticker < 1) {
                    // Same direction held long enough — repeat.
                    dir_ticker = 5;
                } else {
                    // Waiting for repeat — suppress.
                    dir_input = 0;
                    dir_ticker--;
                }
            } else {
                // Released — reset immediately so next tap fires cleanly.
                dir_ticker = 0;
            }

            last_input = input;
            input = dir_input | btn_input;

            // Suppress very first movement: PC joysticks have a strange habit of doing
            // one spurious movement on boot-up for some reason.
            if (first_pad) {
                if (any_button)
                    first_pad = 0;
                else if (dir_input) {
                    first_pad = 0;
                    input = 0;
                }
            }
            if (last_button) {
                if (!any_button)
                    last_button = 0;
                else
                    any_button = 0;
            }
        }
    }

    if (grabbing_key && LastKey) {
        MenuData* item = menu_data + menu_state.selected;
        if (LastKey != KB_ESC) {
            UBYTE j;
            for (j = 0; j < menu_state.items; j++)
                if (menu_data[j].Data == LastKey)
                    menu_data[j].Data = 0;
            item->Data = LastKey;
        }
        Keys[LastKey] = 0;
        grabbing_key = 0;
        return 0;
    }
    if (allow_debug_keys) {
        if (Keys[KB_1] || Keys[KB_2] || Keys[KB_3] || Keys[KB_4]) {
            if (Keys[KB_1]) {
                Keys[KB_1] = 0;
                menu_theme = 0;
            }
            if (Keys[KB_2]) {
                Keys[KB_2] = 0;
                menu_theme = 1;
            }
            if (Keys[KB_3]) {
                Keys[KB_3] = 0;
                menu_theme = 2;
            }
            if (Keys[KB_4]) {
                Keys[KB_4] = 0;
                menu_theme = 3;
            }
            ge_set_background_override(screenfull_back);
            FRONTEND_kibble_init();
        }
    }

    if (Keys[KB_END]) {
        Keys[KB_END] = 0;
        MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        menu_state.selected = menu_state.items - 1;
        if (menu_state.mode == FE_MAPSCREEN)
            mission_selected = mission_count - 1;
        while (((menu_data + menu_state.selected)->Type == OT_LABEL) || (((menu_data + menu_state.selected)->Type == OT_BUTTON) && ((menu_data + menu_state.selected)->Choices == (CBYTE*)1)))
            menu_state.selected--;
    }
    if (Keys[KB_HOME]) {
        Keys[KB_HOME] = 0;
        MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        menu_state.selected = 0;
        if (menu_state.mode == FE_MAPSCREEN)
            mission_selected = 0;
        while (((menu_data + menu_state.selected)->Type == OT_LABEL) || (((menu_data + menu_state.selected)->Type == OT_BUTTON) && ((menu_data + menu_state.selected)->Choices == (CBYTE*)1)))
            menu_state.selected++;
    }
    if (input & INPUT_MASK_FORWARDS) {
        MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        if (menu_state.selected > 0)
            menu_state.selected--;
        else
            menu_state.selected = menu_state.items - 1;
        while (((menu_data + menu_state.selected)->Type == OT_LABEL) || (((menu_data + menu_state.selected)->Type == OT_BUTTON) && ((menu_data + menu_state.selected)->Choices == (CBYTE*)1))) {
            if (menu_state.selected > 0)
                menu_state.selected--;
            else
                menu_state.selected = menu_state.items - 1;
        }
        if ((menu_state.mode == FE_MAPSCREEN) && mission_selected)
            mission_selected--;
    }
    if (input & INPUT_MASK_BACKWARDS) {
        MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        if (menu_state.selected < menu_state.items - 1)
            menu_state.selected++;
        else
            menu_state.selected = 0;
        while (((menu_data + menu_state.selected)->Type == OT_LABEL) || (((menu_data + menu_state.selected)->Type == OT_BUTTON) && ((menu_data + menu_state.selected)->Choices == (CBYTE*)1))) {
            if (menu_state.selected < menu_state.items - 1)
                menu_state.selected++;
            else
                menu_state.selected--;
        }
        if ((menu_state.mode == FE_MAPSCREEN) && (mission_selected < mission_count - 1) && (FRONTEND_ValidMission(mission_selected + 1)))
            mission_selected++;
    }

    if (Keys[KB_ENTER] || Keys[KB_SPACE] || Keys[KB_PENTER] || any_button) {
        Keys[KB_ENTER] = 0;
        Keys[KB_SPACE] = 0;
        Keys[KB_PENTER] = 0;
        MenuData* item = menu_data + menu_state.selected;

        if (fade_mode != 2)
            MFX_play_stereo(0, S_MENU_CLICK_END, MFX_REPLACE);
        FRONTEND_stop_xition();

        if ((menu_state.mode == FE_SAVESCREEN) && (item->Data != FE_BACK)) {
            // save_slot is 1-based (1,2,3); menu_state.selected is 0-based.
            save_slot = menu_state.selected + 1;
        }

        if ((menu_state.mode == FE_SAVESCREEN) && (item->Data == FE_MAPSCREEN)) {
            menu_mode_queued = FE_MAPSCREEN;
            menu_state.stackpos = 0;
            menu_thrash = FE_MAINMENU;
        }

        if ((menu_state.mode == FE_SAVE_CONFIRM) && (item->Data == FE_MAPSCREEN)) {
            if (item->LabelID == X_CANCEL) {
                // Cancel was pressed - just exit.
                menu_mode_queued = FE_MAPSCREEN;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
                menu_state.stackpos = 0;
                menu_thrash = FE_MAINMENU;
                return 0;
            }

            CBYTE ttl[_MAX_PATH];
            FRONTEND_fetch_title_from_id(MISSION_SCRIPT, ttl, mission_launch);
            bool bSuccess = FRONTEND_save_savegame(ttl, save_slot);

            if (bSuccess) {
                menu_mode_queued = FE_MAPSCREEN;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
                menu_state.stackpos = 0;
                menu_thrash = FE_MAINMENU;
                return 0;
            } else {
                // Failed save: buzz and return to the main menu.
                MFX_play_stereo(0, S_MENU_CLICK_END, MFX_REPLACE);
                menu_mode_queued = FE_MAPSCREEN;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
                menu_state.stackpos = 0;
                menu_thrash = FE_MAINMENU;
                return 0;
            }
        }
        if (menu_state.mode == FE_MAPSCREEN) {
            if (mission_count > 0) {
                menu_mode_queued = 100 + mission_selected;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
            }
            return 0;
        }

        if (menu_state.mode >= 100) {
            // Starting a mission: stop any briefing audio.
            MFX_QUICK_stop();
            return FE_START;
        }
        if ((menu_state.mode == FE_LOADSCREEN) && (item->Data == FE_SAVE_CONFIRM)) {
            bool bSuccess = FRONTEND_load_savegame(menu_state.selected + 1);
            if (bSuccess) {
                FRONTEND_MissionHierarchy(MISSION_SCRIPT);
                menu_mode_queued = FE_MAPSCREEN;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
                return 0;
            } else {
                // Failed load: buzz and quit to the main menu.
                MFX_play_stereo(0, S_MENU_CLICK_END, MFX_REPLACE);
                menu_mode_queued = FE_MAINMENU;
                fade_mode = 2;
                FRONTEND_kibble_flurry();
                return 0;
            }
        }
        switch (item->Type) {
        case OT_MULTI:
            // Cycle through all options.
            item->Data = (item->Data & ~0xff) | ((item->Data & 0xff) + 1);
            if ((item->Data & 0xff) >= (item->Data >> 8)) {
                item->Data &= ~0xff;
            }
            break;
        case OT_KEYPRESS:
            grabbing_key = 1;
            LastKey = 0;
            break;
        case OT_PADPRESS:
            if (bCanChangeJoypadButtons) {
                grabbing_pad = 1;
                last_input = 0;
            } else {
                // Can't change button - using a predefined setup.
                MFX_play_stereo(0, S_MENU_CLICK_END, MFX_REPLACE);
            }
            break;
        case OT_PADMOVE:
            // Enter pad-move mode.
            m_bMovingPanel = UC_TRUE;
            break;
        case OT_BUTTON:
        case OT_BUTTON_L:
            if (menu_state.mode == FE_START)
                return FE_LOADSCREEN;
            if (item->Data == FE_NO_REALLY_QUIT)
                return -1;
            if (item->Data == FE_BACK)
                FRONTEND_storedata();
            if (item->Data == FE_START)
                return FE_LOADSCREEN;
            if (item->Data == FE_EDITOR)
                return FE_EDITOR;
            if (item->Data == FE_CREDITS)
                return FE_CREDITS;

            FRONTEND_kibble_flurry();

            menu_mode_queued = item->Data;
            fade_mode = 2 | ((item->Data == FE_BACK) ? 4 : 0);
            break;
        case OT_RESET:
            switch (menu_state.mode) {
            case FE_CONFIG_INPUT_KB:
                menu_data[0].Data = 203;
                menu_data[1].Data = 205;
                menu_data[2].Data = 200;
                menu_data[3].Data = 208;
                menu_data[4].Data = 44;
                menu_data[5].Data = 45;
                menu_data[6].Data = 46;
                menu_data[7].Data = 57;
                menu_data[8].Data = 15;
                menu_data[9].Data = 28;
                // gap for label
                menu_data[11].Data = 207;
                menu_data[12].Data = 211;
                menu_data[13].Data = 209;
                menu_data[14].Data = 30;
                break;
            case FE_CONFIG_INPUT_JP:
                menu_data[0].Data = 4;
                menu_data[1].Data = 3;
                menu_data[2].Data = 0;
                menu_data[3].Data = 1;
                menu_data[4].Data = 7;
                menu_data[5].Data = 8;
                menu_data[6].Data = 2;
                // gap for label
                menu_data[8].Data = 6;
                menu_data[9].Data = 9;
                menu_data[10].Data = 10;
                menu_data[11].Data = 5;
                break;
            }
            break;
        }
    }
    if (input & INPUT_MASK_LEFT) {
        MenuData* item = menu_data + menu_state.selected;
        if ((item->Type == OT_SLIDER) && (item->Data > 0)) {
            item->Data--;
            if (item->Data > 0) {
                item->Data--;
            }
        }

        if ((menu_state.mode == FE_MAPSCREEN) && district_selected) {
            scan = district_selected - 1;
            while ((scan > 0) && !district_valid[scan])
                scan--;
            if (district_valid[scan]) {
                district_selected = scan;
                FRONTEND_MissionList(MISSION_SCRIPT, districts[district_selected][2]);
                MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
            }
        }

        if ((item->Type == OT_MULTI) && ((item->Data & 0xff) > 0)) {
            item->Data--;
            MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        }

        if (menu_state.mode == FE_CONFIG_VIDEO)
            FRONTEND_gamma_update();
        if ((menu_state.mode == FE_CONFIG_AUDIO) && !menu_state.selected) {
            MFX_play_stereo(1, S_TRAFFIC_CONE, 0);
        }
    }
    if (input & INPUT_MASK_RIGHT) {
        MenuData* item = menu_data + menu_state.selected;
        if ((item->Type == OT_SLIDER) && (item->Data < 255)) {
            item->Data++;
            if (item->Data < 255) {
                item->Data++;
            }
        }

        if ((menu_state.mode == FE_MAPSCREEN) && (district_selected < district_count - 1)) {
            scan = district_selected + 1;
            while ((scan < district_count - 1) && !district_valid[scan])
                scan++;
            if (district_valid[scan]) {
                district_selected = scan;
                FRONTEND_MissionList(MISSION_SCRIPT, districts[district_selected][2]);
                MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
            }
        }

        if ((item->Type == OT_MULTI) && ((item->Data & 0xff) < (item->Data >> 8) - 1)) {
            item->Data++;
            MFX_play_stereo(1, S_MENU_CLICK_START, MFX_REPLACE);
        }

        if (menu_state.mode == FE_CONFIG_VIDEO)
            FRONTEND_gamma_update();
        if ((menu_state.mode == FE_CONFIG_AUDIO) && !menu_state.selected) {
            MFX_play_stereo(1, S_TRAFFIC_CONE, 0);
        }
    }
    if (Keys[KB_ESC] || (input & INPUT_MASK_CANCEL)) {
        Keys[KB_ESC] = 0;
        if (fade_mode != 6)
            MFX_play_stereo(1, S_MENU_CLICK_END, MFX_REPLACE);
        if (fade_mode == 2) // cancel a transition
        {
            fade_mode = 1;
            menu_mode_queued = menu_state.mode;
            return 0;
        }
        if (menu_state.stackpos) {
            switch (menu_state.mode) {
            case FE_CONFIG_VIDEO: // eidos want ESC to store opts
                FRONTEND_store_video_data();
                break;
            case FE_CONFIG_AUDIO:
                MFX_stop(WEATHER_REF, MFX_WAVE_ALL);
                MFX_set_volumes(menu_data[0].Data >> 1, menu_data[1].Data >> 1, menu_data[2].Data >> 1);
                break;
            }

            // Store any options settings.
            FRONTEND_storedata();

            menu_mode_queued = FE_BACK;
        } else {
            menu_mode_queued = FE_QUIT;
        }

        if ((menu_state.mode == FE_SAVESCREEN) && !menu_state.stackpos) {
            menu_thrash = FE_MAINMENU;
            menu_mode_queued = FE_MAPSCREEN;
        }
        fade_mode = 6;

        FRONTEND_kibble_flurry();
    }

    return 0;
}

// uc_orig: FRONTEND_init (fallen/Headers/frontend.h)
// Initialises frontend system: language, mission script, fonts, themes, music, kibble.
// Called on first entry and on re-entry after a mission ends.
void FRONTEND_init(bool bGoToTitleScreen)
{
    static bool bFirstTime = UC_TRUE;

    dwAutoPlayFMVTimeout = sdl3_get_ticks() + AUTOPLAY_FMV_DELAY;

    // Stop the music while loading stuff.
    stop_all_fx_and_music();

    // These two are so that when a mission ends with a camera still active, the music
    // stuff actually works properly.
    extern UBYTE EWAY_conv_active;
    extern SLONG EWAY_cam_active;
    EWAY_conv_active = UC_FALSE;
    EWAY_cam_active = UC_FALSE;

    TICK_RATIO = (1 << TICK_SHIFT);

    // Set up the current language.
    switch (0) {
    case 0:
        pcSpeechLanguageDir = "talk2/";
        break;
    case 1:
        pcSpeechLanguageDir = "talk2_french/";
        break;
    default:
        ASSERT(UC_FALSE);
        break;
    }

    // Reset the transition buffer's contents.
    lpFRONTEND_show_xition_LastBlit = NULL;

    CBYTE *str, *lang = ENV_get_value_string("language");

    if (!lang)
        lang = "text/lang_english.txt";
    XLAT_load(lang);
    XLAT_init();

    IsEnglish = !oc_stricmp(XLAT_str(X_THIS_LANGUAGE_IS), "English");

    str = menu_choice_yesno;
    strcpy(str, XLAT_str(X_NO));
    str += strlen(str) + 1;
    strcpy(str, XLAT_str(X_YES));

    strcpy(MISSION_SCRIPT, "data/");
    lang = XLAT_str(X_THIS_LANGUAGE_IS);
    if (strcmp(lang, "English") == 0)
        strcat(MISSION_SCRIPT, "urban");
    else
        strcat(MISSION_SCRIPT, lang);
    strcat(MISSION_SCRIPT, ".sty");

    // Load the mission script into memory so we don't have to scan a file all the time!
    CacheScriptInMemory(MISSION_SCRIPT);

    MENUFONT_Load("olyfont2.tga", POLY_PAGE_NEWFONT_INVERSE, frontend_fonttable);

    void MENUFONT_MergeLower(void);
    MENUFONT_MergeLower();

    menu_theme = 0;

    ge_set_background_override(screenfull_back);

    memset(kibble, 0, sizeof(kibble));
    memset(&menu_state, 0, sizeof(menu_state));
    if (!complete_point)
        memset(mission_hierarchy, 0, 60);
    menu_state.mode = -1;

    FRONTEND_CacheMissionList(MISSION_SCRIPT);

    ge_clear(false, true);

    memset(menu_choice_scanner, 0, 255);
    XLAT_str(X_CAMERA, menu_choice_scanner);
    lang = menu_choice_scanner + strlen(menu_choice_scanner) + 1;
    XLAT_str(X_CHARACTER, lang);

    MUSIC_mode(MUSIC_MODE_FRONTEND);

    FRONTEND_scr_new_theme(
        menu_back_names[menu_theme],
        menu_map_names[menu_theme],
        menu_brief_names[menu_theme],
        menu_config_names[menu_theme]);

    if (!bFirstTime) {
        ge_set_background_override(screenfull_back);
    }

    SLONG fx, amb, mus;
    MFX_get_volumes(&fx, &amb, &mus);
    MUSIC_gain(mus);

    FRONTEND_districts(MISSION_SCRIPT);
    FRONTEND_MissionHierarchy(MISSION_SCRIPT);
    FRONTEND_kibble_init();

    if (m_bGoIntoSaveScreen) {
        // Just won a mission - going into save game.
        FRONTEND_mode(FE_SAVESCREEN);
        m_bGoIntoSaveScreen = UC_FALSE;
    } else {
        // Frontend menu.
        FRONTEND_mode(FE_MAINMENU);
    }

    // Stop all the music - about to start it again, properly.
    stop_all_fx_and_music();
    MUSIC_reset();
    MUSIC_mode(0);
    MUSIC_mode_process();
    MUSIC_mode(MUSIC_MODE_FRONTEND);

    if (mission_launch) {
        // Just won a mission - going into save game.
        return;
    }

    bFirstTime = UC_FALSE;
}

// uc_orig: FRONTEND_level_lost (fallen/Headers/frontend.h)
// Called by Game.cpp after GS_LEVEL_LOST exit. Restores mission_launch to pre-launch value,
// resets menu state and goes to main menu (no save screen).
void FRONTEND_level_lost()
{
    mission_launch = previous_mission_launch;
    // Start up the kibble again.
    FRONTEND_kibble_init();
    memset(&menu_state, 0, sizeof(menu_state));
    menu_state.mode = -1;
    FRONTEND_mode(FE_MAINMENU);
}

// uc_orig: FRONTEND_level_won (fallen/Headers/frontend.h)
// Called by Game.cpp after GS_LEVEL_WON exit. Marks mission as complete, accumulates
// stat deltas (Constitution/Strength/Stamina/Skill) via anti-farm best_found[] system,
// then triggers the save screen.
void FRONTEND_level_won()
{
    ASSERT(mission_launch < 50);

    // update hierarchy data.
    mission_hierarchy[mission_launch] |= 2; // complete

    if (!g_bPunishMePleaseICheatedOnThisLevel) {
        // Update Darci's stats: only credit deltas that exceed previous-best per mission slot.
        if (1) // NET_PERSON(0)->Genus.Person->PersonType==PERSON_DARCI)
        {
            SLONG found;
            found = NET_PLAYER(0)->Genus.Player->Constitution - the_game.DarciConstitution;
            ASSERT(found >= 0);

            if (found > best_found[mission_launch][0]) {
                the_game.DarciConstitution += found - best_found[mission_launch][0];
                best_found[mission_launch][0] = found;
            }

            found = NET_PLAYER(0)->Genus.Player->Strength - the_game.DarciStrength;
            ASSERT(found >= 0);

            if (found > best_found[mission_launch][1]) {
                the_game.DarciStrength += found - best_found[mission_launch][1];
                best_found[mission_launch][1] = found;
            }

            found = NET_PLAYER(0)->Genus.Player->Stamina - the_game.DarciStamina;
            ASSERT(found >= 0);

            if (found > best_found[mission_launch][2]) {
                the_game.DarciStamina += found - best_found[mission_launch][2];
                best_found[mission_launch][2] = found;
            }

            found = NET_PLAYER(0)->Genus.Player->Skill - the_game.DarciSkill;
            ASSERT(found >= 0);

            if (found > best_found[mission_launch][3]) {
                the_game.DarciSkill += found - best_found[mission_launch][3];
                best_found[mission_launch][3] = found;
            }
        } else {
            the_game.RoperConstitution = NET_PLAYER(0)->Genus.Player->Constitution;
            the_game.RoperStrength = NET_PLAYER(0)->Genus.Player->Strength;
            the_game.RoperStamina = NET_PLAYER(0)->Genus.Player->Stamina;
            the_game.RoperSkill = NET_PLAYER(0)->Genus.Player->Skill;
        }
    }

    // Start up the kibble again.
    FRONTEND_kibble_init();
    memset(&menu_state, 0, sizeof(menu_state));
    menu_state.mode = -1;
    if (complete_point < mission_launch)
        complete_point = mission_launch;
    FRONTEND_MissionHierarchy(MISSION_SCRIPT);
    FRONTEND_mode(FE_SAVESCREEN);
    m_bGoIntoSaveScreen = UC_TRUE;
}

// uc_orig: FRONTEND_playambient3d (fallen/Source/frontend.cpp)
// Plays a sound at a random angle around the listener.
// height==1: also randomises vertical position (aerial sounds like sirens).
static void FRONTEND_playambient3d(SLONG channel, SLONG wave_id, SLONG flags, UBYTE height)
{
    SLONG angle = Random() & 2047;

    SLONG x = (COS(angle) << 4);
    SLONG y = 0;
    SLONG z = (SIN(angle) << 4);

    if (height == 1)
        y += (512 + (Random() & 1023)) << 8;

    MFX_play_xyz(channel, wave_id, 0, x, y, z);
}

// uc_orig: FRONTEND_sound (fallen/Source/frontend.cpp)
// Per-frame frontend audio: plays looped ambient wind, periodically spawns a random siren
// from a random direction, and keeps the music volume in sync with the audio config slider.
static void FRONTEND_sound(void)
{
    static SLONG siren_time = 100;
    SLONG wave_id;

    MFX_play_ambient(WEATHER_REF, S_WIND_START, MFX_LOOPED | MFX_QUEUED);
    MFX_set_gain(WEATHER_REF, S_AMBIENCE_END, 255);
    MUSIC_gain(menu_data[2].Data >> 1);
    MFX_set_volumes(menu_data[0].Data >> 1, menu_data[1].Data >> 1, menu_data[2].Data >> 1);

    siren_time--;
    if (siren_time < 0) {
        wave_id = S_SIREN_START + (Random() % 4);
        siren_time = 300 + ((Random() & 0xFFFF) >> 5);
        FRONTEND_playambient3d(SIREN_REF, wave_id, 0, 1);
    }
    MFX_set_listener(0, 0, 0, 0, 0, 0);
    MFX_update();
}

// uc_orig: FRONTEND_diddle_stats (fallen/Source/frontend.cpp)
// No-op � originally planned to adjust player stats when cheating.
static void FRONTEND_diddle_stats(void)
{
}

// uc_orig: FRONTEND_diddle_music (fallen/Source/frontend.cpp)
// Sets MUSIC_bodge_code based on mission filename to trigger mission-specific music variants.
// Code 1=fight/FTutor, 2=Assault, 3=testdrive, 4=Finale1.
static void FRONTEND_diddle_music(void)
{
    MUSIC_bodge_code = 0;
    if (strstr(STARTSCR_mission, "levels/fight") || strstr(STARTSCR_mission, "levels/FTutor"))
        MUSIC_bodge_code = 1;
    else if (strstr(STARTSCR_mission, "levels/Assault"))
        MUSIC_bodge_code = 2;
    else if (strstr(STARTSCR_mission, "levels/testdrive"))
        MUSIC_bodge_code = 3;
    else if (strstr(STARTSCR_mission, "levels/Finale1"))
        MUSIC_bodge_code = 4;
}

// uc_orig: FRONTEND_loop (fallen/Headers/frontend.h)
// Main frontend per-frame logic: advance fade state, kibble, render, audio, input.
// Returns: 0 (continue), STARTS_EXIT (quit), STARTS_EDITOR, STARTS_START (launch mission).
SBYTE FRONTEND_loop()
{
    SBYTE res;

    // BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t
    static uint64_t last = 0;
    static uint64_t now = 0;

    SLONG millisecs;

    now = sdl3_get_ticks();

    if (last < now - 250) {
        last = now - 250;
    }

    millisecs = now - last;
    last = now;

    // How fast should the fade state fade?
    SLONG fade_speed = (millisecs >> 3);

    if (fade_speed < 1) {
        fade_speed = 1;
    }

    switch (fade_mode & 3) {
    case 1:
        if (fade_state < 63) {
            fade_state += fade_speed;

            if (fade_state > 63) {
                fade_state = 63;
            }
        } else {
            FRONTEND_stop_xition();

            fade_mode = 0;
        }
        break;

    case 2:
        if (fade_state > 0) {
            fade_state -= fade_speed;

            if (fade_state < 0) {
                fade_state = 0;
            }
        } else {
            FRONTEND_mode(menu_mode_queued);
        }
        break;
    }
    fade_rgb = (((SLONG)fade_state * 2) << 24) | 0xFFFFFF;

    {
        FRONTEND_kibble_process();
    }

    PolyPage::DisableAlphaSort();
    FRONTEND_display();
    if ((menu_state.mode == FE_CONFIG_AUDIO) && (fade_mode == 0)) {
        FRONTEND_sound();
    } else {
        MFX_set_listener(0, 0, 0, 0, 0, 0);
        MFX_update();
    }
    PolyPage::EnableAlphaSort();
    res = FRONTEND_input();
    MUSIC_mode_process();

    // If the player just committed to a transition (start mission, exit,
    // change language, …), cancel the overlay pending bit set by
    // FRONTEND_display() above. The caller (ATTRACT_loadscreen_init,
    // outro, etc.) takes over rendering from here and runs its own
    // flip(s) — without this clear, the very first of those flips
    // would consume the pending bit and paint the menu overlay over
    // whatever the caller was trying to show (loadscreen image, "THE
    // END" splash, …). Symptom: menu briefly visible *under* the
    // loadscreen "Loading" text on the way into a mission, or under
    // the outro's first frame.
    extern bool g_frontend_overlay_pending;
    if (res != 0) {
        g_frontend_overlay_pending = false;
    }

    // Debug cheat shortcuts: Ctrl+Shift+Numpad+/Numpad* advance complete_point.
    if (ControlFlag && ShiftFlag) {
        if (Keys[KB_PPLUS]) {
            Keys[KB_PPLUS] = 0;
            complete_point++;
            FRONTEND_MissionHierarchy(MISSION_SCRIPT);
            cheating = 1;
        }
        if (Keys[KB_ASTERISK]) {
            Keys[KB_ASTERISK] = 0;
            complete_point = 40;
            FRONTEND_MissionHierarchy(MISSION_SCRIPT);
            cheating = 1;
        }
    }

    if (res == FE_NO_REALLY_QUIT)
        return STARTS_EXIT;
    if (res == FE_EDITOR)
        return STARTS_EDITOR;
    if (res == FE_LOADSCREEN)
        return STARTS_START;

    // Mission selected (mode >= 100, ENTER pressed): look up per-mission data and start loading.
    // whattoload[] = per-mission table: {filename, dontload_bitmask, has_balrog}.
    // DONT_load is always forced to 0 (load everything); the mask is preserved for documentation.
    // this_level_has_the_balrog / this_level_has_bane / is_semtex are read by level setup code.
    if (res == FE_START) {
        struct
        {
            CBYTE* mission;
            SLONG dontload;
            SLONG has_balrog;

        } whattoload[] = {
            { "testdrive1a.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "FTutor1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Assault1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "police1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "police2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Bankbomb1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "police3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Gangorder2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "police4.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "bball2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "wstores1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "e3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | 0, UC_FALSE },
            { "westcrime1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "carbomb1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "botanicc.ucm", (1 << PERSON_MIB1) | 0, UC_FALSE },
            { "Semtex.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "AWOL1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "mission2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "park2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "MIB.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "skymiss2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "factory1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "estate2.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "Stealtst1.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "snow2.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "Gordout1.ucm", (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Baalrog3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_TRUE },
            { "Finale1.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Gangorder1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "FreeCD1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "jung3.ucm", (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | 0, UC_FALSE },
            { "fight1.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "fight2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "testdrive2.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "testdrive3.ucm", (1 << PERSON_MIB1) | (1 << PERSON_TRAMP) | (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },
            { "Album1.ucm", (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | 0, UC_FALSE },

            { "!" }
        };

        SLONG index_into_the_whattoload_array;

        previous_mission_launch = mission_launch;
        strcpy(STARTSCR_mission, "levels/");
        strcat(STARTSCR_mission, FRONTEND_MissionFilename(MISSION_SCRIPT, menu_state.mode - 100));

        index_into_the_whattoload_array = -1;

        SLONG i;

        for (i = 0; whattoload[i].mission[0] != '!'; i++) {
            if (strcmp(FRONTEND_MissionFilename(MISSION_SCRIPT, menu_state.mode - 100), whattoload[i].mission) == 0) {
                ASSERT(index_into_the_whattoload_array == -1);

                index_into_the_whattoload_array = i;
            }
        }

        ASSERT(index_into_the_whattoload_array != -1);

        ASSERT(WITHIN(index_into_the_whattoload_array, 0, 35));

        this_level_has_the_balrog = whattoload[index_into_the_whattoload_array].has_balrog;
        this_level_has_bane = (index_into_the_whattoload_array == 27); // Just in the finale...
        DONT_load = whattoload[index_into_the_whattoload_array].dontload;

        // Override: load all people regardless of the per-mission mask.
        DONT_load = 0;

        // If doing all levels, don't use DONT_load. The two don't mix.
        ASSERT(!DONT_load);

        if (index_into_the_whattoload_array == 20) // skymiss2.ucm
        {
            is_semtex = 1;
        } else {
            is_semtex = 0;
        }

        if (index_into_the_whattoload_array == 31 || index_into_the_whattoload_array == 32 || index_into_the_whattoload_array == 1) {
            // No violence for Album1, Gangorder1, FTutor1.
            VIOLENCE = UC_FALSE;
        } else {
            // Default violence.
            VIOLENCE = UC_TRUE;
        }

        if (cheating)
            FRONTEND_diddle_stats();

        FRONTEND_diddle_music();
        menu_state.stackpos = 0;
        menu_thrash = 0;
        return STARTS_START;
    }

    if (res == FE_CREDITS) {
        {
            extern void OS_hack(void);
            OS_hack();
            MUSIC_mode(MUSIC_MODE_FRONTEND);
            FRONTEND_kibble_init();
        }
    }

    return 0;
}
