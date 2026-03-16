# DENSE SUMMARY (часть 2) — Urban Chaos Knowledge Base

Продолжение [DENSE_SUMMARY.md](DENSE_SUMMARY.md). Секции 13–23.

---

## 13. Эффекты

**Система частиц (psystem.cpp):**
- `PSYSTEM_MAX_PARTICLES = 2048` (PC) / **64** (PSX)
- Timing: `GetTickCount()` + `NORMAL_TICK_TOCK` для delta-time (независимо от FPS)
- Флаги: `PFLAG_SPRITEANI` — анимированный спрайт (цикл anim_start..anim_end)

**Огонь (fire.cpp):**
- `FIRE_MAX_FLAMES = 256`, `FIRE_MAX_FIRE = 8`
- Время жизни пламени: `32 + (random & 0x1f)` кадров
- Смещение: `dx/dz = random & 0xff - 0x1f` (диапазон -31..+224)
- Высота: `255 - манхэттенское_расстояние(dx, dz)`
- Количество точек: `(height >> 6) + 1`, ограничено **2-4**
- Нечётные точки: `angle += 31`, `offset -= 17`; чётные: `angle -= 33`, `offset += 21`
- Каждые **4 кадра**: `size -= shrink`; добавлять языки пока `count < size >> 2`

**Искры (spark.cpp):**
- `SPARK_MAX_SPARKS = 32`; `num_points = 4` всегда
- Звук: `S_ELEC_START + (spark_id % 5)` — **5** вариантов
- `SPARK_in_sphere()`: до **16** вариантов; типы точек: LIMB/CIRCULAR/GROUND/POINT
- Флаги скорости: FAST `<<= 2`; SLOW `>>= 2`
- Lifespan: `rand() % max_life + 8`; движение: `pos += velocity >> 4` per frame
- Электрозаборы: создают искру каждые **32 кадра**

**Ткань (cloth.cpp) — ОТКЛЮЧЕНА:**
- `CLOTH_MAX_CLOTH = 16`, `CLOTH_MAX_LINKS = 256`
- `CLOTH_elasticity = 0.0003f`, `CLOTH_damping = 0.95f`, `CLOTH_gravity = -0.15f`
- Verlet-интеграция, **3** итерации/кадр
- ⚠️ `CLOTH_process()` под `#if 0` — не выполняется

**Расширяющееся кольцо (Effect.cpp):**
```c
SPEED = 128 (старт)
SPEED  = (SPEED * 50) / 64   // затухание ~78%
RADIUS += SPEED
// SPEED == 0 → удалить объект
```

**Капли дождя (drip.cpp):**
- `DRIP_MAX_DRIPS = 1024`
- `DRIP_SFADE = 255`, `DRIP_DFADE = 16`/кадр, `DRIP_DSIZE = 4`/кадр
- ⚠️ `DRIP_create_if_in_puddle()` (PC only, `#ifndef TARGET_DC`) проверяет `PAP_FLAG_REFLECTIVE` + `MAV_SPARE_FLAG_WATER`

**Дымка (mist.cpp):**
- `MIST_MAX_POINTS = 4096`, `MIST_MAX_MIST = 8`
- Затухание du/dv: **25%** + "spring" возврат **0.5%** за кадр

**Лужи (puddle.cpp) — PC only (`#ifndef TARGET_DC`):**
- Предвычисляются при загрузке (`PUDDLE_precalculate()`)
- Зависимость: `DRIP_create_if_in_puddle()` → `PUDDLE_in()` → `PAP_FLAG_REFLECTIVE`

**Статические тени (shadow.cpp):**
- Направление солнца: `SHADOW_DIR_X=+147, SHADOW_DIR_Y=-148, SHADOW_DIR_Z=-147`
- `shadow_dist = 32`; lookup: `shadow[8] = {0, 1, 7, 5, 3, 2, 4, 5}` → `PAP_FLAG_SHADOW_1/2/3`

**Ленточные следы (ribbon.cpp):**
- `MAX_RIBBONS = 64`, `MAX_RIBBON_SIZE = 32` точек
- `RIBBON_FLAG_CONVECT (16)`: `Y += 22`/кадр для всех точек (конвекция огня)
- `RIBBON_FLAG_IALPHA (32)`: инвертированный alpha (дым: прозрачный→непрозрачный)
- Рендеринг: triangle strip; `V = index × (1/TextureV) + Scroll×0.015625`

**Взрывы BANG (bang.cpp):**
- `BANG_Bang[64]`, `BANG_Phwoar[4096]`
- Type 0 (BIG): невидимый корень `radius=16`, спавнит **6** MIDDLE дочерних
- Type 3 (END): финальные частицы `radius=40`, без дочерних
- Рост: `radius += grow>>2 + 1`; дрейф вверх: `y += counter << 1`

**POW (pow.cpp):**
- `POW_Sprite[256]` (PC) / 192 (PSX), `POW_Pow[32]` (PC) / 24 (PSX)
- **8 типов** (0=UNUSED, 1-3=сферы, 4-7=с cascade spawning)
- `POW_TICKS_PER_SECOND = 2000`; **16** кадров анимации
- ⚠️ Safety: при переполнении (>50 POW / >256 sprite) → `POW_init()` (полный сброс!)

