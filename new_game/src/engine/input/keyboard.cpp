// Keyboard input handling via Windows keyboard hook.
// Tracks key states, modifier flags, and handles same-frame press/release latching.

#include <windows.h>
#include <string.h>

#include "engine/input/keyboard.h"

// uc_orig: KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);

// uc_orig: SetupKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
BOOL SetupKeyboard(void)
{
    AltFlag = 0;
    ControlFlag = 0;
    ShiftFlag = 0;
    LastKey = 0;
    memset((char*)&Keys[0], 0, 256);
    memset((char*)&key_turn[0], 0, 256);

    KeyboardHook = NULL;
    return UC_TRUE;
}

// uc_orig: ResetKeyboard (fallen/DDLibrary/Source/GKeyboard.cpp)
void ResetKeyboard(void)
{
    if (KeyboardHook)
        UnhookWindowsHookEx(KeyboardHook);
}

// uc_orig: KEYMASK_REPEAT (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_REPEAT (0x0000ffff)
// uc_orig: KEYMASK_SCAN (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_SCAN (0x00ff0000)
// uc_orig: KEYMASK_EXTENDED (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_EXTENDED (0x01000000)
// uc_orig: KEYMASK_RESERVED (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_RESERVED (0x1e000000)
// uc_orig: KEYMASK_CONTEXT (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_CONTEXT (0x20000000)
// uc_orig: KEYMASK_PSTATE (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_PSTATE (0x40000000)
// uc_orig: KEYMASK_TSTATE (fallen/DDLibrary/Source/GKeyboard.cpp)
#define KEYMASK_TSTATE (0x80000000)

// uc_orig: SetFlagsFromKeyArray (fallen/DDLibrary/Source/GKeyboard.cpp)
inline void SetFlagsFromKeyArray()
{
    AltFlag = Keys[KB_LALT] || Keys[KB_RALT];
    ControlFlag = Keys[KB_LCONTROL];
    ShiftFlag = Keys[KB_LSHIFT] || Keys[KB_RSHIFT];
}

// uc_orig: KeyboardProc (fallen/DDLibrary/Source/GKeyboard.cpp)
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam)
{
    UBYTE key_code;

    if (code < 0) {
        return CallNextHookEx(KeyboardHook, code, wParam, lParam);
    }

    key_code = (UBYTE)((lParam & KEYMASK_SCAN) >> 16);

    if (lParam & KEYMASK_EXTENDED) {
        key_code += 0x80;
    }

    if (lParam & KEYMASK_TSTATE) {

        if (key_turn[key_code] == game_turn && release_count < MAX_RELEASE) {
            Released[release_count++] = key_code;
        } else

        {
            Keys[key_code] = 0;
        }
    } else {
        key_turn[key_code] = game_turn;
        Keys[key_code] = 1;
        LastKey = key_code;
    }

    SetFlagsFromKeyArray();

    return UC_FALSE;
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
