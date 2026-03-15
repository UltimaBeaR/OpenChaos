// Game.h
// Guy Simmons, 17th October 1997.
// claude-ai: OVERVIEW — Game.h
// claude-ai: THE central game header — included by almost every .cpp in the project.
// claude-ai: Defines the `Game` struct (typedef'd, no tag) which holds ALL game-wide state:
// claude-ai:   - simulation state (GameState, GameTurn, TickTock, RandSeed)
// claude-ai:   - object pool pointers (People, Vehicles, etc.)
// claude-ai:   - the world map (MAP array, MAP_WIDTH * MAP_HEIGHT (128x128) cells)
// claude-ai:   - player RPG stats (Darci/Roper Strength/Constitution/Skill/Stamina)
// claude-ai:   - indoors tracking (indoors_height_floor/ceiling, room index, building index)
// claude-ai:   - gameplay flags (GameFlags, CrimeRate, MusicWorld, etc.)
// claude-ai: One global instance: `the_game` (Game.cpp).
// claude-ai: Everything is accessed via macros like PEOPLE, MAP, GAME_TURN, etc.
// claude-ai:
// claude-ai: DRAW DISTANCE:
// claude-ai:   DRAW_DIST = 22 on PC, 13 on PSX — controls how many map squares are rendered.
// claude-ai:
// claude-ai: THING_INDEX / COMMON_INDEX:
// claude-ai:   UWORD (16-bit unsigned) on PC/D3D.
// claude-ai:   SWORD (16-bit signed) on PSX.
// claude-ai:   SLONG (32-bit) on Glide (commented out).
// claude-ai:   These are indices into the object pools (Things[], People[], etc.).

#ifndef GAME_H
#define GAME_H

#define DRAW_DIST 22

//---------------------------------------------------------------
// PC - D3D Specific defines.


#pragma warning(disable : 4244) // truncation warning : useless
#pragma warning(disable : 4101) // unreferenced local : no-one ever bothers deleting these
#pragma warning(disable : 4554) // yes, well ... some people use brackets, others disable this warning

#define THING_INDEX UWORD
#define COMMON_INDEX UWORD

//---------------------------------------------------------------
// PSX specific defines.
/*
#define	VERSION_PsX
#define	THING_INDEX			SWORD
#define	COMMON_INDEX		SWORD
*/
//---------------------------------------------------------------

#include <MFStdLib.h>
#undef LogText
#define LogText

typedef SLONG MAPCO8;
typedef SLONG MAPCO16;
typedef SLONG MAPCO24;

//
// Memory table, the source of all our power
//
struct MemTable {
    CBYTE* Name;
    void** Point;
    UBYTE Type;
    SLONG* CountL;
    UWORD* CountW;
    SLONG Maximum;
    SWORD StructSize;
    SWORD Extra;
};
extern struct MemTable save_table[];

#define SAVE_TABLE_VEHICLE 29
#define SAVE_TABLE_PEOPLE 30
#define SAVE_TABLE_SPECIAL 36
#define SAVE_TABLE_BAT 38
#define SAVE_TABLE_DTWEEN 40
#define SAVE_TABLE_DMESH 41
#define SAVE_TABLE_WMOVE 46
#define SAVE_TABLE_POINTS 15
#define SAVE_TABLE_FACE4 16
#define SAVE_TABLE_PLATS 45

#include "Structs.h"
#include "State.h"
// #include	"Level.h"

#include "drawtype.h"

#include "building.h"
#include "bike.h"
#include "Furn.h"
#include "Vehicle.h"
#include "inline.h"
#include "Person.h"
#include "Animal.h"
#include "barrel.h"
#include "Chopper.h"
#include "Pyro.h"
#include "Player.h"
#include "Plat.h"
#include "Pjectile.h"
#include "Special.h"
#include "bat.h"
#include "Switch.h"
#include "tracks.h"
#include "Thing.h"
#include "Controls.h"
#include "Map.h"
#include "collide.h"
#include "interact.h"

#ifdef VERSION_D3D
#include "aeng.h"
#endif


