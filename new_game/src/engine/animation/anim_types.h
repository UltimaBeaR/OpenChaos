#ifndef ENGINE_ANIMATION_ANIM_TYPES_H
#define ENGINE_ANIMATION_ANIM_TYPES_H

// Animation system type definitions: keyframe structures, fight collision data,
// and the editor-facing Anim/Character class hierarchy.
//
// These types are used by both the game runtime (engine/animation/) and
// asset loading (assets/anim.*).

#include "engine/core/types.h"
#include "engine/core/fmatrix.h"  // Matrix33, CMatrix33, SVector
#include <string.h>        // strcpy used by Anim::SetAnimName

// ---- Creature/people limits ----

// uc_orig: MAX_CREATURE_TYPES (fallen/Editor/Headers/Anim.h)
#define MAX_CREATURE_TYPES 20

// uc_orig: MAX_ANIMS_PER_CREATURE (fallen/Editor/Headers/Anim.h)
#define MAX_ANIMS_PER_CREATURE 20

// uc_orig: MAX_MATRIX_POOL (fallen/Editor/Headers/Anim.h)
#define MAX_MATRIX_POOL 10000

// uc_orig: MAX_PEOPLE_TYPES (fallen/Editor/Headers/Anim.h)
#define MAX_PEOPLE_TYPES 20

// uc_orig: MAX_BODY_BITS (fallen/Editor/Headers/Anim.h)
#define MAX_BODY_BITS 18

// uc_orig: MAX_BODY_VARIETY (fallen/Editor/Headers/Anim.h)
#define MAX_BODY_VARIETY 10

// uc_orig: PEOPLE_NAME_SIZE (fallen/Editor/Headers/Anim.h)
#define PEOPLE_NAME_SIZE 15

// Animation type IDs for each character rig.
// uc_orig: ANIM_PERSON (fallen/Editor/Headers/Anim.h)
#define ANIM_PERSON 1

// uc_orig: ANIM_RHINO (fallen/Editor/Headers/Anim.h)
#define ANIM_RHINO 2

// uc_orig: ANIM_APE (fallen/Editor/Headers/Anim.h)
#define ANIM_APE 3

// uc_orig: TWEEN_OFFSET_SHIFT (fallen/Editor/Headers/Anim.h)
#define TWEEN_OFFSET_SHIFT 0

// uc_orig: ANIM_FLAG_LAST_FRAME (fallen/Editor/Headers/Anim.h)
#define ANIM_FLAG_LAST_FRAME (1 << 0)

// ---- Keyframe chunk limits ----

// uc_orig: MAX_NUMBER_OF_CHUNKS (fallen/Editor/Headers/Anim.h)
#define MAX_NUMBER_OF_CHUNKS 5

// uc_orig: MAX_NUMBER_OF_FRAMES (fallen/Editor/Headers/Anim.h)
#define MAX_NUMBER_OF_FRAMES 5000

// uc_orig: MAX_NUMBER_OF_ELEMENTS (fallen/Editor/Headers/Anim.h)
#define MAX_NUMBER_OF_ELEMENTS 80000

// ---- Anim name/flags ----

// uc_orig: ANIM_NAME_SIZE (fallen/Editor/Headers/Anim.h)
#define ANIM_NAME_SIZE 64

// uc_orig: ANIM_LOOP (fallen/Editor/Headers/Anim.h)
#define ANIM_LOOP 1

// ---- Multi-animation slot ----

// One animation channel for an object part: start/end frame indices into global pool.
// uc_orig: AnimItem (fallen/Editor/Headers/Anim.h)
struct AnimItem {
    UWORD Start;
    UWORD End;
};

// Per-object frame data for multi-part animation (stores matrix + offset delta).
// uc_orig: PrimMultiAnim (fallen/Editor/Headers/Anim.h)
#pragma pack(push, 1)
struct PrimMultiAnim {
    struct Matrix33 Mat;
    SLONG DX;
    SLONG DY;
    SLONG DZ;
    UWORD Next;
};
#pragma pack(pop)
static_assert(sizeof(PrimMultiAnim) == 50, "PrimMultiAnim: binary file layout");

// Binary-file format structures: 1-byte packed for .all animation file compatibility.
#pragma pack(push, 1)

// Body composition definition: which body part index fills each of the 20 slots.
// Slots 1–14 are used for a standard person.
// uc_orig: BodyDef (fallen/Editor/Headers/Anim.h)
struct BodyDef {
    UBYTE BodyPart[20];
};

// ---- Game keyframe elements (runtime compressed format) ----

// Legacy element format (pre-release); not used in final game.
// uc_orig: GameKeyFrameElementCompOld (fallen/Editor/Headers/Anim.h)
struct GameKeyFrameElementCompOld {
    SBYTE m00, m01, m10, m11;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};

