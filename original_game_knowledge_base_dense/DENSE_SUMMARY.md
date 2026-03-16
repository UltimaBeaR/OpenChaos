# DENSE SUMMARY — Urban Chaos Knowledge Base

Компактная выжимка всей KB. Только факты: числа, константы, формулы, зависимости, баги.
Rebuilt: 2026-03-17. Источник: all subsystems/*.md + resource_formats/*.md.

---

## 1. Игровой цикл и время

- Entry point: `Main.cpp` → `WinMain()` → `SetupHost()` → `game()` → `game_loop()`
- **Язык:** C-стиль в .cpp файлах, без C++ классов и наследования
- **Компилятор оригинала:** Visual Studio 6, DirectX 6/7; зависимости: `binkw32.lib`, `mss32.lib`
- **Кодовая база:** ~500K+ строк, ~350+ .cpp, ~150+ .h
- Крупнейшие файлы: `DIManager.cpp` **62K**, `GDisplay.cpp` **51K**, `Game.cpp` **53K**, `Mission.cpp` **43K**, `Special.cpp` **41K**

**Тайминг:**
```c
#define NORMAL_TICK_TOCK  (1000/15)  // 66.67 мс — базовый тик (15 FPS логики)
#define TICK_SHIFT        8
// TICK_RATIO ≈ 256 (норма), ≤ 512 (2.0x медленно), ≥ 128 (0.5x быстро)
// Slow-motion: TICK_RATIO = 32 (~12.5%)
```
- Рендеринг: **30 FPS** (lock_frame_rate spin-loop busy-wait); логика: **15 FPS**
- `TICK_LOOP = 4`; подвеска автомобилей: **3 цикла за кадр** (count=TICK_LOOP; while(--count))
- `TICKS_PER_SECOND = 80` (20 × TICK_LOOP)
- `PCOM_TICKS_PER_SEC = 320` (16 × 20, AI-таймер независим от рендер-FPS)
- Сглаживание TICK_RATIO: **4-кадровая** скользящая средняя (SMOOTH_TICK_RATIO), кольцевой буфер из 4 значений; первые 3 кадра raw, с 4-го: `sum >> 2`
- `GAME_TURN` — глобальный счётчик кадров, сбрасывается при каждой загрузке уровня
- `EWAY_time += 80 × TICK_RATIO >> TICK_SHIFT` (≈**1000 ед/сек** при 50 fps)
- PC: все EWAY-вэйпойнты каждый кадр (step=1); PSX: чётные/нечётные (step=2)
- Inner game loop продолжается пока: `GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON`
- `should_i_process_game()` = FALSE при `GF_PAUSED` или активном tutorial_string
- `PUDDLE_process()`, `DRIP_process()` — выполняются даже при паузе
- Bench cooldown: `if ((GAME_TURN & 0x3ff) == 314)` → каждые **1024 кадра** (~34 с при 30 fps)
- `TIMEOUT_DEMO = 0` — demo_timeout() мёртвый код

**Битовые флаги GAME_STATE:**
- `GS_ATTRACT_MODE = 1<<0`, `GS_PLAY_GAME = 1<<1`, `GS_LEVEL_WON = 1<<3`, `GS_LEVEL_LOST = 1<<4`
- `GS_GAME_WON = 1<<5`, `GS_RECORD = 1<<6`, `GS_PLAYBACK = 1<<7`, `GS_REPLAY = 1<<8`
- `GS_EDITOR = 1<<16` (только разработка)
- `GS_CONFIGURE_NET = 1<<2` (Dreamcast, не нужен в PC порте)

**PC Release конфигурация:** `NDEBUG;_RELEASE;WIN32;_WINDOWS;VERSION_D3D;TEX_EMBED;FINAL`
- `TEX_EMBED` активен в обоих конфигурациях (Debug и Release); `FINAL` — только в Release

---

## 2. Система координат и карта

**Координаты:**
- Тип `GameCoord`: три `SLONG` (X, Y, Z) — **32-bit signed integer**, нет float
- **32.8 fixed-point**: `WorldPos` = реальная позиция × **256**
- `>> 8` → mapsquare (PAP_Hi), `>> 18` → lo-res square (PAP_Lo), `>> 16` → grob
- Углы: **0–2047** (11-bit, **2048 = 360°**); Thing.yaw: **UBYTE 0–255** (255 = 360°)
- 0=восток, 64=юг, 128=запад, 192=север

**Карта:**
- PAP_Hi: **128×128 mapsquares**, **6 байт** на ячейку: `UWORD Texture + UWORD Flags + SBYTE Alt + SBYTE Height`
- PAP_Lo: **32×32 ячеек**, **8 байт** на ячейку; **1 PAP_Lo ячейка = 1024 мировых единицы**
- **1 mapsquare = 256 мировых единиц** (`PAP_SHIFT_HI = 8`)
- Весь мир: **32768 × 32768** мировых единиц
- Высота из PAP_Hi.Alt: `Alt << 3`, диапазон **–1024..+1016** юнитов; Alt диапазон 0–127 → **0–1016**
- `PAP_LO_NO_WATER = -127`; `water_level = -0x80 = -128` (хардкодировано в EL_load_map())

**Флаги PAP_Hi:**
- Бит `PAP_FLAG_ROOF_EXISTS (1<<9)` и `PAP_FLAG_ANIM_TMAP (1<<9)` — **один бит, контекстно-зависимый** ⚠️
- Бит `PAP_FLAG_WANDER (1<<14)` и `PAP_FLAG_FLAT_ROOF (1<<14)` — аналогично ⚠️
- `PAP_FLAG_SHADOW_1/2/3` (биты 0-2), `PAP_FLAG_REFLECTIVE` (бит 3), `PAP_FLAG_HIDDEN` (бит 4)

**Интерполяция высоты (билинейная, диагональная):**
```c
if (xfrac + zfrac < 0x100)
    answer = h0 + (h2-h0)*xfrac + (h1-h0)*zfrac;
else
    answer = h3 + (h1-h3)*(256-xfrac) + (h2-h3)*(256-zfrac);
return answer << 3;  // PAP_ALT_SHIFT
```

**MapWho:**
- Linked list через `PAP_Lo.MapWho` → `Thing->Child` → `Thing->Parent`
- `move_thing_on_map()` вызывается только при смене PAP_Lo-ячейки (каждые **1024 ед.**)
- Хэш: `PAP_SIZE_HI = 128`, паттерн от Bullfrog, реализован в `cnet.cpp`

**Граф дорог (ROAD):**
- Дорожные текстуры PC: **323–356**; зебра: **333–334**; TextureSet 7/8: доп. 35,36,39
- PSX дорожные текстуры: **256–278**
- `ROAD_is_middle()` = все **5×5** квадратов вокруг — дорога
- **ROAD_MAX_NODES = 256**, до **4 связей** на узел
- **ROAD_LANE = 0x100** (256 ед.) — смещение от центра для полосы движения
- Хардкод: нод **(121,33)** на карте `gpost3.iam` принудительно пропускается ⚠️
- `ROAD_find_me_somewhere_to_appear()` — спаун машин на расстоянии **> 0x1000** от игрока

**WAND (зоны пешеходов):**
- `PAP_FLAG_WANDER` ставится ячейкам в **5×5** окрестности дороги
- `WAND_MAX_MOVE = 3` клетки; **4 кандидата** направлений; fallback — **16** случайных проб в радиусе **±15** клеток
- CLASS_BAT (Bane) — может ходить и по WANDER и по ROAD

**Освещение (Lighting):**
- `LIGHT_Colour`: **6-bit каналы** (0–63), ×4 → 0–255 для D3D
- Статических источников: **256**, динамических: **64**
- Кэш: до **256** lo-res квадратов, до **15000** walkable вершин
- Загружается из `.lgt` файлов (`data/Lighting/`)
- `LIGHT_range` max = **0x600** (~1536 units); типы: NORMAL/BROKEN/PULSE/FADE
- Файлы уровней: `Levels/Level%3.3d.lev` — версии **0–3**

---

## 3. Thing System

**Thing struct — **125 байт**:**
- `Class/State/OldState/SubState` — 4×`UBYTE`
- `Flags` — `ULONG` (**32** флага)
- `Child/Parent/LinkChild/LinkParent` — 4×`THING_INDEX`
- `WorldPos` — `GameCoord` (3×`SLONG`)
- `StateFn` — `void (*)(Thing*)`
- `Draw` — `union {DrawTween*; DrawMesh*}`
- `Genus` — union из **15** указателей на типы
- `Velocity/DeltaVelocity/RequiredVelocity/DY` — 4×`SWORD`
- `OnFace` — `SWORD` (-1 = земля/вода)
- `DogPoo1` — Timer1 (`UWORD`); `DogPoo2` — SwitchThing (`THING_INDEX`)

**19 классов (CLASS_*):**
```
NONE(0) PLAYER(1) CAMERA(2) PROJECTILE(3) BUILDING(4) PERSON(5)
ANIMAL(6) FURNITURE(7) SWITCH(8) VEHICLE(9) SPECIAL(10) ANIM_PRIM(11)
CHOPPER(12) PYRO(13) TRACK(14) PLAT(15) BARREL(16) BIKE(17) BAT(18) END(19)
```

**FLAGS_* (биты 0–16):**
- 0: `FLAGS_ON_MAPWHO`; 1: `FLAGS_IN_VIEW`; 5: `FLAGS_IN_BUILDING`; 8: `FLAGS_CABLE_BUILDING`; 9: `FLAGS_COLLIDED`; 14: `FLAGS_BURNING`

**Лимиты:**
- `PRIMARY_THINGS = 400`, `SECONDARY_THINGS = 300` (PC) / **50** (PSX), итого = **700**
- Аллокация/освобождение O(**1**) через linked lists USED/UNUSED

**STATE_* (37 состояний, ключевые):**
```
INIT(0) NORMAL(1) MOVEING(5) IDLE(6) JUMPING(8) FALLING(10)
DOWN(12) DYING(16) DEAD(17) CLIMB_LADDER(19) GUN(22) SHOOT(23)
DRIVING(24) CIRCLING(33) HUG_WALL(34) CARRY(36) FLOAT(37)
```

**THING_find_* маски:**
```c
THING_FIND_PEOPLE  = (1 << CLASS_PERSON)
THING_FIND_LIVING  = (1 << CLASS_PERSON) | (1 << CLASS_ANIMAL)
THING_FIND_MOVING  = PERSON | ANIMAL | PROJECTILE
```

**Barrel System (barrel.cpp, **1937** строк):**
- 2 сферы `BARREL_Sphere`, расстояние `BARREL_SPHERE_DIST = 50`
- Гравитация: `dy -= 0x80` каждый кадр
- Затухание: `d* -= d*/32` (**3.125%**/кадр)
- Bounce: `dy = abs(dy)/2` при ударе о землю
- Покой: `abs(dx)+abs(dy)+abs(dz) < 0x200`, оба still > **64** → статичный
- `BARREL_MAX_BARRELS = 300`; экстренный recycling ближайшего при нехватке Thing
- `BARREL_STACK_RADIUS = 45`; рандом позиции ±`0x1ff`
- Взрыв NORMAL: `shockwave(radius=0x200, damage=250)`
- 4 типа: NORMAL, CONE, BURNING, BIN
- Мёртвый код: `BARREL_position_on_hands()` + `BARREL_throw()` (строки ~**1505-1690**)

**Moving Platforms (plat.cpp, `#ifndef PSX`):**
- Состояния: NONE / GOTO / PAUSE / STOP
- Задержка **10 сек** = "остановиться навсегда"; `PLAT_MAX_FIND = 8`
- Флаги: `PLAT_FLAG_LOCK_X/Y/Z`

