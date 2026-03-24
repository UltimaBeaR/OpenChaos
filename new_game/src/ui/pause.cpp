#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "assets/xlat_str.h"
#include "engine/input/keyboard.h"
#include "ui/pause.h"

#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/graphics_api/d3d_texture.h"

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

// Forward declaration: reads the DirectInput joystick state into the_state.
extern BOOL ReadInputDevice(void);
// Joystick state filled by ReadInputDevice().
extern DIJOYSTATE the_state;

// uc_orig: PAUSE_handler (fallen/Source/pause.cpp)
SLONG PAUSE_handler()
{
    SLONG i, text_colour, input, temp;
    static SLONG lastinput = 0;
    SLONG ans = UC_FALSE;

    input = 0;

    if (ReadInputDevice()) {
        if (the_state.lY > AXIS_MAX)
            input |= PAUSED_KEY_DOWN;

        if (the_state.lY < AXIS_MIN)
            input |= PAUSED_KEY_UP;

        for (i = 0; i < 8; i++) {
            if (the_state.rgbButtons[i])
                input |= PAUSED_KEY_OKAY;
        }
    }

    // Edge-detect: only trigger on new presses.
    temp = input;
    input = input & (~lastinput);
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
