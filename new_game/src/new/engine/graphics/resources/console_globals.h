#ifndef ENGINE_GRAPHICS_RESOURCES_CONSOLE_GLOBALS_H
#define ENGINE_GRAPHICS_RESOURCES_CONSOLE_GLOBALS_H

#include <windows.h>
#include "engine/graphics/resources/console.h"

// Maximum number of simultaneously active positioned messages.
// uc_orig: CONSOLE_MAX_MESSES (fallen/DDEngine/Source/console.cpp)
#define CONSOLE_MAX_MESSES 16

// Number of scrolling message lines.
// uc_orig: CONSOLE_LINES (fallen/DDEngine/Source/console.cpp)
#define CONSOLE_LINES 15

// Maximum characters per console line.
// uc_orig: CONSOLE_WIDTH (fallen/DDEngine/Source/console.cpp)
#define CONSOLE_WIDTH 50

// A single scrolling console line.
// uc_orig: ConsoleText (fallen/DDEngine/Source/console.cpp)
struct ConsoleText {
    CBYTE Text[CONSOLE_WIDTH];
    SLONG Age;
};

// A positioned on-screen message with a countdown timer.
// uc_orig: CONSOLE_Mess (fallen/DDEngine/Source/console.cpp)
struct CONSOLE_Mess {
    SLONG delay; // Milliseconds remaining (0 = inactive).
    SLONG x;
    SLONG y;
    CBYTE mess[CONSOLE_WIDTH];
};

// uc_orig: CONSOLE_mess (fallen/DDEngine/Source/console.cpp)
extern CONSOLE_Mess CONSOLE_mess[];

// Scrolling text buffer (CONSOLE_LINES entries, line 0 is oldest).
// uc_orig: Data (fallen/DDEngine/Source/console.cpp)
extern ConsoleText console_Data[CONSOLE_LINES];

// Persistent status line shown in top-left corner.
// uc_orig: status_text (fallen/DDEngine/Source/console.cpp)
extern CBYTE console_status_text[_MAX_PATH];

// Frame timestamps for per-frame elapsed time calculation.
// uc_orig: last_tick (fallen/DDEngine/Source/console.cpp)
extern SLONG console_last_tick;
// uc_orig: this_tick (fallen/DDEngine/Source/console.cpp)
extern SLONG console_this_tick;

#endif // ENGINE_GRAPHICS_RESOURCES_CONSOLE_GLOBALS_H