**Chopper (chopper.cpp, `#ifndef PSX`):**
- `HARDWIRED_RADIUS = 8192`, `DETECTION_RADIUS = 1024`, `IGNORAMUS_RADIUS = 6144`
- `PRIM_OBJ_CHOPPER = 74`, `PRIM_OBJ_CHOPPER_BLADES = 75`

---

## 4. Физика и коллизии

**Коллизионные структуры:**
- `CollisionVect`: ~**14 байт**, `MAX_COL_VECT = 10000` (PC) / 1000 (PSX); итого ~**300 KB**
- `CollisionVectLink`: **4 байт**, `MAX_COL_VECT_LINK = 10000` (PC) / 4000 (PSX); ~**40 KB**
- `WalkLink`: **4 байт**, `MAX_WALK_POOL = 30000` (PC) / 10000 (PSX); ~**120 KB**

**Вода:**
- = **невидимые коллизионные стены** по периметру водяных ячеек; нет плавания/утопания/замедления
- Огонь гасится в воде: `PAP_FLAG_WATER` → сброс `PYRO_IMMOLATE` (Person.cpp:4414)

**Гравитация:**
```c
// Персонажи — animation-driven; падение при y_diff > 60 юнитов
// Транспорт и projectiles — явная:
#define GRAVITY (-(128 * 10 * 256) / (80 * 80))  // = -51 юн/тик²
```

