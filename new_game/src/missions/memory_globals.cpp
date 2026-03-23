// uc_orig: memory.cpp (fallen/Source/memory.cpp)
// Global variable definitions for the game-level memory allocation system.

#include "fallen/Headers/Game.h"      // Temporary: Game struct, Thing system types, MemTable, save_table externs
#include "world/map/ob.h"
#include "world/map/ob_globals.h"
#include "world/navigation/wmove.h"
#include "world/navigation/wmove_globals.h"
#include "world/map/supermap.h"
#include "engine/lighting/night.h"
#include "engine/lighting/night_globals.h"
#include "actors/items/barrel.h"
#include "actors/items/barrel_globals.h"
#include "fallen/Headers/eway.h"      // Temporary: EWAY_mess, EWAY_way, EWAY_cond, EWAY_edef, EWAY_timer
#include "fallen/Headers/pap.h"       // Temporary: PAP_hi, PAP_lo, PAP_SIZE_HI, PAP_SIZE_LO
#include "fallen/Headers/mav.h"       // Temporary: MAV_opt, MAV_nav, MAV_MAX_OPTS
#include "world/map/road.h"
#include "world/map/road_globals.h"
#include "actors/items/balloon.h"
#include "actors/items/balloon_globals.h"
#include "effects/tracks.h"
#include "world/environment/ware.h"
#include "world/environment/ware_globals.h"
#include "world/environment/tripwire.h"
#include "world/environment/tripwire_globals.h"
#include "engine/effects/psystem.h"
#include "engine/effects/psystem_globals.h"
#include "actors/animals/bat.h"
#include "world/environment/door.h"
#include "world/environment/door_globals.h"
#include "ui/cutscenes/playcuts.h"
#include "ui/cutscenes/playcuts_globals.h"
#include "engine/audio/sound.h"

#include "missions/memory_globals.h"
#include "missions/memory.h"

// EWAY_counter is defined in eway.cpp (not yet migrated) and not in eway.h.
extern UBYTE* EWAY_counter;

// String name macro used in save_table[] entries — identity function, present for PSX compat.
// uc_orig: M_ (fallen/Source/memory.cpp)
#define M_(x) x

// Central allocation table: maps a string name and pointer-to-pointer to a pool slot.
// init_memory() walks this table to carve up a single flat allocation.
// save/load functions use it to serialize and restore all game arrays.
// uc_orig: save_table (fallen/Source/memory.cpp)
struct MemTable save_table[] = {

