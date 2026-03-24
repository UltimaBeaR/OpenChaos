#ifndef ENGINE_CONSOLE_MESSAGE_GLOBALS_H
#define ENGINE_CONSOLE_MESSAGE_GLOBALS_H

#include "engine/core/types.h"

// uc_orig: MSG_MAX_LENGTH (fallen/DDEngine/Source/Message.cpp)
#define MSG_MAX_LENGTH 256
// uc_orig: MSG_MAX_MESSAGES (fallen/DDEngine/Source/Message.cpp)
#define MSG_MAX_MESSAGES 1000

// uc_orig: MSG_Message (fallen/DDEngine/Source/Message.cpp)
typedef struct {
    SLONG timer;
    CBYTE message[MSG_MAX_LENGTH];
} MSG_Message;

// uc_orig: MSG_message (fallen/DDEngine/Source/Message.cpp)
extern MSG_Message MSG_message[MSG_MAX_MESSAGES];
// uc_orig: current_message (fallen/DDEngine/Source/Message.cpp)
extern SLONG current_message;
// uc_orig: message_count (fallen/DDEngine/Source/Message.cpp)
extern SLONG message_count;
// uc_orig: draw_message_offset (fallen/DDEngine/Source/Message.cpp)
extern SLONG draw_message_offset;

#endif // ENGINE_CONSOLE_MESSAGE_GLOBALS_H
