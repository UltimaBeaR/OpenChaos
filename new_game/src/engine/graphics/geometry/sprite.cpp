#include "engine/platform/uc_common.h"
#include "engine/graphics/pipeline/poly.h"
#include "engine/graphics/geometry/sprite.h"
#include "engine/core/math.h"
// Display resolution constants (defined as macros, matching gd_display.h).
#define DisplayWidth  640
#define DisplayHeight 480

// uc_orig: SPRITE_draw_tex_distorted (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw_tex_distorted(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    float u, float v, float w, float h,
    float wx1, float wy1, float wx2, float wy2, float wx3, float wy3, float wx4, float wy4,
    SLONG sort)
{
    float screen_size;

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    POLY_transform(
        world_x,
        world_y,
        world_z,
        &mid);

    if (mid.IsValid()) {
        screen_size = POLY_world_length_to_screen(world_size) * mid.Z;

        if (mid.X + screen_size < 0 || mid.X - screen_size > POLY_screen_width || mid.Y + screen_size < 0 || mid.Y - screen_size > POLY_screen_height) {
            // Off screen.
        } else {
            pp[0].X = mid.X - screen_size + wx1;
            pp[0].Y = mid.Y - screen_size + wy1;
            pp[1].X = mid.X + screen_size + wx2;
            pp[1].Y = mid.Y - screen_size + wy2;
            pp[2].X = mid.X - screen_size + wx3;
            pp[2].Y = mid.Y + screen_size + wy3;
            pp[3].X = mid.X + screen_size + wx4;
            pp[3].Y = mid.Y + screen_size + wy4;

            pp[0].u = u;
            pp[0].v = v;
            pp[1].u = u + w;
            pp[1].v = v;
            pp[2].u = u;
            pp[2].v = v + h;
            pp[3].u = u + w;
            pp[3].v = v + h;

            pp[0].colour = colour;
            pp[1].colour = colour;
            pp[2].colour = colour;
            pp[3].colour = colour;

            pp[0].specular = specular;
            pp[1].specular = specular;
            pp[2].specular = specular;
            pp[3].specular = specular;

            switch (sort) {
            case SPRITE_SORT_NORMAL:
                pp[0].z = mid.z;
                pp[0].Z = mid.Z;
                pp[1].z = mid.z;
                pp[1].Z = mid.Z;
                pp[2].z = mid.z;
                pp[2].Z = mid.Z;
                pp[3].z = mid.z;
                pp[3].Z = mid.Z;
                break;

            case SPRITE_SORT_FRONT:
                pp[0].z = 0.01F;
                pp[0].Z = 1.00F;
                pp[1].z = 0.01F;
                pp[1].Z = 1.00F;
                pp[2].z = 0.01F;
                pp[2].Z = 1.00F;
                pp[3].z = 0.01F;
                pp[3].Z = 1.00F;
                break;

            default:
                ASSERT(0);
            }

            quad[0] = &pp[0];
            quad[1] = &pp[1];
            quad[2] = &pp[2];
            quad[3] = &pp[3];

            POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
        }
    }
}

