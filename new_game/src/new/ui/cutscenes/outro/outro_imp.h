#ifndef UI_CUTSCENES_OUTRO_OUTRO_IMP_H
#define UI_CUTSCENES_OUTRO_OUTRO_IMP_H

#include "fallen/outro/always.h" // Temporary: outro uses its own type definitions
#include "fallen/outro/os.h"     // Temporary: OS_Texture*, OS_Buffer* — until os.cpp is migrated

// Alpha blending modes for IMP_Mat.
// uc_orig: IMP_ALPHA_FILTERED (fallen/outro/imp.h)
#define IMP_ALPHA_FILTERED 1
// uc_orig: IMP_ALPHA_ADDITIVE (fallen/outro/imp.h)
#define IMP_ALPHA_ADDITIVE 2

// Sidedness modes for IMP_Mat.
// uc_orig: IMP_SIDED_SINGLE (fallen/outro/imp.h)
#define IMP_SIDED_SINGLE 1
// uc_orig: IMP_SIDED_DOUBLE (fallen/outro/imp.h)
#define IMP_SIDED_DOUBLE 2

// Face flags. EDGE_A/B/C mark which edges of the triangle are "real" edges (visible in mesh).
// A face with all three edge flags set is a standalone triangle; otherwise it is part of a quad.
// BACKFACE is not set by the importer but may be set externally.
// uc_orig: IMP_FACE_FLAG_EDGE (fallen/outro/imp.h)
#define IMP_FACE_FLAG_EDGE    (1 << 0)
// uc_orig: IMP_FACE_FLAG_EDGE_A (fallen/outro/imp.h)
#define IMP_FACE_FLAG_EDGE_A  (1 << 0)
// uc_orig: IMP_FACE_FLAG_EDGE_B (fallen/outro/imp.h)
#define IMP_FACE_FLAG_EDGE_B  (1 << 1)
// uc_orig: IMP_FACE_FLAG_EDGE_C (fallen/outro/imp.h)
#define IMP_FACE_FLAG_EDGE_C  (1 << 2)
// uc_orig: IMP_FACE_FLAG_QUADDED (fallen/outro/imp.h)
#define IMP_FACE_FLAG_QUADDED (1 << 3)
// uc_orig: IMP_FACE_FLAG_BACKFACE (fallen/outro/imp.h)
#define IMP_FACE_FLAG_BACKFACE (1 << 4)

// Per-material properties. Each material can have a texture and an optional bumpmap.
// uc_orig: IMP_Mat (fallen/outro/imp.h)
typedef struct
{
    float r;
    float g;
    float b;
    float shininess;
    float shinstr;
    UBYTE alpha;       // IMP_ALPHA_*
    UBYTE sided;       // IMP_SIDED_*
    UBYTE has_texture; // Non-zero if a texture is present
    UBYTE has_bumpmap; // Non-zero if a bumpmap is present
    CBYTE tname[32];   // Texture filename or "none"
    CBYTE bname[32];   // Bumpmap filename or "none"

    OS_Texture* ot_tex;  // Loaded texture object
    OS_Texture* ot_bpos; // Bumpmap (positive version)
    OS_Texture* ot_bneg; // Bumpmap (1 - bumpmap, negative version)

    OS_Buffer* ob;

} IMP_Mat;

// World-space vertex position plus lightmap UV (lu, lv).
// uc_orig: IMP_Vert (fallen/outro/imp.h)
typedef struct
{
    float x;
    float y;
    float z;

    float lu;
    float lv;

} IMP_Vert;

// Texture coordinate vertex (UV only).
// uc_orig: IMP_Tvert (fallen/outro/imp.h)
typedef struct
{
    float u;
    float v;

} IMP_Tvert;

// Shared vertex: position index + UVs + per-vertex normal + tangent basis for bumpmapping.
// Only faces in the same smoothing group with matching UVs and similar normals share a vertex.
// uc_orig: IMP_Svert (fallen/outro/imp.h)
typedef struct
{
    float u;
    float v;

    float nx;
    float ny;
    float nz;

    float dxdu; // du tangent vector (partial derivative of world pos w.r.t. u)
    float dydu;
    float dzdu;

    float dxdv; // dv tangent vector
    float dydv;
    float dzdv;

    UWORD vert; // Index into IMP_Mesh.vert[]
    UWORD mat;  // Material index for faces that use this shared vertex

    ULONG colour;   // Computed lighting colour
    ULONG specular;

    float lu; // Lightmap U
    float lv; // Lightmap V

    float du; // Bumpmap U offset
    float dv; // Bumpmap V offset

} IMP_Svert;

