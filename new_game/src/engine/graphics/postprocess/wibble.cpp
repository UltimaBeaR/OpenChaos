#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "engine/graphics/postprocess/wibble.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"

// Display globals (defined in d3d/display_globals.cpp).
extern SLONG ScreenWidth;
extern SLONG ScreenHeight;

// uc_orig: WIBBLE_simple (fallen/DDEngine/Source/wibble.cpp)
// The original implementation per-row shifted backbuffer pixels on the CPU via
// the locked screen surface. That trigger — per-frame glReadPixels + writeback
// — is the hang pattern we're eliminating (see
// new_game_devlog/startup_hang_investigation/fix_progress.md, Stage 3).
// Now a thin wrapper: scale from game-virtual (DisplayWidth × DisplayHeight)
// to real framebuffer pixels, then delegate to the GPU pass in the backend.
void WIBBLE_simple(
    SLONG x1, SLONG y1,
    SLONG x2, SLONG y2,
    UBYTE wibble_y1,
    UBYTE wibble_y2,
    UBYTE wibble_g1,
    UBYTE wibble_g2,
    UBYTE wibble_s1,
    UBYTE wibble_s2)
{
    // Uniform scale by vertical on both axes — matches the s_YScale used to
    // render the world (see PolyPage::SetScaling in game_mode_changed).
    // Scaling X by ScreenWidth/DisplayWidth works only when the monitor
    // aspect is 4:3; on widescreen the horizontal ratio diverges from the
    // vertical one and the wibble bbox drifts off the rendered puddle.
    x1 = x1 * ScreenHeight / DisplayHeight;
    x2 = x2 * ScreenHeight / DisplayHeight;
    y1 = y1 * ScreenHeight / DisplayHeight;
    y2 = y2 * ScreenHeight / DisplayHeight;

    GEWibbleParams p {};
    p.wibble_y1 = wibble_y1;
    p.wibble_y2 = wibble_y2;
    p.wibble_g1 = wibble_g1;
    p.wibble_g2 = wibble_g2;
    p.wibble_s1 = wibble_s1;
    p.wibble_s2 = wibble_s2;
    p.game_turn = GAME_TURN;

    ge_apply_wibble(x1, y1, x2, y2, p);
}
