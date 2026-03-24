// Pyrotechnics — fire, explosions, smoke and bullet-hit effects.
// Each pyro is a CLASS_PYRO Thing with a state machine (INIT -> NORMAL).
// The PYRO_functions table maps PyroType to per-type StateFunction arrays.
// All 18 pyro types share the same PYRO_fn_init/PYRO_fn_normal dispatch,
// except PYRO_EXPLODE which uses PYRO_fn_init_ex/PYRO_fn_normal_ex.

#include "game/game_types.h"
#include "effects/combat/pyro.h"
#include "effects/combat/pyro_globals.h"
#include "things/core/statedef.h"
#include "effects/environment/ribbon.h"
#include "engine/graphics/lighting/night.h"
#include "world_objects/dirt.h"
#include "engine/audio/sound.h"
#include "engine/effects/psystem.h"
#include "ai/pcom.h"
#include "ui/menus/gamemenu.h"
#include "things/characters/anim_ids.h"
#include "combat/combat.h"
#include "things/core/interact.h"      // calc_sub_objects_position
#include "things/characters/person.h"  // set_person_dead, knock_person_down
#include "engine/graphics/pipeline/poly.h"

// uc_orig: init_pyros (fallen/Source/pyro.cpp)
void init_pyros(void)
{
    memset((UBYTE*)PYROS, 0, sizeof(Pyro) * MAX_PYROS);

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
    UBYTE i;

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
        SLONG dx, dz, nx, nz, cosa, sina;

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

// ---- Drawing functions migrated from drawxtra.cpp ----

// 256-entry RGB palette loaded from data\flames1.pal, used for flame colour lookup.
extern UBYTE fire_pal[768];
void draw_flames(SLONG x, SLONG y, SLONG z, SLONG lod, SLONG offset);
void draw_flame_element(SLONG x, SLONG y, SLONG z, SLONG c0, UBYTE base, UBYTE rand = 1);
#include "engine/graphics/pipeline/aeng.h"
#include "map/level_pools.h"
#include "things/items/barrel.h"
#include "effects/environment/ribbon.h"
#include "things/characters/anim_ids.h"
#include "engine/graphics/geometry/sprite.h"
#include "effects/combat/spark.h"
#include "buildings/id.h"
#include "map/pap.h"
// BLOOM_flare_draw migrated to bloom.h (iteration 136); RIBBON_draw_ribbon already in ribbon.h.
#include "engine/graphics/postprocess/bloom.h"

// Forward declarations for draw functions defined later in this file.
// uc_orig: PYRO_draw_explosion (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_explosion(Pyrex* pyro);
// uc_orig: PYRO_draw_explosion2 (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_explosion2(Pyro* pyro);
// uc_orig: PYRO_draw_newdome (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_newdome(Pyro* pyro);
// uc_orig: PYRO_draw_dustwave (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_dustwave(Pyro* pyro);
// uc_orig: PYRO_draw_streamer (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_streamer(Pyro* pyro);
// uc_orig: PYRO_draw_twanger (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_twanger(Pyro* pyro);
// uc_orig: PYRO_draw_armageddon (fallen/DDEngine/Source/drawxtra.cpp)
static void PYRO_draw_armageddon(Pyro* pyro);

// uc_orig: IWouldLikeSomePyroSpritesHowManyCanIHave (fallen/DDEngine/Source/drawxtra.cpp)
// PC build has no throttle — always grants the requested sprite count.
int IWouldLikeSomePyroSpritesHowManyCanIHave(int iIWantThisMany)
{
    return (iIWantThisMany);
}

// uc_orig: IHaveToHaveSomePyroSprites (fallen/DDEngine/Source/drawxtra.cpp)
inline void IHaveToHaveSomePyroSprites(int iINeedThisMany)
{
    // Stub — PC build no-op.
}

// uc_orig: Pyros_EndOfFrameMarker (fallen/DDEngine/Source/drawxtra.cpp)
void Pyros_EndOfFrameMarker(void)
{
    // Stub — PC build no-op.
}

// uc_orig: get_pyro_rand (fallen/DDEngine/Source/drawxtra.cpp)
// LCG pseudo-random generator seeded by pyro_seed. Used for deterministic per-frame fire placement.
static SLONG get_pyro_rand(void)
{
    pyro_seed *= 31415965;
    pyro_seed += 123456789;
    return (pyro_seed >> 8);
}

// uc_orig: SPRITE_draw_tex2 (fallen/DDEngine/Source/drawxtra.cpp)
// Like SPRITE_draw_tex but draws a non-square sprite: width is 0.5x height.
// Used for flame elements that need an asymmetric billboard shape.
static void SPRITE_draw_tex2(
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
        &mid);

    if (mid.IsValid()) {
        screen_size = POLY_world_length_to_screen(world_size) * mid.Z;

        if (mid.X + screen_size < 0 || mid.X - screen_size > POLY_screen_width || mid.Y + screen_size < 0 || mid.Y - screen_size > POLY_screen_height) {
            // Off screen.
        } else {
            pp[0].X = mid.X - (screen_size * 0.5f);
            pp[0].Y = mid.Y - screen_size;
            pp[1].X = mid.X + (screen_size * 0.5f);
            pp[1].Y = mid.Y - screen_size;
            pp[2].X = mid.X - (screen_size * 0.5f);
            pp[2].Y = mid.Y + screen_size;
            pp[3].X = mid.X + (screen_size * 0.5f);
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

// uc_orig: draw_flame_element2 (fallen/DDEngine/Source/drawxtra.cpp)
// Draws one animated fire or smoke sprite at (x,y,z) using LCG seeded by c0.
// Flame sprites use fire_pal for colour; smoke sprites use greyscale alpha fade.
static void draw_flame_element2(SLONG x, SLONG y, SLONG z, SLONG c0)
{
    SLONG trans;
    SLONG page;
    float scale;
    float u, v, w, h;
    SLONG palndx;
    SLONG dx, dy, dz;

    pyro_seed = 54321678 + c0;

    w = h = 1.0;
    u = v = 0.0;

    if (!(c0 & 3))
        page = POLY_PAGE_FLAMES;
    else
        page = POLY_PAGE_SMOKE;

    dy = get_pyro_rand() & 0x1ff;
    dy += (GAME_TURN * 5);
    dy %= 256;
    dx = (((get_pyro_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
    dz = (((get_pyro_rand() & 0xff) - 128) * ((dy >> 2) + 150)) >> 9;
    if (page == POLY_PAGE_FLAMES) {
        dx >>= 2;
        dz >>= 2;
    }

    trans = 255 - dy;

    dx += x;
    dy += y;
    dz += z;

    if (trans >= 1) {
        switch (page) {
        case POLY_PAGE_FLAMES:
            palndx = (256 - trans) * 3;
            trans <<= 24;
            trans += (fire_pal[palndx] << 16) + (fire_pal[palndx + 1] << 8) + fire_pal[palndx + 2];
            scale = 150;
            SPRITE_draw_tex2(
                float(dx),
                float(dy),
                float(dz),
                float(scale),
                trans,
                0,
                page,
                u, v, w, h,
                SPRITE_SORT_NORMAL);
            break;
        case POLY_PAGE_SMOKE:
            trans += (trans << 8) | (trans << 16) | (trans << 24);
            scale = ((dy - y) >> 1) + 50;
            dy += 50;
            SPRITE_draw_tex(
                float(dx),
                float(dy),
                float(dz),
                float(scale),
                trans,
                0,
                page,
                u, v, w, h,
                SPRITE_SORT_NORMAL);
            break;
        }
    }
}

// Hi-detail pyro constants.
// uc_orig: TEXSCALE (fallen/DDEngine/Source/drawxtra.cpp)
#define TEXSCALE 0.003f
// uc_orig: TEXSCALE2 (fallen/DDEngine/Source/drawxtra.cpp)
#define TEXSCALE2 0.006f
// uc_orig: DUSTWAVE_SECTORS (fallen/DDEngine/Source/drawxtra.cpp)
#define DUSTWAVE_SECTORS 16
// uc_orig: FIREBOMB_SPRITES (fallen/DDEngine/Source/drawxtra.cpp)
#define FIREBOMB_SPRITES 16
// uc_orig: DUSTWAVE_MULTIPLY (fallen/DDEngine/Source/drawxtra.cpp)
#define DUSTWAVE_MULTIPLY (2048 / DUSTWAVE_SECTORS)

// uc_orig: PYRO_draw_pyro (fallen/DDEngine/Source/drawxtra.cpp)
// Per-frame draw dispatch for a CLASS_PYRO Thing. Selects the rendering path
// by PyroType and delegates to per-type helpers (PYRO_draw_explosion, etc.).
void PYRO_draw_pyro(Thing* p_pyro)
{
    Pyro* pyro = PYRO_get_pyro(p_pyro);
    SLONG fx, fy, fz;
    SLONG i, j;
    GameCoord pos;
    float dir[8][2] = { { 0.0f, 1.0f }, { 0.7f, 0.7f }, { 1.0f, 0.0f }, { 0.7f, -0.7f }, { 0.0f, -1.0f }, { -0.7f, -0.7f }, { -1.0f, 0.0f }, { -0.7f, 0.7f } };
    float uvs[8][2] = { { 0.5f, 1.0f }, { 0.85f, 0.85f }, { 1.0f, 0.5f }, { 0.85f, 0.15f }, { 0.5f, 0.0f }, { 0.15f, 0.15f }, { 0.0f, 0.5f }, { 0.15f, 0.85f } };

    POLY_flush_local_rot();

    fx = p_pyro->WorldPos.X;
    fy = p_pyro->WorldPos.Y;
    fz = p_pyro->WorldPos.Z;

    switch (pyro->PyroType) {
    case PYRO_EXPLODE:
        PYRO_draw_explosion((Pyrex*)pyro);
        break;

    case PYRO_FIREWALL:
        break;
    case PYRO_FIREPOOL:
        if (pyro->thing->Flags & FLAGS_IN_VIEW) {
            SLONG radsqr, dx, dz, distsqr, radius, id;
            GameCoord ctr, edge;
            POLY_Point pp[3];
            POLY_Point* tri[3];
            POLY_Point temppnt;

            ctr = p_pyro->WorldPos;
            ctr.X >>= 8;
            ctr.Y >>= 8;
            ctr.Z >>= 8;

            POLY_transform(ctr.X, ctr.Y + 0xa, ctr.Z, pp);

            tri[0] = &pp[0];
            tri[1] = &pp[1];
            tri[2] = &pp[2];

            pp[0].colour = 0xFFFFFFFF;
            pp[1].colour = 0xFFFFFFFF;
            pp[2].colour = 0xFFFFFFFF;
            pp[0].specular = 0xFF000000;
            pp[1].specular = 0xFF000000;
            pp[2].specular = 0xFF000000;

            pp[0].u = 0.5;
            pp[0].v = 0.5;

            if (pyro->Flags & PYRO_FLAGS_TOUCHPOINT) {
                radius = pyro->radius;
                distsqr = (pyro->counter * pyro->radius) >> 7;
                distsqr *= distsqr;
            } else {
                radius = (pyro->counter * pyro->radius) >> 8;

                edge = ctr;
                edge.Y += 0xa;
                edge.X += dir[0][0] * pyro->radii[0];
                edge.Z += dir[0][1] * pyro->radii[0];
                POLY_transform(edge.X, edge.Y, edge.Z, &temppnt);
                pp[2].X = temppnt.X;
                pp[2].Y = temppnt.Y;
                pp[2].Z = temppnt.Z;
                pp[2].clip = temppnt.clip;
                pp[2].z = temppnt.z;
                pp[2].u = uvs[0][0];
                pp[2].v = uvs[0][1];

                // Throttle
                int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(16);

                for (i = 0; i < iNumFlames; i++) {
                    if (pyro->radii[i] < (unsigned)radius) {
                        id = (pyro->counter << 3) + (i << 8);
                        id &= 2047;
                        pyro->radii[i] += abs(SIN(id) / 4095);
                    } else {
                        id = ((GAME_TURN << 1) + (i << 8));
                        id &= 2047;
                        pyro->radii[i] = radius + 256 + (SIN(id) / 256);
                    }
                    j = (i + 1) & 7;
                    pp[1] = pp[2];

                    pp[2].u = uvs[j][0];
                    pp[2].v = uvs[j][1];
                    edge = ctr;
                    edge.Y += 0xa;
                    edge.X += dir[j][0] * pyro->radii[j];
                    edge.Z += dir[j][1] * pyro->radii[j];
                    POLY_transform(edge.X, edge.Y, edge.Z, &temppnt);
                    pp[2].X = temppnt.X;
                    pp[2].Y = temppnt.Y;
                    pp[2].Z = temppnt.Z;
                    pp[2].clip = temppnt.clip;
                    pp[2].z = temppnt.z;
                    if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                        POLY_add_triangle(tri, POLY_PAGE_FLAMES, UC_FALSE, UC_TRUE);
                    }
                }
            }
            id = 0;
            radsqr = radius * radius;

            // Throttle
            int iConst = (pyro->radius * pyro->radius);
            int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(iConst / (25 * 25));

            if (iNumFlames < 10) {
                iNumFlames = 10;
            }

            int iStepSize = (int)sqrtf((float)iConst / (float)iNumFlames);

            for (i = -pyro->radius; i < pyro->radius; i += iStepSize) {
                for (j = -pyro->radius; j < pyro->radius; j += iStepSize) {
                    id *= 31415965;
                    id += 123456789;
                    if ((i * i + j * j) < radsqr) {
                        pos = ctr;
                        pos.X += i;
                        pos.Z += j;
                        if (pyro->Flags & PYRO_FLAGS_TOUCHPOINT) {
                            dx = (pyro->target.X >> 8) - pos.X;
                            dz = (pyro->target.Z >> 8) - pos.Z;
                            if ((dx * dx + dz * dz) < distsqr)
                                draw_flame_element(pos.X, pos.Y, pos.Z, id, 1, 0);
                        } else {
                            draw_flame_element(pos.X, pos.Y, pos.Z, id, 1, 0);
                        }
                    }
                }
            }
        }
        break;

    case PYRO_BONFIRE:
        if (pyro->thing->Flags & FLAGS_IN_VIEW) {

            extern int AENG_detail_skyline;

            int iNumFlames = 40;
            if (!AENG_detail_skyline) {
                iNumFlames *= 2;
            } else {
                iNumFlames *= 5;
            }

            // Throttle
            iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(iNumFlames);
            draw_flames(fx >> 8, fy / 256, fz >> 8, iNumFlames, (SLONG)p_pyro);

            if (AENG_detail_skyline)
                if (!(Random() & 7))
                    PARTICLE_Add(fx + ((Random() & 0x9ff) - 0x4ff),
                        fy + ((Random() & 0x9ff) - 0x4ff),
                        fz + ((Random() & 0x9ff) - 0x4ff),
                        (Random() & 0xff) - 0x7f, 256 + (Random() & 0x1ff), (Random() & 0xff) - 0x7f,
                        POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE,
                        300, 70, 1, 1, 2);
        }
        break;

    case PYRO_WHOOMPH:
        if (pyro->thing->Flags & FLAGS_IN_VIEW) {

            UBYTE i, radius;
            // Throttle
            int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave((1 + (pyro->counter >> 4)) * 2);
            UBYTE steps = iNumFlames >> 1;
            if (steps < 3) {
                steps = 3;
            }

            UWORD angle = 0, step = 2048 / steps;
            SLONG px, py, pz;

            for (i = 0; i < steps; i++) {
                radius = SIN(pyro->counter) >> 10;
                radius += (radius >> 1);
                px = fx + ((SIN(angle) >> 6) * radius);
                py = fy;
                pz = fz + ((COS(angle) >> 6) * radius);
                PARTICLE_Add(px + ((Random() & 0x13ff) - 0x9ff),
                    py + ((Random() & 0x13ff) - 0x9ff),
                    pz + ((Random() & 0x13ff) - 0x9ff),
                    0, 0, 0,
                    POLY_PAGE_PCFLAMER, 2 + ((Random() & 3) << 2), 0xaFffffff,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE,
                    150, 70, 1, 8, 2);
                PARTICLE_Add(px + ((Random() & 0x13ff) - 0x9ff),
                    py + ((Random() & 0x13ff) - 0x9ff),
                    pz + ((Random() & 0x13ff) - 0x9ff),
                    0, 0, 0,
                    POLY_PAGE_PCFLAMER, 2 + ((Random() & 3) << 2), 0xaFffffff,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE,
                    150, 70, 1, 8, 2);

                angle += step;
                angle &= 2047;
            }
        }
        break;

    case PYRO_IMMOLATE:
        if (pyro->Flags & PYRO_FLAGS_SMOKE) {
            if (pyro->victim)
                pos = pyro->victim->WorldPos;
            else
                pos = pyro->thing->WorldPos;
            if (pyro->Flags & PYRO_FLAGS_STATIC) {
                pos.X >>= 8;
                pos.Y >>= 8;
                pos.Z >>= 8;

                // Throttle
                int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(40);

                for (i = 0; i < iNumFlames; i++) {
                    draw_flame_element(pos.X, pos.Y, pos.Z, 7 + (i << 4), 0, 0);
                }
            } else {
                PARTICLE_Add(pos.X, pos.Y + 8192, pos.Z, 0, 1024, 0, POLY_PAGE_SMOKE, 0, 0x7FFFFFFF,
                    PFLAGS_SMOKE, 80, 8, 1, 3, 4);
            }
        }
        if (pyro->Flags & PYRO_FLAGS_FLAME) {
            if (pyro->victim)
                pos = pyro->victim->WorldPos;
            else
                pos = pyro->thing->WorldPos;
            if (pyro->Flags & PYRO_FLAGS_STATIC) {
                pos.X >>= 8;
                pos.Y >>= 8;
                pos.Z >>= 8;
                // Throttle
                int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(40);

                for (i = 0; i < iNumFlames; i++) {
                    draw_flame_element(pos.X, pos.Y, pos.Z, i, 0, 0);
                }
            } else {
                PARTICLE_Add(pos.X, pos.Y + 8192, pos.Z, 0, 1024, 0, POLY_PAGE_FLAMES, 0, 0xffFFFFFF,
                    PFLAG_FIRE | PFLAG_FADE | PFLAG_WANDER, 80, 60, 1, 4, -1);
            }
        }
        if (pyro->victim) {
            if (!(pyro->victim->Flags & FLAGS_IN_VIEW))
                break;
            if ((pyro->Flags & PYRO_FLAGS_FLICKER) || (pyro->Flags & PYRO_FLAGS_BONFIRE)) {
                PrimObject* p_obj;
                ULONG sp;
                ULONG ep;
                SLONG px, py, pz;
                POLY_Point* pp;
                SLONG matrix[9];
                GameCoord bob;
                switch (pyro->victim->DrawType) {
                case DT_MESH:
                    switch (pyro->victim->Class) {
                    case CLASS_BARREL: {
                        bob = BARREL_fire_pos(pyro->victim);
                        px = bob.X + (Random() & 0x3fff) - 0x1fff;
                        py = bob.Y + (Random() & 0x3fff) - 0x13ff;
                        pz = bob.Z + (Random() & 0x3fff) - 0x1fff;
                        RIBBON_extend(pyro->radii[0], px >> 8, py >> 8, pz >> 8);
                    } break;
                    }
                    break;
                case DT_CHOPPER: {
                    p_obj = &prim_objects[pyro->victim->Draw.Mesh->ObjectId];
                    sp = p_obj->StartPoint;
                    ep = p_obj->EndPoint;
                    FMATRIX_calc(
                        matrix,
                        pyro->victim->Draw.Mesh->Angle,
                        pyro->victim->Draw.Mesh->Tilt,
                        pyro->victim->Draw.Mesh->Roll);
                    pp = &POLY_buffer[POLY_buffer_upto];

                    if (pyro->Flags & PYRO_FLAGS_FLICKER) {
                        if ((pyro->radii[7] < sp) || (pyro->radii[7] > ep))
                            pyro->radii[7] = sp;
                        pp->x = AENG_dx_prim_points[pyro->radii[7]].X;
                        pp->y = AENG_dx_prim_points[pyro->radii[7]].Y;
                        pp->z = AENG_dx_prim_points[pyro->radii[7]].Z;

                        FMATRIX_MUL(matrix, pp->x, pp->y, pp->z);

                        pos.X = (pp->x) + (pyro->victim->WorldPos.X >> 8);
                        pos.Y = (pp->y) + (pyro->victim->WorldPos.Y >> 8);
                        pos.Z = (pp->z) + (pyro->victim->WorldPos.Z >> 8);

                        RIBBON_extend(pyro->radii[0], pos.X, pos.Y, pos.Z);
                        pyro->radii[7]++;
                    }
                    if (pyro->Flags & PYRO_FLAGS_BONFIRE) {
                        for (i = sp; i < (signed)ep; i++)
                            if (!(i & 7)) {
                                ASSERT(WITHIN(POLY_buffer_upto, 0, POLY_BUFFER_SIZE - 1));

                                pp = &POLY_buffer[POLY_buffer_upto];
                                pp->x = AENG_dx_prim_points[i].X;
                                pp->y = AENG_dx_prim_points[i].Y;
                                pp->z = AENG_dx_prim_points[i].Z;

                                FMATRIX_MUL(matrix, pp->x, pp->y, pp->z);

                                pos.X = (pp->x) + (pyro->victim->WorldPos.X >> 8);
                                pos.Y = (pp->y) + (pyro->victim->WorldPos.Y >> 8);
                                pos.Z = (pp->z) + (pyro->victim->WorldPos.Z >> 8);

                                for (j = 0; j < 8; j++) {
                                    draw_flame_element2(pos.X, pos.Y, pos.Z, i + (j * 128));
                                    draw_flame_element2(pos.X, pos.Y, pos.Z, i + (j * 128) + 1);
                                }
                            }
                    }

                    return;
                } break;
                case DT_ROT_MULTI:
                    if (pyro->Flags & PYRO_FLAGS_FLICKER) {
                        SLONG px, py, pz;
                        UBYTE i, r, p;

                        if (pyro->Dummy == 2)
                            r = 5;
                        else
                            r = 1;
                        r = 2;
                        for (i = 0; i < r; i++) {
                            switch (pyro->victim->State) {
                            case STATE_DYING:
                                p = 7;
                                break;
                            case STATE_DEAD:
                                p = 3;
                                break;
                            default:
                                p = 0xf;
                                break;
                            }
                            p = rand() & p;
                            calc_sub_objects_position(
                                pyro->victim,
                                pyro->victim->Draw.Tweened->AnimTween,
                                p,
                                &px,
                                &py,
                                &pz);
                            px += pyro->victim->WorldPos.X >> 8;
                            py += pyro->victim->WorldPos.Y >> 8;
                            pz += pyro->victim->WorldPos.Z >> 8;
                            RIBBON_extend(pyro->radii[i], px, py, pz);
                        }
                    }
                    return;
                }
            }
        } else {
            if (pyro->Flags & PYRO_FLAGS_FLICKER) {
                pos = pyro->thing->WorldPos;
                pos.X += (rand() / 2);
                pos.Z += (rand() / 2);
                RIBBON_extend(pyro->radii[0], pos.X >> 8, pos.Y / 256, pos.Z >> 8);
            }
        }
        break;

    case PYRO_DUSTWAVE:
        PYRO_draw_dustwave(pyro);
        break;

    case PYRO_EXPLODE2:
        PYRO_draw_explosion2(pyro);
        break;

    case PYRO_STREAMER:
        PYRO_draw_streamer(pyro);
        break;

    case PYRO_TWANGER:
        PYRO_draw_twanger(pyro);
        {
            SLONG str;
            if (pyro->counter < 30) {
                str = pyro->counter * 5;
            } else {
                str = (285 - pyro->counter * 2);
                str >>= 1;
            }
            if (str > 0)
                BLOOM_flare_draw(pyro->thing->WorldPos.X >> 8, pyro->thing->WorldPos.Y >> 8, pyro->thing->WorldPos.Z >> 8, str);
        }
        break;

    case PYRO_NEWDOME:
        PYRO_draw_newdome(pyro);
        break;

    case PYRO_SPLATTERY:
        // this only creates other things so no drawing
        break;

    case PYRO_FIREBOMB:
        if (!(pyro->thing->Flags & FLAGS_IN_VIEW))
            break;
        {
            SLONG x, y, z, d, h, i;

            if (pyro->counter < 10) {

                // Ten "clocks" of particle-creation, each particle lives for about 70 clocks.
                int iNumSprites = IWouldLikeSomePyroSpritesHowManyCanIHave(FIREBOMB_SPRITES * 10 * 70);
                iNumSprites /= (10 * 70);
                int iMultiplier = (16 * (1 << 7)) / iNumSprites;

                for (d = 0; d < iNumSprites; d++) {
                    int iAngle = d * iMultiplier;
                    if ((pyro->Flags & PYRO_FLAGS_WAVE) && (pyro->counter < 6)) {
                        x = SIN(iAngle + (Random() & 127)) >> 3;
                        z = COS(iAngle + (Random() & 127)) >> 3;

                        PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                            x, (Random() & 0xff), z,
                            POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                            PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_DAMPING,
                            55 + (Random() & 0x3f), 80, 1, 8 - (Random() & 3), 4);
                    }

                    x = SIN(iAngle) >> 4;
                    z = COS(iAngle) >> 4;
                    i = Random() & 0xff;
                    if (pyro->counter > 3) {
                        i -= pyro->counter * 15;
                        if (i < 0)
                            i = 0;
                        h = i;
                        i = SIN(h << 1) >> 7;
                        y = COS(h << 1) >> 4;
                    } else {
                        y = (128 + (Random() & 0xff)) << 4;
                    }
                    x *= i;
                    z *= i;
                    x >>= 8;
                    z >>= 8;
                    h = (127 + (Random() & 0x7f)) << 24;
                    h |= 0xFFFFFF;

                    PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                        x, y, z,
                        POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), h,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_DAMPING | PFLAG_GRAVITY,
                        70 + (Random() & 0x3f), 160, 1, 6, -4);
                }
                d = Random() & 2047;
                x = SIN(d) >> 4;
                z = COS(d) >> 4;
                d = Random() & 0xff;
                x *= d;
                z *= d;
                x >>= 8;
                z >>= 8;
                PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                    x, (128 + (Random() & 0xff)) << 4, z,
                    POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0x7FFFFFFF,
                    PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE | PFLAG_DAMPING | PFLAG_GRAVITY,
                    75 + (Random() & 0x3f), 160, 1, 5 + (Random() & 3), -(2 + (Random() & 3)));
            }
            if (pyro->counter < 240) {
                if ((pyro->counter > 110) && (pyro->counter < 140)) {
                    PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                        0, 0, 0,
                        POLY_PAGE_FLAMES2, 2 + ((Random() & 3) << 2), 0xffFFFFFF,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FADE2 | PFLAG_RESIZE,
                        100, 255, 1, 20, 5);
                }
                if ((pyro->counter > 4) && (pyro->counter < 110)) {
                    d = Random() & 2047;
                    x = SIN(d) >> 4;
                    z = COS(d) >> 4;
                    h = (Random() & 0x7f);
                    i = SIN(h << 1) >> 8;
                    y = COS(h << 1) >> 4;
                    x *= i;
                    z *= i;
                    x >>= 8;
                    z >>= 8;
                    h = 0x7fffffff;
                    PARTICLE_Add(pyro->thing->WorldPos.X, pyro->thing->WorldPos.Y, pyro->thing->WorldPos.Z,
                        x, y, z,
                        POLY_PAGE_SMOKECLOUD2, 2 + ((Random() & 3) << 2), h,
                        PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_FIRE | PFLAG_FADE | PFLAG_RESIZE | PFLAG_DAMPING,
                        70 + (Random() & 0x3f), 100, 1, 2, 4 + (Random() & 3));
                }
            }
        }
        break;

    case PYRO_GAMEOVER:
        PYRO_draw_armageddon(pyro);
        break;

    case PYRO_HITSPANG:
        break;
    }
}

