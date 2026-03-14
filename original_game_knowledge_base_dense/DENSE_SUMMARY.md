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

## 13. Взаимодействие

**find_grab_face() — 2-pass алгоритм (interact.cpp):**
- Pass 0: hi-res (крыши), Pass 1: lo-res (terrain)
- Для каждого: проверка 4 рёбер квада, Y-диапазон ±256+dy, угол ±200°, расстояние < radius, не через забор
- Типы: 0=surface, 1=cable, 2=ladder

**Cable/Zipwire (параметры в DFacet):**
- `StyleIndex` (SWORD) = angle_step1 (кривизна сег1)
- `Building` (SWORD) = angle_step2 (кривизна сег2)
- `Height` (UBYTE) = count (кол-во шагов)
- `find_cable_y_along()`: 2 сегмента, косинусная кривая провисания
- `check_grab_cable_facet()`: ближайшая точка на тросе + расчёт провисания

**Ladder (interact.cpp:check_grab_ladder_facet):**
- Расстояние до центральной линии
- Y-диапазон: `bottom ≤ Y ≤ top + 64`
- Return -1: выше лестницы; Return 1: валидная позиция
- ⚠️ `mount_ladder()` из interfac.cpp закомментирован в пре-релизе

---

## 14. Player States

**Главные состояния (STATE_*):**
INIT(0), NORMAL(1), MOVEING(5), IDLE(6), LANDING(7), JUMPING(8), FIGHTING(9), FALLING(10), USE_SCENERY(11), DOWN(12), HIT(13), CHANGE_LOCATION(14), DYING(16), DEAD(17), DANGLING(18), CLIMB_LADDER(19), CLIMBING(21), GUN(22), SHOOT(23), DRIVING(24), NAVIGATING(25), WAIT(26), GRAPPLING(30), CANNING(32), CIRCLING(33), HUG_WALL(34), SEARCH(35), CARRY(36), FLOAT(37)

**Подсостояния движения (1–27):**
WALKING(1), RUNNING(2), SIDELING(3), WALKING_BACKWARDS(16), RUNNING_SKID_STOP(17), CRAWLING(19), SNEAKING(20), SLIPPING(22), RUNNING_VAULT(25), RUNNING_HIT_WALL(27)

**Подсостояния прыжков (30–41):**
STANDING_JUMP(30), STANDING_GRAB_JUMP(31), RUNNING_JUMP(32), RUNNING_JUMP_FLY(33), RUNNING_GRAB_JUMP(34), RUNNING_JUMP_LAND(35), STANDING_JUMP_FORWARDS(38), STANDING_JUMP_BACKWARDS(39), FLYING_KICK(40), FLYING_KICK_FALL(41)

**Подсостояния боя (90–105):**
PUNCH(90), KICK(91), BLOCK(92), GRAPPLE(93), GRAPPLEE(94), WALL_KICK(95), STEP(96), STEP_FORWARD(97), GRAPPLE_HOLD(98), GRAPPLE_HELD(99), ESCAPE(100), GRAPPLE_ATTACK(101), HEADBUTT(102), STRANGLE(103), HEADBUTTV(104), STRANGLEV(105)

**Подсостояния лазания (180–218):**
- Карниз: GRAB_TO_DANGLE(180), DANGLING(181), PULL_UP(182), DROP_DOWN(183), DROP_DOWN_LAND(184), DROP_DOWN_OFF_FACE(188)
- Зипвайр: DANGLING_CABLE(185), DANGLING_CABLE_FORWARD(186), DANGLING_CABLE_BACKWARD(187)
- Лестница: MOUNT(192), ON_LADDER(193), CLIMB_UP(194), CLIMB_DOWN(195), CLIMB_OFF_BOT(196), CLIMB_OFF_TOP(197)
- Стена: CLIMB_LANDING(210), CLIMB_AROUND(211), CLIMB_UP(212), CLIMB_DOWN(213), CLIMB_OVER(214), CLIMB_OFF_BOT(215), CLIMB_LEFT(217), CLIMB_RIGHT(218)

