// Keyboard & mouse help bodies. Edit freely. Inline glyph tokens use kb_* / ms_*
// ids from input_prompt_map. Shared device-agnostic prose lives in
// help_content_common.h. (REFERENCE and MOVEMENT are written; COMBAT and
// WEAPONS are still placeholders.)

#include "ui/menus/help_content_bodies.h"
#include "ui/menus/help_content_common.h"

// Section 1 — full button reference (the "control map"). One line per unique
// control, listing every gameplay situation it is used in (on foot + in a car).
// Short reference, not a how-to; the per-mechanic guide topics cover usage.
const char* const HELP_CONTROLS_KBM =
    HELP_TXT_CONTROLS_INTRO "\n"
    "\n"
    "{ms_move} - Look around.\n"
    "{ms_lmb} - Attack or shoot.\n"
    "{ms_rmb} - Kick.\n"
    "{ms_mmb} - Draw or holster your weapon.\n"
    "{ms_wheel} - Cycle through your weapons.\n"
    "{kb_w}{kb_a}{kb_s}{kb_d} - Move and run. In a car: {kb_w} gas, {kb_s} "
    "reverse, {kb_a}{kb_d} steering.\n"
    "{kb_space} - Jump. In a car: brake.\n"
    "{kb_shift} - Sprint.\n"
    "{kb_ctrl} - Slow walk and rolls.\n"
    "{kb_f} - Action: get in or out of a car, pick up items, search bodies, "
    "arrest, and interact with the world. Switch target in a fight.\n"
    "{kb_c} - Stealth.\n"
    "{kb_e} - Zoom in to look around; back-step while moving. In a car: siren.\n"
    "{kb_1} - Select weapon: pistol.\n"
    "{kb_2} - Select weapon: M16.\n"
    "{kb_3} - Select weapon: shotgun.\n"
    "{kb_4} - Select weapon: grenade.\n"
    "{kb_tab} - Switch melee weapon: bat or knife.\n"
    "{kb_esc} - Pause."
    ;

const char* const HELP_MOVEMENT_KBM =
    "MOVING\n"
    "Run with {kb_w}{kb_a}{kb_s}{kb_d}; hold {kb_shift} while running to "
    "sprint.\n"
    "Hold {kb_ctrl} to walk slowly and quietly.\n"
    "Hold {kb_e} and push back to walk backwards while still facing front; "
    "pushing forward is ignored.\n"
    "\n"
    "CAMERA\n"
    "Move {ms_move} to swing the camera; Darci always turns to face where it "
    "points, so you can also steer where she runs by swinging the camera as she "
    "goes. From a standstill, hold {kb_e} to zoom in for a closer look.\n"
    "\n"
    "JUMPING\n"
    "Tap {kb_space} to jump, on the spot or along your run. Jump while backing "
    "away to do a backflip.\n"
    "\n"
    "CLIMBING\n"
    "Jump at a ledge to grab on if you reach it; tap {kb_space} again to climb "
    "up, or edge left and right while hanging.\n"
    "Walk into a ladder to grab on; to get on heading down, back up to it (hold "
    "{kb_e} and push back) and press {kb_f}. Once on, push forward or back to "
    "climb up or down.\n"
    "Jump onto a wire, often strung between buildings, and Darci catches it and "
    "slides down.\n"
    "\n"
    "ROLLING\n"
    "While moving, tap {kb_ctrl} with a direction to roll that way.\n"
    "\n"
    "STEALTH\n"
    "From a standstill, hold {kb_c} to crouch; then move to crawl on all fours "
    "and stay out of sight.\n"
    "\n"
    "WALL HUG\n"
    "Press {kb_f} against a wall to flatten against it and edge along, left or "
    "right.";

const char* const HELP_COMBAT_KBM =
    "Face an enemy and attack with {ms_lmb}. Keep moving to avoid hits.";

const char* const HELP_WEAPONS_KBM =
    "Pick up weapons found in the level. Press {ms_lmb} to fire the held weapon.";
