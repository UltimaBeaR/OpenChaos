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
// Note: MM_pcFadeTable/Tint/pMatrix/Vertex/pNormal are POINTERS into aligned static arrays
// allocated in figure.cpp (ALIGNED_STATIC_ARRAY macro). They are exported here so that
// chunk 2+ code in old/figure.cpp can access them across translation units.
// They are initialised at file startup before any code runs.

// uc_orig: MM_pcFadeTable (fallen/DDEngine/Source/figure.cpp)
ULONG* MM_pcFadeTable = NULL;

// uc_orig: MM_pcFadeTableTint (fallen/DDEngine/Source/figure.cpp)
ULONG* MM_pcFadeTableTint = NULL;

// uc_orig: MM_pMatrix (fallen/DDEngine/Source/figure.cpp)
GEMatrix* MM_pMatrix = NULL;

// uc_orig: MM_Vertex (fallen/DDEngine/Source/figure.cpp)
GEVertex* MM_Vertex = NULL;

// uc_orig: MM_pNormal (fallen/DDEngine/Source/figure.cpp)
float* MM_pNormal = NULL;

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

// uc_orig: FIGURE_dhpr_rdata2 (fallen/DDEngine/Source/figure.cpp)
structFIGURE_dhpr_rdata2 FIGURE_dhpr_rdata2[MAX_RECURSION];

// uc_orig: MMBodyParts_pMatrix (fallen/DDEngine/Source/figure.cpp)
GEMatrix* MMBodyParts_pMatrix = NULL;

// uc_orig: MMBodyParts_pNormal (fallen/DDEngine/Source/figure.cpp)
float* MMBodyParts_pNormal = NULL;

// --- Misc character draw state ---

// uc_orig: kludge_shrink (fallen/DDEngine/Source/figure.cpp)
UBYTE kludge_shrink = UC_FALSE;

// --- Reflection draw state ---

// uc_orig: FIGURE_rpoint (fallen/DDEngine/Source/figure.cpp)
FIGURE_Rpoint FIGURE_rpoint[FIGURE_MAX_RPOINTS];

// uc_orig: FIGURE_rpoint_upto (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_rpoint_upto = 0;

// uc_orig: FIGURE_reflect_x1 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_x1 = 0;

// uc_orig: FIGURE_reflect_y1 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_y1 = 0;

// uc_orig: FIGURE_reflect_x2 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_x2 = 0;

// uc_orig: FIGURE_reflect_y2 (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_reflect_y2 = 0;

// uc_orig: FIGURE_reflect_height (fallen/DDEngine/Source/figure.cpp)
float FIGURE_reflect_height = 0.0f;

// Backing storage for the aligned MM_* pointers (not original entities — these replace
// the ALIGNED_STATIC_ARRAY macro from figure.cpp which generated equivalent static storage).
// Kept here so all global state is visible in _globals files per project rules.
static char cMM_pcFadeTableStorage[4 + 128 * sizeof(ULONG)];
static char cMM_pcFadeTableTintStorage[4 + 128 * sizeof(ULONG)];
static char cMM_pMatrixStorage[32 + 1 * sizeof(GEMatrix)];
static char cMM_VertexStorage[32 + 4 * sizeof(GEVertex)];
static char cMM_pNormalStorage[8 + 4 * sizeof(float)];
static char cMMBodyParts_pMatrixStorage[32 + MAX_NUM_BODY_PARTS_AT_ONCE * sizeof(GEMatrix)];
static char cMMBodyParts_pNormalStorage[8 + MAX_NUM_BODY_PARTS_AT_ONCE * 4 * sizeof(float)];

// Initialises MM_* global pointers to aligned addresses within the above storage.
// Replaces ALIGNED_STATIC_ARRAY(static ...) declarations from the original figure.cpp.
// This constructor runs before main, guaranteeing MM_* are valid for any code that calls
// BuildMMLightingTable or FIGURE_draw_prim_tween_*.
namespace {
    struct MMLightingTableInit {
        MMLightingTableInit()
        {
            MM_pcFadeTable      = (ULONG*)(((uintptr_t)cMM_pcFadeTableStorage      + 3)  & ~(uintptr_t)3);
            MM_pcFadeTableTint  = (ULONG*)(((uintptr_t)cMM_pcFadeTableTintStorage  + 3)  & ~(uintptr_t)3);
            MM_pMatrix          = (GEMatrix*)(((uintptr_t)cMM_pMatrixStorage          + 31) & ~(uintptr_t)31);
            MM_Vertex           = (GEVertex*)(((uintptr_t)cMM_VertexStorage           + 31) & ~(uintptr_t)31);
            MM_pNormal          = (float*)(    ((uintptr_t)cMM_pNormalStorage           + 7)  & ~(uintptr_t)7);
            MMBodyParts_pMatrix = (GEMatrix*)(((uintptr_t)cMMBodyParts_pMatrixStorage  + 31) & ~(uintptr_t)31);
            MMBodyParts_pNormal = (float*)(    ((uintptr_t)cMMBodyParts_pNormalStorage   + 7)  & ~(uintptr_t)7);
        }
    } g_MMLightingTableInit;
}
