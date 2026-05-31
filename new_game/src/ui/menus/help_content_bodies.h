#ifndef UI_MENUS_HELP_CONTENT_BODIES_H
#define UI_MENUS_HELP_CONTENT_BODIES_H

// Per-device help body strings. Declared here, DEFINED in the per-device files
// (help_content_kbm.cpp / help_content_xbox.cpp / help_content_ps.cpp) so each
// device's text can be edited independently. help_content.cpp wires them into
// HELP_TOPICS. Reusable text fragments live in help_content_common.h.
//
// Naming: HELP_<TOPIC>_<DEVICE>. Add a body here when you add a topic, and define
// it in all three device files.

extern const char* const HELP_CONTROLS_KBM;
extern const char* const HELP_CONTROLS_XBOX;
extern const char* const HELP_CONTROLS_PS;

extern const char* const HELP_MOVEMENT_KBM;
extern const char* const HELP_MOVEMENT_XBOX;
extern const char* const HELP_MOVEMENT_PS;

extern const char* const HELP_COMBAT_KBM;
extern const char* const HELP_COMBAT_XBOX;
extern const char* const HELP_COMBAT_PS;

extern const char* const HELP_CLIMBING_KBM;
extern const char* const HELP_CLIMBING_XBOX;
extern const char* const HELP_CLIMBING_PS;

extern const char* const HELP_WEAPONS_KBM;
extern const char* const HELP_WEAPONS_XBOX;
extern const char* const HELP_WEAPONS_PS;

#endif // UI_MENUS_HELP_CONTENT_BODIES_H
