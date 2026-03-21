// Migrated to new/: MAV_init, MAV_calc_height_array, MAV_turn_off_square,
// MAV_turn_off_whole_square, MAV_turn_off_whole_square_car, MAV_remove_facet_car,
// MAV_turn_movement_off, MAV_turn_movement_on, MAV_precalculate
// Globals (MAV_opt, MAV_nav, MAV_nav_pitch) migrated to new/ai/mav_globals.cpp

// Game.h first: brings in MFStdLib.h (MFFileHandle) and string.h (strcpy) via the anim.h chain.
#include "fallen/Headers/Game.h"
#include "ai/mav.h"
#include "fallen/DDEngine/Headers/aeng.h"
#include "fallen/Headers/supermap.h"
#include "fallen/Headers/pap.h"
#include "fallen/Headers/memory.h"
#include "fallen/Headers/collide.h"
#include "fallen/Headers/walkable.h"
#include "fallen/Headers/ware.h"
#include "fallen/Headers/ob.h"

void MAV_draw(
    SLONG sx1, SLONG sz1,
    SLONG sx2, SLONG sz2)
{
    SLONG i;
    SLONG j;

    SLONG x;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;
    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    SLONG lx;
    SLONG lz;

    MAV_Opt* mo;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    ULONG colour[8] = {
        0x00ff0000,
        0x0000ff00,
        0x000000ff,
        0x00ffff00,
        0x00ff00ff,
        0x0000ffff,
        0x00ffaa88,
        0x00ffffff
    };

    SATURATE(sx1, 0, MAP_WIDTH - 1);
    SATURATE(sz1, 0, MAP_HEIGHT - 1);

    for (x = sx1; x < sx2; x++)
        for (z = sz1; z < sz2; z++) {
            //
            // Draw a blue cross at the height we think the square is at.
            //

            x1 = x + 0 << 8;
            z1 = z + 0 << 8;
            x2 = x + 1 << 8;
            z2 = z + 1 << 8;

            y1 = MAVHEIGHT(x, z) << 6;
            y2 = MAVHEIGHT(x, z) << 6;

            AENG_world_line(
                x1, y1, z1, 4, 0x00000077,
                x2, y2, z2, 4, 0x00000077,
                TRUE);

            AENG_world_line(
                x2, y1, z1, 4, 0x00000077,
                x1, y2, z2, 4, 0x00000077,
                TRUE);

            //
            // Draw the options for leaving this square.
            //

            if (!ControlFlag) {
                ASSERT(WITHIN(MAV_NAV(x, z), 0, MAV_opt_upto - 1));

                mo = &MAV_opt[MAV_NAV(x, z)];

                mx = x1 + x2 >> 1;
                mz = z1 + z2 >> 1;

                for (i = 0; i < 4; i++) {
                    dx = order[i].dx;
                    dz = order[i].dz;

                    lx = mx + dx * 96;
                    lz = mz + dz * 96;

                    lx += dz * (16 * 3);
                    lz += -dx * (16 * 3);

                    for (j = 0; j < 8; j++) {
                        if (mo->opt[i] & (1 << j)) {
                            AENG_world_line(
                                mx, y1, mz, 0, 0,
                                lx, y2, lz, 9, colour[j],
                                TRUE);
                        }

                        lx += -dz * 16;
                        lz += +dx * 16;
                    }
                }
            } else {
                mx = x1 + x2 >> 1;
                mz = z1 + z2 >> 1;

                for (i = 0; i < 4; i++) {
                    dx = order[i].dx;
                    dz = order[i].dz;

                    lx = mx + dx * 96;
                    lz = mz + dz * 96;

                    if (MAV_CAR_GOTO(x, z, i)) {
                        AENG_world_line(
                            mx, y1, mz, 0, 0,
                            lx, y2, lz, 9, colour[0],
                            TRUE);
                    }
                }
            }
        }
}

//
// claude-ai: MAV_can_i_walk is the path-smoothing function: given a sequence of grid waypoints,
// claude-ai: it checks whether the agent can walk in a straight line between two grid cells
// claude-ai: by stepping through intermediate cells and checking MAV_CAPS_GOTO on each crossed edge.
// claude-ai: Used by MAV_get_first_action_from_nodelist to skip intermediate waypoints (path shortcutting).
// claude-ai: Only checks plain-walk (MAV_CAPS_GOTO), not jumps/climbs — those must be traversed cell-by-cell.
// Returns TRUE if you can walk from a to b. If it returns FALSE, then
// (MAV_last_x, MAV_last_z) is the last square it reached, and (MAV_dmx, MAV_dmz)
// is the direction it tried to leave the square in.
//

SLONG MAV_last_mx;
SLONG MAV_last_mz;
SLONG MAV_dmx;
SLONG MAV_dmz;

