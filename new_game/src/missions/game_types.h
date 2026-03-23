#ifndef MISSIONS_GAME_TYPES_H
#define MISSIONS_GAME_TYPES_H

// Central game-state header: the Game struct, all GS_/GF_/GAME_* constants,
// all accessor macros (PEOPLE, MAP, GAME_STATE, TICK_RATIO, etc.),
// and the global instance the_game.
//
// Historically everything in this header lived in fallen/Headers/Game.h,
// which was the mega-header included by virtually every .cpp in the game.

#include "core/types.h"
#include "missions/save.h"                     // MemTable, save_table[], SAVE_TABLE_* indices
#include "engine/graphics/pipeline/poly.h"     // POLY_FLAG_* (used in POLY_* combined macros below)

// The Game struct contains MapElement (the world grid) and GameCoord.
#include "world/map/map.h" // MapElement, MAP_SIZE, MAP_WIDTH, MAP_HEIGHT, MAP_INDEX, ELE_SHIFT
#include "core/vector.h"   // GameCoord

// game_types.h is self-sufficient: pulls in all pool type definitions via thing.h.
// This avoids the need for callers to include thing.h before game_types.h.
#include "actors/core/thing.h"

// Draw distance: number of map tiles rendered in each direction from the player.
// PC value is 22; original PSX value was 13.
// uc_orig: DRAW_DIST (fallen/Headers/Game.h)
#ifndef DRAW_DIST
#define DRAW_DIST 22
#endif

// THING_INDEX, COMMON_INDEX, MAPCO8/16/24 are defined in core/types.h (included above).

//-----------------------------------------------------------------------------
// 24-bit RGB colour entry used for the Gouraud palette (Game::GamePal[256]).
// On PSX this was the entire vertex colour system; on PC D3D computes colours
// per-vertex via the NIGHT cache but the palette array is still present.
// uc_orig: ENGINE_Col (fallen/Headers/Game.h)
typedef struct
{
    UBYTE red;
    UBYTE green;
    UBYTE blue;
} ENGINE_Col;

//-----------------------------------------------------------------------------
// Game states (Game::GameState bitmask). These control the top-level loop mode.
// uc_orig: GS_ATTRACT_MODE (fallen/Headers/Game.h)
#define GS_ATTRACT_MODE  (1 << 0)  // Title/demo loop.
// uc_orig: GS_PLAY_GAME (fallen/Headers/Game.h)
#define GS_PLAY_GAME     (1 << 1)  // Normal gameplay.
// uc_orig: GS_CONFIGURE_NET (fallen/Headers/Game.h)
#define GS_CONFIGURE_NET (1 << 2)  // Network configuration screen.
// uc_orig: GS_LEVEL_WON (fallen/Headers/Game.h)
#define GS_LEVEL_WON     (1 << 3)  // Mission success.
// uc_orig: GS_LEVEL_LOST (fallen/Headers/Game.h)
#define GS_LEVEL_LOST    (1 << 4)  // Mission failure.
// uc_orig: GS_GAME_WON (fallen/Headers/Game.h)
#define GS_GAME_WON      (1 << 5)  // Entire game completed.
// uc_orig: GS_RECORD (fallen/Headers/Game.h)
#define GS_RECORD        (1 << 6)  // Demo recording mode.
// uc_orig: GS_PLAYBACK (fallen/Headers/Game.h)
#define GS_PLAYBACK      (1 << 7)  // Demo playback mode.
// uc_orig: GS_REPLAY (fallen/Headers/Game.h)
#define GS_REPLAY        (1 << 8)  // Replay mode.
// uc_orig: GS_EDITOR (fallen/Headers/Game.h)
#define GS_EDITOR        (1 << 16) // In-level editor (separate from gameplay bits).

