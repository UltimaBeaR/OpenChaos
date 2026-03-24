// uc_orig: hm.cpp (fallen/Source/hm.cpp)
// Hypermatter physics system — spring-lattice rigid-body simulation.
// Objects are represented as a 3D grid of mass-points connected by springs.
// Physics is float-based (unlike character physics in collide.cpp which is integer).
// Entrypoint each frame: HM_process() — integrates all points for all active objects.
// At most HM_MAX_OBJECTS (8) exist simultaneously.
// NOT compiled for Dreamcast (TARGET_DC) in original; always active here.

#include <math.h>
#include <string.h>
#include "engine/graphics/pipeline/aeng.h"  // MSG_add

#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "buildings/prim_types.h" // PrimObject, PrimInfo, PrimFace4, PrimPoint, PrimFace3
#include "buildings/prim.h"       // get_prim_info
#include "engine/core/math.h"
#include "map/pap_globals.h"
#include "engine/core/matrix.h"

#include "engine/physics/hm.h"
#include "engine/physics/hm_globals.h"
#include "map/level_pools.h"
#include "assets/formats/anim_globals.h"
#include "engine/core/memory.h"              // MemAlloc, MemFree

// Forward declarations for debug drawing functions (defined in the graphics engine).
// Not included via header to avoid pulling in all graphics headers.
// uc_orig: e_draw_3d_line (fallen/Source/hm.cpp)
void e_draw_3d_line(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2);
// uc_orig: e_draw_3d_line_col_sorted (fallen/Source/hm.cpp)
void e_draw_3d_line_col_sorted(SLONG x1, SLONG y1, SLONG z1, SLONG x2, SLONG y2, SLONG z2, SLONG r, SLONG g, SLONG b);


// uc_orig: HM_index (fallen/Source/hm.cpp)
// Converts 3D grid coordinates to a flat index into ho->index[].
// Layout: row-major with x innermost — idx = x + y*x_res + z*(x_res*y_res).
inline SLONG HM_index(HM_Object* ho, SLONG x, SLONG y, SLONG z)
{
    SLONG ans;

    ASSERT(WITHIN(x, 0, ho->x_res - 1));
    ASSERT(WITHIN(y, 0, ho->y_res - 1));
    ASSERT(WITHIN(z, 0, ho->z_res - 1));

    ans = x;
    ans += y * ho->x_res;
    ans += z * ho->x_res_times_y_res;

    return ans;
}

// uc_orig: HM_init (fallen/Source/hm.cpp)
void HM_init()
{
    SLONG i;

    for (i = 0; i < HM_MAX_OBJECTS; i++) {
        HM_object[i].used = UC_FALSE;
    }
}

// uc_orig: HM_load (fallen/Source/hm.cpp)
// Reads the HM_Primgrid config table from a binary .hm file (version must be 1).
void HM_load(CBYTE* fname)
{
    SLONG i;

    HM_Header hm_h;
    FILE* handle;

    handle = MF_Fopen(fname, "rb");

    if (handle == NULL) {
        return;
    }

    if (fread(&hm_h, sizeof(HM_Header), 1, handle) != 1)
        goto file_error;

    if (hm_h.version != 1) {
        MSG_add("File %s is an obsolete version\n", fname);
        return;
    }

    HM_primgrid_upto = 0;

    for (i = 0; i < hm_h.num_primgrids; i++) {
        ASSERT(WITHIN(HM_primgrid_upto, 0, HM_MAX_PRIMGRIDS - 1));

        if (fread(&HM_primgrid[HM_primgrid_upto++], sizeof(HM_Primgrid), 1, handle) != 1)
            goto file_error;
    }

    MF_Fclose(handle);
    return;

file_error:;
    MF_Fclose(handle);
    return;
}

// uc_orig: HM_get_primgrid (fallen/Source/hm.cpp)
HM_Primgrid* HM_get_primgrid(SLONG prim)
{
    SLONG i;

    for (i = 0; i < HM_primgrid_upto; i++) {
        if (HM_primgrid[i].prim == prim) {
            return &HM_primgrid[i];
        }
    }

    return &HM_default_primgrid;
}

// uc_orig: HM_find_point_inside_cube (fallen/Source/hm.cpp)
// Encodes a prim vertex position as barycentric coordinates within a lattice cube.
// Used by HM_create() to build the HM_Mesh table for rendering.
// Uses the far-corner tetrahedron when along_x+along_y+along_z > 1.5 (for stability).
static HM_Mesh HM_find_point_inside_cube(
    HM_Object* ho,
    SLONG x, SLONG y, SLONG z,
    float ppx, float ppy, float ppz)
{
    SLONG index_o, index_x, index_y, index_z;
    HM_Point* p_o;
    HM_Point* p_x;
    HM_Point* p_y;
    HM_Point* p_z;
    float along_x, along_y, along_z;
    HM_Mesh ans;

    index_o = ho->index[HM_index(ho, x + 0, y + 0, z + 0)];
    index_x = ho->index[HM_index(ho, x + 1, y + 0, z + 0)];
    index_y = ho->index[HM_index(ho, x + 0, y + 1, z + 0)];
    index_z = ho->index[HM_index(ho, x + 0, y + 0, z + 1)];

    ASSERT(WITHIN(index_o, 1, ho->num_points));
    ASSERT(WITHIN(index_x, 1, ho->num_points));
    ASSERT(WITHIN(index_y, 1, ho->num_points));
    ASSERT(WITHIN(index_z, 1, ho->num_points));

    p_o = &ho->point[index_o];
    p_x = &ho->point[index_x];
    p_y = &ho->point[index_y];
    p_z = &ho->point[index_z];

    along_x = (ppx - p_o->x) / (p_x->x - p_o->x);
    along_y = (ppy - p_o->y) / (p_y->y - p_o->y);
    along_z = (ppz - p_o->z) / (p_z->z - p_o->z);

    ASSERT(WITHIN(along_x, 0.0F, 1.0F));
    ASSERT(WITHIN(along_y, 0.0F, 1.0F));
    ASSERT(WITHIN(along_z, 0.0F, 1.0F));

    if (along_x + along_y + along_z > 1.5F) {
        // Use the far corner's tetrahedron — numerically closer for this point.
        index_o = ho->index[HM_index(ho, x + 1, y + 1, z + 1)];
        index_x = ho->index[HM_index(ho, x + 0, y + 1, z + 1)];
        index_y = ho->index[HM_index(ho, x + 1, y + 0, z + 1)];
        index_z = ho->index[HM_index(ho, x + 1, y + 1, z + 0)];

        ASSERT(WITHIN(index_o, 1, ho->num_points));
        ASSERT(WITHIN(index_x, 1, ho->num_points));
        ASSERT(WITHIN(index_y, 1, ho->num_points));
        ASSERT(WITHIN(index_z, 1, ho->num_points));

        along_x = 1.0F - along_x;
        along_y = 1.0F - along_y;
        along_z = 1.0F - along_z;
    }

    ans.origin = index_o;
    ans.p[0] = index_x;
    ans.p[1] = index_y;
    ans.p[2] = index_z;
    ans.along[0] = along_x;
    ans.along[1] = along_y;
    ans.along[2] = along_z;

    return ans;
}

