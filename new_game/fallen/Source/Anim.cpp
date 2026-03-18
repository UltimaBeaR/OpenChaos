// claude-ai: FILE OVERVIEW — Anim.cpp (fallen/Source/Anim.cpp)
// claude-ai: Handles the character animation system: loading .all animation files,
// claude-ai: building the global_anim_array[], and managing GameKeyFrameChunk data.
// claude-ai:
// claude-ai: KEY DATA STRUCTURES:
// claude-ai:   GameKeyFrameChunk — one per character type (Darci, Roper, CIV/Thug, Roper2).
// claude-ai:     .AnimKeyFrames[] — flat array of all keyframes for this character type.
// claude-ai:     .AnimList[]      — array of pointers into AnimKeyFrames by animation index.
// claude-ai:     .TheElements[]   — per-bone transform data (GameKeyFrameElement per bone per frame).
// claude-ai:     .FightCols[]     — fight collision boxes per keyframe.
// claude-ai:     .MultiObject[]   — prim_multi_object indices for character mesh variants.
// claude-ai:
// claude-ai:   global_anim_array[ANIM_TYPE][ANIM_*] — 2D table mapping character type × action
// claude-ai:   to a specific keyframe list pointer. ANIM_TYPE_DARCI=0, ANIM_TYPE_ROPER=1,
// claude-ai:   ANIM_TYPE_CIV=2, ANIM_TYPE_ROPER2=3. ANIM_* constants are in animate.h.
// claude-ai:
// claude-ai: .ALL FILE FORMAT (binary, loaded by load_anim_system()):
// claude-ai:   SLONG anim_count (negative = has version + body_part_info header)
// claude-ai:   Then anim_count × Anim records.
// claude-ai:   Each Anim: name, flags, frame_count, optional speed byte (version>3),
// claude-ai:   then frame_count × (chunk_id, frame_id, tween_step, fixed, fight_cols).
// claude-ai:
// claude-ai: COMPRESSED MATRIX FORMAT (GameKeyFrameElement / ULTRA_COMPRESSED_ANIMATIONS):
// claude-ai:   Each bone orientation stored as 3x3 rotation matrix, compressed:
// claude-ai:   Only 2 of 3 values in each row stored; 3rd reconstructed via sqrt(1 - a² - b²).
// claude-ai:   Pad byte encodes which element is the largest (reconstructed from the others).
// claude-ai:   OffsetX/Y/Z: bone offset from parent in 8-bit values.
// claude-ai:
// claude-ai: setup_people_anims():
// claude-ai:   Loads .all files: darci1.all, roper.all (or psxroper.all), rthug.all.
// claude-ai:   Appends: police1.all → Roper chunk at index 200+.
// claude-ai:   Appends: newciv.all → CIV chunk at CIV_M_START; newcivf.all at CIV_F_START.
// claude-ai:   Level-specific: banesuit.all on estate2/Album1 levels; trainer.all in tutorial.
// claude-ai:
// claude-ai: setup_global_anim_array():
// claude-ai:   Fills global_anim_array[][] by mapping ANIM_* constants to specific .all indices.
// claude-ai:   Initially all types default to Darci's animations (fallback).
// claude-ai:   Then per-character overrides are applied (Roper pistol, shotgun, AK47, etc.)
// claude-ai:
// samplmanager.c.h
// mfstdlib.h

// reference thingno
//  sample index
//  volume sample_vol_max,
//  pan sample_pan_center
//  freq sample_freq_orig, signed offset from base
//  priority slong
/*
SAVE INGAME
 store data 4
 Pap_Hi -> 98304   tot 98304 (16384)
 Pap_Lo -> 8192   tot 106496 (1024)
 net_peep -> 40   tot 106536 (10)
 net_plyr -> 40   tot 106576 (10)
 f-links -> 11450   tot 118026 (5725/32000)
 dbuildings -> 4848   tot 122874 (202/1024)
 dfacets -> 76180   tot 199054 (2930/16384)
 dwalkables -> 6468   tot 205522 (294/2048)
 dstyles -> 9356   tot 214878 (4678/10000)
 dstoreys -> 3360   tot 218238 (560/10000)
 paintmem -> 2446   tot 220684 (2446/64000)
 insideStoreys -> 110   tot 220794 (5/2000)
 insideStairs -> 20   tot 220814 (2/8000)
 insideblock -> 709   tot 221523 (709/64000)
 roof bounds -> 6   tot 221529 (1/2000)
 prim_points -> 99930   tot 321459 (16655/65000)
 prim_faces4 -> 195330   tot 516789 (5745/32760)
 prim_faces3 -> 151928   tot 668717 (5426/32000)
 prim_objects -> 7536   tot 676253 (471/2000)
 prim_Mobjects -> 120   tot 676373 (15/100)
 ob_ob -> 9296   tot 685669 (1162/2048)
 ob_ mapwho -> 2048   tot 687717 (1024)
 EWAY_mess -> 492   tot 688209 (123/128)
 EWAY_mess buf -> 4457   tot 692666 (4457/8192)
 EWAY_timer -> 0   tot 692666 (32)
 EWAY_cond -> 114   tot 692780 (19/128)
 EWAY_way -> 15136   tot 707916 (473/512)
 EWAY_edef -> 1488   tot 709404 (124/150)
 vehicles -> 9024   tot 718428 (48)
 people -> 21600   tot 740028 (150)
 animals -> 120   tot 740148 (6)
 choppers -> 336   tot 740484 (4)
 pyro -> 5376   tot 745860 (64)
 players -> 1216   tot 747076 (8)
 projectiles -> 240   tot 747316 (10)
 special -> 2880   tot 750196 (80)
 switches -> 440   tot 750636 (10)
 bats -> 1024   tot 751660 (32)
 thing -> 45120   tot 796780 (752)
 drawtween -> 9360   tot 806140 (180)
 drawmesh -> 5000   tot 811140 (500)
 bike -> 640   tot 811780 (8)
 barrelsphere -> 4200   tot 815980 (150)
 barrels -> 354   tot 816334 (59/150)
 plat -> 12   tot 816346 (1/32)
 wmove -> 4608   tot 820954 (192)
 mav_opt -> 2820   tot 823774 (705/1024)
 mav_nav -> 32768   tot 856542 (16384)
 road_noads -> 150   tot 856692 (25/256)
 balloons -> 104   tot 856796 (1/32)
 tracks -> 9600   tot 866396 (300)
 roofface4 -> 38900   tot 905296 (3890/10000)
 fastnav -> 2048   tot 907344 (2048)
 night_slight -> 1968   tot 909312 (246/256)
 night_smap -> 2048   tot 911360 (1024)
 night_dlight -> 768   tot 912128 (64)
 WARE_ware -> 32   tot 912160 (1/32)
 WARE_nav -> 0   tot 912160 (0/4096)
 WARE_height -> 0   tot 912160 (0/8192)
 WARE_rooftex -> 0   tot 912160 (0/4096)
 Trip_Wire -> 14   tot 912174 (1/32)
 Road_edges -> 0   tot 912174 (0/8)
 Thing_heads -> 38   tot 912212 (19)
 psx_remap -> 256   tot 912468 (128)
 psx_tex_xy -> 2000   tot 914468 (1000)
*/

// #include	"Editor.hpp"

#include "stdio.h"
#include "game.h"
// #include	"engine.h"
#include "animate.h"
#include "cop.h"
#include "io.h"
// #include	"prim_draw.h"
#include "memory.h"

