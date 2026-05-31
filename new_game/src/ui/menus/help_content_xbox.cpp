// Xbox help bodies. Edit freely. Inline glyph tokens use xb_* ids from
// input_prompt_map. Shared device-agnostic prose lives in help_content_common.h.
// (Placeholder content — replace with the real text.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

// Section 1 — full button reference (the "control map"). One line per unique
// control, listing every gameplay situation it is used in (on foot + in a car).
const char* const HELP_CONTROLS_XBOX =
    HELP_TXT_CONTROLS_INTRO "\n"
    "\n"
    "{xb_rs} - Look around.\n"
    "{xb_rt} - Attack or shoot. In a car: gas.\n"
    "{xb_rb} - Kick.\n"
    "{xb_rs_press} - Draw or holster your weapon.\n"
    "{xb_rs_press}+{xb_dpad_up} / {xb_rs_press}+{xb_dpad_down} - Cycle through "
    "your weapons.\n"
    "{xb_ls} - Move and run. In a car: steering.\n"
    "{xb_a} - Jump.\n"
    "{xb_b} - Sprint.\n"
    "{xb_lt} - Slow walk and rolls. In a car: reverse.\n"
    "{xb_x} - Action: get in or out of a car, pick up items, search bodies, "
    "arrest, and "
    "interact with the world. Switch target in a fight.\n"
    "{xb_y} - Stealth. In a car: siren.\n"
    "{xb_lb} - Zoom in to look around; back-step while moving. In a car: brake.\n"
    "{xb_dpad_up} - Select weapon: pistol.\n"
    "{xb_dpad_left} - Select weapon: M16.\n"
    "{xb_dpad_right} - Select weapon: shotgun.\n"
    "{xb_dpad_up}+{xb_dpad_right} - Select weapon: grenade.\n"
    "{xb_dpad_down} - Switch melee weapon: bat or knife.\n"
    "{xb_start} - Pause.";

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
