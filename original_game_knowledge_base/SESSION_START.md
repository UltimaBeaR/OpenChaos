# SESSION START — Urban Chaos KB Быстрый Ориентир

**Правило старта сессии:** Читать ТОЛЬКО этот файл + CLAUDE.md. Остальные файлы KB читать точечно по мере необходимости.
**Если задача касается подсистемы X → читать subsystems/X.md перед работой.**

---

## Статус фазы анализа

**Фаза 1 (текущая):** Детальный анализ оригинального кода → запись в `original_game_knowledge_base/`
- KB написана примерно на 75%
- Исходники аннотированы примерно на 40% (приоритетные файлы)

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
| **Камера** | camera.md | ⚠️ Не читал | fc.cpp, cam.cpp — нужно прочесть |
| **Звук** | audio.md | ⚠️ Не читал | Miles Sound System → заменить |
| **UI** | ui.md | ⚠️ Не читал | HUD, инвентарь |
| **Прогресс/сохранения** | player_progress.md | ⚠️ Частично | В missions.md: .wag формат, complete_point |
| **Матем/утилиты** | math_utils.md | ⚠️ Не читал | maths.cpp, Matrix.cpp |
| **Игровые состояния** | game_states.md | ⚠️ Не читал | GS_*, pause, win/lose flow |
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
| **collide.cpp** | 0 | ❌ ПРИОРИТЕТ 1 |
| **Person.cpp** | 0 | ❌ ПРИОРИТЕТ 2 |
| **eway.cpp** | 0 | ❌ ПРИОРИТЕТ 3 |
| **Mission.cpp** | 0 | ❌ ПРИОРИТЕТ 4 |
| **interfac.cpp** | 0 | ❌ ПРИОРИТЕТ 5 |

---

## TODO по подсистемам — что ещё проверить

### ФИЗИКА (physics.md) — TODO
- [ ] `find_face_for_this_pos()` — точная логика выбора face когда несколько перекрываются (критично для movement feel)
- [ ] `slide_along()` детали — как именно проецируется движение вдоль нормали, точная формула
- [ ] `height_above_anything()` — как определяется "приземление" при падении
- [ ] `collide_against_things()` + `collide_against_objects()` — логика коллизий Person-Person и Person-Furniture
- [ ] Лестницы в коллизиях: `mount_ladder()` — точное поведение
- [ ] Коллизии транспорта с персонажами: `VEH_find_runover_things()` детали
- [ ] Воды: как `PAP_FLAG_WATER` влияет на физику, есть ли замедление
- [ ] RWMOVE система (move_thing для не-person объектов)
- [ ] **АННОТИРОВАТЬ** collide.cpp — приоритет 1

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
- [ ] Точные переходы состояний в Darci: JUMPING→LANDING тайминг и условия
- [ ] Как работает `InterruptFrame` в DrawTween (приоритетный кадр)
- [ ] CMatrix33 декомпрессия — Pad encoding, UCA_Lookup детали (в Anim.cpp есть комменты)
- [ ] Roper финальное поведение — fn_roper_normal() пуста в пре-релизе, что в финале?
- [ ] Cop финальное поведение — fn_cop_normal() в #if 0, что в финале?
- [ ] Thug инициализация — ASSERT(0) в пре-релизе, финальная версия?
- [ ] **АННОТИРОВАТЬ** Person.cpp — приоритет 2

### НАВИГАЦИЯ (navigation.md) — TODO
- [ ] `WARE_mav_*()` детали — навигация внутри складов (отдельный граф?)
- [ ] `INSIDE2_mav_*()` детали — навигация в зданиях типа полицейский участок
- [ ] Как MAV обновляется при изменении уровня (взрыв стены, открытая дверь)?
- [ ] Детали MAV_can_i_walk — LOS для навигации
- [ ] Что происходит с навигацией когда NPC на крыше?

### МИССИИ/EWAY (missions.md) — TODO
- [ ] Точный binary layout .ucm файла — 14+58 байт на EP, но exact field mapping нужен
- [ ] Порядок разрешения зависимостей EventPoints — приоритеты, циклические зависимости
- [ ] Как EWAY_process() итерирует по EP — по порядку создания? по linked list?
- [ ] WPT_GOTHERE_DOTHIS (39) — как именно NPC получает задание
- [ ] WPT_GROUP_LIFE / GROUP_DEATH (33/34) — как именно работает активация группы
- [ ] EWAY_COND_COUNTER_GTEQ (25) — как счётчики инкрементируются (WPT_INCREMENT)
- [ ] **АННОТИРОВАТЬ** eway.cpp — приоритет 3
- [ ] **АННОТИРОВАТЬ** Mission.cpp — приоритет 4

### УПРАВЛЕНИЕ/ВВОД (controls.md) — TODO
- [ ] Детали `process_hardware_level_input_for_player()` — точный порядок обработки
- [ ] Как работает аналоговый стик для движения — thresholds, dead zone
- [ ] Double-click 200ms — GetTickCount(), проверить точную реализацию
- [ ] `player_apply_move()` — как направление конвертируется в движение с учётом камеры
- [ ] **АННОТИРОВАТЬ** interfac.cpp — приоритет 5

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
- [ ] **camera.md** — как именно работает follow camera, collision avoidance
- [ ] **audio.md** — звуковые events, Miles Sound System интеграция
- [ ] **ui.md** — HUD layout, инвентарь popup, crime/danger UI
- [ ] **math_utils.md** — CMatrix33, quaternion slerp, QDIST2 формулы
- [ ] **game_states.md** — GS_* flow, pause, win/lose, GS_LEVEL_LOST условия

---

## Быстрые критичные факты (не забыть!)

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
- TIMEOUT_DEMO=0 → demo_timeout() = мёртвый код
- SPECIAL_MINE подбор = ASSERT(0), метание = ASSERT(0)
- save slots: 1-based! (`save_slot = menu_state.selected + 1`)
- complete_point диапазон 0-24+
- CRIME_RATE: если 0 → дефолт 50