#ifdef EDITOR
// #include	"Editor.h"
#endif

//---------------------------------------------------------------
#define MAX_RADIUS (24)

// Thing classes.
#define CLASS_NONE 0
#define CLASS_PLAYER 1
#define CLASS_CAMERA 2
#define CLASS_PROJECTILE 3
#define CLASS_BUILDING 4
#define CLASS_PERSON 5
#define CLASS_ANIMAL 6
#define CLASS_FURNITURE 7
#define CLASS_SWITCH 8
#define CLASS_VEHICLE 9
#define CLASS_SPECIAL 10
#define CLASS_ANIM_PRIM 11
#define CLASS_CHOPPER 12
#define CLASS_PYRO 13
#define CLASS_TRACK 14
#define CLASS_PLAT 15
#define CLASS_BARREL 16
#define CLASS_BIKE 17
#define CLASS_BAT 18
#define CLASS_END 19

// face flags

#define POLY_FLAG_GOURAD (1 << 0)
#define POLY_FLAG_TEXTURED (1 << 1)
#define POLY_FLAG_MASKED (1 << 2)
#define POLY_FLAG_SEMI_TRANS (1 << 3)
#define POLY_FLAG_ALPHA (1 << 4)
#define POLY_FLAG_TILED (1 << 5)
#define POLY_FLAG_DOUBLESIDED (1 << 6)
#define POLY_FLAG_WALKABLE (1 << 7)

//---------------------------------------------------------------

// claude-ai: GAME STATES (GameState bitmask):
// claude-ai:   GS_ATTRACT_MODE — title/demo screen loop
// claude-ai:   GS_PLAY_GAME    — normal gameplay
// claude-ai:   GS_LEVEL_WON / GS_LEVEL_LOST — end-of-mission state
// claude-ai:   GS_GAME_WON     — entire game completed
// claude-ai:   GS_RECORD / GS_PLAYBACK / GS_REPLAY — demo recording/playback
// claude-ai:   GS_EDITOR       — in-level editor mode (bit 16, separate from gameplay bits)
// Game States.
#define GS_ATTRACT_MODE (1 << 0)
#define GS_PLAY_GAME (1 << 1)
#define GS_CONFIGURE_NET (1 << 2)
#define GS_LEVEL_WON (1 << 3)
#define GS_LEVEL_LOST (1 << 4)
#define GS_GAME_WON (1 << 5)
#define GS_RECORD (1 << 6)
#define GS_PLAYBACK (1 << 7)
#define GS_REPLAY (1 << 8)

#define GS_EDITOR (1 << 16)

// claude-ai: GAME FLAGS (GameFlags bitmask) — per-frame environmental state:
// claude-ai:   GF_INDOORS      — player is currently inside a building (triggers indoor ambient)
// claude-ai:   GF_SEWERS       — player is in the sewer network
// claude-ai:   GF_PAUSED       — game is paused (input still runs)
// claude-ai:   GF_RAINING      — rain is active (affects splash particles, detail level)
// claude-ai:   GF_WINDY        — wind is active
// claude-ai:   GF_SHOW_MAP     — minimap overlay visible
// claude-ai:   GF_SHOW_CRIMERATE — crime rate HUD overlay visible
// claude-ai:   GF_PLAYER_FIRED_GUN — set when player shoots; triggers attention/alert in NPCs
// claude-ai:   GF_CONE_PENALTIES   — NPC vision cone penalties active
// claude-ai:   GF_CARS_WITH_ROAD_PRIMS — vehicles use road primitive data
// claude-ai:   GF_SIDE_ON_COMBAT   — side-scrolling combat mode
// claude-ai:   GF_NO_FLOOR         — level has no floor geometry (sky level etc.)
// claude-ai:   GF_DISABLE_BENCH_HEALTH — benches don't restore health
// Game Flags.
#define GF_INDOORS (1 << 0)
#define GF_SEWERS (1 << 1)
#define GF_PAUSED (1 << 2)
#define GF_RAINING (1 << 3)
#define GF_WINDY (1 << 4)
#define GF_SHOW_MAP (1 << 5)
#define GF_SHOW_CRIMERATE (1 << 6)
#define GF_PLAYER_FIRED_GUN (1 << 7)
#define GF_CONE_PENALTIES (1 << 8)
#define GF_CARS_WITH_ROAD_PRIMS (1 << 9)
#define GF_SIDE_ON_COMBAT (1 << 10)
#define GF_NO_FLOOR (1 << 11)
#define GF_DISABLE_BENCH_HEALTH (1 << 12)

