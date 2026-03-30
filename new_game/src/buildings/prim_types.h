#ifndef BUILDINGS_PRIM_TYPES_H
#define BUILDINGS_PRIM_TYPES_H

// Prim geometry types, face flags, and prim object/collision constants.
// These are the binary-format-compatible data structures for static world geometry
// (vertex pools, face pools, object headers) loaded from .SEX/.GOB files.

#include "engine/core/types.h"
#include "engine/core/vector.h"  // SVector (used for PrimNormal typedef)
#include "engine/core/fmatrix.h" // Matrix33, CMAT, SMAT macros

// =====================================================================
// Pool size limits (static and dynamic)
// =====================================================================

// Static upper bounds (used for validation).
// uc_orig: RMAX_PRIM_POINTS (fallen/Headers/building.h)
#define RMAX_PRIM_POINTS   65000
// uc_orig: MAX_PRIM_FACES3 (fallen/Headers/building.h)
#define MAX_PRIM_FACES3    32000
// uc_orig: RMAX_PRIM_FACES4 (fallen/Headers/building.h)
#define RMAX_PRIM_FACES4   32760
// uc_orig: MAX_PRIM_OBJECTS (fallen/Headers/building.h)
#define MAX_PRIM_OBJECTS   2000
// uc_orig: MAX_PRIM_MOBJECTS (fallen/Headers/building.h)
#define MAX_PRIM_MOBJECTS  100

// Dynamic limits — driven by save_table[] at level load.
// Note: save_table is in missions/memory_globals.h (circular dep guard: just use
// the save_table reference directly; consumers include memory_globals.h separately).
// uc_orig: MAX_PRIM_POINTS (fallen/Headers/building.h)
#define MAX_PRIM_POINTS  (save_table[SAVE_TABLE_POINTS].Maximum)
// uc_orig: MAX_PRIM_FACES4 (fallen/Headers/building.h)
#define MAX_PRIM_FACES4  (save_table[SAVE_TABLE_FACE4].Maximum)

// Sentinel values for walkable face identification.
// uc_orig: GRAB_FLOOR (fallen/Headers/building.h)
#define GRAB_FLOOR   (MAX_PRIM_FACES4 + 1)
// uc_orig: GRAB_SEWERS (fallen/Headers/building.h)
#define GRAB_SEWERS  (MAX_PRIM_FACES4 + 2)

// Roof bound pool limit.
// uc_orig: MAX_ROOF_BOUND (fallen/Headers/building.h)
#define MAX_ROOF_BOUND 2000

// Secondary roof face pool limit (memory_globals).
// uc_orig: MAX_ROOF_FACE4 (fallen/Headers/memory.h)
#define MAX_ROOF_FACE4 10000

// =====================================================================
// World scale
// =====================================================================

// Units per map square (8-bit shift precision).
// uc_orig: GAME_SCALE (fallen/Headers/Prim.h)
#define GAME_SCALE     (256.0f)
// uc_orig: GAME_SCALE_DIV (fallen/Headers/Prim.h)
#define GAME_SCALE_DIV (100.0f)

// =====================================================================
// Face save type range (for editor persistence)
// =====================================================================

// uc_orig: PRIM_START_SAVE_TYPE (fallen/Headers/Prim.h)
#define PRIM_START_SAVE_TYPE 5793
// uc_orig: PRIM_END_SAVE_TYPE (fallen/Headers/Prim.h)
#define PRIM_END_SAVE_TYPE   5800

// =====================================================================
// RoofFace4 flags
// =====================================================================

// Shadow contribution bits (3 bits, combine into 0-7 shadow level).
// uc_orig: RFACE_FLAG_SHADOW_1 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SHADOW_1    (1 << 0)
// uc_orig: RFACE_FLAG_SHADOW_2 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SHADOW_2    (1 << 1)
// uc_orig: RFACE_FLAG_SHADOW_3 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SHADOW_3    (1 << 2)
// uc_orig: RFACE_FLAG_SLIDE_EDGE (fallen/Headers/Prim.h)
#define RFACE_FLAG_SLIDE_EDGE   (1 << 3)
// uc_orig: RFACE_FLAG_SLIDE_EDGE_0 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SLIDE_EDGE_0 (1 << 3)
// uc_orig: RFACE_FLAG_SLIDE_EDGE_1 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SLIDE_EDGE_1 (1 << 4)
// uc_orig: RFACE_FLAG_SLIDE_EDGE_2 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SLIDE_EDGE_2 (1 << 5)
// uc_orig: RFACE_FLAG_SLIDE_EDGE_3 (fallen/Headers/Prim.h)
#define RFACE_FLAG_SLIDE_EDGE_3 (1 << 6)