// poostruct GameKeyFrameElement	gamekeyframeelements[MAX_NUMBER_OF_ELEMENTS];

SLONG build_psx = 0; // save_psx
extern SLONG save_psx;

// claude-ai: NROPER_ANIM_* constants — indices into game_chunk[ANIM_TYPE_ROPER].AnimList[]
// claude-ai: for the "new Roper" animation set (roper.all, PC-only non-PSX path).
// claude-ai: These are used in setup_global_anim_array() to map ANIM_* game actions
// claude-ai: to specific numbered animations within the roper.all file.
// claude-ai: The numbering (1, 2, 3...) corresponds to the animation's position in the .all file.
// claude-ai: PSX build uses a different set (psxroper.all) with different indices.
#define NROPER_ANIM_YOMP (1)
#define NROPER_ANIM_STAND_READY (2)
#define NROPER_ANIM_BREATHE (3)
#define NROPER_ANIM_RUNNING_JUMP (4)
#define NROPER_ANIM_RUNNING_JUMP_FLY (5)
#define NROPER_ANIM_RUNNING_JUMP_LAND (6)
#define NROPER_ANIM_RUNNING_JUMP_LAND_RUN (7)
#define NROPER_ANIM_DRAW_GUN (8)
#define NROPER_ANIM_GUN_AIM (9)
#define NROPER_ANIM_GUN_SHOOT (10)
#define NROPER_ANIM_LISTEN (11)
#define NROPER_ANIM_JUMP_SPOT_TAKEOFF (12)
#define NROPER_ANIM_JUMP_SPOT_LAND (13)
#define NROPER_ANIM_JUMP_SPOT_STATIC (14)
#define NROPER_ANIM_TELL (15)
#define NROPER_ANIM_SHOTGUN_TAKEOUT (16)
#define NROPER_ANIM_SHOTGUN_SHOOT (17)
// #define NROPER_ANIM_SHOTGUN_AIM				(18)
#define NROPER_ANIM_SWIG_FLASK (19)
#define NROPER_ANIM_YOMP_START (20)
#define NROPER_ANIM_AK_TAKEOUT (21)
#define NROPER_ANIM_AK_SHOOT (22)
#define NROPER_ANIM_AK_AIM (23)

#define NROPER_ANIM_AK_AIM_L (24)
#define NROPER_ANIM_AK_AIM_R (25)
#define NROPER_ANIM_SHOTGUN_AIM (26)
#define NROPER_ANIM_SHOTGUN_AIM_L (27)
#define NROPER_ANIM_SHOTGUN_AIM_R (28)
#define NROPER_ANIM_GUN_AIM_L (30)
#define NROPER_ANIM_GUN_AIM_R (31)

#define NROPER_ANIM_WALK_BACKWARDS (46)
#define NROPER_ANIM_RUN_WITH_AK (47)
#define NROPER_ANIM_WALK_BACKWARDS_WITH_AK (48)

#define NROPER_PICKUP_CARRY (34)
#define NROPER_START_WALK_CARRY (35)
#define NROPER_WALK_CARRY (36)
#define NROPER_WALK_STOP_CARRY (37)
#define NROPER_PUTDOWN_CARRY (38)

#define NROPER_PICKUP_CARRY_V (39)
#define NROPER_START_WALK_CARRY_V (40)
#define NROPER_WALK_CARRY_V (41)
#define NROPER_WALK_STOP_CARRY_V (42)
#define NROPER_PUTDOWN_CARRY_V (43)

#define NROPER_STAND_CARRY (44)
#define NROPER_STAND_CARRY_V (45)

#define NROPER_TO_WALL (49)
#define NROPER_TO_WALL_STATIC (65)
#define NROPER_TO_WALL_SHOTGUN (50)
#define NROPER_ALONG_WALL (51)
#define NROPER_ALONG_WALL_B (52)
#define NROPER_ALONG_WALL_SHOTGUN (53)
#define NROPER_ALONG_WALL_SHOTGUN_B (54)
#define NROPER_ALONG_WALL_SHOTGUN_C (61)
#define NROPER_ALONG_WALL_SHOTGUN_D (62)
#define NROPER_AIM_WALL_SHOTGUN (55)
#define NROPER_AIM_WALL_SHOTGUN_B (56)
#define NROPER_AIM_WALL_PISTOL (57)
#define NROPER_AIM_WALL_PISTOL_B (63)
#define NROPER_AIM_WALL_PISTOL_C (64)
#define NROPER_FIGHT_WALL (60)

// claude-ai: NROPER_TWO_PISTOL_* — Roper's dual-pistol animation set.
// claude-ai: These are PC-only (not present in PSX/psxroper.all builds).
// claude-ai: The dual-pistol system uses: DRAW(76), FIRE(77), AWAY(78), RUN(72), AIM(90),
// claude-ai:   AIM_L(79), AIM_R(80), CROUCH(87), CROUCH_HOLD(88), CROUCH_STAND_UP(89).
// claude-ai: Mapped to ANIM_PISTOL_DRAW, ANIM_PISTOL_SHOOT, ANIM_PISTOL_JOG etc. in setup_global_anim_array.
#define NROPER_TWO_PISTOL_DRAW (76)
#define NROPER_TWO_PISTOL_FIRE (77)
#define NROPER_TWO_PISTOL_AWAY (78)
#define NROPER_TWO_PISTOL_RUN (72)
#define NROPER_TWO_PISTOL_AIM (90)
#define NROPER_TWO_PISTOL_AIM_L (79)
#define NROPER_TWO_PISTOL_AIM_R (80)
#define NROPER_TWO_PISTOL_CROUCH (87)
#define NROPER_TWO_PISTOL_CROUCH_HOLD (88)
#define NROPER_TWO_PISTOL_CROUCH_STAND_UP (89)
#define NROPER_SHOTGUN_CROUCH (84)
#define NROPER_SHOTGUN_CROUCH_HOLD (85)
#define NROPER_SHOTGUN_CROUCH_STAND_UP (86)
#define NROPER_ANIM_SHOTGUN_RUN (74)
#define NROPER_ANIM_SHOTGUN_START_RUN (82)

#define NROPER_ANIM_CLIMB_OVER_FENCE (91)

// claude-ai: Global counters for the prim data arrays (world geometry + prims).
// claude-ai: next_prim_point: next free slot in prim_points[] (vertex positions).
// claude-ai: next_prim_face4/3: next free slot in quad/triangle face arrays.
// claude-ai: next_prim_object: next free slot in prim_objects[] (mesh objects).
// claude-ai: next_prim_multi_object: used for multi-part character meshes (body part variants).
// claude-ai: Starting at 1 (not 0) — slot 0 may be reserved as a NULL/invalid sentinel.
UWORD next_prim_point = 1;
UWORD next_prim_face4 = 1;
UWORD next_prim_face3 = 1;
UWORD next_prim_object = 1;
UWORD next_prim_multi_object = 1;

UBYTE roper_pickup = 0;

void set_next_prim_point(SLONG v)
{
    next_prim_point = v;
}

// UWORD	matrix_index_pool[MAX_MATRIX_POOL];
// struct	AnimItem	anim_index[MAX_CREATURE_TYPES][MAX_ANIMS_PER_CREATURE];

struct PrimMultiAnim prim_multi_anims[10000];

UWORD next_prim_multi_anim = 1;

