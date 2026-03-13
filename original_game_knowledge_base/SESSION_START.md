# SESSION START — Urban Chaos KB Быстрый Ориентир

**Правило старта сессии:** Читать ТОЛЬКО этот файл + CLAUDE.md. Остальные файлы KB читать точечно по мере необходимости.
**Если задача касается подсистемы X → читать subsystems/X.md перед работой.**

---

## ⚡ СЛЕДУЮЩАЯ ИТЕРАЦИЯ — ПРИОРИТЕТ 1

**Варианты:**
1. **Форматы ресурсов** (КРИТИЧНО для data pipeline): `.all`, `.iam`, `.ucm`, `.prm` binary layout — читать `io.cpp`
2. **AI TODO** (missions.md → ai.md): PCOM_process_default(), Driving AI, MIB поведение
3. **Навигация TODO**: WARE_mav_*, INSIDE2_mav_*, MAV при изменении уровня

**ВЫПОЛНЕНО в этой итерации:**
- interfac.cpp аннотирован (ключевые функции: process_hardware_level_input_for_player, player_apply_move,
  player_interface_move, player_turn_left_right_analogue, apply_button_input_fight, do_an_action,
  InputDone mechanism, weapon hotkeys KB_1..KB_8, main mode dispatcher)
- SESSION_START.md обновлён (статусы audio.md, ui.md, math_utils.md → ✅)

---

## interfac.cpp аннотирование — ИТОГ (выполнено)

**Ключевые открытия:**
- process_hardware_level_input_for_player(): камера сначала → EWAY стоп → form_leave_map стоп → InputDone mask → weapon keys → mode dispatch
- InputDone механизм: накапливает обработанные биты, сбрасывает отпущенные; кнопки срабатывают раз на нажатие (не каждый кадр)
- Главный диспетчер: DRIVING→apply_button_input_car; FIGHT→apply_button_input_fight; нормальный→apply_button_input
- player_apply_move(): STATE machine, только поворот; бег/прыжок/действие — в apply_button_input
- player_turn_left_right(): wMaxTurn=94 (idle), 12 (в воздухе), 70-Velocity (бег); клавиатура → накопительный поворот; стик → пропорциональный
- player_turn_left_right_analogue(): стик → Arctan(-dx,dz) → +camera_angle; Roll = (Velocity-9)*dx>>5 (наклон при беге)
- apply_button_input_fight(): MOVE без FORWARDS = выход из FIGHT в RUN режим (только без analogue)
- do_an_action() приоритеты: выйти из машины > арест > слезть с лестницы > сесть в машину > крюк > переключатель > разговор > подбор > обыск > обнять стену > кола > присесть
- should_i_jump(): проверяет WARE_which_contains по 4 точкам; dx/dz захардкожены (не зависят от угла персонажа — баг/упрощение!)
- Горячие клавиши: KB_1=убрать, KB_2=пистолет, KB_3..KB_8=SHOTGUN/AK47/GRENADE/C4/KNIFE/BAT
- Аннотированы: interfac.cpp ~50+ claude-ai блоков

---

## game_states.md + camera.md — ИТОГ (выполнено)

**game_states.md ключевые открытия:**
- Per-frame порядок: GAMEMENU → tutorial → controls → should_i_process (things+EWAY+FC) → draw_screen → OVERLAY → screen_flip → lock_frame_rate(30) → handle_sfx → GAME_TURN++
- Bench cooldown: кадр 314 из каждых 1024 (~34с) → сброс GF_DISABLE_BENCH_HEALTH
- Post-loop: park2.ucm→cutscene(1), Finale1.ucm→cutscene(3)+OS_hack()
- DarciDeadCivWarnings: 0-2 = предупреждения; >=3 = GS_LEVEL_LOST; персистирует между миссиями!
- GS_REPLAY = goto round_again (полный рестарт); нет чекпоинтов
- SMOOTH_TICK_RATIO: 4-кадровое скользящее среднее TICK_RATIO для машин

