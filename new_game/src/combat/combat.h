#ifndef COMBAT_COMBAT_H
#define COMBAT_COMBAT_H

#include "game/game.h"
#include "combat/combat_globals.h"

// Hit type constants: identify the kind of hit that landed.
// H/M/L = High/Mid/Low -- determines which recoil animation the victim plays.
// uc_orig: HIT_TYPE_GUN_SHOT_H (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_H (1)
// uc_orig: HIT_TYPE_GUN_SHOT_M (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_M (2)
// uc_orig: HIT_TYPE_GUN_SHOT_L (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_L (3)
// uc_orig: HIT_TYPE_PUNCH_H (fallen/Headers/combat.h)
#define HIT_TYPE_PUNCH_H (4)
// uc_orig: HIT_TYPE_PUNCH_M (fallen/Headers/combat.h)
#define HIT_TYPE_PUNCH_M (5)
// uc_orig: HIT_TYPE_PUNCH_L (fallen/Headers/combat.h)
#define HIT_TYPE_PUNCH_L (6)
// uc_orig: HIT_TYPE_KICK_H (fallen/Headers/combat.h)
#define HIT_TYPE_KICK_H (7)
// uc_orig: HIT_TYPE_KICK_M (fallen/Headers/combat.h)
#define HIT_TYPE_KICK_M (8)
// uc_orig: HIT_TYPE_KICK_L (fallen/Headers/combat.h)
#define HIT_TYPE_KICK_L (9)

// Weapon-specific gun shot types used for per-weapon death animations and sounds.
// uc_orig: HIT_TYPE_GUN_SHOT_PISTOL (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_PISTOL (10)
// uc_orig: HIT_TYPE_GUN_SHOT_SHOTGUN (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_SHOTGUN (11)
// uc_orig: HIT_TYPE_GUN_SHOT_AK47 (fallen/Headers/combat.h)
#define HIT_TYPE_GUN_SHOT_AK47 (12)

// Attack direction constants used by find_attack_stance() and turn_to_target().
// Angles are in the 0-2047 range (2048 = 360 degrees).
// uc_orig: FIND_DIR_FRONT (fallen/Headers/combat.h)
#define FIND_DIR_FRONT 1
// uc_orig: FIND_DIR_BACK (fallen/Headers/combat.h)
#define FIND_DIR_BACK 2
// uc_orig: FIND_DIR_LEFT (fallen/Headers/combat.h)
#define FIND_DIR_LEFT 3
// uc_orig: FIND_DIR_RIGHT (fallen/Headers/combat.h)
#define FIND_DIR_RIGHT 4
// uc_orig: FIND_DIR_TURN_LEFT (fallen/Headers/combat.h)
#define FIND_DIR_TURN_LEFT 5
// uc_orig: FIND_DIR_TURN_RIGHT (fallen/Headers/combat.h)
#define FIND_DIR_TURN_RIGHT 6

// uc_orig: FIND_DIR_MASK (fallen/Headers/combat.h)
#define FIND_DIR_MASK (0xff)
// uc_orig: FIND_DIR_DONT_TURN (fallen/Headers/combat.h)
#define FIND_DIR_DONT_TURN (1 << 10)

// If set, always return a fallback anim even if no target is in range.
// uc_orig: FIND_BEST_USE_DEFAULT (fallen/Headers/combat.h)
#define FIND_BEST_USE_DEFAULT (1 << 0)

// Returns the COMBAT_PUNCH/KICK/KNIFE type stored in fight_tree[node][9].
// uc_orig: get_combat_type_for_node (fallen/Source/Combat.cpp)
SLONG get_combat_type_for_node(UBYTE current_node);

// Traverse the fight_tree one step: given current node + button column, return
// next node index and set *new_anim to the animation for that node.
// uc_orig: get_anim_and_node_for_action (fallen/Source/Combat.cpp)
SLONG get_anim_and_node_for_action(UBYTE current_node, UBYTE action, UWORD* new_anim);

