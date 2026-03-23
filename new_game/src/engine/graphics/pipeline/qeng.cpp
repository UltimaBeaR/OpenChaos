#include <MFStdLib.h>
#include "engine/graphics/graphics_api/display_macros.h" // FLIP, SET_BLACK_BACKGROUND, CLEAR_VIEWPORT
#include "engine/graphics/pipeline/qeng.h"
#include "engine/graphics/pipeline/qeng_globals.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/pipeline/vertex_buffer.h"
#include "world/map/qmap_globals.h"

// uc_orig: QENG_init (fallen/DDEngine/Source/qeng.cpp)
void QENG_init()
{
    POLY_init();
}

// uc_orig: QENG_set_camera (fallen/DDEngine/Source/qeng.cpp)
void QENG_set_camera(float x, float y, float z, float yaw, float pitch, float roll)
{
    QENG_cam_x = SLONG(x);
    QENG_cam_y = SLONG(y);
    QENG_cam_z = SLONG(z);

    POLY_camera_set(x, y, z, yaw, pitch, roll, 128.0F * 256.0F, 8.0F);
    POLY_frame_init(UC_FALSE, UC_FALSE);
}

// uc_orig: QENG_world_line (fallen/DDEngine/Source/qeng.cpp)
void QENG_world_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    bool sort_to_front)
{
    POLY_Point pp1;
    POLY_Point pp2;

    QENG_mouse_over = UC_FALSE;

    POLY_transform(float(x1), float(y1), float(z1), &pp1);

    if (pp1.IsValid()) {
        POLY_transform(float(x2), float(y2), float(z2), &pp2);

        if (pp2.IsValid()) {
            if (POLY_valid_line(&pp1, &pp2)) {
                pp1.colour = colour1;
                pp2.colour = colour2;

                pp1.specular = 0x00000000;
                pp2.specular = 0x00000000;

                pp1.u = 0.0F;
                pp1.v = 0.0F;
                pp2.u = 0.0F;
                pp2.v = 0.0F;

                POLY_add_line(&pp1, &pp2, float(width1), float(width2), POLY_PAGE_COLOUR, sort_to_front);
            }
        }
    }
}

// uc_orig: QENG_draw (fallen/DDEngine/Source/qeng.cpp)
void QENG_draw(QMAP_Draw* qd)
{
    SLONG i;

    UWORD point;
    UWORD face;

    SLONG mx;
    SLONG mz;

    QMAP_Point* qp;
    QMAP_Face* qf;

    POLY_Point* pp;
    POLY_Point* quad[4];

    ASSERT(QMAP_MAX_POINTS <= POLY_BUFFER_SIZE);

    mx = qd->map_x << 13;
    mz = qd->map_z << 13;

    // Each frame gets a unique trans counter to avoid re-transforming vertices.
    qd->trans += 1;

    if (qd->trans == 0) {
        qd->trans = 1;
        for (point = qd->next_point; point; point = qp->next) {
            ASSERT(WITHIN(point, 1, QMAP_MAX_POINTS - 1));
            qp = &QMAP_point[point];
            qp->trans = 0;
        }
    }

    for (face = qd->next_face; face; face = qf->next) {
        ASSERT(WITHIN(face, 1, QMAP_MAX_FACES - 1));

        qf = &QMAP_face[face];

        for (i = 0; i < 4; i++) {
            ASSERT(WITHIN(qf->point[i], 1, QMAP_MAX_POINTS - 1));

            qp = &QMAP_point[qf->point[i]];
            pp = &POLY_buffer[qf->point[i]];

            if (qp->trans == qd->trans) {
                // Already transformed this frame.
            } else {
                POLY_transform(float(mx + qp->x), float(qp->y), float(mz + qp->z), pp);
                qp->trans = qd->trans;
            }

            if (!pp->MaybeValid()) {
                goto abandon_this_face;
            }

            pp->colour = qf->texture * 111;
            pp->specular = 0x00000000;
            pp->u = 0.0F;
            pp->v = 0.0F;

            quad[i] = pp;
        }

        if (POLY_valid_quad(quad)) {
            POLY_add_quad(quad, POLY_PAGE_COLOUR, UC_FALSE);
        }

    abandon_this_face:;
    }
}

// uc_orig: QENG_render (fallen/DDEngine/Source/qeng.cpp)
void QENG_render()
{
    POLY_frame_draw(UC_TRUE, UC_TRUE);
}

// uc_orig: QENG_flip (fallen/DDEngine/Source/qeng.cpp)
void QENG_flip()
{
    FLIP(NULL, DDFLIP_WAIT);
}

// uc_orig: QENG_clear_screen (fallen/DDEngine/Source/qeng.cpp)
void QENG_clear_screen()
{
    SET_BLACK_BACKGROUND;
    CLEAR_VIEWPORT;
    TheVPool->ReclaimBuffers();
}

// uc_orig: QENG_fini (fallen/DDEngine/Source/qeng.cpp)
void QENG_fini()
{
}
