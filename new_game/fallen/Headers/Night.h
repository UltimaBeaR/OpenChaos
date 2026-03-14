//
// Cached lighting.
//

// claude-ai: Night.h — the NIGHT lighting system. All in-world lighting passes through here.
// claude-ai:
// claude-ai: SYSTEM OVERVIEW:
// claude-ai:   Urban Chaos uses VERTEX-COLOUR lighting (no per-pixel lighting / no normal maps).
// claude-ai:   Each rendered polygon has its corner vertices coloured by the NIGHT system.
// claude-ai:   The lighting model has three layers:
// claude-ai:     1. AMBIENT light  — a global directional light (sun/sky), set by NIGHT_ambient().
// claude-ai:                         Changes with time-of-day (day = bright white, night = dark blue).
// claude-ai:     2. STATIC lights  — placed by the level editor (streetlamps, window glows).
// claude-ai:                         Stored in NIGHT_slight[]. Count: up to NIGHT_MAX_SLIGHTS=256.
// claude-ai:     3. DYNAMIC lights — created at runtime by explosions, fire, muzzle flashes, etc.
// claude-ai:                         Stored in NIGHT_dlight[]. Count: up to NIGHT_MAX_DLIGHTS=64.
// claude-ai:
// claude-ai: TIME-OF-DAY:
// claude-ai:   Controlled externally (not in this header). The game sets NIGHT_ambient() each frame
// claude-ai:   with a different colour to simulate dawn/dusk/night. No actual clock struct here —
// claude-ai:   the progression is driven by game state code calling NIGHT_ambient().
// claude-ai:   NIGHT_FLAG_DAYTIME (bit 2 of NIGHT_flag) determines whether cloud shadows apply.
// claude-ai:
// claude-ai: LIGHTING CACHE:
// claude-ai:   Lighting is expensive to recompute every frame, so the NIGHT system CACHES results:
// claude-ai:   - NIGHT_square[]: cached vertex colours for terrain map squares (lo-res grid)
// claude-ai:   - NIGHT_cache[][]: 2D array mapping lo-res map coords → NIGHT_square index
// claude-ai:   - NIGHT_dfcache[]: cached vertex colours for DFacets (building faces)
// claude-ai:   - NIGHT_walkable[]: pre-baked colours for walkable prim points (floors/roofs)
// claude-ai:   When a dynamic light moves/appears, affected cache squares are invalidated and rebuilt.
// claude-ai:
// claude-ai: PORTING NOTES:
// claude-ai:   In the new game, NIGHT_Colour values feed into OpenGL vertex colour attributes.
// claude-ai:   NIGHT_get_d3d_colour() converts from NIGHT_Colour (6-bit per channel, range 0-63)
// claude-ai:   to 0xAARRGGBB 32-bit ARGB for Direct3D — the new game will do equivalent conversion
// claude-ai:   to normalized floats for OpenGL (divide by NIGHT_MAX_BRIGHT=64).
// claude-ai:   The specular channel in D3D is repurposed as a fog-override (alpha=0xFF = no fog).
// claude-ai:   In the new game, replace specular with proper fog in the fragment shader.

#ifndef _NIGHT_
#define _NIGHT_


#include "pap.h"

// claude-ai: max 256 static lights per level (placed by editor, e.g. streetlamps)
#define NIGHT_MAX_SLIGHTS 256


#ifdef TARGET_DC

// My DC monitor is seriously dark, so to be able to see anything (i.e. does the texturing work?)
// I turn the brightness of the game up. But on a sensible monitor/TV it looks awful.
#ifdef DEBUG
#define NIGHT_LIGHT_MULTIPLIER 2.0f
#else
#define NIGHT_LIGHT_MULTIPLIER 1.0f
#endif

#else //#ifdef TARGET_DC

#define NIGHT_LIGHT_MULTIPLIER 1.0f

#endif //#else //#ifdef TARGET_DC


//
// The static lights.
//