// claude-ai: ANIM_init() — zeroes all GameKeyFrameChunk pointers.
// claude-ai: game_chunk[0..MAX_GAME_CHUNKS]: the in-game character animation chunks
// claude-ai:   [ANIM_TYPE_DARCI]=0, [ANIM_TYPE_ROPER]=1, [ANIM_TYPE_CIV]=2, [ANIM_TYPE_ROPER2]=3
// claude-ai: anim_chunk[0..MAX_ANIM_CHUNKS]: editor-side animation chunks (not used at runtime).
// claude-ai: Setting pointers to 0 is critical — load_anim_system() checks for NULL before alloc.
void ANIM_init(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_GAME_CHUNKS; c0++) {
        game_chunk[c0].PeopleTypes = 0;
        game_chunk[c0].AnimKeyFrames = 0;
        game_chunk[c0].AnimList = 0;
        game_chunk[c0].TheElements = 0;
        game_chunk[c0].FightCols = 0;
    }
    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++) {
        anim_chunk[c0].PeopleTypes = 0;
        anim_chunk[c0].AnimKeyFrames = 0;
        anim_chunk[c0].AnimList = 0;
        anim_chunk[c0].TheElements = 0;
        anim_chunk[c0].FightCols = 0;
    }
}

// claude-ai: ANIM_fini() — frees memory for Darci, Roper, Roper2, CIV animation chunks.
// claude-ai: free_game_chunk() declared inline here (forward decl pattern used elsewhere in codebase).
// claude-ai: game_chunk[5] (van) omitted — van anims were experimental/unused.
void ANIM_fini(void)
{
    void free_game_chunk(GameKeyFrameChunk * the_chunk);
    free_game_chunk(&game_chunk[ANIM_TYPE_DARCI]);
    free_game_chunk(&game_chunk[ANIM_TYPE_ROPER]);
    free_game_chunk(&game_chunk[ANIM_TYPE_ROPER2]);
    free_game_chunk(&game_chunk[ANIM_TYPE_CIV]);
    //	free_game_chunk(&game_chunk[5]);
}
//************************************************************************************************
// claude-ai: CMatrix33 COMPRESSION SYSTEM — how rotation matrices are stored in animation data:
// claude-ai: A 3x3 rotation matrix has 9 elements, but only 6 degrees of freedom (orthonormal).
// claude-ai: Compression: store only 2 of 3 elements per row; reconstruct 3rd via:
// claude-ai:   c = sqrt(1 - a² - b²)  [using UCA_Lookup table for fast integer sqrt]
// claude-ai: The Pad byte encodes WHICH element is largest (and thus reconstructed):
// claude-ai:   Pad bits 0,1,2,3: row0 dominance  bits 4,5: row1 dominance  bit 6,7: sign
// claude-ai: CMatrix33.M[0] stores row 0 packed: (m00<<22)|(m01<<12)|(m02<<2) — 10 bits each.
// claude-ai: M[1] stores row 1 the same way. M[2] = cross product of rows 0 and 1.
// claude-ai: GameKeyFrameElement: OffsetX/Y/Z (bone local translation) + compressed CMatrix.
// claude-ai: GameKeyFrameElementComp: same but with 8-bit m00/m01/m10/m11 + Pad encoding.
void SetCMatrixComp(GameKeyFrameElementComp* e, CMatrix33* cm)
{
    e->Pad = 0;

    // need to store the two smallest elements of the top row for accuracy

    SBYTE a, b, c;

    a = ((cm->M[0] & CMAT0_MASK) >> 22);
    b = ((cm->M[0] & CMAT1_MASK) >> 12);
    c = ((cm->M[0] & CMAT2_MASK) >> 02);

    if (abs(a) > abs(b)) {
        if (abs(a) > abs(c)) {
            e->m00 = b;
            e->m01 = c;
            e->Pad |= 0;
            if (a < 0)
                e->Pad |= 1;
        } else {
            e->m00 = a;
            e->m01 = b;
            e->Pad |= 4;
            if (c < 0)
                e->Pad |= 1;
        }
    } else {
        if (abs(b) > abs(c)) {
            e->m00 = a;
            e->m01 = c;
            e->Pad |= 8;
            if (b < 0)
                e->Pad |= 1;
        } else {
            e->m00 = a;
            e->m01 = b;
            e->Pad |= 4;
            if (c < 0)
                e->Pad |= 1;
        }
    }

    a = ((cm->M[1] & CMAT0_MASK) >> 22);
    b = ((cm->M[1] & CMAT1_MASK) >> 12);
    c = ((cm->M[1] & CMAT2_MASK) >> 02);

    if (abs(a) > abs(b)) {
        if (abs(a) > abs(c)) {
            e->m10 = b;
            e->m11 = c;
            e->Pad |= 0;
            if (a < 0)
                e->Pad |= 2;
        } else {
            e->m10 = a;
            e->m11 = b;
            e->Pad |= 16;
            if (c < 0)
                e->Pad |= 2;
        }
    } else {
        if (abs(b) > abs(c)) {
            e->m10 = a;
            e->m11 = c;
            e->Pad |= 32;
            if (b < 0)
                e->Pad |= 2;
        } else {
            e->m10 = a;
            e->m11 = b;
            e->Pad |= 16;
            if (c < 0)
                e->Pad |= 2;
        }
    }
}

void convert_to_psx_gke(GameKeyFrameElementComp* to, GameKeyFrameElement* from)
{
    to->OffsetX = (from->OffsetX); //&0xff;
    to->OffsetY = (from->OffsetY); //&0xff;
    to->OffsetZ = (from->OffsetZ); //&0xff;

    SetCMatrixComp(to, &from->CMatrix);
}

//************************************************************************************************

void convert_anim(Anim* key_list, GameKeyFrameChunk* p_chunk, KeyFrameChunk* the_chunk);

void init_anim_prims(void)
{
    SLONG c0;
    for (c0 = 0; c0 < MAX_ANIM_CHUNKS; c0++) {
        anim_chunk[c0].MultiObject[0] = 0;
    }
}

void reset_anim_stuff(void)
{
    if (test_chunk) {
        test_chunk->MultiObject = 0;
        test_chunk->ElementCount = 0;
        //		MemFree(test_chunk);
    }


    //	if(the_elements)
    //		MemFree(the_elements);
}


void setup_anim_stuff(void)
{


    //	if(test_chunk)
    //		MemFree(test_chunk);
}

UBYTE estate = 0; // desperate bodge
UBYTE semtex = 0;



// extern	SLONG	load_anim_system(struct GameKeyFrameChunk *game_chunk,CBYTE	*name);
extern SLONG load_anim_system(struct GameKeyFrameChunk* game_chunk, CBYTE* name, SLONG peep = 0);