//-----------------------------------------------------------------------------
// Game flags (Game::GameFlags bitmask). Per-frame environmental state.
// uc_orig: GF_INDOORS (fallen/Headers/Game.h)
#define GF_INDOORS              (1 << 0)  // Player is inside a building.
// uc_orig: GF_SEWERS (fallen/Headers/Game.h)
#define GF_SEWERS               (1 << 1)  // Player is in the sewer network.
// uc_orig: GF_PAUSED (fallen/Headers/Game.h)
#define GF_PAUSED               (1 << 2)  // Game is paused.
// uc_orig: GF_RAINING (fallen/Headers/Game.h)
#define GF_RAINING              (1 << 3)  // Rain is active.
// uc_orig: GF_WINDY (fallen/Headers/Game.h)
#define GF_WINDY                (1 << 4)  // Wind is active.
// uc_orig: GF_SHOW_MAP (fallen/Headers/Game.h)
#define GF_SHOW_MAP             (1 << 5)  // Minimap overlay visible.
// uc_orig: GF_SHOW_CRIMERATE (fallen/Headers/Game.h)
#define GF_SHOW_CRIMERATE       (1 << 6)  // Crime rate HUD overlay visible.
// uc_orig: GF_PLAYER_FIRED_GUN (fallen/Headers/Game.h)
#define GF_PLAYER_FIRED_GUN     (1 << 7)  // Player shot this frame — alerts NPCs.
// uc_orig: GF_CONE_PENALTIES (fallen/Headers/Game.h)
#define GF_CONE_PENALTIES       (1 << 8)  // NPC vision cone penalties active.
// uc_orig: GF_CARS_WITH_ROAD_PRIMS (fallen/Headers/Game.h)
#define GF_CARS_WITH_ROAD_PRIMS (1 << 9)  // Vehicles use road primitive data.
// uc_orig: GF_SIDE_ON_COMBAT (fallen/Headers/Game.h)
#define GF_SIDE_ON_COMBAT       (1 << 10) // Side-scrolling combat mode.
// uc_orig: GF_NO_FLOOR (fallen/Headers/Game.h)
#define GF_NO_FLOOR             (1 << 11) // Level has no floor geometry (sky level).
// uc_orig: GF_DISABLE_BENCH_HEALTH (fallen/Headers/Game.h)
#define GF_DISABLE_BENCH_HEALTH (1 << 12) // Benches do not restore health.

//-----------------------------------------------------------------------------
// Seasons stored in Game::Season (0..3). Controls sky colour and day length.
// uc_orig: GAME_SEASON_SPRING (fallen/Headers/Game.h)
#define GAME_SEASON_SPRING 0
// uc_orig: GAME_SEASON_SUMMER (fallen/Headers/Game.h)
#define GAME_SEASON_SUMMER 1
// uc_orig: GAME_SEASON_AUTUMN (fallen/Headers/Game.h)
#define GAME_SEASON_AUTUMN 2
// uc_orig: GAME_SEASON_WINTER (fallen/Headers/Game.h)
#define GAME_SEASON_WINTER 3

// Game clock thresholds. Game::Time is 24-hour fixed-point 8 (1 hour = 256 units).
// uc_orig: GAME_TIME_DAWN (fallen/Headers/Game.h)
#define GAME_TIME_DAWN 5   // 5:00 AM — ambient light starts rising.
// uc_orig: GAME_TIME_DUSK (fallen/Headers/Game.h)
#define GAME_TIME_DUSK 20  // 8:00 PM — ambient light starts falling.

//-----------------------------------------------------------------------------
// Detail level flags (Game::DetailLevel bitmask). Control which visual effects fire.
// uc_orig: DETAIL_DIRT1 (fallen/Headers/Game.h)
#define DETAIL_DIRT1       (1 << 0)
// uc_orig: DETAIL_DIRT2 (fallen/Headers/Game.h)
#define DETAIL_DIRT2       (1 << 1)
// uc_orig: DETAIL_PUDDLE (fallen/Headers/Game.h)
#define DETAIL_PUDDLE      (1 << 2)
// uc_orig: DETAIL_REFLECTIONS (fallen/Headers/Game.h)
#define DETAIL_REFLECTIONS (1 << 3)
// uc_orig: DETAIL_SHADOWS (fallen/Headers/Game.h)
#define DETAIL_SHADOWS     (1 << 4)
// uc_orig: DETAIL_SPLASH (fallen/Headers/Game.h)
#define DETAIL_SPLASH      (1 << 5)
// uc_orig: DETAIL_MIST (fallen/Headers/Game.h)
#define DETAIL_MIST        (1 << 6)
// uc_orig: DETAIL_RAIN (fallen/Headers/Game.h)
#define DETAIL_RAIN        (1 << 7)

