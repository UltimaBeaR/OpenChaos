# DENSE_SUMMARY Build Guide

Инструкция по пересборке `DENSE_SUMMARY.md` с нуля.

---

## Целевой размер

~30-40K токенов (~120-150 строк на секцию × 23 секции → ~2500-3500 строк всего).
Проверка: `wc -l DENSE_SUMMARY.md` — цель 2000-3500 строк.
Если больше — сократить развёрнутые объяснения, оставить только числа и факты.

---

## Порядок чтения файлов KB

Читать в параллельных батчах по 3-4 файла:

### Батч 1 (ядро)
1. `QUICK_FACTS.md` — все ключевые числа; станет основой секции 23 (быстрые числа)
2. `subsystems/physics.md` + `subsystems/physics_details.md`
3. `subsystems/game_objects.md`

### Батч 2 (AI и навигация)
4. `subsystems/ai.md` + `subsystems/ai_structures.md` + `subsystems/ai_behaviors.md`
5. `subsystems/navigation.md`

### Батч 3 (геймплей)
6. `subsystems/controls.md`
7. `subsystems/combat.md`
8. `subsystems/vehicles.md`

### Батч 4 (мир и контент)
9. `subsystems/world_map.md`
10. `subsystems/buildings_interiors.md` + `subsystems/buildings_interiors_details.md`
11. `subsystems/missions.md` + `subsystems/missions_implementation.md`

### Батч 5 (оружие и взаимодействие)
12. `subsystems/weapons_items.md` + `subsystems/weapons_items_details.md`
13. `subsystems/interaction_system.md`
14. `subsystems/characters.md` + `subsystems/characters_details.md`

### Батч 6 (эффекты и рендеринг)
15. `subsystems/effects.md`
16. `subsystems/rendering.md` + `subsystems/rendering_mesh.md` + `subsystems/rendering_lighting.md`

### Батч 7 (системы и инфраструктура)
17. `subsystems/camera.md`
18. `subsystems/audio.md`
19. `subsystems/game_states.md`

### Батч 8 (прогресс и загрузка)
20. `subsystems/level_loading.md`
21. `subsystems/player_progress.md`
22. `subsystems/player_states.md`

### Батч 9 (форматы и скоуп)
23. `resource_formats/` — все файлы в папке
24. `cut_features.md`
25. `analysis_scope.md`
26. `subsystems/ui.md` + `subsystems/frontend.md`
27. `subsystems/minor_systems.md`
28. `subsystems/psx_controls.md`

---

## Что извлекать из каждого файла

**Правило:** только то, что нельзя вывести логически — конкретные числа, константы, формулы, неочевидные зависимости, известные баги.

### Из physics.md / physics_details.md
- Константа GRAVITY (-51 в единицах/кадр²)
- Порядок шагов move_thing() — точный список
- Формулы: fall damage, damping, water_level
- Специфика slide_along (нет отскока)
- Особенности find_face_for_this_pos

### Из game_objects.md
- Размер Thing struct (125 байт)
- Максимум объектов (MAX 700)
- Список CLASS_* типов с кратким описанием
- MapWho: формула хэша, размер ячейки
- Механика state machine (fnptr полиморфизм)

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

### Из controls.md
- 18 INPUT_* кнопок
- 52 ACTION_* действий
- Порядок приоритетов do_an_action
- Player struct поля

### Из combat.md
- GameFightCol в кадрах анимации (не в logic)
- Формула hit chance: 230 - abs(Roll)>>1 - Velocity + mods
- Knockback через анимации
- Система fight tree

### Из weapons_items.md
- 30 SPECIAL_* типов
- Таблица урона: Pistol=70hp, Shotgun=300-dist, AK47=100(pl)/40(NPC)
- Fire rates и capacity
- C4 = 5 сек (не 10), мины заглушены

### Из missions.md / missions_implementation.md
- 42 условия, 52 действия EWAY
- EventPoint struct размер
- Как работает STAY_MODE
- .ucm ≠ MuckyBasic

### Из buildings_interiors.md
- 21 STOREY_TYPE_* с кратким описанием
- Иерархия: FBuilding→FStorey→FWall→DFacet
- Как двери синхронизированы с MAV
- Алгоритм размещения лестниц

### Из world_map.md
- PAP_Hi 128×128 (1024×1024 единицы), PAP_Lo 32×32
- MapWho ячейка = 8×8 единиц
- ROAD граф и WAND_init

### Из interaction_system.md
- find_grab_face 2-pass алгоритм
- Cable параметры в полях DFacet
- Лестница: ladder_step_height

### Из effects.md
- psystem: MAX эффектов, ключевые типы
- fire.cpp отдельная система
- PYRO 18 типов, DIRT 16 типов
- RIBBON, BANG, POW механика

### Из rendering.md
- DDEngine bucket sort (painter's algorithm)
- Порядок рендер-пайплайна
- DRAWXTRA_MIB_destruct — игровая логика в рендерере! (критичная зависимость)
- NIGHT система

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

### Из level_loading.md
- 9 шагов загрузки
- MAV_precalculate = самый тяжёлый
- Игрок создаётся через EWAY_process (не напрямую)

### Из player_progress.md
- .wag v3 layout
- best_found анти-фарм механика
- mission_hierarchy bits

### Из resource_formats/
- Каждый формат: расширение, назначение, ключевые структуры

### Из cut_features.md
- Список вырезанных фич с кратким статусом (закомментировано, ASSERT, etc.)

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
23. Быстрые числа ← полная числовая таблица из QUICK_FACTS
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
2. **Камера зависит от MAV** — FC_process использует MAV_find_path для raycast коллизии камеры с геометрией
3. **Двери синхронизированы с MAV** — открытие/закрытие двери должно обновлять MAV граф (MAV_door_open/close), иначе NPC застрянут
4. **EWAY контролирует спавн** — игрок создаётся через EWAY_process, не напрямую
5. **AI таймер независим от frame rate** — PCOM_TICKS_PER_SEC=320, но не привязан к рендер-FPS
6. **Анимации управляют физикой** — GameFightCol в кадрах анимации, gravity_anim_driven у персонажей
7. **MAV_precalculate блокирует загрузку** — самый тяжёлый шаг (нельзя сделать async без изменения архитектуры)

---

## Как проверить качество

После создания DENSE_SUMMARY:

1. **Размер:** `wc -l` — должно быть 2000-3500 строк
2. **Числа:** выбрать 10 случайных чисел и проверить по QUICK_FACTS.md — должны совпадать
3. **Покрытие:** каждая строка таблицы из SESSION_START.md должна иметь соответствующую секцию в DENSE_SUMMARY
4. **Межсистемные зависимости:** секция 20 содержит минимум 7 пунктов из списка выше
5. **Баги:** секция 21 содержит минимум 15 записей (в оригинале было 20+)
