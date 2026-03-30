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

## 12. Убрать глобальный `/Zp1` — локальный `#pragma pack` для бинарных структур

**Проблема:** весь проект компилируется с `/Zp1` (1-byte struct packing). Это ломает внешние библиотеки (SDL3, MSVC STL `<thread>`/`<mutex>`/`<condition_variable>`) и вынуждает использовать bridge-паттерн: отдельные .cpp с `/Zp8`, opaque handles, C API обёртки. Мусорная архитектура ради одного флага.

**Зачем `/Zp1` нужен:** ~23 структуры читаются из бинарных файлов (уровни, меши, анимации, сейвы) через `FileRead(handle, &struct, sizeof(struct))`. Без packed layout компилятор вставит padding → sizeof изменится → бинарные форматы перестанут читаться.

**Решение:** `#pragma pack(push, 1)` / `#pragma pack(pop)` на каждую структуру, которая зависит от packed layout. Убрать `/Zp1` из CMake. Остальной код компилируется с default alignment.

**Бонус:** исчезнет причина существования bridge-паттерна:
- `sdl3_bridge.h/cpp` — SDL3 хедеры можно будет включать напрямую
- `threading_bridge.h/cpp` — `<thread>`/`<mutex>`/`<condition_variable>` можно использовать напрямую
- Все `/Zp8` исключения в CMakeLists.txt — не нужны

### Структуры требующие `#pragma pack(push, 1)`

**Геометрия (prim_types.h):**
- `PrimPoint` (6 bytes) — вершины
- `PrimFace3` (28 bytes) — треугольные грани
- `PrimFace4` (32+ bytes) — четырёхугольные грани
- `RoofFace4` (12 bytes) — крыши

**Карта (supermap.h):**
- `DStorey` (8 bytes)
- `DFacet` (24 bytes) — фасеты
- `DBuilding` (24 bytes) — здания
- `DWalkable` (24 bytes) — walkable поверхности
- `InsideStorey`, `Staircase`

**Анимации (anim_types.h):**
- `GameKeyFrameElement` (20 bytes) — кости
- `GameKeyFrame` (20-28 bytes) — кадры анимации
- `GameFightCol` (16 bytes) — зоны удара
- `BodyDef` (20 bytes) — определения тел

**Текстуры и ресурсы:**
- `TXTY` (4 bytes, building_types.h) — UV координаты текстур
- `AnimTmap` (166 bytes, anim_tmap.h) — анимированные текстуры

**Объекты на карте:**
- `MapThingPSX` (80+ bytes, mapthing.h)
- `LoadGameThing` (40 bytes, level_loader.h)

**Физика:**
- `HM_Header` (8 bytes, hm.h)
- `HM_Primgrid` (~96 bytes, hm.h)

**Сейвы (save.cpp):**
- `SAVE_Person` (18 bytes)
- `SAVE_Person_extra` (6 bytes)
- `SAVE_Special_extra` (8 bytes)
- `SAVE_Vehicle_extra`

**Освещение:**
- `NIGHT_Square`

### Верификация

После каждого `#pragma pack(pop)` — `static_assert(sizeof(StructName) == expected_size)`. 100% надёжная проверка — компилятор скажет если sizeof не совпадает. Плюс runtime проверка: загрузка любого уровня покажет если что-то пропущено.

### Уже используют `#pragma pack` локально (не зависят от `/Zp1`)

- `game_graphics_engine.h` — `#pragma pack(push, 4)` для `GERenderState`
- `polypage.h` — `#pragma pack(push, 4)` для `PolyPage`

## 13. Сборочная система

- `clang-cl` → standalone `clang++` (убрать зависимость от VS)
- CMake флаги: убрать `/Zp1` (после п.12), `/clang:-O2` → `-O2`, убрать `/SAFESEH`, `/ENTRY:mainCRTStartup`
- Убрать `/Zp8` исключения из CMakeLists.txt (после п.12 — не нужны)
- Убрать `vcvarsall.bat x86` из configure.ps1
- Заменить VS-bundled cmake на системный
- Линковка: убрать Windows-only libs (odbc32, odbccp32, comctl32, winmm и т.д.) — обернуть в `if(WIN32)`
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
| 12 | Убрать `/Zp1` → локальный `#pragma pack` | ⏳ |
| 13 | Сборочная система | ⏳ |