**camera.md ключевые открытия:**
- cam.cpp = МЁРТВЫЙ КОД (`#ifdef DOG_POO`); только fc.cpp активен
- FC_alter_for_pos: gun-out→ddist=200; InCar→ddist=356; idle height check → МЁРТВЫЙ КОД (`&& 0`)
- FC_calc_focus: focus_yaw = персонаж.Angle; peeking (HUG_WALL_LOOK) смещает focus_x/z; dangling → камера сбоку (±550)
- FC_focus_above: gun-out → lower 0xa000; InsideIndex → cam_height*1.5; машины разных типов по-разному
- FC_process: 8-шаговый raycast от want_pos к focus для collision + get-behind + distance clamp + smoothing (>>2 pos, >>3 Y, >>2 angles)
- Lens: всегда 0x28000*CAM_MORE_IN (FOV не меняется)
- Toonear 0x90000 = first-person mode (специальный случай)

---

## Mission.cpp / CRIME_RATE / Win-Lose — ИТОГ (выполнено)

**Ключевые открытия:**
- Mission.cpp = WayWind EDITOR файл, не игровой рантайм
- Трансляция WPT_* → EWAY_DO_* происходит в elev.cpp (строки 748-1654) при загрузке .ucm
- WPT_GOTHERE_DOTHIS (39) = НЕ РЕАЛИЗОВАН в пре-релизе (ASSERT(0) в default case)
- WPT_BONUS_POINTS → EWAY_DO_MESSAGE (очки = мёртвый код через `if(0)`)
- set_stats() (Person.cpp:412) = тривиальна: только `stat_game_time = GetTickCount() - stat_start_time`
- CRIME_RATE delta: kill guilty=-2, kill civ wander=+5, arrest guilty=-4, OBJECTIVE мёртв код
- EWAY_counter[7] = счётчик убитых копов (инкр. до 255, не декрементируется)
- DarciDeadCivWarnings (>= 3) → GS_LEVEL_WON превращается в GS_LEVEL_LOST!
- park2.ucm → cutscene 1 (MIB intro); Finale1.ucm → cutscene 3 + OS_hack()
- GROUP_LIFE/DEATH иммунны к себе — только другие WP той же colour+group меняются
- Аннотированы: elev.cpp (WPT→EWAY_DO mapping), eway.cpp (OBJECTIVE, GROUP_LIFE/DEATH, COUNTER), Person.cpp (CRIME_RATE update)
- missions.md обновлён: разделы 10, 10a, 10b — полные детали

---

## eway.cpp — ИТОГ (выполнено)

**Ключевые открытия:**
- File already had extensive annotations from prior sessions + added more this session
- EWAY_COND_PRESSURE и EWAY_COND_CAMERA = **always FALSE** (stubs, не реализованы)
- PSX оптимизация: step=2 (чётные/нечётные кадры), кроме GAME_TURN==0; PC всегда step=1
- EWAY_STAY_ALWAYS → сразу EWAY_FLAG_DEAD (fire-once); в EWAY_process() и в set_inactive
- zone byte в EWAY_create_enemy: bits 0-3=zone, bit 4=INVULNERABLE, bit 5=GUILTY, bit 6=FAKE_WANDER
- Roper bug (пре-релиз): использует Darci stats (Roper блок закомментирован)
- Непрерывные while-active: EMIT_STEAM, WATER_SPOUT (4x DIRT_new_water/кадр), SHAKE_CAMERA (+=3), VISIBLE_COUNT_UP
- Координаты вэйпойнта в mapsquare-единицах → ×256 (<<8) при создании Thing
- EWAY_created_last_waypoint: resolve IDs + count objectives/guilty + init timers + attach camera targets
- 54 функции в файле, детальная карта в KB создана
- missions.md обновлён: детали реализации, stay logic, stub conditions, spawn details

---

## Person.cpp — ИТОГ (выполнено)

