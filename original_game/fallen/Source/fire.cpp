//
// Fire!
//

// claude-ai: FIRE SYSTEM OVERVIEW
// claude-ai: Implements a purely visual fire effect — not physics-based spreading fire.
// claude-ai: Fire is modelled as a pool of FIRE_Fire structures (max 8 simultaneous fires).
// claude-ai: Each FIRE_Fire owns a linked list of FIRE_Flame nodes drawn from a shared pool (max 256 flames total).
// claude-ai: A fire shrinks over time (ff->size decreases every 4 ticks by ff->shrink).
// claude-ai: As size decreases the required flame count drops; flames that exceed the target die naturally.
// claude-ai: This system is SEPARATE from the particle system (psystem.cpp) — it has its own structures and renderer.
// claude-ai: No damage / spreading logic here; damage is handled in psystem.cpp via PFLAG_HURTPEOPLE.
// claude-ai: Port notes: keep the pool concept (8 fires / 256 flames is fine), but drive rendering through
// claude-ai:   the new particle/sprite system instead of the bespoke FIRE_get_* iterator API.

#include "game.h"
#include "fire.h"


//
// Each flame.
//

// claude-ai: Each individual flame wiggles its control points each tick using alternating +/-31/33 offsets.
// claude-ai: fl->die is a random lifetime (32..63 ticks); fl->counter counts up until death.
// claude-ai: fl->height drives how many control points (fl->points, clamped 2..4) the flame sprite has.
// claude-ai: Points with higher (fewer) dist from center get more control points → taller, more complex flames.
#define FIRE_MAX_FLAME_POINTS 4

typedef struct
{
	SBYTE dx;        // claude-ai: horizontal offset from fire center (world units, signed byte)
	SBYTE dz;        // claude-ai: depth offset from fire center (world units, signed byte)
	UBYTE die;       // claude-ai: lifetime limit — flame dies when counter >= die (range 32..63)
	UBYTE counter;   // claude-ai: age counter, incremented every tick
	UBYTE height;    // claude-ai: visual height = 255 - (|dx| + |dz|); center flames are tallest
	UBYTE next;      // claude-ai: intrusive linked-list next index (0 = end of list)
	UBYTE points;    // claude-ai: number of animated control points (2..4, derived from height)
	UBYTE shit;      // claude-ai: padding / unused — original developer comment confirms this

	UBYTE angle [FIRE_MAX_FLAME_POINTS]; // claude-ai: per-control-point animated angle (wraps 0..255)
	UBYTE offset[FIRE_MAX_FLAME_POINTS]; // claude-ai: per-control-point animated radial offset

} FIRE_Flame;

// claude-ai: Global flame pool — index 0 is never used (sentinel); free list starts at index 1.
// claude-ai: FIRE_flame_free holds the head of the singly-linked free list via FIRE_Flame::next.
#define FIRE_MAX_FLAMES 256

FIRE_Flame FIRE_flame[FIRE_MAX_FLAMES];
SLONG      FIRE_flame_free;

//
// Fire is a linked list of flames.
//

// claude-ai: FIRE_Fire is the top-level fire object. There can be at most FIRE_MAX_FIRE (8) simultaneous fires.
// claude-ai: ff->num == 0 means the slot is free. ff->next is the head of this fire's flame linked list.
// claude-ai: ff->size controls desired flame count (via FIRE_num_flames_for_size = size >> 2).
// claude-ai: ff->shrink is the per-4-tick size reduction — effectively the burn-out rate.
// claude-ai: Position (x, y, z) is the world anchor; flames are offset by their own dx/dz within ±127 units.
typedef struct
{
	UBYTE num;		// The number of flames this fire has, 0 => the fire is unused.
	UBYTE next;		// The flames.
	UBYTE size;     // claude-ai: current intensity — decreases over time; controls target flame count
	UBYTE shrink;   // claude-ai: burn-out speed (subtracted from size every 4 ticks); acts as fire lifetime
	UWORD x;        // claude-ai: world X position (fixed-point)
	SWORD y;        // claude-ai: world Y position (signed — fires can be at any height)
	UWORD z;        // claude-ai: world Z position (fixed-point)

} FIRE_Fire;