// Compressed per-bone element: 4 matrix components + 3 offsets packed into 8 bytes.
// uc_orig: GameKeyFrameElementComp (fallen/Editor/Headers/Anim.h)
struct GameKeyFrameElementComp {
    SBYTE m00, m01, m10, m11;
    SBYTE OffsetX;
    SBYTE OffsetY;
    SBYTE OffsetZ;
    UBYTE Pad;
};

// Large variant with SWORD offsets (used for body parts with large displacements).
// uc_orig: GameKeyFrameElementBig (fallen/Editor/Headers/Anim.h)
struct GameKeyFrameElementBig {
    struct CMatrix33 CMatrix;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};

// Standard game runtime bone element: compressed matrix + SWORD position offsets.
// uc_orig: GameKeyFrameElement (fallen/Editor/Headers/Anim.h)
struct GameKeyFrameElement {
    struct CMatrix33 CMatrix;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};

// uc_orig: GetCMatrix (fallen/Editor/Headers/Anim.h)
inline void GetCMatrix(GameKeyFrameElement* e, CMatrix33* c)
{
    *c = e->CMatrix;
}

// uc_orig: SetCMatrix (fallen/Editor/Headers/Anim.h)
inline void SetCMatrix(GameKeyFrameElement* e, CMatrix33* c)
{
    e->CMatrix = *c;
}

// ---- Fight collision ----

// Defines a hit zone for a fight animation frame: arc, distance, damage.
// uc_orig: GameFightCol (fallen/Editor/Headers/Anim.h)
struct GameFightCol {
    UWORD Dist1;
    UWORD Dist2;

    UBYTE Angle;
    UBYTE Priority;
    UBYTE Damage;
    UBYTE Tween;

    UBYTE AngleHitFrom;
    UBYTE Height;
    UBYTE Width;
    UBYTE Dummy;

    struct GameFightCol* Next;
};

// On-disk layout of GameFightCol (fixed 16 bytes, pointers as uint32_t).
// Used when reading .iam animation files which store 32-bit pointer values.
struct GameFightCol_Disk {
    UWORD Dist1;
    UWORD Dist2;
    UBYTE Angle;
    UBYTE Priority;
    UBYTE Damage;
    UBYTE Tween;
    UBYTE AngleHitFrom;
    UBYTE Height;
    UBYTE Width;
    UBYTE Dummy;
    uint32_t Next;
};

// ---- Game keyframe ----

// A single animation keyframe: tween step, element array pointer, fight collision data.
// uc_orig: GameKeyFrame (fallen/Editor/Headers/Anim.h)
struct GameKeyFrame {
    UBYTE XYZIndex;
    UBYTE TweenStep;
    SWORD Flags;
    GameKeyFrameElement* FirstElement;
    GameKeyFrame *PrevFrame, *NextFrame;
    GameFightCol* Fight;
};

// On-disk layout of GameKeyFrame (fixed 20 bytes, pointers as uint32_t).
struct GameKeyFrame_Disk {
    UBYTE XYZIndex;
    UBYTE TweenStep;
    SWORD Flags;
    uint32_t FirstElement;
    uint32_t PrevFrame;
    uint32_t NextFrame;
    uint32_t Fight;
};

#pragma pack(pop)

static_assert(sizeof(BodyDef) == 20, "BodyDef: binary file layout");
static_assert(sizeof(GameKeyFrameElementCompOld) == 12, "GameKeyFrameElementCompOld: binary file layout");
static_assert(sizeof(GameKeyFrameElementComp) == 8, "GameKeyFrameElementComp: binary file layout");
static_assert(sizeof(GameKeyFrameElementBig) == 20, "GameKeyFrameElementBig: binary file layout");
static_assert(sizeof(GameKeyFrameElement) == 20, "GameKeyFrameElement: binary file layout");
static_assert(sizeof(GameFightCol_Disk) == 16, "GameFightCol_Disk: binary file layout");
static_assert(sizeof(GameKeyFrame_Disk) == 20, "GameKeyFrame_Disk: binary file layout");

// ---- Fight collision (editor/pre-bake format) ----

// Extended fight collision used in KeyFrameChunk (editor format).
// uc_orig: FightCol (fallen/Editor/Headers/Anim.h)
struct FightCol {
    UWORD Dist1;
    UWORD Dist2;

    UBYTE Angle;
    UBYTE Priority;
    UBYTE Damage;
    UBYTE Tween;

    UBYTE AngleHitFrom;
    UBYTE Height;
    UBYTE Width;
    UBYTE Dummy;

    ULONG Dummy2;

    struct FightCol* Next;
};

// ---- Game keyframe chunk ----

// Heap-allocated animation block for one character type. Holds all keyframes,
// element data, fight collisions, and BodyDef table for that rig.
// uc_orig: GameKeyFrameChunk (fallen/Editor/Headers/Anim.h)
struct GameKeyFrameChunk {
    UWORD MultiObject[10];
    SLONG ElementCount;
    struct BodyDef* PeopleTypes;
    struct GameKeyFrame* AnimKeyFrames;
    struct GameKeyFrame** AnimList;
    struct GameKeyFrameElement* TheElements;
    struct GameFightCol* FightCols;