void setup_extra_anims(void)
{
    Anim* key_list;
    key_list = NULL;
    /*
            load_key_frame_chunks(test_chunk,"van.vue");

            load_all_anims(test_chunk,&key_list);
            load_anim_system(&game_chunk[5],"van.all");
    */

    /*
            if(key_list)
            {
                    SLONG	c0	=	0;
                    Anim	*curr;

                    convert_anim(key_list,&game_chunk[5],test_chunk);


    //		game_chunk[ANIM_TYPE_CIV].MultiObject[0]=test_chunk->MultiObject;
    //		game_chunk[ANIM_TYPE_CIV].ElementCount=test_chunk->ElementCount;
    //		memcpy(game_chunk[ANIM_TYPE_CIV].PeopleTypes,test_chunk->PeopleTypes,sizeof(struct BodyDef)*MAX_PEOPLE_TYPES);
                    save_anim_system(&game_chunk[5],"u:\\urbanchaos\\data\\van.all");

            extern	void	convert_keyframe_to_pointer(GameKeyFrame *p,GameKeyFrameElement *p_ele,GameFightCol *p_fight,SLONG count);
            extern	void	convert_animlist_to_pointer(GameKeyFrame **p,GameKeyFrame *p_anim,SLONG count);
            extern	void	convert_fightcol_to_pointer(GameFightCol *p,GameFightCol *p_fight,SLONG count);

                    convert_keyframe_to_pointer(game_chunk[5].AnimKeyFrames,game_chunk[5].TheElements,game_chunk[5].FightCols,game_chunk[5].MaxKeyFrames);
                    convert_animlist_to_pointer(game_chunk[5].AnimList,game_chunk[5].AnimKeyFrames,game_chunk[5].MaxAnimFrames);
                    convert_fightcol_to_pointer(game_chunk[5].FightCols,game_chunk[5].FightCols,game_chunk[5].MaxFightCols);


                    curr	=	key_list;
                    while(curr)
                    {
                            van_array[c0]	=	curr->GetFrameList();
                            curr			=	curr->GetNextAnim();
                            c0++;
                    }
    //		if(anim_array[ANIM_PUNCH2])
    //			fix_anim_pos(anim_array[ANIM_PUNCH2],test_chunk);
            }
    */

    //	load_anim_system(&game_chunk[6],"data\\dog2.all");

    next_game_chunk = 4;
}
extern UWORD darci_normal_count;
extern SLONG append_anim_system(struct GameKeyFrameChunk* p_chunk, CBYTE* name, SLONG start_anim, SLONG load_mesh);
// claude-ai: setup_people_anims() — the main animation loading entry point, called at game start.
// claude-ai: Load order:
// claude-ai:   1. darci1.all  → game_chunk[ANIM_TYPE_DARCI] (Darci character, flag=1)
// claude-ai:   2. roper.all / psxroper.all / roper_up.all → game_chunk[ANIM_TYPE_ROPER]
// claude-ai:   3. rthug.all   → game_chunk[ANIM_TYPE_CIV] (Thug/Civ character, flag=2)
// claude-ai:   4. roper2.all  → game_chunk[ANIM_TYPE_ROPER2] (PC only, not PSX)
// claude-ai:   5. police1.all → appended to ROPER chunk at index 200+
// claude-ai:   6. newciv.all  → appended to CIV chunk at CIV_M_START (male civs)
// claude-ai:   7. newcivf.all → appended to CIV chunk at CIV_F_START (female civs)
// claude-ai:   8. trainer.all → appended to ROPER chunk at 240+ (tutorial level only)
// claude-ai:   9. banesuit.all → appended to ROPER at 240+ (estate2/Album1 levels)
// claude-ai: darci_normal_count = next_prim_point after loading, marks end of normal prim range.
void setup_people_anims(void)
{

    setup_anim_stuff();
    load_anim_system(&game_chunk[ANIM_TYPE_DARCI], "darci1.all", 1);
    //	load_anim_system(&game_chunk[1],"police1.all");
    //	if(!build_psx)

    if (save_psx) {
        if (roper_pickup) {
            //			ASSERT(0);
            load_anim_system(&game_chunk[ANIM_TYPE_ROPER], "roper_up.all");
        } else
            load_anim_system(&game_chunk[ANIM_TYPE_ROPER], "psxroper.all");
    } else {
        load_anim_system(&game_chunk[ANIM_TYPE_ROPER], "roper.all");
    }

    if (save_psx)
        load_anim_system(&game_chunk[ANIM_TYPE_CIV], "rthug.all", 2); // rpsxthug.all
    else
        load_anim_system(&game_chunk[ANIM_TYPE_CIV], "rthug.all", 2);

    if (!save_psx) // || roper_pickup)
        load_anim_system(&game_chunk[ANIM_TYPE_ROPER2], "roper2.all");

    append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "police1.all", 200, 0);
    append_anim_system(&game_chunk[ANIM_TYPE_CIV], "newciv.all", CIV_M_START, 1);
    append_anim_system(&game_chunk[ANIM_TYPE_CIV], "newcivf.all", CIV_F_START, 1);

    extern SLONG playing_combat_tutorial(void);
    extern SLONG playing_level(const CBYTE* name);
    if (!save_psx)
        if (playing_combat_tutorial()) {
            game_chunk[ANIM_TYPE_CIV].MultiObject[0] = next_prim_multi_object;
            game_chunk[ANIM_TYPE_CIV].MultiObject[1] = next_prim_multi_object;
            game_chunk[ANIM_TYPE_CIV].MultiObject[2] = next_prim_multi_object;
            append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "trainer.all", 240, 1);
        }
    if (playing_level("semtex.ucm")) {
        semtex = 1;
    } else {
        semtex = 0;
    }

    if (playing_level("estate2.ucm") || playing_level("Album1.ucm")) {
        // bodge bodge bodge
        game_chunk[ANIM_TYPE_CIV].MultiObject[6] = next_prim_multi_object;
        append_anim_system(&game_chunk[ANIM_TYPE_ROPER], "banesuit.all", 240, 1);
        estate = 1;

    } else {
        estate = 0;
    }

    darci_normal_count = next_prim_point;
}