// uc_orig: HM_create (fallen/Source/hm.cpp)
// Builds a new HM physics object for a given prim mesh. Steps:
//   1. Find a free slot (max 8; return HM_NO_MORE_OBJECTS if full).
//   2. Read prim bounding box -> build lattice grid point positions.
//   3. Mark which lattice cubes contain mesh vertices (sparse fill).
//   4. Allocate active lattice points + build the index lookup table.
//   5. Set per-point mass using the linear gradient (x/y/z_dgrav).
//   6. Normalise total mass so gravity is uniform regardless of point count.
//   7. Build edge list: connect each active point to its half-shell neighbours.
//   8. Deduplicate edge rest-lengths into size[]/oversize[] tables.
//   9. Build HM_Mesh table (visual vertex -> lattice barycentric coords).
//  10. Find 4 reference points near COG for HM_find_mesh_pos().
//  11. Apply initial rotation around COG.
//  12. Translate all points to world position.
UBYTE HM_create(
    SLONG prim,
    SLONG pos_x, SLONG pos_y, SLONG pos_z,
    SLONG yaw, SLONG pitch, SLONG roll,
    SLONG vel_x, SLONG vel_y, SLONG vel_z,
    SLONG x_res, SLONG y_res, SLONG z_res,
    SLONG x_point[], SLONG y_point[], SLONG z_point[],
    float x_dgrav, float y_dgrav, float z_dgrav,
    float elasticity, float bounciness, float friction, float damping)
{
    SLONG i;
    SLONG x, y, z;
    SLONG dx, dy, dz;
    SLONG index1, index2;
    SLONG num_points, num_edges;
    SLONG edge_upto, point_upto;
    SLONG ans;
    float dpx, dpy, dpz, size;

    HM_Object* ho;
    HM_Point* hp1;
    HM_Point* hp2;

    // Find a free slot.
    for (i = 0; i < HM_MAX_OBJECTS; i++) {
        ho = &HM_object[i];
        if (!ho->used) {
            ans = i;
            goto found_unused_hm_object;
        }
    }

    return HM_NO_MORE_OBJECTS;

found_unused_hm_object:;

    ho->used = UC_TRUE;
    ho->prim = prim;
    ho->x_res = x_res;
    ho->y_res = y_res;
    ho->z_res = z_res;
    ho->x_res_times_y_res = ho->x_res * ho->y_res;
    ho->elasticity = elasticity;
    ho->bounciness = bounciness;
    ho->friction = friction;
    ho->damping = damping;
    ho->bump = NULL;

    ASSERT(WITHIN(prim, 1, next_prim_object - 1));

    PrimObject* po = &prim_objects[prim];
    PrimInfo* pi = get_prim_info(prim);
    PrimPoint* pp;

    ASSERT(WITHIN(x_res, 2, HM_MAX_RES));
    ASSERT(WITHIN(y_res, 2, HM_MAX_RES));
    ASSERT(WITHIN(z_res, 2, HM_MAX_RES));

    UBYTE empty[HM_MAX_RES][HM_MAX_RES][HM_MAX_RES];
    SLONG cubex[HM_MAX_RES];
    SLONG cubey[HM_MAX_RES];
    SLONG cubez[HM_MAX_RES];

    // Compute lattice plane positions within the prim bounding box.
    for (i = 0; i < x_res; i++) {
        cubex[i] = pi->minx + MUL64(x_point[i], pi->maxx - pi->minx);
    }
    for (i = 0; i < y_res; i++) {
        cubey[i] = pi->miny + MUL64(y_point[i], pi->maxy - pi->miny);
    }
    for (i = 0; i < z_res; i++) {
        cubez[i] = pi->minz + MUL64(z_point[i], pi->maxz - pi->minz);
    }

    // Mark all cubes empty initially.
    for (x = 0; x < x_res - 1; x++)
        for (y = 0; y < y_res - 1; y++)
            for (z = 0; z < z_res - 1; z++) {
                empty[x][y][z] = UC_TRUE;
            }

    // Mark cubes that contain at least one prim vertex as non-empty.
    for (i = po->StartPoint; i < po->EndPoint; i++) {
        pp = &prim_points[i];

        for (x = 0; x < x_res - 1; x++) {
            if (!WITHIN(pp->X, cubex[x], cubex[x + 1])) {
                continue;
            }

            for (y = 0; y < y_res - 1; y++) {
                if (!WITHIN(pp->Y, cubey[y], cubey[y + 1])) {
                    continue;
                }

                for (z = 0; z < z_res - 1; z++) {
                    if (WITHIN(pp->Z, cubez[z], cubez[z + 1])) {
                        empty[x][y][z] = UC_FALSE;
                        goto do_next_point;
                    }
                }
            }
        }

    do_next_point:;
    }

    // Allocate the sparse index table.
    ho->num_indices = ho->x_res * ho->y_res * ho->z_res;
    ho->index = (HM_Index*)MemAlloc(ho->num_indices * sizeof(HM_Index));

    ASSERT(ho->index != NULL);

    memset((UBYTE*)ho->index, 0, ho->num_indices * sizeof(HM_Index));

    // Mark all corner-points of non-empty cubes as active (sentinel = 0xffff).
    for (x = 0; x < x_res - 1; x++)
        for (y = 0; y < y_res - 1; y++)
            for (z = 0; z < z_res - 1; z++) {
                if (!empty[x][y][z]) {
                    ho->index[HM_index(ho, x + 0, y + 0, z + 0)] = 0xffff;
                    ho->index[HM_index(ho, x + 1, y + 0, z + 0)] = 0xffff;
                    ho->index[HM_index(ho, x + 0, y + 1, z + 0)] = 0xffff;
                    ho->index[HM_index(ho, x + 1, y + 1, z + 0)] = 0xffff;
                    ho->index[HM_index(ho, x + 0, y + 0, z + 1)] = 0xffff;
                    ho->index[HM_index(ho, x + 1, y + 0, z + 1)] = 0xffff;
                    ho->index[HM_index(ho, x + 0, y + 1, z + 1)] = 0xffff;
                    ho->index[HM_index(ho, x + 1, y + 1, z + 1)] = 0xffff;
                }
            }

    // Count active points (point 0 is the null sentinel, so start at 1).
    num_points = 1;

    for (x = 0; x < x_res; x++)
        for (y = 0; y < y_res; y++)
            for (z = 0; z < z_res; z++) {
                if (ho->index[HM_index(ho, x, y, z)] != NULL) {
                    num_points += 1;
                }
            }

    ho->point = (HM_Point*)MemAlloc(num_points * sizeof(HM_Point));

    ASSERT(ho->point != NULL);

    // Assign sequential indices to active points and set their initial positions.
    float grav_mid_x = float(x_res) * 0.5F;
    float grav_mid_y = float(y_res) * 0.5F;
    float grav_mid_z = float(z_res) * 0.5F;

    point_upto = 1;

    for (x = 0; x < x_res; x++)
        for (y = 0; y < y_res; y++)
            for (z = 0; z < z_res; z++) {
                if (ho->index[HM_index(ho, x, y, z)] != NULL) {
                    ASSERT(WITHIN(point_upto, 1, num_points - 1));

                    ho->index[HM_index(ho, x, y, z)] = point_upto;

                    ho->point[point_upto].x = float(cubex[x]);
                    ho->point[point_upto].y = float(cubey[y]);
                    ho->point[point_upto].z = float(cubez[z]);

                    ho->point[point_upto].dx = float(vel_x);
                    ho->point[point_upto].dy = float(vel_y);
                    ho->point[point_upto].dz = float(vel_z);

                    // Linear mass gradient: heavier on one side -> rotation tendency.
                    ho->point[point_upto].mass = 1.0F;
                    ho->point[point_upto].mass += (float(x) - grav_mid_x) * x_dgrav;
                    ho->point[point_upto].mass += (float(y) - grav_mid_y) * y_dgrav;
                    ho->point[point_upto].mass += (float(z) - grav_mid_z) * z_dgrav;

                    point_upto += 1;
                }
            }

    ASSERT(point_upto == num_points);

    ho->num_points = num_points;

    // Normalise mass so total gravitational force is independent of point count.
    // Without this, a 4-point lattice would fall slower than an 8-point one.
    {
        float grav_av = 0.0F;
        float grav_adjust;

        for (i = 1; i < ho->num_points; i++) {
            grav_av += ho->point[i].mass;
        }

        grav_av /= float(ho->num_points - 1);

        if (grav_av >= 0.01F) {
            grav_adjust = 1.0F / grav_av;

            for (i = 1; i < ho->num_points; i++) {
                ho->point[i].mass *= grav_adjust;
            }
        }
    }

    // Count edges by enumerating the "forward half-shell" of each active point:
    // 9 neighbours in the next Z-slice + 4 in the same XY plane.
    // This visits each pair exactly once, avoiding duplicate edges.
    num_edges = 0;

    for (z = 0; z < ho->z_res; z++)
        for (y = 0; y < ho->y_res; y++)
            for (x = 0; x < ho->x_res; x++) {
                index1 = HM_index(ho, x, y, z);

                if (ho->index[index1] == NULL) {
                    continue;
                }

                for (dz = 0; dz <= 1; dz++)
                    for (dy = -1; dy <= 1; dy++)
                        for (dx = -1; dx <= 1; dx++) {
                            if ((dz == 1) || (dx == -1 && dy == 1) || (dx == 0 && dy == 1) || (dx == 1 && dy == 1) || (dx == 1 && dy == 0)) {
                                if (WITHIN(x + dx, 0, ho->x_res - 1) && WITHIN(y + dy, 0, ho->y_res - 1) && WITHIN(z + dz, 0, ho->z_res - 1)) {
                                    index2 = HM_index(ho, x + dx, y + dy, z + dz);

                                    if (ho->index[index2] != NULL) {
                                        num_edges += 1;
                                    }
                                }
                            }
                        }
            }

    ho->num_edges = num_edges;

    ho->edge = (HM_Edge*)MemAlloc(num_edges * sizeof(HM_Edge));

    // Build the edge list. Deduplicate squared rest-lengths into size[]/oversize[].
    edge_upto = 0;
    ho->num_sizes = 0;

    for (z = 0; z < ho->z_res; z++)
        for (y = 0; y < ho->y_res; y++)
            for (x = 0; x < ho->x_res; x++) {
                index1 = HM_index(ho, x, y, z);

                if (ho->index[index1] == NULL) {
                    continue;
                }

                for (dz = 0; dz <= 1; dz++)
                    for (dy = -1; dy <= 1; dy++)
                        for (dx = -1; dx <= 1; dx++) {
                            if ((dz == 1) || (dx == -1 && dy == 1) || (dx == 0 && dy == 1) || (dx == 1 && dy == 1) || (dx == 1 && dy == 0)) {
                                if (WITHIN(x + dx, 0, ho->x_res - 1) && WITHIN(y + dy, 0, ho->y_res - 1) && WITHIN(z + dz, 0, ho->z_res - 1)) {
                                    index2 = HM_index(ho, x + dx, y + dy, z + dz);

                                    if (ho->index[index2] != NULL) {
                                        ASSERT(WITHIN(edge_upto, 0, ho->num_edges - 1));

                                        ho->edge[edge_upto].p1 = ho->index[index1];
                                        ho->edge[edge_upto].p2 = ho->index[index2];

                                        hp1 = &ho->point[ho->index[index1]];
                                        hp2 = &ho->point[ho->index[index2]];

                                        dpx = hp2->x - hp1->x;
                                        dpy = hp2->y - hp1->y;
                                        dpz = hp2->z - hp1->z;

                                        size = dpx * dpx + dpy * dpy + dpz * dpz;

                                        for (i = 0; i < ho->num_sizes; i++) {
                                            if (ho->size[i] == size) {
                                                ho->edge[edge_upto].len = i;
                                                goto found_size;
                                            }
                                        }

                                        ASSERT(WITHIN(ho->num_sizes, 0, HM_MAX_SIZES - 1));

                                        ho->size[ho->num_sizes] = size;
                                        ho->oversize[ho->num_sizes] = 1.0F / size;
                                        ho->edge[edge_upto].len = ho->num_sizes;
                                        ho->num_sizes += 1;

                                    found_size:;

                                        edge_upto += 1;
                                    }
                                }
                            }
                        }
            }

    ASSERT(edge_upto == num_edges);

    // Build the mesh table: one entry per prim vertex, encoding its barycentric
    // position within whichever lattice cube it falls in.
    ho->num_meshes = po->EndPoint - po->StartPoint;
    ho->mesh = (HM_Mesh*)MemAlloc(ho->num_meshes * sizeof(HM_Mesh));

    ASSERT(ho->mesh != NULL);

    for (i = 0; i < ho->num_meshes; i++) {
        pp = &prim_points[po->StartPoint + i];

        for (x = 0; x < x_res - 1; x++) {
            if (!WITHIN(pp->X, cubex[x], cubex[x + 1])) {
                continue;
            }

            for (y = 0; y < y_res - 1; y++) {
                if (!WITHIN(pp->Y, cubey[y], cubey[y + 1])) {
                    continue;
                }

                for (z = 0; z < z_res - 1; z++) {
                    if (WITHIN(pp->Z, cubez[z], cubez[z + 1])) {
                        ho->mesh[i] = HM_find_point_inside_cube(
                            ho, x, y, z,
                            float(pp->X), float(pp->Y), float(pp->Z));

                        goto done_this_point;
                    }
                }
            }
        }

        ASSERT(0);

    done_this_point:;
    }

    // Find the 4 reference lattice points closest to the prim COG.
    // These are used by HM_find_mesh_pos() to extract position and orientation.
    {
        float best_score = float(UC_INFINITY);
        SLONG best_origin = -1, best_x = -1, best_y = -1, best_z = -1;
        SLONG index_o, index_x, index_y, index_z;
        float score;

        for (x = 0; x < ho->x_res - 1; x++)
            for (y = 0; y < ho->y_res - 1; y++)
                for (z = 0; z < ho->z_res - 1; z++) {
                    index_o = ho->index[HM_index(ho, x, y, z)];
                    index_x = ho->index[HM_index(ho, x + 1, y, z)];
                    index_y = ho->index[HM_index(ho, x, y + 1, z)];
                    index_z = ho->index[HM_index(ho, x, y, z + 1)];

                    if (index_o != NULL && index_x != NULL && index_y != NULL && index_z != NULL) {
                        score = 0;

                        score += fabs(ho->point[index_o].x - float(pi->cogx));
                        score += fabs(ho->point[index_o].y - float(pi->cogy));
                        score += fabs(ho->point[index_o].z - float(pi->cogz));

                        score += fabs(ho->point[index_o].x);
                        score += fabs(ho->point[index_o].y);
                        score += fabs(ho->point[index_o].z);

                        if (score < best_score) {
                            best_score = score;
                            best_origin = index_o;
                            best_x = index_x;
                            best_y = index_y;
                            best_z = index_z;
                        }
                    }
                }

        if (best_score >= UC_INFINITY) {
            // Could not find valid reference points — free everything.
            MemFree(ho->index);
            MemFree(ho->point);
            MemFree(ho->edge);
            MemFree(ho->mesh);
            return HM_NO_MORE_OBJECTS;
        }

        ho->x_index = best_x;
        ho->y_index = best_y;
        ho->z_index = best_z;
        ho->o_index = best_origin;
        ho->o_prim_x = ho->point[best_origin].x;
        ho->o_prim_y = ho->point[best_origin].y;
        ho->o_prim_z = ho->point[best_origin].z;
        ho->cog_x = float(pi->cogx);
        ho->cog_y = float(pi->cogy);
        ho->cog_z = float(pi->cogz);
    }

    // Apply initial rotation: rotate all lattice points around the COG.
    {
        float f_yaw   = float(yaw)   * 2.0F * PI / 2048.0F;
        float f_pitch = float(pitch) * 2.0F * PI / 2048.0F;
        float f_roll  = float(roll)  * 2.0F * PI / 2048.0F;

        float matrix[9];
        MATRIX_calc(matrix, f_yaw, f_pitch, f_roll);

        float cogx = float(pi->cogx);
        float cogy = float(pi->cogy);
        float cogz = float(pi->cogz);

        for (i = 1; i < ho->num_points; i++) {
            ho->point[i].x -= cogx;
            ho->point[i].y -= cogy;
            ho->point[i].z -= cogz;

            MATRIX_MUL_BY_TRANSPOSE(matrix, ho->point[i].x, ho->point[i].y, ho->point[i].z);

            ho->point[i].x += cogx;
            ho->point[i].y += cogy;
            ho->point[i].z += cogz;
        }
    }

    // Translate to world position.
    {
        float fposx = float(pos_x);
        float fposy = float(pos_y);
        float fposz = float(pos_z);

        for (i = 1; i < ho->num_points; i++) {
            ho->point[i].x += fposx;
            ho->point[i].y += fposy;
            ho->point[i].z += fposz;
        }
    }

    return ans;
}

