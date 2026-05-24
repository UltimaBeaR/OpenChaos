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

#endif // ENGINE_INPUT_MOUSE_GLOBALS_H
