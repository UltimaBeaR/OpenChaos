#include "ui/input_glyphs/input_prompt_map.h"

// ===========================================================================
// EDIT THIS TABLE. Each {col, row} is a 0-based cell in that device's atlas PNG
// (uniform 64px grid). Every glyph cell below is a {0,0} PLACEHOLDER — replace it
// with the real cell counted off the PNG. `NA` = the input has no glyph on that
// device. View the result live via the "INPUT TEST" help topic (it changes with
// the active device and lists every entry as "name - glyph").
// ===========================================================================

// Shorthand for "not present on this device" (do not edit these).
#define NA { INPUT_PROMPT_NONE, INPUT_PROMPT_NONE }

const InputPrompt INPUT_PROMPTS[] = {
    // ---- Mouse (keyboard&mouse atlas) ----
    { "Left mouse button",        {3,1}, NA, NA },
    { "Middle mouse button",      {9,1}, NA, NA },
    { "Right mouse button",       {7,1}, NA, NA },
    { "Mouse wheel",              {15,1}, NA, NA },
    { "Mouse wheel up",           {13,1}, NA, NA },
    { "Mouse wheel down",         {10,1}, NA, NA },
    { "Mouse move",               {5,1}, NA, NA },
    { "Mouse move (horizontal)",  {2,1}, NA, NA },
    { "Mouse move (vertical)",    {2,0}, NA, NA },

    // ---- Keyboard letters ---- (filled, non-outline; derived from metadata)
    { "A", {5,14}, NA, NA },  { "B", {6,12}, NA, NA },  { "C", {6,11}, NA, NA },
    { "D", {6,10}, NA, NA },  { "E", {10,10}, NA, NA }, { "F", {6,9}, NA, NA },
    { "G", {2,7}, NA, NA },   { "H", {4,7}, NA, NA },   { "I", {8,7}, NA, NA },
    { "J", {12,7}, NA, NA },  { "K", {14,7}, NA, NA },  { "L", {0,6}, NA, NA },
    { "M", {2,6}, NA, NA },   { "N", {6,6}, NA, NA },   { "O", {14,6}, NA, NA },
    { "P", {3,5}, NA, NA },   { "Q", {15,5}, NA, NA },  { "R", {5,4}, NA, NA },
    { "S", {9,4}, NA, NA },   { "T", {9,3}, NA, NA },   { "U", {3,2}, NA, NA },
    { "V", {5,2}, NA, NA },   { "W", {7,2}, NA, NA },   { "X", {11,2}, NA, NA },
    { "Y", {13,2}, NA, NA },  { "Z", {15,2}, NA, NA },

    // ---- Keyboard digits ----
    { "0", {1,15}, NA, NA },  { "1", {3,15}, NA, NA },  { "2", {5,15}, NA, NA },
    { "3", {7,15}, NA, NA },  { "4", {9,15}, NA, NA },  { "5", {11,15}, NA, NA },
    { "6", {13,15}, NA, NA }, { "7", {15,15}, NA, NA }, { "8", {1,14}, NA, NA },
    { "9", {3,14}, NA, NA },

    // ---- Keyboard symbols (main block) ----
    { "Backtick `",   {1,2},   NA, NA }, // keyboard_tilde
    { "Minus -",      {4,6},   NA, NA },
    { "Equals =",     {0,9},   NA, NA },
    { "Bracket [",    {4,11},  NA, NA }, // keyboard_bracket_open
    { "Bracket ]",    {14,12}, NA, NA }, // keyboard_bracket_close
    { "Backslash \\", {1,3},   NA, NA }, // keyboard_slash_back
    { "Semicolon ;",  {11,4},  NA, NA },
    { "Apostrophe '", {11,14}, NA, NA }, // keyboard_apostrophe
    { "Comma ,",      {0,10},  NA, NA },
    { "Period .",     {9,5},   NA, NA },
    { "Slash /",      {3,3},   NA, NA }, // keyboard_slash_forward

    // ---- Keyboard special (main block) ---- (text variants, not the _icon ones)
    { "Tab",       {12,3},  NA, NA },
    { "Caps Lock", {8,11},  NA, NA },
    { "Shift",     {14,4},  NA, NA },
    { "Ctrl",      {4,10},  NA, NA },
    { "Alt",       {7,14},  NA, NA },
    { "Space",     {6,3},   NA, NA },
    { "Enter",     {7,4}, NA, NA },
    { "Backspace", {9,12},  NA, NA },

    // ---- Gamepad face buttons (xbox / ps) ----
    { "Face down (A / Cross)",      NA, {4,9}, {5,10} },
    { "Face right (B / Circle)",    NA, {6,9}, {7,11} },
    { "Face left (X / Square)",     NA, {0,6}, {11,10} },
    { "Face up (Y / Triangle)",     NA, {2,6}, {1,9} },

    // ---- Gamepad shoulders / triggers ----
    { "Left bumper (LB / L1)",      NA, {7,3}, {2,6} },
    { "Right bumper (RB / R1)",     NA, {3,2}, {10,6} },
    { "Left trigger (LT / L2)",     NA, {1,2}, {6,6} },
    { "Right trigger (RT / R2)",    NA, {7,2}, {2,5} },

    // ---- Gamepad D-pad ----
    { "D-pad",        NA, {4,6}, {3,9} },
    { "D-pad up",     NA, {3,4}, {2,8} },
    { "D-pad down",   NA, {6,6}, {5,9} },
    { "D-pad left",   NA, {0,5}, {9,9} },
    { "D-pad right",  NA, {3,5}, {0,8} },

    // ---- Gamepad left stick ---- (press = stick_l_press; LS/L3 label is the alt)
    { "Left stick",                 NA, {9,2}, {6,8} },
    { "Left stick (horizontal)",    NA, {1,1}, {8,8} },
    { "Left stick (vertical)",      NA, {6,1}, {1,7} },
    { "Left stick press (LS / L3)", NA, {5,0}, {10,7} },

    // ---- Gamepad right stick ----
    { "Right stick",                 NA, {7,1}, {2,7} },
    { "Right stick (horizontal)",    NA, {9,1}, {4,7} },
    { "Right stick (vertical)",      NA, {4,0}, {9,7} },
    { "Right stick press (RS / R3)", NA, {6,0}, {11,7} },

    // ---- Gamepad menu buttons ---- (PS has no Options/Share in metadata — fill
    // those two ps cells from the PNG by hand; xbox menu/view are below)
    { "Start (Menu / Options)",  NA, {0,7}, {11,3} },
    { "Select (View / Share)",   NA, {8,7}, {5,3} },
};

#undef NA

const int INPUT_PROMPT_COUNT = (int)(sizeof(INPUT_PROMPTS) / sizeof(INPUT_PROMPTS[0]));
