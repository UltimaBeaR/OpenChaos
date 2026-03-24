#include "engine/platform/uc_common.h"
#include "missions/game_types.h"
#include "effects/spark.h"
#include "effects/spark_globals.h"
#include "effects/glitter.h"
#include "effects/glitter_globals.h"
#include "engine/audio/sound.h"
#include "world/map/pap_globals.h"
#include "engine/audio/mfx.h"
#include "engine/physics/collide.h"
#include "things/core/interact.h"
#include "missions/memory.h"

// uc_orig: SPARK_init (fallen/Source/spark.cpp)
void SPARK_init()
{
    SLONG i;

    for (i = 0; i < SPARK_MAX_SPARKS; i++) {
        SPARK_spark[i].used = UC_FALSE;
    }

    SPARK_spark_last = 1;

    for (i = 0; i < MAP_HEIGHT; i++) {
        SPARK_mapwho[i] = NULL;
    }
}

// Sets random velocity on a spark point. LIMB-type points have no movement.
// uc_orig: SPARK_point_set_velocity (fallen/Source/spark.cpp)
static void SPARK_point_set_velocity(SPARK_Point* sp)
{
    if (sp->type == SPARK_TYPE_LIMB) {
        sp->dx = 0;
        sp->dy = 0;
        sp->dz = 0;
        return;
    } else {
        SLONG dx = (rand() & 0xff) - 0x1f;
        SLONG dy = (rand() & 0xff) - 0x1f;
        SLONG dz = (rand() & 0xff) - 0x1f;

        if (sp->flag & SPARK_FLAG_FAST) {
            dx <<= 2;
            dy <<= 2;
            dz <<= 2;
        } else if (sp->flag & SPARK_FLAG_SLOW) {
            dx >>= 2;
            dy >>= 2;
            dz >>= 2;
        }

        SATURATE(dx, -127, +127);
        SATURATE(dy, -127, +127);
        SATURATE(dz, -127, +127);

        if (sp->flag & SPARK_FLAG_CLAMP_X) {
            dx = 0;
        }
        if (sp->flag & SPARK_FLAG_CLAMP_Y) {
            dy = 0;
        }
        if (sp->flag & SPARK_FLAG_CLAMP_Z) {
            dz = 0;
        }

        sp->dx = dx;
        sp->dy = dy;
        sp->dz = dz;
    }
}

// Resolves the world position of a spark point. LIMB-type points track a character bone.
// uc_orig: SPARK_point_pos (fallen/Source/spark.cpp)
static void SPARK_point_pos(
    SPARK_Point* sp,
    SLONG* px,
    SLONG* py,
    SLONG* pz)
{
    if (sp->type == SPARK_TYPE_LIMB) {
        Thing* p_person = TO_THING(sp->Peep.person);

        calc_sub_objects_position(
            p_person,
            p_person->Draw.Tweened->AnimTween,
            sp->Peep.limb,
            px,
            py,
            pz);

        *px += p_person->WorldPos.X >> 8;
        *py += p_person->WorldPos.Y >> 8;
        *pz += p_person->WorldPos.Z >> 8;
    } else {
        *px = sp->Pos.x;
        *py = sp->Pos.y;
        *pz = sp->Pos.z;

        if (sp->type == SPARK_TYPE_GROUND) {
            *py = PAP_calc_map_height_at(sp->Pos.x, sp->Pos.z);
        }
    }
}