// claude-ai: Only 8 fires can exist simultaneously — hard limit. Allocation uses round-robin search.
#define FIRE_MAX_FIRE 8

FIRE_Fire FIRE_fire[FIRE_MAX_FIRE];
SLONG     FIRE_fire_last; // claude-ai: last allocated fire index; round-robin search starts here


// claude-ai: FIRE_init — called once at game start / level load.
// claude-ai: Builds the flame free-list (indices 1..255 chained via FIRE_Flame::next, tail → 0).
// claude-ai: Marks all FIRE_Fire slots as unused (num = 0).
void FIRE_init()
{
	SLONG i;

	//
	// Initialise the linked list of flames.
	//

	FIRE_flame_free = 1;

	for (i = 1; i < FIRE_MAX_FLAMES; i++)
	{
		FIRE_flame[i].next = i + 1;
	}

	FIRE_flame[FIRE_MAX_FLAMES - 1].next = 0;

	//
	// Make all the fire as unused.
	//

	FIRE_fire_last = 0;

	for (i = 0; i < FIRE_MAX_FIRE; i++)
	{
		FIRE_fire[i].num = 0;
	}
}


//
// Returns how many flames a fire of the given size should have.
//

// claude-ai: Simple linear mapping: size 0→0 flames, size 4→1, size 8→2 … size 255→63 flames.
// claude-ai: Maximum theoretical flames per fire = 63 (but globally capped by FIRE_MAX_FLAMES = 256).
UBYTE FIRE_num_flames_for_size(UBYTE size)
{
	return size >> 2;
}


//
// Adds a flame to the given fire structure.
//

// claude-ai: Allocates one flame from the global free-list and prepends it to the given fire's flame list.
// claude-ai: Flame placement: random dx/dz in roughly [-31..224] with Manhattan distance summed.
// claude-ai:   Flames at the center (small dist) get height close to 255 → more control points → taller.
// claude-ai:   Flames at the periphery (large dist) get low height → fewer points → shorter, quicker.
// claude-ai: Lifetime randomised: 32..63 ticks (fl->die).
// claude-ai: Control points get random initial angle/offset that the process loop animates each tick.
void FIRE_add_flame(FIRE_Fire *ff)
{
	SLONG i;

	SLONG dx;
	SLONG dz;
	SLONG dist;
	UBYTE flame;

	ASSERT(WITHIN(ff, &FIRE_fire[0], &FIRE_fire[FIRE_MAX_FIRE - 1]));

	FIRE_Flame *fl;

	if (FIRE_flame_free == NULL)
	{
		//
		// No spare flame structure.
		//

		return;
	}

	ASSERT(WITHIN(FIRE_flame_free, 1, FIRE_MAX_FLAMES - 1));

	// claude-ai: Pop flame from free-list head
	flame           =  FIRE_flame_free;
	fl              = &FIRE_flame[FIRE_flame_free];
	FIRE_flame_free =  fl->next;

	dx   = (Random() & 0xff) - 0x1f; // claude-ai: range ≈ -31..224; biased toward positive (right/back)
	dz   = (Random() & 0xff) - 0x1f;
	dist = abs(dx) + abs(dz);

	fl->dx      = dx;
	fl->dz      = dz;
	fl->height  = 255 - dist; // claude-ai: closer to center → taller flame
	fl->counter = 0;
	fl->die     = 32 + (Random() & 0x1f); // claude-ai: random lifetime 32..63 ticks
	fl->points  = (fl->height >> 6) + 1;  // claude-ai: height 0..63→1pt, 64..127→2pt, 128..191→3pt, 192..255→4pt

	SATURATE(fl->points, 2, FIRE_MAX_FLAME_POINTS); // claude-ai: minimum 2 control points always

	for (i = 0; i < fl->points; i++)
	{
		fl->angle [i] = Random(); // claude-ai: starting angle for each wiggle point (0..255 wraps)
		fl->offset[i] = Random(); // claude-ai: starting radial offset for each wiggle point
	}

	//
	// Add this flame to the fire structure.
	//

	// claude-ai: Prepend to fire's flame linked-list (ff->next = head)
	fl->next = ff->next;
	ff->next = flame;
	ff->num += 1;
}


