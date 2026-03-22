// Pyrotechnics — fire, explosions, smoke and bullet-hit effects.
// Each pyro is a CLASS_PYRO Thing with a state machine (INIT -> NORMAL).
// The PYRO_functions table maps PyroType to per-type StateFunction arrays.
// All 18 pyro types share the same PYRO_fn_init/PYRO_fn_normal dispatch,
// except PYRO_EXPLODE which uses PYRO_fn_init_ex/PYRO_fn_normal_ex.

#include "game.h"
#include "effects/pyro.h"
#include "effects/pyro_globals.h"
#include "statedef.h"
#include "ribbon.h"
#include "night.h"
#include "dirt.h"
#include "sound.h"
#include "psystem.h"
#include "pcom.h"
#include "gamemenu.h"
#include "animate.h"
#include "combat.h"
#include "engine/graphics/pipeline/poly.h"

// uc_orig: init_pyros (fallen/Source/pyro.cpp)
void init_pyros(void)
{
    memset((UBYTE*)PYROS, 0, sizeof(Pyro) * MAX_PYROS);

    extern RadPoint PYRO_defaultpoints2[32];
    memset((UBYTE*)PYRO_defaultpoints2, 0, sizeof(RadPoint) * 32);

    global_spang_count = 0;
    PYRO_COUNT = 1;
}

// uc_orig: alloc_pyro (fallen/Source/pyro.cpp)
Thing* alloc_pyro(UBYTE type)
{

    SLONG i;

    Thing* p_thing;
    Pyro* p_pyro;

    THING_INDEX t_index;
    SLONG a_index;

    ASSERT(WITHIN(type, 1, PYRO_RANGE - 1));

    // Look for an unused pyro structure.
    for (i = 1; i < MAX_PYROS; i++) {
        if (PYROS[i].PyroType == PYRO_NONE) {
            a_index = i;

            goto found_pyro;
        }
    }

    // There are no spare pyro structures.
    return NULL;

found_pyro:
    t_index = alloc_primary_thing(CLASS_PYRO);

    if (t_index) {
        p_thing = TO_THING(t_index);
        p_pyro = TO_PYRO(a_index);

        if (a_index >= PYRO_COUNT)
            PYRO_COUNT = a_index + 1;

        // Link the pyro to the thing and back again.
        p_thing->Class = CLASS_PYRO;
        p_thing->Genus.Pyro = p_pyro;
        p_pyro->PyroType = type;
        p_pyro->thing = p_thing;

        // Set the draw type.
        p_thing->DrawType = DT_PYRO;

        return p_thing;
    } else {
        // Free up the pyro structure.
        TO_PYRO(a_index)->PyroType = PYRO_NONE;

        return NULL;
    }
}

// uc_orig: free_pyro (fallen/Source/pyro.cpp)
void free_pyro(Thing* p_thing)
{
    Pyro* pyro = PYRO_get_pyro(p_thing);

    remove_thing_from_map(p_thing);

    // Free up type-specific info.
    switch (pyro->PyroType) {
    case PYRO_IMMOLATE: {
        UBYTE i, r;
        if (pyro->Dummy == 2)
            r = 5;
        else
            r = 2;
        for (i = 0; i < r; i++)
            RIBBON_free(pyro->radii[i]);
        if (pyro->victim && (pyro->victim->Class == CLASS_PERSON))
            pyro->victim->Genus.Person->BurnIndex = 0;
    } break;
    case PYRO_EXPLODE2:
    case PYRO_BONFIRE:
    case PYRO_WHOOMPH:
        NIGHT_dlight_destroy(pyro->dlight);
        break;
    }

    pyro->soundid = 0;

    // Free the pyro structure and the thing structure.
    pyro->PyroType = PYRO_NONE;

    p_thing->DrawType = DT_NONE;
    free_thing(p_thing);
}

// uc_orig: PYRO_create (fallen/Source/pyro.cpp)
Thing* PYRO_create(GameCoord pos, UBYTE type)
{
    Thing* p_thing = alloc_pyro(type);

    if (p_thing != NULL) {
        p_thing->WorldPos = pos;

        add_thing_to_map(p_thing);

        // Make it initialise itself.
        set_state_function(p_thing, STATE_INIT);
    }

    return p_thing;
}

// uc_orig: PYRO_get_pyro (fallen/Source/pyro.cpp)
Pyro* PYRO_get_pyro(struct Thing* pyro_thing)
{
    Pyro* pyro;

    ASSERT(WITHIN(pyro_thing, TO_THING(1), TO_THING(MAX_THINGS)));
    ASSERT(pyro_thing->Class == CLASS_PYRO);

    pyro = pyro_thing->Genus.Pyro;

    ASSERT(WITHIN(pyro, TO_PYRO(1), TO_PYRO(MAX_PYROS - 1)));

    return pyro;
}

