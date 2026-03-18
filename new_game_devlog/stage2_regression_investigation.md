# Расследование рантайм-регрессии Этапа 2

**Дата расследования:** 2026-03-18
**Диапазон коммитов:** `3e699d6..edb9222` (18 коммитов)
**Статус:** ✅ расследование завершено, все баги исправлены (коммит `7215a28`)

---

## Резюме

Все баги найдены в **одном коммите: `4e7ef92` (stage 2: remove all preprocessor conditions pt3)**.

Этот коммит удалял вызовы `TRACE()`, `LogText()`, `DebugText()`, `ERROR_MSG()` из 66 файлов. Проблема: когда вызов debug-функции был **единственным statement в теле `if`, `else` или `else if`**, его удаление оставляло "dangling" управляющую конструкцию, которая захватывала следующий за ней statement — изменяя логику программы.

**Остальные 17 коммитов — чистые**, багов не обнаружено:
- `3e699d6`, `e6be11a` — удаление debug-кода через препроцессор (coan) — корректно
- `0c9ad12`, `29d624c` — preprocessor pt1, pt2 (coan) — корректно
- `85d3746`, `a6d9585` — preprocessor pt4, pt5 (TEX_EMBED, USE_TOMS_ENGINE) — корректно
- `afaa1ad`..`6dd8192` — preprocessor pt6-pt10 — корректно
- `c650f3c`..`86e8ecb` — поздние итерации (удаление мёртвых сущностей, PSX export, clang-format, пустые файлы) — корректно
- `16a20af`, `edb9222` — только записи в девлог

---

## Найденные баги (все из коммита `4e7ef92`)

### Баг 1 — Message.cpp: сообщения не копируются в буфер

**Файл:** `DDEngine/Source/Message.cpp`, строка 65
**Серьёзность:** средняя (система сообщений на экране)

**Было:**
```c
if (ControlFlag)
    LogText("MSG->%s\n", message);
/* ... комментарий ... */
strncpy(MSG_message[current_message].message, message, MSG_MAX_LENGTH - 1);
```

**Стало:**
```c
if (ControlFlag)
/* ... комментарий ... */
strncpy(MSG_message[current_message].message, message, MSG_MAX_LENGTH - 1);
```

**Проблема:** `strncpy` теперь выполняется только когда `ControlFlag == true`. Остальные поля (timer, current_message) обновляются безусловно. Результат: если `ControlFlag == false`, сообщения показываются с пустым или stale текстом.

**Фикс:** удалить `if (ControlFlag)` — он охранял только LogText.

---

### Баг 2 — build2.cpp: отключена проверка границ при добавлении фейсетов в карту

**Файл:** `Source/build2.cpp`, строка 198 (`link_facet_to_mapwho`)
**Серьёзность:** высокая (memory corruption, out-of-bounds доступ)

**Было:**
```c
if (mx == 13 && mz == 5)
    LogText(" add FACET %d to %d,%d\n", facet, mx, mz);

if (mx < 0 || mx > PAP_SIZE_LO || mz < 0 || mz > PAP_SIZE_LO) {
    return;
}
```

**Стало:**
```c
if (mx == 13 && mz == 5)

    if (mx < 0 || mx > PAP_SIZE_LO || mz < 0 || mz > PAP_SIZE_LO) {
        return;
    }
```

**Проблема:** проверка границ теперь выполняется ТОЛЬКО для тайла (13,5). Для всех остальных координат — нет проверки, возможен out-of-bounds доступ к массивам.

**Фикс:** удалить `if (mx == 13 && mz == 5)`.

---

### Баг 3 — build2.cpp: x1 не инициализируется в add_facet_to_map

**Файл:** `Source/build2.cpp`, строка 258 (`add_facet_to_map`)
**Серьёзность:** высокая (неинициализированная переменная)

**Было:**
```c
if (facet == 288 || facet == 289)
    LogText("break");

x1 = p_f->x[0] << 8;
x2 = p_f->x[1] << 8;
```