// Picks a new point near a sphere, preferring ground-level positions.
// uc_orig: SPARK_new_point (fallen/Source/spark.cpp)
static void SPARK_new_point(
    SLONG mid_x,
    SLONG mid_y,
    SLONG mid_z,
    SLONG radius,
    SPARK_Point* sp)
{
    SLONG i;

    SLONG best_x = mid_x + (rand() & 0x3f) - 0x1f;
    SLONG best_y = mid_y + (rand() & 0x3f) - 0x1f;
    SLONG best_z = mid_z + (rand() & 0x3f) - 0x1f;
    SLONG best_flag = SPARK_FLAG_FAST | SPARK_FLAG_DART_ABOUT;
    SLONG best_type = SPARK_TYPE_CIRCULAR;
    SLONG best_dist = radius;
    SLONG best_score = radius;

    SLONG x;
    SLONG y;
    SLONG z;
    SLONG dist;
    SLONG score;

    SLONG dx;
    SLONG dy;
    SLONG dz;

    for (i = 0; i < 4; i++) {
        x = mid_x + (rand() % radius) - (radius >> 1);
        z = mid_z + (rand() % radius) - (radius >> 1);

        y = calc_map_height_at(x, z);

        dx = abs(x - mid_x);
        dy = abs(y - mid_y);
        dz = abs(z - mid_z);

        dist = QDIST3(dx, dy, dz);

        score = dist;

        if (score < best_score) {
            best_x = x;
            best_y = y;
            best_z = z;
            best_flag = SPARK_FLAG_SLOW;
            best_type = SPARK_TYPE_GROUND;
            best_score = score;
        }
    }

    sp->Pos.x = best_x;
    sp->Pos.y = best_y;
    sp->Pos.z = best_z;
    sp->dist = MIN(best_dist, 255);
    sp->type = best_type;
    sp->flag = best_flag;

    SPARK_point_set_velocity(sp);
}

// Creates the two auxiliary branch points (indices 2 and 3) for an existing spark.
// uc_orig: SPARK_create_auxillary (fallen/Source/spark.cpp)
static void SPARK_create_auxillary(
    SPARK_Spark* ss,
    UBYTE point_2,
    UBYTE point_3)
{
    SLONG px1;
    SLONG py1;
    SLONG pz1;
    SLONG px2;
    SLONG py2;
    SLONG pz2;

    SPARK_point_pos(&ss->point[0], &px1, &py1, &pz1);
    SPARK_point_pos(&ss->point[1], &px2, &py2, &pz2);

    SLONG dx = px2 - px1;
    SLONG dy = py2 - py1;
    SLONG dz = pz2 - pz1;

    SLONG dist = abs(dx) + abs(dy) + abs(dz);

    if (dist < 32) {
        dist = 32;
    }

    SLONG mid_x;
    SLONG mid_y;
    SLONG mid_z;

    if (point_2) {
        mid_x = px1 + (dx >> 1);
        mid_y = py1 + (dy >> 1);
        mid_z = pz1 + (dz >> 1);

        SPARK_new_point(
            mid_x,
            mid_y,
            mid_z,
            dist >> 1,
            &ss->point[2]);
    }

    if (point_3) {
        mid_x = px2;
        mid_y = py2;
        mid_z = pz2;

        SPARK_new_point(
            mid_x,
            mid_y,
            mid_z,
            dist >> 1,
            &ss->point[3]);
    }
}

// uc_orig: SPARK_create (fallen/Source/spark.cpp)
void SPARK_create(
    SPARK_Pinfo* p1,
    SPARK_Pinfo* p2,
    UBYTE max_life)
{
    SLONG i;

    SPARK_Spark* ss;

    if (max_life == 0) {
        max_life = 1;
    }

    for (i = 0; i < SPARK_MAX_SPARKS; i++) {
        SPARK_spark_last += 1;

        if (SPARK_spark_last >= SPARK_MAX_SPARKS) {
            SPARK_spark_last = 1;
        }

        ss = &SPARK_spark[SPARK_spark_last];

        if (!ss->used) {
            goto found_spare_spark;
        }
    }

    return;

found_spare_spark:;

    ss->used = UC_TRUE;
    ss->die = rand() % max_life;
    ss->die += 8;
    ss->glitter = NULL;

    ss->num_points = 4;

    ss->point[0].type = p1->type;
    ss->point[0].flag = p1->flag;
    ss->point[0].dist = p1->dist;
    ss->point[0].Pos.x = p1->x;
    ss->point[0].Pos.y = p1->y;
    ss->point[0].Pos.z = p1->z;

    ss->point[1].type = p2->type;
    ss->point[1].flag = p2->flag;
    ss->point[1].dist = p2->dist;
    ss->point[1].Pos.x = p2->x;
    ss->point[1].Pos.y = p2->y;
    ss->point[1].Pos.z = p2->z;

    if (p1->type == SPARK_TYPE_LIMB) {
        ss->point[0].Peep.person = p1->person;
        ss->point[0].Peep.limb = p1->limb;
    }

    if (p2->type == SPARK_TYPE_LIMB) {
        ss->point[1].Peep.person = p2->person;
        ss->point[1].Peep.limb = p2->limb;
    }

    SPARK_point_set_velocity(&ss->point[0]);
    SPARK_point_set_velocity(&ss->point[1]);

    SLONG px1;
    SLONG py1;
    SLONG pz1;
    SLONG px2;
    SLONG py2;
    SLONG pz2;

    SPARK_point_pos(&ss->point[0], &px1, &py1, &pz1);
    SPARK_point_pos(&ss->point[1], &px2, &py2, &pz2);

    SLONG mx = px1 >> 8;
    SLONG mz = pz1 >> 8;

    if (WITHIN(mz, 0, MAP_HEIGHT - 1)) {
        ss->map_x = mx;
        ss->map_z = mz;
        ss->next = SPARK_mapwho[mz];
        SPARK_mapwho[mz] = SPARK_spark_last;
        ss->glitter = GLITTER_create(0, mx, mz, 0x005566ff);
    }

    SPARK_create_auxillary(ss, UC_TRUE, UC_TRUE);
}