    { M_("Pap_Hi"), (void**)&PAP_hi, MEM_STATIC, 0, 0, PAP_SIZE_HI * PAP_SIZE_HI, sizeof(PAP_Hi), 0 }, // 0
    { M_("Pap_Lo"), (void**)&PAP_lo, MEM_STATIC, 0, 0, PAP_SIZE_LO * PAP_SIZE_LO, sizeof(PAP_Lo), 0 }, // 1
    { M_("net_peep"), (void**)&NETPERSON, MEM_STATIC, 0, 0, 10, sizeof(Thing*), 0 }, // 2
    { M_("net_plyr"), (void**)&NETPLAYERS, MEM_STATIC, 0, 0, 10, sizeof(Thing*), 0 }, // 3
    { M_("f-links"), (void**)&facet_links, MEM_DYNAMIC, 0, (UWORD*)&next_facet_link, MAX_FACET_LINK, sizeof(SWORD), 0 }, // 4
    { M_("dbuildings"), (void**)&dbuildings, MEM_DYNAMIC, &next_dbuilding, 0, MAX_DBUILDINGS, sizeof(struct DBuilding), 0 }, // 5
    { M_("dfacets"), (void**)&dfacets, MEM_DYNAMIC, &next_dfacet, 0, MAX_DFACETS, sizeof(struct DFacet), 0 }, // 6
    { M_("dwalkables"), (void**)&dwalkables, MEM_DYNAMIC, &next_dwalkable, 0, MAX_DWALKABLES, sizeof(struct DWalkable), 0 }, // 7
    { M_("dstyles"), (void**)&dstyles, MEM_DYNAMIC, &next_dstyle, 0, MAX_DSTYLES, sizeof(SWORD), 0 }, // 8
    { M_("dstoreys"), (void**)&dstoreys, MEM_DYNAMIC, 0, (UWORD*)&next_dstorey, MAX_DSTOREYS, sizeof(struct DStorey), 0 }, // 9
    { M_("paintmem"), (void**)&paint_mem, MEM_DYNAMIC, 0, (UWORD*)&next_paint_mem, MAX_PAINTMEM, sizeof(UBYTE), 0 }, // 10
    { M_("insideStoreys"), (void**)&inside_storeys, MEM_DYNAMIC, 0, (UWORD*)&next_inside_storey, MAX_INSIDE_RECT, sizeof(struct InsideStorey), 0 }, // 11
    { M_("insideStairs"), (void**)&inside_stairs, MEM_DYNAMIC, 0, &next_inside_stair, MAX_INSIDE_STAIRS, sizeof(struct Staircase), 0 }, // 12
    { M_("insideblock"), (void**)&inside_block, MEM_DYNAMIC, &next_inside_block, 0, MAX_INSIDE_MEM, sizeof(UBYTE), 0 }, // 13
    { M_("roof bounds"), (void**)&roof_bounds, MEM_DYNAMIC, 0, &next_roof_bound, MAX_ROOF_BOUND, sizeof(struct BoundBox), 0 }, // 14
    { M_("prim_points"), (void**)&prim_points, MEM_DYNAMIC, 0, &next_prim_point, RMAX_PRIM_POINTS, sizeof(struct PrimPoint), 256 }, // 15
    { M_("prim_faces4"), (void**)&prim_faces4, MEM_DYNAMIC, 0, &next_prim_face4, RMAX_PRIM_FACES4, sizeof(struct PrimFace4), 64 }, // 16
    { M_("prim_faces3"), (void**)&prim_faces3, MEM_DYNAMIC, 0, &next_prim_face3, MAX_PRIM_FACES3, sizeof(struct PrimFace3), 0 }, // 17
    { M_("prim_objects"), (void**)&prim_objects, MEM_DYNAMIC, 0, &next_prim_object, MAX_PRIM_OBJECTS, sizeof(struct PrimObject), 0 }, // 18
    { M_("prim_Mobjects"), (void**)&prim_multi_objects, MEM_DYNAMIC, 0, &next_prim_multi_object, MAX_PRIM_MOBJECTS, sizeof(struct PrimMultiObject), 0 }, // 19
    { M_("ob_ob"), (void**)&OB_ob, MEM_DYNAMIC, &OB_ob_upto, 0, OB_MAX_OBS, sizeof(OB_Ob), 0 }, // 20
    { M_("ob_ mapwho"), (void**)&OB_mapwho, MEM_STATIC, 0, 0, OB_SIZE * OB_SIZE, sizeof(OB_Mapwho), 0 }, // 21
    { M_("EWAY_mess"), (void**)&EWAY_mess, MEM_DYNAMIC, &EWAY_mess_upto, 0, EWAY_MAX_MESSES, sizeof(CBYTE*), 0 }, // 22
    { M_("EWAY_mess buf"), (void**)&EWAY_mess_buffer, MEM_DYNAMIC, &EWAY_mess_buffer_upto, 0, EWAY_MESS_BUFFER_SIZE, sizeof(CBYTE), 0 }, // 23
    { M_("EWAY_timer"), (void**)&EWAY_timer, MEM_STATIC, 0, 0, EWAY_MAX_TIMERS, sizeof(UWORD), 0 }, // 24
    { M_("EWAY_cond"), (void**)&EWAY_cond, MEM_DYNAMIC, &EWAY_cond_upto, 0, EWAY_MAX_CONDS, sizeof(EWAY_Cond), 0 }, // 25
    { M_("EWAY_way"), (void**)&EWAY_way, MEM_DYNAMIC, &EWAY_way_upto, 0, EWAY_MAX_WAYS, sizeof(EWAY_Way), 0 }, // 26
    { M_("EWAY_edef"), (void**)&EWAY_edef, MEM_DYNAMIC, &EWAY_edef_upto, 0, EWAY_MAX_EDEFS, sizeof(EWAY_Edef), 0 }, // 27
    { M_("EWAY_counter"), (void**)&EWAY_counter, MEM_STATIC, 0, 0, EWAY_MAX_COUNTERS, sizeof(UBYTE), 0 }, // 28
    { M_("vehicles"), (void**)&VEHICLES, MEM_STATIC, 0, 0, RMAX_VEHICLES, sizeof(Vehicle), 32 }, // 29
    { M_("people"), (void**)&PEOPLE, MEM_STATIC, 0, 0, RMAX_PEOPLE, sizeof(Person), 128 }, // 30
    { M_("animals"), (void**)&ANIMALS, MEM_STATIC, 0, 0, MAX_ANIMALS, sizeof(Animal), 0 }, // 31
    { M_("choppers"), (void**)&CHOPPERS, MEM_STATIC, 0, 0, MAX_CHOPPERS, sizeof(Chopper), 0 }, // 32
    { M_("pyro"), (void**)&PYROS, MEM_STATIC, 0, 0, MAX_PYROS, sizeof(Pyro), 0 }, // 33
    { M_("players"), (void**)&PLAYERS, MEM_STATIC, 0, 0, MAX_PLAYERS, sizeof(Player), 0 }, // 34
    { M_("projectiles"), (void**)&PROJECTILES, MEM_STATIC, 0, 0, MAX_PROJECTILES, sizeof(Projectile), 0 }, // 35
    { M_("special"), (void**)&SPECIALS, MEM_STATIC, 0, 0, RMAX_SPECIALS, sizeof(Special), 128 }, // 36
    { M_("switches"), (void**)&SWITCHES, MEM_STATIC, 0, 0, MAX_SWITCHES, sizeof(Switch), 0 }, // 37
    { M_("bats"), (void**)&BATS, MEM_STATIC, 0, 0, RBAT_MAX_BATS, sizeof(Bat), 32 }, // 38
    { M_("thing"), (void**)&THINGS, MEM_STATIC, 0, 0, MAX_THINGS, sizeof(Thing), 0 }, // 39
    { M_("drawtween"), (void**)&DRAW_TWEENS, MEM_STATIC, 0, 0, RMAX_DRAW_TWEENS, sizeof(DrawTween), 128 }, // 40
    { M_("drawmesh"), (void**)&DRAW_MESHES, MEM_STATIC, 0, 0, RMAX_DRAW_MESHES, sizeof(DrawMesh), 128 }, // 41
    { M_("barrelsphere"), (void**)&BARREL_sphere, MEM_STATIC, 0, 0, BARREL_MAX_SPHERES, sizeof(BARREL_Sphere), 0 }, // 43
    { M_("barrels"), (void**)&BARREL_barrel, MEM_DYNAMIC, &BARREL_barrel_upto, 0, BARREL_MAX_BARRELS, sizeof(Barrel), 0 }, // 44
    { M_("plat"), (void**)&PLAT_plat, MEM_DYNAMIC, &PLAT_plat_upto, 0, RPLAT_MAX_PLATS, sizeof(Plat), 2 }, // 45
    { M_("wmove"), (void**)&WMOVE_face, MEM_DYNAMIC, &WMOVE_face_upto, 0, RWMOVE_MAX_FACES, sizeof(WMOVE_Face), 64 }, // 46
    { M_("mav_opt"), (void**)&MAV_opt, MEM_DYNAMIC, &MAV_opt_upto, 0, MAV_MAX_OPTS, sizeof(MAV_Opt), 0 }, // 47
    { M_("mav_nav"), (void**)&MAV_nav, MEM_STATIC, 0, 0, PAP_SIZE_HI * PAP_SIZE_HI, sizeof(UWORD), 0 }, // 48
    { M_("road_noads"), (void**)&ROAD_node, MEM_DYNAMIC, &ROAD_node_upto, 0, ROAD_MAX_NODES, sizeof(ROAD_Node), 0 }, // 49
    { M_("balloons"), (void**)&BALLOON_balloon, MEM_DYNAMIC, &BALLOON_balloon_upto, 0, BALLOON_MAX_BALLOONS, sizeof(BALLOON_Balloon), 0 }, // 50
    { M_("tracks"), (void**)&tracks, MEM_STATIC, 0, 0, TRACK_BUFFER_LENGTH, sizeof(Track), 0 }, // 51
    { M_("roofface4"), (void**)&roof_faces4, MEM_DYNAMIC, 0, &next_roof_face4, MAX_ROOF_FACE4, sizeof(struct RoofFace4), 0 }, // 52
    { M_("fastnav"), (void**)&COLLIDE_fastnav, MEM_STATIC, 0, 0, PAP_SIZE_HI * PAP_SIZE_HI >> 3, sizeof(UBYTE), 0 }, // 53
    { M_("night_slight"), (void**)&NIGHT_slight, MEM_DYNAMIC, &NIGHT_slight_upto, 0, NIGHT_MAX_SLIGHTS, sizeof(NIGHT_Slight), 0 }, // 54
    { M_("night_smap"), (void**)&NIGHT_smap, MEM_STATIC, 0, 0, PAP_SIZE_LO * PAP_SIZE_LO, sizeof(NIGHT_Smap), 0 }, // 55
    { M_("night_dlight"), (void**)&NIGHT_dlight, MEM_STATIC, 0, 0, NIGHT_MAX_DLIGHTS, sizeof(NIGHT_Dlight), 0 }, // 56
    { M_("WARE_ware"), (void**)&WARE_ware, MEM_DYNAMIC, 0, &WARE_ware_upto, WARE_MAX_WARES, sizeof(WARE_Ware), 0 }, // 57
    { M_("WARE_nav"), (void**)&WARE_nav, MEM_DYNAMIC, 0, &WARE_nav_upto, WARE_MAX_NAVS, sizeof(UWORD), 0 }, // 58
    { M_("WARE_height"), (void**)&WARE_height, MEM_DYNAMIC, 0, &WARE_height_upto, WARE_MAX_HEIGHTS, sizeof(SBYTE), 0 }, // 59
    { M_("WARE_rooftex"), (void**)&WARE_rooftex, MEM_DYNAMIC, 0, &WARE_rooftex_upto, WARE_MAX_ROOFTEXES, sizeof(UWORD), 0 }, // 60
    { M_("Trip_Wire"), (void**)&TRIP_wire, MEM_DYNAMIC, &TRIP_wire_upto, 0, TRIP_MAX_WIRES, sizeof(TRIP_Wire), 0 }, // 61
    { M_("Road_edges"), (void**)&ROAD_edge, MEM_DYNAMIC, 0, &ROAD_edge_upto, ROAD_MAX_EDGES, sizeof(UBYTE), 0 }, // 62
    { M_("Thing_heads"), (void**)&thing_class_head, MEM_STATIC, 0, 0, CLASS_END, sizeof(UWORD), 0 }, // 63
    { M_("psx_remap"), (void**)&psx_remap, MEM_STATIC, 0, 0, 128, sizeof(UWORD), 0 }, // 64
    { M_("psx_tex_xy"), (void**)&psx_textures_xy, MEM_STATIC, 0, 0, 200 * 5, sizeof(UWORD), 0 }, // 65
    { M_("map_beacon"), (void**)&MAP_beacon, MEM_STATIC, 0, 0, MAP_MAX_BEACONS, sizeof(MAP_Beacon), 0 }, // 66
    { M_("cutscene_data"), (void**)&PLAYCUTS_cutscenes, MEM_DYNAMIC, 0, &PLAYCUTS_cutscene_ctr, MAX_CUTSCENES, sizeof(CPData), 0 },
    { M_("cutscene_trks"), (void**)&PLAYCUTS_tracks, MEM_DYNAMIC, 0, &PLAYCUTS_track_ctr, MAX_CUTSCENE_TRACKS, sizeof(CPChannel), 0 },
    { M_("cutscene_pkts"), (void**)&PLAYCUTS_packets, MEM_DYNAMIC, 0, &PLAYCUTS_packet_ctr, MAX_CUTSCENE_PACKETS, sizeof(CPPacket), 0 },
    { M_("cutscene_text"), (void**)&PLAYCUTS_text_data, MEM_DYNAMIC, 0, &PLAYCUTS_text_ctr, MAX_CUTSCENE_TEXT, sizeof(CBYTE), 0 },
    { M_("darci normal"), (void**)&darci_normal, MEM_DYNAMIC, 0, &darci_normal_count, 12000, sizeof(UWORD), 0 },
    { M_("prim info"), (void**)&prim_info, MEM_STATIC, 0, 0, 256, sizeof(PrimInfo), 0 },
    { M_("Doors-gates"), (void**)&DOOR_door, MEM_STATIC, 0, 0, DOOR_MAX_DOORS, sizeof(DOOR_Door), 0 },
    { M_("soundfxmap"), (void**)&SOUND_FXMapping, MEM_STATIC, 0, 0, 1024, sizeof(UBYTE), 0 },
    { M_("soundfxgroup"), (void**)&SOUND_FXGroups, MEM_STATIC, 0, 0, 128 * 2, sizeof(UWORD), 0 },