// claude-ai: FIRE_create — public API to spawn a new fire at a world position.
// claude-ai:   size  = initial intensity (drives flame count via size>>2)
// claude-ai:   life  = stored as ff->shrink = burn-out rate (subtracted from size every 4 ticks)
// claude-ai: Allocation uses round-robin over 8 slots. If all 8 are active, silently drops the request.
// claude-ai: The very first FIRE_add_flame call marks the slot as used (ff->num becomes 1).
void FIRE_create(
		UWORD x,
		SWORD y,
		UWORD z,
		UBYTE size,
		UBYTE life)
{
	SLONG i;

	FIRE_Fire *ff;

	//
	// Look for a spare fire structure.
	//

	// claude-ai: Round-robin search — starts from last allocated slot to spread allocation evenly
	for (i = 0; i < FIRE_MAX_FIRE; i++)
	{
		FIRE_fire_last += 1;

		if (FIRE_fire_last >= FIRE_MAX_FIRE)
		{
			FIRE_fire_last = 0;
		}

		if (!FIRE_fire[FIRE_fire_last].num)
		{
			goto found_spare_fire_structure;
		}
	}

	//
	// No spare fire structures!
	//

	return;

  found_spare_fire_structure:;

	ff = &FIRE_fire[FIRE_fire_last];

	ff->x      = x;
	ff->y      = y;
	ff->z      = z;
	ff->size   = size;
	ff->shrink = life; // claude-ai: confusingly named 'life' in the API but stored as 'shrink' (burn rate)
	ff->num    = 0;

	//
	// Add a flame to this fire structure to mark it as used.
	//

	// claude-ai: Seeding the first flame also sets ff->num = 1, making the slot appear occupied
	FIRE_add_flame(ff);
}



// claude-ai: FIRE_process — called every game tick. Three phases per active fire:
// claude-ai:   1. ANIMATE: walk all flames, increment counter, wiggle control points.
// claude-ai:      Odd-indexed points: angle += 31, offset -= 17 (clockwise drift)
// claude-ai:      Even-indexed points: angle -= 33, offset += 21 (counter-clockwise drift)
// claude-ai:      Using different prime-ish deltas avoids visible periodicity.
// claude-ai:   2. REAP: remove flames whose counter >= die, push them back onto the free-list.
// claude-ai:   3. SHRINK + REPLENISH: every 4th tick, reduce size by shrink amount (fire burns down).
// claude-ai:      If current flame count < target (size>>2), spawn one new flame to maintain density.
// claude-ai:      When size hits 0 and all flames die, ff->num drops to 0 → slot becomes free.
void FIRE_process()
{
	SLONG  i;
	SLONG  j;
	UBYTE  flame;
	UBYTE  next;
	UBYTE *prev;

	FIRE_Fire  *ff;
	FIRE_Flame *fl;

	for (i = 0; i < FIRE_MAX_FIRE; i++)
	{
		ff = &FIRE_fire[i];

		if (ff->num)
		{
			//
			// Process all the nexts of this fire.
			//

			// claude-ai: Phase 1 — animate control points for visual wiggle
			for (next = ff->next; next; next = fl->next)
			{
				ASSERT(WITHIN(next, 1, FIRE_MAX_FLAMES - 1));

				fl = &FIRE_flame[next];

				fl->counter += 1;

				// claude-ai: Skip point [0] (base anchor), animate [1..points-1] only
				for (j = 1; j < fl->points; j++)
				{
					if (j & 1)
					{
						// claude-ai: Odd points spin one way with offset contracting
						fl->angle [j] += 31;
						fl->offset[j] -= 17;
					}
					else
					{
						// claude-ai: Even points spin the other way with offset expanding
						fl->angle [j] -= 33;
						fl->offset[j] += 21;
					}
				}
			}

			//
			// Delete all the dead flames of this fire.
			//

			// claude-ai: Phase 2 — reap expired flames; classic linked-list deletion with prev pointer
			prev = &ff->next;
			next =  ff->next;

			while(next)
			{
				ASSERT(WITHIN(next, 1, FIRE_MAX_FLAMES - 1));

				fl = &FIRE_flame[next];

				if (fl->counter >= fl->die)
				{
					//
					// Kill this flame.
					//

					flame = next;

				   *prev  = fl->next;  // claude-ai: unlink from fire's list
					next  = fl->next;

					//
					// Put it in the free list.
					//

					// claude-ai: Prepend to global free-list
					fl->next        = FIRE_flame_free;
					FIRE_flame_free = flame;

					ff->num -= 1;
				}
				else
				{
					//
					// Keep this flame.
					//

					prev = &fl->next;
					next =  fl->next;
				}
			}

			// claude-ai: Phase 3 — burn-down: reduce size every 4 ticks (GAME_TURN & 0x3 == 0)
			// claude-ai: When ff->size reaches 0, no new flames spawn; existing ones die → fire ends.
			if ((GAME_TURN & 0x3) == 0)
			{
				SLONG size;

				size  = ff->size;
				size -= ff->shrink;

				if (size < 0) {size = 0;}

				ff->size = size;
			}

			//
			// If this fire does not have enough flames...
			//

			// claude-ai: Replenish: spawn at most 1 new flame per tick to smoothly ramp up / sustain density
			if (ff->num < FIRE_num_flames_for_size(ff->size))
			{
				FIRE_add_flame(ff);
			}
		}
	}
}

