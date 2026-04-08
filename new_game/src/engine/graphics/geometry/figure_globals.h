#ifndef ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "game/game.h"
#include "buildings/prim_types.h"  // MAX_PRIM_OBJECTS, TomsPrimObject, Matrix33 (via fmatrix.h)
#include "engine/animation/anim_types.h"  // BodyDef, GameKeyFrameElement, CMatrix33, etc.
#include "engine/graphics/pipeline/poly.h" // POLY_Point

// Forward declaration: MSMesh is fully declared in figure.h (included by figure.cpp).
// figure_globals.h intentionally does not include figure.h to avoid circular dependency.
class MSMesh;

// uc_orig: FIGURE_alpha (fallen/DDEngine/Source/figure.cpp)
// Overall character alpha (0=transparent, 255=opaque). Set before FIGURE_draw calls.
extern SLONG FIGURE_alpha;

// uc_orig: steam_seed (fallen/DDEngine/Source/figure.cpp)
// LCG seed for get_steam_rand(); reset each draw call for reproducible steam patterns.
extern SLONG steam_seed;

// uc_orig: body_part_upper (fallen/DDEngine/Source/figure.cpp)
// Per-part-slot flag: 1=upper body (torso+arms), 0=lower body. 15 entries for 15 body parts.
extern UBYTE body_part_upper[15];

// uc_orig: fire_pal (fallen/DDEngine/Source/figure.cpp)
// 256-entry RGB palette loaded from data\flames1.pal; used by draw_flames/draw_flame_element.
extern UBYTE fire_pal[768];

// uc_orig: jacket_lookup (fallen/DDEngine/Source/figure.cpp)
// Maps gang jacket texture slot [0..3] x skill tier [0..7] to a texture page index.
extern UWORD jacket_lookup[4][8];

// uc_orig: MAX_NUMBER_D3D_PRIMS (fallen/DDEngine/Source/figure.cpp)
#define MAX_NUMBER_D3D_PRIMS MAX_PRIM_OBJECTS

// uc_orig: D3DObj (fallen/DDEngine/Source/figure.cpp)
// Pre-compiled D3D representations of all static prim meshes (one per prim index).
extern TomsPrimObject D3DObj[MAX_NUMBER_D3D_PRIMS];

// uc_orig: m_fObjectBoundingSphereRadius (fallen/DDEngine/Source/figure.cpp)
// Cached bounding sphere radius for each prim (used for near-Z culling).
extern float m_fObjectBoundingSphereRadius[MAX_NUMBER_D3D_PRIMS];

// uc_orig: NUM_ROPERS_THINGIES (fallen/DDEngine/Source/figure.cpp)
#define NUM_ROPERS_THINGIES 8

// uc_orig: MAX_NUMBER_D3D_PEOPLE (fallen/DDEngine/Source/figure.cpp)
#define MAX_NUMBER_D3D_PEOPLE (32 + 16 + NUM_ROPERS_THINGIES)

// uc_orig: D3DPeopleObj (fallen/DDEngine/Source/figure.cpp)
// Pre-compiled D3D representations of all character-specific prim meshes.
extern TomsPrimObject D3DPeopleObj[MAX_NUMBER_D3D_PEOPLE];

// uc_orig: m_iLRUQueueSize (fallen/DDEngine/Source/figure.cpp)
// Current number of entries in the TomsPrimObject LRU cache.
extern int m_iLRUQueueSize;

// uc_orig: PRIM_LRU_QUEUE_LENGTH (fallen/DDEngine/Source/figure.cpp)
// Maximum number of prim meshes that can be cached simultaneously. Must be < 256.
#define PRIM_LRU_QUEUE_LENGTH 250

// uc_orig: PRIM_LRU_QUEUE_SIZE (fallen/DDEngine/Source/figure.cpp)
// Maximum combined vertex count across all cached prim meshes.
#define PRIM_LRU_QUEUE_SIZE 6000

// uc_orig: ptpoLRUQueue (fallen/DDEngine/Source/figure.cpp)
// LRU queue of pointers to cached TomsPrimObjects, indexed by slot number.
extern TomsPrimObject* ptpoLRUQueue[PRIM_LRU_QUEUE_LENGTH];

// uc_orig: dwGameTurnLastUsed (fallen/DDEngine/Source/figure.cpp)
// Game turn at which each LRU slot was last accessed (for eviction ordering).
extern DWORD dwGameTurnLastUsed[PRIM_LRU_QUEUE_LENGTH];