// claude-ai: NIGHT_Slight — a static point light placed by the level editor.
// claude-ai: x/z are UBYTE (top bit used as flags — "inside" flag etc.).
// claude-ai: y is SWORD (world Y = height, can be negative for underground).
// claude-ai: red/green/blue are SBYTE colour tints (signed, so can subtract from ambient).
// claude-ai: radius is UBYTE — falloff distance in world units.
// claude-ai: These are loaded from the .night file (via NIGHT_load_ed_file) at level load.
typedef struct
{
	SWORD y;
	UBYTE x; // I'm grabbing myself the top bit
	UBYTE z; //  of these for flags of some sort (inside or something)
	SBYTE red;
	SBYTE green;
	SBYTE blue;
	UBYTE radius;

} NIGHT_Slight;

extern	NIGHT_Slight *NIGHT_slight;//[NIGHT_MAX_SLIGHTS];
extern	SLONG        NIGHT_slight_upto;

typedef struct
{
	UBYTE index;
	UBYTE number;

} NIGHT_Smap;

typedef	NIGHT_Smap	NIGHT_Smap_2d[PAP_SIZE_LO];

extern	NIGHT_Smap_2d *NIGHT_smap; //[PAP_SIZE_LO][PAP_SIZE_LO];

//
// The dynamic lights.
//

// claude-ai: NIGHT_Dlight — a runtime point light (explosion flash, fire glow, muzzle flash, etc.)
// claude-ai: Created by game code via NIGHT_dlight_create(), destroyed via NIGHT_dlight_destroy().
// claude-ai: 'next' field: linked list index (pool-based allocation, 64 max).
// claude-ai: NIGHT_DLIGHT_FLAG_REMOVE: light is queued for removal at end of this game turn.
// claude-ai: Dynamic lights affect cached NIGHT_square data via dlight_squares_up/down each frame.
#define NIGHT_DLIGHT_FLAG_USED   (1 << 0)
#define NIGHT_DLIGHT_FLAG_REMOVE (1 << 1)	// Will be removed next gameturn.

typedef struct
{
	UWORD x;
	SWORD y;
	UWORD z;
	UBYTE red;
	UBYTE green;
	UBYTE blue;
	UBYTE radius;
	UBYTE next;  // claude-ai: linked list — index of next dlight or 0 for end
	UBYTE flag;  // claude-ai: NIGHT_DLIGHT_FLAG_* bits

} NIGHT_Dlight;

// claude-ai: hard limit: max 64 simultaneous dynamic lights in the scene
#define NIGHT_MAX_DLIGHTS 64

extern	NIGHT_Dlight *NIGHT_dlight; //[NIGHT_MAX_DLIGHTS];



//
// Coloured lighting.
//

// claude-ai: NIGHT_Colour — the fundamental lighting colour type used throughout the system.
// claude-ai: On PC/DC: 3 separate UBYTE channels (red, green, blue). Range: 0..NIGHT_MAX_BRIGHT (0..63).
// claude-ai: On PSX: packed into a UWORD with bit-field macros (get_red/get_green/get_blue below).
// claude-ai: PORTING: in new game, normalize to float: r_f = col.red / 64.0f for OpenGL vertex colour.
typedef struct
{
#ifdef	PSX_COMPRESS_LIGHT
	UWORD	Col;
#else
	UBYTE red;
	UBYTE green;
	UBYTE blue;
#endif

} NIGHT_Colour;



#define	get_red(col)	(((col)>>10)&0x3f)
#define	get_green(col)	(((col)>>5)&0x1f)
#define	get_blue(col)	(((col))&0x1f)

#define	set_red(col,red)	col=(red)<<10
#define	set_green(col,red)	col=(red)<<5
#define	set_blue(col,red)	col=(red)

#define	or_red(col,red)		col|=(red)<<10
#define	or_green(col,red)	col|=(red)<<5
#define	or_blue(col,red)	col|=(red)

#define	nor_red(col,red)	col=(col&(0x3f<<10))|((red)<<10)
#define	nor_green(col,red)	col=(col&(0x1f<<5))|((red)<<5)
#define	nor_blue(col,red)	col=(col&(0x1f<<0))|((red)<<0)