// claude-ai: RENDERER INTERFACE — FIRE_get_start / FIRE_get_next iterator
// claude-ai: These functions let the renderer iterate over all flames visible in a screen-space band.
// claude-ai: FIRE_get_start sets up a filter by z-slice and xmin/xmax screen range.
// claude-ai: FIRE_get_next returns the next FIRE_Info to draw, or NULL when exhausted.
// claude-ai: NOTE: FIRE_get_next is INCOMPLETE — it returns NULL unconditionally after assembling
// claude-ai:   the flame list. The point-assembly logic was never finished in this pre-release build.
// claude-ai: Port notes: replace entirely with a new renderer-driven draw call; do not replicate this API.
#define FIRE_MAX_POINTS 16

// claude-ai: Iterator state stored in globals (not re-entrant — single render pass assumed)
UBYTE       FIRE_get_z;
UBYTE       FIRE_get_xmin;
UBYTE       FIRE_get_xmax;
UBYTE       FIRE_get_fire_upto;
UBYTE       FIRE_get_flame;
FIRE_Info   FIRE_get_info;
FIRE_Point 	FIRE_get_point[FIRE_MAX_POINTS];

// claude-ai: Call before iterating; resets iterator and stores screen-space filter parameters
void FIRE_get_start(UBYTE z, UBYTE xmin, UBYTE xmax)
{
	FIRE_get_fire_upto = 0;
	FIRE_get_flame     = NULL;
	FIRE_get_z         = z;
	FIRE_get_xmin      = xmin;
	FIRE_get_xmax      = xmax;
}

// claude-ai: Returns the next flame's render data or NULL at end.
// claude-ai: UNFINISHED — always returns NULL; point-assembly code was never written here.
// claude-ai: The intent was to project each flame's control points into screen space and fill FIRE_get_point[].
FIRE_Info *FIRE_get_next()
{
	FIRE_Flame *fl;

	if (FIRE_get_flame == NULL)
	{
		// claude-ai: Advance to the next non-empty fire slot
		while(1)
		{
			ASSERT(WITHIN(FIRE_get_fire_upto, 0, FIRE_MAX_FIRE - 1));

			if (FIRE_fire[FIRE_get_fire_upto].num)
			{
				FIRE_get_flame      = FIRE_fire[FIRE_get_fire_upto].next;
				FIRE_get_fire_upto += 1;

				break;
			}

			FIRE_get_fire_upto += 1;

			if (FIRE_get_fire_upto >= FIRE_MAX_FIRE)
			{
				return NULL;
			}
		}
	}

	ASSERT(WITHIN(FIRE_get_flame, 1, FIRE_MAX_FLAMES - 1));

	fl = &FIRE_flame[FIRE_get_flame];

	//
	// Create the points of the flame.
	//

	// claude-ai: STUB — point projection / screen-space assembly was never implemented; always returns NULL
	return NULL;
}
