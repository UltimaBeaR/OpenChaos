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

// Parses a SEX text file (a simple 3ds-exported ASCII mesh format) into an IMP_Mesh.
// Builds shared vertices by merging per-face normals within smoothing groups.
// Also builds quad, edge, and wireframe line lists.
// Coordinate system is converted from 3ds convention (Y-up, X-negated) to game convention.
// Returns a mesh with valid==UC_FALSE on any file or allocation error.
// uc_orig: IMP_load (fallen/outro/imp.cpp)
IMP_Mesh IMP_load(CBYTE* fname, float scale)
{
    SLONG i;
    SLONG j;
    SLONG k;
    SLONG l;

    float x;
    float y;
    float z;

    float r;
    float g;
    float b;

    float u;
    float v;

    float sh;
    float ss;

    float dist;

    SLONG m;
    SLONG v1;
    SLONG v2;
    SLONG p1;
    SLONG p2;
    SLONG p3;
    SLONG t1;
    SLONG t2;
    SLONG t3;
    SLONG e1;
    SLONG e2;
    SLONG e3;
    SLONG p1o;
    SLONG p2o;
    SLONG v1o;
    SLONG v2o;

    CBYTE sided[16];
    CBYTE alpha[16];
    CBYTE tname[128];
    CBYTE bname[128];

    SLONG shared[3];

    SLONG group;
    SLONG match;

    SLONG pivot_valid = UC_FALSE;

    SLONG offset_vert;
    SLONG offset_tvert;
    SLONG offset_mat;

    SLONG max_mats;
    SLONG max_verts;
    SLONG max_tverts;
    SLONG max_faces;
    SLONG max_sverts;
    SLONG max_quads;
    SLONG max_edges;
    SLONG max_lines;

    Point3d fs_norm[3];
    Point3d fs_u[3];
    Point3d fs_v[3];

    IMP_Svert fs[3];

    IMP_Mat*  im;
    IMP_Vert* iv;
    IMP_Tvert* it;
    IMP_Face* ic;
    IMP_Face* ico;
    IMP_Svert* is;
    IMP_Quad* iq;
    IMP_Edge* ie;

    IMP_Mesh ans;
    CBYTE line[256];
    CBYTE oname[32];

    FILE* handle;

    max_mats   = 4;
    max_verts  = 32;
    max_tverts = 32;
    max_faces  = 32;
    max_sverts = 32;
    max_quads  = 16;
    max_edges  = 32;
    max_lines  = 32;

    memset(&ans, 0, sizeof(ans));

    ans.mat   = (IMP_Mat*)malloc(sizeof(IMP_Mat) * max_mats);
    ans.vert  = (IMP_Vert*)malloc(sizeof(IMP_Vert) * max_verts);
    ans.tvert = (IMP_Tvert*)malloc(sizeof(IMP_Tvert) * max_tverts);
    ans.face  = (IMP_Face*)malloc(sizeof(IMP_Face) * max_faces);
    ans.svert = (IMP_Svert*)malloc(sizeof(IMP_Svert) * max_sverts);
    ans.quad  = (IMP_Quad*)malloc(sizeof(IMP_Quad) * max_quads);
    ans.edge  = (IMP_Edge*)malloc(sizeof(IMP_Edge) * max_edges);
    ans.line  = (IMP_Line*)malloc(sizeof(IMP_Line) * max_lines);

    if (ans.mat == NULL || ans.vert == NULL || ans.tvert == NULL || ans.face == NULL || ans.svert == NULL || ans.quad == NULL || ans.edge == NULL || ans.line == NULL) {
        goto file_error;
    }

    handle = fopen_ci(fname, "rb");

    if (!handle) {
        goto file_error;
    }

    while (fgets(line, 256, handle)) {
        if (line[0] == '#') {
            continue;
        }

        match = sscanf(line, "Triangle mesh: %s", oname);

        if (match == 1) {
            if (ans.name[0] == '\000') {
                strcpy(ans.name, oname);
            }

            offset_vert  = ans.num_verts;
            offset_tvert = ans.num_tverts;
            offset_mat   = ans.num_mats;

            continue;
        }

        match = sscanf(line, "Pivot: (%f,%f,%f)", &x, &y, &z);

        if (match == 3) {
            if (!pivot_valid) {
                pivot_valid = UC_TRUE;
            }

            continue;
        }

        match = sscanf(line, "Material: DiffuseRGB (%f,%f,%f), shininess %f, shinstr %f, %s sided, %s alpha, diffuse %s bumpmap %s", &r, &b, &g, &sh, &ss, sided, alpha, tname, bname);

        if (match == 9) {
            if (ans.num_mats >= max_mats) {
                max_mats *= 2;

                ans.mat = (IMP_Mat*)realloc(ans.mat, sizeof(IMP_Mat) * max_mats);

                if (ans.mat == NULL) {
                    goto file_error;
                }
            }

            im = &ans.mat[ans.num_mats++];

            im->r         = r;
            im->g         = g;
            im->b         = b;
            im->shininess = sh;
            im->shinstr   = ss;
            im->alpha     = (strcmp(alpha, "Filtered") == 0) ? IMP_ALPHA_FILTERED : IMP_ALPHA_ADDITIVE;
            im->sided     = (strcmp(sided, "Single") == 0) ? IMP_SIDED_SINGLE : IMP_SIDED_DOUBLE;
            im->has_texture = (strcmp(tname, "none") != 0);
            im->has_bumpmap = (strcmp(bname, "none") != 0);

            strncpy(im->tname, tname, 32);
            strncpy(im->bname, bname, 32);

            continue;
        }

        match = sscanf(line, "Vertex: (%f,%f,%f)", &x, &y, &z);

        if (match == 3) {
            // Convert from 3ds orientation (Y-up) to game orientation.
            SWAP_FL(y, z);
            z = -z;
            x = -x;

            x *= scale;
            y *= scale;
            z *= scale;

            if (ans.num_verts >= max_verts) {
                max_verts *= 2;

                ans.vert = (IMP_Vert*)realloc(ans.vert, sizeof(IMP_Vert) * max_verts);

                if (ans.vert == NULL) {
                    goto file_error;
                }
            }

            iv = &ans.vert[ans.num_verts++];

            iv->x = x;
            iv->y = y;
            iv->z = z;

            continue;
        }

        match = sscanf(line, "Texture Vertex: (%f,%f)", &u, &v);

        if (match == 2) {
            if (ans.num_tverts >= max_tverts) {
                max_tverts *= 2;

                ans.tvert = (IMP_Tvert*)realloc(ans.tvert, sizeof(IMP_Tvert) * max_tverts);

                if (ans.tvert == NULL) {
                    goto file_error;
                }
            }

            it = &ans.tvert[ans.num_tverts++];

            it->u = u;
            it->v = v;

            continue;
        }

        match = sscanf(line, "Face: Material %d xyz (%d,%d,%d) uv (%d,%d,%d) edge (%d,%d,%d) group %d", &m, &p1, &p2, &p3, &t1, &t2, &t3, &e1, &e2, &e3, &group);

        if (match == 11) {
            if (ans.num_faces >= max_faces) {
                max_faces *= 2;

                ans.face = (IMP_Face*)realloc(ans.face, sizeof(IMP_Face) * max_faces);

                if (ans.face == NULL) {
                    goto file_error;
                }
            }

            p1 += offset_vert;
            p2 += offset_vert;
            p3 += offset_vert;

            t1 += offset_tvert;
            t2 += offset_tvert;
            t3 += offset_tvert;

            m += offset_mat;

            ASSERT(WITHIN(p1, 0, ans.num_verts - 1));
            ASSERT(WITHIN(p2, 0, ans.num_verts - 1));
            ASSERT(WITHIN(p3, 0, ans.num_verts - 1));

            ASSERT(WITHIN(t1, 0, ans.num_tverts - 1));
            ASSERT(WITHIN(t2, 0, ans.num_tverts - 1));
            ASSERT(WITHIN(t3, 0, ans.num_tverts - 1));

            ASSERT(WITHIN(m, 0, ans.num_mats - 1));

            if (!ans.mat[m].has_texture && !ans.mat[m].has_bumpmap) {
                // Untextured faces all share UVs at (0,0) to allow svert sharing.
                t1 = 0;
                t2 = 0;
                t3 = 0;
            }

            ic = &ans.face[ans.num_faces++];

            ic->v[0] = p1;
            ic->v[1] = p2;
            ic->v[2] = p3;
            ic->t[0] = t1;
            ic->t[1] = t2;
            ic->t[2] = t3;
            ic->mat   = m;
            ic->group = group;
            ic->flag  = 0;

            if (e1) {
                ic->flag |= IMP_FACE_FLAG_EDGE_A;
            }
            if (e2) {
                ic->flag |= IMP_FACE_FLAG_EDGE_B;
            }
            if (e3) {
                ic->flag |= IMP_FACE_FLAG_EDGE_C;
            }

            // Compute face normal via cross product.
            {
                IMP_Vert* v1 = &ans.vert[p1];
                IMP_Vert* v2 = &ans.vert[p2];
                IMP_Vert* v3 = &ans.vert[p3];

                float ax = v2->x - v1->x;
                float ay = v2->y - v1->y;
                float az = v2->z - v1->z;

                float bx = v3->x - v1->x;
                float by = v3->y - v1->y;
                float bz = v3->z - v1->z;

                float nx = ay * bz - az * by;
                float ny = az * bx - ax * bz;
                float nz = ax * by - ay * bx;

                IMP_normalise_vector(&nx, &ny, &nz);

                ic->nx = nx;
                ic->ny = ny;
                ic->nz = nz;
            }

            // Compute per-face texture tangent and bitangent for bumpmapping.
            {
                IMP_Vert*  iv1 = &ans.vert[ic->v[0]];
                IMP_Vert*  iv2 = &ans.vert[ic->v[1]];
                IMP_Vert*  iv3 = &ans.vert[ic->v[2]];

                IMP_Tvert* it1 = &ans.tvert[ic->t[0]];
                IMP_Tvert* it2 = &ans.tvert[ic->t[1]];
                IMP_Tvert* it3 = &ans.tvert[ic->t[2]];

                float x1 = iv2->x - iv1->x;
                float y1 = iv2->y - iv1->y;
                float z1 = iv2->z - iv1->z;

                float x2 = iv3->x - iv1->x;
                float y2 = iv3->y - iv1->y;
                float z2 = iv3->z - iv1->z;

                float u1 = it2->u - it1->u;
                float v1 = it2->v - it1->v;

                float u2 = it3->u - it1->u;
                float v2 = it3->v - it1->v;

                float ucrossv    = u1 * v2 - u2 * v1;
                float overucrossv = 1.0F / ucrossv;

                ic->dxdu = (x1 * v2 - x2 * v1) * overucrossv;
                ic->dydu = (y1 * v2 - y2 * v1) * overucrossv;
                ic->dzdu = (z1 * v2 - z2 * v1) * overucrossv;

                ic->dxdv = (x1 * u2 - x2 * u1) * -overucrossv;
                ic->dydv = (y1 * u2 - y2 * u1) * -overucrossv;
                ic->dzdv = (z1 * u2 - z2 * u1) * -overucrossv;

                IMP_normalise_vector(&ic->dxdu, &ic->dydu, &ic->dzdu);
                IMP_normalise_vector(&ic->dxdv, &ic->dydv, &ic->dzdv);
            }

            if (e1) {
                if (!IMP_add_line(&ans, &max_lines, p1, p2))
                    goto file_error;
            }
            if (e2) {
                if (!IMP_add_line(&ans, &max_lines, p2, p3))
                    goto file_error;
            }
            if (e3) {
                if (!IMP_add_line(&ans, &max_lines, p3, p1))
                    goto file_error;
            }

            continue;
        }
    }

    fclose(handle);

    // Build per-vertex smooth normals and tangent bases by averaging over smoothing groups.
    for (i = 0; i < ans.num_faces; i++) {
        ic = &ans.face[i];

        for (j = 0; j < 3; j++) {
            ASSERT(WITHIN(ic->v[j], 0, ans.num_verts - 1));
            ASSERT(WITHIN(ic->t[j], 0, ans.num_tverts - 1));

            fs[j].vert = ic->v[j];
            fs[j].u    = ans.tvert[ic->t[j]].u;
            fs[j].v    = ans.tvert[ic->t[j]].v;
            fs[j].mat  = ic->mat;
            fs[j].nx   = 0.0F;
            fs[j].ny   = 0.0F;
            fs[j].nz   = 0.0F;
            fs[j].dxdu = 0.0F;
            fs[j].dydu = 0.0F;
            fs[j].dzdu = 0.0F;
            fs[j].dxdv = 0.0F;
            fs[j].dydv = 0.0F;
            fs[j].dzdv = 0.0F;

            fs_norm[j].x = 0;
            fs_norm[j].y = 0;
            fs_norm[j].z = 0;

            fs_u[j].x = 0;
            fs_u[j].y = 0;
            fs_u[j].z = 0;

            fs_v[j].x = 0;
            fs_v[j].y = 0;
            fs_v[j].z = 0;

            shared[j] = 0;
        }

        // Accumulate face normals and tangents from all faces in the same smoothing group
        // that share this vertex.
        for (j = 0; j < ans.num_faces; j++) {
            ico = &ans.face[j];

            // A group of zero means the face has no smoothing; it still contributes to itself.
            if ((ico->group & ic->group) || (ico == ic)) {
                for (k = 0; k < 3; k++)
                    for (l = 0; l < 3; l++) {
                        if (ic->v[k] == ico->v[l]) {
                            fs_norm[k].x += ico->nx;
                            fs_norm[k].y += ico->ny;
                            fs_norm[k].z += ico->nz;

                            fs_u[k].x += ico->dxdu;
                            fs_u[k].y += ico->dydu;
                            fs_u[k].z += ico->dzdu;

                            fs_v[k].x += ico->dxdv;
                            fs_v[k].y += ico->dydv;
                            fs_v[k].z += ico->dzdv;

                            shared[k] += 1;
                        }
                    }
            }
        }

        // Normalise the accumulated per-vertex vectors and project tangents into the tangent plane.
        for (j = 0; j < 3; j++) {
            ASSERT(shared[j] > 0);

            IMP_normalise_vector(&fs_norm[j].x, &fs_norm[j].y, &fs_norm[j].z);

            ASSERT(fs_norm[j].x <= 1.0F);
            ASSERT(fs_norm[j].y <= 1.0F);
            ASSERT(fs_norm[j].z <= 1.0F);

            float dprod;

            dprod = fs_norm[j].x * fs_u[j].x + fs_norm[j].y * fs_u[j].y + fs_norm[j].z * fs_u[j].z;

            fs_u[j].x -= dprod * fs_norm[j].x;
            fs_u[j].y -= dprod * fs_norm[j].y;
            fs_u[j].z -= dprod * fs_norm[j].z;

            dprod = fs_norm[j].x * fs_v[j].x + fs_norm[j].y * fs_v[j].y + fs_norm[j].z * fs_v[j].z;

            fs_v[j].x -= dprod * fs_norm[j].x;
            fs_v[j].y -= dprod * fs_norm[j].y;
            fs_v[j].z -= dprod * fs_norm[j].z;

            IMP_normalise_vector(&fs_u[j].x, &fs_u[j].y, &fs_u[j].z);
            IMP_normalise_vector(&fs_v[j].x, &fs_v[j].y, &fs_v[j].z);
        }

        // Find an existing svert with matching vert/mat/UV/normal, or create a new one.
        for (j = 0; j < 3; j++) {
            for (k = ans.num_sverts - 1; k >= 0; k--) {
                is = &ans.svert[k];

                if (is->vert == fs[j].vert && is->mat == fs[j].mat && is->u == fs[j].u && is->v == fs[j].v) {
                    if (IMP_norm_similar(
                            is->nx,
                            is->ny,
                            is->nz,
                            fs_norm[j].x,
                            fs_norm[j].y,
                            fs_norm[j].z)) {
                        ic->s[j] = k;

                        goto found_matching_svert;
                    }
                }
            }

            if (ans.num_sverts >= max_sverts) {
                max_sverts *= 2;

                ans.svert = (IMP_Svert*)realloc(ans.svert, sizeof(IMP_Svert) * max_sverts);

                if (ans.svert == NULL) {
                    goto file_error;
                }
            }

            fs[j].nx = fs_norm[j].x;
            fs[j].ny = fs_norm[j].y;
            fs[j].nz = fs_norm[j].z;

            ASSERT(fs[j].nx <= 1.0F);
            ASSERT(fs[j].ny <= 1.0F);
            ASSERT(fs[j].nz <= 1.0F);

            fs[j].dxdu = fs_u[j].x;
            fs[j].dydu = fs_u[j].y;
            fs[j].dzdu = fs_u[j].z;

            fs[j].dxdv = fs_v[j].x;
            fs[j].dydv = fs_v[j].y;
            fs[j].dzdv = fs_v[j].z;

            ans.svert[ans.num_sverts] = fs[j];
            ic->s[j] = ans.num_sverts;
            ans.num_sverts += 1;

        found_matching_svert:;
        }
    }

    // Detect quads: pairs of coplanar adjacent triangles sharing a non-edge diagonal.
    for (i = 0; i < ans.num_faces; i++) {
        ic = &ans.face[i];

        if ((ic->flag & (IMP_FACE_FLAG_EDGE_A | IMP_FACE_FLAG_EDGE_B | IMP_FACE_FLAG_EDGE_C)) == (IMP_FACE_FLAG_EDGE_A | IMP_FACE_FLAG_EDGE_B | IMP_FACE_FLAG_EDGE_C)) {
            // Pure triangle — no shared diagonal.
            continue;
        }

        if (ic->flag & IMP_FACE_FLAG_QUADDED) {
            continue;
        }

        for (j = 0; j < 3; j++) {
            if (ic->flag & (IMP_FACE_FLAG_EDGE << j)) {
                // Real edge — not the quad diagonal.
            } else {
                p1 = j + 0;
                p2 = j + 1;

                if (p2 == 3) {
                    p2 = 0;
                }

                v1 = ic->v[p1];
                v2 = ic->v[p2];

                for (k = i + 1; k < ans.num_faces; k++) {
                    ico = &ans.face[k];

                    if ((ico->flag & (IMP_FACE_FLAG_EDGE_A | IMP_FACE_FLAG_EDGE_B | IMP_FACE_FLAG_EDGE_C)) == (IMP_FACE_FLAG_EDGE_A | IMP_FACE_FLAG_EDGE_B | IMP_FACE_FLAG_EDGE_C)) {
                        continue;
                    }

                    if (ico->flag & IMP_FACE_FLAG_QUADDED) {
                        continue;
                    }

                    // Quads must be co-planar.
                    if (IMP_norm_similar(ic->nx, ic->ny, ic->nz, ico->nx, ico->ny, ico->nz)) {
                        continue;
                    }

                    for (l = 0; l < 3; l++) {
                        if (ico->flag & (IMP_FACE_FLAG_EDGE << l)) {
                            // Real edge.
                        } else {
                            p1o = j + 0;
                            p2o = j + 1;

                            if (p2o == 3) {
                                p2o = 0;
                            }

                            v1o = ico->v[p1o];
                            v2o = ico->v[p2o];

                            if (v2o == v1 && v1o == v2) {
                                if (ans.num_quads >= max_quads) {
                                    max_quads *= 2;

                                    ans.quad = (IMP_Quad*)realloc(ans.quad, sizeof(IMP_Quad) * max_quads);

                                    if (ans.quad == NULL) {
                                        goto file_error;
                                    }
                                }

                                iq = &ans.quad[ans.num_quads++];

                                iq->v[0] = ic->v[(j + 1) % 3];
                                iq->v[1] = ic->v[(j + 2) % 3];
                                iq->v[2] = ic->v[(j + 0) % 3];
                                iq->v[3] = ico->v[(l + 2) % 3];

                                ic->flag  |= IMP_FACE_FLAG_QUADDED;
                                ico->flag |= IMP_FACE_FLAG_QUADDED;

                                goto found_quad;
                            }
                        }
                    }
                }
            }
        }

    found_quad:;
    }

    // Build edge list: each edge records the two adjacent face indices.
    for (i = 0; i < ans.num_faces; i++) {
        ic = &ans.face[i];

        for (j = 0; j < 3; j++) {
            p1 = j + 0;
            p2 = j + 1;

            if (p2 == 3) {
                p2 = 0;
            }

            v1 = ic->v[p1];
            v2 = ic->v[p2];

            for (k = ans.num_edges - 1; k >= 0; k--) {
                ie = &ans.edge[k];

                if (ie->v1 == v2 && ie->v2 == v1) {
                    if (ie->f2 == 0xffff) {
                        ie->f2 = i;

                        goto found_edge;
                    }
                }
            }

            if (ans.num_edges >= max_edges) {
                max_edges *= 2;

                ans.edge = (IMP_Edge*)realloc(ans.edge, sizeof(IMP_Edge) * max_edges);

                if (ans.edge == NULL) {
                    goto file_error;
                }
            }

            ans.edge[ans.num_edges].v1 = v1;
            ans.edge[ans.num_edges].v2 = v2;
            ans.edge[ans.num_edges].f1 = i;
            ans.edge[ans.num_edges].f2 = 0xffff;

            ans.num_edges += 1;

        found_edge:;
        }
    }

    // Compute axis-aligned bounding box and bounding sphere radius.
    ans.min_x  = float(+UC_INFINITY);
    ans.min_y  = float(+UC_INFINITY);
    ans.min_z  = float(+UC_INFINITY);
    ans.max_x  = float(-UC_INFINITY);
    ans.max_y  = float(-UC_INFINITY);
    ans.max_z  = float(-UC_INFINITY);
    ans.radius = 0.0F;

    for (i = 0; i < ans.num_verts; i++) {
        iv = &ans.vert[i];

        if (iv->x < ans.min_x) {
            ans.min_x = iv->x;
        }
        if (iv->y < ans.min_y) {
            ans.min_y = iv->y;
        }
        if (iv->z < ans.min_z) {
            ans.min_z = iv->z;
        }

        if (iv->x > ans.max_x) {
            ans.max_x = iv->x;
        }
        if (iv->y > ans.max_y) {
            ans.max_y = iv->y;
        }
        if (iv->z > ans.max_z) {
            ans.max_z = iv->z;
        }

        dist = iv->x * iv->x + iv->y * iv->y + iv->z * iv->z;

        if (dist > ans.radius) {
            ans.radius = dist;
        }
    }

    ans.radius = sqrt(ans.radius);

    ans.valid = UC_TRUE;

    return ans;

file_error:;

    IMP_free(&ans);

    if (handle) {
        fclose(handle);
    }

    memset(&ans, 0, sizeof(ans));

    ans.valid = UC_FALSE;

    return ans;
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

// Saves a pre-processed binary version of the mesh to disk (version 1 format).
// Writes with fixed on-disk field sizes for x86/x64 compatibility.
// Returns UC_FALSE on any I/O error.
// uc_orig: IMP_binary_save (fallen/outro/imp.cpp)
SLONG IMP_binary_save(CBYTE* fname, IMP_Mesh* im)
{
    FILE* handle = fopen_ci(fname, "wb");

    if (!handle) {
        return UC_FALSE;
    }

    SLONG version_number = 1;

    if (fwrite(&version_number, sizeof(SLONG), 1, handle) != 1)
        goto file_error;

    if (!IMP_write_mesh_header(handle, im))
        goto file_error;
    if (!IMP_write_mat_array(handle, im->mat, im->num_mats))
        goto file_error;
    if (fwrite(im->vert, sizeof(IMP_Vert), im->num_verts, handle) != (size_t)im->num_verts)
        goto file_error;
    if (fwrite(im->tvert, sizeof(IMP_Tvert), im->num_tverts, handle) != (size_t)im->num_tverts)
        goto file_error;
    if (fwrite(im->face, sizeof(IMP_Face), im->num_faces, handle) != (size_t)im->num_faces)
        goto file_error;
    if (fwrite(im->svert, sizeof(IMP_Svert), im->num_sverts, handle) != (size_t)im->num_sverts)
        goto file_error;
    if (fwrite(im->quad, sizeof(IMP_Quad), im->num_quads, handle) != (size_t)im->num_quads)
        goto file_error;
    if (fwrite(im->edge, sizeof(IMP_Edge), im->num_edges, handle) != (size_t)im->num_edges)
        goto file_error;
    if (fwrite(im->line, sizeof(IMP_Line), im->num_lines, handle) != (size_t)im->num_lines)
        goto file_error;

    fclose(handle);

    return UC_TRUE;

file_error:;

    fclose(handle);

    return UC_FALSE;
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
