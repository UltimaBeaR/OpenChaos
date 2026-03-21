#ifndef ACTORS_CORE_PLAYER_H
#define ACTORS_CORE_PLAYER_H

#include "core/types.h"
#include "core/vector.h"
#include "actors/core/state.h"

// Temporary: Player struct uses COMMON (needs THING_INDEX) and GameCoord.
// THING_INDEX is normally defined by Game.h before Structs.h; replicate here for standalone use.
#ifndef THING_INDEX
#define THING_INDEX UWORD
#endif
#include "fallen/Headers/Structs.h"

struct Thing;

// Player type identifiers.
// uc_orig: MAX_PLAYERS (fallen/Headers/Player.h)
#define MAX_PLAYERS 2
// uc_orig: PLAYER_NONE (fallen/Headers/Player.h)
#define PLAYER_NONE 0
// uc_orig: PLAYER_DARCI (fallen/Headers/Player.h)
#define PLAYER_DARCI 1
// uc_orig: PLAYER_ROPER (fallen/Headers/Player.h)
#define PLAYER_ROPER 2
// uc_orig: PLAYER_COP (fallen/Headers/Player.h)
#define PLAYER_COP 3
// uc_orig: PLAYER_THUG (fallen/Headers/Player.h)
#define PLAYER_THUG 4

// uc_orig: Player (fallen/Headers/Player.h)
// Per-player state: input, stats, references to the player's Thing and Person.
typedef struct
{
    COMMON(PlayerType)

    ULONG Input;
    ULONG InputDone;
    UWORD PlayerID;
    UBYTE Stamina;
    UBYTE Constitution;

    ULONG LastInput;
    ULONG ThisInput;
    ULONG Pressed;
    ULONG Released;
    ULONG DoneSomething;
    SLONG LastReleased[16];
    UBYTE DoubleClick[16];

    UBYTE Strength;
    UBYTE RedMarks;
    UBYTE TrafficViolations;
    UBYTE Danger;
    UBYTE PopupFade;
    SBYTE ItemFocus;
    UBYTE ItemCount;
    UBYTE Skill;

    struct Thing *CameraThing,
        *PlayerPerson;

} Player;

typedef Player* PlayerPtr;

// uc_orig: player_functions (fallen/Headers/Player.h)
// Maps PLAYER_* type to its state function table.
extern GenusFunctions player_functions[];

// uc_orig: player_pos (fallen/Source/Player.cpp)
// Last stored player position (currently unused — store_player_pos is a stub).
extern GameCoord player_pos;

// uc_orig: init_players (fallen/Headers/Player.h)
void init_players(void);

// uc_orig: alloc_player (fallen/Headers/Player.h)
Thing* alloc_player(UBYTE type);

// uc_orig: free_player (fallen/Headers/Player.h)
void free_player(Thing* player_thing);

// uc_orig: create_player (fallen/Headers/Player.h)
Thing* create_player(UBYTE type, SLONG x, SLONG y, SLONG z, SLONG id);

// uc_orig: PLAYER_redmark (fallen/Headers/Player.h)
// Adds dredmarks to player's red-mark count (0–10). At 10, triggers game over.
void PLAYER_redmark(SLONG playerid, SLONG dredmarks);

// uc_orig: store_player_pos (fallen/Source/Player.cpp)
void store_player_pos(void);

#endif // ACTORS_CORE_PLAYER_H