// Bit 7: originally SPLIT, repurposed as NODRAW (temporary flag).
// uc_orig: RFACE_FLAG_SPLIT (fallen/Headers/Prim.h)
#define RFACE_FLAG_SPLIT   (1 << 7)
// uc_orig: RFACE_FLAG_NODRAW (fallen/Headers/Prim.h)
#define RFACE_FLAG_NODRAW  (1 << 7)

// =====================================================================
// PrimFace flags (FaceFlags field in PrimFace3/PrimFace4)
// =====================================================================

// Moving walkable face — ThingIndex indexes the WMOVE_face array.
// uc_orig: FACE_FLAG_WMOVE (fallen/Headers/Prim.h)
#define FACE_FLAG_WMOVE      (1 << 0)
// uc_orig: FACE_FLAG_SMOOTH (fallen/Headers/Prim.h)
#define FACE_FLAG_SMOOTH     (1 << 0)
// uc_orig: FACE_FLAG_OUTLINE (fallen/Headers/Prim.h)
#define FACE_FLAG_OUTLINE    (1 << 1)
// uc_orig: FACE_FLAG_SHADOW_1 (fallen/Headers/Prim.h)
#define FACE_FLAG_SHADOW_1   (1 << 2)
// uc_orig: FACE_FLAG_SHADOW_2 (fallen/Headers/Prim.h)
#define FACE_FLAG_SHADOW_2   (1 << 3)
// uc_orig: FACE_FLAG_SHADOW_3 (fallen/Headers/Prim.h)
#define FACE_FLAG_SHADOW_3   (1 << 4)
// uc_orig: FACE_FLAG_ROOF (fallen/Headers/Prim.h)
#define FACE_FLAG_ROOF       (1 << 5)
// uc_orig: FACE_FLAG_WALKABLE (fallen/Headers/Prim.h)
#define FACE_FLAG_WALKABLE   (1 << 6)
// uc_orig: FACE_FLAG_ENVMAP (fallen/Headers/Prim.h)
#define FACE_FLAG_ENVMAP     (1 << 7)
// uc_orig: FACE_FLAG_VERTICAL (fallen/Headers/Prim.h)
#define FACE_FLAG_VERTICAL   (1 << 7)

// uc_orig: FACE_FLAG_TINT (fallen/Headers/Prim.h)
#define FACE_FLAG_TINT       (1 << 8)
// uc_orig: FACE_FLAG_ANIMATE (fallen/Headers/Prim.h)
#define FACE_FLAG_ANIMATE    (1 << 9)
// uc_orig: FACE_FLAG_OTHER_SPLIT (fallen/Headers/Prim.h)
#define FACE_FLAG_OTHER_SPLIT (1 << 10)
// uc_orig: FACE_FLAG_METAL (fallen/Headers/Prim.h)
#define FACE_FLAG_METAL      (1 << 10)
// uc_orig: FACE_FLAG_NON_PLANAR (fallen/Headers/Prim.h)
#define FACE_FLAG_NON_PLANAR (1 << 11)
// uc_orig: FACE_FLAG_TEX2 (fallen/Headers/Prim.h)
#define FACE_FLAG_TEX2       (1 << 12)
// uc_orig: FACE_FLAG_FIRE_ESCAPE (fallen/Headers/Prim.h)
#define FACE_FLAG_FIRE_ESCAPE (1 << 13)
// uc_orig: FACE_FLAG_PRIM (fallen/Headers/Prim.h)
#define FACE_FLAG_PRIM       (1 << 14)
// uc_orig: FACE_FLAG_THUG_JACKET (fallen/Headers/Prim.h)
#define FACE_FLAG_THUG_JACKET (1 << 15)

// Set after texture page was fixed by TEXTURE_fix_prim_textures().
// uc_orig: FACE_FLAG_FIXED (fallen/Headers/Prim.h)
#define FACE_FLAG_FIXED      (1 << 5)

// Slide edge flags — mark which edge of a walkable quad does not connect to a neighbour.
// uc_orig: FACE_FLAG_SLIDE_EDGE (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE    (1 << 1)
// uc_orig: FACE_FLAG_SLIDE_EDGE_0 (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE_0  (1 << 1)
// uc_orig: FACE_FLAG_SLIDE_EDGE_1 (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE_1  (1 << 2)
// uc_orig: FACE_FLAG_SLIDE_EDGE_2 (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE_2  (1 << 3)
// uc_orig: FACE_FLAG_SLIDE_EDGE_3 (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE_3  (1 << 4)
// uc_orig: FACE_FLAG_SLIDE_EDGE_ALL (fallen/Headers/Prim.h)
#define FACE_FLAG_SLIDE_EDGE_ALL (0xf << 1)

