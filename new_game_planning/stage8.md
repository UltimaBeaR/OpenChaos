# Этап 8 — Кросс-платформа

**Цель:** убрать Windows-специфичный код и зависимость от Visual Studio.
**Критерий завершения:** компилируется и запускается (OpenGL бэкенд) на macOS Apple Silicon (M1+). Сборка должна проходить нативно на самом маке, не кросс-компиляцией.
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
- **Бридж** (`sdl3_bridge.h/cpp`) — `sdl3_window_get_native_handle()` единственная функция с `_WIN32` (нужна DX6)
- **DX6 бэкенд:** `hDDLibWindow` заполняется в `OpenDisplay`, `sdl3_window_set_size` после `SetWindowPos` для синхронизации SDL
- **Makefile:** `configure`/`reconfigure` → `configure-opengl`/`configure-d3d6` (явный выбор бэкенда)
- **Верификация:** OpenGL и DX6 бэкенды — оба компилируются и работают

## 14. Сборочная система

### 14a. Компилятор: clang-cl → clang++ ✅
- `clang-cl` → standalone `clang++` (убрать зависимость от VS)
- ~~CMake флаги: убрать `/Zp1`~~ (сделано в п.12)
- ~~Убрать `/Zp8` исключения из CMakeLists.txt~~ (сделано в п.12)
- CMake флаги: `/clang:-O2` → `-O2`, убрать `/SAFESEH`, `/ENTRY:mainCRTStartup`
- Toolchain: `clang-cl` → `clang++`, `--target=i686-pc-windows-msvc` (пока 32-бит)
- Убрать `vcvarsall.bat x86` из configure.ps1 (clang++ автоматически находит MSVC)

### 14b. Скрипты → кросс-платформенные ✅
- Убрать PowerShell скрипты (`configure.ps1`, `build.ps1`, `copy_resources.ps1`)
- Вся логика сборки — в Makefile (bash)
- `CMAKE` → системный `cmake` (не VS-бандленный)
- `copy-resources` → `cp -r` / `rsync` вместо robocopy

### 14c. CMake: кросс-платформенные условия ✅
- Платформо-зависимые флаги компилятора/линковщика (`WIN32`, `APPLE`, `UNIX`)
- Платформо-зависимые библиотеки (opengl32 → find_package(OpenGL))
- `add_executable(Fallen WIN32 ...)` → условно
- vcpkg DLL копирование — только на Windows
- macOS/Linux: отладочная информация (`-g` вместо `/DEBUG`)

### 14d. Переход на 64-бит ⏳

Все правки кода (шаги 1-5) безопасны на текущей 32-бит сборке — ничего не ломают.
Переключение на x64 (шаг 6) — только после завершения всех правок.

#### Шаг 1. Inline asm → C ✅
- `fixed_math.h`: `MUL64`/`DIV64` — x86 `__asm` заменён на `int64_t` арифметику
- MSVC не поддерживает `__asm` в x64 режиме — блокер компиляции
- Других `__asm` в проекте нет

#### Шаг 2. `long` → фиксированные типы ✅
- **Проблема:** `long` — единственный примитивный C-тип с разным размером на 64-бит платформах:
  Windows LLP64 = 4 байта, Linux/macOS LP64 = 8 байт.
  Все остальные типы (`char`, `short`, `int`) одинаковы на всех платформах.
- **Компиляторной опции для фиксации размера `long` нет** — размер определяется ABI платформы,
  менять его нельзя без нарушения совместимости с системными библиотеками (WinAPI, libc, SDL, OpenAL)
- **typedef'ы (3 строки, подтягивают весь код автоматически):**
  - types.h: `typedef unsigned long ULONG` → `typedef uint32_t ULONG`
  - types.h: `typedef signed long SLONG` → `typedef int32_t SLONG`
  - types.h: `typedef unsigned long DWORD` → `typedef uint32_t DWORD`
  - outro_always.h: аналогичные дубликаты ULONG/SLONG
