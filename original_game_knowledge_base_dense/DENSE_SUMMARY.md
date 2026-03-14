# DENSE_SUMMARY — Urban Chaos (Полная компактная база знаний)

Компактная версия всей KB. Содержит числа, формулы, зависимости.
Используй когда нужно несколько подсистем сразу.
Актуально для: PC (Steam) + PS1 релизные версии. Кодовая база — пре-релиз.

---

## 1. Архитектура и структура кода

**Язык:** C (C-стиль в .cpp, без наследования), ~500K+ строк, ~350+ .cpp, ~150+ .h.
**Entry point:** `Main.cpp` → `WinMain()` → `game()` (Game.cpp, ~53K строк).

**Main loop:**
```c
game_startup()
while (SHELL_ACTIVE && GAME_STATE) {
    if (GS_ATTRACT_MODE) game_attract_mode()   // меню
    if (GS_PLAY_GAME)    game_loop()            // 30 FPS render, 15 FPS logic
}
game_shutdown()
```

**Тайминг:** 30 FPS рендеринг, 15 FPS логика, динамический TICK_RATIO.

**Ключевые файлы:**

| Компонент | Файлы | Размер |
|-----------|-------|--------|
| Игровой цикл | Game.cpp | ~53K |
| Миссии | Mission.cpp | ~43K |
| Оружие/предметы | Special.cpp | ~41K |
| DDManager (Direct3D) | DDLibrary/DDManager.cpp | ~49K |
| DIManager (DirectInput) | DDLibrary/DIManager.cpp | ~62K |
| Персонажи/AI | Person.cpp, ai-*.cpp | — |
| Физика/коллизии | collide.cpp | — |
| Рендеринг | DDEngine/ (~60 .cpp), aeng.cpp (~16K) | — |

**Пропускаются полностью:**
- `/fallen/Editor/`, `/fallen/GEdit/`, `/fallen/LeEdit/`, `/fallen/SeEdit/` — редакторы
- `/fallen/PSXENG/` — PSX рендеринг (исключение: psxlib/GHost.cpp — controls задокументированы)
- `/fallen/Glide Engine/` — 3dfx Glide, мёртвый код
- `MFLib1/`, `MFStdLib/`, `MuckyBasic/`, `thrust/` — не игровой код

**⚠️ ВАЖНО:** DDEngine может мутировать game state. Пример: `drawxtra.cpp` мутирует ammo, создаёт эффекты.

---

## 2. Thing System

**Thing struct:** ~125 байт, базовый объект для ВСЕГО в игре.

**15 классов:** CLASS_PLAYER, CLASS_PERSON, CLASS_VEHICLE, CLASS_PROJECTILE, CLASS_ANIMAL, CLASS_BUILDING, CLASS_SPECIAL, CLASS_SWITCH, CLASS_TRACK, CLASS_PLAT, CLASS_CHOPPER, CLASS_PYRO, CLASS_BARREL, CLASS_BAT, CLASS_CAMERA.

**Полиморфизм:** `void (*StateFn)(Thing*)` + `union Genus` (Vehicle/Furniture/Person/Animal/Chopper/Pyro/Projectile/Player/Special/Switch/Track/Plat/Barrel/Bike/Bat).

**Ключевые поля:**
- `GameCoord WorldPos` (32-bit SLONG на ось), 256 юнитов = 1 mapsquare
- `SWORD Velocity`, `DY` (вертикальная скорость), `OnFace` (на какой поверхности)
- `Parent/Child` (иерархия, используется редко), `LinkParent/LinkChild` (управление памятью)

**Лимиты:**
- PRIMARY_THINGS: **400** (PC)
- SECONDARY_THINGS: **300** (PC) / 50 (PSX)
- Итого MAX_THINGS: **700**

**State Machine:**
- 37 состояний (STATE_NORMAL, STATE_FALLING, STATE_DYING и т.д.)
- 20+ подсостояний (SUB_STATE_WALKING, SUB_STATE_RUNNING, SUB_STATE_JUMPING и т.д.)
- `StateFn(Thing)` вызывается каждый кадр

**MapWho — пространственный хэш:**
- `MapElement[128×128]` с head индексом linked list объектов в ячейке
- Поиск: `THING_find_sphere()`, `THING_find_box()`, `THING_find_nearest()`
- **Обязательно** вызывать `move_thing_on_map()` при перемещении — иначе рассинхронизируется

