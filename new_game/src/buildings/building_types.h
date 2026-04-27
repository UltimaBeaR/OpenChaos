#ifndef BUILDINGS_BUILDING_TYPES_H
#define BUILDINGS_BUILDING_TYPES_H

// Building data-model types: storey system, wall/facet definitions,
// pool limits, and all STOREY_TYPE_*/FACET_FLAG_*/FLAG_* constants.
// These are the static data structures for procedural building geometry.

#include "engine/core/types.h"
#include "buildings/prim_types.h" // PrimPoint, PrimFace3/4, etc.

// =====================================================================
// Texture piece indices
// =====================================================================

// uc_orig: TEXTURE_PIECE_LEFT (fallen/Headers/building.h)
#define TEXTURE_PIECE_LEFT (0)
// uc_orig: TEXTURE_PIECE_MIDDLE (fallen/Headers/building.h)
#define TEXTURE_PIECE_MIDDLE (1)
// uc_orig: TEXTURE_PIECE_RIGHT (fallen/Headers/building.h)
#define TEXTURE_PIECE_RIGHT (2)
// uc_orig: TEXTURE_PIECE_MIDDLE1 (fallen/Headers/building.h)
#define TEXTURE_PIECE_MIDDLE1 (3)
// uc_orig: TEXTURE_PIECE_MIDDLE2 (fallen/Headers/building.h)
#define TEXTURE_PIECE_MIDDLE2 (4)
// uc_orig: TEXTURE_PIECE_NUMBER (fallen/Headers/building.h)
#define TEXTURE_PIECE_NUMBER (5)

// =====================================================================
// Map grid dimensions
// =====================================================================

// uc_orig: BLOCK_SHIFT (fallen/Headers/building.h)
#define BLOCK_SHIFT (6)
// uc_orig: BLOCK_SIZE (fallen/Headers/building.h)
#define BLOCK_SIZE (1 << BLOCK_SHIFT)

// =====================================================================
// Build mode
// =====================================================================

// uc_orig: BUILD_MODE_EDITOR (fallen/Headers/building.h)
#define BUILD_MODE_EDITOR (1)
// uc_orig: BUILD_MODE_DX (fallen/Headers/building.h)
#define BUILD_MODE_DX (2)
// uc_orig: BUILD_MODE_SOFT (fallen/Headers/building.h)
#define BUILD_MODE_SOFT (3)
// uc_orig: BUILD_MODE_PSX (fallen/Headers/building.h)
#define BUILD_MODE_PSX (4)
// uc_orig: BUILD_MODE_OTHER (fallen/Headers/building.h)
#define BUILD_MODE_OTHER (5)

// =====================================================================
// Building type codes (FBuilding.BuildingType)
// =====================================================================

// uc_orig: BUILDING_TYPE_HOUSE (fallen/Headers/building.h)
#define BUILDING_TYPE_HOUSE 0
// uc_orig: BUILDING_TYPE_WAREHOUSE (fallen/Headers/building.h)
#define BUILDING_TYPE_WAREHOUSE 1
// uc_orig: BUILDING_TYPE_OFFICE (fallen/Headers/building.h)
#define BUILDING_TYPE_OFFICE 2
// uc_orig: BUILDING_TYPE_APARTEMENT (fallen/Headers/building.h)
#define BUILDING_TYPE_APARTEMENT 3
// uc_orig: BUILDING_TYPE_CRATE_IN (fallen/Headers/building.h)
#define BUILDING_TYPE_CRATE_IN 4
// uc_orig: BUILDING_TYPE_CRATE_OUT (fallen/Headers/building.h)
#define BUILDING_TYPE_CRATE_OUT 5

// =====================================================================
// Building/storey/wall pool limits
// =====================================================================

// uc_orig: MAX_BUILDINGS (fallen/Headers/building.h)
#define MAX_BUILDINGS (500)
// uc_orig: MAX_STOREYS (fallen/Headers/building.h)
#define MAX_STOREYS (MAX_BUILDINGS * 5)
// uc_orig: MAX_WALLS (fallen/Headers/building.h)
#define MAX_WALLS (MAX_STOREYS * 6)
// uc_orig: MAX_WINDOWS (fallen/Headers/building.h)
#define MAX_WINDOWS (50)
// uc_orig: MAX_INSIDE_STOREYS (fallen/Headers/building.h)
#define MAX_INSIDE_STOREYS (1024)
// uc_orig: MAX_BUILDING_FACETS (fallen/Headers/building.h)
#define MAX_BUILDING_FACETS 4000
// uc_orig: MAX_BUILDING_OBJECTS (fallen/Headers/building.h)
#define MAX_BUILDING_OBJECTS 400