// uc_orig: m_dwSizeOfQueue (fallen/DDEngine/Source/figure.cpp)
// Total vertex count currently held in the LRU cache.
extern DWORD m_dwSizeOfQueue;

// --- TPO (TomsPrimObject compilation) working state ---
// Originally static file-private variables in figure.cpp. Moved to globals per project rules.
// Used by FIGURE_TPO_init_3d_object, FIGURE_TPO_add_prim_to_current_object,
// and FIGURE_TPO_finish_3d_object across compilation chunks.

// uc_orig: TPO_pVert (fallen/DDEngine/Source/figure.cpp)
extern GEVertex* TPO_pVert;
// uc_orig: TPO_pStripIndices (fallen/DDEngine/Source/figure.cpp)
extern UWORD* TPO_pStripIndices;
// uc_orig: TPO_pListIndices (fallen/DDEngine/Source/figure.cpp)
extern UWORD* TPO_pListIndices;
// uc_orig: TPO_piVertexRemap (fallen/DDEngine/Source/figure.cpp)
extern int* TPO_piVertexRemap;
// uc_orig: TPO_piVertexLinks (fallen/DDEngine/Source/figure.cpp)
extern int* TPO_piVertexLinks;
// uc_orig: TPO_pCurVertex (fallen/DDEngine/Source/figure.cpp)
extern GEVertex* TPO_pCurVertex;
// uc_orig: TPO_pCurStripIndex (fallen/DDEngine/Source/figure.cpp)
extern UWORD* TPO_pCurStripIndex;
// uc_orig: TPO_pCurListIndex (fallen/DDEngine/Source/figure.cpp)
extern UWORD* TPO_pCurListIndex;
// uc_orig: TPO_pPrimObj (fallen/DDEngine/Source/figure.cpp)
extern TomsPrimObject* TPO_pPrimObj;
// uc_orig: TPO_iNumListIndices (fallen/DDEngine/Source/figure.cpp)
extern int TPO_iNumListIndices;
// uc_orig: TPO_iNumStripIndices (fallen/DDEngine/Source/figure.cpp)
extern int TPO_iNumStripIndices;
// uc_orig: TPO_iNumVertices (fallen/DDEngine/Source/figure.cpp)
extern int TPO_iNumVertices;

// uc_orig: TPO_MAX_NUMBER_PRIMS (fallen/DDEngine/Source/figure.cpp)
#define TPO_MAX_NUMBER_PRIMS 16

// uc_orig: TPO_iNumPrims (fallen/DDEngine/Source/figure.cpp)
extern int TPO_iNumPrims;
// uc_orig: TPO_PrimObjects (fallen/DDEngine/Source/figure.cpp)
extern SLONG TPO_PrimObjects[TPO_MAX_NUMBER_PRIMS];
// uc_orig: TPO_ubPrimObjMMIndex (fallen/DDEngine/Source/figure.cpp)
extern UBYTE TPO_ubPrimObjMMIndex[TPO_MAX_NUMBER_PRIMS];
// uc_orig: TPO_iPrimObjIndexOffset (fallen/DDEngine/Source/figure.cpp)
extern int TPO_iPrimObjIndexOffset[TPO_MAX_NUMBER_PRIMS + 1];

// Allocation limits used by FIGURE_TPO_init_3d_object and FIGURE_TPO_add_prim_to_current_object.
// uc_orig: MAX_VERTS (fallen/DDEngine/Source/figure.cpp)
#define MAX_VERTS 1024
// uc_orig: MAX_INDICES (fallen/DDEngine/Source/figure.cpp)
#define MAX_INDICES (MAX_VERTS * 4)

// --- D3D MultiMatrix lighting working state ---
// These pointers point into aligned static arrays allocated in figure.cpp.
// Used by BuildMMLightingTable (chunk 1) and FIGURE_draw_prim_tween_person_only (chunk 5).

// uc_orig: MM_pcFadeTable (fallen/DDEngine/Source/figure.cpp)
// 128-entry ULONG fade table: 0-63=lit ramp, 64-127=flat ambient.
extern ULONG* MM_pcFadeTable;

// uc_orig: MM_pcFadeTableTint (fallen/DDEngine/Source/figure.cpp)
// Same as MM_pcFadeTable but with colour_and mask applied (for tinted textures).
extern ULONG* MM_pcFadeTableTint;

// uc_orig: MM_pMatrix (fallen/DDEngine/Source/figure.cpp)
// One GEMatrix slot for the MultiMatrix draw extension.
extern GEMatrix* MM_pMatrix;

