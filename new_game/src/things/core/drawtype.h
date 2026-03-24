#ifndef THINGS_CORE_DRAWTYPE_H
#define THINGS_CORE_DRAWTYPE_H

// Draw type system: pool management for animated mesh objects.
// Every Thing in the game has a DrawType field that selects the rendering path.
// DrawTween = vertex-morphing animated characters; DrawMesh = static oriented mesh objects.

#include "engine/core/types.h"
#include "engine/graphics/lighting/cache.h"

// uc_orig: RMAX_DRAW_TWEENS (fallen/Headers/drawtype.h)
#define RMAX_DRAW_TWEENS RMAX_PEOPLE + 30
// uc_orig: RMAX_DRAW_MESHES (fallen/Headers/drawtype.h)
#define RMAX_DRAW_MESHES 320

// These macros require save_table[] and SAVE_TABLE_DTWEEN/DMESH to be declared
// (provided by Game.h). They are resolved at use time.
// uc_orig: MAX_DRAW_TWEENS (fallen/Headers/drawtype.h)
#define MAX_DRAW_TWEENS (save_table[SAVE_TABLE_DTWEEN].Maximum)
// uc_orig: MAX_DRAW_MESHES (fallen/Headers/drawtype.h)
#define MAX_DRAW_MESHES (save_table[SAVE_TABLE_DMESH].Maximum)

// uc_orig: DT_NONE (fallen/Headers/drawtype.h)
#define DT_NONE 0
// uc_orig: DT_BUILDING (fallen/Headers/drawtype.h)
#define DT_BUILDING 1
// uc_orig: DT_PRIM (fallen/Headers/drawtype.h)
#define DT_PRIM 2
// uc_orig: DT_MULTI_PRIM (fallen/Headers/drawtype.h)
#define DT_MULTI_PRIM 3
// uc_orig: DT_ROT_MULTI (fallen/Headers/drawtype.h)
#define DT_ROT_MULTI 4
// uc_orig: DT_EFFECT (fallen/Headers/drawtype.h)
#define DT_EFFECT 5
// uc_orig: DT_MESH (fallen/Headers/drawtype.h)
#define DT_MESH 6
// uc_orig: DT_TWEEN (fallen/Headers/drawtype.h)
// Vertex morph animation — interpolates raw vertex positions between keyframes.
// NOT skeletal animation. Used for all persons/NPCs.
#define DT_TWEEN 7
// uc_orig: DT_SPRITE (fallen/Headers/drawtype.h)
#define DT_SPRITE 8
// uc_orig: DT_VEHICLE (fallen/Headers/drawtype.h)
#define DT_VEHICLE 9
// uc_orig: DT_ANIM_PRIM (fallen/Headers/drawtype.h)
#define DT_ANIM_PRIM 10
// uc_orig: DT_CHOPPER (fallen/Headers/drawtype.h)
#define DT_CHOPPER 11
// uc_orig: DT_PYRO (fallen/Headers/drawtype.h)
#define DT_PYRO 12
// uc_orig: DT_ANIMAL_PRIM (fallen/Headers/drawtype.h)
#define DT_ANIMAL_PRIM 13
// uc_orig: DT_TRACK (fallen/Headers/drawtype.h)
#define DT_TRACK 14
// uc_orig: DT_BIKE (fallen/Headers/drawtype.h)
#define DT_BIKE 15

// uc_orig: DT_FLAG_UNUSED (fallen/Headers/drawtype.h)
#define DT_FLAG_UNUSED (1 << 7)
// uc_orig: DT_FLAG_GUNFLASH (fallen/Headers/drawtype.h)
#define DT_FLAG_GUNFLASH (1 << 6)
// Shadow-cast enable flag stored in DrawTween.Flags.
// uc_orig: FLAGS_DRAW_SHADOW (fallen/Headers/Structs.h)
#define FLAGS_DRAW_SHADOW (1 << 0)

// uc_orig: DrawTween (fallen/Headers/drawtype.h)
// Animation state for a vertex-morphing character (DT_TWEEN).
// Vertex positions are linearly interpolated between CurrentFrame and NextFrame:
//   pos = lerp(CurrentFrame.verts[i], NextFrame.verts[i], AnimTween / MAX_ANIM_TWEEN)
typedef struct
{
    UBYTE TweakSpeed;        // animation playback speed multiplier
    SBYTE Locked;            // if >=0, hold this keyframe index (frozen pose)
    UBYTE FrameIndex;        // current keyframe index being played
    UBYTE QueuedFrameIndex;  // next animation to play after current finishes

    SWORD Angle, AngleTo,    // current and target facing angle (0–2047)
        Roll, DRoll,         // banking roll and delta roll
        Tilt, TiltTo;        // forward/back tilt and target tilt

    SLONG CurrentAnim,       // index of the currently playing animation clip
        AnimTween,           // blend fraction between CurrentFrame and NextFrame
        TweenStage;          // sub-stage within the tween (for multi-step blends)
    struct GameKeyFrame *CurrentFrame,   // keyframe N (start of current blend)
        *NextFrame,                      // keyframe N+1 (end of current blend)
        *InterruptFrame,                 // keyframe to jump to on interruption
        *QueuedFrame;                    // keyframe to play after current animation ends
    struct GameKeyFrameChunk* TheChunk;  // loaded keyframe data block for this mesh/anim

    UBYTE Flags;    // DT_FLAG_* bits (UNUSED, GUNFLASH)
    UBYTE Drawn;    // game turn number when last rendered (for occlusion skip)
    UBYTE MeshID;   // index into the mesh table (which .IMP model to use)
    UBYTE PersonID; // index of the Person/Thing this tween belongs to
} DrawTween;

// uc_orig: DrawMesh (fallen/Headers/drawtype.h)
// Orientation state for a static mesh object (DT_MESH, DT_VEHICLE, etc.).
// No animation — just a mesh instance at a given orientation.
typedef struct
{
    UWORD Angle;      // yaw rotation (0–2047 = 0–360 degrees)
    UWORD Roll;       // roll (banking) rotation
    UWORD Tilt;       // pitch (forward tilt) rotation
    UWORD ObjectId;   // mesh asset ID (index into loaded mesh table)
    CACHE_Index Cache; // texture cache slot
    UBYTE Hm;         // height-map index; 255 = NULL (no height override)
} DrawMesh;

// uc_orig: init_draw_tweens (fallen/Source/drawtype.cpp)
void init_draw_tweens(void);
// uc_orig: alloc_draw_tween (fallen/Source/drawtype.cpp)
DrawTween* alloc_draw_tween(SLONG type);
// uc_orig: free_draw_tween (fallen/Source/drawtype.cpp)
void free_draw_tween(DrawTween* draw_tween);

// uc_orig: init_draw_meshes (fallen/Source/drawtype.cpp)
void init_draw_meshes(void);
// uc_orig: alloc_draw_mesh (fallen/Source/drawtype.cpp)
DrawMesh* alloc_draw_mesh(void);
// uc_orig: free_draw_mesh (fallen/Source/drawtype.cpp)
void free_draw_mesh(DrawMesh* drawmesh);

#endif // THINGS_CORE_DRAWTYPE_H
