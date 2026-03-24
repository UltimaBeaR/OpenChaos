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
    src/core/memory.cpp
    src/core/memory_globals.cpp
    src/core/math.cpp
    src/core/math_globals.cpp
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
    src/core/matrix.cpp
    src/core/fmatrix.cpp
    src/core/quaternion.cpp
    src/core/quaternion_globals.cpp
    src/assets/file_clump.cpp
    src/assets/tga.cpp
    src/assets/tga_globals.cpp
    src/assets/image_compression.cpp
    src/assets/image_compression_globals.cpp
    src/assets/compression.cpp
    src/assets/compression_globals.cpp
    src/assets/anim.cpp
    src/assets/anim_globals.cpp
    src/assets/anim_loader.cpp
    src/assets/anim_loader_globals.cpp
    src/engine/lighting/ngamut.cpp
    src/engine/lighting/ngamut_globals.cpp
    src/engine/graphics/graphics_api/work_screen.cpp
    src/engine/graphics/graphics_api/work_screen_globals.cpp
    src/engine/graphics/geometry/aa.cpp
    src/engine/graphics/geometry/aa_globals.cpp
    src/engine/graphics/pipeline/message.cpp
    src/engine/graphics/pipeline/message_globals.cpp
    src/core/timer.cpp
    src/core/timer_globals.cpp
    src/engine/audio/mfx.cpp
    src/engine/audio/mfx_globals.cpp
    src/engine/graphics/resources/d3d_texture.cpp
    src/engine/graphics/resources/d3d_texture_globals.cpp
    src/engine/graphics/graphics_api/wind_procs.cpp
    src/engine/graphics/graphics_api/wind_procs_globals.cpp
    src/engine/graphics/graphics_api/host.cpp
    src/engine/graphics/graphics_api/host_globals.cpp
    src/engine/graphics/pipeline/render_state.cpp
    src/engine/graphics/pipeline/bucket.cpp
    src/engine/graphics/pipeline/bucket_globals.cpp
    src/engine/graphics/pipeline/vertex_buffer.cpp
    src/engine/graphics/pipeline/vertex_buffer_globals.cpp
    src/engine/graphics/pipeline/polypage.cpp
    src/engine/graphics/pipeline/polypage_globals.cpp
    src/engine/graphics/resources/font.cpp
    src/engine/graphics/resources/font_globals.cpp
    src/engine/graphics/geometry/sprite.cpp
    src/engine/graphics/resources/menufont.cpp
    src/engine/graphics/resources/menufont_globals.cpp
    src/engine/graphics/pipeline/qeng.cpp
    src/engine/graphics/pipeline/qeng_globals.cpp
    src/world/map/qmap_globals.cpp
    src/engine/graphics/pipeline/text.cpp
    src/engine/graphics/pipeline/text_globals.cpp
    src/assets/xlat_str.cpp
    src/assets/xlat_str_globals.cpp
    src/assets/sound_id_globals.cpp
    src/engine/graphics/resources/font2d.cpp
    src/engine/graphics/resources/font2d_globals.cpp
    src/engine/graphics/resources/console.cpp
    src/engine/graphics/resources/console_globals.cpp
    src/core/heap.cpp
    src/core/heap_globals.cpp
    src/assets/anim_tmap.cpp
    src/assets/anim_tmap_globals.cpp
    src/engine/animation/morph.cpp
    src/engine/animation/morph_globals.cpp
    src/world/environment/outline.cpp
    src/engine/lighting/crinkle.cpp
    src/engine/effects/flamengine.cpp
    src/engine/effects/psystem.cpp
    src/engine/effects/psystem_globals.cpp
    src/engine/io/env.cpp
    src/engine/io/env_globals.cpp
    src/engine/graphics/pipeline/wibble.cpp
    src/engine/graphics/pipeline/wibble_globals.cpp
    src/engine/graphics/resources/truetype.cpp
    src/engine/graphics/resources/truetype_globals.cpp
    src/effects/fire.cpp
    src/effects/fire_globals.cpp
    src/effects/dirt.cpp
    src/effects/dirt_globals.cpp
    src/effects/mist.cpp
    src/effects/mist_globals.cpp
    src/effects/glitter.cpp
    src/effects/glitter_globals.cpp
    src/effects/spark.cpp
    src/effects/spark_globals.cpp
    src/effects/ribbon.cpp
    src/effects/ribbon_globals.cpp
    src/effects/drip.cpp
    src/effects/drip_globals.cpp
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
    src/engine/lighting/shadow.cpp
    src/engine/lighting/shadow_globals.cpp
    src/effects/fog.cpp
    src/effects/fog_globals.cpp
    src/actors/core/drawtype.cpp
    src/world/map/pap.cpp
    src/world/map/pap_globals.cpp
    src/world/navigation/walkable.cpp
    src/world/navigation/walkable_globals.cpp
    src/engine/audio/music.cpp
    src/engine/audio/music_globals.cpp
    src/engine/audio/soundenv.cpp
    src/engine/audio/soundenv_globals.cpp
    src/effects/tracks.cpp
    src/effects/tracks_globals.cpp
    src/world/map/map.cpp
    src/world/map/map_globals.cpp
    src/engine/graphics/geometry/cone.cpp
    src/engine/graphics/geometry/cone_globals.cpp
    src/engine/graphics/geometry/farfacet.cpp
    src/engine/graphics/geometry/farfacet_globals.cpp
    src/engine/graphics/geometry/facet.cpp
    src/engine/graphics/geometry/facet_globals.cpp
    src/engine/graphics/pipeline/sky.cpp
    src/engine/graphics/pipeline/sky_globals.cpp
    src/engine/graphics/geometry/fastprim.cpp
    src/engine/graphics/geometry/fastprim_globals.cpp
    src/engine/graphics/pipeline/poly_render.cpp
    src/engine/graphics/pipeline/poly_render_globals.cpp
    src/engine/lighting/smap.cpp
    src/engine/lighting/smap_globals.cpp
    src/engine/lighting/night.cpp
    src/engine/lighting/night_globals.cpp
    src/world/navigation/inside2.cpp
    src/world/navigation/inside2_globals.cpp
    src/world/environment/build2.cpp
    src/actors/core/state.cpp
    src/actors/core/switch.cpp
    src/actors/core/switch_globals.cpp
    src/actors/core/hierarchy.cpp
    src/actors/core/hierarchy_globals.cpp
    src/actors/core/player.cpp
    src/actors/core/player_globals.cpp
    src/actors/core/thing.cpp
    src/actors/core/thing_globals.cpp
    src/actors/core/interact.cpp
    src/actors/core/interact_globals.cpp
    src/actors/characters/cop.cpp
    src/actors/characters/cop_globals.cpp
    src/actors/characters/roper.cpp
    src/actors/characters/roper_globals.cpp
    src/actors/characters/thug.cpp
    src/actors/characters/thug_globals.cpp
    src/actors/characters/snipe.cpp
    src/actors/characters/snipe_globals.cpp
    src/actors/characters/darci.cpp
    src/actors/characters/darci_globals.cpp
    src/actors/characters/person.cpp
    src/actors/characters/person_globals.cpp
    src/actors/animals/animal.cpp
    src/actors/animals/animal_globals.cpp
    src/actors/animals/bat.cpp
    src/actors/animals/bat_globals.cpp
    src/actors/vehicles/chopper.cpp
    src/actors/vehicles/chopper_globals.cpp
    src/actors/vehicles/vehicle.cpp
    src/actors/vehicles/vehicle_globals.cpp
    src/assets/startscr.cpp
    src/assets/startscr_globals.cpp
    src/actors/items/balloon.cpp
    src/actors/items/balloon_globals.cpp
    src/actors/items/grenade.cpp
    src/actors/items/grenade_globals.cpp
    src/actors/items/hook.cpp
    src/actors/items/hook_globals.cpp
    src/effects/pow.cpp
    src/effects/pow_globals.cpp
    src/engine/graphics/geometry/superfacet.cpp
    src/engine/graphics/geometry/superfacet_globals.cpp
    src/assets/level_loader.cpp
    src/assets/level_loader_globals.cpp
    src/engine/graphics/pipeline/aeng.cpp
    src/engine/graphics/pipeline/aeng_globals.cpp
    src/engine/animation/figure.cpp
    src/engine/animation/figure_globals.cpp
    src/engine/graphics/geometry/mesh.cpp
    src/engine/graphics/geometry/mesh_globals.cpp
    src/engine/graphics/pipeline/poly.cpp
    src/engine/graphics/pipeline/poly_globals.cpp
    src/engine/graphics/geometry/shape.cpp
    src/engine/graphics/geometry/shape_globals.cpp
    src/assets/texture.cpp
    src/assets/texture_globals.cpp
    src/ui/cutscenes/outro/outro_tga.cpp
    src/ui/cutscenes/outro/outro_matrix.cpp
    src/ui/cutscenes/outro/outro_back.cpp
    src/ui/cutscenes/outro/outro_back_globals.cpp
    src/ui/cutscenes/outro/outro_cam.cpp
    src/ui/cutscenes/outro/outro_cam_globals.cpp
    src/ui/cutscenes/outro/outro_credits.cpp
    src/ui/cutscenes/outro/outro_credits_globals.cpp
    src/ui/cutscenes/outro/outro_font.cpp
    src/ui/cutscenes/outro/outro_font_globals.cpp
    src/ui/cutscenes/outro/outro_wire.cpp
    src/ui/cutscenes/outro/outro_wire_globals.cpp
    src/ui/cutscenes/outro/outro_main.cpp
    src/ui/cutscenes/outro/outro_imp.cpp
    src/ui/cutscenes/outro/outro_mf.cpp
    src/ui/cutscenes/outro/outro_mf_globals.cpp
    src/ui/cutscenes/outro/outro_os.cpp
    src/ui/cutscenes/outro/outro_os_globals.cpp
    src/ui/attract.cpp
    src/ui/attract_globals.cpp
    src/actors/items/barrel.cpp
    src/actors/items/barrel_globals.cpp
    src/ui/menus/cnet.cpp
    src/ui/menus/cnet_globals.cpp
    src/ai/combat.cpp
    src/ai/combat_globals.cpp
    src/world/environment/door.cpp
    src/world/environment/door_globals.cpp
    src/actors/core/effect.cpp
    src/actors/characters/enemy.cpp

    src/missions/eway.cpp
    src/missions/eway_globals.cpp
    src/ui/camera/fc.cpp
    src/ui/camera/fc_globals.cpp
    src/ui/frontend.cpp
    src/ui/frontend_globals.cpp
    src/missions/game.cpp
    src/missions/game_globals.cpp
    src/ui/menus/gamemenu.cpp
    src/ui/menus/gamemenu_globals.cpp
    src/actors/items/guns.cpp
    src/missions/main.cpp
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
    src/actors/items/projectile.cpp
    src/world/environment/plat.cpp
    src/world/environment/plat_globals.cpp
    src/ui/cutscenes/playcuts.cpp
    src/ui/cutscenes/playcuts_globals.cpp
    src/world/environment/puddle.cpp
    src/world/environment/puddle_globals.cpp
    src/effects/pyro.cpp
    src/effects/pyro_globals.cpp
    src/effects/glow.cpp
    src/engine/graphics/geometry/bloom.cpp
    src/engine/graphics/geometry/bloom_globals.cpp
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
    src/actors/items/special.cpp
    src/actors/items/special_globals.cpp
    src/world/environment/stair.cpp
    src/world/environment/stair_globals.cpp
    src/world/map/supermap.cpp
    src/world/map/supermap_globals.cpp
    src/world/environment/tripwire.cpp
    src/world/environment/tripwire_globals.cpp

    src/world/navigation/wand.cpp
    src/world/environment/ware.cpp
    src/world/environment/ware_globals.cpp
    src/ui/controls.cpp
    src/ui/controls_globals.cpp
    src/ui/interfac.cpp
    src/ui/interfac_globals.cpp
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
    src/platform
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
