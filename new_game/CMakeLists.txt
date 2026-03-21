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
set(SOURCES_MFSTDLIB
    src/old/MFStdLib/Source/StdLib/StdFile.cpp
)

set(SOURCES_NEW
    src/new/core/memory.cpp
    src/new/core/memory_globals.cpp
    src/new/core/math.cpp
    src/new/core/math_globals.cpp
    src/new/engine/io/file.cpp
    src/new/engine/io/file_globals.cpp
    src/new/engine/io/async_file.cpp
    src/new/engine/io/async_file_globals.cpp
    src/new/engine/input/keyboard.cpp
    src/new/engine/input/keyboard_globals.cpp
    src/new/engine/input/mouse.cpp
    src/new/engine/input/mouse_globals.cpp
    src/new/engine/input/joystick.cpp
    src/new/engine/input/joystick_globals.cpp
    src/new/core/matrix.cpp
    src/new/core/fmatrix.cpp
    src/new/core/quaternion.cpp
    src/new/core/quaternion_globals.cpp
    src/new/assets/file_clump.cpp
    src/new/assets/tga.cpp
    src/new/assets/tga_globals.cpp
    src/new/assets/image_compression.cpp
    src/new/assets/image_compression_globals.cpp
    src/new/assets/compression.cpp
    src/new/assets/compression_globals.cpp
    src/new/assets/anim.cpp
    src/new/assets/anim_globals.cpp
    src/new/engine/lighting/ngamut.cpp
    src/new/engine/lighting/ngamut_globals.cpp
    src/new/engine/graphics/graphics_api/work_screen.cpp
    src/new/engine/graphics/graphics_api/work_screen_globals.cpp
    src/new/engine/graphics/geometry/aa.cpp
    src/new/engine/graphics/geometry/aa_globals.cpp
    src/new/engine/graphics/pipeline/message.cpp
    src/new/engine/graphics/pipeline/message_globals.cpp
    src/new/core/timer.cpp
    src/new/core/timer_globals.cpp
    src/new/assets/bink_client.cpp
    src/new/engine/io/drive.cpp
    src/new/engine/io/drive_globals.cpp
    src/new/engine/audio/mfx.cpp
    src/new/engine/audio/mfx_globals.cpp
    src/new/engine/graphics/resources/d3d_texture.cpp
    src/new/engine/graphics/resources/d3d_texture_globals.cpp
    src/new/engine/graphics/graphics_api/wind_procs.cpp
    src/new/engine/graphics/graphics_api/wind_procs_globals.cpp
    src/new/engine/graphics/graphics_api/host.cpp
    src/new/engine/graphics/graphics_api/host_globals.cpp
    src/new/engine/graphics/pipeline/render_state.cpp
    src/new/engine/graphics/pipeline/bucket.cpp
    src/new/engine/graphics/pipeline/bucket_globals.cpp
    src/new/engine/graphics/pipeline/vertex_buffer.cpp
    src/new/engine/graphics/pipeline/vertex_buffer_globals.cpp
    src/new/engine/graphics/pipeline/polypage.cpp
    src/new/engine/graphics/pipeline/polypage_globals.cpp
    src/new/engine/graphics/resources/font.cpp
    src/new/engine/graphics/resources/font_globals.cpp
    src/new/engine/graphics/geometry/sprite.cpp
    src/new/engine/graphics/resources/menufont.cpp
    src/new/engine/graphics/resources/menufont_globals.cpp
    src/new/engine/graphics/pipeline/qeng.cpp
    src/new/engine/graphics/pipeline/qeng_globals.cpp
    src/new/world/map/qmap_globals.cpp
    src/new/engine/graphics/pipeline/text.cpp
    src/new/engine/graphics/pipeline/text_globals.cpp
    src/new/assets/xlat_str.cpp
    src/new/assets/xlat_str_globals.cpp
    src/new/assets/sound_id_globals.cpp
    src/new/engine/graphics/resources/font2d.cpp
    src/new/engine/graphics/resources/font2d_globals.cpp
    src/new/engine/graphics/resources/console.cpp
    src/new/engine/graphics/resources/console_globals.cpp
    src/new/core/heap.cpp
    src/new/core/heap_globals.cpp
    src/new/assets/anim_tmap.cpp
    src/new/assets/anim_tmap_globals.cpp
    src/new/engine/animation/morph.cpp
    src/new/engine/animation/morph_globals.cpp
    src/new/engine/net/net.cpp
    src/new/engine/net/net_globals.cpp
    src/new/world/environment/outline.cpp
    src/new/engine/lighting/crinkle.cpp
    src/new/engine/effects/flamengine.cpp
    src/new/engine/effects/psystem.cpp
    src/new/engine/effects/psystem_globals.cpp
    src/new/engine/io/env.cpp
    src/new/engine/io/env_globals.cpp
    src/new/engine/graphics/pipeline/wibble.cpp
    src/new/engine/graphics/pipeline/wibble_globals.cpp
    src/new/engine/graphics/resources/truetype.cpp
    src/new/engine/graphics/resources/truetype_globals.cpp
    src/new/effects/fire.cpp
    src/new/effects/fire_globals.cpp
    src/new/effects/mist.cpp
    src/new/effects/mist_globals.cpp
    src/new/effects/glitter.cpp
    src/new/effects/glitter_globals.cpp
    src/new/effects/spark.cpp
    src/new/effects/spark_globals.cpp
    src/new/effects/ribbon.cpp
    src/new/effects/ribbon_globals.cpp
    src/new/effects/drip.cpp
    src/new/effects/drip_globals.cpp
    src/new/engine/graphics/geometry/oval.cpp
    src/new/engine/graphics/geometry/oval_globals.cpp
    src/new/world/environment/build.cpp
    src/new/world/environment/build_globals.cpp
    src/new/ui/hud/planmap.cpp
    src/new/ui/hud/planmap_globals.cpp
    src/new/ui/hud/eng_map.cpp
    src/new/ui/hud/eng_map_globals.cpp
    src/new/engine/graphics/graphics_api/dd_manager.cpp
    src/new/engine/graphics/graphics_api/dd_manager_globals.cpp
    src/new/engine/lighting/shadow.cpp
    src/new/engine/lighting/shadow_globals.cpp
    src/new/effects/fog.cpp
    src/new/effects/fog_globals.cpp
    src/new/actors/core/drawtype.cpp
    src/new/world/map/pap.cpp
    src/new/world/map/pap_globals.cpp
    src/new/world/navigation/walkable.cpp
    src/new/world/navigation/walkable_globals.cpp
    src/new/engine/audio/music.cpp
    src/new/engine/audio/music_globals.cpp
    src/new/engine/audio/soundenv.cpp
    src/new/engine/audio/soundenv_globals.cpp
    src/new/effects/tracks.cpp
    src/new/effects/tracks_globals.cpp
    src/new/world/map/map.cpp
    src/new/world/map/map_globals.cpp
    src/new/engine/graphics/geometry/cone.cpp
    src/new/engine/graphics/geometry/cone_globals.cpp
    src/new/engine/graphics/geometry/farfacet.cpp
    src/new/engine/graphics/geometry/farfacet_globals.cpp
    src/new/engine/graphics/pipeline/sky.cpp
    src/new/engine/graphics/pipeline/sky_globals.cpp
    src/new/engine/graphics/geometry/fastprim.cpp
    src/new/engine/graphics/geometry/fastprim_globals.cpp
    src/new/engine/graphics/pipeline/poly_render.cpp
    src/new/engine/graphics/pipeline/poly_render_globals.cpp
    src/new/engine/lighting/smap.cpp
    src/new/engine/lighting/smap_globals.cpp
    src/new/world/navigation/inside2.cpp
    src/new/world/navigation/inside2_globals.cpp
    src/new/world/environment/build2.cpp
    src/new/actors/core/state.cpp
    src/new/actors/core/switch.cpp
    src/new/actors/core/switch_globals.cpp
    src/new/actors/core/hierarchy.cpp
    src/new/actors/core/hierarchy_globals.cpp
    src/new/actors/core/player.cpp
    src/new/actors/core/player_globals.cpp
    src/new/actors/core/thing.cpp
    src/new/actors/core/thing_globals.cpp
    src/new/actors/core/interact.cpp
    src/new/actors/core/interact_globals.cpp
    src/new/actors/characters/cop.cpp
    src/new/actors/characters/cop_globals.cpp
    src/new/actors/characters/roper.cpp
    src/new/actors/characters/roper_globals.cpp
    src/new/actors/characters/thug.cpp
    src/new/actors/characters/thug_globals.cpp
    src/new/actors/characters/snipe.cpp
    src/new/actors/characters/snipe_globals.cpp
    src/new/actors/characters/darci.cpp
    src/new/actors/characters/darci_globals.cpp
    src/new/actors/animals/animal.cpp
    src/new/actors/animals/animal_globals.cpp
    src/new/actors/vehicles/chopper.cpp
    src/new/actors/vehicles/chopper_globals.cpp
    src/new/assets/startscr.cpp
    src/new/assets/startscr_globals.cpp
    src/new/actors/items/balloon.cpp
    src/new/actors/items/balloon_globals.cpp
    src/new/actors/items/grenade.cpp
    src/new/actors/items/grenade_globals.cpp
    src/new/actors/items/dike.cpp
    src/new/actors/items/dike_globals.cpp
    src/new/actors/items/hook.cpp
    src/new/actors/items/hook_globals.cpp
    src/new/effects/pow.cpp
    src/new/effects/pow_globals.cpp
    src/new/engine/graphics/geometry/superfacet.cpp
    src/new/engine/graphics/geometry/superfacet_globals.cpp
)

