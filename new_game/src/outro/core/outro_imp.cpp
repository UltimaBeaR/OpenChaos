#include "outro/core/outro_imp.h"
#include "engine/io/file.h"

// Returns UC_TRUE if the angle between two normals is small enough to be considered
// the same for smoothing group purposes (dot product > 0.999).
// uc_orig: IMP_norm_similar (fallen/outro/imp.cpp)
static SLONG IMP_norm_similar(
    float nx1,
    float ny1,
    float nz1,
    float nx2,
    float ny2,
    float nz2)
{
    float dprod;

    dprod = nx1 * nx2 + ny1 * ny2 + nz1 * nz2;

    if (dprod > 0.999F) {
        return UC_TRUE;
    } else {
        return UC_FALSE;
    }
}

// Normalises a 3D vector in place. Degenerate (near-zero) vectors become (1,0,0).
// uc_orig: IMP_normalise_vector (fallen/outro/imp.cpp)
static void IMP_normalise_vector(
    float* vx,
    float* vy,
    float* vz)
{
    float len = sqrt(*vx * *vx + *vy * *vy + *vz * *vz);

    if (len < 0.00001F) {
        *vx = 1.0F;
        *vy = 0.0F;
        *vz = 0.0F;
    } else {
        float overlen = 1.0F / len;

        *vx *= overlen;
        *vy *= overlen;
        *vz *= overlen;
    }
}

// Adds a wireframe edge between v1 and v2 to the mesh line list.
// Skips duplicate edges. Doubles the line buffer capacity on overflow.
// Returns UC_FALSE on allocation failure.
// uc_orig: IMP_add_line (fallen/outro/imp.cpp)
static SLONG IMP_add_line(IMP_Mesh* im, SLONG* max_lines, UWORD v1, UWORD v2)
{
    SLONG i;

    for (i = im->num_lines - 1; i >= 0; i--) {
        if ((im->line[i].v1 == v1 && im->line[i].v2 == v2) || (im->line[i].v1 == v2 && im->line[i].v2 == v1)) {
            return UC_TRUE;
        }
    }

    if (im->num_lines >= *max_lines) {
        *max_lines *= 2;

        im->line = (IMP_Line*)realloc(im->line, sizeof(IMP_Line) * *max_lines);

        if (im->line == NULL) {
            return UC_FALSE;
        }
    }

    im->line[im->num_lines].v1 = v1;
    im->line[im->num_lines].v2 = v2;

    im->num_lines += 1;

    return UC_TRUE;
}


// Frees all heap-allocated arrays inside the mesh. Sets freed pointers to NULL.
// Safe to call on a partially-initialised mesh (any NULL pointer is skipped).
// uc_orig: IMP_free (fallen/outro/imp.cpp)
void IMP_free(IMP_Mesh* im)
{
    if (im->mat) {
        free(im->mat);
        im->mat = NULL;
    }
    if (im->vert) {
        free(im->vert);
        im->vert = NULL;
    }
    if (im->tvert) {
        free(im->tvert);
        im->tvert = NULL;
    }
    if (im->face) {
        free(im->face);
        im->face = NULL;
    }
    if (im->svert) {
        free(im->svert);
        im->svert = NULL;
    }
    if (im->quad) {
        free(im->quad);
        im->quad = NULL;
    }
    if (im->edge) {
        free(im->edge);
        im->edge = NULL;
    }
    if (im->line) {
        free(im->line);
        im->line = NULL;
    }
}

// On-disk sizes of structures as written by the original 32-bit build.
// These differ from runtime sizeof() on x64 because IMP_Mesh and IMP_Mat contain pointers.
// IMP_Mesh x86: valid(4) + name(32) + 8×num_*(32) + 8×ptr(32) + 7×float(28) + 2×ptr(8) = 136
// IMP_Mat  x86: 5×float(20) + 4×UBYTE(4) + 2×name(64) + 4×ptr(16) = 104
#define IMP_MESH_DISK_SIZE 136
#define IMP_MAT_DISK_SIZE  104