**move_thing() — порядок операций:**
1. Граница карты 128×128 → return 0
2. `collide_against_things()` — сфера радиус + `0x1a0`
3. `collide_against_objects()` — поиск в радиусе ±`0x180`; prim IDs скамеек: **89,95,101,102,105,110,126** → `set_person_sit_down()` при SUB_STATE_WALKING_BACKWARDS
4. `slide_along()` — `SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT = -0x50` (**-80** ед.)
5. `slide_along_edges()` / `slide_along_redges()`
6. `find_face_for_this_pos()` — `y_diff > 0x50` (**80** ед.) → `FALL_OFF_FLAG_TRUE`
7. Fall-through bug workaround: `x2 += dx>>2; y2 += dy>>2; z2 += dz>>2`

**slide_along():**
- `NOGO_COLLIDE_WIDTH = 0x5800` (~**22** ед.)
- Все DFacets оси-aligned (только X или Z, нет диагональных стен)
- Нет restitution (отскока), нет трения
- Circle push: `*pos = endpoint + dp * (radius>>8) / (dist>>8 + 1)`
- Стена "перешагивается": `y_top += extra_wall_height` (-80 ед.)

**find_face_for_this_pos():**
- Поиск в диапазоне ±`0x200` (±2 ячейки)
- Кандидат: `dy <= 0xa0` (**160** ед.)
- GRAB_FLOOR при `|dy| < 0x50` (**80** ед.)
- Отрицательный face index = roof face

**plant_feet():**
- `fx=0; fz=0` перед сложением — X/Z от анимации ноги игнорируется
- GRAB_FLOOR → snap terrain, `OnFace=0`, return **-1**
- Face найден, `new_y < fy-60` → падение, return **0**
- NULL terrain, `new_y < fy-30` → падение, return **0**

**Лестницы:**
- `ok_to_mount_ladder`: `QDIST2(dx,dz) < 75`
- `correct_pos_for_ladder`: `angle = Arctan(-dx,dz)+1024+512`; scale: `+256` = низ, `-64` = верх
- ⚠️ Запрыгивание снизу — **закомментировано** в interfac.cpp (пре-релиз)

**HyperMatter (hm.cpp, ~2000 строк) — только мебель:**
- **13 пружин/точку**; Euler integration; `gy = 0` hardcoded → объекты отскакивают от Y=0, не реального terrain ⚠️
- `HM_stationary(threshold=0.25)` → "засыпание"
- Баг: `already_bumped[i]` вместо `[j]` → некоторые HM-HM столкновения пропускаются ⚠️

**WMOVE (wmove.cpp) — персонажи на крышах движущихся объектов:**
- `WMOVE_MAX_FACES = 192`
- Faces per object: CLASS_PERSON/PLAT: **1**; VAN/AMBULANCE: **1**; JEEP/MEATWAGON: **4**; CAR/TAXI/POLICE/SEDAN: **5**
- PSX: только фургоны создают WMOVE faces
- Threshold: `|dy| < 0x200` → dy=0

**Физика Darci:**
```c
GRAVITY = 4<<8 = 1024 units/tick
Terminal velocity: DY = -30000 (hard clamp)
Fall damage: damage = (-DY - 20000) / 100   // линейная
DY <= -30000 → damage = 250 (instant kill)
DY >= -20000 → damage = 0
// Death plunge (игрок): DY < -12000; lookahead 3 шага, нет пола в 0x600
// Death plunge (NPC): DY < -6000
```

---

## 5. AI система PCOM

