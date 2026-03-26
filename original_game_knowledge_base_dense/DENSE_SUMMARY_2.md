# DENSE_SUMMARY часть 2

Продолжение [DENSE_SUMMARY.md](DENSE_SUMMARY.md). Секции 7–15.

---

## 7. Транспорт

- **9 типов**: VAN(0), CAR(1), TAXI(2), POLICE(3), AMBULANCE(4), JEEP(5), MEATWAGON(6), SEDAN(7), WILDCATVAN(8)
- **MAX_VEHICLES=40**, Health=**300** (взрыв при 0), **6 зон повреждения** (0-4 уровня)
- Двигатели (accel_fwd/rev/soft_brake/hard_brake): LGV **17/10/4/8**, CAR **21/10/4/8**, PIG **25/15/5/10**, AMB **25/10/5/8**
- **VEH_SPEED_LIMIT=750**, **VEH_REVERSE_SPEED=300**
- `UNITS_PER_METER=128`, `TICKS_PER_SECOND=80`
- Подвеска: 4 пружины, MIN/MAX compression **0x0D00/0x7300**, rest ≈ **4300**; Tilt/Roll clamp ±**312** (~27.5°)
- Руление: **WHEELTIME=35** кадров, **WHEELRATIO=45**, steering **-64..+64**
- Занос: **SKID_FORCE=8500**, **SKID_START=3** кадра
- Friction: `new_vel = ((vel<<friction)-vel)>>friction`; base friction=**7**
- Переезд: `damage = |VelX*tx+VelZ*tz|/(distance*200)`, min **10 HP**
- Дым: Health<**128**, шанс = `0x7f-Health`
- BIKE (вырезан): **MAX=2**, **453** вхождения, не компилируется

---

## 8. Управление и ввод

- **MAX_PLAYERS=2**, **18** INPUT_* кнопок (0-17), `INPUT_MASK_ALL=0x3ffff`
- Аналог: `GET_JOYX(input)=((input>>17)&0xfe)-128`; Y аналогично биты 24-31
- Double-click: порог **200мс**, **16** слотов
- Типы: PLAYER_DARCI(1), ROPER(2), COP(3), THUG(4)
- Stamina/Constitution/Strength/Skill — UBYTE; **RedMarks=10 → game over**
- **5 режимов**: RUN(0), WALK(1), SNEAK(2), FIGHT(3), SPRINT(4)
- PSX: **4 конфига**, аналог порог **±96/128** (~75%), DualShock ID=**7**. В пре-релизных исходниках аналог только эмулирует D-Pad; в финале PS1 — плавное управление скоростью (шаг↔бег), D-Pad отключается в аналоговом режиме
- PSX вибрация: малый `>>=1` (быстро), большой `*7>>3` (медленно)

### STATE_* (ключевые)
- INIT(0), NORMAL(1), MOVEING(5), IDLE(6), JUMPING(8), FIGHTING(9), FALLING(10), DOWN(12), HIT(13), DYING(16), DEAD(17), DANGLING(18), CLIMBING(21), GUN(22), SHOOT(23), DRIVING(24), GRAPPLING(30), CANNING(32), HUG_WALL(34), CARRY(36)
- ⚠️ Пропуски: нет 3, 4, 15, 20, 27-29, 31

### FLAG_PERSON_* (ключевые биты)
- NON_INT_M(0), NON_INT_C(1), GUN_OUT(3), DRIVING(4), SLIDING(5), ON_CABLE(12), GRAPPLING(13), HELPLESS(14), CANNING(15), SPRINTING(22), FELON(23), ARRESTED(26), WAREHOUSE(30), KILL_WITH_A_PURPOSE(31)

---

## 9. Бой и оружие

