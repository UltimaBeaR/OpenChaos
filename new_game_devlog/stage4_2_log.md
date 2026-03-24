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

По согласованному плану: двигать файлы, обновлять includes, CMakeLists.txt.

### Шаг 3 — Внутрифайловая реструктуризация

Разбить/объединить файлы где оправдано.

### Шаг 4 — Аудит правил Этапа 4

Глобалки → `_globals`, `uc_orig`, DAG, include guards.

---

## Статистика DAG

```
Файлов: 658 (345 .h, 313 .cpp)
Рёбер: 2963 (среднее 4.5 на файл)

По слоям:
  engine/:   226
  ui/:        94
  actors/:    93
  world/:     87
  effects/:   46
  assets/:    44
  core/:      29
  missions/:  24
  ai/:        13
  platform/:   1
  (root):      1

Циклы (2):
  compression.h ↔ compression_globals.h
  ngamut.h ↔ ngamut_globals.h

Самые зависимые (top 5):
  208 ← core/types.h
  145 ← platform/platform.h
  100 ← missions/game_types.h
   64 ← engine/graphics/pipeline/poly.h
   48 ← engine/graphics/pipeline/aeng.h
```

## Известные проблемы

- `engine/graphics/graphics_api/wind_procs.h/.cpp` — Win32 WndProc, не graphics API → `platform/`
- `platform/platform.h` — мега-umbrella (145 rdeps), тянет core + engine + io

---

## Лог

(пока пусто — записи будут по мере работы)