**Стало:**
```c
if (facet == 288 || facet == 289)

    x1 = p_f->x[0] << 8;
x2 = p_f->x[1] << 8;
```

**Проблема:** `x1` присваивается только для facet 288/289. Для всех остальных — `x1` не инициализирован, содержит мусор. `x2`, `z1`, `z2` инициализируются всегда.

**Фикс:** удалить `if (facet == 288 || facet == 289)`.

---

### Баг 4 — Building.cpp: не инициализируются wall и px в find_wall_for_fe

**Файл:** `Source/Building.cpp`, строки 7111-7115 (`find_wall_for_fe`)
**Серьёзность:** высокая (неинициализированные переменные, некорректная логика поиска стен)

**Было:**
```c
if (storey == 0)
    LogText(" error \n");

wall = storey_list[storey].WallHead;
if (wall == 0)
    LogText(" error \n");
px = storey_list[storey].DX;
pz = storey_list[storey].DZ;
```

**Стало:**
```c
if (storey == 0)

    wall = storey_list[storey].WallHead;
if (wall == 0)
    px = storey_list[storey].DX;
pz = storey_list[storey].DZ;
```

**Проблема (тройная):**
1. `wall` присваивается только когда `storey == 0`
2. `px` присваивается только когда `wall == 0`
3. `pz` безусловный (на отдельном уровне отступа)

Для большинства случаев `wall` и `px` не инициализированы.

**Фикс:** удалить оба `if` (они охраняли только LogText).

---

### Баг 5 — Building.cpp: функция find_wall_for_fe не возвращает значение

**Файл:** `Source/Building.cpp`, строка 7132 (`find_wall_for_fe`)
**Серьёзность:** критическая (undefined behavior)

**Было:**
```c
if (best_wall == -1)
    LogText(" best wall=-1\n");
return (best_wall);
```

**Стало:**
```c
if (best_wall == -1)
    return (best_wall);
```

**Проблема:** `return` выполняется только когда `best_wall == -1`. В остальных случаях функция ничего не возвращает → undefined behavior (MSVC может вернуть мусор в eax).

**Фикс:** удалить `if (best_wall == -1)`.

---

### Баг 6 — Building.cpp: process_external_pieces только для building 3

**Файл:** `Source/Building.cpp`, строка 8852 (`create_building_prim`)
**Серьёзность:** критическая (все здания кроме building 3 не имеют внешних элементов)

**Было:**
```c
if (building == 3)
    LogText(" build external bits\n");

process_external_pieces(building);
```

**Стало:**
```c
if (building == 3)

    process_external_pieces(building);
```

**Проблема:** `process_external_pieces()` вызывается только для building 3. Все остальные здания не получают внешние элементы (заборы, кабели, и т.д.).

**Фикс:** удалить `if (building == 3)`.

---

### Баг 7 — Building.cpp: лестницы только для building 3

**Файл:** `Source/Building.cpp`, строка 9106 (`create_building_prim`)
**Серьёзность:** критическая (лестницы не строятся для большинства зданий)

**Было:**
```c
case STOREY_TYPE_LADDER:
    if (building == 3)
        LogText(" building 3   build ladder\n");
    build_ladder(storey);
    break;
```

**Стало:**
```c
case STOREY_TYPE_LADDER:
    if (building == 3)
        build_ladder(storey);
    break;
```

**Проблема:** `build_ladder` вызывается только для building 3.

**Фикс:** удалить `if (building == 3)`.

---

### Баг 8 — Building.cpp: скайлайты только для building 3

**Файл:** `Source/Building.cpp`, строка 9110 (`create_building_prim`)
**Серьёзность:** критическая

**Аналогично багу 7** — `build_skylight(storey)` только для building 3.

**Фикс:** удалить `if (building == 3)`.

---

### Баг 9 — Building.cpp: финальный build_facet крыши только для building 3

**Файл:** `Source/Building.cpp`, строка 9164 (`create_building_prim`)
**Серьёзность:** критическая (крыши не достраиваются)

