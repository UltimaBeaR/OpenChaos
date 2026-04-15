// Gamepad abstraction — SDL3 backend (via sdl3_bridge) for Xbox/generic controllers,
// Dualsense-Multiplatform (via ds_bridge) for DualSense/DualShock4.
// Translates axes to DirectInput range (0..65535) so downstream code works unchanged.

#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/input/weapon_feel.h"
#include "engine/input/keyboard_globals.h" // Keys[] for active device detection
#include "engine/platform/sdl3_bridge.h"
#include "engine/platform/ds_bridge.h"
#include "game/input_actions_globals.h"
#include <cstring>
#include <cstdio> // temporary debug log

// Sony vendor/product IDs for DualSense detection (SDL3 path).
static constexpr uint16_t SONY_VENDOR_ID       = 0x054C;
static constexpr uint16_t DUALSENSE_PRODUCT_ID = 0x0CE6;
static constexpr uint16_t DUALSENSE_EDGE_PRODUCT_ID = 0x0DF2;

static SDL3_GamepadHandle s_gamepad = nullptr;
static bool s_is_dualsense = false;
static uint32_t s_consume_mask = 0; // bitmask of button indices to consume until released

// PS1-style motor state (maximum tracking + per-tick decay).
// uc_orig: psx_motor[2] (fallen/psxlib/Source/GDisplay.cpp)
static int s_motor_fast = 0; // small motor: 0 or 1 (decays >>= 1)
static int s_motor_slow = 0; // large motor: 0-255 (decays * 7 >> 3)

// Frame time tracking for ds_bridge (needs delta_time in seconds).
static float s_last_poll_delta = 1.0f / 30.0f; // assume 30fps initially

// Last-sent LED color — dirty tracking to avoid unnecessary HID writes.
// On macOS Bluetooth, each HID write can block 7-30ms, starving input reads.
static uint8_t s_led_r = 0, s_led_g = 0, s_led_b = 0;

// Temporary: append backend changes to file for verification (Debug only).
static void debug_log_backend([[maybe_unused]] const char* event)
{
#ifdef _DEBUG
    if (FILE* f = fopen("gamepad_log.txt", "a")) {
        const char* backend = "none";
        if (s_is_dualsense) backend = "DualSense (GamepadCore/ds_bridge)";
        else if (s_gamepad) backend = "Xbox/generic (SDL3)";
        fprintf(f, "[%s] %s\n", event, backend);
        fclose(f);
    }
#endif
}

static bool is_dualsense_sdl(SDL3_GamepadHandle handle)
{
    uint16_t vid = sdl3_gamepad_vendor_id(handle);
    uint16_t pid = sdl3_gamepad_product_id(handle);
    return vid == SONY_VENDOR_ID &&
           (pid == DUALSENSE_PRODUCT_ID || pid == DUALSENSE_EDGE_PRODUCT_ID);
}

void gamepad_init()
{
    // Initialize DualSense bridge first — it sets SDL_HINT to not grab DualSense
    // via SDL3's gamepad subsystem, so SDL3 only sees Xbox/generic controllers.
    ds_init();

    sdl3_gamepad_init();

    // Try to open a gamepad via SDL3 (will be Xbox/generic only, DualSense excluded by hint).
    s_gamepad = sdl3_gamepad_open();
    if (s_gamepad) {
        // Double-check: if somehow SDL3 still grabbed a DualSense, release it.
        if (is_dualsense_sdl(s_gamepad)) {
            sdl3_gamepad_close(s_gamepad);
            s_gamepad = nullptr;
        } else {
            s_is_dualsense = false;
            active_input_device = INPUT_DEVICE_XBOX;
            gamepad_state.connected = true;
        }
    }

    // Let DS-lib do initial device scan.
    ds_poll_registry(2.0f); // force immediate detection (> 1s interval)
    if (ds_is_connected()) {
        s_is_dualsense = true;
        active_input_device = INPUT_DEVICE_DUALSENSE;
        gamepad_state.connected = true;
        gamepad_led_reset(); // start with default blue
    }

    weapon_feel_init();

    debug_log_backend("init");
}

void gamepad_shutdown()
{
    weapon_feel_shutdown();
    if (s_gamepad) {
        sdl3_gamepad_close(s_gamepad);
        s_gamepad = nullptr;
    }
    sdl3_gamepad_shutdown();
    ds_shutdown();
}

// ---------------------------------------------------------------------------
// DualSense input path — translate DS_InputState to GamepadState
// ---------------------------------------------------------------------------