**Ключевые открытия:**
- Per-frame порядок: `StateFn(thing)` → `general_process_person()` → `PCOM_process_person()`
- `people_functions[]`: Darci→darci_states, Roper→roper_states, ВСЕ NPC→cop_states
- `darci_states` — только 7 записей (INIT, NORMAL=NULL, HIT, REMOVE, MOVEING, IDLE). Прыжки/бой/лазание через generic_people_functions
- `fn_person_jumping()` — plant_feet() везде закомментирована! Y задаётся явно из PAP или projectile_move_thing
- `fn_person_dangling()` — единственное место активного plant_feet() при приземлении: SUB_STATE_DROP_DOWN_LAND → end==1
- `set_person_drop_down()` — флаги: KEEP_VEL, KEEP_DY, QUEUED, OFF_FACE; DY=-(4<<8), Velocity=-8 если не KEEP
- `general_process_person()` — ПОСЛЕ StateFn, делает: WMOVE, горение, кровь, гарпун, general_process_player()
- JUMPING→LAND: FLY→hit==1→LAND_FAST(если движется) или idle(если стоп) → STATE_MOVEING

---

## Статус фазы анализа

**Фаза 1 (текущая):** Детальный анализ оригинального кода → запись в `original_game_knowledge_base/`
- KB написана примерно на 80%
- Исходники аннотированы примерно на 55% (добавлены Person.cpp, collide.cpp, walkable.cpp)

---

## Карта подсистем (1-2 строки + связи)

| Подсистема | Файл KB | Статус | Ключевой факт |
|-----------|---------|--------|---------------|
| **Физика/коллизии** | physics.md | ✅ Хорошо | Integer-only, slide_along без отскока, гравитация animation-driven для персонажей, HyperMatter для мебели |
| **Thing/Объекты** | game_objects.md | ✅ Хорошо | union+fnptr полиморфизм, MAX 700, MapWho хэш, state machine |
| **AI (PCOM)** | ai.md | ✅ Хорошо | 21 тип AI, 27 состояний, собаки инертны в пре-релизе |
| **Навигация** | navigation.md | ✅ Хорошо | MAV = greedy best-first (НЕ A*), горизонт 32, NAV = wallhug |
| **Персонажи/анимации** | characters.md | ✅ Хорошо | Vertex morphing (НЕ skeletal), DrawTween, Thug/Cop в #if 0 |
| **Управление/ввод** | controls.md | ✅ Хорошо | 18 кнопок INPUT_*, 52 ACTION_*, zipwire есть в финале |
| **Транспорт** | vehicles.md | ✅ Хорошо | 4 пружины подвески, 6 зон урона, мотоцикл НЕ переносить |
| **Бой** | combat.md | ✅ Хорошо | Урон из анимации (GameFightCol), knockback через анимации |
| **Оружие/предметы** | weapons_items.md | ✅ Хорошо | 30 типов SPECIAL_*, мины заглушены, C4 = 5 сек (не 10) |
| **Мир/карта** | world_map.md | ✅ Хорошо | PAP_Hi 128×128 + PAP_Lo 32×32, MapWho, здания |
| **Здания/интерьеры** | buildings_interiors.md | ✅ Хорошо | 21 тип STOREY_TYPE_*, DFacet, двери через MAV |
| **Миссии (EWAY)** | missions.md | ✅ Хорошо | 41 условие, 57 действий, .ucm ≠ MuckyBasic, polling каждый кадр |
| **Загрузка уровня** | level_loading.md | ✅ Хорошо | 9 шагов, MAV_precalculate самый тяжёлый, игрок создаётся через EWAY |
| **Рендеринг** | rendering.md | ✅ Достаточно | Полностью заменяем, DirectX6 → OpenGL |
| **Состояния игрока** | player_states.md | ✅ Хорошо | Полные списки STATE_* и SUB_STATE_* |
| **Эффекты** | effects.md | ✅ Достаточно | Частицы, огонь, ткань отключена |
| **Форматы ресурсов** | resource_formats/ | ⚠️ Частично | Структура папок ясна, binary форматы не задокументированы |
| **Камера** | camera.md | ✅ Хорошо | FC only (cam.cpp=мёртв), 8-шаг raycast collision, get-behind алгоритм |
| **Звук** | audio.md | ✅ Хорошо | Miles Sound System → miniaudio, 14 MUSIC_MODE_*, 5 биомов ambient |
| **UI** | ui.md | ✅ Хорошо | HUD, инвентарь, frontend, fonts, gamemenu |
| **Прогресс/сохранения** | player_progress.md | ⚠️ Частично | В missions.md: .wag формат, complete_point |
| **Матем/утилиты** | math_utils.md | ✅ Хорошо | 2 стека (PSX int/PC float), glm для новой игры |
| **Игровые состояния** | game_states.md | ✅ Хорошо | per-frame порядок, DarciDeadCivWarnings, GS_REPLAY=goto, bench cooldown |
| **WayWind** | waywind.md | ❌ Не нужен | Редактор, не переносить |
| **MuckyBasic** | muckybasic.md | ❌ Не нужен | Не интегрирован с игрой |

