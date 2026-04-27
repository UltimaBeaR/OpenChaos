#ifndef MISSIONS_SAVE_H
#define MISSIONS_SAVE_H

#include "engine/core/types.h"

// Memory allocation table entry. The engine uses a flat array of these (save_table[])
// to describe every object pool (vehicles, people, specials, etc.). At level load,
// each entry is used to allocate the pool and store a pointer into the Game struct.
// uc_orig: MemTable (fallen/Headers/Game.h)
struct MemTable {
    CBYTE* Name; // Human-readable pool name (for debug output).
    void** Point; // Pointer to the pool pointer inside the_game.
    UBYTE Type; // Allocation type (0 = malloc, 1 = heap, etc.).
    SLONG* CountL; // Optional pointer to an SLONG count field.
    UWORD* CountW; // Optional pointer to a UWORD count field.
    SLONG Maximum; // Maximum number of elements.
    SWORD StructSize; // Size of one element in bytes.
    SWORD Extra; // Extra bytes appended after the pool.
};
extern struct MemTable save_table[];

// Indices into save_table[] for well-known pools.
// uc_orig: SAVE_TABLE_VEHICLE (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_VEHICLE
#define SAVE_TABLE_VEHICLE 29
#endif
// uc_orig: SAVE_TABLE_PEOPLE (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_PEOPLE
#define SAVE_TABLE_PEOPLE 30
#endif
// uc_orig: SAVE_TABLE_SPECIAL (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_SPECIAL
#define SAVE_TABLE_SPECIAL 36
#endif
// uc_orig: SAVE_TABLE_BAT (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_BAT
#define SAVE_TABLE_BAT 38
#endif
// uc_orig: SAVE_TABLE_DTWEEN (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_DTWEEN
#define SAVE_TABLE_DTWEEN 40
#endif
// uc_orig: SAVE_TABLE_DMESH (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_DMESH
#define SAVE_TABLE_DMESH 41
#endif
// uc_orig: SAVE_TABLE_WMOVE (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_WMOVE
#define SAVE_TABLE_WMOVE 46
#endif
// uc_orig: SAVE_TABLE_POINTS (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_POINTS
#define SAVE_TABLE_POINTS 15
#endif
// uc_orig: SAVE_TABLE_FACE4 (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_FACE4
#define SAVE_TABLE_FACE4 16
#endif
// uc_orig: SAVE_TABLE_PLATS (fallen/Headers/Game.h)
#ifndef SAVE_TABLE_PLATS
#define SAVE_TABLE_PLATS 45
#endif

// In-game save/load system: serializes all living things, their genus data, and EWAY timer
// state to "ingame.sav". Used for quick-save/load during gameplay (PC only).
// The save format is a flat binary stream: per-thing records tagged with a type byte,
// followed by an EWAY section. On load: things are cleared and restored from the stream.

#endif // MISSIONS_SAVE_H