static void poll_dualsense()
{
    if (!ds_is_connected()) {
        // DualSense disconnected
        s_is_dualsense = false;
        gamepad_state.connected = (s_gamepad != nullptr);
        if (!gamepad_state.connected) {
            // Reset axes to centre (32768) so downstream code doesn't see phantom input.
            memset(&gamepad_state, 0, sizeof(gamepad_state));
            gamepad_state.lX = 32768;
            gamepad_state.lY = 32768;
            gamepad_state.rX = 32768;
            gamepad_state.rY = 32768;
            active_input_device = INPUT_DEVICE_KEYBOARD_MOUSE;
        }
        debug_log_backend("ds_disconnected");
        return;
    }

    ds_update_input(s_last_poll_delta);

    DS_InputState ds;
    if (!ds_get_input(&ds)) return;

    // Translate float sticks (-1..+1) to DI range (0..65535, center 32768).
    gamepad_state.lX = static_cast<int32_t>(ds.left_stick_x  * 32767.0f) + 32768;
    gamepad_state.lY = static_cast<int32_t>(-ds.left_stick_y * 32767.0f) + 32768; // invert Y
    gamepad_state.rX = static_cast<int32_t>(ds.right_stick_x * 32767.0f) + 32768;
    gamepad_state.rY = static_cast<int32_t>(-ds.right_stick_y * 32767.0f) + 32768;

    // D-Pad overrides axes.
    gamepad_state.dpad_active = ds.dpad_up || ds.dpad_down || ds.dpad_left || ds.dpad_right;
    if (ds.dpad_left)  gamepad_state.lX = 0;
    if (ds.dpad_right) gamepad_state.lX = 65535;
    if (ds.dpad_up)    gamepad_state.lY = 0;
    if (ds.dpad_down)  gamepad_state.lY = 65535;

    // Map to rgbButtons[] — same indices as SDL3 path for compatibility with
    // joypad_button_use[] mapping in input_actions.cpp.
    memset(gamepad_state.rgbButtons, 0, sizeof(gamepad_state.rgbButtons));
    if (ds.cross)    gamepad_state.rgbButtons[0]  = 0x80; // SDL3_BTN_SOUTH
    if (ds.circle)   gamepad_state.rgbButtons[1]  = 0x80; // SDL3_BTN_EAST
    if (ds.square)   gamepad_state.rgbButtons[2]  = 0x80; // SDL3_BTN_WEST
    if (ds.triangle) gamepad_state.rgbButtons[3]  = 0x80; // SDL3_BTN_NORTH
    if (ds.share)    gamepad_state.rgbButtons[4]  = 0x80; // SDL3_BTN_BACK
    if (ds.ps_button) gamepad_state.rgbButtons[5] = 0x80; // SDL3_BTN_GUIDE
    if (ds.start)    gamepad_state.rgbButtons[6]  = 0x80; // SDL3_BTN_START
    if (ds.l3)       gamepad_state.rgbButtons[7]  = 0x80; // SDL3_BTN_LEFT_STICK
    if (ds.r3)       gamepad_state.rgbButtons[8]  = 0x80; // SDL3_BTN_RIGHT_STICK
    if (ds.l1)       gamepad_state.rgbButtons[9]  = 0x80; // SDL3_BTN_LEFT_SHOULDER
    if (ds.r1)       gamepad_state.rgbButtons[10] = 0x80; // SDL3_BTN_RIGHT_SHOULDER
    if (ds.dpad_up)    gamepad_state.rgbButtons[11] = 0x80;
    if (ds.dpad_down)  gamepad_state.rgbButtons[12] = 0x80;
    if (ds.dpad_left)  gamepad_state.rgbButtons[13] = 0x80;
    if (ds.dpad_right) gamepad_state.rgbButtons[14] = 0x80;

    // Triggers as digital buttons (indices 15/16).
    if (ds.l2_digital) gamepad_state.rgbButtons[15] = 0x80;
    if (ds.r2_digital) gamepad_state.rgbButtons[16] = 0x80;

    // Analog trigger values (0..255).
    gamepad_state.trigger_left  = static_cast<uint8_t>(ds.trigger_left  * 255.0f);
    gamepad_state.trigger_right = static_cast<uint8_t>(ds.trigger_right * 255.0f);

    gamepad_state.connected = true;
}

// ---------------------------------------------------------------------------
// SDL3 input path (Xbox/generic)
// ---------------------------------------------------------------------------