//-----------------------------------------------------------------------------
// The global game-state structure. One instance: the_game.
// Everything is accessed through the macros below (GAME_STATE, PEOPLE, MAP, etc.)
// rather than the struct fields directly.
// uc_orig: Game (fallen/Headers/Game.h)
typedef struct
{
    // Top-level game mode (GS_*), frame counter, and per-frame environmental flags (GF_*).
    SLONG GameState,
        GameTurn,
        GameFlags,

        // Active-object counts for each pool (NOT pool sizes; sizes are MAX_* constants).
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

        // Active animated-geometry object counts.
        DrawTweenCount,
        DrawMeshCount,
        PrimaryThingCount,
        SecondaryThingCount,

        // Frame timing: TickTock = ms/frame (target 67ms = 15Hz).
        // TickRatio = (TickTock << TICK_SHIFT) / NORMAL_TICK_TOCK, fixed-point 8.
        // Multiply motion values by TickRatio >> TICK_SHIFT for frame-rate independence.
        TickTock,
        TickRatio,
        TickInvRatio,

        // Deterministic RNG state. Updated by Random() macro. Must stay in sync in multiplayer.
        RandSeed,

        // 24-hour clock in fixed-point 8 (256 units = 1 hour). Range 0..6143.
        Time,
        Season;

    // World collision/occupation grid. MAP_SIZE = MAP_WIDTH * MAP_HEIGHT (128x128).
    // Each cell holds a linked list of Things occupying it plus collision vectors.
    MapElement Map[MAP_SIZE];

    // Object pool pointers — flat arrays allocated at level load, linked into free/used lists.
    Vehicle*     Vehicles;
    Furniture*   Furnitures;
    Person*      People;
    Animal*      Animals;
    Chopper*     Choppers;
    Pyro*        Pyros;
    Player*      Players;
    Projectile*  Projectiles;
    Special*     Specials;
    Switch*      Switches;
    Bat*         Bats;
    Thing*       Things;

    // 256-entry Gouraud palette used for PSX vertex colour; present but secondary on PC/D3D.
    ENGINE_Col GamePal[256];

    // Animated geometry descriptor pools.
    DrawTween* DrawTweens;
    DrawMesh*  DrawMeshes;

    // Primary/secondary Thing linked list head indices.
    THING_INDEX UsedPrimaryThings,
        UnusedPrimaryThings,
        UsedSecondaryThings,
        UnusedSecondaryThings;

    // Multiplayer remote entity pointers (up to 10 remote players).
    struct Thing** net_persons;
    struct Thing** net_players;

    // Indoors tracking: set when player is inside a building storey.
    // indoors_index selects the interior NIGHT_cache lighting set.
    SLONG indoors_height_floor;
    SLONG indoors_height_ceiling;
    SLONG indoors_dbuilding;
    UWORD indoors_index_fade;
    UWORD indoors_index_fade_ext;
    UWORD indoors_index_fade_ext_dir;
    UWORD indoors_index_next;
    UWORD indoors_index;
    UWORD indoors_room;
    UWORD indoors_room_next;

    // Multiplayer session data.
    SLONG NumberPlayers;
    ULONG Packets[16];
    SLONG Scores[16];
    SLONG MyID;

    // UI mode selector, visual quality bitmask, and level texture set.
    SLONG UserInterface;
    UWORD DetailLevel;
    UWORD TextureSet;

    // Crime level (0..100+): affects mission score and police response.
    SLONG CrimeRate;
    SLONG CrimeRateScoreMul;

    // Crowd density, save-slot validity, music set index, NPC boredom rate.
    UBYTE FakeCivs;
    UBYTE SaveValid;
    UBYTE MusicWorld;
    UBYTE BoredomRate;

    // Persistent RPG stats for both playable characters (Darci and Roper).
    // Range 0..255. Affect damage, health cap, accuracy, and stamina.
    UBYTE DarciStrength;
    UBYTE DarciConstitution;
    UBYTE DarciSkill;
    UBYTE DarciStamina;
    UBYTE RoperStrength;
    UBYTE RoperConstitution;
    UBYTE RoperSkill;
    UBYTE RoperStamina;

    // Running count of civilian kills by the player. Triggers warnings at a threshold.
    UBYTE DarciDeadCivWarnings;
    UBYTE padding[2];

} Game;

