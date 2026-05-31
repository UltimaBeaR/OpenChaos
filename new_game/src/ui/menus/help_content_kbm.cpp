// Keyboard & mouse help bodies. Edit freely. Inline glyph tokens use kb_* / ms_*
// ids from input_prompt_map. Shared device-agnostic prose lives in
// help_content_common.h. (Placeholder content — replace with the real text.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

const char* const HELP_MOVEMENT_KBM =
    "Move with {kb_w} {kb_a} {kb_s} {kb_d} and look around with {ms_move}.\n"
    "Hold {kb_shift} to sprint, press {kb_space} to jump, and hold {kb_f} to use "
    "or interact with whatever you are facing.\n"
    "\n" HELP_TXT_MOVEMENT_OUTRO;

const char* const HELP_COMBAT_KBM =
    "Face an enemy and attack with {ms_lmb}. Keep moving to avoid hits.";

const char* const HELP_CLIMBING_KBM =
    "Approach a ledge and press {kb_space} to pull yourself up.";

const char* const HELP_WEAPONS_KBM =
    "Pick up weapons found in the level. Press {ms_lmb} to fire the held weapon.";
