# Stage 8 — Кросс-платформа (девлог)

## 2026-03-30: SDL3 окно + GL контекст + event loop

### Что сделано

Заменили Win32 оконную систему на SDL3. Окно, GL контекст и event loop теперь через SDL3 bridge.

**Изменённые файлы:**

| Файл | Что изменилось |
|------|---------------|
| `sdl3_bridge.h/cpp` | Добавлены bridge-функции: окно (`create/destroy/show/set_size/get_size/get_drawable_size/get_native_handle`), GL контекст (`create/destroy/swap/get_proc_address`), event loop (`SDL3_Callbacks` + `poll_events`), таблица маппинга SDL_Scancode → Win32 Set 1 scancodes |
| `gl_context.cpp/h` | Полностью переписан: WGL → SDL3 bridge. Из ~180 строк Win32-specific стало ~60 строк |
| `host.cpp/h` | `SetupHost`: SDL3 окно вместо `RegisterClass`+`CreateWindowEx`. `LibShellActive`: `sdl3_poll_events` вместо `PeekMessage`/`GetMessage`. Event callbacks конвертируют SDL в формат `KeyboardProc`/`MouseProc`. `HOST_run` принимает `(int argc, char* argv[])` вместо WinMain параметров |
| `main.cpp` | `WinMain` → обычный `main()` + `#undef main` (макрос в uc_common.h) |
| `mouse.cpp` | `RecenterMouse` через bridge (`sdl3_window_get_center` + `sdl3_warp_mouse_global`) вместо Win32 API |
| `CMakeLists.txt` | `/ENTRY:mainCRTStartup` для main() при SUBSYSTEM:WINDOWS (без консоли) |

**Ключевые решения:**

- **Bridge-паттерн сохранён:** SDL3 хедеры по-прежнему только в `sdl3_bridge.cpp` (из-за `/Zp1`). Все новые функции через `sdl3_bridge.h`
- **hDDLibWindow сохранён:** устанавливается через `sdl3_window_get_native_handle()` — legacy код (game_tick, widget, elev) продолжает работать. Полная очистка — follow-up
- **Scancode маппинг:** SDL_Scancode (USB HID) → Win32 Set 1 (KB_* коды). ~80 клавиш. Keyboard/Mouse proc пока принимают Win32 message codes (WM_KEYDOWN и т.д.) — полная кросс-платформенность input будет следующим шагом
- **Gamepad events:** `sdl3_poll_events` теперь единый event loop (SDL_PollEvent). Gamepad connect/disconnect events сохраняются во внутреннюю очередь и читаются через `sdl3_gamepad_poll_event` как раньше
- **Окно фиксированного размера** (без SDL_WINDOW_RESIZABLE)

### Что осталось для полной кросс-платформенности

- Input: keyboard.cpp / mouse.cpp всё ещё используют Win32 типы (WM_*, LPARAM)
- hDDLibWindow: 4 места за пределами окна/GL (game_tick, widget, elev, mouse — mouse уже мигрирован)
- host.cpp: windows.h включен безусловно (нужен для crash handler)
- wind_procs.cpp: DDLibShellProc мёртвый код (никто не вызывает), убрать при cleanup
- host_globals: часть глобалов (DDLibClass, hDDLibAccel и др.) больше не нужна

## 2026-03-30: Timer — QPC/GetTickCount → SDL3

### Что сделано

Полная миграция таймеров с Win32 API на кросс-платформенные вызовы. Одновременно завершён переход тик-переменных на 64-бит (начатый на Этапе 4 как BUGFIX-OC-TICK-OVERFLOW: SLONG → DWORD → uint64_t).

**Изменённые файлы (38 шт):**

