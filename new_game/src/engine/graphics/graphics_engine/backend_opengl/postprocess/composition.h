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

// Map a real-backbuffer pixel coordinate (e.g. a mouse event position)
// into scene-FBO pixel coordinates using the most recent composition
// target rectangle. Returns true when (win_x, win_y) falls inside the
// aspect-fit blit rect, with (*fbo_x, *fbo_y) set. Returns false when
// the point sits in an outer bar — caller should drop the event.
// Uses the (dst_x, dst_y, dst_w, dst_h, real_w, real_h) tuple computed
// in the last composition_blit. Call on whatever event-pump thread the
// caller uses; it's a pure pixel math function, no GL state touched.
bool composition_window_to_fbo(int win_x, int win_y, int* fbo_x, int* fbo_y);

// Return the most recent composition target rectangle on the default
// framebuffer (where composition_blit aspect-fitted the scene FBO).
// Used by the post-composition UI pass to set its viewport so UI lands
// exactly on top of the rendered scene (and not in the outer bars).
// Origin is bottom-left (matches glViewport convention). All four
// outputs are zero before the first composition_blit.
void composition_get_dst_rect(int32_t* x, int32_t* y, int32_t* w, int32_t* h);

// ─── Last-frame snapshot (used by interactive drag-resize) ─────────────
//
// During an OS-level window drag the game's main loop is paused (Windows
// owns the message pump), but SDL keeps firing window-resize events that
// need a present each tick — otherwise the OS shows undefined / black
// content while the window border is being dragged. We can't safely
// re-run the rendering pipeline without a fresh game tick (per-frame UI
// state like timer queues, dirty bits, etc. would be empty / stale and
// individual UI elements would flicker), so we instead snapshot the
// post-flip default framebuffer once per real frame and just rescale
// that snapshot into the new window geometry during drag. End result:
// the window contents simply "stretch" smoothly while being resized,
// then the next real frame renders normally once the drag ends.

// Capture the current default-framebuffer contents into the snapshot
// buffer. Call after composition + UI pass + any other UI work, just
// before swap, so the snapshot matches what the user sees this frame.
// The snapshot resizes itself as the window changes; the first call
// allocates GL resources lazily.
void composition_capture_last_frame(int32_t window_w, int32_t window_h);

// Present the last captured snapshot stretched to fit the current
// window. Caller binds the default framebuffer beforehand and swaps
// after. Aspect-fits the snapshot into the window (pillarbox /
// letterbox bars when the window aspect drifted from the snapshot
// aspect). No-op if no snapshot has been captured yet.
void composition_present_last_frame(int32_t window_w, int32_t window_h);
