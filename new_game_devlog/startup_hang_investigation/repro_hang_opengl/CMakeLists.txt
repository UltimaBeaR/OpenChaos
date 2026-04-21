cmake_minimum_required(VERSION 3.15)
project(repro_hang_opengl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-only (uses WGL + Win32 messaging).
if(NOT WIN32)
    message(FATAL_ERROR "This repro is Windows-only.")
endif()

# Windows subsystem — no console on launch. Key info is mirrored in window title.
# Redirect stderr to file (`app.exe 2> log.txt`) if full log is needed.
add_executable(repro_hang_opengl WIN32 main.cpp)
target_link_libraries(repro_hang_opengl PRIVATE opengl32 gdi32 user32)
if(MSVC)
    set_target_properties(repro_hang_opengl PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()
