#ifndef FALLEN_HEADERS_DRAWTYPE_H
#define FALLEN_HEADERS_DRAWTYPE_H
//
// Draw Types
//

// claude-ai: drawtype.h — defines all renderable object types (DT_*) and their draw-state structs.
// claude-ai: Every Thing in the game has a DrawType field that picks which renderer to use.
// claude-ai: The two main drawable structs are DrawTween (animated characters) and DrawMesh (static objects).

#include "cache.h"

// claude-ai: Pool sizes: RMAX_DRAW_TWEENS ≈ RMAX_PEOPLE+30 (one tween per person + extras)
// claude-ai: RMAX_DRAW_MESHES = 320 max simultaneously active static mesh instances
#define RMAX_DRAW_TWEENS RMAX_PEOPLE + 30
#define RMAX_DRAW_MESHES 320 // 500

#define MAX_DRAW_TWEENS (save_table[SAVE_TABLE_DTWEEN].Maximum)
#define MAX_DRAW_MESHES (save_table[SAVE_TABLE_DMESH].Maximum)

// claude-ai: DT_* = draw type enum. Stored in Thing.DrawType.
// claude-ai: Determines which rendering path processes this Thing each frame.
// claude-ai: --- Types to PORT (needed for gameplay) ---
#define DT_NONE 0 // claude-ai: not rendered
#define DT_BUILDING 1 // claude-ai: static building geometry (DFacet/FStorey system)
#define DT_PRIM 2 // claude-ai: single primitive (prim_points based polygon)
#define DT_MULTI_PRIM 3 // claude-ai: multi-part primitive object
#define DT_ROT_MULTI 4 // claude-ai: rotating multi-prim (e.g. fan blades)
#define DT_EFFECT 5 // claude-ai: generic visual effect placeholder
#define DT_MESH 6 // claude-ai: static .IMP/.MOJ mesh (vehicles, furniture, pickups)
#define DT_TWEEN 7 // claude-ai: *** VERTEX MORPH ANIMATION *** — used for ALL persons/NPCs
// claude-ai: NOT skeletal! Interpolates raw vertex positions between keyframes.
// claude-ai: See DrawTween struct below and resource_formats/animation_format.md
#define DT_SPRITE 8 // claude-ai: 2D billboard sprite (always faces camera)
#define DT_VEHICLE 9 // claude-ai: vehicle (car/truck) — uses mesh + special wheel rendering
#define DT_ANIM_PRIM 10 // claude-ai: animated primitive (frame-by-frame prim swap)
#define DT_CHOPPER 11 // claude-ai: helicopter — mesh + rotor blade rendering
#define DT_PYRO 12 // claude-ai: pyrotechnic / explosion particle effect
#define DT_ANIMAL_PRIM 13 // claude-ai: animal (uses prim-based animation, not tween)
#define DT_TRACK 14 // claude-ai: vehicle track marks / skid marks on ground
#define DT_BIKE 15 // claude-ai: motorbike — mesh + wheel + lean rendering

#define DT_FLAG_UNUSED (1 << 7) // claude-ai: slot is free in the draw pool
#define DT_FLAG_GUNFLASH (1 << 6) // claude-ai: muzzle flash overlay active this frame

//
// structs
//

// claude-ai: DrawTween — animation state for a VERTEX MORPHING character (DT_TWEEN).
// claude-ai: This is NOT skeletal animation. The mesh has N keyframes; each keyframe stores
// claude-ai: the ABSOLUTE position of every vertex. To animate, the renderer linearly interpolates
// claude-ai: vertex positions between CurrentFrame and NextFrame using AnimTween as the blend fraction.
// claude-ai: Formula: pos = CurrentFrame.verts[i] * (1 - t) + NextFrame.verts[i] * t
// claude-ai:   where t = AnimTween / MAX_ANIM_TWEEN  (0.0 → 1.0)
// claude-ai: One DrawTween per person/NPC. MeshID identifies the character mesh (.IMP/.MOJ file).
// claude-ai: PersonID links back to the Thing/Person game object.
typedef struct
{
    UBYTE TweakSpeed; // claude-ai: animation playback speed multiplier
    SBYTE Locked; // claude-ai: if >=0, this keyframe index is held (frozen pose)
    UBYTE FrameIndex; // claude-ai: current keyframe index being played
    UBYTE QueuedFrameIndex; // claude-ai: next animation to play after current finishes

    SWORD Angle, AngleTo, // claude-ai: current and target facing angle (0-2047)
        Roll, DRoll, // claude-ai: banking roll and delta roll (for vehicles/bikes)
        Tilt, TiltTo; // claude-ai: forward/back tilt and target tilt

    SLONG CurrentAnim, // claude-ai: index of the currently playing animation clip
        AnimTween, // claude-ai: blend fraction between CurrentFrame and NextFrame
        TweenStage; // claude-ai: sub-stage within the tween (for multi-step blends)
    struct GameKeyFrame *CurrentFrame, // claude-ai: pointer to keyframe N (start of current blend)
        *NextFrame, // claude-ai: pointer to keyframe N+1 (end of current blend)
        *InterruptFrame, // claude-ai: keyframe to jump to if animation is interrupted
        *QueuedFrame; // claude-ai: keyframe to play after current animation ends
    struct GameKeyFrameChunk* TheChunk; // claude-ai: loaded keyframe data block for this mesh/anim

    UBYTE Flags; // claude-ai: DT_FLAG_* bits (UNUSED, GUNFLASH)
    UBYTE Drawn; // claude-ai: game turn number when last rendered (for occlusion skip)
    UBYTE MeshID; // claude-ai: index into the mesh table (which .IMP model to use)
    UBYTE PersonID; // claude-ai: index of the Person/Thing this tween belongs to
} DrawTween;

// claude-ai: DrawMesh — orientation state for a static mesh object (DT_MESH, DT_VEHICLE, etc.).
// claude-ai: No animation — just a mesh at a given orientation.
// claude-ai: ObjectId identifies which .IMP/.MOJ mesh asset to render.
// claude-ai: Cache is the texture cache slot. Hm=255 means no height-map override.
typedef struct
{
    UWORD Angle; // claude-ai: yaw rotation (0-2047 = 0-360 degrees, engine convention, masked & 2047)
    UWORD Roll; // claude-ai: roll (banking) rotation
    UWORD Tilt; // claude-ai: pitch (forward tilt) rotation
    UWORD ObjectId; // claude-ai: mesh asset ID (index into loaded mesh table)
    CACHE_Index Cache; // claude-ai: texture cache slot (UBYTE)
    UBYTE Hm; // claude-ai: height-map index; 255 = NULL (no special height)

} DrawMesh;

//
// Functions
//

void init_draw_tweens(void);
DrawTween* alloc_draw_tween(SLONG type);
void free_draw_tween(DrawTween* draw_tween);

//
// DrawMesh functions.
//

void init_draw_meshes(void);
DrawMesh* alloc_draw_mesh(void);
void free_draw_mesh(DrawMesh* drawmesh);

#endif // FALLEN_HEADERS_DRAWTYPE_H
