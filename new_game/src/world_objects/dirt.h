#ifndef WORLD_OBJECTS_DIRT_H
#define WORLD_OBJECTS_DIRT_H

#include "engine/core/types.h"

// Ambient debris and interactive throwable objects.
// Manages a pool of 1024 dirt entries that float near the camera focus.
// Types include leaves, cans, pigeons, water droplets, sparks, blood, snow, etc.
// Rendered by the caller via DIRT_get_info().

// uc_orig: DIRT_TYPE_UNUSED (fallen/Headers/dirt.h)
#define DIRT_TYPE_UNUSED 0
// uc_orig: DIRT_TYPE_LEAF (fallen/Headers/dirt.h)
#define DIRT_TYPE_LEAF 1
// uc_orig: DIRT_TYPE_CAN (fallen/Headers/dirt.h)
#define DIRT_TYPE_CAN 2
// uc_orig: DIRT_TYPE_PIGEON (fallen/Headers/dirt.h)
#define DIRT_TYPE_PIGEON 3
// uc_orig: DIRT_TYPE_WATER (fallen/Headers/dirt.h)
#define DIRT_TYPE_WATER 4
// uc_orig: DIRT_TYPE_HELDCAN (fallen/Headers/dirt.h)
#define DIRT_TYPE_HELDCAN 5
// uc_orig: DIRT_TYPE_THROWCAN (fallen/Headers/dirt.h)
#define DIRT_TYPE_THROWCAN 6
// uc_orig: DIRT_TYPE_HEAD (fallen/Headers/dirt.h)
#define DIRT_TYPE_HEAD 7
// uc_orig: DIRT_TYPE_HELDHEAD (fallen/Headers/dirt.h)
#define DIRT_TYPE_HELDHEAD 8
// uc_orig: DIRT_TYPE_THROWHEAD (fallen/Headers/dirt.h)
#define DIRT_TYPE_THROWHEAD 9
// uc_orig: DIRT_TYPE_BRASS (fallen/Headers/dirt.h)
#define DIRT_TYPE_BRASS 10
// uc_orig: DIRT_TYPE_MINE (fallen/Headers/dirt.h)
#define DIRT_TYPE_MINE 11
// uc_orig: DIRT_TYPE_URINE (fallen/Headers/dirt.h)
#define DIRT_TYPE_URINE 12
// uc_orig: DIRT_TYPE_SPARKS (fallen/Headers/dirt.h)
#define DIRT_TYPE_SPARKS 13
// uc_orig: DIRT_TYPE_BLOOD (fallen/Headers/dirt.h)
#define DIRT_TYPE_BLOOD 14
// uc_orig: DIRT_TYPE_SNOW (fallen/Headers/dirt.h)
#define DIRT_TYPE_SNOW 15
// uc_orig: DIRT_TYPE_NUMBER (fallen/Headers/dirt.h)
#define DIRT_TYPE_NUMBER 16

// uc_orig: DIRT_Dirt (fallen/Headers/dirt.h)
typedef struct
{
    UBYTE type;
    UBYTE owner;
    UBYTE flag;
    UBYTE counter;

    union {
        struct
        {
            UBYTE state;
            UBYTE morph1;
            UBYTE morph2;
            UBYTE tween;
        } Pidgeon;
        struct
        {
            UWORD col;
            UWORD fade;
        } Leaf;
        struct
        {
            UWORD prim;
        } Head;
        struct
        {
            UWORD timer;
        } ThingWithTime;
    } UU;

    SWORD dyaw;
    SWORD dpitch;
    SWORD droll;

    SWORD yaw;
    SWORD pitch;
    SWORD roll;

    SWORD x;
    SWORD y;
    SWORD z;

    SWORD dx;
    SWORD dy;
    SWORD dz;

} DIRT_Dirt;

// --- Leaf/dirt field extent & density --------------------------------------
// One knob: the focus radius (the Darci-centred circle within which ambient
// leaves/dirt are kept). Original PC = 0xc00 (PSX used 0x800); we run it ~20%
// larger (0xE66) for a bigger field. Change this freely to make the field
// bigger/smaller — the pool below rescales so the leaf DENSITY stays constant
// (density is referenced against DIRT_DENSITY_REF_RADIUS, NOT this).
// uc_orig: DIRT_set_focus radius arg (fallen/Source/Controls.cpp, PC = 0xc00)
#define DIRT_FOCUS_RADIUS (0xE66)

// Forward distance (map units) from the ACTIVE camera to the VIRTUAL leaf focus
// point — a ground point in front of the camera that the leaf field centres on
// (instead of Darci). ~= the normal camera follow distance (fc->cam_dist ~
// 0x280) so in normal play the point sits roughly on Darci; kept FIXED (not the
// live cam_dist) so the field stays tied to the VIEW — it shifts on zoom and
// follows the cutscene camera, instead of looking glued to Darci/the car. Tune.
#define DIRT_CAM_FOCUS_DIST (0x280)

// Original PC density reference: this many slots at this radius. (Original PC
// pool was 1024 at radius 0xc00 — see fallen/Headers/dirt.h + Controls.cpp.)
#define DIRT_DENSITY_REF_POOL (1024)
#define DIRT_DENSITY_REF_RADIUS (0xc00)