**PYRO (pyro.cpp):**
- `MAX_PYROS = 64` (PC) / **32** (PSX); **18 типов** (PYRO_RANGE=18)
- IMMOLATE: `-10 HP/кадр`; **2** начальных RIBBON + **3** после смерти
- GAMEOVER: **8** dlights по кругу; `GS_LEVEL_LOST` при `counter > 242`
- EXPLODE2: `counter += 7`; DUSTWAVE: `counter += 8`, смерть при **120**; TWANGER: `counter += 16`, смерть при **240**
- ⚠️ `PYRO_blast_radius()` → `THING_find_sphere()` → создаёт PYRO_IMMOLATE на каждого CLASS_PERSON
- `global_spang_count` max = **5** (PYRO_hitspang)
- `MergeSoundFX()`: радиус слияния звуков = `5*256` (sphere)

**DIRT (dirt.cpp):**
- `DIRT_MAX_DIRT = 1024` (PC, степень 2!) / 256 (DC) / 128 (PSX)
- **16 типов** (0=UNUSED, 1-15)
- LEAF/SNOW drag: **75%** x, **50%** y; gravity `4 << TICK_SHIFT`
- WATER/BLOOD gravity: `15*TICK/4`, max dy = -511
- `prob_pigeon = 0` принудительно — никогда не создаётся
- `DIRT_find_useless()`: **8** попыток UNUSED → **8** попыток LEAF → случайный
- Ambient: `DIRT_tree[64]`; попадание в фокус → `DIRT_LEAVES_PER_TREE = 200` листьев
- Гильзы BRASS: prim **253**, PC only

**Малые системы:**

*Вода (Water.cpp):*
- `WATER_Point = 1024` (PC) / 102 (PSX); x,z = 1-bit fixed, y = SBYTE height
- `WATER_Face` max **512**; `WATER_Map` = 128×128 per-square linked list
- Формулы: `x = wp->x * 128.0`, `z = wp->z * 128.0`, `y = wp->y << 5`
- UV: `u = wp->x * 0.19`, `v = wp->z * 0.19`; цвет base: **0xffbfbfbf**
- ⚠️ Wibble анимация — закомментирована в пре-релизе

*Растяжки (trip.cpp):*
- `TRIP_Wire` max **32**; x1,z1,x2,z2 = 8-bit fixed point
- Детекция: point-to-line distance < **0x20**, Y-range: `feet - 0x10` .. `head + 0x10`
- Proximity culling: Manhattan distance > **0x500** → пропуск
- Debounce: `wait = 4` кадра; проверяет 3 точки: left foot, right foot, head
- Deactivation marker: `x1 = 0xffff`

*Sphere-Matter (sm.cpp):*
- `SM_Sphere` max **1024**; `SM_Link` max **1024**; `SM_Object` max **16**
- Resolution 2–5 = **8–125 сфер** на объект; `SM_GRAVITY = -0x200`
- Jellyness: **0x10000..0x00400**; Ground bounce: `dy = -abs(dy); dy -= dy >> 9` (**11.7%** потеря энергии)

*Следы и декали (tracks.cpp):*
- TYRE_SKID: цвет **0xFFFFFF**, ширина **10**
- Кровь: брызги размер 1-31, лужа размер **80-95**; отключается при `!VIOLENCE`

*Воздушные шары (Balloon.cpp) — ВЫРЕЗАНЫ:*
- max **32** шаров; Buoyancy: `dy += 0x40` per frame; ⚠️ `load_prim_object(PRIM_OBJ_BALLOON)` закомментирован

---

## 14. Камера

- ⚠️ `cam.cpp` — мёртвый код: весь файл обёрнут в `#ifdef DOG_POO`, нигде не определяется
- Единственная активная система: **FC (Final Camera)**, `fc.h` / `fc.cpp`
- `FC_MAX_CAMS = 2`; `CAM_MORE_IN = 0.75F` — PC-камера на 25% ближе к игроку
- `FC_Cam.shake` (UBYTE): decay: `shake -= 1 + shake >> 3`
- `FC_Cam.lens = 0x28000 * CAM_MORE_IN` — фиксирован, FOV не меняется

**Режимы камеры (PC cam_dist, cam_height):**

| Тип | cam_dist (PC) | cam_height |
|-----|--------------|------------|
| 0 | `0x280 * 0.75 ≈ 480` | **0x16000** |
| 1 | **0x280 = 640** | **0x20000** |
| 2 | **0x380 = 896** | **0x25000** |
| 3 | **0x300 = 768** | **0x8000** |

- PSX type 0: `cam_dist = 0x300`, `cam_height = 0x18000`

**Смещения focus_dist:**
- Gun-out: `ddist=200`; InCar: `ddist=356`; движение/стояние: `ddist=256`
- ⚠️ Код обнаружения обрыва при STATE_IDLE — мёртвый (`&& 0` в строке 180)

**focus_yaw поправки (из FC_calc_focus):**
- `ACTION_SIDE_STEP / ACTION_SIT_BENCH / ACTION_HUG_WALL`: `yaw += 1024`
- `SUB_STATE_ENTERING_VEHICLE` (ANIM_ENTER_TAXI): `yaw -= 712`; иначе `yaw += 712`
- `STATE_DANGLING`: `±550` от текущего yaw
- `GF_SIDE_ON_COMBAT`: `yaw -= 512`
- Задний ход транспорта: `yaw_car += 1024`

**Высота камеры (FC_focus_above), поправки lower:**
- Gun-out: `lower = 0xa000`; InCar type 4: `lower = -0x3000 * CAM_MORE_IN`
- InCar types 0/5/6/8: `lower = -0x1000 * CAM_MORE_IN`; InCar default: `lower = 0xa000 * CAM_MORE_IN`
- `InsideIndex != 0`: `cam_height * 1.5`; splitscreen (FC_cam[1] active): `above -= 0x4000`

**Алгоритм сглаживания позиции (шаг 11):**
```c
if |delta| > 0x800:
    x += dx >> 2,  y += dy >> 3,  z += dz >> 2
else: snap (want = actual, smooth_transition = FALSE)
```