//
// The cached lighting lights all the points and all the prims
// in each lo-res mapsquare.  The first 16 colour entries are
// the colours of the 4x4 points in this mapsquare.  After this
// there are the colours of all the points of the prims as returned
// by the OB module.
//

#define NIGHT_SQUARE_FLAG_USED		(1 << 0)
#define NIGHT_SQUARE_FLAG_WARE		(1 << 1)
#define NIGHT_SQUARE_FLAG_DELETEME	(1 << 2)

typedef struct
{
	UBYTE         next;
	UBYTE         flag;
	UBYTE         lo_map_x;
	UBYTE         lo_map_z;
	NIGHT_Colour *colour;
	ULONG		  sizeof_colour;	// In bytes.

} NIGHT_Square;

#define NIGHT_MAX_SQUARES 256

extern NIGHT_Square NIGHT_square[NIGHT_MAX_SQUARES];
extern UBYTE        NIGHT_square_free;

//
// The cached lighting mapwho. Contains indices into the
// NIGHT_square array, or NULL if there is no cached lighting
// for that square.
//

extern UBYTE NIGHT_cache[PAP_SIZE_LO][PAP_SIZE_LO];

#ifdef	PSX
extern	UWORD	floor_psx_col[PAP_SIZE_HI][PAP_SIZE_HI];
#endif

//
// The cached lighting for dfacets.
//

typedef struct
{
	UBYTE         next;
	UBYTE         counter;
	UWORD         dfacet;
	NIGHT_Colour *colour;
	UWORD         sizeof_colour;

} NIGHT_Dfcache;

#define NIGHT_MAX_DFCACHES 256

extern NIGHT_Dfcache NIGHT_dfcache[NIGHT_MAX_DFCACHES];
extern UBYTE         NIGHT_dfcache_free;
extern UBYTE		 NIGHT_dfcache_used;

// ========================================================
//
// NEW LIGHTING FUNCTIONS
//
// ========================================================

//
// Initialises all the lighting: Removes all static and dynamic lights,
// and clears all cached info.
//

void NIGHT_init(void);

//
// Converts a colour to its D3D equivalents.
//

// claude-ai: NIGHT_MAX_BRIGHT=64 is the maximum value of a NIGHT_Colour channel.
// claude-ai: In NIGHT_get_d3d_colour: value *= (256/64) = *4 to scale to 0..255 for D3D.
// claude-ai: Overflow above 255 becomes specular highlight (if NIGHT_specular_enable is set).
#define NIGHT_MAX_BRIGHT 64

//
// Make bright light create specular.
//

#ifdef TARGET_DC
static const SLONG NIGHT_specular_enable = 0;
#else
extern SLONG NIGHT_specular_enable;
#endif

inline void NIGHT_get_d3d_colour(NIGHT_Colour col, ULONG *colour, ULONG *specular)
{
	SLONG red   = col.red;
	SLONG green = col.green;
	SLONG blue  = col.blue;

	red   *= (256 / NIGHT_MAX_BRIGHT);
	green *= (256 / NIGHT_MAX_BRIGHT);
	blue  *= (256 / NIGHT_MAX_BRIGHT);

	if (NIGHT_specular_enable)
	{
		SLONG wred   = 0;
		SLONG wgreen = 0;
		SLONG wblue  = 0;

		if (red   > 255) {wred   = (red   - 255) >> 1; red   = 255; if (wred   > 255) {wred   = 255;}}
		if (green > 255) {wgreen = (green - 255) >> 1; green = 255; if (wgreen > 255) {wgreen = 255;}}
		if (blue  > 255) {wblue  = (blue  - 255) >> 1; blue  = 255; if (wblue  > 255) {wblue  = 255;}}

	   *colour    = (red  << 16) | (green  << 8) | (blue  << 0);
	   *specular  = (wred << 16) | (wgreen << 8) | (wblue << 0);
	   *specular |= 0xff000000;		// No fog by default...
	}
	else
	{
		if (red   > 255) {red   = 255;}
		if (green > 255) {green = 255;}
		if (blue  > 255) {blue  = 255;}

	   *colour   = (red << 16) | (green << 8) | (blue << 0);
	   *specular = 0xff000000;		// No fog by default...
	}
	*colour |= 0xff000000;		// No fog by default...
}