// uc_orig: SPRITE_draw (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    SLONG sort)
{
    float screen_size;

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    POLY_transform(
        world_x,
        world_y,
        world_z,
        &mid);

    if (mid.IsValid()) {
        screen_size = POLY_world_length_to_screen(world_size) * mid.Z;

        if (mid.X + screen_size < 0 || mid.X - screen_size > POLY_screen_width || mid.Y + screen_size < 0 || mid.Y - screen_size > POLY_screen_height) {
            // Off screen.
        } else {
            pp[0].X = mid.X - screen_size;
            pp[0].Y = mid.Y - screen_size;
            pp[1].X = mid.X + screen_size;
            pp[1].Y = mid.Y - screen_size;
            pp[2].X = mid.X - screen_size;
            pp[2].Y = mid.Y + screen_size;
            pp[3].X = mid.X + screen_size;
            pp[3].Y = mid.Y + screen_size;

            pp[0].u = 0.0F;
            pp[0].v = 0.0F;
            pp[1].u = 1.0F;
            pp[1].v = 0.0F;
            pp[2].u = 0.0F;
            pp[2].v = 1.0F;
            pp[3].u = 1.0F;
            pp[3].v = 1.0F;

            pp[0].colour = colour;
            pp[1].colour = colour;
            pp[2].colour = colour;
            pp[3].colour = colour;

            pp[0].specular = specular;
            pp[1].specular = specular;
            pp[2].specular = specular;
            pp[3].specular = specular;

            switch (sort) {
            case SPRITE_SORT_NORMAL:
                pp[0].z = mid.z;
                pp[0].Z = mid.Z;
                pp[1].z = mid.z;
                pp[1].Z = mid.Z;
                pp[2].z = mid.z;
                pp[2].Z = mid.Z;
                pp[3].z = mid.z;
                pp[3].Z = mid.Z;
                break;

            case SPRITE_SORT_FRONT:
                pp[0].z = 0.01F;
                pp[0].Z = 1.00F;
                pp[1].z = 0.01F;
                pp[1].Z = 1.00F;
                pp[2].z = 0.01F;
                pp[2].Z = 1.00F;
                pp[3].z = 0.01F;
                pp[3].Z = 1.00F;
                break;

            default:
                ASSERT(0);
            }

            quad[0] = &pp[0];
            quad[1] = &pp[1];
            quad[2] = &pp[2];
            quad[3] = &pp[3];

            POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
        }
    }
}

// uc_orig: SPRITE_draw_tex (fallen/DDEngine/Source/sprite.cpp)
void SPRITE_draw_tex(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    ULONG specular,
    SLONG page,
    float u, float v, float w, float h,
    SLONG sort)
{
    float screen_size;

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];

    POLY_transform(
        world_x,
        world_y,
        world_z,
        &mid,
        UC_TRUE);

    if (mid.IsValid()) {
        screen_size = POLY_world_length_to_screen(world_size) * mid.Z;

        if (mid.X + screen_size < 0 || mid.X - screen_size > POLY_screen_width || mid.Y + screen_size < 0 || mid.Y - screen_size > POLY_screen_height) {
            // Off screen.
        } else {
            pp[0].X = mid.X - screen_size;
            pp[0].Y = mid.Y - screen_size;
            pp[1].X = mid.X + screen_size;
            pp[1].Y = mid.Y - screen_size;
            pp[2].X = mid.X - screen_size;
            pp[2].Y = mid.Y + screen_size;
            pp[3].X = mid.X + screen_size;
            pp[3].Y = mid.Y + screen_size;

            pp[0].u = u;
            pp[0].v = v;
            pp[1].u = u + w;
            pp[1].v = v;
            pp[2].u = u;
            pp[2].v = v + h;
            pp[3].u = u + w;
            pp[3].v = v + h;

            pp[0].colour = colour;
            pp[1].colour = colour;
            pp[2].colour = colour;
            pp[3].colour = colour;

            pp[0].specular = specular;
            pp[1].specular = specular;
            pp[2].specular = specular;
            pp[3].specular = specular;

            switch (sort) {
            case SPRITE_SORT_NORMAL:
                pp[0].z = mid.z;
                pp[0].Z = mid.Z;
                pp[1].z = mid.z;
                pp[1].Z = mid.Z;
                pp[2].z = mid.z;
                pp[2].Z = mid.Z;
                pp[3].z = mid.z;
                pp[3].Z = mid.Z;
                break;

            case SPRITE_SORT_FRONT:
                pp[0].z = 0.01F;
                pp[0].Z = 1.00F;
                pp[1].z = 0.01F;
                pp[1].Z = 1.00F;
                pp[2].z = 0.01F;
                pp[2].Z = 1.00F;
                pp[3].z = 0.01F;
                pp[3].Z = 1.00F;
                break;

            default:
                ASSERT(0);
            }

            quad[0] = &pp[0];
            quad[1] = &pp[1];
            quad[2] = &pp[2];
            quad[3] = &pp[3];

            POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
        }
    }
}

