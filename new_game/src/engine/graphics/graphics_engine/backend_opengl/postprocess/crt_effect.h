#pragma once

// CRT scanline post-process effect (crt-lottes, public domain).
//
// When enabled, overwrites the game area on the default framebuffer with a
// CRT-filtered version sampled directly from the scene FBO color texture.
// The effect runs after composition_blit() and before the UI pass, so the
// HUD and menus render cleanly on top.
//
// Toggle: F2 in bangunsnotgames debug mode.

// Whether the CRT effect is currently active. Settable from anywhere.
extern bool g_crt_enabled;

// Apply the CRT pass to the current default framebuffer.
// Must be called while the default framebuffer is bound (after composition_blit).
// No-op when g_crt_enabled == false.
void crt_effect_apply();

// Release all GPU resources owned by the CRT effect. Idempotent.
void crt_effect_shutdown();