set(SOURCES_DDENGINE
    src/old/fallen/DDEngine/Source/aeng.cpp
    src/old/fallen/DDEngine/Source/drawxtra.cpp
    src/old/fallen/DDEngine/Source/facet.cpp
    src/old/fallen/DDEngine/Source/figure.cpp
    src/old/fallen/DDEngine/Source/mesh.cpp
    src/old/fallen/DDEngine/Source/panel.cpp
    src/old/fallen/DDEngine/Source/poly.cpp
    src/new/engine/graphics/geometry/shape.cpp
    src/new/engine/graphics/geometry/shape_globals.cpp
    src/new/assets/texture.cpp
    src/new/assets/texture_globals.cpp
)

set(SOURCES_DDLIBRARY
    src/old/fallen/DDLibrary/Source/GDisplay.cpp
)

set(SOURCES_OUTRO
    src/old/fallen/outro/back.cpp
    src/old/fallen/outro/cam.cpp
    src/old/fallen/outro/credits.cpp
    src/old/fallen/outro/imp.cpp
    src/old/fallen/outro/mf.cpp
    src/old/fallen/outro/os.cpp
    src/old/fallen/outro/outroFont.cpp
    src/old/fallen/outro/outroMain.cpp
    src/old/fallen/outro/outroMatrix.cpp
    src/old/fallen/outro/OutroTga.cpp
    src/old/fallen/outro/wire.cpp
)

