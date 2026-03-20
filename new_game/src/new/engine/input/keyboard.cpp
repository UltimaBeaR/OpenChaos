// Keyboard input handling via Windows keyboard hook.
// Tracks key states, modifier flags, and handles same-frame press/release latching.

#include <windows.h>
#include <string.h>

#include "engine/input/keyboard.h"

// uc_orig: AltFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
volatile UBYTE AltFlag,
// uc_orig: ControlFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ControlFlag,
// uc_orig: ShiftFlag (fallen/DDLibrary/Source/GKeyboard.cpp)
    ShiftFlag;
// uc_orig: Keys (fallen/DDLibrary/Source/GKeyboard.cpp)
volatile UBYTE Keys[256],
// uc_orig: LastKey (fallen/DDLibrary/Source/GKeyboard.cpp)
    LastKey;

// uc_orig: key_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE key_turn[256];

// uc_orig: MAX_RELEASE (fallen/DDLibrary/Source/GKeyboard.cpp)
#define MAX_RELEASE 10
// uc_orig: Released (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE Released[MAX_RELEASE];
// uc_orig: release_count (fallen/DDLibrary/Source/GKeyboard.cpp)
SWORD release_count = 0;
// uc_orig: game_turn (fallen/DDLibrary/Source/GKeyboard.cpp)
UBYTE game_turn = 0;

// uc_orig: KeyboardHook (fallen/DDLibrary/Source/GKeyboard.cpp)
HHOOK KeyboardHook;

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
    return TRUE;
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
    ULONG virtual_keycode = wParam;

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

    return FALSE;
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