| Категория | Что изменилось |
|-----------|---------------|
| `sdl3_bridge.h/cpp` | +3 bridge-функции: `sdl3_get_ticks()`, `sdl3_get_performance_counter()`, `sdl3_get_performance_frequency()` |
| `timer.cpp/h` | `QueryPerformanceCounter`/`Frequency` → `sdl3_get_performance_counter/frequency`. Убраны `GetFineTimerFreq/Value` helper'ы. Убран `#include <windows.h>` |
| `timer_globals.h/cpp` | `ULONG stopwatch_start` → `uint64_t` |
| `uc_common.h` | `MFTime::Ticks` → `uint64_t`. Добавлен `#include <stdint.h>` |
| `host.cpp` | `Time()`: `GetLocalTime()`/`SYSTEMTIME` → `time()`/`localtime()`, `GetTickCount()` → `sdl3_get_ticks()`. Убран `#ifdef _WIN32` |
| 13 .cpp файлов | `GetTickCount()` → `sdl3_get_ticks()` (~40 вызовов): game.cpp, input_actions.cpp, console.cpp, psystem.cpp, thing.cpp, person.cpp, panel.cpp, eng_map.cpp, frontend.cpp, gamemenu.cpp, pause.cpp, mesh.cpp, outro_os.cpp |
| ~25 файлов (.h/.cpp) | Все `BUGFIX-OC-TICK-OVERFLOW` переменные: `DWORD` → `uint64_t` (глобалы, static locals, struct fields) |
| Cleanup | Убраны `#include <windows.h>` из console.cpp, console_globals.h/.cpp, input_actions_globals.h, outro_os.cpp |

**Ключевые решения:**

- **Bridge-паттерн:** SDL3 вызовы через bridge (как и всё остальное) — из-за /Zp1 ограничения
- **MSeconds = 0:** `localtime()` не даёт миллисекунды. `MFTime::MSeconds` нигде не читается в кодовой базе — потери нет
- **Safe truncation оставлена:** `tick_diff = (SLONG)(cur_tick - prev_tick)` в нескольких местах — дельты фреймов (миллисекунды), не переполняются. `OS_ticks()` возвращает SLONG — outro длится минуты
- **`(unsigned)` → `(uint64_t)`:** исправлен баг усечения в panel.cpp (fadeout_time cast)
- **MAP_Beacon.ticks** → `uint64_t`: структура аллоцируется через save_table runtime pool, sizeof корректно

### Замечания

- `MFTime::MSeconds` всегда 0 — если когда-нибудь понадобятся миллисекунды в wall-clock time, нужно использовать platform-specific API или `std::chrono`
- Комментарии `// claude-ai:` в BUGFIX-OC-TICK-OVERFLOW строках убраны — в `new/` все комментарии и так написаны Claude

## 2026-03-30: Memory — HeapAlloc → calloc/free

- `memory.cpp` — `HeapAlloc(HEAP_ZERO_MEMORY)` → `calloc`, `HeapFree` → `free`, `HeapReAlloc` → `realloc`
- `SetupMemory`/`ResetMemory` — теперь no-op (отдельный Win32 heap не нужен)
- `MFHeap` глобал и `memory_globals.cpp`/`.h` — удалены (никто не использовал напрямую)
- `memory.h` — убран `#include <windef.h>`, `BOOL` добавлен в `types.h` с guard `#ifndef _WINDEF_`
- `MemReAlloc` — нигде не вызывается в кодовой базе, оставлен для API совместимости

## 2026-03-30: File I/O — Win32 HANDLE → FILE*

- `file.h` — `MFFileHandle` теперь `FILE*` вместо `HANDLE`. Sentinel'ы ошибок (`FILE_OPEN_ERROR` и т.д.) = `NULL`
- `file.cpp` — `CreateFile` → `fopen`, `ReadFile` → `fread`, `WriteFile` → `fwrite`, `GetFileSize` → `ftell`, `SetFilePointer` → `fseek`, `CloseHandle` → `fclose`, `DeleteFile` → `remove`, `GetFileAttributes` → `fopen`+`fclose` проверка
- `FileSetBasePath` — теперь принимает и `/` и `\\` как разделитель
- Бэкслэши в путях → `/` по всей кодовой базе (~28 файлов): `server\\`, `data\\`, `levels\\`, `talk2\\`, `Meshes\\` и т.д.
- Абсолютные пути убраны: `C:\\Windows\\Desktop\\...` → относительные, `c:\\fallen.ini` → `fallen.ini`
- Места с `strrchr(fname, '\\')` и `*ch == '\\'` дополнены проверкой `/`

### Замечено

- **CRT Debug Assertion:** `_CrtIsValidHeapPointer(block)` в `debug_heap.cpp:904` — давняя проблема, периодически в Debug build. Double free или невалидный указатель в legacy коде

## 2026-03-30: Threading — Win32 → threading_bridge

### Что сделано

Полная миграция threading в async_file с Win32 API на C++ standard library через bridge-паттерн.

**Новые файлы:**