// =====================================================================
// Facet flags (BuildingFacet.FacetFlags / DFacet flags)
// =====================================================================

// uc_orig: FACET_FLAG_NEAR_SORT (fallen/Headers/building.h)
#define FACET_FLAG_NEAR_SORT (1 << 0)
// uc_orig: FACET_FLAG_ROOF (fallen/Headers/building.h)
#define FACET_FLAG_ROOF (1 << 1)
// uc_orig: FACET_FLAG_CABLE (fallen/Headers/building.h)
#define FACET_FLAG_CABLE (1 << 2)
// uc_orig: FACET_FLAG_NON_SORT (fallen/Headers/building.h)
#define FACET_FLAG_NON_SORT (1 << 3)
// uc_orig: FACET_FLAG_DONT_CULL (fallen/Headers/building.h)
#define FACET_FLAG_DONT_CULL (1 << 3)

// uc_orig: FACET_FLAG_IN_SEWERS (fallen/Headers/building.h)
#define FACET_FLAG_IN_SEWERS (1 << 4)
// uc_orig: FACET_FLAG_LADDER_LINK (fallen/Headers/building.h)
#define FACET_FLAG_LADDER_LINK (1 << 5)

// uc_orig: FACET_FLAG_INVISIBLE (fallen/Headers/building.h)
#define FACET_FLAG_INVISIBLE (1 << 0)
// uc_orig: FACET_FLAG_INSIDE (fallen/Headers/building.h)
#define FACET_FLAG_INSIDE (1 << 3)
// uc_orig: FACET_FLAG_DLIT (fallen/Headers/building.h)
#define FACET_FLAG_DLIT (1 << 4)
// uc_orig: FACET_FLAG_HUG_FLOOR (fallen/Headers/building.h)
#define FACET_FLAG_HUG_FLOOR (1 << 5)
// uc_orig: FACET_FLAG_ELECTRIFIED (fallen/Headers/building.h)
#define FACET_FLAG_ELECTRIFIED (1 << 6)
// uc_orig: FACET_FLAG_2SIDED (fallen/Headers/building.h)
#define FACET_FLAG_2SIDED (1 << 7)
// uc_orig: FACET_FLAG_UNCLIMBABLE (fallen/Headers/building.h)
#define FACET_FLAG_UNCLIMBABLE (1 << 8)
// uc_orig: FACET_FLAG_ONBUILDING (fallen/Headers/building.h)
#define FACET_FLAG_ONBUILDING (1 << 9)
// uc_orig: FACET_FLAG_BARB_TOP (fallen/Headers/building.h)
#define FACET_FLAG_BARB_TOP (1 << 10)
// uc_orig: FACET_FLAG_SEETHROUGH (fallen/Headers/building.h)
#define FACET_FLAG_SEETHROUGH (1 << 11)
// uc_orig: FACET_FLAG_OPEN (fallen/Headers/building.h)
#define FACET_FLAG_OPEN (1 << 12)
// uc_orig: FACET_FLAG_90DEGREE (fallen/Headers/building.h)
#define FACET_FLAG_90DEGREE (1 << 13)
// uc_orig: FACET_FLAG_2TEXTURED (fallen/Headers/building.h)
#define FACET_FLAG_2TEXTURED (1 << 14)
// uc_orig: FACET_FLAG_FENCE_CUT (fallen/Headers/building.h)
#define FACET_FLAG_FENCE_CUT (1 << 15)

// =====================================================================
// Roof flags
// =====================================================================

// uc_orig: FLAG_ROOF_FLAT (fallen/Headers/building.h)
#define FLAG_ROOF_FLAT (1 << 1)
// uc_orig: FLAG_ROOF_OVERLAP_SMALL (fallen/Headers/building.h)
#define FLAG_ROOF_OVERLAP_SMALL (1 << 2)
// uc_orig: FLAG_ROOF_OVERLAP_MEDIUM (fallen/Headers/building.h)
#define FLAG_ROOF_OVERLAP_MEDIUM (1 << 3)
// uc_orig: FLAG_ROOF_WALLED (fallen/Headers/building.h)
#define FLAG_ROOF_WALLED (1 << 4)
// uc_orig: FLAG_ROOF_RECESSED (fallen/Headers/building.h)
#define FLAG_ROOF_RECESSED (1 << 5)