// The one global game-state instance.
// uc_orig: the_game (fallen/Source/Game.cpp)
extern Game the_game;

// Violence content flag (set by command-line or config).
// uc_orig: VIOLENCE (fallen/Source/Game.cpp)
extern UBYTE VIOLENCE;

//-----------------------------------------------------------------------------
// Accessor macros — shorthand for the_game.Field.
// uc_orig: USER_INTERFACE (fallen/Headers/Game.h)
#define USER_INTERFACE   the_game.UserInterface
// uc_orig: NO_PLAYERS (fallen/Headers/Game.h)
#define NO_PLAYERS       CNET_num_players
// uc_orig: PACKET_DATA (fallen/Headers/Game.h)
#define PACKET_DATA(x)   the_game.Packets[x]
// uc_orig: PLAYER_ID (fallen/Headers/Game.h)
#define PLAYER_ID        CNET_player_id
// uc_orig: GAME_SCORE (fallen/Headers/Game.h)
#define GAME_SCORE(x)    the_game.Scores[x]
// uc_orig: MY_SCORE (fallen/Headers/Game.h)
#define MY_SCORE         GAME_SCORE(PLAYER_ID)

// uc_orig: ENGINE_palette (fallen/Headers/Game.h)
#define ENGINE_palette   the_game.GamePal
// uc_orig: DETAIL_LEVEL (fallen/Headers/Game.h)
#define DETAIL_LEVEL     the_game.DetailLevel

// Tick-rate constants and accessors.
// uc_orig: TICK_SHIFT (fallen/Headers/Game.h)
#define TICK_SHIFT       (8)
// uc_orig: NORMAL_TICK_TOCK (fallen/Headers/Game.h)
#define NORMAL_TICK_TOCK (1000 / 15)
// uc_orig: TICK_TOCK (fallen/Headers/Game.h)
#define TICK_TOCK        (the_game.TickTock)
// uc_orig: TICK_RATIO (fallen/Headers/Game.h)
#define TICK_RATIO       (the_game.TickRatio)
// uc_orig: TICK_INV_RATIO (fallen/Headers/Game.h)
#define TICK_INV_RATIO   (the_game.TickInvRatio)

// uc_orig: GAME_STATE (fallen/Headers/Game.h)
#define GAME_STATE  (the_game.GameState)
// uc_orig: GAME_TURN (fallen/Headers/Game.h)
#define GAME_TURN   the_game.GameTurn
// uc_orig: GAME_FLAGS (fallen/Headers/Game.h)
#define GAME_FLAGS  (the_game.GameFlags)

// Note: original macros GAME_TIME/GAME_SEASON reference fields "GameTime"/"GameSeason"
// which don't exist in the struct (fields are "Time" and "Season"). Bug in original.
// uc_orig: GAME_TIME (fallen/Headers/Game.h)
#define GAME_TIME   (the_game.GameTime)
// uc_orig: GAME_SEASON (fallen/Headers/Game.h)
#define GAME_SEASON (the_game.GameSeason)

// uc_orig: CRIME_RATE (fallen/Headers/Game.h)
#define CRIME_RATE          (the_game.CrimeRate)
// uc_orig: CRIME_RATE_SCORE_MUL (fallen/Headers/Game.h)
#define CRIME_RATE_SCORE_MUL (the_game.CrimeRateScoreMul)
// uc_orig: MUSIC_WORLD (fallen/Headers/Game.h)
#define MUSIC_WORLD         (the_game.MusicWorld)
// uc_orig: BOREDOM_RATE (fallen/Headers/Game.h)
#define BOREDOM_RATE        (the_game.BoredomRate)
// uc_orig: TEXTURE_SET (fallen/Headers/Game.h)
#define TEXTURE_SET         (the_game.TextureSet)
// uc_orig: FAKE_CIVS (fallen/Headers/Game.h)
#define FAKE_CIVS           (the_game.FakeCivs)
// uc_orig: SAVE_VALID (fallen/Headers/Game.h)
#define SAVE_VALID          (the_game.SaveValid)
// uc_orig: RAND_SEED (fallen/Headers/Game.h)
#define RAND_SEED           (the_game.RandSeed)