**Типы AI (PCOM_AI_*) — 22 типа (0–21):**
```
NONE(0) CIV(1) GUARD(2) ASSASIN(3) BOSS(4) COP(5) GANG(6) DOORMAN(7)
BODYGUARD(8) DRIVER(9) BDISPOSER(10) BIKER(11,#ifdef BIKE) FIGHT_TEST(12)
BULLY(13) COP_DRIVER(14) SUICIDE(15) FLEE_PLAYER(16) KILL_COLOUR(17)
MIB(18) BANE(19) HYPOCHONDRIA(20) SHOOT_DEAD(21)
```

**Состояния AI (PCOM_AI_STATE_*) — 28 (0–27):**
- SEARCHING(3) — **stub, пустой**; SLEEPING(5) — **stub, пустой**; LOOKAROUND(11) — **только счётчик**
- Активные: NORMAL INVESTIGATING KILLING FLEE_PLACE FLEE_PERSON FOLLOWING NAVTOKILL HOMESICK FINDCAR BDEACTIVATE LEAVECAR SNIPE WARM_HANDS KNOCKEDOUT TAUNT ARREST TALK GRAPPLED HITCH AIMLESS HANDS_UP SUMMON GETITEM

**Типы движения (PCOM_MOVE_*):** STILL(1) PATROL(2) PATROL_RAND(3) WANDER(4) FOLLOW(5) WARM_HANDS(6) FOLLOW_ON_SEE(7) DANCE(8) HANDS_UP(9) TIED_UP(10)

**Черты (PCOM_BENT_*):** LAZY(0) DILIGENT(1) GANG(2) FIGHT_BACK(3) ONLY_KILL_PLAYER(4) ROBOT(5) RESTRICTED(6) PLAYERKILL(7)

**Числовые константы:**
- `PCOM_TICKS_PER_TURN = 16`; `PCOM_TICKS_PER_SEC = 320`
- `PCOM_ARRIVE_DIST = 0x40 = 64` units
- `RMAX_PEOPLE = 180` NPC в сцене
- `PCOM_MAX_GANGS = 16` (цвета 0–15); `PCOM_MAX_GANG_PEOPLE = 64`
- `PCOM_MAX_FIND = 16`; `PCOM_get_duration(n) = n * 32` тиков

**Радиусы звуков (world-units):**
- FOOTSTEP=**640** VAN=**384** DROP=**512** MINE/GRENADE=**768** LOOKAROUND/WANKER/DROP_MED=**1024**
- UNUSUAL/HEY/DRAW_GUN/DROP_BIG=**1536** BANG=**1792** ALARM=**2048** FIGHT=**2304** GUNSHOT=**2560**
- ⚠️ DRAW_GUN — **визуальный триггер, требует LOS** (несмотря на название "звук")

**Восприятие (can_a_see_b):**
- NPC default range: `8 << 8 = 2048` world-units; игрок: `256 << 8 = 65536`
- Eye height standing: **0x60 = 96** units; crouching: **0x20 = 32** units
- FOV (2048=360°): близко (dist < **192**) = **700 (~123°)**; далеко = **420 (~74°)**
- "Краем глаза": **250/2048 ≈ 44°** — видит на половинную дистанцию если цель неподвижна
- Цель приседает: `view_range >>= 1`; цель движется: `view_range += 256`
- dy ≥ **0x80 = 128** units → `+ dy<<2` к эффективной дистанции
- Освещение: `view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256`

**Полиция — арест:**
- Сканирование каждые **4 кадра**, радиус **0x800 = 2048** units
- Группировка банд по `pcom_colour` (0–15), не по внешнему виду
- Другие копы: реагируют на SOUND_HEY (радиус **0x600 = 1536**)
- Приоритет цели: игрок `score <<= 1` (деприоритет); GUILTY `score >>= 2` (высокий приоритет)

**MIB (18):**
- Mass aggro через `THING_find_sphere(radius=0x500=1280)` — агрирует MIB+GUARD+GANG+FIGHT_TEST
- Health=**700**, FLAG2_PERSON_INVULNERABLE (KO невозможен)
- Проверяет `can_a_see_b` **каждый кадр** в NORMAL state

**Bane (19):**
- Всегда в SUMMON state, статичен
- Поднимает до **4** тел; SPARK-дуги каждые `PCOM_get_duration(50) = 160 тиков`
- Электрошок: `dx+dz < 0x60000`; таймер `>= PCOM_get_duration(20) = 640 тиков (~2 сек)` → `Health -= 25`

**Снайпер SHOOT_DEAD (21):**
- Макс. дистанция стрельбы: **0x600 = 1536** units
- ⚠️ "Убрать пистолет после 10 сек без цели" — **закомментирован**

**KILLING state:**
- Дистанция "слишком далеко": цель убегает → **0x150 (~340** units**)**; иначе → **0x250 (~592** units**)**
- Каждые **256 тиков** (PSX: **128**) ищет оружие поблизости → GETITEM
- Каждые **64 тика** агрессивный звук

**LAZY черта:** каждые **0x3f тиков** ищет скамейку в радиусе **0x200 = 512** units

**Воскрешение гражданских:** через **200** кадров не в поле зрения
- ⚠️ **Баг пре-релиза:** teleport на HomeX/HomeZ вычисляется но не применяется — воскресают на месте смерти

**pcom_zone — 4-bit bitmask:**
- PAP_Hi.Flags bits **10–13** = zone ID; `pcom_zone == 0` → нет ограничений

**Собаки (canid.cpp):** в пре-релизе **полностью инертны** — главный dispatch switch закомментирован ⚠️

---

## 6. Навигация (MAV + Wallhug)