---

## Карта связей (что читать вместе)

```
collide.cpp      → physics.md + navigation.md + characters.md
Person.cpp       → ai.md + characters.md + combat.md + controls.md + player_states.md
eway.cpp         → missions.md + game_objects.md
Mission.cpp      → missions.md + weapons_items.md
interfac.cpp     → controls.md + player_states.md + camera.md
Vehicle.cpp      → vehicles.md + physics.md
Special.cpp      → weapons_items.md + combat.md
Building.cpp     → buildings_interiors.md + world_map.md + navigation.md
```

---

## Аннотированные исходники (// claude-ai: комментарии)

| Файл | Кол-во строк | Статус |
|------|-------------|--------|
| hm.cpp | ~309 | ✅ |
| Furn.cpp | ~158 | ✅ |
| pcom.cpp | ~178 | ✅ |
| Special.cpp | ~477 | ✅ |
| bat.cpp | ~128 | ✅ (= Bane AI, не летучие мыши!) |
| canid.cpp | ~101 | ✅ (собаки инертны — dispatch switch закомментирован) |
| cutscene.cpp | ~102 | ✅ |
| ob.cpp | ~124 | ✅ |
| elev.cpp | ~82 | ✅ |
| Anim.cpp | ~111 | ✅ |
| mesh.cpp (game) | ~111 | ✅ |
| id.cpp | ~150 | ✅ |
| facet.cpp | ~109 | ✅ |
| supermap.cpp | ~109 | ✅ |
| io.cpp | ~308 | ✅ |
| Prim.cpp | ~175 | ✅ |
| Game.cpp | ~134 | ✅ |
| figure.cpp (DDEngine) | ~249 | ✅ |
| Controls.cpp | ~215 | ✅ |
| collide.cpp | ~77 ann. | ✅ (уточнено: move_thing порядок, plant_feet, height_above_anything) |
| walkable.cpp | ~20 ann. | ✅ (find_face_for_this_pos подробно аннотирован) |
| Person.cpp | ~8 блоков | ✅ (per-frame порядок, jumping FSM, drop_down, dangling landing) |
| eway.cpp | ~60+ блоков | ✅ (per-frame loop, spawn funcs, cond stubs, stay logic, 54-func map) |
| Mission.cpp | N/A | ✅ (это EDITOR файл, не рантайм; WPT→EWAY_DO в elev.cpp аннотировано) |
| elev.cpp | ~30+ ann. | ✅ (WPT→EWAY_DO mapping, WPT_BONUS_POINTS dead code, WPT_GOTHERE_DOTHIS ASSERT) |
| **interfac.cpp** | ~50+ блоков | ✅ (process_hardware_input, player_apply_move, InputDone, weapon hotkeys, mode dispatcher) |

---

## TODO по подсистемам — что ещё проверить