### Melee
- GameFightCol: Dist1/Dist2(UWORD), Angle(UBYTE 1/256), Height(0=ноги,1=торс,2=голова), Damage(UBYTE)
- Hit check: QDIST2(XZ), abs(dy)>**100**=промах, допуск ±**200** (FIGHT_ANGLE_RANGE=**400**), LOS, dist<Dist2+**0x30**
- Stomp: dist<**0x53** к 3 точкам тела
- Урон: `damage = fight->Damage + модификаторы`; игрок-жертва `>>=1`; min **2**
- Комбо: Punch **10→30→60**(KO), Kick **10→30→60**, Knife **30→60→80**, Bat **60→90**
- Блок: `block_prob = 60 + SKILL*12`; ⚠️ огнестрел **игнорирует блок**
- ⚠️ MIB **не нокаутируются** (кроме FIGHT_TEST)
- Граплинг: 4 приёма (pistol whip dist=**75**, strangle=**50**, headbutt=**65**, grab arm=**60**)

### Огнестрельное
| Оружие | Дальность | Разброс | Урон | Скорострельность |
|--------|-----------|---------|------|------------------|
| Пистолет | **1792** | **15** | **70** | **140** тиков |
| Дробовик | **1536** | **30** | **300-dist** | **400** |
| AK47 (игрок) | **2048** | **20** | **100** | **64** |
| AK47 (NPC) | **2048** | **20** | **40** | **64** |
| MIB | **2048** | **20** | — | **64** |

- ⚠️ Все guns vs игрок: `damage>>=1`
- Hit chance: `base=230-|Roll|/2-Velocity+64(aim)-64(AI→player)+100(AI→AI)-dist/8; min 20/256 (~8%)`
- Автоприцел: конус **±~51°**, при беге **±~13°**; score = `(8<<8-dist)*(MAX_DANGLE-dangle)>>3`

### Взрывы
| Тип | Радиус | Макс. урон |
|-----|--------|-----------|
| Mine | **0x200** | **250** |
| Граната | **0x300** | **500** |
| C4 | **0x500** | **500** |
- Граната: таймер **1920** тиков (**6с**); C4: **1600** тиков (**5с**, ⚠️ не 10 как в комментарии)
- `hitpoints = maxdamage*(radius-dist)/radius`; knockdown если >**30**

### Предметы
- **30** типов SPECIAL_*, **6** групп; макс **260** одновременно
- Боезапас: пистолет **15**, дробовик **8**, AK47 **30**, гранаты стак **8**, C4 стак **4**
- ⚠️ `SPECIAL_THERMODROID(11)` — нефункционален; `SPECIAL_MINE(10)` — нельзя подобрать

### Фракции
- DARCI: не бьёт копов/Roper; THUGS: бьют другие цвета; MIBs: не бьют других MIB; Gang attack: до **8** атакующих с **8** направлений

---

## 10. Взаимодействие

- `find_grab_face()`: двухпроходный (hi-res крыши, lo-res floor), Y-диапазон ±**256+dy**, угол ±**200°**
- Возвращает type: **0**=поверхность, **1**=cable, **2**=ladder
- ⚠️ `valid_grab_angle()` **ВСЕГДА возвращает 1** — валидация отключена
- Cable: параметры **переиспользуют поля DFacet** (StyleIndex=кривизна1, Building=кривизна2, Height=шаги)
- Ladder: высотный диапазон bottom..top+**64**; ⚠️ `mount_ladder()` **закомментирован** — игрок не может снизу; AI может

---

## 11. Миссии EWAY

- **MAX_EVENTPOINTS=512**, EventPoint=**74 байта** (14 header + 58 data)
- `EWAY_process()` — **каждый кадр**; PSX: чётные/нечётные (step=2)
- `EWAY_time += 80*TICK_RATIO>>TICK_SHIFT` (~**1000** ед/сек)
- **42 условия** (COND_*): FALSE(0)..AFTER_FIRST_GAMETURN(41); ⚠️ PRESSURE(4), CAMERA(5) = **STUB**
- **57 действий** (WPT_*): CREATE_PLAYER(2)..SHAKE_CAMERA(57); ⚠️ GOTHERE_DOTHIS(39)=**ASSERT(0)**
- STAY режимы: ALWAYS(одноразовый), WHILE(re-poll), WHILE_TIME, TIME, DIE
- Zone byte: биты 0-3=zone, бит 4=INVULNERABLE, бит 5=GUILTY, бит 6=FAKE_WANDER
- CRIME_RATE: загружается из .iam (дефолт **50**); убийство guilty **-2**, civ **+5**, арест **-4**
- ⚠️ `EWAY_counter[7]` инкрементируется при убийстве COP
- ⚠️ Roper stats bug: EWAY_create_player(ROPER) использует Darci-статы (Roper закомментирован)