SLONG MAV_can_i_walk(
    UBYTE ax, UBYTE az,
    UBYTE bx, UBYTE bz)
{
    SLONG x;
    SLONG z;

    SLONG dx;
    SLONG dz;

    SLONG dist;
    SLONG overdist;

    SLONG mx;
    SLONG mz;

    MAV_Opt* mo;

    ASSERT(WITHIN(ax, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(az, 0, MAP_HEIGHT - 1));

    ASSERT(WITHIN(bx, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(bz, 0, MAP_HEIGHT - 1));

    dx = bx - ax << 4;
    dz = bz - az << 4;

    dist = QDIST2(abs(dx), abs(dz));

    if (dist == 0) {
        return TRUE;
    } else {
        //
        // Normalise (dx,dz) to length 0x40 ish
        //

        overdist = 0x4000 / dist;

        dx = dx * overdist >> 8;
        dz = dz * overdist >> 8;
    }

    MAV_last_mx = ax;
    MAV_last_mz = az;

    x = (ax << 8) + 0x80;
    z = (az << 8) + 0x80;

    while (1) {
        x += dx;
        z += dz;

        mx = x >> 8;
        mz = z >> 8;

        MAV_dmx = mx - MAV_last_mx;
        MAV_dmz = mz - MAV_last_mz;

        if (MAV_dmx | MAV_dmz) {
            ASSERT(WITHIN(MAV_last_mx, 0, MAP_WIDTH - 1));
            ASSERT(WITHIN(MAV_last_mz, 0, MAP_HEIGHT - 1));

            ASSERT(WITHIN(MAV_NAV(MAV_last_mx, MAV_last_mz), 0, MAV_opt_upto - 1));

            mo = &MAV_opt[MAV_NAV(MAV_last_mx, MAV_last_mz)];

            //
            // Is there a wall in the way?
            //

            if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                return FALSE;
            }
            if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                return FALSE;
            }

            if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                return FALSE;
            }
            if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                return FALSE;
            }

            if (MAV_dmx && MAV_dmz) {
                //
                // We have to try the corner pieces as well because we are moving diagonally.
                //

                mo = &MAV_opt[MAV_NAV(mx, MAV_last_mz)];

                if (MAV_dmz == -1 && !(mo->opt[MAV_DIR_ZS] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
                if (MAV_dmz == +1 && !(mo->opt[MAV_DIR_ZL] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }

                mo = &MAV_opt[MAV_NAV(MAV_last_mx, mz)];

                if (MAV_dmx == -1 && !(mo->opt[MAV_DIR_XS] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
                if (MAV_dmx == +1 && !(mo->opt[MAV_DIR_XL] & MAV_CAPS_GOTO)) {
                    return FALSE;
                }
            }

            if (mx == bx && mz == bz) {
                return TRUE;
            }

            MAV_last_mx = mx;
            MAV_last_mz = mz;
        }
    }
}

UBYTE MAV_start_x;
UBYTE MAV_start_z;

UBYTE MAV_dest_x;
UBYTE MAV_dest_z;

//
// claude-ai: MAV_LOOKAHEAD=32 — the A* search explores at most 32 steps from the agent's position.
// claude-ai: If the destination is farther away, the best partial path found so far is used (MAV_MAX_OVERFLOWS=8).
// claude-ai: MAV_node[] stores the recovered path (reversed: node[0]=furthest, node[node_upto-1]=first step).
// How much look-ahead we use in the navigation.  MAV_node[0] is the end of
// the looked-ahead route and (MAV_node_upto - 1) is the start(x,z)
//

// claude-ai: MAV_LOOKAHEAD=32: A* expands at most 32 grid cells from the agent before giving up on finding a direct path.
#define MAV_LOOKAHEAD 32

MAV_Action MAV_node[MAV_LOOKAHEAD];
SLONG MAV_node_upto;

//
// claude-ai: MAV_flag: packed A* open/closed set state. Each UBYTE holds 2 cells (4 bits per cell).
// claude-ai:   bits [2:0] = action taken to reach this cell (MAV_ACTION_* 0-7)
// claude-ai:   bit  [3]   = visited flag (1 = cell has been placed in the priority queue)
// claude-ai: MAV_dir: stores which direction (MAV_DIR_*) we came from (2 bits per cell, 4 cells per UBYTE).
// claude-ai: Both arrays are indexed [z][x/N] and cleared via MAV_clear_bbox before each A* run.
// Each UBYTE is four two squares. There is three bits for action, and one bit
// to say whether we have been here or not.
//

UBYTE MAV_flag[MAP_HEIGHT][MAP_WIDTH / 2];

//
// Each UBYTE is four squares-worth of the direction
// we have come from.
//

UBYTE MAV_dir[MAP_HEIGHT][MAP_WIDTH / 4];

//
// Functions to set bits...
//

inline void MAV_visited_set(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);

    MAV_flag[z][byte] |= bit;
}

inline SLONG MAV_visited_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG bit = 8 << ((x & 0x1) << 2);

    return (MAV_flag[z][byte] & bit);
}

//
// This function also sets the visited flag.
//

inline void MAV_action_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;

    dir |= 0x08; // Set the visited flag too.

    MAV_flag[z][byte] &= ~(0x7 << shift);
    MAV_flag[z][byte] |= (dir << shift);
}

inline SLONG MAV_action_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 1;
    SLONG shift = (x & 0x1) << 2;

    return ((MAV_flag[z][byte] >> shift) & 0x7);
}

inline void MAV_dir_set(SLONG x, SLONG z, SLONG dir)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;

    MAV_dir[z][byte] &= ~(0x3 << shift);
    MAV_dir[z][byte] |= (dir << shift);
}

inline SLONG MAV_dir_get(SLONG x, SLONG z)
{
    ASSERT(WITHIN(x, 0, MAP_WIDTH - 1));
    ASSERT(WITHIN(z, 0, MAP_HEIGHT - 1));

    SLONG byte = x >> 2;
    SLONG shift = (x & 0x3) << 1;

    return ((MAV_dir[z][byte] >> shift) & 0x3);
}

//
// Clears the visited flags in given box. The
// other flags are undefined after this call.
//

void MAV_clear_bbox(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2)
{
    SLONG z;
    SLONG len;
    SLONG count;
    SLONG* zero;

    //
    // Round x1 down and x2 up to the nearest 4 byte boundary.
    //

    x2 += 0x7;
    x1 &= ~0x7;
    x2 &= ~0x7;

    SATURATE(x1, 0, MAP_WIDTH);
    SATURATE(x2, 0, MAP_WIDTH);
    SATURATE(z1, 0, MAP_HEIGHT);
    SATURATE(z2, 0, MAP_HEIGHT);

    //
    // Clear a SLONG at a time.
    //

    len = x2 - x1 >> 3;

    for (z = z1; z < z2; z++) {
        zero = (SLONG*)(&MAV_flag[z][x1 >> 1]);
        count = len;

        while (count--) {
            *zero++ = 0;
        }
    }
}

