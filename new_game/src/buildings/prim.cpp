// Prim (static world mesh object) system.
// Prims are shared 3D mesh definitions instanced at world positions.
// Examples: lamppost, litter bin, bollard, tree, fence panel.
// This file handles: pool management, lighting, collision, normals,
// slide edges, animated prims, and misc utilities.

// MFStdLib first: pulls in windows.h with correct defines (WIN32, _WIN32) before any other headers.
#include "engine/platform/uc_common.h"

#include "buildings/prim.h"
#include "buildings/prim_globals.h"

#include "engine/core/fmatrix.h"               // FMATRIX_calc, FMATRIX_MUL_BY_TRANSPOSE, build_tween_matrix
#include "engine/core/matrix.h"                // MATRIX_MUL
#include "engine/core/math.h"                  // Root
#include "engine/core/macros.h"                // ASSERT, WITHIN, SATURATE, SWAP, INFINITY, QDIST2, Random
#include "engine/core/memory.h"                // MSG_add
#include "map/pap.h"              // PAP_calc_height_at, PAP_2LO, PAP_SIZE_LO, PAP_SIZE_HI, PAP_SHIFT_LO
#include "navigation/walkable.h"  // find_height_for_this_pos
#include "engine/physics/collide.h"     // slide_around_box
#include "things/core/thing.h"
#include "things/vehicles/vehicle.h"
#include "things/characters/person.h"

// Anim types needed for expand_anim_prim_bbox / fn_anim_prim_normal
#include "engine/animation/anim_types.h"  // GameKeyFrameElement, GetCMatrix, CMatrix33, Matrix33, KeyFrameChunk
#include "buildings/prim_types.h"     // PrimObject, PrimFace3/4, PrimPoint, PrimMultiObject, RoofFace4, RFACE_FLAG_*, ROOF_SHIFT, PrimInfo, FACE_FLAG_*, PRIM_COLLIDE_*, PRIM_FLAG_*, PRIM_OBJ_*, PRIM_DAMAGE_*, ANIM_PRIM_TYPE_*
#include "buildings/building_types.h" // BoundBox, MAX_BUILDINGS, STOREY_TYPE_*, FACET_FLAG_*
#include "map/supermap.h"
#include "game/game_types.h"
#include "buildings/building_globals.h" // end_prim_point, end_prim_face4, end_prim_face3, end_prim_object, end_prim_multi_object, building_list, building_facets
#include "assets/formats/anim_globals.h"
#include "engine/graphics/pipeline/aeng.h"
#include "map/level_pools.h"

// Forward declaration for advance_keyframe (defined in person.cpp / old Person.cpp).
// uc_orig: advance_keyframe (fallen/Source/Person.cpp)
SLONG advance_keyframe(DrawTween* draw_info);

// ---- Bit flags used only within this file ----

#define USED_POINT (1 << 0)
#define USED_FACE3 (1 << 1)
#define USED_FACE4 (1 << 2)

// Minimum drop height for a walkable edge to be considered a grab edge.
// If the ground below is within this distance, the edge is not climbable.
#define SLIDE_EDGE_HEIGHT 0x90

// Y-shrink constants for slide_along_prim with the 'shrink' flag set.
#define FURN_UNDERNEATH_BOT 96
#define FURN_UNDERNEATH_TOP 0

// How far beyond a walkable edge to sample ground height (in world units).
#define SLIDE_EDGE_PROBE 35

// Extra search margin when finding walkable face neighbours (in world units).
#define JUST_IN_CASE 8

// ============================================================
// Pool management
// ============================================================

// uc_orig: delete_prim_points_block (fallen/Source/Prim.cpp)
void delete_prim_points_block(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = start + count; c0 < next_prim_point; c0++)
        prim_points[c0 - count] = prim_points[c0];
    next_prim_point -= count;
}

// uc_orig: delete_prim_faces3_block (fallen/Source/Prim.cpp)
void delete_prim_faces3_block(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = start + count; c0 < next_prim_face3; c0++)
        prim_faces3[c0 - count] = prim_faces3[c0];
    next_prim_face3 -= count;
}

// uc_orig: delete_prim_faces4_block (fallen/Source/Prim.cpp)
void delete_prim_faces4_block(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = start + count; c0 < next_prim_face4; c0++)
        prim_faces4[c0 - count] = prim_faces4[c0];
    next_prim_face4 -= count;
}

// Patch prim_faces3/4 point indices after a block of points is removed.
// uc_orig: fix_faces_for_del_points (fallen/Source/Prim.cpp)
void fix_faces_for_del_points(SLONG start, SLONG count)
{
    SLONG c0, c1;
    for (c0 = 1; c0 < next_prim_face3; c0++)
        for (c1 = 0; c1 < 3; c1++)
            if (prim_faces3[c0].Points[c1] > start)
                prim_faces3[c0].Points[c1] -= count;
    for (c0 = 1; c0 < next_prim_face4; c0++)
        for (c1 = 0; c1 < 4; c1++)
            if (prim_faces4[c0].Points[c1] > start)
                prim_faces4[c0].Points[c1] -= count;
}

// uc_orig: fix_objects_for_del_points (fallen/Source/Prim.cpp)
void fix_objects_for_del_points(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = 1; c0 < next_prim_object; c0++) {
        if (prim_objects[c0].StartPoint > start) {
            prim_objects[c0].StartPoint -= count;
            prim_objects[c0].EndPoint -= count;
        }
    }
}

// uc_orig: fix_objects_for_del_faces3 (fallen/Source/Prim.cpp)
void fix_objects_for_del_faces3(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = 1; c0 < next_prim_object; c0++) {
        if (prim_objects[c0].StartFace3 > start) {
            prim_objects[c0].StartFace3 -= count;
            prim_objects[c0].EndFace3 -= count;
        }
    }
}

// uc_orig: fix_objects_for_del_faces4 (fallen/Source/Prim.cpp)
void fix_objects_for_del_faces4(SLONG start, SLONG count)
{
    SLONG c0;
    for (c0 = 1; c0 < next_prim_object; c0++) {
        if (prim_objects[c0].StartFace4 > start) {
            prim_objects[c0].StartFace4 -= count;
            prim_objects[c0].EndFace4 -= count;
        }
    }
}

// Editor GC: mark every referenced slot, then sweep from high to low
// deleting contiguous runs of unreferenced slots.
// global_bright[] is repurposed as a byte-wide bitmask scratch buffer.
// uc_orig: compress_prims (fallen/Source/Prim.cpp)
void compress_prims(void)
{
    SLONG c0, c1;
    UBYTE* pf;
    SLONG count;

    struct PrimObject* p_obj;

    pf = (UBYTE*)&global_bright[0];
    memset(pf, 0, 15560 * 2);

    for (c0 = 1; c0 < next_prim_object; c0++) {
        p_obj = &prim_objects[c0];
        for (c1 = p_obj->StartPoint; c1 < p_obj->EndPoint; c1++)
            pf[c1] |= USED_POINT;
        for (c1 = p_obj->StartFace3; c1 < p_obj->EndFace3; c1++)
            pf[c1] |= USED_FACE3;
        for (c1 = p_obj->StartFace4; c1 < p_obj->EndFace4; c1++)
            pf[c1] |= USED_FACE4;
    }

    for (c0 = next_prim_point - 1; c0 > 0; c0--) {
        if ((pf[c0] & USED_POINT) == 0) {
            count = 1;
            for (c1 = c0 - 1; c1 > 0; c1--) {
                if ((pf[c1] & USED_POINT) == 0)
                    count++;
                else
                    break;
            }
            c0 = c1;
            delete_prim_points_block(c1 + 1, count);
            fix_faces_for_del_points(c1 + 1, count);
            fix_objects_for_del_points(c1 + 1, count);
        }
    }

    for (c0 = next_prim_face3 - 1; c0 > 0; c0--) {
        if ((pf[c0] & USED_FACE3) == 0) {
            count = 1;
            for (c1 = c0 - 1; c1 > 0; c1--) {
                if ((pf[c1] & USED_FACE3) == 0)
                    count++;
                else
                    break;
            }
            c0 = c1;
            delete_prim_faces3_block(c1 + 1, count);
            fix_objects_for_del_faces3(c1 + 1, count);
        }
    }

    for (c0 = next_prim_face4 - 1; c0 > 0; c0--) {
        if ((pf[c0] & USED_FACE4) == 0) {
            count = 1;
            for (c1 = c0 - 1; c1 > 0; c1--) {
                if ((pf[c1] & USED_FACE4) == 0)
                    count++;
                else
                    break;
            }
            c0 = c1;
            delete_prim_faces4_block(c1 + 1, count);
            fix_objects_for_del_faces4(c1 + 1, count);
        }
    }
}