// Used only during SEX object loading.
// uc_orig: FACE_FLAG_EDGE_VISIBLE (fallen/Headers/Prim.h)
#define FACE_FLAG_EDGE_VISIBLE    (1 << 13)
// uc_orig: FACE_FLAG_EDGE_VISIBLE_A (fallen/Headers/Prim.h)
#define FACE_FLAG_EDGE_VISIBLE_A  (FACE_FLAG_EDGE_VISIBLE << 0)
// uc_orig: FACE_FLAG_EDGE_VISIBLE_B (fallen/Headers/Prim.h)
#define FACE_FLAG_EDGE_VISIBLE_B  (FACE_FLAG_EDGE_VISIBLE << 1)
// uc_orig: FACE_FLAG_EDGE_VISIBLE_C (fallen/Headers/Prim.h)
#define FACE_FLAG_EDGE_VISIBLE_C  (FACE_FLAG_EDGE_VISIBLE << 2)

// Face type bits (separate from FaceFlags — stored in PrimFace3/4.Type).
// uc_orig: FACE_TYPE_FENCE (fallen/Headers/Prim.h)
#define FACE_TYPE_FENCE    (1 << 1)
// uc_orig: FACE_TYPE_CABLE (fallen/Headers/Prim.h)
#define FACE_TYPE_CABLE    (1 << 2)
// uc_orig: FACE_TYPE_SKYLIGHT (fallen/Headers/Prim.h)
#define FACE_TYPE_SKYLIGHT (1 << 3)
// uc_orig: FACE_TYPE_PRIM (fallen/Headers/Prim.h)
#define FACE_TYPE_PRIM     (1 << 4)

// Sort level helpers — packed into bits 2-4 of a flags byte.
// uc_orig: GET_SORT_LEVEL (fallen/Headers/Prim.h)
#define GET_SORT_LEVEL(f)    (((f) >> 2) & 7)
// uc_orig: OR_SORT_LEVEL (fallen/Headers/Prim.h)
#define OR_SORT_LEVEL(u, f)  u |= ((f) << 2)
// uc_orig: SET_SORT_LEVEL (u, f) (fallen/Headers/Prim.h)
#define SET_SORT_LEVEL(u, f) \
    u &= ~(7 << 2);          \
    u |= ((f) << 2)

// =====================================================================
// Texture page flags (top bits of wTexturePage in PrimFace3/4)
// =====================================================================

// uc_orig: TEXTURE_PAGE_FLAG_JACKET (fallen/Headers/Prim.h)
#define TEXTURE_PAGE_FLAG_JACKET       (1 << 15)
// uc_orig: TEXTURE_PAGE_FLAG_OFFSET (fallen/Headers/Prim.h)
#define TEXTURE_PAGE_FLAG_OFFSET       (1 << 14)
// uc_orig: TEXTURE_PAGE_FLAG_TINT (fallen/Headers/Prim.h)
#define TEXTURE_PAGE_FLAG_TINT         (1 << 13)
// uc_orig: TEXTURE_PAGE_FLAG_NOT_TEXTURED (fallen/Headers/Prim.h)
#define TEXTURE_PAGE_FLAG_NOT_TEXTURED (1 << 12)
// uc_orig: TEXTURE_PAGE_MASK (fallen/Headers/Prim.h)
#define TEXTURE_PAGE_MASK              (0x0fff)

// =====================================================================
// Prim object types (PRIM_OBJ_* indices into prim_objects[])
// =====================================================================

