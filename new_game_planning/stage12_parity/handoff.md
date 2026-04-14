# Stage 12 parity — handoff для новой сессии

Этот документ — **execution-ready инструкция** для свежей сессии Claude с пустым контекстом.
Пользователь делает `/clear` → новая сессия → читает этот файл → выполняет задачи.

## Контекст (минимум чтобы понять что происходит)

1. **Проект:** OpenChaos — модернизация Urban Chaos 1999. Подробно → [../../CLAUDE.md](../../CLAUDE.md).
2. **Текущий этап:** Stage 12 — релиз 1.0. Критерии → [../stage12.md](../stage12.md).
3. **Stage 12 parity workstream:** разбор ~405 находок из `original_game_compare/docs/release_changes/`, цель — применить релизные фиксы к нашей pre-release базе где они дают реальную ценность.
4. **Главный план с полным контекстом и всеми решениями:** [plan.md](plan.md). ОБЯЗАТЕЛЬНО прочитать целиком перед работой — там критичная часть про пересмотр стратегии (`new/` оказался post-release dev branch, а не retail).
5. **ID-индекс всех находок с per-item решениями:** [findings.md](findings.md). Смотреть когда нужны детали по конкретному ID.

## Ключевые факты которые влияют на стратегию

- **Ветка `original_game_compare/new/` — НЕ retail Steam PC.** Это post-release dev branch MuckyFoot, никогда не шипнулась. Подтверждено визуально через retail бинарь: crumple deformation, mesh fog, cloud shadows, floating damage text — ничего этого в retail нет, хотя в `new/` код добавлен. Значит `new/` содержит WIP-фичи которые не дошли до финала.
- **Наша `new_game/` база ≈ pre-release + Stage 1-11 работа** — она ближе к retail чем `new/`, потому что retail финализировался из pre-release ветки.
- **Default стратегия:** НЕ портировать из `new/` массово. Портировать только явные code bugs (typos, missing returns, buffer overflows, null derefs, memory corruption, compile-breaking). Feature additions и balance changes — VERIFY в retail перед решением.
- **Retail исходников у нас нет вообще.** Арбитр спорных пунктов — запускаемый retail Steam PC бинарь (у пользователя есть).

## Что делать в текущей сессии

Пользователь явно попросил сделать **только** два шага. Остальное отложено.

### Шаг 1. Code audits (параллельно через subagents)

Запустить **2-3 параллельных Explore subagent'а** (не больше — лимиты пользователя). Каждый проверяет состояние нашей `new_game/` базы по конкретным пунктам.

**⚠️ КРИТИЧНО:** subagent'ам давать полный self-contained промпт. Они не видят историю беседы. Каждый агент должен:
- Знать что ищет (конкретные символы, файлы)
- Знать что такое `uc_orig:` комментарии и `entity_mapping.json` (для поиска файлов)
- Возвращать структурированный результат: "сделано / не сделано / сделано по-другому / потеряно"

**Список audit задач (11 пунктов):**

1. **psystem state (PRT-07/08):** в `new_game/` найти `psystem.cpp` (grep `uc_orig:.*psystem.cpp`). Проверить функцию `PARTICLE_Run` или аналог — как реализованы `first_pass`, `prev_tick`, `local_ratio`, `tick_diff` reset. Pre-release версия не компилится (undeclared `first_pass`) — значит у нас есть фикс, нужно точно знать какой.

2. **FIRE_Flame struct (PRT-01):** в `new_game/` найти `fire.cpp` (grep `uc_orig:.*fire.cpp`). Проверить struct `FIRE_Flame` — заполнена полями (`dx, dz, die, counter, height, next, points, angle[], offset[]`) или пустой shell? Плюс `FIRE_add_flame` — инициализирует ли поля.

3. **farfacet / superfacet / fastprim (ADD-03/04/05):** grep на `FARFACET_`, `SUPERFACET_`, `FASTPRIM_` в `new_game/src/`. Для каждого — есть ли вызовы, или только декларации/мёртвый код. farfacet мы знаем что используем осознанно (коммит f14c9145). superfacet и fastprim — unknown.

4. **build.cpp / BUILD_draw (ADD-07):** grep на `BUILD_draw`, `BUILD_draw_inside` в `new_game/src/`. Есть ли файл `build.cpp` или эквивалент. Если функции существуют — есть ли call sites.