// Reset the prim database for a new level load.
// Pool cursors start at 1 (slot 0 = invalid sentinel).
// prim_object cursor starts at 266 to preserve static world prim slots 1-265.
// uc_orig: clear_prims (fallen/Source/Prim.cpp)
void clear_prims(void)
{
    SLONG c0;

    memset((UBYTE*)&prim_objects[0], 0, sizeof(PrimObject) * MAX_PRIM_OBJECTS);
    memset((UBYTE*)&prim_multi_objects[0], 0, sizeof(PrimMultiObject) * MAX_PRIM_MOBJECTS);

    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++)
        anim_chunk[c0].MultiObject[0] = 0;

    next_prim_point = 1;
    next_prim_face4 = 1;
    next_prim_face3 = 1;
    next_prim_object = 266;
    next_prim_multi_object = 1;
}

// ============================================================
// Smooth lighting
// ============================================================

// Accumulate per-vertex brightness for all face occurrences of shared_point.
// uc_orig: sum_shared_brightness_prim (fallen/Source/Prim.cpp)
SLONG sum_shared_brightness_prim(SWORD shared_point, struct PrimObject* p_obj)
{
    SLONG point;
    SLONG face;
    SLONG bright = 0;
    SLONG count = 0;

    for (face = p_obj->StartFace3; face < p_obj->EndFace3; face++)
        for (point = 0; point < 3; point++)
            if (prim_faces3[face].Points[point] == shared_point) {
                bright += prim_faces3[face].Bright[point];
                count++;
            }

    for (face = p_obj->StartFace4; face < p_obj->EndFace4; face++)
        for (point = 0; point < 4; point++)
            if (prim_faces4[face].Points[point] == shared_point) {
                bright += prim_faces4[face].Bright[point];
                count++;
            }

    if (count)
        return (bright / count);
    else
        return (0);
}

// Write an averaged brightness value back to every face occurrence of shared_point.
// uc_orig: set_shared_brightness_prim (fallen/Source/Prim.cpp)
void set_shared_brightness_prim(SWORD shared_point, SWORD bright, struct PrimObject* p_obj)
{
    SLONG point;
    SLONG face;

    for (face = p_obj->StartFace3; face < p_obj->EndFace3; face++)
        for (point = 0; point < 3; point++)
            if (prim_faces3[face].Points[point] == shared_point)
                prim_faces3[face].Bright[point] = bright;

    for (face = p_obj->StartFace4; face < p_obj->EndFace4; face++)
        for (point = 0; point < 4; point++)
            if (prim_faces4[face].Points[point] == shared_point)
                prim_faces4[face].Bright[point] = bright;
}

// Average per-vertex brightness across all shared vertices, and set FACE_FLAG_SMOOTH.
// Called once after loading/editing — not per-frame.
// uc_orig: smooth_a_prim (fallen/Source/Prim.cpp)
void smooth_a_prim(SLONG prim)
{
    SLONG face;
    SLONG bright;
    struct PrimObject* p_obj;
    SLONG point;

    p_obj = &prim_objects[prim];

    for (face = p_obj->StartFace4; face < p_obj->EndFace4; face++) {
        prim_faces4[face].FaceFlags |= FACE_FLAG_SMOOTH;
        for (point = 0; point < 4; point++) {
            bright = sum_shared_brightness_prim(prim_faces4[face].Points[point], p_obj);
            set_shared_brightness_prim(prim_faces4[face].Points[point], bright, p_obj);
        }
    }

    for (face = p_obj->StartFace3; face < p_obj->EndFace3; face++) {
        prim_faces3[face].FaceFlags |= FACE_FLAG_SMOOTH;
        for (point = 0; point < 3; point++) {
            bright = sum_shared_brightness_prim(prim_faces3[face].Points[point], p_obj);
            set_shared_brightness_prim(prim_faces3[face].Points[point], bright, p_obj);
        }
    }
}

// ============================================================
// Editor helpers
// ============================================================

// Duplicate prim mesh data to the descending end of the pool arrays.
// Points are written in reverse order; end_prim_* cursors grow downward from the top.
// ThingIndex is set on each copied face to tag which editor Thing owns it.
// uc_orig: copy_prim_to_end (fallen/Source/Prim.cpp)
SLONG copy_prim_to_end(UWORD prim, UWORD direct, SWORD thing)
{
    SLONG c0;
    struct PrimObject* p_obj;
    SLONG sp, ep;

    p_obj = &prim_objects[prim];
    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    for (c0 = ep - 1; c0 >= sp; c0--) {
        prim_points[end_prim_point] = prim_points[c0];
        end_prim_point--;
    }
    for (c0 = p_obj->EndFace4 - 1; c0 >= p_obj->StartFace4; c0--) {
        prim_faces4[end_prim_face4] = prim_faces4[c0];
        prim_faces4[end_prim_face4].Points[0] += -sp + end_prim_point + 1;
        prim_faces4[end_prim_face4].Points[1] += -sp + end_prim_point + 1;
        prim_faces4[end_prim_face4].Points[2] += -sp + end_prim_point + 1;
        prim_faces4[end_prim_face4].Points[3] += -sp + end_prim_point + 1;
        prim_faces4[end_prim_face4].ThingIndex = thing;
        prim_faces4[end_prim_face4].Bright[0] = 0;
        prim_faces4[end_prim_face4].Bright[1] = 0;
        prim_faces4[end_prim_face4].Bright[2] = 0;
        prim_faces4[end_prim_face4].Bright[3] = 0;
        end_prim_face4--;
    }
    for (c0 = p_obj->EndFace3 - 1; c0 >= p_obj->StartFace3; c0--) {
        prim_faces3[end_prim_face3] = prim_faces3[c0];
        prim_faces3[end_prim_face3].Points[0] += -sp + end_prim_point + 1;
        prim_faces3[end_prim_face3].Points[1] += -sp + end_prim_point + 1;
        prim_faces3[end_prim_face3].Points[2] += -sp + end_prim_point + 1;
        prim_faces3[end_prim_face3].ThingIndex = thing;
        prim_faces3[end_prim_face3].Bright[0] = 0;
        prim_faces3[end_prim_face3].Bright[1] = 0;
        prim_faces3[end_prim_face3].Bright[2] = 0;
        end_prim_face3--;
    }
    prim_objects[end_prim_object].StartPoint = end_prim_point + 1;
    prim_objects[end_prim_object].EndPoint = end_prim_point + 1 + p_obj->EndPoint - p_obj->StartPoint;
    prim_objects[end_prim_object].StartFace3 = end_prim_face3 + 1;
    prim_objects[end_prim_object].EndFace3 = end_prim_face3 + 1 + p_obj->EndFace3 - p_obj->StartFace3;
    prim_objects[end_prim_object].StartFace4 = end_prim_face4 + 1;
    prim_objects[end_prim_object].EndFace4 = end_prim_face4 + 1 + p_obj->EndFace4 - p_obj->StartFace4;
    --end_prim_object;
    return (end_prim_object + 1);
}

// uc_orig: delete_prim_points (fallen/Source/Prim.cpp)
void delete_prim_points(SLONG start, SLONG end)
{
    SLONG c0, offset;
    offset = end - start;

    for (c0 = end; c0 < next_prim_point; c0++)
        prim_points[c0 - offset] = prim_points[c0];

    for (c0 = 1; c0 < next_prim_face3; c0++) {
        if (prim_faces3[c0].Points[0] >= start) prim_faces3[c0].Points[0] -= offset;
        if (prim_faces3[c0].Points[1] >= start) prim_faces3[c0].Points[1] -= offset;
        if (prim_faces3[c0].Points[2] >= start) prim_faces3[c0].Points[2] -= offset;
    }
    for (c0 = 1; c0 < next_prim_face4; c0++) {
        if (prim_faces4[c0].Points[0] >= start) prim_faces4[c0].Points[0] -= offset;
        if (prim_faces4[c0].Points[1] >= start) prim_faces4[c0].Points[1] -= offset;
        if (prim_faces4[c0].Points[2] >= start) prim_faces4[c0].Points[2] -= offset;
        if (prim_faces4[c0].Points[3] >= start) prim_faces4[c0].Points[3] -= offset;
    }
}

// uc_orig: delete_prim_faces3 (fallen/Source/Prim.cpp)
void delete_prim_faces3(SLONG start, SLONG end)
{
    SLONG c0, offset;
    offset = end - start;

    for (c0 = end; c0 < next_prim_point; c0++)
        prim_faces3[c0 - offset] = prim_faces3[c0];

    for (c0 = 1; c0 < next_prim_object; c0++) {
        if (prim_objects[c0].StartFace3 >= start) prim_objects[c0].StartFace3 -= offset;
        if (prim_objects[c0].EndFace3 >= start) prim_objects[c0].EndFace3 -= offset;
    }
}