// ---- Pyro draw helpers (from drawxtra.cpp) ----

// uc_orig: PYRO_draw_explosion (fallen/DDEngine/Source/drawxtra.cpp)
// Draws old-style hemisphere explosion (PYRO_EXPLODE) using a Pyrex per-point radius array.
// 8 bottom-ring quads + 8 top-ring triangles fan to a centre apex.
static void PYRO_draw_explosion(Pyrex* pyro)
{
    POLY_Point pp[3];
    POLY_Point* tri[3];
    UBYTE i, j;
    SLONG spec;
    SLONG cx, cy, cz;
    RadPoint points[17];

    tri[0] = &pp[0];
    tri[1] = &pp[1];
    tri[2] = &pp[2];

    cx = pyro->thing->WorldPos.X >> 8;
    cy = pyro->thing->WorldPos.Y >> 8;
    cz = pyro->thing->WorldPos.Z >> 8;

    for (i = 0; i < 16; i++) {
        points[i].x = (PYRO_defaultpoints[i].x * pyro->points[i].radius) >> 16;
        points[i].y = (PYRO_defaultpoints[i].y * pyro->points[i].radius) >> 16;
        points[i].z = (PYRO_defaultpoints[i].z * pyro->points[i].radius) >> 16;
    }
    points[16].y = (65535 * pyro->points[i].radius) >> 16;

    spec = (255 - (pyro->thing->Genus.Pyro->Timer1 * 4));
    if (spec < 0)
        spec = 0;
    spec *= 0x010101;

    DIRT_gust(pyro->thing, cx, cz, (points[0].x / 4) + cx, (points[0].z / 4) + cz);
    DIRT_gust(pyro->thing, cx, cz, (points[4].x / 4) + cx, (points[4].z / 4) + cz);

    // Draw bottom "rung"
    for (i = 0; i < 8; i++) {

        j = i + 1;
        if (j == 8)
            j = 0;
        POLY_transform(points[i].x + cx,
            points[i].y + cy,
            points[i].z + cz,
            &pp[0]);
        pp[0].u = points[i].x * TEXSCALE;
        pp[0].v = points[i].z * TEXSCALE;
        POLY_transform(points[i + 8].x + cx,
            points[i + 8].y + cy,
            points[i + 8].z + cz,
            &pp[1]);
        pp[1].u = points[i + 8].x * TEXSCALE;
        pp[1].v = points[i + 8].z * TEXSCALE;
        POLY_transform(points[j + 8].x + cx,
            points[j + 8].y + cy,
            points[j + 8].z + cz,
            &pp[2]);
        pp[2].u = points[j + 8].x * TEXSCALE;
        pp[2].v = points[j + 8].z * TEXSCALE;
        pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + (pyro->thing->Genus.Pyro->Timer1 << 24);
        pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
            POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
        }

        POLY_transform(points[i].x + cx,
            points[i].y + cy,
            points[i].z + cz,
            &pp[0]);
        pp[0].u = points[i].x * TEXSCALE;
        pp[0].v = points[i].z * TEXSCALE;
        POLY_transform(points[j + 8].x + cx,
            points[j + 8].y + cy,
            points[j + 8].z + cz,
            &pp[1]);
        pp[1].u = points[j + 8].x * TEXSCALE;
        pp[1].v = points[j + 8].z * TEXSCALE;
        POLY_transform(points[j].x + cx,
            points[j].y + cy,
            points[j].z + cz,
            &pp[2]);
        pp[2].u = points[j].x * TEXSCALE;
        pp[2].v = points[j].z * TEXSCALE;
        pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + (pyro->thing->Genus.Pyro->Timer1 << 24);
        pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
            POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
        }
    }

    // Draw top "rung"
    for (i = 8; i < 16; i++) {
        j = i + 1;
        if (j == 16)
            j = 8;
        POLY_transform(points[i].x + cx,
            points[i].y + cy,
            points[i].z + cz,
            &pp[0]);
        pp[0].u = points[i].x * TEXSCALE;
        pp[0].v = points[i].z * TEXSCALE;
        POLY_transform(cx,
            points[16].y + cy,
            cz,
            &pp[1]);
        pp[1].u = 0;
        pp[1].v = 0;
        POLY_transform(points[j].x + cx,
            points[j].y + cy,
            points[j].z + cz,
            &pp[2]);
        pp[2].u = points[j].x * TEXSCALE;
        pp[2].v = points[j].z * TEXSCALE;
        pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + (pyro->thing->Genus.Pyro->Timer1 << 24);
        pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
            POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
        }
    }
}