// claude-ai: setup_global_anim_array() — builds the global animation dispatch table.
// claude-ai: global_anim_array[ANIM_TYPE][ANIM_*] → pointer to keyframe list for that action.
// claude-ai: STEP 1: Fill all 4 character type rows with Darci's animations as baseline.
// claude-ai:   This ensures every ANIM_* constant has a valid pointer even if not overridden.
// claude-ai: STEP 2 (commented out): Roper-specific overrides (these are dead code in this build).
// claude-ai: STEP 3: CIV/Thug overrides (THUG_ANIM_WALK, CIV_ANIM_IDLE_BREATHE, etc.)
// claude-ai: STEP 4: New Roper overrides (NROPER_* constants) applied on PC (not PSX).
// claude-ai:   PSX takes a slightly different subset of animations (build_psx path).
// claude-ai: NOTE: The large commented-out Roper section is DEAD CODE — an older roper setup
// claude-ai:   was replaced by the new NROPER_* system but not deleted.
void setup_global_anim_array(void)
{
    //
    // Make everyone use Darci's animations (including Darci!)
    //

    SLONG c0;
    for (c0 = 0; c0 < ANIM_END; c0++) {
        //		ASSERT(c0!=203);
        global_anim_array[0][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[1][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[2][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        global_anim_array[3][c0] = game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        //		global_anim_array[4][c0]=game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        //		global_anim_array[5][c0]=game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        //		global_anim_array[6][c0]=game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
        //		global_anim_array[7][c0]=game_chunk[ANIM_TYPE_DARCI].AnimList[c0];
    }

    //
    // If people have their own anims, then set them up to use
    // their animation instead of Darci's.
    //
    /*
    // cop is dead long live errm thug4
            global_anim_array[ANIM_TYPE_COP][ANIM_WALK]= game_chunk[1].AnimList[COP_ANIM_WALK];
            global_anim_array[ANIM_TYPE_COP][ANIM_RUN] = game_chunk[1].AnimList[COP_ANIM_WALK];
            global_anim_array[ANIM_TYPE_COP][ANIM_PUNCH1] = game_chunk[1].AnimList[COP_ANIM_JAB];
            global_anim_array[ANIM_TYPE_COP][ANIM_PUNCH2] = game_chunk[1].AnimList[COP_ANIM_HIT];
            global_anim_array[ANIM_TYPE_COP][ANIM_PUNCH3] = game_chunk[1].AnimList[COP_ANIM_JAB2];
            global_anim_array[ANIM_TYPE_COP][ANIM_KICK_ROUND1] = game_chunk[1].AnimList[COP_ANIM_KICK];
            global_anim_array[ANIM_TYPE_COP][ANIM_KICK2] = game_chunk[1].AnimList[COP_ANIM_HIT];
            global_anim_array[ANIM_TYPE_COP][ANIM_KICK3] = game_chunk[1].AnimList[COP_ANIM_KICK];


            global_anim_array[ANIM_TYPE_COP][ANIM_STAND_HIP] = game_chunk[1].AnimList[COP_ANIM_STAND];
            global_anim_array[ANIM_TYPE_COP][ANIM_STAND_READY] = game_chunk[1].AnimList[COP_ANIM_STAND];
            global_anim_array[ANIM_TYPE_COP][ANIM_IDLE_SCRATCH1] = game_chunk[1].AnimList[COP_ANIM_IDLE];
            global_anim_array[ANIM_TYPE_COP][ANIM_IDLE_SCRATCH2] = game_chunk[1].AnimList[COP_ANIM_IDLE3];
            global_anim_array[ANIM_TYPE_COP][ANIM_BREATHE] = game_chunk[1].AnimList[COP_ANIM_BREATHE];

            global_anim_array[ANIM_TYPE_COP][ANIM_GUT_HIT_SMALL] = game_chunk[1].AnimList[COP_ANIM_GUT_HIT_SMALL];
            global_anim_array[ANIM_TYPE_COP][ANIM_GUT_HIT_BIG] = game_chunk[1].AnimList[COP_ANIM_GUT_HIT_SMALL];

            global_anim_array[ANIM_TYPE_COP][ANIM_HEAD_HIT_SMALL] = game_chunk[1].AnimList[COP_ANIM_HEAD_HIT_SMALL];
            global_anim_array[ANIM_TYPE_COP][ANIM_HEAD_HIT_BIG] = game_chunk[1].AnimList[COP_ANIM_HEAD_HIT_SMALL];

            global_anim_array[ANIM_TYPE_COP][ANIM_KO_BACK] = game_chunk[1].AnimList[COP_ANIM_GUT_DEATH];
            global_anim_array[ANIM_TYPE_COP][ANIM_DIE_KNECK] = game_chunk[1].AnimList[COP_ANIM_DIE_KNECK];
            global_anim_array[ANIM_TYPE_COP][ANIM_BLOCK_LOW] = game_chunk[1].AnimList[COP_ANIM_BLOCK];
            global_anim_array[ANIM_TYPE_COP][ANIM_JUMP_UP_GRAB] = game_chunk[1].AnimList[COP_ANIM_JUMP_UP_GRAB];
            global_anim_array[ANIM_TYPE_COP][ANIM_PULL_UP] = game_chunk[1].AnimList[COP_ANIM_PULL_UP];
    */

    // -- Roper --

    /*
            //roper is dead, long live roper
            global_anim_array[ANIM_TYPE_ROPER][ANIM_WALK]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_WALK];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_RUN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_WALK];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH1]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE1];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH2]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE2];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH3]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE3];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH_COMBO1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE2];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH_COMBO2]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE1];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH_COMBO3]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE3];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH_RETURN1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_IDLE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PUNCH_RETURN2]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_IDLE];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_ROUND1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE1];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK2]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE1];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK3]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PUNCH_CLOSE1];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_IDLE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_READY]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_TO_FIGHT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_HIT_FRONT_MID]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_RECOIL];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_HIT_FRONT_HI]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_RECOIL];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_N]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_FORWARD];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_S]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_BACK];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_E]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_RIGHT];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_W]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_LEFT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_W]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_RIGHT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FIGHT_STEP_E]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_LEFT];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_COMBO1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_KICK_DOOR];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_COMBO2]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_KICK_DOOR];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_COMBO3]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_KICK_DOOR];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_RETURN1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_IDLE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_KICK_RETURN2]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FIGHT_IDLE];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_CAN_PICKUP]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PICKUP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CAN_RELEASE]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_THROW_1H];

            //temp
            global_anim_array[ANIM_TYPE_ROPER][ANIM_STANDING_JUMP]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STANDING_JUMP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_JUMP_UP_GRAB]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_JUMP_UP_GRAB];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FLY_GRABBING_LEDGE]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FLY_GRABBING_LEDGE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_VERT]		    = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_LAND_VERT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING]			    = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FALLING];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIG_LAND]			    = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_LAND_VERT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING_QUEUED]	    = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FALLING];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_HIP]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_READY]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_READY]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_FOLDARMS];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH1]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH2]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_BREATHE]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BREATHE]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ARMSFOLDED];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_ROTATE_L]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ROTATE_LEFT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_ROTATE_R]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ROTATE_RIGHT];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_CROUTCH_DOWN]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CROUCH_DOWN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_CROUTCH]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_IDLE_CROUCH];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CRAWL]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CRAWL];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_GUT_HIT_SMALL]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_GUT_HIT_BIG]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_HEAD_HIT_SMALL]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_HEAD_HIT_BIG]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_STAND];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_LEFT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_JUMP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_RIGHT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_JUMP];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND2]				= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_LAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_LEFT]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_LAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_RIGHT]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_LAND];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_DEATH_SLIDE]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_DEATH_SLIDE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_LEFT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_MID_AIR_TWEEN_LEFT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_RIGHT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ANIM_MID_AIR_TWEEN_RIGHT];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_LAND_ON_FENCE];
    //	global_anim_array[ANIM_TYPE_ROPER][ANIM_LANDED_ON_FENCE]		= game_chunk[ANIM_TYPE_ROPER].AnimList[55]; //heh
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CLIMB_UP_FENCE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_RIGHT_FENCE]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CLIMB_UP_FENCE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_LEFT_FENCE]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CLIMB_UP_FENCE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_OVER_FENCE]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_CLIMB_OVER_FENCE];


            global_anim_array[ANIM_TYPE_ROPER][ANIM_MOUNT_LADDER]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_MOUNT_LADDER];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_ON_LADDER]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ON_LADDER];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_OFF_LADDER_TOP]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_OFF_LADDER_TOP];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_TRAVERSE_LEFT]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_TRAVERSE_LEFT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_TRAVERSE_RIGHT]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_TRAVERSE_RIGHT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_DANGLE]			= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_TRAVERSE_LEFT];

            //global_anim_array[ANIM_TYPE_ROPER][ANIM_KO_BACK]	= game_chunk[ANIM_TYPE_ROPER].AnimList[];
            //global_anim_array[ANIM_TYPE_ROPER][ANIM_DIE_KNECK]	= game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ANIM_DIE_KNECK];
            //global_anim_array[ANIM_TYPE_ROPER][ANIM_BLOCK]		= game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ANIM_BLOCK];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PULL_UP]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PULL_UP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PULL_UP]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_PULL_UP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_DRAW_GUN]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_DRAW_GUN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_GUN_AIM]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_AIM_GUN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOOT]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_SHOOT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_ENTER_VAN]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_ENTER_VAN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_EXIT_VAN]		= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_EXIT_VAN];

            // temp hardcoded while stuff gets sorted out...
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIKE_MOUNT]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_BIKE_MOUNT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIKE_RIDE ]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_BIKE_RIDE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIKE_LEAN ]	= game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_BIKE_RIDE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIKE_LEAN_LEFT ] = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_BIKE_LEAN_LEFT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BIKE_LEAN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[ROPER_BIKE_LEAN_RIGHT];
    */
    // claude-ai: CIV (Thug/Civilian) animation overrides. Only a few anims are set here;
    // claude-ai: the rest use Darci's animations as fallback (from the fill loop above).
    // claude-ai: Comment "The Boss" is a leftover name from an earlier design iteration.
    // claude-ai: ANIM_TYPE_CIV (index 2) covers all non-Darci non-Roper NPCs (thugs, civs).
    // -- The Boss --

    global_anim_array[ANIM_TYPE_CIV][ANIM_WALK] = game_chunk[ANIM_TYPE_CIV].AnimList[THUG_ANIM_WALK];
    //	global_anim_array[ANIM_TYPE_CIV][ANIM_RUN]     =game_chunk[ANIM_TYPE_CIV].AnimList[THUG_ANIM_RUN];
    global_anim_array[ANIM_TYPE_CIV][ANIM_BREATHE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_IDLE_BREATHE];
    global_anim_array[ANIM_TYPE_CIV][ANIM_IDLE_SCRATCH1] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_IDLE_PHONE_ANSWER];
    //	global_anim_array[ANIM_TYPE_CIV][ANIM_FIGHT]    =game_chunk[ANIM_TYPE_CIV].AnimList[THUG_ANIM_FIGHT_IDLE];

    // claude-ai: "New roper" section — applies NROPER_* animation overrides for Roper character.
    // claude-ai: build_psx=1 on PSX forces a reduced subset (fewer wall-press, no two-pistol run).
    // claude-ai: On PC (save_psx=0): full NROPER_TWO_PISTOL_*, NROPER_SHOTGUN_*, NROPER_ANIM_AK_* set.
    // claude-ai: The if(build_psx) branch covers PSX path; else branch covers PC path.
    // claude-ai: Key PC-only animations: wall-press sidling (TO_WALL, ALONG_WALL, AIM_WALL variants).
    //
    // New roper
    //

    if (build_psx) {
        global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_READY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_STAND_READY];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_BREATHE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_BREATHE];

        global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_TELL] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_TELL];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_LISTEN] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_LISTEN];
        //		global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH1]	    =game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SWIG_FLASK];
        //	global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH2]	    =game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SWIG_FLASK];

        //		global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP_START]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP_START];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_LOOP];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_START];

    } else

    //	if(!build_psx)
    {
        global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_LOOP];
        global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_START];
        if (!save_psx) {
            global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_STAND_READY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_STAND_READY];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BREATHE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_BREATHE];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_FLY];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_LAND_RUN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_STAND] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_LAND];

            /*			global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DRAW]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_DRAW_GUN];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_AIM_AHEAD]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_GUN_AIM];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_LEFT]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_GUN_AIM_L];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_RIGHT]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_GUN_AIM_R];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_SHOOT]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_GUN_SHOOT];*/
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DRAW] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_DRAW];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_SHOOT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_FIRE];
            //			global_anim_array[ANIM_TYPE_ROPER][]						=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AWAY];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_AIM_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM_L];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_GUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_AIM_R];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_JOG] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_RUN];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DUCK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_DUCK_HOLD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH_HOLD];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_PISTOL_STANDUP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TWO_PISTOL_CROUCH_STAND_UP];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_STANDING_JUMP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_TAKEOFF];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_VERT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_LAND];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_STATIC];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_FALLING_QUEUED] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_JUMP_SPOT_STATIC];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_TELL] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_TELL];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_TALK_LISTEN] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_LISTEN];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DRAW] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_TAKEOUT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_AIM] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_FIRE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_SHOOT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM_L];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_AIM_R];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DUCK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_DUCK_HOLD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH_HOLD];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_SHOTGUN_STANDUP] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_SHOTGUN_CROUCH_STAND_UP];

            //			global_anim_array[ANIM_TYPE_ROPER][ANIM_AK_DRAW]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_TAKEOUT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_AHEAD] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_LEFT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM_L];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_POINT_SHOTGUN_RIGHT] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_AIM_R];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_AK_FIRE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_AK_SHOOT];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_AK_JOG] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_RUN];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP_START_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SHOTGUN_START_RUN];

            // pistoljump??
            global_anim_array[ANIM_TYPE_ROPER][ANIM_RUN_JUMP_LEFT_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_MID_AIR_TWEEN_LEFT_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_RUNNING_JUMP_FLY];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH1] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SWIG_FLASK];
            //	global_anim_array[ANIM_TYPE_ROPER][ANIM_IDLE_SCRATCH2]	    =game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_SWIG_FLASK];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_YOMP_START] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_YOMP_START];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BACK_WALK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_WALK_BACKWARDS];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_BACK_WALK_AK] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_WALK_BACKWARDS_WITH_AK];

            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_UP_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_LOOP];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_LAND_ON_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[COP_ROPER_ANIM_LADDER_START];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_CLIMB_OVER_FENCE] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_CLIMB_OVER_FENCE];

            // #define	ANIM_CLIMB_OVER_FENCE		(58)

            /*			global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_TURN]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TO_WALL];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_STAND]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TO_WALL_STATIC];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_L]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_R]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_TURN_AK]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_TO_WALL_SHOTGUN];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_STAND_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ANIM_WALK_BACKWARDS_WITH_AK];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_L_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_R_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];

                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_TURN_PISTOL]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_STAND_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_L_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_SIDLE_R_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];

                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_L_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_AIM_WALL_PISTOL_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_R_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_AIM_WALL_PISTOL_C];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_L_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_AIM_WALL_SHOTGUN];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_R_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_AIM_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_L]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_LOOK_R]			=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_L_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_R_PISTOL]	=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_L_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_R_AK]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_L]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
                                    global_anim_array[ANIM_TYPE_ROPER][ANIM_PRESS_WALL_FREEZE_R]		=game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_ALONG_WALL_SHOTGUN_B];
            */

            /*
            #define NROPER_TO_WALL						(49)
            #define NROPER_TO_WALL_STATIC				(65)
            #define NROPER_TO_WALL_SHOTGUN				(50)
            #define NROPER_ALONG_WALL					(51)
            #define NROPER_ALONG_WALL_B					(52)
            #define NROPER_ALONG_WALL_SHOTGUN			(53)
            #define NROPER_ALONG_WALL_SHOTGUN_B			(54)
            #define NROPER_ALONG_WALL_SHOTGUN_C			(61)
            #define NROPER_ALONG_WALL_SHOTGUN_D			(62)
            #define NROPER_AIM_WALL_SHOTGUN				(55)
            #define NROPER_AIM_WALL_SHOTGUN_B			(56)
            #define NROPER_AIM_WALL_PISTOL				(57)
            #define NROPER_AIM_WALL_PISTOL_B			(63)
            #define NROPER_AIM_WALL_PISTOL_C			(64)
            #define NROPER_FIGHT_WALL					(60)
            */

            global_anim_array[ANIM_TYPE_ROPER][ANIM_CRAWL] = game_chunk[ANIM_TYPE_ROPER2].AnimList[144];
            global_anim_array[ANIM_TYPE_ROPER][ANIM_DANGLE] = game_chunk[ANIM_TYPE_ROPER2].AnimList[223];
        }
        //	global_anim_array[ANIM_TYPE_ROPER][ANIM_MOUNT_LADDER]		=game_chunk[ANIM_TYPE_ROPER2].AnimList[13];
        //	global_anim_array[ANIM_TYPE_ROPER][ANIM_ON_LADDER]			=game_chunk[ANIM_TYPE_ROPER2].AnimList[14];
        //
        // Die from kneck snap in for everyone
        //
    }

    for (c0 = 1; c0 < 4; c0++) {
        global_anim_array[c0][ANIM_SIT_DOWN] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_SIT];
        global_anim_array[c0][ANIM_SIT_IDLE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_READ_PAPER1];
    }

    for (c0 = 0; c0 < 4; c0++) {
        //		if(!build_psx)
        {
            if (!save_psx) {
                global_anim_array[c0][ANIM_STRANGLE] = game_chunk[ANIM_TYPE_ROPER2].AnimList[268]; // I don't give a fuck, la la la la
                global_anim_array[c0][ANIM_STRANGLE_VICTIM] = game_chunk[ANIM_TYPE_ROPER2].AnimList[269];

                global_anim_array[c0][ANIM_HEADBUTT] = game_chunk[ANIM_TYPE_ROPER2].AnimList[270];
                global_anim_array[c0][ANIM_HEADBUTT_VICTIM] = game_chunk[ANIM_TYPE_ROPER2].AnimList[271];
            }
        }

        // unused		global_anim_array[c0][ANIM_RUN_TAXI]          =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_RUN_TAXI];
        global_anim_array[c0][ANIM_WANKER] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_WANKER];
        // unused		global_anim_array[c0][ANIM_RUN_FLEE]          =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_RUN_FLEE];
        // unused		global_anim_array[c0][ANIM_HAIL_TAXI]         =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_HAIL_TAXI];
        global_anim_array[c0][ANIM_ENTER_TAXI] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_ENTER_TAXI];
        // unused		global_anim_array[c0][ANIM_FIGHT_STANCE_HARD] =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_BLOCK];
        //		global_anim_array[c0][ANIM_FIGHT_STANCE_WIMP] =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_FIGHT_IDLE];
        //		global_anim_array[c0][ANIM_ENTER_VAN]	      =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_ENTER_TAXI];
        //		global_anim_array[c0][ANIM_EXIT_VAN]	      =game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_ENTER_TAXI];

        global_anim_array[c0][ANIM_DANCE_BOOGIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_BOOGIE];
        global_anim_array[c0][ANIM_DANCE_WOOGIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_WOOGIE];
        global_anim_array[c0][ANIM_DANCE_HEADBANG] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_DANCE_HEADBANG];
        global_anim_array[c0][ANIM_WARM_HANDS] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_WARM_HANDS];
        //		global_anim_array[c0][ANIM_HANDS_UP		 ] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_ANIM_HANDS_UP      ];

        global_anim_array[c0][ANIM_HANDS_UP] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP];
        global_anim_array[c0][ANIM_HANDS_UP_LOOP] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP_LOOP];
        global_anim_array[c0][ANIM_HANDS_UP_LIE] = game_chunk[ANIM_TYPE_CIV].AnimList[CIV_M_ANIM_HANDS_UP_LIE];
        if (!save_psx || roper_pickup)
        {
            global_anim_array[c0][ANIM_PICKUP_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PICKUP_CARRY];
            global_anim_array[c0][ANIM_START_WALK_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_START_WALK_CARRY];
            global_anim_array[c0][ANIM_WALK_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_CARRY];
            global_anim_array[c0][ANIM_WALK_STOP_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_STOP_CARRY];
            global_anim_array[c0][ANIM_PUTDOWN_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PUTDOWN_CARRY];
            global_anim_array[c0][ANIM_STAND_CARRY] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_STAND_CARRY];

            global_anim_array[c0][ANIM_PICKUP_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PICKUP_CARRY_V];
            global_anim_array[c0][ANIM_START_WALK_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_START_WALK_CARRY_V];
            global_anim_array[c0][ANIM_WALK_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_CARRY_V];
            global_anim_array[c0][ANIM_WALK_STOP_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_WALK_STOP_CARRY_V];
            global_anim_array[c0][ANIM_PUTDOWN_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_PUTDOWN_CARRY_V];
            global_anim_array[c0][ANIM_STAND_CARRY_V] = game_chunk[ANIM_TYPE_ROPER].AnimList[NROPER_STAND_CARRY_V];
        }

        //		for(c0=200;c0<300;c0++)
        //			global_anim_array[ANIM_TYPE_ROPER][c0]=game_chunk[ANIM_TYPE_ROPER].AnimList[c0];
    }
}