### ФИЗИКА (physics.md) — TODO
- [x] `find_face_for_this_pos()` — ГОТОВО (в walkable.cpp! Порог 160ед, GRAB_FLOOR=0x50, FIND_ANYFACE/FIND_FACE_NEAR_BELOW)
- [x] `slide_along()` детали — ГОТОВО (NOGO push=0x5800, DFacet axis-aligned, SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT=-0x50)
- [x] `height_above_anything()` — ГОТОВО (if(1||...) делает FIND_ANYFACE мёртвым кодом, всегда FIND_FACE_NEAR_BELOW)
- [x] `collide_against_things()` + `collide_against_objects()` — ГОТОВО (детали в physics.md)
- [x] `move_thing()` реальный порядок операций — ГОТОВО (things → objects → slide_along → edges → find_face → move)
- [ ] `mount_ladder()` — точное поведение (строка 2567 collide.cpp)
- [ ] Коллизии транспорта с персонажами: `VEH_find_runover_things()` детали
- [ ] Воды: как `PAP_FLAG_WATER` влияет на физику, есть ли замедление
- [ ] RWMOVE система (move_thing для не-person объектов)
- [ ] `collide_against_objects()` детали — что именно за OB объекты, их размеры
- [x] `plant_feet()` — ГОТОВО: вызывается из STATE FUNCTIONS, НЕ из move_thing. Активно: fn_person_dangling (SUB_STATE_DROP_DOWN_LAND end==1). В fn_person_jumping — везде закомментирована.

### AI/PCOM (ai.md) — TODO
- [ ] Детали `PCOM_process_default()` — точный порядок проверок в NORMAL state
- [ ] Как именно AI решает перейти из KILLING в FLEE (agression threshold?)
- [ ] `PCOM_AI_MIB` (18) поведение — что отличает MIB от других AI
- [ ] `PCOM_AI_BANE` (19) — разобран в bat.cpp, но детали в KB скудные
- [ ] Driving AI детали — как NPC паркует машину, как выбирает путь
- [ ] Снайперское состояние PCOM_AI_SHOOT_DEAD (21) детали
- [ ] Гражданские воскресают через 200 кадров — проверить точные условия
- [ ] Как работает `pcom_zone` — NPC не слышат звуки из других зон?

### ПЕРСОНАЖИ/АНИМАЦИИ (characters.md) — TODO
- [x] Точные переходы состояний в Darci: JUMPING→LANDING — ГОТОВО (см. Person.cpp итог выше)
- [ ] Как работает `InterruptFrame` в DrawTween (приоритетный кадр)
- [ ] CMatrix33 декомпрессия — Pad encoding, UCA_Lookup детали (в Anim.cpp есть комменты)
- [ ] Roper финальное поведение — fn_roper_normal() пуста в пре-релизе, что в финале?
- [ ] Cop финальное поведение — fn_cop_normal() в #if 0, что в финале?
- [ ] Thug инициализация — ASSERT(0) в пре-релизе, финальная версия?

### НАВИГАЦИЯ (navigation.md) — TODO
- [ ] `WARE_mav_*()` детали — навигация внутри складов (отдельный граф?)
- [ ] `INSIDE2_mav_*()` детали — навигация в зданиях типа полицейский участок
- [ ] Как MAV обновляется при изменении уровня (взрыв стены, открытая дверь)?
- [ ] Детали MAV_can_i_walk — LOS для навигации
- [ ] Что происходит с навигацией когда NPC на крыше?

