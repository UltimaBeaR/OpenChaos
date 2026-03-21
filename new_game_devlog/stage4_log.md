# Лог Этапа 4 — Реструктуризация кодовой базы

## Итерация 0 — Подготовка (2026-03-20)

**Создано:**
- `tools/entity_map.py` + `new_game_planning/entity_mapping.json`
- `src/old/` — старый код перемещён: `src/fallen/` → `src/old/fallen/`, `src/MFStdLib/` → `src/old/MFStdLib/`
- `src/new/` — создана целевая иерархия папок (пустая)
- CMakeLists.txt обновлён: пути источников, include directories (добавлены `src/new/` и `src/old/`)

**Компиляция:** Release ✅, Debug ✅

**Анализ DAG (автоматический):**
- `MFStdLib` (StdMem, StdMaths и др.) — чистый core, нет зависимостей на проект ✓
- `Game.h` — мега-хаб: 26 инклудов, включается в 22+ файлах. Это **не блокер** — `new/` файлы пишутся чистыми, без Game.h
- Нарушения слоёв в `old/`: `MFx.h` (аудио) включает `Thing.h`; `poly.cpp` (graphics) включает `Game.h`/`night.h`/`eway.h` — остаются в `old/` как есть, в `new/` будут чистые include-ы
- Порядок миграции из stage4_rules.md **корректен** — начинаем с core/ (математика, память)

---

## Итерация 1 — core/types + core/macros + core/memory (2026-03-20)

- `memory.h` включает `<windef.h>` для BOOL — временная Windows-зависимость до Этапа 8
- No-op ASSERT вызовы из memory.cpp убраны

---

## Итерация 2 — core/math (2026-03-20)

- `math.h` включает `<cmath>` и `<cstdlib>` (для sqrt в Root() и abs в Hypotenuse)

---

## Итерация 3 — engine/io/file (2026-03-20)

- `TraceText` оставлен в `old/StdFile.cpp` — относится к Host section, не к файловому I/O
- `TraceText` нигде не вызывается (мёртвый код), удалится при миграции Host section
- `file.h` включает `<windows.h>` напрямую для HANDLE/BOOL — self-contained до Этапа 8

---

## Итерация 4 — engine/input/keyboard + engine/input/mouse (2026-03-20)

Штатно.

---

## Итерация 5 — engine/io/drive + engine/io/async_file (2026-03-20)

- `drive.h` — только декларации; `Drive.cpp` остаётся в `old/` (зависит от `env.h`)
- `AsyncFile2.cpp` полностью перенесён в `async_file.cpp`, удалён из CMakeLists
- `AsyncFile.h` (старая версия) — редирект на `async_file.h`, как и `AsyncFile2.h`

---

## Итерация 6 — engine/input/keyboard.cpp + engine/input/mouse.cpp (2026-03-20)

- `mouse.cpp` временно включает `DDlib.h` для `hDDLibWindow` (GDisplay ещё не мигрирован)
- `MouseDX`/`MouseDY`/`RecenterMouse` перенесены из `DDlib.h` в `mouse.h`
- ASSERT вызов в `ClearLatchedKeys` убран (no-op макрос)

---

## Фикс — вынос глобалов в `_globals` файлы (2026-03-20)

Правило было в stage4_rules.md с самого начала, но игнорировалось итерации 1–6.
Созданы: `memory_globals`, `math_globals`, `keyboard_globals`, `mouse_globals` (.h + .cpp).
Entity mapping обновлён (33 записи — file path).
Чеклист самопроверки дополнен пунктом B (проверка `_globals`).

---

## Итерация 7 — engine/input/joystick (DIManager) (2026-03-20)

- `ClearPrimaryDevice` не перенесён — мёртвая декларация, нигде не вызывается
- `DIRECTINPUT_VERSION 0x0700` нужен перед `<dinput.h>` — добавлен в joystick_globals.h и joystick.cpp
- MOUSE/KEYBOARD/JOYSTICK макросы убраны из MFStdLib.h (дубликат, теперь в joystick.h через DDlib.h → DIManager.h redirect)

---

## Итерация 8 — core/fixed_math + core/matrix + core/fmatrix (2026-03-20)