**Специализированные подтипы:**

| Подтип | Параметры | Файл |
|--------|-----------|------|
| **Barrel** | 2-sphere rigid body, BARREL_SPHERE_DIST=50; 4 типа (NORMAL, CONE, BURNING, BIN); макс 300 | barrel.cpp |
| **Platform** | Waypoint-routes (PLAT_STATE_GOTO/PAUSE), smooth accel/decel, max 8 nearby persons | plat.cpp |
| **Chopper** | Hardwired radius=8192, detection=1024, PRIM_OBJ_CHOPPER=74/75 | chopper.cpp |

---

## 3. Персонажи

**15 типов (PERSON_TYPE_*) с ANIM_TYPE:**

| ID | Name | Health | AnimType | MeshID | Статус |
|----|------|--------|----------|--------|--------|
| 0 | Darci | **200** | DARCI(0) | 0 | Полная |
| 1 | Roper | **400** | ROPER(2) | 0 | `fn_roper_normal()` пуста |
| 2 | Cop | 200 | CIV(1) | 4 | `#if 0` (закомм.) |
| 3 | CIV | 130 | CIV(1) | 7 | через PCOM |
| 4–6 | Thug (RASTA/GREY/RED) | 200 | CIV(1) | 0–2 | `fn_thug_init() ASSERT(0)` |
| 7–11 | Slag/Hostage/Mechanic/Tramp | 200 | DARCI/CIV | 1–3 | через PCOM |
| 12–14 | MIB 1/2/3 | **700** | CIV(1) | 5 | через PCOM |

**DrawTween — управление анимацией:**
```
TweakSpeed (0–255), FrameIndex, QueuedFrameIndex
Angle, AngleTo, Roll, Tilt + targets
CurrentAnim, AnimTween, TweenStage(0–256)
CurrentFrame, NextFrame, QueuedFrame
TheChunk (GameKeyFrameChunk)
MeshID (модель), PersonID (текстура/лицо, до 10 вариантов)
```

**Анимационная система (Vertex Morphing + Tweening, НЕ скелетная):**
- **GameKeyFrame:** XYZIndex, TweenStep, FirstElement (цепь костей)
- **GameKeyFrameElement:** CMatrix33 (сжатая 3×3), OffsetX/Y/Z, Next/Parent
- **CMatrix33:** M[3] упакованных, 10-bit элементы, range -512..511
- Декомпрессия: `UCA_Lookup[a][b]=sqrt(16383-a²-b²)`

**Флаги Person (32-bit bitmask):**
- GUN_OUT(1<<3), DRIVING(1<<4), SLIDING(1<<5), GRAPPLING(1<<13)
- HELPLESS(1<<14), CANNING(1<<15), BARRELING(1<<27), KO(1<<29)
- WAREHOUSE(1<<30), BIKING(1<<19), PASSENGER(1<<20)

**Режимы скорости:** RUN(0), WALK(1), SNEAK(2), FIGHT(3), SPRINT(4)

**Darci физика (Darcy.cpp:projectile_move_thing):**
- Гравитация: GRAVITY = 1024 units/tick
- Терминал: DY = -30000 (hard clamp)
- Fall damage: `dmg = (-DY - 20000) / 100` (линейная)
  - DY ≤ -30000 → 250 (instant kill), DY ≥ -20000 → 0
- Plunge-анимация: player DY < -12000, NPC < -6000, lookahead 3 шага
- Velocity scaling: `(vel*3)>>2` (×0.75)

**Типы смерти:** COMBAT(1), LAND(2), OTHER(3), LEG_SWEEP(5), SHOT_PISTOL(10), SHOT_SHOTGUN(11), SHOT_AK47(12)

---

## 4. Физика и коллизии

**Гравитация:**
```c
#define GRAVITY (-(128 * 10 * 256) / (80 * 80))  // = -51 юн/тик²
// TICKS_PER_SECOND = 80 (20 * TICK_LOOP)
// Персонажи: animation-driven, НЕ явная гравитация
// Транспорт: VelY += GRAVITY каждый тик
```

**Тайминг:**
- Базовый кадр: **66.67 мс** (15 FPS логики)
- TICK_RATIO: динамический (256 = 1.0x), range 0.5x–2.0x (скользящая средняя 4 кадра)
- Frame rate cap: **30 FPS** (config.ini), spin-loop busy-wait

