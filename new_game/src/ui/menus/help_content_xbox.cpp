// Xbox help bodies. Edit freely. Inline glyph tokens use xb_* ids from
// input_prompt_map. Shared device-agnostic prose lives in help_content_common.h.
// (REFERENCE and MOVEMENT are written; COMBAT and WEAPONS are still placeholders.)

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
    "MOVING\n"
    "Run with the {xb_ls}; hold {xb_b} while running to sprint.\n"
    "Hold {xb_lt} to walk slowly and quietly.\n"
    "Hold {xb_lb} and push back to walk backwards while still facing front; "
    "pushing forward is ignored.\n"
    "\n"
    "CAMERA\n"
    "Move the {xb_rs} to swing the camera; Darci always turns to face where it "
    "points, so you can also steer where she runs by swinging the camera as she "
    "goes. From a standstill, hold {xb_lb} to zoom in for a closer look.\n"
    "\n"
    "JUMPING\n"
    "Tap {xb_a} to jump, on the spot or along your run. Jump while backing "
    "away to do a backflip.\n"
    "\n"
    "CLIMBING\n"
    "Jump at a ledge to grab on if you reach it; tap {xb_a} again to climb "
    "up, or edge left and right while hanging.\n"
    "Walk into a ladder to grab on; to get on heading down, back up to it (hold "
    "{xb_lb} and push back) and press {xb_x}. Once on, push forward or back to "
    "climb up or down.\n"
    "Jump onto a wire, often strung between buildings, and Darci catches it and "
    "slides down.\n"
    "\n"
    "ROLLING\n"
    "While moving, tap {xb_lt} with a direction to roll that way.\n"
    "\n"
    "STEALTH\n"
    "From a standstill, hold {xb_y} to crouch; then move to crawl on all fours "
    "and stay out of sight.\n"
    "\n"
    "WALL HUG\n"
    "Press {xb_x} against a wall to flatten against it and edge along, left or "
    "right.";

const char* const HELP_COMBAT_XBOX =
    "Face an enemy and attack with {xb_rt}. Keep moving to avoid hits.";

const char* const HELP_WEAPONS_XBOX =
    "Pick up weapons found in the level. Press {xb_rt} to fire the held weapon.";