set(SOURCES_GAME
    src/old/fallen/Source/Anim.cpp
    src/new/ui/attract.cpp
    src/new/ui/attract_globals.cpp
    src/new/actors/items/barrel.cpp
    src/new/actors/items/barrel_globals.cpp
    src/old/fallen/Source/bat.cpp
    src/old/fallen/Source/building.cpp
    src/new/actors/animals/canid.cpp
    src/new/actors/animals/canid_globals.cpp
    src/new/ui/menus/cnet.cpp
    src/new/ui/menus/cnet_globals.cpp
    src/old/fallen/Source/collide.cpp
    src/old/fallen/Source/combat.cpp
    src/old/fallen/Source/Controls.cpp
    src/old/fallen/Source/dirt.cpp
    src/new/world/environment/door.cpp
    src/new/world/environment/door_globals.cpp
    src/old/fallen/Source/drawtype.cpp
    src/new/actors/core/effect.cpp
    src/old/fallen/Source/elev.cpp
    src/new/actors/characters/enemy.cpp

    src/old/fallen/Source/eway.cpp
    src/new/ui/camera/fc.cpp
    src/new/ui/camera/fc_globals.cpp
    src/old/fallen/Source/fog.cpp
    src/old/fallen/Source/frontend.cpp
    src/old/fallen/Source/Game.cpp
    src/new/ui/menus/gamemenu.cpp
    src/new/ui/menus/gamemenu_globals.cpp
    src/new/actors/items/guns.cpp
    src/old/fallen/Source/hm.cpp
    src/old/fallen/Source/interfac.cpp
    src/old/fallen/Source/io.cpp
    src/old/fallen/Source/Main.cpp
    src/new/ai/mav.cpp
    src/new/ai/mav_globals.cpp
    src/old/fallen/Source/mav.cpp
    src/old/fallen/Source/memory.cpp
    src/old/fallen/Source/night.cpp
    src/old/fallen/Source/ns.cpp
    src/new/world/map/ob.cpp
    src/new/world/map/ob_globals.cpp
    src/new/ui/hud/overlay.cpp
    src/new/ui/hud/overlay_globals.cpp

    src/new/ui/pause.cpp
    src/old/fallen/Source/pcom.cpp
    src/old/fallen/Source/Person.cpp
    src/new/actors/items/projectile.cpp
    src/new/world/environment/plat.cpp
    src/new/world/environment/plat_globals.cpp
    src/new/ui/cutscenes/playcuts.cpp
    src/new/ui/cutscenes/playcuts_globals.cpp
    src/old/fallen/Source/Prim.cpp
    src/new/world/environment/puddle.cpp
    src/new/world/environment/puddle_globals.cpp
    src/new/effects/pyro.cpp
    src/new/effects/pyro_globals.cpp
    src/new/world/map/qedit.cpp
    src/new/world/map/qedit_globals.cpp
    src/new/world/map/qmap.cpp
    src/new/world/map/road.cpp
    src/new/world/map/road_globals.cpp
    src/new/missions/save.cpp
    src/new/missions/save_globals.cpp
    src/old/fallen/Source/shadow.cpp
    src/new/engine/physics/sm.cpp
    src/new/engine/physics/sm_globals.cpp
    src/new/engine/audio/sound.cpp
    src/new/engine/audio/sound_globals.cpp
    src/new/actors/items/special.cpp
    src/new/actors/items/special_globals.cpp
    src/new/world/environment/stair.cpp
    src/new/world/environment/stair_globals.cpp
    src/new/world/map/supermap.cpp
    src/new/world/map/supermap_globals.cpp
    src/new/world/environment/tripwire.cpp
    src/new/world/environment/tripwire_globals.cpp
    src/old/fallen/Source/vehicle.cpp

    src/new/world/navigation/wand.cpp
    src/new/world/environment/ware.cpp
    src/new/world/environment/ware_globals.cpp
    src/new/ui/menus/widget.cpp
    src/new/ui/menus/widget_globals.cpp
    src/new/world/navigation/wmove.cpp
    src/new/world/navigation/wmove_globals.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(Fallen WIN32
    ${SOURCES_NEW}
    ${SOURCES_MFSTDLIB}
    ${SOURCES_DDENGINE}
    ${SOURCES_DDLIBRARY}
    ${SOURCES_OUTRO}
    ${SOURCES_GAME}
)

# ---------------------------------------------------------------------------
# Include directories
# ---------------------------------------------------------------------------
target_include_directories(Fallen PRIVATE
    # New structured code (Этап 4+): old/ can include new/ headers
    src/new
    # Old code (legacy, migrating to new/): needed so new/ can still include old/ headers
    # during migration (e.g. #include "fallen/Headers/Map.h"). Remove when migration complete.
    src/old
    # Internal old/ include paths
    src/old/fallen/Headers
    src/old/fallen/DDLibrary/Headers
    src/old/fallen/DDEngine/Headers
    src/old/MFStdLib/Headers
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