// uc_orig: PRIM_OBJ_LAMPPOST (fallen/Headers/Prim.h)
#define PRIM_OBJ_LAMPPOST           1
// uc_orig: PRIM_OBJ_TRAFFIC_LIGHT (fallen/Headers/Prim.h)
#define PRIM_OBJ_TRAFFIC_LIGHT      2
// uc_orig: PRIM_OBJ_WALK_DONT_WALK (fallen/Headers/Prim.h)
#define PRIM_OBJ_WALK_DONT_WALK     3
// uc_orig: PRIM_OBJ_PETROL_PUMP (fallen/Headers/Prim.h)
#define PRIM_OBJ_PETROL_PUMP        4
// uc_orig: PRIM_OBJ_DOUBLE_PETROL_PUMP (fallen/Headers/Prim.h)
#define PRIM_OBJ_DOUBLE_PETROL_PUMP 5
// uc_orig: PRIM_OBJ_GAS_STATION_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_GAS_STATION_SIGN   6
// uc_orig: PRIM_OBJ_BILLBOARD (fallen/Headers/Prim.h)
#define PRIM_OBJ_BILLBOARD          7
// uc_orig: PRIM_OBJ_GAS_STATION_BBOARD (fallen/Headers/Prim.h)
#define PRIM_OBJ_GAS_STATION_BBOARD 8
// uc_orig: PRIM_OBJ_HOTEL_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_HOTEL_SIGN         9
// uc_orig: PRIM_OBJ_GAS_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_GAS_SIGN           10
// uc_orig: PRIM_OBJ_GAS_STATION_LIGHT (fallen/Headers/Prim.h)
#define PRIM_OBJ_GAS_STATION_LIGHT  11
// uc_orig: PRIM_OBJ_FIRE_ESCAPE1 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE1       12
// uc_orig: PRIM_OBJ_FIRE_ESCAPE2 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE2       13
// uc_orig: PRIM_OBJ_FIRE_ESCAPE3 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE3       14
// uc_orig: PRIM_OBJ_FIRE_ESCAPE4 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE4       15
// uc_orig: PRIM_OBJ_FIRE_ESCAPE5 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE5       16
// uc_orig: PRIM_OBJ_FIRE_ESCAPE6 (fallen/Headers/Prim.h)
#define PRIM_OBJ_FIRE_ESCAPE6       17
// uc_orig: PRIM_OBJ_TYRE_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_TYRE_SIGN          18
// uc_orig: PRIM_OBJ_CANOPY1 (fallen/Headers/Prim.h)
#define PRIM_OBJ_CANOPY1            19
// uc_orig: PRIM_OBJ_CANOPY2 (fallen/Headers/Prim.h)
#define PRIM_OBJ_CANOPY2            20
// uc_orig: PRIM_OBJ_CANOPY3 (fallen/Headers/Prim.h)
#define PRIM_OBJ_CANOPY3            21
// uc_orig: PRIM_OBJ_CANOPY4 (fallen/Headers/Prim.h)
#define PRIM_OBJ_CANOPY4            22
// uc_orig: PRIM_OBJ_DINER_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_DINER_SIGN         23
// uc_orig: PRIM_OBJ_MOTEL_SIGN1 (fallen/Headers/Prim.h)
#define PRIM_OBJ_MOTEL_SIGN1        25
// uc_orig: PRIM_OBJ_MOTEL_SIGN2 (fallen/Headers/Prim.h)
#define PRIM_OBJ_MOTEL_SIGN2        26
// uc_orig: PRIM_OBJ_WILDCATVAN_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_WILDCATVAN_BODY    27
// uc_orig: PRIM_OBJ_INTERIOR_STAIRS2 (fallen/Headers/Prim.h)
#define PRIM_OBJ_INTERIOR_STAIRS2   28
// uc_orig: PRIM_OBJ_INTERIOR_STAIRS3 (fallen/Headers/Prim.h)
#define PRIM_OBJ_INTERIOR_STAIRS3   29
// uc_orig: PRIM_OBJ_RADIATOR (fallen/Headers/Prim.h)
#define PRIM_OBJ_RADIATOR           30
// uc_orig: PRIM_OBJ_MOTORBIKE (fallen/Headers/Prim.h)
#define PRIM_OBJ_MOTORBIKE          31
// uc_orig: PRIM_OBJ_BIN (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIN                33
// uc_orig: PRIM_OBJ_ITEM_KEY (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_KEY           36
// uc_orig: PRIM_OBJ_LION2 (fallen/Headers/Prim.h)
#define PRIM_OBJ_LION2              46
// uc_orig: PRIM_OBJ_BIKE (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIKE               47
// uc_orig: PRIM_OBJ_VAN (fallen/Headers/Prim.h)
#define PRIM_OBJ_VAN                48
// uc_orig: PRIM_OBJ_BOAT (fallen/Headers/Prim.h)
#define PRIM_OBJ_BOAT               49
// uc_orig: PRIM_OBJ_CAR (fallen/Headers/Prim.h)
#define PRIM_OBJ_CAR                50
// uc_orig: PRIM_OBJ_STROBE (fallen/Headers/Prim.h)
#define PRIM_OBJ_STROBE             51
// uc_orig: PRIM_OBJ_DOOR (fallen/Headers/Prim.h)
#define PRIM_OBJ_DOOR               52
// uc_orig: PRIM_OBJ_LAMP (fallen/Headers/Prim.h)
#define PRIM_OBJ_LAMP               54
// uc_orig: PRIM_OBJ_SIGN (fallen/Headers/Prim.h)
#define PRIM_OBJ_SIGN               60
// uc_orig: PRIM_OBJ_TRAFFIC_CONE (fallen/Headers/Prim.h)
#define PRIM_OBJ_TRAFFIC_CONE       67
// uc_orig: PRIM_OBJ_CHOPPER (fallen/Headers/Prim.h)
#define PRIM_OBJ_CHOPPER            74
// uc_orig: PRIM_OBJ_CHOPPER_BLADES (fallen/Headers/Prim.h)
#define PRIM_OBJ_CHOPPER_BLADES     75
// uc_orig: PRIM_OBJ_CAN (fallen/Headers/Prim.h)
#define PRIM_OBJ_CAN                76
// uc_orig: PRIM_OBJ_CANOPY (fallen/Headers/Prim.h)
#define PRIM_OBJ_CANOPY             81
// uc_orig: PRIM_OBJ_ITEM_TREASURE (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_TREASURE      84
// uc_orig: PRIM_OBJ_HOOK (fallen/Headers/Prim.h)
#define PRIM_OBJ_HOOK               86
// uc_orig: PRIM_OBJ_VAN_WHEEL (fallen/Headers/Prim.h)
#define PRIM_OBJ_VAN_WHEEL          87
// uc_orig: PRIM_OBJ_VAN_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_VAN_BODY           88
// uc_orig: PRIM_OBJ_PARK_BENCH (fallen/Headers/Prim.h)
#define PRIM_OBJ_PARK_BENCH         89
// uc_orig: PRIM_OBJ_JEEP_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_JEEP_BODY          90
// uc_orig: PRIM_OBJ_MEATWAGON_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_MEATWAGON_BODY     91
// uc_orig: PRIM_OBJ_ARMCHAIR (fallen/Headers/Prim.h)
#define PRIM_OBJ_ARMCHAIR           101
// uc_orig: PRIM_OBJ_COFFEE_TABLE (fallen/Headers/Prim.h)
#define PRIM_OBJ_COFFEE_TABLE       102
// uc_orig: PRIM_OBJ_SOFA (fallen/Headers/Prim.h)
#define PRIM_OBJ_SOFA               105
// uc_orig: PRIM_OBJ_CAR_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_CAR_BODY           108
// uc_orig: PRIM_OBJ_WOODEN_TABLE (fallen/Headers/Prim.h)
#define PRIM_OBJ_WOODEN_TABLE       110
// uc_orig: PRIM_OBJ_ITEM_AK47 (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_AK47          120
// uc_orig: PRIM_OBJ_ITEM_MAGNUM (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_MAGNUM        121
// uc_orig: PRIM_OBJ_CHAIR (fallen/Headers/Prim.h)
#define PRIM_OBJ_CHAIR              126
// uc_orig: PRIM_OBJ_ITEM_SHOTGUN (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_SHOTGUN       127
// uc_orig: PRIM_OBJ_SEDAN_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_SEDAN_BODY         131
// uc_orig: PRIM_OBJ_BALLOON (fallen/Headers/Prim.h)
#define PRIM_OBJ_BALLOON            132
// uc_orig: PRIM_OBJ_BIKE_STEER (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIKE_STEER         137
// uc_orig: PRIM_OBJ_BIKE_BWHEEL (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIKE_BWHEEL        138
// uc_orig: PRIM_OBJ_BIKE_FWHEEL (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIKE_FWHEEL        139
// uc_orig: PRIM_OBJ_BIKE_FRAME (fallen/Headers/Prim.h)
#define PRIM_OBJ_BIKE_FRAME         140
// uc_orig: PRIM_OBJ_BARREL (fallen/Headers/Prim.h)
#define PRIM_OBJ_BARREL             141
// uc_orig: PRIM_OBJ_ITEM_HEALTH (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_HEALTH        142
// uc_orig: PRIM_OBJ_ITEM_GUN (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_GUN           143
// uc_orig: PRIM_OBJ_ITEM_KEYCARD (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_KEYCARD       144
// uc_orig: PRIM_OBJ_ITEM_BOMB (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_BOMB          145
// uc_orig: PRIM_OBJ_POLICE_TARGET (fallen/Headers/Prim.h)
#define PRIM_OBJ_POLICE_TARGET      147
// uc_orig: PRIM_OBJ_POLICE_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_POLICE_BODY        150
// uc_orig: PRIM_OBJ_TAXI_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_TAXI_BODY          155
// uc_orig: PRIM_OBJ_SWITCH_ON (fallen/Headers/Prim.h)
#define PRIM_OBJ_SWITCH_ON          157
// uc_orig: PRIM_OBJ_AMBULANCE_BODY (fallen/Headers/Prim.h)
#define PRIM_OBJ_AMBULANCE_BODY     159
// uc_orig: PRIM_OBJ_ITEM_KNIFE (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_KNIFE         166
// uc_orig: PRIM_OBJ_ITEM_EXPLOSIVES (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_EXPLOSIVES    167
// uc_orig: PRIM_OBJ_ITEM_GRENADE (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_GRENADE       168
// uc_orig: PRIM_OBJ_TRIPWIRE (fallen/Headers/Prim.h)
#define PRIM_OBJ_TRIPWIRE           178
// uc_orig: PRIM_OBJ_MINE (fallen/Headers/Prim.h)
#define PRIM_OBJ_MINE               188
// uc_orig: PRIM_OBJ_SPIKE (fallen/Headers/Prim.h)
#define PRIM_OBJ_SPIKE              189
// uc_orig: PRIM_OBJ_VALVE (fallen/Headers/Prim.h)
#define PRIM_OBJ_VALVE              196
// uc_orig: PRIM_OBJ_THERMODROID (fallen/Headers/Prim.h)
#define PRIM_OBJ_THERMODROID        201
// uc_orig: PRIM_OBJ_ITEM_BASEBALLBAT (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_BASEBALLBAT   205
// uc_orig: PRIM_OBJ_SWITCH_OFF (fallen/Headers/Prim.h)
#define PRIM_OBJ_SWITCH_OFF         216
// uc_orig: PRIM_OBJ_CAR_WHEEL (fallen/Headers/Prim.h)
#define PRIM_OBJ_CAR_WHEEL          220
// uc_orig: PRIM_OBJ_HYDRANT (fallen/Headers/Prim.h)
#define PRIM_OBJ_HYDRANT            228
// uc_orig: PRIM_OBJ_ITEM_FILE (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_FILE          232
// uc_orig: PRIM_OBJ_ITEM_FLOPPY_DISK (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_FLOPPY_DISK   233
// uc_orig: PRIM_OBJ_ITEM_CROWBAR (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_CROWBAR       234
// uc_orig: PRIM_OBJ_ITEM_GASMASK (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_GASMASK       235
// uc_orig: PRIM_OBJ_ITEM_WRENCH (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_WRENCH        236
// uc_orig: PRIM_OBJ_ITEM_VIDEO (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_VIDEO         237
// uc_orig: PRIM_OBJ_ITEM_GLOVES (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_GLOVES        238
// uc_orig: PRIM_OBJ_ITEM_WEEDKILLER (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_WEEDKILLER    239
// uc_orig: PRIM_OBJ_ITEM_AMMO_SHOTGUN (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_AMMO_SHOTGUN  253
// uc_orig: PRIM_OBJ_ITEM_AMMO_AK47 (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_AMMO_AK47     254
// uc_orig: PRIM_OBJ_ITEM_AMMO_PISTOL (fallen/Headers/Prim.h)
#define PRIM_OBJ_ITEM_AMMO_PISTOL   255
// uc_orig: PRIM_OBJ_NUMBER (fallen/Headers/Prim.h)
#define PRIM_OBJ_NUMBER             256