// claude-ai: SEASONS — stored in Game::Season (0..3). Controls sky colour and day length.
// claude-ai:   Not fully implemented in this pre-release codebase; Season may be set but effects are minor.
#define GAME_SEASON_SPRING 0
#define GAME_SEASON_SUMMER 1
#define GAME_SEASON_AUTUMN 2
#define GAME_SEASON_WINTER 3

// claude-ai: TIME OF DAY — Game::Time is a 24-hour clock in 8-bit fixed point (1.0 = 256 units).
// claude-ai:   GAME_TIME_DAWN = 5 (5:00 AM)  — start of dawn, ambient light begins rising
// claude-ai:   GAME_TIME_DUSK = 20 (8:00 PM) — start of dusk, ambient light starts falling
// claude-ai:   At night: NIGHT_FLAG_NIGHTTIME set, lamppost/streetlight dlights become active.
// claude-ai:   Time advances each game tick via NIGHT system (see night.cpp for actual progression).
#define GAME_TIME_DAWN 5
#define GAME_TIME_DUSK 20

#define DETAIL_DIRT1 (1 << 0)
#define DETAIL_DIRT2 (1 << 1)
#define DETAIL_PUDDLE (1 << 2)
#define DETAIL_REFLECTIONS (1 << 3)
#define DETAIL_SHADOWS (1 << 4)
#define DETAIL_SPLASH (1 << 5)
#define DETAIL_MIST (1 << 6)
#define DETAIL_RAIN (1 << 7)

// claude-ai: DETAIL FLAGS — bit flags controlling which visual detail effects are enabled.
// claude-ai:   Stored in Game::DetailLevel (UWORD). Checked before spawning particles, reflections, etc.
// claude-ai:   DETAIL_RAIN / DETAIL_SPLASH / DETAIL_MIST — weather effects
// claude-ai:   DETAIL_REFLECTIONS / DETAIL_SHADOWS — visual quality toggles
// claude-ai:   DETAIL_DIRT1/DIRT2 / DETAIL_PUDDLE — ground decal detail

// Game structure.
typedef struct
{
    UBYTE red;
    UBYTE green;
    UBYTE blue;

} ENGINE_Col;
// claude-ai: ENGINE_Col — 24-bit RGB colour used in the Gouraud palette (GamePal[256]).
// claude-ai: GamePal is the global "gourad palette" — used for palette-based vertex colouring on PSX.

