# DENSE_SUMMARY часть 3

Продолжение [DENSE_SUMMARY_2.md](DENSE_SUMMARY_2.md). Секции 16–23.

---

## 16. Звук

- Движок: **Miles Sound System** (mss32.lib/dll) — проприетарный
- MFX API: `MFX_play(channel, wave_id, flags)`, `MFX_play_xyz(..., x>>8, y>>8, z>>8)`; громкость **0..127**
- Каналы: Thing=свой канал; спец: WIND_REF=MAX_THINGS+**100**, WEATHER=+**101**, MUSIC=+**102**
- Флаги: MFX_REPLACE, QUEUED, OVERLAP, LOOPED, SHORT_QUEUE, WAVE_ALL
- **14 MUSIC_MODE_***: NONE, CRAWL, DRIVING1/2, DRIVING_START, FIGHT1/2, SPRINT1/2, AMBIENT, CINEMATIC, VICTORY, FAIL, MENU
- Приоритет: CINEMATIC > FIGHT > DRIVING > SPRINT > CRAWL > AMBIENT
- Файлы: `data/sfx/1622/musik01-08/` с подпапками per action
- **5 ambient биомов** по texture_set: Jungle(**1**), Snow(**5**: волки каждые **1500+** кадров), Estate(**10**: самолёты **500+**), BusyCity(default), QuietCity(**16**)
- Indoor ambient: office/police/posheeta/club по WARE_ware[].ambience
- Погода: дождь `gain=255-(above_ground>>4)`, ветер `gain=abs_height>>4`; ⚠️ ночью **всегда** дождь
- Шаги: по PAP_Hi.Texture; `FLAGS_PLAYED_FOOTSTEP` бит **12**
- Config: ambient/music/fx_volume=**127** (0..127)

---

## 17. UI и Frontend

