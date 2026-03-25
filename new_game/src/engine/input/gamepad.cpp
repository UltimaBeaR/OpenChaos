// Gamepad abstraction — SDL3 backend (via sdl3_bridge) + Dualsense-Multiplatform stub.
// Translates SDL3 axes (-32768..+32767) to DirectInput range (0..65535) so downstream
// code (deadzone, analog packing) works unchanged.

#include "engine/input/gamepad.h"
#include "engine/input/gamepad_globals.h"
#include "engine/platform/sdl3_bridge.h"
#include <cstring>

// Sony vendor/product IDs for DualSense detection.
static constexpr uint16_t SONY_VENDOR_ID       = 0x054C;
static constexpr uint16_t DUALSENSE_PRODUCT_ID = 0x0CE6;
static constexpr uint16_t DUALSENSE_EDGE_PRODUCT_ID = 0x0DF2;

static SDL3_GamepadHandle s_gamepad = nullptr;
static bool s_is_dualsense = false;

static bool is_dualsense(SDL3_GamepadHandle handle)
{
    uint16_t vid = sdl3_gamepad_vendor_id(handle);
    uint16_t pid = sdl3_gamepad_product_id(handle);
    return vid == SONY_VENDOR_ID &&
           (pid == DUALSENSE_PRODUCT_ID || pid == DUALSENSE_EDGE_PRODUCT_ID);
}

void gamepad_init()
{
    sdl3_gamepad_init();

    s_gamepad = sdl3_gamepad_open();
    if (s_gamepad) {
        s_is_dualsense = is_dualsense(s_gamepad);
        active_input_device = s_is_dualsense ? INPUT_DEVICE_DUALSENSE : INPUT_DEVICE_XBOX;
        gamepad_state.connected = true;
    }
}

void gamepad_shutdown()
{
    if (s_gamepad) {
        sdl3_gamepad_close(s_gamepad);
        s_gamepad = nullptr;
    }
    sdl3_gamepad_shutdown();
}

void gamepad_poll()
{
    // Process connect/disconnect events.
    SDL3_GamepadEvent event;
    while (sdl3_gamepad_poll_event(&event)) {
        if (event.type == SDL3_GAMEPAD_EVENT_ADDED && !s_gamepad) {
            s_gamepad = sdl3_gamepad_open();
            if (s_gamepad) {
                s_is_dualsense = is_dualsense(s_gamepad);
                active_input_device = s_is_dualsense ? INPUT_DEVICE_DUALSENSE : INPUT_DEVICE_XBOX;
                gamepad_state.connected = true;
            }
        } else if (event.type == SDL3_GAMEPAD_EVENT_REMOVED && s_gamepad) {
            sdl3_gamepad_close(s_gamepad);
            s_gamepad = nullptr;
            s_is_dualsense = false;
            gamepad_state.connected = false;
            memset(&gamepad_state, 0, sizeof(gamepad_state));
            active_input_device = INPUT_DEVICE_KEYBOARD_MOUSE;
        }
    }

    if (!s_gamepad) {
        return;
    }

    // TODO: DualSense via Dualsense-Multiplatform (Iteration B).
    // For now, all controllers go through SDL3.

    SDL3_GamepadState sdl_state;
    if (!sdl3_gamepad_get_state(s_gamepad, &sdl_state)) {
        return;
    }

    // Translate SDL3 axes (-32768..+32767) to DI range (0..65535).
    gamepad_state.lX = static_cast<int32_t>(sdl_state.axis_left_x) + 32768;
    gamepad_state.lY = static_cast<int32_t>(sdl_state.axis_left_y) + 32768;

    // Map SDL3 buttons to rgbButtons[] (0x80 = pressed).
    // Button indices match the joypad_button_use[] mapping in input_actions.cpp.
    memset(gamepad_state.rgbButtons, 0, sizeof(gamepad_state.rgbButtons));

    uint32_t btns = sdl_state.buttons;
    // SDL3 gamepad button order → rgbButtons index.
    // Index 0-14 map directly to SDL_GamepadButton enum order.
    for (int i = 0; i < 15; i++) {
        if (btns & (1u << i)) {
            gamepad_state.rgbButtons[i] = 0x80;
        }
    }

    // Triggers as digital buttons (for legacy code that reads rgbButtons).
    // Use indices 15/16 which are unused by face buttons.
    if (sdl_state.trigger_left > 8000) {
        gamepad_state.rgbButtons[15] = 0x80;
    }
    if (sdl_state.trigger_right > 8000) {
        gamepad_state.rgbButtons[16] = 0x80;
    }

    gamepad_state.connected = true;

    // Track that gamepad is the active input device.
    // (Keyboard detection is done elsewhere — here we just flag gamepad activity.)
    if (btns || sdl_state.trigger_left > 8000 || sdl_state.trigger_right > 8000 ||
        sdl_state.axis_left_x < -8000 || sdl_state.axis_left_x > 8000 ||
        sdl_state.axis_left_y < -8000 || sdl_state.axis_left_y > 8000) {
        active_input_device = s_is_dualsense ? INPUT_DEVICE_DUALSENSE : INPUT_DEVICE_XBOX;
    }
}

void gamepad_rumble(uint16_t low_freq, uint16_t high_freq, uint32_t duration_ms)
{
    if (s_gamepad) {
        sdl3_gamepad_rumble(s_gamepad, low_freq, high_freq, duration_ms);
    }
}

InputDeviceType gamepad_get_device_type()
{
    return active_input_device;
}