// uc_orig: PYRO_draw_dustwave (fallen/DDEngine/Source/drawxtra.cpp)
// Renders PYRO_DUSTWAVE as a multi-ring expanding shockwave using triangle strips.
// iNumSectors dynamically throttled via IWouldLikeSomePyroSpritesHowManyCanIHave (min 7).
static void PYRO_draw_dustwave(Pyro* pyro)
{
    POLY_Point pp[3], mid;
    POLY_Point* tri[3] = { &pp[0], &pp[1], &pp[2] };
    SLONG cx, cy, cz, fade;
    UBYTE sections, pass, sector, next;
    SLONG dxs[DUSTWAVE_SECTORS], dys[DUSTWAVE_SECTORS], dists[4], heights[4];

    cx = pyro->thing->WorldPos.X >> 8;
    cy = pyro->thing->WorldPos.Y >> 8;
    cz = pyro->thing->WorldPos.Z >> 8;
    POLY_transform(cx, cy, cz, &mid);
    mid.u = 0.5;
    mid.v = 1.0;

    sections = 3;

    int iNumSectors = IWouldLikeSomePyroSpritesHowManyCanIHave(DUSTWAVE_SECTORS);

    if (iNumSectors < 7) {
        iNumSectors = 7;
    }

    int iMultiplier = 2048 / iNumSectors;

    for (sector = 0; sector < iNumSectors; sector++) {
        dxs[sector] = SIN((sector * iMultiplier)) >> 8;
        dys[sector] = COS((sector * iMultiplier)) >> 8;
    }

    // Precalculate ring radii and heights.
    if (pyro->counter > 1)
        dists[0] = 512 + SIN(pyro->counter * 4) / 256;
    else
        dists[0] = 256 + SIN(pyro->counter * 4) / 256;
    heights[0] = 2;

    dists[1] = (dists[0] * SIN(pyro->counter * 4)) / 65536;
    heights[1] = SIN(pyro->counter * 4) / 1024;

    dists[2] = (dists[1] * SIN(pyro->counter * 4)) / 65536;
    heights[2] = heights[1] * 0.75f;

    dists[3] = (dists[2] * SIN(pyro->counter * 4)) / 65536;
    heights[3] = 2;

    fade = pyro->counter << 25;

    // Draw the triangle-strip rings.
    for (pass = 0; pass < sections; pass++) {
        for (sector = 0; sector < iNumSectors; sector++) {
            next = sector + 1;
            if (next == iNumSectors)
                next = 0;

            POLY_transform(cx + ((dists[pass] * dxs[sector]) / 256), cy + heights[pass], cz + ((dists[pass] * dys[sector]) / 256), &pp[0]);
            POLY_transform(cx + ((dists[pass] * dxs[next]) / 256), cy + heights[pass], cz + ((dists[pass] * dys[next]) / 256), &pp[1]);
            pp[0].u = 0.0f;
            pp[0].v = pass * 0.40f;
            pp[1].u = 1.0f;
            pp[1].v = pass * 0.40f;

            if ((pass < sections - 1) || (pass == 2)) {
                POLY_transform(cx + (dists[pass + 1] * dxs[sector]) / 256, cy + heights[pass + 1], cz + (dists[pass + 1] * dys[sector]) / 256, &pp[2]);
                pp[2].u = 0.0f;
                pp[2].v = (pass + 1) * 0.40f;
            } else {
                pp[2] = mid;
            }
            pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF | fade;
            pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000;
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, POLY_PAGE_DUSTWAVE, UC_FALSE);
            }

            if ((pass < sections - 1) || (pass == 2)) {
                POLY_transform(cx + ((dists[pass + 1] * dxs[next]) / 256), cy + heights[pass + 1], cz + ((dists[pass + 1] * dys[next]) / 256), &pp[0]);
                pp[0].u = 1;
                pp[0].v = (pass + 1) * 0.40f;
                pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + fade;
                pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000;
                if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                    POLY_add_triangle(tri, POLY_PAGE_DUSTWAVE, UC_FALSE);
                }
            }
        }
    }

    // Stir up ground dust from a rotating sector around the outer ring.
    {
        static UBYTE shock_sector = 0;
        DIRT_gust(pyro->thing,
            cx + ((dists[2] * dxs[shock_sector]) / 256),
            cz + ((dists[2] * dys[shock_sector]) / 256),
            cx + ((dists[1] * dxs[shock_sector]) / 256),
            cz + ((dists[1] * dys[shock_sector]) / 256));
        shock_sector++;
        if (shock_sector == iNumSectors)
            shock_sector = 0;
    }
}

