#ifndef GAME_UI_RENDER_H
#define GAME_UI_RENDER_H

// Post-composition UI render pass.
//
// All non-3D-dependent UI (HUD, menus, console, debug overlays, fade-out)
// is drawn here, AFTER the composition pass has blitted the scene FBO into
// the default framebuffer. Drawing into the default FB at native resolution
// keeps UI sharp — it never goes through the FXAA / bilinear-upscale path
// that the scene takes through the composition shader.
//
// Registered as ge_set_post_composition_callback in game_startup.

// Called after composition_blit. Default FB is bound, viewport set to the
// full window. Must save / restore any GL state it touches.
void ui_render_post_composition(void);

#endif // GAME_UI_RENDER_H
