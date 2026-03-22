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