- **Голые `long` в коде** (~45 мест, механическая замена):
  - panel.h/cpp, panel_globals.h/cpp — параметры функций, переменные → `int32_t` или `SLONG`/`ULONG`
  - figure.cpp (3 места), polypage.cpp (1 место) — `unsigned long EVal` → `uint32_t`
  - file.cpp:76,78 — `long pos = ftell()` — оставить как есть (возврат `ftell` = `long` по стандарту C)

#### Шаг 3. Pointer → int касты → `uintptr_t` ✅
- **Проблема:** `(ULONG)ptr` на 64-бит обрезает старшие 4 байта адреса → молчаливая порча данных,
  краш может произойти не сразу, а позже в другом месте — самый трудноотлаживаемый тип бага
- **Сделано — все `(ULONG)ptr` и `(SLONG)ptr` касты заменены на `uintptr_t`/`intptr_t`:**
  - **anim_loader.cpp** (~15 мест): pointer relocation для старого формата анимаций, переменные `a_off`/`ae_off`/`af_off` → `uintptr_t`
  - **memory.cpp** (~30 мест): convert_drawtype_to_index/pointer, convert_keyframe/fightcol/animlist_to_pointer,
    convert_thing_to_pointer (все Genus.* касты для ~15 классов), PLAYCUTS pointer→offset, PYRO thing/victim
  - **person.cpp** (1 место): ASSERT каст → `uintptr_t`
  - **aeng.cpp** (2 места): pointer alignment `& 0xffffffe0` → `& ~(uintptr_t)0x1f`
  - **pyro.cpp** (1 место): address-as-PRNG-seed → двойной каст `(ULONG)(uintptr_t)`
  - **widget.h**: `SLONG data[5]` → `intptr_t data[5]` (массив хранит и числа, и указатели)
  - **widget.cpp** (7 мест): все `(SLONG)item->next/prev/item2` → `(intptr_t)`

#### Шаг 4. Структуры с указателями в файловом I/O ✅

**Суть проблемы:** структуры читаются из файлов через `fread(&s, sizeof(s), ...)`.
На x64 указатели вырастают с 4 до 8 байт → `sizeof(struct)` меняется →
`fread` читает неправильное количество байт → все поля после указателя смещаются.

**Ключевая находка:** во ВСЕХ случаях значения указателей из файла **не используются как реальные адреса** —
они либо перезаписываются сразу после чтения, либо содержат индексы/офсеты которые конвертируются в указатели.
Проблема не в "порче данных", а чисто в sizeof mismatch при чтении.

**Два типа файлов с разным подходом:**
- **Игровые ассеты** (.iam, .ucm) — формат фиксирован оригинальной 32-бит игрой → НУЖНО фиксить
- **Save файлы** — пишутся и читаются нашим кодом → sizeof изменится синхронно для write/read,
  старые 32-бит сейвы станут несовместимы (ожидаемо и приемлемо)

**Что сделано для ассетов (3 структуры):**
- **`MapThingPSX`** (mapthing.h): указатели `CurrentFrame`/`NextFrame` заменены на `uint32_t`
  (pointer-значения из файла никогда не используются — читаются только скалярные поля X,Y,Z).
  Добавлен `static_assert(sizeof == 82)`.
- **`GameKeyFrame`** (anim_types.h): добавлена `GameKeyFrame_Disk` структура (20 байт, указатели = `uint32_t`).
  В anim_loader.cpp: bulk FileRead читает `GameKeyFrame_Disk[]` во временный буфер,
  затем поэлементно копирует в runtime `GameKeyFrame[]` с кастом `(uintptr_t)`.
  Добавлен `static_assert(sizeof(GameKeyFrame_Disk) == 20)`.
- **`GameFightCol`** (anim_types.h): добавлена `GameFightCol_Disk` структура (16 байт, `Next` = `uint32_t`).
  В anim_loader.cpp: аналогичный bulk read + expand.
  Добавлен `static_assert(sizeof(GameFightCol_Disk) == 16)`.
