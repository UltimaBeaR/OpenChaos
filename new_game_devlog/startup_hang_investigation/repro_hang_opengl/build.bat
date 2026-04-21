@echo off
REM Build pure Win32 + OpenGL repro. No external dependencies.
REM Requires only Windows SDK (linked libs: opengl32, gdi32, user32).

clang++ -O2 main.cpp ^
    -D_CRT_SECURE_NO_WARNINGS ^
    -lopengl32 -lgdi32 -luser32 ^
    -Xlinker /subsystem:windows ^
    -Xlinker /entry:mainCRTStartup ^
    -o repro_hang_opengl.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build FAILED
    exit /b 1
)

echo Build OK: repro_hang_opengl.exe
