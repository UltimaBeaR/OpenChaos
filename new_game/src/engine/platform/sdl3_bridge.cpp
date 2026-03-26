// SDL3 bridge — the ONLY translation unit that includes SDL3 headers.
// Compiled with /Zp8 (see CMakeLists.txt set_source_files_properties).
// All SDL3 interactions in the project go through functions declared in sdl3_bridge.h.

#include "engine/platform/sdl3_bridge.h"
#include <SDL3/SDL.h>

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

    // Pump events to update internal SDL state (axes, buttons) without consuming
    // non-gamepad events from the queue.
    SDL_PumpEvents();

    // Pull only gamepad connect/disconnect events, leave everything else.
    SDL_Event e;
    while (SDL_PeepEvents(&e, 1, SDL_GETEVENT,
               SDL_EVENT_GAMEPAD_ADDED, SDL_EVENT_GAMEPAD_REMOVED) > 0) {
        if (e.type == SDL_EVENT_GAMEPAD_ADDED) {
            event->type = SDL3_GAMEPAD_EVENT_ADDED;
            return true;
        }
        if (e.type == SDL_EVENT_GAMEPAD_REMOVED) {
            event->type = SDL3_GAMEPAD_EVENT_REMOVED;
            return true;
        }
    }
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
