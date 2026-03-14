# DENSE_SUMMARY Build Guide

Инструкция по пересборке `DENSE_SUMMARY.md` с нуля.

---

## ⚠️ КРИТИЧНО: Перед ребилдом удалить старый файл!

Перед запуском пересборки **обязательно удалить** существующий `DENSE_SUMMARY.md`. Subagent'ы читают KB файлы и пишут результат — если старый файл существует, возникают проблемы с чтением + записью (файл может быть частично перезаписан или координатор может случайно прочитать устаревшие данные из него вместо свежих результатов от subagent'ов).

```bash
rm original_game_knowledge_base_dense/DENSE_SUMMARY.md
```

---

## ⚠️ КРИТИЧНО: Пересборка только через subagent'ов!

**Никогда не читать всю KB в основном контексте!** KB — это ~40+ файлов по 100-350 строк. Если читать их в основном контексте координатора, он забьётся и начнётся компакция — данные потеряются, и в DENSE_SUMMARY попадут неполные/искажённые данные.

### Правильный workflow:

1. **Координатор** (основной контекст) — НЕ читает KB файлы напрямую. Только:
   - Читает этот Build Guide
   - Запускает subagent'ов на каждый батч
   - Получает от них компактные результаты
   - Компонует результаты в итоговый DENSE_SUMMARY.md
   - Записывает файл

2. **Subagent'ы** — каждый читает свой батч KB файлов и возвращает компактную выжимку:
   - Агент получает задание: "Прочитай файлы батча N, извлеки ключевые факты по списку из Build Guide, верни компактный результат"
   - Агент читает файлы в своём контексте (который потом освобождается)
   - Агент возвращает только выжимку (~100-200 строк на батч) — не весь текст KB

3. **Параллельность:** батчи 1-9 можно запускать параллельно (они независимы). Батч 0 (общие файлы) лучше включить в каждый агент как контекст, либо обработать отдельным агентом первым.

### Формат задания для subagent'а:

```
Прочитай следующие файлы из original_game_knowledge_base/:
- [список файлов батча]

Для контекста также прочитай (Батч 0):
- overview.md, analysis_scope.md, cut_features.md, preprocessor_flags.md

Извлеки и верни ТОЛЬКО:
- Конкретные числа, константы, размеры структур
- Формулы (в code-блоках)
- Неочевидные зависимости между системами
- Известные баги
- Списки типов/перечислений (кратко)

НЕ включай: общие описания, объяснения "как работает", пересказ текста.
Формат: маркированные списки, числа bold, формулы в code-блоках.
Целевой размер: ~100-200 строк.
```

---

## Целевой размер

~30-40K токенов (~120-150 строк на секцию × 23 секции → ~2500-3500 строк всего).
Проверка: `wc -l DENSE_SUMMARY.md` — цель 2000-3500 строк.
Если больше — сократить развёрнутые объяснения, оставить только числа и факты.

---

## Батчи файлов KB для subagent'ов

### Батч 0 (общие файлы — контекст для всех агентов)
Эти файлы дают общий контекст: архитектуру, скоуп анализа, карту связей между подсистемами, препроцессорные флаги. Каждый subagent должен прочитать их для понимания контекста.

0. `KB_INDEX.md` — полный индекс всех файлов KB (понимание структуры)
1. `overview.md` — архитектура оригинала: структура папок, entry point, карта связей, аннотированные файлы
2. `analysis_scope.md` — что анализируется и что пропускается, PC+PS1 эталон
3. `cut_features.md` — вырезанные/отключённые фичи
4. `preprocessor_flags.md` — все #ifdef флаги и что они контролируют

### Батч 1 (ядро)
1. `subsystems/physics.md` + `subsystems/physics_details.md`
2. `subsystems/game_objects.md` + `subsystems/game_objects_details.md`

### Батч 2 (AI и навигация)
3. `subsystems/ai.md` + `subsystems/ai_structures.md` + `subsystems/ai_behaviors.md`
4. `subsystems/navigation.md`

### Батч 3 (геймплей)
5. `subsystems/controls.md` + `subsystems/psx_controls.md`
6. `subsystems/combat.md`
7. `subsystems/vehicles.md`

### Батч 4 (мир и контент)
8. `subsystems/world_map.md`
9. `subsystems/buildings_interiors.md` + `subsystems/buildings_interiors_details.md`
10. `subsystems/missions.md` + `subsystems/missions_implementation.md`

### Батч 5 (оружие и взаимодействие)
11. `subsystems/weapons_items.md` + `subsystems/weapons_items_details.md`
12. `subsystems/interaction_system.md`
13. `subsystems/characters.md` + `subsystems/characters_details.md`

