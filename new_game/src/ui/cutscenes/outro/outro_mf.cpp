#include "ui/cutscenes/outro/outro_mf.h"
#include "ui/cutscenes/outro/outro_mf_globals.h"
#include "ui/cutscenes/outro/outro_matrix.h"
#include "ui/cutscenes/outro/outro_os.h"

// uc_orig: MF_load_textures (fallen/outro/mf.cpp)
void MF_load_textures(IMP_Mesh* im)
{
    SLONG i;
    IMP_Mat* it;

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];

        if (it->has_texture) {
            it->ot_tex = OS_texture_create(it->tname);
        } else {
            it->ot_tex = NULL;
        }

        if (it->has_bumpmap) {
            it->ot_bpos = OS_texture_create(it->bname, UC_FALSE);
            it->ot_bneg = OS_texture_create(it->bname, UC_TRUE);
        } else {
            it->ot_bpos = NULL;
            it->ot_bneg = NULL;
        }
    }
}

// uc_orig: MF_backup (fallen/outro/mf.cpp)
void MF_backup(IMP_Mesh* im)
{
    im->old_vert  = (IMP_Vert*)malloc(sizeof(IMP_Vert)  * im->num_verts);
    im->old_svert = (IMP_Svert*)malloc(sizeof(IMP_Svert) * im->num_sverts);

    memcpy(im->old_vert,  im->vert,  sizeof(IMP_Vert)  * im->num_verts);
    memcpy(im->old_svert, im->svert, sizeof(IMP_Svert) * im->num_sverts);
}

// uc_orig: MF_rotate_mesh (fallen/outro/mf.cpp)
void MF_rotate_mesh(
    IMP_Mesh* im,
    float yaw,
    float pitch,
    float roll,
    float scale,
    float pos_x,
    float pos_y,
    float pos_z)
{
    SLONG i;
    float matrix[9];

    MATRIX_calc(matrix, yaw, pitch, roll);

    // Rotate normals and tangent vectors in the shared vertices.
    for (i = 0; i < im->num_sverts; i++) {
        im->svert[i] = im->old_svert[i];

        MATRIX_MUL(matrix, im->svert[i].nx,   im->svert[i].ny,   im->svert[i].nz);
        MATRIX_MUL(matrix, im->svert[i].dxdu, im->svert[i].dydu, im->svert[i].dzdu);
        MATRIX_MUL(matrix, im->svert[i].dxdv, im->svert[i].dydv, im->svert[i].dzdv);
    }

    // Scale applies only to vertex positions, not normals.
    MATRIX_scale(matrix, scale);

    for (i = 0; i < im->num_verts; i++) {
        im->vert[i] = im->old_vert[i];

        MATRIX_MUL(matrix, im->vert[i].x, im->vert[i].y, im->vert[i].z);

        im->vert[i].x += pos_x;
        im->vert[i].y += pos_y;
        im->vert[i].z += pos_z;
    }
}

// uc_orig: MF_rotate_mesh (fallen/outro/mf.cpp)
void MF_rotate_mesh(
    IMP_Mesh* im,
    float pos_x,
    float pos_y,
    float pos_z,
    float matrix[9])
{
    SLONG i;

    for (i = 0; i < im->num_sverts; i++) {
        im->svert[i] = im->old_svert[i];

        MATRIX_MUL(matrix, im->svert[i].nx,   im->svert[i].ny,   im->svert[i].nz);
        MATRIX_MUL(matrix, im->svert[i].dxdu, im->svert[i].dydu, im->svert[i].dzdu);
        MATRIX_MUL(matrix, im->svert[i].dxdv, im->svert[i].dydv, im->svert[i].dzdv);
    }

    for (i = 0; i < im->num_verts; i++) {
        im->vert[i] = im->old_vert[i];

        MATRIX_MUL(matrix, im->vert[i].x, im->vert[i].y, im->vert[i].z);

        im->vert[i].x += pos_x;
        im->vert[i].y += pos_y;
        im->vert[i].z += pos_z;
    }
}

// uc_orig: MF_transform_points (fallen/outro/mf.cpp)
void MF_transform_points(IMP_Mesh* im)
{
    SLONG i;
    IMP_Vert* iv;

    ASSERT(im->num_verts <= OS_MAX_TRANS);

    for (i = 0; i < im->num_verts; i++) {
        iv = &im->vert[i];
        OS_transform(iv->x, iv->y, iv->z, &OS_trans[i]);
    }
}

