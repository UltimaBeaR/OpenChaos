#pragma once

// DualSense bridge — wraps the Dualsense-Multiplatform (GamepadCore) library.
//
// Like sdl3_bridge, this file is compiled with /Zp8 because both SDL3 and
// GamepadCore headers require standard struct alignment.  Game code accesses
// DualSense functionality exclusively through functions declared here.

#include <cstdint>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Initialize SDL HID subsystem + GamepadCore registry.
// Call once at startup (after SDL_Init).
void ds_init();

// Shutdown: release device handles, cleanup registry.
void ds_shutdown();

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

// Poll for device connect/disconnect (called every ~1s internally).
// delta_time is seconds since last call.
void ds_poll_registry(float delta_time);

// Read input from connected DualSense. Returns false if no device.
bool ds_update_input(float delta_time);

// Send pending output (LED, triggers, rumble) to device.
void ds_update_output();

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

// Returns true if a DualSense is currently connected and active.
bool ds_is_connected();

// ---------------------------------------------------------------------------
// Input — read current state (valid after ds_update_input)
// ---------------------------------------------------------------------------

struct DS_InputState {
    float left_stick_x;    // -1.0 .. +1.0
    float left_stick_y;    // -1.0 .. +1.0
    float right_stick_x;   // -1.0 .. +1.0
    float right_stick_y;   // -1.0 .. +1.0
    float trigger_left;    // 0.0 .. 1.0
    float trigger_right;   // 0.0 .. 1.0

    // Face buttons
    bool cross;
    bool circle;
    bool square;
    bool triangle;

    // Shoulders
    bool l1;
    bool r1;
    bool l2_digital;  // trigger past threshold
    bool r2_digital;

    // D-Pad
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;

    // Menu
    bool start;
    bool share;      // Select/Create
    bool ps_button;

    // Sticks click
    bool l3;
    bool r3;

    // Misc
    bool mute;
    bool touchpad_click;
};

bool ds_get_input(DS_InputState* out);

// ---------------------------------------------------------------------------
// Output — LED, vibration, adaptive triggers
// ---------------------------------------------------------------------------

void ds_set_lightbar(uint8_t r, uint8_t g, uint8_t b);
void ds_set_player_led(uint8_t led_mask);
void ds_set_vibration(uint8_t left, uint8_t right);

// Adaptive trigger modes (hand: 0=left, 1=right, 2=both)
void ds_trigger_off(uint8_t hand);
void ds_trigger_resistance(uint8_t start_zone, uint8_t strength, uint8_t hand);
void ds_trigger_weapon(uint8_t start_zone, uint8_t amplitude, uint8_t behavior, uint8_t trigger, uint8_t hand);
void ds_trigger_bow(uint8_t start_zone, uint8_t snap_back, uint8_t hand);
void ds_trigger_machine(uint8_t start_zone, uint8_t behavior_flag, uint8_t force, uint8_t amplitude, uint8_t period, uint8_t frequency, uint8_t hand);
