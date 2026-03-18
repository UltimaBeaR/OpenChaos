// claude-ai: combat.h — Public interface for the melee/ranged combat system.
// claude-ai: Covers: hit type constants, fight history structs, gang-attack tracking,
// claude-ai: and the main API: apply_violence(), apply_hit_to_person(), find_attack_stance().
// claude-ai: PORT: all of this should be ported — it drives all melee combat gameplay.

#ifndef FALLEN_HEADERS_COMBAT_H
#define FALLEN_HEADERS_COMBAT_H

//
// Defines
//

// claude-ai: HIT_TYPE_* — identifies what kind of hit landed; used in apply_hit_to_person()
// claude-ai: H/M/L = High/Mid/Low. Affects which animation the victim plays.
// claude-ai: Gun shot types 1-3 are generic; 10-12 are weapon-specific (pistol/shotgun/AK47).
#define HIT_TYPE_GUN_SHOT_H (1)
#define HIT_TYPE_GUN_SHOT_M (2)
#define HIT_TYPE_GUN_SHOT_L (3)
#define HIT_TYPE_PUNCH_H (4)
#define HIT_TYPE_PUNCH_M (5)
#define HIT_TYPE_PUNCH_L (6)
#define HIT_TYPE_KICK_H (7)
#define HIT_TYPE_KICK_M (8)
#define HIT_TYPE_KICK_L (9)

// claude-ai: Weapon-specific gun shot types — used for per-weapon death animations & sounds
#define HIT_TYPE_GUN_SHOT_PISTOL (10)
#define HIT_TYPE_GUN_SHOT_SHOTGUN (11)
#define HIT_TYPE_GUN_SHOT_AK47 (12)

// claude-ai: MAX_HISTORY — pool size for GangAttack / ComboHistory / BlockingHistory structs
// claude-ai: Reused LRU-style: oldest entry evicted when pool is full. 20 is enough for typical crowd.
#define MAX_HISTORY 20
// claude-ai: MAX_MOVES — ring-buffer size for recording recent attack sequences (must be even)
#define MAX_MOVES 16 // must be even

// claude-ai: COMBAT_* — values stored in fight_tree[][9] (HitType column); defines what a move IS
#define COMBAT_NONE 0
#define COMBAT_PUNCH 1
#define COMBAT_KICK 2
#define COMBAT_KNIFE 3
//
// Structs
//


//
// Owner is the person under attack by multiple foes
// This structure has slots for angles that enemies are attacking from
// claude-ai: GangAttack — tracks up to 8 attackers (one per compass direction) for a single victim.
// claude-ai: Used by AI to avoid all enemies piling onto one side. Prevents gang pile-ons from same angle.
// claude-ai: PORT: port this — important for authentic AI crowd behaviour.
struct GangAttack {
    UWORD Owner; // who this gang attack is for
    UWORD Perp[8]; // who is attacking in each of the eight compass points

    UWORD LastUsed; // when was this whole structure used
    UBYTE Index;
    UBYTE Count;
};

//
// Data
//

// claude-ai: gang_attacks — global pool. Index 0 is unused (NULL sentinel).
extern struct GangAttack gang_attacks[MAX_HISTORY];

//
// Functions
//

// claude-ai: get_combat_type_for_node — returns COMBAT_PUNCH/KICK/KNIFE for current fight_tree node
extern SLONG get_combat_type_for_node(UBYTE current_node);
// claude-ai: get_anim_and_node_for_action — traverse fight_tree: given current node + button input, return next node + anim
extern SLONG get_anim_and_node_for_action(UBYTE current_node, UBYTE action, UWORD* new_anim);
// claude-ai: apply_violence — called each tick during attack anims; finds nearby targets and hits them
extern SLONG apply_violence(Thing* p_thing);
// claude-ai: apply_hit_to_person — applies damage + animations + sounds + death to a hit victim
// claude-ai: fight param is the GameFightCol from the attacking animation keyframe (may be NULL for gun shots)
extern SLONG apply_hit_to_person(Thing* p_thing, SLONG angle, SLONG type, SLONG damage, Thing* p_aggressor, struct GameFightCol* fight);

//
// Looks for a target in the given direction relative to the given person.
// Returns a position and angle the person would like to be to have an optimum
// fighting stance against the person.
//

// claude-ai: FIND_DIR_* — attack direction constants used by find_attack_stance() and turn_to_target()
// claude-ai: Angles are in the 0-2047 range (2048 = 360 degrees). LEFT=+512, RIGHT=-512, BACK=+1024.
#define FIND_DIR_FRONT 1 // For a frontal attack
#define FIND_DIR_BACK 2 // For a backwards attack
#define FIND_DIR_LEFT 3 // For a left attack
#define FIND_DIR_RIGHT 4 // For a right attack
#define FIND_DIR_TURN_LEFT 5 // For turning to attack a person on your left  from the front
#define FIND_DIR_TURN_RIGHT 6 // For turning to attack a person on your right from the front

#define FIND_DIR_MASK (0xff)

#define FIND_DIR_DONT_TURN (1 << 10)

// claude-ai: find_attack_stance — scores all nearby people and returns the best target + facing angle.
// claude-ai: attack_distance: desired distance to target (8-bits per mapsquare, ~0x80 typical for melee).
// claude-ai: Returns 1 if a target was found, 0 if not. Sets *stance_target, *stance_position, *stance_angle.
SLONG find_attack_stance(
    Thing* p_person,
    SLONG attack_direction,
    SLONG attack_distance, // Desired distance from a person to attack them. 8-bits per mapsquare.
    Thing** stance_target,
    GameCoord* stance_position, // 16-bits per mapsquare position at the desired distance.
    SLONG* stance_angle);

//
// Turns so you are facing someone in the given direction.
//

// claude-ai: turn_to_target — calls find_attack_stance, then rotates person to face best target.
// claude-ai: Returns THING_NUMBER of target if found, negative value if none.
SLONG turn_to_target(
    Thing* p_person,
    SLONG find_dir);

//
// Looks for someone in front of you to punch, turns and positions yourself
// to punch them and then does the punch- the same except you kick.
//

// claude-ai: turn_to_target_and_punch/kick — combined "find + face + attack" helpers used by state machine.
// claude-ai: Also calls should_i_block() on the target so victim can react with a block animation.
SLONG turn_to_target_and_punch(Thing* p_person);
SLONG turn_to_target_and_kick(Thing* p_person);

//
// Finds the best anims for the person to punch or kick someone. If no
// anim is any good the funcion either returns NULL, or, if (flag &
// FUND_BEST_USE_DEFAULT) it returns a default punch/kick.
//

// claude-ai: FIND_BEST_USE_DEFAULT — if set, always return a fallback anim even if no target in range
#define FIND_BEST_USE_DEFAULT (1 << 0)


//
// If somebody is attacking the given person, it returns a pointer to
// the attacker. If there is more than one attacker- it picks the nearest
// and returns him.
//

// claude-ai: is_person_under_attack — scans nearby people with pcom_ai_state==KILLING targeting p_person.
// claude-ai: Radius ~0x800 map units. Returns nearest attacker or NULL.
Thing* is_person_under_attack(Thing* p_person);
// claude-ai: is_anyone_nearby_alive — return nearest alive person (declared here, presumably in another .cpp)
Thing* is_anyone_nearby_alive(Thing* p_person); // The nearest person alive to you.

#endif