// Finds all CLASS_PERSON Things in radius and applies PYRO_IMMOLATE to each.
// uc_orig: PYRO_blast_radius (fallen/Source/pyro.cpp)
void PYRO_blast_radius(SLONG x, SLONG y, SLONG z, SLONG radius, SLONG strength)
{
// Local MAX_COL_WITH is separate from the file-scope col_with[] array.
#define MAX_COL_WITH 16

    SLONG collide_types, col_with_upto, i;
    THING_INDEX local_col_with[MAX_COL_WITH];
    Thing* col_thing;

    collide_types = (1 << CLASS_PERSON);

    col_with_upto = THING_find_sphere(
        x >> 8,
        y >> 8,
        z >> 8,
        radius,
        local_col_with,
        MAX_COL_WITH,
        collide_types);

    ASSERT(col_with_upto <= MAX_COL_WITH);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(local_col_with[i]);

        switch (col_thing->Class) {
        case CLASS_PERSON: {
            Thing* pyro = PYRO_create(col_thing->WorldPos, PYRO_IMMOLATE);
            if (pyro) {
                pyro->Genus.Pyro->victim = col_thing;
                pyro->Genus.Pyro->Flags = PYRO_FLAGS_FLICKER;
            }
        } break;
        }
    }

#undef MAX_COL_WITH
}

// Tries to share a sound effect with a nearby pyro of the same type.
// If no match is found, creates a new sound for EXPLODE2, BONFIRE, or FLICKER.
// uc_orig: MergeSoundFX (fallen/Source/pyro.cpp)
static SLONG MergeSoundFX(Thing* thing, Pyro* pyro)
{
    SLONG col_with_upto;
    SLONG collide_types = (1 << CLASS_PYRO);
    Thing* col_thing;
    SLONG i, sample, mode;

    col_with_upto = THING_find_sphere(
        thing->WorldPos.X >> 8,
        thing->WorldPos.Y >> 8,
        thing->WorldPos.Z >> 8,
        5 * 256,
        col_with,
        PYRO_MAX_COL_WITH,
        collide_types);

    for (i = 0; i < col_with_upto; i++) {
        col_thing = TO_THING(col_with[i]);

        if ((col_thing->State == STATE_DEAD) || (col_thing->State == STATE_DYING) || (col_thing->Genus.Pyro->PyroType != pyro->PyroType) || (!col_thing->Genus.Pyro->soundid))
            continue;

        if ((pyro->PyroType == PYRO_EXPLODE2) && (col_thing->Genus.Pyro->counter > 50))
            continue;

        pyro->soundid = col_thing->Genus.Pyro->soundid;
        return pyro->soundid;
    }

    switch (pyro->PyroType) {
    case PYRO_EXPLODE2:
        sample = S_EXPLODE_START + (rand() % 3);
        mode = 0;
        break;
    case PYRO_BONFIRE:
    case PYRO_FLICKER:
        sample = S_FIRE;
        mode = MFX_LOOPED;
        break;
    default:
        return 0;
    }
    MFX_play_thing(THING_NUMBER(thing), sample, MFX_OVERLAP | mode, thing);
    pyro->soundid = THING_NUMBER(thing);

    return pyro->soundid;
}

// Normalises a 3D vector to magnitude 256 (fixed-point unit vector).
// File-private; Vehicle.cpp has its own independent copy.
// uc_orig: normalise_val256 (fallen/Source/pyro.cpp)
static void normalise_val256(SLONG* vx, SLONG* vy, SLONG* vz)
{
    SLONG len;

    if ((abs(*vx) > 32768) && (abs(*vy) > 32768) && (abs(*vz) > 32768)) {
        (*vx) >>= 8;
        (*vy) >>= 8;
        (*vz) >>= 8;
    }

    len = ((*vx) * (*vx)) + ((*vy) * (*vy)) + ((*vz) * (*vz));

    len = Root(len);

    if (len == 0)
        len = 1;

    *vx = (*vx << 8) / len;
    *vy = (*vy << 8) / len;
    *vz = (*vz << 8) / len;
}