//
// claude-ai: MAV_get_first_action_from_nodelist implements PATH SMOOTHING for GOTO-only segments.
// claude-ai: It walks the nodelist from start backwards; for each GOTO waypoint it tries MAV_can_i_walk.
// claude-ai: The furthest waypoint reachable in a straight line becomes the immediate target.
// claude-ai: If a non-GOTO action (jump, climb, etc.) is encountered first, that action is returned immediately.
// claude-ai: Result: agents don't zigzag through grid cells — they walk directly toward the furthest visible point.
// Returns the first thing that should be done according to the current nodelist.
//

MAV_Action MAV_get_first_action_from_nodelist()
{
    SLONG i;

    UBYTE ax;
    UBYTE az;

    UBYTE bx;
    UBYTE bz;

    MAV_Action ans;

    //
    // Remember the last place we can walk to.
    //

    ax = MAV_start_x;
    az = MAV_start_z;

    ans.action = MAV_ACTION_GOTO;
    ans.dest_x = ax;
    ans.dest_z = az;

    for (i = MAV_node_upto - 1; i >= 0; i--) {
        if (MAV_node[i].action != MAV_ACTION_GOTO) {
            ans.action = MAV_node[i].action;
            ans.dir = MAV_node[i].dir;

            return ans;
        }

        bx = MAV_node[i].dest_x;
        bz = MAV_node[i].dest_z;

        if (MAV_can_i_walk(
                ax, az,
                bx, bz)) {
            ans.dest_x = bx;
            ans.dest_z = bz;
        } else {
            return ans;
        }
    }

    return ans;
}

//
// claude-ai: MAV_create_nodelist_from_pos: PATH RECONSTRUCTION after A* terminates.
// claude-ai: Traces back from end_x/end_z to MAV_start_x/z using stored MAV_dir and MAV_action values.
// claude-ai: Jump actions (JUMP, JUMPPULL) move 2 cells; JUMPPULL2 moves 3 cells — handled by extra back-steps.
// claude-ai: Result is stored in MAV_node[] array (reversed order: [0]=far end, [node_upto-1]=first step).
// Uses the MAV_flag and MAV_dir arrays to constuct a nodelist from
// from the given position back to (start_x,start_z).
//

void MAV_create_nodelist_from_pos(UBYTE end_x, UBYTE end_z)
{
    UBYTE x;
    UBYTE z;

    UBYTE action;
    UBYTE dir;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    x = end_x;
    z = end_z;

    MAV_node_upto = 0;

    while (
        x != MAV_start_x || z != MAV_start_z) {
        ASSERT(WITHIN(MAV_node_upto, 0, MAV_LOOKAHEAD - 1));

        dir = MAV_dir_get(x, z);
        action = MAV_action_get(x, z);

        MAV_node[MAV_node_upto].dest_x = x;
        MAV_node[MAV_node_upto].dest_z = z;
        MAV_node[MAV_node_upto].dir = dir;
        MAV_node[MAV_node_upto].action = action;

        MAV_node_upto += 1;

        ASSERT(WITHIN(dir, 0, 3));

        x -= order[dir].dx;
        z -= order[dir].dz;

        if (action == MAV_ACTION_JUMP || action == MAV_ACTION_JUMPPULL) {
            //
            // Moves you two squares.
            //

            x -= order[dir].dx;
            z -= order[dir].dz;
        }

        if (action == MAV_ACTION_JUMPPULL2) {
            //
            // Moves you three squares.
            //

            x -= order[dir].dx;
            z -= order[dir].dz;

            x -= order[dir].dx;
            z -= order[dir].dz;
        }
    }
}

//
// claude-ai: A* PRIORITY QUEUE setup. PQ_Type is the heap node: (x,z) position, score (heuristic), length (depth).
// claude-ai: PQ_HEAP_MAX_SIZE=256 — max nodes in the open set at any time.
// claude-ai: PQ_better compares by score (lower is better — pure heuristic, no g-cost, making this greedy best-first).
// claude-ai: Note: this is NOT classic A* (no accumulated path cost g). It's a greedy best-first / bounded BFS hybrid.
// The priority queue needs these definitions...
//

typedef struct
{
    UBYTE x;
    UBYTE z;
    UBYTE score; // The lower the score the better...
    UBYTE length;

} PQ_Type;

#define PQ_HEAP_MAX_SIZE 256

SLONG PQ_better(PQ_Type* a, PQ_Type* b)
{
    return a->score < b->score;
}

#include "pq.h"
#include "pq.cpp"

//
// claude-ai: MAV_score_pos: heuristic function — straight-line distance (QDIST2 = integer approximation of sqrt)
// claude-ai: from (x,z) to MAV_dest. Scaled x2 before QDIST2, capped at 255 for UBYTE storage.
// claude-ai: This is the only cost — no accumulated path cost — confirming greedy best-first search.
// Returns the score associated with the given position.
//

UBYTE MAV_score_pos(UBYTE x, UBYTE z)
{
    SLONG dx;
    SLONG dz;

    SLONG dist;

    //
    // Just return the distance to the destination.
    //

    dx = abs((MAV_dest_x - x) << 1);
    dz = abs((MAV_dest_z - z) << 1);

    dist = QDIST2(dx, dz);

    if (dist > 255) {
        dist = 255;
    }

    return dist;
}

UBYTE MAV_do_found_dest;

