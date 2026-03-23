// uc_orig_file: fallen/Editor/Headers/Anim.h
#ifndef FALLEN_HEADERS_ANIM_H
#define FALLEN_HEADERS_ANIM_H

#include "Prim.h"

// define s
#define MAX_CREATURE_TYPES 20
#define MAX_ANIMS_PER_CREATURE 20
#define MAX_MATRIX_POOL 10000

#define MAX_PEOPLE_TYPES 20
#define MAX_BODY_BITS 18
#define MAX_BODY_VARIETY 10
#define PEOPLE_NAME_SIZE 15

#define ANIM_PERSON 1
#define ANIM_RHINO 2
#define ANIM_APE 3

// #define	ANIM_TYPE_WALK						1
// #define	ANIM_TYPE_RUN						2
// #define	ANIM_TYPE_JUMP						3

#define TWEEN_OFFSET_SHIFT 0

#define ANIM_FLAG_LAST_FRAME (1 << 0)

// structures

struct AnimItem {
    UWORD Start;
    UWORD End;
};

struct PrimMultiAnim {
    struct Matrix33 Mat;
    SLONG DX;
    SLONG DY;
    SLONG DZ;
    UWORD Next;
};

struct BodyDef {
    UBYTE BodyPart[20]; // 1..14 is a normal person
};

// data
extern struct AnimItem anim_item[MAX_CREATURE_TYPES][MAX_ANIMS_PER_CREATURE];
extern struct PrimMultiAnim prim_multi_anims[]; // 500K

extern UWORD next_prim_multi_anim;

// FUNCTIONS

extern SBYTE read_a_multi_vue(SLONG m_object);
extern void animate_and_draw_chap(void);
extern void setup_people_anims(void);
extern void setup_extra_anims(void);
extern void setup_global_anim_array(void);

extern SLONG next_game_chunk;
extern SLONG next_anim_chunk;

// Guys stuff.

#define MAX_NUMBER_OF_CHUNKS 5
#define MAX_NUMBER_OF_FRAMES 5000
#define MAX_NUMBER_OF_ELEMENTS 80000

//************************************
//  Game Stuff
//************************************

//
// This contains the prim objects compressed matrix and offset for each object of each keyframe
//
struct KeyFrameElement {
    struct CMatrix33 CMatrix;
    struct Matrix33 Matrix;
    SLONG OffsetX;
    SLONG OffsetY;
    SLONG OffsetZ;
    UWORD Next;
    UWORD Parent;
};

//************************************************************************************************
//************************************************************************************************

//************************************************************************************************

struct GameKeyFrameElementCompOld {
    SBYTE m00, m01, m10, m11;
    //	SBYTE				dm02, dm12, dm20, dm21, dm22;
    //	UBYTE				pad1,pad2,pad3;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};
struct GameKeyFrameElementComp {
    SBYTE m00, m01, m10, m11;
    //	SBYTE				dm02, dm12, dm20, dm21, dm22;
    //	UBYTE				pad1,pad2,pad3;
    SBYTE OffsetX;
    SBYTE OffsetY;
    SBYTE OffsetZ;
    UBYTE Pad;
};

struct GameKeyFrameElementBig {
    struct CMatrix33 CMatrix;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};

struct GameKeyFrameElement {
    struct CMatrix33 CMatrix;
    SWORD OffsetX;
    SWORD OffsetY;
    SWORD OffsetZ;
    UWORD Pad;
};

// inline	void	GetCMatrix(GameKeyFrameElement *e, CMatrix33 *c)
inline void GetCMatrix(GameKeyFrameElement* e, CMatrix33* c)
{
    *c = e->CMatrix;
}

inline void SetCMatrix(GameKeyFrameElement* e, CMatrix33* c)
{
    e->CMatrix = *c;
};

//************************************************************************************************
//************************************************************************************************
//************************************************************************************************

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

//
// This Contains
//
struct GameKeyFrame {
    UBYTE XYZIndex;
    UBYTE TweenStep;
    SWORD Flags;
    GameKeyFrameElement* FirstElement;
    GameKeyFrame *PrevFrame, *NextFrame;
    GameFightCol* Fight;
};

/*
struct	GameKeyFrameChunk
{
        UWORD						MultiObject[10];
        SLONG						ElementCount;
        struct	BodyDef				PeopleTypes[MAX_PEOPLE_TYPES];
        struct GameKeyFrame			AnimKeyFrames[20000];
        struct GameKeyFrame			*AnimList[200];
        struct GameKeyFrameElement	TheElements[MAX_NUMBER_OF_ELEMENTS];
        SLONG	MaxKeyFrames;
        SLONG	MaxElements;
};
*/

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

struct GameKeyFrameChunk {
    UWORD MultiObject[10];
    SLONG ElementCount;
    struct BodyDef* PeopleTypes; //[MAX_PEOPLE_TYPES];
    struct GameKeyFrame* AnimKeyFrames; //[MAX_NUMBER_OF_FRAMES];
    struct GameKeyFrame** AnimList; //[200];
    struct GameKeyFrameElement* TheElements; //[MAX_NUMBER_OF_ELEMENTS];
    struct GameFightCol* FightCols; //[200];

    SWORD MaxPeopleTypes;
    SWORD MaxKeyFrames;
    SWORD MaxAnimFrames;
    SWORD MaxFightCols;
    SLONG MaxElements;
};

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
    //	UBYTE				BodyBits[MAX_BODY_PARTS][MAX_BODY_VARIETY]; //10 types of each body part
    KeyFrame KeyFrames[MAX_NUMBER_OF_FRAMES];
    KeyFrameElement* FirstElement;
    KeyFrameElement* LastElement;
    SLONG KeyFrameCount;
};

struct KeyFrameEdDefaults {
    SLONG Left,
        Top;
    SLONG Height,
        Width;
    //	KeyFrameChunk		KeyFrameChunks[MAX_NUMBER_OF_CHUNKS];
};

#define ANIM_NAME_SIZE 64
#define ANIM_LOOP 1

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
    Anim();
    ~Anim();
    void AddKeyFrame(KeyFrame* the_frame);
    void RemoveKeyFrame(KeyFrame* the_frame);
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
    void SetAllTweens(SLONG tween);
    void StartLooping(void);
    void StopLooping(void);
    inline void SetNextAnim(Anim* next) { NextAnim = next; }
    inline Anim* GetNextAnim(void) { return NextAnim; }
    inline void SetLastAnim(Anim* last) { LastAnim = last; }
    inline Anim* GetLastAnim(void) { return LastAnim; }
    inline UBYTE GetTweakSpeed(void) { return TweakSpeed; }
    inline void SetTweakSpeed(UBYTE speed) { TweakSpeed = speed; }
};

class Character {
private:
    CBYTE CharName[32];
    UWORD MultiObject;
    Anim AnimList[50];

public:
    void AddAnim(Anim* the_anim);
    void RemoveAnim(Anim* the_anim);
    inline CBYTE* GetCharName(void) { return CharName; }
    inline void SetCharName(CBYTE* string) { strcpy(CharName, string); }
    inline UWORD GetMultiObject(void) { return MultiObject; }
    inline void SetMultiObject(UWORD multi) { MultiObject = multi; }
};

//
// Anim initailisation stuff.
//

void ANIM_init(void);
void ANIM_fini(void);

#endif // FALLEN_HEADERS_ANIM_H