**MAV — сетка и память:**
- Размер: **128×128** ячеек
- `MAV_nav`: **128×128×2 байта = 32 КБ** (10 бит = индекс в MAV_opt, 4 бит = car nav, 2 бит = spare/water)
- `MAV_opt`: ≤**1024** записей × 4 байта = **4 КБ** (`struct { UBYTE opt[4]; }` — 4 направления)
- `MAV_flag`: **128×64 = 8 КБ** (4 бит/ячейка)
- `MAV_dir`: **128×32 = 4 КБ** (2 бит/ячейка)

**Алгоритм MAV_do — greedy best-first (НЕ A*):**
- Нет g-cost (накопленной стоимости) — только эвристика
- Горизонт: `MAV_LOOKAHEAD = 32` ячейки
- Priority queue: **256** записей
- При переполнении: до **8** попыток (MAV_MAX_OVERFLOWS), затем лучший частичный путь
- Случайное смещение: `+0..3` для GOTO, `+3` для остальных (естественность движения)

**Эвристика:**
```cpp
UBYTE MAV_score_pos(x, z) {
    SLONG dist = QDIST2(abs(dest_x-x)<<1, abs(dest_z-z)<<1);
    return min(dist, 255);
}
```

**Действия движения (MAV_ACTION_*):** GOTO(0) JUMP(1) JUMPPULL(2) JUMPPULL2(3) PULLUP(4) CLIMB_OVER(5) FALL_OFF(6) LADDER_UP(7)

**Флаги возможностей (MAV_CAPS_*):** GOTO=0x01 JUMP=0x02 JUMPPULL=0x04 JUMPPULL2=0x08 PULLUP=0x10 CLIMB_OVER=0x20 FALL_OFF=0x40 LADDER_UP=0x80; DARCI=**0xff** (умеет всё)

**Предрасчёт MAV (при загрузке):**
- dh=0–2, нет забора → GOTO; есть забор → CLIMB_OVER
- dh<0 (падение) → FALL_OFF или CLIMB_OVER
- dh=2–4 вверх → PULLUP; dh=1–2, нет прямого пути → JUMP
- dh=2–5 → JUMPPULL; >0 на 2 клетки → JUMPPULL2
- Скользкие поверхности (наклон > **100** ед.) → навигация отключена
- Вода: текстуры **454, 99456, 99457** → бит `MAV_SPARE_FLAG_WATER`
- Prim 41 (ступенька): особая обработка узких лестниц

**Масштаб:**
- 1 ячейка MAV = **256** мировых единиц (сдвиг на 8 бит)
- Высота: `0x40 = 64` = 1 блок (SBYTE-шкала)

**Крыши:**
- `PAP_FLAG_HIDDEN` + roof walkable face → `MAVHEIGHT = roof_face.Y / 0x40`
- `PAP_FLAG_HIDDEN` без roof face → MAVHEIGHT = **127 + PAP_FLAG_NOGO** (недоступно)
- Скайлайт prim **226** → удаляет 2 roof-face; prim **227** (large) → **2×3** блок

**LOS (MAV_can_i_walk) — path shortcutting:**
- Проверяет только `MAV_CAPS_GOTO`; прыжки/climb — только cell-by-cell
- Диагональный случай: дополнительно проверяет **2 угловые ячейки**

**Динамическое обновление:**
- `MAV_turn_movement_off` — очищает **только** GOTO бит
- ⚠️ `MAV_turn_movement_on` — **заменяет** весь набор caps на GOTO (остальные caps теряются!)
- Взрывы/разрушения стен MAV **НЕ обновляют** (только при загрузке + Shift+J в debug) ⚠️

**INSIDE2 навигация — баги пре-релиза:**
- `INSIDE2_mav_inside` → `ASSERT(0)` — **полная заглушка** ⚠️
- `INSIDE2_mav_stair`/`mav_exit`: баг `> 16` вместо `>> 16` → GOTO на месте ⚠️
- nav-сетка до **32×32** ячеек; Баг: цикл по Z использует MinX..MaxX ⚠️

**Навигация складов (WARE_mav_*):**
- Swap глобального `MAV_nav` указателя на `WARE_nav[ww->nav]`
- WARE_mav_exit: фильтр по высоте двери `|height_out - height_in| < 0x80`
- Упрощённые правила: нет `both_ground` проверки, max dh = **1** всегда

**Wallhug:**
- Сетка **128×128**, координаты UBYTE (0–127), 4 кардинальных направления
- `wallhug_path` — до **252** вэйпойнтов; `WALLHUG_MAX_COUNT = 20000` шагов
- LOS-оптимизация: `MAX_LOOKAHEAD = 4`; safety limit = **10** итераций
- Два hugger'а одновременно: left-hand(handed=-1) + right-hand(handed=+1)
- `NAV_Waypoint` (8 байт): x(UWORD) z(UWORD) flag(UBYTE) length(UBYTE) next(UWORD)

**Выбор системы:**
- Улица → `MAV_do()`
- Склад (`FLAG_PERSON_WAREHOUSE`) → `WARE_mav_*()`
- INSIDE2-здание → `INSIDE2_mav_*()` (⚠️ в пре-релизе не работает)
- `Person.InWay` — единственный механизм избегания толпы (отдельной системы нет)

---

## 7. Транспорт

- **MAX_VEHICLES = 40**; одновременных коллизий `VEH_MAX_COL = 8`; переезжаемых персонажей `MAX_RUNOVER = 8`
- Типы: VAN(0), CAR(1), TAXI(2), POLICE(3), AMBULANCE(4), JEEP(5), MEATWAGON(6), SEDAN(7), WILDCATVAN(8)
- `Health`: начало **300**, при ≤0 — взрыв (PYRO_FIREBOMB + S_EXPLODE_MEDIUM)
- `damage[6]`: 6 зон, 0–4 уровня; средние зоны [2/3] усредняются из соседних

