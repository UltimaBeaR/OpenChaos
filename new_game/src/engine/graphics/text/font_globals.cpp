#include "engine/graphics/text/font_globals.h"

// Bit-pattern macros for the 5-wide bitmap font glyphs.
// Each row of a glyph is encoded as a 5-bit value (bits 4..0 = columns left..right).
// uc_orig: _____ (fallen/DDEngine/Source/Font.cpp)
#define _____ 0x00
// uc_orig: ____x (fallen/DDEngine/Source/Font.cpp)
#define ____x 0x01
// uc_orig: ___x_ (fallen/DDEngine/Source/Font.cpp)
#define ___x_ 0x02
// uc_orig: ___xx (fallen/DDEngine/Source/Font.cpp)
#define ___xx 0x03
// uc_orig: __x__ (fallen/DDEngine/Source/Font.cpp)
#define __x__ 0x04
// uc_orig: __x_x (fallen/DDEngine/Source/Font.cpp)
#define __x_x 0x05
// uc_orig: __xx_ (fallen/DDEngine/Source/Font.cpp)
#define __xx_ 0x06
// uc_orig: __xxx (fallen/DDEngine/Source/Font.cpp)
#define __xxx 0x07
// uc_orig: _x___ (fallen/DDEngine/Source/Font.cpp)
#define _x___ 0x08
// uc_orig: _x__x (fallen/DDEngine/Source/Font.cpp)
#define _x__x 0x09
// uc_orig: _x_x_ (fallen/DDEngine/Source/Font.cpp)
#define _x_x_ 0x0a
// uc_orig: _x_xx (fallen/DDEngine/Source/Font.cpp)
#define _x_xx 0x0b
// uc_orig: _xx__ (fallen/DDEngine/Source/Font.cpp)
#define _xx__ 0x0c
// uc_orig: _xx_x (fallen/DDEngine/Source/Font.cpp)
#define _xx_x 0x0d
// uc_orig: _xxx_ (fallen/DDEngine/Source/Font.cpp)
#define _xxx_ 0x0e
// uc_orig: _xxxx (fallen/DDEngine/Source/Font.cpp)
#define _xxxx 0x0f
// uc_orig: x____ (fallen/DDEngine/Source/Font.cpp)
#define x____ 0x10
// uc_orig: x___x (fallen/DDEngine/Source/Font.cpp)
#define x___x 0x11
// uc_orig: x__x_ (fallen/DDEngine/Source/Font.cpp)
#define x__x_ 0x12
// uc_orig: x__xx (fallen/DDEngine/Source/Font.cpp)
#define x__xx 0x13
// uc_orig: x_x__ (fallen/DDEngine/Source/Font.cpp)
#define x_x__ 0x14
// uc_orig: x_x_x (fallen/DDEngine/Source/Font.cpp)
#define x_x_x 0x15
// uc_orig: x_xx_ (fallen/DDEngine/Source/Font.cpp)
#define x_xx_ 0x16
// uc_orig: x_xxx (fallen/DDEngine/Source/Font.cpp)
#define x_xxx 0x17
// uc_orig: xx___ (fallen/DDEngine/Source/Font.cpp)
#define xx___ 0x18
// uc_orig: xx__x (fallen/DDEngine/Source/Font.cpp)
#define xx__x 0x19
// uc_orig: xx_x_ (fallen/DDEngine/Source/Font.cpp)
#define xx_x_ 0x1a
// uc_orig: xx_xx (fallen/DDEngine/Source/Font.cpp)
#define xx_xx 0x1b
// uc_orig: xxx__ (fallen/DDEngine/Source/Font.cpp)
#define xxx__ 0x1c
// uc_orig: xxx_x (fallen/DDEngine/Source/Font.cpp)
#define xxx_x 0x1d
// uc_orig: xxxx_ (fallen/DDEngine/Source/Font.cpp)
#define xxxx_ 0x1e
// uc_orig: xxxxx (fallen/DDEngine/Source/Font.cpp)
#define xxxxx 0x1f

