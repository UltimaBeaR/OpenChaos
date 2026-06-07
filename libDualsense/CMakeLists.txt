cmake_minimum_required(VERSION 3.20)
project(libDualsense CXX)

# Self-contained DualSense library used by OpenChaos.
# Depends only on SDL3 (for hidapi). PUBLIC include directory is the
# parent of this folder, so consumers write:
#     #include <libDualsense/ds_device.h>
# and the `libDualsense/` prefix disambiguates from any other lib.

add_library(libDualsense STATIC
    ds_calibration.cpp
    ds_crc.cpp
    ds_device.cpp
    ds_feature.cpp
    ds_input.cpp
    ds_output.cpp
    ds_test.cpp
    ds_trigger.cpp
)

target_include_directories(libDualsense PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(libDualsense PUBLIC SDL3::SDL3)

target_compile_features(libDualsense PUBLIC cxx_std_20)

# Propagate AddressSanitizer when the parent build enables it.
# Consumer targets must be linked with matching flags or lld errors out
# on sanitizer runtime version mismatches.
if(ENABLE_ASAN)
    target_compile_options(libDualsense PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    # Match the parent build: disable MSVC STL container annotations so this lib's
    # objects don't carry a /failifmismatch "annotate_string"/"annotate_vector"
    # record (value 1) that conflicts with the non-instrumented vcpkg static libs
    # (value 0). See the matching note in new_game/CMakeLists.txt.
    target_compile_options(libDualsense PRIVATE -D_DISABLE_STRING_ANNOTATION -D_DISABLE_VECTOR_ANNOTATION)
    target_link_options(libDualsense PRIVATE -fsanitize=address)
endif()