// uc_orig: MM_Vertex (fallen/DDEngine/Source/figure.cpp)
// Scratch GEVertex array for MultiMatrix vertex submission.
extern GEVertex* MM_Vertex;

// uc_orig: MM_pNormal (fallen/DDEngine/Source/figure.cpp)
// Light direction in object space (4 floats: padding + x + y + z).
extern float* MM_pNormal;

// uc_orig: MM_vLightDir (fallen/DDEngine/Source/figure.cpp)
// Dominant light direction vector (unit), computed per character in BuildMMLightingTable.
extern GEVector MM_vLightDir;

// uc_orig: MM_bLightTableAlreadySetUp (fallen/DDEngine/Source/figure.cpp)
// Set to true once BuildMMLightingTable has been called for the current body part.
extern bool MM_bLightTableAlreadySetUp;

// uc_orig: mesh (fallen/DDEngine/Source/figure.cpp)
// File-private MSMesh instance used during FIGURE_TPO_finish_3d_object.
// Renamed g_mesh to avoid conflict with potential future global 'mesh'.
extern MSMesh g_mesh;

// --- Hierarchical body-part draw state ---
// Global iterative-recursion data for FIGURE_draw_hierarchical_prim_recurse.

// uc_orig: MAX_RECURSION (fallen/DDEngine/Source/figure.cpp)
// Maximum bone hierarchy depth (15 parts uses at most 4 levels).
#define MAX_RECURSION 5

// uc_orig: structFIGURE_dhpr_data (fallen/DDEngine/Source/figure.cpp)
// Input data block for FIGURE_draw_hierarchical_prim_recurse: the thing to draw.
struct structFIGURE_dhpr_data {
    SLONG start_object;
    BodyDef* body_def;
    Matrix31* world_pos;
    SLONG tween;
    GameKeyFrameElement* ae1;
    GameKeyFrameElement* ae2;
    Matrix33* world_mat;
    Matrix33* world_mat2;
    SLONG dx;
    SLONG dy;
    SLONG dz;
    ULONG colour;
    ULONG specular;
    BYTE bPersonType;
    BYTE bPersonID;
};

// uc_orig: structFIGURE_dhpr_rdata1 (fallen/DDEngine/Source/figure.cpp)
// Per-level recursion state block 1 for the hierarchical draw traversal.
struct structFIGURE_dhpr_rdata1 {
    SLONG part_number;
    SLONG current_child_number;
    CMatrix33* parent_base_mat;
    Matrix31* parent_base_pos;
    Matrix33* parent_current_mat;
    Matrix31* parent_current_pos;
    Matrix31 pos;
};

// uc_orig: structFIGURE_dhpr_rdata2 (fallen/DDEngine/Source/figure.cpp)
// Per-level recursion state block 2 (end position/matrix for parent-to-child transform).
struct structFIGURE_dhpr_rdata2 {
    Matrix31 end_pos;
    Matrix33 end_mat;
};

// uc_orig: FIGURE_dhpr_data (fallen/DDEngine/Source/figure.cpp)
extern structFIGURE_dhpr_data FIGURE_dhpr_data;

// uc_orig: FIGURE_dhpr_rdata1 (fallen/DDEngine/Source/figure.cpp)
extern structFIGURE_dhpr_rdata1 FIGURE_dhpr_rdata1[MAX_RECURSION];

// uc_orig: FIGURE_dhpr_rdata2 (fallen/DDEngine/Source/figure.cpp)
extern structFIGURE_dhpr_rdata2 FIGURE_dhpr_rdata2[MAX_RECURSION];

// uc_orig: PART_FACE (fallen/DDEngine/Source/figure.cpp)
#define PART_FACE 1
// uc_orig: PART_TROUSERS (fallen/DDEngine/Source/figure.cpp)
#define PART_TROUSERS 2
// uc_orig: PART_JACKET (fallen/DDEngine/Source/figure.cpp)
#define PART_JACKET 3
// uc_orig: PART_SHOES (fallen/DDEngine/Source/figure.cpp)
#define PART_SHOES 4
// uc_orig: PART_PELVIS (fallen/DDEngine/Source/figure.cpp)
#define PART_PELVIS 5
// uc_orig: PART_HANDS (fallen/DDEngine/Source/figure.cpp)
#define PART_HANDS 6

// uc_orig: part_type (fallen/DDEngine/Source/figure.cpp)
// Body-part clothing category for each of the 15 character body parts (PART_* constants).
extern UBYTE part_type[15];

