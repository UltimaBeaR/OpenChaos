# Малые подсистемы — Urban Chaos

Системы обнаруженные при верификации KB, ранее не задокументированные.

---

## 1. Динамическая вода (Water.cpp — 747 строк)

**НЕ "просто невидимые стены"** — полная система анимации водной поверхности.

**Структуры:**
- `WATER_Point` (1024 PC / 102 PSX): x,z (1-bit fixed), y (SBYTE height), dyaw/ddyaw (вращение)
- `WATER_Face` (max 512): 4 corner indices + linked list next
- `WATER_Map[128×128]`: per-square linked list голов

**Ключевые формулы:**
- Position: `x = wp->x * 128.0`, `z = wp->z * 128.0`, `y = wp->y << 5`
- UV: `u = wp->x * 0.19`, `v = wp->z * 0.19`
- Gushing (от движения через воду): `strength = QDIST2(dx, dz) << 2`
- Wibble анимация: **закомментирована в пре-релизе** (массивы есть, формула нет)
- Цвет: base `0xffbfbfbf` с динамическим brightening

**API:** `WATER_init()`, `WATER_add()`, `WATER_gush()`, `WATER_process()` (per-frame)

**Перенос:** Визуальная система, переписать на современный water shader. Но `WATER_gush()` — геймплей-реакция.

---

## 2. Растяжки/ловушки (trip.cpp — 361 строк)

**Tripwire alarm система** для level design. Невидимые проволоки, Darci пересекает → alarm.

**Структура:** `TRIP_Wire` (max 32):
- `y` — высота, `x1,z1,x2,z2` — линия (8-bit fixed point)
- `along` — позиция вдоль провода (0-255 normalized)
- `wait` — debounce counter (4 кадра после срабатывания)

**Детекция:** point-to-line distance < `0x20`, Y-range: `feet - 0x10` .. `head + 0x10`
- Проверяет: left foot, right foot, head — отдельно
- Proximity culling: Manhattan distance > `0x500` → пропускаем
- Deactivation: `x1 = 0xffff` маркер

**API:** `TRIP_init()`, `TRIP_create()`, `TRIP_collide()`, `TRIP_process()` (per-frame), `TRIP_activated()`, `TRIP_deactivate()`


---

## 3. Sphere-Matter (sm.cpp — 502 строк)

**Soft-body physics** — отдельная от HyperMatter (hm.cpp). Деформируемые кубы из связанных сфер.

**Структуры:**
- `SM_Sphere` (max 1024): x,y,z (16-bit fixed), dx,dy,dz (8-bit fixed), radius, mass
- `SM_Link` (max 1024): sphere1, sphere2, dist (squared!)
- `SM_Object` (max 16): sphere_index/num, link_index/num, jellyness, resolution, density

**Ключевые константы:**
- Resolution: 2-5 (= 8-125 сфер на объект)
- `SM_GRAVITY = -0x200`
- Jellyness (spring): `0x10000` .. `0x00400`
- Friction: `vel -= vel/256; vel -= vel/512; vel -= SIGN(vel)` (тройное)
- Ground bounce: `dy = -abs(dy); dy -= dy >> 9` (11.7% потеря энергии)
- Spring force: `force = (current_dist - rest_dist) * jellyness`

**API:** `SM_init()`, `SM_create_cube()`, `SM_process()` (per-frame), `SM_get_start()`/`SM_get_next()` (rendering)


---

## 4. Воздушные шары (Balloon.cpp — 576 строк, PC only)

Гелиевые шары, которые персонажи могут хватать и нести.

**Структуры:**
- `BALLOON_Point`: x,y,z + dx,dy,dz (6 SLONG)
- `BALLOON_Balloon` (max 32): type (0=free,1=yellow,2=red), thing (привязка), 4 points, yaw/pitch

**Физика:**
- `BALLOON_POINT_DIST = 0x2000` (расстояние между точками)
- Buoyancy: `dy += 0x40` per frame (постоянная сила вверх)
- Damping: `dx -= dx/0x10` per frame (1/16)
- Point[0] привязан к SUB_OBJECT левой руки персонажа каждый кадр
- Multi-balloon collision: **ЗАКОММЕНТИРОВАН** ("PEOPLE ONLY CARRY ONE BALLOON NOW")

**API:** `BALLOON_init()`, `BALLOON_create()`, `BALLOON_process()`, `BALLOON_release()`, `BALLOON_find_grab()`