static void poll_sdl3()
{
    // Process connect/disconnect events.
    SDL3_GamepadEvent event;
    while (sdl3_gamepad_poll_event(&event)) {
        if (event.type == SDL3_GAMEPAD_EVENT_ADDED && !s_gamepad) {
            s_gamepad = sdl3_gamepad_open();
            if (s_gamepad) {
                if (is_dualsense_sdl(s_gamepad)) {
                    // DualSense snuck through SDL3 — release, let ds_bridge handle it.
                    sdl3_gamepad_close(s_gamepad);
                    s_gamepad = nullptr;
                } else {
                    s_is_dualsense = false;
                    active_input_device = INPUT_DEVICE_XBOX;
                    gamepad_state.connected = true;
                    debug_log_backend("sdl3_connected");
                }
            }
        } else if (event.type == SDL3_GAMEPAD_EVENT_REMOVED && s_gamepad) {
            sdl3_gamepad_close(s_gamepad);
            s_gamepad = nullptr;
            gamepad_state.connected = false;
            // Reset axes to centre (32768) so downstream code doesn't see phantom input.
            memset(&gamepad_state, 0, sizeof(gamepad_state));
            gamepad_state.lX = 32768;
            gamepad_state.lY = 32768;
            gamepad_state.rX = 32768;
            gamepad_state.rY = 32768;
            active_input_device = INPUT_DEVICE_KEYBOARD_MOUSE;
            debug_log_backend("sdl3_disconnected");
        }
    }

    if (!s_gamepad) return;

    SDL3_GamepadState sdl_state;
    if (!sdl3_gamepad_get_state(s_gamepad, &sdl_state)) return;

    // Translate SDL3 axes (-32768..+32767) to DI range (0..65535).
    gamepad_state.lX = static_cast<int32_t>(sdl_state.axis_left_x) + 32768;
    gamepad_state.lY = static_cast<int32_t>(sdl_state.axis_left_y) + 32768;
    gamepad_state.rX = static_cast<int32_t>(sdl_state.axis_right_x) + 32768;
    gamepad_state.rY = static_cast<int32_t>(sdl_state.axis_right_y) + 32768;

    // D-Pad overrides axes.
    uint32_t btns = sdl_state.buttons;
    uint32_t dpad = btns & (SDL3_BTN_DPAD_LEFT | SDL3_BTN_DPAD_RIGHT | SDL3_BTN_DPAD_UP | SDL3_BTN_DPAD_DOWN);
    gamepad_state.dpad_active = (dpad != 0);
    if (btns & SDL3_BTN_DPAD_LEFT)  gamepad_state.lX = 0;
    if (btns & SDL3_BTN_DPAD_RIGHT) gamepad_state.lX = 65535;
    if (btns & SDL3_BTN_DPAD_UP)    gamepad_state.lY = 0;
    if (btns & SDL3_BTN_DPAD_DOWN)  gamepad_state.lY = 65535;

    // Map SDL3 buttons to rgbButtons[].
    memset(gamepad_state.rgbButtons, 0, sizeof(gamepad_state.rgbButtons));
    for (int i = 0; i < 15; i++) {
        if (btns & (1u << i)) {
            gamepad_state.rgbButtons[i] = 0x80;
        }
    }

    // Triggers as digital buttons.
    if (sdl_state.trigger_left > 8000)  gamepad_state.rgbButtons[15] = 0x80;
    if (sdl_state.trigger_right > 8000) gamepad_state.rgbButtons[16] = 0x80;

    // Analog trigger values: SDL3 range 0..32767 → 0..255.
    gamepad_state.trigger_left  = static_cast<uint8_t>(sdl_state.trigger_left  >> 7);
    gamepad_state.trigger_right = static_cast<uint8_t>(sdl_state.trigger_right >> 7);

    gamepad_state.connected = true;

    // Track activity.
    if (btns || sdl_state.trigger_left > 8000 || sdl_state.trigger_right > 8000 ||
        sdl_state.axis_left_x < -8000 || sdl_state.axis_left_x > 8000 ||
        sdl_state.axis_left_y < -8000 || sdl_state.axis_left_y > 8000 ||
        sdl_state.axis_right_x < -8000 || sdl_state.axis_right_x > 8000 ||
        sdl_state.axis_right_y < -8000 || sdl_state.axis_right_y > 8000) {
        active_input_device = INPUT_DEVICE_XBOX;
    }
}

// ---------------------------------------------------------------------------
// Main poll
// ---------------------------------------------------------------------------

