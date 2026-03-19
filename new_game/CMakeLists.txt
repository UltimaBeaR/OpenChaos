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
    src/MFStdLib/Source/StdLib/StdFile.cpp
    src/MFStdLib/Source/StdLib/StdMaths.cpp
    src/MFStdLib/Source/StdLib/StdMem.cpp
)

set(SOURCES_DDENGINE
    src/fallen/DDEngine/Source/aa.cpp
    src/fallen/DDEngine/Source/aeng.cpp
    src/fallen/DDEngine/Source/BreakTimer.cpp
    src/fallen/DDEngine/Source/Bucket.cpp
    src/fallen/DDEngine/Source/build.cpp
    src/fallen/DDEngine/Source/comp.cpp
    src/fallen/DDEngine/Source/cone.cpp
    src/fallen/DDEngine/Source/console.cpp
    src/fallen/DDEngine/Source/Crinkle.cpp
    src/fallen/DDEngine/Source/drawxtra.cpp
    src/fallen/DDEngine/Source/engineMap.cpp
    src/fallen/DDEngine/Source/facet.cpp
    src/fallen/DDEngine/Source/farfacet.cpp
    src/fallen/DDEngine/Source/fastprim.cpp
    src/fallen/DDEngine/Source/figure.cpp
    src/fallen/DDEngine/Source/flamengine.cpp
    src/fallen/DDEngine/Source/Font.cpp
    src/fallen/DDEngine/Source/font2d.cpp
    src/fallen/DDEngine/Source/Gamut.cpp
    src/fallen/DDEngine/Source/ic.cpp
    src/fallen/DDEngine/Source/Matrix.cpp
    src/fallen/DDEngine/Source/menufont.cpp
    src/fallen/DDEngine/Source/mesh.cpp
    src/fallen/DDEngine/Source/Message.cpp
    src/fallen/DDEngine/Source/NGamut.cpp
    src/fallen/DDEngine/Source/oval.cpp
    src/fallen/DDEngine/Source/panel.cpp
    src/fallen/DDEngine/Source/planmap.cpp
    src/fallen/DDEngine/Source/poly.cpp
    src/fallen/DDEngine/Source/polypage.cpp
    src/fallen/DDEngine/Source/polyrenderstate.cpp
    src/fallen/DDEngine/Source/qeng.cpp
    src/fallen/DDEngine/Source/Quaternion.cpp
    src/fallen/DDEngine/Source/renderstate.cpp
    src/fallen/DDEngine/Source/shape.cpp
    src/fallen/DDEngine/Source/sky.cpp
    src/fallen/DDEngine/Source/smap.cpp
    src/fallen/DDEngine/Source/sprite.cpp
    src/fallen/DDEngine/Source/superfacet.cpp
    src/fallen/DDEngine/Source/Text.cpp
    src/fallen/DDEngine/Source/texture.cpp
    src/fallen/DDEngine/Source/truetype.cpp
    src/fallen/DDEngine/Source/vertexbuffer.cpp
    src/fallen/DDEngine/Source/wibble.cpp
)

set(SOURCES_DDLIBRARY
    src/fallen/DDLibrary/Source/AsyncFile2.cpp
    src/fallen/DDLibrary/Source/BinkClient.cpp
    src/fallen/DDLibrary/Source/D3DTexture.cpp
    src/fallen/DDLibrary/Source/DDManager.cpp
    src/fallen/DDLibrary/Source/DIManager.cpp
    src/fallen/DDLibrary/Source/Drive.cpp
    src/fallen/DDLibrary/Source/FileClump.cpp
    src/fallen/DDLibrary/Source/GDisplay.cpp
    src/fallen/DDLibrary/Source/GHost.cpp
    src/fallen/DDLibrary/Source/GKeyboard.cpp
    src/fallen/DDLibrary/Source/GMouse.cpp
    src/fallen/DDLibrary/Source/GWorkScreen.cpp
    src/fallen/DDLibrary/Source/MFX.cpp
    src/fallen/DDLibrary/Source/net.cpp
    src/fallen/DDLibrary/Source/Tga.cpp
    src/fallen/DDLibrary/Source/WindProcs.cpp
)

set(SOURCES_OUTRO
    src/fallen/outro/back.cpp
    src/fallen/outro/cam.cpp
    src/fallen/outro/credits.cpp
    src/fallen/outro/imp.cpp
    src/fallen/outro/mf.cpp
    src/fallen/outro/os.cpp
    src/fallen/outro/outroFont.cpp
    src/fallen/outro/outroMain.cpp
    src/fallen/outro/outroMatrix.cpp
    src/fallen/outro/OutroTga.cpp
    src/fallen/outro/wire.cpp
)

