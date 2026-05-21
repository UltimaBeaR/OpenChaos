#include "engine/graphics/geometry/figure_globals.h"
#include "engine/graphics/geometry/figure.h"

// uc_orig: FIGURE_alpha (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_alpha = 255;

// uc_orig: steam_seed (fallen/DDEngine/Source/figure.cpp)
SLONG steam_seed = 0;

// uc_orig: fire_pal (fallen/DDEngine/Source/figure.cpp)
UBYTE fire_pal[768];

// uc_orig: jacket_lookup (fallen/DDEngine/Source/figure.cpp)
UWORD jacket_lookup[4][8] = {
    { 64 + 21, 10 * 64 + 2, 10 * 64 + 2, 10 * 64 + 32 },
    { 64 + 22, 10 * 64 + 3, 10 * 64 + 3, 10 * 64 + 33 },
    { 64 + 24, 10 * 64 + 4, 10 * 64 + 4, 10 * 64 + 36 },
    { 64 + 25, 10 * 64 + 5, 10 * 64 + 5, 10 * 64 + 37 }
};

// uc_orig: D3DObj (fallen/DDEngine/Source/figure.cpp)
TomsPrimObject D3DObj[MAX_NUMBER_D3D_PRIMS];

// uc_orig: m_fObjectBoundingSphereRadius (fallen/DDEngine/Source/figure.cpp)
float m_fObjectBoundingSphereRadius[MAX_NUMBER_D3D_PRIMS];

// uc_orig: D3DPeopleObj (fallen/DDEngine/Source/figure.cpp)
TomsPrimObject D3DPeopleObj[MAX_NUMBER_D3D_PEOPLE];

// Non-person consolidated rig cache. Empty slot = chunk==NULL. See
// figure_globals.h for the lookup scheme.
AnimObjKey     D3DAnimObjKeys[MAX_NUMBER_D3D_ANIMALS] = {};
TomsPrimObject D3DAnimObj    [MAX_NUMBER_D3D_ANIMALS];

// uc_orig: m_iLRUQueueSize (fallen/DDEngine/Source/figure.cpp)
int m_iLRUQueueSize = 0;

// uc_orig: ptpoLRUQueue (fallen/DDEngine/Source/figure.cpp)
TomsPrimObject* ptpoLRUQueue[PRIM_LRU_QUEUE_LENGTH];

// uc_orig: dwGameTurnLastUsed (fallen/DDEngine/Source/figure.cpp)
DWORD dwGameTurnLastUsed[PRIM_LRU_QUEUE_LENGTH];

// uc_orig: m_dwSizeOfQueue (fallen/DDEngine/Source/figure.cpp)
DWORD m_dwSizeOfQueue = 0;

// --- TPO working state ---
// uc_orig: TPO_pVert (fallen/DDEngine/Source/figure.cpp)
GEVertex* TPO_pVert = NULL;
// uc_orig: TPO_pStripIndices (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pStripIndices = NULL;
// uc_orig: TPO_pListIndices (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pListIndices = NULL;
// uc_orig: TPO_piVertexRemap (fallen/DDEngine/Source/figure.cpp)
int* TPO_piVertexRemap = NULL;
// uc_orig: TPO_piVertexLinks (fallen/DDEngine/Source/figure.cpp)
int* TPO_piVertexLinks = NULL;
// uc_orig: TPO_pCurVertex (fallen/DDEngine/Source/figure.cpp)
GEVertex* TPO_pCurVertex = NULL;
// uc_orig: TPO_pCurStripIndex (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pCurStripIndex = NULL;
// uc_orig: TPO_pCurListIndex (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pCurListIndex = NULL;
// uc_orig: TPO_pPrimObj (fallen/DDEngine/Source/figure.cpp)
TomsPrimObject* TPO_pPrimObj = NULL;
// uc_orig: TPO_iNumListIndices (fallen/DDEngine/Source/figure.cpp)
int TPO_iNumListIndices = 0;
// uc_orig: TPO_iNumStripIndices (fallen/DDEngine/Source/figure.cpp)
int TPO_iNumStripIndices = 0;
// uc_orig: TPO_iNumVertices (fallen/DDEngine/Source/figure.cpp)
int TPO_iNumVertices = 0;
// uc_orig: TPO_iNumPrims (fallen/DDEngine/Source/figure.cpp)
int TPO_iNumPrims = 0;
// uc_orig: TPO_PrimObjects (fallen/DDEngine/Source/figure.cpp)
SLONG TPO_PrimObjects[TPO_MAX_NUMBER_PRIMS];
// uc_orig: TPO_ubPrimObjMMIndex (fallen/DDEngine/Source/figure.cpp)
UBYTE TPO_ubPrimObjMMIndex[TPO_MAX_NUMBER_PRIMS];
// uc_orig: TPO_iPrimObjIndexOffset (fallen/DDEngine/Source/figure.cpp)
int TPO_iPrimObjIndexOffset[TPO_MAX_NUMBER_PRIMS + 1];

