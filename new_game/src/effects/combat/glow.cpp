// uc_orig: drawxtra.cpp (fallen/DDEngine/Source/drawxtra.cpp)
// Rotating glow overlay for the Guardian of Baalrog (final boss).


#include "engine/graphics/pipeline/poly.h"
#include "engine/core/math.h"              // SIN, COS lookup tables
#include "game/game_types.h"

#include "effects/combat/glow.h"

// uc_orig: DRAWXTRA_final_glow (fallen/DDEngine/Source/drawxtra.cpp)
void DRAWXTRA_final_glow(SLONG x, SLONG y, SLONG z, UBYTE fade)
{
    static SLONG rotation = 0;

    POLY_Point mid;

    POLY_flush_local_rot();

    POLY_transform(
        float(x),
        float(y),
        float(z),
        &mid);

    if (!(mid.clip & POLY_CLIP_TRANSFORMED)) {
        return;
    }

    rotation += 10 * TICK_RATIO >> TICK_SHIFT;

    // Push the glow slightly forward in the z-buffer to avoid z-fighting.
    mid.z -= 1.0F / 22.0F;

    if (mid.z < POLY_ZCLIP_PLANE) {
        mid.z = POLY_ZCLIP_PLANE;
    }

    mid.Z = POLY_ZCLIP_PLANE / mid.z;

    SLONG i;
    SLONG j;
    SLONG angle;
    SLONG colour;

    float dx;
    float dy;

    POLY_Point pp[4];
    POLY_Point* quad[4];

    colour = fade * 0x60 >> 8;
    colour |= colour << 8;
    colour |= colour << 8;

    pp[0].u = 0.0F;
    pp[0].v = 0.0F;
    pp[0].colour = colour;
    pp[0].specular = 0x00000000;
    pp[0].Z = mid.Z;
    pp[0].z = mid.z;

    pp[1].u = 1.0F;
    pp[1].v = 0.0F;
    pp[1].colour = colour;
    pp[1].specular = 0x00000000;
    pp[1].Z = mid.Z;
    pp[1].z = mid.z;

    pp[2].u = 0.0F;
    pp[2].v = 1.0F;
    pp[2].colour = colour;
    pp[2].specular = 0x00000000;
    pp[2].Z = mid.Z;
    pp[2].z = mid.z;

    pp[3].u = 1.0F;
    pp[3].v = 1.0F;
    pp[3].colour = colour;
    pp[3].specular = 0x00000000;
    pp[3].Z = mid.Z;
    pp[3].z = mid.z;

    POLY_fadeout_point(&pp[0]);
    POLY_fadeout_point(&pp[1]);
    POLY_fadeout_point(&pp[2]);
    POLY_fadeout_point(&pp[3]);

    quad[0] = &pp[0];
    quad[1] = &pp[1];
    quad[2] = &pp[2];
    quad[3] = &pp[3];

    for (i = 0; i < 4; i++) {
        switch (i) {
        case 0:
            angle = rotation >> 0;
            break;
        case 1:
            angle = -rotation >> 0;
            break;
        case 2:
            angle = rotation >> 1;
            break;
        case 3:
            angle = -rotation >> 1;
            break;
        }

        angle += i << 6;
        angle += i << 3;
        angle &= 2047;

        dx = float(SIN(angle)) * mid.Z * (1.0F / 32.0F);
        dy = float(COS(angle)) * mid.Z * (1.0F / 32.0F);

        for (j = 0; j < 4; j++) {
            if (j & 1) {
                pp[j].X = mid.X + dx;
                pp[j].Y = mid.Y + dy;
            } else {
                pp[j].X = mid.X - dx;
                pp[j].Y = mid.Y - dy;
            }

            if (j & 2) {
                pp[j].X += -dy;
                pp[j].Y += +dx;
            } else {
                pp[j].X -= -dy;
                pp[j].Y -= +dx;
            }
        }

        POLY_add_quad(quad, POLY_PAGE_FINALGLOW, UC_FALSE, UC_TRUE);
    }
}