void calc_sub_objects_position_no_tween(GameKeyFrame* cur_frame, UWORD object, SLONG* x, SLONG* y, SLONG* z)
{
    struct Matrix31 offset;
    struct GameKeyFrameElement* anim_info;

    anim_info = &cur_frame->FirstElement[object];

    *x = (anim_info->OffsetX) >> TWEEN_OFFSET_SHIFT;
    *y = (anim_info->OffsetY) >> TWEEN_OFFSET_SHIFT;
    *z = (anim_info->OffsetZ) >> TWEEN_OFFSET_SHIFT;
}
/*
void	fix_anim_pos(struct KeyFrame *keyframe,struct KeyFrameChunk *chunk)
{
        struct	KeyFrame	*first_frame;
        SLONG	o_count,c0;

        SLONG	dx,dy,dz;

        calc_sub_objects_position_no_tween(keyframe,SUB_OBJECT_LEFT_FOOT,&dx,&dy,&dz);
        o_count=prim_multi_objects[chunk->MultiObject].EndObject-prim_multi_objects[chunk->MultiObject].StartObject;

        first_frame=keyframe;

        while(keyframe)
        {
                struct KeyFrameElement	*the_element;

                for(c0=0;c0<=o_count;c0++)
                {
                        the_element=&keyframe->FirstElement[c0];
                        the_element->OffsetX-=dx;
                        the_element->OffsetY-=dy;
                        the_element->OffsetZ-=dz;
                }


                keyframe=keyframe->NextFrame;
                if(keyframe==first_frame)
                        break;
        }
}
*/

