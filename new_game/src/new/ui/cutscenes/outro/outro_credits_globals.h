#ifndef UI_CUTSCENES_OUTRO_OUTRO_CREDITS_GLOBALS_H
#define UI_CUTSCENES_OUTRO_OUTRO_CREDITS_GLOBALS_H

#include "fallen/outro/always.h" // Temporary: outro uses its own type definitions

// uc_orig: CREDITS_current_section (fallen/outro/credits.cpp)
extern SLONG CREDITS_current_section;

// uc_orig: CREDITS_current_y (fallen/outro/credits.cpp)
// Y position of the current section's first line (scrolls upward from 1.0 to 0.0).
extern float CREDITS_current_y;

// uc_orig: CREDITS_current_end_y (fallen/outro/credits.cpp)
// Y position of the last drawn line (used to detect when to advance to next section).
extern float CREDITS_current_end_y;

// uc_orig: CREDITS_last (fallen/outro/credits.cpp)
// Timestamp of last scroll update (milliseconds).
extern SLONG CREDITS_last;

// uc_orig: CREDITS_now (fallen/outro/credits.cpp)
// Current timestamp (milliseconds).
extern SLONG CREDITS_now;

// uc_orig: CREDITS_Section (fallen/outro/credits.cpp)
// One section of the credits scroll: a title string and a null-terminated list of lines.
typedef struct
{
    CBYTE* title;
    CBYTE** line;
} CREDITS_Section;

// uc_orig: CREDITS_NUM_SECTIONS (fallen/outro/credits.cpp)
#define CREDITS_NUM_SECTIONS 7

// uc_orig: CREDITS_section (fallen/outro/credits.cpp)
extern CREDITS_Section CREDITS_section[CREDITS_NUM_SECTIONS];

#endif // UI_CUTSCENES_OUTRO_OUTRO_CREDITS_GLOBALS_H