// uc_orig: MF_invert_zeds (fallen/outro/mf.cpp)
void MF_invert_zeds(IMP_Mesh* im)
{
    SLONG i;

    ASSERT(im->num_verts <= OS_MAX_TRANS);

    for (i = 0; i < im->num_verts; i++) {
        OS_trans[i].z = 1.0F - OS_trans[i].z;
    }
}

// uc_orig: MF_ambient (fallen/outro/mf.cpp)
void MF_ambient(
    IMP_Mesh* im,
    float light_dx,
    float light_dy,
    float light_dz,
    SLONG light_r,
    SLONG light_g,
    SLONG light_b,
    SLONG amb_r,
    SLONG amb_g,
    SLONG amb_b)
{
    SLONG i;
    SLONG r, g, b;
    float dprod;
    IMP_Svert* is;

    // Normalise the light direction vector.
    float len = sqrt(light_dx * light_dx + light_dy * light_dy + light_dz * light_dz);
    float overlen = 1.0F / len;
    light_dx *= overlen;
    light_dy *= overlen;
    light_dz *= overlen;

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];

        r = amb_r;
        g = amb_g;
        b = amb_b;

        dprod = light_dx * is->nx + light_dy * is->ny + light_dz * is->nz;

        if (dprod < 0.0F) {
            SLONG ibright = ftol(-dprod * 256.0F);

            r += ibright * light_r >> 8;
            g += ibright * light_g >> 8;
            b += ibright * light_b >> 8;

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
        }

        is->colour = (r << 16) | (g << 8) | b;
    }
}

// uc_orig: MF_diffuse_spotlight (fallen/outro/mf.cpp)
void MF_diffuse_spotlight(
    IMP_Mesh* im,
    float light_x,
    float light_y,
    float light_z,
    float light_matrix[9],
    float light_lens)
{
    SLONG i;
    float x, y, z, X, Y, Z, dprod;
    IMP_Vert* iv;
    IMP_Svert* is;

    // Compute lightmap UV for each vertex.
    for (i = 0; i < im->num_verts; i++) {
        iv = &im->vert[i];

        x = iv->x - light_x;
        y = iv->y - light_y;
        z = iv->z - light_z;

        MATRIX_MUL(light_matrix, x, y, z);

        if (z > 0.0F) {
            Z = light_lens / z;
            X = 0.5F + x * Z;
            Y = 0.5F + y * Z;
            iv->lu = X;
            iv->lv = Y;
        } else {
            // Point is behind the light — should not happen.
            ASSERT(0);
            iv->lu = 0.0F;
            iv->lv = 0.0F;
        }
    }

    // Compute gouraud colour and bumpmap UV offsets for each shared vertex.
    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];

        dprod = is->nx * light_matrix[6] + is->ny * light_matrix[7] + is->nz * light_matrix[8];

        if (dprod < 0) {
            SLONG bright = ftol(dprod * -255.0F);
            ASSERT(WITHIN(bright, 0, 255));
            is->colour = bright | (bright << 8) | (bright << 16);
        } else {
            is->colour = 0x00000000;
        }

        dprod  = is->dxdu * light_matrix[6] + is->dydu * light_matrix[7] + is->dzdu * light_matrix[8];
        is->du = dprod * 0.005F;

        dprod  = is->dxdv * light_matrix[6] + is->dydv * light_matrix[7] + is->dzdv * light_matrix[8];
        is->dv = dprod * 0.005F;
    }
}