**move_thing — точный порядок:**
1. Граница карты: выход → return 0
2. `collide_against_things()` — Things в сфере (radius+0x1a0), только CLASS_PERSON на PRIM
3. `collide_against_objects()` — уличные OB (фонари, урны), PRIM_COLLIDE_BOX → `slide_along_prim()`
4. `slide_along()` — DFacets зданий (extra_wall_height=-0x50)
5. `slide_along_edges()` / `slide_along_redges()` — кромки (STATE_CIRCLING, STEP_FORWARD, FIRE_ESCAPE)
6. `find_face_for_this_pos()` — новый face; y_diff>0x50 → FALL_OFF
7. Fall-through workaround: `x2+= dx>>2; y2+= dy>>2; z2+= dz>>2`
8. Побочные: `DIRT_gust()`, `MIST_gust()`, `BARREL_hit_with_sphere()`
9. `move_thing_on_map()` — MapWho sync

**slide_along() — алгоритм (NO BOUNCE, NO FRICTION):**
```c
slide_along(x1, y1, z1, *x2, *y2, *z2, extra_wall_height, radius, flags)
```
- NOGO буфер: 0x5800 (~22 ед.)
- Фильтр Y: `y_top += extra_wall_height` (стена "ниже" → перешагивание на -0x50)
- ВСЕ DFacets оси-aligned: вдоль X (push по Z), вдоль Z (push по X)
- Circle formula: `*pos = endpoint + dp * (radius>>8) / (dist>>8 + 1)`
- Глобальные выходы: `actual_sliding`, `last_slide_colvect`, `slide_door`, `slide_ladder`

**find_face_for_this_pos() (walkable.cpp):**
- Алгоритм: быстрый путь (roof first) → поиск lo-res ±0x200 → `is_thing_on_this_quad()` → dy-check
- Пороги: dy ≤ **0xa0** (160 ед.) = кандидат; fallback 0x50 (80 ед.) → GRAB_FLOOR
- Отрицательный index = roof face (`roof_faces4[]`)
- Return: face index, GRAB_FLOOR, или 0 (падение)

**plant_feet():**
- Игнорирует X/Z смещение анимированной ноги (`fx=0; fz=0`)
- Returns: -1 (GRAB_FLOOR), 0 (падение), 1 (snap к face)

**Коллизионные структуры:**
```
CollisionVect[10000 PC / 1000 PSX]   // Линейные отрезки-барьеры
CollisionVectLink[10000/4000]         // Linked list → lo-res ячейки
WalkLink[30000/10000]                 // lo-res → список walkable faces
```

**Высота (5 вариантов):**
1. `PAP_calc_height_at_point()` — высота угловой точки, no interpolation
2. `PAP_calc_height_at()` — билинейная интерполяция между 4 углами
3. `PAP_calc_height_at_thing()` — с контекстом (WARE/NS vs PAP)
4. `PAP_calc_map_height_at()` — max(terrain, buildings)
5. `PAP_calc_height_noroads()` — без бордюров

**HyperMatter (hm.cpp) — spring-lattice физика мебели:**
- 3D решётка масс-точек + пружины, Euler integration, 13 пружин/точка
- **gy=0 hardcoded** — объекты отскакивают от Y=0, не от terrain!
- Stationary threshold=**0.25** → "засыпание" объекта

**WMOVE — движущиеся ходимые поверхности (wmove.cpp):**
- Виртуальный walkable face (FACE_FLAG_WMOVE) для каждого транспорта/платформы
- Лимит: **WMOVE_MAX_FACES=192**
- Faces per object: CLASS_PERSON/PLAT=1, VAN/AMBULANCE=1, JEEP/MEATWAGON=4, CAR/TAXI=5
- Жизненный цикл: `WMOVE_create()` → `WMOVE_process()` → `WMOVE_remove()`

---

## 5. AI система PCOM