//
// z in range 0->1.0
//

#ifndef	POLY_FADEOUT_START
//
// sneak it in here
//
#define POLY_FADEOUT_START	(0.60F)
#define POLY_FADEOUT_END	(0.95F)
#endif

inline void NIGHT_get_d3d_colour_and_fade(NIGHT_Colour col, ULONG *colour, ULONG *specular,float z)
{
	SLONG red   = col.red;
	SLONG green = col.green;
	SLONG blue  = col.blue;
	SLONG multi = 255 - (SLONG)((z - POLY_FADEOUT_START) * (256.0F / (POLY_FADEOUT_END - POLY_FADEOUT_START)));

//red   *= (256 / NIGHT_MAX_BRIGHT);
//green *= (256 / NIGHT_MAX_BRIGHT);
//blue  *= (256 / NIGHT_MAX_BRIGHT);

	if(multi>255)
	{
		multi=255;
	}

	if (multi < 0)
	{
		multi = 0;
	}
	multi*=4; //replaces above

	red		= red	*multi>>8;  // fade out on distance
	green	= green	*multi>>8; 
	blue	= blue	*multi>>8;


/* //dead I believe
	if (NIGHT_specular_enable)
	{
		SLONG wred   = 0;
		SLONG wgreen = 0;
		SLONG wblue  = 0;

		if (red   > 255) {wred   = (red   - 255) >> 1; red   = 255; if (wred   > 255) {wred   = 255;}}
		if (green > 255) {wgreen = (green - 255) >> 1; green = 255; if (wgreen > 255) {wgreen = 255;}}
		if (blue  > 255) {wblue  = (blue  - 255) >> 1; blue  = 255; if (wblue  > 255) {wblue  = 255;}}

	   *colour    = (red  << 16) | (green  << 8) | (blue  << 0);
	   *specular  = (wred << 16) | (wgreen << 8) | (wblue << 0);
	   *specular |= 0xff000000;		// No fog by default...
	}
	else
*/
	{
		if (red   > 255) {red   = 255;}
		if (green > 255) {green = 255;}
		if (blue  > 255) {blue  = 255;}

	   *colour   = (red << 16) | (green << 8) | (blue << 0);
	   *specular = 0xff000000;		// No fog by default...
	}
	*colour |= 0xff000000;		// No fog by default...
}

inline void NIGHT_get_d3d_colour_dim(NIGHT_Colour col, ULONG *colour, ULONG *specular)
{
	SLONG red   = col.red;
	SLONG green = col.green;
	SLONG blue  = col.blue;

	red   *= (256 / NIGHT_MAX_BRIGHT);
	green *= (256 / NIGHT_MAX_BRIGHT);
	blue  *= (256 / NIGHT_MAX_BRIGHT);

	red>>=1;
	green>>=1;
	blue>>=1;

	if (NIGHT_specular_enable)
	{
		SLONG wred   = 0;
		SLONG wgreen = 0;
		SLONG wblue  = 0;

		if (red   > 255) {wred   = (red   - 255) >> 1; red   = 255; if (wred   > 255) {wred   = 255;}}
		if (green > 255) {wgreen = (green - 255) >> 1; green = 255; if (wgreen > 255) {wgreen = 255;}}
		if (blue  > 255) {wblue  = (blue  - 255) >> 1; blue  = 255; if (wblue  > 255) {wblue  = 255;}}

	   *colour    = (red  << 16) | (green  << 8) | (blue  << 0);
	   *specular  = (wred << 16) | (wgreen << 8) | (wblue << 0);
	   *specular |= 0xff000000;		// No fog by default...
	}
	else
	{
		if (red   > 255) {red   = 255;}
		if (green > 255) {green = 255;}
		if (blue  > 255) {blue  = 255;}

	   *colour   = (red << 16) | (green << 8) | (blue << 0);
	   *specular = 0xff000000;		// No fog by default...
	}
	*colour |= 0xff000000;		// No fog by default...
}

//
// Returns the amount of light at the given spot.
//