// uc_orig: DRAW_TWEEN_COUNT (fallen/Headers/Game.h)
#define DRAW_TWEEN_COUNT  (the_game.DrawTweenCount)
// uc_orig: DRAW_MESH_COUNT (fallen/Headers/Game.h)
#define DRAW_MESH_COUNT   (the_game.DrawMeshCount)

// Object-pool count accessors.
// uc_orig: PERSON_COUNT (fallen/Headers/Game.h)
#define PERSON_COUNT     (the_game.PersonCount)
// uc_orig: ANIMAL_COUNT (fallen/Headers/Game.h)
#define ANIMAL_COUNT     (the_game.AnimalCount)
// uc_orig: CHOPPER_COUNT (fallen/Headers/Game.h)
#define CHOPPER_COUNT    (the_game.ChopperCount)
// uc_orig: PYRO_COUNT (fallen/Headers/Game.h)
#define PYRO_COUNT       (the_game.PyroCount)
// uc_orig: PLAYER_COUNT (fallen/Headers/Game.h)
#define PLAYER_COUNT     (the_game.PlayerCount)
// uc_orig: PROJECTILE_COUNT (fallen/Headers/Game.h)
#define PROJECTILE_COUNT (the_game.ProjectileCount)
// uc_orig: SPECIAL_COUNT (fallen/Headers/Game.h)
#define SPECIAL_COUNT    (the_game.SpecialCount)
// uc_orig: SWITCH_COUNT (fallen/Headers/Game.h)
#define SWITCH_COUNT     (the_game.SwitchCount)
// uc_orig: BAT_COUNT (fallen/Headers/Game.h)
#define BAT_COUNT        (the_game.BatCount)
// uc_orig: PRIMARY_COUNT (fallen/Headers/Game.h)
#define PRIMARY_COUNT    (the_game.PrimaryThingCount)
// uc_orig: SECONDARY_COUNT (fallen/Headers/Game.h)
#define SECONDARY_COUNT  (the_game.SecondaryThingCount)

// uc_orig: NET_PERSON (fallen/Headers/Game.h)
#define NET_PERSON(i) (the_game.net_persons[i])
// uc_orig: NET_PLAYER (fallen/Headers/Game.h)
#define NET_PLAYER(i) (the_game.net_players[i])

// uc_orig: MAP (fallen/Headers/Game.h)
#define MAP            (the_game.Map)
// uc_orig: MAP2 (fallen/Headers/Game.h)
#define MAP2(x, y)     (the_game.Map[(y) + ((x) * MAP_WIDTH)])
// uc_orig: MAP_WHO (fallen/Headers/Game.h)
#define MAP_WHO(i)     (MAP[i].MapWho)

// uc_orig: PRIMARY_USED (fallen/Headers/Game.h)
#define PRIMARY_USED    (the_game.UsedPrimaryThings)
// uc_orig: PRIMARY_UNUSED (fallen/Headers/Game.h)
#define PRIMARY_UNUSED  (the_game.UnusedPrimaryThings)
// uc_orig: SECONDARY_USED (fallen/Headers/Game.h)
#define SECONDARY_USED  (the_game.UsedSecondaryThings)
// uc_orig: SECONDARY_UNUSED (fallen/Headers/Game.h)
#define SECONDARY_UNUSED (the_game.UnusedSecondaryThings)

// uc_orig: DRAW_TWEENS (fallen/Headers/Game.h)
#define DRAW_TWEENS  (the_game.DrawTweens)
// uc_orig: DRAW_MESHES (fallen/Headers/Game.h)
#define DRAW_MESHES  (the_game.DrawMeshes)

