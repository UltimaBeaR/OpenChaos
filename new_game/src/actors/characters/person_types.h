#ifndef ACTORS_CHARACTERS_PERSON_TYPES_H
#define ACTORS_CHARACTERS_PERSON_TYPES_H

// Person struct definition, type constants, flag macros, and mode constants.
// Kept separate from person.h so headers that only need the struct (e.g. thing.h)
// do not pull in the full person API.

#include "core/types.h"
#include "core/vector.h"          // GameCoord (used by GunMuzzle)
#include "actors/core/common.h"   // COMMON macro
#include "ai/mav_action.h"        // MAV_Action
#include "missions/save.h"        // save_table[], SAVE_TABLE_PEOPLE (needed by MAX_PEOPLE)

// ---- Character rig type IDs ----

// uc_orig: ANIM_TYPE_DARCI (fallen/Headers/Person.h)
#define ANIM_TYPE_DARCI 0

// uc_orig: ANIM_TYPE_ROPER (fallen/Headers/Person.h)
#define ANIM_TYPE_ROPER 1

// uc_orig: ANIM_TYPE_ROPER2 (fallen/Headers/Person.h)
#define ANIM_TYPE_ROPER2 2

// uc_orig: ANIM_TYPE_CIV (fallen/Headers/Person.h)
#define ANIM_TYPE_CIV 3

// ---- Person character type IDs ----

// uc_orig: PERSON_NONE (fallen/Headers/Person.h)
#define PERSON_NONE 100

// uc_orig: PERSON_DARCI (fallen/Headers/Person.h)
#define PERSON_DARCI 0

// uc_orig: PERSON_ROPER (fallen/Headers/Person.h)
#define PERSON_ROPER 1

// uc_orig: PERSON_COP (fallen/Headers/Person.h)
#define PERSON_COP 2

// uc_orig: PERSON_CIV (fallen/Headers/Person.h)
#define PERSON_CIV 3

// uc_orig: PERSON_THUG_RASTA (fallen/Headers/Person.h)
#define PERSON_THUG_RASTA 4

// uc_orig: PERSON_THUG_GREY (fallen/Headers/Person.h)
#define PERSON_THUG_GREY 5

// uc_orig: PERSON_THUG_RED (fallen/Headers/Person.h)
#define PERSON_THUG_RED 6

// uc_orig: PERSON_SLAG_TART (fallen/Headers/Person.h)
#define PERSON_SLAG_TART 7

// uc_orig: PERSON_SLAG_FATUGLY (fallen/Headers/Person.h)
#define PERSON_SLAG_FATUGLY 8

// uc_orig: PERSON_HOSTAGE (fallen/Headers/Person.h)
#define PERSON_HOSTAGE 9

// uc_orig: PERSON_MECHANIC (fallen/Headers/Person.h)
#define PERSON_MECHANIC 10

// uc_orig: PERSON_TRAMP (fallen/Headers/Person.h)
#define PERSON_TRAMP 11

// uc_orig: PERSON_MIB1 (fallen/Headers/Person.h)
#define PERSON_MIB1 12

// uc_orig: PERSON_MIB2 (fallen/Headers/Person.h)
#define PERSON_MIB2 13

// uc_orig: PERSON_MIB3 (fallen/Headers/Person.h)
#define PERSON_MIB3 14

// uc_orig: PERSON_NUM_TYPES (fallen/Headers/Person.h)
#define PERSON_NUM_TYPES 15

// Max people pool size (hard cap used for save table sizing).
// uc_orig: RMAX_PEOPLE (fallen/Headers/Person.h)
#define RMAX_PEOPLE 180

// Runtime maximum: actual pool size from save_table at level load.
// uc_orig: MAX_PEOPLE (fallen/Headers/Person.h)
#define MAX_PEOPLE (save_table[SAVE_TABLE_PEOPLE].Maximum)

// ---- Person flags (FLAG_PERSON_*) ----
// Stored in Person.Flags / Thing.Flags (32-bit).

// uc_orig: FLAG_PERSON_NON_INT_M (fallen/Headers/Person.h)
#define FLAG_PERSON_NON_INT_M (1 << 0)

// uc_orig: FLAG_PERSON_NON_INT_C (fallen/Headers/Person.h)
#define FLAG_PERSON_NON_INT_C (1 << 1)

// uc_orig: FLAG_PERSON_LOCK_ANIM_CHANGE (fallen/Headers/Person.h)
#define FLAG_PERSON_LOCK_ANIM_CHANGE (1 << 2)

