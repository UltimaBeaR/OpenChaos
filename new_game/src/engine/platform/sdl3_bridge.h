#pragma once

// SDL3 bridge — the ONLY file in the project that includes SDL3 headers.
//
// The project compiles with /Zp1 (1-byte struct packing) for binary resource
// format compatibility.  SDL3 headers contain static_asserts that fail under
// /Zp1, so sdl3_bridge.cpp is compiled with /Zp8.  All other code accesses
// SDL3 functionality exclusively through functions declared here.

#include <cstdint>

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

// Opaque handle to an SDL3 gamepad (hides SDL_Gamepad* from /Zp1 code).
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