// uc_orig: NETPERSON (fallen/Headers/Game.h)
#define NETPERSON    (the_game.net_persons)
// uc_orig: NETPLAYERS (fallen/Headers/Game.h)
#define NETPLAYERS   (the_game.net_players)
// uc_orig: FURNITURE (fallen/Headers/Game.h)
#define FURNITURE    (the_game.Furnitures)
// uc_orig: PEOPLE (fallen/Headers/Game.h)
#define PEOPLE       (the_game.People)
// uc_orig: ANIMALS (fallen/Headers/Game.h)
#define ANIMALS      (the_game.Animals)
// uc_orig: CHOPPERS (fallen/Headers/Game.h)
#define CHOPPERS     (the_game.Choppers)
// uc_orig: PYROS (fallen/Headers/Game.h)
#define PYROS        (the_game.Pyros)
// uc_orig: PLAYERS (fallen/Headers/Game.h)
#define PLAYERS      (the_game.Players)
// uc_orig: PROJECTILES (fallen/Headers/Game.h)
#define PROJECTILES  (the_game.Projectiles)
// uc_orig: SPECIALS (fallen/Headers/Game.h)
#define SPECIALS     (the_game.Specials)
// uc_orig: SWITCHES (fallen/Headers/Game.h)
#define SWITCHES     (the_game.Switches)
// uc_orig: THINGS (fallen/Headers/Game.h)
#define THINGS       (the_game.Things)
// uc_orig: VEHICLES (fallen/Headers/Game.h)
#define VEHICLES     (the_game.Vehicles)
// uc_orig: BATS (fallen/Headers/Game.h)
#define BATS         (the_game.Bats)

// Indoors state accessors.
// uc_orig: INDOORS_HEIGHT_FLOOR (fallen/Headers/Game.h)
#define INDOORS_HEIGHT_FLOOR      (the_game.indoors_height_floor)
// uc_orig: INDOORS_HEIGHT_CEILING (fallen/Headers/Game.h)
#define INDOORS_HEIGHT_CEILING    (the_game.indoors_height_ceiling)
// uc_orig: INDOORS_DBUILDING (fallen/Headers/Game.h)
#define INDOORS_DBUILDING         (the_game.indoors_dbuilding)
// uc_orig: INDOORS_INDEX_FADE (fallen/Headers/Game.h)
#define INDOORS_INDEX_FADE        (the_game.indoors_index_fade)
// uc_orig: INDOORS_INDEX_FADE_EXT (fallen/Headers/Game.h)
#define INDOORS_INDEX_FADE_EXT    (the_game.indoors_index_fade_ext)
// uc_orig: INDOORS_INDEX_FADE_EXT_DIR (fallen/Headers/Game.h)
#define INDOORS_INDEX_FADE_EXT_DIR (the_game.indoors_index_fade_ext_dir)
// uc_orig: INDOORS_ROOM_NEXT (fallen/Headers/Game.h)
#define INDOORS_ROOM_NEXT         (the_game.indoors_room_next)
// uc_orig: INDOORS_INDEX_NEXT (fallen/Headers/Game.h)
#define INDOORS_INDEX_NEXT        (the_game.indoors_index_next)
// uc_orig: INDOORS_INDEX (fallen/Headers/Game.h)
#define INDOORS_INDEX             (the_game.indoors_index)
// uc_orig: INDOORS_ROOM (fallen/Headers/Game.h)
#define INDOORS_ROOM              (the_game.indoors_room)

