// uc_orig_file: fallen/Editor/Headers/Thing.h (partial copy: only MapThingPSX + MAP_THING_TYPE_* constants)
// mapthing.h
// Map thing structs used in save file loading (PSX save format).
// Extracted from original Editor/Headers/Thing.h for runtime use.

#ifndef FALLEN_HEADERS_MAPTHING_H
#define FALLEN_HEADERS_MAPTHING_H

#include "anim.h"

//---------------------------------------------------------------

#define MAP_THING_TYPE_PRIM 1
#define MAP_THING_TYPE_MULTI_PRIM 2
#define MAP_THING_TYPE_ROT_MULTI 3
#define MAP_THING_TYPE_SPRITE 4
#define MAP_THING_TYPE_AGENT 5
#define MAP_THING_TYPE_LIGHT 6
#define MAP_THING_TYPE_BUILDING 7
#define MAP_THING_TYPE_ANIM_PRIM 8

struct MapThingPSX {
    SLONG X;
    SLONG Y;
    SLONG Z;
    UWORD MapChild;
    UWORD MapParent;
    UBYTE Type;
    UBYTE SubType;
    ULONG Flags;

    SWORD IndexOther;
    UWORD Width;
    UWORD Height;
    UWORD IndexOrig;
    UWORD AngleX;
    UWORD AngleY;
    UWORD AngleZ;
    UWORD IndexNext;
    SWORD LinkSame;
    SWORD OnFace;
    SWORD State;
    SWORD SubState;
    SLONG BuildingList;
    ULONG EditorFlags,
        EditorData;
    ULONG DummyArea[3];
    SLONG TweenStage;
    KeyFrame* CurrentFrame;
    KeyFrame* NextFrame;
};

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_MAPTHING_H