**Было:**
```c
if (valid) {
    if (building == 3)
        LogText(" building 3   build last facet\n");
    prev_facet = build_facet(..., FACET_FLAG_ROOF, 0);
}
```

**Стало:**
```c
if (valid) {
    if (building == 3)
        prev_facet = build_facet(..., FACET_FLAG_ROOF, 0);
}
```

**Проблема:** финальный фейсет крыши строится только для building 3.

**Фикс:** удалить `if (building == 3)`.

---

### Баг 10 — Building.cpp: бесконечный цикл в build_roof_grid

**Файл:** `Source/Building.cpp`, строка 3624 (`build_roof_grid`)
**Серьёзность:** критическая (бесконечный цикл → зависание)

**Было:**
```c
while (s) {
    if (do_storeys_overlap(s, storey) && ...) {
        build_more_edge_list(min_z, max_z, s, 0);
    } else
        LogText(" failed storey ...");

    s = storey_list[s].Next;
}
```

**Стало:**
```c
while (s) {
    if (do_storeys_overlap(s, storey) && ...) {
        build_more_edge_list(min_z, max_z, s, 0);
    } else

        s = storey_list[s].Next;
}
```

**Проблема:** `s = storey_list[s].Next` стало телом `else`. Когда `do_storeys_overlap` возвращает true → s не обновляется → бесконечный цикл.

**Фикс:** убрать `else`, сделать `s = storey_list[s].Next` безусловным (вне if/else).

---

### Баги 11a, 11b, 11c — Building.cpp: break в else → fallthrough в switch

**Файл:** `Source/Building.cpp`, строки 3811, 3850, 3941 (`build_roof_grid`)
**Серьёзность:** высокая (fallthrough между case-ами в switch)

**Было (для всех трёх):**
```c
                    } else
                        LogText(" pooerrorN\n");

                    break;
```

**Стало:**
```c
                    } else
                        break;
```

**Проблема:** `break` раньше был безусловным (после if/else). Теперь `break` выполняется только в else-ветке. Если треугольник был успешно создан (if/else if попал) — нет break → fallthrough в следующий case.

**Затронутые case-ы:**
- 11a (строка 3811): `case (TL + TR)` → fallthrough в `case (BL + BR)`
- 11b (строка 3850): `case (BL + BR)` → fallthrough в `case (TL + BL)`
- 11c (строка 3941): `case (TL + BL)` → fallthrough в `case (TR + BR + BL)`

**Фикс:** для каждого — удалить `else`, оставить `break` безусловным.

---

## Также обнаружено в комментированном блоке (не баги)

В Building.cpp, строки 9089-9098 внутри `if (0) { /* ... */ }` — аналогичные `if(building==3)` захватывающие `build_roof()`, `build_roof_quad()`, `build_ladder()`. Это мёртвый код (внутри `if(0)` + `/* */`), не влияет на работу.

---

## Рекомендации по фиксу

Все баги имеют один паттерн и простой фикс: удалить оставшийся `if`/`else` который раньше охранял только debug-вызов.

**Порядок:**
1. Сначала баг 10 (бесконечный цикл) — самый деструктивный
2. Затем баги 6-9 (здания ломаются)
3. Затем баги 2-5 (инициализация, границы)
4. Затем баги 11a-c (fallthrough)
5. Наконец баг 1 (Message.cpp)

**Всего 14 точек правки в 3 файлах:**
- `DDEngine/Source/Message.cpp` — 1 правка
- `Source/build2.cpp` — 2 правки
- `Source/Building.cpp` — 11 правок

---

## Методология расследования

1. Получен диапазон подозрительных коммитов из девлога (`3e699d6..HEAD`)
2. Параллельный анализ 4-мя агентами по группам коммитов
3. Верификация каждой находки — чтение текущего состояния файлов
4. Полный ручной просмотр диффа коммита pt3 (самого большого — 66 файлов)
5. Автоматический поиск паттерна "dangling if/else" по всей кодобазе
6. Сверка с оригиналом для каждого подозрительного места