// Pool-to-pointer cast helpers (index → pointer into pool array).
// uc_orig: TO_DRAW_TWEEN (fallen/Headers/Game.h)
#define TO_DRAW_TWEEN(t)  (&DRAW_TWEENS[t])
// uc_orig: TO_DRAW_MESH (fallen/Headers/Game.h)
#define TO_DRAW_MESH(t)   (&DRAW_MESHES[t])
// uc_orig: TO_VEHICLE (fallen/Headers/Game.h)
#define TO_VEHICLE(t)     (&VEHICLES[t])
// uc_orig: TO_FURNITURE (fallen/Headers/Game.h)
#define TO_FURNITURE(t)   (&FURNITURE[t])
// uc_orig: TO_PERSON (fallen/Headers/Game.h)
#define TO_PERSON(t)      (&PEOPLE[t])
// uc_orig: TO_ANIMAL (fallen/Headers/Game.h)
#define TO_ANIMAL(t)      (&ANIMALS[t])
// uc_orig: TO_CHOPPER (fallen/Headers/Game.h)
#define TO_CHOPPER(t)     (&CHOPPERS[t])
// uc_orig: TO_PYRO (fallen/Headers/Game.h)
#define TO_PYRO(t)        (&PYROS[t])
// uc_orig: TO_PLAYER (fallen/Headers/Game.h)
#define TO_PLAYER(t)      (&PLAYERS[t])
// uc_orig: TO_PROJECTILE (fallen/Headers/Game.h)
#define TO_PROJECTILE(t)  (&PROJECTILES[t])
// uc_orig: TO_SPECIAL (fallen/Headers/Game.h)
#define TO_SPECIAL(t)     (&SPECIALS[t])
// uc_orig: TO_SWITCH (fallen/Headers/Game.h)
#define TO_SWITCH(t)      (&SWITCHES[t])
// uc_orig: TO_THING (fallen/Headers/Game.h)
#define TO_THING(t)       (&THINGS[t])
// uc_orig: TO_BAT (fallen/Headers/Game.h)
#define TO_BAT(t)         (&BATS[t])

// Pointer-to-index helpers (pointer → index into pool array).
// uc_orig: ANIMAL_NUMBERb (fallen/Headers/Game.h)
#define ANIMAL_NUMBERb(t)     (COMMON_INDEX)(t - TO_ANIMAL(0))
// uc_orig: CHOPPER_NUMBER (fallen/Headers/Game.h)
#define CHOPPER_NUMBER(t)     (COMMON_INDEX)(t - TO_CHOPPER(0))
// uc_orig: DRAW_TWEEN_NUMBER (fallen/Headers/Game.h)
#define DRAW_TWEEN_NUMBER(t)  (COMMON_INDEX)(t - TO_DRAW_TWEEN(0))
// uc_orig: DRAW_MESH_NUMBER (fallen/Headers/Game.h)
#define DRAW_MESH_NUMBER(t)   (COMMON_INDEX)(t - TO_DRAW_MESH(0))
// uc_orig: VEHICLE_NUMBER (fallen/Headers/Game.h)
#define VEHICLE_NUMBER(t)     (COMMON_INDEX)(t - TO_VEHICLE(0))
// uc_orig: FURNITURE_NUMBER (fallen/Headers/Game.h)
#define FURNITURE_NUMBER(t)   (COMMON_INDEX)(t - TO_FURNITURE(0))
// uc_orig: PERSON_NUMBER (fallen/Headers/Game.h)
#define PERSON_NUMBER(t)      (COMMON_INDEX)(t - TO_PERSON(0))
// uc_orig: PLAYER_NUMBER (fallen/Headers/Game.h)
#define PLAYER_NUMBER(t)      (COMMON_INDEX)(t - TO_PLAYER(0))
// uc_orig: PROJECTILE_NUMBER (fallen/Headers/Game.h)
#define PROJECTILE_NUMBER(t)  (COMMON_INDEX)(t - TO_PROJECTILE(0))
// uc_orig: SPECIAL_NUMBER (fallen/Headers/Game.h)
#define SPECIAL_NUMBER(t)     (COMMON_INDEX)(t - TO_SPECIAL(0))
// uc_orig: SWITCH_NUMBER (fallen/Headers/Game.h)
#define SWITCH_NUMBER(t)      (COMMON_INDEX)(t - TO_SWITCH(0))
// uc_orig: PYRO_NUMBER (fallen/Headers/Game.h)
#define PYRO_NUMBER(t)        (COMMON_INDEX)(t - TO_PYRO(0))
// uc_orig: BAT_NUMBER (fallen/Headers/Game.h)
#define BAT_NUMBER(t)         (COMMON_INDEX)(t - TO_BAT(0))
// uc_orig: THING_NUMBER (fallen/Headers/Game.h)
#define THING_NUMBER(t)       (THING_INDEX)(t - TO_THING(0))

