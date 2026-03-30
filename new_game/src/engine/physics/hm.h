#ifndef ENGINE_PHYSICS_HM_H
#define ENGINE_PHYSICS_HM_H

// uc_orig: hm.h (fallen/Headers/hm.h)
// Hypermatter physics system — spring-lattice soft-body simulation for barrels,
// boxes, and furniture that can be knocked over or blown up.
// Each object is represented as a grid of mass-points connected by springs.
// At most HM_MAX_OBJECTS (8) can exist simultaneously.

#include "engine/core/types.h"

// uc_orig: HM_MAX_RES (fallen/Headers/hm.h)
#define HM_MAX_RES 8

// uc_orig: HM_Header (fallen/Headers/hm.h)
// Binary file header for the .hm primgrid data file.
#pragma pack(push, 1)
typedef struct
{
    SLONG version;
    SLONG num_primgrids;

} HM_Header;

// uc_orig: HM_Primgrid (fallen/Headers/hm.h)
// Per-prim spring lattice configuration, loaded from the .hm editor file.
// Defines how many lattice divisions a prim gets and where the division planes
// fall within the bounding box (x/y/z_point as 0x0000..0x10000 fractions).
// x/y/z_dgrav: linear mass gradient — positive y_dgrav makes the top heavier.
typedef struct
{
    UBYTE prim;
    UBYTE x_res;
    UBYTE y_res;
    UBYTE z_res;

    SLONG x_point[HM_MAX_RES];
    SLONG y_point[HM_MAX_RES];
    SLONG z_point[HM_MAX_RES];

    float x_dgrav;
    float y_dgrav;
    float z_dgrav;

} HM_Primgrid;
#pragma pack(pop)

static_assert(sizeof(HM_Header) == 8, "HM_Header: binary file layout");
static_assert(sizeof(HM_Primgrid) == 112, "HM_Primgrid: binary file layout");

// Internal simulation types exposed here so hm_globals.h can define the pool.

// uc_orig: HM_Point (fallen/Source/hm.cpp)
// A single mass-point in the spring lattice.
// (x,y,z) = world-space position; (dx,dy,dz) = current velocity.
// mass = gravitational scale factor (non-uniform mass creates rotational inertia).
typedef struct
{
    float x, y, z;
    float dx, dy, dz;
    float mass;

} HM_Point;

// uc_orig: HM_Edge (fallen/Source/hm.cpp)
// A spring connecting two mass-points.
// p1, p2 = indices into HM_Object.point[].
// len = index into ho->size[] (squared rest-length of this spring).
typedef struct
{
    UWORD p1;
    UWORD p2;
    UBYTE len;
    UBYTE shit;

} HM_Edge;

// uc_orig: HM_Index (fallen/Source/hm.cpp)
// Sparse 3D grid entry: 0 = no point exists, non-zero = point array index.
typedef UWORD HM_Index;

// uc_orig: HM_Mesh (fallen/Source/hm.cpp)
// Maps one original mesh vertex back to the spring lattice for rendering.
// Barycentric encoding: pos = origin + along[i]*(p[i] - origin) for i=0..2.
typedef struct
{
    UWORD origin;
    UWORD p[3];
    float along[3];

} HM_Mesh;

// uc_orig: HM_NUM_OPP_POINTS (fallen/Source/hm.cpp)
#define HM_NUM_OPP_POINTS 8

// uc_orig: HM_Bump (fallen/Source/hm.cpp)
// Records an active HM-vs-HM penetration event for one point of object A
// that has entered a cube of object B.
typedef struct hm_bump {
    UWORD point;
    UWORD shit;

    HM_Index hm_index;
    UBYTE cube_x;
    UBYTE cube_y;
    UBYTE cube_z;

    float rel_x;
    float rel_y;
    float rel_z;

    UWORD opp_point[HM_NUM_OPP_POINTS];
    float opp_prop[HM_NUM_OPP_POINTS];

    struct hm_bump* next;
} HM_Bump;

// uc_orig: HM_Col (fallen/Source/hm.cpp)
// A 2D XZ wall segment that an HM object collides against (infinite height).
typedef struct
{
    ULONG consider;     // Bitmask: bit i set = test point[i] against this wall.
    float x1, z1;
    float x2, z2;
    float len;

} HM_Col;

// uc_orig: HM_MAX_SIZES (fallen/Source/hm.cpp)
#define HM_MAX_SIZES 256

// uc_orig: HM_COLS_PER_OBJECT (fallen/Source/hm.cpp)
#define HM_COLS_PER_OBJECT 64