// claude-ai: struct Game — THE single global game state struct. One instance: `the_game`.
// claude-ai: Accessed everywhere via macros (GAME_STATE, PEOPLE, MAP, etc.)
// claude-ai: In new game: split into proper subsystem objects. Do NOT port as a single monolith.
typedef struct
{
    // claude-ai: GameState  — bitmask of GS_* flags (current game mode: play/menu/record/etc.)
    // claude-ai: GameTurn   — frame counter; increments every tick; used for cache invalidation (LIGHT_context_gameturn).
    // claude-ai: GameFlags  — bitmask of GF_* flags (current environmental state per frame).
    SLONG GameState,
        GameTurn,
        GameFlags,

        // claude-ai: Object counts — number of active objects of each type in the pools.
        // claude-ai: These are NOT pool sizes; actual pool sizes are defined as MAX_* in headers.
        // claude-ai: Checked before iterating, e.g. `for (i=0; i<PERSON_COUNT; i++)`.
        CameraCount,
        PersonCount,
        PlayerCount,
        AnimalCount,
        ChopperCount,
        PyroCount,
        ProjectileCount,
        SpecialCount,
        SwitchCount,
        BatCount,

        // claude-ai: DrawTweenCount / DrawMeshCount — active animated objects (tweened vertex and mesh).
        // claude-ai: PrimaryThingCount / SecondaryThingCount — Things linked list counts (primary = full Things, secondary = lightweight).
        DrawTweenCount,
        DrawMeshCount,
        PrimaryThingCount,
        SecondaryThingCount,
        // claude-ai: TickTock    — milliseconds per frame (target = NORMAL_TICK_TOCK = 1000/15 ≈ 67ms = 15Hz).
        // claude-ai: TickRatio   — (TickTock << TICK_SHIFT) / NORMAL_TICK_TOCK; scaled so 1.0 = 256 (fixed-point 8).
        // claude-ai:               Multiply physics/motion values by TickRatio >> TICK_SHIFT to get frame-rate independent motion.
        // claude-ai: TickInvRatio — inverse of TickRatio for division-free scaling.
        TickTock,
        TickRatio,
        TickInvRatio,
        // claude-ai: RandSeed — deterministic RNG state. Updated by Random() macro: seed = seed*69069 + 1.
        // claude-ai:            Critical for multiplayer sync — must use same seed on all clients.
        RandSeed,
        // claude-ai: Time   — game clock, 24-hour format, fixed-point 8 (256 units = 1 hour).
        // claude-ai:          Range 0..6143 (24*256). Dawn at 5*256=1280, Dusk at 20*256=5120.
        // claude-ai:          Set/read via GAME_TIME macro (note: macro references the_game.GameTime but field is `Time` — macro may be wrong/unused).
        // claude-ai: Season — 0=Spring, 1=Summer, 2=Autumn, 3=Winter. Controls sky colour presets.
        Time, // In 24-hours in fixed point 8...
        Season;

    // Map members.
    // claude-ai: Map — the main world collision/occupation grid.
    // claude-ai:   Each MapElement holds: MapWho (linked list of Things occupying this cell), ColVectHead (collision vectors), flags.
    // claude-ai:   Size is MAP_SIZE = MAP_HEIGHT * MAP_WIDTH (typically 128x128 = 16384 cells).
    // claude-ai:   On PSX/DC: Map[1] (dynamically allocated elsewhere to save BSS), on PC: full static array.
    // claude-ai:   Access via MAP macro, MAP2(x,z) macro, or MAP_WHO(i) for occupation list.
    MapElement Map[MAP_SIZE];

    // Thing members.
    // claude-ai: Object pool pointers — all allocated at level load time (see save_table[]).
    // claude-ai: Each pool is a flat array; objects are linked into free/used lists via Thing.Next indices.
    // claude-ai: Pool sizes defined as MAX_VEHICLES, MAX_PEOPLE, etc. in respective headers.
    // claude-ai: Accessed via VEHICLES, PEOPLE, THINGS macros. Cast via TO_VEHICLE(i), TO_PERSON(i) etc.
    Vehicle* Vehicles; //[MAX_VEHICLES];
    Furniture* Furnitures; //[MAX_FURNITURE];
    Person* People; //[MAX_PEOPLE];
    Animal* Animals; //[MAX_ANIMALS];
    Chopper* Choppers; //[MAX_CHOPPERS];
    Pyro* Pyros; //[MAX_PYROS];
    Player* Players; //[MAX_PLAYERS];
    Projectile* Projectiles; //[MAX_PROJECTILES];
    Special* Specials; //[MAX_SPECIALS];
    Switch* Switches; //[MAX_SWITCHES];
    Bat* Bats; //[BAT_MAX_BATS];
    Thing* Things; //[MAX_THINGS];

    //
    // The gourad palette
    //
    // claude-ai: GamePal[256] — 256-entry RGB palette used for Gouraud-shaded rendering on PSX.
    // claude-ai: On PC/D3D this is present but vertex colours are computed differently (per NIGHT system).
    // claude-ai: Accessed via ENGINE_palette macro.
    ENGINE_Col GamePal[256];

    // draw types
    //  claude-ai: DrawTweens / DrawMeshes — pools for animated geometry draw descriptors.
    //  claude-ai:   DrawTween: keyframe-interpolated vertex animation (characters, doors, etc.)
    //  claude-ai:   DrawMesh:  rigid mesh with transform (Angle/Tilt/Roll orientation).
    DrawTween* DrawTweens; //[MAX_DRAW_TWEENS];
    DrawMesh* DrawMeshes; //[MAX_DRAW_MESHES];

    // claude-ai: UsedPrimaryThings / UnusedPrimaryThings — head indices of linked lists for primary Thing pool.
    // claude-ai: Same pattern for Secondary. Secondary Things are lightweight (no physics, just renderable markers).
    THING_INDEX UsedPrimaryThings,
        UnusedPrimaryThings,
        UsedSecondaryThings,
        UnusedSecondaryThings;

    // claude-ai: net_persons / net_players — pointers to Things for multiplayer remote entities.
    // claude-ai: Up to 10 remote players. Only valid during multiplayer game (GS_PLAY_GAME with NumberPlayers > 1).
    struct Thing** net_persons; //[10];
    struct Thing** net_players; //[10];

    // claude-ai: INDOORS TRACKING — set when player is inside a building storey.
    // claude-ai:   indoors_height_floor/ceiling — vertical bounds of current storey (fixed-point world units).
    // claude-ai:   indoors_dbuilding — index into dbuildings[] of the building the player is currently inside.
    // claude-ai:   indoors_index — which building "inside slot" is active (maps to inside lighting set).
    // claude-ai:   indoors_index_fade / _fade_ext / _fade_ext_dir — transition blend indices for entering/leaving buildings.
    // claude-ai:   indoors_room / indoors_room_next — current room index for multi-room buildings.
    // claude-ai:   indoors_index_next — next inside index (used during transitions).
    // claude-ai: These control which NIGHT_cache square to use: exterior vs interior lighting.
    // claude-ai: On Dreamcast: indoors_index_next, INDOORS_INDEX, INDOORS_ROOM all return 0 (hardcoded via macros).
    SLONG indoors_height_floor; // The heights of the storey you are in.
    SLONG indoors_height_ceiling;
    SLONG indoors_dbuilding; // The index of the FBuilding you are in.
    UWORD indoors_index_fade;
    UWORD indoors_index_fade_ext;
    UWORD indoors_index_fade_ext_dir;
    UWORD indoors_index_next;
    UWORD indoors_index;
    UWORD indoors_room;
    UWORD indoors_room_next;
    // claude-ai: NumberPlayers — actual number of connected players (1 = single player, 2+ = multiplayer).
    // claude-ai: Packets[16] / Scores[16] — per-player network packet data and scores (up to 16 players).
    // claude-ai: MyID — this client's player ID (0-based) in multiplayer.
    SLONG NumberPlayers;
    ULONG Packets[16];
    SLONG Scores[16];
    SLONG MyID;
    // claude-ai: UserInterface — UI mode selector (which HUD/menu is showing). Accessed via USER_INTERFACE macro.
    // claude-ai: DetailLevel — bitmask of DETAIL_* flags controlling visual quality toggles (rain, shadows, etc.).
    // claude-ai: TextureSet — which texture set to load for this level (different cities/themes). Accessed via TEXTURE_SET.
    SLONG UserInterface;
    UWORD DetailLevel;
    UWORD TextureSet;
    // claude-ai: CrimeRate — current crime level (0..100+). Loaded from level file via elev.cpp.
    // claude-ai:             Affects mission score multiplier and police response intensity.
    // claude-ai:             Accessed via CRIME_RATE macro. Modified by player actions during play.
    // claude-ai: CrimeRateScoreMul — score multiplier derived from CrimeRate. Accessed via CRIME_RATE_SCORE_MUL.
    SLONG CrimeRate;
    SLONG CrimeRateScoreMul;
    // claude-ai: FakeCivs — number of fake (scripted, non-AI) civilian Things active. Used by crowd system.
    // claude-ai: SaveValid — non-zero if a valid save game exists for this slot. Checked before load.
    // claude-ai: MusicWorld — index of the music set/world to use for this level. Accessed via MUSIC_WORLD.
    // claude-ai: BoredomRate — how quickly NPCs become bored/wander. Accessed via BOREDOM_RATE.
    UBYTE FakeCivs;
    UBYTE SaveValid;
    UBYTE MusicWorld;
    UBYTE BoredomRate;

    // claude-ai: PLAYER RPG STATS — persistent character attributes for both playable characters.
    // claude-ai:   Darci Mason (female protagonist) and Roper (male partner).
    // claude-ai:   Range: 0..255 each. Affect combat damage, health cap, hit chance, stamina recovery.
    // claude-ai:   Strength:      melee/weapon damage multiplier
    // claude-ai:   Constitution:  max health
    // claude-ai:   Skill:         accuracy / hit probability
    // claude-ai:   Stamina:       fatigue resistance / sprint duration
    // claude-ai:   Saved/loaded with game save. Persist across missions.
    UBYTE DarciStrength;
    UBYTE DarciConstitution;
    UBYTE DarciSkill;
    UBYTE DarciStamina;
    UBYTE RoperStrength;
    UBYTE RoperConstitution;
    UBYTE RoperSkill;
    UBYTE RoperStamina;

    // claude-ai: DarciDeadCivWarnings — running count of civilian kills by player.
    // claude-ai:   Triggers warnings/mission failure when threshold exceeded.
    // claude-ai:   Reset on new mission. Checked in person death logic.
    UBYTE DarciDeadCivWarnings;
    UBYTE padding[2];

} Game;
// claude-ai: NOTE: Game does NOT contain mission_hierarchy[] or best_found[][] or complete_point.
// claude-ai: Those fields (mission progress, best results) are stored in a separate save-game struct,
// claude-ai: NOT in `Game`. They are loaded/saved by elev.cpp / save.cpp into a different global.