- `fmatrix.cpp` временно включает `fallen/Headers/prim.h` — нужны struct Matrix33/CMatrix33/Matrix31/SMatrix31 + CMAT masks
- `MATRIX_find_angles_old` перенесён (неиспользуемая старая версия, но код 1:1)
- `MATRIX_calc_int` перенесён — не объявлен в хедере, но определён в оригинале
- ASSERT вызовы в matrix_transform/matrix_transform_small убраны (no-op)
- `ray.h` (DDEngine) — мёртвый файл (нет реализации, нигде не вызывается), не переносить

---

## Итерация 10 — core/fmatrix типы + core/vector + maths.cpp (2026-03-20)

- Matrix33/CMatrix33/Matrix31/SMatrix31 + SMAT/CMAT макросы перенесены из prim.h в fmatrix.h
- prim.h теперь включает core/fmatrix.h вместо дублирующих определений
- Structs.h теперь включает core/vector.h (SVector/SmallSVector/SVECTOR/GameCoord/TinyXZ)
- matrix_mult33 и rotate_obj перенесены из maths.cpp в core/fmatrix.cpp
- MATHS_seg_intersect перенесён из maths.cpp в core/math.cpp
- maths.cpp удалён (опустел), убран из CMakeLists
- Временные `#include "fallen/Headers/prim.h"` удалены из fmatrix.cpp и quaternion.cpp

---

## Итерация 9 — assets/file_clump + assets/tga + core/quaternion (2026-03-20)

- `tga.h` потребовал `<windows.h>` для `BOOL` в сигнатуре `TGA_load`
- `quaternion.cpp` агент использовал `<windef.h>` без `_WIN32` — заменено на `<windows.h>`
- `quaternion.cpp` временно включает `fallen/Headers/prim.h` (Matrix33/CMatrix33 + CMAT masks)
- tga_width/tga_height/TGA_header — non-static глобалы → вынесены в `assets/tga_globals.h/.cpp`
- Первая итерация с использованием subagent'ов (tga + quaternion параллельно)

---

## Итерация 11 — assets/image_compression + assets/compression + engine/lighting/ngamut + engine/graphics/graphics_api/work_screen (2026-03-20)

- `work_screen_globals.h` использовала `<DDLib.h>` → циклический include (`DDLib.h` → `GWorkScreen.h` → `work_screen.h` → `work_screen_globals.h`); исправлено заменой на `"core/types.h"` (MFRect и SLONG там определены)
- Все file-private глобалы в compression.cpp и image_compression.cpp помечены `static` (не экспортируются)
- `NGAMUT_add_square` — `static inline` в ngamut.cpp
- Баг `MAX3` в NGamut.cpp (несовпадающие скобки: `MAX(MAX(a,b,c))`) перенесён 1:1; MAX3 нигде не используется в реальном коде
- Фикс после итерации: `COMP_tga_data`, `COMP_tga_info`, `COMP_data`, `COMP_frame` (comp.cpp) и `test` (ic.cpp) были ошибочно помечены `static` — в оригинале они не были static; исправлено через вынос в `compression_globals` и `image_compression_globals`
- `COMP_data` — анонимный struct в оригинале; для `extern`-декларации в globals.h потребовалась именованная обёртка `COMP_DataBuffer` (определена в compression.h); `COMP_MAX_DATA` перенесён из compression.cpp в compression.h (нужен для типа)

---

## Итерация 12 — engine/graphics/geometry/aa + core/timer + assets/bink_client + engine/io/drive + engine/graphics/pipeline/message (2026-03-20)

- `Gamut.cpp` удалён как мёртвый код — `build_gamut_table`/`draw_gamut`/`gamut_ele_pool`/`gamut_ele_ptr` нигде не вызываются
- `timer.cpp`: агент изменил ULONG → LONGLONG в `stopwatch_start`/`GetFineTimerFreq`/`GetFineTimerValue` — откатано вручную до оригинального поведения с усечением до 32 бит (`.u.LowPart`)
- `BreakTimer.h` содержал no-op макросы (`BreakTime(X)` и др.) используемые в `aeng.cpp` — перенесены в `core/timer.h`
- `MSG_add` в старом `Message.h` был закомментирован, но функция реально используется. В новом `message.h` добавлена активная декларация с `char*` (= CBYTE*) — иначе линкер не находит символ.
- `drive.cpp` включает `env.h` через temporary include — `env.h` использует CBYTE/SLONG, поэтому перед ним добавлен `#include "core/types.h"`
- `bink_client.h` forward-декларирует `struct IDirectDrawSurface` — не включает `<ddraw.h>` чтобы не тянуть весь COM стек в header

