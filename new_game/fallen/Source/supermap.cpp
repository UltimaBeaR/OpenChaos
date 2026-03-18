// claude-ai: supermap.cpp — high-level city map acceleration structure and level serialisation.
// claude-ai: "Supermap" = the pre-built spatial index of all buildings, facets and walkables.
// claude-ai: Loaded from the .pam level file; see load_super_map() / save_super_map().
// claude-ai:
// claude-ai: Key data structures (defined in Headers/building.h and Headers/supermap.h):
// claude-ai:   DBuilding   — one logical building; contains StartFacet..EndFacet range.
// claude-ai:   DFacet      — one exterior wall face (see facet.cpp for rendering).
// claude-ai:   DStorey     — per-storey texture info (painted textures override).
// claude-ai:   DWalkable   — roof walkable surface quads.
// claude-ai:   InsideStorey— interior floor data (bounding box, room map, stair links).
// claude-ai:   Staircase   — stair connection between InsideStoreys.
// claude-ai:   inside_block[]— 1-byte room-id per mapsquare, packed per InsideStorey.
// claude-ai:
// claude-ai: Spatial index for collision/AI queries:
// claude-ai:   PAP_2LO(mx,mz).ColVectHead → singly-linked list of facet indices touching this mapsquare.
// claude-ai:   facet_links[] — the flat array that the ColVectHead lists index into.
// claude-ai:   Negative facet index = last entry for this mapsquare (sentinel convention).
// claude-ai:
// claude-ai: Key functions:
// claude-ai:   create_dfacets_for_building() — converts editor building data → DFacet list
// claude-ai:   add_dfacet()                  — appends one DFacet and registers it in the spatial grid
// claude-ai:   find_electric_fence_dbuilding()— spatial query: nearest electrified fence to a point
// claude-ai:   set_electric_fence_state()    — set FACET_FLAG_ELECTRIFIED on all facets of a dbuilding
// claude-ai:   SUPERMAP_counter_increase()   — wraps a per-facet counter used for spatial de-dup
// claude-ai:   load_super_map() / save_super_map() — level binary serialisation
// claude-ai:   create_inside_rect()          — builds InsideStorey from editor storey data
// claude-ai:   calc_inside_for_xyz()         — spatial query: which InsideStorey contains a world point

#include "game.h"
#include "..\headers\supermap.h"
#include "..\headers\pap.h"
#include "..\headers\inside2.h"
#include "..\headers\ob.h"
#include "memory.h"
#include "mav.h"
#include "noserver.h"
#include "..\ddengine\headers\texture.h"
#include "..\headers\env.h"

extern UBYTE roper_pickup;

