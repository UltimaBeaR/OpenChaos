#ifndef ENGINE_GRAPHICS_BACKEND_DIRECTX6_INTERNAL_H
#define ENGINE_GRAPHICS_BACKEND_DIRECTX6_INTERNAL_H

// Internal backend declarations shared between common/ and game/ subdirectories.
// NOT part of the public ge_* contract.

#include "engine/graphics/graphics_engine/game_graphics_engine.h"

// Backend-local data directory (set via ge_set_data_dir, stored in game/core.cpp).
const char* ge_get_data_dir();

// Callbacks stored in game/core.cpp, called from common/ code.
extern GERenderStatesResetCallback s_render_states_reset_callback;
extern GETGALoadCallback s_tga_load_callback;
extern GETextureReloadPrepareCallback s_texture_reload_prepare_callback;

#endif // ENGINE_GRAPHICS_BACKEND_DIRECTX6_INTERNAL_H