// Weapon flash / melee prim IDs (D3D-era extensions).
// uc_orig: PRIM_OBJ_WEAPON_GUN (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_GUN          256
// uc_orig: PRIM_OBJ_WEAPON_KNIFE (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_KNIFE        257
// uc_orig: PRIM_OBJ_WEAPON_SHOTGUN (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_SHOTGUN      258
// uc_orig: PRIM_OBJ_WEAPON_BAT (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_BAT          259
// uc_orig: PRIM_OBJ_WEAPON_AK47 (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_AK47         260
// uc_orig: PRIM_OBJ_WEAPON_GUN_FLASH (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_GUN_FLASH    261
// uc_orig: PRIM_OBJ_WEAPON_SHOTGUN_FLASH (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_SHOTGUN_FLASH 262
// uc_orig: PRIM_OBJ_WEAPON_AK47_FLASH (fallen/Headers/Prim.h)
#define PRIM_OBJ_WEAPON_AK47_FLASH   263

// =====================================================================
// Prim collision model (per-prim, returned by prim_get_collision_model)
// =====================================================================

// uc_orig: PRIM_COLLIDE_BOX (fallen/Headers/Prim.h)
#define PRIM_COLLIDE_BOX       0
// uc_orig: PRIM_COLLIDE_NONE (fallen/Headers/Prim.h)
#define PRIM_COLLIDE_NONE      1
// uc_orig: PRIM_COLLIDE_CYLINDER (fallen/Headers/Prim.h)
#define PRIM_COLLIDE_CYLINDER  2
// uc_orig: PRIM_COLLIDE_SMALLBOX (fallen/Headers/Prim.h)
#define PRIM_COLLIDE_SMALLBOX  3