// uc_orig: FONT_upper (fallen/DDEngine/Source/Font.cpp)
FONT_Char FONT_upper[26] = {
    { { _xx__, x__x_, x__x_, x__x_, xxxx_, x__x_, x__x_, _____, _____ }, 4 }, // A
    { { xxx__, x__x_, x__x_, xxx__, x__x_, x__x_, xxx__, _____, _____ }, 4 }, // B
    { { _xxx_, x____, x____, x____, x____, x____, _xxx_, _____, _____ }, 4 }, // C
    { { xxx__, x__x_, x__x_, x__x_, x__x_, x__x_, xxx__, _____, _____ }, 4 }, // D
    { { xxxx_, x____, x____, xxx__, x____, x____, xxxx_, _____, _____ }, 4 }, // E
    { { xxxx_, x____, x____, xxx__, x____, x____, x____, _____, _____ }, 4 }, // F
    { { _xxx_, x____, x____, x____, x_xx_, x__x_, _xxx_, _____, _____ }, 4 }, // G
    { { x__x_, x__x_, x__x_, xxxx_, x__x_, x__x_, x__x_, _____, _____ }, 4 }, // H
    { { xxx__, _x___, _x___, _x___, _x___, _x___, xxx__, _____, _____ }, 3 }, // I
    { { xxxx_, ___x_, ___x_, ___x_, ___x_, x__x_, _xx__, _____, _____ }, 4 }, // J
    { { x____, x__x_, x_x__, xx___, xx___, x_x__, x__x_, _____, _____ }, 4 }, // K
    { { x____, x____, x____, x____, x____, x____, xxxx_, _____, _____ }, 4 }, // L
    { { x___x, xx_xx, x_x_x, x___x, x___x, x___x, x___x, _____, _____ }, 5 }, // M
    { { x___x, x___x, xx__x, x_x_x, x__xx, x___x, x___x, _____, _____ }, 5 }, // N
    { { _xx__, x__x_, x__x_, x__x_, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // O
    { { xxx__, x__x_, x__x_, x__x_, xxx__, x____, x____, _____, _____ }, 4 }, // P
    { { _xx__, x__x_, x__x_, x__x_, x__x_, x_xx_, _xx__, ___x_, _____ }, 4 }, // Q
    { { xxx__, x__x_, x__x_, x__x_, xxx__, x_x__, x__x_, _____, _____ }, 4 }, // R
    { { _xx__, x__x_, x____, _x___, __x__, x__x_, _xx__, _____, _____ }, 4 }, // S
    { { xxx__, _x___, _x___, _x___, _x___, _x___, _x___, _____, _____ }, 3 }, // T
    { { x__x_, x__x_, x__x_, x__x_, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // U
    { { x__x_, x__x_, x__x_, x__x_, x__x_, _x_x_, __xx_, _____, _____ }, 4 }, // V
    { { x___x, x___x, x___x, x___x, x_x_x, xx_xx, x___x, _____, _____ }, 5 }, // W
    { { x___x, x___x, _x_x_, __x__, _x_x_, x___x, x___x, _____, _____ }, 5 }, // X
    { { x__x_, x__x_, x__x_, _x_x_, __xx_, __x__, xx___, _____, _____ }, 4 }, // Y
    { { xxxx_, ___x_, ___x_, __x__, _x___, x____, xxxx_, _____, _____ }, 4 }, // Z
};

// uc_orig: FONT_lower (fallen/DDEngine/Source/Font.cpp)
FONT_Char FONT_lower[26] = {
    { { _____, _____, _xx__, ___x_, _xxx_, x__x_, _xxx_, _____, _____ }, 4 }, // a
    { { _____, x____, xxx__, x__x_, x__x_, x__x_, xxx__, _____, _____ }, 4 }, // b
    { { _____, _____, _xx__, x____, x____, x____, _xx__, _____, _____ }, 3 }, // c
    { { _____, ___x_, _xxx_, x__x_, x__x_, x__x_, _xxx_, _____, _____ }, 4 }, // d
    { { _____, _____, _xx__, x__x_, xxxx_, x____, _xxx_, _____, _____ }, 4 }, // e
    { { _____, _xx__, x____, x____, xx___, x____, x____, _____, _____ }, 3 }, // f
    { { _____, _____, _xxx_, x__x_, x__x_, x__x_, _xxx_, ___x_, _xx__ }, 4 }, // g
    { { _____, x____, x____, xxx__, x__x_, x__x_, x__x_, _____, _____ }, 4 }, // h
    { { _____, _x___, _____, xx___, _x___, _x___, _x___, _____, _____ }, 2 }, // i
    { { _____, _x___, _____, xx___, _x___, _x___, _x___, _x___, x____ }, 2 }, // j
    { { _____, x____, x____, x____, x_x__, xx___, x_x__, _____, _____ }, 3 }, // k
    { { _____, xx___, _x___, _x___, _x___, _x___, _x___, _____, _____ }, 2 }, // l
    { { _____, _____, xx_x_, x_x_x, x___x, x___x, x___x, _____, _____ }, 5 }, // m
    { { _____, _____, xxx__, x__x_, x__x_, x__x_, x__x_, _____, _____ }, 4 }, // n
    { { _____, _____, _xx__, x__x_, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // o
    { { _____, _____, xxx__, x__x_, x__x_, x__x_, xxx__, x____, x____ }, 4 }, // p
    { { _____, _____, _xxx_, x__x_, x__x_, x__x_, _xxx_, ___xx, ___x_ }, 4 }, // q
    { { _____, _____, x_xx_, xx___, x____, x____, x____, _____, _____ }, 4 }, // r
    { { _____, _____, _xxx_, x____, _xx__, ___x_, xxx__, _____, _____ }, 4 }, // s
    { { _____, x____, x____, xxx__, x____, x____, _xx__, _____, _____ }, 3 }, // t
    { { _____, _____, x__x_, x__x_, x__x_, x__x_, _xxx_, _____, _____ }, 4 }, // u
    { { _____, _____, x__x_, x__x_, x__x_, _x_x_, __xx_, _____, _____ }, 4 }, // v
    { { _____, _____, x___x, x___x, x___x, x_x_x, _x_x_, _____, _____ }, 5 }, // w
    { { _____, _____, x___x, _x_x_, __x__, _x_x_, x___x, _____, _____ }, 5 }, // x
    { { _____, _____, x__x_, x__x_, x__x_, x__x_, _xxx_, ___x_, _xx__ }, 4 }, // y
    { { _____, _____, xxxx_, ___x_, __x__, _x___, xxxx_, _____, _____ }, 4 }, // z
};

// uc_orig: FONT_number (fallen/DDEngine/Source/Font.cpp)
FONT_Char FONT_number[10] = {
    { { _xx__, x__x_, x_xx_, xx_x_, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // 0
    { { _x___, xx___, _x___, _x___, _x___, _x___, xxx__, _____, _____ }, 3 }, // 1
    { { _xx__, x__x_, ___x_, __x__, _x___, x____, xxxx_, _____, _____ }, 4 }, // 2
    { { _xx__, x__x_, ___x_, __x__, ___x_, x__x_, _xx__, _____, _____ }, 4 }, // 3
    { { __x__, _xx__, x_x__, x_x__, xxxx_, __x__, __x__, _____, _____ }, 4 }, // 4
    { { xxxx_, x____, x____, xxx__, ___x_, ___x_, xxx__, _____, _____ }, 4 }, // 5
    { { __x__, _x___, x____, xxx__, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // 6
    { { xxxx_, ___x_, ___x_, __x__, _x___, _x___, _x___, _____, _____ }, 4 }, // 7
    { { _xx__, x__x_, x__x_, _xx__, x__x_, x__x_, _xx__, _____, _____ }, 4 }, // 8
    { { _xx__, x__x_, x__x_, _xxx_, ___x_, ___x_, _xx__, _____, _____ }, 4 }, // 9
};

// uc_orig: FONT_punct (fallen/DDEngine/Source/Font.cpp)
FONT_Char FONT_punct[FONT_PUNCT_NUMBER] = {
    { { _____, _____, _____, _____, _____, _____, x____, _____, _____ }, 1 }, // .
    { { _____, _____, _____, _____, _____, _____, x____, x____, _____ }, 2 }, // ,
    { { _xx__, x__x_, ___x_, _xx__, _x___, _____, _x___, _____, _____ }, 4 }, // ?
    { { x____, x____, x____, x____, x____, _____, x____, _____, _____ }, 1 }, // !
    { { x_x__, x_x__, _____, _____, _____, _____, _____, _____, _____ }, 3 }, // "
    { { _____, _x___, x____, x____, x____, x____, _x___, _____, _____ }, 2 }, // (
    { { _____, x____, _x___, _x___, _x___, _x___, x____, _____, _____ }, 2 }, // )
    { { _____, _____, __x__, __x__, xxxxx, __x__, __x__, _____, _____ }, 5 }, // +
    { { _____, _____, _____, _____, xxxxx, _____, _____, _____, _____ }, 5 }, // -
    { { _____, _____, _____, xxxxx, _____, xxxxx, _____, _____, _____ }, 5 }, // =
    { { _____, _____, _x_x_, xxxxx, _x_x_, xxxxx, _x_x_, _____, _____ }, 5 }, // #
    { { _____, _____, xx__x, x__x_, __x__, _x__x, x__xx, _____, _____ }, 5 }, // %
    { { _____, __x__, x_x_x, _xxx_, xxxxx, _xxx_, x_x_x, __x__, _____ }, 5 }, // *
    { { x____, x____, _x___, _x___, __x__, __x__, ___x_, ___x_, _____ }, 4 }, // '\'
    { { ___x_, ___x_, __x__, __x__, _x___, _x___, x____, x____, _____ }, 4 }, // /
    { { _____, _____, _____, x____, _____, x____, _____, _____, _____ }, 1 }, // :
    { { _____, _____, _____, x____, _____, x____, x____, _____, _____ }, 1 }, // ;
    { { _____, x____, x____, _____, _____, _____, _____, _____, _____ }, 1 }, // '
    { { _xx__, x__x_, x_x__, _x___, x_x_x, x__x_, _xx_x, _____, _____ }, 5 }, // &
    { { _xx__, x__x_, x____, _x___, xxx__, _x___, xxxx_, _____, _____ }, 4 }, // pound
    { { _____, _____, __x__, _xxx_, x_x__, _xxx_, __x_x, _xxx_, __x__ }, 5 }, // $
    { { _____, ___x_, __x__, _x___, x____, _x___, __x__, ___x_, _____ }, 4 }, // <
    { { _____, x____, _x___, __x__, ___x_, __x__, _x___, x____, _____ }, 4 }, // >
    { { _____, _____, _xxx_, x___x, x_xxx, x_xx_, x____, _xxxx, _____ }, 5 }, // @
    { { _____, _____, _____, _____, _____, _____, _____, xxxx_, _____ }, 4 }, // _
};

// uc_orig: FONT_buffer (fallen/DDEngine/Source/Font.cpp)
CBYTE FONT_buffer[FONT_BUFFER_SIZE];
// uc_orig: FONT_buffer_upto (fallen/DDEngine/Source/Font.cpp)
CBYTE* FONT_buffer_upto = nullptr;

// uc_orig: FONT_message (fallen/DDEngine/Source/Font.cpp)
FONT_Message FONT_message[FONT_MAX_MESSAGES];
// uc_orig: FONT_message_upto (fallen/DDEngine/Source/Font.cpp)
SLONG FONT_message_upto = 0;
