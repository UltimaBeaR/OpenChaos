#ifndef THINGS_ITEMS_SPECIAL_H
#define THINGS_ITEMS_SPECIAL_H

#include "engine/core/types.h"

#include "things/core/common.h"

// Forward declaration: only pointer-to-Thing is used in function signatures.
struct Thing;

// MAX_SPECIALS uses save_table[SAVE_TABLE_SPECIAL].Maximum at runtime.
// MemTable is fully defined in Game.h; forward-declare it here to break the
// Game.h → Special.h → Game.h circular include.
#ifndef FALLEN_HEADERS_GAME_H
struct MemTable;
extern struct MemTable save_table[];
#endif
// uc_orig: SAVE_TABLE_SPECIAL (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_SPECIAL
#define SAVE_TABLE_SPECIAL 36
#endif

// uc_orig: SPECIAL_AMMO_IN_A_PISTOL (fallen/Headers/Special.h)
#define SPECIAL_AMMO_IN_A_PISTOL 15
// uc_orig: SPECIAL_AMMO_IN_A_SHOTGUN (fallen/Headers/Special.h)
#define SPECIAL_AMMO_IN_A_SHOTGUN 8
// uc_orig: SPECIAL_AMMO_IN_A_GRENADE (fallen/Headers/Special.h)
#define SPECIAL_AMMO_IN_A_GRENADE 3
// uc_orig: SPECIAL_AMMO_IN_A_AK47 (fallen/Headers/Special.h)
#define SPECIAL_AMMO_IN_A_AK47 30

// Maximum items alive simultaneously on a level (runtime value from save_table).
// uc_orig: RMAX_SPECIALS (fallen/Headers/Special.h)
#define RMAX_SPECIALS 260
// uc_orig: MAX_SPECIALS (fallen/Headers/Special.h)
#define MAX_SPECIALS (save_table[SAVE_TABLE_SPECIAL].Maximum)

// Item type constants.
// uc_orig: SPECIAL_NONE (fallen/Headers/Special.h)
#define SPECIAL_NONE 0
// uc_orig: SPECIAL_KEY (fallen/Headers/Special.h)
#define SPECIAL_KEY 1
// uc_orig: SPECIAL_GUN (fallen/Headers/Special.h)
#define SPECIAL_GUN 2
// uc_orig: SPECIAL_HEALTH (fallen/Headers/Special.h)
#define SPECIAL_HEALTH 3
// uc_orig: SPECIAL_BOMB (fallen/Headers/Special.h)
#define SPECIAL_BOMB 4
// uc_orig: SPECIAL_SHOTGUN (fallen/Headers/Special.h)
#define SPECIAL_SHOTGUN 5
// uc_orig: SPECIAL_KNIFE (fallen/Headers/Special.h)
#define SPECIAL_KNIFE 6
// uc_orig: SPECIAL_EXPLOSIVES (fallen/Headers/Special.h)
#define SPECIAL_EXPLOSIVES 7
// uc_orig: SPECIAL_GRENADE (fallen/Headers/Special.h)
#define SPECIAL_GRENADE 8
// uc_orig: SPECIAL_AK47 (fallen/Headers/Special.h)
#define SPECIAL_AK47 9
// uc_orig: SPECIAL_MINE (fallen/Headers/Special.h)
#define SPECIAL_MINE 10
// uc_orig: SPECIAL_THERMODROID (fallen/Headers/Special.h)
#define SPECIAL_THERMODROID 11
// uc_orig: SPECIAL_BASEBALLBAT (fallen/Headers/Special.h)
#define SPECIAL_BASEBALLBAT 12
// uc_orig: SPECIAL_AMMO_PISTOL (fallen/Headers/Special.h)
#define SPECIAL_AMMO_PISTOL 13
// uc_orig: SPECIAL_AMMO_SHOTGUN (fallen/Headers/Special.h)
#define SPECIAL_AMMO_SHOTGUN 14
// uc_orig: SPECIAL_AMMO_AK47 (fallen/Headers/Special.h)
#define SPECIAL_AMMO_AK47 15
// uc_orig: SPECIAL_KEYCARD (fallen/Headers/Special.h)
#define SPECIAL_KEYCARD 16
// uc_orig: SPECIAL_FILE (fallen/Headers/Special.h)
#define SPECIAL_FILE 17
// uc_orig: SPECIAL_FLOPPY_DISK (fallen/Headers/Special.h)
#define SPECIAL_FLOPPY_DISK 18
// uc_orig: SPECIAL_CROWBAR (fallen/Headers/Special.h)
#define SPECIAL_CROWBAR 19
// uc_orig: SPECIAL_VIDEO (fallen/Headers/Special.h)
#define SPECIAL_VIDEO 20
// uc_orig: SPECIAL_GLOVES (fallen/Headers/Special.h)
#define SPECIAL_GLOVES 21
// uc_orig: SPECIAL_WEEDAWAY (fallen/Headers/Special.h)
#define SPECIAL_WEEDAWAY 22
// uc_orig: SPECIAL_TREASURE (fallen/Headers/Special.h)
#define SPECIAL_TREASURE 23
// uc_orig: SPECIAL_CARKEY_RED (fallen/Headers/Special.h)
#define SPECIAL_CARKEY_RED 24
// uc_orig: SPECIAL_CARKEY_BLUE (fallen/Headers/Special.h)
#define SPECIAL_CARKEY_BLUE 25
// uc_orig: SPECIAL_CARKEY_GREEN (fallen/Headers/Special.h)
#define SPECIAL_CARKEY_GREEN 26
// uc_orig: SPECIAL_CARKEY_BLACK (fallen/Headers/Special.h)
#define SPECIAL_CARKEY_BLACK 27
// uc_orig: SPECIAL_CARKEY_WHITE (fallen/Headers/Special.h)
#define SPECIAL_CARKEY_WHITE 28
// uc_orig: SPECIAL_WIRE_CUTTER (fallen/Headers/Special.h)
#define SPECIAL_WIRE_CUTTER 29
// uc_orig: SPECIAL_NUM_TYPES (fallen/Headers/Special.h)
#define SPECIAL_NUM_TYPES 30

