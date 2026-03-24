#ifndef ENGINE_LIGHTING_ED_LIGHT_H
#define ENGINE_LIGHTING_ED_LIGHT_H

// Light editor API — used at load time to apply baked level lighting.
// Original source: fallen/Ledit/Headers/ed.h + fallen/Ledit/Source/ed.cpp

#include "core/types.h"

// uc_orig: LIGHT_FLAGS_INSIDE (fallen/Ledit/Headers/ed.h)
// Set when a light is placed inside a building.
#define LIGHT_FLAGS_INSIDE (1)

// uc_orig: ED_Light (fallen/Ledit/Headers/ed.h)
typedef struct
{
    UBYTE range;
    SBYTE red;
    SBYTE green;
    SBYTE blue;
    UBYTE next;
    UBYTE used;
    UBYTE flags;
    UBYTE padding;
    SLONG x;
    SLONG y;
    SLONG z;

} ED_Light;

// uc_orig: ED_MAX_LIGHTS (fallen/Ledit/Headers/ed.h)
#define ED_MAX_LIGHTS 256

// uc_orig: ED_light (fallen/Ledit/Headers/ed.h)
extern ED_Light ED_light[ED_MAX_LIGHTS];
// uc_orig: ED_init (fallen/Ledit/Headers/ed.h)
// Clears all light info.
void ED_init(void);

// uc_orig: ED_create (fallen/Ledit/Headers/ed.h)
// Creates a new light. Range 1-255, RGB are signed (+/-127).
SLONG ED_create(
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG range,
    SLONG red,
    SLONG green,
    SLONG blue);

// uc_orig: ED_delete (fallen/Ledit/Headers/ed.h)
void ED_delete(SLONG light);
// uc_orig: ED_delete_all (fallen/Ledit/Headers/ed.h)
void ED_delete_all(void);

// uc_orig: ED_light_move (fallen/Ledit/Headers/ed.h)
void ED_light_move(
    SLONG light,
    SLONG x,
    SLONG y,
    SLONG z);

// uc_orig: ED_light_change (fallen/Ledit/Headers/ed.h)
void ED_light_change(
    SLONG light,
    SLONG range,
    SLONG red,
    SLONG green,
    SLONG blue);

// uc_orig: ED_amb_get (fallen/Ledit/Headers/ed.h)
// Get/set ambient light colour (values 0-255).
void ED_amb_get(
    SLONG* red,
    SLONG* green,
    SLONG* blue);

// uc_orig: ED_amb_set (fallen/Ledit/Headers/ed.h)
void ED_amb_set(
    SLONG red,
    SLONG green,
    SLONG blue);

// uc_orig: ED_lampost_on_set (fallen/Ledit/Headers/ed.h)
void ED_lampost_on_set(SLONG lamposts_are_on);

// uc_orig: ED_night_set (fallen/Ledit/Headers/ed.h)
void ED_night_set(SLONG its_night_time);

// uc_orig: ED_darken_bottoms_on_set (fallen/Ledit/Headers/ed.h)
void ED_darken_bottoms_on_set(SLONG darken_bottoms_on);

// uc_orig: ED_lampost_get (fallen/Ledit/Headers/ed.h)
// Get/set lamp-post light colour. Range 1-255, RGB signed (+/-127).
void ED_lampost_get(
    SLONG* range,
    SLONG* red,
    SLONG* green,
    SLONG* blue);

// uc_orig: ED_lampost_set (fallen/Ledit/Headers/ed.h)
void ED_lampost_set(
    SLONG range,
    SLONG red,
    SLONG green,
    SLONG blue);

// uc_orig: ED_sky_get (fallen/Ledit/Headers/ed.h)
// Get/set sky light colour (values 0-255).
void ED_sky_get(
    SLONG* red,
    SLONG* green,
    SLONG* blue);

// uc_orig: ED_sky_set (fallen/Ledit/Headers/ed.h)
void ED_sky_set(
    SLONG red,
    SLONG green,
    SLONG blue);

// uc_orig: ED_undo_store (fallen/Ledit/Headers/ed.h)
void ED_undo_store(void);
// uc_orig: ED_undo_undo (fallen/Ledit/Headers/ed.h)
void ED_undo_undo(void);
// uc_orig: ED_undo_redo (fallen/Ledit/Headers/ed.h)
void ED_undo_redo(void);
#endif // ENGINE_LIGHTING_ED_LIGHT_H
