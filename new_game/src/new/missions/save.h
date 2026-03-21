#ifndef MISSIONS_SAVE_H
#define MISSIONS_SAVE_H

#include "core/types.h"

// In-game save/load system: serializes all living things, their genus data, and EWAY timer
// state to "ingame.sav". Used for quick-save/load during gameplay (PC only).
// The save format is a flat binary stream: per-thing records tagged with a type byte,
// followed by an EWAY section. On load: things are cleared and restored from the stream.

// uc_orig: SAVE_ingame (fallen/Headers/save.h)
SLONG SAVE_ingame(CBYTE* fname);

// uc_orig: LOAD_ingame (fallen/Headers/save.h)
SLONG LOAD_ingame(CBYTE* fname);

#endif // MISSIONS_SAVE_H
