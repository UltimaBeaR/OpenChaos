#include "map/supermap_globals.h"
#include "things/characters/person_types.h"

// uc_orig: level_index (fallen/Source/supermap.cpp)
ULONG level_index = 0;

// uc_orig: SUPERMAP_counter (fallen/Source/supermap.cpp)
UBYTE SUPERMAP_counter[2] = { 0, 0 };

// uc_orig: first_walkable_prim_point (fallen/Source/supermap.cpp)
SLONG first_walkable_prim_point = 0;
// uc_orig: number_of_walkable_prim_points (fallen/Source/supermap.cpp)
SLONG number_of_walkable_prim_points = 0;
// uc_orig: first_walkable_prim_face4 (fallen/Source/supermap.cpp)
SLONG first_walkable_prim_face4 = 0;
// uc_orig: number_of_walkable_prim_faces4 (fallen/Source/supermap.cpp)
SLONG number_of_walkable_prim_faces4 = 0;

// uc_orig: people_types (fallen/Source/supermap.cpp)
SWORD people_types[50] = {};

// uc_orig: DONT_load (fallen/Source/supermap.cpp)
ULONG DONT_load = 0;

// Allocation cursors for supermap arrays.
// 0 is unused sentinel — all arrays start filling from index 1.
// uc_orig: next_dbuilding (fallen/Source/supermap.cpp)
SLONG next_dbuilding = 1;
// uc_orig: next_dfacet (fallen/Source/supermap.cpp)
SLONG next_dfacet = 1;
// uc_orig: next_dstyle (fallen/Source/supermap.cpp)
SLONG next_dstyle = 1;
// uc_orig: next_dwalkable (fallen/Source/supermap.cpp)
SLONG next_dwalkable = 1;
// uc_orig: next_dstorey (fallen/Source/supermap.cpp)
SWORD next_dstorey = 1;
// uc_orig: next_paint_mem (fallen/Source/supermap.cpp)
SWORD next_paint_mem = 1;
// uc_orig: next_facet_link (fallen/Source/supermap.cpp)
SWORD next_facet_link = 1;
// uc_orig: facet_link_count (fallen/Source/supermap.cpp)
SWORD facet_link_count = 0;
// uc_orig: next_inside_mem (fallen/Source/supermap.cpp)
SLONG next_inside_mem = 1;

// uc_orig: levels (fallen/Source/supermap.cpp)
struct Levels levels[] = {
    { "FTutor1",     "fight1.map",       1,  0 },
    { "assault1",    "assault.map",      2,  0 },
    { "testdrive1a", "oval1.map",        3,  0 },
    { "fight1",      "fight1.map",       4,  0 },
    { "police1",     "disturb1.map",     5,  0 },
    { "testdrive2",  "road4_2.map",      6,  0 },
    { "fight2",      "fight1.map",       7,  0 },
    { "police2",     "disturb1.map",     8,  0 },
    { "testdrive3",  "road4_3.map",      9,  0 },
    { "bankbomb1",   "gang1.map",        10, 0 },
    { "police3",     "disturb1.map",     11, 0 },
    { "gangorder2",  "gang1.map",        12, 0 },
    { "police4",     "disturb1.map",     13, 0 },
    { "bball2",      "bball2.map",       14, 0 },
    { "wstores1",    "gpost3.map",       15, (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) },
    { "\\e3",        "snocrime.map",     16, (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) },
    { "westcrime1",  "disturb1.map",     17, 0 },
    { "carbomb1",    "gang1.map",        18, 0 },
    { "botanicc",    "botanicc.map",     19, 0 },
    { "semtex",      "snocrime.map",     20, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MIB1) },
    { "awol1",       "gpost3.map",       21, (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | (1 << PERSON_MIB1) },
    { "mission2",    "snocrime.map",     22, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) },
    { "park2",       "parkypsx03.map",   23, 0 },
    { "mib",         "southside.map",    24, (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) },
    { "skymiss2",    "skymap30.map",     25, 0 },
    { "factory1",    "tv1.map",          26, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) },
    { "estate2",     "nestate01.map",    27, 0 },
    { "stealtst1",   "stealth1.map",     28, 0 },
    { "snow2",       "snow2.map",        29, 0 },
    { "gordout1",    "botanicc.map",     30, 0 },
    { "jung3",       "jung3.map",        31, 0 },
    { "BAALROG3",    "Balbash1.map",     32, 0 },
    { "finale1",     "Rooftest2.map",    33, 0 },
    { "Gangorder1",  "gang1.map",        34, 0 },
    { "",            "",                 0,  0 }
};