extern Game the_game;

extern UBYTE VIOLENCE;

// Defines for 'Game' member access.

//
// Multiplayer stuff
//

#define USER_INTERFACE the_game.UserInterface
#define NO_PLAYERS CNET_num_players
#define PACKET_DATA(x) the_game.Packets[x]
#define PLAYER_ID CNET_player_id
#define GAME_SCORE(x) the_game.Scores[x]
#define MY_SCORE GAME_SCORE(PLAYER_ID)

#define ENGINE_palette the_game.GamePal
#define DETAIL_LEVEL the_game.DetailLevel

#define TICK_SHIFT (8)
#define NORMAL_TICK_TOCK (1000 / 15)
#define TICK_TOCK (the_game.TickTock)
#define TICK_RATIO (the_game.TickRatio)
#define TICK_INV_RATIO (the_game.TickInvRatio)
#define GAME_STATE (the_game.GameState)
#define GAME_TURN the_game.GameTurn
#define GAME_FLAGS (the_game.GameFlags)

#define GAME_TIME (the_game.GameTime) // In 24-hours in fixed point 8...
#define GAME_SEASON (the_game.GameSeason)

#define CRIME_RATE (the_game.CrimeRate)
#define CRIME_RATE_SCORE_MUL (the_game.CrimeRateScoreMul)
#define MUSIC_WORLD (the_game.MusicWorld)
#define BOREDOM_RATE (the_game.BoredomRate)

