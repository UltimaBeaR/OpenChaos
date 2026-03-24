#ifndef ENGINE_GRAPHICS_TEXT_FONT_GLOBALS_H
#define ENGINE_GRAPHICS_TEXT_FONT_GLOBALS_H

#include "engine/core/types.h"
#include "engine/graphics/text/font.h"

// Internal character bitmap type. Defined here (not in font.h) because font.h
// is the public API — this type is only needed by globals and font.cpp.
// uc_orig: FONT_Char (fallen/DDEngine/Source/Font.cpp)
struct FONT_Char {
    UBYTE bit[FONT_HEIGHT];
    UBYTE width;
};

// Character bitmap tables — one entry per character.
// uc_orig: FONT_upper (fallen/DDEngine/Source/Font.cpp)
extern FONT_Char FONT_upper[26];
// uc_orig: FONT_lower (fallen/DDEngine/Source/Font.cpp)
extern FONT_Char FONT_lower[26];
// uc_orig: FONT_number (fallen/DDEngine/Source/Font.cpp)
extern FONT_Char FONT_number[10];

// Punctuation index constants (used by font.cpp when dispatching characters).
// uc_orig: FONT_PUNCT_DOT (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_DOT     0
// uc_orig: FONT_PUNCT_COMMA (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_COMMA   1
// uc_orig: FONT_PUNCT_QMARK (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_QMARK   2
// uc_orig: FONT_PUNCT_PLING (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_PLING   3
// uc_orig: FONT_PUNCT_QUOTES (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_QUOTES  4
// uc_orig: FONT_PUNCT_OPEN (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_OPEN    5
// uc_orig: FONT_PUNCT_CLOSE (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_CLOSE   6
// uc_orig: FONT_PUNCT_PLUS (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_PLUS    7
// uc_orig: FONT_PUNCT_MINUS (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_MINUS   8
// uc_orig: FONT_PUNCT_EQUAL (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_EQUAL   9
// uc_orig: FONT_PUNCT_HASH (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_HASH    10
// uc_orig: FONT_PUNCT_PCENT (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_PCENT   11
// uc_orig: FONT_PUNCT_STAR (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_STAR    12
// uc_orig: FONT_PUNCT_BSLASH (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_BSLASH  13
// uc_orig: FONT_PUNCT_FSLASH (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_FSLASH  14
// uc_orig: FONT_PUNCT_COLON (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_COLON   15
// uc_orig: FONT_PUNCT_SCOLON (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_SCOLON  16
// uc_orig: FONT_PUNCT_APOST (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_APOST   17
// uc_orig: FONT_PUNCT_AMPER (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_AMPER   18
// uc_orig: FONT_PUNCT_POUND (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_POUND   19
// uc_orig: FONT_PUNCT_DOLLAR (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_DOLLAR  20
// uc_orig: FONT_PUNCT_LT (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_LT      21
// uc_orig: FONT_PUNCT_GT (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_GT      22
// uc_orig: FONT_PUNCT_AT (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_AT      23
// uc_orig: FONT_PUNCT_UNDER (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_UNDER   24

// Total punctuation entries.
// uc_orig: FONT_PUNCT_NUMBER (fallen/DDEngine/Source/Font.cpp)
#define FONT_PUNCT_NUMBER 25
// uc_orig: FONT_punct (fallen/DDEngine/Source/Font.cpp)
extern FONT_Char FONT_punct[FONT_PUNCT_NUMBER];

// Message queue for deferred rendering (filled by FONT_buffer_add, flushed by FONT_buffer_draw).
// uc_orig: FONT_BUFFER_SIZE (fallen/DDEngine/Source/Font.cpp)
#define FONT_BUFFER_SIZE (1024 * 8)
// uc_orig: FONT_MAX_MESSAGES (fallen/DDEngine/Source/Font.cpp)
#define FONT_MAX_MESSAGES 256

// uc_orig: FONT_buffer (fallen/DDEngine/Source/Font.cpp)
extern CBYTE FONT_buffer[FONT_BUFFER_SIZE];
// uc_orig: FONT_buffer_upto (fallen/DDEngine/Source/Font.cpp)
extern CBYTE* FONT_buffer_upto;

// Internal message record for the deferred queue.
// uc_orig: FONT_Message (fallen/DDEngine/Source/Font.cpp)
struct FONT_Message {
    SLONG x;
    SLONG y;
    UBYTE r;
    UBYTE g;
    UBYTE b;
    UBYTE s;
    CBYTE* m;
};

// uc_orig: FONT_message (fallen/DDEngine/Source/Font.cpp)
extern FONT_Message FONT_message[FONT_MAX_MESSAGES];
// uc_orig: FONT_message_upto (fallen/DDEngine/Source/Font.cpp)
extern SLONG FONT_message_upto;

#endif // ENGINE_GRAPHICS_TEXT_FONT_GLOBALS_H
