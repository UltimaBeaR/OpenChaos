# DENSE_SUMMARY — Urban Chaos Knowledge Base (Compact)

Компактная версия всей KB. Только числа, константы, формулы, зависимости, баги.
Полные описания → `original_game_knowledge_base/subsystems/` и `resource_formats/`.

---

## 1. Игровой цикл и время

- **GAME_STATE** — битовые флаги: `GS_ATTRACT_MODE`=**1<<0**, `GS_PLAY_GAME`=**1<<1**, `GS_CONFIGURE_NET`=**1<<2**, `GS_LEVEL_WON`=**1<<3**, `GS_LEVEL_LOST`=**1<<4**, `GS_GAME_WON`=**1<<5**, `GS_RECORD`=**1<<6**, `GS_PLAYBACK`=**1<<7**, `GS_REPLAY`=**1<<8**, `GS_EDITOR`=**1<<16**
- Inner loop: `SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON))`
- **Per-frame порядок (PC):**
  1. `GAMEMENU_process()` → DO_NOTHING/DO_RESTART/DO_CHOOSE_NEW_MISSION/DO_NEXT_LEVEL
  2. GS_LEVEL_WON check (Finale1.ucm → break)
  3. EWAY_tutorial_string (порог **64×40=2560**; should_i_process_game=FALSE)
  4. `demo_timeout(0)` — мёртвый код (TIMEOUT_DEMO=0)
  5. `special_keys()`, `process_controls()` (skip при LOST/WON/tutorial)
  6. `check_pows()`
  7. if `should_i_process_game()`: process_things → PARTICLE_Run → OB_process → TRIP_process → DOOR_process → EWAY_process → SPARK → RIBBON → DIRT → Grenades → WMOVE_draw → BALLOON → MAP → POW → FC_process
  8. PUDDLE/DRIP — **всегда** (даже при паузе)
  9. draw_screen → OVERLAY_handle → CONSOLE_draw → GAMEMENU_draw → PANEL_fadeout_draw → screen_flip
  10. `lock_frame_rate(30)` — spin-loop, дефолт **30 FPS**
  11. `handle_sfx()`, `GAME_TURN++`
  12. Bench cooldown: `(GAME_TURN & 0x3ff)==314` → снять GF_DISABLE_BENCH_HEALTH (каждые **1024** кадра ≈ **~34с**)
- **TICK_RATIO**: масштаб скорости, **256**=норма, clamp **128-512**; **SMOOTH_TICK_RATIO**: кольцевой буфер **4** значения
- Победа: WPT_END_GAME_WIN → GS_LEVEL_WON; Поражение: HP≤0 или WPT_END_GAME_LOSE → GS_LEVEL_LOST
- GAMEMENU_wait: порог **2560** (~1.3с); DC доп. **5120** (~2.5с)
- **GS_REPLAY**: goto round_again → полный рестарт, **чекпоинтов нет**
- DarciDeadCivWarnings: RedMarks>1 → 0/1/2=предупреждения, **3=GS_LEVEL_LOST**; персистирует между миссиями

---

## 2. Система координат и карта

- **PAP_Hi**: **128×128**, **6 байт**/ячейка. 1 ячейка = **256** ед. (`PAP_SHIFT_HI=8`)
- **PAP_Lo**: **32×32**, **8 байт**/ячейка. 1 ячейка = **1024** ед. (`PAP_SHIFT_LO=10`)
- Полная карта: **32768×32768** ед. Оси: X=запад-восток, **Y=вниз-вверх**, Z=север-юг
- PAP_Hi: Texture(UWORD), Flags(UWORD, 15 бит), Alt(SBYTE, реальная=`Alt<<3`, диапазон **0-1016**)
- PAP_Lo: MapWho(UWORD), Walkable(SWORD, <0=roof), ColVectHead(UWORD), water(SBYTE), Flag(UBYTE)
- ⚠️ Биты 9 и 14 PAP_Hi **контекстно-зависимые**: бит 9 = ANIM_TMAP или ROOF_EXISTS; бит 14 = WANDER или FLAT_ROOF
- Преобразования: `мир→Hi: >>8`; `мир→Lo: >>18`
- Интерполяция высоты: диагональная (два треугольника), результат `<<3`
- MapWho: linked list в PAP_Lo, `move_thing_on_map()` при смене ячейки (каждые **1024** ед.)
- **ROAD**: max **256** нодов, **4** связи, `ROAD_LANE=0x100`; дорожные текстуры PC: **323-356**
- **WAND**: 5×5 окрестность дороги → PAP_FLAG_WANDER; `WAND_get_next_place()`: 4 направления × **WAND_MAX_MOVE=3**
- Освещение: `LIGHT_Colour` RGB **3 байта** (0-63/канал), макс **256** статических + **64** динамических

---

## 3. Thing System