### МИССИИ/EWAY (missions.md) — TODO
- [x] Порядок разрешения зависимостей EventPoints — ГОТОВО (EWAY_created_last_waypoint linear pass, no cycle detection)
- [x] Как EWAY_process() итерирует по EP — ГОТОВО (по порядку создания 1..upto, linear array, NOT linked list)
- [x] EWAY_COND_PRESSURE / EWAY_COND_CAMERA — ГОТОВО (always FALSE, stubs)
- [ ] Точный binary layout .ucm файла — 14+58 байт на EP, но exact field mapping нужен
- [x] WPT_GOTHERE_DOTHIS (39) — НЕ РЕАЛИЗОВАН в пре-релизе (ASSERT(0) в elev.cpp default)
- [x] WPT_GROUP_LIFE / GROUP_DEATH (33/34) — ГОТОВО (scans colour+group, иммунны сами к себе)
- [x] EWAY_COND_COUNTER_GTEQ (25) — ГОТОВО (EWAY_counter[subtype] += 1 при DO_INCREASE_COUNTER)
- [x] **АННОТИРОВАТЬ** eway.cpp — ВЫПОЛНЕНО (OBJECTIVE, GROUP_LIFE/DEATH, COUNTER, MISSION_COMPLETE/FAIL)
- [x] **АННОТИРОВАТЬ** Mission.cpp — N/A (это редакторский файл, не игровой рантайм)
- [x] **АННОТИРОВАТЬ** elev.cpp WPT→EWAY_DO mapping — ВЫПОЛНЕНО

### УПРАВЛЕНИЕ/ВВОД (controls.md) — TODO
- [x] Детали `process_hardware_level_input_for_player()` — ГОТОВО (8-шаговый порядок, InputDone, камера первая)
- [x] Как работает аналоговый стик — ГОТОВО (NOISE_TOLERANCE=8192 PC / 24 DC, ANALOGUE_MIN_VELOCITY=8 PC / 32 PSX)
- [x] `player_apply_move()` — ГОТОВО (STATE machine: turn, jump, вызов через player_interface_move)
- [x] `player_turn_left_right_analogue()` — ГОТОВО (стик → Arctan(-dx,dz) + camera angle + Roll visual)
- [x] Горячие клавиши KB_1..KB_8 — ГОТОВО (1=убрать, 2=пистолет, 3-8=спецоружие)
- [x] **АННОТИРОВАТЬ** interfac.cpp — ВЫПОЛНЕНО
- [ ] Double-click 200ms — GetTickCount(), точная реализация DoubleClick[] + LastReleased[]
- [ ] `apply_button_input()` полный флоу (нормальный режим бега)

### ФОРМАТЫ РЕСУРСОВ — TODO (КРИТИЧНО для data pipeline)
- [ ] **`.all` файлы** (darci1.all, hero.all, bossprtg.all) — binary layout НЕ задокументирован. Читать `load_anim_system()` в io.cpp
- [ ] **`.iam` файлы** — точный binary layout карты (PAP_Hi + animated prims + OB + supermap)
- [ ] **`.ucm` файлы** — точная binary структура EventPoint (14+58 байт, exact fields)
- [ ] **`.prm` файлы** — формат prim объектов вообще не задокументирован. Читать `load_prim_object()` в io.cpp
- [ ] **`.lgt` файлы** — формат освещения, читать `NIGHT_load_ed_file()`
- [ ] Документировать в `resource_formats/` — создать отдельные файлы

### МИР/КАРТА (world_map.md) — TODO
- [ ] Детали `build_quick_city()` — что именно строит, структура
- [ ] Система дорог ROAD_* — `ROAD_wander_calc()` детали построения графа
- [ ] WAND (зоны блуждания) — `WAND_init()` детали
- [ ] Как освещение `MapElement.Colour` применяется к вершинам при рендеринге

### ЗДАНИЯ/ИНТЕРЬЕРЫ (buildings_interiors.md) — TODO
- [ ] `ID_generate_floorplan()` детали — как именно генерируется планировка комнат
- [ ] `RoomID` система — как rooms scoring работает в `ID_score_layout_house_ground()`
- [ ] Как STAIR_calculate выбирает позицию лестницы (scoring детали)
- [ ] `WARE_init()` — структура данных складских интерьеров