// Initialises a pyro: resets counters and sets up type-specific resources
// (ribbons, dynamic lights, streamer data, etc.), then transitions to STATE_NORMAL.
// uc_orig: PYRO_fn_init (fallen/Source/pyro.cpp)
void PYRO_fn_init(Thing* thing)
{
    Pyro* pyro = PYRO_get_pyro(thing);
    UBYTE i;

    pyro->counter = 0;
    pyro->radius = 0;
    pyro->soundid = 0;
    for (i = 0; i < 8; i++)
        pyro->radii[i] = 0;

    switch (pyro->PyroType) {
    case PYRO_BONFIRE:
        pyro->dlight = NIGHT_dlight_create(
            thing->WorldPos.X >> 8, thing->WorldPos.Y >> 8, thing->WorldPos.Z >> 8,
            100, 35, 30, 0);
        pyro->Dummy = 0;
        pyro->soundid = MergeSoundFX(thing, pyro);
        break;
    case PYRO_WHOOMPH:
        pyro->dlight = NIGHT_dlight_create(
            thing->WorldPos.X >> 8, thing->WorldPos.Y >> 8, thing->WorldPos.Z >> 8,
            100, 1, 0, 0);
        break;
    case PYRO_IMMOLATE:
        pyro->radii[0] = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 8 + (rand() & 7), POLY_PAGE_FLAMES3, -1, 8, 5 + (rand() & 7));
        if (!pyro->radii[0]) {
            free_pyro(thing);
            return;
        }
        pyro->radii[1] = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 8 + (rand() & 7), POLY_PAGE_FLAMES3, -1, 8, 5 + (rand() & 7));
        if (!pyro->radii[1]) {
            RIBBON_free(pyro->radii[0]);
            free_pyro(thing);
            return;
        }
        pyro->tints[0] = pyro->tints[1] = 0;
        pyro->Dummy = 0;
        break;
    case PYRO_STREAMER:
        for (i = 0; i < 4; i++) {
            // Set streamer start time offsets.
            pyro->radii[i] = rand() & 0x3f;
            // Prepare streamer attitudes.
            pyro->radii[i + 4] = rand();
        }
        pyro->counter = 4; // Each streamer decrements this when done.
        break;
    case PYRO_TWANGER:
        for (i = 0; i < 8; i++) {
            // Set twanger attitudes.
            pyro->radii[i] = rand();
        }
        pyro->tints[0] = pyro->tints[1] = 0;
        break;
    case PYRO_FLICKER:
        pyro->Dummy = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 8 + (rand() & 0xf), POLY_PAGE_FLAMES3, -1, 8, 5 + (rand() & 7));
        pyro->scale = 140 + (rand() & 0x7f);
        pyro->radii[0] = 256;
        pyro->radii[1] = 0;
        pyro->tints[0] = pyro->tints[1] = 0;
        if (!pyro->Dummy) {
            free_pyro(thing);
            return;
        }
        pyro->soundid = MergeSoundFX(thing, pyro);
        break;
    case PYRO_EXPLODE2:
        pyro->soundid = MergeSoundFX(thing, pyro);
        MFX_play_ambient(0, S_EXPLODE_AMBIENT, 0);
        pyro->dlight = NIGHT_dlight_create(thing->WorldPos.X >> 8, thing->WorldPos.Y >> 8, thing->WorldPos.Z >> 8, pyro->scale >> 1, 0, 0, 0);
        break;
    case PYRO_HITSPANG:
        GameCoord vec, old;

        if (pyro->Dummy) {
            // We're shooting a thing, so set up a hit location.
            // Hopefully the creator has set a 'victim' by now.

            if (pyro->victim->Class == CLASS_PERSON) {
                calc_sub_objects_position(
                    pyro->victim,
                    pyro->victim->Draw.Tweened->AnimTween,
                    SUB_OBJECT_HEAD,
                    &vec.X,
                    &vec.Y,
                    &vec.Z);

                vec.X <<= 8;
                vec.Y <<= 8;
                vec.Z <<= 8;
                vec.X += pyro->victim->WorldPos.X;
                vec.Y += pyro->victim->WorldPos.Y;
                vec.Z += pyro->victim->WorldPos.Z;
                if (VIOLENCE)
                    for (i = 0; i < 2; i++)
                        DIRT_new_water(
                            vec.X >> 8,
                            vec.Y >> 8,
                            vec.Z >> 8,
                            (Random() & 0xf) - 7,
                            0,
                            (Random() & 0xf) - 7,
                            DIRT_TYPE_BLOOD);
            } else {
                // Shooting non-people.
                vec = pyro->victim->WorldPos;
                DIRT_new_sparks(vec.X >> 8, vec.Y >> 8, vec.Z >> 8, 2);
            }

        } else {
            // We're shooting a random location, target should be preset.
            vec = pyro->target;
        }

        // The WorldPos should be the source of the shot.
        pyro->target.X = pyro->thing->WorldPos.X - vec.X;
        pyro->target.Y = pyro->thing->WorldPos.Y - vec.Y;
        pyro->target.Z = pyro->thing->WorldPos.Z - vec.Z;

        old = pyro->target;
        old.X >>= 8;
        old.Y >>= 8;
        old.Z >>= 8;

        normalise_val256(&pyro->target.X, &pyro->target.Y, &pyro->target.Z);

        if (abs(old.X) < abs(pyro->target.X << 1)) {
            pyro->target = old;
            pyro->target.X >>= 1;
            pyro->target.Y >>= 1;
            pyro->target.Z >>= 1;
        }

        move_thing_on_map(pyro->thing, &vec);

        break;
    }

    // Start being a normal pyro.
    set_state_function(thing, STATE_NORMAL);
}

