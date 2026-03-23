#ifndef WORLD_MAP_QMAP_H
#define WORLD_MAP_QMAP_H

#include "core/types.h"

// uc_orig: QMAP_SIZE (fallen/Headers/qmap.h)
#define QMAP_SIZE 1024

// ========================================================
// MAP CREATION
// ========================================================

// uc_orig: QMAP_init (fallen/Headers/qmap.h)
void QMAP_init(void);

// uc_orig: QMAP_add_road (fallen/Headers/qmap.h)
void QMAP_add_road(SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: QMAP_add_cube (fallen/Headers/qmap.h)
void QMAP_add_cube(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG height);

// uc_orig: QMAP_add_prim (fallen/Headers/qmap.h)
void QMAP_add_prim(SLONG x, SLONG y, SLONG z, SLONG prim, SLONG yaw);

// ========================================================
// MAP ACCESS
// ========================================================

// uc_orig: QMAP_calc_height_at (fallen/Headers/qmap.h)
SLONG QMAP_calc_height_at(SLONG x, SLONG z);

// uc_orig: QMAP_get_cube_coords (fallen/Headers/qmap.h)
void QMAP_get_cube_coords(UWORD cube, SLONG* x1, SLONG* y1, SLONG* z1,
    SLONG* x2, SLONG* y2, SLONG* z2);

// ========================================================
// MAP DATA STRUCTURES
// ========================================================

// uc_orig: QMAP_MAX_TEXTURES (fallen/Headers/qmap.h)
#define QMAP_MAX_TEXTURES 4096

// uc_orig: QMAP_texture (fallen/Headers/qmap.h)
extern UWORD QMAP_texture[QMAP_MAX_TEXTURES];
// uc_orig: QMAP_texture_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_texture_upto;

// uc_orig: QMAP_STYLE_USED (fallen/Headers/qmap.h)
#define QMAP_STYLE_USED (1 << 0)
// uc_orig: QMAP_STYLE_WRAP_X (fallen/Headers/qmap.h)
#define QMAP_STYLE_WRAP_X (1 << 1)
// uc_orig: QMAP_STYLE_WRAP_Y (fallen/Headers/qmap.h)
#define QMAP_STYLE_WRAP_Y (1 << 2)

// uc_orig: QMAP_Style (fallen/Headers/qmap.h)
typedef struct {
    UBYTE size_x;
    UBYTE size_y;
    UWORD flag;
    UWORD index; // Index into the QMAP_texture array.
} QMAP_Style;

// uc_orig: QMAP_MAX_STYLES (fallen/Headers/qmap.h)
#define QMAP_MAX_STYLES 256

// uc_orig: QMAP_style (fallen/Headers/qmap.h)
extern QMAP_Style QMAP_style[QMAP_MAX_STYLES];
// uc_orig: QMAP_style_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_style_upto;

// uc_orig: QMAP_Road (fallen/Headers/qmap.h)
typedef struct {
    UWORD x;
    UWORD z;
    UBYTE size_x;
    UBYTE size_z;
} QMAP_Road;

// uc_orig: QMAP_MAX_ROADS (fallen/Headers/qmap.h)
#define QMAP_MAX_ROADS 1024

// uc_orig: QMAP_road (fallen/Headers/qmap.h)
extern QMAP_Road QMAP_road[QMAP_MAX_ROADS];
// uc_orig: QMAP_road_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_road_upto;

// uc_orig: QMAP_Cube (fallen/Headers/qmap.h)
typedef struct {
    UWORD x;
    UWORD z;
    UBYTE size_x;
    UBYTE size_y;
    UBYTE size_z;
    UBYTE flag;
    UWORD style[5];
} QMAP_Cube;

// uc_orig: QMAP_MAX_CUBES (fallen/Headers/qmap.h)
#define QMAP_MAX_CUBES 1024

// uc_orig: QMAP_cube (fallen/Headers/qmap.h)
extern QMAP_Cube QMAP_cube[QMAP_MAX_CUBES];
// uc_orig: QMAP_cube_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_cube_upto;

// uc_orig: QMAP_Gtex (fallen/Headers/qmap.h)
typedef struct {
    UWORD x;
    UWORD z;
    UBYTE size_x;
    UBYTE size_z;
    UWORD style;
} QMAP_Gtex;

// uc_orig: QMAP_MAX_GTEXES (fallen/Headers/qmap.h)
#define QMAP_MAX_GTEXES 4096

// uc_orig: QMAP_gtex (fallen/Headers/qmap.h)
extern QMAP_Gtex QMAP_gtex[QMAP_MAX_GTEXES];
// uc_orig: QMAP_gtex_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_gtex_upto;

// uc_orig: QMAP_Cable (fallen/Headers/qmap.h)
typedef struct {
    UWORD x1;
    UWORD z1;
    UWORD x2;
    UWORD z2;
    UBYTE y1;
    UBYTE y2;
    UBYTE flag;
    UBYTE along;
} QMAP_Cable;

// uc_orig: QMAP_MAX_CABLES (fallen/Headers/qmap.h)
#define QMAP_MAX_CABLES 512

// uc_orig: QMAP_cable (fallen/Headers/qmap.h)
extern QMAP_Cable QMAP_cable[QMAP_MAX_CABLES];
// uc_orig: QMAP_cable_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_cable_upto;

// uc_orig: QMAP_MAX_HEIGHTS (fallen/Headers/qmap.h)
#define QMAP_MAX_HEIGHTS 8192

// uc_orig: QMAP_height (fallen/Headers/qmap.h)
extern SBYTE QMAP_height[QMAP_MAX_HEIGHTS];
// uc_orig: QMAP_height_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_height_upto;

// uc_orig: QMAP_Hmap (fallen/Headers/qmap.h)
typedef struct {
    UWORD x;
    UWORD z;
    UBYTE size_x;
    UBYTE size_z;
    // Top bit set: constant height (value = height - 0xa000).
    // Top bit clear: index into QMAP_height array.
    UWORD height;
} QMAP_Hmap;

// uc_orig: QMAP_MAX_HMAPS (fallen/Headers/qmap.h)
#define QMAP_MAX_HMAPS 64

// uc_orig: QMAP_hmap (fallen/Headers/qmap.h)
extern QMAP_Hmap QMAP_hmap[QMAP_MAX_HMAPS];
// uc_orig: QMAP_hmap_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_hmap_upto;

// uc_orig: QMAP_FENCE_NORMAL (fallen/Headers/qmap.h)
#define QMAP_FENCE_NORMAL 1
// uc_orig: QMAP_FENCE_BARBED (fallen/Headers/qmap.h)
#define QMAP_FENCE_BARBED 2
// uc_orig: QMAP_FENCE_WALL (fallen/Headers/qmap.h)
#define QMAP_FENCE_WALL 3

// uc_orig: QMAP_Fence (fallen/Headers/qmap.h)
typedef struct {
    UBYTE type;
    UBYTE y; // Height above ground.
    UWORD x1;
    UWORD z1;
    UBYTE size_x;
    UBYTE size_z;
} QMAP_Fence;

// uc_orig: QMAP_MAX_FENCES (fallen/Headers/qmap.h)
#define QMAP_MAX_FENCES 1024

// uc_orig: QMAP_fence (fallen/Headers/qmap.h)
extern QMAP_Fence QMAP_fence[QMAP_MAX_FENCES];
// uc_orig: QMAP_fence_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_fence_upto;

// uc_orig: QMAP_Light (fallen/Headers/qmap.h)
typedef struct {
    UBYTE x;
    UBYTE y;
    UBYTE z;
    SBYTE red;
    SBYTE green;
    SBYTE blue;
    UBYTE range;
} QMAP_Light;

// uc_orig: QMAP_MAX_LIGHTS (fallen/Headers/qmap.h)
#define QMAP_MAX_LIGHTS 2048

// uc_orig: QMAP_light (fallen/Headers/qmap.h)
extern QMAP_Light QMAP_light[QMAP_MAX_LIGHTS];
// uc_orig: QMAP_light_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_light_upto;

// uc_orig: QMAP_Prim (fallen/Headers/qmap.h)
typedef struct {
    UBYTE x;
    UBYTE y;
    UBYTE z;
    UBYTE yaw;
    UBYTE prim;
} QMAP_Prim;

// uc_orig: QMAP_MAX_PRIMS (fallen/Headers/qmap.h)
#define QMAP_MAX_PRIMS 16384

// uc_orig: QMAP_prim (fallen/Headers/qmap.h)
extern QMAP_Prim QMAP_prim[QMAP_MAX_PRIMS];
// uc_orig: QMAP_prim_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_prim_upto;

// Number of high-level map squares (map is QMAP_MAPSIZE x QMAP_MAPSIZE).
// uc_orig: QMAP_MAPSIZE (fallen/Headers/qmap.h)
#define QMAP_MAPSIZE (QMAP_SIZE / 32)

// uc_orig: QMAP_MAX_ALL (fallen/Headers/qmap.h)
#define QMAP_MAX_ALL 32768

// uc_orig: QMAP_all (fallen/Headers/qmap.h)
extern UWORD QMAP_all[QMAP_MAX_ALL];
// uc_orig: QMAP_all_upto (fallen/Headers/qmap.h)
extern SLONG QMAP_all_upto;

// uc_orig: QMAP_Map (fallen/Headers/qmap.h)
typedef struct {
    UWORD index_all;
    UWORD index_prim;
    UBYTE num_roads;
    UBYTE num_cubes;
    UBYTE num_gtexes;
    UBYTE num_cables;
    UBYTE num_hmaps;
    UBYTE num_fences;
    UBYTE num_lights;
    UBYTE num_prims;
} QMAP_Map;

// uc_orig: QMAP_map (fallen/Headers/qmap.h)
extern QMAP_Map QMAP_map[QMAP_MAPSIZE][QMAP_MAPSIZE];

// Accessors for per-type indices within a map square's 'all' array.
// uc_orig: QMAP_ALL_INDEX_ROADS (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_ROADS(qm) ((qm)->index_all)
// uc_orig: QMAP_ALL_INDEX_CUBES (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_CUBES(qm) ((qm)->index_all + (qm)->num_roads)
// uc_orig: QMAP_ALL_INDEX_GTEXES (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_GTEXES(qm) ((qm)->index_all + (qm)->num_roads + (qm)->num_cubes)
// uc_orig: QMAP_ALL_INDEX_CABLES (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_CABLES(qm) ((qm)->index_all + (qm)->num_roads + (qm)->num_cubes + (qm)->num_gtexes)
// uc_orig: QMAP_ALL_INDEX_HMAPS (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_HMAPS(qm)  ((qm)->index_all + (qm)->num_roads + (qm)->num_cubes + (qm)->num_gtexes) + (qm)->num_cables)
// uc_orig: QMAP_ALL_INDEX_FENCES (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_FENCES(qm) ((qm)->index_all + (qm)->num_roads + (qm)->num_cubes + (qm)->num_gtexes) + (qm)->num_cables + (qm)->num_hmaps)
// uc_orig: QMAP_ALL_INDEX_LIGHTS (fallen/Headers/qmap.h)
#define QMAP_ALL_INDEX_LIGHTS(qm) ((qm)->index_all + (qm)->num_roads + (qm)->num_cubes + (qm)->num_gtexes) + (qm)->num_cables + (qm)->num_hmaps + (qm)->num_fences)

// uc_orig: QMAP_TOTAL_ALL (fallen/Headers/qmap.h)
#define QMAP_TOTAL_ALL(qm) ((qm)->num_roads + (qm)->num_cubes + (qm)->num_gtexes + (qm)->num_cables + (qm)->num_hmaps + (qm)->num_fences + (qm)->num_lights)

// ========================================================
// DRAW DATA STRUCTURE
// ========================================================

// Normals for QMAP_Point vertices.
// uc_orig: QMAP_POINT_NORMAL_XPOS (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_XPOS 0
// uc_orig: QMAP_POINT_NORMAL_XNEG (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_XNEG 1
// uc_orig: QMAP_POINT_NORMAL_YPOS (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_YPOS 2
// uc_orig: QMAP_POINT_NORMAL_YNEG (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_YNEG 3
// uc_orig: QMAP_POINT_NORMAL_ZPOS (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_ZPOS 4
// uc_orig: QMAP_POINT_NORMAL_ZNEG (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_ZNEG 5
// uc_orig: QMAP_POINT_NORMAL_NUMBER (fallen/Headers/qmap.h)
#define QMAP_POINT_NORMAL_NUMBER 6

// uc_orig: QMAP_Point (fallen/Headers/qmap.h)
typedef struct {
    UWORD x; // Relative to the mapsquare, 8-bit fixed point.
    SWORD y;
    UWORD z;
    UBYTE red;
    UBYTE green;
    UBYTE blue;
    UBYTE trans; // Gameturn on which this point was last transformed.
    UBYTE normal;
    UBYTE padding;
    UWORD next;
} QMAP_Point;

// uc_orig: QMAP_MAX_POINTS (fallen/Headers/qmap.h)
#define QMAP_MAX_POINTS 2048

// uc_orig: QMAP_point (fallen/Headers/qmap.h)
extern QMAP_Point QMAP_point[QMAP_MAX_POINTS];
// uc_orig: QMAP_point_free (fallen/Headers/qmap.h)
extern UWORD QMAP_point_free;

// uc_orig: QMAP_FACE_FLAG_SHADOW1 (fallen/Headers/qmap.h)
#define QMAP_FACE_FLAG_SHADOW1 (1 << 0)
// uc_orig: QMAP_FACE_FLAG_SHADOW2 (fallen/Headers/qmap.h)
#define QMAP_FACE_FLAG_SHADOW2 (1 << 1)

// uc_orig: QMAP_Face (fallen/Headers/qmap.h)
typedef struct {
    UWORD point[4];
    UWORD texture;
    UWORD flag;
    UWORD next;
} QMAP_Face;

// uc_orig: QMAP_MAX_FACES (fallen/Headers/qmap.h)
#define QMAP_MAX_FACES 2048

// uc_orig: QMAP_face (fallen/Headers/qmap.h)
extern QMAP_Face QMAP_face[QMAP_MAX_FACES];
// uc_orig: QMAP_face_free (fallen/Headers/qmap.h)
extern UWORD QMAP_face_free;

// uc_orig: QMAP_Hsquare (fallen/Headers/qmap.h)
typedef struct {
    UWORD texture;
    UWORD flag;
    SBYTE height;
    UBYTE red;
    UBYTE green;
    UBYTE blue;
} QMAP_Hsquare;

// Per-mapsquare render data: heightfield + polygon lists built each frame.
// uc_orig: QMAP_Draw (fallen/Headers/qmap.h)
typedef struct {
    QMAP_Hsquare hf[33][33]; // 33x33 heightfield (one extra row/col for seams).
    UBYTE map_x;
    UBYTE map_z;
    UBYTE trans;
    UWORD next_point; // Head of the point linked list for this square.
    UWORD next_face;  // Head of the face linked list for this square.
} QMAP_Draw;

// uc_orig: QMAP_draw_init (fallen/Headers/qmap.h)
void QMAP_draw_init(void);

// uc_orig: QMAP_create (fallen/Headers/qmap.h)
void QMAP_create(QMAP_Draw* fill_me_in, SLONG map_x, SLONG map_z);

// uc_orig: QMAP_free (fallen/Headers/qmap.h)
void QMAP_free(QMAP_Draw* free_me_up);

#endif // WORLD_MAP_QMAP_H