// Live pool size = density * radius^2, i.e. derived from DIRT_FOCUS_RADIUS so
// the density (slots per area) always equals the original. It is exactly
// DIRT_DENSITY_REF_POOL (1024) when DIRT_FOCUS_RADIUS == the reference radius;
// at the current 0xE66 (radius +20%) it works out to ~1474 slots. The pool need
// NOT be a power of two — slot picking uses modulo (see dirt.cpp), not a
// bitmask. 64-bit intermediate: 1024 * radius^2 overflows 32-bit.
// uc_orig: DIRT_MAX_DIRT (fallen/Headers/dirt.h) — was a fixed 1024
#define DIRT_MAX_DIRT ((SLONG)((long long)DIRT_DENSITY_REF_POOL * (DIRT_FOCUS_RADIUS) * (DIRT_FOCUS_RADIUS) / ((long long)DIRT_DENSITY_REF_RADIUS * (DIRT_DENSITY_REF_RADIUS))))

// uc_orig: DIRT_FLAG_STILL (fallen/Headers/dirt.h)
#define DIRT_FLAG_STILL (1 << 0)
// uc_orig: DIRT_FLAG_HIT_FLOOR (fallen/Headers/dirt.h)
#define DIRT_FLAG_HIT_FLOOR (1 << 1)
// uc_orig: DIRT_FLAG_DELETE_OK (fallen/Headers/dirt.h)
#define DIRT_FLAG_DELETE_OK (1 << 2)

// VIRTUAL ambient leaf (NOT in the original). A LEAF slot whose spawn position
// landed on a cell that can't hold a leaf (building / water / off-map). It
// keeps the position and occupies the pool slot, but is never drawn or
// processed — it is the "hole". It is range-checked and recycled exactly like a
// real leaf, so the field holds a constant real+virtual count: real leaves keep
// a uniform density on valid ground, holes stay holes, and moving never leaves
// a bald patch (every freed slot is immediately re-placed as real or virtual).
#define DIRT_FLAG_VIRTUAL (1 << 3)

// uc_orig: DIRT_MARK_AS_OFFSCREEN_QUICK (fallen/Headers/dirt.h)
#define DIRT_MARK_AS_OFFSCREEN_QUICK(which)           \
    {                                                 \
        DIRT_dirt[which].flag |= DIRT_FLAG_DELETE_OK; \
    }

struct Thing;

// uc_orig: DIRT_init (fallen/Headers/dirt.h)
void DIRT_init(
    SLONG prob_leaf,
    SLONG prob_can,
    SLONG prob_pigeon,
    SLONG pigeon_map_x1,
    SLONG pigeon_map_z1,
    SLONG pigeon_map_x2,
    SLONG pigeon_map_z2);

// uc_orig: DIRT_set_focus (fallen/Headers/dirt.h)
void DIRT_set_focus(SLONG x, SLONG z, SLONG radius);

// Request a whole-disc refill (even-density fill from centre to edge) over the
// next few DIRT_set_focus calls — used after a camera cut/teleport where the
// focus lands somewhere the edge-only top-up would leave a bald leading edge.
// DIRT_set_focus also calls this itself on big focus jumps; callers use it for
// cuts too small for that (e.g. a short cutscene camera jump near Darci).
void DIRT_request_refill(void);

// uc_orig: DIRT_gust (fallen/Headers/dirt.h)
void DIRT_gust(Thing* p_thing, SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: DIRT_new_water (fallen/Headers/dirt.h)
void DIRT_new_water(SLONG x, SLONG y, SLONG z, SLONG dx, SLONG dy, SLONG dz,
    SLONG dirt_type = DIRT_TYPE_WATER);

// uc_orig: DIRT_process (fallen/Headers/dirt.h)
void DIRT_process(void);

// uc_orig: DIRT_get_nearest_can_or_head_dist (fallen/Headers/dirt.h)
SLONG DIRT_get_nearest_can_or_head_dist(SLONG x, SLONG y, SLONG z);

// uc_orig: DIRT_pick_up_can_or_head (fallen/Headers/dirt.h)
void DIRT_pick_up_can_or_head(Thing* p_person);

// uc_orig: DIRT_release_can_or_head (fallen/Headers/dirt.h)
void DIRT_release_can_or_head(Thing* p_person, SLONG power);

// uc_orig: DIRT_create_grenade (fallen/Headers/dirt.h)
void DIRT_create_grenade(Thing* p_person, SLONG ticks_to_go, SLONG power);

// uc_orig: DIRT_shoot (fallen/Headers/dirt.h)
SLONG DIRT_shoot(Thing* p_person);

// uc_orig: DIRT_create_papers (fallen/Headers/dirt.h)
void DIRT_create_papers(SLONG x, SLONG y, SLONG z);

// uc_orig: DIRT_create_cans (fallen/Headers/dirt.h)
void DIRT_create_cans(SLONG x, SLONG z, SLONG angle);

// uc_orig: DIRT_mark_as_offscreen (fallen/Headers/dirt.h)
void DIRT_mark_as_offscreen(SLONG which);

// uc_orig: DIRT_new_sparks (fallen/Headers/dirt.h)
void DIRT_new_sparks(SLONG px, SLONG py, SLONG pz, UBYTE dir);

// Not in original header but referenced as extern in Person.cpp.
// uc_orig: DIRT_create_brass (fallen/Source/dirt.cpp)
void DIRT_create_brass(SLONG x, SLONG y, SLONG z, SLONG angle);

// uc_orig: DIRT_dirt (fallen/Headers/dirt.h)
extern DIRT_Dirt DIRT_dirt[DIRT_MAX_DIRT];

#endif // WORLD_OBJECTS_DIRT_H
