#include "engine/animation/morph.h"
#include "engine/animation/morph_globals.h"
#include "engine/core/macros.h"
#include "engine/platform/uc_common.h"
#include <cstdio>

// uc_orig: MORPH_load (fallen/Source/morph.cpp)
void MORPH_load()
{
    SLONG i;
    SLONG d;
    float x;
    float y;
    float z;
    SLONG match;
    MORPH_Morph* mm;
    FILE* handle;
    char line[256];
    char* ch;

    MORPH_point_upto = 0;

    for (i = 0; i < MORPH_NUMBER; i++) {
        mm = &MORPH_morph[i];
        mm->num_points = 0;
        mm->index = MORPH_point_upto;

        handle = MF_Fopen(MORPH_filename[i], "rb");

        if (handle != NULL) {
            while (fgets(line, 256, handle)) {
                // Normalise European decimal separators (comma → dot).
                for (ch = line; *ch; ch++) {
                    if (*ch == ',') {
                        *ch = '.';
                    }
                }

                // 3DS Max ASCII export format: "Vertex N: X: f Y: f Z: f"
                match = sscanf(line, "Vertex %d: X: %f Y: %f Z: %f", &d, &x, &y, &z);
                if (match == 4) {
                    ASSERT(WITHIN(MORPH_point_upto, 0, MORPH_MAX_POINTS - 1));
                    // Convert from 3DS orientation (Y-up) to engine orientation (Z-up).
                    SWAP_FL(y, z);
                    x *= 2.56F;
                    y *= 2.56F;
                    z *= 2.56F;
                    MORPH_point[MORPH_point_upto].x = SWORD(x);
                    MORPH_point[MORPH_point_upto].y = SWORD(y);
                    MORPH_point[MORPH_point_upto].z = SWORD(z);
                    MORPH_point_upto += 1;
                    mm->num_points += 1;
                }

                // Alternative 3DS export format: "Vertex: (f,f,f)"
                match = sscanf(line, "Vertex: (%f,%f,%f)", &x, &y, &z);
                if (match == 3) {
                    ASSERT(WITHIN(MORPH_point_upto, 0, MORPH_MAX_POINTS - 1));
                    SWAP_FL(y, z);
                    x *= 2.56F;
                    y *= 2.56F;
                    z *= 2.56F;
                    MORPH_point[MORPH_point_upto].x = SWORD(x);
                    MORPH_point[MORPH_point_upto].y = SWORD(y);
                    MORPH_point[MORPH_point_upto].z = SWORD(z);
                    MORPH_point_upto += 1;
                    mm->num_points += 1;
                }
            }

            MF_Fclose(handle);
        }
    }
}


