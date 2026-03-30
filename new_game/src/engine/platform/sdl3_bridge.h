#pragma once

// SDL3 bridge — thin abstraction layer over SDL3.
// Isolates SDL3 headers from game code so that SDL3 details don't leak
// into the rest of the codebase.  Game code accesses SDL3 functionality
// exclusively through functions declared here.

#include <cstdint>

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------

// Create an SDL3 window with OpenGL support.
bool sdl3_window_create(const char* title, int width, int height);

// Destroy the SDL3 window.
void sdl3_window_destroy();

// Show the window (after GL context is ready).
void sdl3_window_show();

// Get client area size.
void sdl3_window_get_size(int* w, int* h);

// Get window position (screen coordinates of top-left corner).
void sdl3_window_get_position(int* x, int* y);

// Get window center in screen coordinates.
void sdl3_window_get_center(int* x, int* y);

// Resize the window client area.
void sdl3_window_set_size(int w, int h);

// Get actual drawable size in pixels (may differ from window size on HiDPI).
void sdl3_window_get_drawable_size(int* w, int* h);

// Warp mouse cursor to screen coordinates.
void sdl3_warp_mouse_global(int x, int y);

// Get global mouse cursor position (screen coordinates).
void sdl3_get_global_mouse_pos(int* x, int* y);

// Show / hide the system cursor.
void sdl3_show_cursor();
void sdl3_hide_cursor();

// Grab / release mouse input (confine to window).
void sdl3_set_mouse_grab(bool grab);

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

// Get a human-readable name for a game scancode (KB_* value).
// Writes into out (up to out_size chars). Returns out.
char* sdl3_get_key_name(int game_scancode, char* out, int out_size);

// ---------------------------------------------------------------------------
// GL context
// ---------------------------------------------------------------------------

// Create an OpenGL core profile context on the SDL3 window.
bool sdl3_gl_create_context(int major, int minor);

// Destroy the GL context.
void sdl3_gl_destroy_context();

// Swap buffers (present frame).
void sdl3_gl_swap();

// Get a GL function pointer (for GLAD loader).
void* sdl3_gl_get_proc_address(const char* name);

// ---------------------------------------------------------------------------
// Event loop
// ---------------------------------------------------------------------------

// Callbacks for dispatching SDL events to game subsystems.
struct SDL3_Callbacks {
    void (*on_key_down)(uint8_t scancode);       // scancode = game KB_* code
    void (*on_key_up)(uint8_t scancode);
    void (*on_mouse_move)(int x, int y);
    void (*on_mouse_button)(int button, bool down, int x, int y); // 0=left, 1=right, 2=middle
    void (*on_focus_gained)();
    void (*on_focus_lost)();
    void (*on_window_moved)();
    void (*on_window_resized)();
    void (*on_close)();
};

// Register event callbacks. Must be called before sdl3_poll_events().
void sdl3_set_callbacks(const SDL3_Callbacks* cb);

// Poll and dispatch all pending SDL events.
// Returns false when SDL_QUIT is received (app should exit).
bool sdl3_poll_events();

// ---------------------------------------------------------------------------
// Timer
// ---------------------------------------------------------------------------

// Returns milliseconds since SDL init (wraps SDL_GetTicks).
uint64_t sdl3_get_ticks();

// Returns high-resolution performance counter value (wraps SDL_GetPerformanceCounter).
uint64_t sdl3_get_performance_counter();

// Returns performance counter frequency in Hz (wraps SDL_GetPerformanceFrequency).
uint64_t sdl3_get_performance_frequency();

// ---------------------------------------------------------------------------
// Audio
// ---------------------------------------------------------------------------

struct SDL3_WavData {
    uint8_t* buffer;
    uint32_t size;
    int channels;
    int freq;
};

bool sdl3_load_wav(const char* path, SDL3_WavData* out);
void sdl3_free_wav(uint8_t* buffer);

// ---------------------------------------------------------------------------
// Gamepad
// ---------------------------------------------------------------------------

// Opaque handle to an SDL3 gamepad (hides SDL_Gamepad* from game code).
using SDL3_GamepadHandle = void*;

enum SDL3_GamepadEventType {
    SDL3_GAMEPAD_EVENT_NONE,
    SDL3_GAMEPAD_EVENT_ADDED,
    SDL3_GAMEPAD_EVENT_REMOVED,
};

struct SDL3_GamepadEvent {
    SDL3_GamepadEventType type;
};

struct SDL3_GamepadState {
    int16_t axis_left_x;   // -32768..+32767
    int16_t axis_left_y;
    int16_t axis_right_x;
    int16_t axis_right_y;
    int16_t trigger_left;  // 0..32767
    int16_t trigger_right;
    uint32_t buttons;      // bitmask, see SDL3_GAMEPAD_BUTTON_* below
};

// Button bitmask values (match SDL_GamepadButton order).
enum SDL3_GamepadButton : uint32_t {
    SDL3_BTN_SOUTH        = 1 << 0,   // A / Cross
    SDL3_BTN_EAST         = 1 << 1,   // B / Circle
    SDL3_BTN_WEST         = 1 << 2,   // X / Square
    SDL3_BTN_NORTH        = 1 << 3,   // Y / Triangle
    SDL3_BTN_BACK         = 1 << 4,   // Back / Select
    SDL3_BTN_GUIDE        = 1 << 5,   // Xbox / PS button
    SDL3_BTN_START        = 1 << 6,
    SDL3_BTN_LEFT_STICK   = 1 << 7,
    SDL3_BTN_RIGHT_STICK  = 1 << 8,
    SDL3_BTN_LEFT_SHOULDER  = 1 << 9,  // LB / L1
    SDL3_BTN_RIGHT_SHOULDER = 1 << 10, // RB / R1
    SDL3_BTN_DPAD_UP      = 1 << 11,
    SDL3_BTN_DPAD_DOWN    = 1 << 12,
    SDL3_BTN_DPAD_LEFT    = 1 << 13,
    SDL3_BTN_DPAD_RIGHT   = 1 << 14,
};

bool sdl3_gamepad_init();
void sdl3_gamepad_shutdown();

// Pumps SDL events. Returns true if a gamepad event occurred (check *event).
bool sdl3_gamepad_poll_event(SDL3_GamepadEvent* event);

// Opens the first available gamepad. Returns handle or nullptr.
SDL3_GamepadHandle sdl3_gamepad_open();
void sdl3_gamepad_close(SDL3_GamepadHandle handle);

// Reads current state. Returns false if not connected.
bool sdl3_gamepad_get_state(SDL3_GamepadHandle handle, SDL3_GamepadState* out);

// Rumble. Duration in ms. Intensities 0..65535.
bool sdl3_gamepad_rumble(SDL3_GamepadHandle handle, uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms);

// Returns SDL3's vendor/product ID for DualSense detection.
uint16_t sdl3_gamepad_vendor_id(SDL3_GamepadHandle handle);
uint16_t sdl3_gamepad_product_id(SDL3_GamepadHandle handle);
