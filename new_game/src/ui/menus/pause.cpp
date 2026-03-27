#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "assets/xlat_str.h"
#include "engine/input/keyboard.h"
#include "ui/menus/pause.h"

#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/graphics_engine/d3d/d3d_texture.h"

#include "engine/graphics/text/font2d.h"
#include "ui/hud/panel.h"
#include "ui/hud/panel_globals.h"

// Joystick axis centre value and deadzone thresholds.
#define AXIS_CENTRE 32768
#define NOISE_TOLERANCE 1024
#define AXIS_MIN (AXIS_CENTRE - NOISE_TOLERANCE)
#define AXIS_MAX (AXIS_CENTRE + NOISE_TOLERANCE)

#define NORMAL_COLOUR 0xff9f9f
#define SELECT_COLOUR 0xffffff

// Currently highlighted menu item index.
// uc_orig: selected (fallen/Source/pause.cpp)
static SWORD selected;

// Bitmask values for pause input events decoded from keyboard/joystick this frame.
#define PAUSED_KEY_START (1 << 0)
#define PAUSED_KEY_UP    (1 << 1)
#define PAUSED_KEY_DOWN  (1 << 2)
#define PAUSED_KEY_OKAY  (1 << 3)

// Menu item indices.
#define PAUSE_MENU_RESUME  0
#define PAUSE_MENU_RESTART 1
#define PAUSE_MENU_EXIT    2
#define PAUSE_MENU_SIZE    3

#include "engine/input/joystick.h"

// uc_orig: PAUSE_handler (fallen/Source/pause.cpp)
SLONG PAUSE_handler()
{
    SLONG i, text_colour, input, temp;
    static SLONG lastinput = 0;
    static DWORD dir_next_fire = 0;
    SLONG ans = UC_FALSE;

    input = 0;

    if (ReadInputDevice()) {
        if (the_state.lY > AXIS_MAX)
            input |= PAUSED_KEY_DOWN;

        if (the_state.lY < AXIS_MIN)
            input |= PAUSED_KEY_UP;

        // Start button (index 6) → toggle pause.
        if (the_state.rgbButtons[6])
            input |= PAUSED_KEY_START;

        // Cross/A (index 0) = confirm selection.
        if (the_state.rgbButtons[0])
            input |= PAUSED_KEY_OKAY;

        // Triangle/Y (index 3) = unpause (back/cancel, PS1 behavior).
        if (the_state.rgbButtons[3])
            input |= PAUSED_KEY_START;
    }

    // Edge-detect for buttons, ticker-based repeat for directions.
    temp = input;
    {
        SLONG dir_mask = PAUSED_KEY_UP | PAUSED_KEY_DOWN;
        SLONG dir_input = input & dir_mask;
        SLONG btn_input = input & ~dir_mask;
        SLONG last_btn = lastinput & ~dir_mask;
        SLONG last_dir = lastinput & dir_mask;

        // Buttons: edge-detect only.
        btn_input = btn_input & (~last_btn);

        // Directions: time-based auto-repeat (same values as gamemenu).
        {
            DWORD now = GetTickCount();
            if (dir_input) {
                if (dir_input != last_dir) {
                    dir_next_fire = now + 400;
                } else if (now < dir_next_fire) {
                    dir_input = 0;
                } else {
                    dir_next_fire = now + 150;
                }
            }
        }

        input = dir_input | btn_input;
    }
    lastinput = temp;

    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        input |= PAUSED_KEY_START;
    }

    if (input & PAUSED_KEY_START) {
        GAME_FLAGS ^= GF_PAUSED;
        selected = 0;
    }

    if (!(GAME_FLAGS & GF_PAUSED))
        return UC_FALSE;

    // Keyboard repeat delay (time-based, same values as controller).
    {
        static DWORD kb_next_fire = 0;
        static UBYTE kb_last_dir = 0;
        UBYTE kb_dir = (Keys[KB_UP] ? 1 : 0) | (Keys[KB_DOWN] ? 2 : 0);
        DWORD now = GetTickCount();

        if (kb_dir) {
            if (kb_dir != kb_last_dir) {
                kb_next_fire = now + 400;
            } else if (now < kb_next_fire) {
                Keys[KB_UP] = Keys[KB_DOWN] = 0;
            } else {
                kb_next_fire = now + 150;
            }
        }
        kb_last_dir = kb_dir;
    }
    if (Keys[KB_UP]) {
        Keys[KB_UP] = 0;
        input |= PAUSED_KEY_UP;
    }
    if (Keys[KB_DOWN]) {
        Keys[KB_DOWN] = 0;
        input |= PAUSED_KEY_DOWN;
    }
    if (Keys[KB_SPACE] | Keys[KB_ENTER]) {
        Keys[KB_SPACE] = Keys[KB_ENTER] = 0;
        input |= PAUSED_KEY_OKAY;
    }

    if (input & PAUSED_KEY_UP) {
        selected--;
        if (selected < 0)
            selected = PAUSE_MENU_SIZE - 1;
    }
    if (input & PAUSED_KEY_DOWN) {
        selected++;
        if (selected >= PAUSE_MENU_SIZE)
            selected = 0;
    }

    if (input & PAUSED_KEY_OKAY) {
        switch (selected) {
        case PAUSE_MENU_RESUME:
            GAME_FLAGS ^= GF_PAUSED;
            break;

        case PAUSE_MENU_RESTART:
            GAME_FLAGS &= ~GF_PAUSED;
            GAME_STATE = GS_REPLAY;
            ans = UC_TRUE;
            break;

        case PAUSE_MENU_EXIT:
            GAME_FLAGS &= ~GF_PAUSED;
            GAME_STATE = 0;
            ans = UC_TRUE;
            break;
        }
    }

    // Draw semi-transparent background panel behind the menu items.
    POLY_frame_init(UC_FALSE, UC_FALSE);

    PANEL_draw_quad(
        320 - 170,
        150 - 20,
        320 + 170,
        150 + 40 * PAUSE_MENU_SIZE + 20,
        POLY_PAGE_ALPHA_OVERLAY,
        0x88000000);

    POLY_frame_draw(UC_FALSE, UC_TRUE);
    POLY_frame_init(UC_FALSE, UC_FALSE);

    SLONG offset;
    SLONG text_size;

    for (i = 0; i < PAUSE_MENU_SIZE; i++) {
        if (selected == i) {
            text_colour = 0xffffff;
            text_size = 768;
            offset = -8;
        } else {
            text_colour = 0x00ff00;
            text_size = 512;
            offset = 0;
        }

        // text_size and offset overrides below match original (both branches collapsed to same values).
        text_size = 512;
        offset = 0;

        FONT2D_DrawStringCentred(
            XLAT_str(X_RESUME_LEVEL + i),
            320,
            150 + 40 * i + offset,
            text_colour,
            text_size);
    }

    POLY_frame_draw(UC_FALSE, UC_TRUE);

    return ans;
}