    { 0, 0, 0, 0, 0, 0, 0 }
};

// uc_orig: MAP_beacon (fallen/Source/memory.cpp)
MAP_Beacon* MAP_beacon;

// uc_orig: psx_textures_xy (fallen/Source/memory.cpp)
PSX_TEX* psx_textures_xy;

// uc_orig: mem_all (fallen/Source/memory.cpp)
void* mem_all = 0;
// uc_orig: mem_all_size (fallen/Source/memory.cpp)
ULONG mem_all_size = 0;

// uc_orig: psx_remap (fallen/Source/memory.cpp)
UWORD* psx_remap;

// uc_orig: facet_links (fallen/Source/memory.cpp)
SWORD* facet_links;

// uc_orig: dbuildings (fallen/Source/memory.cpp)
struct DBuilding* dbuildings;
// uc_orig: dfacets (fallen/Source/memory.cpp)
struct DFacet* dfacets;
// uc_orig: dwalkables (fallen/Source/memory.cpp)
struct DWalkable* dwalkables;
// uc_orig: dstyles (fallen/Source/memory.cpp)
SWORD* dstyles;
// uc_orig: dstoreys (fallen/Source/memory.cpp)
struct DStorey* dstoreys;

// uc_orig: paint_mem (fallen/Source/memory.cpp)
UBYTE* paint_mem;

