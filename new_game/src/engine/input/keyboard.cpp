// Keyboard input handling.
// Tracks key states, modifier flags, and handles same-frame press/release latching.

#include <string.h>

#include "engine/input/keyboard.h"

// uc_orig: SetFlagsFromKeyArray (fallen/DDLibrary/Source/GKeyboard.cpp)
static void SetFlagsFromKeyArray()
{
    AltFlag = Keys[KB_LALT] || Keys[KB_RALT];
    ControlFlag = Keys[KB_LCONTROL];
    ShiftFlag = Keys[KB_LSHIFT] || Keys[KB_RSHIFT];
}

// uc_orig: SetupKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
BOOL SetupKeyboard(void)
{
    AltFlag = 0;
    ControlFlag = 0;
    ShiftFlag = 0;
    LastKey = 0;
    memset((char*)&Keys[0], 0, 256);
    memset((char*)&key_turn[0], 0, 256);

    return UC_TRUE;
}

// uc_orig: ResetKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
void ResetKeyboard(void)
{
    // No-op: Win32 keyboard hook removed.
}

// Called from SDL3 event loop when a key is pressed.
// scancode is a KB_* scancode (Set 1, extended keys have 0x80 added).
// uc_orig: derived from KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
void keyboard_key_down(UBYTE scancode)
{
    key_turn[scancode] = game_turn;
    Keys[scancode] = 1;
    LastKey = scancode;

    SetFlagsFromKeyArray();
}

// Called from SDL3 event loop when a key is released.
// uc_orig: derived from KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
void keyboard_key_up(UBYTE scancode)
{
    if (key_turn[scancode] == game_turn && release_count < MAX_RELEASE) {
        // Key pressed and released in the same game turn — latch the release.
        Released[release_count++] = scancode;
    } else {
        Keys[scancode] = 0;
    }

    SetFlagsFromKeyArray();
}

// uc_orig: ClearLatchedKeys (fallen/DDLibrary/Source/GKeyboard.cpp)
void ClearLatchedKeys()
{
    game_turn++;

    while (release_count) {
        release_count--;
        Keys[Released[release_count]] = 0;
    }

    SetFlagsFromKeyArray();
}