void gamepad_poll()
{
    // Detect keyboard activity.
    for (int i = 0; i < 256; i++) {
        if (Keys[i]) {
            active_input_device = INPUT_DEVICE_KEYBOARD_MOUSE;
            break;
        }
    }

    // Always scan for DualSense connect/disconnect (registry has 1s internal cooldown).
    ds_poll_registry(s_last_poll_delta);

    // DualSense path (DS-lib via ds_bridge).
    if (s_is_dualsense || ds_is_connected()) {
        poll_dualsense();
        if (s_is_dualsense) {
            // Track DualSense activity for device switching.
            for (int i = 0; i < 17; i++) {
                if (gamepad_state.rgbButtons[i]) {
                    active_input_device = INPUT_DEVICE_DUALSENSE;
                    break;
                }
            }
            if (gamepad_state.lX < 16384 || gamepad_state.lX > 49152 ||
                gamepad_state.lY < 16384 || gamepad_state.lY > 49152 ||
                gamepad_state.rX < 16384 || gamepad_state.rX > 49152 ||
                gamepad_state.rY < 16384 || gamepad_state.rY > 49152) {
                active_input_device = INPUT_DEVICE_DUALSENSE;
            }
        }
    }

    // Also poll SDL3 for Xbox/generic (and for detecting new non-DualSense controllers).
    if (!s_is_dualsense) {
        poll_sdl3();
    } else {
        // Still pump SDL3 events even when DualSense active, to detect Xbox hotplug.
        SDL3_GamepadEvent event;
        while (sdl3_gamepad_poll_event(&event)) {
            // Ignore — DualSense is primary. Xbox hotplug will be picked up
            // when DualSense disconnects and we fall back to poll_sdl3().
        }
    }

    // Check if DualSense appeared while we were on Xbox or no controller.
    if (!s_is_dualsense && ds_is_connected()) {
        s_is_dualsense = true;
        active_input_device = INPUT_DEVICE_DUALSENSE;
        gamepad_state.connected = true;
        gamepad_led_reset(); // default blue until gameplay starts
        debug_log_backend("ds_hotplug");
    }

    // Consume buttons marked by gamepad_consume_until_released().
    if (s_consume_mask) {
        for (int i = 0; i < 32; i++) {
            if (s_consume_mask & (1u << i)) {
                if (gamepad_state.rgbButtons[i]) {
                    gamepad_state.rgbButtons[i] = 0;
                } else {
                    s_consume_mask &= ~(1u << i);
                }
            }
        }
    }
}

void gamepad_rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms)
{
    if (s_is_dualsense && ds_is_connected()) {
        // DS-lib vibration: scale 16-bit to 8-bit.
        ds_set_vibration(static_cast<uint8_t>(low_freq >> 8),
                         static_cast<uint8_t>(high_freq >> 8));
        ds_update_output();
    } else if (s_gamepad) {
        sdl3_gamepad_rumble(s_gamepad, low_freq, high_freq, duration_ms);
    }
}

void gamepad_set_shock(int fast, int slow)
{
    if (!g_bEngineVibrations) return;
    if (slow > 255) slow = 255;
    // Maximum tracking: only update if new value exceeds current.
    if (fast > s_motor_fast) s_motor_fast = fast;
    if (slow > s_motor_slow) s_motor_slow = slow;
}

void gamepad_rumble_tick()
{
    static bool s_was_active = false;

    // Envelope-based weapon haptic (DualSense-only). Runs independently of
    // the PS1-style shock motors and is max-merged into the slow motor.
    bool haptic_active = false;
    uint8_t haptic_slow = weapon_feel_tick_haptic(&haptic_active);

    if (!s_motor_fast && !s_motor_slow && !haptic_active) {
        // Motors at zero — send one final stop command to DualSense (no auto-timeout).
        if (s_was_active) {
            s_was_active = false;
            if (s_is_dualsense && ds_is_connected()) {
                ds_set_vibration(0, 0);
                ds_update_output();
            }
        }
        return;
    }

    s_was_active = true;

    bool has_gamepad = s_is_dualsense ? ds_is_connected() : (s_gamepad != nullptr);
    if (!has_gamepad || active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE) {
        s_motor_fast = 0;
        s_motor_slow = 0;
        return;
    }

    int out_slow = s_motor_slow;
    if (haptic_slow > out_slow) out_slow = haptic_slow;

    if (s_is_dualsense) {
        uint8_t vib_left = static_cast<uint8_t>(out_slow);
        uint8_t vib_right = s_motor_fast ? 255 : 0;
        // Only send HID write when vibration values actually changed.
        static uint8_t s_last_vib_l = 0, s_last_vib_r = 0;
        if (vib_left != s_last_vib_l || vib_right != s_last_vib_r) {
            s_last_vib_l = vib_left;
            s_last_vib_r = vib_right;
            ds_set_vibration(vib_left, vib_right);
            ds_update_output();
        }
    } else {
        uint16_t low = static_cast<uint16_t>(out_slow * 257);
        uint16_t high = s_motor_fast ? 65535 : 0;
        sdl3_gamepad_rumble(s_gamepad, low, high, 100);
    }

    // Decay motors.
    s_motor_fast >>= 1;
    s_motor_slow = (s_motor_slow * 7) >> 3;
}