// uc_orig: delete_prim_faces4 (fallen/Source/Prim.cpp)
void delete_prim_faces4(SLONG start, SLONG end)
{
    SLONG c0, offset;
    offset = end - start;

    for (c0 = end; c0 < next_prim_point; c0++)
        prim_faces3[c0 - offset] = prim_faces3[c0];

    for (c0 = 1; c0 < next_prim_object; c0++) {
        if (prim_objects[c0].StartFace4 >= start) prim_objects[c0].StartFace4 -= offset;
        if (prim_objects[c0].EndFace4 >= start) prim_objects[c0].EndFace4 -= offset;
    }
}

// uc_orig: delete_prim_objects (fallen/Source/Prim.cpp)
void delete_prim_objects(SLONG start, SLONG end)
{
    SLONG c0, offset;
    offset = end - start;
    for (c0 = end; c0 < next_prim_point; c0++)
        prim_objects[c0 - offset] = prim_objects[c0];
}

// uc_orig: delete_last_prim (fallen/Source/Prim.cpp)
void delete_last_prim(void)
{
    next_prim_point = prim_objects[next_prim_object - 1].StartPoint;
    next_prim_face3 = prim_objects[next_prim_object - 1].StartFace3;
    next_prim_face4 = prim_objects[next_prim_object - 1].StartFace4;
    next_prim_object--;
}

// uc_orig: delete_a_prim (fallen/Source/Prim.cpp)
void delete_a_prim(UWORD prim)
{
    SLONG c0;

    MSG_add("ERROR: c0 is undefined! Should it be prim?");

    c0 = prim;

    delete_prim_points(prim_objects[c0].StartPoint, prim_objects[c0].EndPoint);
    delete_prim_faces3(prim_objects[c0].StartFace3, prim_objects[c0].EndFace3);
    delete_prim_faces4(prim_objects[c0].StartFace4, prim_objects[c0].EndFace4);
    delete_prim_objects(prim, prim + 1);
}

// ============================================================
// Normal calculation
// ============================================================

// Compute the face normal for face index:
//   face < 0  → triangle (-face = prim_faces3[] index)
//   face >= 0 → quad (prim_faces4[] index)
// Uses points 0,1,3 for quads (point 2 is skipped).
// Output normalised to length 256. Both edges must be non-degenerate;
// if either is zero-length the output is (0, 255, 0) as a safe fallback.
// uc_orig: calc_normal (fallen/Source/Prim.cpp)
void calc_normal(SWORD face, struct SVector* p_normal)
{
    SLONG vx, vy, vz, wx, wy, wz;
    struct PrimFace3* this_face3;
    struct PrimFace4* this_face4;
    SLONG nx, ny, nz;
    SLONG length;
    struct PrimPoint *p_op0, *p_op1, *p_op2;

    if (face < 0) {
        this_face3 = &prim_faces3[-face];
        p_op0 = &prim_points[this_face3->Points[0]];
        p_op1 = &prim_points[this_face3->Points[1]];
        p_op2 = &prim_points[this_face3->Points[2]];
    } else {
        this_face4 = &prim_faces4[face];
        p_op0 = &prim_points[this_face4->Points[0]];
        p_op1 = &prim_points[this_face4->Points[1]];
        p_op2 = &prim_points[this_face4->Points[3]];
    }

    vx = -p_op0->X + p_op1->X;
    vy = -p_op0->Y + p_op1->Y;
    vz = -p_op0->Z + p_op1->Z;

    wx = p_op2->X - p_op1->X;
    wy = p_op2->Y - p_op1->Y;
    wz = p_op2->Z - p_op1->Z;

    if ((vx == 0 && vy == 0 && vz == 0) || (wx == 0 && wy == 0 && wz == 0)) {
        p_normal->X = 0;
        p_normal->Y = 255;
        p_normal->Z = 0;
        return;
    }

    length = vx * vx + vy * vy + vz * vz;
    length = Root(length);
    if (length == 0) length = 1;
    vx = (vx << 8) / length;
    vy = (vy << 8) / length;
    vz = (vz << 8) / length;

    length = Root(wx * wx + wy * wy + wz * wz);
    if (length == 0) length = 1;
    wx = (wx << 8) / length;
    wy = (wy << 8) / length;
    wz = (wz << 8) / length;

    nx = ((vy) * (wz)) - (vz * wy) >> 8;
    ny = ((vz) * (wx)) - (vx * wz) >> 8;
    nz = ((vx) * (wy)) - (vy * wx) >> 8;

    length = Root((nx * nx + ny * ny + nz * nz));
    if (length == 0) length = 1;
    nx = (nx << 8) / length;
    ny = (ny << 8) / length;
    nz = (nz << 8) / length;
    if (nx == 0 && ny == 0 && nz == 0) ny = 255;
    p_normal->X = -nx;
    p_normal->Y = -ny;
    p_normal->Z = -nz;
}

// Unnormalised cross product of face edges — sign test only, no length computation.
// Leaves 'length' uninitialised when face == -9823 (original quirk, preserved 1:1).
// uc_orig: quick_normal (fallen/Source/Prim.cpp)
void quick_normal(SWORD face, SLONG* nx, SLONG* ny, SLONG* nz)
{
    SLONG vx, vy, vz, wx, wy, wz;
    struct PrimFace3* this_face3;
    struct PrimFace4* this_face4;
    struct PrimPoint *p_op0, *p_op1, *p_op2;

    if (face < 0) {
        this_face3 = &prim_faces3[-face];
        p_op0 = &prim_points[this_face3->Points[0]];
        p_op1 = &prim_points[this_face3->Points[1]];
        p_op2 = &prim_points[this_face3->Points[2]];
    } else {
        this_face4 = &prim_faces4[face];
        p_op0 = &prim_points[this_face4->Points[0]];
        p_op1 = &prim_points[this_face4->Points[1]];
        p_op2 = &prim_points[this_face4->Points[3]];
    }

    vx = -p_op0->X + p_op1->X;
    vy = -p_op0->Y + p_op1->Y;
    vz = -p_op0->Z + p_op1->Z;

    wx = p_op2->X - p_op1->X;
    wy = p_op2->Y - p_op1->Y;
    wz = p_op2->Z - p_op1->Z;

    *nx = ((vy) * (wz)) - (vz * wy);
    *ny = ((vz) * (wx)) - (vx * wz);
    *nz = ((vx) * (wy)) - (vy * wx);
}

// ============================================================
// Lighting
// ============================================================

// Disabled macros for the shadow-tracing path (permanently compiled out in original).
#define shadow_calc 0
#define in_shadow(x, y, z, i, j, k) 0
#define in_shadowo(x, y, z, i, j, k) 0

// Bake a single directional light into face brightness values for a prim.
// lnx/lny/lnz: light direction normal (length 256 fixed-point).
// intense: maximum brightness [0..255]. Ambient floor = intense >> 3 (12.5%).
// Returns 0 (next value, always zero — the early_out labels are unused).
// uc_orig: apply_ambient_light_to_object (fallen/Source/Prim.cpp)
UWORD apply_ambient_light_to_object(UWORD object, SLONG lnx, SLONG lny, SLONG lnz, UWORD intense)
{
    struct PrimFace3* this_face;
    SLONG nx, ny, nz;
    UWORD no_faces;
    UWORD start_face, current_face;
    SLONG light;
    UWORD next = 0;
    UWORD no_faces4, start_face4;
    struct PrimFace4* this_face4;

    start_face4 = prim_objects[object].StartFace4;
    no_faces4 = prim_objects[object].EndFace4 - prim_objects[object].StartFace4;
    start_face = prim_objects[object].StartFace3;
    no_faces = prim_objects[object].EndFace3 - prim_objects[object].StartFace3;

    for (current_face = 0; current_face < no_faces; current_face++) {
        struct SVector normal;
        this_face = &prim_faces3[start_face + current_face];
        calc_normal(-(start_face + current_face), &normal);
        nx = normal.X; ny = normal.Y; nz = normal.Z;

        light = (nx * lnx + ny * lny + nz * lnz) >> 8;
        light = (light * intense) >> 8;
        if (light < intense >> 3) light = intense >> 3;

        if (light >= 0) {
            this_face->Bright[0] = light;
            this_face->Bright[1] = light;
            this_face->Bright[2] = light;
        }
    }

    for (current_face = 0; current_face < no_faces4; current_face++) {
        struct SVector normal;
        this_face4 = &prim_faces4[start_face4 + current_face];
        calc_normal((start_face4 + current_face), &normal);
        nx = normal.X; ny = normal.Y; nz = normal.Z;

        light = (nx * lnx + ny * lny + nz * lnz) >> 8;
        light = (light * intense) >> 8;
        if (light < intense >> 3) light = intense >> 3;

        if (light >= 0) {
            this_face4->Bright[0] = light;
            this_face4->Bright[1] = light;
            this_face4->Bright[2] = light;
            this_face4->Bright[3] = light;
        }
    }

    return next;
}

// ============================================================
// Bounding info
// ============================================================