//-----------------------------------------------------------------------------
// Stops all music and sound effects immediately (called on scene transitions).
// uc_orig: stop_all_fx_and_music (fallen/Headers/Game.h)
void stop_all_fx_and_music(void);

// Computes the angle (0..2047) from direction vector (dx, dz).
// uc_orig: calc_angle (fallen/Headers/Game.h)
SLONG calc_angle(SLONG dx, SLONG dz);

// Returns the signed shortest arc between two angles (0..2047 range).
// uc_orig: angle_diff (fallen/Headers/Game.h)
SLONG angle_diff(SLONG angle1, SLONG angle2);

// Tick smoothing for vehicle movement (averages last 4 TICK_RATIO values).
// uc_orig: ResetSmoothTicks (fallen/Headers/Game.h)
extern void ResetSmoothTicks();
// uc_orig: SmoothTicks (fallen/Headers/Game.h)
extern SLONG SmoothTicks(SLONG raw_ticks);

//-----------------------------------------------------------------------------
// RNG helpers.

// Sets the deterministic random seed.
// uc_orig: SetSeed (fallen/Headers/Game.h)
inline void SetSeed(ULONG seed)
{
    RAND_SEED = seed;
}

// Returns the current random seed.
// uc_orig: GetSeed (fallen/Headers/Game.h)
inline ULONG GetSeed(void)
{
    return RAND_SEED;
}

// Linear congruential RNG: seed = seed*69069 + 1; returns bits [7..22].
// Must use the same sequence on all multiplayer clients for sync.
// uc_orig: Random (fallen/Headers/Game.h)
#define Random() ((UWORD)((RAND_SEED = ((RAND_SEED * 69069) + 1)) >> 7))

//-----------------------------------------------------------------------------
// Polygon render-mode flag combinations (shortcuts for common draw modes).
// uc_orig: POLY_50MGT (fallen/Headers/Game.h)
#define POLY_50MGT (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
// uc_orig: POLY_50MT (fallen/Headers/Game.h)
#define POLY_50MT  (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
// uc_orig: POLY_MGT (fallen/Headers/Game.h)
#define POLY_MGT   (POLY_FLAG_GOURAD | POLY_FLAG_MASKED | POLY_FLAG_TEXTURED)
// uc_orig: POLY_MT (fallen/Headers/Game.h)
#define POLY_MT    (POLY_FLAG_MASKED | POLY_FLAG_TEXTURED)
// uc_orig: POLY_NULL (fallen/Headers/Game.h)
#define POLY_NULL  (POLY_FLAG_SEMI_TRANS | POLY_FLAG_MASKED)
// uc_orig: POLY_50F (fallen/Headers/Game.h)
#define POLY_50F   (POLY_FLAG_SEMI_TRANS)
// uc_orig: POLY_50GT (fallen/Headers/Game.h)
#define POLY_50GT  (POLY_FLAG_SEMI_TRANS | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
// uc_orig: POLY_50T (fallen/Headers/Game.h)
#define POLY_50T   (POLY_FLAG_SEMI_TRANS | POLY_FLAG_TEXTURED)
// uc_orig: POLY_50G (fallen/Headers/Game.h)
#define POLY_50G   (POLY_FLAG_SEMI_TRANS | POLY_FLAG_GOURAD)
// uc_orig: POLY_TGT (fallen/Headers/Game.h)
#define POLY_TGT   (POLY_FLAG_TILED | POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
// uc_orig: POLY_GT (fallen/Headers/Game.h)
#define POLY_GT    (POLY_FLAG_GOURAD | POLY_FLAG_TEXTURED)
// uc_orig: POLY_TT (fallen/Headers/Game.h)
#define POLY_TT    (POLY_FLAG_TILED | POLY_FLAG_TEXTURED)
// uc_orig: POLY_T (fallen/Headers/Game.h)
#define POLY_T     (POLY_FLAG_TEXTURED)
// uc_orig: POLY_G (fallen/Headers/Game.h)
#define POLY_G     (POLY_FLAG_GOURAD)
// uc_orig: POLY_F (fallen/Headers/Game.h)
#define POLY_F     (0)

#endif // MISSIONS_GAME_TYPES_H