/*
"Performance Analyser"  -PA

This piece of kit really is the most fantastic programming tool that has ever existed on any platform.
I really can't complement it enough.

Programmers thrive on information, games development is sort of like having a black box with mysterious inner workings, you change the stuff you put into the box (the code) then you watch the different results that get displayed on the screen. And you have to deduce what's going on in the black box from this poke it and see what happens sort of trial and error. Of course it's not quite this bad, as the black box usually has lots of documentation about what's inside it. But when you cosider the black box that is the playstation contains multiple processors all doing millions of things a second, and all interacting with each other you can see that no matter how much documentation you have it's not going to tell you what your particular bit of code is doing.

And this is where the PA comes in.

It basically tells you exactly what each part of the playstation is doing in time steps of the order of a millionth of a second and it does it in nice colour coded histograms that let you zoom in and out without loss of information. At its furthest zoom out, (seeing the whole of a gameturn on screen at once) you can see exactly how much work the polygon drawing hardware (gpu)  was doing compared to the processor. The perfect game would have both running to capacity, with neither waiting for the other. So the PA lets you tweak the code until it as nearly as possible reaches this state of perfection.

The PA has a few other nifty features, it's operated by a foot peddle, so to the uninitialised it looks like your operating it by mind control. The PA is constantly storing information about the running of your program ( or even released games, so you can have a snoop on the opposition)  and when you press the trigger it dumps the information it has stored so far (upto about 8 60ths of a second worth), so when you get a glitch onscreen you press the trigger and it dumps the information showing you what lead upto the glitch (and hopefully the cause).
Another cool trick it has, is it can show you polygon overwrite, it shows you how many times each on screen pixel has been written or read. Everytime a polygon causes a pixel to be drawn over a previously drawn pixel, your wasting gpu time, minimal polygon overwrite is another of these states of perfection a good engine strives to achieve.

The PA easily doubled the speed of Urban Chaos on the PSX after just 3 weeks of use.


The only other trick up the playstation sleave is it's age means that the are a lot of very experienced programmers out there who are capable of pushing it to the limit.

The dreamcast could run at very high resolutions, but the great equaliser is the NTSC/PAL/SECAM television standards which limit the resolutions of consoles  to 640x256 (higher than this requires interlace mode which looks nasty) So on screen resolution the PSX,N64 and Dreamcast are playing on a level pitch (until HD TV arrives)

Mike.

P.S I hope this is the sort of thing you wanted, I also hope I haven't crossed any Sony NDA boundaries



This piece of kit really is the most fantastic programming tool that has ever existed on any platform.
I really can't complement it enough.

Programmers thrive on information, games development is sort of like having a black box with mysterious inner workings, you change the stuff you put into the box (the code) then you watch the different results that get displayed on the screen. And you have to deduce what's going on in the black box from this poke it and see what happens sort of trial and error. Of course it's not quite this bad, as the black box usually has lots of documentation about what's inside it. But when you cosider the black box that is the playstation contains multiple processors all doing millions of things a second, and all interacting with each other you can see that no matter how much documentation you have it's not going to tell you what your particular bit of code is doing.

And this is where the PA comes in.

It basically tells you exactly what each part of the playstation is doing in time steps of the order of a millionth of a second and it does it in nice colour coded histograms that let you zoom in and out without loss of information. At its furthest zoom out, (seeing the whole of a gameturn on screen at once) you can see exactly how much work the polygon drawing hardware (gpu)  was doing compared to the processor. The perfect game would have both running to capacity, with neither waiting for the other. So the PA lets you tweak the code until it as nearly as possible reaches this state of perfection.

The PA has a few other nifty features, it's operated by a foot peddle, so to the uninitialised it looks like your operating it by mind control. The PA is constantly storing information about the running of your program ( or even released games, so you can have a snoop on the opposition)  and when you press the trigger it dumps the information it has stored so far (upto about 8 60ths of a second worth), so when you get a glitch onscreen you press the trigger and it dumps the information showing you what lead upto the glitch (and hopefully the cause).
Another cool trick it has, is it can show you polygon overwrite, it shows you how many times each on screen pixel has been written or read. Everytime a polygon overwrites
  */

SLONG find_connect_wall(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG* connect_storey, SLONG storey);
SLONG add_dfacet(SLONG x1, SLONG z1, SLONG x2, SLONG z2, SLONG y, SLONG count, SLONG style_index, SLONG storey_type, SLONG flags, SLONG offsety, SLONG block_height);
SLONG add_painted_textures(UBYTE* t, SLONG tcount, SLONG style);

ULONG level_index = 0;

//
// temporary!
//
// UBYTE	inside_mem[MAX_INSIDE_MEM];

SLONG next_dbuilding = 1;
SLONG next_dwalkable = 1;
SLONG next_dfacet = 1;
SLONG next_dstyle = 1;
SWORD next_facet_link = 1;
SWORD next_paint_mem = 1;
SWORD next_dstorey = 1;
SWORD facet_link_count = 0;

SLONG next_inside_mem = 1;
// SLONG	next_inside_rect=1;

UBYTE SUPERMAP_counter[2];
extern SLONG TEXTURE_set;

#define NO_MOVE_UP 1
#define NO_MOVE_RIGHT 2
#define NO_MOVE_DOWN 4
#define NO_MOVE_LEFT 8
#define DOOR_UP 16
#define DOOR_RIGHT 32
#define DOOR_DOWN 64
#define DOOR_LEFT 128

//
// returns inside_index && room id for position in world
//
// claude-ai: calc_inside_for_xyz — spatial query: which InsideStorey contains world point (x,y,z)?
// claude-ai: Linear scan through all inside_storeys[] (up to next_inside_storey).
// claude-ai: Checks bounding box (MinX..MaxX, MinZ..MaxZ) then Y band (StoreyY..StoreyY+256).
// claude-ai: If in range, calls find_inside_room() to check the room map at (mx,mz).
// claude-ai: Returns inside_storey index (or 0 = not indoors); also fills *room.
// claude-ai: Called every tick by game logic to determine if player/NPC is inside a building.
// claude-ai: O(N) scan — N = total number of inside storeys in the level. Adequate for ~50 storeys.
UWORD calc_inside_for_xyz(SLONG x, SLONG y, SLONG z, UWORD* room)
{
    SLONG c0;
    SLONG mx, mz;
    mx = x >> 8;
    mz = z >> 8;
    for (c0 = 1; c0 < next_inside_storey; c0++) {
        if (mx >= inside_storeys[c0].MinX)
            if (mx < inside_storeys[c0].MaxX)
                if (mz >= inside_storeys[c0].MinZ)
                    if (mz < inside_storeys[c0].MaxZ) {
                        //
                        // in x,z range
                        //

                        if (y > inside_storeys[c0].StoreyY && y < inside_storeys[c0].StoreyY + 256) {
                            SLONG r;

                            r = find_inside_room(c0, mx, mz);

                            if (r) {
                                *room = r;
                                return (c0);
                            }
                        }
                    }
    }

    *room = 0;
    return (0);
}