// uc_orig: PYRO_draw_explosion2 (fallen/DDEngine/Source/drawxtra.cpp)
// Draws PYRO_EXPLODE2 sphere explosion with 4 rings of 8 points, throttled via iIncrement.
// PYRO_defaultpoints2 is initialised lazily on first call (flags == 0).
static void PYRO_draw_explosion2(Pyro* pyro)
{
    POLY_Point pp[3];
    POLY_Point* tri[3];
    UBYTE i, j, k, b;
    SLONG spec, fade[4];
    SLONG cx, cy, cz;
    RadPoint points[33];
    SLONG sc_radius;
    float subscale;

    subscale = (170 - pyro->counter);
    subscale /= 32;
    subscale *= TEXSCALE2;

    // Lazy init of the 32-point hemisphere template.
    if (!PYRO_defaultpoints2[0].flags) {
        SLONG height, radius;
        RadPoint* pt;

        PYRO_defaultpoints2[0].flags = 1;
        pt = PYRO_defaultpoints2;
        for (i = 0; i < 4; i++) {

            height = SIN(i * 128);
            radius = COS(i * 128) >> 8;

            for (j = 0; j < 8; j++) {
                pt->x = (radius * ((SLONG)SIN(j * 256))) / 256;
                pt->z = (radius * ((SLONG)COS(j * 256))) / 256;
                pt->y = height;
                pt++;
            }
        }
    }

    tri[0] = &pp[0];
    tri[1] = &pp[1];
    tri[2] = &pp[2];

    cx = pyro->thing->WorldPos.X >> 8;
    cy = pyro->thing->WorldPos.Y >> 8;
    cz = pyro->thing->WorldPos.Z >> 8;

    sc_radius = (pyro->radius * pyro->scale) / 256;

    // Throttle: reduce sector count if too many sprites requested.
    int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(8 * 4);
    iNumFlames >>= 2;
    int iIncrement = 1;
    if (iNumFlames < 6) {
        iIncrement = 2;
    }

    for (i = 0; i < 32; i += iIncrement) {
        points[i].x = (PYRO_defaultpoints2[i].x * sc_radius) >> 16;
        points[i].y = (PYRO_defaultpoints2[i].y * sc_radius) >> 16;
        points[i].z = (PYRO_defaultpoints2[i].z * sc_radius) >> 16;
    }
    points[32].y = (65535 * sc_radius) >> 16;

    spec = (255 - (pyro->counter * 2));
    if (spec < 0)
        spec = 0;
    spec *= 0x010101;

    if (pyro->counter > 170) {
        fade[3] = SIN((pyro->counter - 170) * 6) >> 8;

        fade[2] = fade[3] * 2;
        if (fade[2] > 255)
            fade[2] = 255;

        fade[1] = fade[2] * 2;
        if (fade[1] > 255)
            fade[1] = 255;

        fade[0] = fade[1] * 2;
        if (fade[0] > 255)
            fade[0] = 255;

        fade[0] <<= 24;
        fade[1] <<= 24;
        fade[2] <<= 24;
        fade[3] <<= 24;
    } else
        fade[0] = fade[1] = fade[2] = fade[3] = 0;

    // Draw 3 rings of quads.
    for (k = 0; k < 3; k++) {
        b = k * 8;
        for (i = b; i < b + 8; i += iIncrement) {
            j = i + iIncrement;
            if (j == b + 8)
                j = b;
            POLY_transform(points[i].x + cx,
                points[i].y + cy,
                points[i].z + cz,
                &pp[0]);
            pp[0].u = points[i].x * subscale;
            pp[0].v = points[i].z * subscale;
            POLY_transform(points[i + 8].x + cx,
                points[i + 8].y + cy,
                points[i + 8].z + cz,
                &pp[1]);
            pp[1].u = points[i + 8].x * subscale;
            pp[1].v = points[i + 8].z * subscale;
            POLY_transform(points[j + 8].x + cx,
                points[j + 8].y + cy,
                points[j + 8].z + cz,
                &pp[2]);
            pp[2].u = points[j + 8].x * subscale;
            pp[2].v = points[j + 8].z * subscale;
            pp[0].colour = 0xFFFFFF + fade[k];
            pp[1].colour = pp[2].colour = 0xFFFFFF + fade[k + 1];
            pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
            }

            POLY_transform(points[j].x + cx,
                points[j].y + cy,
                points[j].z + cz,
                &pp[1]);
            pp[1].u = points[j].x * subscale;
            pp[1].v = points[j].z * subscale;
            pp[1].colour = 0xFFFFFF + fade[k];
            pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
            }
        }
    }

    // Draw top cap fan.
    for (i = 24; i < 32; i += iIncrement) {
        j = i + iIncrement;
        if (j == 32)
            j = 24;
        POLY_transform(points[i].x + cx,
            points[i].y + cy,
            points[i].z + cz,
            &pp[0]);
        pp[0].u = points[i].x * subscale;
        pp[0].v = points[i].z * subscale;
        POLY_transform(cx,
            points[32].y + cy,
            cz,
            &pp[1]);
        pp[1].u = 0;
        pp[1].v = 0;
        POLY_transform(points[j].x + cx,
            points[j].y + cy,
            points[j].z + cz,
            &pp[2]);
        pp[2].u = points[j].x * subscale;
        pp[2].v = points[j].z * subscale;
        pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + fade[3];
        pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
            POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
        }
    }
}