// uc_orig: FLAG_PERSON_GUN_OUT (fallen/Headers/Person.h)
#define FLAG_PERSON_GUN_OUT (1 << 3)

// InCar is THING_INDEX of vehicle being driven.
// uc_orig: FLAG_PERSON_DRIVING (fallen/Headers/Person.h)
#define FLAG_PERSON_DRIVING (1 << 4)

// uc_orig: FLAG_PERSON_SLIDING (fallen/Headers/Person.h)
#define FLAG_PERSON_SLIDING (1 << 5)

// After doing your talk anim, don't return to normal processing.
// uc_orig: FLAG_PERSON_NO_RETURN_TO_NORMAL (fallen/Headers/Person.h)
#define FLAG_PERSON_NO_RETURN_TO_NORMAL (1 << 6)

// uc_orig: FLAG_PERSON_HIT_WALL (fallen/Headers/Person.h)
#define FLAG_PERSON_HIT_WALL (1 << 7)

// If player presses use near this person, something happens.
// uc_orig: FLAG_PERSON_USEABLE (fallen/Headers/Person.h)
#define FLAG_PERSON_USEABLE (1 << 8)

// uc_orig: FLAG_PERSON_REQUEST_KICK (fallen/Headers/Person.h)
#define FLAG_PERSON_REQUEST_KICK (1 << 9)

// uc_orig: FLAG_PERSON_REQUEST_JUMP (fallen/Headers/Person.h)
#define FLAG_PERSON_REQUEST_JUMP (1 << 10)

// uc_orig: FLAG_PERSON_NAV_TO_KILL (fallen/Headers/Person.h)
#define FLAG_PERSON_NAV_TO_KILL (1 << 11)

// uc_orig: FLAG_PERSON_ON_CABLE (fallen/Headers/Person.h)
#define FLAG_PERSON_ON_CABLE (1 << 12)

// Carrying the grappling hook.
// uc_orig: FLAG_PERSON_GRAPPLING (fallen/Headers/Person.h)
#define FLAG_PERSON_GRAPPLING (1 << 13)

// Incapable of doing any AI.
// uc_orig: FLAG_PERSON_HELPLESS (fallen/Headers/Person.h)
#define FLAG_PERSON_HELPLESS (1 << 14)

// Carrying a coke-can.
// uc_orig: FLAG_PERSON_CANNING (fallen/Headers/Person.h)
#define FLAG_PERSON_CANNING (1 << 15)

// uc_orig: FLAG_PERSON_REQUEST_PUNCH (fallen/Headers/Person.h)
#define FLAG_PERSON_REQUEST_PUNCH (1 << 16)

// uc_orig: FLAG_PERSON_REQUEST_BLOCK (fallen/Headers/Person.h)
#define FLAG_PERSON_REQUEST_BLOCK (1 << 17)

// uc_orig: FLAG_PERSON_FALL_BACKWARDS (fallen/Headers/Person.h)
#define FLAG_PERSON_FALL_BACKWARDS (1 << 18)

// InCar is THING_INDEX of bike being ridden.
// uc_orig: FLAG_PERSON_BIKING (fallen/Headers/Person.h)
#define FLAG_PERSON_BIKING (1 << 19)

// InCar is the THING_INDEX of the vehicle we are a passenger in.
// uc_orig: FLAG_PERSON_PASSENGER (fallen/Headers/Person.h)
#define FLAG_PERSON_PASSENGER (1 << 20)

// This person has been hit.
// uc_orig: FLAG_PERSON_HIT (fallen/Headers/Person.h)
#define FLAG_PERSON_HIT (1 << 21)

// uc_orig: FLAG_PERSON_SPRINTING (fallen/Headers/Person.h)
#define FLAG_PERSON_SPRINTING (1 << 22)

// This person is wanted by the police.
// uc_orig: FLAG_PERSON_FELON (fallen/Headers/Person.h)
#define FLAG_PERSON_FELON (1 << 23)

// uc_orig: FLAG_PERSON_PEEING (fallen/Headers/Person.h)
#define FLAG_PERSON_PEEING (1 << 24)

// uc_orig: FLAG_PERSON_SEARCHED (fallen/Headers/Person.h)
#define FLAG_PERSON_SEARCHED (1 << 25)

// uc_orig: FLAG_PERSON_ARRESTED (fallen/Headers/Person.h)
#define FLAG_PERSON_ARRESTED (1 << 26)

