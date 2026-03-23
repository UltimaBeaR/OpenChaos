#include "ui/menus/widget_globals.h"
#include "ui/menus/widget.h"

// uc_orig: WidgetTick (fallen/Source/widget.cpp)
SLONG WidgetTick;

// uc_orig: EatenKey (fallen/Source/widget.cpp)
SLONG EatenKey;

// Widget type method tables. Each table maps widget lifecycle events to
// the corresponding handler functions in widget.cpp.

// uc_orig: BUTTON_Methods (fallen/Source/widget.cpp)
Methods BUTTON_Methods = { 0, BUTTON_Free, BUTTON_Draw, BUTTON_Push, 0, 0, BUTTON_HitTest };

// uc_orig: STATIC_Methods (fallen/Source/widget.cpp)
Methods STATIC_Methods = { STATIC_Init, BUTTON_Free, BUTTON_Draw, BUTTON_Push, 0, 0, BUTTON_HitTest };

// uc_orig: CHECK_Methods (fallen/Source/widget.cpp)
Methods CHECK_Methods = { 0, BUTTON_Free, CHECK_Draw, CHECK_Push, 0, 0, BUTTON_HitTest };

// uc_orig: RADIO_Methods (fallen/Source/widget.cpp)
Methods RADIO_Methods = { 0, BUTTON_Free, CHECK_Draw, RADIO_Push, 0, 0, BUTTON_HitTest };

// uc_orig: INPUT_Methods (fallen/Source/widget.cpp)
Methods INPUT_Methods = { INPUT_Init, INPUT_Free, INPUT_Draw, 0, INPUT_Char, INPUT_Data, BUTTON_HitTest };

// uc_orig: LISTS_Methods (fallen/Source/widget.cpp)
Methods LISTS_Methods = { 0, LISTS_Free, LISTS_Draw, LISTS_Push, LISTS_Char, LISTS_Data, BUTTON_HitTest };

// uc_orig: TEXTS_Methods (fallen/Source/widget.cpp)
Methods TEXTS_Methods = { TEXTS_Init, TEXTS_Free, TEXTS_Draw, 0, TEXTS_Char, TEXTS_Data, BUTTON_HitTest };

// uc_orig: GLYPH_Methods (fallen/Source/widget.cpp)
Methods GLYPH_Methods = { 0, BUTTON_Free, GLYPH_Draw, BUTTON_Push, 0, 0, BUTTON_HitTest };

// uc_orig: SHADE_Methods (fallen/Source/widget.cpp)
Methods SHADE_Methods = { 0, BUTTON_Free, SHADE_Draw, 0, 0, 0, 0 };