**Скорости:**
- `VEH_SPEED_LIMIT = 750` (~35 mph, вперёд); `VEH_REVERSE_SPEED = 300` (назад)
- `Steering`: -64..+64; `Dir`: +2=вперёд, +1=торм., -1=торм.назад, -2=назад, 0=стой

**Параметры двигателей (accel_fwd, accel_rev, soft_brake, hard_brake):**
```c
ENGINE_LGV: 17, 10, 4, 8   // фургон
ENGINE_CAR: 21, 10, 4, 8   // легковой
ENGINE_PIG: 25, 15, 5, 10  // полицейский
ENGINE_AMB: 25, 10, 5, 8   // скорая
```

**Рулевое управление:**
- `WHEELTIME = 35` кадров до полного поворота; `WHEELRATIO = 45`
- `SKID_START = 3` кадра; `SKID_FORCE = 8500`; `NEAR_SKID_FORCE = 5000` (звук без заноса)
- Skid start: `VelR = Velocity >> 6`; escalation: `VelR = Velocity >> 5`

**Подвеска:**
- `MIN_COMPRESSION = 13 << 8 (0x0D00)`; `MAX_COMPRESSION = 115 << 8 (0x7300)`
- Сжатие >50% → Smokin=TRUE; Tilt/Roll clamp: **±312** game units (~27.5°)
- Rest compression ≈ 4300; rest Y = `(PAP_height + 107) << 8`
- Velocity damping: `vel = ((vel << 4) - vel) >> 4` = vel × 15/16

**Физика:**
- `UNITS_PER_METER = 128`; `TICKS_PER_SECOND = 80`
- `GRAVITY = -(128 * 10 * 256) / (80²) = -51`

**Friction:**
```c
base friction = 7 (bit shift)
new_vel = ((vel << friction) - vel) >> friction  // ≈ vel × (1 - 1/2^7)
// Terminal velocity — emergent, НЕ hard cap
```

**Runover damage:**
```c
damage = |VelX*tx + VelZ*tz| / (distance * 200)  // минимум 10 HP
```

- Дым: `Health < 128`, chance = `0x7f - Health`
- ⚠️ Мотоциклы (BIKE): код существует, **но в финальную игру не вошли** — не раскапывать

---

## 8. Управление и ввод

- `MAX_PLAYERS = 2` (split-screen); типы: PLAYER_NONE(0), DARCI(1), ROPER(2), COP(3), THUG(4)
- **18 кнопок INPUT_*** (0–17); `INPUT_MASK_ALL_BUTTONS = 0x3ffff`
- `RedMarks`: **10 нарушений = немедленный GS_LEVEL_LOST**
- `LastReleased[16]` / `DoubleClick[16]`: double-click если кнопка нажата в течение **200ms** после отпускания

**Аналоговые стики:**
```c
GET_JOYX(input) = (((input>>17)&0xfe)-128)  // биты 17–24
GET_JOYY(input) = (((input>>24)&0xfe)-128)  // биты 24–31
```

- Режимы персонажа: RUN(0), WALK(1), SNEAK(2), FIGHT(3), SPRINT(4)
- Клавиатура (DirectInput коды): стрелки 200/203/205/208, Z=44(punch), X=45(kick), C=46(action), V=47(move/run), пробел=57(jump)

**Zipwire sub-states:**
- `SUB_STATE_DANGLING_CABLE(185)`, `_FORWARD(186)`, `_BACKWARD(187)`, `SUB_STATE_DEATH_SLIDE(189)`
- Звук `S_ZIPWIRE` pitch = `224 + velocity/4`; позиция вдоль троса: 0–4096, отступ 4–252/256

**PSX-специфика:**
- **4 конфига кнопок** (pad_cfg0–pad_cfg3); cfg0: D-Up=RUN, Triangle=KICK, Square=PUNCH, Circle=ACTION, Cross=JUMP, L1=CAMERA, R1=STEP_RIGHT
- Аналог DualShock (ID=7) эмулирует D-Pad, **порог ±96** (~75%); правый стик не используется
- Вибрация: `psx_motor[0]` малый (÷2/тик), `psx_motor[1]` большой (×7/8/тик)
- PC мёртвая зона `NOISE_TOLERANCE = 8`; PSX `= 96`; Frontend `= 4096`
- PSX частицы DIRT_set_focus радиус `0x800`; PC `0xC00` (+50%)

---

## 9. Бой и оружие

**Боевая механика:**
- Коллизия удара — через `GameFightCol` встроенную в кадры анимации (не физический движок) ⚠️
- Углы в **2048-градусной** системе; допуск угла удара `FIGHT_ANGLE_RANGE = 400` (±200)
- Высота удара: 0=низкая(ноги), 1=средняя(торс), 2=высокая(голова)
- `abs(dy) > 100` по Y = автопромах; `COMBAT_HIT_DIST_LEEWAY = 0x30`
- Стомп: расстояние до 3 точек тела (таз, голова, правая голень) < **0x53**
- **Минимальный урон = 2**; **макс. здоровье = 200**

**Модификаторы урона:**
- KICK_NAD: +50 (муж.) / +10 (жен.); KICK_RIGHT/LEFT: +30; KICK_NEAR: `damage=40`; KICK_BEHIND: +20
- BAT_HIT1/HIT2: +20; FLYKICK: +20; STOMP: +50
- KNIFE_ATTACK1/2/3: `damage=30/50/70`
- Roper стреляет: ×2; Roper ближний: +20
- Игрок стреляет: `+player.Skill`; Игрок ближний: `+player.Strength`
- Жертва-игрок: `damage >>= 1`; жертва-NPC: `damage -= GET_SKILL(victim)`, минимум 2

