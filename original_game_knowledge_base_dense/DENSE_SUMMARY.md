# DENSE SUMMARY — Urban Chaos Knowledge Base

**Назначение:** Сверхкомпактная версия всей KB для задач требующих полного контекста (архитектурные решения, поиск зависимостей, ответы на сложные вопросы).
**Когда грузить:** многоподсистемные задачи, архитектурный анализ, аудит KB.
**Когда НЕ грузить:** работа над одной подсистемой → достаточно SESSION_START + subsystems/X.md.
**Детали:** все subsystems/*.md и physics_details.md, ai_behaviors.md, etc.

---

## 1. Игровой цикл и время

**Порядок за кадр (PC):**
```
GAMEMENU_process() → tutorial_string → process_controls() → check_pows()
→ process_things() → PARTICLE_Run() → OB_process() → TRIP_process()
→ DOOR_process() → EWAY_process() → SPARK_show_electric_fences()
→ RIBBON_process() → DIRT_process() → ProcessGrenades() → WMOVE_draw()
→ BALLOON_process() → MAP_process() → POW_process() → FC_process()
→ [always] PUDDLE_process() → DRIP_process()
→ draw_screen() → OVERLAY_handle() → GAMEMENU_draw()
→ screen_flip() → lock_frame_rate(30) → handle_sfx() → GAME_TURN++
→ bench cooldown check: (GAME_TURN & 0x3ff)==314 → сброс GF_DISABLE_BENCH_HEALTH
```

**Ключевые константы:**
- FPS cap: `lock_frame_rate(30)` — spin-loop busy-wait
- `PCOM_TICKS_PER_SEC = 320` (16 тиков/кадр × 20 кадров/сек)
- `POW_TICKS_PER_SECOND = 2000`
- `SMOOTH_TICK_RATIO` = 4-кадровое скользящее среднее TICK_RATIO (для машин)
- `TIMEOUT_DEMO = 0` → demo_timeout() = мёртвый код
- `GAME_TURN` сбрасывается в 0 при каждой загрузке уровня
- Ночью всегда дождь: `if (!(NIGHT_FLAG_DAYTIME)) GAME_FLAGS |= GF_RAINING` — хардкод

**Пауза:** `GF_PAUSED` → `should_i_process_game() = FALSE` → пропускает process_things..FC_process
**PSX EWAY:** шаг = 2 (чётные/нечётные кадры); PC: шаг = 1 (каждый кадр)

---

## 2. Система координат и карта

**Базовые типы:**
- Мировые координаты: `SLONG` (32-bit signed integer), **без float в физике**
- 256 мировых единиц = 1 mapsquare (PAP_Hi)
- 1024 мировых единицы = 1 PAP_Lo ячейка (4×4 mapsquares)

**Преобразования:**
```c
world → mapsquare_hi: >> 8
world → maplo_cell:   >> 18  (= >> (8+10))
PAP_Hi.Alt → реальная высота: << 3  (PAP_ALT_SHIFT = 3)
PAP_Lo.water при загрузке: = -128 >> 3 = -16; PAP_LO_NO_WATER = -127
```

**Карта:**
- PAP_Hi: 128×128 ячеек (16 384), 6 байт/ячейка = ~96 KB
- PAP_Lo: 32×32 ячеек (1 024), 8 байт/ячейка
- Полный мир: 128×256 = 32 768 мировых единиц по X и Z
- `water_level = -128` хардкод (мировые единицы)

**Вращение:** UBYTE 0-255 = полные 360°: 0=восток(+X), 64=юг(+Z), 128=запад(-X), 192=север(-Z)

**Важные флаги PAP_Hi:**
- `PAP_FLAG_HIDDEN` (1<<4) = под крышей/внутри — используется MAV для крыш
- `PAP_FLAG_NOGO` (1<<8) = AI не ходит; `PAP_FLAG_WATER` (1<<15) = вода (невидимые стены)
- `PAP_FLAG_ZONE1..4` (биты 10-13) = AI-зоны (4-bit bitmask = 16 зон)
- `PAP_FLAG_ROOF_EXISTS` (1<<9) — переиспользует бит ANIM_TMAP (контекстно-зависимый!)
- `PAP_FLAG_WANDER` (1<<14) — переиспользует бит FLAT_ROOF

**MapWho:** `PAP_Lo[x][z].MapWho` → голова linked list объектов; `Thing->Child/Parent` — двусвязный список. `move_thing_on_map()` вызывается только при смене PAP_Lo ячейки (каждые 1024 ед.).

---

## 3. Thing System — игровые объекты

**Thing (125 байт) — всё в игре:**
```c
struct Thing {
    UBYTE Class, State, OldState, SubState;
    ULONG Flags;                    // 32 флага
    THING_INDEX Child, Parent;      // иерархия
    THING_INDEX LinkChild, LinkParent; // управление памятью
    GameCoord WorldPos;             // SLONG X,Y,Z
    void (*StateFn)(Thing*);        // функция состояния (вызывается КАЖДЫЙ кадр)
    union { DrawTween*, DrawMesh* } Draw;
    union { Vehicle/Furniture/Person/... } Genus;
    UBYTE DrawType; UBYTE Lead;
    SWORD Velocity, DeltaVelocity, RequiredVelocity, DY;
    UWORD Index, OnFace, NextLink, DogPoo1;
    THING_INDEX DogPoo2;
};
```

**Классы:** NONE(0), PLAYER(1), CAMERA(2), PROJECTILE(3), BUILDING(4), PERSON(5), ANIMAL(6), FURNITURE(7), SWITCH(8), VEHICLE(9), SPECIAL(10), ANIM_PRIM(11), CHOPPER(12), PYRO(13), TRACK(14), PLAT(15), BARREL(16), BIKE(17), BAT(18), END(19)

**Лимиты:** PRIMARY=400 + SECONDARY=300(PC)/50(PSX) = MAX_THINGS=700; RMAX_PEOPLE=180; MAX_VEHICLES=40; MAX_SPECIALS=260; MAX_PYROS=64

**Управление памятью:** PRIMARY_USED/UNUSED двусвязные списки; O(1) alloc/free. Итерация по классу: `thing_class_head[CLASS] → NextLink`.

**Поиск:**
```c
THING_find_sphere(cx,cy,cz, radius, array, size, class_mask)  // по сфере
THING_find_box(x1,z1, x2,z2, array, size, class_mask)         // по прямоугольнику
THING_find_nearest(cx,cy,cz, radius, class_mask)
// Маски: THING_FIND_PEOPLE=(1<<5), THING_FIND_LIVING=PERSON|ANIMAL
```

**Состояния (STATE_*):** INIT(0), NORMAL(1), COLLISION(2), ABOUT_TO_REMOVE(3), REMOVE_ME(4), MOVEING(5), IDLE(6), LANDING(7), JUMPING(8), FIGHTING(9), FALLING(10), USE_SCENERY(11), DOWN(12), HIT(13), CHANGE_LOCATION(14), DYING(16), DEAD(17), DANGLING(18), CLIMB_LADDER(19), HIT_RECOIL(20), CLIMBING(21), GUN(22), SHOOT(23), DRIVING(24), NAVIGATING(25), WAIT(26), FIGHT(27), STAND_UP(28), MAVIGATING(29), GRAPPLING(30), GOTOING(31), CANNING(32), CIRCLING(33), HUG_WALL(34), SEARCH(35), CARRY(36), FLOAT(37)

---

## 4. Физика и коллизии

**Ключевые константы:**
- `GRAVITY = -51` (units/frame²) — **НЕ -5120!** Ошибка была исправлена в итерации 2.
- Fall damage: `(-DY - 20000) / 100`; порог смерти: игрок=-12000 / NPC=-6000
- `NOGO push = 0x5800`; `SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT = -0x50` (-80 ед.)
- `HyperMatter: gy=0` хардкод — объекты отскакивают от Y=0, не от реального terrain

**Функции высоты:**
1. `PAP_calc_height_at_point(mx,mz)` — прямой lookup угловой точки
2. `PAP_calc_height_at(x,z)` — билинейная интерполяция (4 угла, диагональное разбиение на треугольники)
3. `PAP_calc_height_at_thing(thing,x,z)` — с учётом склада/здания
4. `PAP_calc_map_height_at(x,z)` — максимум terrain и зданий
5. `PAP_calc_height_noroads(x,z)` — без бордюров

**find_face_for_this_pos()** (walkable.cpp, НЕ collide.cpp!): порог кандидата 0xa0=160 ед.; режимы FIND_ANYFACE=1, FIND_FACE_NEAR_BELOW=3; отрицательный индекс = roof face.

**move_thing() — порядок (только CLASS_PERSON):**
1. Граница 128×128 → return 0
2. `collide_against_things()` — сфера radius+0x1a0
3. `collide_against_objects()` — OB в радиусе ±0x180; скамейки 89/95/101/102/105/110/126 + SUB_STATE_WALKING_BACKWARDS → `set_person_sit_down()`
4. `slide_along()` — DFacet зданий; STATE_HUG_WALL: radius >>= 2
5. `slide_along_edges/redges()` — только STATE_CIRCLING, SUB_STATE_STEP_FORWARD, walking player, FIRE_ESCAPE
6. `find_face_for_this_pos()` — новый face; y_diff > 0x50 → FALL_OFF_FLAG_TRUE
7. Fall-through workaround: `x2 += dx>>2; z2 += dz>>2`
8. `DIRT_gust(), MIST_gust(), BARREL_hit_with_sphere()`
9. `move_thing_on_map()`

**slide_along() особенности:**
- Нет отскока (restitution=0). Нет трения. **Все DFacets оси-aligned** (нет диагональных стен).
- Глобальные переменные: `actual_sliding, last_slide_colvect, fence_colvect, slide_door, slide_ladder, slide_into_warehouse`
- STOREY_TYPE_CABLE: игнорируется (провода не блокируют)
- STOREY_TYPE_OUTSIDE_DOOR + FACET_FLAG_OPEN: игнорируется (открытая дверь)
- `extra_wall_height = -0x50`: стена "ниже" на 80 ед. → персонаж перешагивает порог

**move_thing_quick():** без коллизий — телепортация (ragdoll, knockback, катсцены)

**Лестницы:** `mount_ladder()` закомментирован из interfac.cpp (пре-релиз) → игрок не может залезть снизу. AI через pcom.cpp:12549 — может.

**Коллизионные структуры:**
- `col_vects[10000]` (PC) — линейные барьеры; `col_vects_links[10000]` — привязка к ячейке
- `walk_links[30000]` (PC) — ходимые поверхности на ячейку

**Вода:** `PAP_FLAG_WATER` = невидимые стены; нет плавания, нет утопания; огонь гасится
**WMOVE:** максимум 192 движущихся поверхностей; PSX: только VAN/WILDCATVAN/AMBULANCE

---

## 5. AI система (PCOM)

**22 типа AI (PCOM_AI_*):** NONE(0), CIV(1), GUARD(2), ASSASIN(3), BOSS(4), COP(5), GANG(6), DOORMAN(7), BODYGUARD(8), DRIVER(9), BDISPOSER(10), BIKER(11,#ifdef BIKE не переносить), FIGHT_TEST(12), BULLY(13), COP_DRIVER(14), SUICIDE(15), FLEE_PLAYER(16), KILL_COLOUR(17), MIB(18), BANE(19), HYPOCHONDRIA(20), SHOOT_DEAD(21)

**28 состояний AI (PCOM_AI_STATE_*):** PLAYER(0), NORMAL(1), INVESTIGATING(2), SEARCHING(3,STUB!), KILLING(4), SLEEPING(5,STUB!), FLEE_PLACE(6), FLEE_PERSON(7), FOLLOWING(8), NAVTOKILL(9), HOMESICK(10), LOOKAROUND(11,только счётчик!), FINDCAR(12), BDEACTIVATE(13), LEAVECAR(14), SNIPE(15), WARM_HANDS(16), FINDBIKE(17,#ifdef), KNOCKEDOUT(18), TAUNT(19), ARREST(20), TALK(21), GRAPPLED(22), HITCH(23), AIMLESS(24), HANDS_UP(25), SUMMON(26), GETITEM(27)

**Черты (PCOM_BENT_*):** LAZY(0,скамейка каждые 0x3f тиков), DILIGENT(1), GANG(2), FIGHT_BACK(3), ONLY_KILL_PLAYER(4), ROBOT(5), RESTRICTED(6,нет прыжков/лазания), PLAYERKILL(7)

**Поток AI за кадр:**
```
PCOM_process_person():
    StateFn(p_person)           // анимационный state machine
    if PlayerID → [танец join логика] → return
    PCOM_process_movement()     // низкоуровневое движение
    if HELPLESS → return
    if DEAD: if CIV+WANDER+!IN_VIEW: counter++→>200 → resurrect
    else: PCOM_process_state_change()  // main AI dispatcher
```

**Видимость (can_a_see_b):**
- Дальность: NPC = 2048 ед. (по умолчанию); игрок = 65536 (всегда видит)
- Освещение влияет: `view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256`
- FOV: < 192 ед. → 700/2048 (≈123°); ≥ 192 ед. → 420/2048 (≈74°); "краем глаза" 250/2048 ≈ 44°
- Eye height: стоя 0x60=96 ед.; на корточках 0x20=32 ед.
- Присевший: view_range >>= 1; движущийся: + 256

**Звуковые радиусы (PCOM_SOUND_*):** FOOTSTEP=640, VAN=384, DROP=512, MINE/GRENADE=768, UNUSUAL/FIGHT_source/HEY=1536-2304, GUNSHOT=2560 (max). Зоны и стены складов блокируют.

**Воскрешение гражданских (WANDER):** >200 кадров + !IN_VIEW → heal+teleport. **Баг пре-релиза:** `newpos` вычислен но НЕ присвоен → воскрешают на месте смерти (вместо home).

**MIB-специфика:** Health=700, FLAG2_PERSON_INVULNERABLE → KO невозможен. Mass aggro radius = 1280 (can_a_see_b каждый кадр). **Исключение:** PCOM_AI_FIGHT_TEST умирает от комбо даже если invulnerable.

**zone byte в EWAY_create_enemy:** bits 0-3=spawn zone, bit4=INVULNERABLE, bit5=GUILTY, bit6=FAKE_WANDER
**KILLING→FLEE:** ТОЛЬКО для FAKE_WANDER NPC (Rasta/Cop); обычные враги → NAVTOKILL/NORMAL
**pcom_zone:** PAP_Hi.Flags биты 10-13 = 4-bit bitmask; zone!=0 → фильтр звука/видения/NAVTOKILL

**Группировка банд:** `pcom_colour` (0–15), до 64 членов суммарно. `PCOM_alert_my_gang_to_a_fight()` → все NORMAL/WARM_HANDS/FOLLOWING/SEARCHING/TAUNT/INVESTIGATING → KILLING.

**Числовые константы:**
- `PCOM_TICKS_PER_TURN = 16` тиков/кадр
- `PCOM_ARRIVE_DIST = 0x40 = 64` ед.
- `PCOM_get_duration(n) = n * 32` тиков (из десятых секунды)
- `PCOM_BENT_LAZY`: скамейка каждые `0x3f` тиков в радиусе `512`
- Коп: сканирует радиус `0x800` каждые 4 кадра на FELON

---

## 6. Навигация

**Две системы:** MAV (основная, greedy best-first) + NAV/Wallhug (вспомогательная/отладочная)

**MAV — ключевые факты:**
- `MAV_nav[128×128]` UWORD: 10 бит = индекс в MAV_opt[], 4 бит car-nav, 2 бит spare (вода)
- `MAV_opt[≤1024]` — таблица вариантов движения: `opt[4] = {XS,XL,ZS,ZL}` направления
- **НЕ A\***: нет g-cost, только эвристика расстояния → greedy best-first
- Горизонт: `MAV_LOOKAHEAD = 32` ячейки
- Действия: GOTO(0), JUMP(1), JUMPPULL(2), JUMPPULL2(3), PULLUP(4), CLIMB_OVER(5), FALL_OFF(6), LADDER_UP(7)
- Случайное смещение (+0–3 для GOTO, +3 для остальных) — для естественности движения
- `MAV_turn_movement_on()`: **заменяет** весь набор caps на GOTO (теряет прыжки/pull-up!)
- `MAV_turn_movement_off()`: очищает **только** бит GOTO (остальные caps не трогает)
- **Взрывы стен НЕ обновляют MAV** — только при загрузке и через Shift+J (debug)
- MAV_precalculate() — самая тяжёлая операция при загрузке (128×128 ячеек)

**Крыши в MAV:** MAVHEIGHT = roof_face.Y/0x40; без roof_face при PAP_FLAG_HIDDEN → NOGO (-127)

**Склады (WARE_mav_*):** swap указателя `MAV_nav` → стандартный MAV_do() с локальной сеткой
- `WARE_mav_enter`: НЕ меняет MAV_nav; глобальный MAV к точке снаружи двери
- `WARE_mav_inside/exit`: swap MAV_nav + перевод координат
- Отличие exit: проверка `|height_out - height_in| < 0x80` (не выйти через дверь другого этажа)

**INSIDE2_mav_inside = ASSERT(0)** — полная заглушка! NPC внутри INSIDE2 зданий застревают.
**INSIDE2_mav_stair/exit:** баг `> 16` вместо `>> 16` → GOTO на месте.
**INSIDE2_mav_nav_calc:** баг цикла Z: `MinX..MaxX` вместо `MinZ..MaxZ` → неправильная nav-сетка.

**Wallhug:** двусторонний обход препятствий, max 20000 шагов, 252 waypoints, LOS-оптимизация (4 lookahead)

**Масштаб:** 1 MAV ячейка = 256 мировых ед.; высота 0x40 = 1 блок = 64 ед. SBYTE-шкалы

---

## 7. Транспорт

**9 типов:** VAN(0), CAR(1), TAXI(2), POLICE(3), AMBULANCE(4), JEEP(5), MEATWAGON(6), SEDAN(7), WILDCATVAN(8)
`FLAG_FURN_DRIVING` определён в **Furn.h** (не Vehicle.h!)

**Параметры двигателей (accel_fwd, accel_back, soft_brake, hard_brake):**
- ENGINE_LGV: 17/10/4/8; ENGINE_CAR: 21/10/4/8; ENGINE_PIG: 25/15/5/10; ENGINE_AMB: 25/10/5/8

**Физика:**
- Friction bit-shift: `((vel<<f) - vel)>>f` = vel × (1 - 1/2^f); base f=7, engine braking -1, hard braking -4
- Terminal velocity — emergent (нет жёсткого cap, только `VEH_SPEED_LIMIT=750` / `VEH_REVERSE_SPEED=300`)
- 4 пружины подвески; Tilt/Roll ±312 ед. (≈27.5°); velocity damping: `vel × 15/16`
- Skid start: VelR = Velocity >> 6 (random sign); escalation: VelR = Velocity >> 5
- Runover damage: `|VelX*tx + VelZ*tz| / (distance*200)`; минимум 10 HP
- Smoke: Health < 128, chance = `0x7f - Health`

**6 зон урона (damage[0..5]):** FL, FR, LM, RM, RL, RR; Middle zones усредняются из Front/Rear; Health ≤ 0 → PYRO_FIREBOMB + STATE_DEAD

**VEH_find_runover_things():** 2 сферы вперёд; `infront = 512 - |WheelAngle*3|` clamped [256,512]

**Мотоцикл (BIKE):** не в финальной игре (подтверждено Mike Diskett). Код существует в #ifdef BIKE.

**WMOVE:** персонажи стоят на крышах машин (max 192 движущихся поверхностей)

---

## 8. Управление и ввод

**18 кнопок INPUT_\*:** FORWARDS(0), BACKWARDS(1), LEFT(2), RIGHT(3), START(4), CANCEL(5), PUNCH(6), KICK(7), ACTION(8), JUMP(9), CAMERA(10), CAM_LEFT(11), CAM_RIGHT(12), CAM_BEHIND(13), MOVE(14), SELECT(15), STEP_LEFT(16), STEP_RIGHT(17)

**52 ACTION_\*** (animate.h: 688-740): IDLE(0)..CLIMBING(13)..GRAPPLE(43)..INSIDE_VEHICLE(46)..

**process_controls()** = диспетчер подсистем (НЕ обработка ввода игрока!)
**InputDone:** накапливает обработанные биты → кнопка срабатывает один раз за нажатие

**apply_button_input() — приоритеты do_an_action():**
выйти из машины > арест > слезть с лестницы > сесть в машину > крюк > переключатель > разговор > подбор > обыск > обнять стену > кола > присесть

**Ввод без ACTION:** SPRINT→RUN downgrade; crouching → `set_person_idle_uncroutch()`

**Горячие клавиши PC:** KB_1=убрать, KB_2=пистолет, KB_3=шотган, KB_4=AK47, KB_5=граната, KB_6=C4, KB_7=нож, KB_8=бита

**Аналоговый ввод:** `GET_JOYX(input) = ((input>>17)&0xfe)-128`; `GET_JOYY = ((input>>24)&0xfe)-128`

**player_turn_left_right():** wMaxTurn = 94 idle / 12 в воздухе / (70-Velocity) при беге
**should_i_jump():** dx/dz захардкожены (не зависят от угла персонажа — баг/упрощение!)
**Double-click:** `GetTickCount()` WIN32 ms; окно 200ms; не GAME_TURN!

**NOISE_TOLERANCE:** PC=8192 (из 65535); **ANALOGUE_MIN_VELOCITY:** PC=8, PSX=32

**PSX layouts (4 конфига):** cfg0 = Triangle/KICK, Square/PUNCH, Circle/ACTION, Cross/JUMP; L1=CAMERA, R1=STEP_RIGHT, L2=CAM_LEFT, R2=CAM_RIGHT → детали в psx_controls.md

**Zipwire:** `ACTION_DEATH_SLIDE(35)`, `FLAG_PERSON_ON_CABLE(1<<12)`, `STOREY_TYPE_CABLE=9` в геометрии. Есть в финальной игре. В пре-релизе grab закомментирован.

---

## 9. Бой и оружие

### Рукопашный бой

**GameFightCol** встроен в кадры анимации: `{Dist1,Dist2,Angle,Priority,Damage,Tween,AngleHitFrom,Height,Width,Dummy,*Next}`

**Алгоритм попадания (check_combat_hit_with_person):**
1. Жертва не мертва; KO → только ANIM_FIGHT_STOMP
2. QDIST2 по XZ плоскости; `|dy| > 100` = промах
3. Угол: допуск ±200/2048 (FIGHT_ANGLE_RANGE=400)
4. LOS через `there_is_a_los_things()` (без учёта заборов и примов)
5. dist < Dist2 + 0x30 (COMBAT_HIT_DIST_LEEWAY)

**Формула урона:**
- Base = `fight->Damage` + модификаторы типа атаки (KICK_NAD+50, KNIFE_ATTACK3=70, STOMP+50, ...)
- Атакующий: Roper бой+20; игрок + player.Strength; все + GET_SKILL(attacker)
- Жертва-игрок: damage >>= 1; жертва-NPC: damage -= GET_SKILL(victim), мин. 2
- Блок (SUB_STATE_BLOCK): damage = 0 только против melee; огнестрел блок игнорирует

**Knockback:** через выбор анимации из `take_hit[7][2]`, НЕ через физические векторы!
**KO:** `set_person_dead(PERSON_DEATH_TYPE_STAY_ALIVE)`, `Agression=-60`; от COMBO3/COMBO3B

**Fight tree комбо:** punch 10→30→60(KO); kick 10→30→60(KO); cross-combo 30/80/80; knife 30→60→80; bat 60→90

**Граплинг:** {PISTOL_WHIP(только Darci), STRANGLE, HEADBUTT, GRAB_ARM}; требует стоящую жертву, dist точный
**SNAP_KNECK** → мгновенная смерть. Strangle/Headbutt: жертва может вырваться.

### Оружие

**Урон:**
| Оружие | Урон | Rate (тики) |
|--------|------|-------------|
| Пистолет | 70 | 140 |
| Дробовик | 300-dist, max 250 | 400 |
| AK47 (игрок) | 100 | 64 |
| AK47 (NPC) | 40 | 64 |
| Все guns vs игрок | damage >>= 1 | — |

**Hit chance:** `230 - abs(Roll)>>1 - Velocity + 64(прицел) - 64(AI vs игрок) + 100(AI vs AI) - dist modifiers`; минимум 20/256 ≈ 8%

**Взрывы:**
| Тип | Радиус | Max урон |
|-----|--------|----------|
| Mine | 0x200 | 250 |
| Grenade | 0x300 | 500 |
| C4 (EXPLOSIVES) | 0x500 | 500 |
| Bomb | нет shockwave | visual only |

Формула: `hitpoints = maxdamage × (radius-dist) / radius`; KO если > 30; прыгающий: /2+1

**C4:** timer = `16*20*5 = 1600` тиков = **5 секунд** (комментарий "10 сек" в коде — ошибка!)
**Граната:** `SPECIAL_prime_grenade()` — 6 секунд запал; `SPECIAL_throw_grenade()` + DIRT-физика
**SPECIAL_MINE:** подбор = ASSERT(0); метание = ASSERT(0) — нефункциональна
**Магазины:** пистолет 15, шотган 8, AK47 30, гранаты стак 8, C4 стак 4

**30 типов SPECIAL_\*:** NONE(0)..KEY(1)..GUN(2)..HEALTH(3,+100HP)..SHOTGUN(5)..KNIFE(6)..EXPLOSIVES(7)..GRENADE(8)..AK47(9)..MINE(10,нефункц.)..BAT(12)..AMMO_PISTOL/SHOTGUN/AK47(13-15)..TREASURE(23)..CAR_KEYS(24-28)..WIRE_CUTTER(29)

**Выпадение:** только player-dropped оружие рандомизирует ammo (shotgun 2-3, ak47 6-13); cooldown ~1.25с

**Автоприцеливание:** `find_target_new()` по сфере; `calc_target_score_new()` учитывает distance, угол, visibility, вражность

---

## 10. Взаимодействие

**find_grab_face():** двухпроходный поиск (pass0: hi-res крыши, pass1: lo-res terrain); возвращает type 0/1(cable)/2(ladder)
**valid_grab_angle():** ВСЕГДА 1 (валидация отключена в пре-релизе)

**Cable параметры в полях DFacet:** `StyleIndex` = angle_step1, `Building` = angle_step2, `Height` = count
**find_cable_y_along():** косинусная кривая провисания + линейная интерполяция Y
**check_grab_ladder_facet():** возвращает -1 если игрок ВЫШЕ (ещё падает)

**Иерархия приоритетов взаимодействия:** выйти из машины > арест > слезть с лестницы > сесть в машину > крюк > переключатель > разговор > подбор > обыск > обнять стену > кола > присесть

**calc_sub_objects_position():** мировая позиция кости; субобъекты: LEFT/RIGHT_HAND, LEFT/RIGHT_FOOT, HEAD

---

## 11. Миссии (EWAY)

**EWAY** = реальная система скриптинга миссий. `.ucm` = EventPoint массивы (НЕ MuckyBasic скрипты).

**Polling:** каждый кадр; PC step=1 (все), PSX step=2 (чётные/нечётные). Стоп при GS_LEVEL_WON/LOST.
`EWAY_time += 80 × TICK_RATIO >> TICK_SHIFT` каждый кадр (≈1000 ед/сек при 50fps)

**Режимы активации (EWAY_Stay):**
- ALWAYS → DEAD immediately (fire-once, однократный)
- WHILE → re-poll каждый кадр, при FALSE → inactive
- WHILE_TIME → WHILE + N тиков after release
- TIME → фиксированный timer от активации
- DIE → как ALWAYS через set_inactive

**42 условия (EWAY_COND_\*):**
- 0=FALSE, 1=TRUE(immediately), 2=PROXIMITY, 3=TRIPWIRE, 4=PRESSURE(**STUB always FALSE**), 5=CAMERA(**STUB always FALSE**), 6=SWITCH, 7=TIME, 8=DEPENDENT, 9=BOOL_AND, 10=BOOL_OR, 11=COUNTDOWN, 12=COUNTDOWN_SEE, 13=PERSON_DEAD, 14=PERSON_NEAR, 15=CAMERA_AT, 16=PLAYER_CUBE, 17=A_SEE_B, 18=GROUP_DEAD, 19=HALF_DEAD, 20=PERSON_USED, 21=ITEM_HELD, 22=RADIUS_USED, 23=PRIM_DAMAGED, 24=CONVERSE_END, 25=COUNTER_GTEQ, 26=PERSON_ARRESTED, 27=PLAYER_CUBOID, 28=KILLED_NOT_ARRESTED, 29=CRIME_RATE_GTEQ, 30=CRIME_RATE_LTEQ, 31=IS_MURDERER, 32=PERSON_IN_VEHICLE, 33=THING_RADIUS_DIR, 34=SPECIFIC_ITEM_HELD, 35=RANDOM, 36=PLAYER_FIRED_GUN, 37=PRIM_ACTIVATED, 38=DARCI_GRABBED, 39=PUNCHED_AND_KICKED, 40=MOVE_RADIUS_DIR, 41=AFTER_FIRST_GAMETURN

**52 действия (WPT_* → EWAY_DO_\*):** CREATE_PLAYER/ENEMIES/VEHICLE/ITEM/CREATURE/CAMERA(2-7), MESSAGE(12), SOUND_EFFECT(13), VISUAL_EFFECT(14), CUT_SCENE(15), TELEPORT(16-17), END_GAME_LOSE(18), SHOUT(19), ACTIVATE_PRIM(20), ADJUST_ENEMY(22), CREATE_BOMB(24), END_GAME_WIN(26), NAV_BEACON(27), SPOT_EFFECT(28), CREATE_BARREL(29), KILL_WAYPOINT(30), CREATE_TREASURE(31), BONUS_POINTS(32→МЁРТВЫЙ КОД if(0)), GROUP_LIFE(33), GROUP_DEATH(34), CONVERSATION(35), INCREMENT(37), DYNAMIC_LIGHT(38), GOTHERE_DOTHIS(39,ASSERT(0)!), TRANSFER_PLAYER(40), AUTOSAVE(41), LOCK_VEHICLE(43), GROUP_RESET(44), COUNT_UP_TIMER(45), RESET_COUNTER(46), CREATE_MIST(47), ENEMY_FLAGS(48), STALL_CAR(49), EXTEND(50), MOVE_THING(51), MAKE_PERSON_PEE(52), CONE_PENALTIES(53), SIGN(54), WAREFX(55), NO_FLOOR(56), SHAKE_CAMERA(57)

**Непрерывные эффекты (каждый кадр пока активны):** EMIT_STEAM, WATER_SPOUT(4×DIRT_new_water), SHAKE_CAMERA(FC+3/+6 PSX, max 100), VISIBLE_COUNT_UP(инкр. таймера+PANEL_draw_timer)

**Ключевые факты:**
- Координаты вэйпойнта: mapsquare-единицы; `<< 8` при создании объекта
- Yaw: хранится `>> 3` в EWAY_Way.yaw; восстанавливается `<< 3`
- `EWAY_counter[7]` = счётчик убитых копов (cap 255)
- `EWAY_STAY_ALWAYS` → немедленно EWAY_FLAG_DEAD (fire-once semantics!)
- `GROUP_LIFE/DEATH` иммунны к себе — только другие WP той же colour+group
- `WPT_BONUS_POINTS` → if(0) (мёртвый код — очки не даются)
- **Roper stats bug:** EWAY_create_player(PLAYER_ROPER) использует Darci-статы (блок Roper закомментирован)

**EWAY_created_last_waypoint:** resolve ID→index + подсчёт objetive очков + WHY_LOST строки + CAMERA_TARGET_THING attach

---

## 12. Здания и интерьеры

**Иерархия:** FBuilding(≤500) → FStorey(≤2500) → FWall(≤15000) → [compile] → DBuilding(≤1024) → DFacet(≤16384)

**21 STOREY_TYPE_\*:** NORMAL(1), ROOF(2), WALL(3), ROOF_QUAD(4), FLOOR_POINTS(5), FIRE_ESCAPE(6), STAIRCASE(7), SKYLIGHT(8), **CABLE(9,тросы! есть в финале)**, FENCE(10), FENCE_BRICK(11), **LADDER(12)**, FENCE_FLAT(13), TRENCH(14), JUST_COLLISION(15), PARTITION(16), **INSIDE(17)**, DOOR(18), INSIDE_DOOR(19), OINSIDE(20), **OUTSIDE_DOOR(21)**

**DFacet:** все оси-aligned! `Y[2]` = нижний/верхний Y; `Open` (0-255) = анимация двери; поле `StyleIndex/Building/Height` переиспользуются для параметров кабелей!

**Интерьеры:** не отдельная геометрия — те же DFacets типов 17/19/20. Рендер переключает фильтрацию. STOREY_TYPE_OUTSIDE_DOOR(21) = триггер входа.

**Двери:** DOOR_MAX_DOORS=4 одновременно; step +4/кадр; синхронизированы с MAV через MAV_turn_movement_on/off()

**Лестницы:** автогенерация через STAIR_calculate() с seed; scoring: edges +0x5000, opposite_wall +0xbabe, near_outside_door = -INFINITY

**Здания неуязвимы** — нет damage mesh. `Shake` в DFacet = только для заборов.

**build_quick_city():** регистрирует DFacets и roof_faces4 в PAP_Lo. Положительные face индексы (prim_faces4) **закомментированы** — только отрицательные (roof_faces4) регистрируются.

**ROAD_wander_calc():** ROAD_MAX_NODES=256; ROAD_LANE=0x100 (левая сторона); `ROAD_is_road` = PAP_Hi.Texture & 0x3ff: PC 323-356, PSX 256-278
**Хардкод:** нод (121,33) на gpost3.iam — пропускается в ROAD_wander_calc

**WAND_init():** PAP_FLAG_WANDER ставится ячейкам-тротуарам (5×5 окрестность от дорог); очищается для OB с PRIM_COLLIDE_BOX/SMALLBOX/CYLINDER

---

## 13. Эффекты

**Частицы (psystem.cpp):** MAX_PARTICLES = 2048(PC) / 64(PSX); `PARTICLE_Add()` с gravity/drag; timing через GetTickCount() (delta-time!)

**Огонь:** FIRE_MAX_FLAMES=256, FIRE_MAX_FIRE=8; время жизни 32+rand(31) кадров; size -= shrink каждые 4 кадра

**Ribbon:** MAX_RIBBONS=64, MAX_RIBBON_SIZE=32; circular buffer trail; флаги FADE/SLIDE/CONVECT(+22Y/frame)/IALPHA; используют: PYRO_IMMOLATE/FLICKER, FLAMER, CHOPPER

**BANG (взрывы визуально):** 4 типа каскада (BIG→MIDDLE→NEARLY→END); 64 BANG + 4096 Phwoar частиц; **урон НЕ отсюда** → `create_shockwave()` в collide.cpp

**POW:** sprite-based взрывы, 8 типов, POW_Sprite[256], POW_Pow[32]

**PYRO (18 типов Thing-based):** IMMOLATE = -10HP/frame; GAMEOVER = 8 dlights, counter>242 → GS_LEVEL_LOST; MAX_PYROS=64

**DIRT:** 16 типов ambient debris; MAX_DIRT=1024; pigeon = `prob_pigeon=0` хардкод ("no bug ridden pigeons"); банки колы = TYPE_CAN через DIRT с prob_can=1/101; prim 253 = ejected shell (PC only)

**Shadow (статические):** запекаются при загрузке; sun_dir = (+147,-148,-147); 3 бита → PAP_FLAG_SHADOW_1/2/3

**Лужи (PC only):** `PUDDLE_precalculate()` при загрузке; `PUDDLE_in()` проверяет точку; `DRIP_create_if_in_puddle()` под `#ifndef TARGET_DC`

**Вода:** `WATER_gush()` возмущение от машин/людей; `MIST_gust()/FOG_gust()` от движения
**Cloth.cpp:** `CLOTH_process()` в `#if 0` — не выполняется. Не переносить.

**Spark:** SPARK_MAX_SPARKS=32; 4 точки (LIMB/CIRCULAR/GROUND/POINT); звук S_ELEC_START+(spark_id%5)

**PYRO_HITSPANG:** max `global_spang_count=5`

---

## 14. Камера

**Только FC (Final Camera):** `cam.cpp` = мёртвый код (`#ifdef DOG_POO`). FC_MAX_CAMS=2 (splitscreen).

**4 режима камеры:** 0=стандарт(dist≈480,h=0x16000), 1=дальше(640,0x20000), 2=ещё дальше(896,0x25000), 3=низкий(768,0x8000)
**PSX type0:** dist=0x300(768), h=0x18000; `CAM_MORE_IN=0.75F` → PC камера на 25% ближе

**FC_process — 15 шагов:**
1-2. alter_for_pos + calc_focus; 3. Warehouse transition; 4. lookabove (умирающий: -0x80/кадр); 5. Rotate (decay ±0x80/кадр); 6. Toonear cancel; **7. Collision detection (8 шагов от want к focus: MAV_inside + стены + заборы)**; 8. Get-behind (скорость: entering_vehicle>>3, driving>>5, стена>>6+>>5, normal>>3); 9. Adjust Y (dy>>3, cap [-0xc00,+0xd00]; WMOVE → dy<<5 мгновенно); 10. Distance clamp; 11. Position smoothing (если |delta|>0x800); 12. FC_look_at_focus; 13. Angle smoothing (если |total|>0x180 → delta>>2); **14. Lens = 0x28000×CAM_MORE_IN (ВСЕГДА, FOV не меняется!)**; 15. Shake (random×128, decay: -1-shake>>3)

**Toonear:** `toonear_dist=0x90000` → first-person mode

**FC_alter_for_pos:** idle height check (`&& 0`) = МЁРТВЫЙ КОД → реально всегда dheight=0,ddist=256

**focus_yaw специальные случаи:** ACTION_SIDE_STEP/SIT_BENCH/HUG_WALL → +1024; SUB_STATE_ENTERING_VEHICLE: TAXI → -712, иначе +712; STATE_DANGLING: ±550; GF_SIDE_ON_COMBAT → -512; машина задним ходом → +1024

---

## 15. Рендеринг

**DDEngine — DirectX6 fixed-function pipeline. Переписываем с нуля.**

**Типы (DT_\*):** NONE(0), BUILDING(1), PRIM(2), MULTI_PRIM(3), ROT_MULTI(4), EFFECT(5), MESH(6), TWEEN(7,персонажи), SPRITE(8), VEHICLE(9), ANIM_PRIM(10), CHOPPER(11), PYRO(12), ANIMAL_PRIM(13), TRACK(14), BIKE(15)

**Пайплайн рендеринга:**
1. `POLY_frame_init()` — очистка буферов
2. `MESH_draw_poly()` + `SPRITE_draw()` + `SKY_draw_poly_*()` — накопление полигонов
3. `POLY_frame_draw()` — Z-сортировка + рендер:
   - Непрозрачные: в порядке индекса, POLY_PAGE_COLOUR первым
   - Прозрачные: **bucket sort** `buckets[2048]`, `sort_z=ZCLIP/vz`, bucket=[0]=дальние→[2047]=близкие; render FAR→NEAR = back-to-front
   - POLY_PAGE_SHADOW: `if !draw_shadow_page` — пропускается
   - POLY_PAGE_PUDDLE: отдельный `POLY_frame_draw_puddles()`

**POLY_NUM_PAGES=1508** (0-1407 обычные, 1408+ служебные); `MAX_VERTICES=32000`

**Трансформация вершины:**
```c
vx = world_x - cam_x;  // translate к камере
MATRIX_MUL(cam_matrix, vx,vy,vz);  // view space
Z = ZCLIP_PLANE / vz;  // reciprocal depth
X = mid_x - mul_x * vx * Z;  Y = mid_y - mul_y * vy * Z;
```

**Vertex lighting (не per-pixel):** `NIGHT_Colour {r,g,b}` 0-63 (6-bit!) → ×4 → 0-255 для D3D
**Ambient direction:** (110,-148,-177) — **захардкожена** везде
**Crinkle (bump mapping):** отключён в пре-релизе (`CRINKLE_load→NULL`), но **работает в финальном PC**

**NIGHT система:**
- Статические источники: max 256, из `.lgt` файлов; LIGHT_range max=0x600; BROKEN/PULSE/FADE типы
- Динамические: max 64 (`NIGHT_Dlight`)
- Статические тени: запекаются в PAP_FLAG_SHADOW_1/2/3; LOS ray-cast к (147,-148,-147)
- **Динамических теней НЕТ** — flat sprite под ногами
- `NIGHT_Colour` диапазон 0-63 (НЕ 0-255!)
- **БАГ night.cpp:746:** `dprod = dz*nx` вместо `dz*nz`
- `NIGHT_generate_walkable_lighting()` = **МЁРТВЫЙ КОД** (`return;` в начале)
- `MapElement.Colour` = МЁРТВЫЙ КОД в DDEngine (PC)

**Fog:** outdoor = `NIGHT_sky_colour`; GF_SEWERS/GF_INDOORS = чёрный (0)

**USE_TOMS_ENGINE_PLEASE_BOB=1** — единственный активный путь рендеринга
**InterruptFrame = МЁРТВЫЙ КОД** (всегда NULL)

**DrawMesh.Angle = 0-2047** (полные 360°)

**⚠️ Game logic в рендерере:**
- `DRAWXTRA_MIB_destruct()` мутирует `ammo_packs_pistol`, создаёт PYRO_TWANGER и SPARK
- `PYRO_draw_armageddon()` создаёт PYRO_NEWDOME, PARTICLE_Add(), MFX_play_xyz()

---

## 16. Звук

**Miles Sound System (MSS32)** — проприетарная. При переносе: заменить на miniaudio или SDL_mixer.

**MFX API:** `MFX_play(ch,wave,flags)`, `MFX_play_xyz(ch,wave,flags,x>>8,y>>8,z>>8)`, `MFX_stop(ch)`, `MFX_set_volume(ch,vol)`, `MFX_update_xyz(ch,x>>8,y>>8,z>>8)`
3D координаты со сдвигом `>>8` (world units → mapsquares)

**14 MUSIC_MODE_\*:** NONE, CRAWL, DRIVING1/2, DRIVING_START, FIGHT1/2, SPRINT1/2, AMBIENT, CINEMATIC, VICTORY, FAIL, MENU

**Приоритеты музыки:** CINEMATIC > FIGHT1/2 > DRIVING > SPRINT > CRAWL > AMBIENT

**5 биомов ambient:** Jungle(texture_set=1), Snow(=5), Estate(=10), BusyCity(default), QuietCity(=16)

**Indoor/Outdoor:** `GF_INDOORS` флаг → fade ambient; типы indoor: office/police/posheeta/club

**Погода:** дождь громче у земли (`gain = 255 - above_ground>>4`); ветер громче на высоте (`gain = abs_height>>4`)

**Шаги:** `FLAGS_PLAYED_FOOTSTEP` (bit 12); тип звука по `PAP_Hi.Texture`

---

## 17. UI и Frontend

**Состояния игры (GS_\*):** ATTRACT_MODE(1<<0), PLAY_GAME(1<<1), CONFIGURE_NET(1<<2,не нужен), LEVEL_WON(1<<3), LEVEL_LOST(1<<4), GAME_WON(1<<5), RECORD(1<<6), PLAYBACK(1<<7), REPLAY(1<<8), EDITOR(1<<16)

**Внешний loop:** ATTRACT_MODE → game_attract_mode(); PLAY_GAME → game_loop(); inner loop продолжается пока `GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON`

**GAMEMENU ожидание:** `GAMEMENU_wait` += 64/кадр; порог 2560 ≈ 40 кадров ≈ 1.3с → показ меню

**WON-меню:** нет выбираемых пунктов → любое нажатие = DO_NEXT_LEVEL
**LOST-меню:** RESTART → DO_RESTART; ABANDON → SURE → DO_CHOOSE_NEW_MISSION

**GS_REPLAY:** `goto round_again` → полный рестарт с нуля (чекпоинтов нет)

**DarciDeadCivWarnings:** `Player->RedMarks > 1` → предупреждения; счётчик 3 → `GS_LEVEL_LOST`! **Персистирует между миссиями!**

**CRIME_RATE delta:** kill guilty=-2, kill wander_civ=+5, arrest guilty=-4; если CRIME_RATE=0 при загрузке → дефолт 50

**mission_hierarchy биты:** bit0=exists, bit1=complete, bit2=available; [1]=3 (корень) при старте

**complete_point пороги:** <8→theme0, <16→theme1, <24→theme2, ≥24→theme3; max чит=40

**ScoresDraw():** killed/arrested/at-large/bonus found/missed/time; Mucky Times = DC времена (PC в #if 0); код-хэш для спидрана: `(i+1)*(m+1)*(s+1)*3141 % 12345 + 0x9a2f`

**STARTSCR_mission flow:** FE_MAPSCREEN+ENTER → mode=100+N → FE_START → STARTSCR_mission set → STARTS_START → GS_PLAY_GAME → elev.cpp → *STARTSCR_mission=0

**Особые миссии:**
- park2.ucm → RunCutscene(1) (MIB intro) при победе
- Finale1.ucm → RunCutscene(3) + OS_hack() (финал)
- `this_level_has_bane` = ТОЛЬКО index 27 в whattoload[] = Finale1.ucm
- VIOLENCE=FALSE для туториалов: FTutor1(1), Album1(31), Gangorder1(32)
- `DONT_load = 0` жёстко → per-mission dontload bitmasks игнорируются
- `ANNOYING_HACK_FOR_SIMON`: policeID требует fight1+assault1+testdrive1a

---

## 18. Прогресс и сохранения

**3 слота, 1-based!** `save_slot = menu_state.selected + 1`

**.wag бинарный layout (version 3):**
```
mission_name: strlen(txt) байт + CRLF(0x0D 0x0A)  ← FRONTEND_SaveString
complete_point: UBYTE
version: UBYTE = 3  ← "Historical Reasons" — странное место
[v≥1] DarciStrength/Constitution/Skill/Stamina: UBYTE×4 + Roper×4 + DarciDeadCivWarnings: UBYTE = 9 байт
[v≥2] mission_hierarchy[60]: UBYTE[60] = 60 байт
[v≥3] best_found[50][4]: UBYTE[200] = 200 байт
```
Итого v3: len(name)+2+271 байт

**best_found [mission][stat]:** [0]=Constitution, [1]=Strength, [2]=Stamina, [3]=Skill
**Анти-фарм:** при повтоной миссии даётся только `found - best_found` (разница с рекордом)
`FRONTEND_LoadString`: читает до 0x0A (LF), переменная длина — **не фиксированные 32 байта!**

---

## 19. Загрузка уровня (9 шагов)

```
1. ELEV_load_name(.ucm) → 4 имени файлов
2. ELEV_game_init() → FC/BIKE/BARREL/BALLOON/NIGHT/OB/TRIP/FOG/MIST/PUDDLE/DRIP/GLITTER/POW/SPARK init
3. load_game_map(.iam) → PAP_Hi + animated prims + OB objects + texture_set
4. Постобработка: calc_prim_info + build_quick_city + init_animals + DIRT_init + ROAD_sink
                + ROAD_wander_calc + WAND_init + WARE_init + MAV_precalculate (тяжёлая!)
                + BUILD_car_facets + SHADOW_do + COLLIDE_calc_fastnav_bits + ...
5. Освещение: NIGHT_load_ed_file(.lgt) или NIGHT_ambient(дефолт); ночь→GF_RAINING
6. ELEV_load_level(.ucm часть 2) → сброс GAME_TURN=0 + metadata + фейковые горожане + EventPoints
7. Финализация: calc_prim_normals + TEXTURE_load_needed + EWAY_process() (← создаёт игрока/NPC!) + calc_slide_edges
8. Настройка камеры: FC_look_at() + FC_setup_initial_camera()
9. Спецэффекты: if GF_RAINING → PUDDLE_precalculate() + insert_collision_facets
```

**Игрок создаётся через EWAY_process()** → WPT_CREATE_PLAYER → `EWAY_create_player()` → тот же `PCOM_create_person()` что и NPC

---

## 20. Межсистемные зависимости (неочевидные)

### Рендер мутирует game state:
- `DRAWXTRA_MIB_destruct()` → `ammo_packs_pistol--, PYRO_TWANGER, SPARK` — из рендерера!
- `PYRO_draw_armageddon()` → `PYRO_NEWDOME, PARTICLE_Add(), MFX_play_xyz()` — тоже из рендерера!

### Камера зависит от навигации:
- `FC_process` collision detection → `MAV_inside()` для обнаружения стен
- `WARE_get_caps()` / `WARE_inside()` для складских интерьеров

### Двери синхронизированы с MAV:
- `DOOR_process()` → при открытии: `MAV_turn_movement_on()` (теряет все caps кроме GOTO!)
- При закрытии: `MAV_turn_movement_off()` (очищает только GOTO бит)

### Освещение инициализируется в рендерере, влияет на AI:
- `NIGHT_get_light_at(pos)` в `can_a_see_b()` — свет влияет на дальность видения NPC

### EWAY контролирует весь spawn:
- Игрок и NPC создаются через EWAY, не через отдельную систему
- `STARTSCR_notify_gameover()` закомментирован → auto_advance никогда не ставится

### Физика прыжков завязана на анимацию:
- Гравитация `animation-driven` для персонажей — StateFn управляет DY
- `plant_feet()` активно ТОЛЬКО в `fn_person_dangling` (SUB_STATE_DROP_DOWN_LAND)
- `InterruptFrame` = МЁРТВЫЙ КОД (всегда NULL) — прерывание анимаций не работает

### Боевой урон из анимации, не из физики:
- `GameFightCol` = данные удара встроены в keyframe анимации
- Knockback = выбор анимации (`take_hit[]`), НЕ физические векторы

### EWAY_STAY_ALWAYS = мгновенно DEAD (не INACTIVE):
- Это означает что такой вэйпойнт **никогда не будет повторно активирован**
- WHILE vs ALWAYS = ключевое различие при дизайне триггеров

### SPECIAL_MINE нефункциональна:
- pickup = ASSERT(0); throw = ASSERT(0); оба пути мертвы
- DIRT_dirt индекс хранится в `Special.waypoint` при активации — поле переиспользовано

### MapWho участвует в AI и рендеринге:
- AI: THING_find_sphere через PAP_Lo.MapWho
- Рендер BANG: MapWho linked list для culling
- Sync критична: `move_thing_on_map()` при смене PAP_Lo ячейки

---

## 21. Пре-релизные баги и особенности

| Факт | Описание |
|------|----------|
| GRAVITY = -51 | **НЕ -5120!** (правильное значение) |
| Resurrect bug | Гражданские воскресают на месте смерти (newpos не присваивается) |
| Roper stats bug | EWAY_create_player(ROPER) использует Darci-статы |
| mount_ladder | Закомментирован для игрока снизу; AI может |
| fn_cop_normal | `#if 0` в пре-релизе |
| fn_thug_init | `ASSERT(0)` |
| fn_roper_normal | Пустая функция |
| INSIDE2_mav_inside | `ASSERT(0)` — NPC застревают в INSIDE2 |
| INSIDE2_mav_nav_calc | Баг цикла Z (`MinX/MaxX` вместо `MinZ/MaxZ`) |
| COP_DRIVER arrest-from-car | Закомментирован |
| STARTSCR_notify_gameover | Закомментирован в Game.cpp |
| valid_grab_angle | ВСЕГДА 1 (валидация отключена) |
| FC_alter_for_pos idle | `&& 0` = мёртвый код (высота не меняется в idle) |
| cam.cpp | `#ifdef DOG_POO` = весь файл мёртвый код |
| briefing.cpp | МЁРТВЫЙ КОД (Dec 1998 прототип) |
| NIGHT_generate_walkable_lighting | `return;` в начале = мёртвый код |
| Crinkle | Отключён в пре-релизе; работает в финальном PC |
| SM_process() | Закомментирован и в Controls.cpp |
| Balloon | `load_prim_object(PRIM_OBJ_BALLOON)` закомментирован |
| SPECIAL_MINE | pickup=ASSERT(0), throw=ASSERT(0) |
| is_semtex | `(index==20)` = skymiss2.ucm (вероятно ошибка) |
| night.cpp:746 | `dprod = dz*nx` вместо `dz*nz` (баг освещения) |
| Canid dispatch | Закомментирован — собаки инертны |
| OB_height_fiddle_de_dee | `/* */` мёртвый код |
| InterruptFrame | ВСЕГДА 0 = мёртвый код |

**Что есть в финале, нет в пре-релизе:** Zipwire (grab закомментирован), Crinkle bump mapping, Ladder mount снизу, fn_cop_normal, fn_thug_init

**Что нет ни в пре-релизе ни в финале:** Bike, Mine (рабочая), Grapple-weapon, Sniper rifle, SM (soft body), Balloon, канализация gameplay, multiplayer, GS_REPLAY (отправка на финал)

---

## 22. Форматы файлов

| Расширение | Назначение | Ключевые факты |
|-----------|-----------|----------------|
| `.ucm` | Миссия: метаданные + EventPoints (NOT MuckyBasic!) | header 1316б + 512×74б EP + version tail |
| `.iam` | Карта: PAP + объекты + anim prims | читается в load_game_map() |
| `.lgt` | Освещение: static/dynamic lights | NIGHT_load_ed_file() |
| `.wag` | Сохранение | v0-3, variable-string+CRLF |
| `.txc` | FileClump архив | MaxID+Offsets+Lengths+data; size_t платформозависим! |
| `.all` | Анимации | pointer fixup при загрузке (NextFrame/PrevFrame — runtime адреса) |
| `.tma` | Текстурные стили | style.tma: 200×5 слотов; instyle.tma: count_x*count_y UBYTE |
| `.prm` | Меши примов | загружаются по требованию |
| `.txt` | Диалоги горожан | EWAY_load_fake_wander_text() |

**sizeof(ED_Light) = 20 байт** (8 однобайт. + 3 SLONG)
**sizeof(PrimFace3) = 28 байт**; **sizeof(PrimFace4) = 34 байта**
**EventPoint binary = 74 байта** (14 заголовок + 60 данных)
**EventPoints MAX = 512** → 37888 байт на миссию

**PRIM_START_SAVE_TYPE=5793:** если save_type==base+1 → PrimPoint как SWORD(6б), иначе SLONG(12б)
**PrimFace4 FACE_FLAG_WALKABLE=(1<<6)** — только эти квады образуют ходимые поверхности
**ROAD_is_road:** PC = texture & 0x3ff: 323-356; TextureSet 7/8: также 35/36/39; PSX: 256-278

---

## 23. Быстрые числа (весь справочник)

```
Игровой мир:
  карта: 128×128 mapsquares = 32768×32768 world units
  GRAVITY = -51 (НЕ -5120!)
  fall damage: (-DY-20000)/100; death: player=-12000, npc=-6000
  water_level = -128; PAP_Lo.water = -16 при загрузке
  WMOVE max = 192; MAX_THINGS=700; RMAX_PEOPLE=180; MAX_VEHICLES=40