void gamepad_rumble_stop()
{
    s_motor_fast = 0;
    s_motor_slow = 0;
    weapon_feel_stop_haptic();

    if (s_is_dualsense && ds_is_connected()) {
        ds_set_vibration(0, 0);
        ds_update_output();
    } else if (s_gamepad) {
        sdl3_gamepad_rumble(s_gamepad, 0, 0, 0);
    }
}

void gamepad_led_reset()
{
    if (!s_is_dualsense || !ds_is_connected()) return;
    s_led_r = 0; s_led_g = 0; s_led_b = 255;
    ds_set_lightbar(0, 0, 255); // default blue
    ds_update_output();
}

void gamepad_led_update(float health_fraction, bool siren)
{
    if (!s_is_dualsense || !ds_is_connected()) return;

    uint8_t r, g, b;

    if (siren) {
        // Police siren: fast red/blue alternation
        static int siren_counter = 0;
        siren_counter++;
        bool phase = (siren_counter / 4) & 1; // ~7.5 Hz at 30fps — fast strobe
        if (phase) {
            r = 255; g = 0; b = 0;
        } else {
            r = 0; g = 0; b = 255;
        }
    } else {
        // Health-based color
        if (health_fraction < 0.0f) health_fraction = 0.0f;
        if (health_fraction > 1.0f) health_fraction = 1.0f;

        if (health_fraction > 0.75f) {
            // 100-75%: green
            r = 0; g = 255; b = 0;
        } else if (health_fraction > 0.5f) {
            // 75-50%: green → yellow gradient
            float t = (health_fraction - 0.5f) / 0.25f;
            r = static_cast<uint8_t>(255 * (1.0f - t));
            g = 255;
            b = 0;
        } else if (health_fraction > 0.25f) {
            // 50-25%: yellow → orange/red gradient
            float t = (health_fraction - 0.25f) / 0.25f;
            r = 255;
            g = static_cast<uint8_t>(200 * t);
            b = 0;
        } else {
            // <25%: red, blinking
            static int blink_counter = 0;
            blink_counter++;
            bool on = (blink_counter / 15) & 1;
            r = on ? 255 : 40;
            g = 0;
            b = 0;
        }
    }

    // Only send HID write when color actually changed — on macOS Bluetooth,
    // each write (~78 bytes) can block 7-30ms, flooding the pipe and delaying input reads.
    if (r == s_led_r && g == s_led_g && b == s_led_b) return;
    s_led_r = r; s_led_g = g; s_led_b = b;

    ds_set_lightbar(r, g, b);
    ds_update_output();
}

// ---------------------------------------------------------------------------
// Adaptive triggers (DualSense only)
// ---------------------------------------------------------------------------

// Track current mode to avoid spamming HID output every frame.
enum TriggerMode {
    TRIGGER_MODE_NONE,      // triggers free (no effect)
    TRIGGER_MODE_AIM_GUN,   // weapon click on R2 (shooting)
    TRIGGER_MODE_CAR,       // machine feel (R2=gas, L2=brake)
};
static TriggerMode s_trigger_mode = TRIGGER_MODE_NONE;

// Weapon25 params for the most recently activated AIM_GUN profile. Looked
// up from the weapon_feel profile at apply time so per-weapon tuning flows
// through without touching this state machine.
static uint8_t s_aim_gun_start_zone = 7;
static uint8_t s_aim_gun_amplitude  = 2;