SLONG first_walkable_prim_point;
SLONG number_of_walkable_prim_points;

SLONG first_walkable_prim_face4;
SLONG number_of_walkable_prim_faces4;

void load_walkables(MFFileHandle handle, SLONG save_type)
{
    SLONG next_point = 1, next_face4 = 1, next_face3 = 1;

    SLONG c0, c1;

    FileRead(handle, &next_dwalkable, 2);
    FileRead(handle, &next_roof_face4, 2);
    FileRead(handle, &dwalkables[0], sizeof(struct DWalkable) * next_dwalkable);
    FileRead(handle, &roof_faces4[0], sizeof(struct RoofFace4) * next_roof_face4);

    /*
            FileRead(handle,&next_dwalkable,2);
            FileRead(handle,&next_point,2);
            FileRead(handle,&next_face3,2);
            FileRead(handle,&next_face4,2);
            FileRead(handle,&dwalkables[0],sizeof(struct DWalkable)*next_dwalkable);


            if(save_type>=22)
            {
                    FileRead(handle,&prim_points[next_prim_point],sizeof(struct PrimPoint)*next_point);
            }
            else
            {
                    struct	OldPrimPoint	pp;
                    SLONG	c0;

                    for(c0=0;c0<next_point;c0++)
                    {

                            FileRead(handle,(UBYTE*)&pp,sizeof(struct	OldPrimPoint));
                            prim_points[next_prim_point+c0].X=(SWORD)pp.X;
                            prim_points[next_prim_point+c0].Y=(SWORD)pp.Y;
                            prim_points[next_prim_point+c0].Z=(SWORD)pp.Z;
                    }
            }


            FileRead(handle,&prim_faces3[next_prim_face3],sizeof(struct PrimFace3)*next_face3);
            FileRead(handle,&prim_faces4[next_prim_face4],sizeof(struct PrimFace4)*next_face4);

            for(c0=0;c0<next_face3;c0++)
            {
                    for(c1=0;c1<3;c1++)
                            prim_faces3[c0+next_prim_face3].Points[c1]+=next_prim_point;
            }

            for(c0=0;c0<next_face4;c0++)
            {
                    for(c1=0;c1<4;c1++)
                            prim_faces4[c0+next_prim_face4].Points[c1]+=next_prim_point;
                    prim_faces4[c0+next_prim_face4].FaceFlags&=~FACE_FLAG_OUTLINE;
            }

            for(c0=1;c0<next_dwalkable;c0++)
            {
                    SLONG	face;
                    dwalkables[c0].StartPoint+=next_prim_point;
                    dwalkables[c0].EndPoint+=next_prim_point;

                    dwalkables[c0].StartFace3+=next_prim_face3;
                    dwalkables[c0].EndFace3+=next_prim_face3;

                    dwalkables[c0].StartFace4+=next_prim_face4;
                    dwalkables[c0].EndFace4+=next_prim_face4;

                    for(face=dwalkables[c0].StartFace4;face<dwalkables[c0].EndFace4;face++)
                    {
                            for(c1=0;c1<4;c1++)
                            {
                                    ASSERT(prim_faces4[face].Points[c1]>=dwalkables[c0].StartPoint);
                                    ASSERT(prim_faces4[face].Points[c1]<=dwalkables[c0].EndPoint);

                            }
                    }

            }
    */

    first_walkable_prim_point = 0; // next_prim_point;
    number_of_walkable_prim_points = 0; // next_point;

    first_walkable_prim_face4 = 0; // next_prim_face4;
    number_of_walkable_prim_faces4 = 0; // next_face4;

    //	next_prim_point+=next_point;
    //	next_prim_face3+=next_face3;
    //	next_prim_face4+=next_face4;

    {
        struct RoofFace4* p_roof;
        SLONG c0;
        p_roof = &roof_faces4[0];

        for (c0 = 0; c0 < next_roof_face4; c0++) {
            if (p_roof->DY[0] || p_roof->DY[1] || p_roof->DY[2])
                p_roof->RZ |= (1 << 7);
            else
                p_roof->RZ &= 127;
            p_roof++;
        }
    }
}