void fix_multi_object_for_anim(UWORD obj, struct PrimMultiAnim* p_anim)
{
    SLONG sp, ep;
    UWORD c0, c1;
    struct PrimObject* p_obj;
    struct Matrix33* mat;
    struct Matrix31 temp;
}

//---------------------------------------------------------------
// class Anim
//---------------------------------------------------------------

Anim::Anim()
{
    CurrentFrame = NULL;
    FrameListStart = NULL;
    FrameListEnd = NULL;
    FrameCount = 0;
    LastAnim = NULL;
    NextAnim = NULL;
    AnimFlags = 0;
}

//---------------------------------------------------------------

Anim::~Anim()
{
    while (FrameCount) {
        CurrentFrame = FrameListStart;
        FrameListStart = CurrentFrame->NextFrame;
        if (CurrentFrame)
            MemFree(CurrentFrame);
        FrameCount--;
    }
}

//---------------------------------------------------------------
void Anim::StartLooping(void)
{
    if (FrameListEnd) {
        FrameListEnd->NextFrame = FrameListStart;
    }
}

void Anim::StopLooping(void)
{
    if (FrameListEnd) {
        FrameListEnd->NextFrame = 0;
    }
}

void Anim::SetAllTweens(SLONG tween)
{
    KeyFrame* frame_ptr;

    frame_ptr = FrameListStart;

    while (frame_ptr) {
        frame_ptr->TweenStep = tween;
        if (frame_ptr == FrameListEnd)
            break;
        frame_ptr = frame_ptr->NextFrame;
    }
}