// =====================================================================
// Wall flags
// =====================================================================

// uc_orig: FLAG_WALL_USED (fallen/Headers/building.h)
#define FLAG_WALL_USED (1 << 0)
// uc_orig: FLAG_WALL_AUTO_WINDOWS (fallen/Headers/building.h)
#define FLAG_WALL_AUTO_WINDOWS (1 << 1)
// uc_orig: FLAG_WALL_FACET_LINKED (fallen/Headers/building.h)
#define FLAG_WALL_FACET_LINKED (1 << 2)
// uc_orig: FLAG_WALL_FENCE_POST1 (fallen/Headers/building.h)
#define FLAG_WALL_FENCE_POST1 (1 << 3)
// uc_orig: FLAG_WALL_FENCE_POST2 (fallen/Headers/building.h)
#define FLAG_WALL_FENCE_POST2 (1 << 4)
// uc_orig: FLAG_WALL_RECESSED (fallen/Headers/building.h)
#define FLAG_WALL_RECESSED (1 << 5)
// uc_orig: FLAG_WALL_CLIMBABLE (fallen/Headers/building.h)
#define FLAG_WALL_CLIMBABLE (1 << 6)
// uc_orig: FLAG_WALL_ARCH_TOP (fallen/Headers/building.h)
#define FLAG_WALL_ARCH_TOP (1 << 7)
// uc_orig: FLAG_WALL_BARB_TOP (fallen/Headers/building.h)
#define FLAG_WALL_BARB_TOP (1 << 8)

// =====================================================================
// Storey flags
// =====================================================================

// uc_orig: FLAG_STOREY_USED (fallen/Headers/building.h)
#define FLAG_STOREY_USED (1 << 0)
// uc_orig: FLAG_STOREY_FLAT_TILED_ROOF (fallen/Headers/building.h)
#define FLAG_STOREY_FLAT_TILED_ROOF (1 << 1)
// uc_orig: FLAG_STOREY_LEDGE (fallen/Headers/building.h)
#define FLAG_STOREY_LEDGE (1 << 2)
// uc_orig: FLAG_STOREY_ROOF (fallen/Headers/building.h)
#define FLAG_STOREY_ROOF (1 << 3)
// uc_orig: FLAG_STOREY_FACET_LINKED (fallen/Headers/building.h)
#define FLAG_STOREY_FACET_LINKED (1 << 4)
// uc_orig: FLAG_STOREY_TILED_ROOF (fallen/Headers/building.h)
#define FLAG_STOREY_TILED_ROOF (1 << 5)
// uc_orig: FLAG_STOREY_ROOF_RIM (fallen/Headers/building.h)
#define FLAG_STOREY_ROOF_RIM (1 << 6)
// uc_orig: FLAG_STOREY_ROOF_RIM2 (fallen/Headers/building.h)
#define FLAG_STOREY_ROOF_RIM2 (1 << 7)
// uc_orig: FLAG_STOREY_UNCLIMBABLE (fallen/Headers/building.h)
#define FLAG_STOREY_UNCLIMBABLE (1 << 10)
// uc_orig: FLAG_STOREY_ONBUILDING (fallen/Headers/building.h)
#define FLAG_STOREY_ONBUILDING (1 << 11)

// Extra storey flags (FStorey.ExtraFlags).
// uc_orig: FLAG_STOREY_EXTRA_UNCLIMBABLE (fallen/Headers/building.h)
#define FLAG_STOREY_EXTRA_UNCLIMBABLE (1 << 0)
// uc_orig: FLAG_STOREY_EXTRA_ONBUILDING (fallen/Headers/building.h)
#define FLAG_STOREY_EXTRA_ONBUILDING (1 << 1)
// uc_orig: FLAG_STOREY_EXTRA_90DEGREE (fallen/Headers/building.h)
#define FLAG_STOREY_EXTRA_90DEGREE (1 << 2)

// uc_orig: FLAG_ISTOREY_DOOR (fallen/Headers/building.h)
#define FLAG_ISTOREY_DOOR (1 << 1)

// =====================================================================
// Storey type codes (FStorey.StoreyType)
// =====================================================================