// uc_orig: PYRO_draw_newdome (fallen/DDEngine/Source/drawxtra.cpp)
// Like PYRO_draw_explosion2 but overlays animated sprite flashes via SPRITE_draw_tex,
// and uses a deterministic random seed per pyro instance for repeatable spark patterns.
static void PYRO_draw_newdome(Pyro* pyro)
{
    POLY_Point pp[3];
    POLY_Point* tri[3];
    UBYTE i, j, k, b;
    SLONG spec, fade[4];
    SLONG cx, cy, cz;
    RadPoint points[33];
    SLONG sc_radius;
    float subscale;
    float u, v;
    UBYTE iu, iv;
    ULONG store_seed;

    subscale = (170 - pyro->counter);
    subscale /= 32;
    subscale *= TEXSCALE2;

    // Lazy init of the 32-point hemisphere template (shared with PYRO_draw_explosion2).
    if (!PYRO_defaultpoints2[0].flags) {
        SLONG height, radius;
        RadPoint* pt;

        PYRO_defaultpoints2[0].flags = 1;
        pt = PYRO_defaultpoints2;
        for (i = 0; i < 4; i++) {

            height = SIN(i * 128);
            radius = COS(i * 128) >> 8;

            for (j = 0; j < 8; j++) {
                pt->x = (radius * ((SLONG)SIN(j * 256))) / 256;
                pt->z = (radius * ((SLONG)COS(j * 256))) / 256;
                pt->y = height;
                pt++;
            }
        }
    }

    // Save and restore random seed to get consistent per-pyro spark placement.
    store_seed = GetSeed();
    SetSeed((ULONG)pyro);

    tri[0] = &pp[0];
    tri[1] = &pp[1];
    tri[2] = &pp[2];

    cx = pyro->thing->WorldPos.X >> 8;
    cy = pyro->thing->WorldPos.Y >> 8;
    cz = pyro->thing->WorldPos.Z >> 8;

    sc_radius = (pyro->radius * pyro->scale) / 256;

    int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(8 * 4);
    iNumFlames >>= 2;
    int iIncrement = 1;
    if (iNumFlames < 6) {
        iIncrement = 2;
    }

    for (i = 0; i < 32; i += iIncrement) {
        points[i].x = (PYRO_defaultpoints2[i].x * sc_radius) >> 16;
        points[i].y = (PYRO_defaultpoints2[i].y * sc_radius) >> 16;
        points[i].z = (PYRO_defaultpoints2[i].z * sc_radius) >> 16;
    }
    points[32].y = (65535 * sc_radius) >> 16;

    spec = (255 - (pyro->counter * 1));
    if (spec < 0)
        spec = 0;
    spec *= 0x010101;

    if (pyro->counter > 170) {
        fade[3] = SIN((pyro->counter - 170) * 6) >> 8;

        fade[2] = fade[3] * 2;
        if (fade[2] > 255)
            fade[2] = 255;

        fade[1] = fade[2] * 2;
        if (fade[1] > 255)
            fade[1] = 255;

        fade[0] = fade[1] * 2;
        if (fade[0] > 255)
            fade[0] = 255;

        fade[0] <<= 24;
        fade[1] <<= 24;
        fade[2] <<= 24;
        fade[3] <<= 24;
    } else
        fade[0] = fade[1] = fade[2] = fade[3] = 0;

    // Animated sprite overlay (4x4 sprite sheet, 16 frames by counter).
    iu = (pyro->counter >> 4);
    iv = iu >> 2;
    iu = iu & 3;
    u = (float)iu;
    v = (float)iv;
    u *= 0.25f;
    v *= 0.25f;
    SPRITE_draw_tex(cx, cy, cz,
        pyro->radius << 2, 0xFFFFFF | (pyro->counter << 24), 0xff000000, POLY_PAGE_EXPLODE1_ADDITIVE - (Random() & 1), u, v, 0.25, 0.25, SPRITE_SORT_NORMAL);
    SPRITE_draw_tex(cx, cy, cz,
        pyro->radius << 3, 0xFFFFFF | (pyro->counter << 24), 0xff000000, POLY_PAGE_EXPLODE1_ADDITIVE - (Random() & 1), u, v, 0.25, 0.25, SPRITE_SORT_NORMAL);

    // Draw 3 rings of quads.
    for (k = 0; k < 3; k++) {
        b = k * 8;
        for (i = b; i < b + 8; i += iIncrement) {
            j = i + iIncrement;
            if (j == b + 8)
                j = b;
            POLY_transform(points[i].x + cx,
                points[i].y + cy,
                points[i].z + cz,
                &pp[0]);
            pp[0].u = points[i].x * subscale;
            pp[0].v = points[i].z * subscale;
            POLY_transform(points[i + 8].x + cx,
                points[i + 8].y + cy,
                points[i + 8].z + cz,
                &pp[1]);
            pp[1].u = points[i + 8].x * subscale;
            pp[1].v = points[i + 8].z * subscale;
            POLY_transform(points[j + 8].x + cx,
                points[j + 8].y + cy,
                points[j + 8].z + cz,
                &pp[2]);
            pp[2].u = points[j + 8].x * subscale;
            pp[2].v = points[j + 8].z * subscale;
            pp[0].colour = 0xFFFFFF + fade[k];
            pp[1].colour = pp[2].colour = 0xFFFFFF + fade[k + 1];
            pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
            }

            if (Random() & 3) {
                iu = (pyro->counter >> 4);
                iv = iu >> 2;
                iu = iu & 3;
                u = (float)iu;
                v = (float)iv;
                u *= 0.25f;
                v *= 0.25f;
                SPRITE_draw_tex(points[j].x + cx, points[j].y + cy, points[j].z + cz,
                    pyro->radius << 1, 0xFFFFFF | (pyro->counter << 24), 0xff000000, POLY_PAGE_EXPLODE1_ADDITIVE - (Random() & 1), u, v, 0.25, 0.25, SPRITE_SORT_NORMAL);
            }

            POLY_transform(points[j].x + cx,
                points[j].y + cy,
                points[j].z + cz,
                &pp[1]);
            pp[1].u = points[j].x * subscale;
            pp[1].v = points[j].z * subscale;
            pp[1].colour = 0xFFFFFF + fade[k];
            pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
            if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
                POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
            }
        }
    }

    // Draw top cap fan with sprite overlays.
    for (i = 24; i < 32; i += iIncrement) {
        j = i + iIncrement;
        if (j == 32)
            j = 24;
        if (Random() & 1)
            SPRITE_draw_tex(points[i].x + cx, points[i].y + cy, points[i].z + cz,
                pyro->radius << 1, 0xFFFFFF | (pyro->counter << 24), 0xff000000, POLY_PAGE_EXPLODE1_ADDITIVE, u, v, 0.25, 0.25, SPRITE_SORT_NORMAL);
        POLY_transform(points[i].x + cx,
            points[i].y + cy,
            points[i].z + cz,
            &pp[0]);
        pp[0].u = points[i].x * subscale;
        pp[0].v = points[i].z * subscale;
        POLY_transform(cx,
            points[32].y + cy,
            cz,
            &pp[1]);
        pp[1].u = 0;
        pp[1].v = 0;
        POLY_transform(points[j].x + cx,
            points[j].y + cy,
            points[j].z + cz,
            &pp[2]);
        pp[2].u = points[j].x * subscale;
        pp[2].v = points[j].z * subscale;
        pp[0].colour = pp[1].colour = pp[2].colour = 0xFFFFFF + fade[3];
        pp[0].specular = pp[1].specular = pp[2].specular = 0xFF000000 + spec;
        if (pp[0].MaybeValid() && pp[1].MaybeValid() && pp[2].MaybeValid()) {
            POLY_add_triangle(tri, POLY_PAGE_BIGBANG, UC_FALSE);
        }
    }

    SetSeed(store_seed);
}