Физика:
  NOGO push = 0x5800; wall_height extra = -0x50 (-80 ед.)
  HyperMatter: gy=0 (отскок от Y=0, не terrain)
  set_person_drop_down: DY=-(4<<8)=-1024, Velocity=-8 if !KEEP_VEL

AI:
  22 типа AI, 28 состояний; SEARCHING/SLEEPING=stub; LOOKAROUND=только счётчик
  PCOM_TICKS_PER_SEC=320 (16×20); ARRIVE_DIST=0x40=64; MAX_FIND=16
  MIB mass aggro radius=1280; CIV resurrect >200 кадров
  cop scan radius=0x800=2048 каждые 4 кадра

Оружие и урон:
  Pistol: 70hp/140t; Shotgun: 300-dist/400t; AK47: 100pl/40npc/64t
  Grenade radius=0x300=768, max=500; C4: timer=5сек(1600тиков), radius=0x500=1280, max=500
  Hit chance min: 20/256≈8%; base: 230-|Roll|>>1-Velocity

Камера и рендеринг:
  NIGHT_Colour: 0-63 (НЕ 0-255!); ambient dir: (110,-148,-177)
  bucket sort: 2048 buckets; MAX_VERTICES=32000
  camera toonear: dist=0x90000 → first-person
  lens = 0x28000×0.75 (PC), FOV НЕ МЕНЯЕТСЯ

Транспорт:
  VEH_SPEED_LIMIT=750; VEH_REVERSE=300; WHEELTIME=35кадров
  SKID_START=3кадра; SKID_FORCE=8500
  runover: |VelX*tx+VelZ*tz|/(dist*200), min 10HP; smoke: Health<128

Сохранения:
  .wag: 3 слота, 1-based; v3 layout = name+CRLF+2+271 байт
  best_found: анти-фарм; complete_point: пороги 8/16/24/40

Миссии:
  42 условия, 52 EWAY_DO; polling каждый кадр
  EWAY_counter[7]=убитые копы(cap 255)
  bench cooldown: (GAME_TURN&0x3ff)==314 каждые ~34с
  GAMEMENU wait: 64/кадр, порог 2560≈1.3с
  DarciDeadCivWarnings≥3 → LEVEL_WON→LEVEL_LOST!
```
