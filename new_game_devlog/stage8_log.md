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