// uc_orig: HM_destroy (fallen/Source/hm.cpp)
// Frees all allocated arrays for an HM slot and marks it unused.
// Note: does NOT free pending HM_Bump nodes — potential leak if bumps are active.
void HM_destroy(UBYTE hm_index)
{
    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];

    if (ho->used) {
        ho->used = UC_FALSE;

        MemFree(ho->index);
        MemFree(ho->point);
        MemFree(ho->edge);
        MemFree(ho->mesh);
    }
}

// uc_orig: HM_find_cog (fallen/Source/hm.cpp)
// Returns the mass-weighted centre of gravity across all active lattice points.
void HM_find_cog(UBYTE hm_index, float* x, float* y, float* z)
{
    SLONG i;
    float ans_x = 0.0F, ans_y = 0.0F, ans_z = 0.0F;
    float tot_mass = 0.0F;

    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];
    HM_Point* hp;

    ASSERT(ho->used);

    for (i = 1; i < ho->num_points; i++) {
        hp = &ho->point[i];

        ans_x += hp->x * hp->mass;
        ans_y += hp->y * hp->mass;
        ans_z += hp->z * hp->mass;

        tot_mass += hp->mass;
    }

    ans_x /= tot_mass;
    ans_y /= tot_mass;
    ans_z /= tot_mass;

    *x = ans_x;
    *y = ans_y;
    *z = ans_z;
}