// uc_orig: MF_specular_spotlight (fallen/outro/mf.cpp)
void MF_specular_spotlight(
    IMP_Mesh* im,
    float light_x,
    float light_y,
    float light_z,
    float light_matrix[9],
    float light_lens)
{
    SLONG i;
    float x, y, z, X, Y, Z;
    float dx, dy, dz;
    float along_x, along_y, along_z;
    float dprod, len, rx, ry, rz, bright;
    IMP_Vert* iv;
    IMP_Svert* is;
    IMP_Mat* it;

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        iv = &im->vert[is->vert];
        it = &im->mat[is->mat];

        x = iv->x - light_x;
        y = iv->y - light_y;
        z = iv->z - light_z;

        MATRIX_MUL(light_matrix, x, y, z);

        if (z > 0.0F) {
            Z = light_lens / z;
            X = 0.5F + x * Z;
            Y = 0.5F + y * Z;

            // Compute camera-to-point ray.
            dx = iv->x - OS_cam_x;
            dy = iv->y - OS_cam_y;
            dz = iv->z - OS_cam_z;

            // Reflect the camera ray off the surface normal.
            dprod = dx * is->nx + dy * is->ny + dz * is->nz;
            dprod *= 2.0F;
            rx = dx - is->nx * dprod;
            ry = dy - is->ny * dprod;
            rz = dz - is->nz * dprod;

            {
                float overlen = 1.0F / sqrt(rx * rx + ry * ry + rz * rz);
                rx *= overlen;
                ry *= overlen;
                rz *= overlen;
            }

            along_x = rx * light_matrix[0] + ry * light_matrix[1] + rz * light_matrix[2];
            along_y = rx * light_matrix[3] + ry * light_matrix[4] + rz * light_matrix[5];
            along_z = rx * light_matrix[6] + ry * light_matrix[7] + rz * light_matrix[8];

            bright = -along_z;

            if (bright <= 0.0F) {
                is->colour = 0x00000000;
            } else {
                // Attenuate by distance from spotlight centre.
                len = (X - 0.5F) * (X - 0.5F) + (Y - 0.5F) * (Y - 0.5F);

                if (len > 0.25F) {
                    is->colour = 0x00000000;
                } else {
                    len *= 4.0F;
                    len *= len;
                    len *= len;
                    len = 1.0F - len;
                    bright *= len;

                    is->colour = (int)(bright * (255.0F * it->shinstr));
                    is->colour |= is->colour << 8;
                    is->colour |= is->colour << 8;
                }
            }

            // Shift UVs by the reflected ray to create the specular look.
            X += along_x * it->shininess;
            Y += along_y * it->shininess;

            is->lu = X;
            is->lv = Y;
        } else {
            // Point is behind the light — should not happen.
            ASSERT(0);
            is->lu = 0.0F;
            is->lv = 0.0F;
        }
    }
}

// uc_orig: MF_add_triangles_normal (fallen/outro/mf.cpp)
void MF_add_triangles_normal(IMP_Mesh* im, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x00000000;
        ov->u1       = is->u;
        ov->v1       = is->v;
        ov->u2       = 0.0F;
        ov->v2       = 0.0F;
    }

    for (i = 0; i < im->num_mats; i++) {
        im->mat[i].ob = OS_buffer_new();
    }

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        it = &im->mat[ic->mat];
        OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];
        OS_buffer_draw(it->ob, it->ot_tex, NULL, draw | ((it->sided == IMP_SIDED_DOUBLE) ? OS_DRAW_DOUBLESIDED : 0));
    }
}

// uc_orig: MF_add_triangles_normal_colour (fallen/outro/mf.cpp)
void MF_add_triangles_normal_colour(IMP_Mesh* im, ULONG draw, ULONG colour)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = colour;
        ov->specular = 0x00000000;
        ov->u1       = is->u;
        ov->v1       = is->v;
        ov->u2       = 0.0F;
        ov->v2       = 0.0F;
    }

    for (i = 0; i < im->num_mats; i++) {
        im->mat[i].ob = OS_buffer_new();
    }

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        it = &im->mat[ic->mat];
        OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];
        OS_buffer_draw(it->ob, it->ot_tex, NULL, draw | ((it->sided == IMP_SIDED_DOUBLE) ? OS_DRAW_DOUBLESIDED : 0));
    }
}

// uc_orig: MF_add_triangles_light (fallen/outro/mf.cpp)
void MF_add_triangles_light(IMP_Mesh* im, OS_Texture* ot, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Face* ic;
    IMP_Vert* iv;
    OS_Buffer* ob;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];
        iv = &im->vert[is->vert];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x00000000;
        ov->u1       = iv->lu;
        ov->v1       = iv->lv;
        ov->u2       = 0.0F;
        ov->v2       = 0.0F;
    }

    ob = OS_buffer_new();

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        OS_buffer_add_triangle(ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    OS_buffer_draw(ob, ot, NULL, draw);
}

