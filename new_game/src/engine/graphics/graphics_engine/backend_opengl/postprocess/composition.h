#pragma once

// Composition module — owns the scene FBO and the composition pass.
//
// The scene FBO is the render target for everything the game draws each
// frame (3D, HUD, overlays — all go through it for now). Its size is
// physical_window_size × OC_RENDER_SCALE. After the game is done rendering,
// composition_blit() runs a fullscreen quad that reads the scene texture
// and writes to the default framebuffer at full window size, with bilinear
// upscale and optional FXAA (OC_AA_ENABLE).
//
// Typical per-frame use:
//   composition_bind_scene();     // redirects rendering into scene FBO
//   ... game draws everything ...
//   composition_bind_default();   // back to window framebuffer
//   composition_blit();           // scene FBO -> window with FXAA + upscale
//   gl_context_swap();
//
// Resize/recreate is driven by the graphics engine (OpenDisplay /
// mode_change path) via composition_resize().

#include <cstdint>

// Create the scene FBO at the given pixel dimensions and initialize the
// composite shader program. Safe to call multiple times — rebuilds when
// the requested size differs from the current one. Returns false on GL
// error. The dimensions here are the post-scale render size (not the
// physical window size); callers do the (physical × scale) math.
bool composition_init(int32_t scene_w, int32_t scene_h);

// Destroy all GL resources (FBO, textures, shader, VAO/VBO). Idempotent.
void composition_shutdown();

// Reallocate the scene FBO attachments at new size. No-op if the current
// size already matches. Keeps the shader / VAO.
bool composition_resize(int32_t scene_w, int32_t scene_h);

// Bind the scene FBO as the current draw framebuffer. Viewport/scissor
// are caller's responsibility (ge_set_viewport will already size them
// against the scene FBO dimensions via gl_context_get_width/height).
void composition_bind_scene();

// Bind the default framebuffer (the window's backbuffer).
void composition_bind_default();

// Run the fullscreen composition pass: sample the scene color texture,
// apply FXAA if OC_AA_ENABLE, write to whichever framebuffer is currently
// bound (typically the default FB) at the given window size. Caller must
// have bound the target framebuffer before calling this.
void composition_blit(int32_t window_w, int32_t window_h);

// Query current scene FBO size. 0 if not initialized.
int32_t composition_scene_width();
int32_t composition_scene_height();
