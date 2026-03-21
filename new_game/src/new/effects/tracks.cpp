// Temporary: game.h must be first — sets up all cross-module types
#include "fallen/Headers/game.h"

#include "effects/tracks.h"
#include "effects/tracks_globals.h"
#include "engine/graphics/pipeline/poly.h"

// Temporary: pap.h needed for PAP_2HI, PAP_calc_map_height_at, PAP_calc_height_at_thing
#include "fallen/Headers/pap.h"
// Temporary: animate.h needed for calc_sub_objects_position, SUB_OBJECT_*
#include "fallen/Headers/animate.h"
// Temporary: puddle.h needed for PUDDLE_in
#include "fallen/Headers/puddle.h"
// Temporary: interact.h needed for calc_sub_objects_position declaration
#include "fallen/Headers/interact.h"

// world_type is in Sound.cpp; WORLD_TYPE_SNOW from sound.h (pulled via game.h chain indirectly)
// but we use extern to avoid the sound.h → Structs.h → anim.h inclusion issues
extern SWORD world_type;
#define WORLD_TYPE_SNOW 4

// Internal colour constants for track types.
#define TRACK_WATER_COLOUR 0x203D60
#define TRACK_MUDDY_COLOUR 0x482000
#define TRACK_ONSNOW_COLOUR 0x000000

// uc_orig: TRACKS_InitOnce (fallen/Source/tracks.cpp)
void TRACKS_InitOnce(SWORD size)
{
    track_eob = size;
    track_head = track_tail = 0;
    memset((UBYTE*)tracks, 0, sizeof(Track) * size);
}

// uc_orig: TRACKS_Reset (fallen/Source/tracks.cpp)
void TRACKS_Reset(SWORD size)
{
    while (track_tail != track_head) {
        remove_thing_from_map(TO_THING(TO_TRACK(track_tail)->thing));
        free_thing(TO_THING(TO_TRACK(track_tail)->thing));
        track_tail++;
        if (track_tail == track_eob)
            track_tail = 0;
    }
    TRACKS_InitOnce(size);
}

// uc_orig: RShift8 (fallen/Source/tracks.cpp)
static inline void RShift8(SLONG& x, SLONG& y, SLONG& z)
{
    x >>= 8;
    y >>= 8;
    z >>= 8;
}

// uc_orig: TRACKS_Draw (fallen/Source/tracks.cpp)
void TRACKS_Draw()
{
    // Commented out in original.
}

// uc_orig: TRACKS_CalcDiffs (fallen/Source/tracks.cpp)
void TRACKS_CalcDiffs(Track& track, UBYTE width)
{
    SLONG x, z, f;

    x = (track.dz);
    z = -(track.dx);
    f = Root((x * x) + (z * z));
    if (!f)
        f = 1;
    x *= width;
    z *= width;
    x /= f;
    z /= f;
    track.sx = x;
    track.sz = z;
}

// uc_orig: TRACKS_AddQuad (fallen/Source/tracks.cpp)
void TRACKS_AddQuad(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, SLONG page, SLONG colour, UBYTE width, UBYTE flip, UBYTE flags)
{
    Track track;
    Thing* thing;

    thing = alloc_thing(CLASS_TRACK);
    if (thing) {
        track.thing = THING_NUMBER(thing);
        thing->Class = CLASS_TRACK;
        thing->WorldPos.X = x;
        thing->WorldPos.Y = y;
        thing->WorldPos.Z = z;
        thing->DrawType = DT_TRACK;
        thing->Flags = 0;
        add_thing_to_map(thing);
        track.dx = dx;
        track.dy = dy;
        track.dz = dz;
        track.page = page;
        track.colour = colour;
        track.flip = flip;
        track.flags = flags;
        track.splut = 0;
        track.splutmax = 1 + (width >> 1);
        TRACKS_CalcDiffs(track, width);
        TRACKS_AddTrack(track);
    }
}

