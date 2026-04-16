// Modal input debug panel — see input_debug.h.

#include "engine/debug/input_debug/input_debug.h"

#include "engine/graphics/pipeline/aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/text/font.h"
#include "engine/input/keyboard.h"
#include "engine/input/keyboard_globals.h"
#include "engine/platform/uc_common.h"
#include "ui/hud/panel.h"      // PANEL_start / PANEL_finish — frame the 2D batch

namespace {

bool s_active = false;

// Higher layer = closer to camera in AENG_draw_rect. Health HUD uses 2;
// pick something well above to cover HUD without fighting over z.
constexpr SLONG LAYER = 10;

} // namespace

void input_debug_open()
{
    s_active = true;
}

void input_debug_close()
{
    s_active = false;
}

void input_debug_toggle()
{
    s_active = !s_active;
}

bool input_debug_is_active()
{
    return s_active;
}

void input_debug_tick()
{
    if (!s_active) return;

    // ESC closes the panel. Consume the key so pause menu doesn't see it.
    if (Keys[KB_ESC]) {
        Keys[KB_ESC] = 0;
        s_active = false;
    }
}

void input_debug_render()
{
    if (!s_active) return;

    // 2D HUD primitives queued into POLY pages must be framed by
    // PANEL_start / PANEL_finish (which call POLY_frame_init and
    // POLY_frame_draw respectively). Without the finish flush the
    // geometry sits in the page buffer until next frame and ends up
    // in the shadow pass via VB slot reuse — see
    // new_game_devlog/shadow_corruption_investigation.md.
    PANEL_start();

    // Full-screen translucent black backdrop (80% opaque). Uses
    // POLY_PAGE_COLOUR_ALPHA which enables SrcAlpha/InvSrcAlpha blending
    // — the high byte of the colour is the alpha. 0xCC = 204 ≈ 80%.
    const SLONG w = (SLONG)POLY_screen_width;
    const SLONG h = (SLONG)POLY_screen_height;
    AENG_draw_rect(0, 0, w, h, 0xCC000000, LAYER, POLY_PAGE_COLOUR_ALPHA);

    PANEL_finish();

    // Text uses a separate buffer (FONT_buffer_draw in screen_flip),
    // independent of the POLY page batch above. Placeholder content.
    FONT_buffer_add(20, 20,  255, 255, 255, 1, (CBYTE*)"INPUT DEBUG PANEL  (iteration 1)");
    FONT_buffer_add(20, 40,  200, 200, 200, 1, (CBYTE*)"F11 or ESC — exit");
    FONT_buffer_add(20, 56,  200, 200, 200, 1, (CBYTE*)"Keyboard / gamepad / DualSense pages come in later iterations.");
    FONT_buffer_add(20, 80,  0, 255, 0, 1,     (CBYTE*)"Character input is blocked while this panel is open.");
}