    SWORD MaxPeopleTypes;
    SWORD MaxKeyFrames;
    SWORD MaxAnimFrames;
    SWORD MaxFightCols;
    SLONG MaxElements;
};

// ---- Editor keyframe types ----

// Full-precision per-bone element used in editor (not compressed).
// uc_orig: KeyFrameElement (fallen/Editor/Headers/Anim.h)
struct KeyFrameElement {
    struct CMatrix33 CMatrix;
    struct Matrix33 Matrix;
    SLONG OffsetX;
    SLONG OffsetY;
    SLONG OffsetZ;
    UWORD Next;
    UWORD Parent;
};

// Editor keyframe node.
// uc_orig: KeyFrame (fallen/Editor/Headers/Anim.h)
struct KeyFrame {
    SWORD ChunkID;
    SWORD Flags;
    SLONG FrameID,
        TweenStep,
        ElementCount;
    KeyFrameElement* FirstElement;
    KeyFrame *PrevFrame,
        *NextFrame;
    SWORD Dx, Dy, Dz;
    SWORD Fixed;
    struct FightCol* Fight;
};

// Editor animation chunk: one .anm file worth of data.
// uc_orig: KeyFrameChunk (fallen/Editor/Headers/Anim.h)
struct KeyFrameChunk {
    CBYTE ANMName[64],
        ASCName[64],
        VUEName[64];
    UWORD MultiObject;
    UWORD MultiObjectStart;
    UWORD MultiObjectEnd;
    SLONG ElementCount;
    struct BodyDef PeopleTypes[MAX_PEOPLE_TYPES];
    CBYTE PeopleNames[MAX_PEOPLE_TYPES][PEOPLE_NAME_SIZE];
    KeyFrame KeyFrames[MAX_NUMBER_OF_FRAMES];
    KeyFrameElement* FirstElement;
    KeyFrameElement* LastElement;
    SLONG KeyFrameCount;
};

// Default editor position/size for the keyframe editor window.
// uc_orig: KeyFrameEdDefaults (fallen/Editor/Headers/Anim.h)
struct KeyFrameEdDefaults {
    SLONG Left,
        Top;
    SLONG Height,
        Width;
};

// ---- Anim class (editor/runtime linked list of keyframes) ----

// A named animation: a linked list of KeyFrames with playback metadata.
// uc_orig: Anim (fallen/Editor/Headers/Anim.h)
class Anim {
private:
    CBYTE AnimName[ANIM_NAME_SIZE];
    ULONG AnimFlags;
    SLONG FrameCount;
    Anim *LastAnim,
        *NextAnim;
    KeyFrame *CurrentFrame,
        *FrameListEnd,
        *FrameListStart;
    UBYTE TweakSpeed;

public:
    inline void SetAnimName(CBYTE* string) { strcpy(AnimName, string); }
    inline CBYTE* GetAnimName(void) { return AnimName; }
    inline void SetCurrentFrame(KeyFrame* the_frame) { CurrentFrame = the_frame; }
    inline KeyFrame* GetFrameList(void) { return FrameListStart; }
    inline void SetFrameList(KeyFrame* frame_list) { FrameListStart = frame_list; }
    inline KeyFrame* GetFrameListEnd(void) { return FrameListEnd; }
    inline KeyFrame* GetFrameListStart(void) { return FrameListStart; }
    inline void SetFrameListEnd(KeyFrame* frame_list_end) { FrameListEnd = frame_list_end; }
    inline SLONG GetFrameCount(void) { return FrameCount; }
    inline void SetFrameCount(SLONG count) { FrameCount = count; }
    inline ULONG GetAnimFlags(void) { return AnimFlags; }
    inline void SetAnimFlags(ULONG flags) { AnimFlags = flags; }
    inline void SetNextAnim(Anim* next) { NextAnim = next; }
    inline Anim* GetNextAnim(void) { return NextAnim; }
    inline void SetLastAnim(Anim* last) { LastAnim = last; }
    inline Anim* GetLastAnim(void) { return LastAnim; }
    inline UBYTE GetTweakSpeed(void) { return TweakSpeed; }
    inline void SetTweakSpeed(UBYTE speed) { TweakSpeed = speed; }
};

// A named character rig: holds a fixed array of Anim slots.
// uc_orig: Character (fallen/Editor/Headers/Anim.h)
class Character {
private:
    CBYTE CharName[32];
    UWORD MultiObject;
    Anim AnimList[50];

public:
    inline CBYTE* GetCharName(void) { return CharName; }
    inline void SetCharName(CBYTE* string) { strcpy(CharName, string); }
    inline UWORD GetMultiObject(void) { return MultiObject; }
    inline void SetMultiObject(UWORD multi) { MultiObject = multi; }
};

#endif // ENGINE_ANIMATION_ANIM_TYPES_H