// claude-ai: load_super_map — deserialises the precomputed supermap from the level file.
// claude-ai: Reads (in order): next_dbuilding, next_dfacet, next_dstyle, next_paint_mem,
// claude-ai:   next_dstorey, dbuildings[], dfacets[], dstyles[], paint_mem[], dstoreys[],
// claude-ai:   next_inside_storey, next_inside_stair, next_inside_block,
// claude-ai:   inside_storeys[], inside_stairs[], inside_block[], walkables, OB_ob[].
// claude-ai: save_type >= 17 required for painted texture data; >= 21 for inside data.
// claude-ai: Resets Dfcache to 0 for all loaded facets (lighting cache rebuilt on first draw).
// claude-ai: This is called from the main level load path — NOT the editor path.
void load_super_map(MFFileHandle handle, SLONG save_type)
{
    SLONG c0;

    next_dbuilding = 1;
    next_dfacet = 1;
    next_dstyle = 1;

    INDOORS_INDEX = 0;
    INDOORS_INDEX_NEXT = 0;

    FileRead(handle, &next_dbuilding, 2);
    FileRead(handle, &next_dfacet, 2);
    FileRead(handle, &next_dstyle, 2);

    if (save_type >= 17) {
        FileRead(handle, &next_paint_mem, 2);
        FileRead(handle, &next_dstorey, 2);
    }

    FileRead(handle, &dbuildings[0], sizeof(struct DBuilding) * next_dbuilding);
    FileRead(handle, &dfacets[0], sizeof(struct DFacet) * next_dfacet);
    FileRead(handle, &dstyles[0], sizeof(UWORD) * next_dstyle);
    if (save_type >= 17) {
        FileRead(handle, &paint_mem[0], sizeof(UBYTE) * next_paint_mem);
        FileRead(handle, &dstoreys[0], sizeof(struct DStorey) * next_dstorey);
    }

    for (c0 = 1; c0 < next_dfacet; c0++) {
        dfacets[c0].Dfcache = 0;
    }

    if (save_type >= 21) {
        FileRead(handle, &next_inside_storey, sizeof(next_inside_storey));
        FileRead(handle, &next_inside_stair, sizeof(next_inside_stair));
        FileRead(handle, &next_inside_block, sizeof(next_inside_block));

        FileRead(handle, &inside_storeys[0], sizeof(struct InsideStorey) * next_inside_storey);
        FileRead(handle, &inside_stairs[0], sizeof(struct Staircase) * next_inside_stair);
        FileRead(handle, &inside_block[0], sizeof(UBYTE) * next_inside_block);
    }
    load_walkables(handle, save_type);

    if (save_type >= 23) {
        /*		SLONG OB_ob_temp;
                        FileRead(handle,(UBYTE*)&OB_ob_temp,sizeof(OB_ob_upto));
                        FileSeek(handle,SEEK_MODE_CURRENT,sizeof(OB_Ob)*OB_ob_temp);
                        FileSeek(handle,SEEK_MODE_CURRENT,sizeof(OB_Mapwho)*OB_SIZE*OB_SIZE);
        */
        FileRead(handle, (UBYTE*)&OB_ob_upto, sizeof(OB_ob_upto));
        FileRead(handle, (UBYTE*)&OB_ob[0], sizeof(OB_Ob) * OB_ob_upto);

        //
        // Notice that strangely they have their very own mapwho
        //

        FileRead(handle, (UBYTE*)&OB_mapwho[0][0], sizeof(OB_Mapwho) * OB_SIZE * OB_SIZE);
    }
}

//
// build2.cpp doesn't have an h-file for it!
//

void add_facet_to_map(SLONG facet);