#define TEXTURE_SET (the_game.TextureSet)
#define FAKE_CIVS (the_game.FakeCivs)
#define SAVE_VALID (the_game.SaveValid)

#define RAND_SEED (the_game.RandSeed)

#define DRAW_TWEEN_COUNT (the_game.DrawTweenCount)
#define DRAW_MESH_COUNT (the_game.DrawMeshCount)

#define PERSON_COUNT (the_game.PersonCount)
#define ANIMAL_COUNT (the_game.AnimalCount)
#define CHOPPER_COUNT (the_game.ChopperCount)
#define PYRO_COUNT (the_game.PyroCount)
#define PLAYER_COUNT (the_game.PlayerCount)
#define PROJECTILE_COUNT (the_game.ProjectileCount)
#define SPECIAL_COUNT (the_game.SpecialCount)
#define SWITCH_COUNT (the_game.SwitchCount)
#define BAT_COUNT (the_game.BatCount)
#define PRIMARY_COUNT (the_game.PrimaryThingCount)
#define SECONDARY_COUNT (the_game.SecondaryThingCount)

#define NET_PERSON(i) (the_game.net_persons[i])
#define NET_PLAYER(i) (the_game.net_players[i])

#define MAP (the_game.Map)
#define MAP2(x, y) (the_game.Map[(y) + ((x) * MAP_WIDTH)])
#define MAP_WHO(i) (MAP[i].MapWho)