5. **Camera.h CameraMan (ADD-17 / AI-01):** grep на `CameraMan`, `MAX_CAMERAS`, `alloc_camera`, `create_camera`, `process_t_camera`, `process_f_camera`, `TO_CAMERA`. Есть ли в нашей базе полноценная camera-thing entity система.

6. **flag_v_faces / FACE_FLAG_VERTICAL (ENG-05):** grep на `FACE_FLAG_VERTICAL` и `flag_v_faces` в `new_game/src/`. Где выставляется этот флаг, где читается, работает ли.

7. **save_table reserve counts (ENG-03):** найти `save_table[]` в `new_game/src/` (likely `memory.cpp` или аналог). Извлечь значения поля `Maximum` (или `Extra`) для SAVE_TABLE_VEHICLE, PEOPLE, SPECIAL, BAT, DTWEEN, DMESH, PLAT, WMOVE. Нужны для будущего сравнения с release values.

8. **Thing struct members (AI-01):** найти `Thing.h` в `new_game/src/` (grep `uc_orig:.*Thing.h`). Проверить — есть ли поля `Timer1`, `BuildingList` union, `CameraMan* Camera` в union Genus.

9. **height_above_anything debug (PHY-01):** найти `height_above_anything` в `new_game/src/` (likely `collide.cpp`, grep `uc_orig:.*collide.cpp`). Проверить — есть ли debug short-circuit `if (1 || p_person->Ware)` или уже чистое `if (p_person->Ware)`. Если есть `1 ||` — баг подтверждён, идёт в auto-port Шаг 2.

10. **apply_cloud state (GFX-01):** найти `apply_cloud` в `new_game/src/` (likely `facet.cpp`). Реальная реализация (walks `cloud_data[32][32]`) или no-op stub/inline макрос? **ВАЖНО:** даже если у нас stub, НЕ восстанавливать — retail тоже не показывает cloud shadows (подтверждено пользователем). Audit нужен только чтобы знать текущее состояние.

11. **CurDrawDistance representation (GFX-38):** найти `CurDrawDistance` в `new_game/src/` (likely `aeng.cpp`). Хранится как 8.8 fixed-point (значение ~5632 для 22 мап-сквэров) или plain integer (22)?

**Группировка в 2-3 субагента:** например агент #1 делает 1-4, агент #2 делает 5-8, агент #3 делает 9-11. Или другая разбивка — главное 2-3 параллельно.

**Формат результата от агента:** markdown таблица с колонками `Audit ID | Status | Details | Action`, где:
- Status = `DONE` (фикс уже применён в нашей базе) / `MISSING` (баг присутствует) / `DIFFERENT` (сделано по-другому) / `NOT_APPLICABLE`
- Action = `SKIP port (already done)` / `PORT needed` / `INVESTIGATE manually`

### Шаг 2. Применить auto-port batch (16 bug fixes)

После получения результатов audits отфильтровать список — не порт-ить то что audit пометил как `DONE`. Финальный список (максимум 16 пунктов, возможно меньше после аудитов):