**Подсостояния транспорта (140–147) и оружия (220–225):**
- Транспорт: ENTERING(140), INSIDE(141), EXITING(142), MOUNTING_BIKE(145), RIDING_BIKE(146), DISMOUNTING_BIKE(147)
- Оружие: DRAW_GUN(220), AIM_GUN(221), SHOOT_GUN(222), GUN_AWAY(223), DRAW_ITEM(224), ITEM_AWAY(225)

**Смерть/арест (170–236):**
- Арест: TURN_OVER(170), CUFFED(171), ARRESTED(172), INJURED(173), RESPAWN(174)
- Смерть: DYING_INITIAL(230), DYING_FINAL(231), DYING_ACTUALLY_DIE(232), KNOCK_DOWN(233), GET_UP(234), DYING_PRONE(235), KNOCK_DOWN_WAIT(236)
- FLAG_PERSON_ARRESTED (бит 26); мёртвый → STATE_DEAD (необратимо)

---

## 15. Игровой цикл и время

**GS_* флаги:**
- `GS_ATTRACT_MODE (1<<0)` — меню
- `GS_PLAY_GAME (1<<1)` — игра
- `GS_LEVEL_WON (1<<3)`, `GS_LEVEL_LOST (1<<4)` — окончание
- `GS_GAME_WON (1<<5)` — кампания завершена
- `GS_REPLAY (1<<8)` — рестарт уровня

**Inner loop порядок (game_loop):**
1. GAMEMENU_process()
2. GS_LEVEL_WON проверка (Finale1.ucm → break)
3. EWAY_tutorial_string (should_i_process_game=FALSE)
4. special_keys() — отладка
5. process_controls()
6. check_pows()
7. if(should_i_process_game): process_things(1), particles, doors, EWAY, FC_process()
8. PUDDLE/DRIP (всегда)
9. draw_screen() 3D
10. OVERLAY (HUD)
11. GAMEMENU_draw()
12. PANEL_fadeout_draw()
13. screen_flip()
14. lock_frame_rate(30)
15. handle_sfx()
16. GAME_TURN++ (глобальный счётчик)
17. Bench cooldown: каждые 1024 кадра → разрешить лечение

**Таймирование:**
- `TICK_RATIO` — масштаб от реального времени кадра
- `SMOOTH_TICK_RATIO` — 4-кадровое скользящее среднее (кольцевой буфер)
- `lock_frame_rate()` — spin-loop к 30 FPS (из config.ini)

**Win/Lose flow:**
- Win → GS_LEVEL_WON, MUSIC_VICTORY
- Lose (живой) → GS_LEVEL_LOST, MUSIC_CHAOS
- Lose (мёртвый) → GS_LEVEL_LOST, MUSIC_GAMELOST
- GAMEMENU WON: {X_LEVEL_COMPLETE} (ENTER → DO_NEXT_LEVEL)
- GAMEMENU LOST: {X_LEVEL_LOST, X_RESTART, X_ABANDON}

**DarciDeadCivWarnings (RedMarks):**
- 0 → warning 1; 1 → warning 2; 2 → warning 3
- 3 → GS_LEVEL_LOST + warning 4
- Счётчик персистирует между миссиями

---

## 16. Камера

- **FC только:** cam.cpp мёртв (`#ifdef DOG_POO`), активна fc.cpp
- **FC_MAX_CAMS = 2** (поддержка splitscreen)

**4 режима камеры (PC):**
- 0: cam_dist=0x280×0.75≈480, cam_height=0x16000
- 1: cam_dist=0x280=640, cam_height=0x20000
- 2: cam_dist=0x380=896, cam_height=0x25000
- 3: cam_dist=0x300=768, cam_height=0x8000
- PSX тип 0: cam_dist=0x300, cam_height=0x18000 (дальше)
- **CAM_MORE_IN = 0.75F** — PC камера на 25% ближе к игроку

**Toonear режим:** toonear_dist=0x90000 (first-person при упоре в стену)

