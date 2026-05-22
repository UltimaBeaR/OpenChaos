#ifndef ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H
#define ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H

#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "game/game.h"
#include "buildings/prim_types.h" // MAX_PRIM_OBJECTS, TomsPrimObject, Matrix33 (via fmatrix.h)
#include "engine/animation/anim_types.h" // BodyDef, GameKeyFrameElement, CMatrix33, etc.
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

// Pre-compiled consolidated D3D meshes for ANIM_obj_draw rigs (non-person
// skeletons: Bane, Balrog, bats, Gargoyle, ...). A "rig" here is the
// (chunk, MultiObject-start_object) pair — each such pair gets its own
// consolidated TomsPrimObject so the whole creature is one mesh + one
// draw call per material, same shape as D3DPeopleObj for persons.
//
// 32 slots is comfortable headroom: the game has ~4 non-person chunks
// and each chunk realistically uses a handful of MultiObjects max.
//
// Built lazily on first draw of a given rig (see ANIM_obj_draw); slot
// reuse is keyed by chunk pointer + start_object, so different rigs
// land in different slots and there is no false sharing.
#define MAX_NUMBER_D3D_ANIMALS 32
struct AnimObjKey {
    const struct GameKeyFrameChunk* chunk;
    SLONG                           start_object;
};
extern AnimObjKey     D3DAnimObjKeys[MAX_NUMBER_D3D_ANIMALS];
extern TomsPrimObject D3DAnimObj    [MAX_NUMBER_D3D_ANIMALS];

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
// Was 16 originally — enough for the 15-bone person rig. Bumped to 20
// (= POSE_MAX_BONES, matches MAX_NUM_BODY_PARTS_AT_ONCE) so the same
// TPO machinery can consolidate flat-skeleton creatures (Balrog uses
// more than 16 parts).
#define TPO_MAX_NUMBER_PRIMS 20

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

// --- Lighting working state (consumed by the world-skin shader) ---

// Compressed half-Lambert ramp endpoints — the legacy 64-entry table
// (uc_orig: MM_pcFadeTable, fallen/DDEngine/Source/figure.cpp) was built
// as a linear float ramp truncated to bytes per step, so two RGB-float
// endpoints fully describe it: ramp[i] = clamp(start + i*step, 0, 255),
// floored. The shader reconstructs the byte-truncated value at the
// queried index (see skin_world_vert.glsl `u_fade_start`/`u_fade_step`).
// Built per character in BuildMMLightingTable.
extern float MM_FadeStart[3];     // ambient end (idx 0)
extern float MM_FadeStep[3];      // per-index delta

// Tint variant — the legacy table applied a per-byte colour_and mask
// to each entry; for the masks we actually use (each byte is 0x00 or
// 0xFF, never partial) that's equivalent to zeroing the corresponding
// start/step channels here. (uc_orig: MM_pcFadeTableTint).
extern float MM_FadeStartTint[3];
extern float MM_FadeStepTint[3];

// uc_orig: MM_Vertex (fallen/DDEngine/Source/figure.cpp)
// Scratch GEVertex array used by BuildMMLightingTable.
extern GEVertex* MM_Vertex;

// uc_orig: MM_vLightDir (fallen/DDEngine/Source/figure.cpp)
// Dominant light direction vector (unit), computed per character in
// BuildMMLightingTable. Uploaded to the world-skin shader pre-scaled
// by 251 (= legacy fNormScale) so the per-vertex half-Lambert math
// in the shader matches the original CPU path.
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

// uc_orig: FIGURE_dhpr_data (fallen/DDEngine/Source/figure.cpp)
extern structFIGURE_dhpr_data FIGURE_dhpr_data;

// uc_orig: FIGURE_dhpr_rdata1 (fallen/DDEngine/Source/figure.cpp)
extern structFIGURE_dhpr_rdata1 FIGURE_dhpr_rdata1[MAX_RECURSION];

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

// uc_orig: MAX_NUM_BODY_PARTS_AT_ONCE (fallen/DDEngine/Source/figure.cpp)
// Maximum number of body parts in one consolidated character mesh.
// Was originally the cap on a D3D DrawIndPrimMM draw; today this is
// the upper bound on bones in the world-skin palette per character.
#define MAX_NUM_BODY_PARTS_AT_ONCE 20

// --- Misc character draw state ---

// uc_orig: kludge_shrink (fallen/DDEngine/Source/figure.cpp)
// When true, scales down the next MESH_draw_poly call (used for grenade-in-hand rendering).
extern UBYTE kludge_shrink;

// --- Reflection draw state (P2-I, GPU) ---
// FIGURE_draw_reflection now uses the bind-space skinning path and computes
// the screen-space bbox from per-bone AABB corners (no per-vertex CPU walk).
// The legacy FIGURE_rpoint[] / FIGURE_Rpoint / FIGURE_MAX_RPOINTS storage
// is gone — only the bbox + water-plane height globals remain (consumed by
// aeng.cpp's wibble intersection step).

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

#endif // ENGINE_GRAPHICS_GEOMETRY_FIGURE_GLOBALS_H