// claude-ai: add_sewer_ladder — creates a DFacet of type STOREY_TYPE_LADDER for sewer entry.
// claude-ai: FACET_FLAG_IN_SEWERS marks it as a sewer ladder (renders as DRAW_ladder).
// claude-ai: FACET_FLAG_LADDER_LINK additionally means: "this is the entry point to sewers"
// claude-ai:   (used to trigger the player transition from surface to sewer layer).
// claude-ai: Appended directly to dfacets[] (bypasses create_dfacets_for_building).
// claude-ai: add_facet_to_map() registers it in the PAP spatial grid for collision/AI queries.
void add_sewer_ladder(
    SLONG x1, SLONG z1,
    SLONG x2, SLONG z2,
    SLONG bottom,
    SLONG height,
    SLONG link)
{
    DFacet* df;

    if (!WITHIN(next_dfacet, 0, MAX_DFACETS - 1)) {
        //
        // No more facets left.
        //

        return;
    }

    //
    // Hmm...!
    //

    if (next_dfacet == 0) {
        next_dfacet = 1;
    }

    //
    // Create the dfacet.
    //

    df = &dfacets[next_dfacet];

    df->FacetType = STOREY_TYPE_LADDER;
    df->FacetFlags = FACET_FLAG_IN_SEWERS;
    df->x[0] = x1 >> 8;
    df->z[0] = z1 >> 8;
    df->x[1] = x2 >> 8;
    df->z[1] = z2 >> 8;
    df->Y[0] = bottom;
    df->Y[1] = bottom;
    df->Height = height;

    if (link) {
        df->FacetFlags |= FACET_FLAG_LADDER_LINK;
    }

    //
    // Add it to the map.
    //

    add_facet_to_map(next_dfacet);

    next_dfacet += 1;

    return;
}

/*
void create_super_dbuilding(SLONG building)
{
        next_dbuilding=1;
        next_dfacet=1;
        next_dstyle=1;

        create_dfacets_for_building(building);
}
*/

// #endif
//  claude-ai: find_electric_fence_dbuilding — spatial query: find closest electrified fence to point.
//  claude-ai: Uses PAP_2LO mapsquare grid for a broad-phase search within 'range' world units.
//  claude-ai: Iterates facet_links[] linked lists per mapsquare (PAP_2LO.ColVectHead chains).
//  claude-ai: Negative facet index = sentinel for end of list (sign flip convention).
//  claude-ai: Fine-phase: distance_to_line() for each candidate facet.
//  claude-ai: Returns the DBuilding index of the nearest fence, or NULL if none found.
//  claude-ai: Used by game logic to detect if a player/NPC touched an electric fence.
SLONG find_electric_fence_dbuilding(
    SLONG world_x,
    SLONG world_y,
    SLONG world_z,
    SLONG range)
{
    SLONG dist;

    SLONG best_dist = INFINITY;
    SLONG best_facet = NULL;

    SLONG mx;
    SLONG mz;

    SLONG mx1, mx2;
    SLONG mz1, mz2;

    SLONG f_list;
    SLONG facet;
    SLONG build;
    SLONG exit;

    mx1 = (world_x - range) >> PAP_SHIFT_LO;
    mz1 = (world_z - range) >> PAP_SHIFT_LO;
    mx2 = (world_x + range) >> PAP_SHIFT_LO;
    mz2 = (world_z + range) >> PAP_SHIFT_LO;

    SATURATE(mx1, 0, PAP_SIZE_LO - 1);
    SATURATE(mz1, 0, PAP_SIZE_LO - 1);
    SATURATE(mx2, 0, PAP_SIZE_LO - 1);
    SATURATE(mz2, 0, PAP_SIZE_LO - 1);

    for (mx = mx1; mx <= mx2; mx++)
        for (mz = mz1; mz <= mz2; mz++) {
            //
            // Check all the facets on this mapsquare.
            //

            f_list = PAP_2LO(mx, mz).ColVectHead;

            if (f_list) {
                exit = FALSE;

                while (!exit) {
                    facet = facet_links[f_list];

                    ASSERT(facet);

                    if (facet < 0) {
                        //
                        // The last facet in the list for each square
                        // is negative.
                        //

                        facet = -facet;
                        exit = TRUE;
                    }

                    dist = distance_to_line(
                        dfacets[facet].x[0] << 8,
                        dfacets[facet].z[0] << 8,
                        dfacets[facet].x[1] << 8,
                        dfacets[facet].z[1] << 8,
                        world_x,
                        world_z);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_facet = facet;
                    }

                    f_list++;
                }
            }
        }

    if (best_facet == NULL) {
        return NULL;
    } else {
        return dfacets[best_facet].Building;
    }
}
//
// Sets the state of the given electric fence dbuilding. It sets the
// flags in all the facets of the dbuilding.
//

