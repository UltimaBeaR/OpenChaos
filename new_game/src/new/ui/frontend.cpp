// Chunk 1: globals, script helpers, screen management, draw helpers,
// kibble particle system (lines 1-1579 of original frontend.cpp).
// Remaining: FRONTEND_kibble_process, FRONTEND_fetch_title_from_id,
// FRONTEND_display, FRONTEND_init, FRONTEND_loop, etc.

#include "DDLib.h"
#include "ui/frontend.h"
#include "ui/frontend_globals.h"

// Temporary: xlat_str.h for X_* string IDs
#include "fallen/Headers/xlat_str.h"
// Temporary: startscr.h for StartMenu, MissionListCallback, etc.
#include "fallen/Headers/startscr.h"
// Temporary: interfac.h for init_joypad_config, DIJOYSTATE
#include "fallen/Headers/interfac.h"
// Temporary: menufont.h for MENUFONT_Draw, MENUFONT_Dimensions, etc.
#include "fallen/DDEngine/Headers/menufont.h"
// Temporary: font2d.h for FONT2D_DrawStringRightJustify
#include "fallen/DDEngine/Headers/font2d.h"
// Temporary: polypage.h for POLY_add_quad
#include "fallen/DDEngine/Headers/polypage.h"
// Temporary: poly.h for POLY_PAGE_* constants, POLY_Point
#include "fallen/DDEngine/Headers/poly.h"
// Temporary: fmatrix.h for FMATRIX_calc, FMATRIX_MUL
#include "fallen/Headers/FMatrix.h"
// Temporary: DrawXtra.h for DRAW2D_Box
#include "fallen/DDEngine/Headers/DrawXtra.h"
// Temporary: GDisplay.h for UseBackSurface, the_display, RealDisplayWidth/Height
#include "fallen/DDLibrary/Headers/GDisplay.h"
// Temporary: io.h for FileOpen, FileRead, FileClose, FileSeek, FileWrite
#include "fallen/Headers/io.h"
// Temporary: music.h for MUSIC_mode, MUSIC_MODE_FRONTEND, stop_all_fx_and_music
#include "fallen/Headers/music.h"
// Temporary: game.h for MemAlloc, MemFree, Random, DATA_DIR
#include "fallen/Headers/Game.h"
// Temporary: dclowlevel.h for DCL types used in some inline display code
#include "fallen/DDLibrary/Headers/DCLowLevel.h"

extern void CopyBackground(UBYTE* image_data, IDirectDrawSurface4* surface);

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

// uc_orig: FRONTEND_scr_add (fallen/Source/frontend.cpp)
// Creates a system-memory DirectDraw surface matching the back buffer and
// copies image_data into it. Used to hold the menu background images.
void FRONTEND_scr_add(LPDIRECTDRAWSURFACE4* screen, UBYTE* image_data)
{
    DDSURFACEDESC2 back;
    DDSURFACEDESC2 mine;

    InitStruct(back);

    the_display.lp_DD_BackSurface->GetSurfaceDesc(&back);

    InitStruct(mine);

    mine.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    mine.dwWidth = back.dwWidth;
    mine.dwHeight = back.dwHeight;
    mine.ddpfPixelFormat = back.ddpfPixelFormat;
    mine.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;

    HRESULT result = the_display.lp_DD4->CreateSurface(&mine, screen, NULL);

    if (FAILED(result)) {
        ASSERT(UC_FALSE);
        *screen = NULL;
        return;
    }

    CopyBackground(image_data, *screen);

    return;
}

// uc_orig: FRONTEND_scr_img_load_into_screenfull (fallen/Source/frontend.cpp)
// Loads a raw 640x480 BGR image file into a new DirectDraw surface.
// Image is read bottom-up (standard BMP/TGA orientation).
void FRONTEND_scr_img_load_into_screenfull(CBYTE* name, LPDIRECTDRAWSURFACE4* screen)
{
    MFFileHandle image_file;
    SLONG height;
    CBYTE fname[200];
    UBYTE* image;
    UBYTE* image_data;

    *screen = NULL;

    sprintf(fname, "%sdata\\%s", DATA_DIR, name);

    image_data = (UBYTE*)MemAlloc(640 * 480 * 3);

    if (image_data) {

        image_file = FileOpen(fname);
        if (image_file != nullptr) {
            FileSeek(image_file, SEEK_MODE_BEGINNING, 18);
            image = image_data + (640 * 479 * 3);
            for (height = 480; height; height--, image -= (640 * 3)) {
                FileRead(image_file, image, 640 * 3);
            }
            FileClose(image_file);
        } else {
        }

        FRONTEND_scr_add(screen, image_data);

        MemFree(image_data);
    } else {
    }
}