// uc_orig: TRACKS_AddTrack (fallen/Source/tracks.cpp)
void TRACKS_AddTrack(Track& track)
{
    *(TO_TRACK(track_head)) = track;

    TO_THING(TO_TRACK(track_head)->thing)->Genus.Track = TO_TRACK(track_head);

    track_head++;
    if (track_head == track_eob)
        track_head = 0;
    if (track_head == track_tail) {
        // Buffer full: stomp the oldest entry and free its Thing.
        remove_thing_from_map(TO_THING(TO_TRACK(track_tail)->thing));
        free_thing(TO_THING(TO_TRACK(track_tail)->thing));
        track_tail++;
        if (track_tail == track_eob)
            track_tail = 0;
    }
}

// uc_orig: TRACKS_Add (fallen/Source/tracks.cpp)
UWORD TRACKS_Add(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz, UBYTE type, UWORD last)
{
    UBYTE age = last >> 8;
    UBYTE lastkind = last & 0xff;
    SLONG code, kind, page, colour;
    CBYTE msg[20];

    switch (type) {
    case TRACK_TYPE_TYRE_SKID:
        TRACKS_AddQuad(x, y, z, dx, dy, dz, POLY_PAGE_TYRESKID, 0x00ffffff, 10, 0, 0);
        break;
    case TRACK_TYPE_TYRE:
        if ((!dx) && (!dy) && (!dz))
            return last;
        if (SDIST2(dx, dz) > 25) {
            code = TRACKS_GroundAtXZ(x, z);
            switch (code) {
            case PERSON_ON_WATER:
                kind = TRACK_SURFACE_WATER;
                break;
            case PERSON_ON_GRAVEL:
            case PERSON_ON_GRASS:
                kind = TRACK_SURFACE_MUDDY;
                break;
            default:
                kind = TRACK_SURFACE_NONE;
            }
            if (lastkind != TRACK_SURFACE_NONE) {
                page = POLY_PAGE_TYRETRACK;
                switch (lastkind) {
                case TRACK_SURFACE_MUDDY:
                    colour = TRACK_MUDDY_COLOUR;
                    break;
                case TRACK_SURFACE_WATER:
                    colour = TRACK_WATER_COLOUR;
                    break;
                }
                colour += ((255 - age) << 24);
                TRACKS_AddQuad(x, y, z, dx, dy, dz, page, colour, 10, 0, 0);
            }
        }
        if (kind == TRACK_SURFACE_NONE) {
            if (age > 8) {
                age -= 8;
            } else {
                lastkind = TRACK_SURFACE_NONE;
                age = 0;
            }
        } else {
            age = 255;
            lastkind = kind;
        }
        break;
    case TRACK_TYPE_LEFT_PRINT:
    case TRACK_TYPE_RIGHT_PRINT:
        if (world_type == WORLD_TYPE_SNOW)
            kind = TRACK_SURFACE_ONSNOW;
        else {
            code = TRACKS_GroundAtXZ(x, z);
            switch (code) {
            case PERSON_ON_WATER:
                kind = TRACK_SURFACE_WATER;
                break;
            case PERSON_ON_GRAVEL:
            case PERSON_ON_GRASS:
                kind = TRACK_SURFACE_MUDDY;
                break;
            default:
                kind = TRACK_SURFACE_NONE;
            }
        }

        if (lastkind != TRACK_SURFACE_NONE) {
            page = POLY_PAGE_FOOTPRINT;
            switch (lastkind) {
            case TRACK_SURFACE_MUDDY:
                colour = TRACK_MUDDY_COLOUR;
                break;
            case TRACK_SURFACE_WATER:
                colour = TRACK_WATER_COLOUR;
                break;
            case TRACK_SURFACE_ONSNOW:
                colour = TRACK_ONSNOW_COLOUR;
            }
            colour += ((255 - age) << 24);
            TRACKS_AddQuad(x, y, z, dx, dy, dz, page, colour, 10, (type == TRACK_TYPE_RIGHT_PRINT), TRACK_FLAGS_FLIPABLE);
        }
        if (kind == TRACK_SURFACE_NONE) {
            if (age > 8) {
                age -= 8;
            } else {
                lastkind = TRACK_SURFACE_NONE;
                age = 0;
            }
        } else {
            age = 255;
            lastkind = kind;
        }
        break;
    }

    last = (age << 8) + lastkind;
    return last;
}

