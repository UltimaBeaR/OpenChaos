// SDL3 bridge implementation — see sdl3_bridge.h for API.

#include "engine/platform/sdl3_bridge.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <dwmapi.h>
#endif

// ===========================================================================
// Window
// ===========================================================================

static SDL_Window*   s_window   = nullptr;
static SDL_GLContext  s_gl_ctx   = nullptr;
static SDL3_Callbacks s_callbacks = {};

// Whether sdl3_gl_swap() should call DwmFlush() before SDL_GL_SwapWindow.
// Set by sdl3_gl_configure_vsync(); see that function + sdl3_bridge.h for
// the strategy matrix.
static bool s_use_dwm_flush = false;

// SDL event watcher — fires synchronously when SDL_PushEvent is called,
// including from inside the OS modal resize loop (Windows edge-drag),
// where our main loop is blocked. We use it to repaint the window with
// the last rendered scene composed against the new client area.
//
// We deliberately do NOT try to filter out events that arrive through
// our own sdl3_poll_events drain. On Windows the OS modal loop runs
// INSIDE our SDL_PollEvent call (Windows enters its sizing loop from
// DefWindowProc, which SDL invokes via DispatchMessage), so any "we're
// in the pump" flag would always be true during the very modal loop we
// want to paint over. The cost on platforms without a modal loop
// (Mac/Linux, Windows borderless) is one extra composition swap per
// resize event during a drag — visually harmless under double buffering
// since the next regular frame's swap follows within the same vsync
// window and overwrites it.
//
// Type filter is essential: the watcher may be invoked from non-main
// threads for event types like joystick hotplug. Window pixel-size
// events come from the OS message pump on the main thread, so it's
// safe to call back into GL from here on Windows. SDL3 ignores the
// return value of event watchers (it's only meaningful for
// SDL_SetEventFilter); we always return true.
static bool SDLCALL window_event_watch(void* userdata, SDL_Event* event)
{
    (void)userdata;
    if (!event) return true;
    if (event->type != SDL_EVENT_WINDOW_RESIZED &&
        event->type != SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) return true;
    if (s_callbacks.on_window_resize_live) {
        s_callbacks.on_window_resize_live();
    }
    return true;
}