// Write IMP_Mesh header in 32-bit compatible on-disk format.
static int IMP_write_mesh_header(FILE* handle, IMP_Mesh* im)
{
    UBYTE buf[IMP_MESH_DISK_SIZE];
    memset(buf, 0, sizeof(buf));

    UBYTE* p = buf;
    memcpy(p, &im->valid,      4);  p += 4;
    memcpy(p, im->name,        32); p += 32;
    memcpy(p, &im->num_mats,   4);  p += 4;
    memcpy(p, &im->num_verts,  4);  p += 4;
    memcpy(p, &im->num_tverts, 4);  p += 4;
    memcpy(p, &im->num_faces,  4);  p += 4;
    memcpy(p, &im->num_sverts, 4);  p += 4;
    memcpy(p, &im->num_quads,  4);  p += 4;
    memcpy(p, &im->num_edges,  4);  p += 4;
    memcpy(p, &im->num_lines,  4);  p += 4;
    p += 8 * 4; // skip 8 x86 pointer slots (zeroed)
    memcpy(p, &im->min_x,      4);  p += 4;
    memcpy(p, &im->min_y,      4);  p += 4;
    memcpy(p, &im->min_z,      4);  p += 4;
    memcpy(p, &im->max_x,      4);  p += 4;
    memcpy(p, &im->max_y,      4);  p += 4;
    memcpy(p, &im->max_z,      4);  p += 4;
    memcpy(p, &im->radius,     4);  p += 4;
    // 2 x86 pointer slots (old_vert, old_svert) — already zeroed

    return fwrite(buf, IMP_MESH_DISK_SIZE, 1, handle) == 1;
}

// Write IMP_Mat array in 32-bit compatible on-disk format.
static int IMP_write_mat_array(FILE* handle, IMP_Mat* mats, SLONG count)
{
    UBYTE buf[IMP_MAT_DISK_SIZE];

    for (SLONG i = 0; i < count; i++) {
        memset(buf, 0, sizeof(buf));
        UBYTE* p = buf;
        memcpy(p, &mats[i].r,           4);  p += 4;
        memcpy(p, &mats[i].g,           4);  p += 4;
        memcpy(p, &mats[i].b,           4);  p += 4;
        memcpy(p, &mats[i].shininess,   4);  p += 4;
        memcpy(p, &mats[i].shinstr,     4);  p += 4;
        *p++ = mats[i].alpha;
        *p++ = mats[i].sided;
        *p++ = mats[i].has_texture;
        *p++ = mats[i].has_bumpmap;
        memcpy(p, mats[i].tname,        32); p += 32;
        memcpy(p, mats[i].bname,        32); p += 32;
        // 4 x86 pointer slots — already zeroed

        if (fwrite(buf, IMP_MAT_DISK_SIZE, 1, handle) != 1)
            return 0;
    }

    return 1;
}

// Read IMP_Mesh header from a 32-bit .imp file, skipping pointer fields.
static int IMP_read_mesh_header(FILE* handle, IMP_Mesh* out)
{
    UBYTE buf[IMP_MESH_DISK_SIZE];
    if (fread(buf, IMP_MESH_DISK_SIZE, 1, handle) != 1)
        return 0;

    UBYTE* p = buf;
    memcpy(&out->valid,      p, 4);  p += 4;
    memcpy(out->name,        p, 32); p += 32;
    memcpy(&out->num_mats,   p, 4);  p += 4;
    memcpy(&out->num_verts,  p, 4);  p += 4;
    memcpy(&out->num_tverts, p, 4);  p += 4;
    memcpy(&out->num_faces,  p, 4);  p += 4;
    memcpy(&out->num_sverts, p, 4);  p += 4;
    memcpy(&out->num_quads,  p, 4);  p += 4;
    memcpy(&out->num_edges,  p, 4);  p += 4;
    memcpy(&out->num_lines,  p, 4);  p += 4;
    p += 8 * 4; // skip 8 x86 pointers (mat..line)
    memcpy(&out->min_x,      p, 4);  p += 4;
    memcpy(&out->min_y,      p, 4);  p += 4;
    memcpy(&out->min_z,      p, 4);  p += 4;
    memcpy(&out->max_x,      p, 4);  p += 4;
    memcpy(&out->max_y,      p, 4);  p += 4;
    memcpy(&out->max_z,      p, 4);  p += 4;
    memcpy(&out->radius,     p, 4);  p += 4;
    // skip 2 x86 pointers (old_vert, old_svert)

    return 1;
}

