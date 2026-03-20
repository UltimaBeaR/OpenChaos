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
    src/new/core/math.cpp
)

set(SOURCES_DDENGINE
    src/old/fallen/DDEngine/Source/aa.cpp
    src/old/fallen/DDEngine/Source/aeng.cpp
    src/old/fallen/DDEngine/Source/BreakTimer.cpp
    src/old/fallen/DDEngine/Source/Bucket.cpp
    src/old/fallen/DDEngine/Source/build.cpp
    src/old/fallen/DDEngine/Source/comp.cpp
    src/old/fallen/DDEngine/Source/cone.cpp
    src/old/fallen/DDEngine/Source/console.cpp
    src/old/fallen/DDEngine/Source/Crinkle.cpp
    src/old/fallen/DDEngine/Source/drawxtra.cpp
    src/old/fallen/DDEngine/Source/engineMap.cpp
    src/old/fallen/DDEngine/Source/facet.cpp
    src/old/fallen/DDEngine/Source/farfacet.cpp
    src/old/fallen/DDEngine/Source/fastprim.cpp
    src/old/fallen/DDEngine/Source/figure.cpp
    src/old/fallen/DDEngine/Source/flamengine.cpp
    src/old/fallen/DDEngine/Source/Font.cpp
    src/old/fallen/DDEngine/Source/font2d.cpp
    src/old/fallen/DDEngine/Source/Gamut.cpp
    src/old/fallen/DDEngine/Source/ic.cpp
    src/old/fallen/DDEngine/Source/Matrix.cpp
    src/old/fallen/DDEngine/Source/menufont.cpp
    src/old/fallen/DDEngine/Source/mesh.cpp
    src/old/fallen/DDEngine/Source/Message.cpp
    src/old/fallen/DDEngine/Source/NGamut.cpp
    src/old/fallen/DDEngine/Source/oval.cpp
    src/old/fallen/DDEngine/Source/panel.cpp
    src/old/fallen/DDEngine/Source/planmap.cpp
    src/old/fallen/DDEngine/Source/poly.cpp
    src/old/fallen/DDEngine/Source/polypage.cpp
    src/old/fallen/DDEngine/Source/polyrenderstate.cpp
    src/old/fallen/DDEngine/Source/qeng.cpp
    src/old/fallen/DDEngine/Source/Quaternion.cpp
    src/old/fallen/DDEngine/Source/renderstate.cpp
    src/old/fallen/DDEngine/Source/shape.cpp
    src/old/fallen/DDEngine/Source/sky.cpp
    src/old/fallen/DDEngine/Source/smap.cpp
    src/old/fallen/DDEngine/Source/sprite.cpp
    src/old/fallen/DDEngine/Source/superfacet.cpp
    src/old/fallen/DDEngine/Source/Text.cpp
    src/old/fallen/DDEngine/Source/texture.cpp
    src/old/fallen/DDEngine/Source/truetype.cpp
    src/old/fallen/DDEngine/Source/vertexbuffer.cpp
    src/old/fallen/DDEngine/Source/wibble.cpp
)