#define PRIMARY_USED (the_game.UsedPrimaryThings)
#define PRIMARY_UNUSED (the_game.UnusedPrimaryThings)
#define SECONDARY_USED (the_game.UsedSecondaryThings)
#define SECONDARY_UNUSED (the_game.UnusedSecondaryThings)

#define DRAW_TWEENS (the_game.DrawTweens)
#define DRAW_MESHES (the_game.DrawMeshes)

#define NETPERSON (the_game.net_persons)
#define NETPLAYERS (the_game.net_players)
#define FURNITURE (the_game.Furnitures)
#define PEOPLE (the_game.People)
#define ANIMALS (the_game.Animals)
#define CHOPPERS (the_game.Choppers)
#define PYROS (the_game.Pyros)
#define PLAYERS (the_game.Players)
#define PROJECTILES (the_game.Projectiles)
#define SPECIALS (the_game.Specials)
#define SWITCHES (the_game.Switches)
#define THINGS (the_game.Things)
#define VEHICLES (the_game.Vehicles)
#define BATS (the_game.Bats)


#define INDOORS_HEIGHT_FLOOR (the_game.indoors_height_floor)
#define INDOORS_HEIGHT_CEILING (the_game.indoors_height_ceiling)
#define INDOORS_DBUILDING (the_game.indoors_dbuilding)
#define INDOORS_INDEX_FADE (the_game.indoors_index_fade)
#define INDOORS_INDEX_FADE_EXT (the_game.indoors_index_fade_ext)
#define INDOORS_INDEX_FADE_EXT_DIR (the_game.indoors_index_fade_ext_dir)
#define INDOORS_ROOM_NEXT (the_game.indoors_room_next)
#define INDOORS_INDEX_NEXT (the_game.indoors_index_next)
#define INDOORS_INDEX (the_game.indoors_index)
#define INDOORS_ROOM (the_game.indoors_room)


#define TO_DRAW_TWEEN(t) (&DRAW_TWEENS[t])
#define TO_DRAW_MESH(t) (&DRAW_MESHES[t])

#define TO_VEHICLE(t) (&VEHICLES[t])
#define TO_FURNITURE(t) (&FURNITURE[t])
#define TO_PERSON(t) (&PEOPLE[t])
#define TO_ANIMAL(t) (&ANIMALS[t])
#define TO_CHOPPER(t) (&CHOPPERS[t])
#define TO_PYRO(t) (&PYROS[t])
#define TO_PLAYER(t) (&PLAYERS[t])
#define TO_PROJECTILE(t) (&PROJECTILES[t])
#define TO_SPECIAL(t) (&SPECIALS[t])
#define TO_SWITCH(t) (&SWITCHES[t])
#define TO_THING(t) (&THINGS[t])
#define TO_BAT(t) (&BATS[t])