- **AnimList** (массив `GameKeyFrame*`): файл хранит 32-бит значения → читается как `uint32_t[]`,
  expand в `GameKeyFrame*[]` с кастом.

**Save файлы (6 структур) — изменения не требуются:**
- `NIGHT_Square`, `NIGHT_Dfcache` — sizeof изменится, но write/read парны
- `CPChannel` — sizeof изменится, но write/read парны
- `IMP_Mesh`, `IMP_Mat` — sizeof изменится, но write/read парны
- `Thing` — sizeof изменится, но convert_to_index/pointer работает с `intptr_t`

#### Шаг 5. Прочие sizeof-зависимости ✅
- `game_tick.cpp:2197` — `sizeof(names) >> 2` → `sizeof(names)/sizeof(names[0])`
  (`>> 2` = деление на 4, предполагало sizeof(pointer)==4; на x64 pointer=8 → неверный count)

#### Шаг 6. Тулчейн x64 ✅
- **Новый toolchain** `cmake/clang-x64-windows.cmake`:
  - `--target=x86_64-pc-windows-msvc`, `VCPKG_TARGET_TRIPLET "x64-windows"`, `CMAKE_SYSTEM_PROCESSOR x86_64`
  - Старый `clang-x86-windows.cmake` сохранён (может пригодиться для DX6 бэкенда)
- **Makefile:** `TOOLCHAIN` → `clang-x64-windows.cmake`
- **CMakeLists.txt:**
  - `/SAFESEH:NO` — обёрнут в `if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")` (x86-only)
  - `/ENTRY:mainCRTStartup` — оставлен (нужен и на x64 для SUBSYSTEM:WINDOWS + main())
- **vcpkg:** x64-windows пакеты установились автоматически
- **Linker fix:** `SetLastClumpfile(char*, unsigned int)` → `size_t` в OpenGL и stub бэкендах
  (на x86 `size_t`=`unsigned int`=4, на x64 `size_t`=8 → mismatch с объявлением в texture.cpp)
- **Результат:** Release и Debug — оба компилируются и линкуются на x64
- **macOS (будущее):** добавить toolchain для `arm64-osx` (Apple Silicon) — отдельный шаг

#### Шаг 7. Верификация 🔧 В РАБОТЕ

**Сборка:** ✅ Release + Debug компилируются и линкуются на x64.

**Runtime фиксы (обнаружены при верификации):**
- **file_clump.cpp** — `size_t` в fread/fwrite: clump-файлы (.txc/.gob) хранят offsets/lengths как 32-бит,
  а `size_t` на x64 = 8 байт → fread читал вдвое больше данных → мусорные индексы → abort.
  Фикс: читаем как `uint32_t[]`, расширяем в `size_t[]`. Аналогично для write.
- **playcuts.cpp** — `FileRead(handle, channel, sizeof(CPChannel))`: CPChannel содержит указатель,
  sizeof на x64 = 16 вместо 12 → чтение лишних 4 байт из файла → десинхронизация потока → SIGSEGV.
  Фикс: читаем через фиксированную 12-байтную on-disk структуру, копируем поля.
- **SetLastClumpfile** — `unsigned int` vs `size_t` mismatch между объявлением (texture.cpp) и
  определением (OpenGL/stub backends). На x86 оба = 4 байта, на x64 size_t = 8 → linker error.
  Фикс: `unsigned int` → `size_t` в обоих бэкендах.

**Runtime фиксы (обнаружены при верификации, продолжение):**
- **farfacet.cpp, fastprim.cpp, superfacet.cpp, aeng.cpp** (8 мест) — pointer alignment через
  `SLONG(ptr_buffer) + 31) & ~0x1f` → `uintptr_t`. `SLONG` = 32-бит, обрезает адрес на x64 → SIGSEGV.
  Аналогичный фикс что в aeng.cpp шаг 3, но эти места были пропущены (другой синтаксис каста — 
  `SLONG(var)` вместо `(SLONG)var`).