// Post-load pass: compute per-prim runtime metadata.
// Y extent uses all points; X/Z extent uses only points at the bottom 1/8th
// of height (collision footprint = base of object, not widest point).
// Special cases: prim 41 (step), prims 181/182 (estate gate), PRIM_OBJ_WILDCATVAN_BODY.
// Sets PRIM_FLAG_ENVMAPPED and PRIM_DAMAGE_NOLOS flags.
// Calls VEH_init_vehinfo() to set up vehicle crumple zone data.
// uc_orig: calc_prim_info (fallen/Source/Prim.cpp)
void calc_prim_info()
{
    SLONG i, j, dist, below;
    PrimObject* obj;
    PrimInfo* inf;
    PrimPoint* pt;

    prim_objects[29].coltype = PRIM_COLLIDE_NONE;

    for (i = 1; i < 256; i++) {
        obj = &prim_objects[i];
        inf = &prim_info[i];

        if (obj->StartPoint == NULL)
            continue;

        inf->minx = +UC_INFINITY; inf->miny = +UC_INFINITY; inf->minz = +UC_INFINITY;
        inf->maxx = -UC_INFINITY; inf->maxy = -UC_INFINITY; inf->maxz = -UC_INFINITY;
        inf->radius = -UC_INFINITY;

        for (j = obj->StartPoint; j < obj->EndPoint; j++) {
            pt = &prim_points[j];
            if (pt->Y < inf->miny) inf->miny = pt->Y;
            if (pt->Y > inf->maxy) inf->maxy = pt->Y;
        }

        below = inf->miny + (inf->maxy - inf->miny >> 3);

        for (j = obj->StartPoint; j < obj->EndPoint; j++) {
            pt = &prim_points[j];

            if (i == 41) {
                if (pt->X < inf->minx) inf->minx = pt->X;
                if (pt->Z < inf->minz) inf->minz = pt->Z;
                if (pt->X > inf->maxx) inf->maxx = pt->X;
                if (pt->Z > inf->maxz) inf->maxz = pt->Z;
            } else {
                if (pt->Y < below) {
                    if (pt->X < inf->minx) inf->minx = pt->X;
                    if (pt->Z < inf->minz) inf->minz = pt->Z;
                    if (pt->X > inf->maxx) inf->maxx = pt->X;
                    if (pt->Z > inf->maxz) inf->maxz = pt->Z;
                }
            }

            dist = pt->X * pt->X + pt->Y * pt->Y + pt->Z * pt->Z;
            dist = Root(dist);
            if (dist > inf->radius) inf->radius = dist;
        }

        if (i == PRIM_OBJ_WILDCATVAN_BODY) {
            if (inf->minx > -64) {
                for (j = obj->StartPoint; j < obj->EndPoint; j++) {
                    pt = &prim_points[j];
                    pt->X -= 128;
                    pt->Z += 256;
                }
                inf->minx -= 128; inf->minz += 256;
                inf->maxx -= 128; inf->maxz += 256;
            }
        }

        dist = inf->maxx - inf->minx;
        if (dist < PRIM_MIN_BBOX) {
            inf->maxx += PRIM_MIN_BBOX - dist >> 1;
            inf->minx -= PRIM_MIN_BBOX - dist >> 1;
        }

        dist = inf->maxz - inf->minz;
        if (dist < PRIM_MIN_BBOX) {
            inf->maxz += PRIM_MIN_BBOX - dist >> 1;
            inf->minz -= PRIM_MIN_BBOX - dist >> 1;
        }

        if (i == 181 || i == 182) {
            inf->minx -= 0x40; inf->minz -= 0x40;
            inf->maxx += 0x40; inf->maxz += 0x40;
        }

        inf->cogx = 0; inf->cogy = 0; inf->cogz = 0;

        obj->flag &= ~PRIM_FLAG_ENVMAPPED;

        for (j = obj->StartFace3; j < obj->EndFace3; j++) {
            if (prim_faces3[j].FaceFlags & FACE_FLAG_ENVMAP) {
                obj->flag |= PRIM_FLAG_ENVMAPPED;
                goto set_the_envmapped_flag;
            }
        }
        for (j = obj->StartFace4; j < obj->EndFace4; j++) {
            if (prim_faces4[j].FaceFlags & FACE_FLAG_ENVMAP) {
                obj->flag |= PRIM_FLAG_ENVMAPPED;
                goto set_the_envmapped_flag;
            }
        }

    set_the_envmapped_flag:;

        obj->damage &= ~PRIM_DAMAGE_NOLOS;

        if (obj->coltype == PRIM_COLLIDE_BOX) {
            if (inf->maxy >= inf->miny + 0xc0) {
                if (inf->maxx >= inf->minx + 0x80 && inf->maxz >= inf->minz + 0x80) {
                    obj->damage |= PRIM_DAMAGE_NOLOS;
                }
            }
        }
    }

    VEH_init_vehinfo();
}