// =====================================================================
// Prim shadow type (per-prim, returned by prim_get_shadow_type)
// =====================================================================

// uc_orig: PRIM_SHADOW_NONE (fallen/Headers/Prim.h)
#define PRIM_SHADOW_NONE     0
// uc_orig: PRIM_SHADOW_BOXEDGE (fallen/Headers/Prim.h)
#define PRIM_SHADOW_BOXEDGE  1
// uc_orig: PRIM_SHADOW_CYLINDER (fallen/Headers/Prim.h)
#define PRIM_SHADOW_CYLINDER 2
// uc_orig: PRIM_SHADOW_FOURLEGS (fallen/Headers/Prim.h)
#define PRIM_SHADOW_FOURLEGS 3
// uc_orig: PRIM_SHADOW_FULLBOX (fallen/Headers/Prim.h)
#define PRIM_SHADOW_FULLBOX  4

// =====================================================================
// Prim object flags (PrimObject.flag)
// =====================================================================

// uc_orig: PRIM_FLAG_LAMPOST (fallen/Headers/Prim.h)
#define PRIM_FLAG_LAMPOST                    (1 << 0)
// uc_orig: PRIM_FLAG_CONTAINS_WALKABLE_FACES (fallen/Headers/Prim.h)
#define PRIM_FLAG_CONTAINS_WALKABLE_FACES    (1 << 1)
// uc_orig: PRIM_FLAG_GLARE (fallen/Headers/Prim.h)
#define PRIM_FLAG_GLARE                      (1 << 2)
// uc_orig: PRIM_FLAG_ITEM (fallen/Headers/Prim.h)
#define PRIM_FLAG_ITEM                       (1 << 3)
// uc_orig: PRIM_FLAG_TREE (fallen/Headers/Prim.h)
#define PRIM_FLAG_TREE                       (1 << 4)
// uc_orig: PRIM_FLAG_ENVMAPPED (fallen/Headers/Prim.h)
#define PRIM_FLAG_ENVMAPPED                  (1 << 5)
// uc_orig: PRIM_FLAG_JUST_FLOOR (fallen/Headers/Prim.h)
#define PRIM_FLAG_JUST_FLOOR                 (1 << 6)
// uc_orig: PRIM_FLAG_ON_FLOOR (fallen/Headers/Prim.h)
#define PRIM_FLAG_ON_FLOOR                   (1 << 7)

