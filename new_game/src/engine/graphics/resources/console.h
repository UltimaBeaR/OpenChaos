#ifndef ENGINE_GRAPHICS_RESOURCES_CONSOLE_H
#define ENGINE_GRAPHICS_RESOURCES_CONSOLE_H

#include "engine/core/types.h"

// Sets the font used for console output.
// uc_orig: CONSOLE_font (fallen/DDEngine/Headers/console.h)
void CONSOLE_font(CBYTE* fontpath, float scale = 1.0f);

// Renders all active console messages to the screen.
// uc_orig: CONSOLE_draw (fallen/DDEngine/Headers/console.h)
void CONSOLE_draw(void);

// Queues a message to display for 'delay' milliseconds (default 4000ms).
// uc_orig: CONSOLE_text (fallen/DDEngine/Headers/console.h)
void CONSOLE_text(CBYTE* text, SLONG delay = 4000);

// Clears all queued messages.
// uc_orig: CONSOLE_clear (fallen/DDEngine/Headers/console.h)
void CONSOLE_clear(void);

// Removes the oldest expired message from the queue.
// uc_orig: CONSOLE_scroll (fallen/DDEngine/Headers/console.h)
void CONSOLE_scroll(void);

// Displays a message at a fixed screen position (x,y) for 'delay' milliseconds.
// If a message already exists at that position, it is replaced.
// uc_orig: CONSOLE_text_at (fallen/DDEngine/Headers/console.h)
void CONSOLE_text_at(SLONG x, SLONG y, SLONG delay, CBYTE* fmt, ...);

// Sets a persistent status line drawn in the top-left corner.
// Not declared in original console.h; called externally via forward declaration in Controls.cpp.
// uc_orig: CONSOLE_status (fallen/DDEngine/Source/console.cpp)
void CONSOLE_status(CBYTE* msg);

#endif // ENGINE_GRAPHICS_RESOURCES_CONSOLE_H