| Tag | File | Fix |
|---|---|---|
| **PHY-02** | `collide.cpp` `calc_map_height_at` | typo `MAVHEIGHT(mx, mz)` → `(mx, z)` |
| **PHY-01** | `collide.cpp` `height_above_anything` | убрать `if (1 \|\| ...)` debug short-circuit (ТОЛЬКО если audit #9 = MISSING) |
| **ENG-18** | `ob.cpp` `OB_remove` | раскомментировать `return` после `remove_walkable_from_map(i)` |
| **GFX-07** | `poly.h` | `POLY_shadow[POLY_SHADOW_SIZE]` → `[POLY_BUFFER_SIZE]` |
| **GFX-19+37** | `aeng.cpp` | двухшаговый lock-step фикс: (1) resize `AENG_upper`/`AENG_lower` с `[MAP_WIDTH/2]` до `[MAP_WIDTH][MAP_HEIGHT]`, (2) убрать `& 63` маски на ~30 call sites. ОБЯЗАТЕЛЬНО вместе, один без другого = overflow |
| **ANM-15** | `mesh.cpp` `MESH_draw_guts` | убрать `ASSERT(0)` на untextured face (crash fix) |
| **ANM-03** | `figure.cpp` `FIGURE_draw` | добавить null-check на `ae1`/`ae2` с error message |
| **PRT-04** | `fire.cpp` `FIRE_process` | free-list fix: добавить `*prev = fl->next;` при unlink (ТОЛЬКО если audit #2 показал что не сделано) |
| **PRT-10** | `psystem.cpp` `PARTICLE_Run` | fix iterator advance в free path: `p = particles + temp` после unlink (ТОЛЬКО если audit #1 показал что не сделано) |
| **PRT-11** | `psystem.cpp` `PARTICLE_Run` | fix iterator advance в normal path: `p = particles + p->prev` (ТОЛЬКО если audit #1) |
| **AI-08** | `Special.cpp` | (1) удалить дубликат `SPECIAL_TREASURE` обработки в `person_get_item()`, (2) swap STA/REF ObjectId 71↔81 в `special_normal` |
| **AI-04** | `bat.cpp` `BAT_balrog_slide_along` | удалить весь `PAP_FLAG_HIDDEN` collision block (проверка 4 соседних тайлов) |
| **VEH-04** | `Vehicle.cpp` | убрать guard `&& p_thing->Genus.Vehicle->Driver` перед siren toggle (null-safety) |
| **VEH-10** | `wmove.cpp` | **новая функция** `void WMOVE_remove(UBYTE which_class)` — bulk walkable cleanup для level transitions. Код функции в `original_game_compare/new/Fallen/Source/wmove.cpp` (grep `WMOVE_remove` в `new/`) |
| **VEH-11** | `wmove.cpp` `WMOVE_relative_pos` | fixed-point rework: shifts `<<13/12` → `<<12/11`, split Y-sum, убрать hard `0x10000` teleport clamps. Точная формула в `original_game_compare/new/Fallen/Source/wmove.cpp` — портировать ДОСЛОВНО |
| **AUD-07** | `sound_id.cpp` `sound_list[]` | reorder wind samples — WIND2 сейчас на позиции WIND5, фикс индексного бага. Точный порядок в `new/Fallen/Source/sound_id.cpp` |
| **PRT-50** | `pcom.cpp` `PCOM_add_gang_member` | shuffle loop: start index `PCOM_gang_person_upto - 1` → `- 2` (off-by-one, пишет за пределы валидного диапазона) |
| **PRT-23** | `pcom.cpp` | удалить `ASSERT(0)` тайпрайтеры в AI-функциях: `PCOM_add_gang_member`, `PCOM_alert_my_gang_to_a_fight`, `PCOM_set_person_ai_arrest`, `PCOM_set_person_ai_kill_person`, `PCOM_set_person_ai_navtokill*` (защитные asserts крашат на валидных game states) |

**Процесс порта:**
1. Для каждого пункта — прочитать текущее состояние в нашей базе через `grep "uc_orig:.*<filename>" new_game/src/` → Read файла → применить Edit
2. После каждого 3-4 фиксов — попросить пользователя собрать и проверить что не сломалось
3. Коммиты **не создавать автоматически** — пользователь коммитит вручную
4. Если внутри фикса натыкаешься на непонятное (файл переименован, функция выглядит иначе чем в отчёте) — **останови работу и спроси пользователя**

### Что ОТЛОЖЕНО (не делать в этой сессии)

Всё остальное из плана:
- ❌ VERIFY session retail Steam checks (пользователь проведёт позже)
- ❌ Person.cpp guards batches A/B/C (blocked на VERIFY feel check)
- ❌ Determinism/PTIME cluster (OPEN решение)
- ❌ P1 UI additions (karma/compass/menufont/lang/mucky_times)
- ❌ P1 visual/draw-order graphics
- ❌ P1 AI/combat improvements
- ❌ P2 trivial cleanups
- ❌ AUDIT inverted findings mass pass
- ❌ Balance pack

**НЕ инициативничать** и не начинать эти пункты без явного слова пользователя. Они все задокументированы в [plan.md](plan.md) с решениями и [findings.md](findings.md) с ID — вернёмся когда пользователь сам захочет.

## Когда работа завершена

1. Отчитаться о том что сделано (какие audit'ы прошли, какие фиксы применены, что было пропущено как DONE, если что-то отложено — причина)
2. Обновить статусы в [findings.md](findings.md) для применённых пунктов (колонка Decision → `DONE`)
3. Добавить короткий лог выполненного в конец этого handoff.md (секция "Лог выполнения")
4. Сказать пользователю что можно продолжить другими задачами проекта

## Лог выполнения

(пусто — заполнит новая сессия по мере выполнения)