// =====================================================================
// Prim damage flags (PrimObject.damage)
// =====================================================================

// uc_orig: PRIM_DAMAGE_DAMAGABLE (fallen/Headers/Prim.h)
#define PRIM_DAMAGE_DAMAGABLE (1 << 0)
// uc_orig: PRIM_DAMAGE_EXPLODES (fallen/Headers/Prim.h)
#define PRIM_DAMAGE_EXPLODES  (1 << 1)
// uc_orig: PRIM_DAMAGE_CRUMPLE (fallen/Headers/Prim.h)
#define PRIM_DAMAGE_CRUMPLE   (1 << 2)
// uc_orig: PRIM_DAMAGE_LEAN (fallen/Headers/Prim.h)
#define PRIM_DAMAGE_LEAN      (1 << 3)
// uc_orig: PRIM_DAMAGE_NOLOS (fallen/Headers/Prim.h)
#define PRIM_DAMAGE_NOLOS     (1 << 4)

// =====================================================================
// Animated prim type codes
// =====================================================================

// uc_orig: ANIM_PRIM_TYPE_NORMAL (fallen/Headers/Prim.h)
#define ANIM_PRIM_TYPE_NORMAL 0
// uc_orig: ANIM_PRIM_TYPE_DOOR (fallen/Headers/Prim.h)
#define ANIM_PRIM_TYPE_DOOR   1
// uc_orig: ANIM_PRIM_TYPE_SWITCH (fallen/Headers/Prim.h)
#define ANIM_PRIM_TYPE_SWITCH 2

// =====================================================================
// RoofFace shift
// =====================================================================

// Shift applied to RoofFace Y coordinates during rendering.
// uc_orig: ROOF_SHIFT (fallen/Headers/Prim.h)
#define ROOF_SHIFT 3

// WALKABLE is an alias for PrimFace4.Col2, used as a walkable-face marker.
// uc_orig: WALKABLE (fallen/Headers/Prim.h)
#define WALKABLE Col2

// =====================================================================
// Geometry structures (binary-format-compatible — do not reorder fields)
// =====================================================================

// 32-bit integer vertex (legacy editor precision).
// uc_orig: OldPrimPoint (fallen/Headers/building.h)
struct OldPrimPoint {
    SLONG X, Y, Z;
};

// Binary-file format structures: 1-byte packed for .prm / .iam / level file compatibility.
#pragma pack(push, 1)

// 16-bit integer vertex (runtime precision).
// uc_orig: PrimPoint (fallen/Headers/building.h)
struct PrimPoint {
    SWORD X, Y, Z;
};

// Vertex normal alias.
// uc_orig: PrimNormal (fallen/Headers/building.h)
typedef SVector PrimNormal;

// Walkable roof quad face.
// uc_orig: RoofFace4 (fallen/Headers/building.h)
struct RoofFace4 {
    SWORD  Y;
    SBYTE  DY[3];
    UBYTE  DrawFlags;
    UBYTE  RX;
    UBYTE  RZ;
    SWORD  Next; // linked list of walkables off floor
};

// Triangle face.
// uc_orig: PrimFace3 (fallen/Headers/building.h)
struct PrimFace3 {
    UBYTE  TexturePage;
    UBYTE  DrawFlags;
    UWORD  Points[3];
    UBYTE  UV[3][2];
    SWORD  Bright[3];
    SWORD  ThingIndex;
    UWORD  Col2;
    UWORD  FaceFlags;
    UBYTE  Type;
    SBYTE  ID;
};