set(SOURCES_DDLIBRARY
    src/old/fallen/DDLibrary/Source/AsyncFile2.cpp
    src/old/fallen/DDLibrary/Source/BinkClient.cpp
    src/old/fallen/DDLibrary/Source/D3DTexture.cpp
    src/old/fallen/DDLibrary/Source/DDManager.cpp
    src/old/fallen/DDLibrary/Source/DIManager.cpp
    src/old/fallen/DDLibrary/Source/Drive.cpp
    src/old/fallen/DDLibrary/Source/FileClump.cpp
    src/old/fallen/DDLibrary/Source/GDisplay.cpp
    src/old/fallen/DDLibrary/Source/GHost.cpp
    src/old/fallen/DDLibrary/Source/GKeyboard.cpp
    src/old/fallen/DDLibrary/Source/GMouse.cpp
    src/old/fallen/DDLibrary/Source/GWorkScreen.cpp
    src/old/fallen/DDLibrary/Source/MFX.cpp
    src/old/fallen/DDLibrary/Source/net.cpp
    src/old/fallen/DDLibrary/Source/Tga.cpp
    src/old/fallen/DDLibrary/Source/WindProcs.cpp
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
    src/old/fallen/Source/animal.cpp
    src/old/fallen/Source/animtmap.cpp
    src/old/fallen/Source/Attract.cpp
    src/old/fallen/Source/balloon.cpp
    src/old/fallen/Source/barrel.cpp
    src/old/fallen/Source/bat.cpp
    src/old/fallen/Source/build2.cpp
    src/old/fallen/Source/building.cpp
    src/old/fallen/Source/canid.cpp
    src/old/fallen/Source/chopper.cpp
    src/old/fallen/Source/cnet.cpp
    src/old/fallen/Source/collide.cpp
    src/old/fallen/Source/combat.cpp
    src/old/fallen/Source/Controls.cpp
    src/old/fallen/Source/Cop.cpp
    src/old/fallen/Source/Darci.cpp
    src/old/fallen/Source/dike.cpp
    src/old/fallen/Source/dirt.cpp
    src/old/fallen/Source/door.cpp
    src/old/fallen/Source/drawtype.cpp
    src/old/fallen/Source/drip.cpp
    src/old/fallen/Source/Effect.cpp
    src/old/fallen/Source/elev.cpp
    src/old/fallen/Source/Enemy.cpp
    src/old/fallen/Source/env2.cpp
    src/old/fallen/Source/eway.cpp
    src/old/fallen/Source/fc.cpp
    src/old/fallen/Source/fire.cpp
    src/old/fallen/Source/FMatrix.cpp
    src/old/fallen/Source/fog.cpp
    src/old/fallen/Source/frontend.cpp
    src/old/fallen/Source/Game.cpp
    src/old/fallen/Source/gamemenu.cpp
    src/old/fallen/Source/glitter.cpp
    src/old/fallen/Source/grenade.cpp
    src/old/fallen/Source/guns.cpp
    src/old/fallen/Source/heap.cpp
    src/old/fallen/Source/Hierarchy.cpp
    src/old/fallen/Source/hm.cpp
    src/old/fallen/Source/hook.cpp
    src/old/fallen/Source/inside2.cpp
    src/old/fallen/Source/interact.cpp
    src/old/fallen/Source/interfac.cpp
    src/old/fallen/Source/io.cpp
    src/old/fallen/Source/Main.cpp
    src/old/fallen/Source/Map.cpp
    src/old/fallen/Source/maths.cpp
    src/old/fallen/Source/mav.cpp
    src/old/fallen/Source/memory.cpp
    src/old/fallen/Source/mist.cpp
    src/old/fallen/Source/morph.cpp
    src/old/fallen/Source/music.cpp
    src/old/fallen/Source/night.cpp
    src/old/fallen/Source/ns.cpp
    src/old/fallen/Source/ob.cpp
    src/old/fallen/Source/outline.cpp
    src/old/fallen/Source/overlay.cpp
    src/old/fallen/Source/pap.cpp
    src/old/fallen/Source/pause.cpp
    src/old/fallen/Source/pcom.cpp
    src/old/fallen/Source/Person.cpp
    src/old/fallen/Source/Pjectile.cpp
    src/old/fallen/Source/plat.cpp
    src/old/fallen/Source/playcuts.cpp
    src/old/fallen/Source/Player.cpp
    src/old/fallen/Source/pow.cpp
    src/old/fallen/Source/Prim.cpp
    src/old/fallen/Source/psystem.cpp
    src/old/fallen/Source/puddle.cpp
    src/old/fallen/Source/pyro.cpp
    src/old/fallen/Source/qedit.cpp
    src/old/fallen/Source/qmap.cpp
    src/old/fallen/Source/ribbon.cpp
    src/old/fallen/Source/road.cpp
    src/old/fallen/Source/Roper.cpp
    src/old/fallen/Source/save.cpp
    src/old/fallen/Source/shadow.cpp
    src/old/fallen/Source/sm.cpp
    src/old/fallen/Source/snipe.cpp
    src/old/fallen/Source/Sound.cpp
    src/old/fallen/Source/soundenv.cpp
    src/old/fallen/Source/sound_id.cpp
    src/old/fallen/Source/spark.cpp
    src/old/fallen/Source/Special.cpp
    src/old/fallen/Source/stair.cpp
    src/old/fallen/Source/startscr.cpp
    src/old/fallen/Source/State.cpp
    src/old/fallen/Source/supermap.cpp
    src/old/fallen/Source/Switch.cpp
    src/old/fallen/Source/Thing.cpp
    src/old/fallen/Source/Thug.cpp
    src/old/fallen/Source/tracks.cpp
    src/old/fallen/Source/trip.cpp
    src/old/fallen/Source/vehicle.cpp
    src/old/fallen/Source/walkable.cpp
    src/old/fallen/Source/wand.cpp
    src/old/fallen/Source/ware.cpp
    src/old/fallen/Source/widget.cpp
    src/old/fallen/Source/wmove.cpp
    src/old/fallen/Source/xlat_str.cpp
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