// uc_orig: HM_colvect_clear (fallen/Source/hm.cpp)
// Resets the per-frame wall segment list for an HM object.
void HM_colvect_clear(UBYTE hm)
{
    ASSERT(WITHIN(hm, 0, HM_MAX_OBJECTS - 1));

    HM_object[hm].num_cols = 0;
}

// uc_orig: HM_colvect_add (fallen/Source/hm.cpp)
// Adds a 2D XZ wall segment. Deduplicates identical segments.
// Sets consider=0xffffffff so all points test against this wall.
void HM_colvect_add(UBYTE hm, SLONG x1, SLONG z1, SLONG x2, SLONG z2)
{
    SLONG i;
    float dx, dz;

    ASSERT(WITHIN(hm, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm];
    HM_Col* hc;

    ASSERT(ho->used);
    ASSERT(WITHIN(ho->num_cols, 0, HM_COLS_PER_OBJECT - 1));

    // Deduplicate.
    for (i = 0; i < ho->num_cols; i++) {
        hc = &ho->col[i];

        if (hc->x1 == float(x1) && hc->z1 == float(z1) && hc->x2 == float(x2) && hc->z2 == float(z2)) {
            return;
        }
    }

    hc = &ho->col[ho->num_cols++];

    hc->x1 = float(x1);
    hc->z1 = float(z1);
    hc->x2 = float(x2);
    hc->z2 = float(z2);

    dx = hc->x2 - hc->x1;
    dz = hc->z2 - hc->z1;

    hc->len = sqrt(dx * dx + dz * dz);
    hc->consider = 0xffffffff;
}

// Forward-declare calc_height_at rather than including collide.h
// (which would drag in thing.h and cascading headers — original comment preserved).
// uc_orig: calc_height_at (fallen/Source/hm.cpp)
SLONG calc_height_at(SLONG x, SLONG z);


// uc_orig: HM_find_mesh_point (fallen/Source/hm.cpp)
// Reconstructs a visual vertex position from the deformed lattice using barycentric coords.
void HM_find_mesh_point(UBYTE hm_index, SLONG point, float* x, float* y, float* z)
{
    float ansx, ansy, ansz;
    HM_Point* p_o;
    HM_Point* p_x;
    HM_Point* p_y;
    HM_Point* p_z;

    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];
    HM_Mesh* hm;

    ASSERT(WITHIN(point, 0, ho->num_meshes - 1));

    hm = &ho->mesh[point];

    ASSERT(WITHIN(hm->origin, 1, ho->num_points - 1));
    ASSERT(WITHIN(hm->p[0], 1, ho->num_points - 1));
    ASSERT(WITHIN(hm->p[1], 1, ho->num_points - 1));
    ASSERT(WITHIN(hm->p[2], 1, ho->num_points - 1));

    p_o = &ho->point[hm->origin];
    p_x = &ho->point[hm->p[0]];
    p_y = &ho->point[hm->p[1]];
    p_z = &ho->point[hm->p[2]];

    ansx = p_o->x;
    ansy = p_o->y;
    ansz = p_o->z;

    ansx += hm->along[0] * (p_x->x - p_o->x);
    ansy += hm->along[0] * (p_x->y - p_o->y);
    ansz += hm->along[0] * (p_x->z - p_o->z);

    ansx += hm->along[1] * (p_y->x - p_o->x);
    ansy += hm->along[1] * (p_y->y - p_o->y);
    ansz += hm->along[1] * (p_y->z - p_o->z);

    ansx += hm->along[2] * (p_z->x - p_o->x);
    ansy += hm->along[2] * (p_z->y - p_o->y);
    ansz += hm->along[2] * (p_z->z - p_o->z);

    *x = ansx;
    *y = ansy;
    *z = ansz;
}

// uc_orig: HM_cube_exists (fallen/Source/hm.cpp)
// Returns UC_TRUE if all 8 corner-points of a lattice cube are active.
// Used by HM_collide() to detect boundary faces.
static SLONG HM_cube_exists(HM_Object* ho, SLONG x_cube, SLONG y_cube, SLONG z_cube)
{
    SLONG i, dx, dy, dz, px, py, pz, index;

    if (!WITHIN(x_cube, 0, ho->x_res - 2) || !WITHIN(y_cube, 0, ho->y_res - 2) || !WITHIN(z_cube, 0, ho->z_res - 2)) {
        return UC_FALSE;
    }

    for (i = 0; i < 8; i++) {
        dx = (i >> 0) & 1;
        dy = (i >> 1) & 1;
        dz = (i >> 2) & 1;

        px = x_cube + dx;
        py = y_cube + dy;
        pz = z_cube + dz;

        if (!WITHIN(px, 0, ho->x_res - 1) || !WITHIN(py, 0, ho->y_res - 1) || !WITHIN(pz, 0, ho->z_res - 1)) {
            return UC_FALSE;
        }

        index = HM_index(ho, px, py, pz);

        if (ho->index[index] == NULL) {
            return UC_FALSE;
        }
    }

    return UC_TRUE;
}

// uc_orig: HM_is_point_in_cube (fallen/Source/hm.cpp)
// Tests whether world-space point 'p' is inside the deformed lattice cube.
// Transforms p into cube-local space using the deformed axis vectors.
// Returns UC_TRUE if all three local coordinates are within [0,1].
static SLONG HM_is_point_in_cube(
    HM_Object* ho,
    HM_Point* p,
    SLONG x_cube, SLONG y_cube, SLONG z_cube,
    float* rel_x, float* rel_y, float* rel_z)
{
    HM_Point *hp_o, *hp_x, *hp_y, *hp_z;
    float rx, ry, rz, len, matrix[9];
    SLONG index_o, index_x, index_y, index_z;

    ASSERT(WITHIN(x_cube, 0, ho->x_res - 2));
    ASSERT(WITHIN(y_cube, 0, ho->y_res - 2));
    ASSERT(WITHIN(z_cube, 0, ho->z_res - 2));

    if (!HM_cube_exists(ho, x_cube, y_cube, z_cube)) {
        return UC_FALSE;
    }

    index_o = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 0);
    index_x = HM_index(ho, x_cube + 1, y_cube + 0, z_cube + 0);
    index_y = HM_index(ho, x_cube + 0, y_cube + 1, z_cube + 0);
    index_z = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 1);

    ASSERT(WITHIN(ho->index[index_o], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_x], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_y], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_z], 1, ho->num_points - 1));

    hp_o = &ho->point[ho->index[index_o]];
    hp_x = &ho->point[ho->index[index_x]];
    hp_y = &ho->point[ho->index[index_y]];
    hp_z = &ho->point[ho->index[index_z]];

    matrix[0] = hp_x->x - hp_o->x;
    matrix[1] = hp_x->y - hp_o->y;
    matrix[2] = hp_x->z - hp_o->z;
    matrix[3] = hp_y->x - hp_o->x;
    matrix[4] = hp_y->y - hp_o->y;
    matrix[5] = hp_y->z - hp_o->z;
    matrix[6] = hp_z->x - hp_o->x;
    matrix[7] = hp_z->y - hp_o->y;
    matrix[8] = hp_z->z - hp_o->z;

    // Normalise each row by 1/len² so cube-local coords map to [0,1].
    len = matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2];
    len = 1.0F / len;
    matrix[0] *= len; matrix[1] *= len; matrix[2] *= len;

    len = matrix[3] * matrix[3] + matrix[4] * matrix[4] + matrix[5] * matrix[5];
    len = 1.0F / len;
    matrix[3] *= len; matrix[4] *= len; matrix[5] *= len;

    len = matrix[6] * matrix[6] + matrix[7] * matrix[7] + matrix[8] * matrix[8];
    len = 1.0F / len;
    matrix[6] *= len; matrix[7] *= len; matrix[8] *= len;

    rx = p->x - hp_o->x;
    ry = p->y - hp_o->y;
    rz = p->z - hp_o->z;

    MATRIX_MUL(matrix, rx, ry, rz);

    if (WITHIN(rx, 0.0F, 1.0F) && WITHIN(ry, 0.0F, 1.0F) && WITHIN(rz, 0.0F, 1.0F)) {
        *rel_x = rx;
        *rel_y = ry;
        *rel_z = rz;
        return UC_TRUE;
    }

    return UC_FALSE;
}

