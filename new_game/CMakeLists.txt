cmake_minimum_required(VERSION 3.25)

# CMake 4.2 introduced CMP0172: use $Config variable in Ninja rule names.
# Ninja does not support variable references in rule names — disable this policy.
if(POLICY CMP0172)
    cmake_policy(SET CMP0172 OLD)
endif()

# Disable vcpkg's automatic DLL copying (applocal.ps1): it invokes powershell.exe
# via cmd.exe without a full path, which fails when building from non-standard shells.
# We copy DLLs manually below via add_custom_command.
set(VCPKG_APPLOCAL_DEPS OFF)
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF)

project(OpenChaos LANGUAGES CXX)

# Windows: enable the RC language so the .exe gets an embedded application icon
# (resources/icon/OpenChaos.rc -> OpenChaos.ico). No-op on other platforms.
if(WIN32)
    enable_language(RC)
endif()

# Emit compile_commands.json for clangd / other tooling. Ninja Multi-Config
# writes it to ${CMAKE_BINARY_DIR}/compile_commands.json (i.e. new_game/build/).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# C++20 standard — required for libDualsense and future modules (Stage 9)
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Require Clang 20+ (C++20 support). Apple Clang uses different versioning — skip check.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
   AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "AppleClang"
   AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "20.0")
    message(FATAL_ERROR "Clang 20+ is required. Found: ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# ---------------------------------------------------------------------------
# Windows: static CRT (single self-contained exe)
# ---------------------------------------------------------------------------
# The Windows vcpkg triplet is x64-windows-static (see cmake/clang-x64-windows
# .cmake), which builds every dependency with the static CRT (/MT). Our own
# translation units must match, otherwise the linker rejects the mix of static
# (libcmt) and dynamic (msvcrt) runtimes. CMake injects the CRT selection from
# CMAKE_MSVC_RUNTIME_LIBRARY (default = dynamic DLL), so set it to the static
# runtime here — before add_subdirectory below, so libDualsense inherits it.
# Requires CMP0091 = NEW (default at our cmake_minimum_required).
if(WIN32)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------------------
# Build output: new_game/build/Debug/ and new_game/build/Release/
# All generated artifacts stay inside build/ — a single gitignore entry covers everything.
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")

# ---------------------------------------------------------------------------
# Dependencies (via vcpkg)
# ---------------------------------------------------------------------------
find_package(SDL3         CONFIG REQUIRED)
find_package(OpenAL       CONFIG REQUIRED)

# Static FFmpeg's FindFFMPEG.cmake resolves FFmpeg's private system-lib deps via
# find_library. Its pass-through list (libs it links by bare name without
# searching) is missing `ncrypt`, so it calls find_library(ncrypt REQUIRED) —
# which fails under our standalone clang++ toolchain because CMake has no Windows
# SDK lib path (we don't run from a VS dev environment). Pre-seed the cache var
# so find_library is skipped; clang resolves ncrypt from the SDK at link time
# (same as the other system libs in that pass-through list).
if(WIN32)
    set(FFMPEG_DEPENDENCY_ncrypt_RELEASE ncrypt CACHE FILEPATH "" FORCE)
    set(FFMPEG_DEPENDENCY_ncrypt_DEBUG   ncrypt CACHE FILEPATH "" FORCE)
endif()

find_package(FFMPEG       REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Optional: AddressSanitizer for memory error detection (heap overflows, etc.)
# Usage: cmake -DENABLE_ASAN=ON ... (or `make configure-asan`)
# ---------------------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer (heap overflow detection)" OFF)

# ---------------------------------------------------------------------------
# Optional: dead-code report. Enables function-level linking + verbose linker
# output so the linker prints which functions/sections were discarded as
# unreferenced. Used for Stage 13 dead-code cleanup (see
# new_game_planning/stage13_dead_code_cleanup.md).
# Usage: CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure && make build-release
# Then check build/Release/.ninja_log or capture build output to a file.
# ---------------------------------------------------------------------------
option(DEAD_CODE_REPORT "Print dropped (dead) sections during link" OFF)

# ---------------------------------------------------------------------------
# DualSense backend
# ---------------------------------------------------------------------------
# libDualsense itself lives at repo root as an independent static library
# (see ../libDualsense/CMakeLists.txt). Here we only list the bridge that
# adapts it for OpenChaos game code. The lib target is added via
# add_subdirectory further below and linked into OpenChaos as a dependency.
set(DS_BRIDGE_SOURCES
    src/engine/platform/ds_bridge.cpp
)

add_subdirectory(${CMAKE_SOURCE_DIR}/../libDualsense libDualsense_build)

# ---------------------------------------------------------------------------
# Graphics backend: OpenGL 4.1 Core Profile (only backend)
# ---------------------------------------------------------------------------

# Embed GLSL shaders as C++ string constants at build time.
# Uses a CMake script invoked via add_custom_command so changes to .glsl
# files are picked up automatically on every build (not just cmake configure).
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/engine/graphics/graphics_engine/backend_opengl/shaders")
set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/gl_shaders_embedded.h")
set(SHADER_EMBED_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/embed_shaders.cmake")
file(WRITE "${SHADER_EMBED_SCRIPT}" "
file(READ \"\${SHADER_DIR}/tl_vert.glsl\" TL)
file(READ \"\${SHADER_DIR}/skin_world_vert.glsl\" SKIN_WORLD_VERT)
file(READ \"\${SHADER_DIR}/skin_shadow_vert.glsl\" SKIN_SHADOW_VERT)
file(READ \"\${SHADER_DIR}/skin_reflect_vert.glsl\" SKIN_REFLECT_VERT)
file(READ \"\${SHADER_DIR}/shadow_sil_frag.glsl\" SHADOW_SIL_FRAG)
file(READ \"\${SHADER_DIR}/common_frag.glsl\" FRAG)
file(READ \"\${SHADER_DIR}/video_vert.glsl\" VID_VERT)
file(READ \"\${SHADER_DIR}/video_frag.glsl\" VID_FRAG)
file(READ \"\${SHADER_DIR}/fullscreen_quad_vert.glsl\" FSQ_VERT)
file(READ \"\${SHADER_DIR}/wibble_frag.glsl\" WIBBLE_FRAG)
file(READ \"\${SHADER_DIR}/composite_frag.glsl\" COMPOSITE_FRAG)
file(READ \"\${SHADER_DIR}/crt_frag.glsl\" CRT_FRAG)
# Resolve `#include \\\"_skin_common.glsl\\\"` in skin shaders via plain text
# substitution. GLSL has no preprocessor for #include, so we do it here at
# embed time — keeps shared layout/uniform declarations in one file.
file(READ \"\${SHADER_DIR}/_skin_common.glsl\" SKIN_COMMON)
string(REPLACE \"#include \\\"_skin_common.glsl\\\"\" \"\${SKIN_COMMON}\" SKIN_WORLD_VERT \"\${SKIN_WORLD_VERT}\")
string(REPLACE \"#include \\\"_skin_common.glsl\\\"\" \"\${SKIN_COMMON}\" SKIN_SHADOW_VERT \"\${SKIN_SHADOW_VERT}\")
string(REPLACE \"#include \\\"_skin_common.glsl\\\"\" \"\${SKIN_COMMON}\" SKIN_REFLECT_VERT \"\${SKIN_REFLECT_VERT}\")
file(WRITE \"\${OUT}\"
    \"// Auto-generated from .glsl files — do not edit.\\n\"
    \"static const char* const SHADER_TL_VERT = R\\\"glsl(\\n\${TL})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_SKIN_WORLD_VERT = R\\\"glsl(\\n\${SKIN_WORLD_VERT})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_SKIN_SHADOW_VERT = R\\\"glsl(\\n\${SKIN_SHADOW_VERT})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_SKIN_REFLECT_VERT = R\\\"glsl(\\n\${SKIN_REFLECT_VERT})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_SHADOW_SIL_FRAG = R\\\"glsl(\\n\${SHADOW_SIL_FRAG})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_FRAG = R\\\"glsl(\\n\${FRAG})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_VIDEO_VERT = R\\\"glsl(\\n\${VID_VERT})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_VIDEO_FRAG = R\\\"glsl(\\n\${VID_FRAG})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_FULLSCREEN_QUAD_VERT = R\\\"glsl(\\n\${FSQ_VERT})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_WIBBLE_FRAG = R\\\"glsl(\\n\${WIBBLE_FRAG})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_COMPOSITE_FRAG = R\\\"glsl(\\n\${COMPOSITE_FRAG})glsl\\\";\\n\\n\"
    \"static const char* const SHADER_CRT_FRAG = R\\\"glsl(\\n\${CRT_FRAG})glsl\\\";\\n\"
)
")
add_custom_command(
    OUTPUT "${SHADER_HEADER}"
    COMMAND ${CMAKE_COMMAND} -DSHADER_DIR="${SHADER_DIR}" -DOUT="${SHADER_HEADER}" -P "${SHADER_EMBED_SCRIPT}"
    DEPENDS
        "${SHADER_DIR}/tl_vert.glsl"
        "${SHADER_DIR}/skin_world_vert.glsl"
        "${SHADER_DIR}/skin_shadow_vert.glsl"
        "${SHADER_DIR}/skin_reflect_vert.glsl"
        "${SHADER_DIR}/_skin_common.glsl"
        "${SHADER_DIR}/shadow_sil_frag.glsl"
        "${SHADER_DIR}/common_frag.glsl"
        "${SHADER_DIR}/video_vert.glsl"
        "${SHADER_DIR}/video_frag.glsl"
        "${SHADER_DIR}/fullscreen_quad_vert.glsl"
        "${SHADER_DIR}/wibble_frag.glsl"
        "${SHADER_DIR}/composite_frag.glsl"
        "${SHADER_DIR}/crt_frag.glsl"
    COMMENT "Embedding GLSL shaders into gl_shaders_embedded.h"
)

# Embed the input-prompt glyph PNG atlases (Kenney Input Prompts, CC0) as C
# byte arrays at build time. Same mechanism as the shader embed above: a
# generated CMake script invoked via add_custom_command, so edits to the PNGs
# are picked up on every build. file(READ ... HEX) reads each PNG as a hex
# string; string(REGEX REPLACE) turns it into "0x..,0x..,".
set(GLYPH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/input_glyphs/assets")
set(GLYPH_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/input_glyphs_embedded.h")
set(GLYPH_EMBED_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/embed_input_glyphs.cmake")
file(WRITE "${GLYPH_EMBED_SCRIPT}" "
file(WRITE \"\${OUT}\" \"// Auto-generated from input-prompt PNG atlases — do not edit.\\n\")
foreach(PAIR KEYBOARD=glyphs_keyboard GENERIC=glyphs_generic PS=glyphs_ps DECK=glyphs_deck)
    string(REPLACE \"=\" \";\" PAIR_LIST \"\${PAIR}\")
    list(GET PAIR_LIST 0 NAME)
    list(GET PAIR_LIST 1 FILE)
    file(READ \"\${GLYPH_DIR}/\${FILE}.png\" HEX HEX)
    string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" BYTES \"\${HEX}\")
    file(APPEND \"\${OUT}\"
        \"static const unsigned char GLYPH_\${NAME}_PNG[] = { \${BYTES} };\\n\"
        \"static const unsigned int  GLYPH_\${NAME}_PNG_LEN = sizeof(GLYPH_\${NAME}_PNG);\\n\\n\")
endforeach()
")
add_custom_command(
    OUTPUT "${GLYPH_HEADER}"
    COMMAND ${CMAKE_COMMAND} -DGLYPH_DIR="${GLYPH_DIR}" -DOUT="${GLYPH_HEADER}" -P "${GLYPH_EMBED_SCRIPT}"
    DEPENDS
        "${GLYPH_DIR}/glyphs_keyboard.png"
        "${GLYPH_DIR}/glyphs_generic.png"
        "${GLYPH_DIR}/glyphs_ps.png"
        "${GLYPH_DIR}/glyphs_deck.png"
    COMMENT "Embedding input-prompt PNG atlases into input_glyphs_embedded.h"
)

# Embed the window-icon PNG (resources/icon/window_icon.png) as a C byte array
# for SDL_SetWindowIcon at runtime. On Windows the .exe resource already
# supplies the taskbar icon; on Linux this embedded PNG is the only source.
# Same file(READ ... HEX) mechanism as the glyph embed above.
set(WINDOW_ICON_PNG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/window_icon.png")
set(WINDOW_ICON_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/window_icon_embedded.h")
set(WINDOW_ICON_EMBED_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/embed_window_icon.cmake")
file(WRITE "${WINDOW_ICON_EMBED_SCRIPT}" "
file(READ \"\${SRC}\" HEX HEX)
string(REGEX REPLACE \"(..)\" \"0x\\\\1,\" BYTES \"\${HEX}\")
file(WRITE \"\${OUT}\"
    \"// Auto-generated from resources/icon/window_icon.png — do not edit.\\n\"
    \"static const unsigned char WINDOW_ICON_PNG[] = { \${BYTES} };\\n\"
    \"static const unsigned int  WINDOW_ICON_PNG_LEN = sizeof(WINDOW_ICON_PNG);\\n\")
")
add_custom_command(
    OUTPUT "${WINDOW_ICON_HEADER}"
    COMMAND ${CMAKE_COMMAND} -DSRC="${WINDOW_ICON_PNG_FILE}" -DOUT="${WINDOW_ICON_HEADER}" -P "${WINDOW_ICON_EMBED_SCRIPT}"
    DEPENDS "${WINDOW_ICON_PNG_FILE}"
    COMMENT "Embedding window icon PNG into window_icon_embedded.h"
)

set(BACKEND_SOURCES
    # --- GLAD loader ---
    src/engine/graphics/graphics_engine/backend_opengl/common/glad/src/gl.c
    # --- common ---
    src/engine/graphics/graphics_engine/backend_opengl/common/gl_context.cpp
    src/engine/graphics/graphics_engine/backend_opengl/common/gl_shader.cpp
    # --- game (ge_* implementation) ---
    src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp
    # --- post-process (GPU effect passes) ---
    src/engine/graphics/graphics_engine/backend_opengl/postprocess/wibble_effect.cpp
    src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp
    src/engine/graphics/graphics_engine/backend_opengl/postprocess/crt_effect.cpp
    # --- outro (oge_* implementation) ---
    src/engine/graphics/graphics_engine/backend_opengl/outro/core.cpp
    # --- API-agnostic render state cache ---
    src/engine/graphics/graphics_engine/game_graphics_engine.cpp
)
# OpenGL library: platform-specific name
if(WIN32)
    # dwmapi — used by DwmFlush() as a VSync replacement to avoid the
    # NVIDIA/WDDM hang on hard SDL VSync (see startup_hang_investigation).
    set(BACKEND_LIBS opengl32 dwmapi)
elseif(APPLE)
    find_package(OpenGL REQUIRED)
    # CoreGraphics: physical display size (CGDisplayScreenSize) for the CRT
    # effect's real-DPI scanline pitch (see sdl3_display_get_physical_ppcm).
    find_library(CORE_GRAPHICS_LIB CoreGraphics REQUIRED)
    set(BACKEND_LIBS OpenGL::GL ${CORE_GRAPHICS_LIB})
else()
    set(BACKEND_LIBS GL)
endif()
message(STATUS "Graphics backend: OpenGL 4.1 Core Profile")

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(SOURCES_NEW
    # === Entry point ===
    src/main.cpp
    src/feature_flags.cpp

    # === Engine: core ===
    src/engine/core/memory.cpp
    src/engine/core/math.cpp
    src/engine/core/math_globals.cpp
    src/engine/core/matrix.cpp
    src/engine/core/fmatrix.cpp
    src/engine/core/quaternion.cpp
    src/engine/core/heap.cpp
    src/engine/core/heap_globals.cpp

    # === Engine: platform ===
    src/engine/platform/wind_procs_globals.cpp
    src/engine/platform/host.cpp
    src/engine/platform/host_globals.cpp
    src/engine/platform/crash_handler_win.cpp
    src/engine/platform/single_instance.cpp
    src/engine/platform/sdl3_bridge.cpp
    ${DS_BRIDGE_SOURCES}

    # === Engine: I/O ===
    src/engine/io/file.cpp
    src/engine/io/file_globals.cpp
    src/engine/io/user_data.cpp
    src/engine/io/env.cpp
    src/engine/io/oc_config.cpp

    # === Engine: input ===
    src/engine/input/keyboard.cpp
    src/engine/input/keyboard_globals.cpp
    src/engine/input/mouse.cpp
    src/engine/input/mouse_globals.cpp
    src/engine/input/mouse_capture.cpp
    src/engine/input/gamepad.cpp
    src/engine/input/gamepad_globals.cpp
    src/engine/input/weapon_feel.cpp
    src/engine/input/input_frame.cpp

    # === Engine: debug ===
    src/engine/debug/input_debug/input_debug.cpp
    src/engine/debug/input_debug/input_debug_keyboard.cpp
    src/engine/debug/input_debug/input_debug_gamepad.cpp
    src/engine/debug/input_debug/input_debug_dualsense.cpp
    src/engine/debug/debug_help/debug_help.cpp
    src/engine/debug/dbglog/dbglog.cpp
    src/engine/debug/perf_diag/perf_diag.cpp

    # === Engine: video ===
    src/engine/video/video_player.cpp

    # === Engine: audio ===
    src/engine/audio/mfx.cpp
    src/engine/audio/mfx_globals.cpp
    src/engine/audio/music.cpp
    src/engine/audio/music_globals.cpp
    src/engine/audio/sound.cpp
    src/engine/audio/sound_globals.cpp

    # === Engine: console ===
    src/engine/console/message.cpp
    src/engine/console/message_globals.cpp
    src/engine/console/console.cpp
    src/engine/console/console_globals.cpp

    # === Engine: animation ===
    src/engine/animation/morph.cpp
    src/engine/animation/morph_globals.cpp

    # === Engine: physics ===
    src/engine/physics/sm.cpp
    src/engine/physics/sm_globals.cpp
    src/engine/physics/collide.cpp
    src/engine/physics/collide_globals.cpp

    # === Engine: effects ===
    src/engine/effects/psystem.cpp
    src/engine/effects/psystem_globals.cpp

    # === Engine: graphics — pipeline ===
    src/engine/graphics/pipeline/polypage.cpp
    src/engine/graphics/pipeline/polypage_globals.cpp
    src/engine/graphics/pipeline/poly_render.cpp
    src/engine/graphics/pipeline/poly_render_globals.cpp
    src/engine/graphics/pipeline/aeng.cpp
    src/engine/graphics/pipeline/aeng_globals.cpp
    src/engine/graphics/render_interp.cpp
    src/engine/graphics/pipeline/poly.cpp
    src/engine/graphics/pipeline/poly_globals.cpp
    src/engine/graphics/pipeline/draw2d.cpp

    # === Engine: graphics — geometry ===
    src/engine/graphics/geometry/sprite.cpp
    src/engine/graphics/geometry/cone.cpp
    src/engine/graphics/geometry/cone_globals.cpp
    src/engine/graphics/geometry/farfacet.cpp
    src/engine/graphics/geometry/farfacet_globals.cpp
    src/engine/graphics/geometry/facet.cpp
    src/engine/graphics/geometry/facet_globals.cpp
    src/engine/graphics/geometry/sky.cpp
    src/engine/graphics/geometry/sky_globals.cpp
    src/engine/graphics/geometry/fastprim.cpp
    src/engine/graphics/geometry/fastprim_globals.cpp
    src/engine/graphics/geometry/oval.cpp
    src/engine/graphics/geometry/oval_globals.cpp
    src/engine/graphics/geometry/superfacet.cpp
    src/engine/graphics/geometry/superfacet_globals.cpp
    src/engine/graphics/geometry/figure.cpp
    src/engine/graphics/geometry/figure_globals.cpp
    src/engine/graphics/geometry/pose_composer.cpp
    src/engine/graphics/geometry/bind_palette.cpp
    src/engine/graphics/geometry/mesh.cpp
    src/engine/graphics/geometry/mesh_globals.cpp
    src/engine/graphics/geometry/shape.cpp
    src/engine/graphics/geometry/shape_globals.cpp

    # === Engine: graphics — text ===
    src/engine/graphics/text/font.cpp
    src/engine/graphics/text/font_atlas.cpp
    src/engine/graphics/text/font_globals.cpp
    src/engine/graphics/text/menufont.cpp
    src/engine/graphics/text/menufont_globals.cpp
    src/engine/graphics/text/text_globals.cpp
    src/engine/graphics/text/font2d.cpp
    src/engine/graphics/text/font2d_globals.cpp

    # === Engine: graphics — lighting ===
    src/engine/graphics/lighting/ngamut.cpp
    src/engine/graphics/lighting/ngamut_globals.cpp
    src/engine/graphics/lighting/shadow.cpp
    src/engine/graphics/lighting/smap.cpp
    src/engine/graphics/lighting/smap_globals.cpp
    src/engine/graphics/lighting/night.cpp
    src/engine/graphics/lighting/night_globals.cpp
    src/engine/graphics/lighting/crinkle.cpp

    # === Engine: graphics — postprocess ===
    src/engine/graphics/postprocess/wibble.cpp
    src/engine/graphics/postprocess/bloom.cpp
    src/engine/graphics/postprocess/bloom_globals.cpp

    # === Engine: graphics — UI coordinate system ===
    src/engine/graphics/ui_coords.cpp

    # === Engine: graphics — backend (selected by GRAPHICS_BACKEND) ===
    ${BACKEND_SOURCES}

    # === Assets ===
    src/assets/formats/file_clump.cpp
    src/assets/formats/tga.cpp
    src/assets/formats/tga_globals.cpp
    src/assets/formats/stb_image_impl.cpp
    src/assets/formats/anim.cpp
    src/assets/formats/anim_globals.cpp
    src/assets/formats/anim_loader.cpp
    src/assets/formats/anim_tmap.cpp
    src/assets/formats/anim_tmap_globals.cpp
    src/assets/formats/level_loader.cpp
    src/assets/formats/level_loader_globals.cpp
    src/assets/formats/elev.cpp
    src/assets/formats/elev_globals.cpp
    src/assets/texture.cpp
    src/assets/texture_globals.cpp
    src/assets/xlat_str.cpp
    src/assets/xlat_str_globals.cpp
    src/assets/sound_id_globals.cpp

    # === Map ===
    src/map/map.cpp
    src/map/pap.cpp
    src/map/pap_globals.cpp
    src/map/road.cpp
    src/map/road_globals.cpp
    src/map/sewers.cpp
    src/map/sewers_globals.cpp
    src/map/supermap.cpp
    src/map/supermap_globals.cpp
    src/map/ob.cpp
    src/map/ob_globals.cpp

    # === Navigation ===
    src/navigation/walkable.cpp
    src/navigation/walkable_globals.cpp
    src/navigation/inside2.cpp
    src/navigation/inside2_globals.cpp
    src/navigation/wand.cpp
    src/navigation/wmove.cpp
    src/navigation/wmove_globals.cpp

    # === Buildings ===
    src/buildings/build2.cpp
    src/buildings/building_globals.cpp
    src/buildings/prim.cpp
    src/buildings/prim_globals.cpp

    src/buildings/ware.cpp
    src/buildings/ware_globals.cpp

    # === Things: core ===
    src/things/core/drawtype.cpp
    src/things/core/state.cpp
    src/things/core/hierarchy.cpp
    src/things/core/hierarchy_globals.cpp
    src/things/core/player.cpp
    src/things/core/player_globals.cpp
    src/things/core/thing.cpp
    src/things/core/thing_globals.cpp
    src/things/core/interact.cpp
    src/things/core/interact_globals.cpp

    # === Things: characters ===
    src/things/characters/cop.cpp
    src/things/characters/cop_globals.cpp
    src/things/characters/roper.cpp
    src/things/characters/roper_globals.cpp
    src/things/characters/snipe.cpp
    src/things/characters/snipe_globals.cpp
    src/things/characters/darci.cpp
    src/things/characters/darci_globals.cpp
    src/things/characters/person.cpp
    src/things/characters/person_globals.cpp

    # === Things: animals ===
    src/things/animals/animal.cpp
    src/things/animals/animal_globals.cpp
    src/things/animals/bat.cpp
    src/things/animals/bat_globals.cpp

    # === Things: vehicles ===
    src/things/vehicles/chopper.cpp
    src/things/vehicles/chopper_globals.cpp
    src/things/vehicles/vehicle.cpp
    src/things/vehicles/vehicle_globals.cpp

    # === Things: items ===
    src/things/items/barrel.cpp
    src/things/items/barrel_globals.cpp
    src/things/items/balloon.cpp
    src/things/items/balloon_globals.cpp
    src/things/items/grenade.cpp
    src/things/items/grenade_globals.cpp
    src/things/items/hook.cpp
    src/things/items/hook_globals.cpp
    src/things/items/special.cpp
    src/things/items/special_globals.cpp

    # === World objects ===
    src/world_objects/dirt.cpp
    src/world_objects/dirt_globals.cpp
    src/world_objects/door.cpp
    src/world_objects/door_globals.cpp
    src/world_objects/plat.cpp
    src/world_objects/plat_globals.cpp
    src/world_objects/puddle.cpp
    src/world_objects/puddle_globals.cpp
    src/world_objects/tripwire.cpp
    src/world_objects/tripwire_globals.cpp

    # === Effects: combat ===
    src/effects/combat/glitter.cpp
    src/effects/combat/glitter_globals.cpp
    src/effects/combat/spark.cpp
    src/effects/combat/spark_globals.cpp
    src/effects/combat/pow.cpp
    src/effects/combat/pow_globals.cpp
    src/effects/combat/pyro.cpp
    src/effects/combat/pyro_globals.cpp
    src/effects/combat/glow.cpp

    # === Effects: environment ===
    src/effects/environment/fire.cpp
    src/effects/environment/fire_globals.cpp
    src/effects/environment/ribbon.cpp
    src/effects/environment/ribbon_globals.cpp
    src/effects/environment/tracks.cpp
    src/effects/environment/tracks_globals.cpp

    # === Effects: weather ===
    src/effects/weather/mist.cpp
    src/effects/weather/mist_globals.cpp
    src/effects/weather/drip.cpp
    src/effects/weather/drip_globals.cpp
    src/effects/weather/fog.cpp
    src/effects/weather/fog_globals.cpp

    # === Combat ===
    src/combat/combat.cpp
    src/combat/combat_globals.cpp
    src/combat/combo_gate.cpp
    src/combat/combat_cooldown.cpp
    src/combat/combat_test_mode.cpp
    src/shooting/guns.cpp
    src/shooting/projectile.cpp

    # === AI ===
    src/ai/mav.cpp
    src/ai/mav_globals.cpp
    src/ai/pcom.cpp
    src/ai/pcom_globals.cpp

    # === Missions ===
    src/missions/eway.cpp
    src/missions/eway_globals.cpp
    src/missions/playcuts.cpp
    src/missions/playcuts_globals.cpp
    src/missions/memory.cpp
    src/missions/memory_globals.cpp
    src/missions/save.cpp
    src/missions/save_globals.cpp

    # === Camera ===
    src/camera/camera_mode.cpp
    src/camera/fc.cpp
    src/camera/look_sensitivity.cpp
    src/camera/fc_globals.cpp
    src/camera/vis_cam.cpp

    # === UI ===
    src/ui/input_glyphs/input_glyphs.cpp
    src/ui/input_glyphs/input_glyph_coords.cpp
    src/ui/input_glyphs/input_prompt_map.cpp
    src/ui/frontend/startscr_globals.cpp
    src/ui/frontend/attract.cpp
    src/ui/frontend/attract_globals.cpp
    src/ui/frontend/frontend.cpp
    src/ui/frontend/frontend_globals.cpp
    src/ui/menus/gamemenu.cpp
    src/ui/menus/gamemenu_globals.cpp
    src/ui/menus/help_content.cpp
    src/ui/menus/help_content_kbm.cpp
    src/ui/menus/help_content_xbox.cpp
    src/ui/menus/help_content_ps.cpp
    src/ui/menus/widget.cpp
    src/ui/menus/widget_globals.cpp
    src/ui/hud/eng_map.cpp
    src/ui/hud/eng_map_globals.cpp
    src/ui/hud/overlay.cpp
    src/ui/hud/overlay_globals.cpp
    src/ui/hud/panel.cpp
    src/ui/hud/panel_globals.cpp

    # === Outro ===
    src/outro/core/outro_tga.cpp
    src/outro/core/outro_matrix.cpp
    src/outro/core/outro_cam.cpp
    src/outro/core/outro_cam_globals.cpp
    src/outro/core/outro_font.cpp
    src/outro/core/outro_font_globals.cpp
    src/outro/core/outro_imp.cpp
    src/outro/core/outro_mf.cpp
    src/outro/core/outro_mf_globals.cpp
    src/outro/core/outro_os.cpp
    src/outro/core/outro_os_globals.cpp
    src/outro/outro_back.cpp
    src/outro/outro_back_globals.cpp
    src/outro/outro_credits.cpp
    src/outro/outro_credits_globals.cpp
    src/outro/outro_wire.cpp
    src/outro/outro_wire_globals.cpp
    src/outro/outro_main.cpp

    # === Game ===
    src/game/game.cpp
    src/game/game_globals.cpp
    src/game/missing_resources.cpp
    src/game/network_state_globals.cpp
    src/game/startup.cpp
    src/game/game_tick.cpp
    src/game/game_tick_globals.cpp
    src/game/input_actions.cpp
    src/game/input_actions_globals.cpp
    src/game/ui_render.cpp
    src/game/debug_timing_overlay.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
# Windows: .rc resource embedding the application icon into the .exe.
# Empty on other platforms so add_executable below stays portable.
set(WIN_RESOURCES "")
if(WIN32)
    set(WIN_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/OpenChaos.rc)
endif()

add_executable(OpenChaos
    ${SOURCES_NEW}
    # Generated shader header (triggers rebuild when .glsl files change).
    ${CMAKE_CURRENT_BINARY_DIR}/generated/gl_shaders_embedded.h
    # Generated glyph-atlas header (triggers rebuild when PNGs change).
    ${CMAKE_CURRENT_BINARY_DIR}/generated/input_glyphs_embedded.h
    # Generated window-icon header (triggers rebuild when window_icon.png changes).
    ${WINDOW_ICON_HEADER}
    # Windows: application icon resource (no-op elsewhere).
    ${WIN_RESOURCES}
)
# WIN32 = SUBSYSTEM:WINDOWS (no console window). Only meaningful on Windows.
if(WIN32)
    set_target_properties(OpenChaos PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(OpenChaos PRIVATE
    # Main source root: resolves #include "core/types.h", "engine/...", etc.
    src
    # Platform abstraction: resolves #include <MFStdLib.h> and StdFile/StdKeybd/StdMaths/StdMouse
    src/engine/platform
    # GLAD GL loader headers (resolves #include <glad/gl.h>)
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/graphics/graphics_engine/backend_opengl/common/glad/include
    # Generated headers (embedded shaders)
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    # FFmpeg (video playback)
    ${FFMPEG_INCLUDE_DIRS}
)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------

# GLAD is a C file compiled as C++.
set_source_files_properties(
    src/engine/graphics/graphics_engine/backend_opengl/common/glad/src/gl.c
    PROPERTIES LANGUAGE CXX
)

# Strip the absolute build-path prefix from __FILE__ so assert / crash-log paths
# read repo-relative (e.g. "OpenChaos/new_game/src/...") instead of leaking where
# the source tree lives on the build machine — and so the absolute path is never
# even embedded in the shipped binary. __FILE__ is used only by ASSERT (->
# uc_assert_fail), so this affects only those log paths. CMAKE_SOURCE_DIR is
# .../OpenChaos/new_game, so its grandparent is the directory above the repo.
get_filename_component(OC_REPO_PARENT "${CMAKE_SOURCE_DIR}/../.." ABSOLUTE)

target_compile_options(OpenChaos PRIVATE
    # Repo-relative __FILE__ (see OC_REPO_PARENT above).
    -fmacro-prefix-map=${OC_REPO_PARENT}/=
    # Release: optimizations matching original vcxproj settings.
    # -fno-inline-functions: without this, Clang -O2 breaks body part rendering
    #   (FIGURE_draw_prim_tween_person_only_just_set_matrix inlining issue, TODO Этап 7+).
    # -fno-strict-aliasing: code uses pointer type punning (*(float*)&intVal),
    #   strict aliasing optimizations would produce incorrect results.
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Release>:-fno-inline-functions>
    $<$<CONFIG:Release>:-fno-strict-aliasing>
    # AddressSanitizer: detects heap overflows, use-after-free, etc.
    $<$<BOOL:${ENABLE_ASAN}>:-fsanitize=address>
    $<$<BOOL:${ENABLE_ASAN}>:-fno-omit-frame-pointer>
    # vcpkg static libs (x64-windows-static) are built without ASan. The MSVC STL
    # emits a /failifmismatch "annotate_string"/"annotate_vector" record when ASan
    # is on (value 1), which conflicts at link time with the non-instrumented libs
    # (value 0): lld-link errors with "mismatch detected for 'annotate_string'".
    # Disabling the STL container annotations makes our objects match (value 0).
    # We keep full heap/stack/global + use-after-free coverage; only the
    # std::string/std::vector internal-buffer overflow annotation is lost.
    # Gated on ENABLE_ASAN, so release builds are unaffected.
    $<$<BOOL:${ENABLE_ASAN}>:-D_DISABLE_STRING_ANNOTATION>
    $<$<BOOL:${ENABLE_ASAN}>:-D_DISABLE_VECTOR_ANNOTATION>
    # Dead-code report: function/data-level sections so linker can drop unreferenced ones.
    # On clang-cl/clang targeting MSVC, -ffunction-sections maps to /Gy.
    $<$<BOOL:${DEAD_CODE_REPORT}>:-ffunction-sections>
    $<$<BOOL:${DEAD_CODE_REPORT}>:-fdata-sections>
)

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
target_compile_definitions(OpenChaos PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:DEBUG>

    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Release>:_RELEASE>
    $<$<CONFIG:Release>:FINAL>
)
# MSVC CRT warnings — Windows only
if(WIN32)
    target_compile_definitions(OpenChaos PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ---------------------------------------------------------------------------
# Linker flags
# ---------------------------------------------------------------------------

if(WIN32)
    # /ENTRY:mainCRTStartup — use main() instead of WinMain while keeping
    # /SUBSYSTEM:WINDOWS (no console window). Cross-platform main() entry point.
    target_link_options(OpenChaos PRIVATE "LINKER:/ENTRY:mainCRTStartup")
    # Debug: generate PDB for crash symbolization (llvm-symbolizer --relative-address).
    target_link_options(OpenChaos PRIVATE $<$<CONFIG:Debug>:LINKER:/DEBUG>)
endif()

# ASan linker flags (must be outside the WIN32 block for cross-platform support)
if(ENABLE_ASAN)
    target_link_options(OpenChaos PRIVATE -fsanitize=address)
endif()

# Dead-code report linker flags. On Windows (lld-link) use /OPT:REF + /VERBOSE.
# On Unix (ld.lld / ld) use --gc-sections + --print-gc-sections.
# /VERBOSE on lld-link prints "Discarded ..." lines for sections dropped by /OPT:REF.
if(DEAD_CODE_REPORT)
    if(WIN32)
        target_link_options(OpenChaos PRIVATE
            "LINKER:/OPT:REF"
            "LINKER:/VERBOSE"
        )
    else()
        target_link_options(OpenChaos PRIVATE
            "LINKER:--gc-sections"
            "LINKER:--print-gc-sections"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(OpenChaos PRIVATE
    # vcpkg packages
    SDL3::SDL3
    OpenAL::OpenAL
    ${FFMPEG_LIBRARIES}
    nlohmann_json::nlohmann_json

    # Graphics backend libraries
    ${BACKEND_LIBS}

    # Project-owned standalone libraries (see ../libDualsense/)
    libDualsense
)


# ---------------------------------------------------------------------------
# Post-build: copy game data to output directory
# ---------------------------------------------------------------------------

# Windows: copy vcpkg DLLs to output directory.
# (VCPKG_APPLOCAL_DEPS is OFF because its applocal.ps1 requires powershell.exe on PATH.)
# On macOS/Linux shared libs are found via rpath — no copying needed.
# Skipped entirely for the static triplet (x64-windows-static): everything is
# linked into the exe, so there are no DLLs to copy.
if(WIN32 AND NOT VCPKG_TARGET_TRIPLET MATCHES "static")
    set(VCPKG_TRIPLET_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

    # Release DLLs
    foreach(_dll SDL3.dll OpenAL32.dll fmt.dll avcodec-62.dll avformat-62.dll avutil-60.dll swresample-6.dll swscale-9.dll)
        add_custom_command(TARGET OpenChaos POST_BUILD
            COMMAND "$<$<CONFIG:Release>:${CMAKE_COMMAND}>" $<$<CONFIG:Release>:-E>
                $<$<CONFIG:Release>:copy_if_different>
                "$<$<CONFIG:Release>:${VCPKG_TRIPLET_DIR}/bin/${_dll}>"
                "$<$<CONFIG:Release>:$<TARGET_FILE_DIR:OpenChaos>/${_dll}>"
            COMMAND_EXPAND_LISTS
        )
    endforeach()

    # Debug DLLs (some have 'd' suffix)
    foreach(_dll SDL3.dll OpenAL32.dll fmtd.dll avcodec-62.dll avformat-62.dll avutil-60.dll swresample-6.dll swscale-9.dll)
        add_custom_command(TARGET OpenChaos POST_BUILD
            COMMAND "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>" $<$<CONFIG:Debug>:-E>
                $<$<CONFIG:Debug>:copy_if_different>
                "$<$<CONFIG:Debug>:${VCPKG_TRIPLET_DIR}/debug/bin/${_dll}>"
                "$<$<CONFIG:Debug>:$<TARGET_FILE_DIR:OpenChaos>/${_dll}>"
            COMMAND_EXPAND_LISTS
        )
    endforeach()
endif()

# text/ and config.ini are part of the original game resources.
# They are copied via: make copy-resources (uses original_game_resources/)