// uc_orig: anim_mids (fallen/Source/memory.cpp)
struct PrimPoint* anim_mids;
// uc_orig: next_anim_mids (fallen/Source/memory.cpp)
ULONG next_anim_mids = 0;

// uc_orig: inside_storeys (fallen/Source/memory.cpp)
struct InsideStorey* inside_storeys;
// uc_orig: inside_stairs (fallen/Source/memory.cpp)
struct Staircase* inside_stairs;
// uc_orig: inside_block (fallen/Source/memory.cpp)
UBYTE* inside_block;
// uc_orig: inside_tex (fallen/Source/memory.cpp)
UBYTE inside_tex[64][16];

// uc_orig: roof_bounds (fallen/Source/memory.cpp)
struct BoundBox* roof_bounds;

// uc_orig: prim_points (fallen/Source/memory.cpp)
struct PrimPoint* prim_points;
// uc_orig: prim_faces4 (fallen/Source/memory.cpp)
struct PrimFace4* prim_faces4;
// uc_orig: prim_faces3 (fallen/Source/memory.cpp)
struct PrimFace3* prim_faces3;
// uc_orig: prim_objects (fallen/Source/memory.cpp)
struct PrimObject* prim_objects;
// uc_orig: prim_multi_objects (fallen/Source/memory.cpp)
struct PrimMultiObject* prim_multi_objects;

// uc_orig: prim_normal (fallen/Source/memory.cpp)
PrimNormal* prim_normal;

// prim_info definition lives in world/environment/prim_globals.cpp (migrated from Prim.cpp).
// extern PrimInfo* prim_info is declared in both memory_globals.h and prim_globals.h —
// both refer to the same variable defined in prim_globals.cpp.

// uc_orig: next_roof_face4 (fallen/Source/memory.cpp)
UWORD next_roof_face4 = 1;
// uc_orig: roof_faces4 (fallen/Source/memory.cpp)
struct RoofFace4* roof_faces4;

// uc_orig: darci_normal (fallen/Source/memory.cpp)
UWORD* darci_normal;
// uc_orig: darci_normal_count (fallen/Source/memory.cpp)
UWORD darci_normal_count = 0;

// uc_orig: MEMORY_quick_avaliable (fallen/Source/memory.cpp)
SLONG MEMORY_quick_avaliable;