// uc_orig: MF_add_triangles_light_bumpmapped (fallen/outro/mf.cpp)
void MF_add_triangles_light_bumpmapped(IMP_Mesh* im, OS_Texture* ot, ULONG draw)
{
    SLONG i, pass;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;
    IMP_Vert* iv;

    for (pass = 0; pass < 2; pass++) {
        if (pass == 0) {
            ASSERT(im->num_sverts <= MF_MAX_SVERTS);

            for (i = 0; i < im->num_sverts; i++) {
                is = &im->svert[i];
                ov = &MF_vert[i];
                iv = &im->vert[is->vert];

                ov->trans    = is->vert;
                ov->index    = NULL;
                ov->colour   = is->colour;
                ov->specular = 0x00000000;
                ov->u1       = iv->lu;
                ov->v1       = iv->lv;
                ov->u2       = is->u + is->du;
                ov->v2       = is->v + is->dv;
            }
        } else {
            // Second pass: shift bump UVs the other direction.
            for (i = 0; i < im->num_sverts; i++) {
                ov = &MF_vert[i];
                is = &im->svert[i];

                ov->index = NULL;
                ov->u2    = is->u - is->du;
                ov->v2    = is->v - is->dv;
            }
        }

        for (i = 0; i < im->num_mats; i++) {
            it = &im->mat[i];
            if (it->has_bumpmap || pass == 0) {
                it->ob = OS_buffer_new();
            }
        }

        for (i = 0; i < im->num_faces; i++) {
            ic = &im->face[i];
            it = &im->mat[ic->mat];
            if (it->has_bumpmap || pass == 0) {
                OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
            }
        }

        for (i = 0; i < im->num_mats; i++) {
            it = &im->mat[i];
            if (it->has_bumpmap) {
                OS_buffer_draw(it->ob, ot, (pass == 0) ? it->ot_bpos : it->ot_bneg, draw | OS_DRAW_TEX_MUL);
            } else {
                if (pass == 0) {
                    OS_buffer_draw(it->ob, ot, NULL, draw);
                }
            }
        }

        draw |= OS_DRAW_ADD; // Second pass is always additive.
    }
}

// uc_orig: MF_add_triangles_specular (fallen/outro/mf.cpp)
void MF_add_triangles_specular(IMP_Mesh* im, OS_Texture* ot, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Face* ic;
    OS_Buffer* ob;
    OS_Vert* ov1;
    OS_Vert* ov2;
    OS_Vert* ov3;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x000000;
        ov->u1       = is->lu;
        ov->v1       = is->lv;
        ov->u2       = 0.0F;
        ov->v2       = 0.0F;
    }

    ob = OS_buffer_new();

    for (i = 0; i < im->num_faces; i++) {
        ic  = &im->face[i];
        ov1 = &MF_vert[ic->s[0]];
        ov2 = &MF_vert[ic->s[1]];
        ov3 = &MF_vert[ic->s[2]];
        OS_buffer_add_triangle(ob, ov1, ov2, ov3);
    }

    OS_buffer_draw(ob, ot, NULL, draw);
}

// uc_orig: MF_add_triangles_specular_bumpmapped (fallen/outro/mf.cpp)
void MF_add_triangles_specular_bumpmapped(IMP_Mesh* im, OS_Texture* ot, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x00000000;
        ov->u1       = is->lu;
        ov->v1       = is->lv;
        ov->u2       = is->u;
        ov->v2       = is->v;
    }

    for (i = 0; i < im->num_mats; i++) {
        im->mat[i].ob = OS_buffer_new();
    }

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        it = &im->mat[ic->mat];
        OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];
        if (it->has_bumpmap) {
            OS_buffer_draw(it->ob, ot, it->ot_bpos, draw | OS_DRAW_TEX_MUL);
        } else {
            OS_buffer_draw(it->ob, ot, NULL, draw);
        }
    }
}

// uc_orig: MF_add_triangles_specular_shadowed (fallen/outro/mf.cpp)
void MF_add_triangles_specular_shadowed(IMP_Mesh* im, OS_Texture* ot_specdot, OS_Texture* ot_diffdot, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Face* ic;
    IMP_Vert* iv;
    OS_Buffer* ob;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];
        iv = &im->vert[is->vert];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x00000000;
        ov->u1       = is->lu;
        ov->v1       = is->lv;
        ov->u2       = iv->lu;
        ov->v2       = iv->lv;
    }

    ob = OS_buffer_new();

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        OS_buffer_add_triangle(ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    OS_buffer_draw(ob, ot_specdot, ot_diffdot, draw);
}

