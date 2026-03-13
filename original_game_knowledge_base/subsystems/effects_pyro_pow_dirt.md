# Эффекты: POW, PYRO, DIRT — Urban Chaos

**См. также:** [effects.md](effects.md) — основные эффекты (частицы, огонь, искры, ткань, капли, туман, лужи, тени)

---

## 1. Взрывы — POW система (pow.cpp)

**Ключевые файлы:** `fallen/Source/pow.cpp`, `fallen/Headers/pow.h`

**Суть:** Sprite-based визуальные взрывы. Это ТОЛЬКО визуальный эффект (рендер спрайтов-вспышек), НЕ shockwave/урон (тот в `PYRO_blast_radius`).

**Пулы:**
- `POW_Sprite[256]` (PC) / 192 (PSX) / 128 (DC) — отдельные спрайты
- `POW_Pow[32]` (PC) / 24 (PSX) / 16 (DC) — контейнеры взрывов
- `POW_mapwho[PAP_SIZE_LO]` — 1D mapwho (32 байта)

**8 типов взрывов (POW_Type):**

| # | Тип | Arrangement | Speed | Density | Spawns |
|---|-----|-------------|-------|---------|--------|
| 0 | UNUSED | NOTHING | - | - | - |
| 1 | BASIC_SPHERE_LARGE | SPHERE | 1 | 3 (high) | нет |
| 2 | BASIC_SPHERE_MEDIUM | SPHERE | 2 | 1 | нет |
| 3 | BASIC_SPHERE_SMALL | SPHERE | 3 | 0 (low) | нет |
| 4 | SPAWN_SPHERE_LARGE | SPHERE | 1 | 3 | 3× MEDIUM (AWAY+FAR) |
| 5 | SPAWN_SPHERE_MEDIUM | SPHERE | 2 | 1 | 3× SMALL (AWAY) |
| 6 | MULTISPAWN | SPHERE | 1 | 3 | 3× SPAWN_MEDIUM (AWAY+FAR) |
| 7 | MULTISPAWN_SEMI | SEMISPHERE | 1 | 3 | 3× SPAWN_MEDIUM (AWAY+FAR) |

**POW_create()** (inline в .h) — высокоуровневый API:
- `POW_CREATE_LARGE_SEMI(0)` → MULTISPAWN_SEMI (полусфера + каскадные дети)
- `POW_CREATE_MEDIUM(1)` → SPAWN_SPHERE_MEDIUM
- `POW_CREATE_LARGE(2)` → MULTISPAWN (полная сфера + каскадные дети)

**Алгоритм генерации спрайтов (POW_new):**
- Сфера: pitch от -512 до +512, yaw полный оборот; кольца = `COS(pitch) * around >> 16`
- Полусфера: pitch от 0+ до 512 (только верхняя полусфера)
- Density: `around = 5 + density*3`, `upndown = 4 + density*1`
- Скорость: вектор из `FMATRIX_vector(yaw,pitch)` + random jitter `>> (speed+1)`
- Framespeed: `96 + framespeed*32` + random `& 0x3f`

**POW_process() per-frame:**
1. `ticks = (2000/20) * TICK_RATIO >> TICK_SHIFT` — delta-time
2. Для каждого спрайта: анимация кадров (frame_counter), движение (dx/dy/dz << 4), damping (`dx -= dx >> damp`)
3. Спрайт мёртв если `frame >= 16` (16 кадров анимации)
4. POW мёртв если sprite==NULL && timer==0
5. Child spawning: 3 слота, spawn когда `timer < when`; позиция = parent + случайный вектор; FAR_OFF = +50% расстояние; AWAY = наследует часть скорости

**Safety:** count guards (50 итераций POW, 256 итераций sprite) — при переполнении `POW_init()` (полный сброс!)

**Timing:** `POW_TICKS_PER_SECOND = 2000`; `POW_DELTA_SHIFT = 4` (dx/dy/dz хранятся как SWORD, сдвинуты >> 4 при записи, << 4 при движении)

---

## 2. Пиротехника — PYRO система (pyro.cpp)

**Ключевые файлы:** `fallen/Source/pyro.cpp`, `fallen/Headers/pyro.h`

**Суть:** Высокоуровневая система визуальных эффектов огня/взрывов/дыма. Каждый PYRO — это Thing с CLASS_PYRO и state machine (INIT→NORMAL). Использует PARTICLE, RIBBON, DIRT, NIGHT_dlight как субсистемы.