// claude-ai: set_electric_fence_state — sets or clears FACET_FLAG_ELECTRIFIED on all facets of a DBuilding.
// claude-ai: Called by mission scripts to turn electric fences on or off at runtime.
// claude-ai: Iterates dfacets[StartFacet..EndFacet) for the given dbuilding index.
void set_electric_fence_state(SLONG dbuilding, SLONG onoroff)
{
    SLONG facet;

    for (facet = dbuildings[dbuilding].StartFacet; facet < dbuildings[dbuilding].EndFacet; facet++) {
        if (onoroff) {
            dfacets[facet].FacetFlags |= FACET_FLAG_ELECTRIFIED;
        } else {
            dfacets[facet].FacetFlags &= ~FACET_FLAG_ELECTRIFIED;
        }
    }
}

// claude-ai: SUPERMAP_counter_increase — increments one of 2 global frame counters.
// claude-ai: Each DFacet has DFacet.Counter[2] — when Counter[which] == SUPERMAP_counter[which]
// claude-ai:   the facet has already been "visited" this frame (de-duplication in spatial queries).
// claude-ai: On wrap-around (counter goes to 0), resets ALL facet counters to 0 to stay consistent.
// claude-ai: This is a standard "generation stamp" trick to avoid clearing N entries every frame.
// claude-ai: Used by AI pathfinding broad-phase to avoid processing the same facet twice.
void SUPERMAP_counter_increase(UBYTE which)
{
    SUPERMAP_counter[which] += 1;

    if (SUPERMAP_counter[which] == 0) {
        SLONG i;

        //
        // It has wrapped around. We must clear the counter in every dfacet.
        //

        for (i = 1; i < next_dfacet; i++) {
            dfacets[i].Counter[which] = 0;
        }

        SUPERMAP_counter[which] = 1;
    }
}

struct Levels {
    CBYTE* name;
    CBYTE* map_name;
    UWORD level;
    ULONG dontload;
};

struct Levels levels_quick[] = {
    //	{"finale1","Roof1.map",32},
    { "BAALROG3", "Balbash1.map", 31 },
    //	{"gang order1","gang1.map",33},
    { "", "", 0 }
};

struct Levels levels_demo[] = {
    { "FTutor1", "fight1.map", 1, 0 },
    { "freecd1", "freeoncd1.map", 1, 0 },
    { "", "", 0 }
};

struct Levels levels2[] = {
    { "skymiss2", "skymap30.map", 25, 0 }, // problem
    { "", "", 0 }
};

struct Levels levels[] = {

    { "FTutor1", "fight1.map", 1, 0 }, // 0

    { "assault1", "assault.map", 2, 0 }, // 1
    { "testdrive1a", "oval1.map", 3, 0 }, // 2
    { "fight1", "fight1.map", 4, 0 }, // 3
    { "police1", "disturb1.map", 5, 0 }, // 4
    { "testdrive2", "road4_2.map", 6, 0 }, // 5
    { "fight2", "fight1.map", 7, 0 }, // 6

    { "police2", "disturb1.map", 8, 0 }, // 7
    { "testdrive3", "road4_3.map", 9, 0 }, // 8
    { "bankbomb1", "gang1.map", 10, 0 }, // 9
    { "police3", "disturb1.map", 11, 0 }, // 10
    { "gangorder2", "gang1.map", 12, 0 }, // 11
    { "police4", "disturb1.map", 13, 0 }, // 12
    { "bball2", "bball2.map", 14, 0 }, // 13

    { "wstores1", "gpost3.map", 15, (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) }, // 14
    // here
    { "\\e3", "snocrime.map", 16, (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) }, // 15
    { "westcrime1", "disturb1.map", 17, 0 }, // 16
    { "carbomb1", "gang1.map", 18, 0 }, // 17
    { "botanicc", "botanicc.map", 19, 0 }, // 18
    { "semtex", "snocrime.map", 20, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MIB1) }, // 19
    { "awol1", "gpost3.map", 21, (1 << PERSON_SLAG_TART) | (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_MECHANIC) | (1 << PERSON_MIB1) }, // 20
    { "mission2", "snocrime.map", 22, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) | (1 << PERSON_MIB1) }, // 21
    { "park2", "parkypsx03.map", 23, 0 }, // 22

    { "mib", "southside.map", 24, (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) }, // 23
    { "skymiss2", "skymap30.map", 25, 0 }, // problem //24
    { "factory1", "tv1.map", 26, (1 << PERSON_SLAG_FATUGLY) | (1 << PERSON_HOSTAGE) | (1 << PERSON_TRAMP) }, // 25
    { "estate2", "nestate01.map", 27, 0 }, // 26
    { "stealtst1", "stealth1.map", 28, 0 }, // 27
    { "snow2", "snow2.map", 29, 0 },
    { "gordout1", "botanicc.map", 30, 0 },
    { "jung3", "jung3.map", 31, 0 },
    { "BAALROG3", "Balbash1.map", 32, 0 },
    { "finale1", "Rooftest2.map", 33, 0 }, // problem
    { "Gangorder1", "gang1.map", 34, 0 },
    { "", "", 0 }

};