---

## Итерация 13 — engine/audio/mfx (2026-03-20)

- `MFX.h` включал `thing.h` → `Game.h`, давая всем включателям неявный доступ к `the_game`, `MAX_THINGS` и т.д. После замены на redirect к `engine/audio/mfx.h` (который только forward-декларирует `struct Thing;`) сломались `GHost.cpp` и `music.cpp` — добавлены явные `#include "game.h"` в обоих файлах
- `alDevice` и `alContext` в оригинале не-static, но не объявлены ни в одном заголовке → помечены `static` в новом файле как file-private
- `SetLinearScale` и `SetPower` объявлены без `static` в оригинале, но нигде не используются снаружи → помечены `static` в новом файле

---

## Итерация 14 — engine/graphics/resources/d3d_texture + engine/graphics/graphics_api/wind_procs + host (2026-03-21)

- `wind_procs.h` временно включает `DDManager.h` (через `<windows.h>/<ddraw.h>/<d3d.h>` prefix) — `ChangeDDInfo` использует `DDDriverInfo/D3DDeviceInfo/DDModeInfo`
- `d3d_texture.cpp` требовал `#include <MFStdLib.h>` для `ASSERT/MFnew/MFdelete` (оригинал шёл через `DDLib.h`)
- `wind_procs.cpp` требовал явного `#include "assets/bink_client.h"` для `BinkMessage` (оригинал шёл через `DDLib.h`)
- `host_globals.h` — `UWORD` тянется через Windows SDK (winsmcrd.h → typedef WORD UWORD); добавлен `core/types.h` для явности
- `quaternion_globals.h` — предыдущий баг: `BOOL` не было определено при компиляции в изоляции; исправлено добавлением `<windows.h>`
- `drive.cpp` — предыдущий баг: `core/types.h` не был добавлен перед `env.h` (хотя итерация 12 планировала это); исправлено
- `GHost.cpp`, `WindProcs.cpp`, `D3DTexture.cpp` — удалены из CMakeLists, старые хедеры стали редиректами

---

## Итерация 15 — engine/graphics/pipeline/render_state + bucket + vertex_buffer (2026-03-21)

- `vertex_buffer.h` потребовал `<MFStdLib.h>` (для `ASSERT` в inline `GetPtr()`) и `<stdio.h>` (для `FILE*` в `DumpInfo`) — без них build падал
- `vertex_buffer.cpp` включал `console.h` и `poly.h` в оригинале — оба не используются в коде файла; в новом не включены
- `RenderState::s_State` — статический член класса, определён в render_state.cpp (не в _globals) — это корректно, static member обязан определяться в .cpp класса
- `vertex_pool[]` в bucket.h — extern объявление (определение в aeng.cpp, ещё не перенесён)

---

## Итерация 16 — engine/graphics/pipeline/polypoint + polypage (2026-03-21)

- `polypage.cpp` требовал `core/matrix.h` для `MATRIX_MUL` (в `GenerateMMMatrixFromStandardD3DOnes`) — в оригинале шёл через `matrix.h`
- Инлайн `DRAWN_PP` макрос (`#define DRAWN_PP ppDrawn`) не перенесён — это был локальный алиас переменной `ppDrawn` в той же функции, замена прямая
- `FanAlloc` и `SetTexOffset(UBYTE)` — dead declarations без реализации, не перенесены
- `ppLastPolyPageSetup` — объявлен в оригинале но нигде не используется, не перенесён
- `PolyPage::s_AlphaSort` и другие статические члены — определены в `polypage.cpp` (не в `_globals`), аналогично прецеденту `RenderState::s_State` из итерации 15
- `polypage.h` включает `poly.h` (temporary) — `POLY_Point` ещё не мигрирован

---

## Итерация 17 — engine/graphics/resources/font (2026-03-21)

- `font.cpp` включает `<MFStdLib.h>` перед `GDisplay.h` — MFStdLib тянет `<windows.h>/<ddraw.h>/<d3d.h>`, без этого DDManager.h не компилируется
- Bit-pattern макросы (`_____` … `xxxxx`) вынесены в `font_globals.cpp` вместе с данными шрифта — они используются только для инициализации таблиц

---
