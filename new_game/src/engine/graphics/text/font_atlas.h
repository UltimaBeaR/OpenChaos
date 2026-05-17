#ifndef ENGINE_GRAPHICS_TEXT_FONT_ATLAS_H
#define ENGINE_GRAPHICS_TEXT_FONT_ATLAS_H

#include "engine/core/types.h"

// GPU-native replacement for the pre-GPU per-pixel plot path in FONT_draw.
//
// Original code rendered a 5×9 glyph by reading/modifying the backbuffer pixel
// by pixel through ge_plot_pixel. On modern WDDM/DWM that per-frame readback
// pattern triggers a driver throttle (see new_game_devlog/startup_hang_investigation).
// This module keeps the same glyph bitmaps but uploads them once to a GPU
// texture atlas and draws each glyph as a small textured quad through the
// regular TL pipeline — no CPU readback of the framebuffer.

// One-time upload of FONT_upper/lower/number/punct into a GPU atlas. Lazy —
// safe to call from any frame, does nothing on subsequent calls. Needs a
// current GL context.
void FONT_atlas_ensure();

// Emits a single textured glyph quad. Matches FONT_draw_coloured_char semantics
// except it draws via GL instead of plotting into a locked screen buffer.
// Caller is responsible for feeding a printable ASCII char; space is a no-op.
// Off-screen clipping is the caller's responsibility (cheap to always emit —
// GPU will clip anyway, but a caller-side bbox reject is kept for parity).
//
// Must be called between FONT_atlas_begin_batch / FONT_atlas_end_batch —
// those manage GE render state save/restore for the whole group of draws.
// `scale` (≥1) blows the glyph quad up to scale× its 5×FONT_HEIGHT pixel
// footprint while sampling the same atlas texels — a chunky nearest-style
// upscale. Default 1 = original size (existing callers unaffected).
void FONT_atlas_draw_glyph(SLONG sx, SLONG sy, UBYTE r, UBYTE g, UBYTE b, CBYTE ch, SLONG scale = 1);

// Save/restore the GE render state that FONT_atlas_draw_glyph mutates.
// Wraps any run of glyph draws — one push/pop per batch keeps the GL
// state-query cost flat (not per-glyph).
void FONT_atlas_begin_batch();
void FONT_atlas_end_batch();

// Literal-framebuffer-pixel filled rectangle, drawn via the SAME
// pre-transformed TL path the glyphs use (untextured, alpha-blended,
// no depth). Coordinates are real framebuffer pixels — identical space
// to FONT_buffer_add literal text — so a rect drawn here lines up with
// that text in every render mode (gameplay / menu / cutscene), unlike
// AENG_draw_rect which goes through the game's logical/letterbox POLY
// pipeline. `argb` = 0xAARRGGBB (alpha in the high byte). Wrap a run of
// rects in FONT_atlas_fill_begin / FONT_atlas_fill_end (one state
// push/pop for the whole batch, like the glyph batch).
void FONT_atlas_fill_begin();
void FONT_atlas_fill_rect(SLONG x, SLONG y, SLONG w, SLONG h, ULONG argb);
void FONT_atlas_fill_end();

#endif // ENGINE_GRAPHICS_TEXT_FONT_ATLAS_H
