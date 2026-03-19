# Девлог Этапа 3.5 — Актуализация базы знаний

## Итерация 1 — Ревизия KB по находкам Этапа 2 (2026-03-19)

**Модель:** Opus (1M контекст)

### Методология

1. Прочитан весь девлог (`new_game_devlog/`): stage2_log.md (1638 строк, 54 итерации), stage2_regression_investigation.md, build_and_run_notes.md, stage3_log.md, community_links.md, todo.md
2. Прочитаны ключевые файлы KB: overview.md, preprocessor_flags.md, cut_features.md, rendering.md, camera.md, effects.md, audio.md, navigation.md, minor_systems.md, buildings_interiors.md, game_objects.md, controls.md, math_utils.md, level_loading.md, ui.md, frontend.md, game_states.md, interaction_system.md, vehicles.md
3. Выписаны все расхождения между KB и девлогом
4. Внесены исправления
5. Проверена внутренняя согласованность KB

### Исправления

#### overview.md (3 правки)

1. **MFStdLib**: было "IGNORED: внутренние утилиты студии, не игровой код" → исправлено на "базовые утилиты: типы (SLONG, UBYTE, UWORD — MFTypes.h), память (StdMem.cpp), математика (inline.h: MUL64/DIV64), Always.h. Активно используется всей кодовой базой."
   - **Причина:** при Этапе 2 обнаружено что MFStdLib/ не просто "утилиты студии", а фундамент всей кодовой базы. Без MFTypes.h ничего не компилируется. StdMem.cpp — единственная реализация аллокатора. inline.h содержит MUL64/DIV64 для 64-bit арифметики.

2. **Editor/**: было "IGNORED: инструмент редактора уровней" → добавлено описание рантайм-кода, который живёт в Editor/.
   - **Причина:** итерация 12 Этапа 2 обнаружила что Editor/ содержит файлы необходимые рантайму: Anim.h (анимационные структуры через Structs.h), Prim.h (прим-объекты), prim_draw.h (rotate_obj для interact.cpp), map.h (edit_map для building/collide), outline.cpp (build_roof_grid — рантайм), ed.h (ED_Light для загрузки .lgt), Thing.h (MapThingPSX для io.cpp save_type==18). Все были извлечены в Headers/.

3. **Камера**: "cam.cpp, fc.cpp" → "fc.cpp (активная); cam.cpp — МЁРТВЫЙ КОД (#ifdef DOG_POO)"
   - **Причина:** камера cam.cpp задокументирована в camera.md как мёртвая, но в таблице overview.md это не было отражено.

#### rendering.md (1 правка)

- Убрана ссылка на несуществующий "DDEngine/cam.cpp". Такого файла в DDEngine нет — мёртвый cam.cpp находится в Source/cam.cpp.

#### preprocessor_flags.md (3 правки)

1. **USE_A3D**: было "в psxeng.h" → исправлено на "определён в Sound.h". A3D_SOUND описан отдельно (A3DManager.cpp). Добавлена пометка что Aureal обанкротилась в 2000.
   - **Причина:** итерация 28 Этапа 2 обнаружила что `#define USE_A3D` в Sound.h, не в psxeng.h.

2. **BUILD_PSX**: уточнено что определяется ТОЛЬКО внутри `#ifdef PSX` в Game.h → на PC никогда не определён. Описание "препроцессорный этап" было неясным.

3. **Баг /MTd в Release**: добавлена документация бага — Release конфиг vcxproj использовал MultiThreadedDebug, что автоматически определяло _DEBUG и активировало весь debug-код в Release.
   - **Причина:** обнаружено при первой сборке (build_and_run_notes.md). Фундаментальный факт об оригинальном билде.

#### cut_features.md (16+ новых записей)

Добавлены вырезанные фичи обнаруженные при Этапе 2:
- Симуляция ткани (cloth.cpp CLOTH_process в #if 0)
- Indoor navigation AI (inside2.cpp INSIDE2_mav_nav_calc в #if 0)
- Heartbeat HUD (PANEL_do_heartbeat в #if 0)
- 3D компас (PANEL_draw_compass в #if 0)
- Портреты врагов (track_enemy под OLD_POO, заменены floating HP bar)
- Цифры урона (DAMAGE_TEXT)
- TCP/IP консоль (console.cpp в #if 0)
- 3D шрифты (Font3D в #if 0)
- Aureal A3D звук (компания обанкротилась)
- Force Feedback (FFManager — не реализован)
- Ray система (ray.cpp в #if 0, никем не вызывается)
- ENV система (Env.cpp в #if 0)
- Attract mode DDEngine (вырезан)
- Шарики (Balloon.cpp — load_prim_object закомментировано)
- Старая камера (cam.cpp — #ifdef DOG_POO)
- Command система (legacy, заменена PCOM)
- Software renderer (sw.cpp/tom.cpp — не в vcxproj)
- Thug old AI (fn_thug_normal тело в #if 0)

#### navigation.md (1 правка)

- INSIDE2_mav_nav_calc: добавлена пометка что весь блок (~400 строк) находится в #if 0. Ранее функция описывалась как рабочая (с багом), но не упоминалось что она мертва.

### Проверка внутренней согласованности

Прочитано 19 subsystem-файлов + 4 общих файла KB. Найденные несоответствия:
- Все исправлены (см. выше)

Оставшиеся вопросы (низкий приоритет, не требуют срочного анализа):
- Баг INSIDE2_mav_nav_calc (цикл Z по MinX..MaxX) — задокументирован в KB, верификация по коду не проводилась
- Влияние /Zp1 на WinAPI структуры — задокументирован риск, не проявлялся на практике

### Статус

- [x] Шаг 1: Прочитать все файлы девлога
- [x] Шаг 2: Выписать расхождения
- [x] Шаг 3: Обновить KB файлы
- [x] Шаг 4: Проверить внутреннюю согласованность
- [x] Шаг 5: Точечный анализ (не требуется — нет критичных вопросов)
- [x] Шаг 6: Пересборка DENSE_SUMMARY (3 файла, 23 секции, ~39KB суммарно)