// uc_orig: SPARK_process (fallen/Source/spark.cpp)
void SPARK_process()
{
    SLONG i;
    SLONG j;

    UBYTE point_2;
    UBYTE point_3;

    SPARK_Spark* ss;
    SPARK_Point* sp;

    for (i = 0; i < SPARK_MAX_SPARKS; i++) {
        ss = &SPARK_spark[i];

        if (!ss->used) {
            continue;
        }

        if (ss->die == 0) {
            ss->used = UC_FALSE;

            GLITTER_destroy(ss->glitter);

            UBYTE next;
            UBYTE* prev;

            prev = &SPARK_mapwho[ss->map_z];
            next = SPARK_mapwho[ss->map_z];

            while (1) {
                if (next == i) {
                    *prev = ss->next;
                    break;
                } else {
                    ASSERT(WITHIN(next, 1, SPARK_MAX_SPARKS - 1));

                    prev = &SPARK_spark[next].next;
                    next = SPARK_spark[next].next;
                }
            }
        } else {
            ss->die -= 1;

            for (j = 0; j < ss->num_points; j++) {
                sp = &ss->point[j];

                if (sp->type != SPARK_TYPE_LIMB) {
                    sp->Pos.x += sp->dx >> 4;
                    sp->Pos.y += sp->dy >> 4;
                    sp->Pos.z += sp->dz >> 4;

                    if (sp->type == SPARK_TYPE_CIRCULAR) {
                        sp->dx = 0;
                        sp->dy = 0;
                        sp->dz = 0;
                    }

                    if (sp->flag & SPARK_FLAG_DART_ABOUT) {
                        if ((rand() & 0xf) == (i & 0xf)) {
                            SPARK_point_set_velocity(sp);
                        }
                    }
                }
            }

            point_2 = ((rand() & 0xf) == (i & 0xf));
            point_3 = ((rand() & 0xf) == (i & 0xf));

            if (point_2 | point_3) {
                //				play_quick_wave_xyz(ss->map_x<<16,256<<8,ss->map_z<<16,S_ELEC_START+(i%5),point_3,WAVE_PLAY_INTERUPT);
                MFX_play_xyz(point_3, S_ELEC_START + (i % 5), ss->map_x << 16, 256 << 8, ss->map_z << 16, MFX_REPLACE);

                SPARK_create_auxillary(
                    ss,
                    point_2,
                    point_3);
            }

            {
                SLONG px;
                SLONG py;
                SLONG pz;

                j = rand() % (ss->num_points - 1);
                j += 1;

                SPARK_point_pos(
                    &ss->point[j],
                    &px,
                    &py,
                    &pz);
                GLITTER_add(
                    ss->glitter,
                    px,
                    py,
                    pz);
            }
        }
    }
}

