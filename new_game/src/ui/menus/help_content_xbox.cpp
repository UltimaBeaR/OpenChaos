// Xbox help bodies. Edit freely. Inline glyph tokens use xb_* ids from
// input_prompt_map. Shared device-agnostic prose lives in help_content_common.h.
// (Placeholder content — replace with the real text.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

const char* const HELP_MOVEMENT_XBOX =
    "Move with the {xb_ls} and look around with the {xb_rs}.\n"
    "Hold {xb_b} to sprint, press {xb_a} to jump, and hold {xb_x} to use or "
    "interact with whatever you are facing.\n"
    "\n" HELP_TXT_MOVEMENT_OUTRO;

const char* const HELP_COMBAT_XBOX =
    "Face an enemy and attack with {xb_rt}. Keep moving to avoid hits.";

const char* const HELP_CLIMBING_XBOX =
    "Approach a ledge and press {xb_a} to pull yourself up.";

const char* const HELP_WEAPONS_XBOX =
    "Pick up weapons found in the level. Press {xb_rt} to fire the held weapon.";
