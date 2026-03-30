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

## 2. Input ✅

- Клавиатура: ✅ SDL3 events → `keyboard_key_down/up(scancode)` (прямой маппинг KB_* scancodes)
- Мышь: ✅ SDL3 events → `mouse_on_move/button()` (прямое обновление глобалов)
- Геймпад: ✅ уже на SDL3
- ~~Win32 прослойка (WM_*, LPARAM, LRESULT CALLBACK) — убрана~~

## 3. Звук ✅

- OpenAL уже кросс-платформенный, MSS32 → OpenAL Soft (сделано ранее)

---

## 4. Memory ✅

- ~~Win32 Heap API → `calloc`/`realloc`/`free`~~
- ~~`MFHeap` глобал и `memory_globals.cpp`/`.h` удалены~~
- **Сделано**

## 5. File I/O ✅

- ~~`MFFileHandle` (Win32 `HANDLE`) → `FILE*`~~
- ~~Win32 `CreateFile/ReadFile/WriteFile/...` → `fopen/fread/fwrite/...`~~
- ~~Пути: `\\` → `/`, абсолютные → относительные~~
- **Сделано**

## 6. Timer ✅

- ~~`QueryPerformanceCounter` / `GetTickCount()` → SDL3 bridge~~
- ~~`GetLocalTime()` → `time()` + `localtime()`~~
- ~~Все тик-переменные `DWORD` → `uint64_t`~~
- **Сделано**

## 7. Threading ✅

- async_file (единственный threading в кодовой базе) — **удалён целиком** как мёртвый код
  - `LoadAsyncFile`/`InitAsyncFile`/`TermAsyncFile` нигде не вызывались
  - Удалены: `async_file.cpp/h`, `async_file_globals.cpp/h`, `threading_bridge.cpp/h`
- **Сделано**

## 8. INI config ✅

- ~~`GetPrivateProfileInt/String` → свой INI парсер (ini_read_string/int/write_string)~~
- ~~`WritePrivateProfileString` → `ini_write_string` (перезапись файла)~~
- ~~`GetCurrentDirectory` → `_getcwd`/`getcwd` (кросс-платформенное)~~
- ~~`_MAX_PATH` → `ENV_MAX_PATH` (260, определён в env_globals.h)~~
- **Сделано**

## 9. Input cleanup ✅

- ~~Win32 прослойка убрана — SDL callbacks напрямую обновляют Keys[]/Mouse state~~
- ~~`LRESULT CALLBACK KeyboardProc/MouseProc` → `keyboard_key_down/up()`, `mouse_on_move/button()`~~
- ~~`WM_KEYDOWN`, `WM_MOUSEMOVE`, `LPARAM`, `MAKELPARAM`, `LOWORD`, `HIWORD` — убраны~~
- ~~`KeyboardHook` (мёртвый глобал) — убран~~
- **Сделано**

## 10. Types cleanup ✅

- Убраны лишние `#include <windows.h>` из 8 файлов:
  playcuts_globals.h, tga.h, joystick.h, text_globals.h, bucket.h,
  quaternion_globals.h, quaternion.cpp, panel.cpp