// Initialize the gang_attacks[] pool to all zeros.
// uc_orig: init_gangattack (fallen/Source/Combat.cpp)
void init_gangattack(void);

// Assign or reclaim a gang_attacks slot for p_person.
// Evicts the LRU entry when the pool is full.
// uc_orig: get_gangattack (fallen/Source/Combat.cpp)
SLONG get_gangattack(Thing* p_person);

// Returns 1 if p_agressor is allowed to hit p_victim based on faction/alignment rules.
// uc_orig: people_allowed_to_hit_each_other (fallen/Source/Combat.cpp)
SLONG people_allowed_to_hit_each_other(Thing* p_victim, Thing* p_agressor);

// Decides if p_person will attempt a block against the incoming attack 'anim' from p_agressor.
// Players auto-block only when stepping back; AI uses a probability based on Skill.
// uc_orig: should_i_block (fallen/Source/Combat.cpp)
SLONG should_i_block(Thing* p_person, Thing* p_agressor, SLONG anim);

// Top-level melee hit tick function; called when an animation frame has a Fight pointer.
// Iterates the fight_info->Next chain and calls find_combat_target for each zone.
// uc_orig: apply_violence (fallen/Source/Combat.cpp)
SLONG apply_violence(Thing* p_thing);

// Applies damage, recoil animations, sounds, and death to a hit victim.
// fight may be NULL for gun shots (no anim fight data needed).
// uc_orig: apply_hit_to_person (fallen/Source/Combat.cpp)
SLONG apply_hit_to_person(Thing* p_thing, SLONG angle, SLONG type, SLONG damage, Thing* p_aggressor, struct GameFightCol* fight);

// Scans nearby people and returns the best target + facing angle for the attacker.
// attack_distance: desired distance to target in 8-bit map units.
// Returns 1 if a target was found; fills stance_target, stance_position, stance_angle.
// uc_orig: find_attack_stance (fallen/Source/Combat.cpp)
SLONG find_attack_stance(
    Thing* p_person,
    SLONG attack_direction,
    SLONG attack_distance,
    SLONG attack_range,
    Thing** stance_target,
    GameCoord* stance_position,
    SLONG* stance_angle);

// Calls find_attack_stance then rotates p_person to face the best target.
// Returns THING_NUMBER of target if found, negative value if none.
// uc_orig: turn_to_target (fallen/Source/Combat.cpp)
SLONG turn_to_target(Thing* p_person, SLONG find_dir);

// Like turn_to_target but also adjusts facing for left/right/back direction modes.
// Used by interfac.cpp for directional attack input.
// uc_orig: turn_to_direction_and_find_target (fallen/Source/Combat.cpp)
SLONG turn_to_direction_and_find_target(Thing* p_person, SLONG find_dir);

// Combined find+face+punch: turns toward best target and plays punch animation.
// Also calls should_i_block() on the target so it can react.
// uc_orig: turn_to_target_and_punch (fallen/Source/Combat.cpp)
SLONG turn_to_target_and_punch(Thing* p_person);

// Combined find+face+kick: turns toward best target and plays kick animation.
// Uses stomp if target is KO'd on the ground; kick_near if very close.
// uc_orig: turn_to_target_and_kick (fallen/Source/Combat.cpp)
SLONG turn_to_target_and_kick(Thing* p_person);

// Scans the grapples[] table and executes the best grapple move if valid target is found.
// Returns 1 on success, 0 if no valid target.
// uc_orig: find_best_grapple (fallen/Source/Combat.cpp)
SLONG find_best_grapple(Thing* p_person);

// Scans nearby people with KILLING ai_state targeting p_person.
// any_state must be UC_FALSE. radius controls search area.
// uc_orig: is_person_under_attack_low_level (fallen/Source/Combat.cpp)
Thing* is_person_under_attack_low_level(Thing* p_person, SLONG any_state, SLONG radius);

#endif // COMBAT_COMBAT_H