**FC_process (15 шагов):**
1. FC_alter_for_pos() → offset_height, offset_dist
2. FC_calc_focus() → focus_x/y/z, focus_yaw
3. Warehouse transition (EWAY_cam_jumped=10)
4. lookabove update (STATE_DEAD: −0x80/кадр; иначе 0xa000)
5. Rotate decay ±0x80/кадр; nobehind=0x2000 при rotate
6. Toonear cancel: если dist > toonear_dist+0x4000 AND dangle>200 → snap
7. Collision detection (8 шагов; WARE_inside для склада)
8. Get-behind: behind = focus + SIN/COS(focus_yaw)×cam_dist×offset_dist
9. Y adjust: dy в caps [−0x0c00, +0x0d00]; на WMOVE: dy<<5 (мгновенно)
10. Distance clamp
11. Position smoothing: если |delta|>0x800 → x+=dx>>2, y+=dy>>3, z+=dz>>2
12. FC_look_at_focus() — yaw/pitch/roll
13. Angle smoothing: если |total_delta|>0x180 → delta>>2
14. Lens = 0x28000×CAM_MORE_IN (всегда)
15. Shake: shake_x/y/z = random(shake)−shake/2 << 7; decay = shake−shake>>3

---

## 17. Звук

- **MSS32 API** (Miles Sound System, mss32.lib)
- `MFX_play(channel, wave_id, flags)` — 2D
- `MFX_play_xyz(channel, wave_id, flags, x>>8, y>>8, z>>8)` — 3D
- `MFX_update_xyz(channel, x>>8, y>>8, z>>8)` — обновить позицию

**14 режимов музыки (MUSIC_MODE_*):**
NONE, CRAWL, DRIVING1, DRIVING2, DRIVING_START, FIGHT1, FIGHT2, SPRINT1, SPRINT2, AMBIENT, CINEMATIC, VICTORY, FAIL, MENU

**5 биомов ambient (по texture_set):**
- Jungle (ts=1): тропический шум, какаду/сверчки
- Snow (ts=5): вой волков каждые 1500+ кадров
- Estate (ts=10): пролёты самолётов каждые 500+ кадров
- BusyCity (default): собаки, кошки, бьющееся стекло, полиция
- QuietCity (ts=16): минимальный ambient

**Каналы:**
- WIND_REF = MAX_THINGS+100, WEATHER_REF = +101, MUSIC_REF = +102
- FLAGS_HAS_ATTACHED_SOUND — звук привязан к объекту

**Голосовые функции:** SOUND_Gender(), DieSound(), PainSound(), EffortSound(), ScreamFallSound()

**Шаги:** FLAGS_PLAYED_FOOTSTEP (бит 12), по типу поверхности (PAP_Hi.Texture)

---

## 18. Эффекты

### Система частиц (psystem.cpp)
- **MAX_PARTICLES:** PC **2048**, PSX 64
- `PARTICLE_Add()`: позиция (32.8 fixed), скорость, page, size, ARGB colour, flags, gravity, drag

### Огонь (fire.cpp)
- **FIRE_MAX_FLAMES=256**, **FIRE_MAX_FIRE=8** очагов одновременно
- Жизнь: 32+(rand&0x1f) кадров
- Per-frame: нечётные angle+=31/offset-=17, чётные angle-=33/offset+=21

### Искры (spark.cpp)
- **MAX_SPARKS: 32**
- Типы: LIMB, CIRCULAR, GROUND, POINT
- Флаги: FAST(×4), SLOW(÷4), CLAMP_X/Y/Z, DART_ABOUT, STILL
- Звук: S_ELEC_START+(spark_id%5) — 5 вариантов

### POW система (pow.cpp) — sprite-based взрывы
- **POW_Sprite[256 PC]**, **POW_Pow[32 PC]**
- **8 типов:** BASIC_SPHERE/SPAWN_SPHERE/MULTISPAWN/MULTISPAWN_SEMI (3 размера)
- density=(5+dens×3) around × (4+dens) updown; framespeed=96+fs×32+(rand&0x3f)

