#include "game/ui_render.h"

#include "engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/graphics/graphics_engine/backend_opengl/postprocess/composition.h"
#include "engine/graphics/graphics_engine/backend_opengl/common/glad/include/glad/gl.h"
#include "engine/graphics/pipeline/polypage.h"             // PolyPage::s_XScale etc.
#include "engine/graphics/pipeline/polypage_globals.h"     // not_private_smiley_* mirrors
#include "engine/graphics/ui_coords.h"                     // ui_coords::recompute

// UI draws moved out of the scene FBO and into this post-composition pass.
#include "ui/hud/panel.h"
#include "ui/menus/gamemenu.h"
#include "engine/console/console.h"
#include "engine/debug/input_debug/input_debug.h"
#include "engine/debug/debug_help/debug_help.h"
#include "engine/graphics/text/font.h"                    // FONT_buffer_draw
#include "engine/graphics/text/font2d.h"                  // FONT2D_DrawString (cheat==2 FPS overlay)
#include "engine/graphics/pipeline/aeng.h"                // AENG_draw_messages
#include "engine/input/keyboard_globals.h"                // ControlFlag
#include "missions/eway.h"                                // EWAY_stop_player_moving
#include "ui/frontend/frontend.h"                         // FRONTEND_display_overlay

// GAME_FLAGS / GF_PAUSED + GAME_STATE / GS_* + NET_PERSON / NET_PLAYER.
#include "game/game_types.h"

// allow_debug_keys toggles Ctrl-held debug draws.
extern BOOL allow_debug_keys;
// cheat == 2 enables the FPS / position overlay (was inside OVERLAY_handle).
extern UBYTE cheat;
// Per-frame integer ms delta from sdl3_get_ticks (used by FPS overlay).
extern SLONG tick_tock_unclipped;