// Holding a barrel.
// uc_orig: FLAG_PERSON_BARRELING (fallen/Headers/Person.h)
#define FLAG_PERSON_BARRELING (1 << 27)

// uc_orig: FLAG_PERSON_MOVE_ANGLETO (fallen/Headers/Person.h)
#define FLAG_PERSON_MOVE_ANGLETO (1 << 28)

// uc_orig: FLAG_PERSON_KO (fallen/Headers/Person.h)
#define FLAG_PERSON_KO (1 << 29)

// This person is inside a warehouse given by their "Ware" field.
// uc_orig: FLAG_PERSON_WAREHOUSE (fallen/Headers/Person.h)
#define FLAG_PERSON_WAREHOUSE (1 << 30)

// An enemy is killing someone with ulterior motives.
// uc_orig: FLAG_PERSON_KILL_WITH_A_PURPOSE (fallen/Headers/Person.h)
#define FLAG_PERSON_KILL_WITH_A_PURPOSE (1 << 31)

// ---- Person flags 2 (FLAG2_PERSON_*) ----
// Stored in Person.Flags2 (UBYTE, 8 bits).

// uc_orig: FLAG2_PERSON_LOOK (fallen/Headers/Person.h)
#define FLAG2_PERSON_LOOK (1 << 0)

// Set once a sound has started playing for that anim.
// uc_orig: FLAG2_SYNC_SOUNDFX (fallen/Headers/Person.h)
#define FLAG2_SYNC_SOUNDFX (1 << 1)

// uc_orig: FLAG2_PERSON_GUILTY (fallen/Headers/Person.h)
#define FLAG2_PERSON_GUILTY (1 << 2)

// uc_orig: FLAG2_PERSON_INVULNERABLE (fallen/Headers/Person.h)
#define FLAG2_PERSON_INVULNERABLE (1 << 3)

// uc_orig: FLAG2_PERSON_IS_MURDERER (fallen/Headers/Person.h)
#define FLAG2_PERSON_IS_MURDERER (1 << 4)

// uc_orig: FLAG2_PERSON_FAKE_WANDER (fallen/Headers/Person.h)
#define FLAG2_PERSON_FAKE_WANDER (1 << 5)

// Person's (HomeX,HomeZ) is inside a warehouse.
// uc_orig: FLAG2_PERSON_HOME_IN_WAREHOUSE (fallen/Headers/Person.h)
#define FLAG2_PERSON_HOME_IN_WAREHOUSE (1 << 6)

// uc_orig: FLAG2_PERSON_CARRYING (fallen/Headers/Person.h)
#define FLAG2_PERSON_CARRYING (1 << 7)

// ---- PTIME macro ----

// Access the GTimer field of a person Thing (short for Person Timer).
// uc_orig: PTIME (fallen/Headers/Person.h)
#define PTIME(p) (p->Genus.Person->GTimer)

// ---- Person struct ----

