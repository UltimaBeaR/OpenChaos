# Этап 8 — Кросс-платформа

**Цель:** убрать Windows-специфичный код и зависимость от Visual Studio.
**Критерий завершения:** компилируется и работает на Linux и macOS без Visual Studio.
**Девлог:** [`stage8_log.md`](../new_game_devlog/stage8_log.md)

---

## 1. Оконная система + GL контекст ✅

- ~~Win32 `CreateWindowEx`, `WndProc`, message loop → SDL3 window + event loop~~
- ~~WGL контекст (`gl_context.cpp`) → `SDL_GL_CreateContext`~~
- ~~`WinMain` → обычный `main()`~~
- **Сделано:** окно, GL контекст, event loop, entry point — через SDL3 bridge
- **Остаток (cleanup):** `hDDLibWindow` в 3 местах (game_tick, widget, elev) — через native handle, убрать

## 2. Input ✅ (частично)

- Клавиатура: ✅ SDL3 events → маппинг на KB_* scancodes (через bridge)
- Мышь: ✅ SDL3 events → MouseProc (через bridge)
- Геймпад: ✅ уже на SDL3
- **Остаток (cleanup):** keyboard.cpp / mouse.cpp используют Win32 типы (WM_*, LPARAM) внутри — убрать прослойку, напрямую обновлять Keys[]/Mouse state из SDL callbacks

## 3. Звук ✅

- OpenAL уже кросс-платформенный, MSS32 → OpenAL Soft (сделано ранее)

---

## 4. Memory (маленький)

**Файлы:** `engine/core/memory.cpp`, `engine/core/memory_globals.h`

Win32 Heap API → стандартные функции:
- `HeapCreate()` → убрать (не нужен отдельный heap)
- `HeapAlloc(HEAP_ZERO_MEMORY)` → `calloc` / `malloc`
- `HeapReAlloc()` → `realloc`
- `HeapFree()` → `free`
- `HANDLE MFHeap` → убрать глобал

## 5. File I/O (средний)

**Файлы:** `engine/io/file.cpp`, `engine/io/file.h`

`MFFileHandle` (Win32 `HANDLE`) → стандартные `FILE*` или POSIX:
- `CreateFile()` → `fopen`
- `ReadFile()` → `fread`
- `WriteFile()` → `fwrite`
- `SetFilePointer()` → `fseek`
- `GetFileSize()` → `fseek` + `ftell`
- `CloseHandle()` → `fclose`
- `DeleteFile()` → `remove`
- `GetFileAttributes()` → `stat` или `fopen` проверка
- Пути: `\\` разделители → `/` (SDL path functions или простая замена)

## 6. Timer (маленький)

**Файлы:** `engine/core/timer.cpp`, `engine/platform/host.cpp`, `engine/console/console.cpp`

- `QueryPerformanceCounter` / `QueryPerformanceFrequency` → `SDL_GetPerformanceCounter` / `SDL_GetPerformanceFrequency`
- `GetTickCount()` → `SDL_GetTicks`
- `GetLocalTime()` / `SYSTEMTIME` → `time()` + `localtime()` или SDL

## 7. Threading (средний)

**Файлы:** `engine/io/async_file.cpp`, `engine/io/async_file.h`

- `CreateThread()` → `std::thread`
- `CRITICAL_SECTION` → `std::mutex`
- `CreateEvent()` / `SetEvent()` / `WaitForSingleObject()` → `std::condition_variable`
- `Sleep()` → `std::this_thread::sleep_for`
- `CloseHandle()` → деструкторы (RAII)

## 8. INI config (средний)

**Файлы:** `engine/io/env.cpp`, `engine/io/env_globals.h`

- `GetPrivateProfileInt()` / `GetPrivateProfileString()` → свой INI парсер или SDL preferences
- `WritePrivateProfileString()` → аналогично
- `GetCurrentDirectory()` → `SDL_GetBasePath()` или POSIX `getcwd`

## 9. Input cleanup

**Файлы:** `engine/input/keyboard.cpp`, `engine/input/mouse.cpp`, `engine/platform/host.cpp`

Убрать Win32 прослойку — сейчас SDL events конвертируются в WM_*/LPARAM формат и передаются в KeyboardProc/MouseProc. Вместо этого:
- Bridge callbacks напрямую обновляют `Keys[]`, `LastKey`, модификаторы
- Bridge callbacks напрямую обновляют `MouseX`/`MouseY`/`LeftButton` и т.д.
- Убрать `WM_KEYDOWN`, `WM_MOUSEMOVE`, `LPARAM`, `MAKELPARAM`, `LOWORD`, `HIWORD`
- Убрать `LRESULT CALLBACK` сигнатуры
- Убрать `KeyboardHook` (мёртвый глобал)

## 10. Types cleanup (механическое)

**~15 файлов** — замена Win32 типов на стандартные:
- `DWORD` → `uint32_t` (уже определён в types.h, но хедеры тянут windows.h)
- `BOOL` → `int32_t` или `bool`
- `HANDLE` → `void*`
- `HWND` → `void*` (где ещё используется)
- `LPSTR` → `char*`
- `WNDCLASS`, `HACCEL`, `HINSTANCE` — убрать (мёртвые глобалы)
- Убрать лишние `#include <windows.h>` из файлов где нет Win32 вызовов (`bucket.h`, `joystick.h`, `panel.cpp`, `tga.h` и др.)

## 11. Мёртвый код — удалить

- `wind_procs.cpp` / `wind_procs.h` — DDLibShellProc больше не вызывается
- `host_globals` — DDLibClass, hDDLibAccel, hDDLibThread, ShellID, PauseFlags, PauseCount
- `hDDLibWindow` — после cleanup пункта 1 и 9

## 12. Сборочная система

- `clang-cl` → standalone `clang++` (убрать зависимость от VS)
- CMake флаги: `/Zp1` → `-fpack-struct=1`, `/clang:-O2` → `-O2`, убрать `/SAFESEH`, `/ENTRY:mainCRTStartup`
- Убрать `vcvarsall.bat x86` из configure.ps1
- Заменить VS-bundled cmake на системный
- Линковка: убрать Windows-only libs (odbc32, odbccp32, comctl32, winmm и т.д.) — обернуть в `if(WIN32)`
- Добавить Linux/macOS toolchain файлы или условия в CMakeLists.txt

---

## Порядок работы (рекомендуемый)

| Шаг | Что | Зависимости |
|-----|-----|-------------|
| 4 | Memory | — |
| 5 | File I/O | — |
| 6 | Timer | — |
| 7 | Threading | — |
| 8 | INI config | File I/O (5) |
| 9 | Input cleanup | — |
| 10 | Types cleanup | после 4-9 |
| 11 | Мёртвый код | после 9-10 |
| 12 | Сборочная система | после всего |

Шаги 4-7 независимы друг от друга, можно делать в любом порядке. Шаг 12 — последний, когда весь код уже кросс-платформенный.
