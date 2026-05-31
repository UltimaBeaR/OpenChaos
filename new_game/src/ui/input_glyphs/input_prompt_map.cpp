#include "ui/input_glyphs/input_prompt_map.h"

#include <cstring> // strncmp

// ===========================================================================
// Glyph map. One row per glyph per device. `id` is the inline token used in help
// text; `label` shows in the dev catalog; {col,row} is the atlas cell (64px grid,
// row counted from the BOTTOM — see input_prompt_map.h). To re-check visually,
// open the "INPUT TEST" help topic (dev flag OC_DEBUG_INPUT_PROMPT_CATALOG).
// ===========================================================================

const InputGlyph INPUT_GLYPHS[] = {
    // ---- Mouse (keyboard & mouse atlas) ----
    { "ms_lmb",       "Left mouse button",       GLYPH_DEV_KBM, 3, 1 },
    { "ms_mmb",       "Middle mouse button",     GLYPH_DEV_KBM, 9, 1 },
    { "ms_rmb",       "Right mouse button",      GLYPH_DEV_KBM, 7, 1 },
    { "ms_wheel",     "Mouse wheel",             GLYPH_DEV_KBM, 15, 1 },
    { "ms_wheel_up",  "Mouse wheel up",          GLYPH_DEV_KBM, 13, 1 },
    { "ms_wheel_down","Mouse wheel down",        GLYPH_DEV_KBM, 10, 1 },
    { "ms_move",      "Mouse move",              GLYPH_DEV_KBM, 5, 1 },
    { "ms_move_h",    "Mouse move (horizontal)", GLYPH_DEV_KBM, 2, 1 },
    { "ms_move_v",    "Mouse move (vertical)",   GLYPH_DEV_KBM, 2, 0 },

    // ---- Keyboard letters ----
    { "kb_a", "A", GLYPH_DEV_KBM, 5, 14 },  { "kb_b", "B", GLYPH_DEV_KBM, 6, 12 },
    { "kb_c", "C", GLYPH_DEV_KBM, 6, 11 },  { "kb_d", "D", GLYPH_DEV_KBM, 6, 10 },
    { "kb_e", "E", GLYPH_DEV_KBM, 10, 10 }, { "kb_f", "F", GLYPH_DEV_KBM, 6, 9 },
    { "kb_g", "G", GLYPH_DEV_KBM, 2, 7 },   { "kb_h", "H", GLYPH_DEV_KBM, 4, 7 },
    { "kb_i", "I", GLYPH_DEV_KBM, 8, 7 },   { "kb_j", "J", GLYPH_DEV_KBM, 12, 7 },
    { "kb_k", "K", GLYPH_DEV_KBM, 14, 7 },  { "kb_l", "L", GLYPH_DEV_KBM, 0, 6 },
    { "kb_m", "M", GLYPH_DEV_KBM, 2, 6 },   { "kb_n", "N", GLYPH_DEV_KBM, 6, 6 },
    { "kb_o", "O", GLYPH_DEV_KBM, 14, 6 },  { "kb_p", "P", GLYPH_DEV_KBM, 3, 5 },
    { "kb_q", "Q", GLYPH_DEV_KBM, 15, 5 },  { "kb_r", "R", GLYPH_DEV_KBM, 5, 4 },
    { "kb_s", "S", GLYPH_DEV_KBM, 9, 4 },   { "kb_t", "T", GLYPH_DEV_KBM, 9, 3 },
    { "kb_u", "U", GLYPH_DEV_KBM, 3, 2 },   { "kb_v", "V", GLYPH_DEV_KBM, 5, 2 },
    { "kb_w", "W", GLYPH_DEV_KBM, 7, 2 },   { "kb_x", "X", GLYPH_DEV_KBM, 11, 2 },
    { "kb_y", "Y", GLYPH_DEV_KBM, 13, 2 },  { "kb_z", "Z", GLYPH_DEV_KBM, 15, 2 },

    // ---- Keyboard digits ----
    { "kb_0", "0", GLYPH_DEV_KBM, 1, 15 },  { "kb_1", "1", GLYPH_DEV_KBM, 3, 15 },
    { "kb_2", "2", GLYPH_DEV_KBM, 5, 15 },  { "kb_3", "3", GLYPH_DEV_KBM, 7, 15 },
    { "kb_4", "4", GLYPH_DEV_KBM, 9, 15 },  { "kb_5", "5", GLYPH_DEV_KBM, 11, 15 },
    { "kb_6", "6", GLYPH_DEV_KBM, 13, 15 }, { "kb_7", "7", GLYPH_DEV_KBM, 15, 15 },
    { "kb_8", "8", GLYPH_DEV_KBM, 1, 14 },  { "kb_9", "9", GLYPH_DEV_KBM, 3, 14 },

    // ---- Keyboard symbols (main block) ----
    { "kb_backtick",   "Backtick `",   GLYPH_DEV_KBM, 1, 2 },
    { "kb_minus",      "Minus -",      GLYPH_DEV_KBM, 4, 6 },
    { "kb_equals",     "Equals =",     GLYPH_DEV_KBM, 0, 9 },
    { "kb_lbracket",   "Bracket [",    GLYPH_DEV_KBM, 4, 11 },
    { "kb_rbracket",   "Bracket ]",    GLYPH_DEV_KBM, 14, 12 },
    { "kb_backslash",  "Backslash \\", GLYPH_DEV_KBM, 1, 3 },
    { "kb_semicolon",  "Semicolon ;",  GLYPH_DEV_KBM, 11, 4 },
    { "kb_apostrophe", "Apostrophe '", GLYPH_DEV_KBM, 11, 14 },
    { "kb_comma",      "Comma ,",      GLYPH_DEV_KBM, 0, 10 },
    { "kb_period",     "Period .",     GLYPH_DEV_KBM, 9, 5 },
    { "kb_slash",      "Slash /",      GLYPH_DEV_KBM, 3, 3 },

    // ---- Keyboard special (main block) ----
    { "kb_tab",       "Tab",       GLYPH_DEV_KBM, 12, 3 },
    { "kb_caps",      "Caps Lock", GLYPH_DEV_KBM, 8, 11 },
    { "kb_shift",     "Shift",     GLYPH_DEV_KBM, 14, 4 },
    { "kb_ctrl",      "Ctrl",      GLYPH_DEV_KBM, 4, 10 },
    { "kb_alt",       "Alt",       GLYPH_DEV_KBM, 7, 14 },
    { "kb_space",     "Space",     GLYPH_DEV_KBM, 6, 3 },
    { "kb_enter",     "Enter",     GLYPH_DEV_KBM, 7, 4 },
    { "kb_backspace", "Backspace", GLYPH_DEV_KBM, 9, 12 },
    { "kb_esc",       "Esc",       GLYPH_DEV_KBM, 2, 9 },

    // ---- Xbox ----
    { "xb_a", "A", GLYPH_DEV_XBOX, 4, 9 },
    { "xb_b", "B", GLYPH_DEV_XBOX, 6, 9 },
    { "xb_x", "X", GLYPH_DEV_XBOX, 0, 6 },
    { "xb_y", "Y", GLYPH_DEV_XBOX, 2, 6 },
    { "xb_lb", "LB", GLYPH_DEV_XBOX, 7, 3 },
    { "xb_rb", "RB", GLYPH_DEV_XBOX, 3, 2 },
    { "xb_lt", "LT", GLYPH_DEV_XBOX, 1, 2 },
    { "xb_rt", "RT", GLYPH_DEV_XBOX, 7, 2 },
    { "xb_dpad",       "D-pad",       GLYPH_DEV_XBOX, 4, 6 },
    { "xb_dpad_up",    "D-pad Up",    GLYPH_DEV_XBOX, 3, 4 },
    { "xb_dpad_down",  "D-pad Down",  GLYPH_DEV_XBOX, 6, 6 },
    { "xb_dpad_left",  "D-pad Left",  GLYPH_DEV_XBOX, 0, 5 },
    { "xb_dpad_right", "D-pad Right", GLYPH_DEV_XBOX, 3, 5 },
    { "xb_ls",        "Left Stick",          GLYPH_DEV_XBOX, 9, 2 },
    { "xb_ls_h",      "Left Stick (horiz)",  GLYPH_DEV_XBOX, 1, 1 },
    { "xb_ls_v",      "Left Stick (vert)",   GLYPH_DEV_XBOX, 6, 1 },
    { "xb_ls_press",  "LS (press)",          GLYPH_DEV_XBOX, 5, 0 },
    { "xb_rs",        "Right Stick",         GLYPH_DEV_XBOX, 7, 1 },
    { "xb_rs_h",      "Right Stick (horiz)", GLYPH_DEV_XBOX, 9, 1 },
    { "xb_rs_v",      "Right Stick (vert)",  GLYPH_DEV_XBOX, 4, 0 },
    { "xb_rs_press",  "RS (press)",          GLYPH_DEV_XBOX, 6, 0 },
    { "xb_start",     "Start (Menu)",        GLYPH_DEV_XBOX, 0, 7 },
    { "xb_select",    "Select (View)",       GLYPH_DEV_XBOX, 8, 7 },

    // ---- PlayStation ----
    { "ps_cross",    "Cross",    GLYPH_DEV_PS, 5, 10 },
    { "ps_circle",   "Circle",   GLYPH_DEV_PS, 7, 11 },
    { "ps_square",   "Square",   GLYPH_DEV_PS, 11, 10 },
    { "ps_triangle", "Triangle", GLYPH_DEV_PS, 1, 9 },
    { "ps_l1", "L1", GLYPH_DEV_PS, 2, 6 },
    { "ps_r1", "R1", GLYPH_DEV_PS, 10, 6 },
    { "ps_l2", "L2", GLYPH_DEV_PS, 6, 6 },
    { "ps_r2", "R2", GLYPH_DEV_PS, 2, 5 },
    { "ps_dpad",       "D-pad",       GLYPH_DEV_PS, 3, 9 },
    { "ps_dpad_up",    "D-pad Up",    GLYPH_DEV_PS, 2, 8 },
    { "ps_dpad_down",  "D-pad Down",  GLYPH_DEV_PS, 5, 9 },
    { "ps_dpad_left",  "D-pad Left",  GLYPH_DEV_PS, 9, 9 },
    { "ps_dpad_right", "D-pad Right", GLYPH_DEV_PS, 0, 8 },
    { "ps_ls",       "Left Stick",          GLYPH_DEV_PS, 6, 8 },
    { "ps_ls_h",     "Left Stick (horiz)",  GLYPH_DEV_PS, 8, 8 },
    { "ps_ls_v",     "Left Stick (vert)",   GLYPH_DEV_PS, 1, 7 },
    { "ps_l3",       "L3 (press)",          GLYPH_DEV_PS, 10, 7 },
    { "ps_rs",       "Right Stick",         GLYPH_DEV_PS, 2, 7 },
    { "ps_rs_h",     "Right Stick (horiz)", GLYPH_DEV_PS, 4, 7 },
    { "ps_rs_v",     "Right Stick (vert)",  GLYPH_DEV_PS, 9, 7 },
    { "ps_r3",       "R3 (press)",          GLYPH_DEV_PS, 11, 7 },
    { "ps_start",    "Options",             GLYPH_DEV_PS, 11, 3 },
    { "ps_select",   "Share",               GLYPH_DEV_PS, 5, 3 },
};

const int INPUT_GLYPH_COUNT = (int)(sizeof(INPUT_GLYPHS) / sizeof(INPUT_GLYPHS[0]));

const InputGlyph* input_glyph_find(const char* id, unsigned long len)
{
    for (int i = 0; i < INPUT_GLYPH_COUNT; ++i) {
        const char* gid = INPUT_GLYPHS[i].id;
        if (strncmp(gid, id, len) == 0 && gid[len] == '\0') {
            return &INPUT_GLYPHS[i];
        }
    }
    return nullptr;
}