**Перенос: НЕТ.** Фича вырезана: `load_prim_object(PRIM_OBJ_BALLOON)` закомментировано в ob.cpp:757 (`//no`), меш шарика не грузится. Один из двух call site завёрнут в мёртвый `NO_MORE_HAPPY_BALOONS`. Пользователь не видел шариков в PC лонгплее. Код остался, но фича неактивна.

---

## 5. Следы и декали (tracks.cpp — 378 строк)

Следы шин, отпечатки ног, кровь. Circular buffer декалей.

**Типы:**
- `TRACK_TYPE_TYRE_SKID` — белые (0xFFFFFF), ширина 10
- `TRACK_TYPE_TYRE` — зависит от поверхности: вода(0x203D60), грязь(0x482000), снег(0x000000)
- `TRACK_TYPE_LEFT/RIGHT_PRINT` — отпечатки ног (тоже от поверхности)

**Поверхности** (по texture page):
- 65, 66, 143 → WOOD
- 69-74 → GRASS
- 68, 106-111 → GRAVEL
- + проверка PUDDLE_in()

**Кровь:**
- `TRACKS_Bleed()` — брызги от ног, случайный размер 1-31
- `TRACKS_Bloodpool()` — лужа от таза, размер 80-95
- **Отключаются при `!VIOLENCE`** (цензура)

**Текстуры:** `POLY_PAGE_TYRETRACK`, `POLY_PAGE_FOOTPRINT`, `POLY_PAGE_BLOODSPLAT`

**Перенос:** Визуальный фидбек, переписать как decal system.

---

## 6. Электрические искры (Spark.cpp — 1326 строк)

Ветвящиеся электрические разряды. Электрозаборы + эффекты ударов.

**Структуры:**
- `SPARK_Spark` (max 32): die (life), 4 points, mapwho-linked
- `SPARK_Point`: type (LIMB/CIRCULAR/GROUND/POINT), velocity flags, position OR limb tracking
- `SPARK_Info` (для рендера): 5 координатных точек, цвет, размер

**Физика:**
- Lifespan: `rand() % max_life + 8`
- Movement: `pos += velocity >> 4` per frame
- Velocity: random(-31..+31), FAST=×4, SLOW=÷4, CLAMP_X/Y/Z
- Recursive midpoint generation для ветвления
- LIMB-attached: отслеживает кость персонажа в реалтайме

**Электрозаборы:** `SPARK_show_electric_fences()` — каждые 32 кадра создаёт искру от забора к земле

**Перенос:** Визуальный эффект. Геймплей-часть: электрозаборы = урон при контакте (обработка в другом месте).

---

## 7. Command система (Command.cpp — 520 строк, LEGACY)

**Editor-driven NPC commands. В основном МЁРТВЫЙ КОД — заменён PCOM.**

**Архитектура:** Waypoints(1000) → Conditions(1000) → Commands(2000) → CommandLists(200)

**Только работает:** `COM_PATROL_WAYPOINT` (patrol по linked list waypoints) + 4 из 14 условий (THING_DEAD, THING_NEAR_PLAYER, THING_NEAR_THING, SWITCH_TRIGGERED)

**10 из 14 условий = стабы** (GROUP_*, CLASS_*, TIME, CLIST). 12 из 13 команд = не реализованы.

**Интеграция:** Level.cpp загружает из .lev файлов → `init_person_command_list()` → `advance_person_command()`. Но PCOM полностью заменяет эту систему в финальной игре.


---

## 8. Game Logic в рендерере (drawxtra.cpp)

**⚠️ ВНИМАНИЕ:** `drawxtra.cpp` содержит игровую логику внутри функций рисования.

**`DRAWXTRA_MIB_destruct()`** — МУТИРУЕТ game state:
- `ammo_packs_pistol = (3200-ctr)>>3` — меняет амуницию
- Создаёт `PYRO_TWANGER`, `SPARK_create()` — аллоцирует игровые объекты

**`PYRO_draw_armageddon()`** — СОЗДАЁТ объекты из рендерера:
- `PYRO_create(pos, PYRO_NEWDOME)`, `PARTICLE_Add()`, `MFX_play_xyz()`

**`engineMap.cpp`** — БЕЗОПАСЕН (только читает game state для рендеринга, не мутирует).