// uc_orig: FRONTEND_scr_unload_theme (fallen/Source/frontend.cpp)
// Releases all four background DirectDraw surfaces.
void FRONTEND_scr_unload_theme()
{
    the_display.lp_DD_Background_use_instead = NULL;

    if (screenfull_back) {
        screenfull_back->Release();
        screenfull_back = NULL;
    }
    if (screenfull_map) {
        screenfull_map->Release();
        screenfull_map = NULL;
    }
    if (screenfull_brief) {
        screenfull_brief->Release();
        screenfull_brief = NULL;
    }
    if (screenfull_config) {
        screenfull_config->Release();
        screenfull_config = NULL;
    }

    screenfull = NULL;
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

    if (the_display.lp_DD_Background_use_instead == screenfull_back) {
        last = 1;
    }
    if (the_display.lp_DD_Background_use_instead == screenfull_map) {
        last = 2;
    }
    if (the_display.lp_DD_Background_use_instead == screenfull_brief) {
        last = 3;
    }
    if (the_display.lp_DD_Background_use_instead == screenfull_config) {
        last = 4;
    }

    FRONTEND_scr_unload_theme();

    FRONTEND_scr_img_load_into_screenfull(fname_back, &screenfull_back);
    FRONTEND_scr_img_load_into_screenfull(fname_map, &screenfull_map);
    FRONTEND_scr_img_load_into_screenfull(fname_brief, &screenfull_brief);
    FRONTEND_scr_img_load_into_screenfull(fname_config, &screenfull_config);

    switch (last) {
    case 1:
        the_display.lp_DD_Background_use_instead = screenfull_back;
        break;
    case 2:
        the_display.lp_DD_Background_use_instead = screenfull_map;
        break;
    case 3:
        the_display.lp_DD_Background_use_instead = screenfull_brief;
        break;
    case 4:
        the_display.lp_DD_Background_use_instead = screenfull_config;
        break;
    default:
        ASSERT(UC_FALSE);
        break;
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
void FRONTEND_init_xition(void)
{
    MidX = RealDisplayWidth / 2;
    MidY = RealDisplayHeight / 2;
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
    RECT rc;

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
        rc.top = 0;
        rc.bottom = RealDisplayHeight;
        rc.left = 0;
        if (RealDisplayWidth == 640) {
            rc.right = fade_state * 10;
        } else {
            rc.right = fade_state * 10 * RealDisplayWidth / 640;
        }
        if (rc.right > 0) {
            bDoBlit = UC_TRUE;
        }
    }

    if (bDoBlit) {
        the_display.lp_DD_BackSurface->Blt(&rc, screenfull, &rc, DDBLT_WAIT, 0);
    }
}

extern UBYTE* image_mem;

// uc_orig: FRONTEND_stop_xition (fallen/Source/frontend.cpp)
// Finalizes a transition by making the full background surface active.
void FRONTEND_stop_xition()
{
    switch (menu_state.mode) {
    case FE_QUIT:
    case FE_MAINMENU:
        UseBackSurface(screenfull_back);
        break;
    case FE_CONFIG:
    case FE_CONFIG_VIDEO:
    case FE_CONFIG_AUDIO:
    case FE_CONFIG_INPUT_KB:
    case FE_CONFIG_INPUT_JP:
    case FE_LOADSCREEN:
    case FE_SAVESCREEN:
        UseBackSurface(screenfull_config);
        break;
    case FE_MAPSCREEN:
        UseBackSurface(screenfull_map);
        break;
    default:
        if (menu_state.mode >= 100) {
            UseBackSurface(screenfull_brief);
        } else {
            UseBackSurface(screenfull_config);
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

        if (GetTickCount() & 0x200) {
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
    pp[0].specular = 0;
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
            pp[0].specular = 0;
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
// Uses Windows GetKeyNameText() to convert the scancode to a readable string.
void FRONTEND_DrawKey(MenuData* md)
{
    SLONG x, y, dy, c0, rgb;
    CBYTE str[25];
    rgb = FRONTEND_fix_rgb(fade_rgb, (grabbing_key && ((menu_data + menu_state.selected == md) && ((GetTickCount() & 0x7ff) < 0x3ff))));
    dy = md->Y + menu_state.base - menu_state.scroll;

    c0 = md->Data;
    if (c0 & 0x80) {
        c0 = md->Data & ~0x80;
        c0 <<= 16;
        c0 |= 1 << 24;
    } else {
        c0 <<= 16;
    }

    GetKeyNameText(c0, str, 25);

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
    SLONG x, y, dy, c0, rgb;
    CBYTE str[20];
    rgb = FRONTEND_fix_rgb(fade_rgb, (grabbing_pad && ((menu_data + menu_state.selected == md) && ((GetTickCount() & 0x7ff) < 0x3ff))));
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

    ZeroMemory(kibble, sizeof(kibble));

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

// Temporary: drive.h for GetSpeechPath
#include "engine/io/drive.h"
// Temporary: mfx.h for MFX_QUICK_play, MFX_get_volumes, MFX_play_stereo
#include "engine/audio/mfx.h"
// Temporary: sound_id.h for S_TUNE_BONUS
#include "assets/sound_id.h"
// Temporary: dd_manager.h for DDDriverInfo, DDDriverManager
#include "engine/graphics/graphics_api/dd_manager.h"
// Temporary: dd_manager_globals.h for the_manager
#include "engine/graphics/graphics_api/dd_manager_globals.h"
// Temporary: wind_procs.h for ChangeDDInfo
#include "engine/graphics/graphics_api/wind_procs.h"
// Temporary: gd_display.h for RealDisplayWidth, DisplayBPP
#include "engine/graphics/graphics_api/gd_display.h"
// Temporary: aeng.h for AENG_get_detail_levels, AENG_set_detail_levels
#include "engine/graphics/pipeline/aeng.h"
// Temporary: env.h for ENV_get_value_number, ENV_set_value_number
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

    // BUGFIX-OC-TICK-OVERFLOW: use DWORD (unsigned) to prevent signed underflow
    // when now wraps around ~49 days of uptime.
    static DWORD last = 0;
    static DWORD now = 0;

    ASSERT(kibble != NULL);

    now = GetTickCount();

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

    CreateDirectory("saves", NULL);

    sprintf(fn, "saves\\slot%d.wag", slot);
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

    sprintf(fn, "saves\\slot%d.wag", slot);
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
    WIN32_FIND_DATA data;
    HANDLE handle;
    BOOL ok;
    SLONG c0;
    MenuData* md = menu_data;
    CBYTE* str = menu_buffer;
    SLONG x, y, y2 = 0;
    FILETIME time, high_time = { 0, 0 };

    for (c0 = 1; c0 < 11; c0++) {
        md->Type = OT_BUTTON;
        md->Data = FE_SAVE_CONFIRM;
        md->Choices = (CBYTE*)0;

        MFFileHandle file;
        sprintf(dir, "saves\\slot%d.wag", c0);
        file = FileOpen(dir);
        GetFileTime(file, NULL, NULL, &time);
        if (file != FILE_OPEN_ERROR) {
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
        if (CompareFileTime(&time, &high_time) > 0) {
            high_time = time;
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
    MFFileHandle file;
    CBYTE *text, *str = menu_buffer;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();
    MenuData* md = menu_data;
    SLONG x, y, y2 = 0;

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
    MFFileHandle file;
    SLONG best_score;
    CBYTE* text;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();
    MenuData* md = menu_data;
    UBYTE i = 0, j, flag;
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
            UseBackSurface(screenfull_back);
            break;
        case FE_CONFIG:
        case FE_CONFIG_VIDEO:
        case FE_CONFIG_AUDIO:
        case FE_CONFIG_OPTIONS:
        case FE_CONFIG_INPUT_KB:
        case FE_CONFIG_INPUT_JP:
        case FE_LOADSCREEN:
        case FE_SAVESCREEN:
            UseBackSurface(screenfull_config);
            break;
        case FE_MAPSCREEN:
            UseBackSurface(screenfull_map);
            break;
        default:
            UseBackSurface(screenfull_brief);
        }
        FRONTEND_kibble_init();
    }

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    ZeroMemory(district_valid, sizeof(district_valid));
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

            _strlwr(mdata->fn);

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
                    if (stricmp(mdata->fn, suggest_order[order]) == 0) {
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
    MFFileHandle file;
    CBYTE *text, *str = menu_buffer;
    SLONG ver;
    MissionData* mdata = MFnew<MissionData>();
    MenuData* md = menu_data;
    SLONG x, y, y2 = 0;

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
        strcpy(path, GetSpeechPath());
        strcat(path, "talk2\\");
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
    MFFileHandle file;
    CBYTE *text, *str;
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
    MFFileHandle file;
    CBYTE *text, *str = menu_buffer;
    SLONG ver, mapx = 0, mapy = 0;
    MenuData* md = menu_data;
    SLONG x, y;
    UBYTE i = 0, ct, index = 0;
    SWORD temp_dist[40][3];
    UBYTE crap_remap[640][10];

    text = (CBYTE*)MemAlloc(4096);
    memset(text, 0, 4096);

    district_count = 0;
    ZeroMemory(crap_remap, sizeof(crap_remap));

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

// uc_orig: FRONTEND_do_drivers (fallen/Source/frontend.cpp)
// Populates the display driver/resolution/bit-depth menus by querying
// the DirectDraw driver list and current display settings.
void FRONTEND_do_drivers()
{
    SLONG result, count = 0, selected = 0;
    ChangeDDInfo* change_info;
    DDDriverInfo *current_driver = 0,
                 *driver_list;
    GUID* DD_guid;
    TCHAR szBuff[80];
    CBYTE *str = menu_buffer, *str_tmp;

    switch (RealDisplayWidth) {
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
    CurrentBitDepth = DisplayBPP;

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

    selected = 0;

    current_driver = the_manager.CurrDriver;

    driver_list = the_manager.DriverList;
    while (driver_list) {
        if (driver_list->IsPrimary())
            wsprintf(szBuff, TEXT("%s (Primary)"), driver_list->szName);
        else
            wsprintf(szBuff, TEXT("%s"), driver_list->szName);

        strcpy(str, szBuff);
        str += strlen(str) + 1;

        if (current_driver == driver_list)
            selected = count;

        count++;
        driver_list = driver_list->Next;
    }
    menu_data[2].Data = selected | (count << 8);
    menu_data[2].Choices = str_tmp;
}

// uc_orig: FRONTEND_gamma_update (fallen/Source/frontend.cpp)
// Applies the current gamma slider value to the display.
void FRONTEND_gamma_update()
{
    if (menu_state.selected == GammaIndex)
        the_display.SetGamma(menu_data[GammaIndex].Data & 0xff, 256);
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

    int a, b;
    the_display.GetGamma(&a, &b);
    menu_data[GammaIndex].Data = a;
}

// ---- Mode switching ---------------------------------------------------------

// uc_orig: FRONTEND_mode (fallen/Source/frontend.cpp)
// Switches the frontend to a new screen/mode. Pushes current mode onto a stack
// so FE_BACK (-2) can return to it. Clears menu_data[] and repopulates it for
// the new mode. mode >= 100 is shorthand for MISSIONBRIEFING for mission (mode-100).
void FRONTEND_mode(SBYTE mode, bool bDoTransition)
{
    dwAutoPlayFMVTimeout = timeGetTime() + AUTOPLAY_FMV_DELAY;

    SBYTE last = menu_state.mode;
    fade_mode = 1;
    ZeroMemory(menu_data, sizeof(menu_data));
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
            UseBackSurface(screenfull_config);
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
        int a, b, c, d, e, f, g, h, i, j, k;
        if (bDoTransition) {
            FRONTEND_init_xition();
        }
        FRONTEND_easy(mode);
        if (the_display.IsGammaAvailable())
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