// uc_orig: SPRITE_draw_rotated (fallen/DDEngine/Source/drawxtra.cpp)
// Like SPRITE_draw but the quad is rotated around the screen-space centre by `rotate` angle
// (in PSX 0-2047 units). If rotate == 0xffffff, the angle is computed automatically so that
// the sprite faces away from the screen centre (billboard-style lens glow behaviour).
void SPRITE_draw_rotated(
    float world_x,
    float world_y,
    float world_z,
    float world_size,
    ULONG colour,
    SLONG page,
    SLONG sort,
    SLONG rotate)
{
    float screen_size;

    POLY_Point mid;
    POLY_Point pp[4];
    POLY_Point* quad[4];
    SLONG opp, adj, xofs, yofs, angle;

    POLY_transform(
        world_x,
        world_y,
        world_z,
        &mid);

    if (mid.IsValid()) {
        screen_size = POLY_world_length_to_screen(world_size) * mid.Z;

        if (mid.X + screen_size < 0 || mid.X - screen_size > POLY_screen_width || mid.Y + screen_size < 0 || mid.Y - screen_size > POLY_screen_height) {
            // Off screen.
        } else {
            if (rotate == 0xffffff) {
                opp = (DisplayWidth >> 1) - mid.X;
                adj = (DisplayHeight >> 1) - mid.Y;
                angle = -Arctan(opp, adj);
                angle &= 2047;
            } else
                angle = rotate;

            xofs = screen_size * SIN(angle);
            yofs = screen_size * COS(angle);
            xofs >>= 16;
            yofs >>= 16;

            pp[0].X = mid.X - xofs;
            pp[0].Y = mid.Y - yofs;
            pp[1].X = mid.X + yofs;
            pp[1].Y = mid.Y - xofs;
            pp[2].X = mid.X - yofs;
            pp[2].Y = mid.Y + xofs;
            pp[3].X = mid.X + xofs;
            pp[3].Y = mid.Y + yofs;

            pp[0].u = 0;
            pp[0].v = 0;
            pp[1].u = 1;
            pp[1].v = 0;
            pp[2].u = 0;
            pp[2].v = 1;
            pp[3].u = 1;
            pp[3].v = 1;

            pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = colour;

            pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xFF000000;

            switch (sort) {
            case SPRITE_SORT_NORMAL:

                mid.z -= 1.0F / 64.0F;
                mid.Z += 1.0F / 64.0F;

                if (mid.z < 0.0F) {
                    mid.z = 0.0F;
                }

                if (mid.Z > 0.999F) {
                    mid.Z = 0.999F;
                }

                pp[0].z = mid.z;
                pp[0].Z = mid.Z;
                pp[1].z = mid.z;
                pp[1].Z = mid.Z;
                pp[2].z = mid.z;
                pp[2].Z = mid.Z;
                pp[3].z = mid.z;
                pp[3].Z = mid.Z;
                break;

            case SPRITE_SORT_FRONT:
                pp[0].z = 0.01F;
                pp[0].Z = 1.00F;
                pp[1].z = 0.01F;
                pp[1].Z = 1.00F;
                pp[2].z = 0.01F;
                pp[2].Z = 1.00F;
                pp[3].z = 0.01F;
                pp[3].Z = 1.00F;
                break;

            default:
                ASSERT(0);
            }

            quad[0] = &pp[0];
            quad[1] = &pp[1];
            quad[2] = &pp[2];
            quad[3] = &pp[3];

            POLY_add_quad(quad, page, UC_FALSE, UC_TRUE);
        }
    }
}