//---------------------------------------------------------------

void Anim::AddKeyFrame(KeyFrame* the_frame)
{
    KeyFrame* frame_ptr;

    if (the_frame) {
        StopLooping();
        frame_ptr = (KeyFrame*)MemAlloc(sizeof(KeyFrame));
        *frame_ptr = *the_frame;
        frame_ptr->NextFrame = NULL;
        frame_ptr->PrevFrame = NULL;
        frame_ptr->TweenStep = the_frame->TweenStep;
        frame_ptr->Fight = the_frame->Fight;
        if (FrameListStart == NULL) {
            FrameListStart = frame_ptr;
            FrameListEnd = FrameListStart;
            FrameListEnd->Flags |= ANIM_FLAG_LAST_FRAME;
        } else if (CurrentFrame == NULL) {
            if (FrameListEnd) {
                FrameListEnd->NextFrame = frame_ptr;
                frame_ptr->PrevFrame = FrameListEnd;
            }
            FrameListEnd->Flags &= ~ANIM_FLAG_LAST_FRAME;
            FrameListEnd = frame_ptr;
            FrameListEnd->Flags |= ANIM_FLAG_LAST_FRAME;
        } else {
            frame_ptr->PrevFrame = CurrentFrame->PrevFrame;
            if (CurrentFrame->PrevFrame)
                CurrentFrame->PrevFrame->NextFrame = frame_ptr;
            else
                FrameListStart = frame_ptr;
            CurrentFrame->PrevFrame = frame_ptr;
            frame_ptr->NextFrame = CurrentFrame;
            CurrentFrame = 0;
        }
        FrameCount++;
        if (AnimFlags & ANIM_LOOP)
            StartLooping();
    }
}
/*

  //
  // Untested
  //

void	Anim::AppendKeyFrame(KeyFrame *the_frame)
{
        KeyFrame		*frame_ptr;


        if(the_frame)
        {
                StopLooping();
                frame_ptr				=	(KeyFrame*)MemAlloc(sizeof(KeyFrame));
                *frame_ptr				=	*the_frame;
                frame_ptr->NextFrame	=	NULL;
                frame_ptr->PrevFrame	=	NULL;
                frame_ptr->TweenStep	=	the_frame->TweenStep;
                frame_ptr->Fight		=	the_frame->Fight;
                if(FrameListStart==NULL)
                {
                        FrameListStart			=	frame_ptr;
                        FrameListEnd			=	FrameListStart;
                        FrameListEnd->Flags		|=	ANIM_FLAG_LAST_FRAME;
                }
                else if(CurrentFrame==NULL)
                {
                        if(FrameListEnd)
                        {
                                FrameListEnd->NextFrame	=	frame_ptr;
                                frame_ptr->PrevFrame	=	FrameListEnd;
                        }
                        FrameListEnd->Flags		&=	~ANIM_FLAG_LAST_FRAME;
                        FrameListEnd			=	frame_ptr;
                        FrameListEnd->Flags		|=	ANIM_FLAG_LAST_FRAME;
                }
                else
                {
                        frame_ptr->PrevFrame	=	CurrentFrame;
                        frame_ptr->NextFrame	=	CurrentFrame->NextFrame;
                        if(CurrentFrame->NextFrame	==0)
                                FrameListEnd = frame_ptr;
                        CurrentFrame->NextFrame	=	frame_ptr;

                        CurrentFrame			=	0;
                }
                FrameCount++;
                if(AnimFlags&ANIM_LOOP)
                        StartLooping();
        }
}
*/
//---------------------------------------------------------------

void Anim::RemoveKeyFrame(KeyFrame* the_frame)
{
    if (the_frame) {
        StopLooping();
        if (the_frame->NextFrame) {
            the_frame->NextFrame->PrevFrame = the_frame->PrevFrame;
        } else {
            FrameListEnd = the_frame->PrevFrame;
            if (FrameListEnd)
                FrameListEnd->Flags = ANIM_FLAG_LAST_FRAME;
        }
        if (the_frame->PrevFrame) {
            the_frame->PrevFrame->NextFrame = the_frame->NextFrame;
        } else
            FrameListStart = the_frame->NextFrame;
        MemFree(the_frame);
        FrameCount--;
        if (AnimFlags & ANIM_LOOP)
            StartLooping();
    }
}

//---------------------------------------------------------------
// class Character
//---------------------------------------------------------------

void Character::AddAnim(Anim* the_anim)
{
}

//---------------------------------------------------------------

void Character::RemoveAnim(Anim* the_anim)
{
}

//---------------------------------------------------------------

//
// go through all the elements converting them to the compressed matrix format
//

void convert_elements(KeyFrameChunk* the_chunk, GameKeyFrameChunk* game_chunk, UWORD* p_reloc, SLONG max_ele)
{
    struct KeyFrameElement* first_element;
    struct KeyFrameElement* last_element;
    struct KeyFrameElement *element, *e2;
    SLONG count = 0;
    SLONG unique = 0;
    UWORD pos = 0;

    first_element = the_chunk->FirstElement;
    last_element = the_chunk->LastElement;

    for (element = first_element; element < last_element; element++) {

        if (p_reloc[count] != 0xffff) {
            pos = p_reloc[count];

            SetCMatrix(&game_chunk->TheElements[pos], &element->CMatrix);
            game_chunk->TheElements[pos].OffsetX = (SWORD)element->OffsetX;
            game_chunk->TheElements[pos].OffsetY = (SWORD)element->OffsetY;
            game_chunk->TheElements[pos].OffsetZ = (SWORD)element->OffsetZ;
            /*
            for(e2=first_element;e2<last_element;e2++)
            {
                    if(memcmp((UBYTE*)&game_chunk->TheElements[count].CMatrix,(UBYTE*)&e2->CMatrix,sizeof(struct CMatrix33))==0)
                    {
                            if(e2!=element)
                            {
                                    unique--;
                                    break;
                            }
                    }
            }
            unique++;
            */
        }

        count++;
    }
    game_chunk->MaxElements = max_ele;
}


void free_game_chunk(GameKeyFrameChunk* the_chunk)
{
    if (the_chunk->PeopleTypes)
        MemFree((UBYTE*)the_chunk->PeopleTypes);

    //	if(the_chunk->TweakSpeeds)
    //		MemFree((UBYTE*)the_chunk->TweakSpeeds);

    if (the_chunk->AnimKeyFrames)
        MemFree((UBYTE*)the_chunk->AnimKeyFrames);
    if (the_chunk->AnimList)
        MemFree((UBYTE*)the_chunk->AnimList);
    if (the_chunk->TheElements)
        MemFree((UBYTE*)the_chunk->TheElements);
    if (the_chunk->FightCols)
        MemFree((UBYTE*)the_chunk->FightCols);

    the_chunk->PeopleTypes = 0;
    the_chunk->AnimKeyFrames = 0;
    the_chunk->AnimList = 0;
    the_chunk->TheElements = 0;
    the_chunk->FightCols = 0;
    //		the_chunk->TweakSpeeds=0;
}