NIGHT_Colour NIGHT_get_light_at(
				SLONG x,
				SLONG y,
				SLONG z);


#ifndef PSX

//
// Fills the array with all the lights that effect the given point.
// (not including the ambient light)
//

typedef struct
{
	SLONG r;	// (r,g,b) falling on the point.
	SLONG g;
	SLONG b;
	SLONG dx;	// Normalised vector from the point to the light (256 long)
	SLONG dy;
	SLONG dz;

} NIGHT_Found;

#define NIGHT_MAX_FOUND 4

extern NIGHT_Found NIGHT_found[NIGHT_MAX_FOUND];
extern SLONG       NIGHT_found_upto;

void NIGHT_find(SLONG x, SLONG y, SLONG z);

#endif


//
// Initialises the heap and gets rid of all cached lighting.
//

void NIGHT_destroy_all_cached_info(void);

//
// Lighting flags and variables.
//

// claude-ai: NIGHT_flag bitmask — global lighting mode switches.
// claude-ai:   NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS: streetlamps are active (night time)
// claude-ai:   NIGHT_FLAG_DARKEN_BUILDING_POINTS: building vertices get darkened (shadow side)
// claude-ai:   NIGHT_FLAG_DAYTIME: it is daytime — enables cloud shadows, no lampost lights
// claude-ai: The game sets these flags based on current time-of-day to drive lighting behaviour.
#define NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS	(1 << 0)
#define NIGHT_FLAG_DARKEN_BUILDING_POINTS	(1 << 1)
#define NIGHT_FLAG_DAYTIME					(1 << 2)

extern ULONG        NIGHT_flag;
// claude-ai: NIGHT_sky_colour: current ambient sky/fog colour (changes with time of day)
extern NIGHT_Colour NIGHT_sky_colour;
// claude-ai: NIGHT_lampost_*: colour and radius of all streetlamp static lights (shared params)
extern UBYTE        NIGHT_lampost_radius;
extern SBYTE        NIGHT_lampost_red;
extern SBYTE        NIGHT_lampost_green;
extern SBYTE        NIGHT_lampost_blue;

//
// Loads a lighting file saved from the ED module.  Return TRUE
// on success.
//

extern SLONG NIGHT_load_ed_file(CBYTE *name);


// ========================================================
//
// AMBIENT LIGHT
//
// ========================================================

// claude-ai: Ambient (directional) light — simulates the sun/sky illuminating the whole scene.
// claude-ai: NIGHT_amb_red/green/blue: colour of the ambient light (0..255 range here, not 0..63).
// claude-ai: NIGHT_amb_norm_x/y/z: direction the light comes FROM (normalised to 256 length).
// claude-ai: NIGHT_amb_d3d_colour / NIGHT_amb_d3d_specular: precomputed D3D ARGB values for fast use.
// claude-ai: These are recomputed by NIGHT_ambient() whenever time-of-day changes.
// claude-ai: PORTING: in new game, pass norm_x/y/z and colour as a UBO directional light struct.
extern ULONG NIGHT_amb_d3d_colour;
extern ULONG NIGHT_amb_d3d_specular;
extern SLONG NIGHT_amb_red;
extern SLONG NIGHT_amb_green;
extern SLONG NIGHT_amb_blue;
extern SLONG NIGHT_amb_norm_x;  // claude-ai: sun direction X (normalised, scale 256)
extern SLONG NIGHT_amb_norm_y;  // claude-ai: sun direction Y (up/down)
extern SLONG NIGHT_amb_norm_z;  // claude-ai: sun direction Z

//
// The normal should be normalised to 256.
//

void NIGHT_ambient(
		UBYTE red,
		UBYTE green,
		UBYTE blue,
		SLONG norm_x,
		SLONG norm_y,
		SLONG norm_z);

//
// Returns the amount of light captured by a point with the
// given normal. (256 long)
// 

NIGHT_Colour NIGHT_ambient_at_point(
				SLONG norm_x,
				SLONG norm_y,
				SLONG norm_z);

// ========================================================
//
// STATIC LIGHTS
//
// ========================================================

