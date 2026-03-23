#include "engine/animation/figure_globals.h"
#include "engine/animation/figure.h"

// uc_orig: FIGURE_alpha (fallen/DDEngine/Source/figure.cpp)
SLONG FIGURE_alpha = 255;

// uc_orig: steam_seed (fallen/DDEngine/Source/figure.cpp)
SLONG steam_seed = 0;

// uc_orig: body_part_upper (fallen/DDEngine/Source/figure.cpp)
UBYTE body_part_upper[15] = {
    0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
};

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
D3DVERTEX* TPO_pVert = NULL;
// uc_orig: TPO_pStripIndices (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pStripIndices = NULL;
// uc_orig: TPO_pListIndices (fallen/DDEngine/Source/figure.cpp)
UWORD* TPO_pListIndices = NULL;
// uc_orig: TPO_piVertexRemap (fallen/DDEngine/Source/figure.cpp)
int* TPO_piVertexRemap = NULL;
// uc_orig: TPO_piVertexLinks (fallen/DDEngine/Source/figure.cpp)
int* TPO_piVertexLinks = NULL;
// uc_orig: TPO_pCurVertex (fallen/DDEngine/Source/figure.cpp)
D3DVERTEX* TPO_pCurVertex = NULL;
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
D3DCOLOR* MM_pcFadeTable = NULL;

// uc_orig: MM_pcFadeTableTint (fallen/DDEngine/Source/figure.cpp)
D3DCOLOR* MM_pcFadeTableTint = NULL;

// uc_orig: MM_pMatrix (fallen/DDEngine/Source/figure.cpp)
D3DMATRIX* MM_pMatrix = NULL;

// uc_orig: MM_Vertex (fallen/DDEngine/Source/figure.cpp)
D3DVERTEX* MM_Vertex = NULL;

// uc_orig: MM_pNormal (fallen/DDEngine/Source/figure.cpp)
float* MM_pNormal = NULL;

// uc_orig: MM_vLightDir (fallen/DDEngine/Source/figure.cpp)
D3DVECTOR MM_vLightDir;

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

// uc_orig: part_type (fallen/DDEngine/Source/figure.cpp)
UBYTE part_type[15] = {
    2, // pelvis
    2, // lfemur
    2, // ltibia
    4, // lfoot
    3, // torso
    3, // rhumorus
    3, // rradius
    6, // rhand
    3, // lhumorus
    3, // lradius
    6, // lhand
    1, // skull
    2, // rfemur
    2, // rtibia
    4  // rfoot
};

// uc_orig: local_seed (fallen/DDEngine/Source/figure.cpp)
ULONG local_seed = 0;

// uc_orig: jacket_col (fallen/DDEngine/Source/figure.cpp)
ULONG jacket_col = 0;

// uc_orig: leg_col (fallen/DDEngine/Source/figure.cpp)
ULONG leg_col = 0;

// uc_orig: MMBodyParts_pMatrix (fallen/DDEngine/Source/figure.cpp)
D3DMATRIX* MMBodyParts_pMatrix = NULL;

// uc_orig: MMBodyParts_pNormal (fallen/DDEngine/Source/figure.cpp)
float* MMBodyParts_pNormal = NULL;

// --- Misc character draw state ---

// uc_orig: peep_recol (fallen/DDEngine/Source/figure.cpp)
PeepRecolEntry peep_recol[16] = {
    { 12, 64, 64 },
    { 64, 28, 64 },
    { 64, 4, 55 },
    { 54, 32, 22 },
    { 42, 14, 12 },
    { 32, 42, 64 },
    { 22, 64, 54 },
    { 64, 14, 32 },
    { 34, 32, 64 },
    { 0, 32, 64 },
    { 32, 64, 0 },
    { 64, 0, 32 },
    { 56, 30, 0 },
    { 0, 40, 56 },
    { 33, 26, 70 },
    { 56, 36, 0 },
};

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
static char cMM_pcFadeTableStorage[4 + 128 * sizeof(D3DCOLOR)];
static char cMM_pcFadeTableTintStorage[4 + 128 * sizeof(D3DCOLOR)];
static char cMM_pMatrixStorage[32 + 1 * sizeof(D3DMATRIX)];
static char cMM_VertexStorage[32 + 4 * sizeof(D3DVERTEX)];
static char cMM_pNormalStorage[8 + 4 * sizeof(float)];
static char cMMBodyParts_pMatrixStorage[32 + MAX_NUM_BODY_PARTS_AT_ONCE * sizeof(D3DMATRIX)];
static char cMMBodyParts_pNormalStorage[8 + MAX_NUM_BODY_PARTS_AT_ONCE * 4 * sizeof(float)];

// Initialises MM_* global pointers to aligned addresses within the above storage.
// Replaces ALIGNED_STATIC_ARRAY(static ...) declarations from the original figure.cpp.
// This constructor runs before main, guaranteeing MM_* are valid for any code that calls
// BuildMMLightingTable or FIGURE_draw_prim_tween_*.
namespace {
    struct MMLightingTableInit {
        MMLightingTableInit()
        {
            MM_pcFadeTable      = (D3DCOLOR*)(((DWORD)cMM_pcFadeTableStorage      + 3)  & ~3u);
            MM_pcFadeTableTint  = (D3DCOLOR*)(((DWORD)cMM_pcFadeTableTintStorage  + 3)  & ~3u);
            MM_pMatrix          = (D3DMATRIX*)(((DWORD)cMM_pMatrixStorage          + 31) & ~31u);
            MM_Vertex           = (D3DVERTEX*)(((DWORD)cMM_VertexStorage           + 31) & ~31u);
            MM_pNormal          = (float*)(    ((DWORD)cMM_pNormalStorage           + 7)  & ~7u);
            MMBodyParts_pMatrix = (D3DMATRIX*)(((DWORD)cMMBodyParts_pMatrixStorage  + 31) & ~31u);
            MMBodyParts_pNormal = (float*)(    ((DWORD)cMMBodyParts_pNormalStorage   + 7)  & ~7u);
        }
    } g_MMLightingTableInit;
}