// Item group flags used for behavioural classification.
// uc_orig: SPECIAL_GROUP_USEFUL (fallen/Headers/Special.h)
#define SPECIAL_GROUP_USEFUL 1
// uc_orig: SPECIAL_GROUP_ONEHANDED_WEAPON (fallen/Headers/Special.h)
#define SPECIAL_GROUP_ONEHANDED_WEAPON 2
// uc_orig: SPECIAL_GROUP_TWOHANDED_WEAPON (fallen/Headers/Special.h)
#define SPECIAL_GROUP_TWOHANDED_WEAPON 3
// uc_orig: SPECIAL_GROUP_STRANGE (fallen/Headers/Special.h)
#define SPECIAL_GROUP_STRANGE 4
// uc_orig: SPECIAL_GROUP_AMMO (fallen/Headers/Special.h)
#define SPECIAL_GROUP_AMMO 5
// uc_orig: SPECIAL_GROUP_COOKIE (fallen/Headers/Special.h)
#define SPECIAL_GROUP_COOKIE 6

// Per-type display name, mesh ID, and group.
// uc_orig: SPECIAL_Info (fallen/Headers/Special.h)
typedef struct
{
    CBYTE* name;
    UBYTE prim;
    UBYTE group;
} SPECIAL_Info;

// uc_orig: SPECIAL_info (fallen/Headers/Special.h)
extern SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES];

// Item substate constants.
// uc_orig: SPECIAL_SUBSTATE_NONE (fallen/Headers/Special.h)
#define SPECIAL_SUBSTATE_NONE 0
// uc_orig: SPECIAL_SUBSTATE_ACTIVATED (fallen/Headers/Special.h)
#define SPECIAL_SUBSTATE_ACTIVATED 1
// uc_orig: SPECIAL_SUBSTATE_IS_DIRT (fallen/Headers/Special.h)
#define SPECIAL_SUBSTATE_IS_DIRT 2
// uc_orig: SPECIAL_SUBSTATE_PROJECTILE (fallen/Headers/Special.h)
#define SPECIAL_SUBSTATE_PROJECTILE 3