SLONG NIGHT_slight_create(		// Returns FALSE on failure.
		SLONG x,
		SLONG y,
		SLONG z,
		UBYTE radius,
		SBYTE red,
		SBYTE green,
		SBYTE blue);

void  NIGHT_slight_delete(
		SLONG x,
		SLONG y,
		SLONG z,
		UBYTE radius,
		SBYTE red,
		SBYTE green,
		SBYTE blue);

void  NIGHT_slight_delete_all(void);


// ========================================================
// 
// DYNAMIC LIGHTS
//
// ========================================================

UBYTE NIGHT_dlight_create(
		SLONG x,
		SLONG y,
		SLONG z,
		UBYTE radius,
		UBYTE red,
		UBYTE green,
		UBYTE blue);

void NIGHT_dlight_destroy(UBYTE dlight_index);
void NIGHT_dlight_move   (UBYTE dlight_index, SLONG x, SLONG y, SLONG z);
void NIGHT_dlight_colour (UBYTE dlight_index, UBYTE red, UBYTE green, UBYTE blue);

//
// Lights all the used cached lighting info from the dynamic lights.
// Undoes all the dynamic lighting.
//

void NIGHT_dlight_squares_up  (void);
void NIGHT_dlight_squares_down(void);


// ========================================================
//
// CACHED LIGHTING OF THE MAP AND PRIMS
//
// ========================================================

//
// Recalculates all the cache entries.
// Creates a mapwho entry in the given lo-res mapwho square.
// Frees up all the memory.
//

void NIGHT_cache_recalc (void);
void NIGHT_cache_create (UBYTE lo_map_x, UBYTE lo_map_z, UBYTE inside_warehouse = FALSE);
void NIGHT_cache_create_inside(UBYTE lo_map_x, UBYTE lo_map_z,SLONG floor_y);
void NIGHT_cache_destroy(UBYTE square_index);


// ========================================================
//
// CACHED LIGHTING OF DFACETS ON THE SUPERMAP
//
// ========================================================

void  NIGHT_dfcache_recalc (void);
UBYTE NIGHT_dfcache_create (UWORD dfacet_index);
void  NIGHT_dfcache_destroy(UBYTE dfcache_index);


// ========================================================
//
// STATIC LIGHTING OF THE WALKABLE FACES' POINTS
//
// ========================================================

//
// The first few thousand prim points dont belong to the walkable faces.
// This is the index of the first walkable prim point.  
// Walkable primpoint x is index as NIGHT_walkable[x - NIGHT_first_walkable_point]...
//

// claude-ai: NIGHT_walkable[] — pre-baked vertex colours for up to 15000 walkable prim points.
// claude-ai: "Walkable prim points" are the polygon corners of floor/ground surfaces NPCs walk on.
// claude-ai: These are computed once at level load by NIGHT_generate_walkable_lighting().
// claude-ai: NIGHT_first_walkable_prim_point: offset — prim_points[0..N] are non-walkable geometry;
// claude-ai:   walkable ones start at this index. Access via NIGHT_WALKABLE_POINT(prim_point_index).
// claude-ai: NIGHT_roof_walkable[]: same but for roof surfaces; indexed by (facet * 4 + corner).
#define NIGHT_MAX_WALKABLE 15000

extern SLONG NIGHT_first_walkable_prim_point;
extern NIGHT_Colour NIGHT_walkable[NIGHT_MAX_WALKABLE];
extern NIGHT_Colour NIGHT_roof_walkable[];

#define NIGHT_ROOF_WALKABLE_POINT(f,p) (NIGHT_roof_walkable[f*4+p])

#ifndef NDEBUG
SLONG NIGHT_check_index(SLONG walkable_prim_point_index);
#define NIGHT_WALKABLE_POINT(p) (NIGHT_check_index(p), NIGHT_walkable[p - NIGHT_first_walkable_prim_point])
#else

//
// Returns the colour of the given walkable prim_point.
//

#define NIGHT_WALKABLE_POINT(p) (NIGHT_walkable[(p) - NIGHT_first_walkable_prim_point])

#endif

//
// Generates the walkable info for the current map.
//

void NIGHT_generate_walkable_lighting(void);


#endif