MAV_Action MAV_do(
    SLONG me_x,
    SLONG me_z,
    SLONG dest_x,
    SLONG dest_z,
    UBYTE caps)
{
    SLONG i;
    SLONG j;

    UBYTE opt;
    UBYTE move_one;
    UBYTE move_two;
    UBYTE move_three;
    UBYTE action;

    SLONG overflows;
    SLONG best_score;
    MAV_Action ans;

    MAV_Opt* mo;

    PQ_Type start;
    PQ_Type best;
    PQ_Type next;

    SLONG mx;
    SLONG mz;

    SLONG dx;
    SLONG dz;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    //
    // Remember the destination.
    //

    MAV_start_x = me_x;
    MAV_start_z = me_z;

    MAV_dest_x = dest_x;
    MAV_dest_z = dest_z;

    //
    // Clear the flag by default.
    //

    // claude-ai: MAV_do is the main navigation function called by PCOM AI each tick for an NPC.
    // claude-ai: Parameters: agent grid position (me_x,me_z), target (dest_x,dest_z), capability mask (caps).
    // claude-ai: caps is ANDed with each cell's opt[dir] to filter moves the agent cannot perform.
    // claude-ai: Example: MAV_CAPS_DARCI=0xff allows all moves; regular cops have fewer bits set.
    // claude-ai: Returns a MAV_Action describing: action type, direction, and destination cell.
    // claude-ai: Sets MAV_do_found_dest=TRUE if the exact destination was reached during search.
    // claude-ai: PATH FAIL case: if PQ empties without reaching destination, returns best partial-path action.
    MAV_do_found_dest = FALSE;

    //
    // Clear the flags.
    //

    // claude-ai: Clear only the ±MAV_LOOKAHEAD box around the agent (not the whole 128x128 map).
    // claude-ai: This avoids resetting the entire flag array each tick — performance optimization.
    MAV_clear_bbox(
        me_x - MAV_LOOKAHEAD,
        me_z - MAV_LOOKAHEAD,
        me_x + MAV_LOOKAHEAD,
        me_z + MAV_LOOKAHEAD);

    // memset(MAV_flag, 0, sizeof(MAV_flag));

    //
    // Initialise the heap with our start square.
    //

    PQ_init();

    start.x = me_x;
    start.z = me_z;
    start.score = MAV_score_pos(me_x, me_z);
    start.length = 0;

    PQ_add(start);

    MAV_visited_set(me_x, me_z);

//
// Initialise the score and answer.
//

// claude-ai: MAV_MAX_OVERFLOWS=8: when the frontier reaches depth MAV_LOOKAHEAD, it counts as an "overflow".
// claude-ai: After 8 overflows the search stops and returns the best partial answer found so far (PATH_FAIL equivalent).
// claude-ai: This bounds the total work to O(LOOKAHEAD * MAX_OVERFLOWS) expansions per tick.
#define MAV_MAX_OVERFLOWS 8

    overflows = 0;
    best_score = INFINITY;
    ans.action = MAV_ACTION_GOTO;
    ans.dir = 0;
    ans.dest_x = me_x;
    ans.dest_z = me_z;

    while (1) {
        if (PQ_empty()) {
            break;
        }

        //
        // Get the best square so far and move it on a bit.
        //

        best = PQ_best();
        PQ_remove();

        if (best.length >= MAV_LOOKAHEAD) {
            if (best.score < best_score) {
                best_score = best.score;

                //
                // Work out the first action and put it in answer.
                //

                MAV_create_nodelist_from_pos(best.x, best.z);
                ans = MAV_get_first_action_from_nodelist();
            }

            overflows += 1;

            if (overflows >= MAV_MAX_OVERFLOWS) {
                //
                // Dont do any more calculation.
                //

                return ans;
            }

            continue;
        }

        if (best.x == MAV_dest_x && best.z == MAV_dest_z) {
            //
            // Found the destination.
            //

            MAV_do_found_dest = TRUE;

            //
            // Work out the first action and return it as the answer.
            //

            MAV_create_nodelist_from_pos(best.x, best.z);
            ans = MAV_get_first_action_from_nodelist();

            return ans;
        }

        ASSERT(WITHIN(best.x, 0, MAP_WIDTH - 1));
        ASSERT(WITHIN(best.z, 0, MAP_HEIGHT - 1));

        ASSERT(WITHIN(MAV_NAV(best.x, best.z), 0, MAV_opt_upto - 1));

        mo = &MAV_opt[MAV_NAV(best.x, best.z)];

        //
        // claude-ai: NEIGHBOUR EXPANSION — A* open-set growth:
        // claude-ai:   opt = cell's movement options ANDed with agent's caps bitmask.
        // claude-ai:   move_one:   MAV_CAPS_GOTO|PULLUP|CLIMB_OVER|FALL_OFF|LADDER_UP — move 1 cell.
        // claude-ai:   move_two:   MAV_CAPS_JUMP|JUMPPULL — move 2 cells (skip intermediate).
        // claude-ai:   move_three: MAV_CAPS_JUMPPULL2 — move 3 cells.
        // claude-ai: Non-GOTO actions add a score penalty (bias = Random()&3 + 3) to prefer normal walking.
        // claude-ai: FALL_OFF gets no extra penalty (falling is "free").
        // claude-ai: Only unvisited cells are added (MAV_visited_get check).
        // Add neighbouring squares to the priority queue.
        //

        for (i = 0; i < 4; i++) {
            opt = mo->opt[i] & caps;

            //
            // Moving one square.
            //

            move_one = opt & (MAV_CAPS_GOTO | MAV_CAPS_PULLUP | MAV_CAPS_CLIMB_OVER | MAV_CAPS_FALL_OFF | MAV_CAPS_LADDER_UP);
            move_two = opt & (MAV_CAPS_JUMP | MAV_CAPS_JUMPPULL);
            move_three = opt & (MAV_CAPS_JUMPPULL2);

            dx = order[i].dx;
            dz = order[i].dz;

            if (move_one) {
                mx = best.x + dx;
                mz = best.z + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    //
                    // What action did we use to get here?
                    //

                    if (opt & MAV_CAPS_GOTO) {
                        action = MAV_ACTION_GOTO;
                    } else if (opt & MAV_CAPS_PULLUP) {
                        action = MAV_ACTION_PULLUP;
                    } else if (opt & MAV_CAPS_CLIMB_OVER) {
                        action = MAV_ACTION_CLIMB_OVER;
                    } else if (opt & MAV_CAPS_FALL_OFF) {
                        action = MAV_ACTION_FALL_OFF;
                    } else if (opt & MAV_CAPS_LADDER_UP) {
                        action = MAV_ACTION_LADDER_UP;
                    } else
                        ASSERT(0);

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    if (action != MAV_ACTION_GOTO) {
                        //
                        // Bias against using strange methods of travel!
                        //

                        next.score += Random() & 0x3;

                        if (action == MAV_ACTION_FALL_OFF) {
                            //
                            // Falling off isn't so bad...
                            //
                        } else {
                            next.score += 3;
                        }
                    }

                    //
                    // Add this square to the priority queue.
                    //

                    PQ_add(next);
                }
            }

            if (move_two) {
                mx = best.x + dx + dx;
                mz = best.z + dz + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    //
                    // What action did we use to get here?
                    //

                    if (opt & MAV_CAPS_JUMP) {
                        action = MAV_ACTION_JUMP;
                    } else if (opt & MAV_CAPS_JUMPPULL) {
                        action = MAV_ACTION_JUMPPULL;
                    } else
                        ASSERT(0);

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    //
                    // Add this square to the priority queue.
                    //

                    PQ_add(next);
                }
            }

            if (move_three) {
                mx = best.x + dx + dx + dx;
                mz = best.z + dz + dz + dz;

                if (!MAV_visited_get(mx, mz)) {
                    next.x = mx;
                    next.z = mz;
                    next.length = best.length + 1;
                    next.score = MAV_score_pos(mx, mz);

                    //
                    // What action did we use to get here?
                    //

                    action = MAV_ACTION_JUMPPULL2;

                    //
                    // How we reached this square.
                    //

                    MAV_action_set( // Sets the visited flag aswell.
                        mx,
                        mz,
                        action);

                    MAV_dir_set(
                        mx,
                        mz,
                        i);

                    //
                    // Add this square to the priority queue.
                    //

                    PQ_add(next);
                }
            }
        }
    }

    return ans;
}