// uc_orig: HM_last_point_in_last_cube (fallen/Source/hm.cpp)
// Reconstructs where a point WAS last frame, in last-frame cube-local space.
// Uses (pos - vel) to get "previous frame" positions for swept collision detection.
static void HM_last_point_in_last_cube(
    HM_Object* ho,
    HM_Point* p,
    SLONG x_cube, SLONG y_cube, SLONG z_cube,
    float* last_rel_x, float* last_rel_y, float* last_rel_z)
{
    HM_Point *hp_o, *hp_x, *hp_y, *hp_z;
    float rx, ry, rz, len, matrix[9];
    SLONG index_o, index_x, index_y, index_z;

    ASSERT(WITHIN(x_cube, 0, ho->x_res - 2));
    ASSERT(WITHIN(y_cube, 0, ho->y_res - 2));
    ASSERT(WITHIN(z_cube, 0, ho->z_res - 2));

    ASSERT(HM_cube_exists(ho, x_cube, y_cube, z_cube));

    index_o = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 0);
    index_x = HM_index(ho, x_cube + 1, y_cube + 0, z_cube + 0);
    index_y = HM_index(ho, x_cube + 0, y_cube + 1, z_cube + 0);
    index_z = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 1);

    ASSERT(WITHIN(ho->index[index_o], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_x], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_y], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_z], 1, ho->num_points - 1));

    hp_o = &ho->point[ho->index[index_o]];
    hp_x = &ho->point[ho->index[index_x]];
    hp_y = &ho->point[ho->index[index_y]];
    hp_z = &ho->point[ho->index[index_z]];

    // Use (pos - vel) for previous-frame positions.
    matrix[0] = (hp_x->x - hp_x->dx) - (hp_o->x - hp_o->dx);
    matrix[1] = (hp_x->y - hp_x->dy) - (hp_o->y - hp_o->dy);
    matrix[2] = (hp_x->z - hp_x->dz) - (hp_o->z - hp_o->dz);
    matrix[3] = (hp_y->x - hp_y->dx) - (hp_o->x - hp_o->dx);
    matrix[4] = (hp_y->y - hp_y->dy) - (hp_o->y - hp_o->dy);
    matrix[5] = (hp_y->z - hp_y->dz) - (hp_o->z - hp_o->dz);
    matrix[6] = (hp_z->x - hp_z->dx) - (hp_o->x - hp_o->dx);
    matrix[7] = (hp_z->y - hp_z->dy) - (hp_o->y - hp_o->dy);
    matrix[8] = (hp_z->z - hp_z->dz) - (hp_o->z - hp_o->dz);

    len = matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2];
    len = 1.0F / len;
    matrix[0] *= len; matrix[1] *= len; matrix[2] *= len;

    len = matrix[3] * matrix[3] + matrix[4] * matrix[4] + matrix[5] * matrix[5];
    len = 1.0F / len;
    matrix[3] *= len; matrix[4] *= len; matrix[5] *= len;

    len = matrix[6] * matrix[6] + matrix[7] * matrix[7] + matrix[8] * matrix[8];
    len = 1.0F / len;
    matrix[6] *= len; matrix[7] *= len; matrix[8] *= len;

    rx = (p->x - p->dx) - (hp_o->x - hp_o->dx);
    ry = (p->y - p->dy) - (hp_o->y - hp_o->dy);
    rz = (p->z - p->dz) - (hp_o->z - hp_o->dz);

    MATRIX_MUL(matrix, rx, ry, rz);

    *last_rel_x = rx;
    *last_rel_y = ry;
    *last_rel_z = rz;
}

// uc_orig: HM_collide_all (fallen/Source/hm.cpp)
// O(n^2) broad-phase: calls HM_collide in both directions for each active pair.
// Bug in original: inner loop checks HM_object[i].used instead of [j] — harmless
// since outer loop already ensures i is used and max 8 objects keep cost trivial.
void HM_collide_all()
{
    SLONG i, j;

    for (i = 0; i < HM_MAX_OBJECTS; i++) {
        if (!HM_object[i].used) {
            continue;
        }

        for (j = i + 1; j < HM_MAX_OBJECTS; j++) {
            if (!HM_object[i].used) {   // Bug in original: should be [j]. Preserved 1:1.
                continue;
            }

            HM_collide(i, j);
            HM_collide(j, i);
        }
    }
}

// uc_orig: HM_rel_cube_to_world (fallen/Source/hm.cpp)
// Converts barycentric cube-local coordinates back to world space.
// The normalisation block is disabled (WE_WANT_TO_NORMALISE_THE_MATRIX = false)
// so the matrix is used un-normalised — consistent with how rel coords were computed.
static void HM_rel_cube_to_world(
    HM_Object* ho,
    SLONG x_cube, SLONG y_cube, SLONG z_cube,
    float rel_x, float rel_y, float rel_z,
    float* world_x, float* world_y, float* world_z)
{
    float wx, wy, wz;
    float matrix[9];
    HM_Point *hp_o, *hp_x, *hp_y, *hp_z;
    SLONG index_o, index_x, index_y, index_z;

    ASSERT(WITHIN(x_cube, 0, ho->x_res - 2));
    ASSERT(WITHIN(y_cube, 0, ho->y_res - 2));
    ASSERT(WITHIN(z_cube, 0, ho->z_res - 2));

    ASSERT(HM_cube_exists(ho, x_cube, y_cube, z_cube));

    index_o = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 0);
    index_x = HM_index(ho, x_cube + 1, y_cube + 0, z_cube + 0);
    index_y = HM_index(ho, x_cube + 0, y_cube + 1, z_cube + 0);
    index_z = HM_index(ho, x_cube + 0, y_cube + 0, z_cube + 1);

    ASSERT(WITHIN(ho->index[index_o], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_x], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_y], 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->index[index_z], 1, ho->num_points - 1));

    hp_o = &ho->point[ho->index[index_o]];
    hp_x = &ho->point[ho->index[index_x]];
    hp_y = &ho->point[ho->index[index_y]];
    hp_z = &ho->point[ho->index[index_z]];

    matrix[0] = hp_x->x - hp_o->x;
    matrix[1] = hp_x->y - hp_o->y;
    matrix[2] = hp_x->z - hp_o->z;
    matrix[3] = hp_y->x - hp_o->x;
    matrix[4] = hp_y->y - hp_o->y;
    matrix[5] = hp_y->z - hp_o->z;
    matrix[6] = hp_z->x - hp_o->x;
    matrix[7] = hp_z->y - hp_o->y;
    matrix[8] = hp_z->z - hp_o->z;

    wx = rel_x;
    wy = rel_y;
    wz = rel_z;

    MATRIX_MUL_BY_TRANSPOSE(matrix, wx, wy, wz);

    wx += hp_o->x;
    wy += hp_o->y;
    wz += hp_o->z;

    *world_x = wx;
    *world_y = wy;
    *world_z = wz;
}