// Quad face.
// uc_orig: PrimFace4 (fallen/Headers/building.h)
struct PrimFace4 {
    UBYTE TexturePage;
    UBYTE DrawFlags;
    UWORD Points[4];
    UBYTE UV[4][2];

    union {
        SWORD Bright[4]; // used for characters

        struct {
            UBYTE red;
            UBYTE green;
            UBYTE blue;
        } col; // used for building faces
    };

    SWORD ThingIndex;
    SWORD Col2;
    UWORD FaceFlags;
    UBYTE Type;
    SBYTE ID;
};

// PSX quad face (different field layout / sizes).
// uc_orig: PrimFace4PSX (fallen/Headers/building.h)
struct PrimFace4PSX {
    SWORD TexturePage;
    UBYTE AltPal;
    UBYTE DrawFlags;
    UWORD Points[4];
    UBYTE UV[4][2];
    SWORD ThingIndex;
    UWORD FaceFlags;
};

// PSX triangle face.
// uc_orig: PrimFace3PSX (fallen/Headers/building.h)
struct PrimFace3PSX {
    SWORD TexturePage;
    UBYTE AltPal;
    UBYTE DrawFlags;
    UWORD Points[3];
    UBYTE UV[3][2];
    SWORD ThingIndex;
    UWORD FaceFlags;
};

// Prim object header (index ranges into vertex/face pools).
// uc_orig: PrimObject (fallen/Headers/building.h)
struct PrimObject {
    UWORD StartPoint;
    UWORD EndPoint;
    UWORD StartFace4;
    UWORD EndFace4;
    SWORD StartFace3;
    SWORD EndFace3;

    UBYTE coltype;
    UBYTE damage;
    UBYTE shadowtype;
    UBYTE flag;
};

#pragma pack(pop)

static_assert(sizeof(PrimPoint) == 6, "PrimPoint: binary file layout");
static_assert(sizeof(RoofFace4) == 10, "RoofFace4: binary file layout");
static_assert(sizeof(PrimFace3) == 28, "PrimFace3: binary file layout");
static_assert(sizeof(PrimFace4) == 34, "PrimFace4: binary file layout");
static_assert(sizeof(PrimFace4PSX) == 24, "PrimFace4PSX: binary file layout");
static_assert(sizeof(PrimFace3PSX) == 20, "PrimFace3PSX: binary file layout");
static_assert(sizeof(PrimObject) == 16, "PrimObject: binary file layout");

// Per-material descriptor for D3D-era high/low quality mesh.
// uc_orig: PrimObjectMaterial (fallen/Headers/building.h)
struct PrimObjectMaterial {
    UWORD wTexturePage;
    UWORD wNumListIndices;
    UWORD wNumStripIndices;
    UWORD wNumVertices;
    UWORD wNumLoListIndices;
    UWORD wNumLoStripIndices;
    UWORD wNumLoVertices;
};

// D3D prim object header with material list and vertex pointers.
// uc_orig: TomsPrimObject (fallen/Headers/building.h)
struct TomsPrimObject {
    UWORD             wFlags;
    UWORD             wNumMaterials;
    UWORD             wTotalSizeOfObj;
    UBYTE             bLRUQueueNumber;
    UBYTE             bPadding;
    PrimObjectMaterial* pMaterials;
    void*             pD3DVertices;
    UWORD*            pwListIndices;
    UWORD*            pwStripIndices;
    float             fBoundingSphereRadius;
};

// Legacy prim object header (includes name field, pre-D3D).
// uc_orig: PrimObjectOld (fallen/Headers/building.h)
struct PrimObjectOld {
    CBYTE ObjectName[32];
    UWORD StartPoint;
    UWORD EndPoint;
    UWORD StartFace4;
    UWORD EndFace4;
    SWORD StartFace3;
    SWORD EndFace3;

    UBYTE coltype;
    UBYTE damage;
    UBYTE shadowtype;
    UBYTE flag;

    UWORD Dummy[4];
};

// Multi-object animation descriptor (tween between frames).
// uc_orig: PrimMultiObject (fallen/Headers/building.h)
struct PrimMultiObject {
    UWORD StartObject;
    UWORD EndObject;
    SWORD Tween;
    SWORD Frame;
};

// =====================================================================
// PrimInfo — per-prim runtime metadata (computed at load time)
// =====================================================================

// uc_orig: PrimInfo (fallen/Headers/Prim.h)
typedef struct {
    SLONG minx, miny, minz; // bounding rectangle
    SLONG maxx, maxy, maxz;
    SLONG cogx, cogy, cogz; // centre of gravity
    SLONG radius;           // bounding sphere radius about origin
} PrimInfo;

#endif // BUILDINGS_PRIM_TYPES_H