// uc_orig: PYRO_alpha_line (fallen/DDEngine/Source/drawxtra.cpp)
// Draws an alpha-blended world-space line segment between two points with variable width.
static void PYRO_alpha_line(
    SLONG x1, SLONG y1, SLONG z1, SLONG width1, ULONG colour1,
    SLONG x2, SLONG y2, SLONG z2, SLONG width2, ULONG colour2,
    SLONG sort_to_front)
{
    POLY_Point p1;
    POLY_Point p2;

    POLY_transform(float(x1), float(y1), float(z1), &p1);
    POLY_transform(float(x2), float(y2), float(z2), &p2);

    if (POLY_valid_line(&p1, &p2)) {
        p1.colour = colour1;
        p1.specular = 0xff000000;

        p2.colour = colour2;
        p2.specular = 0xff000000;

        POLY_add_line(&p1, &p2, float(width1), float(width2), POLY_PAGE_ALPHA, sort_to_front);
    }
}

// uc_orig: PYRO_draw_twanger (fallen/DDEngine/Source/drawxtra.cpp)
// Draws PYRO_TWANGER as 8 alpha lines radiating from the centre, rotating over time.
static void PYRO_draw_twanger(Pyro* pyro)
{
    SLONG cx, cy, cz, c;
    SLONG dx, dy, dz;
    SLONG col1, col2, dir, ang;
    UBYTE i;

    cx = pyro->thing->WorldPos.X >> 8;
    cy = pyro->thing->WorldPos.Y >> 8;
    cz = pyro->thing->WorldPos.Z >> 8;

    int iNumFlames = IWouldLikeSomePyroSpritesHowManyCanIHave(8);
    int iIncrement = 1;
    if (iNumFlames < 6) {
        iIncrement = 2;
    }

    for (i = 0; i < 8; i += iIncrement) {
        ang = pyro->radii[i] & 0xff;
        dir = (pyro->radii[i] >> 4);

        c = ((COS(ang) >> 8) * pyro->counter) / 128;

        dx = ((SIN(dir) / 256) * c) / 256;
        dy = 0;
        dz = ((COS(dir) / 256) * c) / 256;

        dx = (dx * pyro->scale) / 256;
        dz = (dz * pyro->scale) / 256;

        if ((!pyro->tints[0]) && !pyro->tints[1]) {
            col1 = 0x00FFFFFF + ((COS(pyro->counter * 2) & 0xFF00) << 16);
            col2 = 0x00FFFFFF - (SIN(pyro->counter * 2) >> 8);
        } else {
            col1 = pyro->tints[0] + ((COS(pyro->counter * 2) & 0xFF00) << 16);
            col2 = pyro->tints[1];
        }

        c = 256 - (COS(pyro->counter * 2) >> 8);

        PYRO_alpha_line(cx, cy, cz, 2, col1, cx + dx, cy + dy, cz + dz, c, col2, 1);
    }
}

