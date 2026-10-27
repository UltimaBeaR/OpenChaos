# 69 — Resolution / fullscreen toggle in the options menu

- Status: 🔵 open
- Area: ui
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In 1.0 the resolution and window mode are changed **only via the config** (`config.json`), no UI is added to the menu. Post-1.0: add these settings to the options menu. **⚠️ Implementation warnings (important during implementation):** **(1) mipmap LOD bias:** when adding resolution selection to the UI, be sure to check that the mipmap LOD bias (-1.7) doesn't cause blur/shimmer at all resolutions (640×480, 1080p, 1440p, 4K). If it drifts — compute the bias from the resolution. **(2) VSync when toggling fullscreen/windowed on Windows:** the VSync strategy depends on the window mode due to a WDDM throttle workaround (see [`archive/new_game_devlog/startup_hang_investigation/fix_progress.md`](../../../archive/new_game_devlog/startup_hang_investigation/fix_progress.md)). In `sdl3_gl_create_context` and when toggling the window mode you need to choose: ▸ **Windowed:** `SDL_GL_SetSwapInterval(0)` + `DwmFlush()` before swap. ▸ **Fullscreen:** `SDL_GL_SetSwapInterval(1)` + no `DwmFlush` (it's a no-op in fullscreen). ▸ On macOS/Linux **always** `SDL_GL_SetSwapInterval(1)` — there's no bug there. ▸ Do fullscreen via `SDL_WINDOW_FULLSCREEN` (SDL3 default = borderless-desktop — DWM works for alt-tab and other windows, but for our window NVIDIA goes directly to scan-out bypassing the WDDM throttle path). Exclusive fullscreen via `SDL_SetWindowFullscreenMode` with a specific mode — not needed, borderless covers all use cases.

Old-list status: *Implement*.
