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