// uc_orig: PYRO_draw_streambit (fallen/DDEngine/Source/drawxtra.cpp)
// Emits one steam particle for a single streamer arm at the given counter value.
static void PYRO_draw_streambit(Pyro* pyro, SLONG cx, SLONG cy, SLONG cz, SLONG c, UBYTE i)
{
    SLONG x, y, z, dx, dy, dir;

    dir = (pyro->radii[i + 4] >> 8) * 16;
    dx = SIN(pyro->radii[i + 4] & 0xff) / 256;
    dy = COS(pyro->radii[i + 4] & 0xff) / 256;
    y = ((SIN(c * 4) / 256) * dy) + cy;
    c = (c * dx) / 128;
    x = ((SIN(dir) * c) / 128) + cx;
    z = ((COS(dir) * c) / 128) + cz;
    PARTICLE_Add(x, y, z, 0, -2, 0, POLY_PAGE_STEAM, 1 + ((rand() & 3) << 2), 0x888888, PFLAG_RESIZE | PFLAG_FADE, 40 + (rand() & 0xf), 4, 1, 5, 2);
}

// uc_orig: PYRO_draw_streamer (fallen/DDEngine/Source/drawxtra.cpp)
// Draws PYRO_STREAMER: emits two steam particles per active arm when counter is in [64, 320).
static void PYRO_draw_streamer(Pyro* pyro)
{
    UBYTE i;
    SLONG cx, cy, cz;

    cx = pyro->thing->WorldPos.X;
    cy = pyro->thing->WorldPos.Y;
    cz = pyro->thing->WorldPos.Z;

    for (i = 0; i < 4; i++)
        if ((pyro->radii[i] > 64) && (pyro->radii[i] < 320)) {
            PYRO_draw_streambit(pyro, cx, cy, cz, pyro->radii[i] - 64, i);
            PYRO_draw_streambit(pyro, cx, cy, cz, pyro->radii[i] - 60, i);
        } else if ((pyro->radii[i] > 320) && (pyro->radii[i] < 400)) {
            pyro->radii[i] = 400;
            pyro->counter--;
        }
}