// Initialises PYRO_EXPLODE: builds old hemisphere geometry from PYRO_defaultpoints,
// then transitions to STATE_NORMAL.
// uc_orig: PYRO_fn_init_ex (fallen/Source/pyro.cpp)
void PYRO_fn_init_ex(Thing* thing)
{
    Pyrex* pyro = (Pyrex*)PYRO_get_pyro(thing);
    UBYTE i, j;
    SLONG height, radius;
    RadPoint* pt;

    // On first explosion, precalculate the hemisphere vertex positions.
    if (!PYRO_defaultpoints[0].flags) {
        PYRO_defaultpoints[0].flags = 1;

        pt = PYRO_defaultpoints;
        for (i = 0; i < 2; i++) {

            height = SIN(i * 350);
            radius = COS(i * 350) >> 8;

            for (j = 0; j < 8; j++) {
                pt->x = (radius * ((SLONG)SIN(j * 256))) / 256;
                pt->z = (radius * ((SLONG)COS(j * 256))) / 256;
                pt->y = height;
                pt++;
            }
        }
    }

    // Set up per-instance explosion data.
    for (i = 0; i < 8; i++) {
        pyro->points[i].delta = 40 + (rand() & 0x1f);
        pyro->points[i].radius = 10;
    }
    height = 0;
    for (i = 8; i < 16; i++) {
        pyro->points[i].delta = 30 + (rand() & 0x1f);
        pyro->points[i].radius = 10;
        height += pyro->points[i].delta;
    }
    pyro->points[16].delta = height * 0.125f;
    pyro->points[16].radius = 100;

    thing->Genus.Pyro->Timer1 = 0;

    set_state_function(thing, STATE_NORMAL);
}

