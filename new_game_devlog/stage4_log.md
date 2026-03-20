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