### РЕНДЕРИНГ (rendering.md) — TODO (низкий приоритет — заменяем)
- [ ] Crinkle система — что это точно и как заменить на normal mapping
- [ ] `POLY_frame_draw()` порядок сортировки — bucket sort детали
- [ ] Как `NIGHT_generate_walkable_lighting()` работает

### НЕ ЧИТАНЫ (нужно прочесть KB файлы)
- [x] **camera.md** — ГОТОВО: FC only, 8-шаг raycast, get-behind, focus_yaw, toonear
- [x] **game_states.md** — ГОТОВО: per-frame порядок, DarciDeadCivWarnings, GS_REPLAY
- [x] **audio.md** — ГОТОВО: 14 MUSIC_MODE_*, 5 биомов, MFX→miniaudio, indoor/outdoor, footsteps
- [x] **ui.md** — ГОТОВО: HUD, enemy tracking, frontend modes, fonts, gamemenu
- [x] **math_utils.md** — ГОТОВО: PSX 0-2047 / PC radians, SinTable 2560 entries, glm замена

---

## Быстрые критичные факты (не забыть!)

- Mission.cpp = WayWind EDITOR файл (не игровой рантайм); WPT_* → EWAY_DO_* трансляция в elev.cpp:748-1654
- WPT_BONUS_POINTS → EWAY_DO_MESSAGE (очки = мёртвый код if(0)); WPT_GOTHERE_DOTHIS = ASSERT(0) не реализован
- set_stats() (Person.cpp:412) тривиальна: только записывает stat_game_time
- CRIME_RATE delta: kill guilty=-2, kill wander_civ=+5, arrest guilty=-4
- DarciDeadCivWarnings>=3 → GS_LEVEL_WON превращается в GS_LEVEL_LOST (накапл. между миссиями!)
- EWAY_counter[7] = счётчик убитых копов (cap 255)
- park2.ucm→cutscene 1 (MIB intro); Finale1.ucm→cutscene 3 + OS_hack()
- `bat.cpp` = Bane AI (главный антагонист), НЕ летучие мыши
- Собаки (canid.cpp) = инертны в пре-релизе, dispatch switch закомментирован
- C4 взрывчатка = 5 сек (не 10 как написано в комментарии кода)
- HyperMatter = собственная разработка MuckyFoot (НЕ Criterion middleware)
- HyperMatter: `gy=0` hardcoded — объекты отскакивают от плоскости Y=0, не от реального terrain
- `.ucm` файлы = EWAY Mission Data, НЕ MuckyBasic скрипты
- Roper Health = 400 (в 2× больше Darci = 200)
- MIB Health = 700, KO принципиально невозможен
- FLAG_FURN_DRIVING определён в Furn.h (не Vehicle.h!)
- Ночью всегда дождь (hardcoded: `if (!(NIGHT_FLAG_DAYTIME)) GAME_FLAGS |= GF_RAINING`)
- fn_cop_normal() = #if 0 в пре-релизе
- fn_thug_init() = ASSERT(0) в пре-релизе
- Мотоцикл (BIKE) = НЕ переносить (не вошёл в финал)
- Zipwire (тросы) = ПЕРЕНОСИТЬ (есть в финале, первый уровень)
- Гарпун = НЕ переносить (never shipped)
- Канализация = НЕ переносить (never shipped)
- process_controls() = НЕ обработка ввода игрока (это диспетчер подсистем!)
- InputDone: накапливает обработанные биты input, сбрасывает при отпускании — кнопка срабатывает один раз
- player_apply_move() → только поворот (STATE machine); бег/прыжок/action — в apply_button_input()
- player_turn_left_right(): wMaxTurn=94 idle / 12 в воздухе / (70-Velocity) при беге; клавиатура=накопительный, стик=пропорциональный
- apply_button_input_fight(): MOVE без FORWARDS = выход в RUN (в аналоговом режиме отключено)
- should_i_jump(): dx/dz захардкожены (не от угла персонажа!) — проверяет WARE_which_contains по 4 точкам
- Горячие клавиши PC: KB_1=убрать, KB_2=пистолет, KB_3=шотган, KB_4=AK47, KB_5=граната, KB_6=C4, KB_7=нож, KB_8=бита
- NOISE_TOLERANCE: PC=8192 (из 65535), DC=24 (из 128); ANALOGUE_MIN_VELOCITY: PC=8, PSX=32 (из 128)
- audio.md: 14 MUSIC_MODE_*, приоритет: CINEMATIC>FIGHT>DRIVING>SPRINT>CRAWL>AMBIENT; замена MSS32→miniaudio
- math_utils.md: PSX углы 0-2047, SinTable[2560] (не 2048!), CosTable = &SinTable[512]; PC float radians+glm
- TIMEOUT_DEMO=0 → demo_timeout() = мёртвый код
- SPECIAL_MINE подбор = ASSERT(0), метание = ASSERT(0)
- save slots: 1-based! (`save_slot = menu_state.selected + 1`)
- complete_point диапазон 0-24+
- CRIME_RATE: если 0 → дефолт 50
- Thing.cpp per-frame: StateFn → general_process_person → PCOM_process_person (для CLASS_PERSON)
- people_functions[]: Darci→darci_states, Roper→roper_states, ВСЕ остальные (Cop,Thug,MIB...) → cop_states
- darci_states: только 7 записей; STATE_NORMAL=NULL (игрок), прыжки/бой через generic_people_functions
- plant_feet() активно ТОЛЬКО в fn_person_dangling (SUB_STATE_DROP_DOWN_LAND, финальная анимация)
- В fn_person_jumping plant_feet() везде закомментирована — Y задаётся из PAP/projectile_move_thing
- set_person_drop_down() флаги: KEEP_VEL, KEEP_DY, QUEUED, OFF_FACE; DY=-(4<<8)=-1024, Vel=-8 if not KEEP
- JUMPING→LAND последовательность: RUNNING_JUMP → RUNNING_JUMP_FLY → (hit==1) → RUNNING_JUMP_LAND_FAST → STATE_MOVEING+RUNNING
- EWAY_COND_PRESSURE = always FALSE (stub), EWAY_COND_CAMERA = always FALSE (stub)
- PSX EWAY_process: step=2 (odd/even frames), PC: step=1 (all waypoints each frame)
- EWAY_STAY_ALWAYS → immediately EWAY_FLAG_DEAD (fire-once semantics)
- zone byte in EWAY_create_enemy: bits 0-3=zone, bit4=INVULNERABLE, bit5=GUILTY, bit6=FAKE_WANDER
- Roper stats bug (пре-релиз): EWAY_create_player(ROPER) использует Darci stats
- EWAY вэйпойнт coords = mapsquare units (×256 для WorldPos)
- cam.cpp = МЁРТВЫЙ КОД (`#ifdef DOG_POO` никогда не определён); FC only
- FC_alter_for_pos idle height check (`&& 0`) = МЁРТВЫЙ КОД — always dheight=0, ddist=256
- FC_process: 8-шаговый raycast collision (want→focus); push xforce/yforce/zforce<<4 к want_pos
- FC get-behind speed: vehicle>>5; entering>>3; у стены>>5+>>6; нормально>>3; gun-out добавляет >>4
- FC toonear_dist=0x90000 = first-person mode (специальный случай)
- Lens = всегда 0x28000*CAM_MORE_IN (FOV не меняется, не зависит от action)
- Per-frame game_loop: GAMEMENU→tutorial→controls→process_things→EWAY→FC→draw→OVERLAY→flip→lock_fps→sfx→GAME_TURN++
- Bench cooldown: GAME_TURN & 0x3ff == 314 → сброс GF_DISABLE_BENCH_HEALTH (~34с при 30fps)
- SMOOTH_TICK_RATIO = скользящее среднее TICK_RATIO по 4 кадрам (для машин)
- DarciDeadCivWarnings: 0/1/2=предупреждения (экран deadcivs.tga); >=3=GS_LEVEL_LOST; персистирует!
