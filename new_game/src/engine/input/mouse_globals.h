#ifndef ENGINE_INPUT_MOUSE_GLOBALS_H
#define ENGINE_INPUT_MOUSE_GLOBALS_H

#include "engine/core/types.h"

// Cursor position in scene-FBO pixel coordinates. Updated on every SDL3
// mouse-motion event regardless of whether the cursor is captured —
// captured-mode reads infinite relative motion, but the last known
// absolute FBO position stays available here (used by debug mine-spawn
// and teleport-to-cursor under the bangunsnotgames gate).
extern volatile SLONG MouseX;
extern volatile SLONG MouseY;

// Accumulated relative motion delta since the last consumer read.
// Updated by every SDL3 mouse-motion event (independent of capture state
// or position). Consumers (mouse camera) call input_mouse_consume_rel()
// each tick — that function reads and resets the accumulator atomically
// from the consumer's perspective. Window-pixel units; mouse-driven
// camera scales by its own sensitivity constants.
extern volatile SLONG MouseRelDX;
extern volatile SLONG MouseRelDY;

#endif // ENGINE_INPUT_MOUSE_GLOBALS_H
