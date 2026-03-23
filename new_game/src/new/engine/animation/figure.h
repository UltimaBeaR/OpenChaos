#ifndef ENGINE_ANIMATION_FIGURE_H
#define ENGINE_ANIMATION_FIGURE_H

#include "engine/animation/figure_globals.h"
#include "fallen/Headers/prim.h" // Temporary: Matrix33, TomsPrimObject

struct Thing;

// --- Microsoft Index List Optimizer types ---
// Used by MSMesh and MSOptimizeIndexedList in figure.cpp.

// uc_orig: EdgeList (fallen/DDEngine/Source/figure.cpp)
struct EdgeList {
    WORD wPt1, wPt2;
    WORD wMidPt;
};

// uc_orig: tagIndexTri (fallen/DDEngine/Source/figure.cpp)
typedef struct tagIndexTri {
    int v1, v2, v3;
    int done;
} INDEXTRISTRUCT;

// uc_orig: tagIndexVert (fallen/DDEngine/Source/figure.cpp)
typedef struct tagIndexVert {
    WORD wIndex;
    int nShareCount;
    int rgSharedTris[30];
} INDEXVERTSTRUCT;

// uc_orig: MSMesh (fallen/DDEngine/Source/figure.cpp)
// Helper class for the Microsoft strip optimiser. Represents a mesh as adjacency lists.
class MSMesh {
private:
    INDEXTRISTRUCT* m_aTri;
    int m_nNumTri;
    int m_nMaxTri;
    INDEXVERTSTRUCT* m_aVert;
    int m_nNumVert;
    int m_nMaxVert;

    int NextTri(int v1, int v2, int* v3);
    void ClearTempDone(int nValue);
    int StripLen(int startTri, int dir, int setit, WORD* pwOut, int nMaxLen);

public:
    MSMesh();
    ~MSMesh();
    void Clear()
    {
        m_nNumTri = 0;
        m_nNumVert = 0;
    }
    int AddTri(WORD wV1, WORD wV2, WORD wV3);
    int GetStrip(WORD* pwV, int maxlen);
    int NumVertices() { return m_nNumVert; }
    int FindOrAddVertex(WORD wVert);
    int SetSize(int nTriangles);
};

// Paints a solid 50x50 colour block on the front DirectDraw surface for crash diagnostics.
// uc_orig: DeadAndBuried (fallen/DDEngine/Source/figure.cpp)
void DeadAndBuried(DWORD dwColour);

// Reorders pwIndices (nTriangles*3 indices) into vertex-cache-friendly strips in-place.
// uc_orig: MSOptimizeIndexedList (fallen/DDEngine/Source/figure.cpp)
BOOL MSOptimizeIndexedList(WORD* pwIndices, int nTriangles);

// Builds MM_pcFadeTable and MM_pcFadeTableTint for the current frame's lighting.
// p: if non-null, the character is on fire (darkens with soot).
// colour_and: applied to MM_pcFadeTableTint for tinted texture materials.
// uc_orig: BuildMMLightingTable (fallen/DDEngine/Source/figure.cpp)
void BuildMMLightingTable(Pyro* p, DWORD colour_and = 0xffffffff);

// Loads the flame colour palette from data\flames1.pal.
// Must be called once during game startup before draw_flames or draw_flame_element.
// uc_orig: init_flames (fallen/DDEngine/Source/figure.cpp)
void init_flames();

// Draws a column of animated steam/vapour sprites above world position (x,y,z).
// lod: number of sprites to draw, capped at 40.
// uc_orig: draw_steam (fallen/DDEngine/Source/figure.cpp)
void draw_steam(int x, int y, int z, int lod);

// Draws layered fire sprites at world position (x,y,z).
// lod: sprite count. offset: seeds per-fire variation.
// uc_orig: draw_flames (fallen/DDEngine/Source/figure.cpp)
void draw_flames(int x, int y, int z, int lod, int offset);

// Draws a single animated fire/smoke sprite for one animation step.
// c0: frame index, base: 0=FLAMES, 1=FLAMES2, rand: 1=random seed variation.
// uc_orig: draw_flame_element (fallen/DDEngine/Source/figure.cpp)
void draw_flame_element(int x, int y, int z, int c0, unsigned char base, unsigned char rand);

// Builds a 3x3 rotation matrix from pitch/yaw/roll (game fixed-point 0..2047 = 0..360 degrees).
// Matrix elements are scaled by 32768 (fixed-point 1.0).
// uc_orig: FIGURE_rotate_obj2 (fallen/DDEngine/Source/figure.cpp)
void FIGURE_rotate_obj2(SLONG pitch, SLONG yaw, SLONG roll, Matrix33* r3);