| Файл | Что делает |
|------|-----------|
| `threading_bridge.h` | C API: `ThreadMutex`, `ThreadCondVar`, `ThreadHandle` — opaque handles + функции create/destroy/lock/unlock/wait/notify/join/yield |
| `threading_bridge.cpp` | Реализация через `std::mutex`, `std::condition_variable`, `std::thread`. Компилируется с `/Zp8` (как sdl3_bridge) |

**Изменённые файлы:**

| Файл | Что изменилось |
|------|---------------|
| `async_file.h` | `HANDLE hFile` → `FILE*`, `DWORD blen` → `uint32_t blen`, убран `#include <windows.h>` |
| `async_file.cpp` | `CRITICAL_SECTION` → `thread_mutex_*`, `CreateEvent/SetEvent/WaitForSingleObject` → `thread_condvar_*`, `CreateThread` → `thread_create`, `Sleep(0)` → `thread_yield`, `ReadFile/CreateFile` → `fread/MF_Fopen` |
| `async_file_globals.h/cpp` | `CRITICAL_SECTION csLock` → `ThreadMutex csLock`, `HANDLE hEvent` → `ThreadCondVar cvEvent`, `HANDLE hThread` → `ThreadHandle workerThread` |
| `CMakeLists.txt` | Добавлен `threading_bridge.cpp` в sources + `/Zp8` |

**Ключевые решения:**

- **Bridge-паттерн** — та же причина что SDL3: `/Zp1` ломает alignment static_asserts в `<thread>`/`<mutex>`/`<condition_variable>` MSVC STL headers
- **Predicate через callback** — `thread_condvar_wait` принимает function pointer (не lambda), чтобы API оставался чистым C
- **TermAsyncFile упрощён** — вместо spin-wait + CloseHandle используется `thread_join` (блокирующий, RAII cleanup)

## 2026-03-30: INI config — GetPrivateProfile* → свой парсер

- `env.cpp` — полностью переписан: `GetPrivateProfileInt/String` → `ini_read_int/string` (простой парсер), `WritePrivateProfileString` → `ini_write_string` (перезапись файла)
- `GetCurrentDirectory` → `_getcwd`/`getcwd` (кросс-платформенное через `#ifdef _WIN32`)
- `stricmp` → `_stricmp`/`strcasecmp` через макрос `oc_stricmp`
- `env_globals.h` — `_MAX_PATH` → `ENV_MAX_PATH` (260), убран `#include <windows.h>`
- INI парсер: секции `[Section]`, ключи `key=value`, комментарии `;`/`#`, case-insensitive section/key matching

## 2026-03-30: Input cleanup — убрана Win32 прослойка

### Что сделано

Убрана прослойка которая конвертировала SDL events в Win32 message format (WM_KEYDOWN, LPARAM encoding) и обратно.

**Изменённые файлы:**

| Файл | Что изменилось |
|------|---------------|
| `keyboard.cpp` | `LRESULT CALLBACK KeyboardProc(int, WPARAM, LPARAM)` → `keyboard_key_down/up(UBYTE scancode)`. Убраны KEYMASK_* defines, CallNextHookEx, `#include <windows.h>` |
| `keyboard.h` | Добавлены `keyboard_key_down(UBYTE)`, `keyboard_key_up(UBYTE)` |
| `keyboard_globals.h` | Убраны `#include <windows.h>` и `HHOOK KeyboardHook` |
| `keyboard_globals.cpp` | Убран `KeyboardHook` |
| `mouse.cpp` | `LRESULT CALLBACK MouseProc(int, WPARAM, LPARAM)` → `mouse_on_move(int, int)`, `mouse_on_button(int, bool, int, int)`. Убраны WM_*, LOWORD/HIWORD, `#include <windows.h>` |
| `mouse.h` | Добавлены `mouse_on_move`, `mouse_on_button` |
| `host.cpp` | Callback'и `on_key_down/up/mouse_move/button` упрощены — прямые вызовы вместо LPARAM encoding. Убраны extern KeyboardProc/MouseProc |
| `message.cpp` | Добавлен `#include <cstring>` (strncpy больше не приходит транзитивно) |

### Побочный эффект

`wind_procs.cpp` перестал линковаться (ссылался на удалённые KeyboardProc/MouseProc). DDLibShellProc нигде не вызывается → файл убран из сборки.

## 2026-03-30: Types + dead code cleanup

### Types cleanup

Убраны лишние `#include <windows.h>` из 8 файлов: `playcuts_globals.h`, `tga.h`, `joystick.h`, `text_globals.h`, `bucket.h`, `quaternion_globals.h`, `quaternion.cpp`, `panel.cpp`.