// Triangle face: three vertex indices, three texture-vertex indices, three shared-vertex indices,
// material index, edge flags, smoothing group, and face normal + tangent basis.
// uc_orig: IMP_Face (fallen/outro/imp.h)
typedef struct
{
    UWORD v[3]; // Index into IMP_Mesh.vert[]
    UWORD t[3]; // Index into IMP_Mesh.tvert[]
    UWORD s[3]; // Index into IMP_Mesh.svert[]
    UBYTE mat;  // Material index
    UBYTE flag; // IMP_FACE_FLAG_*
    ULONG group; // Smoothing group bitmask

    float nx; // Face normal
    float ny;
    float nz;

    float dxdu; // Face-level texture tangent
    float dydu;
    float dzdu;

    float dxdv;
    float dydv;
    float dzdv;

} IMP_Face;

// Quad: four vertex indices defining two merged coplanar triangles.
// uc_orig: IMP_Quad (fallen/outro/imp.h)
typedef struct
{
    UWORD v[4];

} IMP_Quad;

// Mesh edge: two endpoint vertices and the (up to two) faces that share it.
// f2 == 0xffff means the edge belongs to only one face (boundary edge).
// uc_orig: IMP_Edge (fallen/outro/imp.h)
typedef struct
{
    UWORD v1;
    UWORD v2;
    UWORD f1;
    UWORD f2; // 0xffff = boundary edge (one face only)

} IMP_Edge;

// Visible wireframe edge: two vertex indices. Only edges exported from Max are stored here.
// uc_orig: IMP_Line (fallen/outro/imp.h)
typedef struct
{
    UWORD v1;
    UWORD v2;

} IMP_Line;

// Complete imported mesh. All pointer arrays are heap-allocated; use IMP_free() to release.
// uc_orig: IMP_Mesh (fallen/outro/imp.h)
typedef struct
{
    SLONG valid;       // UC_TRUE if the mesh was loaded successfully
    CBYTE name[32];    // Name of the first object in the SEX file
    SLONG num_mats;
    SLONG num_verts;
    SLONG num_tverts;
    SLONG num_faces;
    SLONG num_sverts;
    SLONG num_quads;
    SLONG num_edges;
    SLONG num_lines;
    IMP_Mat*   mat;
    IMP_Vert*  vert;
    IMP_Tvert* tvert;
    IMP_Face*  face;
    IMP_Svert* svert;
    IMP_Quad*  quad;
    IMP_Edge*  edge;
    IMP_Line*  line;

    float min_x; // Axis-aligned bounding box
    float min_y;
    float min_z;
    float max_x;
    float max_y;
    float max_z;
    float radius; // Bounding sphere radius

    IMP_Vert*  old_vert;  // Backup copy of vert[] for rotation (after MF_backup)
    IMP_Svert* old_svert; // Backup copy of svert[]

} IMP_Mesh;

// Loads a SEX (3ds-exported text) file and returns a fully built IMP_Mesh.
// Returns a mesh with valid==UC_FALSE on failure.
// uc_orig: IMP_load (fallen/outro/imp.h)
IMP_Mesh IMP_load(CBYTE* fname, float scale = 1.0F);

// Frees all heap-allocated arrays in the mesh. Safe to call on a partially-initialised mesh.
// uc_orig: IMP_free (fallen/outro/imp.h)
void IMP_free(IMP_Mesh* im);

// Saves/loads a binary (pre-processed) version of the mesh for faster loading.
// Returns UC_FALSE on failure.
// uc_orig: IMP_binary_save (fallen/outro/imp.h)
SLONG IMP_binary_save(CBYTE* fname, IMP_Mesh* im);
// uc_orig: IMP_binary_load (fallen/outro/imp.h)
IMP_Mesh IMP_binary_load(CBYTE* fname);

#endif // UI_CUTSCENES_OUTRO_OUTRO_IMP_H