// --- D3D MultiMatrix lighting working state ---

// Compressed half-Lambert ramp endpoints (see figure_globals.h). Zero-
// initialised — BuildMMLightingTable writes them every frame per character
// before any draw call that reads them.
float MM_FadeStart[3]     = { 0.0f, 0.0f, 0.0f };
float MM_FadeStep[3]      = { 0.0f, 0.0f, 0.0f };
float MM_FadeStartTint[3] = { 0.0f, 0.0f, 0.0f };
float MM_FadeStepTint[3]  = { 0.0f, 0.0f, 0.0f };

// uc_orig: MM_Vertex (fallen/DDEngine/Source/figure.cpp)
GEVertex* MM_Vertex = NULL;

// uc_orig: MM_vLightDir (fallen/DDEngine/Source/figure.cpp)
GEVector MM_vLightDir;

// uc_orig: MM_bLightTableAlreadySetUp (fallen/DDEngine/Source/figure.cpp)
bool MM_bLightTableAlreadySetUp = false;

// uc_orig: mesh (fallen/DDEngine/Source/figure.cpp)
// File-private MSMesh instance used during FIGURE_TPO_finish_3d_object.
// Renamed g_mesh to avoid conflict with potential future global 'mesh'.
MSMesh g_mesh;

// --- Hierarchical body-part draw state ---

// uc_orig: FIGURE_dhpr_data (fallen/DDEngine/Source/figure.cpp)
structFIGURE_dhpr_data FIGURE_dhpr_data;

// uc_orig: FIGURE_dhpr_rdata1 (fallen/DDEngine/Source/figure.cpp)
structFIGURE_dhpr_rdata1 FIGURE_dhpr_rdata1[MAX_RECURSION];

// --- Misc character draw state ---

// uc_orig: kludge_shrink (fallen/DDEngine/Source/figure.cpp)
UBYTE kludge_shrink = UC_FALSE;

// --- Reflection draw state (P2-I, GPU) ---
// FIGURE_rpoint[] / FIGURE_rpoint_upto removed — the bind-space GPU path
// computes screen-space bbox directly from per-bone AABB (figure.cpp).

// uc_orig: FIGURE_reflect_x1 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_x1 = 0;

// uc_orig: FIGURE_reflect_y1 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_y1 = 0;

// uc_orig: FIGURE_reflect_x2 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_x2 = 0;

// uc_orig: FIGURE_reflect_y2 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_y2 = 0;

// Backing storage for the aligned MM_Vertex pointer (not an original
// entity — replaces the ALIGNED_STATIC_ARRAY macro from figure.cpp).
// Kept here so all global state is visible in _globals files per project
// rules.
static char cMM_VertexStorage[32 + 4 * sizeof(GEVertex)];

namespace {
struct MMLightingTableInit {
    MMLightingTableInit()
    {
        MM_Vertex = (GEVertex*)(((uintptr_t)cMM_VertexStorage + 31) & ~(uintptr_t)31);
    }
} g_MMLightingTableInit;
}