**Скорость get-behind (шаг 8):**
- entering_vehicle: `>> 3` (быстро); driving: `>> 5` (медленно); near wall: `>> 6 + >> 5`; normal: `>> 3`

**Ограничения dy (шаг 9):**
- `dy >>= 3`; cap нормально: `dy ∈ [-0x0c00, +0x0d00]`; `dy > 0x10000`: cap = 0x2000
- `FACE_FLAG_WMOVE`: `dy <<= 5` (мгновенно)

- `toonear = TRUE` → first-person mode; `toonear_dist = 0x90000`
- Отмена toonear: `dist > toonear_dist + 0x4000` И `dangle > 200`
- Shake: смещение `random(shake) × 128` юнитов
- lookabove: `STATE_DEAD/DYING`: `-0x80/кадр`; иначе `0xa000`
- Разрыв с warehouse: `EWAY_cam_jumped = 10` → `FC_setup_camera_for_warehouse()`

---

## 15. Рендеринг

**Константы:**
- `POLY_NUM_PAGES = 1508` (0–1407 обычные, 1408+ служебные)
- `MAX_VERTICES = 32 000`
- `TEXTURE_NUM_STANDARD = 22 × 64 = 1408` базовых страниц
- `TEXTURE_SHADOW_SIZE = 64×64` px

**Типы DT_* — 15 типов** (DT_NONE=0 .. DT_BIKE=15)

**Трансформация:**
```c
Z  = POLY_ZCLIP_PLANE / vz   // reciprocal depth; POLY_ZCLIP_PLANE = 1/64 (near clip)
pp->X = POLY_screen_mid_x - POLY_screen_mul_x * vx * Z
pp->Y = POLY_screen_mid_y - POLY_screen_mul_y * vy * Z
```