// Calculates a perturbed midpoint between two world-space endpoints.
// Used to build the jagged lightning polyline.
// uc_orig: SPARK_find_midpoint (fallen/Source/spark.cpp)
static void SPARK_find_midpoint(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2,
    SLONG* mx, SLONG* my, SLONG* mz)
{
    SLONG dx = x2 - x1 >> 1;
    SLONG dy = y2 - y1 >> 1;
    SLONG dz = z2 - z1 >> 1;

    SLONG dist = abs(dx) + abs(dy) + abs(dz);

    if (dist < 4) {
        dist = 4;
    }

    *mx = (x1 + dx) + (rand() % (dist >> 1)) - (dist >> 2);
    *my = (y1 + dy) + (rand() % (dist >> 2));
    *mz = (z1 + dz) + (rand() % (dist >> 1)) - (dist >> 2);
}

// Fills si with a 5-point jagged lightning path between the two endpoints.
// uc_orig: SPARK_build_spark (fallen/Source/spark.cpp)
static void SPARK_build_spark(
    SPARK_Info* si,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    si->x[0] = x1;
    si->y[0] = y1;
    si->z[0] = z1;

    si->x[4] = x2;
    si->y[4] = y2;
    si->z[4] = z2;

    SPARK_find_midpoint(
        si->x[0], si->y[0], si->z[0],
        si->x[4], si->y[4], si->z[4],
        &si->x[2], &si->y[2], &si->z[2]);

    SPARK_find_midpoint(
        si->x[0], si->y[0], si->z[0],
        si->x[2], si->y[2], si->z[2],
        &si->x[1], &si->y[1], &si->z[1]);

    SPARK_find_midpoint(
        si->x[2], si->y[2], si->z[2],
        si->x[4], si->y[4], si->z[4],
        &si->x[3], &si->y[3], &si->z[3]);
}

// uc_orig: SPARK_get_start (fallen/Source/spark.cpp)
void SPARK_get_start(UBYTE xmin, UBYTE xmax, UBYTE z)
{
    SPARK_get_xmin = xmin;
    SPARK_get_xmax = xmax;
    SPARK_get_z = z;

    if (WITHIN(SPARK_get_z, 0, MAP_HEIGHT - 1)) {
        SPARK_get_spark = SPARK_mapwho[z];
        SPARK_get_point = 1;
    } else {
        SPARK_get_spark = NULL;
    }
}

// uc_orig: SPARK_get_next (fallen/Source/spark.cpp)
SPARK_Info* SPARK_get_next()
{
    SPARK_Spark* ss;

tail_recurse:;

    if (SPARK_get_spark == NULL) {
        return NULL;
    }

    ASSERT(WITHIN(SPARK_get_spark, 0, SPARK_MAX_SPARKS - 1));

    ss = &SPARK_spark[SPARK_get_spark];

    ASSERT(ss->used);

    if (!WITHIN(ss->map_x, SPARK_get_xmin, SPARK_get_xmax)) {
        SPARK_get_spark = ss->next;

        goto tail_recurse;
    }

    if (SPARK_get_point >= ss->num_points) {
        SPARK_get_spark = ss->next;
        SPARK_get_point = 1;

        goto tail_recurse;
    }

    SPARK_Point* sp1;
    SPARK_Point* sp2;
    SPARK_Point* sp;

    SLONG x1, y1, z1;
    SLONG x2, y2, z2;

    if (SPARK_get_point == 1) {
        sp1 = &ss->point[0];
        sp2 = &ss->point[1];

        SPARK_point_pos(sp1, &x1, &y1, &z1);
        SPARK_point_pos(sp2, &x2, &y2, &z2);

        SPARK_get_info.num_points = 5;
        SPARK_get_info.colour = 0x002233ff;
        SPARK_get_info.size = 40;

        SPARK_build_spark(
            &SPARK_get_info,
            x1, y1, z1,
            x2, y2, z2);

        SPARK_get_x1 = SPARK_get_info.x[1];
        SPARK_get_y1 = SPARK_get_info.y[1];
        SPARK_get_z1 = SPARK_get_info.z[1];

        SPARK_get_x2 = SPARK_get_info.x[3];
        SPARK_get_y2 = SPARK_get_info.y[3];
        SPARK_get_z2 = SPARK_get_info.z[3];
    } else {
        if ((rand() & 0xf) > ss->die) {
            SPARK_get_point += 1;

            goto tail_recurse;
        }

        sp = &ss->point[SPARK_get_point];

        SPARK_point_pos(sp, &x2, &y2, &z2);

        if (SPARK_get_point == 2) {
            x1 = SPARK_get_x1;
            y1 = SPARK_get_y1;
            z1 = SPARK_get_z1;
        } else {
            x1 = SPARK_get_x2;
            y1 = SPARK_get_y2;
            z1 = SPARK_get_z2;
        }

        SPARK_get_info.num_points = 5;
        SPARK_get_info.colour = 0x001825dd;
        SPARK_get_info.size = 20;

        SPARK_build_spark(
            &SPARK_get_info,
            x1, y1, z1,
            x2, y2, z2);
    }

    SPARK_get_point += 1;

    return &SPARK_get_info;
}