**Пул:** `MAX_PYROS = 64` (PC) / 32 (PSX)

**18 типов PYRO (PYRO_RANGE=18):**

| # | Тип | Назначение | Активен? |
|---|-----|-----------|----------|
| 0 | NONE | пусто | - |
| 1 | FIREWALL | стена огня по вектору | ✅ (создаёт FLICKER по линии) |
| 2 | FIREPOOL | горящая площадь | ✅ (counter < 254) |
| 3 | BONFIRE | костёр, маленький пожар | ✅ (dynamic light + звук S_FIRE) |
| 4 | IMMOLATE | сожжение персонажа/Bane | ✅ (частицы на костях, -10 HP/кадр, RIBBON огонь) |
| 5 | EXPLODE | старый купол (Pyrex, hemisphere) | ✅ (17 PyrPoint, decelerate+shrink) |
| 6 | DUSTWAVE | ударная волна пыли | ✅ (counter += 8, die at 120) |
| 7 | EXPLODE2 | новый большой взрыв | ✅ (dynamic light COS fade, counter += 7) |
| 8 | STREAMER | 4 дымовых хвоста | ✅ (radii[0..3] += 8, die when counter==0) |
| 9 | TWANGER | лучи света при взрыве | ✅ (counter += 16, die at 240) |
| 10 | FLICKER | мерцающее пламя (RIBBON) | ✅ (SIN/COS oscillation + pinch+twist) |
| 11 | HITSPANG | вспышка при попадании пули | ✅ (5 кадров PC / 3 PSX, blood или sparks) |
| 12 | FIREBOMB | огненный взрыв | ✅ (TICK_RATIO driven, die at 0xfe00) |
| 13 | SPLATTERY | разлёт крови | ✅ (PARTICLE smoke + TRACKS_AddQuad blood) |
| 14 | IRONICWATERFALL | фонтан/дымоход | ✅ (radius=0:фонтан, 2:дым, 3:камин) |
| 15 | NEWDOME | взрыв-купол со спрайтами | ✅ (SIN radius + MFX_play bang/fireball) |
| 16 | WHOOMPH | удар файрболла Balrog | ✅ (SIN flash, die at 255-16) |
| 17 | GAMEOVER | конец света (Bane финал) | ✅ (8 dlights по кругу, GS_LEVEL_LOST при counter>242) |

**PYRO_construct()** — bitmask-конструктор: `types & 1` = TWANGER, `& 2` = EXPLODE2 + DIRT_gust, `& 4` = DUSTWAVE, `& 8` = STREAMER, `& 16` = BONFIRE, `& 32` = NEWDOME + DIRT_gust, `& 64` = FIREBOMB, `& 128` = FIREBOMB + WAVE flag

**PYRO_blast_radius(x,y,z, radius, strength):** Находит CLASS_PERSON в сфере через THING_find_sphere → создаёт PYRO_IMMOLATE на каждого (поджигает).

**PYRO_hitspang():** Два варианта — (person,victim) и (person,x,y,z). Max `global_spang_count = 5`. Создаёт из LEFT_HAND позиции стрелка. Для person target: кровь (DIRT_new_water BLOOD) из HEAD. Для non-person: DIRT_new_sparks.

**IMMOLATE (сожжение) детали:**
- Крепится к victim; частицы на случайных костях (1-5 на кадр в зависимости от radius)
- Урон: -10 HP/кадр если не INVULNERABLE и не PLAYERKILL; смерть через set_person_dead(DEATH_TYPE_OTHER)
- Жизненный цикл: 2 начальных RIBBON → после смерти жертвы ещё 3 RIBBON → free_pyro при radius>350 если BurnIndex==0

**MergeSoundFX():** Переиспользует звук ближайшего PYRO того же типа (sphere radius 5*256) вместо создания нового. EXPLODE2: S_EXPLODE_START+(rand%3). BONFIRE/FLICKER: S_FIRE (looped).

---

## 3. Мусор/Debris — DIRT система (dirt.cpp)

**Ключевые файлы:** `fallen/Source/dirt.cpp`, `fallen/Headers/dirt.h`

**Суть:** Система ambient-мусора вокруг камеры (листья, банки, голуби, снег) + специальные объекты (брошенные банки, отрубленные головы, гильзы, водяные частицы, кровь, мины).

**Пул:** `DIRT_MAX_DIRT = 1024` (PC, степень 2!) / 256 (DC) / 128 (PSX)

**16 типов DIRT:**