**Формула попадания:**
```c
base = 230 - abs(Roll)>>1 - Velocity
+ 64 (игрок целится)
- 64 (AI → игрок)
+ 100 (AI vs AI)
- dist>>3 (pistol/AK если dist > 0x400)
- dist>>2 (shotgun если dist > 0x400)
/ 2 (цель делает flip/dodge)
// Min: 20/256 (~8%)
```

**Блок:**
- Только против melee (type==0 или type > HIT_TYPE_GUN_SHOT_L); guns игнорируют блок
- AI вероятность блока: base = `60 + GET_SKILL * 12`; cap = `150 + GET_SKILL * 5`; если атакующий не виден: `/= 2`

**Fight tree (комбо):**
- Punch/Kick: COMBO1(dmg 10) → COMBO2(30) → COMBO3(60, KO)
- Cross-combo финишеры: 30, 80, 80 (nodes 11–13)
- Knife: 30 → 60 → 80 (nodes 14, 16, 18); Bat: 60 → 90 (nodes 19, 21)
- KO → `Agression = -60`

**Взрывы (shockwave):**
```c
hitpoints = maxdamage * (radius - dist) / radius
// нокдаун если hitpoints > 30; jumping/flipping: hitpoints /= 2 + 1
// SHOCKWAVE_FIND = 16 (макс. сущностей)
```

**Оружие — вместимость и урон:**
- Пистолет: **15** патронов, урон **70**
- Дробовик: **8** патронов, урон `300 - dist`, max range ~`0x600`
- АК-47: **30** патронов, урон игрок **100** / NPC **40**
- Гранаты (стак подбора): **3**, макс. стак **8**; C4 стак **4**

**Дальность (блоки = 256 ед.):** MIB/АК-47=**8 (2048)**; Пистолет=**7 (1792)**; Дробовик=**6 (1536)**

**Скорострельность (тики):** Пистолет=**140**; Дробовик=**400**; АК-47=**64**

**Параметры взрывов:**

| Тип | Радиус | Max урон |
|-----|--------|---------|
| Mine | **0x200** | **250** |
| Grenade | **0x300** | **500** |
| C4 | **0x500** | **500** |

- Граната: активация `timer = 1920` тиков (**6 секунд**)
- C4: `timer = 1600` тиков (**5 секунд**); ⚠️ комментарий в коде говорит "10 seconds" — баг в комментарии
- Bounce: `dy = abs(dy) >> 1`; если `dy < 0x400` → полная остановка

**Автоприцеливание:**
```c
Score = (8<<8 - dist) × (MAX_DANGLE - dangle) >> 3
MAX_NEW_TARGET_DANGLE = 2048/7 ≈ 292 (±~51°)
// При беге (SUB_STATE_RUNNING): конус сужается ×4 (±~13°)
```
- Враг атакует игрока: ×4; Thugs/MIB: ×2; Копы (для Darci): ×0.5; Гражданские: ×180/256

**DIRT (банки колы):**
- `DIRT_MAX_DIRT = 256` (PC); `prob_can=1` vs `prob_leaf=100` → ~**2–3 банки** из 256 слотов
- `DIRT_create_cans()` спавнит **5 банок** при столкновении бочек
- ⚠️ `SPECIAL_MINE (10)` — нельзя подобрать (заглушено в `should_person_get_item()`)

**Faction rules:**
- DARCI не бьёт копов/Roper без вины; COP — только меченых; MIB нельзя нокаутировать (кроме FIGHT_TEST)
- `GangAttack { Owner; Perp[8]; }` — до **8 атакующих** с **8 направлений**

---

## 10. Взаимодействие

- `find_grab_face()`: двухпроходный поиск — Pass 0: hi-res (roof faces), Pass 1: lo-res (terrain)
- Y-диапазон проверки: **±256 + dy** от позиции игрока
- Угол к поверхности: **±200°** от направления
- ⚠️ `valid_grab_angle()` — **ВСЕГДА возвращает 1** (валидация отключена)

**Типы grab:**
- `type=0` — обычная поверхность
- `type=1` — cable (трос/zipwire)
- `type=2` — ladder (лестница)

**Cable параметры** (упакованы в поля DFacet — переиспользование):
```c
angle_step1 = (SWORD)p_facet->StyleIndex   // кривизна сегмента 1
angle_step2 = (SWORD)p_facet->Building     // кривизна сегмента 2
count       = p_facet->Height              // количество шагов
```

- Лестница: нижний порог `bottom ≤ Y ≤ top + 64`; возвращает `-1` если игрок выше лестницы
- ⚠️ **Баг пре-релиз:** `mount_ladder()` из `interfac.cpp` закомментирован → игрок не может залезть снизу; AI (pcom.cpp:12549) может

**Кости для позиционирования:** `SUB_OBJECT_LEFT_HAND`, `RIGHT_HAND`, `LEFT_FOOT`, `RIGHT_FOOT`; захардкоженные `FOOT_HEIGHT`, `HAND_HEIGHT`

---

## 11. Миссии EWAY

**Формат .ucm:**
- **Заголовок: 1316 байт** (4+4+260×5+2+2+2+1+1)
- **EventPoint = 74 байта** (14 header + 60 data)
- **MAX_EVENTPOINTS = 512** → **37 888 байт** EP-секция
- Версии: 0–10 (текущая `M_VERSION = 10`, `EP_VERSION = 1`)
- `version > 5` → пропуск `SkillLevels[254]`; `version > 8` → FAKE_CARS + MUSIC_WORLD