// Read IMP_Mat array from a 32-bit .imp file, skipping pointer fields.
static int IMP_read_mat_array(FILE* handle, IMP_Mat* out, SLONG count)
{
    UBYTE buf[IMP_MAT_DISK_SIZE];

    for (SLONG i = 0; i < count; i++) {
        if (fread(buf, IMP_MAT_DISK_SIZE, 1, handle) != 1)
            return 0;

        UBYTE* p = buf;
        memcpy(&out[i].r,           p, 4);  p += 4;
        memcpy(&out[i].g,           p, 4);  p += 4;
        memcpy(&out[i].b,           p, 4);  p += 4;
        memcpy(&out[i].shininess,   p, 4);  p += 4;
        memcpy(&out[i].shinstr,     p, 4);  p += 4;
        out[i].alpha       = *p++;
        out[i].sided       = *p++;
        out[i].has_texture = *p++;
        out[i].has_bumpmap = *p++;
        memcpy(out[i].tname,        p, 32); p += 32;
        memcpy(out[i].bname,        p, 32); p += 32;
        // skip 4 x86 pointers (ot_tex, ot_bpos, ot_bneg, ob)

        out[i].ot_tex  = NULL;
        out[i].ot_bpos = NULL;
        out[i].ot_bneg = NULL;
        out[i].ob      = NULL;
    }

    return 1;
}

// Loads a pre-processed binary mesh (version 1 format). Allocates all dynamic arrays.
// Reads with fixed on-disk field sizes to handle x86→x64 struct size differences.
// Returns an empty mesh (valid==UC_FALSE) on version mismatch or I/O error.
// uc_orig: IMP_binary_load (fallen/outro/imp.cpp)
IMP_Mesh IMP_binary_load(CBYTE* fname)
{
    IMP_Mesh ans;

    memset(&ans, 0, sizeof(ans));

    FILE* handle = fopen_ci(fname, "rb");

    if (!handle) {
        return ans;
    }

    SLONG version_number;

    if (fread(&version_number, sizeof(SLONG), 1, handle) != 1)
        goto file_error;

    if (version_number != 1) {
        return ans;
    }

    if (!IMP_read_mesh_header(handle, &ans))
        goto file_error;

    ans.mat   = (IMP_Mat*)malloc(sizeof(IMP_Mat) * ans.num_mats);
    ans.vert  = (IMP_Vert*)malloc(sizeof(IMP_Vert) * ans.num_verts);
    ans.tvert = (IMP_Tvert*)malloc(sizeof(IMP_Tvert) * ans.num_tverts);
    ans.face  = (IMP_Face*)malloc(sizeof(IMP_Face) * ans.num_faces);
    ans.svert = (IMP_Svert*)malloc(sizeof(IMP_Svert) * ans.num_sverts);
    ans.quad  = (IMP_Quad*)malloc(sizeof(IMP_Quad) * ans.num_quads);
    ans.edge  = (IMP_Edge*)malloc(sizeof(IMP_Edge) * ans.num_edges);
    ans.line  = (IMP_Line*)malloc(sizeof(IMP_Line) * ans.num_lines);

    if (ans.mat == NULL || ans.vert == NULL || ans.tvert == NULL || ans.face == NULL || ans.svert == NULL || ans.quad == NULL || ans.edge == NULL || ans.line == NULL) {
        goto file_error;
    }

    if (!IMP_read_mat_array(handle, ans.mat, ans.num_mats))
        goto file_error;
    // Remaining structs have no pointers — on-disk size == runtime sizeof.
    if (fread(ans.vert,  sizeof(IMP_Vert),  ans.num_verts,  handle) != (size_t)ans.num_verts)
        goto file_error;
    if (fread(ans.tvert, sizeof(IMP_Tvert), ans.num_tverts, handle) != (size_t)ans.num_tverts)
        goto file_error;
    if (fread(ans.face,  sizeof(IMP_Face),  ans.num_faces,  handle) != (size_t)ans.num_faces)
        goto file_error;
    if (fread(ans.svert, sizeof(IMP_Svert), ans.num_sverts, handle) != (size_t)ans.num_sverts)
        goto file_error;
    if (fread(ans.quad,  sizeof(IMP_Quad),  ans.num_quads,  handle) != (size_t)ans.num_quads)
        goto file_error;
    if (fread(ans.edge,  sizeof(IMP_Edge),  ans.num_edges,  handle) != (size_t)ans.num_edges)
        goto file_error;
    if (fread(ans.line,  sizeof(IMP_Line),  ans.num_lines,  handle) != (size_t)ans.num_lines)
        goto file_error;

    fclose(handle);

    return ans;

file_error:;

    IMP_free(&ans);

    memset(&ans, 0, sizeof(ans));

    fclose(handle);

    return ans;
}