// uc_orig: STOREY_TYPE_NORMAL (fallen/Headers/building.h)
#define STOREY_TYPE_NORMAL (1)
// uc_orig: STOREY_TYPE_ROOF (fallen/Headers/building.h)
#define STOREY_TYPE_ROOF (2)
// uc_orig: STOREY_TYPE_WALL (fallen/Headers/building.h)
#define STOREY_TYPE_WALL (3)
// uc_orig: STOREY_TYPE_ROOF_QUAD (fallen/Headers/building.h)
#define STOREY_TYPE_ROOF_QUAD (4)
// uc_orig: STOREY_TYPE_FLOOR_POINTS (fallen/Headers/building.h)
#define STOREY_TYPE_FLOOR_POINTS (5)
// uc_orig: STOREY_TYPE_FIRE_ESCAPE (fallen/Headers/building.h)
#define STOREY_TYPE_FIRE_ESCAPE (6)
// uc_orig: STOREY_TYPE_STAIRCASE (fallen/Headers/building.h)
#define STOREY_TYPE_STAIRCASE (7)
// uc_orig: STOREY_TYPE_SKYLIGHT (fallen/Headers/building.h)
#define STOREY_TYPE_SKYLIGHT (8)
// Zip-line/cable that Darci can slide along.
// uc_orig: STOREY_TYPE_CABLE (fallen/Headers/building.h)
#define STOREY_TYPE_CABLE (9)
// uc_orig: STOREY_TYPE_FENCE (fallen/Headers/building.h)
#define STOREY_TYPE_FENCE (10)
// uc_orig: STOREY_TYPE_FENCE_BRICK (fallen/Headers/building.h)
#define STOREY_TYPE_FENCE_BRICK (11)
// uc_orig: STOREY_TYPE_LADDER (fallen/Headers/building.h)
#define STOREY_TYPE_LADDER (12)
// uc_orig: STOREY_TYPE_FENCE_FLAT (fallen/Headers/building.h)
#define STOREY_TYPE_FENCE_FLAT (13)
// uc_orig: STOREY_TYPE_TRENCH (fallen/Headers/building.h)
#define STOREY_TYPE_TRENCH (14)
// uc_orig: STOREY_TYPE_JUST_COLLISION (fallen/Headers/building.h)
#define STOREY_TYPE_JUST_COLLISION (15)
// uc_orig: STOREY_TYPE_PARTITION (fallen/Headers/building.h)
#define STOREY_TYPE_PARTITION (16)
// uc_orig: STOREY_TYPE_INSIDE (fallen/Headers/building.h)
#define STOREY_TYPE_INSIDE (17)
// uc_orig: STOREY_TYPE_DOOR (fallen/Headers/building.h)
#define STOREY_TYPE_DOOR (18)
// uc_orig: STOREY_TYPE_INSIDE_DOOR (fallen/Headers/building.h)
#define STOREY_TYPE_INSIDE_DOOR (19)
// uc_orig: STOREY_TYPE_OINSIDE (fallen/Headers/building.h)
#define STOREY_TYPE_OINSIDE (20)
// External entrance to a building interior.
// uc_orig: STOREY_TYPE_OUTSIDE_DOOR (fallen/Headers/building.h)
#define STOREY_TYPE_OUTSIDE_DOOR (21)

// uc_orig: STOREY_TYPE_NORMAL_FOUNDATION (fallen/Headers/building.h)
#define STOREY_TYPE_NORMAL_FOUNDATION (100)

// Sentinel prim-type values written into collision vectors by the building system.
// uc_orig: STOREY_TYPE_NOT_REALLY_A_STOREY_TYPE_BUT_A_VALUE_TO_PUT_IN_THE_PRIM_TYPE_FIELD_OF_COLVECTS_GENERATED_BY_INSIDE_BUILDINGS (fallen/Headers/building.h)
#define STOREY_TYPE_NOT_REALLY_A_STOREY_TYPE_BUT_A_VALUE_TO_PUT_IN_THE_PRIM_TYPE_FIELD_OF_COLVECTS_GENERATED_BY_INSIDE_BUILDINGS (254)
// uc_orig: STOREY_TYPE_NOT_REALLY_A_STOREY_TYPE_AGAIN_THIS_IS_THE_VALUE_PUT_INTO_PRIMTYPE_BY_THE_SEWERS (fallen/Headers/building.h)
#define STOREY_TYPE_NOT_REALLY_A_STOREY_TYPE_AGAIN_THIS_IS_THE_VALUE_PUT_INTO_PRIMTYPE_BY_THE_SEWERS (255)