**Сортировка (bucket sort — painter's algorithm):**
- `buckets[2048]`; `bucket = int(sort_z * 2048)`, clamp [0, 2047]
- Рендер: i=0..2047 → FAR TO NEAR (back-to-front)
- `POLY_FADEOUT_START = 0.60F`, `POLY_FADEOUT_END = 0.95F`
- Fog outdoor: `fog_colour = NIGHT_sky_colour`; GF_SEWERS/GF_INDOORS: чёрный

**UV упаковка PrimFace4:**
```c
u = float(UV[i][0] & 0x3f) * (1.0F / 32.0F)   // 6 бит U
v = float(UV[i][1]       ) * (1.0F / 32.0F)   // 8 бит V
page = (UV[i][0] & 0xc0) << 2 | face->TexturePage + FACE_PAGE_OFFSET
// FACE_PAGE_OFFSET = 8*64 = 512; Атлас: 8×8 тайлов × 64×64 px
```

**Vertex Morphing:**
- `tween ∈ [0, 256]`, линейная интерполяция между keyframes
- `CMatrix33 = {SLONG M[3]}` — 3 packed SLONG, scale=128
- `GameKeyFrameElement = 8 байт`: {m00,m01,m10,m11: SBYTE; OffX,Y,Z: SBYTE; Pad: UBYTE}
- ⚠️ `InterruptFrame` = МЁРТВЫЙ КОД (всегда NULL)
- ⚠️ `HIGH_REZ_PEOPLE_PLEASE_BOB` = закомментирован

**Деформация машин (crumple):**
- `MESH_car_crumples[5 уровней × 8 вариантов × 6 точек]`

**Освещение NIGHT:**
- `NIGHT_MAX_SLIGHTS = 256`, `NIGHT_MAX_DLIGHTS = 64`, `NIGHT_MAX_SQUARES = 256`
- `NIGHT_MAX_WALKABLE = 15000`, `NIGHT_MAX_BRIGHT = 64` (6-bit, ×4 → D3D)
- `NIGHT_MAX_FOUND = 4` (макс. источников на 1 точку)
- Конвертация: `r = col.red * 4` (0..63 → 0..252)
- Pseudo-specular (PC only): если `r > 255` → overflow в `wred = (r-255)>>1`
- Terrain lighting cache: `NIGHT_Square.colour[16]` (4×4 блок); `NIGHT_cache[32][32]`
- ⚠️ **Баг:** `NIGHT_light_mapsquare()`: `dprod += dz*nx` вместо `dz*nz`
- Ambient нормаль: `nx = (h[mx-1,mz] - h[mx+1,mz]) * 4`; `ny = 256 - QDIST2(|nx|,|nz|)`
- `dprod = clamp(dprod, 0..65536) >> 9 + 128` → диапазон **128..255**
- Indoor: фиксированный `colour = 0x80808080`
- ⚠️ `NIGHT_generate_walkable_lighting()` = МЁРТВЫЙ КОД (`return` сразу)
- ⚠️ `MapElement.Colour` = МЁРТВЫЙ КОД (только в Glide Engine)
- Динамических теней НЕТ — только flat sprite `POLY_PAGE_SHADOW` под ногами

**Crinkle (micro-geometry bump mapping):**
- Лимиты: **256** crinkles, **8192** points, **8192** faces, **700** points/crinkle
- LOD: близко → полный extrude; далеко → не рендерится
- ⚠️ Отключены в пре-релизе (`CRINKLE_load()` → `return NULL`), но **работают в финальной PC** (bump на ящиках)

**Compile-time полиморфизм (tom.cpp, включается 3 раза):**
```c
SW_draw_span_masked(...)   → ALPHA_MODE = ALPHA_TEST
SW_draw_span_alpha(...)    → ALPHA_MODE = ALPHA_BLEND
SW_draw_span_additive(...) → ALPHA_MODE = ALPHA_ADD
```

**Compile-time флаги рендеринга:**
- `DRAW_WHOLE_PERSON_AT_ONCE = 1` → batch **15** body parts, D3D MultiMatrix extension
- `USE_D3D_VBUF = 1` → "+33% faster"
- `ALPHA_BLEND_NOT_DOUBLE_LIGHTING = 1` → `shl ecx,23` vs `shl ecx,23-1`
- `DISABLE_CRINKLES`: DC=1, PC=0 → crinkles включены на PC (до **8192** точек)
- `SUPERCRINKLES_ENABLED = 0` — отключён, stubbed
- `CALC_CAR_CRUMPLE = 0` — отключён, используется таблица

**⚠️ КРИТИЧНАЯ ЗАВИСИМОСТЬ — рендерер мутирует game state:**
- `DRAWXTRA_MIB_destruct()` (drawxtra.cpp): модифицирует `ammo_packs_pistol`, создаёт PYRO_TWANGER и SPARK — из рендерера!
- `PYRO_draw_armageddon()` (drawxtra.cpp): создаёт PYRO_NEWDOME, PARTICLE_Add(), MFX_play_xyz() — из рендерера!
- `engineMap.cpp` — безопасен (read-only)

---

## 16. Звук

- Движок: **Miles Sound System (MSS32)**, `mss32.dll`
- Координаты 3D-аудио: world units `>> 8` → mapsquares
- Специальные каналы: `WIND_REF = MAX_THINGS + 100`, `WEATHER_REF = +101`, `MUSIC_REF = +102`
- Громкость: **0..127**; `config.ini`: `ambient_volume`, `music_volume`, `fx_volume`

**14 музыкальных режимов (MUSIC_MODE_*):**
```
NONE, CRAWL, DRIVING1, DRIVING2, DRIVING_START, FIGHT1, FIGHT2,
SPRINT1, SPRINT2, AMBIENT, CINEMATIC, VICTORY, FAIL, MENU
```
- Приоритет: CINEMATIC > FIGHT > DRIVING > SPRINT > CRAWL > AMBIENT
- Файлы: `data/sfx/1622/musik01-musik08/` — 8 вариантов × подпапки

**Биомы ambient (по texture_set):**

| Биом | texture_set | Интервалы |
|------|-------------|-----------|
| Jungle | 1 | — |
| Snow | 5 | волки каждые **1500+** кадров |
| Estate | 10 | самолёты каждые **500+** кадров |
| BusyCity | default | — |
| QuietCity | 16 | — |

**Погода:**
```c
дождь gain = 255 - (above_ground >> 4)  // громче у земли
ветер gain = abs_height >> 4            // громче на высоте
```
- `GF_RAINING` — ночью всегда дождь (хардкод)
- Indoor ambient по `WARE_ware[].ambience`: office, police, posheeta, club
- Таймеры ambient: `creature_time = 400`, `siren_time = 300`
- `FLAGS_PLAYED_FOOTSTEP` (бит 12), сбрасывается каждый кадр
- MFX флаги: `MFX_REPLACE`, `MFX_QUEUED`, `MFX_OVERLAP`, `MFX_LOOPED`, `MFX_SHORT_QUEUE`, `MFX_WAVE_ALL`
- SFX: WAV PCM 16-bit, **22050 Hz** или **44100 Hz**
- Диалоги: `talk2/{LevelName}.ucm{EventIndex}.wav` или `{CharacterType}{Index}.wav`

---

## 17. UI и Frontend

**HUD pipeline (каждый кадр):**
- Порядок: `PANEL_start()` → `PANEL_draw_buffered()` → прицелы → HP → `PANEL_last()` → инвентарь → debug → `PANEL_finish()`
- Таймеры обратного отсчёта одновременно: **до 8**
- HP индикатор: центр (+74, -90), радиус **66px**, **8 сегментов**, arc **-43°→-227°**
- HP коэффициенты: Darci `hp*(1/200)`, Roper `hp*(1/400)`, Vehicle `hp*(1/300)`
- Стамина: **5 квадратов 10×10px**, интервал `+3x/-3y`; цвета: `Stamina/25` → red→orange→yellow→green→bright green
- Прицелы: до `MAX_TRACK=4` целей; **4 линии** внешних (R=**164**) + **4** внутренних (R=`20+accuracy/4`)
- Scale прицела: Person=**256**, Special=**128**, Balrog=**450**, Barrel=**200**, Vehicle=**450**
- HP полоска над врагом: MIB `hp*100/700`; Person `hp*100/health[type]` (radius 60)
- PC bar: **54×4px**; DC bar: **54×8px**
- `m_iPanelXPos`: PC=**0**, DC=**32**; `m_iPanelYPos`: PC=**480**, DC=**448**
- `FONT2D_LETTER_HEIGHT = 16`

**Frontend режимы:**
- `FE_MAINMENU=1`, `FE_MAPSCREEN=2`, `FE_MISSIONSELECT=3`, `FE_MISSIONBRIEFING=4`
- `FE_LOADSCREEN=5`, `FE_SAVESCREEN=6`, `FE_CONFIG=7..13`, `FE_CREDITS=-5`, `FE_START=-3`, `FE_BACK=-4`
- mode **≥ 100** = брифинг (`100 + mission_index`)
- Стек меню: **до 10 уровней** (`MenuStack stack[10]`)
- Джойпад deadzone: `NOISE_TOLERANCE = 4096` (ось ±4096 от 32768)
- Слайдер шаг: `Data ± 2` (LEFT/RIGHT)

**MenuFont флаги:**
- `MENUFONT_FUTZING=1`, `MENUFONT_HALOED=2`, `MENUFONT_GLIMMER=4`, `MENUFONT_CENTRED=8`
- `MENUFONT_FLANGED=16`, `MENUFONT_SINED=32`, `MENUFONT_RIGHTALIGN=128`, `MENUFONT_SHAKE=1024`

**DarciDeadCivWarnings (персистирует между миссиями):**
- Порог: `Player->RedMarks > 1`; **Счётчик 3** → `GS_LEVEL_LOST`; фон: `deadcivs.tga`

**Mucky Times speedrun код:**
- `hash = (i+1)*(m+1)*(s+1)*3141 % 12345 + 0x9a2f`; список ~**35 миссий**

**Зависимости Frontend:**
- ⚠️ Тема меню (`complete_point`) → `texture_set` уровней — одна переменная управляет обоими
- `MUSIC_bodge_code` (0–4) override музыкальной темы устанавливается в frontend перед запуском
- ⚠️ `startscr.cpp` в game-сборке = одна строка: `CBYTE STARTSCR_mission[_MAX_PATH] = {0};`

**Мёртвый код Frontend:**
- `track_enemy()` — `#ifdef OLD_POO`
- `help_system()` — `#ifdef UNUSED`
- `DAMAGE_TEXT` — define не задан; `MAX_DAMAGE_VALUES=16`
- Старый attract demo (playback `.pkt`) — закомментирован
- `DONT_load` bitmask = 0 → грузится всё всегда
- ⚠️ `STARTSCR_notify_gameover()` закомментирован → `auto_advance` никогда не ставится

---

## 18. Прогресс и сохранения

- Три слота: `saves/slot1.wag`…`saves/slot3.wag` (1-based)
- ⚠️ **Версия v3:** байт `version` стоит **после** `complete_point` — "strange place, Historical Reasons"

**Бинарный layout .wag v3:**
```
mission_name : variable-length string + CRLF (0x0D 0x0A)
complete_point : UBYTE
version        : UBYTE
--- version >= 1 ---
DarciStrength/Constitution/Skill/Stamina : UBYTE×4
RoperStrength/Constitution/Skill/Stamina : UBYTE×4
DarciDeadCivWarnings : UBYTE
--- version >= 2 ---
mission_hierarchy[60] : UBYTE[60]
--- version >= 3 ---
best_found[50][4] : UBYTE[200]
```
- Итого v3: `len(name) + 274` байт
- `complete_point` практический максимум: **40** (чит `KB_ASTERISK`)

**Пороги темы меню по complete_point:**
```c
if (complete_point < 8)  theme = 0;  // leaves
if (complete_point < 16) theme = 1;  // rain
if (complete_point < 24) theme = 2;  // snow
// иначе                  theme = 3  // blood
```

- `mission_hierarchy[60]` биты: **bit 0** = существует, **bit 1** = завершена, **bit 2** = доступна
- Корневая миссия: `mission_hierarchy[1] = 3`
- `best_found[50][4]` индексы: `[0]`=Constitution, `[1]`=Strength, `[2]`=Stamina, `[3]`=Skill

**Механика анти-фарм:**
```c
found = current_stat - saved_stat;
if (found > best_found[mission][i])
    stat += found - best_found[mission][i];  // только разница с рекордом
```
- ⚠️ **Баг:** `memset(&best_found[0][0], 50*4, 0)` — аргументы size/value перепутаны
- `VIOLENCE_ALLOWED = 1` по умолчанию; FALSE для FTutor1, Gangorder1, Album1
- Чекпоинтов нет; `GS_REPLAY` → полный рестарт через `goto round_again`
- `config.ini`: `max_frame_rate=30`, `draw_distance=22`

---

## 19. Загрузка уровня

**Порядок шагов:**
1. Загрузка PAP-сетки из `.iam` (128×128 × 6 байт = **96 KB**)
2. Загрузка SuperMap (DBuilding, DFacet, DStyle, DStorey и т.д.)
3. Загрузка примов `.prm`
4. `OB_add_walkable_faces()` + `calc_slide_edges()` — ПОСЛЕ загрузки примов ⚠️
5. Загрузка освещения `.lgt`; дефолт при отсутствии:
```c
NIGHT_ambient(255, 255, 255, 110, -148, -177);
```
6. Загрузка текстур `.txc`
7. `build_quick_city()` → `ROAD_wander_calc()` → `WAND_init()`
8. ⚠️ **`MAV_precalculate()`** — тяжелейшая операция: 128×128 ячеек, нельзя async
9. ⚠️ `EWAY_process()` — создаёт игрока и NPC (без него `create_player()` не вызывается)

- `MAX_EVENTPOINTS = 512`; кэш: **128-элементный**
- Ночью всегда дождь: `if (!(NIGHT_flag & NIGHT_FLAG_DAYTIME)) GAME_FLAGS |= GF_RAINING`
- `GAME_TURN = 0` сбрасывается при каждой загрузке
- `texture_set`: диапазон **0–21** (`TEXTURE_choose_set()`)
- PSX texture remapping читается только если `version >= 25`
- Игрок создаётся через тот же `PCOM_create_person()`, что и NPC — только с другим `player_id`

---

## 20. Межсистемные зависимости

1. ⚠️ **Рендерер мутирует game state** — `DRAWXTRA_MIB_destruct()` в `drawxtra.cpp` изменяет `ammo_packs_pistol`, запускает PYRO/SPARK. Нарушение слоистости.
2. ⚠️ **Камера зависит от MAV** — `FC_process` использует `MAV_inside` для коллизии камеры с геометрией.
3. ⚠️ **Двери синхронизированы с MAV** — открытие/закрытие двери → `MAV_turn_movement_on/off`; при turn_on caps заменяются только GOTO (прыжки теряются навсегда).
4. ⚠️ **EWAY контролирует спавн** — игрок создаётся через `EWAY_process`, не напрямую.
5. ⚠️ **AI таймер независим от frame rate** — `PCOM_TICKS_PER_SEC=320`, не привязан к рендер-FPS.
6. ⚠️ **Анимации управляют физикой** — `GameFightCol` в кадрах анимации; `gravity_anim_driven` у персонажей.
7. ⚠️ **MAV_precalculate блокирует загрузку** — самый тяжёлый шаг, нельзя сделать async без изменения архитектуры.
8. ⚠️ **Тема меню = тема текстур уровней** — одна переменная `complete_point` управляет обоими.
9. ⚠️ **INSIDE2 навигация не работает** — полная заглушка в пре-релизе; NPC в некоторых зданиях застревают.
10. ⚠️ **Боевые коллизии привязаны к кадрам анимации** — `GameFightCol` встроен в animation data, не в физику.
11. ⚠️ **Рендерер Bane создаёт эффекты** — `PYRO_draw_armageddon()` в drawxtra.cpp создаёт PYRO/PARTICLE/MFX из рендерера.
12. ⚠️ **PAP_FLAG бит-конфликты** — бит 9: `ROOF_EXISTS` vs `ANIM_TMAP`; бит 14: `WANDER` vs `FLAT_ROOF` — контекстно-зависимы.
13. ⚠️ **Плавание в MAV но нет анимации** — `MAV_SPARE_FLAG_WATER` есть, анимации плавания нет.
14. ⚠️ **Постпроцессинг зданий зависит от дорог** — `build_quick_city()` → `ROAD_wander_calc()` → `WAND_init()` — цепочка.

---

## 21. Пре-релизные баги и особенности

**Коды/заглушки пре-релиза (в финале исправлены или работают):**
1. ⚠️ `INSIDE2_mav_inside` → `ASSERT(0)` — AI в некоторых зданиях не навигирует
2. ⚠️ `INSIDE2_mav_stair`/`mav_exit`: баг `> 16` вместо `>> 16`; цикл по Z использует MinX..MaxX
3. ⚠️ Воскрешение гражданских на месте смерти (teleport вычислен, но не применён)
4. ⚠️ `fn_cop_normal()` под `#if 0` — AI копов = заглушка
5. ⚠️ `fn_roper_normal()` — пустая функция
6. ⚠️ `fn_thug_init()` содержит `ASSERT(0)` (но не вызывается из people_functions[])
7. ⚠️ `mount_ladder()` закомментирован — игрок не может залезть снизу; AI может (pcom.cpp:12549)
8. ⚠️ `WPT_BONUS_POINTS` → `if(0)` → всегда `EWAY_DO_MESSAGE`, очки не начисляются
9. ⚠️ `WPT_GOTHERE_DOTHIS (39)` → `default: ASSERT(0)`
10. ⚠️ `EWAY_COND_PRESSURE (4)` и `EWAY_COND_CAMERA (5)` — always FALSE
11. ⚠️ Roper stats bug: `EWAY_create_player(PLAYER_ROPER)` использует Darci-статы
12. ⚠️ Canid/Dog — главный dispatch switch закомментирован; полностью инертны
13. ⚠️ Wibble анимация воды — закомментирована
14. ⚠️ Запрыгивание на лестницу снизу — закомментировано в `interfac.cpp`
15. ⚠️ Zipwire — закомментирован в пре-релизе, **работает в финале** (тренировочный уровень 1)
16. ⚠️ `CRINKLE_load()` → `return NULL` — crinkle отключены в пре-релизе, **работают в финале**
17. ⚠️ Снайпер SHOOT_DEAD: "убрать пистолет после 10 сек" закомментирован
18. ⚠️ Комментарий в коде говорит "10 seconds" для C4, реально **5 секунд** (баг в комментарии)
19. ⚠️ `NIGHT_light_mapsquare()`: `dprod += dz*nx` вместо `dz*nz` — баг освещения рельефа
20. ⚠️ HM_collide_all: `already_bumped[i]` вместо `[j]` — пропускаются некоторые HM-HM столкновения
21. ⚠️ `STARTSCR_notify_gameover()` закомментирован → `auto_advance` никогда не ставится
22. ⚠️ `valid_grab_angle()` — ВСЕГДА возвращает 1 (угловая валидация grab отключена)
23. ⚠️ Плавание — `MAV_SPARE_FLAG_WATER` есть, анимации нет
24. ⚠️ `gy = 0` hardcoded в HyperMatter → объекты отскакивают от Y=0, не реального terrain
25. ⚠️ `memset(&best_found[0][0], 50*4, 0)` — аргументы size/value перепутаны в save system
26. ⚠️ `POW_init()` (полный сброс) при переполнении счётчиков (>50 POW / >256 sprite)
27. ⚠️ Хардкод нод (121,33) в `gpost3.iam` принудительно пропускается в ROAD

**Мёртвый код (не переносить):**
- `CLOTH_process()` — под `#if 0`; `BALLOON_*` — prim не загружается
- `BARREL_position_on_hands()` + `BARREL_throw()` (строки ~1505-1690)
- `BIKE_*` — 453 вхождения, 38 файлов; никогда не компилируется (нет `#define BIKE`)
- `SPECIAL_THERMODROID (11)` — нефункционален; `SPECIAL_MINE (10)` — нельзя подобрать
- `InterruptFrame` в DrawTween — всегда NULL
- `NIGHT_generate_walkable_lighting()` — return сразу; `MapElement.Colour` — только Glide
- Command.cpp — 12 из 13 команд = стабы; заменено PCOM
- `GS_REPLAY` — есть в коде, не используется
- `TIMEOUT_DEMO = 0` — demo_timeout() мёртвый код
- `DeltaVelocity` в Thing.h — поле есть, использование неясно (незавершённая физика?)

---

## 22. Форматы файлов

### .iam — карта уровня
- PAP_Hi: **128×128 × 6 байт = 98 304 байт**; `UWORD Texture + UWORD Flags + SBYTE Alt + SBYTE Height`
- `save_type < 18` = PSX; `>= 18` = PC; `>= 20` = texture_set; `>= 23` = OB данные в конце SuperMap
- `LoadGameThing`: **44 байта** (2+2+4+4+4+4+2+2+2+2+16)
- SuperMap секции: DBuilding, DFacet, DStyle, DStorey (>= 17), InsideStorey+Staircase+room_id (>= 21)
- MapWho: 128×128 spatial hash — заново строится, не хранится

### .prm — статические примы (1–265 типов)
- `PRIM_START_SAVE_TYPE = 5793`; save_type == PRIM_START_SAVE_TYPE+1 → PrimPoint = SWORD (6 байт); иначе = SLONG (12 байт)
- PrimObject: **16 байт** (UWORD×4 + SWORD×2 + UBYTE×4)
- PrimFace3: **28 байт** (TexturePage, DrawFlags, Points[3], UV[3][2], Bright[3], ThingIndex, Col2, FaceFlags, Type, ID)
- PrimFace4: **34 байта** (аналогично + Points[4])
- **FACE_FLAG_WALKABLE = (1<<6)**; StartFace3/EndFace3: SWORD (отрицательные = нет треугольников)

### .moj / .all — модели и анимации
- `.all` save_type 2–5; **15 body parts** у людей; check magic: **666**
- `global_anim_array[4][450]`: AnimType 0=DARCI, 1=ROPER, 2=ROPER2, 3=CIV
- `AnimBank` файлы: `darci1.all` → game_chunk[0], `hero.all` → game_chunk[1], `bossprtg.all` → game_chunk[3]
- `CMatrix33`: `float * 32768.f` → SLONG (fixed-point 16.15); распаковка: `((value & mask) >> shift) / 512.f`
- `GameKeyFrameElement = 8 байт`: {m00,m01,m10,m11,OffX,OffY,OffZ: SBYTE; Pad: UBYTE}
- Гражданские: `PersonID = 6 + random_number % 4` (4 варианта)

### .txc — архив текстур уровня
- Header: `ULONG MaxID` → `Offsets[MaxID]` → `Lengths[MaxID]`
- ⚠️ `sizeof(size_t)` — платформозависимо, **32/64-bit несовместимо**!
- Несжатый: 2+2+2 байта header + width×height×2 пикселей
- RGBA 4:4:4:4 (contains_alpha=1): биты 3..0=B, 7..4=G, 11..8=R, 15..12=A
- RGB 5:6:5 (contains_alpha=0): биты 4..0=B, 10..5=G, 15..11=R
- Сжатый (WriteSquished): маркер **0xFFFF**, затем палитра + битстрим (ceil(log2(total)) бит/пиксель, MSB first)

### Текстурные страницы
- `TEXTURE_MAX_TEXTURES = 22*64 + 160 = 1568` слотов
- Диапазоны: 0-255 World, 256-511 Shared, 512-575 Interior, 576-703 People, 704-1151 Prims, 1152-1407 People2, 1408-1567 спецэффекты, 1568-1759 Crinkle
- Разрешения: `tex%03d.tga` (32×32), `tex%03dhi.tga` (64×64), `tex%03dto.tga` (128×128)
- 2-pass страницы (`POLY_PAGE_FLAG_2PASS`): страница N = цвет, N+1 = glow маска

### .tma — таблицы тайловых текстур
- `style.tma`: **200 стилей × 5 слотов**; DXTXTY: только Page (UWORD) + Flip (UWORD)
- `instyle.tma`: count_x × count_y байт индексов интерьерных текстур

### .lgt — освещение уровня
- `ED_Light`: **20 байт** (8 однобайтных + SLONG x/y/z)
- `NIGHT_Colour`: **3 байта**, диапазон **0–63** (не 0–255!)
- Ambient: `amb_red * 820 >> 8` (нормировка)

### .sty — скрипт набора миссий
- Буфер: **20KB** (`SCRIPT_MEMORY`); Format v4
- `@` в Briefing → `\r` (перенос строки)
- **27 районов** (0–26); `suggest_order[]` — захардкожен в `frontend.cpp`, 36 миссий + sentinel `"!"`
- ⚠️ Миссии не в suggest_order пропускаются при автовыборе

### .ucm — скрипт миссии
- Заголовок **1316 байт**; EventPoint **74 байта**; MAX_EVENTPOINTS=**512** → **37 888 байт** EP-секция
- Версия `M_VERSION = 10`

---

## 23. Математика и утилиты

**Два стека: PSX vs PC:**
- PSX: углы **0–2047** за полный оборот; целочисленная арифметика; lookup tables
- PC: углы в **радианах** (float); libm

**PSX sin/cos (StdMaths.cpp):**
- SinTable: **2560 entries** (не 2048!); CosTable = `&SinTable[512]` (сдвиг π/2)
- Значения масштаб: **65536 = 1.0** (16.16 fixed-point, использовать `>>16`)
- В maths.cpp: `sinx = SIN(xangle & (2048-1)) >> 1` → точность **15 бит**

**Arctan (StdMaths.cpp):**
- AtanTable[256]: покрывает 0–90° в масштабе **0–512**
- Возвращает **0–2047**; квадрант через смещения 512/1024/1536
- `FMATRIX_find_angles`: `*yaw = 1024 - Arctan(x, z)`

**PC матрицы (Matrix.h) — 3×3 row-major, 9 float:**
```
matrix[0..2] — row 0
matrix[3..5] — row 1
matrix[6..8] — row 2
```
- `MATRIX_find_angles`: `pitch = asin(matrix[7])`, `yaw = atan2(matrix[6], matrix[8])`, обрабатывает gimbal lock (pitch > π/4)

**Кватернионы (только PC):**
- `CQuaternion: { float w, x, y, z }`
- SLERP: если `dot > 0.95` → линейная интерполяция; иначе `scale0 = sin((1-t)*omega)/sin(omega)`
- Integer версия: fixed-point **15.15** (`1 << 15` = 32768); `acos_table[1025]`

**Fixed-point форматы:**

| Формат | Масштаб | Где используется |
|--------|---------|-----------------|
| 15.15 | 32768 | Кватернион integer |
| 16.16 | 65536 | GameCoord, sin/cos таблица |
| 32.8 | 256 | Thing.WorldPos (позиции объектов) |

**Root() (StdMaths.cpp):**
- Обёртка над `sqrt()`: `return (int)sqrt(square)`; используется в FMatrix.cpp строки 213, 233

---

## Быстрые числа

**Карта и координаты:**
- PAP_Hi: 128×128, 6 байт/ячейка, мир 32768×32768
- WorldPos × 256 = world units; `>> 8` = mapsquare; `>> 18` = lo-res cell
- Углы: 0–2047 (2048=360°); Thing.yaw: 0–255 UBYTE

**Thing System:**
- Thing struct: **125 байт**; MAX_THINGS: PRIMARY=400 + SECONDARY=300 = **700** (PC); 19 классов

**Физика:**
- GRAVITY = **–51** юн/тик² (транспорт); Darci GRAVITY = `4<<8 = 1024` units/tick
- TICKS_PER_SECOND = **80**; TICK_LOOP = **4**; NORMAL_TICK_TOCK = **66.67 мс**
- Fall damage Darci: порог DY=–20000, instant kill DY=–30000; water_level = **–128**

**AI:**
- PCOM_TICKS_PER_SEC = **320**; PCOM_TICKS_PER_TURN = **16**
- PCOM_ARRIVE_DIST = **0x40 = 64** units; RMAX_PEOPLE = **180**
- NPC view range = **2048**; Player view range = **65536**
- NPC FOV near = **700** (~123°), far = **420** (~74°)
- Eye height stand = **96**, crouch = **32**

**Навигация:**
- MAV: 128×128 ячеек, 1 ячейка = 256 мировых единиц
- MAV_LOOKAHEAD = **32** ячейки; priority queue = **256** записей
- Wallhug: до **252** вэйпойнтов; **20000** шагов max

**Транспорт:**
- MAX_VEHICLES = **40**; Health start = **300**
- VEH_SPEED_LIMIT = **750**; VEH_REVERSE_SPEED = **300**
- WHEELTIME = **35** кадров; SKID_FORCE = **8500**

**Оружие:**
- Пистолет: 15 патр., урон **70**, скорострельность **140** тиков
- АК-47: 30 патр., урон **100**/40, скорострельность **64** тиков
- Дробовик: 8 патр., урон `300-dist`, скорострельность **400** тиков
- Граната: timer = **1920** тиков (6с); C4: **1600** тиков (5с)
- Взрыв: `hitpoints = maxdamage * (radius - dist) / radius`; KO если > **30**

**Здания:**
- FBuilding max **500**, FStorey max **2500**, FWall max **15000**
- DBuilding max **1024**, DFacet max **16384**; BLOCK_SIZE = **64**
- 21 тип STOREY_TYPE_*; DOOR_MAX_DOORS = **4**

**Эффекты:**
- PSYSTEM: 2048 частиц (PC); FIRE: 256 пламеней; MAX_PYROS = **64**
- DIRT_MAX_DIRT = **1024** (PC); MAX_RIBBONS = **64**; SPARK_MAX_SPARKS = **32**
- POW_Sprite = **256**; BANG_Phwoar = **4096**

**Рендеринг:**
- MAX_VERTICES = **32 000**; buckets = **2048**
- TEXTURE_MAX_TEXTURES = **1568**; POLY_NUM_PAGES = **1508**
- POLY_FADEOUT_START = **0.60**; POLY_FADEOUT_END = **0.95**
- NIGHT_MAX_SLIGHTS = **256**; NIGHT_MAX_DLIGHTS = **64**; NIGHT_MAX_WALKABLE = **15000**

**Сохранения:**
- .wag v3: `len(name) + 274` байт; complete_point max = **40**
- best_found[50][4] = **200** байт

**Персонажи:**
- PERSON_NUM_TYPES = **15**; MIB Health = **700**; Darci max Health = **200**
- TweenStage: 0–256; GameKeyFrameElement = **8 байт**

**Миссии:**
- MAX_EVENTPOINTS = **512**; EventPoint = **74 байта**
- CRIME_RATE default = **50**; RedMarks max = **10**

**Препроцессорные флаги — ключевые:**
- PC Release: `NDEBUG;_RELEASE;WIN32;_WINDOWS;VERSION_D3D;TEX_EMBED;FINAL`
- `TARGET_DC` — 1054 вхождений, 165 файлов; `PSX` — 1739 вхождений, 178 файлов
- `BIKE` — нигде не определён → 453 вхождения никогда не компилируются
- Региональный активный: только `EIDOS`