// uc_orig: local_seed (fallen/DDEngine/Source/figure.cpp)
// LCG seed used by mandom() for per-character clothing colour randomisation.
extern ULONG local_seed;

// uc_orig: jacket_col (fallen/DDEngine/Source/figure.cpp)
// Current jacket colour value computed per character during hierarchical draw.
extern ULONG jacket_col;

// uc_orig: leg_col (fallen/DDEngine/Source/figure.cpp)
// Current leg/trouser colour value computed per character during hierarchical draw.
extern ULONG leg_col;

// uc_orig: MAX_NUM_BODY_PARTS_AT_ONCE (fallen/DDEngine/Source/figure.cpp)
// Maximum number of body parts batched in one DrawIndPrimMM call.
#define MAX_NUM_BODY_PARTS_AT_ONCE 20

// uc_orig: MMBodyParts_pMatrix (fallen/DDEngine/Source/figure.cpp)
// Pointer to aligned storage block for MAX_NUM_BODY_PARTS_AT_ONCE GEMatrix objects.
// Initialised at startup by MMBodyPartsInit. Passed as matrices in GEMultiMatrix.
extern GEMatrix* MMBodyParts_pMatrix;

// uc_orig: MMBodyParts_pNormal (fallen/DDEngine/Source/figure.cpp)
// Pointer to aligned float storage for MAX_NUM_BODY_PARTS_AT_ONCE * 4 light direction floats.
// Initialised at startup by MMBodyPartsInit. Passed as lpvLightDirs in GEMultiMatrix.
extern float* MMBodyParts_pNormal;

// --- Misc character draw state ---

// uc_orig: peep_recol (fallen/DDEngine/Source/figure.cpp)
// 16-entry RGB colour table for civilian clothing variation (r/g/b scaled 0-70).
// PeepRecolEntry is a new name for the anonymous struct used in the original.
struct PeepRecolEntry {
    SWORD r, g, b;
};
// uc_orig: peep_recol (fallen/DDEngine/Source/figure.cpp)
extern PeepRecolEntry peep_recol[16];

// uc_orig: kludge_shrink (fallen/DDEngine/Source/figure.cpp)
// When true, scales down the next MESH_draw_poly call (used for grenade-in-hand rendering).
extern UBYTE kludge_shrink;

// --- Reflection draw state ---
// Used by FIGURE_draw_prim_tween_reflection and FIGURE_draw_reflection.

// uc_orig: FIGURE_Rpoint (fallen/DDEngine/Source/figure.cpp)
// One entry in the screen-space reflection point buffer: distance from reflection plane + projected POLY_Point.
struct FIGURE_Rpoint {
    union {
        float distance;
        ULONG clip;
    };
    POLY_Point pp;
};

// uc_orig: FIGURE_MAX_RPOINTS (fallen/DDEngine/Source/figure.cpp)
#define FIGURE_MAX_RPOINTS 256

// uc_orig: FIGURE_rpoint (fallen/DDEngine/Source/figure.cpp)
// Per-body-part screen-space reflection point buffer, filled by FIGURE_draw_prim_tween_reflection.
extern FIGURE_Rpoint FIGURE_rpoint[FIGURE_MAX_RPOINTS];

// uc_orig: FIGURE_rpoint_upto (fallen/DDEngine/Source/figure.cpp)
// Number of valid entries in FIGURE_rpoint[].
extern SLONG FIGURE_rpoint_upto;

// uc_orig: FIGURE_reflect_x1 (fallen/DDEngine/Source/figure.cpp)
// Screen-space bounding box of reflected points: left edge.
extern SLONG FIGURE_reflect_x1;

// uc_orig: FIGURE_reflect_y1 (fallen/DDEngine/Source/figure.cpp)
// Screen-space bounding box of reflected points: top edge.
extern SLONG FIGURE_reflect_y1;

// uc_orig: FIGURE_reflect_x2 (fallen/DDEngine/Source/figure.cpp)
// Screen-space bounding box of reflected points: right edge.
extern SLONG FIGURE_reflect_x2;

// uc_orig: FIGURE_reflect_y2 (fallen/DDEngine/Source/figure.cpp)
// Screen-space bounding box of reflected points: bottom edge.
extern SLONG FIGURE_reflect_y2;

// uc_orig: FIGURE_reflect_height (fallen/DDEngine/Source/figure.cpp)
// World-space Y coordinate of the water reflection plane.
extern float FIGURE_reflect_height;

#endif // ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H