// uc_orig: PYRO_draw_armageddon (fallen/DDEngine/Source/drawxtra.cpp)
// Draws PYRO_GAMEOVER (final boss): spawns PYRO_NEWDOME and meteor particles around Darci's
// position each frame, plus a fireball sound. Moves the pyro thing to Darci every frame.
static void PYRO_draw_armageddon(Pyro* pyro)
{
    Thing* thing;
    GameCoord pos;

    if (GAMEMENU_is_paused())
        return;

    move_thing_on_map(pyro->thing, &NET_PERSON(0)->WorldPos);

    SLONG and_1;
    SLONG and_2;

    {
        and_1 = 2;
        and_2 = 3;
    }

    if (!(Random() & and_1)) {
        pos = pyro->thing->WorldPos;
        pos.X += ((Random() & 0xff00) - 0x7f00) << 3;
        pos.Z += ((Random() & 0xff00) - 0x7f00) << 3;
        pos.Y = PAP_calc_map_height_at(pos.X >> 8, pos.Z >> 8) << 8;
        thing = PYRO_create(pos, PYRO_NEWDOME);
        if (thing)
            thing->Genus.Pyro->scale = (400 + Random() & 0x7f) + (pyro->counter << 1);
    }
    if (!(Random() & and_2)) {
        SLONG flags = PFLAG_SPRITEANI | PFLAG_SPRITELOOP | PFLAG_EXPLODE_ON_IMPACT | PFLAG_LEAVE_TRAIL;
        if (Random() & 1)
            flags |= PFLAG_GRAVITY;
        pos = pyro->thing->WorldPos;
        pos.X += ((Random() & 0xff00) - 0x7f00) << 3;
        pos.Z += ((Random() & 0xff00) - 0x7f00) << 3;
        pos.Y = PAP_calc_map_height_at(pos.X >> 8, pos.Z >> 8) << 8;

        PARTICLE_Add(
            pos.X,
            pos.Y + 0x1000,
            pos.Z,
            0,
            (0xff + (Random() & 0xff) << 4),
            0,
            POLY_PAGE_METEOR,
            2 + ((Random() & 0x3) << 2),
            0xffffffff,
            flags,
            100,
            160,
            1,
            1,
            1);

        MFX_play_xyz(THING_NUMBER(pyro->thing), S_BALROG_FIREBALL, MFX_OVERLAP, pos.X, pos.Y, pos.Z);
    }
}