- **memory.cpp:406** — `(SLONG)(EWAY_mess[c0] - EWAY_mess_buffer)` → `(intptr_t)` (pointer diff в 32 бит)

**Полный sweep pointer truncation ✅** — систематический grep всех паттернов каста указателя в 32-бит:
`(SLONG)ptr`, `(ULONG)ptr`, `SLONG(ptr)`, `ULONG(ptr)`, `(int)ptr`, `(unsigned)ptr`,
`(uint32_t)ptr`, `(int32_t)ptr`. Найдено и исправлено 11 мест в 6 файлах:
- **widget.h/cpp** — `WIDGET_Data` callback: `SLONG data1/data2` → `intptr_t` (typedef + 3 реализации
  `INPUT_Data`/`LISTS_Data`/`TEXTS_Data` + 4 call-site каста `(SLONG)ptr` → `(intptr_t)`)
- **widget.cpp:329** — `(SLONG)MemAlloc(sz)` → `(intptr_t)` (указатель обрезался до 32 бит)
- **playcuts.cpp** — `pos.X` хранил `(SLONG)ptr` (текст катсцен). Редизайн: хранит offset+1 от
  `PLAYCUTS_text_data` (0 = sentinel "нет текста"). При чтении: `PLAYCUTS_text_data + (pos.X - 1)`.
  Убрана ptr↔offset конверсия в memory.cpp save/load (pos.X уже offset, не указатель).
- **memory.cpp:898-903** — `(SLONG)NET_PERSON/NET_PLAYER/EWAY_mess` → `(intptr_t)` (index-in-pointer)
- **pyro.cpp:885,1468** — `(SLONG)pyro` → `(SLONG)(uintptr_t)` (PRNG seed, double cast)
- **vehicle.cpp:815-816** — `SLONG(p_car)` → `(SLONG)(uintptr_t)` (siren animation offset, double cast)

**Runtime фиксы (sentinel и sizeof в аним-системе):**
- **memory.cpp `convert_*_to_pointer`** — sentinel -1 (0xFFFFFFFF) не ловился на x64: `(intptr_t)` 
  zero-extended значения — `0x00000000FFFFFFFF` = положительное → проверка `< 0` не срабатывала →
  индексация по 4 млрд → SIGSEGV. Фикс: `(int32_t)(uintptr_t)` для sign-extend перед проверкой.
  Затронуты: `convert_keyframe_to_pointer`, `convert_animlist_to_pointer`, `convert_fightcol_to_pointer`.
- **anim_loader.cpp `load_append_game_chunk`** — все FileRead'ы использовали runtime sizeof вместо 
  on-disk sizeof: GameKeyFrame (36 vs 20), GameKeyFrame* (8 vs 4), GameFightCol (20 vs 16).
  Фикс: аналогично `load_game_chunk` — `_Disk` temp буфер + поэлементное копирование.
- **anim_loader.cpp old-format relocation** — byte-offset relocation `a += (new_base - old_base)` 
  сломана когда sizeof элемента изменился. Переписана на index-based: 
  `(old_ptr - old_base) / sizeof(_Disk)` → `&new_array[index]`.
- **figure_globals.cpp** (7 мест) — pointer alignment `(DWORD)ptr & ~mask` → `(uintptr_t)`.
- **figure.cpp:1593** — `(DWORD)pcBlock + 31) & ~31` → `(uintptr_t)`.

**Crash handler:**
- Восстановлен полноценный Windows crash handler: `crash_handler_win.cpp` (отдельный TU из-за 
  конфликта windows.h с types.h). Пишет Exception, RVA, адрес, регистры, стек с именами функций.
  На других платформах — fallback через `signal()`.

**⚠️ Отладка всегда в Debug-билде** (`make d`). Release — только для финальной проверки.