// uc_orig: MF_add_wireframe (fallen/outro/mf.cpp)
void MF_add_wireframe(IMP_Mesh* im, OS_Texture* ot, ULONG colour, float width, ULONG draw)
{
    SLONG i;
    OS_Trans* ot1;
    OS_Trans* ot2;
    OS_Buffer* ob = OS_buffer_new();

    for (i = 0; i < im->num_lines; i++) {
        ot1 = &OS_trans[im->line[i].v1];
        ot2 = &OS_trans[im->line[i].v2];

        if (ot1->clip & ot2->clip & OS_CLIP_TRANSFORMED) {
            OS_buffer_add_line_3d(
                ob,
                ot1->X, ot1->Y,
                ot2->X, ot2->Y,
                width,
                0.0F, 0.0F,
                1.0F, 1.0F,
                1.0F - ot1->Z,
                1.0F - ot2->Z,
                colour);
        }
    }

    OS_buffer_draw(ob, ot, NULL, draw);
}

// uc_orig: MF_add_triangles_bumpmapped_pass (fallen/outro/mf.cpp)
void MF_add_triangles_bumpmapped_pass(IMP_Mesh* im, SLONG pass, ULONG draw)
{
    SLONG i;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;

    if (pass == 0) {
        ASSERT(im->num_sverts <= MF_MAX_SVERTS);

        for (i = 0; i < im->num_sverts; i++) {
            is = &im->svert[i];
            ov = &MF_vert[i];

            ov->trans    = is->vert;
            ov->index    = NULL;
            ov->colour   = is->colour;
            ov->specular = 0x00000000;
            ov->u1       = is->u + is->du;
            ov->v1       = is->v + is->dv;
            ov->u2       = 0.0F;
            ov->v2       = 0.0F;
        }
    } else {
        // Pass 1: shift UVs in the opposite direction.
        ASSERT(im->num_sverts <= MF_MAX_SVERTS);

        for (i = 0; i < im->num_sverts; i++) {
            is = &im->svert[i];
            ov = &MF_vert[i];

            ov->trans    = is->vert;
            ov->index    = NULL;
            ov->colour   = is->colour;
            ov->specular = 0x00000000;
            ov->u1       = is->u - is->du;
            ov->v1       = is->v - is->dv;
            ov->u2       = 0.0F;
            ov->v2       = 0.0F;
        }
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];
        if (it->has_bumpmap) {
            it->ob = OS_buffer_new();
        }
    }

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        it = &im->mat[ic->mat];
        if (it->has_bumpmap) {
            OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
        }
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];
        if (it->has_bumpmap) {
            OS_buffer_draw(it->ob, (pass == 0) ? it->ot_bpos : it->ot_bneg, NULL, draw);
        }
    }
}

// uc_orig: MF_add_triangles_texture_after_bumpmap (fallen/outro/mf.cpp)
void MF_add_triangles_texture_after_bumpmap(IMP_Mesh* im)
{
    SLONG i;
    ULONG draw;
    IMP_Svert* is;
    OS_Vert* ov;
    IMP_Mat* it;
    IMP_Face* ic;

    ASSERT(im->num_sverts <= MF_MAX_SVERTS);

    for (i = 0; i < im->num_sverts; i++) {
        is = &im->svert[i];
        ov = &MF_vert[i];

        ov->trans    = is->vert;
        ov->index    = NULL;
        ov->colour   = is->colour;
        ov->specular = 0x00000000;
        ov->u1       = is->u;
        ov->v1       = is->v;
        ov->u2       = 0.0F;
        ov->v2       = 0.0F;
    }

    for (i = 0; i < im->num_mats; i++) {
        im->mat[i].ob = OS_buffer_new();
    }

    for (i = 0; i < im->num_faces; i++) {
        ic = &im->face[i];
        it = &im->mat[ic->mat];
        OS_buffer_add_triangle(it->ob, &MF_vert[ic->s[0]], &MF_vert[ic->s[1]], &MF_vert[ic->s[2]]);
    }

    for (i = 0; i < im->num_mats; i++) {
        it = &im->mat[i];

        draw = OS_DRAW_NORMAL;
        if (it->sided)      draw |= OS_DRAW_DOUBLESIDED;
        if (it->has_bumpmap) draw |= OS_DRAW_MULBYONE | OS_DRAW_DECAL;

        OS_buffer_draw(it->ob, it->ot_tex, NULL, draw);
    }
}