// uc_orig: HM_process_bump (fallen/Source/hm.cpp)
// Applies spring-back force to a point that has penetrated another HM object.
// Draws a debug line from penetrating point to entry point each frame.
// Equal-and-opposite force to object B is disabled in original — one-sided response only.
static void HM_process_bump(HM_Object* ho, HM_Bump* hb)
{
    float out_x, out_y, out_z;
    float squash;
    float dx, dy, dz;
    float fx, fy, fz;
    float dist;

    ASSERT(WITHIN(hb->point, 1, ho->num_points - 1));
    ASSERT(WITHIN(hb->hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Point* hp = &ho->point[hb->point];
    HM_Object* ho2 = &HM_object[hb->hm_index];

    MSG_add("Bumping!");

    HM_rel_cube_to_world(
        ho2,
        hb->cube_x, hb->cube_y, hb->cube_z,
        hb->rel_x, hb->rel_y, hb->rel_z,
        &out_x, &out_y, &out_z);

    e_draw_3d_line(
        hp->x, hp->y, hp->z,
        out_x, out_y, out_z);

    dx = out_x - hp->x;
    dy = out_y - hp->y;
    dz = out_z - hp->z;

    dist = dx * dx + dy * dy + dz * dz;

    squash = ho2->elasticity * dist * 0.0003F;

    fx = squash * dx;
    fy = squash * dy;
    fz = squash * dz;

    hp->dx += fx;
    hp->dy += fy;
    hp->dz += fz;
}

// uc_orig: HM_bump_dead (fallen/Source/hm.cpp)
// Returns UC_TRUE if a tracked penetration is over (point has exited the other object).
// Checks the original entry cube first, then scans all cubes if needed.
static SLONG HM_bump_dead(HM_Object* ho, HM_Bump* hb)
{
    SLONG sx, sy, sz;
    float rel_x, rel_y, rel_z;

    ASSERT(WITHIN(hb->point, 1, ho->num_points - 1));
    ASSERT(WITHIN(hb->hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Point* hp = &ho->point[hb->point];
    HM_Object* ho2 = &HM_object[hb->hm_index];

    // Fast path: check original entry cube first.
    if (HM_is_point_in_cube(ho2, hp, hb->cube_x, hb->cube_y, hb->cube_z, &rel_x, &rel_y, &rel_z)) {
        return UC_FALSE;
    }

    // Slow path: scan all cubes.
    for (sx = 0; sx < ho2->x_res - 1; sx++)
        for (sy = 0; sy < ho2->y_res - 1; sy++)
            for (sz = 0; sz < ho2->z_res - 1; sz++) {
                if (sx == hb->cube_x && sy == hb->cube_y && sz == hb->cube_z) {
                    // Already checked above.
                } else {
                    if (HM_is_point_in_cube(ho2, hp, sx, sy, sz, &rel_x, &rel_y, &rel_z)) {
                        return UC_FALSE;
                    }
                }
            }

    return UC_TRUE;
}

// uc_orig: HM_collide (fallen/Source/hm.cpp)
// Narrow-phase HM-vs-HM collision: tests points of object1 against cubes of object2.
// For each newly penetrating point, allocates an HM_Bump and adds it to object1's list.
// The already_bumped bitmask prevents duplicate entries (16 bytes = 128 bits max).
void HM_collide(UBYTE hm_index1, UBYTE hm_index2)
{
    SLONG i, j, k;
    SLONG sx, sy, sz;

    float dpx, dpy, dpz;
    float rel_x, rel_y, rel_z;
    float last_rel_x, last_rel_y, last_rel_z;
    float along_x, along_y, along_z;
    float along_enter;
    float enter_rel_x, enter_rel_y, enter_rel_z;
    float out_x, out_y, out_z;
    float total_dist, dist;

    SLONG byte, bit;

    HM_Object* ho1;
    HM_Object* ho2;
    HM_Point* hp;
    HM_Bump* hb;

// uc_orig: HM_ALREADY_BYTES (fallen/Source/hm.cpp)
#define HM_ALREADY_BYTES 16
    UBYTE already_bumped[HM_ALREADY_BYTES];

    ASSERT(WITHIN(hm_index1, 0, HM_MAX_OBJECTS - 1));
    ASSERT(WITHIN(hm_index2, 0, HM_MAX_OBJECTS - 1));

    ho1 = &HM_object[hm_index1];
    ho2 = &HM_object[hm_index2];

    // Build a bitmask of points already tracked in a bump record against object2.
    memset((UBYTE*)already_bumped, 0, sizeof(already_bumped));

    for (hb = ho1->bump; hb; hb = hb->next) {
        if (hb->hm_index == hm_index2) {
            byte = hb->point >> 3;
            bit  = hb->point & 7;

            ASSERT(WITHIN(byte, 0, HM_ALREADY_BYTES - 1));

            already_bumped[byte] |= 1 << bit;
        }
    }

    for (i = 1; i < ho1->num_points; i++) {
        hp = &ho1->point[i];

        if (already_bumped[i >> 3] & (1 << (i & 7))) {
            continue;
        }

        for (sx = 0; sx < ho2->x_res - 1; sx++)
            for (sy = 0; sy < ho2->y_res - 1; sy++)
                for (sz = 0; sz < ho2->z_res - 1; sz++) {
                    if (HM_is_point_in_cube(ho2, hp, sx, sy, sz, &rel_x, &rel_y, &rel_z)) {
                        HM_last_point_in_last_cube(ho2, hp, sx, sy, sz, &last_rel_x, &last_rel_y, &last_rel_z);

                        // Find which face the point entered through (swept CCD).
                        along_x = float(UC_INFINITY);
                        along_y = float(UC_INFINITY);
                        along_z = float(UC_INFINITY);

                        if (last_rel_x > 1.0F && rel_x <= 1.0F && !HM_cube_exists(ho2, sx + 1, sy, sz)) {
                            along_x = (1.0F - last_rel_x) / (rel_x - last_rel_x);
                        } else if (last_rel_x < 0.0F && rel_x >= 0.0F && !HM_cube_exists(ho2, sx - 1, sy, sz)) {
                            along_x = (0.0F - last_rel_x) / (rel_x - last_rel_x);
                        }

                        if (last_rel_y > 1.0F && rel_y <= 1.0F && !HM_cube_exists(ho2, sx, sy + 1, sz)) {
                            along_y = (1.0F - last_rel_y) / (rel_y - last_rel_y);
                        } else if (last_rel_y < 0.0F && rel_y >= 0.0F && !HM_cube_exists(ho2, sx, sy - 1, sz)) {
                            along_y = (0.0F - last_rel_y) / (rel_y - last_rel_y);
                        }

                        if (last_rel_z > 1.0F && rel_z <= 1.0F && !HM_cube_exists(ho2, sx, sy, sz + 1)) {
                            along_z = (1.0F - last_rel_z) / (rel_z - last_rel_z);
                        } else if (last_rel_z < 0.0F && rel_z >= 0.0F && !HM_cube_exists(ho2, sx, sy, sz - 1)) {
                            along_z = (0.0F - last_rel_z) / (rel_z - last_rel_z);
                        }

                        along_enter = float(UC_INFINITY);
                        if (along_x < along_enter) along_enter = along_x;
                        if (along_y < along_enter) along_enter = along_y;
                        if (along_z < along_enter) along_enter = along_z;

                        enter_rel_x = last_rel_x + along_enter * (rel_x - last_rel_x);
                        enter_rel_y = last_rel_y + along_enter * (rel_y - last_rel_y);
                        enter_rel_z = last_rel_z + along_enter * (rel_z - last_rel_z);

                        HM_rel_cube_to_world(
                            ho2, sx, sy, sz,
                            enter_rel_x, enter_rel_y, enter_rel_z,
                            &out_x, &out_y, &out_z);

                        // Find the 8 nearest points of object2 to the entry point.
                        SLONG opp_num;
                        SLONG opp_point[HM_NUM_OPP_POINTS];
                        float opp_dist[HM_NUM_OPP_POINTS];

                        dpx = ho2->point[1].x - out_x;
                        dpy = ho2->point[1].y - out_y;
                        dpz = ho2->point[1].z - out_z;

                        dist = dpx * dpx + dpy * dpy + dpz * dpz;

                        opp_point[0] = 1;
                        opp_dist[0] = dist;
                        opp_num = 1;

                        for (j = 2; j < ho2->num_points; j++) {
                            dpx = ho2->point[j].x - out_x;
                            dpy = ho2->point[j].y - out_y;
                            dpz = ho2->point[j].z - out_z;

                            dist = dpx * dpx + dpy * dpy + dpz * dpz;

                            for (k = opp_num - 1; k >= 0; k--) {
                                if (opp_dist[k] > dist) {
                                    if (k + 1 < HM_NUM_OPP_POINTS) {
                                        opp_point[k + 1] = opp_point[k];
                                        opp_dist[k + 1] = opp_dist[k];
                                    }
                                } else {
                                    if (k + 1 < HM_NUM_OPP_POINTS) {
                                        opp_point[k + 1] = j;
                                        opp_dist[k + 1] = dist;
                                        break;
                                    }
                                }
                            }

                            opp_num += 1;
                            if (opp_num > HM_NUM_OPP_POINTS) {
                                opp_num = HM_NUM_OPP_POINTS;
                            }
                        }

                        hb = (HM_Bump*)MemAlloc(sizeof(HM_Bump));

                        hb->point     = i;
                        hb->hm_index  = hm_index2;
                        hb->cube_x    = sx;
                        hb->cube_y    = sy;
                        hb->cube_z    = sz;
                        hb->rel_x     = enter_rel_x;
                        hb->rel_y     = enter_rel_y;
                        hb->rel_z     = enter_rel_z;

                        total_dist = 0;

                        for (j = 0; j < HM_NUM_OPP_POINTS; j++) {
                            hb->opp_point[j] = opp_point[j];
                            hb->opp_prop[j]  = 1.0F / opp_dist[j];
                            total_dist += hb->opp_prop[j];
                        }

                        for (j = 0; j < HM_NUM_OPP_POINTS; j++) {
                            hb->opp_prop[j] *= (1.0F / total_dist);
                        }

                        {
                            HM_Bump* bb;
                            for (bb = ho1->bump; bb; bb = bb->next) {
                                if (bb->hm_index == hm_index2) {
                                    if (bb->point == i) {
                                        MSG_add("Two that are the same.");
                                    }
                                }
                            }
                        }

                        hb->next = ho1->bump;
                        ho1->bump = hb;
                    }
                }
    }
}

// uc_orig: HM_GRAVITY (fallen/Source/hm.cpp)
// Downward acceleration per tick in block units. ~2400 world-units/s² at 25fps.
#define HM_GRAVITY (-0.015F)

// uc_orig: HM_process (fallen/Source/hm.cpp)
// Main physics tick. Per active object, per frame:
//   Step 1 — Spring forces (Hooke's law on squared distance, avoids sqrt).
//   Step 2 — HM-vs-HM bump forces.
//   Step 3 — Integration: damping, gravity, ground bounce, wall reflection, position update.
//   Step 4 — Cleanup expired bump records.
void HM_process()
{
    SLONG i, j, k;
    float squaredist, dpx, dpy, dpz, pdist, wantdist;
    float ddist, squash;
    float fx, fy, fz;
    float gy;

    HM_Object* ho;
    HM_Point* hp;
    HM_Point* hp1;
    HM_Point* hp2;
    HM_Edge* he;
    HM_Col* hc;
    HM_Bump* hb;
    HM_Bump** prev;
    HM_Bump* dead;

    for (i = 0; i < HM_MAX_OBJECTS; i++) {
        ho = &HM_object[i];

        if (!ho->used) {
            continue;
        }

        // Step 1: Spring forces via the edge list.
        // Force = (current_sq_dist - rest_sq_dist) * elasticity / rest_sq_dist
        // Applied equally to both endpoints (Newton 3rd law, ignores mass difference).
        // Using squared distance avoids sqrt per spring per frame.
        for (j = 0; j < ho->num_edges; j++) {
            he = &ho->edge[j];

            ASSERT(WITHIN(he->p1, 0, ho->num_points - 1));
            ASSERT(WITHIN(he->p2, 0, ho->num_points - 1));

            hp1 = &ho->point[he->p1];
            hp2 = &ho->point[he->p2];

            dpx = hp2->x - hp1->x;
            dpy = hp2->y - hp1->y;
            dpz = hp2->z - hp1->z;

            squaredist  = dpx * dpx;
            squaredist += dpy * dpy;
            squaredist += dpz * dpz;

            if (squaredist > 0.001F) {
                ASSERT(WITHIN(he->len, 0, ho->num_sizes - 1));

                pdist    = squaredist;
                wantdist = ho->size[he->len];
                ddist    = pdist - wantdist;
                squash   = ddist * ho->elasticity * ho->oversize[he->len];

                fx = dpx * squash;
                fy = dpy * squash;
                fz = dpz * squash;

                hp1->dx += fx;
                hp1->dy += fy;
                hp1->dz += fz;

                hp2->dx -= fx;
                hp2->dy -= fy;
                hp2->dz -= fz;
            } else {
                MSG_add("Points too close together.");
            }
        }

        // Step 2: Process all active HM-vs-HM bump forces.
        for (hb = ho->bump; hb; hb = hb->next) {
            HM_process_bump(ho, hb);
        }

        // Step 3: Integrate all points.
        // Damping first, then gravity, then ground/wall collision, then position update.
        // Ground: hardcoded gy=0 — HM objects bounce on flat y=0, not actual terrain.
        // Wall: 2-frame velocity look-ahead (hp->dx + hp->dx) prevents tunnelling.
        for (j = 1; j < ho->num_points; j++) {
            hp = &ho->point[j];

            hp->dx *= ho->damping;
            hp->dy *= ho->damping;
            hp->dz *= ho->damping;

            hp->dy += HM_GRAVITY * hp->mass;

            {
                gy = 0;

                if (hp->y < gy) {
                    hp->dy = -hp->dy * ho->bounciness;
                    hp->dx *= ho->friction;
                    hp->dz *= ho->friction;
                    hp->y = gy - hp->y;
                }
            }

            for (k = 0; k < ho->num_cols; k++) {
                hc = &ho->col[k];

                if (MATHS_seg_intersect(
                        SLONG(hc->x1), SLONG(hc->z1),
                        SLONG(hc->x2), SLONG(hc->z2),
                        SLONG(hp->x),
                        SLONG(hp->z),
                        SLONG(hp->x + hp->dx + hp->dx),
                        SLONG(hp->z + hp->dz + hp->dz))) {
                    if (hc->x1 == hc->x2) {
                        hp->dx = -hp->dx;
                    } else if (hc->z1 == hc->z2) {
                        hp->dz = -hp->dz;
                    } else {
                        hp->dx = -hp->dx;
                        hp->dz = -hp->dz;
                    }

                    MSG_add("Point collided.");

                    goto dont_move_this_point;
                }
            }

            hp->x += hp->dx;
            hp->y += hp->dy;
            hp->z += hp->dz;

        dont_move_this_point:;
        }

        // Step 4: Remove expired bump records.
        prev = &ho->bump;
        hb   = ho->bump;

        while (hb) {
            if (HM_bump_dead(ho, hb)) {
                dead = hb;

                *prev = hb->next;
                hb    = hb->next;

                MemFree(dead);
            } else {
                prev = &hb->next;
                hb   = hb->next;
            }
        }
    }
}

// uc_orig: HM_draw (fallen/Source/hm.cpp)
// Debug visualiser: draws the spring lattice as coloured 3D lines.
// Also draws the actual mesh wireframe when Keys[KB_5] is held.
// The half-velocity offset (hp->x - hp->dx * 0.5) gives a motion-blur-like effect.
void HM_draw(void)
{
    SLONG i, j, k;
    SLONG x, y, z;
    SLONG x1, y1, z1, x2, y2, z2;
    SLONG index, nindex;
    SLONG r, g, b;

    float px[4], py[4], pz[4];

    HM_Object* ho;
    HM_Point* hp;
    HM_Point* np;

    PrimObject* po;
    PrimFace4* f4;

    for (i = 0; i < HM_MAX_OBJECTS; i++) {
        ho = &HM_object[i];

        if (!ho->used) {
            continue;
        }

        if (Keys[KB_5]) {
            // Draw the actual prim mesh wireframe.
            ASSERT(WITHIN(ho->prim, 1, next_prim_object - 1));

            po = &prim_objects[ho->prim];

            for (j = po->StartFace4; j < po->EndFace4; j++) {
                f4 = &prim_faces4[j];

                for (k = 0; k < 4; k++) {
                    HM_find_mesh_point(i, f4->Points[k] - po->StartPoint, &px[k], &py[k], &pz[k]);
                }

                e_draw_3d_line_col_sorted((SLONG)px[0], (SLONG)py[0], (SLONG)pz[0], (SLONG)px[1], (SLONG)py[1], (SLONG)pz[1], 255, 255, 255);
                e_draw_3d_line_col_sorted((SLONG)px[1], (SLONG)py[1], (SLONG)pz[1], (SLONG)px[3], (SLONG)py[3], (SLONG)pz[3], 255, 255, 255);
                e_draw_3d_line_col_sorted((SLONG)px[3], (SLONG)py[3], (SLONG)pz[3], (SLONG)px[2], (SLONG)py[2], (SLONG)pz[2], 255, 255, 255);
                e_draw_3d_line_col_sorted((SLONG)px[2], (SLONG)py[2], (SLONG)pz[2], (SLONG)px[0], (SLONG)py[0], (SLONG)pz[0], 255, 255, 255);
            }
        }

        index = 0;

        for (z = 0; z < ho->z_res; z++) {
            for (y = 0; y < ho->y_res; y++) {
                for (x = 0; x < ho->x_res; x++) {
                    if (ho->index[index]) {
                        ASSERT(WITHIN(ho->index[index], 1, ho->num_points - 1));

                        hp = &ho->point[ho->index[index]];

                        x1 = SLONG(hp->x - hp->dx * 0.5F);
                        y1 = SLONG(hp->y - hp->dy * 0.5F);
                        z1 = SLONG(hp->z - hp->dz * 0.5F);

                        if (z && (x == 0 || x == ho->x_res - 1 || y == 0 || y == ho->y_res - 1)) {
                            r = 32; g = 32; b = 32;

                            if (x == 0 || x == ho->x_res - 1) {
                                r = 244 - x * 24;
                            } else if (y == 0 || y == ho->y_res - 1) {
                                g = 244 - y * 24;
                            }

                            nindex = index - ho->x_res_times_y_res;
                            ASSERT(WITHIN(nindex, 0, ho->num_indices - 1));

                            if (ho->index[nindex]) {
                                ASSERT(WITHIN(ho->index[nindex], 1, ho->num_points - 1));
                                np = &ho->point[ho->index[nindex]];
                                x2 = SLONG(np->x - np->dx * 0.5F);
                                y2 = SLONG(np->y - np->dy * 0.5F);
                                z2 = SLONG(np->z - np->dz * 0.5F);
                                e_draw_3d_line_col_sorted(x1, y1, z1, x2, y2, z2, r, g, b);
                            }
                        }

                        if (y && (x == 0 || x == ho->x_res - 1 || z == 0 || z == ho->y_res - 1)) {
                            r = 32; g = 32; b = 32;

                            if (x == 0 || x == ho->x_res - 1) {
                                r = 244 - x * 24;
                            } else if (z == 0 || z == ho->z_res - 1) {
                                b = 244 - z * 24;
                            }

                            nindex = index - ho->x_res;
                            ASSERT(WITHIN(nindex, 0, ho->num_indices - 1));

                            if (ho->index[nindex]) {
                                ASSERT(WITHIN(ho->index[nindex], 1, ho->num_points - 1));
                                np = &ho->point[ho->index[nindex]];
                                x2 = SLONG(np->x - np->dx * 0.5F);
                                y2 = SLONG(np->y - np->dy * 0.5F);
                                z2 = SLONG(np->z - np->dz * 0.5F);
                                e_draw_3d_line_col_sorted(x1, y1, z1, x2, y2, z2, r, g, b);
                            }
                        }

                        if (x && (y == 0 || y == ho->x_res - 1 || z == 0 || z == ho->y_res - 1)) {
                            r = 32; g = 32; b = 32;

                            if (y == 0 || y == ho->y_res - 1) {
                                g = 244 - y * 24;
                            } else if (z == 0 || z == ho->z_res - 1) {
                                b = 244 - z * 24;
                            }

                            nindex = index - 1;
                            ASSERT(WITHIN(nindex, 0, ho->num_indices - 1));

                            if (ho->index[nindex]) {
                                ASSERT(WITHIN(ho->index[nindex], 1, ho->num_points - 1));
                                np = &ho->point[ho->index[nindex]];
                                x2 = SLONG(np->x - np->dx * 0.5F);
                                y2 = SLONG(np->y - np->dy * 0.5F);
                                z2 = SLONG(np->z - np->dz * 0.5F);
                                e_draw_3d_line_col_sorted(x1, y1, z1, x2, y2, z2, r, g, b);
                            }
                        }
                    }

                    index++;
                }
            }
        }
    }
}

// uc_orig: HM_find_mesh_pos (fallen/Source/hm.cpp)
// Extracts world position and Euler angles from the deformed spring lattice.
// Builds a 3x3 matrix from the 4 reference points chosen at HM_create() time,
// normalises it, converts to Euler angles, then back-solves for world origin.
void HM_find_mesh_pos(
    UBYTE hm_index,
    SLONG* x, SLONG* y, SLONG* z,
    SLONG* yaw, SLONG* pitch, SLONG* roll)
{
    float len, overlen;
    float matrix[9];
    float ans_x, ans_y, ans_z;

    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];
    HM_Point* hp_o;
    HM_Point* hp_x;
    HM_Point* hp_y;
    HM_Point* hp_z;

    ASSERT(WITHIN(ho->o_index, 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->x_index, 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->y_index, 1, ho->num_points - 1));
    ASSERT(WITHIN(ho->z_index, 1, ho->num_points - 1));

    hp_o = &ho->point[ho->o_index];
    hp_x = &ho->point[ho->x_index];
    hp_y = &ho->point[ho->y_index];
    hp_z = &ho->point[ho->z_index];

    matrix[0] = hp_x->x - hp_o->x;
    matrix[1] = hp_x->y - hp_o->y;
    matrix[2] = hp_x->z - hp_o->z;
    matrix[3] = hp_y->x - hp_o->x;
    matrix[4] = hp_y->y - hp_o->y;
    matrix[5] = hp_y->z - hp_o->z;
    matrix[6] = hp_z->x - hp_o->x;
    matrix[7] = hp_z->y - hp_o->y;
    matrix[8] = hp_z->z - hp_o->z;

    // Orthonormalise each row (assumes near-rigid deformation).
    len = matrix[0] * matrix[0] + matrix[1] * matrix[1] + matrix[2] * matrix[2];
    len = sqrt(len);
    overlen = 1.0F / len;
    matrix[0] *= overlen; matrix[1] *= overlen; matrix[2] *= overlen;

    len = matrix[3] * matrix[3] + matrix[4] * matrix[4] + matrix[5] * matrix[5];
    len = sqrt(len);
    overlen = 1.0F / len;
    matrix[3] *= overlen; matrix[4] *= overlen; matrix[5] *= overlen;

    len = matrix[6] * matrix[6] + matrix[7] * matrix[7] + matrix[8] * matrix[8];
    len = sqrt(len);
    overlen = 1.0F / len;
    matrix[6] *= overlen; matrix[7] *= overlen; matrix[8] *= overlen;

    Direction rot = MATRIX_find_angles(matrix);

    rot.yaw   *= 2048.0F / (2.0F * PI);
    rot.pitch *= 2048.0F / (2.0F * PI);
    rot.roll  *= 2048.0F / (2.0F * PI);

    *yaw   = SLONG(rot.yaw);
    *pitch = SLONG(rot.pitch);
    *roll  = SLONG(rot.roll);

    // Back-solve: unrotate the prim origin to find world position.
    ans_x = ho->o_prim_x;
    ans_y = ho->o_prim_y;
    ans_z = ho->o_prim_z;

    ans_x -= ho->cog_x;
    ans_y -= ho->cog_y;
    ans_z -= ho->cog_z;

    MATRIX_MUL_BY_TRANSPOSE(matrix, ans_x, ans_y, ans_z);

    ans_x += ho->cog_x;
    ans_y += ho->cog_y;
    ans_z += ho->cog_z;

    *x = SLONG(hp_o->x - ans_x);
    *y = SLONG(hp_o->y - ans_y);
    *z = SLONG(hp_o->z - ans_z);
}

// uc_orig: HM_stationary (fallen/Source/hm.cpp)
// Returns UC_TRUE if average |dx|+|dy|+|dz| < 0.25 (object has come to rest).
SLONG HM_stationary(UBYTE hm_index)
{
    SLONG i;
    float dx = 0, dy = 0, dz = 0;

    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];

    for (i = 1; i < ho->num_points; i++) {
        dx += fabs(ho->point[i].dx);
        dy += fabs(ho->point[i].dy);
        dz += fabs(ho->point[i].dz);
    }

    dx /= ho->num_points;
    dy /= ho->num_points;
    dz /= ho->num_points;

    if (dx + dy + dz < 0.25F) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// uc_orig: HM_shockwave (fallen/Source/hm.cpp)
// Applies a radial explosion impulse to all lattice points within range.
// Force falls off as: push = (range-dist)/range * force/dist.
// Y component is doubled for more dramatic upward blast.
void HM_shockwave(UBYTE hm_index, float x, float y, float z, float range, float force)
{
    ASSERT(WITHIN(hm_index, 0, HM_MAX_OBJECTS - 1));

    HM_Object* ho = &HM_object[hm_index];

    SLONG i;
    float dx, dy, dz, dist, push;

    for (i = 1; i < ho->num_points; i++) {
        dx = ho->point[i].x - x;
        dy = ho->point[i].y - y;
        dz = ho->point[i].z - z;

        dist = sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < range) {
            push = ((range - dist) / range) * force / dist;

            ho->point[i].dx += dx * push;
            ho->point[i].dy += dy * push * 2.0F;
            ho->point[i].dz += dz * push;
        }
    }
}
