#ifndef UI_MENUS_HELP_CONTENT_COMMON_H
#define UI_MENUS_HELP_CONTENT_COMMON_H

// Reusable help-text fragments shared across the per-device body files
// (help_content_kbm/xbox/ps.cpp). These are #define'd string literals so they
// concatenate with adjacent literals inside a body, e.g.:
//
//   const char* const HELP_CONTROLS_KBM =
//       HELP_TXT_CONTROLS_INTRO "\n\n" "{kb_space} - Jump.\n";
//
// Use these for device-AGNOSTIC prose (feature descriptions, general tips, "what
// this is for") so the same wording isn't copy-pasted into all three files.
// Anything that names a button stays in the per-device file (the glyph tokens
// differ per device).

// Intro line shared by the CONTROLS reference on every device — it names no
// button, so the same wording fits all three.
#define HELP_TXT_CONTROLS_INTRO \
    "Every button used during gameplay. Menu controls are not listed here."

#endif // UI_MENUS_HELP_CONTENT_COMMON_H
