// PlayStation help bodies. Edit freely. Inline glyph tokens use ps_* ids from
// input_prompt_map. Shared device-agnostic prose lives in help_content_common.h.
// (Placeholder content — replace with the real text.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

const char* const HELP_MOVEMENT_PS =
    "Move with the {ps_ls} and look around with the {ps_rs}.\n"
    "Hold {ps_circle} to sprint, press {ps_cross} to jump, and hold {ps_square} "
    "to use or interact with whatever you are facing.\n"
    "\n" HELP_TXT_MOVEMENT_OUTRO;

const char* const HELP_COMBAT_PS =
    "Face an enemy and attack with {ps_r2}. Keep moving to avoid hits.";

const char* const HELP_CLIMBING_PS =
    "Approach a ledge and press {ps_cross} to pull yourself up.";

const char* const HELP_WEAPONS_PS =
    "Pick up weapons found in the level. Press {ps_r2} to fire the held weapon.";