// uc_orig: HM_MAX_OBJECTS (fallen/Source/hm.cpp)
#define HM_MAX_OBJECTS 8

// uc_orig: HM_MAX_PRIMGRIDS (fallen/Source/hm.cpp)
#define HM_MAX_PRIMGRIDS 64

// uc_orig: HM_Object (fallen/Source/hm.cpp)
// Per-slot physics simulation state for one Hypermatter object.
// elasticity: spring stiffness (Hooke's k). bounciness: floor restitution.
// friction: XZ velocity retention on ground bounce. damping: per-tick velocity decay.
typedef struct
{
    UBYTE used;
    UBYTE shit;
    UWORD prim;
    SLONG x_res, y_res, z_res;
    SLONG x_res_times_y_res;
    SLONG num_indices;
    SLONG num_points;
    SLONG num_edges;
    SLONG num_sizes;
    SLONG num_meshes;
    SLONG num_cols;

    HM_Index* index;
    HM_Point* point;
    HM_Edge* edge;
    HM_Mesh* mesh;
    HM_Col col[HM_COLS_PER_OBJECT];
    HM_Bump* bump;

    // Reference points for HM_find_mesh_pos() — 4 lattice points near the COG
    // used to reconstruct world position and orientation each frame.
    SLONG x_index, y_index, z_index, o_index;
    float o_prim_x, o_prim_y, o_prim_z;
    float cog_x, cog_y, cog_z;

    float size[HM_MAX_SIZES];
    float oversize[HM_MAX_SIZES];

    float elasticity;
    float bounciness;
    float friction;
    float damping;

} HM_Object;

// uc_orig: HM_NO_MORE_OBJECTS (fallen/Headers/hm.h)
#define HM_NO_MORE_OBJECTS 255

// --- Public API ---

// uc_orig: HM_init (fallen/Headers/hm.h)
void HM_init(void);

// uc_orig: HM_load (fallen/Headers/hm.h)
void HM_load(CBYTE* fname);

// uc_orig: HM_get_primgrid (fallen/Headers/hm.h)
HM_Primgrid* HM_get_primgrid(SLONG prim);

// uc_orig: HM_create (fallen/Headers/hm.h)
UBYTE HM_create(
    SLONG prim,
    SLONG x, SLONG y, SLONG z,
    SLONG yaw, SLONG pitch, SLONG roll,
    SLONG dx, SLONG dy, SLONG dz,
    SLONG x_res, SLONG y_res, SLONG z_res,
    SLONG x_point[], SLONG y_point[], SLONG z_point[],
    float x_dgrav, float y_dgrav, float z_dgrav,
    float elasticity, float bounciness, float friction, float damping);

// uc_orig: HM_destroy (fallen/Headers/hm.h)
void HM_destroy(UBYTE hm_index);

// uc_orig: HM_shockwave (fallen/Headers/hm.h)
void HM_shockwave(UBYTE hm_index, float x, float y, float z, float range, float force);

// uc_orig: HM_find_cog (fallen/Headers/hm.h)
void HM_find_cog(UBYTE hm_index, float* x, float* y, float* z);

// uc_orig: HM_collide_all (fallen/Headers/hm.h)
void HM_collide_all(void);

// uc_orig: HM_collide (fallen/Headers/hm.h)
void HM_collide(UBYTE hm_index1, UBYTE hm_index2);

// uc_orig: HM_colvect_clear (fallen/Headers/hm.h)
void HM_colvect_clear(UBYTE hm_index);

// uc_orig: HM_colvect_add (fallen/Headers/hm.h)
void HM_colvect_add(UBYTE hm_index, SLONG x1, SLONG z1, SLONG x2, SLONG z2);

// uc_orig: HM_process (fallen/Headers/hm.h)
void HM_process(void);

// uc_orig: HM_find_mesh_point (fallen/Headers/hm.h)
void HM_find_mesh_point(UBYTE hm_index, SLONG point, float* x, float* y, float* z);

// uc_orig: HM_find_mesh_pos (fallen/Headers/hm.h)
void HM_find_mesh_pos(UBYTE hm_index, SLONG* x, SLONG* y, SLONG* z, SLONG* yaw, SLONG* pitch, SLONG* roll);

// uc_orig: HM_stationary (fallen/Headers/hm.h)
SLONG HM_stationary(UBYTE hm_index);

// uc_orig: HM_draw (fallen/Headers/hm.h)
void HM_draw(void);

#endif // ENGINE_PHYSICS_HM_H
