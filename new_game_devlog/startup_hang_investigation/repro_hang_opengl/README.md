# Minimal OpenGL hang repro (stars-analogue)

Standalone **pure Win32 + WGL** приложение, воспроизводящее startup_hang bug из OpenChaos. **Без внешних зависимостей** — только Windows SDK (`opengl32`, `gdi32`, `user32`). Никаких DLL копировать не нужно.

Зеркалит per-frame pattern подтверждённого триггера — stars render path в игре
([aeng.cpp:4050](../../../new_game/src/engine/graphics/pipeline/aeng.cpp#L4050) →
[ge_lock_screen / ge_unlock_screen](../../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L1560)).

## Что делает (per frame)

1. `glClear` backbuffer в тёмно-серый.
2. Draw цветной треугольник → backbuffer не пустой (driver не optimized out readpixels как no-op).
3. Повторить `READBACK_CYCLES_PER_FRAME` раз:
   - `glReadPixels(0,0,640,480,GL_RGBA,...)` — sync GPU→CPU, 1.2 MB.
   - Y-flip in place + CPU plot 500 случайных пикселей (mimics `SKY_draw_stars`).
   - `glGenTextures` + `glBindTexture` + `glTexParameteri×2` (MIN/MAG=NEAREST) + `glTexImage2D(GL_RGBA8,...)`.
   - Draw fullscreen textured quad (writeback step).
   - `glDeleteTextures`.
4. `SwapBuffers`.

## Hardware-specificity

**Hang воспроизводится не на каждой машине.** На авторском железе (Windows 11 + NVIDIA) hang воспроизводится при `READBACK_CYCLES_PER_FRAME=8`. Другие пользователи с тем же бинарем — hang может не воспроизвестись вообще. Конкретная связка GPU model + driver version + Windows version определяет поведение.

**Не OpenGL-specific**: тот же readback pattern через `IDirectDrawSurface7::Lock` в PieroZ D3D6 сборке OpenChaos — тоже воспроизводит hang. Значит root cause — на уровне WDDM scheduler / DWM compositor при любом per-frame GPU sync read (GL / D3D / DDraw).

## Bisect флаги

В начале [main.cpp](main.cpp) — compile-time флаги для отключения элементов цикла:

- `DO_TRIANGLE_DRAW` — рисование геометрии перед readpixels.
- `DO_READPIXELS` — сам `glReadPixels`.
- `DO_CPU_WORK` — Y-flip + 500 scattered writes на CPU.
- `DO_TEXIMAGE` — `glTexImage2D` upload.
- `DO_WRITEBACK_QUAD` — fullscreen textured quad draw.
- `READBACK_CYCLES_PER_FRAME` — число lock/unlock циклов в одном кадре.

## Контекст

- OpenGL 4.1 core profile (matches game).
- MSAA 4× (matches game default).
- 640×480 window (matches game).
- SwapInterval=2 (matches game's sdl3_bridge.cpp — blocking VSync **обязателен** для триггера).

## Билд

```
build.bat
```

Линкует `opengl32 + gdi32 + user32`. Требует только clang++ в PATH (или отредактируй на `cl` для MSVC).

Или через CMake:

```
cmake -B build -A x64
cmake --build build --config Release
```

## Запуск

```
repro_hang_opengl.exe
```

Или с логом:

```
repro_hang_opengl.exe 2> log.txt
```

## Критерий "воспроизвелось"

Строка в stderr с `max_frame_ms > 80` и меткой `<-- HANG`. Нормальный кадр 0-2 мс. Hang кадр 100-200+ мс.

## Механизм

См. [../FINDINGS.md](../FINDINGS.md) (факты) и [../hypothesis.md](../hypothesis.md) (предположения о причинах).

Коротко: `glReadPixels` per frame — sync GPU stall + scattered CPU writes + glTexImage2D upload + writeback quad, при VSync-blocked `SwapBuffers` → NVIDIA driver detect'ит sync-read pattern и отключает threaded optimization, WDDM GPU scheduler throttле'ит процесс, DWM present блокируется на 100+ мс per frame.