**Типы AI (PCOM_AI_*, 22 типа, 0–21):**
`NONE` `CIV` `GUARD` `ASSASIN` `BOSS` `COP` `GANG` `DOORMAN` `BODYGUARD` `DRIVER` `BDISPOSER` `BIKER`(#ifdef) `FIGHT_TEST` `BULLY` `COP_DRIVER` `SUICIDE` `FLEE_PLAYER` `KILL_COLOUR` `MIB` `BANE` `HYPOCHONDRIA` `SHOOT_DEAD`

**Состояния AI (PCOM_AI_STATE_*, 28 состояний, 0–27):**
`PLAYER` `NORMAL` `INVESTIGATING` `SEARCHING`(stub) `KILLING` `SLEEPING`(stub) `FLEE_PLACE` `FLEE_PERSON` `FOLLOWING` `NAVTOKILL` `HOMESICK` `LOOKAROUND` `FINDCAR` `BDEACTIVATE` `LEAVECAR` `SNIPE` `WARM_HANDS` `FINDBIKE`(#ifdef) `KNOCKEDOUT` `TAUNT` `ARREST` `TALK` `GRAPPLED` `HITCH` `AIMLESS` `HANDS_UP` `SUMMON` `GETITEM`

**Константы:**
```
PCOM_TICKS_PER_SEC = 320   // 16 тиков/фрейм × 20 фреймов/сек
PCOM_ARRIVE_DIST = 0x40    // 64 units
RMAX_PEOPLE = 180           // макс NPC в сцене
PCOM_MAX_GANGS = 16
PCOM_MAX_GANG_PEOPLE = 64
```

**Восприятие (видимость):**
- Дальность: NPC default = `0x800` (2048 units), игрок = `0x10000` (65536)
- Освещение: `view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256`
- Приседание: range ÷2 · Движение цели: range +256
- FOV: близко (<192 units) = **700** (~123°), далеко = **420** (~74°)
- Eye height: стоя = 0x60 (96), приседая = 0x20 (32)

**Звуки (world-units распространение):**
- 0x280 (640) — шаг
- 0x300 (768) — мина, граната
- 0x600 (1536) — unusual (банка колы)
- 0x700 (1792) — выстрел-bang
- 0x800 (2048) — тревога
- **0xa00 (2560) — выстрел (самый громкий)**

**MIB специфика:**
- Проверяет can_a_see_b(Darci) **каждый фрейм** в NORMAL state
- PCOM_alert_nearby_mib_to_attack(): radius=1280
- Health=700, FLAG2_PERSON_INVULNERABLE (KO невозможен)

**Bane специфика:**
- Всегда в SUMMON state, поднимает до 4 тел в воздух
- Электрошок: если Darci в 0x60000 (~384 units) и стоит 2 сек → -25 Health

**⚠️ Пре-релизный баг (воскрешение гражданских):**
- Условие: PCOM_AI_CIV + PCOM_MOVE_WANDER + !IN_VIEW + counter > 200
- Teleport на HomeX/HomeZ вычисляется, но **НЕ применяется** → воскресают на месте смерти

---

## 6. Навигация (MAV + Wallhug)

**MAV — основная система (mav.cpp):**
- Greedy best-first поиск, **НЕ A\*** (нет g-cost, только эвристика расстояния)
- Сетка: **128×128 ячеек**, горизонт поиска: **32 ячейки**
- Выход: первое MAV_Action; до 1024 записей MAV_opt[], 4 байта/запись

**MAV_Action (действия):**
```
0=GOTO · 1=JUMP · 2=JUMPPULL (1 блок) · 3=JUMPPULL2 (2 блока)
4=PULLUP · 5=CLIMB_OVER · 6=FALL_OFF · 7=LADDER_UP
```

**MAV_precalculate:**
- Для каждого из 4 направлений вычисляет MAV_CAPS_* (8 флагов возможностей)
- Правило: dh=0–2 → GOTO; 2–4 → PULLUP; 1–2 → JUMP; 2–5 → JUMPPULL; >5 → JUMPPULL2

**MAV_can_i_walk() — path shortcutting:**
- Bresenham от (ax,az) к (bx,bz), проверяет `MAV_CAPS_GOTO` при пересечении границ
- Диагональный случай: проверяет +2 угловые ячейки

**Динамическое обновление (двери):**
- `MAV_turn_movement_off()` — очищает только бит GOTO
- `MAV_turn_movement_on()` — **заменяет** весь набор caps на GOTO (другие caps теряются!)

**Wallhug алгоритм (Wallhug.cpp, 644 строк):**
- 2D сетка 128×128, 4 кардинальных направления
- До 252 waypoints в пути
- `wallhug_hugstep()`: левая/правая рука, обход стены по Bresenham
- Отпускание стены: 4 условия (цель не за стеной, смотрит на цель, ближе, не на старте)
- `wallhug_cleanup()`: LOS-оптимизация (макс 4 шага), реоптимизация по парам

**⚠️ Пре-релизные баги INSIDE2:**
- `INSIDE2_mav_inside()` → **ASSERT(0)** (заглушка, NPC не ходит внутри зданий)
- `INSIDE2_mav_stair/exit()` → баг `> 16` вместо `>> 16` (возвращает текущую позицию)

---

## 7. Система координат и карта

**PAP_Hi (128×128, ячейка = 256 ед. мира, PAP_SHIFT_HI=8):**
- `Texture` (UWORD), `Flags` (UWORD), `Alt` (SBYTE, реальная = Alt × 8)
- Флаги: SHADOW_1/2/3, REFLECTIVE, HIDDEN, SINK_SQUARE/POINT, ANIM_TMAP, ROOF_EXISTS, ZONE1–4, WANDER, FLAT_ROOF, WATER
- Alt диапазон: 0–127 → реальная 0–1016

**PAP_Lo (32×32, ячейка = 1024 ед., PAP_SHIFT_LO=10):**
- `MapWho` (THING индекс), `Walkable` (face индекс), `ColVectHead` (коллизия), `water`, `Flag` (WAREHOUSE)

**Интерполяция высоты:** билинейная в PAP_Hi через треугольники при xfrac+zfrac < 0x100

**ROAD граф:**
- До **256 нодов** (ROAD_MAX_NODES), до 4 граничных (ROAD_MAX_EDGES)
- Смещение полосы: **ROAD_LANE = 0x100**
- Текстуры дорог PC: 323–356; PSX: 256–278
- Маркеры: `ROAD_is_road()`, `ROAD_is_middle()` (5×5 квадратов = надёжно внутри)
- Построение: сканирование Z/X-параллелей, рекурсивный split на пересечениях

**WAND_init():**
- Алгоритм: 5×5 окрестность, тротуар рядом с дорогой ИЛИ зебра (текстура 333–334)
- Исключение: OB_Info коллизия (PRIM_COLLIDE_BOX) y ≤ terrain+0x40 → снять WANDER

---

## 8. Здания и интерьеры

**21 STOREY_TYPE:** NORMAL, ROOF, WALL, ROOF_QUAD, FLOOR_POINTS, FIRE_ESCAPE, STAIRCASE, SKYLIGHT, **CABLE** (тросы, есть в финале!), FENCE, FENCE_BRICK, LADDER, FENCE_FLAT, TRENCH, JUST_COLLISION, PARTITION, **INSIDE**, DOOR, INSIDE_DOOR, OINSIDE, **OUTSIDE_DOOR**

**Иерархия:**
- `FBuilding` → `FStorey` (linked list) → `FWall` (linked list) → `FWindow`
- Runtime: `DBuilding` (до 1024) → `DFacet` (граней)
- `InsideStorey` — интерьеры: MinX/Z, MaxX/Z, InsideBlock, TexType, FacetStart/End

**DFacet (24+ байта):** FacetType, Height (UBYTE), x[2]/z[2], Y[2] (мировые), FacetFlags, StyleIndex, Building, DStorey, **Open** (дверь 0–255), Shake (забор), CutHole, Dfcache

**Двери:**
- До **4 одновременно** (DOOR_MAX_DOORS=4)
- Синхронизация: `DOOR_find()` → `FACET_FLAG_OPEN` → `MAV_turn_movement_on/off()` соседей
- Анимация: без FACET_FLAG_90DEGREE: Open 0→255 (180°); с флагом: 0→128 (90°), шаг +4/кадр

**Лестницы:**
- До 32 (STAIR_MAX_STAIRS), до 3 на этаж (STAIR_MAX_PER_STOREY)
- Алгоритм: Fisher-Yates перемешивание, AND bitset соседних этажей, scoring
- Структура: `flag` (UP|DOWN), `up`/`down` (целевой InsideStorey), x1/z1/x2/z2

**id.cpp рендеринг (процедурная геометрия):**
- Pipeline: `ID_set_outline()` → `ID_calculate_in_squares/points()` → `ID_generate_floorplan()`
- Layout scoring: ratio = max(dx,dz)/min(dx,dz) × 256, цель = 414 (золотое сечение)

**WARE (склады):**
- До 32 (WARE_MAX_WARES=32)
- До 4 дверей (out_x/z, in_x/z), bounding box, nav/height индексы
- Пулы: `WARE_nav[4096]`, `WARE_height[8192]`, `WARE_rooftex[4096]`

---

## 9. Миссии EWAY

**EventPoint struct (74 байта):** Colour, Group, WaypointType, Used (4), TriggeredBy, OnTrigger, Direction, Flags (4), EPRef, EPRefBool, AfterTimer (6), Data[10] (40), Radius, X, Y, Z, Next/Prev

**42 условия EWAY_COND_* (0–41):**
FALSE, TRUE, PROXIMITY, TRIPWIRE, **PRESSURE**(STUB), **CAMERA**(STUB), SWITCH, TIME, DEPENDENT, BOOL_AND/OR, COUNTDOWN, COUNTDOWN_SEE, PERSON_DEAD, PERSON_NEAR, CAMERA_AT, PLAYER_CUBE, A_SEE_B, GROUP_DEAD, HALF_DEAD, PERSON_USED, ITEM_HELD, RADIUS_USED, PRIM_DAMAGED, CONVERSE_END, COUNTER_GTEQ, PERSON_ARRESTED, PLAYER_CUBOID, KILLED_NOT_ARRESTED, CRIME_RATE_GTEQ/LTEQ, IS_MURDERER, PERSON_IN_VEHICLE, THING_RADIUS_DIR, SPECIFIC_ITEM_HELD, RANDOM, PLAYER_FIRED_GUN, PRIM_ACTIVATED, DARCI_GRABBED, PUNCHED_AND_KICKED, MOVE_RADIUS_DIR, AFTER_FIRST_GAMETURN

**52 EWAY_DO_* действия (ключевые):**
CREATE_PLAYER/ENEMIES/VEHICLE/ITEM/CREATURE/CAMERA/TARGET/MAP_EXIT, MESSAGE, SOUND_EFFECT, VISUAL_EFFECT, CUT_SCENE, TELEPORT, END_GAME_LOSE/WIN, SHOUT, ACTIVATE_PRIM, CREATE_TRAP, ADJUST_ENEMY, LINK_PLATFORM, CREATE_BOMB, BURN_PRIM, NAV_BEACON, SPOT_EFFECT, CREATE_BARREL, KILL_WAYPOINT, CREATE_TREASURE, GROUP_LIFE/DEATH, CONVERSATION, INCREMENT, DYNAMIC_LIGHT, **GOTHERE_DOTHIS**(ASSERT, не реализован), TRANSFER_PLAYER, AUTOSAVE, LOCK_VEHICLE, GROUP_RESET, COUNT_UP_TIMER, RESET_COUNTER, CREATE_MIST, ENEMY_FLAGS, STALL_CAR, MOVE_THING, SHAKE_CAMERA, SIGN, WAREFX, NO_FLOOR

**4 режима активации EWAY_STAY_*:** ALWAYS (одноразовый), WHILE (re-poll), WHILE_TIME (условие + N тиков), TIME (фиксированное время)

**Spawn детали:**
- Координаты вэйпойнта: mapsquare × 256 → world
- Yaw: << 3 при сохранении, >> 3 восстановление
- Zone byte: [0–3]=spawn zone, [4]=INVULNERABLE, [5]=GUILTY, [6]=FAKE_WANDER
- MAX 2 игрока (NO_PLAYERS ≤ 1 ASSERT)

**.ucm формат (binary, 1316 байт заголовок):**
- Header: version (4), flags (4), BriefName/LightMapName/MapName/MissionName/CitSezMapName (260×5), MapIndex (2), UsedEPoints (2), FreeEPoints (2), CrimeRate (1), FakeCivs (1)
- EventPoint цикл: 512 записей × 74 байта
- После: SkillLevels[254] skip (v>5), BOREDOM_RATE (1), FAKE_CARS (1, v>8), MUSIC_WORLD (1, v>8)
- **.ucm ≠ MuckyBasic** — это EventPoint данные, не VM скрипты

**EWAY_process() per-frame:** если EWAY_FLAG_ACTIVE → непрерывные эффекты (EMIT_STEAM, WATER_SPOUT, SHAKE_CAMERA, VISIBLE_COUNT_UP); COUNTDOWN decrement; иначе evaluate condition → activate; стоп при GS_LEVEL_LOST|WON

---

## 10. Транспорт

**9 типов:** VEH_TYPE_VAN(0), CAR(1), TAXI(2), POLICE(3), AMBULANCE(4), JEEP(5), MEATWAGON(6), SEDAN(7), WILDCATVAN(8)

**Подвеска (4 пружины):**
```c
struct Suspension { UWORD Compression; UWORD Length; };
MIN_COMPRESSION = 0x0D00 (13<<8)
MAX_COMPRESSION = 0x7300 (115<<8)
// При >50% компрессии → Smokin=TRUE
```

**6 зон повреждения (0–5):** FL, FR, LM, RM, LR, RR (4 уровня каждая)

**Параметры двигателя (разгон вперёд, назад, мягкий тормоз, жёсткий):**
- VAN: **17, 10, 4, 8**
- CAR: **21, 10, 4, 8**
- POLICE: **25, 15, 5, 10**
- AMBULANCE: **25, 10, 5, 8**

**Скоростные лимиты:**
- VEH_SPEED_LIMIT = **750** (макс. вперёд)
- VEH_REVERSE_SPEED = **300** (макс. назад)

**Friction формула:**
```
new_vel = ((vel << friction) - vel) >> friction  // = vel × (1 - 1/2^friction)
// Base friction=7; Engine braking -1; Hard braking -4
```

**Рулевое управление:**
- WHEELTIME = **35** кадров; WHEELRATIO = **45**
- SKID_START = **3** кадров; SKID_FORCE = **8500**

**Прочие константы:**
- UNITS_PER_METER = 128
- MAX_VEHICLES = **40**; MAX_RUNOVER = **8** персонажей

---

## 11. Управление и ввод

**INPUT_ кнопки (18):**
FORWARDS(0), BACKWARDS(1), LEFT(2), RIGHT(3), START(4), CANCEL(5), PUNCH(6), KICK(7), ACTION(8), JUMP(9), CAMERA(10), CAM_LEFT(11), CAM_RIGHT(12), CAM_BEHIND(13), MOVE(14), SELECT(15), STEP_LEFT(16), STEP_RIGHT(17)

**ACTION_ действия (52):**
- Движение: IDLE(0), WALK(1), RUN(2), WALK_BACK(34), CRAWLING(41), STOPPING(30), SKID(42), CROUTCH(40)
- Прыжки: STANDING_JUMP(3), STANDING_JUMP_GRAB(4), RUNNING_JUMP(5), DANGLING(6), PULL_UP(7), FALLING(11), LANDING(12), CLIMBING(13), DANGLING_CABLE(17), TRAVERSE_LEFT/RIGHT(36/37)
- Бой: FIGHT_IDLE(16), FIGHT_PUNCH(14), FIGHT_KICK(15), GRAPPLE(43), GRAPPLEE(44), HOP_BACK(18), SIDE_STEP(19), FLIP_LEFT/RIGHT(27/28), BLOCK_FLAG(39), PUNCH_FLAG(38), KICK_FLAG(32)
- Оружие: DRAW_SPECIAL(21), AIM_GUN(22), SHOOT(23), GUN_AWAY(24)
- Прочее: DYING(20), DEAD(26), DEATH_SLIDE(35), RESPAWN(25), ENTER_VEHICLE(45), INSIDE_VEHICLE(46), SIT_BENCH(47), HUG_WALL(48), HUG_LEFT/RIGHT(49/50), UNSIT(51), RUN_JUMP(31)

**Player struct ключевые поля:**
- `ULONG Input` — текущий ввод (bitmask); `LastInput`/`ThisInput`/`Pressed`/`Released`
- `UBYTE Stamina`; `Constitution` (защита); `Strength` (сила удара)
- `UBYTE RedMarks` (нарушения, 10 = game over); `TrafficViolations`; `Danger` (0–3)
- `SBYTE ItemFocus` (-1 = ничего); `UBYTE ItemCount`, `Skill`
- `SLONG LastReleased[16]` — для double-click; `UBYTE DoubleClick[16]`

---

## 12. Бой и оружие

### Бой (melee)

**GameFightCol — структура зон удара:**
- `Dist1` (внутр. граница), `Dist2` (внешняя), `Angle` (1/256 круга), `Priority`, `Damage`
- `Tween`, `AngleHitFrom`, `Height` (0=ноги, 1=торс, 2=голова), `Width`, `*Next`

**Попадание (check_combat_hit_with_person):**
1. QDIST2 таз-таз только XZ; `abs(dy) > 100` → промах
2. Угол: ±200 ед. из 2048-градусной системы
3. LOS-проверка
4. dist < Dist2 + 0x30

**Формула урона:**
```
damage = fight->Damage
// Тип атаки: KICK_NAD +50M/+10F; KICK_RIGHT/LEFT +30; BAT_HIT +20+taunt;
//            KNIFE1/2/3 =30/50/70; STOMP +50; FLYKICK +20+taunt
// Атакующий: Roper×2 (стрельба) или +20 (melee) + player.Strength/Skill
// Защита: vs player: damage>>=1; vs NPC: damage -= GET_SKILL(victim), min=2
```

**Fight tree (комбо):**
- Punch: COMBO1(10)→COMBO2(30)→COMBO3(60 KO)
- Kick: COMBO1(10)→COMBO2(30)→COMBO3(60 KO)
- Cross-комбо: 30, 80, 80 (nodes 11–13)
- Knife: 30→60→80 (nodes 14, 16, 18)
- Bat: 60→90 (nodes 19, 21)

**AI вероятность блока:**
```
base = 60 + GET_SKILL(person)*12
if attacker not visible: prob /= 2
cap = 150 + GET_SKILL(person)*5
block = Random() % cap < block_prob
```

**Grappling:**
- PISTOL_WHIP (Darci, dist=75), STRANGLE (dist=50), HEADBUTT (dist=65), GRAB_ARM (dist=60)
- Проверка: `|dist - pg->Dist| < pg->Range`, Y<50, жертва не в граплинге/KO
- SNAP_NECK → ANIM_DIE_KNECK

### Оружие

**SPECIAL_* типы (30):**
NONE, KEY, GUN, HEALTH, BOMB, SHOTGUN, KNIFE, EXPLOSIVES, GRENADE, AK47, MINE(заглушена), THERMODROID(не функц.), BASEBALLBAT, AMMO_PISTOL, AMMO_SHOTGUN, AMMO_AK47, KEYCARD, FILE, FLOPPY_DISK, CROWBAR, VIDEO, GLOVES, WEEDAWAY, TREASURE, CARKEY_RED/BLUE/GREEN/BLACK/WHITE, WIRE_CUTTER

**Магазины:**
- PISTOL: 15 | SHOTGUN: 8 | AK47: 30 | GRENADE: 3
- Стак max: гранаты 8, взрывчатка 4, резерв патронов 255 (UBYTE)

**Урон:**
- Пистолет: 70 HP
- Дробовик: `300 - dist` (saturated 0–250, range ~0x600)
- АК-47: игрок 100, NPC 40
- Все оружие vs игрок: урон >>= 1

**Скорострельность (тики между выстрелами):**
- Пистолет: 140 (~4.7с/30fps)
- Дробовик: 400 (~13.3с)
- АК-47: 64 (~2.1с)

**Взрывы:**
```
Type        Radius  Max DMG  Aggressor
Mine        0x200   250      null
Grenade     0x300   500      thrower
C4/Explos.  0x500   500      placer (таймер: 1600 тиков @ 20fps)
Formula: hitpoints = maxdmg × (radius - dist) / radius
```

**Гранаты (grenade.cpp):**
- Активация: `timer = 1920 тиков` (**6 сек**)
- Гравитация: `dy -= TICK_RATIO × 2`, терминал -0x2000
- Отскок: `dy = abs(dy) >> 1`, если dy < 0x400 → остановка

**Автоприцеливание (guns.cpp):**
- Дальность (блоки): MIB 8, Pistol 7, AK47 8, Shotgun 6
- Конус: MAX_DANGLE = 2048/7 ≈ 292 (~±51°), при беге ×4 уже
- Hit chance: `base = 230 - abs(Roll)>>1 - Velocity + модификаторы`, min=20/256 (~8%)
- Scoring: `(8<<8 - dist) × (MAX_DANGLE - dangle) >> 3`

**Банки колы (DIRT система):**
- DIRT типы: LEAF(1), CAN(2), THROWCAN(6), BRASS(10), BLOOD(14)
- Подбор (только Darci): `STATE_CANNING=32`, `FLAG_PERSON_CANNING=(1<<15)`
- Бросок: `dy = power >> 2` (вверх)
- Звук при ударе: `S_KICK_CAN`, alert NPC = `PCOM_SOUND_UNUSUAL`

---

→ Продолжение: [DENSE_SUMMARY_2.md](DENSE_SUMMARY_2.md)
