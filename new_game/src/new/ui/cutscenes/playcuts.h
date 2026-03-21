#ifndef UI_CUTSCENES_PLAYCUTS_H
#define UI_CUTSCENES_PLAYCUTS_H

#include <MFStdLib.h>
#include "core/types.h"
#include "core/vector.h"

// Cutscene playback system: reads .cutscene files and drives characters + camera
// along recorded tracks. Used for in-game cutscenes authored in the level editor.
// Format: CPData -> CPChannel[] -> CPPacket[]. Each packet is a keyframe.

// uc_orig: MAX_CUTSCENES (fallen/Headers/playcuts.h)
#define MAX_CUTSCENES (20)
// uc_orig: MAX_CUTSCENE_TRACKS (fallen/Headers/playcuts.h)
#define MAX_CUTSCENE_TRACKS (20 * 15)
// uc_orig: MAX_CUTSCENE_PACKETS (fallen/Headers/playcuts.h)
#define MAX_CUTSCENE_PACKETS (20 * 15 * 128)
// uc_orig: MAX_CUTSCENE_TEXT (fallen/Headers/playcuts.h)
#define MAX_CUTSCENE_TEXT (4096)

// Forward declarations; Game.h types not included here.
// uc_orig: CPData (fallen/Headers/playcuts.h)
struct CPData;
// uc_orig: CPChannel (fallen/Headers/playcuts.h)
struct CPChannel;
// uc_orig: CPPacket (fallen/Headers/playcuts.h)
struct CPPacket;

// uc_orig: CPData (fallen/Headers/playcuts.h)
struct CPData {
    UBYTE version;
    UBYTE channelcount;
    UBYTE pad1, pad2;
    CPChannel* channels;
};

// uc_orig: CPChannel (fallen/Headers/playcuts.h)
struct CPChannel {
    UBYTE type;        // 0=unused, 1=character, 2=camera, 3=spot sound, 4=vfx
    UBYTE flags;
    UBYTE pad1, pad2;
    UWORD index;       // Sound/character index or effect type.
    UWORD packetcount;
    CPPacket* packets;
};

// uc_orig: CPPacket (fallen/Headers/playcuts.h)
struct CPPacket {
    UBYTE type;        // 0=unused, 1=animation, 2=action, 3=sound, 4=camerarec
    UBYTE flags;
    UWORD index;       // Animation/sound index.
    UWORD start;       // Packet start time (frame number).
    UWORD length;      // Natural packet length (and packed fade level / lens in high byte).
    GameCoord pos;     // World-space position keyframe.
    UWORD angle, pitch;
};

// Read a cutscene from an open file handle into the static pool.
// uc_orig: PLAYCUTS_Read (fallen/Headers/playcuts.h)
CPData* PLAYCUTS_Read(MFFileHandle handle);

// Release a loaded cutscene (currently a no-op; pool is reset with PLAYCUTS_Reset).
// uc_orig: PLAYCUTS_Free (fallen/Headers/playcuts.h)
void PLAYCUTS_Free(CPData* cutscene);

// Run the cutscene playback loop; blocks until done or space is pressed.
// uc_orig: PLAYCUTS_Play (fallen/Headers/playcuts.h)
void PLAYCUTS_Play(CPData* cutscene);

// Reset the static pool counters so the pool can be reused.
// uc_orig: PLAYCUTS_Reset (fallen/Headers/playcuts.h)
void PLAYCUTS_Reset();

#endif // UI_CUTSCENES_PLAYCUTS_H