// Per-person runtime data. Accessed via Thing.Genus.Person.
// uc_orig: Person (fallen/Headers/Person.h)
typedef struct
{
    COMMON(PersonType)

    UBYTE Action;
    UBYTE SubAction;
    UBYTE Ammo;
    UBYTE PlayerID;

    // Current health (signed). 0 = dead. Can go negative on heavy damage.
    SWORD Health;
    UWORD Timer1;

    // Target: THING_INDEX of current AI target. InWay: thing blocking movement.
    THING_INDEX Target;
    THING_INDEX InWay;

    // InCar: THING_INDEX of vehicle/bike. FLAG_PERSON_DRIVING = car, FLAG_PERSON_BIKING = bike.
    THING_INDEX InCar;
    UWORD NavIndex;

    THING_INDEX SpecialList;
    THING_INDEX SpecialUse;
    THING_INDEX SpecialDraw;
    UBYTE pcom_colour;
    UBYTE pcom_group;

    MAV_Action MA;

    SWORD NavX, NavZ;

    ULONG sewerbits;

    UBYTE Power;
    UBYTE GTimer;
    UBYTE Mode;
    UBYTE GotoSpeed;

    UWORD HomeX;
    UWORD HomeZ;

    UWORD GotoX;
    UWORD GotoZ;

    UWORD muckyfootprint;
    SWORD Hold;

    UWORD UnderAttack;
    SBYTE Shove;
    UBYTE Balloon;

    UWORD OnFacet;
    UBYTE BurnIndex;
    SBYTE CombatNode;

    UBYTE pcom_bent;
    UBYTE pcom_ai;
    UBYTE pcom_ai_state;
    UBYTE pcom_ai_substate;

    UWORD pcom_ai_counter;
    UWORD pcom_ai_arg;

    UWORD pcom_ai_other;
    UBYTE pcom_ai_excar_state;
    UBYTE pcom_ai_excar_substate;

    UWORD pcom_ai_excar_arg;
    UBYTE pcom_move;
    UBYTE pcom_move_state;

    UBYTE pcom_move_substate;
    UBYTE pcom_move_flag;
    UWORD pcom_move_counter;

    UWORD pcom_move_arg;
    UWORD pcom_move_follow;

    MAV_Action pcom_move_ma;

    UBYTE AnimType;
    UBYTE InsideRoom;
    UWORD InsideIndex;

    UWORD pcom_ai_memory;
    UBYTE HomeYaw;
    UBYTE Stamina;

    UBYTE AttackAngle;
    UBYTE Escape;
    UBYTE Ware;
    UBYTE FightRating;
    SWORD TargetX;
    SWORD TargetZ;

    SWORD Agression;
    UBYTE ammo_packs_pistol;
    UBYTE GangAttack;

    UBYTE ammo_packs_shotgun;
    UBYTE ammo_packs_ak47;
    UBYTE drop;
    UBYTE pcom_zone;

    UBYTE pcom_lookat_what;
    UBYTE pcom_lookat_counter;
    UWORD pcom_lookat_index;

    UWORD Passenger;
    UBYTE Flags2;
    UBYTE SlideOdd;

    // World position of gun muzzle (PC-only). Updated during aiming. Used for bullet raycasts.
    GameCoord GunMuzzle;

} Person;

// uc_orig: PersonPtr (fallen/Headers/Person.h)
typedef Person* PersonPtr;

// ---- FightRating accessors ----

// Skill level is packed into the low 4 bits of FightRating.
// uc_orig: GET_SKILL (fallen/Headers/Person.h)
#define GET_SKILL(p) ((p)->Genus.Person->FightRating & 0xf)
// uc_orig: SET_SKILL (fallen/Headers/Person.h)
#define SET_SKILL(p, s) (p)->Genus.Person->FightRating = (((p)->Genus.Person->FightRating & 0xf0) | s)

// Max simultaneous attackers is in bits 4–5.
// uc_orig: GET_SKILL_GROUP (fallen/Headers/Person.h)
#define GET_SKILL_GROUP(p) (((p)->Genus.Person->FightRating & 0x30) >> 4)
// uc_orig: SET_SKILL_GROUP (fallen/Headers/Person.h)
#define SET_SKILL_GROUP(p, s) (p)->Genus.Person->FightRating = (((p)->Genus.Person->FightRating & (~0x30)) | (s << 4))

// Reserved bits 6–7.
// uc_orig: GET_SKILL_OTHER (fallen/Headers/Person.h)
#define GET_SKILL_OTHER(p) (((p)->Genus.Person->FightRating & 0xc0) >> 4)
// uc_orig: SET_SKILL_OTHER (fallen/Headers/Person.h)
#define SET_SKILL_OTHER(p, s) (p)->Genus.Person->FightRating = (((p)->Genus.Person->FightRating & (~0xc0)) | (s << 6))

// ---- Drop-down flags ----

// uc_orig: PERSON_DROP_DOWN_KEEP_VEL (fallen/Headers/Person.h)
#define PERSON_DROP_DOWN_KEEP_VEL (1 << 0)
// uc_orig: PERSON_DROP_DOWN_OFF_FACE (fallen/Headers/Person.h)
#define PERSON_DROP_DOWN_OFF_FACE (1 << 1)
// uc_orig: PERSON_DROP_DOWN_KEEP_DY (fallen/Headers/Person.h)
#define PERSON_DROP_DOWN_KEEP_DY (1 << 2)
// uc_orig: PERSON_DROP_DOWN_QUEUED (fallen/Headers/Person.h)
#define PERSON_DROP_DOWN_QUEUED (1 << 3)

// ---- Movement modes ----

