#pragma once

// Compile-time debug toggles. Sibling to config.h — kept separate so the
// "main" config stays focused on user-facing tuning (resolution, vsync,
// FOV, render scale) while developer debug flags don't pollute it.
// Always keep these `false` in shipping builds.

// Paint the composition layer's outer pillar/letterbox bars dark red
// instead of black so the FBO boundary is visible during layout
// debugging.
#define OC_DEBUG_COMPOSITION_BARS_RED false

// Strongly tint + blur the scene during composition so the UI/scene
// split is visible at a glance — anything touched by the composition
// pass (= the 3D scene and any UI that's still living in the scene FBO)
// comes out magenta and blurry; UI that's drawn in the post-composition
// pass stays sharp and original colours. Used to audit which call sites
// still need to migrate.
#define OC_DEBUG_HIGHLIGHT_NON_UI false

// Silence all sound/music by forcing the OpenAL listener gain to 0 right
// after init. OpenAL itself stays fully operational (skipping init
// caused busy AL_INVALID_OPERATION spin in update paths and tanked FPS)
// — this is the cheap mute knob.
#define OC_DEBUG_SOUND_DISABLED false
