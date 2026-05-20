#ifndef ENGINE_GRAPHICS_GEOMETRY_FIGURE_H
#define ENGINE_GRAPHICS_GEOMETRY_FIGURE_H

#include "engine/graphics/geometry/figure_globals.h"
#include "buildings/prim_types.h" // Matrix33 (via fmatrix.h), TomsPrimObject
#include "effects/combat/pyro.h" // Pyro
#include "things/core/drawtype.h" // DrawTween

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

// 15-body-part character renderer. Builds a consolidated TomsPrimObject
// on first use, then draws the whole rig in one pass through the
// world-skin shader (one ge_skin_world_draw_range call per material).
// Held items (gun / muzzle flash) for armed characters are drawn as
// rigid meshes at the hand bone's world transform.
// uc_orig: FIGURE_draw_hierarchical_prim_recurse (fallen/DDEngine/Source/figure.cpp)
void FIGURE_draw_hierarchical_prim_recurse(Thing* p_person);

// Top-level entry point: renders one character Thing* for the current
// frame. Calls FIGURE_draw_hierarchical_prim_recurse for the 15-bone
// person rig (the only person rig that ships).
// uc_orig: FIGURE_draw (fallen/DDEngine/Source/figure.cpp)
void FIGURE_draw(Thing* p_thing);

// Draws a flat-skeleton creature (DT_ANIM_PRIM: bats, Bane, Balrog,
// Gargoyle) through the same world-skin path persons use — a single
// consolidated VBO per (chunk, start_object) cached in D3DAnimObj[],
// one ge_skin_world_draw_range call per material.
// uc_orig: ANIM_obj_draw (fallen/DDEngine/Source/figure.cpp)
void ANIM_obj_draw(Thing* p_thing, DrawTween* dt);

// Get the consolidated skin mesh for any skinned Thing — both the
// 15-bone person rig (D3DPeopleObj[]) and flat-skeleton creatures
// (D3DAnimObj[]). Lazily builds the TomsPrimObject AND bind-space VBO
// if neither is built yet, so callers running BEFORE the body draw
// (notably SMAP_person_gpu for character shadows) get the same mesh
// the body draw will use later this frame.
//
// On success: *out_mesh = the consolidated GESkinMesh*; *out_bone_aabb =
// pointer to bone_count × 6 floats (per-bone bind-space AABB) for shadow
// projection box; *out_bone_count = number of bones; *out_chunk = the
// thing's anim chunk; *out_bind_inv = bone_count inverse-bind matrices.
// Out params may individually be NULL if the caller doesn't need them.
// Returns false (leaving outs untouched) on missing pose / chunk data.
struct GESkinMesh;
struct TomsPrimObject;
bool FIGURE_get_skin_mesh_for_thing(Thing* p_thing,
                                    GESkinMesh**             out_mesh,
                                    const float**            out_bone_aabb,
                                    int*                     out_bone_count,
                                    const GameKeyFrameChunk** out_chunk,
                                    const GEMatrix**         out_bind_inv,
                                    TomsPrimObject**         out_prim_obj = nullptr);

// Top-level water reflection renderer (P2-I, GPU). One ge_skin_reflect_draw_range
// call per material — uses the same bind-space VBO + skin palette the body
// and shadow share, with mirror Y applied in the vertex shader.
// 15-bone people only (matches the legacy filter).
void FIGURE_draw_reflection(Thing* p_thing, SLONG height);

#endif // ENGINE_GRAPHICS_GEOMETRY_FIGURE_H