// The per-item data block stored in Thing->Genus.Special.
// uc_orig: Special (fallen/Headers/Special.h)
typedef struct
{
    COMMON(SpecialType)

    THING_INDEX NextSpecial,
        OwnerThing;

    UWORD ammo;    // ammo count in the magazine, or countdown for an activated mine
    UWORD waypoint; // waypoint that spawned this item, or 0; also dirt index for SUBSTATE_IS_DIRT

    UWORD counter;
    UWORD timer;   // grenade fuse countdown (16*20 ticks/sec); also encodes velocity in PROJECTILE substate

} Special;

// uc_orig: SpecialPtr (fallen/Headers/Special.h)
typedef Special* SpecialPtr;

// Finds a free slot in the SPECIALS pool; returns its index or NULL if full.
// uc_orig: find_empty_special (fallen/Source/Special.cpp)
SLONG find_empty_special(void);

// Initialises the specials pool at level start.
// uc_orig: init_specials (fallen/Headers/Special.h)
void init_specials(void);

// Creates an item Thing at world_xyz (8-bits per mapsquare). waypoint=0 if not spawned by a script.
// uc_orig: alloc_special (fallen/Headers/Special.h)
Thing* alloc_special(
    UBYTE type,
    UBYTE substate,
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    UWORD waypoint);

// Removes a carried special from person's inventory and places it in the world.
// uc_orig: special_drop (fallen/Headers/Special.h)
void special_drop(Thing* p_special, Thing* p_person);

// Returns the special Thing of the given type that p_person is carrying, or NULL.
// uc_orig: person_has_special (fallen/Headers/Special.h)
Thing* person_has_special(Thing* p_person, SLONG special_type);

// Returns UC_TRUE if p_person can and should pick up p_special (ignores distance).
// uc_orig: should_person_get_item (fallen/Headers/Special.h)
SLONG should_person_get_item(Thing* p_person, Thing* p_special);

// Executes item pickup: merges ammo, updates inventory, shows HUD message, destroys item.
// uc_orig: person_get_item (fallen/Headers/Special.h)
void person_get_item(Thing* p_person, Thing* p_special);

// Arms the grenade: sets ACTIVATED substate and 6-second fuse timer.
// uc_orig: SPECIAL_prime_grenade (fallen/Headers/Special.h)
void SPECIAL_prime_grenade(Thing* p_special);

// Converts a carried primed grenade into a physics projectile.
// uc_orig: SPECIAL_throw_grenade (fallen/Headers/Special.h)
void SPECIAL_throw_grenade(Thing* p_special);

// Arms and places a C4 explosive block at person's current position with a 5-second fuse.
// uc_orig: SPECIAL_set_explosives (fallen/Headers/Special.h)
void SPECIAL_set_explosives(Thing* p_person);

// Destroys a Special Thing: clears pool slot, frees mesh, removes from map.
// uc_orig: free_special (fallen/Source/Special.cpp)
void free_special(Thing* special_thing);

// Per-tick state function for all CLASS_SPECIAL Things.
// uc_orig: special_normal (fallen/Source/Special.cpp)
void special_normal(Thing* s_thing);

// Adds a special to person's inventory linked list. Caller must remove the Thing from the map.
// uc_orig: special_pickup (fallen/Source/Special.cpp)
void special_pickup(Thing* p_special, Thing* p_person);

// Detonates a mine at its current position (shockwave + pyro effect).
// uc_orig: special_activate_mine (fallen/Source/Special.cpp)
void special_activate_mine(Thing* p_mine);

// Draws extra visual effects for a Special Thing (bloom for mines and explosives).
// Called from the render loop after drawing the Thing mesh.
// uc_orig: DRAWXTRA_Special (fallen/DDEngine/Source/drawxtra.cpp)
void DRAWXTRA_Special(Thing* p_thing);

#endif // THINGS_ITEMS_SPECIAL_H