Оставлены файлы реально использующие Win32 API: `host.cpp`, `host_globals.h`, `uc_common.h`, backend_directx6/*.

### Dead code cleanup

- **wind_procs.cpp** — убран из CMakeLists (DDLibShellProc мёртвый, SDL3 event loop заменил)
- **wind_procs_globals.h** — убран `#include <windows.h>`
- **host_globals.h/cpp** — удалены 11 мёртвых глобалов: `hGlobalPrevInst`, `iGlobalWinMode`, `ShellID`, `hDDLibAccel`, `hDDLibThread`, `lpszGlobalArgs`, `DDLibClass`, `PauseFlags`, `PauseCount`, `argc`, `argv`. Оставлены: `hGlobalThisInst` (DX6), `ShellActive`
- **outro_os_globals.h/cpp** — удалены 6 мёртвых Win32 глобалов: `OS_this_instance`, `OS_last_instance`, `OS_command_line`, `OS_start_show_state`, `OS_wcl`, `OS_window_handle`. Убран `#include <windows.h>`
- **host.cpp bug fix** — `MF_main(argc, argv)` передавал неинициализированные глобалы → исправлено на `MF_main((UWORD)argc_in, argv_in)`

### Результат

Warnings: 78 → 56. Object files: 318 → 317 (убран wind_procs.cpp).

## 2026-03-31: Удалён мёртвый async_file + threading_bridge

При ревью обнаружено: `LoadAsyncFile`/`InitAsyncFile`/`TermAsyncFile`/`GetNextCompletedAsyncFile`/`CancelAsyncFile` нигде не вызываются за пределами async_file.cpp. Это часть DDLibrary MuckyFoot — заготовка для асинхронной загрузки файлов, не использованная в PC билде.

Удалены:
- `async_file.cpp`, `async_file.h`, `async_file_globals.cpp`, `async_file_globals.h`
- `threading_bridge.cpp`, `threading_bridge.h` (единственный потребитель был async_file)

Object files: 317 → 314.

## 2026-03-31: 14a — Компилятор clang-cl → clang++

### Что сделано

Переход с `clang-cl` (MSVC frontend) на `clang++` (GNU-style frontend). Убрана зависимость от vcvarsall.bat — clang++ автоматически находит MSVC.

**Изменённые файлы:**

| Файл | Что изменилось |
|------|---------------|
| `cmake/clang-x86-windows.cmake` | `clang-cl` → `clang++`/`clang` |
| `CMakeLists.txt` | Compile flags: `/clang:-O2` → `-O2` и т.д. Linker flags: обёрнуты в `if(WIN32)` + `LINKER:` prefix. OpenGL lib: платформо-зависимый (`opengl32`/`OpenGL::GL`/`GL`) |
| `scripts/configure.ps1` | Убран vcvarsall.bat. cmake/ninja: ищутся в PATH (без VS fallback). Добавлен `-DCMAKE_MAKE_PROGRAM` для ninja |
| `scripts/build.ps1` | cmake из PATH (без VS fallback) |
| `Makefile` | `CMAKE` = `cmake` (системный) |
| `SETUP.md` | Новые prerequisites: `winget install` cmake, ninja, LLVM. VS Build Tools как лёгкая альтернатива полной VS |
| `README.md` | Build system: уточнено "Clang++ (standalone, no clang-cl)" |

**Ключевые решения:**

- **MSVC ABI сохранён:** target остаётся `i686-pc-windows-msvc`, vcpkg triplet `x86-windows` — совместимость с существующими пакетами
- **VS Build Tools вместо полной VS:** для Windows SDK + MSVC runtime достаточно Build Tools (~3 GB вместо ~10+ GB)
- **Линкер flags через `LINKER:` prefix:** CMake корректно транслирует MSVC-style flags (`/SAFESEH:NO`, `/ENTRY:mainCRTStartup`, `/DEBUG`) через clang++ driver в lld-link
- **Без VS fallback:** cmake, ninja, clang++ — все из PATH, никаких хардкоженных VS путей

## 2026-03-31: 14b — Скрипты → кросс-платформенные

### Что сделано

Убраны PowerShell скрипты. Вся логика сборки — в Makefile (bash).

- **Удалены:** `scripts/configure.ps1`, `scripts/build.ps1`, `scripts/copy_resources.ps1`, каталог `scripts/`
- **Makefile:** все вызовы `$(POWERSHELL) -File ...` → прямые `cmake` / `cmake -E` команды
- **vcpkg auto-detect:** через `vswhere` (Windows) или `VCPKG_ROOT` (macOS/Linux) — в Makefile, не в скрипте
- **copy-resources:** robocopy → `cmake -E copy_directory` (кросс-платформенное)
- **run:** PowerShell `Start-Process` → `cd dir && ./Fallen.exe`
- **SETUP.md:** обновлены prerequisites (`winget install` для cmake, ninja, LLVM, make)

## 2026-03-31: 14c — CMake кросс-платформенные условия

- `add_executable(Fallen WIN32 ...)` → `add_executable(Fallen ...)` + `set_target_properties(WIN32_EXECUTABLE TRUE)` в `if(WIN32)`
- Clang version check: пропуск для AppleClang (своя нумерация)
- D3D6 backend: `FATAL_ERROR` если выбран не на Windows
- DLL копирование: обёрнуто в `if(WIN32)`, triplet path через `${VCPKG_TARGET_TRIPLET}` (не хардкод)
- `_CRT_SECURE_NO_WARNINGS`: только на Windows
- OpenGL library: `opengl32` (Win) / `OpenGL::GL` (macOS) / `GL` (Linux)
- CMake 4.2 CMP0175 warnings: убраны — `CONFIGURATIONS` в POST_BUILD заменён на `$<$<CONFIG:...>:>` generator expressions

## 2026-03-31: 14d — Переход на 64-бит (IN PROGRESS)

### Шаг 1. Inline asm → C ✅
- `fixed_math.h`: `MUL64`/`DIV64` — x86 `__asm` (MSVC-синтаксис) → `int64_t` арифметика
- MSVC не поддерживает `__asm` в x64 — блокер компиляции. Других `__asm` в проекте нет.

### Шаг 2. `long` → фиксированные типы ✅
- `long` — единственный примитивный C-тип с разным размером на 64-бит: Win LLP64 = 4, Linux/Mac LP64 = 8
- Компиляторной опции для фиксации нет — размер определяется ABI платформы
- types.h: `unsigned long` → `uint32_t` для ULONG, SLONG, DWORD. outro_always.h: аналогично.
- ~45 голых `long` в panel.h/cpp, panel_globals.h/cpp, figure.cpp, polypage.cpp → `SLONG`/`ULONG`
- Добавлен `#include "engine/core/types.h"` в panel.h (ранее получал типы транзитивно)

### Шаг 3. Pointer → int касты → `uintptr_t` ✅
- anim_loader.cpp (~15 мест): pointer relocation `(ULONG)ptr` → `(uintptr_t)ptr`
- memory.cpp (~30 мест): convert_drawtype/keyframe/fightcol/animlist/thing ptr↔index
- person.cpp: ASSERT каст
- aeng.cpp: pointer alignment `& 0xffffffe0` → `& ~(uintptr_t)0x1f`
- pyro.cpp: address-as-seed double cast
- widget.h: `SLONG data[5]` → `intptr_t data[5]` (хранит и числа и указатели)
- widget.cpp: 7 мест pointer→data[] кастов

### Шаг 4. Packed structs с указателями в файловом I/O ✅
- Только ассеты нуждаются в фиксе (save/load самосогласован — sizeof одинаков для write/read)
- `GameKeyFrame_Disk` / `GameFightCol_Disk` (anim_types.h) — on-disk структуры с `uint32_t` вместо указателей
- anim_loader.cpp: bulk read в `_Disk` temp буфер, поэлементное копирование в runtime struct
- AnimList: fread `uint32_t[]` → expand в `GameKeyFrame*[]`
- MapThingPSX: указатели `CurrentFrame`/`NextFrame` → `uint32_t` (никогда не используются после чтения)
- static_assert'ы: `GameKeyFrame_Disk==20`, `GameFightCol_Disk==16`, `MapThingPSX==82`

### Шаг 5. sizeof-зависимости ✅
- game_tick.cpp: `sizeof(names) >> 2` → `sizeof(names)/sizeof(names[0])`

### Шаг 6. Тулчейн x64 ✅
- Новый `cmake/clang-x64-windows.cmake`: `x86_64-pc-windows-msvc`, triplet `x64-windows`
- Makefile: TOOLCHAIN → новый файл
- CMakeLists.txt: `/SAFESEH:NO` обёрнут в `if(x86)`
- `SetLastClumpfile(char*, unsigned int)` → `size_t` — linker mismatch на x64

### Шаг 7. Верификация 🔧
**Runtime фиксы при верификации:**
- **file_clump.cpp**: `size_t` в fread/fwrite → `uint32_t` (clump файлы = 32-бит формат)
- **playcuts.cpp**: `sizeof(CPChannel)` в FileRead → фиксированная on-disk структура 12 байт

**Полный sweep pointer truncation ✅** (11 мест в 6 файлах):
- **widget.h/cpp** — `WIDGET_Data` callback typedef + 3 реализации: `SLONG data1/data2` → `intptr_t`.
  `(SLONG)MemAlloc()` → `(intptr_t)`. 3 call-site каста `(SLONG)str/tmp` → `(intptr_t)`.
- **playcuts.cpp** — `pos.X` хранил `(SLONG)PLAYCUTS_text_ptr` (абсолютный указатель → обрезка).
  Редизайн: хранит offset+1 от `PLAYCUTS_text_data` (0 = "нет текста" sentinel).
  Убрана ptr↔offset конверсия в memory.cpp save/load (pos.X уже offset).
- **memory.cpp** — `(SLONG)NET_PERSON/NET_PLAYER` → `(intptr_t)`, `(SLONG)EWAY_mess` → `(intptr_t)`.
- **pyro.cpp** — 2 места: `(SLONG)pyro` → `(SLONG)(uintptr_t)` (PRNG seed, double cast).
- **vehicle.cpp** — 2 места: `SLONG(p_car)` → `(SLONG)(uintptr_t)` (siren animation offset).

**Runtime фиксы (sentinel, sizeof, alignment):**
- **memory.cpp `convert_*_to_pointer`**: sentinel -1 (0xFFFFFFFF) zero-extended до 0x00000000FFFFFFFF →
  `(intptr_t) < 0` не срабатывала → индексация по 4 млрд → SIGSEGV в `check_combat_hit_with_person`.
  Фикс: `int32_t idx = (int32_t)(uintptr_t)ptr; if (idx < 0) NULL; else &array[idx];`
- **anim_loader.cpp `load_append_game_chunk`**: FileRead sizeof runtime вместо disk:
  GameKeyFrame 36 vs 20, pointer 8 vs 4, GameFightCol 20 vs 16. Фикс: _Disk temp буфер.
- **anim_loader.cpp old-format relocation**: byte-offset relocation сломана при изменённом sizeof.
  Переписана на index-based: `(old_ptr - old_base) / sizeof(_Disk)` → `&new_array[index]`.
- **figure_globals.cpp** (7 мест) + **figure.cpp** (1 место): `(DWORD)ptr & ~mask` → `(uintptr_t)`.

**Crash handler восстановлен:**
- `crash_handler_win.cpp` — отдельный TU (windows.h конфликтует с types.h).
  SetUnhandledExceptionFilter → crash_log.txt с Exception, RVA, регистрами, стек с символами.
  На не-Windows: signal() fallback в host.cpp.

**Runtime фиксы (heap corruption):**
- **heap.cpp** — `HEAP_QUANTISE` (16) < `sizeof(HEAP_Free)` (32 на x64).
  Leftover 16 байт → записывается 32-байтный HEAP_Free → overflow → corruption соседних аллокаций.
  Краш проявлялся как `NIGHT_Square.colour = 0xFFFFFFFFFFFFFFFF` → SIGSEGV в `HEAP_give`.
  Фикс: `#define HEAP_QUANTISE (sizeof(HEAP_Free) < 16 ? 16 : sizeof(HEAP_Free))`
- **figure.cpp:1601** — `(DWORD)pcBlock < (DWORD)pPrimObj->pwListIndices` → `(uintptr_t)` (pointer
  truncation в ASSERT, не влияет на flow но ложное срабатывание на x64)

**Makefile:** `make d` / `make r` теперь пишут stderr в файл `stderr.log` рядом с exe (rm -f перед стартом).

**Текущий статус:**
- x64 билд компилируется (Release + Debug)
- Запуск → главное меню ✅
- Загрузка уровня ✅, синематик ✅, геймплей ✅
- ⏳ Визуальный баг: некоторые стены и ящики не рисуются вблизи (появился после x64, rendering
  pipeline аудирован — чисто, подозрение на corrupted данные или memory issue)
- ⏳ Outro: 3D модель справа не видна (возможно IMP_Mesh pointer issue)