// Legacy wrapper for FIGURE_rotate_obj2 (old integer-only path is dead code, returns immediately).
// uc_orig: FIGURE_rotate_obj (fallen/DDEngine/Source/figure.cpp)
void FIGURE_rotate_obj(SLONG xangle, SLONG yangle, SLONG zangle, Matrix33* r3);

// Resolves the D3D texture page for a prim face (tri or quad).
// Returns a UWORD combining page index and flag bits (TEXTURE_PAGE_FLAG_JACKET, _OFFSET, _TINT, etc.).
// uc_orig: FIGURE_find_face_D3D_texture_page (fallen/DDEngine/Source/figure.cpp)
unsigned short FIGURE_find_face_D3D_texture_page(int iFaceNum, bool bTri);

// Frees the D3D vertex/index memory for one LRU cache slot and clears its metadata.
// uc_orig: FIGURE_clean_LRU_slot (fallen/DDEngine/Source/figure.cpp)
void FIGURE_clean_LRU_slot(int iSlot);

// Flushes all cached TomsPrimObjects. Call at level end to release GPU/CPU mesh memory.
// uc_orig: FIGURE_clean_all_LRU_slots (fallen/DDEngine/Source/figure.cpp)
void FIGURE_clean_all_LRU_slots(void);

// Adds pPrimObj to the LRU cache, evicting the oldest entry if there is no room.
// uc_orig: FIGURE_find_and_clean_prim_queue_item (fallen/DDEngine/Source/figure.cpp)
void FIGURE_find_and_clean_prim_queue_item(TomsPrimObject* pPrimObj, int iThrashIndex = 0);

// Updates the last-used game turn for a cached prim mesh to prevent premature eviction.
// uc_orig: FIGURE_touch_LRU_of_object (fallen/DDEngine/Source/figure.cpp)
void FIGURE_touch_LRU_of_object(TomsPrimObject* pPrimObj);

// Begins lazy D3D compilation of a TomsPrimObject: allocates staging CPU buffers.
// uc_orig: FIGURE_TPO_init_3d_object (fallen/DDEngine/Source/figure.cpp)
void FIGURE_TPO_init_3d_object(TomsPrimObject* pPrimObj);

// Registers one prim into the active TomsPrimObject compilation context.
// Call between FIGURE_TPO_init_3d_object and FIGURE_TPO_finish_3d_object.
// uc_orig: FIGURE_TPO_add_prim_to_current_object (fallen/DDEngine/Source/figure.cpp)
void FIGURE_TPO_add_prim_to_current_object(SLONG prim, UBYTE ubSubObjectNumber);

// Finalises the TomsPrimObject pPrimObj: groups faces by material, deduplicates vertices,
// builds index/strip lists, copies to a heap block, registers in the LRU cache.
// uc_orig: FIGURE_TPO_finish_3d_object (fallen/DDEngine/Source/figure.cpp)
void FIGURE_TPO_finish_3d_object(TomsPrimObject* pPrimObj, int iThrashIndex = 0);

// Convenience wrapper: init + add + finish for a single-prim D3D object (lazy-compiles D3DObj[prim]).
// uc_orig: FIGURE_generate_D3D_object (fallen/DDEngine/Source/figure.cpp)
void FIGURE_generate_D3D_object(SLONG prim);

// Software-path body-part renderer with keyframe interpolation (lerp offsets, slerp rotations).
// Also sets up the D3D MultiMatrix for the GPU hardware path on opaque non-clipped materials.
// uc_orig: FIGURE_draw_prim_tween (fallen/DDEngine/Source/figure.cpp)
void FIGURE_draw_prim_tween(
    SLONG prim,
    SLONG x,
    SLONG y,
    SLONG z,
    SLONG tween,
    struct GameKeyFrameElement* anim_info,
    struct GameKeyFrameElement* anim_info_next,
    struct Matrix33* rot_mat,
    SLONG off_dx,
    SLONG off_dy,
    SLONG off_dz,
    ULONG colour,
    ULONG specular,
    CMatrix33* parent_base_mat,
    Matrix31* parent_base_pos,
    Matrix33* parent_curr_mat,
    Matrix31* parent_curr_pos,
    Matrix33* end_mat,
    Matrix31* end_pos,
    Thing* p_thing,
    SLONG part_number = 0xffffffff,
    ULONG colour_and = 0xffffffff);

// --- The following will be added in future chunks as they are migrated ---
// FIGURE_draw_prim_tween_warped          (chunk 3)
// FIGURE_draw_hierarchical_prim_recurse  (chunk 3)
// FIGURE_draw                            (chunk 4)
// ANIM_obj_draw                          (chunk 4)
// ANIM_obj_draw_warped                   (chunk 4)
// FIGURE_draw_prim_tween_reflection      (chunk 4)
// FIGURE_draw_reflection                 (chunk 5)
// FIGURE_draw_prim_tween_person_only     (chunk 5)

#endif // ENGINE_ANIMATION_FIGURE_H
