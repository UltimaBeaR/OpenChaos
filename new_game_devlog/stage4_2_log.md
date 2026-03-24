# Этап 4.2 — Доработка структуры (план + лог)

Правила: [`stage4.md`](../new_game_planning/stage4.md) → секция 4.2

## Инструменты

- `tools/st_4_2_dep_graph.py` — генерация и запросы DAG зависимостей
- `new_game_devlog/st_4_2_dep_graph.json` — граф: 658 файлов, 2963 рёбер, 2 цикла

## Связанные документы

- [stage4_2_files.md](stage4_2_files.md) — описания всех файлов (что в каждом)
- [stage4_2_moves.md](stage4_2_moves.md) — таблица перемещений (текущий путь → новый путь)

## План

### Шаг 1 — Анализ и планирование

1. ~~Сгенерировать DAG зависимостей~~ ✅
2. ~~Составить таблицу описаний файлов~~ ✅
3. Обсудить перемещения на основе DAG + описаний
4. Заполнить таблицу соответствий (текущий путь → новый путь)

### Шаг 2 — Перемещения файлов

По согласованному плану. Чеклист на каждое перемещение:

1. `git mv` файлов в новое место
2. Обновить `#include` пути во **всех** потребителях (grep → replace)
3. Обновить `CMakeLists.txt` — пути к .cpp файлам + `target_include_directories`
4. Обновить include guards в перемещённых .h файлах (`PATH_FROM_SRC_UPPERCASED_H`)
5. Обновить includes **внутри** самих перемещённых файлов (они тоже могут ссылаться на другие перемещённые)
6. `make build-release` — должен пройти
7. `python tools/st_4_2_dep_graph.py generate` — регенерировать граф
8. Обновить таблицу в [stage4_2_moves.md](stage4_2_moves.md)

### Шаг 3 — Внутрифайловая реструктуризация

Разбить/объединить файлы где оправдано.

### Шаг 4 — Аудит правил Этапа 4

Глобалки → `_globals`, `uc_orig`, DAG, include guards.

---

## Статистика DAG (после перемещений итерации 1)

```
Файлов: 658 (345 .h, 313 .cpp)
Рёбер: 2963 (среднее 4.5 на файл)

По слоям:
  engine/:   256  (было 226 + core 29 + platform 1)
  ui/:        94
  actors/:    93
  world/:     87
  effects/:   46
  assets/:    44
  missions/:  24
  ai/:        13
  (root):      1

Циклы (2):
  compression.h ↔ compression_globals.h
  ngamut.h ↔ ngamut_globals.h
```

## Известные проблемы

- `engine/platform/platform.h` — мега-umbrella (145 rdeps), тянет core + io + input

---

## Лог

### Итерация 1 — core/, platform/, wind_procs (2026-03-24)

Принцип: engine = самодостаточный движок-комбайн (а-ля Unity), всё базовое — внутри.

- `core/*` (29 файлов) → `engine/core/*` — 332 include-пути в 272 файлах
- `platform/platform.h` → `engine/platform/platform.h` — 145 include-путей
  - Было `#include <platform.h>` (angle brackets через include root) → стало `#include "engine/platform/platform.h"`
- `wind_procs.*` (4 файла) из `engine/graphics/graphics_api/` → `engine/platform/` — WndProc это platform, не graphics
- CMakeLists.txt: source paths + include_directories (`src/platform` → `src/engine/platform`)
- Include guards обновлены во всех .h

### Итерация 2 — actors/ → things/ (2026-03-24)

- `actors/*` (93 файла) → `things/*` — переименование папки
- Thing — оригинальное название системы в коде, "actors" подразумевает только активных агентов
- 117 файлов с include-путями обновлены, 49 include guards ACTORS_ → THINGS_

### Итерация 3 — platform.h → uc_common.h (2026-03-24)

- `engine/platform/platform.h` → `engine/platform/uc_common.h`
- Название "platform" создавало ложное впечатление платформенной абстракции, на деле это легаси umbrella-заголовок (бывший MFStdLib.h) — тянет Windows/D3D, типы, math, memory, io, input
- Разбиение на отдельные заголовки → этап 9
- 145 include-путей обновлены

### Итерация 4 — lighting/ → graphics/lighting/ (2026-03-24)

- `engine/lighting/*` (22 файла) → `engine/graphics/lighting/*`
- Освещение — чисто визуальная подсистема (view frustum, shadows, per-vertex lighting), не влияет на геймплей
- 41 файл с include-путями, 13 include guards обновлены