set(SOURCES_GAME
    src/fallen/Source/Anim.cpp
    src/fallen/Source/animal.cpp
    src/fallen/Source/animtmap.cpp
    src/fallen/Source/Attract.cpp
    src/fallen/Source/balloon.cpp
    src/fallen/Source/barrel.cpp
    src/fallen/Source/bat.cpp
    src/fallen/Source/build2.cpp
    src/fallen/Source/building.cpp
    src/fallen/Source/canid.cpp
    src/fallen/Source/chopper.cpp
    src/fallen/Source/cnet.cpp
    src/fallen/Source/collide.cpp
    src/fallen/Source/combat.cpp
    src/fallen/Source/Controls.cpp
    src/fallen/Source/Cop.cpp
    src/fallen/Source/Darci.cpp
    src/fallen/Source/dike.cpp
    src/fallen/Source/dirt.cpp
    src/fallen/Source/door.cpp
    src/fallen/Source/drawtype.cpp
    src/fallen/Source/drip.cpp
    src/fallen/Source/Effect.cpp
    src/fallen/Source/elev.cpp
    src/fallen/Source/Enemy.cpp
    src/fallen/Source/env2.cpp
    src/fallen/Source/eway.cpp
    src/fallen/Source/fc.cpp
    src/fallen/Source/fire.cpp
    src/fallen/Source/FMatrix.cpp
    src/fallen/Source/fog.cpp
    src/fallen/Source/frontend.cpp
    src/fallen/Source/Game.cpp
    src/fallen/Source/gamemenu.cpp
    src/fallen/Source/glitter.cpp
    src/fallen/Source/grenade.cpp
    src/fallen/Source/guns.cpp
    src/fallen/Source/heap.cpp
    src/fallen/Source/Hierarchy.cpp
    src/fallen/Source/hm.cpp
    src/fallen/Source/hook.cpp
    src/fallen/Source/inside2.cpp
    src/fallen/Source/interact.cpp
    src/fallen/Source/interfac.cpp
    src/fallen/Source/io.cpp
    src/fallen/Source/Main.cpp
    src/fallen/Source/Map.cpp
    src/fallen/Source/maths.cpp
    src/fallen/Source/mav.cpp
    src/fallen/Source/memory.cpp
    src/fallen/Source/mist.cpp
    src/fallen/Source/morph.cpp
    src/fallen/Source/music.cpp
    src/fallen/Source/night.cpp
    src/fallen/Source/ns.cpp
    src/fallen/Source/ob.cpp
    src/fallen/Source/outline.cpp
    src/fallen/Source/overlay.cpp
    src/fallen/Source/pap.cpp
    src/fallen/Source/pause.cpp
    src/fallen/Source/pcom.cpp
    src/fallen/Source/Person.cpp
    src/fallen/Source/Pjectile.cpp
    src/fallen/Source/plat.cpp
    src/fallen/Source/playcuts.cpp
    src/fallen/Source/Player.cpp
    src/fallen/Source/pow.cpp
    src/fallen/Source/Prim.cpp
    src/fallen/Source/psystem.cpp
    src/fallen/Source/puddle.cpp
    src/fallen/Source/pyro.cpp
    src/fallen/Source/qedit.cpp
    src/fallen/Source/qmap.cpp
    src/fallen/Source/ribbon.cpp
    src/fallen/Source/road.cpp
    src/fallen/Source/Roper.cpp
    src/fallen/Source/save.cpp
    src/fallen/Source/shadow.cpp
    src/fallen/Source/sm.cpp
    src/fallen/Source/snipe.cpp
    src/fallen/Source/Sound.cpp
    src/fallen/Source/soundenv.cpp
    src/fallen/Source/sound_id.cpp
    src/fallen/Source/spark.cpp
    src/fallen/Source/Special.cpp
    src/fallen/Source/stair.cpp
    src/fallen/Source/startscr.cpp
    src/fallen/Source/State.cpp
    src/fallen/Source/supermap.cpp
    src/fallen/Source/Switch.cpp
    src/fallen/Source/Thing.cpp
    src/fallen/Source/Thug.cpp
    src/fallen/Source/tracks.cpp
    src/fallen/Source/trip.cpp
    src/fallen/Source/vehicle.cpp
    src/fallen/Source/walkable.cpp
    src/fallen/Source/wand.cpp
    src/fallen/Source/ware.cpp
    src/fallen/Source/widget.cpp
    src/fallen/Source/wmove.cpp
    src/fallen/Source/xlat_str.cpp
)

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
add_executable(Fallen WIN32
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
    src/fallen/Headers
    src/fallen/DDLibrary/Headers
    src/fallen/DDEngine/Headers
    src/MFStdLib/Headers
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