bool sdl3_window_create(const char* title, int width, int height, bool fullscreen)
{
    if (s_window) return false;

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL3: SDL_INIT_VIDEO failed: %s\n", SDL_GetError());
        return false;
    }

    // No MSAA on the default framebuffer — antialiasing is done in the
    // composition pass via FXAA (see postprocess/composition.cpp). Scene
    // rendering targets a single-sample scene FBO; the default framebuffer
    // is only touched by the fullscreen composition quad that upscales it.
    //
    // SDL_WINDOW_HIGH_PIXEL_DENSITY: on HiDPI displays (macOS Retina,
    // Windows per-monitor scaling) make the GL backing store / drawable size
    // match physical pixels instead of logical points. The config width/
    // height below are treated as physical pixels too — we convert them to
    // logical points before SDL_CreateWindow.
    //
    // SDL_WINDOW_RESIZABLE: allows the user to drag the window edges to
    // resize at runtime. SDL ignores this flag in fullscreen, so it's safe
    // to always pass. The renderer reconfigures on SDL_EVENT_WINDOW_PIXEL_
    // SIZE_CHANGED via ge_resize_display() (see host.cpp on_window_resized).
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
                 | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;

    // Remember the caller's physical-pixel request before we mutate width/
    // height for fullscreen. Used after window creation to resize if the
    // HiDPI ratio turned out to be > 1 (see below).
    const int requested_phys_w = width;
    const int requested_phys_h = height;

    if (fullscreen) {
        // Explicitly use the primary display's desktop resolution as the
        // window size — otherwise SDL3 takes the passed (width, height) as
        // the requested exclusive fullscreen mode and the drawable stays
        // stuck at that size, leaving a rendered region in the bottom-left
        // of the physical panel on monitors with a different native mode.
        // SDL_GetDesktopDisplayMode returns logical-point dimensions, which
        // is what SDL_CreateWindow expects; HIGH_PIXEL_DENSITY then makes
        // the actual drawable match the physical panel.
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(display);
        if (mode) {
            width  = mode->w;
            height = mode->h;
        }
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    s_window = SDL_CreateWindow(title, width, height, flags);
    if (!s_window) {
        fprintf(stderr, "SDL3: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // Defensive floor so a tiny window can't drive fbo_w/h → 1 and trip
    // division-by-zero further down the pipeline. The aspect clamp in
    // ge_resize_display still handles the extreme-aspect edges above this.
    SDL_SetWindowMinimumSize(s_window, 320, 240);

    // Install the live-drag event watcher. SDL3 returns false on failure;
    // a missing watcher only loses the live-drag stretch (the regular
    // post-release resize still works), so the failure isn't fatal.
    if (!SDL_AddEventWatch(window_event_watch, nullptr)) {
        fprintf(stderr, "SDL3: SDL_AddEventWatch failed: %s\n", SDL_GetError());
    }

    if (!fullscreen) {
        // Config supplies window size in physical pixels (consistent across
        // platforms). SDL_CreateWindow takes logical points — on a Retina
        // Mac that means a 640-point window becomes 1280 physical pixels,
        // twice what we asked for. Fix it with a two-pass resize: query
        // the actual pixel density of the window we just created, and if
        // it's > 1 (HiDPI), shrink the logical size so the physical size
        // matches the config.
        // SDL_GetDisplayContentScale does *not* help here: that function
        // returns the user's accessibility text-scale preference (usually
        // 1.0 even on Retina), not the Retina pixel ratio.
        const float density = SDL_GetWindowPixelDensity(s_window);
        if (density > 1.0f) {
            const int new_logical_w = (int)((float)requested_phys_w / density + 0.5f);
            const int new_logical_h = (int)((float)requested_phys_h / density + 0.5f);
            SDL_SetWindowSize(s_window, new_logical_w, new_logical_h);
            // Re-center after resize so the window isn't off-screen.
            SDL_SetWindowPosition(s_window,
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED);
        }
    } else {
        // Belt-and-suspenders: force borderless-desktop fullscreen (passing
        // NULL as the mode). Prevents SDL from staying in an exclusive mode
        // if it auto-picked one that matched the desktop size by coincidence.
        SDL_SetWindowFullscreenMode(s_window, nullptr);

        // Hide the OS cursor — in fullscreen there's no reason for it to be
        // visible over the game. Menus that need a pointer draw their own.
        SDL_HideCursor();
    }

    return true;
}

void sdl3_window_destroy()
{
    if (s_window) {
        SDL_DestroyWindow(s_window);
        s_window = nullptr;
    }
}

void sdl3_window_show()
{
    if (s_window) SDL_ShowWindow(s_window);
}

void sdl3_window_get_size(int* w, int* h)
{
    if (s_window) {
        SDL_GetWindowSize(s_window, w, h);
    } else {
        if (w) *w = 0;
        if (h) *h = 0;
    }
}

void sdl3_window_get_position(int* x, int* y)
{
    if (s_window) {
        SDL_GetWindowPosition(s_window, x, y);
    } else {
        if (x) *x = 0;
        if (y) *y = 0;
    }
}

void sdl3_window_get_center(int* x, int* y)
{
    int wx, wy, ww, wh;
    sdl3_window_get_position(&wx, &wy);
    sdl3_window_get_size(&ww, &wh);
    if (x) *x = wx + ww / 2;
    if (y) *y = wy + wh / 2;
}

void sdl3_window_get_drawable_size(int* w, int* h)
{
    if (s_window) {
        SDL_GetWindowSizeInPixels(s_window, w, h);
    } else {
        if (w) *w = 0;
        if (h) *h = 0;
    }
}

void* sdl3_window_get_native_handle()
{
    if (!s_window) return nullptr;
    SDL_PropertiesID props = SDL_GetWindowProperties(s_window);
#ifdef _WIN32
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
    return nullptr;
#endif
}

bool sdl3_window_is_maximized()
{
    if (!s_window) return false;
    return (SDL_GetWindowFlags(s_window) & SDL_WINDOW_MAXIMIZED) != 0;
}

void sdl3_window_maximize()
{
    if (s_window) SDL_MaximizeWindow(s_window);
}

void sdl3_warp_mouse_global(int x, int y)
{
    SDL_WarpMouseGlobal((float)x, (float)y);
}

void sdl3_get_global_mouse_pos(int* x, int* y)
{
    float fx, fy;
    SDL_GetGlobalMouseState(&fx, &fy);
    if (x) *x = (int)fx;
    if (y) *y = (int)fy;
}

void sdl3_show_cursor()
{
    SDL_ShowCursor();
}

void sdl3_hide_cursor()
{
    SDL_HideCursor();
}

void sdl3_set_mouse_grab(bool grab)
{
    if (s_window)
        SDL_SetWindowMouseGrab(s_window, grab);
}

// ===========================================================================
// GL context
// ===========================================================================

bool sdl3_gl_create_context(int major, int minor)
{
    if (!s_window) return false;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    s_gl_ctx = SDL_GL_CreateContext(s_window);
    if (!s_gl_ctx) {
        fprintf(stderr, "SDL3: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    // VSync / pacing strategy is set by the caller via sdl3_gl_configure_vsync()
    // — we don't touch SetSwapInterval here. SDL leaves swap interval at its
    // default until configure is called; the caller must invoke it before the
    // first swap so driver VSync never runs in an unintended mode.
    return true;
}

void sdl3_gl_destroy_context()
{
    if (s_gl_ctx) {
        SDL_GL_DestroyContext(s_gl_ctx);
        s_gl_ctx = nullptr;
    }
}

void sdl3_gl_swap()
{
#ifdef _WIN32
    // Only when the active strategy needs it — see sdl3_gl_configure_vsync().
    // In that mode driver VSync is off and DwmFlush is our pacing mechanism;
    // in all other modes (VSync off, fullscreen VSync, non-Windows) the
    // driver-side path handles pacing and we skip this.
    if (s_use_dwm_flush) DwmFlush();
#endif
    if (s_window) SDL_GL_SwapWindow(s_window);
}

void sdl3_gl_configure_vsync(bool vsync_enabled)
{
    const bool fullscreen =
        s_window && (SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN) != 0;

    // Three paths:
    //   1. vsync_enabled=false → driver VSync off, no DwmFlush. Max FPS, tearing.
    //   2. vsync_enabled=true, windowed on Windows → driver VSync off +
    //      DwmFlush on swap. Avoids the WDDM-throttle hang that hard driver
    //      VSync triggers on affected NVIDIA hardware when DWM is composing
    //      our window.
    //   3. vsync_enabled=true, fullscreen OR non-Windows → regular driver
    //      VSync (SetSwapInterval(1)). In fullscreen NVIDIA goes straight
    //      to scan-out and skips the WDDM path that throttles. On macOS
    //      and Linux the WDDM bug doesn't exist, and DwmFlush is Windows-
    //      only anyway.
    if (!vsync_enabled) {
        SDL_GL_SetSwapInterval(0);
        s_use_dwm_flush = false;
        return;
    }

#ifdef _WIN32
    if (fullscreen) {
        SDL_GL_SetSwapInterval(1);
        s_use_dwm_flush = false;
    } else {
        SDL_GL_SetSwapInterval(0);
        s_use_dwm_flush = true;
    }
#else
    (void)fullscreen; // not consulted on this platform
    SDL_GL_SetSwapInterval(1);
    s_use_dwm_flush = false;
#endif
}

void* sdl3_gl_get_proc_address(const char* name)
{
    return (void*)SDL_GL_GetProcAddress(name);
}

bool sdl3_gl_get_swap_interval(int* interval)
{
    return SDL_GL_GetSwapInterval(interval);
}

// ===========================================================================
// Timer
// ===========================================================================

uint64_t sdl3_get_ticks()
{
    return SDL_GetTicks();
}

uint64_t sdl3_get_performance_counter()
{
    return SDL_GetPerformanceCounter();
}

uint64_t sdl3_get_performance_frequency()
{
    return SDL_GetPerformanceFrequency();
}

void sdl3_delay_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

// ===========================================================================
// Event loop — scancode mapping
// ===========================================================================

// Map SDL_Scancode (USB HID) → game KB_* code (Windows Set 1 scancodes).
// Extended keys get +0x80 in the game's scheme.
static uint8_t sdl_to_game_scancode(SDL_Scancode sc)
{
    switch (sc) {
    // Row 0: Escape, F-keys
    case SDL_SCANCODE_ESCAPE:    return 0x01;
    case SDL_SCANCODE_F1:        return 0x3b;
    case SDL_SCANCODE_F2:        return 0x3c;
    case SDL_SCANCODE_F3:        return 0x3d;
    case SDL_SCANCODE_F4:        return 0x3e;
    case SDL_SCANCODE_F5:        return 0x3f;
    case SDL_SCANCODE_F6:        return 0x40;
    case SDL_SCANCODE_F7:        return 0x41;
    case SDL_SCANCODE_F8:        return 0x42;
    case SDL_SCANCODE_F9:        return 0x43;
    case SDL_SCANCODE_F10:       return 0x44;
    case SDL_SCANCODE_F11:       return 0x57;
    case SDL_SCANCODE_F12:       return 0x58;

    // Row 1: Number row
    case SDL_SCANCODE_GRAVE:     return 0x29;
    case SDL_SCANCODE_1:         return 0x02;
    case SDL_SCANCODE_2:         return 0x03;
    case SDL_SCANCODE_3:         return 0x04;
    case SDL_SCANCODE_4:         return 0x05;
    case SDL_SCANCODE_5:         return 0x06;
    case SDL_SCANCODE_6:         return 0x07;
    case SDL_SCANCODE_7:         return 0x08;
    case SDL_SCANCODE_8:         return 0x09;
    case SDL_SCANCODE_9:         return 0x0a;
    case SDL_SCANCODE_0:         return 0x0b;
    case SDL_SCANCODE_MINUS:     return 0x0c;
    case SDL_SCANCODE_EQUALS:    return 0x0d;
    case SDL_SCANCODE_BACKSPACE: return 0x0e;

    // Row 2: Tab, QWERTY...
    case SDL_SCANCODE_TAB:       return 0x0f;
    case SDL_SCANCODE_Q:         return 0x10;
    case SDL_SCANCODE_W:         return 0x11;
    case SDL_SCANCODE_E:         return 0x12;
    case SDL_SCANCODE_R:         return 0x13;
    case SDL_SCANCODE_T:         return 0x14;
    case SDL_SCANCODE_Y:         return 0x15;
    case SDL_SCANCODE_U:         return 0x16;
    case SDL_SCANCODE_I:         return 0x17;
    case SDL_SCANCODE_O:         return 0x18;
    case SDL_SCANCODE_P:         return 0x19;
    case SDL_SCANCODE_LEFTBRACKET:  return 0x1a;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
    case SDL_SCANCODE_RETURN:    return 0x1c;

    // Row 3: Caps, ASDF...
    case SDL_SCANCODE_CAPSLOCK:  return 0x3a;
    case SDL_SCANCODE_A:         return 0x1e;
    case SDL_SCANCODE_S:         return 0x1f;
    case SDL_SCANCODE_D:         return 0x20;
    case SDL_SCANCODE_F:         return 0x21;
    case SDL_SCANCODE_G:         return 0x22;
    case SDL_SCANCODE_H:         return 0x23;
    case SDL_SCANCODE_J:         return 0x24;
    case SDL_SCANCODE_K:         return 0x25;
    case SDL_SCANCODE_L:         return 0x26;
    case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_BACKSLASH: return 0x2b;

    // Row 4: Shift, ZXCV...
    case SDL_SCANCODE_LSHIFT:    return 0x2a;
    case SDL_SCANCODE_Z:         return 0x2c;
    case SDL_SCANCODE_X:         return 0x2d;
    case SDL_SCANCODE_C:         return 0x2e;
    case SDL_SCANCODE_V:         return 0x2f;
    case SDL_SCANCODE_B:         return 0x30;
    case SDL_SCANCODE_N:         return 0x31;
    case SDL_SCANCODE_M:         return 0x32;
    case SDL_SCANCODE_COMMA:     return 0x33;
    case SDL_SCANCODE_PERIOD:    return 0x34;
    case SDL_SCANCODE_SLASH:     return 0x35;
    case SDL_SCANCODE_RSHIFT:    return 0x36;

    // Row 5: Ctrl, Alt, Space
    case SDL_SCANCODE_LCTRL:     return 0x1d;
    case SDL_SCANCODE_LALT:      return 0x38;
    case SDL_SCANCODE_SPACE:     return 0x39;
    case SDL_SCANCODE_RALT:      return 0x38 + 0x80; // extended
    case SDL_SCANCODE_RCTRL:     return 0x1d + 0x80; // extended

    // Navigation (extended keys, +0x80)
    case SDL_SCANCODE_INSERT:    return 0x52 + 0x80;
    case SDL_SCANCODE_HOME:      return 0x47 + 0x80;
    case SDL_SCANCODE_PAGEUP:    return 0x49 + 0x80;
    case SDL_SCANCODE_DELETE:    return 0x53 + 0x80;
    case SDL_SCANCODE_END:       return 0x4f + 0x80;
    case SDL_SCANCODE_PAGEDOWN:  return 0x51 + 0x80;

    // Arrow keys (extended)
    case SDL_SCANCODE_UP:        return 0x48 + 0x80;
    case SDL_SCANCODE_DOWN:      return 0x50 + 0x80;
    case SDL_SCANCODE_LEFT:      return 0x4b + 0x80;
    case SDL_SCANCODE_RIGHT:     return 0x4d + 0x80;

    // Numpad
    case SDL_SCANCODE_NUMLOCKCLEAR: return 0x45;
    case SDL_SCANCODE_KP_DIVIDE:    return 0x35 + 0x80; // extended
    case SDL_SCANCODE_KP_MULTIPLY:  return 0x37;
    case SDL_SCANCODE_KP_MINUS:     return 0x4a;
    case SDL_SCANCODE_KP_PLUS:      return 0x4e;
    case SDL_SCANCODE_KP_ENTER:     return 0x1c + 0x80; // extended
    case SDL_SCANCODE_KP_7:         return 0x47;
    case SDL_SCANCODE_KP_8:         return 0x48;
    case SDL_SCANCODE_KP_9:         return 0x49;
    case SDL_SCANCODE_KP_4:         return 0x4b;
    case SDL_SCANCODE_KP_5:         return 0x4c;
    case SDL_SCANCODE_KP_6:         return 0x4d;
    case SDL_SCANCODE_KP_1:         return 0x4f;
    case SDL_SCANCODE_KP_2:         return 0x50;
    case SDL_SCANCODE_KP_3:         return 0x51;
    case SDL_SCANCODE_KP_0:         return 0x52;
    case SDL_SCANCODE_KP_PERIOD:    return 0x53;

    // Misc
    case SDL_SCANCODE_PRINTSCREEN:  return 0x37 + 0x80; // extended
    case SDL_SCANCODE_SCROLLLOCK:   return 0x46;

    default: return 0;
    }
}

// ===========================================================================
// Event loop
// ===========================================================================

// Internal queue for gamepad connect/disconnect events.
// Filled by sdl3_poll_events(), drained by sdl3_gamepad_poll_event().
static constexpr int GP_EVENT_QUEUE_SIZE = 8;
static SDL3_GamepadEvent s_gp_queue[GP_EVENT_QUEUE_SIZE];
static int s_gp_queue_count = 0;
static int s_gp_queue_read  = 0;

void sdl3_set_callbacks(const SDL3_Callbacks* cb)
{
    if (cb) {
        s_callbacks = *cb;
    } else {
        memset(&s_callbacks, 0, sizeof(s_callbacks));
    }
}

bool sdl3_poll_events()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            if (s_callbacks.on_close) s_callbacks.on_close();
            return false;

        case SDL_EVENT_KEY_DOWN:
            if (!e.key.repeat) {
                uint8_t sc = sdl_to_game_scancode(e.key.scancode);
                if (sc && s_callbacks.on_key_down) s_callbacks.on_key_down(sc);
            }
            break;

        case SDL_EVENT_KEY_UP: {
            uint8_t sc = sdl_to_game_scancode(e.key.scancode);
            if (sc && s_callbacks.on_key_up) s_callbacks.on_key_up(sc);
            break;
        }

        case SDL_EVENT_MOUSE_MOTION:
            if (s_callbacks.on_mouse_move)
                s_callbacks.on_mouse_move((int)e.motion.x, (int)e.motion.y);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            bool down = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            int btn = -1;
            if (e.button.button == SDL_BUTTON_LEFT)   btn = 0;
            if (e.button.button == SDL_BUTTON_RIGHT)  btn = 1;
            if (e.button.button == SDL_BUTTON_MIDDLE) btn = 2;
            if (btn >= 0 && s_callbacks.on_mouse_button)
                s_callbacks.on_mouse_button(btn, down, (int)e.button.x, (int)e.button.y);
            break;
        }

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            if (s_callbacks.on_focus_gained) s_callbacks.on_focus_gained();
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            if (s_callbacks.on_focus_lost) s_callbacks.on_focus_lost();
            break;

        case SDL_EVENT_WINDOW_MOVED:
            if (s_callbacks.on_window_moved) s_callbacks.on_window_moved();
            break;

        // RESIZED fires on logical (point) size changes; PIXEL_SIZE_CHANGED
        // fires on drawable (pixel) size changes. On non-HiDPI displays the
        // two coincide; on HiDPI (Retina, per-monitor scaling) a window drag
        // between monitors of differing density may only fire PIXEL_SIZE.
        // We need the pixel path for FBO sizing, so handle both — the host
        // coalesces duplicate events into a single per-frame apply via
        // s_resize_pending (no time-based debounce).
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (s_callbacks.on_window_resized) s_callbacks.on_window_resized();
            break;

        // Queue gamepad connect/disconnect for sdl3_gamepad_poll_event().
        // SDL fires these for *every* gamepad (including ones we don't
        // open via SDL, like DualSense handled by ds_bridge), so the
        // joystick instance ID is preserved here for the consumer to
        // disambiguate.
        case SDL_EVENT_GAMEPAD_ADDED:
            if (s_gp_queue_count < GP_EVENT_QUEUE_SIZE)
                s_gp_queue[s_gp_queue_count++] =
                    { SDL3_GAMEPAD_EVENT_ADDED, (uint32_t)e.gdevice.which };
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            if (s_gp_queue_count < GP_EVENT_QUEUE_SIZE)
                s_gp_queue[s_gp_queue_count++] =
                    { SDL3_GAMEPAD_EVENT_REMOVED, (uint32_t)e.gdevice.which };
            break;

        default:
            break;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

bool sdl3_load_wav(const char* path, SDL3_WavData* out)
{
    SDL_AudioSpec spec;
    Uint8* buf;
    Uint32 len;
    if (!SDL_LoadWAV(path, &spec, &buf, &len)) {
        return false;
    }
    out->buffer = buf;
    out->size = len;
    out->channels = spec.channels;
    out->freq = spec.freq;
    return true;
}

void sdl3_free_wav(uint8_t* buffer)
{
    SDL_free(buffer);
}

// ---------------------------------------------------------------------------
// Gamepad
// ---------------------------------------------------------------------------

bool sdl3_gamepad_init()
{
    return SDL_Init(SDL_INIT_GAMEPAD) != 0;
}

void sdl3_gamepad_shutdown()
{
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

bool sdl3_gamepad_poll_event(SDL3_GamepadEvent* event)
{
    event->type = SDL3_GAMEPAD_EVENT_NONE;

    // Drain from internal queue (filled by sdl3_poll_events).
    if (s_gp_queue_read < s_gp_queue_count) {
        *event = s_gp_queue[s_gp_queue_read++];
        // Reset queue when fully drained.
        if (s_gp_queue_read >= s_gp_queue_count) {
            s_gp_queue_read = 0;
            s_gp_queue_count = 0;
        }
        return true;
    }

    // Reset queue indices.
    s_gp_queue_read = 0;
    s_gp_queue_count = 0;
    return false;
}

SDL3_GamepadHandle sdl3_gamepad_open()
{
    int count = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
    if (!joysticks || count == 0) {
        SDL_free(joysticks);
        return nullptr;
    }

    SDL_Gamepad* gp = SDL_OpenGamepad(joysticks[0]);
    SDL_free(joysticks);
    return static_cast<SDL3_GamepadHandle>(gp);
}

uint32_t sdl3_gamepad_instance_id(SDL3_GamepadHandle handle)
{
    if (!handle) return 0;
    SDL_JoystickID id = SDL_GetGamepadID(static_cast<SDL_Gamepad*>(handle));
    return (uint32_t)id;
}

SDL3_GamepadHandle sdl3_gamepad_open_id(uint32_t instance_id)
{
    SDL_Gamepad* gp = SDL_OpenGamepad((SDL_JoystickID)instance_id);
    return static_cast<SDL3_GamepadHandle>(gp);
}

void sdl3_gamepad_close(SDL3_GamepadHandle handle)
{
    if (handle) {
        SDL_CloseGamepad(static_cast<SDL_Gamepad*>(handle));
    }
}

bool sdl3_gamepad_get_state(SDL3_GamepadHandle handle, SDL3_GamepadState* out)
{
    SDL_Gamepad* gp = static_cast<SDL_Gamepad*>(handle);
    if (!gp) return false;

    out->axis_left_x   = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTX);
    out->axis_left_y   = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFTY);
    out->axis_right_x  = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHTX);
    out->axis_right_y  = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHTY);
    out->trigger_left  = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    out->trigger_right = SDL_GetGamepadAxis(gp, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

    uint32_t btns = 0;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_SOUTH))          btns |= SDL3_BTN_SOUTH;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_EAST))           btns |= SDL3_BTN_EAST;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_WEST))           btns |= SDL3_BTN_WEST;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_NORTH))          btns |= SDL3_BTN_NORTH;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_BACK))           btns |= SDL3_BTN_BACK;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_GUIDE))          btns |= SDL3_BTN_GUIDE;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_START))          btns |= SDL3_BTN_START;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_STICK))     btns |= SDL3_BTN_LEFT_STICK;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_STICK))    btns |= SDL3_BTN_RIGHT_STICK;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))  btns |= SDL3_BTN_LEFT_SHOULDER;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) btns |= SDL3_BTN_RIGHT_SHOULDER;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_UP))        btns |= SDL3_BTN_DPAD_UP;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_DOWN))      btns |= SDL3_BTN_DPAD_DOWN;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_LEFT))      btns |= SDL3_BTN_DPAD_LEFT;
    if (SDL_GetGamepadButton(gp, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))     btns |= SDL3_BTN_DPAD_RIGHT;
    out->buttons = btns;

    return true;
}