// claude-ai: MAV_inside: returns TRUE if world-space point (x,y,z) is below the MAVHEIGHT surface.
// claude-ai: Used by MAV_height_los_fast/slow to test whether a line-of-sight ray goes underground.
// claude-ai: x/z are in game units (>>8 to get grid cell), y is in game units (>>6 to get MAVHEIGHT units).
SLONG MAV_inside(
    SLONG x,
    SLONG y,
    SLONG z)
{
    x >>= 8;
    y >>= 6;
    z >>= 8;

    if (WITHIN(x, 0, MAP_WIDTH - 1) && WITHIN(z, 0, MAP_HEIGHT - 1)) {
        if (y < -127) {
            return TRUE;
        }
        if (y > +127) {
            return FALSE;
        }

        if (y < MAVHEIGHT(x, z)) {
            return TRUE;
        }
    }

    return FALSE;
}

SLONG MAV_height_los_fail_x;
SLONG MAV_height_los_fail_y;
SLONG MAV_height_los_fail_z;

// claude-ai: MAV_height_los_fast: fast terrain LOS using the MAVHEIGHT grid (not full geometry).
// claude-ai: Steps along the ray in increments of ~1 cell (dist>>8 steps). Returns FALSE if any step is underground.
// claude-ai: Records the last valid point before failure in MAV_height_los_fail_*.
// claude-ai: Used by AI for visibility/sighting checks over the height map rather than full collision geometry.
SLONG MAV_height_los_fast(
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG dist = QDIST2(abs(dx), abs(dz));
    SLONG steps = (dist >> 8) + 1;

    SLONG x = x1;
    SLONG y = y1;
    SLONG z = z1;

    dx /= steps;
    dy /= steps;
    dz /= steps;

    while (steps-- >= 0) {
        if (MAV_inside(x, y, z)) {
            MAV_height_los_fail_x = x - (dx >> 0);
            MAV_height_los_fail_y = y - (dy >> 0);
            MAV_height_los_fail_z = z - (dz >> 0);

            return FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return TRUE;
}

// claude-ai: MAV_height_los_slow: like MAV_height_los_fast but optionally uses WARE_inside() for warehouse interiors.
// claude-ai: When ware!=0, tests each step against the warehouse bounding volume instead of global MAVHEIGHT.
// claude-ai: Starts from (x1+dx, y1+dy, z1+dz) (skips the source point to avoid self-occlusion).
SLONG MAV_height_los_slow(
    SLONG ware,
    SLONG x1, SLONG y1, SLONG z1,
    SLONG x2, SLONG y2, SLONG z2)
{
    SLONG dx = x2 - x1;
    SLONG dy = y2 - y1;
    SLONG dz = z2 - z1;

    SLONG dist = QDIST2(abs(dx), abs(dz));
    SLONG steps = (dist >> 8) + 1;

    dx /= steps;
    dy /= steps;
    dz /= steps;

    SLONG x = x1 + dx;
    SLONG y = y1 + dy;
    SLONG z = z1 + dz;

    while (steps-- > 0) {
        SLONG inside;

        if (ware) {
            inside = WARE_inside(ware, x, y, z);
        } else {
            inside = MAV_inside(x, y, z);
        }

        if (inside) {
            MAV_height_los_fail_x = x;
            MAV_height_los_fail_y = y;
            MAV_height_los_fail_z = z;

            return FALSE;
        }

        x += dx;
        y += dy;
        z += dz;
    }

    return TRUE;
}

// claude-ai: MAV_precalculate_warehouse_nav: builds a SEPARATE nav grid for a single warehouse interior.
// claude-ai: Warehouses are large indoor areas with PAP_FLAG_HIDDEN set on their cells.
// claude-ai: The function temporarily redirects MAV_nav to WARE_nav[ww->nav] with the warehouse's own pitch.
// claude-ai: Only cells inside the warehouse bounding box (ww->minx..maxx, ww->minz..maxz) are processed.
// claude-ai: Navigation rules are slightly different: no "both_ground" check; max walk dh=1 always.
// claude-ai: After precalculation, MAV_nav is restored to the global city grid pointer.
// claude-ai: Port note: warehouse nav grids must be stored separately from the main city grid.
void MAV_precalculate_warehouse_nav(UBYTE ware)
{
    SLONG i;

    SLONG x;
    SLONG y;
    SLONG z;

    SLONG x1;
    SLONG y1;
    SLONG z1;

    SLONG x2;
    SLONG y2;
    SLONG z2;

    SLONG dx;
    SLONG dz;

    SLONG mx;
    SLONG mz;

    SLONG tx;
    SLONG tz;

    SLONG rx;
    SLONG rz;

    SLONG dh;

    SLONG useangle;
    SLONG matrix[4];
    SLONG ladder;

    SLONG sin_yaw;
    SLONG cos_yaw;

    SLONG both_ground;

    struct
    {
        SLONG dx;
        SLONG dz;

    } order[4] = {
        { -1, 0 },
        { +1, 0 },
        { 0, -1 },
        { 0, +1 }
    };

    UBYTE opt[4];

    WARE_Ware* ww = &WARE_ware[ware];

    //
    // Remember the old MAV_nav array.
    //

    UWORD* old_mav_nav = MAV_nav;
    SLONG old_mav_nav_pitch = MAV_nav_pitch;

    //
    // Set the MAV_nav array to point to the warehouse's private array.
    //

    MAV_nav = &WARE_nav[ww->nav];
    MAV_nav_pitch = ww->nav_pitch;

    //
    // Make the staircase prims change the MAV_height array
    //

    OB_Info* oi;

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    //
                    // Find which mapsquare the middle of this prim is over.
                    //

                    PrimInfo* pi = get_prim_info(oi->prim);

                    SLONG mx = pi->minx + pi->maxx >> 1;
                    SLONG mz = pi->minz + pi->maxz >> 1;

                    SLONG matrix[4];
                    SLONG useangle;

                    SLONG sin_yaw;
                    SLONG cos_yaw;

                    SLONG rx;
                    SLONG rz;

                    SLONG sx;
                    SLONG sz;

                    useangle = -oi->yaw;
                    useangle &= 2047;

                    sin_yaw = SIN(useangle);
                    cos_yaw = COS(useangle);

                    matrix[0] = cos_yaw;
                    matrix[1] = -sin_yaw;
                    matrix[2] = sin_yaw;
                    matrix[3] = cos_yaw;

                    rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                    rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                    rx += oi->x;
                    rz += oi->z;

                    rx >>= 8;
                    rz >>= 8;

                    if (WITHIN(rx, ww->minx, ww->maxx) && WITHIN(rz, ww->minz, ww->maxz)) {
                        MAVHEIGHT(rx, rz) = oi->y + 0x40 >> 6;
                    }
                }
            }
        }

    //
    // Work out the mav for each square in the bounding box of the warehouse.
    //

    for (x = ww->minx; x <= ww->maxx; x++)
        for (z = ww->minz; z <= ww->maxz; z++) {
            mx = x - ww->minx;
            mz = z - ww->minz;

            //
            // Look for a nearby ladder.
            //

            ladder = find_nearby_ladder_colvect_radius(
                (x << 8) + 0x80,
                (z << 8) + 0x80,
                0x100);

            for (i = 0; i < 4; i++) {
                opt[i] = 0;

                dx = order[i].dx;
                dz = order[i].dz;

                tx = x + dx;
                tz = z + dz;

                if (!(PAP_2HI(x, z).Flags & PAP_FLAG_HIDDEN)) {
                    //
                    // This square is outside the warehouse.
                    //

                    continue;
                }

                if (!WITHIN(tx, ww->minx, ww->maxx) || !WITHIN(tz, ww->minz, ww->maxz)) {
                    //
                    // Cannot navigate in this direction because it is
                    // outside the warehouse.
                    //

                    continue;
                }

                if (!(PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                    //
                    // This square is outside the warehouse.
                    //

                    continue;
                }

                //
                // Can we walk from (x,z) to (tx,tz)?
                //

                dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                if (abs(dh) > 1) {
                    //
                    // There are at different heights, so you cant
                    // just walk between the two squares.
                    //

                    if (dh < 0) {
                        //
                        // There might be a wall or fence in the way.
                        //

                        x1 = (x << 8) + 0x80;
                        z1 = (z << 8) + 0x80;
                        x2 = (tx << 8) + 0x80;
                        z2 = (tz << 8) + 0x80;

                        y1 = (MAVHEIGHT(x, z) << 6) + 0x50;
                        y2 = (MAVHEIGHT(tx, tz) << 6) + 0x50;

                        y = MAX(y1, y2);

                        if (there_is_a_los(
                                x1, y, z1,
                                x2, y, z2,
                                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                            //
                            // We can always fall down becuase there is nothing in the way.
                            //

                            opt[i] |= MAV_CAPS_FALL_OFF;
                        } else {
                            //
                            // If there is a fence in the way, then we can scale the fence.
                            //

                            DFacet* df = &dfacets[los_failure_dfacet];

                            if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                                if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                    //
                                    // Unclimbable fence.
                                    //
                                } else {
                                    //
                                    // We can scale this fence.
                                    //

                                    opt[i] |= MAV_CAPS_CLIMB_OVER;
                                }
                            }
                        }
                    } else {
                        if (WITHIN(dh, 3, 5)) {
                            //
                            // We can pull ourselves up.
                            //

                            opt[i] |= MAV_CAPS_PULLUP;
                        }

                        if (ladder) {
                            DFacet* df_ladder;

                            ASSERT(WITHIN(ladder, 1, next_dfacet - 1));

                            df_ladder = &dfacets[ladder];

                            ASSERT(df_ladder->FacetType == STOREY_TYPE_LADDER);

                            //
                            // There is a ladder- can we climb up it in this direction?
                            //

                            x1 = (x << 8) + 0x80;
                            z1 = (z << 8) + 0x80;
                            x2 = (tx << 8) + 0x80;
                            z2 = (tz << 8) + 0x80;

                            if (two4_line_intersection(
                                    x1, z1,
                                    x2, z2,
                                    df_ladder->x[0] << 8, df_ladder->z[0] << 8,
                                    df_ladder->x[1] << 8, df_ladder->z[1] << 8)) {
                                opt[i] |= MAV_CAPS_LADDER_UP;
                            }
                        }
                    }
                } else {
                    //
                    // There might be a wall or fence in the way.
                    //

                    x1 = (x << 8) + 0x80;
                    z1 = (z << 8) + 0x80;
                    x2 = (tx << 8) + 0x80;
                    z2 = (tz << 8) + 0x80;

                    y1 = (MAVHEIGHT(x, z) << 6) + 0x50;
                    y2 = (MAVHEIGHT(tx, tz) << 6) + 0x50;

                    y = MAX(y1, y2);

                    if (there_is_a_los(
                            x1, y, z1,
                            x2, y, z2,
                            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                        //
                        // Nothing in the way...
                        //

                        opt[i] |= MAV_CAPS_GOTO;
                    } else {
                        //
                        // If there is a fence in the way, then we can scale the fence.
                        //

                        DFacet* df = &dfacets[los_failure_dfacet];

                        if (df->FacetType == STOREY_TYPE_FENCE_FLAT) {
                            if (df->FacetFlags & FACET_FLAG_UNCLIMBABLE) {
                                //
                                // Unclimbable fence.
                                //
                            } else {
                                //
                                // We can scale this fence.
                                //

                                opt[i] |= MAV_CAPS_CLIMB_OVER;
                            }
                        }
                    }
                }

                // claude-ai: If no direct walk or fence-climb is possible, try jump moves.
                // claude-ai: Jump checks 2 cells ahead (tx+dx) and then 3 cells ahead (tx+dx+dx).
                // claude-ai: JUMP: 2 cells, dh<2. JUMPPULL: 2 cells, dh in [2,5]. JUMPPULL2: 3 cells, dh in (-6,4].
                // claude-ai: Each requires LOS at jump height: MAVHEIGHT<<6 + 0xa0 (approx head+shoulder height).
                if (!(opt[i] & MAV_CAPS_GOTO) && !(opt[i] & MAV_CAPS_CLIMB_OVER)) {
                    //
                    // Now what about jumping one block?
                    //

                    tx += dx;
                    tz += dz;

                    if (WITHIN(tx, ww->minx, ww->maxx) && WITHIN(tz, ww->minz, ww->maxz) && (PAP_2HI(tx, tz).Flags & PAP_FLAG_HIDDEN)) {
                        dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                        //
                        // Can we jump there?
                        //

                        x1 = (x << 8) + 0x80;
                        z1 = (z << 8) + 0x80;
                        x2 = (tx << 8) + 0x80;
                        z2 = (tz << 8) + 0x80;

                        y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                        y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                        if (there_is_a_los(
                                x1, y1, z1,
                                x2, y2, z2,
                                LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                            if (dh < 2) {
                                opt[i] |= MAV_CAPS_JUMP;
                            } else {
                                if (WITHIN(dh, 2, 5)) {
                                    opt[i] |= MAV_CAPS_JUMPPULL;
                                }
                            }
                        }

                        //
                        // What about jumping two blocks?
                        //

                        tx += dx;
                        tz += dz;

                        if (WITHIN(tx, 0, MAP_WIDTH - 1) && WITHIN(tz, 0, MAP_HEIGHT - 1)) {
                            dh = MAVHEIGHT(tx, tz) - MAVHEIGHT(x, z);

                            if (dh > 4) {
                                //
                                // Can't make this jump
                                //
                            } else {
                                if (dh > -6) {
                                    //
                                    // Can we jump there?
                                    //

                                    x1 = (x << 8) + 0x80;
                                    z1 = (z << 8) + 0x80;
                                    x2 = (tx << 8) + 0x80;
                                    z2 = (tz << 8) + 0x80;

                                    y1 = (MAVHEIGHT(x, z) << 6) + 0xa0;
                                    y2 = (MAVHEIGHT(tx, tz) << 6) + 0xa0;

                                    if (there_is_a_los(
                                            x1, y1, z1,
                                            x2, y2, z2,
                                            LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG)) {
                                        opt[i] |= MAV_CAPS_JUMPPULL2;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            StoreMavOpts(mx, mz, opt);
        }

    //
    // A hack for the staircase prim.
    //

    for (x = 0; x < PAP_SIZE_LO; x++)
        for (z = 0; z < PAP_SIZE_LO; z++) {
            for (oi = OB_find(x, z); oi->prim; oi++) {
                if (oi->prim == 41) {
                    //
                    // The step prim!
                    //

                    {
                        //
                        // Find which mapsquare the middle of this prim is over.
                        //

                        PrimInfo* pi = get_prim_info(oi->prim);

                        SLONG mx = pi->minx + pi->maxx >> 1;
                        SLONG mz = pi->minz + pi->maxz >> 1;

                        SLONG matrix[4];
                        SLONG useangle;

                        SLONG sin_yaw;
                        SLONG cos_yaw;

                        SLONG rx;
                        SLONG rz;

                        SLONG sx;
                        SLONG sz;

                        useangle = -oi->yaw;
                        useangle &= 2047;

                        sin_yaw = SIN(useangle);
                        cos_yaw = COS(useangle);

                        matrix[0] = cos_yaw;
                        matrix[1] = -sin_yaw;
                        matrix[2] = sin_yaw;
                        matrix[3] = cos_yaw;

                        rx = MUL64(mx, matrix[0]) + MUL64(mz, matrix[1]);
                        rz = MUL64(mx, matrix[2]) + MUL64(mz, matrix[3]);

                        rx += oi->x;
                        rz += oi->z;

                        rx >>= 8;
                        rz >>= 8;

                        if (WITHIN(rx, ww->minx, ww->maxx) && WITHIN(rz, ww->minz, ww->maxz)) {
                            mx = rx - ww->minx;
                            mz = rz - ww->minz;

                            if (oi->yaw < 256 || oi->yaw > 1792 || WITHIN(oi->yaw, 768, 1280)) {
                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx - 1, rz)) {
                                    //
                                    // Walking left-right on a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_XS);
                                    MAV_turn_movement_off(mx - 1, mz, MAV_DIR_XL);
                                }

                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx + 1, rz)) {
                                    //
                                    // Walking left-right on a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_XL);
                                    MAV_turn_movement_off(mx + 1, mz, MAV_DIR_XS);
                                }

                                if (!WITHIN(oi->yaw, 768, 1280)) {
                                    if (MAVHEIGHT(rx, rz + 1) <= MAVHEIGHT(rx, rz) + 3) {
                                        MAV_turn_movement_on(mx, mz, MAV_DIR_ZL);
                                        MAV_turn_movement_on(mx, mz + 1, MAV_DIR_ZS);
                                    }
                                } else {
                                    if (MAVHEIGHT(rx, rz - 1) <= MAVHEIGHT(rx, rz) + 3) {
                                        MAV_turn_movement_on(mx, mz, MAV_DIR_ZS);
                                        MAV_turn_movement_on(mx, mz - 1, MAV_DIR_ZL);
                                    }
                                }
                            } else {
                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz - 1)) {
                                    //
                                    // Walking across a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_ZS);
                                    MAV_turn_movement_off(mx, mz - 1, MAV_DIR_ZL);
                                }

                                if (MAVHEIGHT(rx, rz) == MAVHEIGHT(rx, rz + 1)) {
                                    //
                                    // Walking across a wide staircase.
                                    //
                                } else {
                                    MAV_turn_movement_off(mx, mz, MAV_DIR_ZL);
                                    MAV_turn_movement_off(mx, mz + 1, MAV_DIR_ZS);
                                }

                                if (!WITHIN(oi->yaw, 256, 768)) {
                                    if (MAVHEIGHT(rx - 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                        MAV_turn_movement_on(mx, mz, MAV_DIR_XS);
                                        MAV_turn_movement_on(mx - 1, mz, MAV_DIR_XL);
                                    }
                                } else {
                                    if (MAVHEIGHT(rx + 1, rz) <= MAVHEIGHT(rx, rz) + 3) {
                                        MAV_turn_movement_on(mx, mz, MAV_DIR_XL);
                                        MAV_turn_movement_on(mx + 1, mz, MAV_DIR_XS);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    //
    // Restore the old MAV_nav array.
    //

    MAV_nav = old_mav_nav;
    MAV_nav_pitch = old_mav_nav_pitch;
}

// claude-ai: MAV_get_caps: reads the raw capability bitmask for one direction from a given cell.
// claude-ai: Returns 0 if out of bounds. Used by external code to query what moves are possible from a cell.
// claude-ai: Works on whatever MAV_nav is currently active (city grid or warehouse grid).
UBYTE MAV_get_caps(
    UBYTE x,
    UBYTE z,
    UBYTE dir)
{
    UBYTE ans;

    MAV_Opt* mo;

    if (WITHIN(x, 0, MAV_nav_pitch - 1) && WITHIN(z, 0, MAV_nav_pitch - 1)) {
        mo = &MAV_opt[MAV_NAV(x, z)];

        ASSERT(WITHIN(dir, 0, 3));

        ans = mo->opt[dir];

        return ans;
    }

    return 0;
}

// claude-ai: MAV_turn_car_movement_on/off: runtime helpers to toggle car-nav edges (e.g. when a car blocks a road).
// claude-ai: Operate on MAV_CAR bits in the packed MAV_nav UWORD array — separate from foot-nav MAV_opt data.
void MAV_turn_car_movement_on(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav |= 1 << dir;

    SET_MAV_CAR(mx, mz, mav);
}

void MAV_turn_car_movement_off(UBYTE mx, UBYTE mz, UBYTE dir)
{
    UBYTE mav;

    ASSERT(WITHIN(mx, 0, PAP_SIZE_HI - 1));
    ASSERT(WITHIN(mz, 0, PAP_SIZE_HI - 1));

    mav = MAV_CAR(mx, mz);
    mav &= ~(1 << dir);

    SET_MAV_CAR(mx, mz, mav);
}
