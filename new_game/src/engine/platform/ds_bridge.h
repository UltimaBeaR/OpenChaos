#pragma once

// DualSense bridge — thin abstraction layer over libDualsense.
// Isolates HID-level headers from game code. Game code accesses
// DualSense functionality exclusively through functions declared here.

#include <cstdint>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// Initialize SDL HID subsystem + DualSense device layer.
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

// Read input from connected DualSense. Returns false on no device /
// disconnect (including BT silence timeout — libDualsense handles
// that internally).
bool ds_update_input();

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
    float left_stick_x; // -1.0 .. +1.0
    float left_stick_y; // -1.0 .. +1.0
    float right_stick_x; // -1.0 .. +1.0
    float right_stick_y; // -1.0 .. +1.0
    float trigger_left; // 0.0 .. 1.0
    float trigger_right; // 0.0 .. 1.0

    // Face buttons
    bool cross;
    bool circle;
    bool square;
    bool triangle;

    // Shoulders
    bool l1;
    bool r1;
    bool l2_digital; // trigger past threshold
    bool r2_digital;

    // D-Pad
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;

    // Menu
    bool start;
    bool share; // Select/Create
    bool ps_button;

    // Sticks click
    bool l3;
    bool r3;

    // Misc
    bool mute;
    bool touchpad_click;

    // Adaptive trigger feedback status (reported by the controller for
    // the currently active effect). State value: 0 = before effect zone,
    // 1 = inside effect zone, 2 = past effect zone. *_effect_active is
    // true when the controller signals that the effect is engaged.
    // Only meaningful for effect modes Off (0x05), Feedback (0x21),
    // Weapon (0x25), Vibration/MachineGun (0x26).
    uint8_t left_trigger_feedback_state;
    uint8_t right_trigger_feedback_state;
    bool left_trigger_effect_active;
    bool right_trigger_effect_active;

    // Touchpad fingers. Coordinates are raw touchpad pixels (hardware
    // is 1920×1080). When a finger isn't touching, `*_down` is false and
    // X/Y hold the last position the finger had while down.
    uint16_t touchpad_finger_1_x;
    uint16_t touchpad_finger_1_y;
    bool touchpad_finger_1_down;
    uint16_t touchpad_finger_2_x;
    uint16_t touchpad_finger_2_y;
    bool touchpad_finger_2_down;
};

bool ds_get_input(DS_InputState* out);

// ---------------------------------------------------------------------------
// Output — LED, vibration, adaptive triggers
// ---------------------------------------------------------------------------

void ds_set_lightbar(uint8_t r, uint8_t g, uint8_t b);
void ds_set_player_led(uint8_t led_mask);
void ds_set_player_led_brightness(uint8_t brightness); // 0 = brightest, 2 = dimmest
void ds_set_vibration(uint8_t left, uint8_t right);

// Mic-mute LED: 0 = Off, 1 = On (steady), 2 = Blink. Values outside
// that range are clamped to Off.
void ds_set_mute_led(uint8_t mode);

// Overall haptic (rumble) volume, 0..7 (clamped). Applied by the
// controller on top of per-motor amplitudes.
void ds_set_haptic_volume(uint8_t volume);

// Lightbar setup/override byte — raw DualSense wire value. 0 = default.
// Sent on the wire but no documented meaning has been verified on real
// hardware. Kept as a passthrough for API completeness.
void ds_set_lightbar_setup(uint8_t setup);

// Audio volume control — opt-in so the game does not fight the host OS
// for audio state. While `enabled` is false, the lib leaves the
// controller's speaker/jack volume untouched. Set enabled + both
// volumes to drive audio from the panel / game.
void ds_set_audio_volumes_enabled(bool enabled);
void ds_set_speaker_volume(uint8_t volume);
void ds_set_headphone_volume(uint8_t volume);

// Audio physical routing: 0 = 3.5 mm headphone jack, 1 = built-in
// speaker. Only meaningful when audio_volumes_enabled=true. Values
// outside 0..1 are treated as 0.
void ds_set_audio_route(uint8_t route);

// Adaptive trigger modes (hand: 0=left, 1=right, 2=both)
void ds_trigger_off(uint8_t hand);
void ds_trigger_resistance(uint8_t start_zone, uint8_t strength, uint8_t hand);
void ds_trigger_feedback(uint8_t position, uint8_t strength, uint8_t hand);
void ds_trigger_weapon(uint8_t start_zone, uint8_t amplitude, uint8_t behavior, uint8_t trigger, uint8_t hand);
void ds_trigger_vibration(uint8_t position, uint8_t amplitude, uint8_t frequency, uint8_t hand);
// Full-parameter variants used by the input debug panel trigger tester.
// The simplified *_trigger_bow / *_trigger_machine above collapse
// parameters for game code; these expose every tunable byte.
void ds_trigger_bow_full(uint8_t start, uint8_t end,
    uint8_t strength, uint8_t snap, uint8_t hand);
void ds_trigger_machine_full(uint8_t start, uint8_t end,
    uint8_t amplitude_a, uint8_t amplitude_b,
    uint8_t frequency, uint8_t period, uint8_t hand);

// Additional effects — not used by game code today, exposed for the
// input debug panel trigger tester. hand: 0=left, 1=right, 2=both.
void ds_trigger_galloping(uint8_t start, uint8_t end,
    uint8_t first_foot, uint8_t second_foot,
    uint8_t frequency, uint8_t hand);
void ds_trigger_simple_weapon(uint8_t start_raw, uint8_t end_raw,
    uint8_t strength_raw, uint8_t hand);
void ds_trigger_simple_vibration(uint8_t position, uint8_t amplitude,
    uint8_t frequency, uint8_t hand);
void ds_trigger_limited_feedback(uint8_t position, uint8_t strength, uint8_t hand);
void ds_trigger_limited_weapon(uint8_t start_raw, uint8_t end_raw,
    uint8_t strength, uint8_t hand);

// Debug: copy the current 10-byte trigger effect slot bytes
// (as they will be sent in the next output report). Used by the
// input debug panel's trigger tester to verify what the game
// is packing.
void ds_debug_get_trigger_slots(uint8_t left[10], uint8_t right[10]);