// uc_orig: TRACKS_GroundAtXZ (fallen/Source/tracks.cpp)
SLONG TRACKS_GroundAtXZ(SLONG X, SLONG Z)
{
    if (PUDDLE_in(X >> 8, Z >> 8))
        return PERSON_ON_WATER;

    SLONG mx = X >> 16;
    SLONG mz = Z >> 16;

    if (WITHIN(mx, 0, MAP_WIDTH - 1) && WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        SLONG page = PAP_2HI(mx, mz).Texture & 0x3ff;

        if (page == 65 || page == 66 || page == 143) {
            return PERSON_ON_WOOD;
        }

        if (page >= 69 && page <= 74) {
            return PERSON_ON_GRASS;
        }

        if (page == 68 || (page >= 106 && page <= 111)) {
            return PERSON_ON_GRAVEL;
        }
    }

    return PERSON_ON_DUNNO;
}

// uc_orig: TRACKS_Bleed (fallen/Source/tracks.cpp)
void TRACKS_Bleed(Thing* bleeder)
{
    if (!VIOLENCE)
        return;

    UBYTE sz = 1 + (rand() & 0x1f);
    UBYTE u = (Random() & 1) ? SUB_OBJECT_LEFT_FOOT : SUB_OBJECT_RIGHT_FOOT;
    SLONG dx, dr, dz, x, y, z;
    dr = rand() & 2047;
    dx = ((SIN(dr) >> 8) * sz) >> 8;
    dz = ((COS(dr) >> 8) * sz) >> 8;
    if ((dx == 0) && (dz == 0))
        dz = 1;

    calc_sub_objects_position(
        bleeder,
        bleeder->Draw.Tweened->AnimTween,
        u,
        &x,
        &y,
        &z);

    x <<= 8;
    z <<= 8;
    x += bleeder->WorldPos.X;
    z += bleeder->WorldPos.Z;
    x += (rand() & 0247) - 1024;
    z += (rand() & 2047) - 1024;
    y = (PAP_calc_map_height_at(x >> 8, z >> 8) << 8) + 257;
    TRACKS_AddQuad(x, y, z, dx, 0, dz, POLY_PAGE_BLOODSPLAT, 0x00ffffff, sz, 0, TRACK_FLAGS_SPLUTTING);
}

// uc_orig: TRACKS_Bloodpool (fallen/Source/tracks.cpp)
void TRACKS_Bloodpool(Thing* bleeder)
{
    if (!VIOLENCE)
        return;

    UBYTE sz = 80 + (rand() & 0x1f);
    SLONG dx, dr, dz, x, y, z;
    dr = rand() & 2047;
    dx = ((SIN(dr) >> 8) * sz) >> 8;
    dz = ((COS(dr) >> 8) * sz) >> 8;
    if ((dx == 0) && (dz == 0))
        dz = 1;

    calc_sub_objects_position(
        bleeder,
        bleeder->Draw.Tweened->AnimTween,
        SUB_OBJECT_PELVIS,
        &x,
        &y,
        &z);

    x <<= 8;
    z <<= 8;
    x += bleeder->WorldPos.X;
    z += bleeder->WorldPos.Z;

    x -= (dx / 2);
    z -= (dz / 2);

    y = (PAP_calc_height_at_thing(bleeder, x >> 8, z >> 8) << 8) + 257;
    TRACKS_AddQuad(x, y, z, dx, 0, dz, POLY_PAGE_BLOODSPLAT, 0x00ffffff, sz, 0, TRACK_FLAGS_SPLUTTING);
}