// Per-frame update for all pyro types except PYRO_EXPLODE.
// Each type has its own timing, particle emission, and lifetime logic.
// uc_orig: PYRO_fn_normal (fallen/Source/pyro.cpp)
void PYRO_fn_normal(Thing* thing)
{
    GameCoord new_pos;
    SLONG mag, rpos, altitude;
    UBYTE i;
    CBYTE msg[300];

    Pyro* pyro = PYRO_get_pyro(thing);

    ASSERT(pyro->thing == thing);

    switch (pyro->PyroType) {
    case PYRO_FIREWALL:
        if (pyro->counter < 254) {
            GameCoord posn;
            Thing* new_pyro;
            SLONG angle;

            pyro->counter += 16;
            posn = pyro->target;
            posn.X -= thing->WorldPos.X;
            posn.Y -= thing->WorldPos.Y;
            posn.Z -= thing->WorldPos.Z;
            posn.X /= 256;
            posn.Y /= 256;
            posn.Z /= 256;
            new_pos.X = posn.X * pyro->counter;
            new_pos.Y = posn.Y * pyro->counter;
            new_pos.Z = posn.Z * pyro->counter;
            new_pos.X += thing->WorldPos.X;
            new_pos.Y += thing->WorldPos.Y;
            new_pos.Z += thing->WorldPos.Z;
            new_pyro = PYRO_create(new_pos, PYRO_FLICKER);
            if (new_pyro) {
                PYRO_fn_init(new_pyro);
                new_pyro->Genus.Pyro->radii[0] = 255;
                new_pyro->Genus.Pyro->radii[1] = 512;
                angle = -Arctan(posn.X, posn.Z);
                new_pyro->Genus.Pyro->radii[2] = angle & 2047;
            }
        }
        break;
    case PYRO_FIREPOOL:
        if (pyro->counter < 254)
            pyro->counter += 2;
        break;
    case PYRO_BONFIRE:
        // Update dynamic light with flickering colour.
        if ((((THING_NUMBER(thing)) + GAME_TURN) & 7) == 0)
            MFX_play_thing(99, S_FIRE, MFX_LOOPED | MFX_QUEUED, thing);

        if (pyro->dlight) {
            SWORD max = pyro->Dummy;
            if (max > 0x7f)
                max += (Random() & 0x3f) - 0x1f;
            else
                max += Random() & 0x3f;
            SATURATE(max, 0, 0xff);
            pyro->Dummy = max;
            NIGHT_dlight_colour(pyro->dlight, max, max - (Random() & 0xf), 0);
        }

        break;
    case PYRO_IMMOLATE:
        if (pyro->counter < 255)
            pyro->counter++;
        if (pyro->victim) {

            new_pos = pyro->victim->WorldPos;
            move_thing_on_map(thing, &new_pos);

            if (pyro->victim->Class == CLASS_BAT) {
                UBYTE p, q, r;
                SLONG px, py, pz;
                r = pyro->radius >> 6;
                r = (r < 5) ? 5 - r : 1;
                for (p = 0; p < r; p++) {
                    q = Random() % 15;
                    calc_sub_objects_position(
                        pyro->victim,
                        pyro->victim->Draw.Tweened->AnimTween,
                        q,
                        &px,
                        &py,
                        &pz);
                    px <<= 8;
                    py <<= 8;
                    pz <<= 8;
                    px += pyro->victim->WorldPos.X;
                    py += pyro->victim->WorldPos.Y;
                    pz += pyro->victim->WorldPos.Z;
                    PARTICLE_Add(px, py, pz,
                        256 + (Random() & 0xff), 256 + (Random() & 0xff), 256 + (Random() & 0xff),
                        POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                        300, 90, 1, 4, -1);
                }
            }

            if (pyro->victim->Class == CLASS_PERSON) {
                UBYTE p, q, r;
                SLONG px, py, pz;
                r = pyro->radius >> 6;
                r = (r < 5) ? 5 - r : 1;
                for (p = 0; p < r; p++) {
                    q = Random() % 15;
                    calc_sub_objects_position(
                        pyro->victim,
                        pyro->victim->Draw.Tweened->AnimTween,
                        q,
                        &px,
                        &py,
                        &pz);
                    px <<= 8;
                    py <<= 8;
                    pz <<= 8;
                    px += pyro->victim->WorldPos.X;
                    py += pyro->victim->WorldPos.Y;
                    pz += pyro->victim->WorldPos.Z;
                    PARTICLE_Add(px, py, pz,
                        256 + (Random() & 0xff), 256 + (Random() & 0xff), 256 + (Random() & 0xff),
                        POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                        300, 40, 1, 3, -1);
                }

                if ((pyro->victim->State != STATE_DEAD) && (pyro->victim->State != STATE_DYING) && (pyro->radius < 290)) {
                    if ((pyro->victim->Genus.Person->Flags2 & FLAG2_PERSON_INVULNERABLE) || (pyro->victim->Genus.Person->pcom_bent & PCOM_BENT_PLAYERKILL)) {
                        // Nothing hurts this person.
                    } else {
                        pyro->victim->Genus.Person->Health -= 10;

                        if (pyro->victim->Genus.Person->Health <= 0) {
                            pyro->victim->Genus.Person->Health = 0;

                            set_person_dead(
                                pyro->victim,
                                NULL,
                                PERSON_DEATH_TYPE_OTHER,
                                UC_FALSE,
                                0);
                        }
                    }
                } else {
                    if (!pyro->Dummy) {
                        RIBBON_life(pyro->radii[0], 30);
                        RIBBON_life(pyro->radii[1], 50);
                        pyro->Dummy = 2;
                        pyro->radius = 0;
                    } else {
                        pyro->radius++;
                        switch (pyro->radius) {
                        case 30:
                            pyro->radii[2] = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 7, POLY_PAGE_FLAMES3, 100, 7, 5 + (rand() & 7));
                            break;
                        case 100:
                            pyro->radii[3] = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 5, POLY_PAGE_FLAMES3, 90, 5, 5 + (rand() & 7));
                            break;
                        case 150:
                            pyro->radii[4] = RIBBON_alloc(RIBBON_FLAG_CONVECT | RIBBON_FLAG_FADE | RIBBON_FLAG_SLIDE, 3, POLY_PAGE_FLAMES3, 90, 3, 5 + (rand() & 7));
                            break;
                        }
                        if ((pyro->radius > 350) && (!pyro->victim->Genus.Person->BurnIndex))
                            free_pyro(thing);
                        else if (!(pyro->victim->Flags & FLAGS_ON_MAPWHO))
                            free_pyro(thing);
                    }
                }
            }
        }
        break;
    case PYRO_DUSTWAVE:
        if (pyro->counter < 120)
            pyro->counter += 8;
        else
            free_pyro(thing);
        break;
    case PYRO_EXPLODE2:
        if (pyro->counter < 249) {

            pyro->counter += 7;
            pyro->radius += 9;

            if (pyro->dlight) {
                SLONG rgb;

                rgb = pyro->counter << 2;
                if (rgb > 512)
                    rgb = 512;
                rgb = COS(rgb) >> 10;

                NIGHT_dlight_colour(pyro->dlight, rgb, rgb, rgb >> 1);
            }

            for (i = 0; i < 4; i++)
                pyro->radii[i]++;

        } else
            free_pyro(thing);
        break;
    case PYRO_STREAMER:
        for (i = 0; i < 4; i++) {
            pyro->radii[i] += 8;
        }
        if (pyro->counter == 0)
            free_pyro(thing);
        break;
    case PYRO_TWANGER:
        if (pyro->counter < 240)
            pyro->counter += 16;
        else
            free_pyro(thing);
        break;

    case PYRO_NEWDOME:
        if (pyro->counter < 249) {
            if ((!pyro->counter) && !(Random() & 3)) {
                if (Random() & 1)
                    MFX_play_thing(THING_NUMBER(pyro->thing), S_BIG_BANG_START, MFX_OVERLAP, pyro->thing);
                else
                    MFX_play_thing(THING_NUMBER(pyro->thing), S_FIREBALL, MFX_OVERLAP, pyro->thing);
            }
            pyro->radius = SIN(pyro->counter << 1) >> 8;
            pyro->radius = (pyro->radius * pyro->scale) >> 9;
            pyro->counter += 7;
            for (i = 0; i < 4; i++)
                pyro->radii[i]++;
        } else
            free_pyro(thing);
        break;

    case PYRO_IRONICWATERFALL:
        if (pyro->thing->Flags & FLAGS_IN_VIEW) {
            SLONG px, py, pz;
            px = pyro->thing->WorldPos.X >> 8;
            py = pyro->thing->WorldPos.Y >> 8;
            pz = pyro->thing->WorldPos.Z >> 8;
            switch (pyro->radius) {
            case 0: // water fountain
                DIRT_new_water(px + 2, py, pz, -1, 28, 0);
                DIRT_new_water(px, py, pz + 2, 0, 29, -1);
                DIRT_new_water(px, py, pz - 2, 0, 28, +1);
                DIRT_new_water(px - 2, py, pz, +1, 29, 0);
                break;
            case 1:
                if (!(rand() & 0xff))
                    DIRT_new_water(px, py, pz, 0, 0, 0);
                break;
            case 2: // smokestack
                if (!(Random() & 3))
                    PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                        256 + (Random() & 0xff), 256 + (Random() & 0xff), 256 + (Random() & 0xff),
                        POLY_PAGE_SMOKECLOUD, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE,
                        300, 60, 1, 1, 1);
                break;
            case 3: // chimney
                if (!(Random() & 3))
                    PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                        256 + (Random() & 0xff), 256 + (Random() & 0xff), 256 + (Random() & 0xff),
                        POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE,
                        300, 60, 1, 1, 1);
                break;
            }
        }
        break;

    case PYRO_FLICKER: {
        GameCoord pos;
        SLONG dx, dz, nx, nz, cr, cosa, sina;

        pyro->counter++;
        if (pyro->Dummy & 1)
            pyro->radius += pyro->scale;
        else
            pyro->radius -= pyro->scale;

        pyro->scale += pyro->tints[0];
        if (pyro->scale < 100)
            pyro->tints[0] += 4;
        if (pyro->scale > 300)
            pyro->tints[0] -= 4;
        if (pyro->tints[0] > 40)
            pyro->tints[0] = 40;
        if (pyro->tints[0] < -40)
            pyro->tints[0] = -40;

        dx = SIN(pyro->radius & 2047) / 8;
        dz = COS(pyro->radius & 2047) / 8;

        // Pinch+twist deformation for non-straight flames.
        if (pyro->radii[0] < 256) {
            dx *= pyro->radii[0];
            dx /= 256;
            dz *= pyro->radii[1];
            dz /= 256;

            cosa = COS(pyro->radii[2] & 2047) / 256;
            sina = SIN(pyro->radii[2] & 2047) / 256;

            nx = ((dx * cosa) / 256) + ((dz * sina) / 256);
            nz = -((dx * sina) / 256) + ((dz * cosa) / 256);
            dx = nx;
            dz = nz;
        }

        pos = pyro->thing->WorldPos;
        pos.X += dx;
        pos.Z += dz;
        RIBBON_extend(pyro->Dummy, pos.X >> 8, pos.Y / 256, pos.Z >> 8);
        pos = pyro->thing->WorldPos;
        pos.X -= dx;
        pos.Z -= dz;
        RIBBON_extend(pyro->Dummy, pos.X >> 8, pos.Y / 256, pos.Z >> 8);
    } break;

    case PYRO_HITSPANG:

        if (pyro->counter < 5) {
            pyro->counter++;
        } else {
            global_spang_count--;
            free_pyro(thing);
        }
        break;

    case PYRO_GAMEOVER:
        if (GAMEMENU_is_paused())
            break;
        if (!pyro->counter) {
            SLONG x, y, z;
            for (i = 0; i < 8; i++) {
                x = SIN(i << 8);
                z = COS(i << 8);
                x <<= 2;
                z <<= 2;
                x += pyro->thing->WorldPos.X;
                y = pyro->thing->WorldPos.Y;
                z += pyro->thing->WorldPos.Z;
                pyro->radii[i] = NIGHT_dlight_create(x >> 8, (y >> 8) + 10, z >> 8, 200, 1, 1, 1);
            }
            pyro->Dummy = 0;
        } else {
            if ((pyro->counter > 100) && !pyro->Dummy) {
                pyro->Dummy = 1;
                MFX_play_thing(THING_NUMBER(pyro->thing), S_BIG_BANG_END, MFX_OVERLAP, pyro->thing);
            }
            for (i = 0; i < 8; i++) {
                UBYTE col = pyro->counter >> 3;
                UBYTE col2 = Random() % (1 + (col >> 1));
                NIGHT_dlight_colour(pyro->radii[i], col, col2, 0);
            }
        }
        if (pyro->counter > 242) {
            GAME_STATE = GS_LEVEL_LOST;

            if (NET_PERSON(0)->State != STATE_DYING && NET_PERSON(0)->State != STATE_DEAD) {
                knock_person_down(
                    NET_PERSON(0),
                    500,
                    NET_PERSON(0)->WorldPos.X + 0x1000 >> 8,
                    NET_PERSON(0)->WorldPos.Z + 0x1000 >> 8,
                    NULL);
            }
        }

        if (pyro->counter < 253)
            pyro->counter += 3;
        else {
            for (i = 0; i < 8; i++) {
                NIGHT_dlight_destroy(pyro->radii[i]);
            }
            free_pyro(thing);
        }
        break;

    case PYRO_FIREBOMB:
        if (pyro->radii[0] < 0xfe00) {
            if (pyro->radii[0] > 2560)
                pyro->radii[0] += 9 * TICK_RATIO;
            pyro->radii[0] += TICK_RATIO;
            if (pyro->radii[0] > 0xff00)
                pyro->radii[0] = 0xff00;
            pyro->counter = pyro->radii[0] >> 8;
        } else
            free_pyro(thing);
        break;

    case PYRO_SPLATTERY:
        if (pyro->counter < 254) {
            for (SLONG i = 0; i < 4; i++) {
                UBYTE sz;
                SLONG dx, dr, dz, x, y, z, b, ox, oz;

                sz = (255 - pyro->counter) >> 3;
                dr = rand() & 2047;
                ox = SIN(dr) >> 4;
                oz = COS(dr) >> 4;
                PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                    ox, ((Random() & 0x1f) + 0x1f) << 5, oz,
                    POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFF0000,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE | PFLAG_GRAVITY | PFLAG_RESIZE, 130, 30, 1, 10, -2);

                dr = rand() & 2047;
                dx = ((SIN(dr) >> 8) * sz) >> 8;
                dz = ((COS(dr) >> 8) * sz) >> 8;
                if ((dx == 0) && (dz == 0))
                    dz = 1;
                x = pyro->thing->WorldPos.X;
                z = pyro->thing->WorldPos.Z;
                x -= (dx / 2);
                z -= (dz / 2);
                y = (PAP_calc_map_height_at(x >> 8, z >> 8) << 8) + 257;

                for (b = 0; b < 5; b++) {
                    dr = ((SLONG)pyro) + b;
                    dr *= 31415965;
                    dr += 123456789;
                    dr >>= 8;
                    dr &= 0xff;
                    ox = ((SIN((b * 340) + dr) >> 8) * pyro->counter);
                    oz = ((COS((b * 340) + dr) >> 8) * pyro->counter);
                    TRACKS_AddQuad(x + ox, y, z + oz, dx, 0, dz, POLY_PAGE_BLOODSPLAT, 0x00ffffff, sz, 0, 0);
                }
                pyro->counter += 18;
                if (pyro->counter > 254)
                    break;
            }

        } else
            free_pyro(thing);
        break;

    case PYRO_WHOOMPH:
        if (pyro->counter < 255 - 16) {
            if (pyro->dlight) {
                SWORD max = SIN(((UWORD)pyro->counter) << 1) >> 11;
                NIGHT_dlight_colour(pyro->dlight, max, max >> 1, max >> 3);
            }
            pyro->counter += 16;
        } else
            free_pyro(thing);
        break;
    }
}