### Батч 6 (эффекты и рендеринг)
14. `subsystems/effects.md` + `subsystems/effects_pyro_pow_dirt.md`
15. `subsystems/rendering.md` + `subsystems/rendering_mesh.md` + `subsystems/rendering_lighting.md`

### Батч 7 (системы и инфраструктура)
16. `subsystems/camera.md`
17. `subsystems/audio.md`
18. `subsystems/game_states.md`
19. `subsystems/player_states.md`

### Батч 8 (прогресс и загрузка)
20. `subsystems/level_loading.md`
21. `subsystems/player_progress.md`
22. `subsystems/ui.md` + `subsystems/frontend.md`

### Батч 9 (форматы и малые системы)
23. `resource_formats/` — все файлы в папке (включая `mission_script_format.md`)
24. `subsystems/minor_systems.md`
25. `subsystems/math_utils.md`

Файл `subsystems/waywind.md` — описывает редактор (WayWind), вне скоупа новой игры. Читать не обязательно, но из него можно взять связи с AI системой.

---

## Что извлекать из каждого файла

**Правило:** только то, что нельзя вывести логически — конкретные числа, константы, формулы, неочевидные зависимости, известные баги.

### Из общих файлов (Батч 0)
- **overview.md:** архитектура (MFC app, game loop entry), карта связей между подсистемами, какие файлы к какой подсистеме относятся
- **analysis_scope.md:** что пропущено при анализе и почему; PC+PS1 = двойной эталон
- **cut_features.md:** список вырезанных фич с кратким статусом (закомментировано, ASSERT, etc.)
- **preprocessor_flags.md:** ключевые #ifdef (VERSION_D3D, PSX, TARGET_DC, FINAL) — что включено в PC сборке, что нет

### Из physics.md / physics_details.md
- Константа GRAVITY (-51 в единицах/кадр²)
- Порядок шагов move_thing() — точный список
- Формулы: fall damage, damping, water_level
- Специфика slide_along (нет отскока)
- Особенности find_face_for_this_pos

### Из game_objects.md / game_objects_details.md
- Размер Thing struct (125 байт)
- Максимум объектов (MAX 700)
- Список CLASS_* типов с кратким описанием
- MapWho: формула хэша, размер ячейки
- Механика state machine (fnptr полиморфизм)
- Barrel, Platform, Chopper — ключевые параметры

### Из ai.md + ai_structures.md + ai_behaviors.md
- Полные списки PCOM_AI (22 типа, 0-21) и PCOM_AI_STATE (28 состояний, 0-27)
- Размер Person struct
- Константы: PCOM_TICKS_PER_SEC=320, PCOM_ARRIVE_DIST=0x40
- Алгоритм восприятия (угол, дистанция, line-of-sight)
- Специфические факты по MIB/Bane/Driving AI

### Из navigation.md
- MAV = greedy best-first (не A*), горизонт 32 тайла
- Wallhug алгоритм — wallhug_step()
- Известный баг MAV_turn_movement_on
- INSIDE2_mav_inside = ASSERT(0) — тупик

### Из vehicles.md
- Список 9 типов транспорта
- 4 пружины подвески, 6 зон урона
- Формула friction (bit-shift)
- Константы скорости/ускорения

### Из controls.md / psx_controls.md
- 18 INPUT_* кнопок
- 52 ACTION_* действий
- Порядок приоритетов do_an_action
- Player struct поля
- PSX layouts (4 конфига)

### Из combat.md
- GameFightCol в кадрах анимации (не в logic)
- Формула hit chance: 230 - abs(Roll)>>1 - Velocity + mods
- Knockback через анимации
- Система fight tree

### Из weapons_items.md / weapons_items_details.md
- 30 SPECIAL_* типов
- Таблица урона: Pistol=70hp, Shotgun=300-dist, AK47=100(pl)/40(NPC)
- Fire rates и capacity
- C4 = 5 сек (не 10), мины заглушены
- Гранаты, автоприцел, DIRT банки

### Из missions.md / missions_implementation.md
- 42 условия, 52 действия EWAY
- EventPoint struct размер
- Как работает STAY_MODE
- .ucm ≠ MuckyBasic

### Из buildings_interiors.md / buildings_interiors_details.md
- 21 STOREY_TYPE_* с кратким описанием
- Иерархия: FBuilding→FStorey→FWall→DFacet
- Как двери синхронизированы с MAV
- Алгоритм размещения лестниц
- id.cpp рендеринг, WARE склады

### Из world_map.md
- PAP_Hi 128×128 (1024×1024 единицы), PAP_Lo 32×32
- MapWho ячейка = 8×8 единиц
- ROAD граф и WAND_init

### Из interaction_system.md
- find_grab_face 2-pass алгоритм
- Cable параметры в полях DFacet
- Лестница: ladder_step_height