// uc_orig: SPARK_in_sphere (fallen/Source/spark.cpp)
void SPARK_in_sphere(
    SLONG mid_x,
    SLONG mid_y,
    SLONG mid_z,
    SLONG radius,
    UBYTE max_life,
    UBYTE max_create)
{
    SLONG x1;
    SLONG z1;
    SLONG x2;
    SLONG z2;

    SLONG mx;
    SLONG mz;

#define SPARK_MAX_CHOICES 16

    struct
    {
        SPARK_Pinfo p1;
        SPARK_Pinfo p2;

    } choice[SPARK_MAX_CHOICES];
    SLONG choice_upto = 0;

    x1 = mid_x - radius >> 8;
    z1 = mid_z - radius >> 8;
    x2 = mid_x + radius >> 8;
    z2 = mid_z + radius >> 8;

    SATURATE(x1, 0, MAP_WIDTH - 1);
    SATURATE(z1, 0, MAP_HEIGHT - 1);
    SATURATE(x2, 0, MAP_WIDTH - 1);
    SATURATE(z2, 0, MAP_HEIGHT - 1);

    for (mx = x1; mx <= x2; mx++)
        for (mz = z1; mz <= z2; mz++) {
            /*
            // (prim-based spark origins — commented out in original)
            */
        }

    SLONG choose;

    while (max_create && choice_upto) {
        choose = rand() % choice_upto;

        SPARK_create(
            &choice[choose].p1,
            &choice[choose].p2,
            max_life);

        choice[choose] = choice[choice_upto - 1];

        max_create -= 1;
        choice_upto -= 1;
    }
}

// uc_orig: SPARK_show_electric_fences (fallen/Source/spark.cpp)
void SPARK_show_electric_fences()
{
    SLONG i;

    SLONG dx;
    SLONG dz;

    SLONG bottom;

    SLONG along;

    DFacet* df;

    if (GAME_TURN & 0x1f) {
        return;
    }

    for (i = 1; i < next_dfacet; i++) {
        df = &dfacets[i];

        if (df->FacetFlags & FACET_FLAG_ELECTRIFIED) {
            dx = df->x[1] - df->x[0] << 8;
            dz = df->z[1] - df->z[0] << 8;

            along = Random() & 0xff;

            {
                SPARK_Pinfo p1;
                SPARK_Pinfo p2;

                extern SLONG get_fence_bottom(SLONG x, SLONG z, SLONG col);

                p1.type = SPARK_TYPE_POINT;
                p1.flag = 0;
                p1.x = (df->x[0] << 8) + (dx * along >> 8);
                p1.z = (df->z[0] << 8) + (dz * along >> 8);
                p1.y = (bottom = get_fence_bottom(p1.x, p1.z, i)) + (Random() & 0x7f) + 0x40;

                p2.type = SPARK_TYPE_GROUND;
                p2.flag = 0;
                p2.x = p1.x + (Random() & 0x7f) - 0x40;
                p2.z = p1.z + (Random() & 0x7f) - 0x40;
                p2.y = PAP_calc_map_height_at(p2.x, p2.z);

                if (dx == 0) {
                    p1.flag |= SPARK_FLAG_CLAMP_X;
                } else {
                    p1.flag |= SPARK_FLAG_CLAMP_Z;
                }

                if (abs(bottom - p2.y) < 0x40) {
                    SPARK_create(&p1, &p2, 50);
                }
            }
        }
    }
}