// Per-frame update for PYRO_EXPLODE (old hemisphere explosion).
// Expands each point by its delta until they collapse back, then frees.
// uc_orig: PYRO_fn_normal_ex (fallen/Source/pyro.cpp)
void PYRO_fn_normal_ex(Thing* thing)
{
    Pyrex* pyro = (Pyrex*)PYRO_get_pyro(thing);
    UBYTE i, dead = 0;

    for (i = 0; i < 17; i++) {
        pyro->points[i].radius += pyro->points[i].delta;
        if (pyro->points[i].delta > 20)
            pyro->points[i].delta--;
        if (pyro->points[i].delta > -20)
            pyro->points[i].delta--;
        pyro->points[i].delta--;
        if (pyro->points[i].radius < 0)
            dead = 1;
    }
    thing->Genus.Pyro->Timer1 += 7;
    if (dead || (thing->Genus.Pyro->Timer1 >= 255))
        free_pyro(thing);
}

// Creates one or more pyro effects at posn based on a types bitmask.
// uc_orig: PYRO_construct (fallen/Source/pyro.cpp)
void PYRO_construct(GameCoord posn, SLONG types, SLONG scale)
{
    Thing* thing;
    if (types & 1) {
        thing = PYRO_create(posn, PYRO_TWANGER);
        if (thing)
            thing->Genus.Pyro->scale = scale;
    }
    if (types & 2) {
        thing = PYRO_create(posn, PYRO_EXPLODE2);
        if (thing) {
            SLONG cx, cz;
            thing->Genus.Pyro->scale = scale;

            // Do initial blast gust.
            cx = thing->WorldPos.X >> 8;
            cz = thing->WorldPos.Z >> 8;
            DIRT_gust(thing, cx, cz, cx + scale, cz);
            DIRT_gust(thing, cx + scale, cz, cx, cz);
        }
    }
    if (types & 4) {
        thing = PYRO_create(posn, PYRO_DUSTWAVE);
        if (thing)
            thing->Genus.Pyro->scale = scale;
    }
    if (types & 8) {
        thing = PYRO_create(posn, PYRO_STREAMER);
        if (thing)
            thing->Genus.Pyro->scale = scale;
    }
    if (types & 16) {
        thing = PYRO_create(posn, PYRO_BONFIRE);
    }
    if (types & 32) {
        thing = PYRO_create(posn, PYRO_NEWDOME);
        if (thing) {
            SLONG cx, cz;
            thing->Genus.Pyro->scale = scale;

            cx = thing->WorldPos.X >> 8;
            cz = thing->WorldPos.Z >> 8;
            DIRT_gust(thing, cx, cz, cx + scale, cz);
            DIRT_gust(thing, cx + scale, cz, cx, cz);
        }
    }
    if (types & 64) {
        thing = PYRO_create(posn, PYRO_FIREBOMB);
    }
    if (types & 128) {
        thing = PYRO_create(posn, PYRO_FIREBOMB);
        if (thing)
            thing->Genus.Pyro->Flags |= PYRO_FLAGS_WAVE;
    }
}