// =====================================================================
// Cable system constants
// =====================================================================

// Position along a cable, 0 to CABLE_ALONG_MAX (2^CABLE_ALONG_SHIFT).
// uc_orig: CABLE_ALONG_SHIFT (fallen/Headers/building.h)
#define CABLE_ALONG_SHIFT 12
// uc_orig: CABLE_ALONG_MAX (fallen/Headers/building.h)
#define CABLE_ALONG_MAX (1 << CABLE_ALONG_SHIFT)

// =====================================================================
// Stair flags
// =====================================================================

// uc_orig: STAIR_FLAG_UP (fallen/Headers/building.h)
#define STAIR_FLAG_UP (1 << 0)
// uc_orig: STAIR_FLAG_DOWN (fallen/Headers/building.h)
#define STAIR_FLAG_DOWN (1 << 1)
// uc_orig: STAIR_FLAGS_DIR (fallen/Headers/building.h)
#define STAIR_FLAGS_DIR ((1 << 2) | (1 << 3))
// uc_orig: SET_STAIR_DIR (fallen/Headers/building.h)
#define SET_STAIR_DIR(x, dir) ((x) = ((x) & ~STAIR_FLAGS_DIR) | (dir << 2))
// uc_orig: GET_STAIR_DIR (fallen/Headers/building.h)
#define GET_STAIR_DIR(x) (((x) & STAIR_FLAGS_DIR) >> 2)

// =====================================================================
// Wall texture style IDs
// =====================================================================

// uc_orig: BROWN_BRICK1 (fallen/Headers/building.h)
#define BROWN_BRICK1 1
// uc_orig: BROWN_BRICK2 (fallen/Headers/building.h)
#define BROWN_BRICK2 2
// uc_orig: GREY_RIM1 (fallen/Headers/building.h)
#define GREY_RIM1 3
// uc_orig: GREY_RIM2 (fallen/Headers/building.h)
#define GREY_RIM2 4
// uc_orig: RED_WINDOW (fallen/Headers/building.h)
#define RED_WINDOW 5
// uc_orig: GREY_CORIGATED (fallen/Headers/building.h)
#define GREY_CORIGATED 6
// uc_orig: CRATES_SMALL_BROWN (fallen/Headers/building.h)
#define CRATES_SMALL_BROWN 7
// uc_orig: GREY_POSH (fallen/Headers/building.h)
#define GREY_POSH 8
// uc_orig: HOTEL_SIGN1 (fallen/Headers/building.h)
#define HOTEL_SIGN1 9
// uc_orig: HOTEL_SIGN2 (fallen/Headers/building.h)
#define HOTEL_SIGN2 10

// =====================================================================
// Building data structures (binary-format-compatible)
// =====================================================================

// Object geometry reference within a building.
// uc_orig: BuildingObject (fallen/Headers/building.h)
struct BuildingObject {
    SWORD FacetHead;
    UWORD StartFace4;
    UWORD EndFace4;
    SWORD StartFace3;
    SWORD EndFace3;
    UWORD StartPoint;
    UWORD EndPoint;
};

// Runtime building wall facet: index ranges into global prim pools.
// uc_orig: BuildingFacet (fallen/Headers/building.h)
struct BuildingFacet {
    UWORD StartFace4;
    UWORD MidFace4;
    UWORD EndFace4;
    SWORD StartFace3;
    SWORD EndFace3;
    UWORD StartPoint;
    UWORD MidPoint;
    UWORD EndPoint;
    SWORD NextFacet;
    UWORD FacetFlags;
    SWORD ColVect;
};

// Roof bounding box (tile coordinates, used for roof culling).
// uc_orig: BoundBox (fallen/Headers/building.h)
struct BoundBox {
    UBYTE MinX;
    UBYTE MaxX;
    UBYTE MinZ;
    UBYTE MaxZ;
    SWORD Y;
};

// Editor scratch building.
// uc_orig: TempBuilding (fallen/Headers/building.h)
struct TempBuilding {
    UWORD FacetHead;
    UWORD FacetCount;
};