bool sdl3_gamepad_rumble(SDL3_GamepadHandle handle, uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms)
{
    SDL_Gamepad* gp = static_cast<SDL_Gamepad*>(handle);
    if (!gp) return false;
    return SDL_RumbleGamepad(gp, low_freq, high_freq, duration_ms);
}

uint16_t sdl3_gamepad_vendor_id(SDL3_GamepadHandle handle)
{
    SDL_Gamepad* gp = static_cast<SDL_Gamepad*>(handle);
    if (!gp) return 0;
    return SDL_GetGamepadVendor(gp);
}

uint16_t sdl3_gamepad_product_id(SDL3_GamepadHandle handle)
{
    SDL_Gamepad* gp = static_cast<SDL_Gamepad*>(handle);
    if (!gp) return 0;
    return SDL_GetGamepadProduct(gp);
}

// ===========================================================================
// Keyboard
// ===========================================================================

// Reverse map: game KB_* scancode → SDL_Scancode.
static SDL_Scancode game_to_sdl_scancode(int kb)
{
    // Brute-force reverse lookup: iterate SDL scancodes and find the one
    // that maps to the given game scancode.
    for (int sc = 0; sc < SDL_SCANCODE_COUNT; sc++) {
        if (sdl_to_game_scancode((SDL_Scancode)sc) == (uint8_t)kb)
            return (SDL_Scancode)sc;
    }
    return SDL_SCANCODE_UNKNOWN;
}

char* sdl3_get_key_name(int game_scancode, char* out, int out_size)
{
    SDL_Scancode sc = game_to_sdl_scancode(game_scancode);
    if (sc != SDL_SCANCODE_UNKNOWN) {
        const char* name = SDL_GetScancodeName(sc);
        if (name && name[0]) {
            strncpy(out, name, out_size - 1);
            out[out_size - 1] = '\0';
            // MENUFONT only has uppercase A-Z; SDL returns lowercase for letter keys.
            for (char* p = out; *p; ++p)
                *p = (char)toupper((unsigned char)*p);
            return out;
        }
    }
    strncpy(out, "???", out_size - 1);
    out[out_size - 1] = '\0';
    return out;
}
