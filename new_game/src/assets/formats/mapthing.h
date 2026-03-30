#ifndef ASSETS_FORMATS_MAPTHING_H
#define ASSETS_FORMATS_MAPTHING_H

// Map thing structs used in PSX save file loading.
// Extracted from original fallen/Editor/Headers/Thing.h for runtime use.

#include "assets/formats/anim.h"

// uc_orig: MAP_THING_TYPE_PRIM (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_PRIM       1
// uc_orig: MAP_THING_TYPE_MULTI_PRIM (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_MULTI_PRIM 2
// uc_orig: MAP_THING_TYPE_ROT_MULTI (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_ROT_MULTI  3
// uc_orig: MAP_THING_TYPE_SPRITE (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_SPRITE     4
// uc_orig: MAP_THING_TYPE_AGENT (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_AGENT      5
// uc_orig: MAP_THING_TYPE_LIGHT (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_LIGHT      6
// uc_orig: MAP_THING_TYPE_BUILDING (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_BUILDING   7
// uc_orig: MAP_THING_TYPE_ANIM_PRIM (fallen/Editor/Headers/Thing.h)
#define MAP_THING_TYPE_ANIM_PRIM  8

// uc_orig: MapThingPSX (fallen/Editor/Headers/Thing.h)
// PSX map thing entry as stored in level files. Describes a placed object:
// position, type, animation state, and editor metadata.
#pragma pack(push, 1)
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
#pragma pack(pop)

#endif // ASSETS_FORMATS_MAPTHING_H
