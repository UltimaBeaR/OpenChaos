#ifndef THINGS_CORE_SWITCH_H
#define THINGS_CORE_SWITCH_H

#include "engine/core/types.h"

#include "things/core/common.h"

struct Thing;

// uc_orig: MAX_SWITCHES (fallen/Headers/Switch.h)
#define MAX_SWITCHES 2

// Switch type identifiers.
// uc_orig: SWITCH_NONE (fallen/Headers/Switch.h)
#define SWITCH_NONE 0
// uc_orig: SWITCH_PLAYER (fallen/Headers/Switch.h)
#define SWITCH_PLAYER 1
// uc_orig: SWITCH_THING (fallen/Headers/Switch.h)
#define SWITCH_THING 2
// uc_orig: SWITCH_GROUP (fallen/Headers/Switch.h)
#define SWITCH_GROUP 3
// uc_orig: SWITCH_CLASS (fallen/Headers/Switch.h)
#define SWITCH_CLASS 4

// Switch state flags.
// uc_orig: SWITCH_FLAGS_TRIGGERED (fallen/Headers/Switch.h)
#define SWITCH_FLAGS_TRIGGERED (1 << 0)
// uc_orig: SWITCH_FLAGS_RESET (fallen/Headers/Switch.h)
#define SWITCH_FLAGS_RESET (1 << 1)

// Scan mode constants (unused in practice — only sphere is implemented).
// uc_orig: SCAN_MODE_SPHERE (fallen/Headers/Switch.h)
#define SCAN_MODE_SPHERE 0
// uc_orig: SCAN_MODE_RECT (fallen/Headers/Switch.h)
#define SCAN_MODE_RECT 1

// uc_orig: Switch (fallen/Headers/Switch.h)
// A trigger volume that fires when a thing enters its radius.
typedef struct
{
    COMMON(SwitchType)

    UBYTE ScanMode;
    UBYTE padtoword;
    UWORD Scanee;

    SLONG Depth,
        Height,
        Radius,
        Width;
} Switch;

typedef Switch* SwitchPtr;

#endif // THINGS_CORE_SWITCH_H