---

## 12. Здания и интерьеры

### Иерархия
- FBuilding(**500**) → FStorey(**2500**) → FWall(**15000**) → FWindow(**50** типов)
- Компиляция при загрузке → DBuilding(**1024**) → DFacet(**16384**)

### 21 STOREY_TYPE_*
- NORMAL(1), ROOF(2), WALL(3), ROOF_QUAD(4), FLOOR_POINTS(5), FIRE_ESCAPE(6), STAIRCASE(7), SKYLIGHT(8), ⚠️**CABLE(9)—есть в финале**, FENCE(10), FENCE_BRICK(11), LADDER(12), FENCE_FLAT(13), TRENCH(14), JUST_COLLISION(15), PARTITION(16), INSIDE(17), DOOR(18), INSIDE_DOOR(19), OINSIDE(20), OUTSIDE_DOOR(21)

### FACET_FLAG_*
- CABLE(2), INSIDE(3), DLIT(4), HUG_FLOOR(5), ELECTRIFIED(6), 2SIDED(7), UNCLIMBABLE(8), BARB_TOP(10), SEETHROUGH(11), OPEN(12), 90DEGREE(13), FENCE_CUT(15)
- ⚠️ IN_SEWERS(4) **совпадает** с DLIT(4)

### Двери: **MAX=4** одновременно, Open 0→**255**(180°) или →**128**(90°), шаг **+4/кадр**, синхронизированы с MAV

### Лестницы: **MAX=32**, **3** на этаж; scoring: outside==2 → **+0x10000**, opposite wall → **+0xbabe**, блокирует дверь → **-∞**

### Интерьеры
- Inside/outside определяется по STOREY_TYPE (17/19/20 vs 1-16/21)
- RoomID: до **16** комнат, **10** лестниц
- `ID_generate_floorplan`: **32** сида, стены = `floor_area >> 4 + 1`
- WARE: max **32** склада, **4** двери/склад, навигация через swap MAV_nav

### Фасады: `FLAG_WALL_AUTO_WINDOWS`, окна = `dist/(BLOCK_SIZE*4)`, **BLOCK_SIZE=64**
- ⚠️ **Здания неуязвимы!** Shake/CutHole только для заборов

---

## 13. Эффекты

