// PlayStation help bodies. Edit freely. Inline glyph tokens use ps_* ids from
// input_prompt_map. Shared device-agnostic prose lives in help_content_common.h.
// (Placeholder content — replace with the real text.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

// Section 1 — full button reference (the "control map"). One line per unique
// control, listing every gameplay situation it is used in (on foot + in a car).
const char* const HELP_CONTROLS_PS =
    HELP_TXT_CONTROLS_INTRO "\n"
    "\n"
    "{ps_rs} - Look around.\n"
    "{ps_r2} - Attack or shoot. In a car: gas.\n"
    "{ps_r1} - Kick.\n"
    "{ps_r3} - Draw or holster your weapon.\n"
    "{ps_r3}+{ps_dpad_up} / {ps_r3}+{ps_dpad_down} - Cycle through "
    "your weapons.\n"
    "{ps_ls} - Move and run. In a car: steering.\n"
    "{ps_cross} - Jump.\n"
    "{ps_circle} - Sprint.\n"
    "{ps_l2} - Slow walk and rolls. In a car: reverse.\n"
    "{ps_square} - Action: get in or out of a car, pick up items, search "
    "bodies, arrest, and interact with the world. Switch target in a fight.\n"
    "{ps_triangle} - Stealth. In a car: siren.\n"
    "{ps_l1} - Zoom in to look around; back-step while moving. In a car: brake.\n"
    "{ps_dpad_up} - Select weapon: pistol.\n"
    "{ps_dpad_left} - Select weapon: M16.\n"
    "{ps_dpad_right} - Select weapon: shotgun.\n"
    "{ps_dpad_up}+{ps_dpad_right} - Select weapon: grenade.\n"
    "{ps_dpad_down} - Switch melee weapon: bat or knife.\n"
    "{ps_start} - Pause.";

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