// Compute per-vertex normals for all loaded prims by averaging adjacent face normals.
// Stores result in prim_normal[] (parallel to prim_points[]).
// Normals are normalised to length 256 and negated (matching engine winding order).
// uc_orig: calc_prim_normals (fallen/Source/Prim.cpp)
void calc_prim_normals(void)
{
    SLONG i, j, k;
    SLONG dx, dy, dz, dist, num_points;
    SVector fnormal;
    SLONG p_index;

    PrimObject* p_obj;
    PrimFace3* p_f3;
    PrimFace4* p_f4;

    for (i = 1; i < next_prim_object; i++) {
        p_obj = &prim_objects[i];

        if (p_obj->StartPoint == NULL)
            continue;

        num_points = p_obj->EndPoint - p_obj->StartPoint;
        ASSERT(num_points <= MAX_POINTS_PER_PRIM);
        ASSERT(num_points >= 0);

        memset(each_point, 0, sizeof(UBYTE) * num_points);

        ASSERT((-p_obj->StartFace3 + p_obj->EndFace3) < 2000);
        for (j = p_obj->StartFace3; j < p_obj->EndFace3; j++) {
            p_f3 = &prim_faces3[j];
            calc_normal(-j, &fnormal);

            for (k = 0; k < 3; k++) {
                p_index = p_f3->Points[k] - p_obj->StartPoint;
                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_PRIM - 1));
                ASSERT(p_f3->Points[k] < MAX_PRIM_POINTS);

                if (each_point[p_index] == 0) {
                    prim_normal[p_f3->Points[k]] = fnormal;
                    each_point[p_index] = 1;
                } else {
                    prim_normal[p_f3->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z *= each_point[p_index];
                    prim_normal[p_f3->Points[k]].X += fnormal.X;
                    prim_normal[p_f3->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f3->Points[k]].Z += fnormal.Z;
                    each_point[p_index] += 1;
                    prim_normal[p_f3->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f3->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        ASSERT((-p_obj->StartFace4 + p_obj->EndFace4) < 2000);
        for (j = p_obj->StartFace4; j < p_obj->EndFace4; j++) {
            p_f4 = &prim_faces4[j];
            calc_normal(j, &fnormal);

            for (k = 0; k < 4; k++) {
                p_index = p_f4->Points[k] - p_obj->StartPoint;
                ASSERT(WITHIN(p_index, 0, MAX_POINTS_PER_PRIM - 1));
                ASSERT(p_f4->Points[k] < MAX_PRIM_POINTS);

                if (each_point[p_index] == 0) {
                    prim_normal[p_f4->Points[k]] = fnormal;
                    each_point[p_index] = 1;
                } else {
                    prim_normal[p_f4->Points[k]].X *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z *= each_point[p_index];
                    prim_normal[p_f4->Points[k]].X += fnormal.X;
                    prim_normal[p_f4->Points[k]].Y += fnormal.Y;
                    prim_normal[p_f4->Points[k]].Z += fnormal.Z;
                    each_point[p_index] += 1;
                    prim_normal[p_f4->Points[k]].X /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Y /= each_point[p_index];
                    prim_normal[p_f4->Points[k]].Z /= each_point[p_index];
                }
            }
        }

        // Normalise all vertex normals to length 256 and negate them.
        for (j = p_obj->StartPoint; j < p_obj->EndPoint; j++) {
            ASSERT(j < MAX_PRIM_POINTS);

            dx = abs(prim_normal[j].X);
            dy = abs(prim_normal[j].Y);
            dz = abs(prim_normal[j].Z);

            dist = dx * dx + dy * dy + dz * dz;
            dist = Root(dist);
            dist += 2;

            prim_normal[j].X <<= 8;
            prim_normal[j].Y <<= 8;
            prim_normal[j].Z <<= 8;

            prim_normal[j].X /= -dist;
            prim_normal[j].Y /= -dist;
            prim_normal[j].Z /= -dist;

            if ((prim_normal[j].X * prim_normal[j].X + prim_normal[j].Y * prim_normal[j].Y + prim_normal[j].Z * prim_normal[j].Z) > (65536 + 400)) {
                ASSERT(0);
            }
        }
    }
}

// uc_orig: get_prim_info (fallen/Source/Prim.cpp)
PrimInfo* get_prim_info(SLONG prim)
{
    ASSERT(WITHIN(prim, 1, 255));
    return &prim_info[prim];
}

// ============================================================
// Slide edges
// ============================================================

// Flag which edges of roof walkable faces (roof_faces4[]) are grabbable.
// Clears RFACE_FLAG_SLIDE_EDGE_* bits for edges blocked by fences
// or shared with adjacent roof faces that are close enough to step across.
// uc_orig: calc_slide_edges_roof (fallen/Source/Prim.cpp)
void calc_slide_edges_roof()
{
    SLONG c0;
    struct RoofFace4 *rf1, *rf2;
    SLONG dx, dz;

    for (c0 = 1; c0 < next_roof_face4; c0++)
        roof_faces4[c0].DrawFlags |= 0x78;

    rf1 = &roof_faces4[1];

    for (c0 = 1; c0 < next_roof_face4; c0++) {
        SLONG index;
        SLONG x, z;
        x = (rf1->RX & 127) << 8;
        z = (rf1->RZ & 127) << 8;

        if (does_fence_lie_along_line(x, z, x + 256, z))         rf1->DrawFlags &= ~(RFACE_FLAG_SLIDE_EDGE_0);
        if (does_fence_lie_along_line(x + 256, z, x + 256, z + 256)) rf1->DrawFlags &= ~(RFACE_FLAG_SLIDE_EDGE_1);
        if (does_fence_lie_along_line(x, z + 256, x + 256, z + 256)) rf1->DrawFlags &= ~(RFACE_FLAG_SLIDE_EDGE_2);
        if (does_fence_lie_along_line(x, z, x, z + 256))         rf1->DrawFlags &= ~(RFACE_FLAG_SLIDE_EDGE_3);

        {
            SLONG pap[4], roof[4];
            SLONG d1, d2, c1;
            SLONG mx, mz;
            mx = (rf1->RX & 127) << 8;
            mz = (rf1->RZ & 127) << 8;

            pap[0] = PAP_calc_height_at(mx, mz);
            pap[1] = PAP_calc_height_at(mx + 256, mz);
            pap[2] = PAP_calc_height_at(mx + 256, mz + 256);
            pap[3] = PAP_calc_height_at(mx, mz + 256);

            roof[0] = rf1->Y;
            roof[1] = rf1->Y + (rf1->DY[0] << ROOF_SHIFT);
            roof[2] = rf1->Y + (rf1->DY[1] << ROOF_SHIFT);
            roof[3] = rf1->Y + (rf1->DY[2] << ROOF_SHIFT);

            for (c1 = 0; c1 < 4; c1++) {
                d1 = roof[c1] - pap[c1];
                d2 = roof[(c1 + 1) & 3] - pap[(c1 + 1) & 3];
                if (d1 < SLIDE_EDGE_HEIGHT && d2 <= SLIDE_EDGE_HEIGHT)
                    rf1->DrawFlags &= ~(RFACE_FLAG_SLIDE_EDGE << c1);
            }
        }

        for (dx = -1; dx <= 1; dx++)
            for (dz = -1; dz <= 1; dz++) {
                SLONG mx, mz;
                mx = ((rf1->RX & 127) >> 2) + dx;
                mz = ((rf1->RZ & 127) >> 2) + dz;

                if (mx >= 0 && mz >= 0 && mx < PAP_SIZE_LO && mz < PAP_SIZE_LO) {
                    index = PAP_2LO(mx, mz).Walkable;
                    while (index) {
                        if (index < 0) {
                            SLONG d1, d2;
                            rf2 = &roof_faces4[-index];
                            {
                                if ((rf1->RX & 127) == (rf2->RX & 127)) {
                                    if ((rf1->RZ & 127) == (rf2->RZ & 127) + 1) {
                                        SLONG d1, d2;
                                        d1 = (rf1->Y) - (rf2->Y + (rf2->DY[2] << ROOF_SHIFT));
                                        d2 = (rf1->Y + (rf1->DY[0] << ROOF_SHIFT)) - (rf2->Y + (rf2->DY[1] << ROOF_SHIFT));
                                        if (d1 >= 0 && d1 < SLIDE_EDGE_HEIGHT && d2 >= 0 && d2 <= SLIDE_EDGE_HEIGHT)
                                            rf1->DrawFlags &= ~RFACE_FLAG_SLIDE_EDGE_0;
                                    }
                                    if ((rf1->RZ & 127) == (rf2->RZ & 127) - 1) {
                                        d1 = (rf1->Y + (rf1->DY[2] << ROOF_SHIFT)) - (rf2->Y);
                                        d2 = (rf1->Y + (rf1->DY[1] << ROOF_SHIFT)) - (rf2->Y + (rf2->DY[0] << ROOF_SHIFT));
                                        if (d1 >= 0 && d1 < SLIDE_EDGE_HEIGHT && d2 >= 0 && d2 <= SLIDE_EDGE_HEIGHT)
                                            rf1->DrawFlags &= ~RFACE_FLAG_SLIDE_EDGE_2;
                                    }
                                }
                                if ((rf1->RZ & 127) == (rf2->RZ & 127)) {
                                    if ((rf1->RX & 127) == (rf2->RX & 127) + 1) {
                                        d1 = (rf1->Y) - (rf2->Y + (rf2->DY[0] << ROOF_SHIFT));
                                        d2 = (rf1->Y + (rf1->DY[2] << ROOF_SHIFT)) - (rf2->Y + (rf2->DY[1] << ROOF_SHIFT));
                                        if (d1 >= 0 && d1 < SLIDE_EDGE_HEIGHT && d2 >= 0 && d2 <= SLIDE_EDGE_HEIGHT)
                                            rf1->DrawFlags &= ~RFACE_FLAG_SLIDE_EDGE_3;
                                    }
                                    if ((rf1->RX & 127) == (rf2->RX & 127) - 1) {
                                        d1 = (rf1->Y + (rf1->DY[0] << ROOF_SHIFT)) - (rf2->Y);
                                        d2 = (rf1->Y + (rf1->DY[1] << ROOF_SHIFT)) - (rf2->Y + (rf2->DY[2] << ROOF_SHIFT));
                                        if (d1 >= 0 && d1 < SLIDE_EDGE_HEIGHT && d2 >= 0 && d2 <= SLIDE_EDGE_HEIGHT)
                                            rf1->DrawFlags &= ~RFACE_FLAG_SLIDE_EDGE_1;
                                    }
                                }
                            }
                            index = rf2->Next;
                        } else {
                            index = prim_faces4[index].WALKABLE;
                        }
                    }
                }
            }
        rf1++;
    }
}

// Flag which edges of prim walkable quad faces are grabbable.
// Pass 1: all walkable face edges start as SLIDE_EDGE.
// Pass 2: clear bits where two walkable faces share an edge (interior edges).
// Pass 3: clear edges too close to the ground (within SLIDE_EDGE_HEIGHT).
// Pass 4: clear edges that lie along a fence.
// Also calls calc_slide_edges_roof() for roof faces.
// uc_orig: calc_slide_edges (fallen/Source/Prim.cpp)
void calc_slide_edges()
{
    SLONG i, j, p;
    SLONG dx, dz, len;
    SLONG bx, by, bz;
    SLONG px, pz;
    SLONG x1, z1, x2, z2;
    SLONG mx, mz;
    SLONG ei, ej;
    SLONG ip1, ip2, jp1, jp2;
    SLONG ip1x, ip1y, ip1z;
    SLONG ip2x, ip2y, ip2z;
    SLONG jp1x, jp1y, jp1z;
    SLONG jp2x, jp2y, jp2z;
    SLONG near_height, pos_face;

    PrimFace4 *f, *g;
    SLONG index;

    calc_slide_edges_roof();

    // Pass 1: mark all walkable quad face edges as slide edges.
    for (i = 1; i < next_prim_face4; i++) {
        f = &prim_faces4[i];
        if (f->FaceFlags & FACE_FLAG_WALKABLE)
            f->FaceFlags |= FACE_FLAG_SLIDE_EDGE_0 | FACE_FLAG_SLIDE_EDGE_1 | FACE_FLAG_SLIDE_EDGE_2 | FACE_FLAG_SLIDE_EDGE_3;
    }

    // Point traversal order for quad edges (canonical winding).
    UBYTE point_order[4] = { 0, 1, 3, 2 };

    // Pass 2: clear shared interior edges between neighbouring walkable faces.
    for (i = 1; i < next_prim_face4; i++) {
        f = &prim_faces4[i];
        if (!(f->FaceFlags & FACE_FLAG_WALKABLE)) continue;
        if (f->FaceFlags & FACE_FLAG_WMOVE) continue;

        x1 = +UC_INFINITY; z1 = +UC_INFINITY;
        x2 = -UC_INFINITY; z2 = -UC_INFINITY;

        for (p = 0; p < 4; p++) {
            px = prim_points[f->Points[p]].X;
            pz = prim_points[f->Points[p]].Z;
            if (px < x1) x1 = px;
            if (px > x2) x2 = px;
            if (pz < z1) z1 = pz;
            if (pz > z2) z2 = pz;
        }

        x1 -= JUST_IN_CASE; z1 -= JUST_IN_CASE;
        x2 += JUST_IN_CASE; z2 += JUST_IN_CASE;

        x1 >>= PAP_SHIFT_LO; z1 >>= PAP_SHIFT_LO;
        x2 >>= PAP_SHIFT_LO; z2 >>= PAP_SHIFT_LO;

        SATURATE(x1, 0, PAP_SIZE_LO - 1); SATURATE(x2, 0, PAP_SIZE_LO - 1);
        SATURATE(z1, 0, PAP_SIZE_LO - 1); SATURATE(z2, 0, PAP_SIZE_LO - 1);

        for (mx = x1; mx <= x2; mx++)
            for (mz = z1; mz <= z2; mz++) {
                index = PAP_2LO(mx, mz).Walkable;
                while (index) {
                    j = index;
                    if (j < 0)
                        index = roof_faces4[-j].Next;
                    else
                        index = prim_faces4[j].WALKABLE;

                    if (j <= 0 || j == i) continue;

                    g = &prim_faces4[j];
                    ASSERT(g->FaceFlags & FACE_FLAG_WALKABLE);
                    g->FaceFlags |= FACE_FLAG_WALKABLE;
                    g->DrawFlags |= POLY_FLAG_WALKABLE;

                    for (ei = 0; ei < 4; ei++) {
                        ip1 = f->Points[point_order[ei]];
                        ip2 = f->Points[point_order[(ei + 1) & 0x3]];

                        ip1x = prim_points[ip1].X; ip1y = prim_points[ip1].Y; ip1z = prim_points[ip1].Z;
                        ip2x = prim_points[ip2].X; ip2y = prim_points[ip2].Y; ip2z = prim_points[ip2].Z;

                        for (ej = 0; ej < 4; ej++) {
                            jp1 = g->Points[point_order[ej]];
                            jp2 = g->Points[point_order[(ej + 1) & 0x3]];

                            jp1x = prim_points[jp1].X; jp1y = prim_points[jp1].Y; jp1z = prim_points[jp1].Z;
                            jp2x = prim_points[jp2].X; jp2y = prim_points[jp2].Y; jp2z = prim_points[jp2].Z;

                            if ((ip1 == jp2 && ip2 == jp1) || (ip1x == jp2x && ip1y == jp2y && ip1z == jp2z && ip2x == jp1x && ip2y == jp1y && ip2z == jp1z)) {
                                f->FaceFlags &= ~(FACE_FLAG_SLIDE_EDGE << ei);
                                g->FaceFlags &= ~(FACE_FLAG_SLIDE_EDGE << ej);
                            }
                        }
                    }
                }
            }
    }

    // Pass 3: clear slide edges where the ground drop is less than SLIDE_EDGE_HEIGHT.
    for (i = 1; i < next_prim_face4; i++) {
        f = &prim_faces4[i];
        if (!(f->FaceFlags & FACE_FLAG_WALKABLE)) continue;
        if (f->FaceFlags & FACE_FLAG_WMOVE) continue;

        for (ei = 0; ei < 4; ei++) {
            if (!(f->FaceFlags & (FACE_FLAG_SLIDE_EDGE << ei))) continue;

            ip1 = f->Points[point_order[(ei + 0) & 0x3]];
            ip2 = f->Points[point_order[(ei + 1) & 0x3]];

            ip1x = prim_points[ip1].X; ip1y = prim_points[ip1].Y; ip1z = prim_points[ip1].Z;
            ip2x = prim_points[ip2].X; ip2y = prim_points[ip2].Y; ip2z = prim_points[ip2].Z;

            bx = ip1x + ip2x >> 1;
            by = ip1y + ip2y >> 1;
            bz = ip1z + ip2z >> 1;

            dx = ip2x - ip1x; dz = ip2z - ip1z;
            len = QDIST2(abs(dx), abs(dz)) + 1;

            dx = dx * SLIDE_EDGE_PROBE / len;
            dz = dz * SLIDE_EDGE_PROBE / len;

            bx += dz;
            bz -= dx;

            if (!WITHIN(bx, 0, (PAP_SIZE_HI << 8) - 1) || !WITHIN(bz, 0, (PAP_SIZE_HI << 8) - 1))
                near_height = 0;
            else
                near_height = find_height_for_this_pos(bx, bz, &pos_face);

            if (near_height > by - SLIDE_EDGE_HEIGHT)
                f->FaceFlags &= ~(FACE_FLAG_SLIDE_EDGE << ei);
        }
    }

    // Pass 4: clear edges that lie along a fence.
    for (i = 1; i < next_prim_face4; i++) {
        f = &prim_faces4[i];
        if (!(f->FaceFlags & FACE_FLAG_WALKABLE)) continue;
        if (f->FaceFlags & FACE_FLAG_WMOVE) continue;

        for (ei = 0; ei < 4; ei++) {
            if (!(f->FaceFlags & (FACE_FLAG_SLIDE_EDGE << ei))) continue;

            ip1 = f->Points[point_order[(ei + 0) & 0x3]];
            ip2 = f->Points[point_order[(ei + 1) & 0x3]];

            ip1x = prim_points[ip1].X; ip1y = prim_points[ip1].Y; ip1z = prim_points[ip1].Z;
            ip2x = prim_points[ip2].X; ip2y = prim_points[ip2].Y; ip2z = prim_points[ip2].Z;

            if (does_fence_lie_along_line(ip1x, ip1z, ip2x, ip2z))
                f->FaceFlags &= ~(FACE_FLAG_SLIDE_EDGE << ei);
        }
    }
}

// ============================================================
// Collision
// ============================================================

// Convert a prim-local vertex to world position using the prim's YPR rotation.
// Positions are in integer world units (not the <<8 fixed-point format).
// uc_orig: get_rotated_point_world_pos (fallen/Source/Prim.cpp)
void get_rotated_point_world_pos(
    SLONG point,
    SLONG prim,
    SLONG prim_x, SLONG prim_y, SLONG prim_z,
    SLONG prim_yaw, SLONG prim_pitch, SLONG prim_roll,
    SLONG* px, SLONG* py, SLONG* pz)
{
    SLONG matrix[9];

    ASSERT(WITHIN(prim, 1, next_prim_object - 1));

    PrimObject* po = &prim_objects[prim];
    SLONG num_points = po->EndPoint - po->StartPoint;

    FMATRIX_calc(matrix, prim_yaw, prim_pitch, prim_roll);

    if (point == -1) {
        point = Random() % num_points;
        point += po->StartPoint;
    } else {
        point += po->StartPoint;
    }

    SLONG x = prim_points[point].X;
    SLONG y = prim_points[point].Y;
    SLONG z = prim_points[point].Z;

    FMATRIX_MUL_BY_TRANSPOSE(matrix, x, y, z);

    x += prim_x; y += prim_y; z += prim_z;

    *px = x; *py = y; *pz = z;
}

// Test movement vector (x1,y1,z1)→(x2,y2,z2) against prim bounding box.
// Positions are in <<8 fixed-point; they are divided by 256 internally.
// Returns UC_TRUE if collision occurred and x2/z2 were slid to the nearest edge.
// dont_slide: if UC_TRUE, return UC_TRUE on collision without modifying x2/z2.
// shrink: reduce radius by 0x30 and compress Y range (tight-space traversal).
// uc_orig: slide_along_prim (fallen/Source/Prim.cpp)
SLONG slide_along_prim(
    SLONG prim,
    SLONG prim_x, SLONG prim_y, SLONG prim_z, SLONG prim_yaw,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG* x2, SLONG* y2, SLONG* z2,
    SLONG radius, SLONG shrink, SLONG dont_slide)
{
    SLONG old_x2 = *x2;
    SLONG old_y2 = *y2;
    SLONG old_z2 = *z2;

    SWORD y_bot, y_top;
    PrimInfo* pi;

    x1 >>= 8; y1 >>= 8; z1 >>= 8;
    *x2 >>= 8; *y2 >>= 8; *z2 >>= 8;

    if (shrink)
        radius -= 0x30;

    pi = get_prim_info(prim);

    y_bot = prim_y + pi->miny - FURN_UNDERNEATH_BOT;
    y_top = prim_y + pi->maxy - FURN_UNDERNEATH_TOP;

    if (shrink)
        y_top = y_bot + (y_top - y_bot >> 2);

    if (WITHIN(y1, y_bot, y_top)) {
        if (slide_around_box(
                prim_x, prim_z,
                pi->minx, pi->minz,
                pi->maxx, pi->maxz,
                prim_yaw, radius,
                x1, z1, x2, z2)) {
            *x2 <<= 8;
            *y2 <<= 8;
            *z2 <<= 8;
            return UC_TRUE;
        }
    }

    *x2 = old_x2;
    *y2 = old_y2;
    *z2 = old_z2;

    return UC_FALSE;
}

// uc_orig: prim_get_collision_model (fallen/Source/Prim.cpp)
UBYTE prim_get_collision_model(SLONG prim)
{
    ASSERT(WITHIN(prim, 0, 255));
    return prim_objects[prim].coltype;
}

// uc_orig: prim_get_shadow_type (fallen/Source/Prim.cpp)
UBYTE prim_get_shadow_type(SLONG prim)
{
    ASSERT(WITHIN(prim, 0, 255));
    return prim_objects[prim].shadowtype;
}

// ============================================================
// Animated prims
// ============================================================

// Per-frame update for an animated world prim Thing.
// Advances AnimTween by TweenStep*2 (scaled by TICK_RATIO for frame-rate independence).
// On anim completion: DOOR sets/clears FLAGS_SWITCHED_ON, SWITCH toggles it.
// Setting StateFn = NULL stops further calls (prim becomes idle).
// uc_orig: fn_anim_prim_normal (fallen/Source/Prim.cpp)
void fn_anim_prim_normal(Thing* p_thing)
{
    SLONG tween_step;
    DrawTween* draw_info;

    draw_info = p_thing->Draw.Tweened;
    tween_step = draw_info->CurrentFrame->TweenStep << 1;
    tween_step = (tween_step * TICK_RATIO) >> TICK_SHIFT;
    if (tween_step == 0) tween_step = 1;
    draw_info->AnimTween += tween_step;

    if (draw_info->AnimTween > 256) {
        draw_info->AnimTween -= 256;

        SLONG advance_keyframe(DrawTween * draw_info);

        if (advance_keyframe(draw_info)) {
            switch (get_anim_prim_type(p_thing->Index)) {
            case ANIM_PRIM_TYPE_NORMAL:
                break;

            case ANIM_PRIM_TYPE_DOOR:
                if (p_thing->Draw.Tweened->CurrentAnim == 2)
                    p_thing->Flags |= FLAGS_SWITCHED_ON;
                else
                    p_thing->Flags &= ~FLAGS_SWITCHED_ON;
                p_thing->StateFn = NULL;
                break;

            case ANIM_PRIM_TYPE_SWITCH:
                p_thing->Flags ^= FLAGS_SWITCHED_ON;
                p_thing->StateFn = NULL;
                break;

            default:
                ASSERT(0);
                break;
            }
        }
    }
}

// Spawn an animated world object Thing at the given world position.
// Called from load_game_map() for each MAP_THING_TYPE_ANIM_PRIM entry.
// ANIM_PRIM_TYPE_NORMAL gets StateFn = fn_anim_prim_normal (loops continuously).
// DOOR and SWITCH start idle (StateFn = NULL) until triggered.
// uc_orig: create_anim_prim (fallen/Source/Prim.cpp)
void create_anim_prim(SLONG x, SLONG y, SLONG z, SLONG prim, SLONG yaw)
{
    SLONG new_thing;
    Thing* t_thing;
    new_thing = alloc_primary_thing(CLASS_ANIM_PRIM);
    if (new_thing) {
        t_thing = TO_THING(new_thing);
        t_thing->Class = CLASS_ANIM_PRIM;
        t_thing->WorldPos.X = x << 8;
        t_thing->WorldPos.Y = y << 8;
        t_thing->WorldPos.Z = z << 8;
        t_thing->StateFn = NULL;
        t_thing->Draw.Tweened = alloc_draw_tween(DT_ROT_MULTI);
        t_thing->DrawType = DT_ANIM_PRIM;
        t_thing->Index = prim;
        t_thing->Flags = 0;

        t_thing->Draw.Tweened->Angle = yaw;
        t_thing->Draw.Tweened->Roll = 0;
        t_thing->Draw.Tweened->Tilt = 0;
        t_thing->Draw.Tweened->AnimTween = 0;
        t_thing->Draw.Tweened->TweenStage = 0;
        t_thing->Draw.Tweened->NextFrame = NULL;
        t_thing->Draw.Tweened->QueuedFrame = NULL;
        t_thing->Draw.Tweened->TheChunk = &anim_chunk[prim];
        t_thing->Draw.Tweened->CurrentFrame = anim_chunk[prim].AnimList[1];
        t_thing->Draw.Tweened->CurrentAnim = 1;

        if (t_thing->Draw.Tweened->CurrentFrame)
            t_thing->Draw.Tweened->NextFrame = t_thing->Draw.Tweened->CurrentFrame->NextFrame;

        t_thing->Draw.Tweened->FrameIndex = 0;
        t_thing->Draw.Tweened->Flags = 0;

        add_thing_to_map(t_thing);

        switch (get_anim_prim_type(t_thing->Index)) {
        case ANIM_PRIM_TYPE_NORMAL:
            t_thing->StateFn = fn_anim_prim_normal;
            break;
        case ANIM_PRIM_TYPE_DOOR:
            t_thing->Flags |= FLAGS_SWITCHED_ON;
            break;
        case ANIM_PRIM_TYPE_SWITCH:
            break;
        default:
            ASSERT(0);
            break;
        }
    }
}

// uc_orig: set_anim_prim_anim (fallen/Source/Prim.cpp)
void set_anim_prim_anim(SLONG anim_prim, SLONG anim)
{
    Thing* t_thing = TO_THING(anim_prim);

    ASSERT(WITHIN(anim, 1, anim_chunk[t_thing->Index].MaxAnimFrames - 1));

    t_thing->Draw.Tweened->AnimTween = 0;
    t_thing->Draw.Tweened->NextFrame = NULL;
    t_thing->Draw.Tweened->QueuedFrame = NULL;
    t_thing->Draw.Tweened->CurrentFrame = anim_chunk[t_thing->Index].AnimList[anim];
    t_thing->Draw.Tweened->FrameIndex = 0;
    t_thing->Draw.Tweened->CurrentAnim = anim;

    if (t_thing->Draw.Tweened->CurrentFrame)
        t_thing->Draw.Tweened->NextFrame = t_thing->Draw.Tweened->CurrentFrame->NextFrame;

    t_thing->StateFn = fn_anim_prim_normal;
}

// Hard-coded mapping from anim_prim index to behaviour class:
//   3,5,6,7,8,10,11 → ANIM_PRIM_TYPE_DOOR
//   4               → ANIM_PRIM_TYPE_SWITCH
//   everything else → ANIM_PRIM_TYPE_NORMAL
// uc_orig: get_anim_prim_type (fallen/Source/Prim.cpp)
SLONG get_anim_prim_type(SLONG anim_prim)
{
    switch (anim_prim) {
    case 4: return ANIM_PRIM_TYPE_SWITCH;
    case 3: case 5: case 6: case 7: case 8: case 10: case 11: return ANIM_PRIM_TYPE_DOOR;
    default: return ANIM_PRIM_TYPE_NORMAL;
    }
}

// Find the nearest CLASS_ANIM_PRIM Thing within range of the given position
// whose type matches type_bit_field (bitmask of (1 << ANIM_PRIM_TYPE_*) flags).
// Distance is Manhattan (abs(dx)+abs(dy)+abs(dz)) for speed.
// Returns THING_INDEX of best match, or NULL if none found.
// uc_orig: find_anim_prim (fallen/Source/Prim.cpp)
SLONG find_anim_prim(SLONG x, SLONG y, SLONG z, SLONG range, ULONG type_bit_field)
{
    SLONG i;
    SLONG dx, dy, dz, dist;
    THING_INDEX best_thing = NULL;
    SLONG best_dist = range + 1;
    SLONG num;

    num = THING_find_sphere(x, y, z, range, found_aprim, MAX_FIND_ANIM_PRIMS, (1 << CLASS_ANIM_PRIM));

    for (i = 0; i < num; i++) {
        if (type_bit_field & (1 << get_anim_prim_type(TO_THING(found_aprim[i])->Index))) {
            dx = (TO_THING(found_aprim[i])->WorldPos.X >> 8) - x;
            dy = (TO_THING(found_aprim[i])->WorldPos.Y >> 8) - y;
            dz = (TO_THING(found_aprim[i])->WorldPos.Z >> 8) - z;
            dist = abs(dx) + abs(dy) + abs(dz);
            if (dist < best_dist) {
                best_dist = dist;
                best_thing = found_aprim[i];
            }
        }
    }

    return best_thing;
}

// Player-triggered toggle for switch-type anim prims.
// Ignored if the prim is currently animating (StateFn != NULL).
// uc_orig: toggle_anim_prim_switch_state (fallen/Source/Prim.cpp)
void toggle_anim_prim_switch_state(SLONG anim_prim_thing_index)
{
    Thing* t_thing = TO_THING(anim_prim_thing_index);
    if (t_thing->StateFn)
        return;
    if (t_thing->Flags & FLAGS_SWITCHED_ON)
        set_anim_prim_anim(anim_prim_thing_index, 2);
    else
        set_anim_prim_anim(anim_prim_thing_index, 1);
}

// Expand a bounding box to contain all vertices of a prim in the pose
// described by anim_info. Used to compute the full motion envelope of
// an animated prim for collision/shadow purposes.
// uc_orig: expand_anim_prim_bbox (fallen/Source/Prim.cpp)
void expand_anim_prim_bbox(
    SLONG prim,
    struct GameKeyFrameElement* anim_info,
    SLONG* min_x, SLONG* min_y, SLONG* min_z,
    SLONG* max_x, SLONG* max_y, SLONG* max_z)
{
    SLONG i;
    float px, py, pz;
    SLONG ix, iy, iz;
    Matrix33 mat;
    PrimObject* po;

    CMatrix33 m1;
    GetCMatrix(anim_info, &m1);
    build_tween_matrix(&mat, &m1, &m1, 0);

    float fmatrix[9];
    float ox = float(anim_info->OffsetX);
    float oy = float(anim_info->OffsetY);
    float oz = float(anim_info->OffsetZ);

    fmatrix[0] = float(mat.M[0][0]) * (1.0F / 32768.0F);
    fmatrix[1] = float(mat.M[0][1]) * (1.0F / 32768.0F);
    fmatrix[2] = float(mat.M[0][2]) * (1.0F / 32768.0F);
    fmatrix[3] = float(mat.M[1][0]) * (1.0F / 32768.0F);
    fmatrix[4] = float(mat.M[1][1]) * (1.0F / 32768.0F);
    fmatrix[5] = float(mat.M[1][2]) * (1.0F / 32768.0F);
    fmatrix[6] = float(mat.M[2][0]) * (1.0F / 32768.0F);
    fmatrix[7] = float(mat.M[2][1]) * (1.0F / 32768.0F);
    fmatrix[8] = float(mat.M[2][2]) * (1.0F / 32768.0F);

    ASSERT(WITHIN(prim, 1, next_prim_object - 1));
    po = &prim_objects[prim];

    for (i = po->StartPoint; i < po->EndPoint; i++) {
        px = float(prim_points[i].X);
        py = float(prim_points[i].Y);
        pz = float(prim_points[i].Z);

        MATRIX_MUL(fmatrix, px, py, pz);

        px += ox; py += oy; pz += oz;

        ix = SLONG(px); iy = SLONG(py); iz = SLONG(pz);

        if (ix < *min_x) *min_x = ix;
        if (iy < *min_y) *min_y = iy;
        if (iz < *min_z) *min_z = iz;
        if (ix > *max_x) *max_x = ix;
        if (iy > *max_y) *max_y = iy;
        if (iz > *max_z) *max_z = iz;
    }
}

// Compute bounding boxes for all loaded anim-prims using their initial pose.
// Enforces PRIM_MIN_BBOX minimum on X/Z dimensions.
// uc_orig: find_anim_prim_bboxes (fallen/Source/Prim.cpp)
void find_anim_prim_bboxes()
{
    SLONG i, j, dist, ele_count, start_object;
    GameKeyFrameElement* ele;
    AnimPrimBbox* pmb;

    for (i = 1; i < MAX_ANIM_CHUNKS; i++) {
        pmb = &anim_prim_bbox[i];

        pmb->minx = +UC_INFINITY; pmb->miny = +UC_INFINITY; pmb->minz = +UC_INFINITY;
        pmb->maxx = -UC_INFINITY; pmb->maxy = -UC_INFINITY; pmb->maxz = -UC_INFINITY;

        if (anim_chunk[i].MultiObject[0] == 0)
            continue;

        ASSERT(anim_chunk[i].AnimList[1]);

        ele = anim_chunk[i].AnimList[1]->FirstElement;
        ASSERT(ele != NULL);

        ele_count = anim_chunk[i].ElementCount;
        start_object = prim_multi_objects[anim_chunk[i].MultiObject[0]].StartObject;

        for (j = 0; j < ele_count; j++) {
            expand_anim_prim_bbox(
                start_object + j, ele + j,
                &pmb->minx, &pmb->miny, &pmb->minz,
                &pmb->maxx, &pmb->maxy, &pmb->maxz);
        }

        dist = pmb->maxx - pmb->minx;
        if (dist < PRIM_MIN_BBOX) {
            pmb->maxx += PRIM_MIN_BBOX - dist >> 1;
            pmb->minx -= PRIM_MIN_BBOX - dist >> 1;
        }

        dist = pmb->maxz - pmb->minz;
        if (dist < PRIM_MIN_BBOX) {
            pmb->maxz += PRIM_MIN_BBOX - dist >> 1;
            pmb->minz -= PRIM_MIN_BBOX - dist >> 1;
        }
    }
}

// uc_orig: mark_prim_objects_as_unloaded (fallen/Source/Prim.cpp)
void mark_prim_objects_as_unloaded()
{
    memset((UBYTE*)prim_objects, 0, sizeof(PrimObject) * 256);
}

// uc_orig: re_center_prim (fallen/Source/Prim.cpp)
void re_center_prim(SLONG prim, SLONG dx, SLONG dy, SLONG dz)
{
    SLONG c0;
    struct PrimObject* p_obj;
    SLONG sp, ep;

    p_obj = &prim_objects[prim];
    sp = p_obj->StartPoint;
    ep = p_obj->EndPoint;

    for (c0 = sp; c0 < ep; c0++) {
        prim_points[c0].X += dx;
        prim_points[c0].Y += dy;
        prim_points[c0].Z += dz;
    }
}

// Returns UC_TRUE if a fence (of any fence type or barb-top facet) lies entirely
// along the given orthogonal world-space line.
// Only orthogonal lines (vertical or horizontal) are supported.
// Coordinates are integer world units (one unit = one mapsquare / 256).
// uc_orig: does_fence_lie_along_line (fallen/Source/Prim.cpp)
SLONG does_fence_lie_along_line(SLONG x1, SLONG z1, SLONG x2, SLONG z2)
{
    SLONG dx = abs(x2 - x1);
    SLONG dz = abs(z2 - z1);

    if (dx < dz)
        x1 = x2;
    else
        z1 = z2;

    SLONG mx1 = x1 - 0x80 >> PAP_SHIFT_LO;
    SLONG mz1 = z1 - 0x80 >> PAP_SHIFT_LO;
    SLONG mx2 = x2 + 0x80 >> PAP_SHIFT_LO;
    SLONG mz2 = z2 + 0x80 >> PAP_SHIFT_LO;

    SLONG minx, maxx, minz, maxz;
    SLONG mx, mz;
    SLONG f_list, exit, facet;
    DFacet* df;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1); SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1); SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            f_list = PAP_2LO(mx, mz).ColVectHead;
            if (f_list) {
                exit = UC_FALSE;
                do {
                    facet = facet_links[f_list++];
                    if (facet < 0) {
                        facet = -facet;
                        exit = UC_TRUE;
                    }

                    df = &dfacets[facet];

                    if (df->FacetType == STOREY_TYPE_FENCE ||
                        df->FacetType == STOREY_TYPE_FENCE_FLAT ||
                        df->FacetType == STOREY_TYPE_FENCE_BRICK ||
                        (df->FacetFlags & FACET_FLAG_BARB_TOP)) {
                        if (x1 == x2) {
                            if ((df->x[0] << 8) == x1) {
                                minz = df->z[0] << 8;
                                maxz = df->z[1] << 8;
                                if (minz > maxz) SWAP(minz, maxz);
                                if (WITHIN(z1, minz, maxz) && WITHIN(z2, minz, maxz))
                                    return UC_TRUE;
                            }
                        } else {
                            if ((df->z[0] << 8) == z1) {
                                minx = df->x[0] << 8;
                                maxx = df->x[1] << 8;
                                if (minx > maxx) SWAP(minx, maxx);
                                if (WITHIN(x1, minx, maxx) && WITHIN(x2, minx, maxx))
                                    return UC_TRUE;
                            }
                        }
                    }
                } while (!exit);
            }
        }

    return UC_FALSE;
}

// Clear FACE_FLAG_WMOVE from all prim face3 and face4 pools.
// Called before rebuilding the WMOVE (moving walkable face) list.
// uc_orig: clear_all_wmove_flags (fallen/Source/Prim.cpp)
void clear_all_wmove_flags(void)
{
    SLONG i;
    PrimFace3* f3;
    PrimFace4* f4;

    for (i = 1; i < next_prim_face3; i++) {
        f3 = &prim_faces3[i];
        f3->FaceFlags &= ~FACE_FLAG_WMOVE;
    }
    for (i = 1; i < next_prim_face4; i++) {
        f4 = &prim_faces4[i];
        f4->FaceFlags &= ~FACE_FLAG_WMOVE;
    }
}