- Добавлен `#include <cstring>` в message.cpp (ранее получал strncpy транзитивно через windows.h)
- Оставшиеся `#include <windows.h>` — в файлах которые реально используют Win32 API:
  host.cpp (crash handler), host_globals.h (HINSTANCE для DX6), uc_common.h (umbrella),
  outro_os_globals.h (не нуждался — очищен), backend_directx6/* (легитимно)

## 11. Мёртвый код ✅

- **wind_procs.cpp** — убран из сборки (DDLibShellProc не вызывается, SDL3 event loop заменил)
- **wind_procs_globals.h** — убран `#include <windows.h>` (не нужен)
- **host_globals** — удалены мёртвые глобалы:
  `hGlobalPrevInst`, `iGlobalWinMode`, `ShellID`, `hDDLibAccel`, `hDDLibThread`,
  `lpszGlobalArgs`, `DDLibClass`, `PauseFlags`, `PauseCount`, `argc`, `argv`
  Оставлены: `hGlobalThisInst` (DX6 display), `ShellActive` (host.cpp event loop)
- **outro_os_globals** — удалены мёртвые Win32 глобалы:
  `OS_this_instance`, `OS_last_instance`, `OS_command_line`, `OS_start_show_state`,
  `OS_wcl` (WNDCLASSEX), `OS_window_handle` (HWND)
  Убран `#include <windows.h>`
- **HOST_run bug fix** — `MF_main(argc, argv)` использовал неинициализированные глобалы вместо параметров → исправлено на `MF_main((UWORD)argc_in, argv_in)`

## 12. Убрать глобальный `/Zp1` — локальный `#pragma pack` для бинарных структур ✅

- ~~Глобальный `/Zp1` из CMake — убран~~
- ~~Все `/Zp8` исключения (sdl3_bridge, ds_bridge, GLAD, gl_context, gl_shader) — убраны~~
- ~~`WINDOWS_IGNORE_PACKING_MISMATCH` — убран~~
- **25 структур** обёрнуты `#pragma pack(push, 1)` / `#pragma pack(pop)` + `static_assert`:
  - **prim_types.h:** PrimPoint(6), RoofFace4(10), PrimFace3(28), PrimFace4(34), PrimFace4PSX(24), PrimFace3PSX(20), PrimObject(16)
  - **supermap.h:** DStorey(6), DFacet(26), DBuilding(24), DWalkable(22)
  - **anim_types.h:** PrimMultiAnim(50), BodyDef(20), GameKeyFrameElementCompOld(12), GameKeyFrameElementComp(8), GameKeyFrameElementBig(20), GameKeyFrameElement(20), GameFightCol, GameKeyFrame
  - **building_types.h:** TXTY(4)
  - **anim_tmap.h:** AnimTmap(166)
  - **mapthing.h:** MapThingPSX
  - **level_loader.h:** LoadGameThing(44)
  - **hm.h:** HM_Header(8), HM_Primgrid(112)
  - **save.cpp:** SAVE_Person(18), SAVE_Person_extra(6), SAVE_Special_extra(8), SAVE_just_vehicle(14), SAVE_Vehicle_extra(8)
  - **inside2.h:** InsideStorey(22), Staircase(10)
  - **night.h:** NIGHT_Square
- Структуры с указателями (GameFightCol, GameKeyFrame, MapThingPSX, NIGHT_Square) — без static_assert (размер зависит от sizeof(void*))
- `game_graphics_engine.h` (`#pragma pack(push, 4)` для GERenderState) и `polypage.h` (`#pragma pack(push, 4)` для PolyPage) — уже были, не затронуты
- **Верификация:** компиляция Release+Debug OK, загрузка уровня в игре OK
- **Бонус:** bridge-паттерн (`sdl3_bridge`, `/Zp8` исключения) больше не нужен — SDL3 хедеры можно включать напрямую

## 13. Убрать Windows-зависимости из игрового кода ✅

### Удалено
- **`#include <windows.h>` из `uc_common.h`** — главный umbrella хедер больше не тянет Windows
- **`WIN32`, `_WINDOWS`** define'ы из CMakeLists.txt — не нужны (компилятор ставит `_WIN32` сам)
- **9 мёртвых Windows-библиотек** из линковки: odbc32, odbccp32, comctl32, imm32, Version, winmm, amstrmid, quartz, strmbase
- **wind_procs.h/cpp** — мёртвый код (Win32 WndProc), удалён
- **Editor file dialogs** в elev.cpp — весь OPENFILENAME/GetOpenFileName блок удалён
- **`hDDLibWindow`** — убран из OpenGL/stub бэкендов и host.cpp (остался только в DX6)
- **`sdl3_window_get_native_handle()`** — удалена из бриджа (не нужна без DX6)
- **`hGlobalThisInst`** — перенесён в DX6 бэкенд (display.cpp)
- **Single-instance guard** (CreateEventA) — удалён

### Заменено на кросс-платформенное
- `ZeroMemory(p, s)` → `memset(p, 0, s)` (~15 мест)
- `timeGetTime()` → `sdl3_get_ticks()` (2 места)
- `GetKeyNameText()` → `sdl3_get_key_name()` (новая функция в бридже)
- `MessageBox()` → `fprintf(stderr, ...)` (game.cpp)
- `ShowCursor/SetCapture/ReleaseCapture` → `sdl3_show_cursor()`/`sdl3_hide_cursor()`/`sdl3_set_mouse_grab()` (новые функции в бридже)
- `GetCursorPos/SetCursorPos/ScreenToClient` → `sdl3_get_global_mouse_pos()`/`sdl3_warp_mouse_global()`/`sdl3_window_get_position()` (widget.cpp, outro_os.cpp)
- `GetAsyncKeyState(VK_LBUTTON)` → `LeftButton` глобал (widget.cpp)
- `GetClientRect` → `sdl3_window_get_size()` (game_tick.cpp)
- `OutputDebugString` → `fprintf(stderr, ...)` (host.cpp, outro_os.cpp)
- `CreateDirectory` → `oc_mkdir()` (game_tick.cpp, frontend.cpp)
- `GetCurrentDirectory` → `oc_getcwd()` (elev.cpp)
- `GetPrivateProfileInt/Section` → `INI_get_int()`/`INI_get_section()` (texture.cpp, mfx.cpp, sound.cpp)
- `FILETIME/GetFileTime/CompareFileTime` → `stat()/st_mtime` (frontend.cpp)
- `TCHAR` → `char`, `wsprintf` → `sprintf`, `TEXT()` → удалён
- `LOWORD/HIWORD` → определены в types.h
- Crash handler: Win32 SEH → кросс-платформенный `signal(SIGSEGV/SIGABRT/SIGFPE)`
- `strnicmp` → `oc_strnicmp` (кросс-платформенный макрос)

### Кросс-платформенные compat-макросы (types.h)
- `oc_getcwd`, `oc_stricmp`, `oc_strnicmp`, `oc_mkdir` — POSIX имена, на MSVC маппятся в `_getcwd` и т.д.
- `BYTE`, `WORD`, `CHAR`, `DWORD`, `BOOL`, `TCHAR`, `LOWORD`, `HIWORD`, `MAX_PATH`, `TEXT` — определены в types.h с guard'ами `_WINDEF_`/`_WINNT_`

### Результат
- **Ноль `_WIN32` в игровом коде** — только в GLAD (сторонняя) и DX6 бэкенде
- **`windows.h`** включается только в DX6 бэкенде (4 файла)
- **Бридж** (`sdl3_bridge.h/cpp`) — полностью кросс-платформенный, без `_WIN32`
- **Верификация:** Release+Debug OK, игра загружается и работает

## 14. Сборочная система

- `clang-cl` → standalone `clang++` (убрать зависимость от VS)
- ~~CMake флаги: убрать `/Zp1`~~ (сделано в п.12)
- ~~Убрать `/Zp8` исключения из CMakeLists.txt~~ (сделано в п.12)
- CMake флаги: `/clang:-O2` → `-O2`, убрать `/SAFESEH`, `/ENTRY:mainCRTStartup`
- Убрать `vcvarsall.bat x86` из configure.ps1
- Заменить VS-bundled cmake на системный
- Добавить Linux/macOS toolchain файлы или условия в CMakeLists.txt

---

## Порядок работы

| Шаг | Что | Статус |
|-----|-----|--------|
| 1 | Оконная система + GL контекст | ✅ |
| 2 | Input (SDL3 events) | ✅ |
| 3 | Звук | ✅ (ранее) |
| 4 | Memory | ✅ |
| 5 | File I/O | ✅ |
| 6 | Timer | ✅ |
| 7 | Threading | ✅ |
| 8 | INI config | ✅ |
| 9 | Input cleanup | ✅ |
| 10 | Types cleanup | ✅ |
| 11 | Мёртвый код | ✅ |
| 12 | Убрать `/Zp1` → локальный `#pragma pack` | ✅ |
| 13 | Убрать Windows-зависимости из игрового кода | ✅ |
| 14 | Сборочная система | ⏳ |