**stderr автоматически пишется в файл** при каждом запуске через `make d` / `make r`:
- `new_game/build/Debug/stderr.log` / `new_game/build/Release/stderr.log`
- Файл очищается при каждом запуске (rm -f перед стартом)
- Для чтения отладочного вывода после запуска — `Read` на этот файл
- Для целевого debug-логирования в коде — скилл `debug-log` (пишет в `debug_TOPIC.txt` рядом с exe)

**AddressSanitizer (ASAN)** — встроенный в clang инструмент поиска memory-багов:
- Ловит buffer overflow, use-after-free, stack corruption, heap corruption
- Включение: добавить в CMakeLists.txt `$<$<CONFIG:Debug>:-fsanitize=address>` в compile + link options
- Замедляет ~2x, но автоматически печатает стек вызовов при ошибке в stderr
- **Нюанс:** prebuilt библиотеки (GamepadCore.lib и т.п.) могут конфликтовать с ASAN-аннотациями
  (string/vector). Фикс: `-D_DISABLE_STRING_ANNOTATION=1 -D_DISABLE_VECTOR_ANNOTATION=1`
- **⚠️ Windows Debug CRT (ucrtbased.dll) несовместим с ASAN** — CRT делает internal free которые
  ASAN считает bad-free, крашит до main(). Если нужен ASAN — собирать Release с ASAN, не Debug.
- Полезен для поиска скрытых memory corruption багов которые проявляются по-разному на x86/x64

**Текущий статус:**
- ✅ Запуск → главное меню работает
- ✅ Загрузка уровня → проходит
- ✅ Синематик начинает проигрываться
- ✅ Краш через несколько секунд синематика — heap corruption: `HEAP_QUANTISE` (16) < `sizeof(HEAP_Free)` (32 на x64)
  → overflow при записи free-list узла в слишком маленький остаток → порча `NIGHT_Square.colour`.
  Фикс: `HEAP_QUANTISE` = `max(sizeof(HEAP_Free), 16)`.
- ✅ Синематик проходит, геймплей работает
- ✅ Визуальный баг: пропадающие стены/окна/двери/ящики при приближении — **CRINKLE_read_bin sizeof bug**.
  `CRINKLE_Crinkle` содержит указатели (`point`, `face`), sizeof = 16 на x86, 24 на x64.
  `memcpy(cc, bptr, sizeof(*cc))` + `bptr += sizeof(*cc)` в CRINKLE_read_bin читала 24 байта из
  16-байтной записи → сдвиг на 8 байт → мусорные данные crinkle mesh → при приближении (z < 0.3)
  CRINKLE_do рисовала сломанный mesh вместо обычного POLY_add_quad.
  Фикс: читать num_points/num_faces по фиксированным offset'ам, bptr += 16 (on-disk size).
  Тот же паттерн что GameKeyFrame/GameFightCol/CPChannel — пропущен т.к. struct static внутри .cpp.
- ⏳ Outro: 3D модель справа не видна (возможно IMP_Mesh pointer issue)

**Полный аудит файлового I/O завершён — все sizeof(struct) с указателями в fread/fwrite найдены:**
- Ассеты (фиксированный формат): file_clump ✅, anim_loader ✅ (обе функции), playcuts ✅, mapthing ✅, crinkle ✅
- Save/load (самосогласованное): memory.cpp, save.cpp, outro_imp.cpp, thing.cpp — не требуют фикса
  (write и read используют одинаковый sizeof; старые 32-бит сейвы несовместимы — ожидаемо)

**Чеклист после починки краша:**
- Загрузка уровня без краша
- Геймплей 5 минут
- Анимации персонажей
- Катсцены
- Quick save / quick load

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
| 14a | Компилятор: clang-cl → clang++ | ✅ |
| 14b | Скрипты → кросс-платформенные | ✅ |
| 14c | CMake: кросс-платформенные условия | ✅ |
| 14d | Переход на 64-бит | ⏳ |
| 15 | ASan прогон: играть под ASan, фиксить memory bugs один за одним | ⏳ |