- **Частицы**: MAX **2048**(PC)/**64**(PSX); GetTickCount() delta-time
- **Огонь**: MAX flames **256**, fires **8**; die=**32+(rand&0x1f)**; points=(height>>6)+1 clamp 2-4
- **Искры**: MAX **32**, 4 точки/искра; типы LIMB/CIRCULAR/GROUND/POINT; электрозаборы: каждые **32** кадра
- **Ткань**: ⚠️ CLOTH_process в **#if 0** — мёртвый код
- **Effect**: DT_EFFECT, speed затухание **78%/кадр**
- **Капли**: MAX **1024**, fade **-16/кадр**, size **+4/кадр**
- **Дымка/туман**: MAX points **4096**, mist **8**; drag **25%**, spring **0.5%**
- **Лужи**: PC only, предвычисляются; PAP_FLAG_REFLECTIVE
- **Тени**: статические, солнце **(+147,-148,-147)**, ray-cast dist=**32**; ⚠️ **динамических нет** — flat sprite
- **Ленты**: MAX **64**, **32** точек; CONVECT дрейф **+22/кадр**; IMMOLATE **2+3** ленты
- **Взрывы (bang)**: Bang[**64**], Phwoar[**4096**]; 4-уровневый каскад: BIG→MIDDLE(6)→NEARLY→END
- **POW**: Sprite[**256**], Pow[**32**]; **8** типов; **16** кадров; ⚠️ overflow → POW_init() полный сброс
- **PYRO**: MAX **64**(PC)/**32**(PSX); **18** типов; IMMOLATE **-10HP/кадр**; GAMEOVER **8** dlights → GS_LEVEL_LOST при counter>**242**
- **DIRT**: MAX **1024**(PC); **16** типов; ⚠️ PIGEON prob=**0** (никогда); деревья **64**, **200** листьев/дерево
- DIRT банки: бросок → `PCOM_SOUND_UNUSUAL` → NPC идут проверять (тактика)

---

## 14. Камера

- **Только FC** (fc.cpp); cam.cpp мёртв (#ifdef DOG_POO). **FC_MAX_CAMS=2**, **CAM_MORE_IN=0.75F**
- Режимы: type0 dist=**480** h=**0x16000**; type1 **640/0x20000**; type2 **896/0x25000**; type3 **768/0x8000**
- PSX type0: dist=**0x300**, h=**0x18000**
- FC_process **15 шагов**: alter_for_pos → calc_focus → warehouse transition → lookabove → rotate(decay ±**0x80**) → toonear cancel(dist>toonear+**0x4000** AND dangle>**200**) → collision(**8** шагов, force<<4) → get-behind(>>3 normal, >>5 driving) → adjust Y(dy>>3, cap **[-0x0c00,+0x0d00]**) → distance clamp → position smoothing(|delta|>**0x800**: x+=dx>>2) → look_at → angle smoothing(|delta|>**0x180**: +=delta>>2) → lens(**0x28000×0.75**, фикс) → shake(decay: `-1-(shake>>3)`)
- Toonear: `dist=0x90000` → first-person mode
- ⚠️ Обнаружение обрыва (idle) — мёртвый код (`&& 0`)

---

## 15. Рендеринг

- **16 DT_*** типов (NONE..BIKE)
- Pipeline: POLY_frame_init → трансформация+накопление → POLY_frame_draw (Z-sort → DrawPrimitive)
- **POLY_NUM_PAGES=1508**, **MAX_VERTICES=32000**, **POLY_ZCLIP_PLANE=1/64**
- Bucket sort: **2048** buckets, sort_z=reciprocal depth, рендер far→near
- Текстуры: **1408** базовых (22×64), TGA из Debug/Textures/; TEX_EMBED=1 всегда
- Gamut: NGAMUT хранит видимый X-диапазон per Z-строке (coarse culling)
- Tom's Engine: `USE_TOMS_ENGINE_PLEASE_BOB=1`; `DRAW_WHOLE_PERSON_AT_ONCE=1` — batch **15** body parts, DrawIndPrimMM; ⚠️ "NOT portable"
- NIGHT: **256** static + **64** dynamic lights; 6-bit каналы (0-63) → ×4 → D3D
- ⚠️ Баг NIGHT: `dz*nx` вместо `dz*nz` в dprod расчёте
- Crinkle: micro-geometry bump; **256** crinkles, **8192** points/faces, **700** pts/crinkle; ⚠️ **ОТКЛЮЧЕНЫ** в пре-релизе (`CRINKLE_load()→return NULL`), **работают в финале**
- CMatrix33: 3 SLONG, 3×10 бит, scale **128**
- Деформация машин: **5** уровней × **8** вариантов × **6** точек (таблица)
- ⚠️ **Game logic в рендерере**: DRAWXTRA_MIB_destruct мутирует ammo/PYRO/SPARK; PYRO_draw_armageddon создаёт объекты
- ⚠️ MapElement.Colour — мёртвый код в DDEngine (только Glide)
- Fog: outdoor=NIGHT_sky_colour, indoor/sewers=**0** (чёрный)

---

→ Продолжение: [DENSE_SUMMARY_3.md](DENSE_SUMMARY_3.md)
