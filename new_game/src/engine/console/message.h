#ifndef ENGINE_CONSOLE_MESSAGE_H
#define ENGINE_CONSOLE_MESSAGE_H

// On-screen debug message overlay.
// MSG_add queues a formatted message; MSG_draw renders all active messages to screen.
// Output is gated by allow_debug_keys (can be toggled in-game).

// uc_orig: MSG_add (fallen/DDEngine/Source/Message.cpp)
void MSG_add(char* fmt, ...);
// uc_orig: MSG_draw (fallen/DDEngine/Source/Message.cpp)
void MSG_draw(void);

#endif // ENGINE_CONSOLE_MESSAGE_H