//
// create PSX tims (oh my god)
//

UWORD pals16[256][64]; // a single page holds all pals

UWORD moved_from[16 * 64];
UWORD moved_to[16 * 64];

struct TimStuff {
    int Stb, ClutX, ClutY, PageX, PageY;
};

struct TimStuff tim_stuff[] = {
    { 0, 0, 500, 512 + 64 * 0, 0 },
    { 0, 0, 500, 512 + 64 * 1, 0 },
    { 0, 0, 500, 512 + 64 * 2, 0 },
    { 0, 0, 500, 512 + 64 * 3, 0 },
    { 0, 0, 500, 512 + 64 * 4, 0 },
    { 0, 0, 500, 512 + 64 * 5, 0 },
    { 0, 0, 500, 512 + 64 * 6, 0 },
    { 0, 0, 500, 512 + 64 * 7, 0 },

    { 0, 0, 500, 512 + 64 * 0, 256 },
    { 0, 0, 500, 512 + 64 * 1, 256 },
    { 0, 0, 500, 512 + 64 * 2, 256 },
    { 0, 0, 500, 512 + 64 * 3, 256 },
    { 0, 0, 500, 512 + 64 * 4, 256 },
    { 0, 0, 500, 512 + 64 * 5, 256 },
    { 0, 0, 500, 512 + 64 * 6, 256 },
    { 0, 0, 500, 512 + 64 * 7, 256 },

    { 1, 0, 501, 448, 0 },
    { 1, 0, 502, 576, 0 },
    { 1, 0, 503, 704, 0 },
    { 1, 0, 504, 832, 0 },
    { 1, 0, 505, 320, 256 },
    { 1, 0, 506, 448, 256 },
    { 1, 0, 507, 576, 256 },
    { 1, 0, 508, 704, 256 },
    { 1, 0, 509, 832, 256 }

};

SLONG load_alt_pal(char* fname, UBYTE* pal)
{
    FILE *handle, *phandle;
    UBYTE remap_pal[256 * 4];
    SLONG c0;

    //
    // Open the file.
    //

    phandle = MF_Fopen(fname, "rb");
    if (phandle == NULL) {
        goto file_error;
    }

    if (fread(&pal[0], 1, 24, phandle) != 24)
        goto file_error;

    if (fread(&remap_pal[0], 1, 256 * 4, phandle) != 256 * 4)
        goto file_error;

    {
        SLONG pal1 = 0, pal2 = 0, c0;
        for (c0 = 0; c0 < 16 * 4; c0++)
            pal1 += remap_pal[c0];

        for (c0 = 240 * 4; c0 < 256 * 4; c0++)
            pal2 += remap_pal[c0];

        if (pal2 > pal1) {

            //
            // shunt the pallette down
            //
            for (c0 = 0; c0 < 16 * 4; c0++) {
                remap_pal[c0] = remap_pal[240 * 4 + c0];
            }
        }
    }

    for (c0 = 0; c0 < 256; c0++) {
        pal[c0 * 3 + 0] = remap_pal[c0 * 4 + 0];
        pal[c0 * 3 + 1] = remap_pal[c0 * 4 + 1];
        pal[c0 * 3 + 2] = remap_pal[c0 * 4 + 2];
    }

    return (1);
file_error:;
    return (0);
}