### Из effects.md + effects_pyro_pow_dirt.md
- psystem: MAX эффектов, ключевые типы
- fire.cpp отдельная система
- PYRO 18 типов (детальная таблица из effects_pyro_pow_dirt.md)
- DIRT 16 типов (детальная таблица)
- RIBBON, BANG, POW механика

### Из rendering.md + rendering_mesh.md + rendering_lighting.md
- DDEngine bucket sort (painter's algorithm)
- Порядок рендер-пайплайна
- DRAWXTRA_MIB_destruct — игровая логика в рендерере! (критичная зависимость)
- NIGHT система, Crinkle
- Tom's Engine, морфинг мешей

### Из camera.md
- FC only (cam.cpp мёртв)
- 15 шагов FC_process
- Toonear mode
- FOV фиксирован

### Из audio.md
- MSS32 API
- 14 MUSIC_MODE_*
- 5 биомов ambient
- MFX API

### Из game_states.md
- GS_* флаги per-frame порядок
- Win/Lose flow
- DarciDeadCivWarnings

### Из player_states.md
- Все STATE_* и SUB_STATE_* константы
- Группировка подсостояний (движение, прыжки, бой, смерть, лазание, транспорт, оружие)
- Типы персонажей (PLAYER_*, PERSON_*)

### Из level_loading.md
- 9 шагов загрузки
- MAV_precalculate = самый тяжёлый
- Игрок создаётся через EWAY_process (не напрямую)

### Из player_progress.md
- .wag v3 layout
- best_found анти-фарм механика
- mission_hierarchy bits

### Из ui.md + frontend.md
- HUD, шрифты, панели
- Frontend pipeline (STARTSCR → миссия)
- Особые миссии (park2, Finale1, туториалы)

### Из resource_formats/ (все файлы)
- Каждый формат: расширение, назначение, ключевые структуры
- mission_script_format.md: .sty формат, suggest_order[], районы

### Из cut_features.md
- Список вырезанных фич с кратким статусом (закомментировано, ASSERT, etc.)

### Из minor_systems.md
- Вода, растяжки (trip), SM, шары, следы, искры, command system, drawxtra

### Из math_utils.md
- FMatrix vs Matrix (два набора), Root(), fixed-point форматы

---

## Структура DENSE_SUMMARY.md

23 секции (нумерованные `##`):

```
1.  Игровой цикл и время
2.  Система координат и карта
3.  Thing System
4.  Физика и коллизии
5.  AI система PCOM
6.  Навигация (MAV + Wallhug)
7.  Транспорт
8.  Управление и ввод
9.  Бой и оружие
10. Взаимодействие
11. Миссии EWAY
12. Здания и интерьеры
13. Эффекты
14. Камера
15. Рендеринг
16. Звук
17. UI и Frontend
18. Прогресс и сохранения
19. Загрузка уровня
20. Межсистемные зависимости ← критично не забыть
21. Пре-релизные баги и особенности
22. Форматы файлов
23. Быстрые числа ← собрать из всех секций KB
```

Каждая секция:
- Маркированные списки, не абзацы
- Числа и константы bold (`**`)
- Формулы в code-блоках
- Неочевидные факты отмечены `⚠️` или `[!]`

---

## Секция 20 (Межсистемные зависимости) — особое внимание

Это самая важная секция для архитектурных решений. Обязательно включить:

1. **Рендерер мутирует игровое состояние** — DRAWXTRA_MIB_destruct() в drawxtra.cpp изменяет ammo_packs_pistol, запускает PYRO/SPARK. Это нарушение слоистости.
2. **Камера зависит от MAV** — FC_process использует MAV_inside для коллизии камеры с геометрией
3. **Двери синхронизированы с MAV** — открытие/закрытие двери должно обновлять MAV граф (MAV_turn_movement_on/off), иначе NPC застрянут
4. **EWAY контролирует спавн** — игрок создаётся через EWAY_process, не напрямую
5. **AI таймер независим от frame rate** — PCOM_TICKS_PER_SEC=320, но не привязан к рендер-FPS
6. **Анимации управляют физикой** — GameFightCol в кадрах анимации, gravity_anim_driven у персонажей
7. **MAV_precalculate блокирует загрузку** — самый тяжёлый шаг (нельзя сделать async без изменения архитектуры)

---

## Как проверить качество

После создания DENSE_SUMMARY:

1. **Размер:** `wc -l` — должно быть 2000-3500 строк
2. **Числа:** выбрать 10 случайных чисел из разных секций и проверить по соответствующим subsystems/*.md — должны совпадать
3. **Покрытие:** каждая подсистема из таблицы в `KB_INDEX.md` должна иметь соответствующую секцию или упоминание в DENSE_SUMMARY
4. **Межсистемные зависимости:** секция 20 содержит минимум 7 пунктов из списка выше
5. **Баги:** секция 21 содержит минимум 15 записей (в оригинале было 20+)