#define ANIMAL_NUMBERb(t) (COMMON_INDEX)(t - TO_ANIMAL(0))
#define CHOPPER_NUMBER(t) (COMMON_INDEX)(t - TO_CHOPPER(0))
#define DRAW_TWEEN_NUMBER(t) (COMMON_INDEX)(t - TO_DRAW_TWEEN(0))
#define DRAW_MESH_NUMBER(t) (COMMON_INDEX)(t - TO_DRAW_MESH(0))
#define VEHICLE_NUMBER(t) (COMMON_INDEX)(t - TO_VEHICLE(0))
#define FURNITURE_NUMBER(t) (COMMON_INDEX)(t - TO_FURNITURE(0))
#define PERSON_NUMBER(t) (COMMON_INDEX)(t - TO_PERSON(0))
#define PLAYER_NUMBER(t) (COMMON_INDEX)(t - TO_PLAYER(0))
#define PROJECTILE_NUMBER(t) (COMMON_INDEX)(t - TO_PROJECTILE(0))
#define SPECIAL_NUMBER(t) (COMMON_INDEX)(t - TO_SPECIAL(0))
#define SWITCH_NUMBER(t) (COMMON_INDEX)(t - TO_SWITCH(0))
#define PYRO_NUMBER(t) (COMMON_INDEX)(t - TO_PYRO(0))
#define BAT_NUMBER(t) (COMMON_INDEX)(t - TO_BAT(0))
#define THING_NUMBER(t) (THING_INDEX)(t - TO_THING(0))

// #define	NPLAYER_NUMBER(t)		(THING_INDEX)(t-NET_PLAYER(0))
// #define	NPERSON_NUMBER(t)		(THING_INDEX)(t-NET_PERSON(0))

extern void ResetSmoothTicks();
extern SLONG SmoothTicks(SLONG raw_ticks);

//---------------------------------------------------------------

inline void SetSeed(ULONG seed)
{
    RAND_SEED = seed;
}

inline ULONG GetSeed(void)
{
    return RAND_SEED;
}
/*
inline UWORD Random(void)
{
        //
        // The numbers are as recommended by Knuth.
        //

        // RAND_SEED *= 65277;
        // RAND_SEED += 13849;

        //
        // These numbers are as recommened by somebody else.
        //
        // _seed = 69069 * _seed + 12359;
        //

        //
        // This is the super-duper random number generator.
        //

        RAND_SEED *= 69069;
        RAND_SEED += 1;

        return	(RAND_SEED >> 7);
}
*/
// #define	Random()    (DebugText("random  file %s line %d %d\n",__FILE__,__LINE__,RAND_SEED),(UWORD)((RAND_SEED = ((RAND_SEED*69069)+1) )>>7))
#define Random() ((UWORD)((RAND_SEED = ((RAND_SEED * 69069) + 1)) >> 7))

//---------------------------------------------------------------

//---------------------------------------------------------------

void game_startup(void);
void game_shutdown(void);
BOOL game_init(void);
void game(void);
void game_attract_mode(void);
UBYTE game_loop(void);

//---------------------------------------------------------------

SLONG calc_angle(SLONG dx, SLONG dz);
SLONG angle_diff(SLONG angle1, SLONG angle2);

// Useful.
void stop_all_fx_and_music();

//---------------------------------------------------------------

//
// where should it go though?
//

#define POLY_50MGT (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
#define POLY_50MT (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
#define POLY_MGT (POLY_FLAG_GOURAD | POLY_FLAG_MASKED | POLY_FLAG_TEXTURED)
#define POLY_MT (POLY_FLAG_MASKED | POLY_FLAG_TEXTURED)
#define POLY_NULL (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED)

#define POLY_50F (POLY_FLAG_SEMI_TRANS)
#define POLY_50GT (POLY_FLAG_SEMI_TRANS | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
#define POLY_50T (POLY_FLAG_SEMI_TRANS | POLY_FLAG_TEXTURED)
#define POLY_50G (POLY_FLAG_SEMI_TRANS | POLY_FLAG_GOURAD)
#define POLY_TGT (POLY_FLAG_TILED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
#define POLY_GT (POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
#define POLY_TT (POLY_FLAG_TILED | POLY_FLAG_TEXTURED)
#define POLY_T (POLY_FLAG_TEXTURED)
#define POLY_G (POLY_FLAG_GOURAD)
#define POLY_F (0)

#endif
