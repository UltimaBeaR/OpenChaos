// uc_orig: controls_globals (fallen/Source/Controls.cpp)
// Global variables for the controls/game-controller subsystem.

#include "game/game_tick_globals.h"

// uc_orig: NIGHT_specular_enable (fallen/Source/Controls.cpp)
SLONG NIGHT_specular_enable = UC_FALSE;

// uc_orig: draw_3d (fallen/Source/Controls.cpp)
SLONG draw_3d;

// uc_orig: stealth_debug (fallen/Source/Controls.cpp)
UBYTE stealth_debug = 0;

// uc_orig: CONTROLS_inventory_mode (fallen/Source/Controls.cpp)
SWORD CONTROLS_inventory_mode = 0;

// uc_orig: InkeyToAscii (fallen/Source/Controls.cpp)
UBYTE InkeyToAscii[] = {
    /*   0 - 9   */ 0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
    /*  10 - 19  */ '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    /*  20 - 29  */ 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    /*  30 - 39  */ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /*  40 - 49  */ '\'', '`', 0, '#', 'z', 'x', 'c', 'v', 'b', 'n',
    /*  50 - 59  */ 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0,
    /*  60 - 69  */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*  70 - 79  */ 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
    /*  80 - 89  */ 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0,
    /*  90 - 99  */ 0, 0, 0, 0, 0, 0, '/', 0, 0, '(',
    /* 100 - 109 */ ')', '/', '*', 0, 0, 0, 0, 0, 0, 0,
    /* 110 - 119 */ 0, 0, 0, '.', 0, 0, 0, 0, 0, 0,
    /* 120 - 127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

// uc_orig: InkeyToAsciiShift (fallen/Source/Controls.cpp)
UBYTE InkeyToAsciiShift[] = {
    /*   0 - 9   */ 0, 0, '!', '"', (UBYTE)0xa3, '$', '%', '^', '&', '*',
    /*  10 - 19  */ '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
    /*  20 - 29  */ 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
    /*  30 - 39  */ 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
    /*  40 - 49  */ '@', '~', 0, '~', 'Z', 'X', 'C', 'V', 'B', 'N',
    /*  50 - 59  */ 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0,
    /*  60 - 69  */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*  70 - 79  */ 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
    /*  80 - 89  */ 0, 0, 0, 0, 0, 0, '|', 0, 0, 0,
    /*  90 - 99  */ 0, 0, 0, 0, 0, 0, '/', 0, 0, '(',
    /* 100 - 109 */ ')', '/', '*', 0, 0, 0, 0, 0, 0, 0,
    /* 110 - 119 */ 0, 0, 0, '.', 0, 0, 0, 0, 0, 0,
    /* 120 - 127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

// uc_orig: cmd_list (fallen/Source/Controls.cpp)
// Debug console command names. Index matches the switch in parse_console().
CBYTE* cmd_list[] = { "cam", "echo", "tels", "telr", "telw", "break", "wpt", "vtx", "alpha", "gamma", "bangunsnotgames", "cctv", "win", "lose", "s", "l", "restart", "ambient", "analogue", "world", "fade", "roper", "darci", "crinkles", "bangunsnotgames", "boo", NULL };

// uc_orig: allow_debug_keys (fallen/Source/Controls.cpp)
BOOL allow_debug_keys = 0;

// OpenChaos debug toggle (F10 after bangunsnotgames cheat): suppresses the
// normal level geometry draw and switches FARFACET_draw into the shader's
// debug-split mode (purple where opaque, green where semi-transparent), so
// the far-facet silhouette layer can be inspected in isolation. Used for
// tuning the FARFACET system — see stage12_farfacets.md.
BOOL g_farfacet_debug = 0;

// uc_orig: dkeys_have_been_used (fallen/Source/Controls.cpp)
BOOL dkeys_have_been_used;

// uc_orig: yomp_speed (fallen/Source/Controls.cpp)
SLONG yomp_speed = 40;

// uc_orig: sprint_speed (fallen/Source/Controls.cpp)
SLONG sprint_speed = 70;