### HUD
- Pipeline: OVERLAY_handle → PANEL_start → buffered → gun sights → enemy HP → PANEL_last → inventory → PANEL_finish
- Base: PC X=**0**, Y=**480**
- Здоровье: круговой, **(+74,-90)**, R=**66**, **8** сегментов; Darci hp/**200**, Roper hp/**400**, Vehicle hp/**300**
- Стамина: **(+107,-36)**, **5** квадратов **10×10**; stamina/**25** → 5 цветов
- Радар: R=**50**, стрелки маяков + точки врагов
- Gun sights: MAX_TRACK=**4**; внешние R=**164**, внутренние R=**20+accuracy/4**; цвет зелёный→красный
- Enemy HP bar: PC **54×4px**, DC **54×8px**
- Шрифты: MenuFont (пропорциональный, **10** эффектов), Font2D (моноширинный, h=**16**)

### Frontend
- Pipeline: game_attract_mode → FRONTEND_loop → mode≥100=брифинг → FE_START → GS_PLAY_GAME
- **14 режимов** FE_*: MAINMENU(1)..CREDITS(-5); стек **10** уровней
- **4** сезонных темы: leaves/rain/snow/blood по complete_point (<8/16/24/≥24)
- ⚠️ DONT_load жёстко **=0** — bitmask не работает, грузится всё
- VIOLENCE=FALSE для: FTutor1, Gangorder1, Album1

### Мёртвый код UI
- track_enemy (OLD_POO), help_system (UNUSED), DAMAGE_TEXT, level_won/lost (#if 0), attract demo (закомментирован)

---

## 18. Прогресс и сохранения

- Файл: `saves/slot{N}.wag`, N=**1,2,3** (⚠️ 1-based!)
- Version **3**, layout: mission_name + CRLF + complete_point(UBYTE) + version(UBYTE) + Darci stats(4) + Roper stats(4) + DarciDeadCivWarnings(1) + mission_hierarchy(**60**) + best_found(**50×4=200**)
- ⚠️ version после complete_point — "Historical Reasons"
- complete_point: UBYTE, max ~**40**, пороги тем: <8/16/24/≥24
- mission_hierarchy[60]: bit0=exists, bit1=complete, bit2=available
- best_found[50][4]: анти-фарм — начисляется только разница с рекордом
- ⚠️ Баг memset: `memset(best_found, 50*4, 0)` — size и value перепутаны (работает случайно)
- Config.ini: `max_frame_rate=30`, `draw_distance=22`, `video_res` (нужно **4** для стабильной работы)

---

## 19. Загрузка уровня

### Файлы: .ucm(миссия), .iam(карта), .lgt(свет), .txt(диалоги), .prm(меши)
### 9 шагов:
1. ELEV_load_name: .ucm → 4 имени файлов
2. ELEV_game_init: **15** подсистем init
3. load_game_map: PAP_Hi **128×128×6**, OB, super map, texture_set(**0-21**)
4. Постобработка: **18** вызовов; ⚠️ **MAV_precalculate — самый тяжёлый**
5. Освещение: дефолт `NIGHT_ambient(255,255,255, 110,-148,-177)`; ⚠️ ночь → дождь (хардкод)
6. Эвент-вэйпойнты: GAME_TURN=0, **MAX_EVENTPOINTS=512** (14+58 байт каждый)
7. Финальная обработка: нормали, текстуры; ⚠️ **EWAY_process() → создание игрока и NPC**
8. Камера: FC_look_at + FC_setup_initial_camera
9. Спецэффекты: PUDDLE_precalculate (если дождь)

- ⚠️ Игрок создаётся через EWAY (WPT_CREATE_PLAYER → PCOM_create_person), не напрямую
- Кеш: **128** элементов

---

## 20. Межсистемные зависимости

1. ⚠️ **Рендерер мутирует game state** — DRAWXTRA_MIB_destruct() изменяет ammo_packs_pistol, создаёт PYRO/SPARK. PYRO_draw_armageddon() создаёт объекты из рендерера. Нарушение слоистости.
2. ⚠️ **Камера зависит от MAV** — FC_process использует MAV_inside для коллизии камеры с геометрией
3. ⚠️ **Двери синхронизированы с MAV** — MAV_turn_movement_on/off при открытии/закрытии; если рассинхронизация → NPC застрянут
4. ⚠️ **EWAY контролирует спавн** — игрок создаётся через EWAY_process, НЕ напрямую
5. ⚠️ **AI таймер независим от FPS** — PCOM_TICKS_PER_SEC=320, не привязан к рендер-FPS
6. ⚠️ **Анимации управляют физикой** — GameFightCol в кадрах анимации определяет зоны попадания; gravity_anim_driven у персонажей
7. ⚠️ **MAV_precalculate блокирует загрузку** — самый тяжёлый шаг, нельзя async без изменения архитектуры
8. ⚠️ **MapWho синхронизация** — move_thing_on_map() обязателен при перемещении; без него AI не видит объекты
9. ⚠️ **EWAY_process каждый кадр** — миссионная логика встроена в game loop, не отделена от рендеринга
10. ⚠️ **Вода = коллизионные стены** — PAP_FLAG_WATER создаёт невидимые барьеры, нет физической среды (плавания, замедления)
11. ⚠️ **MAV_turn_movement_on теряет caps** — заменяет ВСЕ действия на GOTO; если дверь открывалась у прыжковой точки → JUMP/PULLUP потеряны навсегда

---

## 21. Пре-релизные баги и особенности

1. **INSIDE2_mav_inside = ASSERT(0)** — NPC не может ходить внутри зданий
2. **INSIDE2_mav_nav_calc в #if 0** — nav-сетка зданий не строится
3. **INSIDE2_mav_stair/exit**: `>16` вместо `>>16` — возвращают текущую позицию
4. **INSIDE2 Z-цикл**: `MinX..MaxX` вместо `MinZ..MaxZ`
5. **HM_collide_all**: `already_bumped[i]` вместо `[j]` — пропуск коллизий
6. **CIV воскрешение**: newpos НЕ присваивается WorldPos → на месте смерти
7. **Собаки (canid)**: dispatch закомментирован → полностью инертны
8. **mount_ladder()**: закомментирован в interfac.cpp → игрок не может залезть снизу
9. **valid_grab_angle()**: ВСЕГДА возвращает 1 — валидация отключена
10. **Crinkle**: CRINKLE_load()→return NULL → bump mapping отключён (работает в финале!)
11. **Roper stats**: EWAY_create_player(ROPER) использует Darci-статы (блок закомментирован)
12. **vcxproj /MTd в Release**: MultiThreadedDebug автоматически определяет _DEBUG → debug-код в Release
13. **NIGHT dprod баг**: `dz*nx` вместо `dz*nz` в расчёте освещения
14. **Обнаружение обрыва (idle)**: `&& 0` на строке 180 fc.cpp → мёртвый код
15. **best_found memset**: size и value перепутаны (работает случайно т.к. value=0)
16. **MAV_turn_movement_on**: заменяет ВСЕ caps на GOTO (потеря jump/pullup/climb)
17. **Взрывы стен не обновляют MAV** — MAV_precalculate только при загрузке
18. **CLOTH_process в #if 0** — симуляция ткани отключена
19. **Font3D в #if 0** — 3D шрифты отключены
20. **OVERLAY_draw_tracked_enemies в #ifdef OLD_POO** — старая HUD система отключена
21. **WPT_GOTHERE_DOTHIS(39) = ASSERT(0)** — NPC scripting не реализован
22. **WPT_BONUS_POINTS(32)**: `if(0)` → очки не начисляются
23. **EWAY_COND PRESSURE/CAMERA**: стабы, always FALSE
24. **NIGHT_generate_walkable_lighting**: return сразу после roof_walkable → мёртвый код
25. **plant_feet**: fx=0, fz=0 — X/Z смещение ноги игнорируется

---

## 22. Форматы файлов

| Формат | Назначение | Ключевые данные |
|--------|-----------|----------------|
| **.ALL** | Анимации+меши | Vertex morphing, checksum **0x29A(666)×5**, GameKeyFrameElement=CMatrix33+offsets, **15** body parts, **4** AnimType×**450** слотов |
| **.WAV** | Аудио | PCM 16-bit, 22050/44100 Hz, MSS32; `data/sfx/1622/`, `talk2/`, `musik01-08/` |
| **.LGT** | Освещение | ED_Light **20 байт**, NIGHT_Colour **3 байта** (0-63), ambient dir **(110,-148,-177)** |
| **.IAM** | Карта | PAP_Hi **128×128×6**, save_type **18-28**, LoadGameThing **44 байта**, texture_set **0-21** |
| **.STY** | Миссии | Текстовый, max **20KB**, **28** районов, suggest_order[**36**] захардкожен |
| **.IMP** | 3D модели | version=**1**, vertex=float xyz+uv, face=UWORD indices |
| **.MOJ** | Иерархич. объекты | PrimPoint **6 байт** (SWORD), PrimFace3 **28 байт**, PrimFace4 **34 байта** |
| **.PRM** | Статич. примы | START_SAVE_TYPE=**5793**, PrimObject **16 байт**, ~**265** типов |
| **.TGA** | Текстуры | 32/24-bit RGBA/RGB, степени двойки (32-512), mipmap при загрузке |
| **.TXC** | Архивы текстур | FileClump: MaxID+offsets+data; ⚠️ sizeof(size_t) проблема 32/64-bit |
| **.TMA** | Тайл-таблицы | style.tma **200×5** стилей, TXTY **4 байта**; страницы: World 0-255, Shared 256-511, Interior 512-575, People 576-703, Prims 704-1151, People2 1152-1407, Effects 1408-1567, Crinkle 1568-1759 |
| **.UCM** | Миссия (бинарный) | Header **~1316 байт** + EP **512×74=37888** + SkillLevels + messages |
| **.WAG** | Сохранение | version **3**, mission_name+stats+hierarchy(**60**)+best_found(**200**), ~**271+** байт |

---

## 23. Быстрые числа

### Математика
- PSX углы: **0-2047** (2048=360°); PC: float радианы
- SinTable: **2560** entries, CosTable=SinTable+**512**; масштаб **65536**=1.0
- AtanTable: **256** entries, 0-90° → 0-512; Arctan возвращает **0-2047**
- Root() = обёртка над sqrt()

### Глобальные лимиты
| Ресурс | PC | PSX | DC |
|--------|-----|-----|-----|
| Things (primary+secondary) | **400+300=700** | **400+50=450** | — |
| Particles | **2048** | **64** | — |
| POW Sprites | **256** | **192** | **128** |
| PYRO | **64** | **32** | — |
| DIRT | **1024** | **128** | **256** |
| CollisionVect | **10000** | **1000** | — |
| WalkLink | **30000** | **10000** | — |
| MAV_opt | **1024** | — | — |
| Vehicles | **40** | — | — |
| Buildings | **500** | — | — |
| DFacets | **16384** | — | — |
| EventPoints | **512** | — | — |
| Specials | **260** | — | — |
| RMAX_PEOPLE | **180** | — | — |
| Fire flames/fires | **256/8** | — | — |
| Sparks | **32** | — | — |
| Ribbons | **64** | — | — |
| Bang/Phwoar | **64/4096** | — | — |
| Barrels | **300** | — | — |
| WMOVE faces | **192** | — | — |
| Crinkle pts/faces | **8192/8192** | — | — |
| Stair | **32** | — | — |
| Doors (animated) | **4** | — | — |
| WATER points/faces | **1024/512** | **102** | — |
| Trip wires | **32** | — | — |
| SM spheres/links/objects | **1024/1024/16** | — | — |
| Balloons | **32** | — | — |
| Grenades | **6** | — | — |

### Minor Systems
- **Water**: WATER_gush strength=`QDIST2(dx,dz)<<2`; ⚠️ wibble анимация закомментирована
- **Trip**: point-to-line dist<**0x20**, проверяет 3 точки тела, debounce **4** кадра
- **SM**: soft-body, `SM_GRAVITY=-0x200`, jellyness **0x10000..0x00400**, bounce потеря **11.7%**
- **Balloon**: ⚠️ ВЫРЕЗАНЫ (load закомментировано), buoyancy `dy+=0x40`, damping `dx-=dx/0x10`
- **Tracks**: circular buffer декалей; кровь размер **1-31** (bleed), **80-95** (pool); ⚠️ отключаются при !VIOLENCE
- **Spark**: lifespan `rand()%max_life+8`, recursive midpoint для ветвления
- **Command**: LEGACY, заменён PCOM; только COM_PATROL_WAYPOINT + **4/14** условий работают
- ⚠️ **drawxtra.cpp**: DRAWXTRA_MIB_destruct + PYRO_draw_armageddon мутируют game state из рендерера