static void apply_trigger_mode(TriggerMode mode)
{
    if (!s_is_dualsense || !ds_is_connected()) return;

    switch (mode) {
    case TRIGGER_MODE_NONE:
        ds_trigger_off(2); // both hands
        break;

    case TRIGGER_MODE_AIM_GUN:
        // First-person with gun drawn:
        // R2 = weapon trigger (click point then resistance) — feels like pulling a trigger.
        // L2 = free (no effect).
        // Weapon25 StartZone and Amplitude are 4-bit fields (0..15); values
        // outside that range overflow inside GamepadCore's Weapon25 encoder.
        // Both values come from the active weapon's WeaponFeelProfile.
        ds_trigger_weapon(s_aim_gun_start_zone, s_aim_gun_amplitude, 0, 0, 1);
        ds_trigger_off(0);                      // L2: free
        break;

    case TRIGGER_MODE_CAR:
        // Vehicle pedals. R2 = gas: free (analog throttle, no resistance).
        // L2 = brake: strong resistance (no click, just progressive force).
        ds_trigger_off(1);                     // R2 gas: free
        ds_trigger_resistance(20, 200, 0);     // L2 brake: strong resistance, no click
        break;
    }

    ds_update_output();
}

void gamepad_triggers_update(bool in_car, bool weapon_ready, int32_t current_weapon)
{
    if (!s_is_dualsense || !ds_is_connected()) return;
    if (active_input_device != INPUT_DEVICE_DUALSENSE) {
        // Not actively using DualSense — clear triggers if they were on.
        if (s_trigger_mode != TRIGGER_MODE_NONE) {
            s_trigger_mode = TRIGGER_MODE_NONE;
            apply_trigger_mode(TRIGGER_MODE_NONE);
        }
        return;
    }

    // Refresh AIM_GUN params from the current weapon's profile every frame.
    // Cheap lookup, lets a weapon swap mid-game update the click feel on the
    // very next transition without any extra plumbing.
    const WeaponFeelProfile* profile = weapon_feel_get_profile(current_weapon);
    s_aim_gun_start_zone = profile->trigger_start_zone;
    s_aim_gun_amplitude  = profile->trigger_amplitude;
    // Profiles with zero amplitude (or start_zone) opt out of the adaptive
    // click entirely — treat as "weapon has no trigger effect".
    const bool weapon_has_click = profile->trigger_amplitude != 0;

    // AIM_GUN is enabled whenever the weapon is ready to fire — there is no
    // R2-position gate. Previously we gated re-entry on "R2 has dipped into
    // the safe zone below click point" to avoid a phantom click when the
    // effect activated while the trigger was already past threshold; in
    // practice that gate caused a worse bug:
    //
    //   HOLD → cooldown expires → keep holding → release fast → press fast
    //
    // The brief release dip was caught by the per-frame poll, the game-side
    // shot logic armed and fired on the re-press, but the HID command to
    // re-enable AIM_GUN needed one BT round-trip (~7-30ms) to reach the
    // controller — by which time the physical trigger was already past
    // StartZone again. The hardware only fires the click on a rising
    // crossing AFTER the effect is active, so the player felt a shot
    // without a click. Keeping AIM_GUN continuously enabled while the
    // weapon is ready eliminates the HID latency race entirely — the
    // effect is already live when the user releases and re-presses.
    TriggerMode desired;
    if (in_car) {
        desired = TRIGGER_MODE_CAR;
    } else if (weapon_ready && weapon_has_click) {
        desired = TRIGGER_MODE_AIM_GUN;
    } else {
        desired = TRIGGER_MODE_NONE;
    }

    if (desired != s_trigger_mode) {
        s_trigger_mode = desired;
        apply_trigger_mode(desired);
    }
}

void gamepad_triggers_off()
{
    if (s_trigger_mode == TRIGGER_MODE_NONE) return;
    s_trigger_mode = TRIGGER_MODE_NONE;
    apply_trigger_mode(TRIGGER_MODE_NONE);
}

void gamepad_triggers_lockout(int /*duration_ms*/)
{
    // Immediately switch to NONE on a fire event so the HID mode change
    // propagates out on the same frame the game registered the shot. The
    // actual lockout duration is driven by the game's Timer1 cooldown via
    // gamepad_triggers_update(weapon_ready) — once Timer1 reaches 0, the
    // next triggers_update re-enables AIM_GUN unconditionally.
    if (s_trigger_mode != TRIGGER_MODE_NONE) {
        s_trigger_mode = TRIGGER_MODE_NONE;
        apply_trigger_mode(TRIGGER_MODE_NONE);
    }
}

InputDeviceType gamepad_get_device_type()
{
    return active_input_device;
}

void gamepad_consume_until_released(uint8_t button_index)
{
    if (button_index < 32) {
        s_consume_mask |= (1u << button_index);
    }
}
