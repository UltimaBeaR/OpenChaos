// Temporary: game.h brings in Thing, POLY_FLAG_DOUBLESIDED, building_objects, building_facets,
// prim_faces4/3, and many other game-level types needed here.
#include "game.h"
#include "aeng.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/lighting/light.h"
// Temporary: memory.h needed for prim_faces4/prim_faces3 externs
#include "fallen/Headers/memory.h"
#include "world/environment/build.h"
#include "world/environment/build_globals.h"

// uc_orig: BUILD_draw (fallen/DDEngine/Source/build.cpp)
void BUILD_draw(Thing* p_thing)
{
    SLONG i;
    SLONG j;

    SLONG sp;
    SLONG ep;

    SLONG p0;
    SLONG p1;
    SLONG p2;
    SLONG p3;

    LIGHT_Colour pcol;

    PrimFace4* p_f4;
    PrimFace3* p_f3;
    PrimObject* p_obj;

    POLY_Point* pp;
    POLY_Point* ps;

    POLY_Point* tri[3];
    POLY_Point* quad[4];

    SLONG page;
    SLONG backface_cull;
    ULONG shadow;
    ULONG face_colour;
    ULONG face_specular;

    float bx = float(p_thing->WorldPos.X >> 8);
    float by = float(p_thing->WorldPos.Y >> 8);
    float bz = float(p_thing->WorldPos.Z >> 8);

    SLONG bo_index;
    SLONG bf_index;

    BuildingFacet* bf;
    BuildingObject* bo;

    bo_index = p_thing->Index;
    bo = &building_objects[bo_index];

    // Shadow colour derived from ambient light.
    shadow = ((LIGHT_amb_colour.red >> 1) << 16) | ((LIGHT_amb_colour.green >> 1) << 8) | ((LIGHT_amb_colour.blue >> 1) << 0);

    ULONG colour;
    ULONG specular;

    LIGHT_get_d3d_colour(
        LIGHT_amb_colour,
        &colour,
        &specular);

    // Triangles always use full white colour regardless of ambient.
    colour = 0xffffffff;
    specular = 0xffffffff;

    bf_index = bo->FacetHead;

    while (bf_index) {
        bf = &building_facets[bf_index];

        sp = bf->StartPoint;
        ep = bf->EndPoint;

        POLY_buffer_upto = 0;

        for (i = sp; i < ep; i++) {
            ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

            pp = &POLY_buffer[POLY_buffer_upto++];

            POLY_transform(
                AENG_dx_prim_points[i].X + bx,
                AENG_dx_prim_points[i].Y + by,
                AENG_dx_prim_points[i].Z + bz,
                pp);

            if (pp->MaybeValid()) {
                LIGHT_get_d3d_colour(
                    LIGHT_building_point[i],
                    &pp->colour,
                    &pp->specular);

                POLY_fadeout_point(pp);
            }
        }

        for (i = bf->StartFace4; i < bf->EndFace4; i++) {
            p_f4 = &prim_faces4[i];

            p0 = p_f4->Points[0] - sp;
            p1 = p_f4->Points[1] - sp;
            p2 = p_f4->Points[2] - sp;
            p3 = p_f4->Points[3] - sp;

            ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p3, 0, POLY_buffer_upto - 1));

            quad[0] = &POLY_buffer[p0];
            quad[1] = &POLY_buffer[p1];
            quad[2] = &POLY_buffer[p2];
            quad[3] = &POLY_buffer[p3];

            if (POLY_valid_quad(quad)) {
                backface_cull = !(p_f4->DrawFlags & POLY_FLAG_DOUBLESIDED);

                quad[0]->u = float(p_f4->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                quad[0]->v = float(p_f4->UV[0][1]) * (1.0F / 32.0F);

                quad[1]->u = float(p_f4->UV[1][0]) * (1.0F / 32.0F);
                quad[1]->v = float(p_f4->UV[1][1]) * (1.0F / 32.0F);

                quad[2]->u = float(p_f4->UV[2][0]) * (1.0F / 32.0F);
                quad[2]->v = float(p_f4->UV[2][1]) * (1.0F / 32.0F);

                quad[3]->u = float(p_f4->UV[3][0]) * (1.0F / 32.0F);
                quad[3]->v = float(p_f4->UV[3][1]) * (1.0F / 32.0F);

                page = p_f4->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f4->TexturePage;

                if (p_f4->FaceFlags & (FACE_FLAG_SHADOW_1 | FACE_FLAG_SHADOW_2)) {
                    POLY_Point ps[4];

                    ps[0] = *(quad[0]);
                    ps[1] = *(quad[1]);
                    ps[2] = *(quad[2]);
                    ps[3] = *(quad[3]);

                    for (j = 0; j < 4; j++) {
                        pcol = LIGHT_building_point[p_f4->Points[j]];

                        pcol.red -= 16;
                        pcol.green -= 16;
                        pcol.blue -= 16;

                        LIGHT_get_d3d_colour(
                            pcol,
                            &ps[j].colour,
                            &ps[j].specular);
                    }

                    if ((p_f4->FaceFlags & FACE_FLAG_SHADOW_1) && (p_f4->FaceFlags & FACE_FLAG_SHADOW_2)) {
                        quad[0] = &ps[0];
                        quad[1] = &ps[1];
                        quad[2] = &ps[2];
                        quad[3] = &ps[3];

                        POLY_add_quad(quad, page, backface_cull);
                    } else {
                        if (p_f4->FaceFlags & FACE_FLAG_SHADOW_2) {
                            tri[0] = quad[0];
                            tri[1] = quad[1];
                            tri[2] = quad[2];

                            POLY_add_triangle(tri, page, backface_cull);

                            tri[0] = &ps[1];
                            tri[1] = &ps[3];
                            tri[2] = &ps[2];

                            POLY_add_triangle(tri, page, backface_cull);
                        } else {
                            tri[0] = quad[1];
                            tri[1] = quad[3];
                            tri[2] = quad[2];

                            POLY_add_triangle(tri, page, backface_cull);

                            tri[0] = &ps[0];
                            tri[1] = &ps[1];
                            tri[2] = &ps[2];

                            POLY_add_triangle(tri, page, backface_cull);
                        }
                    }
                } else {
                    POLY_add_quad(quad, page, backface_cull);
                }
            }
        }

        for (i = bf->StartFace3; i < bf->EndFace3; i++) {
            p_f3 = &prim_faces3[i];

            p0 = p_f3->Points[0] - sp;
            p1 = p_f3->Points[1] - sp;
            p2 = p_f3->Points[2] - sp;

            ASSERT(WITHIN(p0, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p1, 0, POLY_buffer_upto - 1));
            ASSERT(WITHIN(p2, 0, POLY_buffer_upto - 1));

            tri[0] = &POLY_buffer[p0];
            tri[1] = &POLY_buffer[p1];
            tri[2] = &POLY_buffer[p2];

            if (POLY_valid_triangle(tri)) {
                backface_cull = !(p_f3->DrawFlags & POLY_FLAG_DOUBLESIDED);

                tri[0]->u = float(p_f3->UV[0][0] & 0x3f) * (1.0F / 32.0F);
                tri[0]->v = float(p_f3->UV[0][1]) * (1.0F / 32.0F);

                tri[1]->u = float(p_f3->UV[1][0]) * (1.0F / 32.0F);
                tri[1]->v = float(p_f3->UV[1][1]) * (1.0F / 32.0F);

                tri[2]->u = float(p_f3->UV[2][0]) * (1.0F / 32.0F);
                tri[2]->v = float(p_f3->UV[2][1]) * (1.0F / 32.0F);

                page = p_f3->UV[0][0] & 0xc0;
                page <<= 2;
                page |= p_f3->TexturePage;

                POLY_add_triangle(tri, page, backface_cull);
            }
        }

        bf_index = bf->NextFacet;
    }
}

// uc_orig: BUILD_draw_inside (fallen/DDEngine/Source/build.cpp)
// The entire implementation is commented out in the pre-release codebase.
void BUILD_draw_inside()
{
}