// Creates a PYRO_HITSPANG from p_person's left hand toward a victim Thing.
// uc_orig: PYRO_hitspang (fallen/Source/pyro.cpp)
void PYRO_hitspang(Thing* p_person, Thing* victim)
{
    GameCoord vec;
    Thing* thing;

    ASSERT(global_spang_count >= 0);
    if (global_spang_count > 5)
        return;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_HAND,
        &vec.X,
        &vec.Y,
        &vec.Z);
    vec.X <<= 8;
    vec.Y <<= 8;
    vec.Z <<= 8;
    vec.X += p_person->WorldPos.X;
    vec.Y += p_person->WorldPos.Y;
    vec.Z += p_person->WorldPos.Z;
    thing = PYRO_create(vec, PYRO_HITSPANG);
    if (!thing)
        return;
    thing->Genus.Pyro->victim = victim;
    thing->Genus.Pyro->Dummy = 1; // 1 = targeting a Thing (not a coordinate)
    PYRO_fn_init(thing);
    global_spang_count++;
}

// Creates a PYRO_HITSPANG from p_person's left hand toward a world coordinate.
// uc_orig: PYRO_hitspang (fallen/Source/pyro.cpp)
void PYRO_hitspang(Thing* p_person, SLONG x, SLONG y, SLONG z)
{
    GameCoord vec;
    Thing* thing;

    ASSERT(global_spang_count >= 0);
    if (global_spang_count > 5)
        return;

    calc_sub_objects_position(
        p_person,
        p_person->Draw.Tweened->AnimTween,
        SUB_OBJECT_LEFT_HAND,
        &vec.X,
        &vec.Y,
        &vec.Z);
    vec.X <<= 8;
    vec.Y <<= 8;
    vec.Z <<= 8;
    vec.X += p_person->WorldPos.X;
    vec.Y += p_person->WorldPos.Y;
    vec.Z += p_person->WorldPos.Z;
    thing = PYRO_create(vec, PYRO_HITSPANG);
    if (!thing)
        return;
    thing->Genus.Pyro->target.X = x;
    thing->Genus.Pyro->target.Y = y;
    thing->Genus.Pyro->target.Z = z;
    thing->Genus.Pyro->Dummy = 0; // 0 = targeting a world coordinate
    DIRT_new_sparks(x >> 8, y >> 8, z >> 8, 2);
    PYRO_fn_init(thing);
    global_spang_count++;
}