**WPT→EWAY трансляция (в elev.cpp, строки 748–1654):**
- ⚠️ `.ucm` хранит WPT_* (редакторские), runtime использует EWAY_DO_* — **разные нумерации**
- ⚠️ `WPT_BONUS_POINTS` → всегда `EWAY_DO_MESSAGE` из-за `if(0)` блока — очки **не начисляются**
- ⚠️ `WPT_GOTHERE_DOTHIS (39)` — не реализован, попадает в `default: ASSERT(0)`

**EWAY_process() — модель выполнения:**
- Вызывается **каждый кадр** из main game loop
- Индекс **0 зарезервирован**, никогда не используется

**42 условия EWAY_COND_* (0–41):**
- ⚠️ `EWAY_COND_PRESSURE (4)` — **STUB, always FALSE**
- ⚠️ `EWAY_COND_CAMERA (5)` — **STUB, always FALSE**
- `EWAY_COND_TIME (7)` — arg1 = время в **1/100 сек**

**57 действий WPT_* (0–57) → 52 EWAY_DO_***

**CRIME_RATE:**
- Загружается из `.iam`; если 0 → устанавливается **50** (дефолт); диапазон: **0–100**
- Убийство guilty NPC: **-2**; убийство civ: **+5**; арест guilty: **-4**
```c
for_guilty = num_guilty_people * 4;
SATURATE(for_guilty, 0, CRIME_RATE >> 1);
CRIME_RATE_SCORE_MUL = (CRIME_RATE - for_guilty) << 8 / total_objective_points;
```
- `EWAY_counter[7]` — счётчик убийств PERSON_COP (max **255**)

**Spawn-детали:**
- Координаты вэйпойнта в mapsquare; `EWAY_create_enemy()` умножает на **256** (`<< 8`)
- Yaw в файле: `yaw >> 3`; при spawn: `yaw << 3`
- zone byte: биты 0-3 = spawn zone; бит 4 = `FLAG2_PERSON_INVULNERABLE`; бит 5 = GUILTY; бит 6 = FAKE_WANDER
- ⚠️ **Roper stats bug:** `EWAY_create_player(PLAYER_ROPER)` использует Darci-статы

**Непрерывные эффекты:**
- `EWAY_DO_WATER_SPOUT` → `DIRT_new_water()` **4×** за кадр (крест: ±X, ±Z)
- `EWAY_DO_SHAKE_CAMERA` → `FC_cam[0].shake += 3` (PC) / `+6` (PSX) до max **100**

**Post-load fixup (EWAY_created_last_waypoint):**
- Resolve ID→array index; подсчёт guilty людей; инициализация COUNTDOWN таймеров
- CAMERA_TARGET_THING → attach к ближайшему CREATE_* в радиусе **0x300**

---

## 12. Здания и интерьеры

**Иерархия:**
- FBuilding (max **500**) → FStorey (max **2500**) → FWall (max **15000**)
- Компилируется в: DBuilding (max **1024**) → DFacet (max **16384**)
- **BLOCK_SIZE = 64** мировых единицы (1 ячейка сетки зданий)

**21 тип STOREY_TYPE_* (1–21):**
```
1=NORMAL, 2=ROOF, 3=WALL, 4=ROOF_QUAD, 5=FLOOR_POINTS, 6=FIRE_ESCAPE
7=STAIRCASE, 8=SKYLIGHT, 9=CABLE, 10=FENCE, 11=FENCE_BRICK, 12=LADDER
13=FENCE_FLAT, 14=TRENCH, 15=JUST_COLLISION, 16=PARTITION, 17=INSIDE
18=DOOR, 19=INSIDE_DOOR, 20=OINSIDE, 21=OUTSIDE_DOOR
```
- Тип 9=CABLE (тросы/death-slide) — есть в финальной игре!
- Тип 21 (OUTSIDE_DOOR) → активирует `FStorey.InsideStorey` → переключение рендера inside/outside

**DFacet — 16 байт:**
- `Open` (0–255) — степень открытия двери
- Здания **неуязвимы** (нет damage системы)

**Двери:**
- `DOOR_MAX_DOORS = 4` одновременно
- Шаг анимации: **+4 за кадр**; без `FACET_FLAG_90DEGREE`: 0→255 (180°); с флагом: 0→128 (90°)
- При открытии → `MAV_turn_movement_on()` для навигации ⚠️ (caps заменяются на GOTO only)

**Лестницы:**
- `STAIR_MAX_STAIRS = 32`, до **3 на этаж** (STAIR_MAX_PER_STOREY)
- Scoring: `outside==2` (обе у стены): **+0x10000**; `outside==1`: **-0x10000**
- Совпадение с краем bbox: **+0x5000**; подсказка "opposite wall": **+0xbabe**; блокировка двери: **-INFINITY**

**Процедурные интерьеры (id.cpp ~9300 строк):**
- `ID_MAX_FITS = 32` сидов при `find_good_layout==TRUE`
- `num_walls = ID_floor_area >> 4 + 1`; apartment: `>> 3 + 2`
- BFS queue: **64 элемента**
- Scoring: `ratio = max(dx,dz)/min(dx,dz) × 256`; цель = **414** (золотое сечение)
- RoomID: до **16 комнат**, до **10 лестничных позиций**
- `ID_should_i_draw(x,z)` — видимость через RoomID

**Склады (WARE):**
- `WARE_MAX_WARES = 32`
- Пулы: `WARE_nav[4096]`, `WARE_height[8192]`, `WARE_rooftex[4096]`
- `PAP_LO_FLAG_WAREHOUSE` — флаг на PAP_Lo для ячеек внутри склада
- ⚠️ Размещение door prims **закомментировано** (`#if 0`)

---

→ Продолжение: [DENSE_SUMMARY_2.md](DENSE_SUMMARY_2.md)
