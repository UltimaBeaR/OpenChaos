#ifndef UI_MENUS_HELP_CONTENT_COMMON_H
#define UI_MENUS_HELP_CONTENT_COMMON_H

// Reusable help-text fragments shared across the per-device body files
// (help_content_kbm/xbox/ps.cpp). These are #define'd string literals so they
// concatenate with adjacent literals inside a body, e.g.:
//
//   const char* const HELP_MOVEMENT_KBM =
//       "Press {kb_space} to jump.\n\n" HELP_TXT_MOVEMENT_OUTRO;
//
// Use these for device-AGNOSTIC prose (feature descriptions, general tips, "what
// this is for") so the same wording isn't copy-pasted into all three files.
// Anything that names a button stays in the per-device file (the glyph tokens
// differ per device).

// Intro line shared by the CONTROLS reference on every device — it names no
// button, so the same wording fits all three.
#define HELP_TXT_CONTROLS_INTRO \
    "Every button used during gameplay. Menu controls are not listed here."

// Placeholder shared snippets — replace with real prose as content is written.
#define HELP_TXT_MOVEMENT_OUTRO \
    "Keep moving with a plan: stay out of sight, chain your moves, and never get " \
    "caught standing still in the open."

#endif // UI_MENUS_HELP_CONTENT_COMMON_H
