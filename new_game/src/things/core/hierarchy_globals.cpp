#include "things/core/hierarchy_globals.h"

// uc_orig: body_part_parent (fallen/Source/Hierarchy.cpp)
CBYTE* body_part_parent[][2] = {
    { "pelvis", "" },
    { "lfemur", "pelvis" },
    { "ltibia", "lfemur" },
    { "lfoot", "ltibia" },
    { "torso", "pelvis" },
    { "rhumorus", "torso" },
    { "rradius", "rhumorus" },
    { "rhand", "rradius" },
    { "lhumorus", "torso" },
    { "lradius", "lhumorus" },
    { "lhand", "lradius" },
    { "skull", "torso" },
    { "rfemur", "pelvis" },
    { "rtibia", "rfemur" },
    { "rfoot", "rtibia" }
};

// uc_orig: body_part_parent_numbers (fallen/Source/Hierarchy.cpp)
// Index of parent bone for each of the 15 bones. -1 = root.
SLONG body_part_parent_numbers[] = {
    -1,
    0,
    1,
    2,
    0,
    4,
    5,
    6,
    4,
    8,
    9,
    4,
    0,
    12,
    13
};

// uc_orig: body_part_children (fallen/Source/Hierarchy.cpp)
// Each entry lists up to 5 child bone indices, terminated by -1.
SLONG body_part_children[][5] = {
    { 1, 4, 12, -1, 0 },
    { 2, -1, 0, 0, 0 },
    { 3, -1, 0, 0, 0 },
    { -1, 0, 0, 0, 0 },
    { 5, 8, 11, -1, 0 },
    { 6, -1, 0, 0, 0 },
    { 7, -1, 0, 0, 0 },
    { -1, 0, 0, 0, 0 },
    { 9, -1, 0, 0, 0 },
    { 10, -1, 0, 0, 0 },
    { -1, 0, 0, 0, 0 },
    { -1, 0, 0, 0, 0 },
    { 13, -1, 0, 0, 0 },
    { 14, -1, 0, 0, 0 },
    { -1, 0, 0, 0, 0 }
};
