// Keyboard & mouse help bodies. Edit freely. Inline glyph tokens use kb_* / ms_*
// ids from input_prompt_map. Shared device-agnostic prose lives in
// help_content_common.h. (Placeholder content — replace with the real text.)

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