| # | Тип | Категория | Физика | Подбор |
|---|-----|-----------|--------|--------|
| 0 | UNUSED | - | - | - |
| 1 | LEAF | ambient | waft (парение), drag 75%, gravity + pitch/roll подъём | нет |
| 2 | CAN | ambient/pickup | gravity, bounce off walls, S_KICK_CAN | → HELDCAN |
| 3 | PIGEON | ambient | state machine (11 состояний), morph-анимация | нет, **ОТКЛЮЧЁН** (prob=0) |
| 4 | WATER | projectile | gravity 15*TICK/4, bounce once → DRIP → die | нет |
| 5 | HELDCAN | held | tracks SUB_OBJECT_PREFERRED_HAND | → THROWCAN |
| 6 | THROWCAN | projectile | gravity, bounce → S_KICK_CAN + PCOM_SOUND_UNUSUAL → CAN | нет |
| 7 | HEAD | ground | как CAN | → HELDHEAD |
| 8 | HELDHEAD | held | как HELDCAN | → THROWHEAD |
| 9 | THROWHEAD | projectile | как THROWCAN → HEAD | нет |
| 10 | BRASS | ambient | gravity, bounce, prim 253, без звука | нет |
| 11 | MINE | projectile | как THROWCAN | DIRT_destroy_mine |
| 12 | URINE | projectile | как WATER | нет |
| 13 | SPARKS | projectile | timer → DIRT_spark_shower → die | нет |
| 14 | BLOOD | projectile | как WATER, без DRIP | нет |
| 15 | SNOW | ambient | как LEAF, fade когда STILL | нет |

**Ambient-система (DIRT_set_focus):**
- Фокус = камера (x, z, radius)
- Деревья: `DIRT_tree[64]` — при попадании в фокус создают `DIRT_LEAVES_PER_TREE=200` листьев
- Новый мусор: `DIRT_get_new_type()` — вероятности prob_leaf/prob_can/prob_pigeon (8-bit, сумма=256)
- Голуби: `prob_pigeon = 0` принудительно (строка 193 "no bug ridden pigeons for us!")
- world_type=FOREST → все LEAF; SNOW → все SNOW

**DIRT_process() per-frame (по типам):**
- **LEAF/SNOW:** waft эффект от pitch/roll; drag 75%x/50%y; gravity `4<<TICK_SHIFT`; подъёмная сила от наклона; wall bounce (PAP_FLAG_HIDDEN); STILL когда dx==dz==0
- **CAN/HEAD/BRASS:** yaw/pitch вращение; gravity `4<<TICK_SHIFT`; wall bounce при PAP_FLAG_HIDDEN; friction `/16`; STILL когда всё == 0; S_KICK_CAN при ударе об пол
- **WATER/BLOOD/URINE:** gravity 15*TICK/4, max dy=-511; random jitter на движении; wall bounce (dx/dz反); double-hit → DRIP → die; кровь без DRIP
- **SPARKS:** timer → DIRT_spark_shower() (PARTICLE_Add облако); потом die
- **HELDCAN/HELDHEAD:** tracks правая рука владельца (droll = owner thing index)
- **THROWCAN/THROWHEAD/MINE:** projectile с bounce; THROWCAN → CAN при малой скорости; THROWHEAD → HEAD; S_KICK_CAN + PCOM_SOUND_UNUSUAL при приземлении
- **PIGEON:** 11-state machine; morph-анимация; **только PC**; **prob_pigeon=0** → никогда не создаётся

**DIRT_find_useless():** Recycling — 8 попыток найти UNUSED/DELETE_OK; потом 8 попыток найти LEAF; потом любой случайный.

**DIRT_gust(thing, x1,z1, x2,z2):** Ветер от движения. Strength = QDIST2(dx,dz)*8. Воздействует на LEAF/CAN/HEAD/WATER/BLOOD в пределах strength.

**Специальные функции:**
- `DIRT_create_cans(x,z,angle)` — 5 банок (из barrel.cpp при столкновении бочек)
- `DIRT_create_brass(x,y,z,angle)` — 1 гильза (PC only, prim 253)
- `DIRT_create_papers(x,y,z)` — 4 листа мусора
- `DIRT_behead_person(person, attacker)` — создаёт HEAD из головы жертвы
- `DIRT_shoot(person)` — raycast попадание в CAN/HEAD debris
- `DIRT_create_mine(person)` — MINE с таймером
- `DIRT_gale(dx,dz)` — глобальный ветер