// Editor scratch facet.
// uc_orig: TempFacet (fallen/Headers/building.h)
struct TempFacet {
    SLONG x1, z1, x2, z2;
    SWORD Y;
    UWORD PrevFacet;
    UWORD NextFacet;
    UWORD RoofType;
    UWORD StoreyHead;
    UWORD StoreyCount;
};

// Editor scratch storey.
// uc_orig: TempStorey (fallen/Headers/building.h)
struct TempStorey {
    UWORD StoreyFlags;
    UBYTE WallStyle;
    UBYTE WindowStyle;
    SWORD Height;
    SWORD Next;
    SWORD Count;
};

// Packed texture page/UV/flip descriptor (integer UV).
// uc_orig: TXTY (fallen/Headers/building.h)
#pragma pack(push, 1)
struct TXTY {
    UBYTE Page, Tx, Ty, Flip;
};
#pragma pack(pop)
static_assert(sizeof(TXTY) == 4, "TXTY: binary file layout");

// Packed texture page/flip descriptor (16-bit UV).
// uc_orig: DXTXTY (fallen/Headers/building.h)
struct DXTXTY {
    UWORD Page;
    UWORD Flip;
};

// Texture type descriptor used for wall style lookup.
// uc_orig: TextureInfo (fallen/Headers/building.h)
struct TextureInfo {
    UBYTE Type;
    UBYTE SubType;
};

// Window descriptor attached to a wall segment.
// uc_orig: FWindow (fallen/Headers/building.h)
struct FWindow {
    UWORD Dist;
    UWORD Height;
    UWORD WindowFlags;
    UWORD WindowWidth;
    UWORD WindowHeight;
    SWORD Next;
    UWORD Dummy[6];
};

// Wall segment: defines one side of a storey's footprint.
// uc_orig: FWall (fallen/Headers/building.h)
struct FWall {
    SWORD DX, DZ;
    UWORD WallFlags;

    SWORD TextureStyle2;
    UWORD TextureStyle;

    UWORD Tcount2;
    SWORD Next;

    UWORD DY;
    UWORD StoreyHead;

    UBYTE* Textures;
    UWORD Tcount;
    UBYTE* Textures2;

    UWORD Dummy[1];
};

// Storey/section of a building.
// uc_orig: FStorey (fallen/Headers/building.h)
struct FStorey {
    SWORD DX, DY, DZ;
    UBYTE StoreyType;
    UBYTE StoreyFlags;

    UWORD Height;
    SWORD WallHead;

    UWORD ExtraFlags;

    UWORD InsideIDIndex;

    SWORD Next;
    SWORD Prev;
    SWORD Info1;
    SWORD Inside;
    UWORD BuildingHead;

    UWORD InsideStorey;
};

// Maximum rooms/stairs per floor interior.
// uc_orig: MAX_ROOMS_PER_FLOOR (fallen/Headers/building.h)
#define MAX_ROOMS_PER_FLOOR 16
// uc_orig: MAX_STAIRS_PER_FLOOR (fallen/Headers/building.h)
#define MAX_STAIRS_PER_FLOOR 10

// Interior room layout descriptor.
// uc_orig: RoomID (fallen/Headers/building.h)
struct RoomID {
    UBYTE X[MAX_ROOMS_PER_FLOOR], Y[MAX_ROOMS_PER_FLOOR];
    UBYTE Flag[MAX_ROOMS_PER_FLOOR];

    UBYTE StairsX[MAX_STAIRS_PER_FLOOR];
    UBYTE StairsY[MAX_STAIRS_PER_FLOOR];
    UBYTE StairFlags[MAX_STAIRS_PER_FLOOR];

    UBYTE FloorType;

    UBYTE Dummy[5 * 4];
};

// Building instance: root of the storey linked list.
// uc_orig: FBuilding (fallen/Headers/building.h)
struct FBuilding {
    UWORD ThingIndex;
    UWORD LastDrawn;
    UBYTE Dummy2;
    UBYTE Foundation;
    SWORD OffsetY;
    UWORD InsideSeed;
    UBYTE MinFoundation;
    UBYTE ExtraFoundation;
    UWORD BuildingFlags;
    SWORD StoreyHead;
    SWORD Angle;
    UWORD StoreyCount;
    CBYTE str[20];
    UBYTE StairSeed;
    UBYTE BuildingType;
    UWORD Walkable;
    UWORD Dummy[4];
};

#endif // BUILDINGS_BUILDING_TYPES_H