// uc_orig: PERSON_MODE_RUN (fallen/Headers/Person.h)
#define PERSON_MODE_RUN 0
// uc_orig: PERSON_MODE_WALK (fallen/Headers/Person.h)
#define PERSON_MODE_WALK 1
// uc_orig: PERSON_MODE_SNEAK (fallen/Headers/Person.h)
#define PERSON_MODE_SNEAK 2
// uc_orig: PERSON_MODE_FIGHT (fallen/Headers/Person.h)
#define PERSON_MODE_FIGHT 3
// uc_orig: PERSON_MODE_SPRINT (fallen/Headers/Person.h)
#define PERSON_MODE_SPRINT 4
// uc_orig: PERSON_MODE_NUMBER (fallen/Headers/Person.h)
#define PERSON_MODE_NUMBER 5

// ---- Movement speeds ----

// uc_orig: PERSON_SPEED_WALK (fallen/Headers/Person.h)
#define PERSON_SPEED_WALK 1
// uc_orig: PERSON_SPEED_RUN (fallen/Headers/Person.h)
#define PERSON_SPEED_RUN 2
// uc_orig: PERSON_SPEED_SNEAK (fallen/Headers/Person.h)
#define PERSON_SPEED_SNEAK 3
// uc_orig: PERSON_SPEED_SPRINT (fallen/Headers/Person.h)
#define PERSON_SPEED_SPRINT 4
// uc_orig: PERSON_SPEED_YOMP (fallen/Headers/Person.h)
#define PERSON_SPEED_YOMP 5
// uc_orig: PERSON_SPEED_CRAWL (fallen/Headers/Person.h)
#define PERSON_SPEED_CRAWL 6

// ---- Ground surface types ----

// uc_orig: PERSON_ON_DUNNO (fallen/Headers/Person.h)
#define PERSON_ON_DUNNO 0
// uc_orig: PERSON_ON_WATER (fallen/Headers/Person.h)
#define PERSON_ON_WATER 1
// uc_orig: PERSON_ON_PRIM (fallen/Headers/Person.h)
#define PERSON_ON_PRIM 2
// uc_orig: PERSON_ON_GRAVEL (fallen/Headers/Person.h)
#define PERSON_ON_GRAVEL 3
// uc_orig: PERSON_ON_WOOD (fallen/Headers/Person.h)
#define PERSON_ON_WOOD 4
// uc_orig: PERSON_ON_GRASS (fallen/Headers/Person.h)
#define PERSON_ON_GRASS 5
// uc_orig: PERSON_ON_METAL (fallen/Headers/Person.h)
#define PERSON_ON_METAL 6
// uc_orig: PERSON_ON_SEWATER (fallen/Headers/Person.h)
#define PERSON_ON_SEWATER 7

// ---- Person lying position ----

// uc_orig: PERSON_ON_HIS_FRONT (fallen/Headers/Person.h)
#define PERSON_ON_HIS_FRONT 0
// uc_orig: PERSON_ON_HIS_BACK (fallen/Headers/Person.h)
#define PERSON_ON_HIS_BACK 1
// uc_orig: PERSON_ON_HIS_SOMETHING (fallen/Headers/Person.h)
#define PERSON_ON_HIS_SOMETHING 2

// ---- Death types ----

// uc_orig: PERSON_DEATH_TYPE_COMBAT (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_COMBAT 1
// uc_orig: PERSON_DEATH_TYPE_LAND (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_LAND 2
// uc_orig: PERSON_DEATH_TYPE_OTHER (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_OTHER 3
// Gets up again — not really dead.
// uc_orig: PERSON_DEATH_TYPE_STAY_ALIVE (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_STAY_ALIVE 4
// uc_orig: PERSON_DEATH_TYPE_LEG_SWEEP (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_LEG_SWEEP 5
// uc_orig: PERSON_DEATH_TYPE_PRONE (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_PRONE 6
// uc_orig: PERSON_DEATH_TYPE_STAY_ALIVE_PRONE (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_STAY_ALIVE_PRONE 7
// uc_orig: PERSON_DEATH_TYPE_COMBAT_PRONE (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_COMBAT_PRONE 8
// uc_orig: PERSON_DEATH_TYPE_SHOT_PISTOL (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_SHOT_PISTOL 10
// uc_orig: PERSON_DEATH_TYPE_SHOT_SHOTGUN (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_SHOT_SHOTGUN 11
// uc_orig: PERSON_DEATH_TYPE_SHOT_AK47 (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_SHOT_AK47 12
// Gets up again — not really dead.
// uc_orig: PERSON_DEATH_TYPE_GET_DOWN (fallen/Headers/Person.h)
#define PERSON_DEATH_TYPE_GET_DOWN 13

#endif // ACTORS_CHARACTERS_PERSON_TYPES_H