### PYRO система (pyro.cpp) — 18 типов
| # | Тип | Детали |
|---|-----|--------|
| 4 | **IMMOLATE** | сожжение (**-10 HP/frame**, частицы на костях) |
| 7 | EXPLODE2 | большой взрыв (dlight+COS fade) |
| 11 | HITSPANG | вспышка при пуле (**5 кадров** PC/**3 PSX**) |
| 12 | FIREBOMB | огненный взрыв |
| 17 | **GAMEOVER** | конец света (**8 dlights**, GS_LEVEL_LOST@counter>242) |

- **MAX_PYROS:** **64** PC / **32** PSX
- `PYRO_blast_radius()`: sphere находит CLASS_PERSON, создаёт IMMOLATE
- `MergeSoundFX()`: переиспользует ближайший звук (sphere 5×256)

### DIRT система (dirt.cpp) — 16 типов ambient debris
| # | Тип | Примечание |
|---|-----|------------|
| 1 | LEAF | ambient, окружность камеры |
| 2 | CAN | pickup, S_KICK_CAN, PCOM_SOUND_UNUSUAL |
| 3 | PIGEON | **prob_pigeon=0** (никогда не создаётся!) |
| 10 | BRASS | гильзы (PC only), prim 253 |

- **MAX_DIRT:** **1024** PC / **256** DC / **128** PSX
- LEAF/SNOW: gravity 4<<TICK_SHIFT, drag 75%x/50%y
- CAN/HEAD/BRASS: rotation, gravity, friction/16
- `DIRT_gust()`: ветер от движения, strength=QDIST2(dx,dz)×8

### RIBBON система (ribbon.cpp) — ленточные следы
- **MAX_RIBBONS: 64**, **MAX_RIBBON_SIZE: 32** точек
- Флаги: USED, FADE, SLIDE, CONVECT(+22/frame вверх), IALPHA
- Рендеринг: triangle strip, alpha fade через FadePoint
- Используется: IMMOLATE, FLICKER

### BANG система (bang.cpp)
- **BANG_Bang[64]**, **BANG_Phwoar[4096]**
- **4-типовый каскад:** BIG(невидим, 6 дочерних) → MIDDLE(radius grow=120) → NEARLY → END(radius=40)
- Child[6]: {counter, type, where}

---

## 19. Рендеринг

### DDEngine Bucket Sort — Painter's Algorithm
- **POLY_NUM_PAGES:** **1508** (0–1407 стандартные, 1408+ служебные)
- **MAX_VERTICES:** **32000** глобальный пул
- Bucket sort для прозрачных:
  ```
  buckets[2048], sort_z = ZCLIP_PLANE/vz (reciprocal depth)
  bucket = int(sort_z × 2048), clamp [0, 2047]
  Render loop: i=0..2047 → FAR-TO-NEAR
  ```
- Непрозрачные: рендеряются по индексу, POLY_PAGE_COLOUR первым

### Рендер-пайплайн за кадр
1. `POLY_frame_init()` → инициализация, очистка буферов
2. Накопление полигонов: `MESH_draw_poly()`, `SPRITE_draw()`, `SKY_draw_poly_*()`
3. `POLY_frame_draw()` → SortBackFirst() → `DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT)`
4. Fog colour: outdoor=NIGHT_sky_colour; SEWERS/INDOORS=0 (чёрный)

### ⚠️ DRAWXTRA_MIB_destruct — игровая логика в рендерере
1. Модифицирует `ammo_packs_pistol`
2. Создаёт `PYRO_TWANGER` и `SPARK` эффекты
Аналогично: `PYRO_draw_armageddon()` создаёт объекты из рендерера.

### NIGHT система — Vertex Lighting
- NIGHT_MAX_SLIGHTS=**256** (static), NIGHT_MAX_DLIGHTS=**64** (dynamic)
- NIGHT_MAX_WALKABLE=**15000** vertices, NIGHT_MAX_BRIGHT=**64** (6-bit)
- **NIGHT_Colour:** r,g,b в 6-bit (0–63) → D3D через ×4 (0–252)
- **Per-frame:** `NIGHT_cache[32][32]` → `NIGHT_Square` = 16 вершин (4×4)
- **Ambient:** `value × 820 >> 8` = ×3.2
- **Static lights:** 3×3 lo-cell радиус; ⚠️ **БАГ: dz×nx вместо dz×nz!**
- **Динамические (NIGHT_Dlight):** до 64, типы NORMAL/BROKEN/PULSE/FADE
- **Тени:** статические ray-cast при загрузке, **динамических теней НЕТ** (flat sprite POLY_PAGE_SHADOW)

### Tom's Engine (figure.cpp) — рендеринг персонажей
- `USE_TOMS_ENGINE_PLEASE_BOB=1` — единственный активный путь
- Vertex morphing: tween ∈ [0,256], линейная интерполяция между keyframes
- **HIGH_REZ_PEOPLE_PLEASE_BOB:** закомментирован
- Anti-lights: отрицательные источники вычитают яркость

### Morph (mesh.cpp) и Crinkle
- Нет скелетной анимации — только vertex morphing между keyframes
- **Crinkle:** ⚠️ **ОТКЛЮЧЕНА в пре-релизе** (`return NULL`), **но работает в финальной PC**
- Ground crinkle: XZ варьируется; wall crinkle: XY варьируется
- Лимиты: 256 crinkles, 8192 points, 8192 faces, 700 points/crinkle

### MESH_car_crumples
- **5 уровней урона × 8 вариантов × 6 точек** predefined smash vectors

### Отражения в лужах
- `MESH_draw_reflection()` + `PUDDLE_in()` проверка зоны
- Страница POLY_PAGE_PUDDLE, отдельный рендеринг через `POLY_frame_draw_puddles()`

### Служебные страницы (1408+)
POLY_PAGE_SHADOW, POLY_PAGE_SHADOW_OVAL, POLY_PAGE_FLAMES, POLY_PAGE_SMOKE, POLY_PAGE_EXPLODE1/2, POLY_PAGE_MOON, POLY_PAGE_SKY, POLY_PAGE_COLOUR

---

## 20. UI и Frontend

**HUD Pipeline (OVERLAY_handle → PANEL_last):**
1. `PANEL_start()`
2. `PANEL_draw_buffered()` — таймеры (MM:SS, до 8)
3. `OVERLAY_draw_gun_sights()` — прицелы на целях
4. `OVERLAY_draw_enemy_health()` — HP полоска над врагом
5. **`PANEL_last()`** — основной HUD:
   - Здоровье: круговой индикатор (R=66px, 8 сегментов, arc -43°…-227°)
     - Darci: hp/200, Roper: hp/400, Vehicle: hp/300
   - Стамина: 5 квадратов 10×10, Stamina/25 → красный→оранж→жёлтый→зелёный→ярко-зелёный
   - Оружие: спрайт + число патронов
   - Радар: маяки + точки врагов (красные/серые)
   - Crime rate: пульсирующий красный %
   - Таймер: MM:SS, мигает красным при <30s
6. `PANEL_inventory()` — список оружия при переключении
7. `PANEL_finish()`

**Gun Sights:** прицелы через `track_gun_sight()` (max 4 целей/кадр). Цвет: зелёный (accuracy=255) → красный (0).

**Frontend Pipeline:**
```
game_attract_mode() → FRONTEND_loop()
  → FE_MAPSCREEN: ENTER → mode = 100 + mission_selected
  → mode≥100: ENTER → return FE_START
  → if (FE_START):
      STARTSCR_mission = "levels\\" + FRONTEND_MissionFilename(mode-100)
      VIOLENCE = FALSE только для туториалов (FTutor1, Album1, Gangorder1)
```

**Специальные миссии:**
- park2 — `is_semtex=1`
- Finale1 — `this_level_has_bane=1`
- Balrog3 — `this_level_has_the_balrog=1`
- Туториалы → `VIOLENCE=FALSE`

**Шрифты:**
- MenuFont: пропорциональный, флаги MENUFONT_FUTZING/HALOED/CENTRED/SHAKE/SINED/FLANGED
- Font2D: моноширинный (FONT2D_LETTER_HEIGHT=16), для HUD

**Фоны меню:** 4 сезонных темы — leaves/rain/snow/blood по `complete_point` (0–3).

---

## 21. Загрузка уровня

**9 шагов загрузки:**
1. `ELEV_load_name(fname)` — открыть `.ucm`, прочитать пути файлов
2. `ELEV_game_init()` — инициализировать подсистемы
3. `load_game_map(fname_map)` — загрузить `.iam`: PAP_Hi, примы, OB, текстуры
4. Постобработка: `build_quick_city()`, `DIRT_init()`, `ROAD_wander_calc()`, `WARE_init()`
5. **`MAV_precalculate()`** — самая тяжёлая: навигационный граф 128×128
6. `NIGHT_load_ed_file(fname_lighting)` — освещение; ночью = GF_RAINING
7. `ELEV_load_level(fname_level)` — загрузить эвент-вэйпойнты (до 512), создать фейковых граждан
8. Финальная обработка: calc норм, загрузить текстуры, **`EWAY_process()`** (создание игрока + NPC!)
9. Настройка камеры + спецэффекты

**Ключевой вызов:** `EWAY_process()` → `WPT_CREATE_PLAYER` → `EWAY_create_player()` → `create_player()`. НЕ прямое создание.

**Форматы файлов:**
| Расширение | Назначение | Загрузчик |
|-----------|-----------|----------|
| `.ucm` | Миссия: метаданные + пути | `ELEV_load_name()` |
| `.iam` | Карта: PAP + объекты + примы | `load_game_map()` |
| `.lgt` | Освещение | `NIGHT_load_ed_file()` |
| `.txt` | Диалоги NPC | `EWAY_load_fake_wander_text()` |
| `.prm` | Меши примов | `load_prim_object()` |

---

## 22. Прогресс и сохранения

**.wag v3 layout:**
```
mission_name      : variable-length + CRLF
complete_point    : UBYTE
version           : UBYTE = 3

[v≥1] Stats (9 байт):
  DarciStrength, DarciConstitution, DarciSkill, DarciStamina (4)
  RoperStrength, RoperConstitution, RoperSkill, RoperStamina (4)
  DarciDeadCivWarnings (1)

[v≥2] mission_hierarchy[60] : UBYTE[60]
[v≥3] best_found[50][4]     : UBYTE[50][4] (200 байт)
```

**Слоты:** 1, 2, 3 (1-based).

**complete_point (0–255):**
- <8 → theme 0; <16 → 1; <24 → 2; ≥24 → 3
- Влияет на доступные миссии через `FRONTEND_MissionHierarchy`

**mission_hierarchy[60] биты:**
- bit 0 (1) = существует
- bit 1 (2) = завершена
- bit 2 (4) = доступна/ожидает
- `mission_hierarchy[1] = 3` — корневая миссия (стартовая)

**best_found[50][4] — анти-фарм механика:**
```
// Если found > best_found[mission][i]:
//   bonus = found - best_found[mission][i]
//   stat += bonus   (только разница засчитывается)
//   best_found[mission][i] = found
// Индексы: [0]=Constitution, [1]=Strength, [2]=Stamina, [3]=Skill
```

---

## 23. Форматы файлов ресурсов

**Mission Script (.sty):**
- `data/urban.sty`, версия 4
- Формат: `ObjID : GroupID : ParentID : ParentIsGroup : Type : Flags : District : Filename : Title : Briefing`
- **27 районов** (0–26): Baseball Ground, Botanical Gardens, West District, Southside Beach, Rio canal, Central Park, Snow Plains, Gangland, Skyline, Academy, Banes Estate, Medical Centre, El Cossa Island, Test track, Television Centre, Assault Course, Clandon Heights, Combat Centre, Oval Circuit, Advanced Circuit, West Block
- `suggest_order[]` захардкожена в frontend.cpp: 31 основная + 3 альтернативные + 1 скрытая

**Анимация (.ALL, .IMP, .MOJ):**
- `.ALL` содержит: версия, mesh блоки (.moj), keyframe chunk, fight hittables
- `global_anim_array[4][450]`: Type (0=DARCI, 1=ROPER/hero, 2=ROPER2, 3=CIV)
- Загрузка: `darci1.all`, `hero.all`, `bossprtg.all`

**Аудио (WAV, Miles Sound System):**
- SFX: 22050/44100 Hz PCM 16-bit
- Структура: `sfx/1622/Ambience/`, `Footsteps/`, `General/`, `music01/` — `Crawl/`, `Driving1/2/`...
- Диалоги: `talk2/{Level}.ucm{Index}.wav`, `{CharType}{Index}.wav`

**Освещение (.LGT):**
- `ED_Light` (20 байт: range, RGB signed, x/y/z SLONG)
- `NIGHT_sky_colour`: RGB 0–63 (3 байта)
- Ambient: `value × 820 >> 8` = ×3.2

**Карта уровня (.IAM):**
- **PAP_Hi** (128×128, 6 байт/ячейка): Texture (UWORD), Flags (UWORD), Alt (SBYTE ×8)
- **LoadGameThing** (44 байт): Type, SubType, X/Y/Z, Flags, IndexOther, AngleX/Y/Z
- **SuperMap**: DBuildings, DFacets, DStyle, (v17+) DStorey, (v21+) interior/staircase, (v23+) OB
- **Наборы текстур:** 22 набора (0–21)

**Модели (.MOJ, .PRM, .TXC):**
- `.MOJ`: PrimPoint (SWORD x/y/z = 6 байт), PrimFace3 (28 байт), PrimFace4 (34 байта)
- **FACE_FLAG_WALKABLE (1<<6)** = ходимые поверхности
- `.TXC` (FileClump): архив `[MaxID offsets][MaxID lengths][data blob]`; может быть сжато

**Текстуры (.TGA, .TXC, .TMA):**
- `.TGA`: 32-bit RGBA/24-bit RGB, 512×512 до 32×32; мип-маппинг на CPU
- `style.tma`: 200 стилей × 5 слотов, `TXTY` struct (Page, Tx, Ty, Flip, UBYTE each)
- `instyle.tma`: count_x, count_y, inside_tex индексы
- **TEXTURE_MAX_TEXTURES=1568**: world0–3(256), shared(256), interior(64), people(128), prims(448), people2(256), effects(160), crinkle(192)

---

## 24. Малые системы

**Вода (Water.cpp):**
- 1024 (PC) / 102 (PSX) `WATER_Point`: x/z (1-bit fixed), y (SBYTE height)
- Position: `x = wp->x * 128.0`, `y = wp->y << 5`, UV: `u/v = wp->x/z * 0.19`
- Wibble: закомментирован в пре-релизе

**Tripwire (trip.cpp):**
- `TRIP_Wire` (max 32): y, x1/z1/x2/z2 (8-bit fixed), along (0–255), wait (debounce)
- Детекция: point-to-line distance < 0x20, Y-range feet-0x10..head+0x10
- Deactivation: `x1 = 0xffff`

**Sphere-Matter (sm.cpp):**
- `SM_Sphere` (max 1024), `SM_Link` (max 1024)
- `SM_GRAVITY = -0x200`, jellyness 0x10000..0x00400
- Ground bounce: `dy = -abs(dy); dy -= dy >> 9` (11.7% потеря)

**Следы (tracks.cpp):**
- TYRE_SKID (белый, ширина 10), TYRE, LEFT/RIGHT_PRINT
- Кровь: `TRACKS_Bleed()` (размер 1–31), `TRACKS_Bloodpool()` (80–95)
- Отключается при `!VIOLENCE`

**Command система (Command.cpp — LEGACY):**
- Заменена PCOM в финале
- Работает ТОЛЬКО: `COM_PATROL_WAYPOINT` + 4 из 14 условий

---

## 25. Математика

**PSX система (FMatrix.h):**
- Углы: **0–2047 за полный оборот**
- `SinTable[2560]`, `CosTable = &SinTable[512]` (смещение π/2)
- Масштаб: `65536 = 1.0` (16.16 fixed)
- `FMATRIX_calc(matrix[9], yaw, pitch, roll)` → row-major 3×3
- `Arctan(X, Y)` использует `AtanTable[256]`, возвращает 0–2047

**PC система (Matrix.h, libm):**
- Углы: радианы (float)
- `MATRIX_calc()`, `MATRIX_find_angles()` → `Direction` struct (yaw, pitch, roll float)
- Гимбал-лок обработка при pitch > π/4

**Кватернионы (Quaternion.h/cpp — только PC):**
- `CQuaternion { float w, x, y, z }`
- `QuatSlerp()`: если dot > 0.95 → линейная, иначе стандартный SLERP
- Integer версия: fixed-point 15.15

**Fixed-point форматы:**
- 32.8: позиции объектов (1 mapsquare = 256 ед.)
- 15.15: `1 << 15 = 32768` (quaternion integer)
- 16.16: `1 << 16 = 65536` (coordinates)

---

## 26. Вырезанные фичи

| Фича | Статус | Код |
|------|--------|-----|
| Мотоцикл (BIKE) | never shipped | bike.cpp, ~15 файлов, `#ifdef BIKE` |
| Кошка-гарпун (hook.cpp) | never shipped | 256-точечная верёвка, rope physics |
| Канализация (sewer.cpp, ns.cpp) | never shipped | — |
| Процедурные интерьеры зданий | never shipped | building.cpp (вручную размещённые есть) |
| Мультиплеер / split-screen | отключён в финале | CTF mode, MAX_PLAYERS=2 в коде |
| GS_REPLAY (demo/replay) | не в игре | код есть, не используется |
| **Zipwire/тросы** | **закомментирован в пре-релизе, есть в финале!** | тренировочная полоса, уровень 1 |
| Bat, Gargoyle | есть модели, не спаунятся | — |
| Balloon.cpp (шары) | отключены | `load_prim_object(PRIM_OBJ_BALLOON)` закомментирован |

**НЕ путать:** закомментированный код ≠ вырезанная фича.

---

## 27. Препроцессорные флаги (PC Release)

**Release конфигурация:**
```
NDEBUG;_RELEASE;WIN32;_WINDOWS;VERSION_D3D;TEX_EMBED;FINAL
```

**Активные флаги:**
| Флаг | Назначение |
|------|-----------|
| `VERSION_D3D` | DirectX 3D (основной API) |
| `TEX_EMBED` | Встраивание текстур (обе конфигурации) |
| `FINAL` | Финальная сборка (только Release) |
| `WE_NEED_POLYBUFFERS_PLEASE_BOB` | Z-сортировка полигонов |
| `USE_TOMS_ENGINE_PLEASE_BOB` | D3D-friendly рендерер персонажей (=1) |
| `NO_BACKFACE_CULL_PLEASE_BOB` | Отключить backface culling |
| `USE_PASSWORD` | Чит-коды |
| `UNICODE` | Unicode диалоги |

**Удалить (мёртвые):**
| Флаг | Причина |
|------|---------|
| `TARGET_DC` | Dreamcast |
| `PSX` | PlayStation 1 (осторожно в физике!) |
| `VERSION_GLIDE` | 3dfx Glide |
| `EDITOR`, `BUILD_PSX` | Редактор, PSX сборка |
| `BIKE` | Мотоцикл (не переносить) |
| `EIDOS` + региональные | DRM, издатель |
| `USE_A3D`, `__WATCOMC__`, `__DOS__` | Legacy |