void ui_render_post_composition(void)
{
    // Snapshot GL state we touch so the next frame's scene rendering finds
    // the viewport / scissor / depth it expects. Composition has already
    // restored its own state; we layer on top.
    GLint     prev_vp[4]   = {};
    GLint     prev_sc[4]   = {};
    GLboolean prev_sc_on   = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean prev_depth_on = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prev_depth_mask = GL_TRUE;
    glGetIntegerv(GL_VIEWPORT,    prev_vp);
    glGetIntegerv(GL_SCISSOR_BOX, prev_sc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &prev_depth_mask);

    // Map our viewport to the rect where composition_blit landed the scene.
    // On windows wider/taller than the FBO aspect this rect is centred with
    // outer pillar/letterbox bars around it; the bars belong to composition,
    // UI must stay inside the scene rect.
    int32_t dst_x = 0, dst_y = 0, dst_w = 0, dst_h = 0;
    composition_get_dst_rect(&dst_x, &dst_y, &dst_w, &dst_h);
    if (dst_w <= 0 || dst_h <= 0) {
        // Fallback: composition hasn't published a rect yet — skip the
        // pass rather than draw at an uninitialised viewport.
        return;
    }

    glDisable(GL_SCISSOR_TEST);

    // Set viewport + TL shader's u_viewport together. The latter must match
    // the dst rect — otherwise glyph quads (which pass literal pixel coords
    // via GEVertexTL, bypassing PolyPage scaling) get normalised by a
    // scene-FBO-sized u_viewport and end up stretched onto the larger
    // default-FB viewport. Symptom: at OC_RENDER_SCALE=0.25 with a
    // 1024×768 window, F1 text came out 4× too big.
    ge_enter_ui_viewport(dst_x, dst_y, dst_w, dst_h);

    // Save + reconfigure PolyPage scaling and ui_coords for the UI pass.
    // Scene rendering leaves these tied to the scene FBO size; the UI
    // pass draws into the default FB dst rect which can be a different
    // size (render scale < 1.0, or when the composition aspect-fit
    // pillarboxes the FBO inside the window). Recompute to match dst_w /
    // dst_h so virtual 640×480 UI coords map to the correct dst pixels.
    const float prev_xscale  = PolyPage::s_XScale;
    const float prev_yscale  = PolyPage::s_YScale;
    const float prev_xoffset = PolyPage::s_XOffset;
    const float prev_yoffset = PolyPage::s_YOffset;
    const int32_t scene_w = composition_scene_width();
    const int32_t scene_h = composition_scene_height();
    {
        const float uniform_scale = float(dst_h) / 480.0f;
        PolyPage::SetScaling(uniform_scale, uniform_scale);
        ui_coords::recompute(dst_w, dst_h);
    }

    // UI draws on a freshly-cleared default FB depth buffer (composition_blit
    // clears depth to 1.0). Our draws use depth bodges near the far plane
    // which would fail GL_LESS against 1.0 — disable depth for UI entirely.
    // Also disable depth writes so any future scene using default FB depth
    // doesn't see UI-poisoned values.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Flip ge_begin_scene() redirection on. UI code calls POLY_frame_init
    // transitively (e.g. PANEL_fadeout_draw does so internally), which
    // calls ge_begin_scene and would rebind the scene FBO — silently
    // sending our UI draws to the wrong target. With ui_mode on they
    // bind the default FB instead and our draws land on top of the
    // composed scene.
    ge_set_ui_mode(true);
    composition_bind_default();

    // ───── UI calls (will grow stage by stage) ─────
    // Order mirrors the pre-refactor game loop (game.cpp:1221-1243). The
    // comment around input_debug_render in the old location noted that
    // GAMEMENU_draw clears the POLY queue via POLY_frame_init, so anything
    // queued *before* GAMEMENU_draw must be flushed before it runs; those
    // flushes happen inside PANEL_finish / FONT_buffer_draw paths.

    // HUD block — was OVERLAY_handle's 2D portion. Gated on the actual
    // precondition the PANEL_* code requires: an alive player Thing.
    //
    // ui_render_post_composition runs every flip (main menu, attract,
    // video, **loadscreen during mission start-up**, etc). A GAME_STATE
    // bit check is *not* enough — during ATTRACT_loadscreen_init the
    // gameplay state bits are already set but NET_PERSON(0) is still
    // null (player hasn't been materialised yet), and PANEL_inventory
    // dereferences it unconditionally. Original OVERLAY_handle only
    // ever ran inside the game loop where darci was guaranteed alive,
    // so it skipped the null check; we have to add it.
    Thing* darci  = NET_PERSON(0);
    Thing* player = NET_PLAYER(0);
    if (darci && player) {

        PANEL_start();

        if (!EWAY_stop_player_moving()) {
            PANEL_draw_buffered();
        }

        if (!(GAME_STATE & GS_LEVEL_WON)) {
            PANEL_last();
        }

        PANEL_inventory(darci, player);

        if (cheat == 2) {
            CBYTE str[50];

            if (tick_tock_unclipped == 0)
                tick_tock_unclipped = 1;

            // tick_tock_unclipped is the raw per-frame delta in integer
            // milliseconds from sdl3_get_ticks. At 30 FPS it toggles between
            // 33 and 34 ms (true period 33.333), so 1000/33 = 30 and
            // 1000/34 = 29 alternate. Smooth with an EMA so the displayed
            // value doesn't flicker between 29 and 30 every frame.
            static float fps_ema = 0.0f;
            float fps_instant = 1000.0f / float(tick_tock_unclipped);
            if (fps_ema == 0.0f) {
                fps_ema = fps_instant;  // seed on first frame
            } else {
                // α = 0.05 → ~20-frame time constant (~0.67 s at 30 FPS).
                fps_ema = 0.95f * fps_ema + 0.05f * fps_instant;
            }

            sprintf(str, "(%d,%d,%d) fps %.2f",
                    darci->WorldPos.X >> 16,
                    darci->WorldPos.Y >> 16,
                    darci->WorldPos.Z >> 16,
                    fps_ema);

            FONT2D_DrawString(str, 2, 2, 0xffffff, 256);
        }

        PANEL_finish();
    }

    input_debug_render();
    debug_help_render();
    if (!(GAME_FLAGS & GF_PAUSED)) {
        CONSOLE_draw();
    }
    GAMEMENU_draw();
    PANEL_fadeout_draw();

    // Debug message overlay (Ctrl-held + debug mode).
    if (ControlFlag && allow_debug_keys) {
        AENG_draw_messages();
    }

    // Flush the pixel-path FONT buffer. Anything queued via FONT_buffer_add
    // by the debug text paths (debug_help, FPS overlay, gamepad debug,
    // AENG_draw_messages) renders here — into the default FB at native
    // resolution, untouched by the composition shader. Before the split
    // this flush ran inside screen_flip() before AENG_blit, so the text
    // landed in the scene FBO and got softened by FXAA / bilinear upscale.
    FONT_buffer_draw();

    // Frontend menu overlay (text + map markers + corner tab buttons + title
    // wibble + mission select + moving-panel preview). Drawn LAST so it sits
    // on top of any other UI in attract mode. The frontend's background
    // (back image, transitions, kibble particles) stays in the scene FBO
    // half — `FRONTEND_display()` is still called from `FRONTEND_loop()`.
    //
    // Gate via a sticky flag rather than a per-frame dirty bit:
    //   - Set true by `FRONTEND_display()` (the matching scene-FBO half).
    //   - Cleared explicitly when leaving the frontend (FRONTEND_loop
    //     on a transition action, ATTRACT_loadscreen_init,
    //     MAIN_main / outro entry).
    //
    // The "consume after one render" version misbehaved during
    // interactive window drag-resize: SDL fires the post-composition
    // callback from `ge_present_for_drag` while the game's main loop
    // is paused (OS owns the message pump), so `FRONTEND_display()`
    // doesn't run that "frame" and the dirty bit had been cleared by
    // the previous render — leaving the user dragging a frontend
    // window with the scene visible but no menu text.
    extern bool g_frontend_overlay_pending;
    if (g_frontend_overlay_pending) {
        FRONTEND_display_overlay();
    }
    // ──────────────────────────────────────────────

    ge_set_ui_mode(false);

    // Restore PolyPage scaling + ui_coords so the next frame's scene
    // rendering sees the scene-FBO-based values it was configured with.
    // Keep the not_private_smiley_* mirrors in sync — SetScaling /
    // push_ui_mode write both, so skipping the mirror would leave it
    // stale for any legacy path that still reads from it.
    PolyPage::s_XScale  = prev_xscale;
    PolyPage::s_YScale  = prev_yscale;
    PolyPage::s_XOffset = prev_xoffset;
    PolyPage::s_YOffset = prev_yoffset;
    not_private_smiley_xscale = prev_xscale;
    not_private_smiley_yscale = prev_yscale;
    if (scene_w > 0 && scene_h > 0) {
        ui_coords::recompute(scene_w, scene_h);
    }

    // Restore GL state.
    glDepthMask(prev_depth_mask);
    if (prev_depth_on) glEnable(GL_DEPTH_TEST);
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glScissor (prev_sc[0], prev_sc[1], prev_sc[2], prev_sc[3]);
    if (prev_sc_on) glEnable(GL_SCISSOR_TEST);

    // Viewport restored via glViewport, but s_vp_* inside the backend was
    // reconfigured by ge_enter_ui_viewport and must be resynced so the
    // next scene draw sees the scene-FBO viewport in u_viewport. The
    // simplest path is to just force a uniform re-upload on the next
    // draw — scene's first ge_set_viewport call will then populate
    // s_vp_* and the uniform afresh.
    ge_invalidate_uniform_cache();

    // CRT scanline effect runs last — after all UI/HUD — so the filter
    // covers the full frame just like a real CRT monitor would.
    crt_effect_apply();
}