void save_tim(char* fname, unsigned char dat[256][128], SLONG index, SLONG copy_tom)
{
    FILE* handle;
    SLONG x, y;

    fname[strlen(fname) - 2] = 'o';

    handle = MF_Fopen(fname, "wb");
    if (handle) {
        fwrite(&dat[0][0], 1, 256 * 128, handle);
        MF_Fclose(handle);
    }

    fname[strlen(fname) - 2] = 'i';

    handle = MF_Fopen(fname, "wb");

    struct
    {
        SLONG ID;
        SLONG Flag;
    } clut_head;

    if (handle) {
        SLONG bnum;

        clut_head.ID = 0x10;
        clut_head.Flag = 0; // 4 bit no cluts in tim.   //9; //0x10|1; // has clut & clut is 8bit
        fwrite(&clut_head, 1, sizeof(clut_head), handle);

        bnum = 12 + 256 * 256;
        fwrite(&bnum, 1, sizeof(bnum), handle); // number of bytes
        bnum = ((tim_stuff[index].PageY) << 16) + tim_stuff[index].PageX;
        fwrite(&bnum, 1, sizeof(bnum), handle); // dy dx
        bnum = (256 << 16) + 64; // 64 16bit = 256 4 bits
        fwrite(&bnum, 1, sizeof(bnum), handle); // h  w

        for (y = 0; y < 256; y += 1)
            for (x = 0; x < 128; x += 4) {
                SLONG data;

                data = (dat[y][x + 3] << 24) + (dat[y][x + 2] << 16) + (dat[y][x + 1] << 8) + (dat[y][x] << 0);
                fwrite(&data, 1, sizeof(data), handle); // h  w
            }

        MF_Fclose(handle);
    }
}

void save_tim_pal16(char* fname, UWORD index, SLONG dy, SLONG height)
{
    FILE* handle;
    int x, y;

    ASSERT(height < 256);
    handle = MF_Fopen(fname, "wb");

    struct
    {
        int ID;
        int Flag;
    } clut_head;

    if (handle) {
        int bnum;

        clut_head.ID = 0x10;
        clut_head.Flag = 2; // 2=15 bit ; // 4 bit no cluts in tim.   //9; //0x10|1; // has clut & clut is 8bit
        fwrite(&clut_head, 1, sizeof(clut_head), handle);

        bnum = 12 + 256 * height;
        fwrite(&bnum, 1, sizeof(bnum), handle); // number of bytes
        bnum = ((tim_stuff[index].PageY + dy) << 16) + tim_stuff[index].PageX;
        fwrite(&bnum, 1, sizeof(bnum), handle); // dy dx
        bnum = (height << 16) + 64; // 64 16bit = 256 4 bits
        fwrite(&bnum, 1, sizeof(bnum), handle); // h  w

        for (y = 0; y < height; y += 1)
            for (x = 0; x < 128; x += 4) {
                int data;

                data = (pals16[y + dy][(x + 2) >> 1] << 16) + (pals16[y + dy][x >> 1]); //(dat[y][x+3]<<24)+(dat[y][x+2]<<16)+(dat[y][x+1]<<8)+(dat[y][x]<<0);
                fwrite(&data, 1, sizeof(data), handle); // h  w
            }

        MF_Fclose(handle);
    }
    //
    // tom file is raw data
    //
    fname[strlen(fname) - 2] = 'o';
    handle = MF_Fopen(fname, "wb");
    if (handle) {
        fwrite(&pals16[0][0], 2, 256 * 64, handle);
        MF_Fclose(handle);
    }
}

SLONG get_level_no(CBYTE* name)
{
    SLONG p0, p1, c0 = 0;

    roper_pickup = 0;

    while (levels[c0].level) {
        p0 = strlen(name) - 5; // last letter of name
        p1 = strlen(levels[c0].name) - 1;
        for (; p1 >= 0; p1--, p0--) {
            CBYTE comp;
            if (tolower(name[p0]) != tolower(levels[c0].name[p1]))
                break;
        }
        if (p1 < 0) {
            //			if(levels[c0].level==33)
            //				roper_pickup=1;

            level_index = c0;

            return (levels[c0].level);
        }
        c0++;
    }
    return (0);
}

BOOL make_texture_clumps(CBYTE* mission_name);
extern void TesterText(CBYTE* error, ...);
extern CBYTE ELEV_fname_level[];

// SLONG    MAV_opt_upto;

SWORD people_types[50];
ULONG DONT_load = 0; // nice global used for people loading
void save_all_nads(void)
{
}

extern int TEXTURE_create_clump;

void make_all_clumps(void)
{

    SLONG p0, p1, c0 = 0;
    SLONG highest = 0;

    TEXTURE_create_clump = 1;

    TEXTURE_load_needed("levels\\frontend.ucm", 0, 256, 40);

    Levels* lptr = levels; //_demo;

    while (lptr[c0].level) {

        CBYTE name[100];
        sprintf(name, "levels\\%s.ucm", lptr[c0].name);
        make_texture_clumps(name);
        if (MAV_opt_upto > highest) {
            highest = MAV_opt_upto;
        }
        c0++;
    }

    exit(0);
}
