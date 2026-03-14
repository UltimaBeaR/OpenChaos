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