### Структура Thing — 125 байт
- **15 классов** (0-18 с пропусками): PLAYER(1), CAMERA(2), PROJECTILE(3), BUILDING(4), PERSON(5), ANIMAL(6), FURNITURE(7), SWITCH(8), VEHICLE(9), SPECIAL(10), ANIM_PRIM(11), CHOPPER(12), PYRO(13), TRACK(14), PLAT(15), BARREL(16), BIKE(17), BAT(18)
- Полиморфизм: `union Genus` + `void (*StateFn)(Thing*)`
- Координаты: `GameCoord WorldPos` — **SLONG** (32-bit), НЕ float
- Скорость: `SWORD Velocity/DeltaVelocity/RequiredVelocity/DY` — **16-bit**
- Yaw: **UBYTE 0-255** = 360°; 0=восток, 64=юг, 128=запад, 192=север
- ⚠️ Физика использует углы **0-2047** — две разные системы!
- `OnFace`: SWORD, <0 = roof face, 0 = ground

### Лимиты
- **PRIMARY=400, SECONDARY=300**(PC)/**50**(PSX), **MAX_THINGS=700**(PC)/**450**(PSX)
- Free-list: O(1) alloc/free; `thing_class_head[CLASS]` → per-class list

### Флаги (32-bit)
- ON_MAPWHO(0), IN_VIEW(1), PROJECTILE_MOVEMENT(3), LOCKED_ANIM(4), IN_BUILDING(5), LOCKED(7), CABLE_BUILDING(8), COLLIDED(9), IN_SEWERS(10), SWITCHED_ON(11), PLAYED_FOOTSTEP(12), HAS_GUN(13), BURNING(14), HAS_ATTACHED_SOUND(15), BEEN_SHOT(16)

### State Machine — 37 состояний (0-37, пропуск 15)
- Dispatch: `people_functions[PersonType].StateFunctions[state]`; вызывается каждый кадр

### MapWho — 128×128, linked list через Child/Parent
- `THING_find_sphere/box/nearest(cx,cy,cz,radius,array,size,class_mask)`

### Barrel — 2-sphere rigid body
- `BARREL_SPHERE_DIST=50`, **MAX=300**, гравитация `dy-=0x80`/кадр, затухание `d*-=d*/32`
- Покой: sum<0x200 → still++; оба>**64** → stationary
- 4 типа: NORMAL (взрыв radius=**0x200** damage=**250**), CONE, BURNING, BIN

### Персонажи — 15 типов
- DARCI(0) HP=**200**, ROPER(1) HP=**400**, COP(2) HP=**200**, CIV(3) HP=**130**, Thugs HP=**200**, MIB1/2/3 HP=**700**
- **4 AnimType**: DARCI(0), ROPER(1), ROPER2(2), CIV(3)
- ⚠️ Все NPC кроме Darci/Roper маппятся на `cop_states`; `fn_roper_normal()` **пустая**
- Vertex morphing (НЕ скелетная): tween **0-256**, **15** body parts
- ⚠️ `InterruptFrame` — МЁРТВЫЙ КОД (всегда 0)

---

## 4. Физика и коллизии

### Координаты
- **SLONG** 32-bit integer, НЕ float — детерминизм; `>>8`=блок→юнит, `<<8`=юнит→блок
- `PAP_ALT_SHIFT=3`, `ELE_SIZE=256`; углы: **0-2047** (физика), **0-255** (Thing yaw)

### Heightmap
- PAP_Hi **128×128×6 байт** (~96KB); PAP_Lo **32×32×8 байт**
- Вода: уровень хардкод **-128**, хранится как **-16** (>>3); `PAP_LO_NO_WATER=-127`
- ⚠️ Вода = **невидимые коллизионные стены**, нет плавания/замедления

### Коллизии
- `CollisionVect[10000]` (~300KB PC), `CollisionVectLink[10000]` (~40KB), `WalkLink[30000]` (~120KB)
- `find_face_for_this_pos()`: порог |dy|<**0xa0**(160); fallback GRAB_FLOOR если |dy|<**0x50**(80)
- `plant_feet()`: ⚠️ X/Z смещение ноги **игнорируется** (fx=0, fz=0)

### move_thing() — только CLASS_PERSON
- ASSERT: |dx/dz| < **2** mapsquares
- Порядок: граница карты → collide_against_things (radius+**0x1a0**) → collide_against_objects (±**0x180**) → slide_along (DFacets) → slide_along_edges → find_face → side effects (DIRT_gust, MIST_gust, BARREL_hit)
- ⚠️ Скамейки (prim IDs **89,95,101,102,105,110,126**) + angle<**220** + backwards → set_person_sit_down

### slide_along()
- `NOGO_COLLIDE_WIDTH=0x5800`; ⚠️ ВСЕ DFacets **оси-aligned** (нет диагональных стен)
- Нет отскока (restitution=0), нет трения; CABLE/OPEN двери — игнорируются

### Гравитация
- Персонажи: animation-driven, `DY` ~**-20/кадр** в FALLING; порог **60** ед. (plant_feet)
- Транспорт: `GRAVITY=-51` юн/тик²; `VelY += GRAVITY`
- Барели: `dy -= 0x80` (128)/кадр

### Тайминг
- `NORMAL_TICK_TOCK=1000/15` = **66.67мс** (15 FPS логики)
- `TICK_SHIFT=8`; Vehicle: `TICK_LOOP=4`, `TICKS_PER_SECOND=80`
- Frame rate: spin-loop busy-wait, дефолт **30 FPS**

### HyperMatter (hm.cpp) — spring-lattice физика мебели
- **13 пружин/точку**, Euler integration
- ⚠️ `gy=0` хардкод — отскок от Y=0, НЕ от terrain
- ⚠️ Баг: `already_bumped[i]` вместо `[j]` — пропуск HM-HM столкновений

### WMOVE — движущиеся поверхности
- `WMOVE_MAX_FACES=192`; faces: PERSON/PLAT=**1**, VAN=**1**, JEEP/MEATWAGON=**4**, CAR/TAXI/POLICE/SEDAN=**5**

---

## 5. AI система PCOM

### Типы и перечисления
- **22 типа AI** (PCOM_AI_*): NONE(0), CIV(1), GUARD(2), ASSASIN(3), BOSS(4), COP(5), GANG(6), DOORMAN(7), BODYGUARD(8), DRIVER(9), BDISPOSER(10), BIKER(11—вырезан), FIGHT_TEST(12), BULLY(13), COP_DRIVER(14), SUICIDE(15), FLEE_PLAYER(16), KILL_COLOUR(17), MIB(18), BANE(19), HYPOCHONDRIA(20), SHOOT_DEAD(21)
- **28 состояний**, **10 типов движения**, **8 черт**, **6 режимов**, **6 скоростей**
- **RMAX_PEOPLE=180**, **PCOM_TICKS_PER_SEC=320**, **PCOM_ARRIVE_DIST=0x40**, **PCOM_MAX_GANGS=16**

### Восприятие — видимость
- NPC range: **2048** ед.; игрок: **65536**
- `view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256`; приседание `>>1`; движение `+256`
- FOV: близко(<**192** ед.)=**700**(~123°); далеко=**420**(~74°); "краем глаза" **250**(~44°): дист/2
- Eye height: стоя **96**, приседая **32**

### Восприятие — звуки (радиусы)
- FOOTSTEP **640**, VAN **384**, GUNSHOT **2560**, ALARM **2048**, FIGHT **2304**, BANG **1792**

### MIB AI (HP=**700**, INVULNERABLE)
- Проверяет видимость **каждый кадр**; aggro сфера **0x500=1280** → массовый переход в NAVTOKILL

### Bane AI — статичен, SPARK дуги каждые **160** тиков, электрошок **-25HP** если Darci стоит **~2с**

### Воскрешение CIV: !IN_VIEW + мёртв **200+** кадров → воскрешение
- ⚠️ Баг: newpos НЕ присваивается WorldPos → воскресают на месте смерти

### Собаки: ⚠️ **полностью инертны** в пре-релизе (dispatch закомментирован)

---

## 6. Навигация (MAV + Wallhug)

- **MAV** — основная, greedy best-first на **128×128** (⚠️ НЕ A\* — нет g-cost)
- **Wallhug** — вспомогательная, left/right-hand rule

### MAV данные
- `MAV_nav[UWORD]`: **10 бит** индекс MAV_opt + **4 бита** машины + **2 бита** spare
- `MAV_Opt` = 4 байта: caps для 4 направлений; **8 действий** (GOTO..LADDER_UP), **8 caps** битов
- **MAV_CAPS_DARCI=0xff** — умеет всё
- Память: opt **4KB** + nav **32KB** + flag **8KB** + dir **4KB**

### MAV_do поиск
- Горизонт **32** ячейки, priority queue **256**, overflow до **8** попыток
- Случайное смещение +0-3 для естественности

### Динамическое обновление
- ⚠️ **Только двери**: `MAV_turn_movement_on` **заменяет** все caps на GOTO (остальное теряется!)
- ⚠️ **Взрывы НЕ обновляют MAV**

### Навигация складов (WARE_mav_*) — swap MAV_nav → стандартный MAV_do → restore
### INSIDE2 — ⚠️ БАГИ: `mav_nav_calc` в **#if 0**, `mav_inside`=**ASSERT(0)**, `mav_stair/exit`=баг `>16` вместо `>>16`

### Wallhug
- **128×128**, **252** макс waypoints, **20000** макс шагов
- 2 hugger'а (left/right-hand rule), первый done = победитель
- Постобработка: LOS cleanup (lookahead **4**), рекурсивная оптимизация (max **10** итераций)

---

→ Продолжение: [DENSE_SUMMARY_2.md](DENSE_SUMMARY_2.md)
