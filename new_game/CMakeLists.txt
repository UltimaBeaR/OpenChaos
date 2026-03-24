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

project(Fallen LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Build output: new_game/build/Debug/ and new_game/build/Release/
# All generated artifacts stay inside build/ — a single gitignore entry covers everything.
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")

# ---------------------------------------------------------------------------
# Dependencies (via vcpkg)
# ---------------------------------------------------------------------------
find_package(SDL2   CONFIG REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Source files (mirrors Fallen.vcxproj ClCompile list)
# ---------------------------------------------------------------------------
set(SOURCES_NEW
    src/main.cpp
    src/engine/core/memory.cpp
    src/engine/core/memory_globals.cpp
    src/engine/core/math.cpp
    src/engine/core/math_globals.cpp
    src/engine/io/file.cpp
    src/engine/io/file_globals.cpp
    src/engine/io/async_file.cpp
    src/engine/io/async_file_globals.cpp
    src/engine/input/keyboard.cpp
    src/engine/input/keyboard_globals.cpp
    src/engine/input/mouse.cpp
    src/engine/input/mouse_globals.cpp
    src/engine/input/joystick.cpp
    src/engine/input/joystick_globals.cpp
    src/engine/core/matrix.cpp
    src/engine/core/fmatrix.cpp
    src/engine/core/quaternion.cpp
    src/engine/core/quaternion_globals.cpp
    src/assets/file_clump.cpp
    src/assets/tga.cpp
    src/assets/tga_globals.cpp
    src/assets/image_compression.cpp
    src/assets/compression.cpp
    src/assets/compression_globals.cpp
    src/assets/anim.cpp
    src/assets/anim_globals.cpp
    src/assets/anim_loader.cpp
    src/assets/anim_loader_globals.cpp
    src/engine/graphics/lighting/ngamut.cpp
    src/engine/graphics/lighting/ngamut_globals.cpp
    src/engine/graphics/graphics_api/work_screen.cpp
    src/engine/graphics/graphics_api/work_screen_globals.cpp
    src/engine/graphics/geometry/aa.cpp
    src/engine/graphics/geometry/aa_globals.cpp
    src/engine/console/message.cpp
    src/engine/console/message_globals.cpp
    src/engine/core/timer.cpp
    src/engine/core/timer_globals.cpp
    src/engine/audio/mfx.cpp
    src/engine/audio/mfx_globals.cpp
    src/engine/graphics/graphics_api/d3d_texture.cpp
    src/engine/graphics/graphics_api/d3d_texture_globals.cpp
    src/engine/platform/wind_procs.cpp
    src/engine/platform/wind_procs_globals.cpp
    src/engine/platform/host.cpp
    src/engine/platform/host_globals.cpp
    src/engine/graphics/graphics_api/render_state.cpp
    src/engine/graphics/pipeline/bucket.cpp
    src/engine/graphics/pipeline/bucket_globals.cpp
    src/engine/graphics/graphics_api/vertex_buffer.cpp
    src/engine/graphics/graphics_api/vertex_buffer_globals.cpp
    src/engine/graphics/pipeline/polypage.cpp
    src/engine/graphics/pipeline/polypage_globals.cpp
    src/engine/graphics/text/font.cpp
    src/engine/graphics/text/font_globals.cpp
    src/engine/graphics/geometry/sprite.cpp
    src/engine/graphics/text/menufont.cpp
    src/engine/graphics/text/menufont_globals.cpp
    src/engine/graphics/pipeline/qeng.cpp
    src/engine/graphics/pipeline/qeng_globals.cpp
    src/world/map/qmap_globals.cpp
    src/engine/graphics/text/text.cpp
    src/engine/graphics/text/text_globals.cpp
    src/assets/xlat_str.cpp
    src/assets/xlat_str_globals.cpp
    src/assets/sound_id_globals.cpp
    src/engine/graphics/text/font2d.cpp
    src/engine/graphics/text/font2d_globals.cpp
    src/engine/console/console.cpp
    src/engine/console/console_globals.cpp
    src/engine/core/heap.cpp
    src/engine/core/heap_globals.cpp
    src/assets/anim_tmap.cpp
    src/assets/anim_tmap_globals.cpp
    src/engine/animation/morph.cpp
    src/engine/animation/morph_globals.cpp
    src/world/environment/outline.cpp
    src/engine/graphics/lighting/crinkle.cpp
    src/engine/effects/flamengine.cpp
    src/engine/effects/psystem.cpp
    src/engine/effects/psystem_globals.cpp
    src/engine/io/env.cpp
    src/engine/io/env_globals.cpp
    src/engine/graphics/postprocess/wibble.cpp
    src/engine/graphics/postprocess/wibble_globals.cpp
    src/engine/graphics/text/truetype.cpp
    src/engine/graphics/text/truetype_globals.cpp
    src/effects/environment/fire.cpp
    src/effects/environment/fire_globals.cpp
    src/effects/environment/dirt.cpp
    src/effects/environment/dirt_globals.cpp
    src/effects/weather/mist.cpp
    src/effects/weather/mist_globals.cpp
    src/effects/combat/glitter.cpp
    src/effects/combat/glitter_globals.cpp
    src/effects/combat/spark.cpp
    src/effects/combat/spark_globals.cpp
    src/effects/environment/ribbon.cpp
    src/effects/environment/ribbon_globals.cpp
    src/effects/weather/drip.cpp
    src/effects/weather/drip_globals.cpp
    src/engine/graphics/geometry/oval.cpp
    src/engine/graphics/geometry/oval_globals.cpp
    src/world/environment/build.cpp
    src/world/environment/build_globals.cpp
    src/world/environment/building.cpp
    src/world/environment/building_globals.cpp
    src/ui/hud/planmap.cpp
    src/ui/hud/planmap_globals.cpp
    src/ui/hud/eng_map.cpp
    src/ui/hud/eng_map_globals.cpp
    src/engine/graphics/graphics_api/dd_manager.cpp
    src/engine/graphics/graphics_api/dd_manager_globals.cpp
    src/engine/graphics/graphics_api/display.cpp
    src/engine/graphics/graphics_api/display_globals.cpp
    src/engine/graphics/lighting/shadow.cpp
    src/engine/graphics/lighting/shadow_globals.cpp
    src/effects/weather/fog.cpp
    src/effects/weather/fog_globals.cpp
    src/things/core/drawtype.cpp
    src/world/map/pap.cpp
    src/world/map/pap_globals.cpp
    src/world/navigation/walkable.cpp
    src/world/navigation/walkable_globals.cpp
    src/engine/audio/music.cpp
    src/engine/audio/music_globals.cpp
    src/engine/audio/soundenv.cpp
    src/engine/audio/soundenv_globals.cpp
    src/effects/environment/tracks.cpp
    src/effects/environment/tracks_globals.cpp
    src/world/map/map.cpp
    src/world/map/map_globals.cpp
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
    src/engine/graphics/pipeline/poly_render.cpp
    src/engine/graphics/pipeline/poly_render_globals.cpp
    src/engine/graphics/lighting/smap.cpp
    src/engine/graphics/lighting/smap_globals.cpp
    src/engine/graphics/lighting/night.cpp
    src/engine/graphics/lighting/night_globals.cpp
    src/world/navigation/inside2.cpp
    src/world/navigation/inside2_globals.cpp
    src/world/environment/build2.cpp
    src/things/core/state.cpp
    src/things/core/switch.cpp
    src/things/core/switch_globals.cpp
    src/things/core/hierarchy.cpp
    src/things/core/hierarchy_globals.cpp
    src/things/core/player.cpp
    src/things/core/player_globals.cpp
    src/things/core/thing.cpp
    src/things/core/thing_globals.cpp
    src/things/core/interact.cpp
    src/things/core/interact_globals.cpp
    src/things/characters/cop.cpp
    src/things/characters/cop_globals.cpp
    src/things/characters/roper.cpp
    src/things/characters/roper_globals.cpp
    src/things/characters/thug.cpp
    src/things/characters/thug_globals.cpp
    src/things/characters/snipe.cpp
    src/things/characters/snipe_globals.cpp
    src/things/characters/darci.cpp
    src/things/characters/darci_globals.cpp
    src/things/characters/person.cpp
    src/things/characters/person_globals.cpp
    src/things/animals/animal.cpp
    src/things/animals/animal_globals.cpp
    src/things/animals/bat.cpp
    src/things/animals/bat_globals.cpp
    src/things/vehicles/chopper.cpp
    src/things/vehicles/chopper_globals.cpp
    src/things/vehicles/vehicle.cpp
    src/things/vehicles/vehicle_globals.cpp
    src/assets/startscr.cpp
    src/assets/startscr_globals.cpp
    src/things/items/balloon.cpp
    src/things/items/balloon_globals.cpp
    src/things/items/grenade.cpp
    src/things/items/grenade_globals.cpp
    src/things/items/hook.cpp
    src/things/items/hook_globals.cpp
    src/effects/combat/pow.cpp
    src/effects/combat/pow_globals.cpp
    src/engine/graphics/geometry/superfacet.cpp
    src/engine/graphics/geometry/superfacet_globals.cpp
    src/assets/level_loader.cpp
    src/assets/level_loader_globals.cpp
    src/engine/graphics/pipeline/aeng.cpp
    src/engine/graphics/pipeline/aeng_globals.cpp
    src/engine/graphics/geometry/figure.cpp
    src/engine/graphics/geometry/figure_globals.cpp
    src/engine/graphics/geometry/mesh.cpp
    src/engine/graphics/geometry/mesh_globals.cpp
    src/engine/graphics/pipeline/poly.cpp
    src/engine/graphics/pipeline/poly_globals.cpp
    src/engine/graphics/geometry/shape.cpp
    src/engine/graphics/geometry/shape_globals.cpp
    src/assets/texture.cpp
    src/assets/texture_globals.cpp
    src/outro/core/outro_tga.cpp
    src/outro/core/outro_matrix.cpp
    src/outro/outro_back.cpp
    src/outro/outro_back_globals.cpp
    src/outro/core/outro_cam.cpp
    src/outro/core/outro_cam_globals.cpp
    src/outro/outro_credits.cpp
    src/outro/outro_credits_globals.cpp
    src/outro/core/outro_font.cpp
    src/outro/core/outro_font_globals.cpp
    src/outro/outro_wire.cpp
    src/outro/outro_wire_globals.cpp
    src/outro/outro_main.cpp
    src/outro/core/outro_imp.cpp
    src/outro/core/outro_mf.cpp
    src/outro/core/outro_mf_globals.cpp
    src/outro/core/outro_os.cpp
    src/outro/core/outro_os_globals.cpp
    src/ui/attract.cpp
    src/ui/attract_globals.cpp
    src/things/items/barrel.cpp
    src/things/items/barrel_globals.cpp
    src/ai/combat.cpp
    src/ai/combat_globals.cpp
    src/world/environment/door.cpp
    src/world/environment/door_globals.cpp

    src/missions/eway.cpp
    src/missions/eway_globals.cpp
    src/ui/camera/fc.cpp
    src/ui/camera/fc_globals.cpp
    src/ui/frontend.cpp
    src/ui/frontend_globals.cpp
    src/game/game.cpp
    src/game/game_globals.cpp
    src/game/network_state_globals.cpp
    src/ui/menus/gamemenu.cpp
    src/ui/menus/gamemenu_globals.cpp
    src/things/items/guns.cpp
    src/game/startup.cpp
    src/ai/mav.cpp
    src/ai/mav_globals.cpp
    src/world/environment/ns.cpp
    src/world/environment/ns_globals.cpp
    src/world/environment/prim.cpp
    src/world/environment/prim_globals.cpp
    src/world/map/ob.cpp
    src/world/map/ob_globals.cpp
    src/ui/hud/overlay.cpp
    src/ui/hud/overlay_globals.cpp
    src/ui/hud/panel.cpp
    src/ui/hud/panel_globals.cpp

    src/ui/pause.cpp
    src/ai/pcom.cpp
    src/ai/pcom_globals.cpp
    src/things/items/projectile.cpp
    src/world/environment/plat.cpp
    src/world/environment/plat_globals.cpp
    src/missions/playcuts.cpp
    src/missions/playcuts_globals.cpp
    src/world/environment/puddle.cpp
    src/world/environment/puddle_globals.cpp
    src/effects/combat/pyro.cpp
    src/effects/combat/pyro_globals.cpp
    src/effects/combat/glow.cpp
    src/engine/graphics/postprocess/bloom.cpp
    src/engine/graphics/postprocess/bloom_globals.cpp
    src/engine/graphics/pipeline/draw2d.cpp
    src/world/map/qmap.cpp
    src/world/map/road.cpp
    src/world/map/road_globals.cpp
    src/missions/memory.cpp
    src/missions/memory_globals.cpp
    src/missions/save.cpp
    src/missions/save_globals.cpp
    src/missions/elev.cpp
    src/missions/elev_globals.cpp
    src/engine/physics/sm.cpp
    src/engine/physics/sm_globals.cpp
    src/engine/physics/hm.cpp
    src/engine/physics/hm_globals.cpp
    src/engine/physics/collide.cpp
    src/engine/physics/collide_globals.cpp
    src/engine/audio/sound.cpp
    src/engine/audio/sound_globals.cpp
    src/things/items/special.cpp
    src/things/items/special_globals.cpp
    src/world/environment/stair.cpp
    src/world/environment/stair_globals.cpp
    src/world/map/supermap.cpp
    src/world/map/supermap_globals.cpp
    src/world/environment/tripwire.cpp
    src/world/environment/tripwire_globals.cpp

    src/world/navigation/wand.cpp
    src/world/environment/ware.cpp
    src/world/environment/ware_globals.cpp
    src/game/game_tick.cpp
    src/game/game_tick_globals.cpp
    src/game/input_actions.cpp
    src/game/input_actions_globals.cpp
    src/ui/menus/widget.cpp
    src/ui/menus/widget_globals.cpp
    src/world/navigation/wmove.cpp
    src/world/navigation/wmove_globals.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(Fallen WIN32
    ${SOURCES_NEW}
)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(Fallen PRIVATE
    # Main source root: resolves #include "core/types.h", "engine/...", etc.
    src
    # Platform abstraction: resolves #include <MFStdLib.h> and StdFile/StdKeybd/StdMaths/StdMouse
    src/engine/platform
)

# ---------------------------------------------------------------------------
# Compiler flags
# ---------------------------------------------------------------------------
target_compile_options(Fallen PRIVATE
    # 1-byte struct alignment — required for binary resource format compatibility
    # (.gob, .ilf, .spd, .ucm etc. are read by casting raw bytes to structs).
    /Zp1

    # Release: optimizations matching original vcxproj ClangCL settings.
    # -fno-inline-functions: without this, Clang -O2 breaks body part rendering
    #   (FIGURE_draw_prim_tween_person_only_just_set_matrix inlining issue, TODO Этап 7+).
    # -fno-strict-aliasing: code uses pointer type punning (*(float*)&intVal),
    #   strict aliasing optimizations would produce incorrect results.
    $<$<CONFIG:Release>:/clang:-O2>
    $<$<CONFIG:Release>:/clang:-fno-inline-functions>
    $<$<CONFIG:Release>:/clang:-fno-strict-aliasing>
)

# ---------------------------------------------------------------------------
# Preprocessor definitions
# ---------------------------------------------------------------------------
target_compile_definitions(Fallen PRIVATE
    WIN32
    _WINDOWS
    VERSION_D3D
    WINDOWS_IGNORE_PACKING_MISMATCH   # suppresses Windows SDK static_assert for /Zp1

    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:DEBUG>

    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:Release>:_RELEASE>
    $<$<CONFIG:Release>:FINAL>
    $<$<CONFIG:Release>:_CRT_SECURE_NO_WARNINGS>
)

# ---------------------------------------------------------------------------
# Linker flags
# ---------------------------------------------------------------------------

# /SAFESEH:NO — original code uses exception handlers not registered in SafeSEH table.
target_link_options(Fallen PRIVATE /SAFESEH:NO)

# ---------------------------------------------------------------------------
# Link libraries
# ---------------------------------------------------------------------------
target_link_libraries(Fallen PRIVATE
    # vcpkg packages
    SDL2::SDL2
    OpenAL::OpenAL

    # Windows system
    odbc32
    odbccp32
    comctl32
    imm32
    Version
    winmm

    # DirectX
    ddraw
    dinput8
    dxguid

    # DirectShow
    amstrmid
    quartz
    strmbase
)

# ---------------------------------------------------------------------------
# Post-build: copy game data to output directory
# ---------------------------------------------------------------------------

# Copy vcpkg DLLs to output directory.
# (VCPKG_APPLOCAL_DEPS is OFF because its applocal.ps1 requires powershell.exe on PATH.)
# vcpkg_installed/ lives at new_game/vcpkg_installed/ (next to vcpkg.json, outside build/).
# Debug and Release DLL names differ: SDL2d.dll / fmtd.dll for Debug, SDL2.dll / fmt.dll for Release.
# OpenAL32.dll has the same name in both configs.
set(VCPKG_BIN       "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows/bin")
set(VCPKG_DEBUG_BIN "${CMAKE_SOURCE_DIR}/vcpkg_installed/x86-windows/debug/bin")

foreach(_dll SDL2.dll OpenAL32.dll fmt.dll)
    add_custom_command(TARGET Fallen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_BIN}/${_dll}"
            "$<TARGET_FILE_DIR:Fallen>/${_dll}"
        COMMENT "Copying ${_dll}"
        CONFIGURATIONS Release
    )
endforeach()

foreach(_dll SDL2d.dll OpenAL32.dll fmtd.dll)
    add_custom_command(TARGET Fallen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_DEBUG_BIN}/${_dll}"
            "$<TARGET_FILE_DIR:Fallen>/${_dll}"
        COMMENT "Copying ${_dll}"
        CONFIGURATIONS Debug
    )
endforeach()

# text/ and config.ini are part of the original game resources.
# They are copied via: make copy-resources (uses original_game_resources/)
