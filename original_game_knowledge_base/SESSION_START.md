# SESSION START — Urban Chaos KB Быстрый Ориентир

**Правило старта сессии:** Читать ТОЛЬКО этот файл + CLAUDE.md. Остальные файлы KB читать точечно по мере необходимости.
**Если задача касается подсистемы X → читать subsystems/X.md перед работой.**

---

## ⚠️ ВАЖНОЕ ИЗМЕНЕНИЕ ПРАВИЛ (2026-03-13)

**Переносим фичи из ОБЕИХ релизных версий: PC + PS1.** Не только PS1.
- Если фича есть в PC но нет в PS1 — переносим (пример: лужи, bump mapping)
- Если конфликт реализаций — берём PC версию
- **TODO:** пересмотреть ВСЕ решения "не переносить" — некоторые могли быть приняты ошибочно (фичи нет в PS1, но есть в PC)
- Подтверждённые PC-only фичи для переноса: **лужи** (puddle.cpp), **bump mapping/Crinkle** (на ящиках)

---

## ⚡ СЛЕДУЮЩАЯ ИТЕРАЦИЯ

**План:** 8 итераций глубокого аудита и верификации KB перед переходом к фазе 2.
**Детали:** см. `ANALYSIS_ITERATIONS_PLAN.md`
**Текущий статус:** план согласован, ожидается команда на старт итерации 1 (очистка KB)

**Перекрёстная верификация ВЫПОЛНЕНА — результаты:**

**A. Протриажены ~180 неаннотированных файлов:**
- ~18 файлов содержат АКТИВНУЮ игровую логику (ранее не документированы)
- ~15 файлов MIXED (платформо-зависимые но с активным кодом)
- Остальные = editor/PSX/rendering (подтверждено)

**B. Новые системы обнаружены и задокументированы (minor_systems.md):**
- Water.cpp (747 строк) — динамическая водная поверхность (НЕ "просто стены")
- trip.cpp (361 строк) — растяжки/ловушки (АКТИВНАЯ геймплей-механика)
- sm.cpp (502 строк) — sphere-matter soft body physics
- Balloon.cpp (576 строк) — воздушные шары (PC only)
- tracks.cpp (378 строк) — следы шин/крови/отпечатки ног
- Spark.cpp (1326 строк) — электрические искры/заборы
- Command.cpp (520 строк) — legacy NPC commands (МЁРТВЫЙ КОД, только COM_PATROL_WAYPOINT)
- drawxtra.cpp — КРИТИЧНО: game logic в рендерере (MIB destruct мутирует ammo)

**C. Перекрёстная верификация аннотаций vs KB (Combat/Special/Vehicle):**
- combat.md: +10 фактов (AI block cap, kick NAD по полу, full damage table, FIGHT_TEST exception)
- weapons_items.md: +13 категорий (damage values, fire rates, hit chance formula, explosion radii, treasure stats)
- vehicles.md: +18 фактов (friction formula, damping 15/16, runover damage, tilt clamp)
- characters.md: +Darci physics (fall damage формула, death plunge, velocity scaling)
- rendering.md: +warning о game logic в drawxtra.cpp

**D. Ключевые открытия:**
- Darci fall damage: `(-DY - 20000) / 100`, death plunge player=-12000 / NPC=-6000
- Pistol=70dmg, Shotgun=300-dist, AK47=100(player)/40(NPC); fire rates: 140/400/64 ticks
- Hit chance: `230 - abs(Roll)>>1 - Velocity + modifiers`; min 20/256 (~8%)
- Vehicle friction: bit-shift based `(1 - 1/2^friction)`, terminal velocity emergent
- DRAWXTRA_MIB_destruct() МУТИРУЕТ game state из рендерера → вынести при портировании

**ВЫПОЛНЕНО в этой итерации (ribbon + bang + interact + triage):**
- ribbon.cpp полный анализ (144 строки): circular buffer trail renderer для огня/дыма
- Ribbon: MAX_RIBBONS=64, MAX_RIBBON_SIZE=32; CONVECT(+22 Y/frame), FADE, SLIDE, IALPHA
- RIBBON_alloc circular scan, 1-based ID (0=fail); RIBBON_extend circular buffer Head/Tail
- RIBBON_draw_ribbon() в drawxtra.cpp:2536 — triangle strip рендерер
- Используется в: PYRO(IMMOLATE/FLICKER/FLAMER), drawxtra(CHOPPER/FIREWORK), bike(dead)
- bang.cpp полный анализ: иерархическая визуальная система взрывов (НЕ урон)
- BANG_Bang[64] + BANG_Phwoar[4096]; 4 каскадных типа: BIG→MIDDLE→NEARLY→END
- Phwoar: direction norm=64, radius+=grow>>2+1, Y drift +=counter<<1; child[6] spawn schedule
- interact.cpp полный анализ (~900 строк): grab/ladder/cable система — НОВАЯ ВАЖНАЯ ИНФО
- find_grab_face(): 2-pass search (hi-res roof / lo-res walkable), 4 edges per quad, fence check
- Cable params PACKED in DFacet: StyleIndex=angle_step1, Building=angle_step2, Height=count
- find_cable_y_along(): cosine dip curve; check_grab_ladder_facet(): -1=above(falling), 1=valid
- valid_grab_angle() = ALWAYS 1 (disabled); find_grab_face_in_sewers = PC-only (НЕ переносить)
- calc_sub_objects_position(): bone world pos during animation (tween 0-255)
- interaction_system.md создан: полное описание grab/ladder/cable механик
- Триаж ~188 неаннотированных файлов: большинство = редактор/PSX/рендеринг
- plat.cpp: moving platforms — АКТИВНЫЙ КОД, waypoint paths, GOTO/PAUSE/STOP, LOCK_X/Y/Z
- chopper.cpp: вертолёт-враг — АКТИВНЫЙ КОД, CHOPPER_CIVILIAN, detection radii
- Pjectile.cpp: generic projectile pool — минимальный wrapper (3 функции)
- Switch.cpp: proximity triggers — ПОЛУМЁРТВЫЙ (sphere detect ок, group/class = стабы)
- lead.cpp: верёвка/поводок — НЕДОДЕЛАН; nd.cpp = ПУСТОЙ; morph.cpp = НЕ используется
- effects.md: добавлены разделы 11 (RIBBON) и 11a (BANG) с полными деталями + таблица переноса
- game_objects.md: добавлены разделы 11 (Moving Platforms), 12 (Helicopter), 13 (мелкие системы триаж)
- Аннотированы: ribbon.cpp, bang.cpp, interact.cpp, plat.cpp, chopper.cpp, Pjectile.cpp (header блоки)

**ВЫПОЛНЕНО в этой итерации (pow + pyro + dirt):**
- pow.cpp полный анализ (973 строки): sprite-based visual explosions, 8 типов, cascade spawning
- POW_Sprite[256] 16-frame animation, POW_Pow[32] containers; FMATRIX_vector sphere generation
- POW_process: animate→move(dx<<4)→damp(dx-=dx>>damp)→remove dead(frame>=16)→spawn children
- Safety: count guards 50/256, POW_init() full reset on overflow
- pyro.cpp полный анализ (1349 строк): 18 типов PYRO, Thing(CLASS_PYRO) state machine
- IMMOLATE: -10HP/frame, RIBBON fire, particles on bones; GAMEOVER: 8 dlights + GS_LEVEL_LOST
- PYRO_construct bitmask API; PYRO_blast_radius→IMMOLATE на каждого в сфере; HITSPANG max 5
- MergeSoundFX: reuses sound from nearby same-type PYRO (sphere 5*256)
- dirt.cpp полный анализ (3149 строк): 16 типов debris, ambient+interactive
- PIGEON отключён (prob=0 hardcoded); FOREST→LEAF, SNOW→SNOW; trees[64]×200 leaves
- Can/Head bounce physics, THROWCAN→CAN conversion, S_KICK_CAN+PCOM_SOUND_UNUSUAL
- DIRT_gust(strength=QDIST2*8), DIRT_create_cans(5), DIRT_create_brass(prim 253 PC)
- effects.md обновлён: 3 новых раздела (POW, PYRO, DIRT) с полными деталями
- Аннотированы: pow.cpp (header блок), pyro.cpp (header блок), dirt.cpp (header блок)

**ВЫПОЛНЕНО в этой итерации (barrel + wallhug):**
- barrel.cpp полный анализ (1937 строк, 14 функций) + аннотация header блок
- Barrel: 2-sphere rigid body, gravity=0x80, damping/32, MAV wall collision, standing-up correction
- 4 типа: NORMAL(взрыв), CONE(малый), BURNING(огонь), BIN(мусор+банки)
- BARREL_alloc: recycling ближайшего бареля при нехватке Thing; стакинг через THING_find_box
- BARREL_shoot: PYRO_FIREBOMB + shockwave(0x200,250) + PCOM_oscillate_tympanum для NORMAL/BURNING
- МЁРТВЫЙ КОД: position_on_hands+throw (подбор/бросок), cone penalty, старые particle эффекты
- game_objects.md: новый раздел 10 (Barrel System) с полными деталями
- Wallhug.cpp полный анализ (644 строки, 11 функций) + аннотация header блок
- Wallhug: 2D grid pathfinding, Bresenham+left/right huggers, LOS cleanup (lookahead=4), max 10 итераций
- navigation.md: раздел "Алгоритм Wallhug" существенно расширен (все 4 release conditions, cleanup pipeline)
- Аудит аннотаций: fire/psystem/Vehicle/Building/mav/sound/door — подтверждено что аннотации на месте (были в списке с прошлых итераций)

**ВЫПОЛНЕНО в этой итерации (overlay + guns + grenade + fc + nav):**
- overlay.cpp полный анализ: OVERLAY_handle per-frame pipeline, PANEL_last HUD layout (circular health, 5 stamina marks, weapon+ammo, radar/beacons, timer MM:SS)
- ui.md раздел 1 полностью переписан: per-frame pipeline, PANEL_last layout table, gun sights per-class, enemy health modes, inventory, dead code list
- МЁРТВЫЙ КОД в overlay.cpp: track_enemy (#ifdef OLD_POO), help_system (#ifdef UNUSED), DAMAGE_TEXT (not defined), beacons, ScoresDraw, crime rate — всё закомментировано
- fc.cpp FC_process аннотация расширена: 15-шаговый алгоритм (collision raycast, get-behind, Y-position, distance clamp, smoothing, shake)
- guns.cpp полный анализ + аннотация: auto-aim scoring (MAX_NEW_TARGET_DANGLE=292≈±51°, running=±13°), модификаторы целей, sphere search
- grenade.cpp полный анализ + аннотация: физика (gravity, bounce, wall reflect), CreateGrenadeExplosion (shockwave radius=0x300), show_grenade_path симуляция
- weapons_items.md: добавлен раздел 8a (auto-aim), раздел 8 переписан (grenade.cpp детали), 8 новых функций в таблицу
- Nav.cpp аннотирован: обёртка над wallhug_tricky(), NAV_Waypoint pool, NAV_wall_in_way FLOOR_HIDDEN check
- Аудит: обнаружено 6 файлов с аннотациями НЕ записанных в SESSION_START (fire, psystem, Vehicle, Building, mav, sound, door)

**ВЫПОЛНЕНО в этой итерации (ScoresDraw + end-of-level round-trip):**
- Полный round-trip победа/поражение задокументирован в game_states.md раздел 5:
  - GAMEMENU wait timer: 64/кадр → порог 2560 (~40 кадров = 1.3с) → показать меню
  - WON-меню = только заголовок X_LEVEL_COMPLETE, нет пунктов → ESC/ENTER → DO_NEXT_LEVEL
  - LOST-меню = Restart/Abandon → SURE → DO_RESTART / DO_CHOOSE_NEW_MISSION
  - exit_game_loop → fadeout → break inner loop → post-loop блок
  - DarciDeadCivWarnings check → cutscenes (park2/Finale1) → switch GAME_STATE
  - GS_LEVEL_WON → FRONTEND_level_won() → FE_SAVESCREEN + m_bGoIntoSaveScreen=TRUE
  - GS_LEVEL_LOST → FRONTEND_level_lost() → FE_MAINMENU + restore previous_mission_launch
- FRONTEND_level_won() детально: hierarchy bit 2, stat deltas best_found анти-фарм, complete_point, FE_SAVESCREEN
- ScoresDraw() детально: 6 полей (killed/arrested/at-large/bonus found/missed/time) + mucky times hash-код
- Mucky times: активна DC таблица (~35 миссий + разработчики); PC таблица в #if 0
- Аннотированы: gamemenu.cpp (header блок), Attract.cpp (ScoresDraw расширенный header), frontend.cpp (level_won/lost)

**ВЫПОЛНЕНО в этой итерации (STARTSCR_mission + Attract.cpp):**
- startscr.cpp в non-EDITOR build = одна строка: `CBYTE STARTSCR_mission[_MAX_PATH]`. Весь остальной код = `#ifdef EDITOR` (старый pre-release mission selector, мёртвый код)
- Полный pipeline frontend→game: FE_MAPSCREEN+ENTER → mode=100+mission_id → ENTER → FE_START → STARTSCR_mission set → STARTS_START → GS_PLAY_GAME → elev.cpp loads level → `*STARTSCR_mission=0`
- `FRONTEND_MissionFilename(urban.sty, i)` = i-я доступная миссия в выбранном районе (hierarchy&4); side effect: `mission_launch=ObjID`
- whattoload[] = хардкод 35 миссий; `DONT_load=0` переопределяет все маски → всё грузится
- `this_level_has_bane = (index==27)` = Finale1.ucm only; `this_level_has_the_balrog` из массива
- `VIOLENCE = FALSE` для FTutor1 (idx 1), и двух поздних туториалов (idx 31/32)
- `is_semtex = (index==20)` = skymiss2.ucm (подозрительное имя, скорее всего ошибка)
- `BRIEFING_select()` в Attract.cpp → `#ifdef OBEY_SCRIPT` → МЁРТВЫЙ КОД (не компилируется)
- `STARTSCR_notify_gameover()` закомментирован в Game.cpp → auto_advance никогда не ставится
- Старый attract demo (playback .pkt) весь закомментирован; `level_won()/level_lost()` = `#if 0`
- ui.md обновлён: добавлен раздел "Frontend → Game Pipeline" с полным флоу и переменными
- Аннотированы: Attract.cpp (header + main loop + STARTS_START block), startscr.cpp (header), frontend.cpp (FE_START block)

**ВЫПОЛНЕНО в этой итерации (briefing.cpp + исправления форматов):**
- briefing.cpp = МЁРТВЫЙ КОД (декабрь 1998, прототип): BRIEFING_menu() закомментирован целиком; BRIEFING_mission_filename в elev.cpp обёрнут в `#ifdef OBEY_SCRIPT` (а OBEY_SCRIPT закомментирован в briefing.h → не определён); 8 hardcoded миссий/файлов; save = 1-байт "savegame.dat" (заменено .wag)
- Реальная переменная-мост = `STARTSCR_mission[]` в startscr.cpp — её читает elev.cpp при загрузке миссии
- `BRIEFING_select()` вызывается ТОЛЬКО из Attract.cpp (attract mode / demo) — не из основного игрового пути
- **ИСПРАВЛЕНО** `sizeof(ED_Light)` = 20 байт (не 12/16): 8 однобайт.полей + SLONG×3 = 8+12; lighting_format.md обновлён
- **ИСПРАВЛЕНО** `sizeof(PrimFace3)` = 28 байт (не 22): 1+1+6+6+6+2+2+2+1+1; model_format.md обновлён
- **ИСПРАВЛЕНО** `sizeof(PrimFace4)` = 34 байта (не ~26): 1+1+8+8+8(union)+2+2+2+1+1; model_format.md обновлён
- **ИСПРАВЛЕНО** заголовок PrimObject с "(10 байт)" на "(16 байт)"; в KB уже была корректная формула 16б в коде

**ВЫПОЛНЕНО в этой итерации (.sty формат + FRONTEND_input):**
- resource_formats/mission_script_format.md создан:
  - .sty формат v4: `ObjID:GroupID:ParentID:ParentIsGroup:Type:Flags:District:fn:title:brief`
  - История версий v2/v3/v4/default с различиями полей
  - [districts] секция: `Name=X,Y` или `=0,0`; полный список 28 районов
  - suggest_order[] = ЗАХАРДКОЖЕН в frontend.cpp (строки 224-267), заканчивается "!"; не из файла
  - Загрузка: text mode ("rt"), кэш 20KB, LoadStringScript построчно
- ui.md раздел 2 дополнен деталями FRONTEND_input():
  - UP/DOWN → selected±1 (wrap), пропуск OT_LABEL и greyed OT_BUTTON
  - HOME/END → первый/последний активный пункт
  - ENTER → OT_BUTTON: queue transition; OT_MULTI: cycle; OT_KEYPRESS/PADPRESS: grab mode
  - FE_MAPSCREEN: ENTER → 100+mission_selected; mode≥100: ENTER → FE_START
  - LEFT/RIGHT: Slider ±2, Multi ±1, MapScreen → district_selected
  - ESC: fade_mode==2 → отмена transition; stackpos→FE_BACK; иначе→FE_MAINMENU
  - Джойпад: NOISE_TOLERANCE=4096, диагонали запрещены, edge-triggered, first_pad=suppress boot glitch

**ВЫПОЛНЕНО в этой итерации (water height + frontend аннотации):**
- physics.md: секция 5c воды полностью переписана:
  - PAP_LO_NO_WATER = -127; water_level = -0x80 (хардкод в EL_load_map)
  - PAP_2LO.water = water_level >> 3 = -16 при загрузке для водяных ячеек
  - OB объекты в воде: oi->y = water_level - miny = -128 - miny (на старте)
  - stop_movement_between(): water→non-water → invisible DFacet (активный код)
  - OB_height_fiddle_de_dee() в ob.cpp = МЁРТВЫЙ КОД (/* */ обёрнута)
  - PAP_FLAG_SINK_POINT: вода + 3 соседних, для системы луж и az.cpp рендеринга
- frontend.cpp аннотирован: FRONTEND_ParseMissionData + FRONTEND_MissionHierarchy + FRONTEND_mode + FRONTEND_display
  - ParseMissionData версии v2/3/4: поля ObjID/GroupID/ParentID/Type/Flags/District/fn/title/brief
  - MissionHierarchy: menu_theme по complete_point, ANNOYING_HACK_FOR_SIMON активен (policeID требует fight1+assault1+testdrive1a), suggest_order автовыбор
  - FRONTEND_mode stack/pop, mode≥100=mission brief, FE_MAINMENU=reset stack
  - FRONTEND_display порядок: background→xition→kibble→menu items→title wibble→districts

**ВЫПОЛНЕНО в этой итерации (player_progress + rendering split):**
- player_progress.md полностью переписан с точными данными из frontend.cpp:
  - .wag: mission_name = variable-length string + CRLF (НЕ фиксированные 32б!)
  - version field ПОСЛЕ complete_point (исторически — "Historical Reasons")
  - Версионная загрузка: v0=только cp, v1=+stats, v2=+hierarchy, v3=+best_found
  - best_found[50][4] = лучшие приросты Constitution/Strength/Stamina/Skill за миссию (анти-фарм механика)
  - mission_hierarchy биты: 1=exists, 2=complete, 4=available/waiting
  - complete_point пороги текстурных тем: <8=0, <16=1, <24=2, ≥24=3
- rendering.md разбит (446→386 строк): секции 3b+4+2b перенесены в rendering_mesh.md
- rendering_mesh.md создан: Tom's Engine, vertex morphing, crumple, reflections, UV packing
- README.md обновлён: добавлены rendering_mesh.md и rendering_lighting.md

**ВЫПОЛНЕНО в этой итерации (MapElement.Colour + POLY_frame_draw):**
- MapElement.Colour = МЁРТВЫЙ КОД в DDEngine (PC) — нигде не вызывается из-за движка рендеринга
- Glide Engine (legacy): LIGHT_get_glide_colour(me->Colour) → pp->colour (напрямую)
- Полный DDEngine vertex colour pipeline задокументирован (rendering_lighting.md):
  NIGHT_light_mapsquare → ambient(dprod-нормаль) + Slight(range check+QDIST3) + llapost → NIGHT_Colour[16]
  ⚠️ БАГ в dprod: `dz*nx` вместо `dz*nz` (строка 746 night.cpp)
  NIGHT_get_d3d_colour: ×4 scale (0..63→0..252), overflow→pseudo-specular, 0xFF_alpha
  Per-frame: NIGHT_cache[lo_x][lo_z]→square→colour[dx+dz*4]→NIGHT_get_d3d_colour→pp→apply_cloud→fadeout
  Indoor path: фиксированный 0x80808080 (NIGHT indoor код в /* */, не активен)
- POLY_frame_draw bucket sort полностью задокументирован (rendering.md Section 3):
  sort_z = max(Z) вершин = ZCLIP_PLANE/vz (reciprocal → larger=nearer)
  buckets[2048]: bucket=int(sort_z*2048); bucket[0]=far, bucket[2047]=near
  Render i=0..2047 → FAR TO NEAR = BACK-TO-FRONT (painter's algorithm) ✓
  POLY_PAGE_PUDDLE: скипается из bucket sort (отдельный проход)
  Альтернативный #else (отключён): merge sort per page SortBackFirst()
  Fog colour: NIGHT_sky_colour (outdoor) / 0 (sewers/indoors) / grayscale (draw_3d)

**ВЫПОЛНЕНО в этой итерации (Controls + Missions):**
- controls double-click: tick = GetTickCount() (WIN32 мс, НЕ GAME_TURN!); окно 200мс; исправлена ошибочная аннотация в interfac.cpp
- missions.md раздел 11: добавлена "После EventPoints" версионная секция — SkillLevels skip if version>5; BOREDOM_RATE always read; CarsRate+MusicWorld if version>8; messages section
- Уточнён полный .ucm binary layout (заголовок 1316 б + 512×74 EventPoints + версионный хвост)

**ВЫПОЛНЕНО в этой итерации (Навигация + Characters + Рендеринг):**
- navigation.md: MAV_can_i_walk детальный алгоритм — нормализованный шаг ~0x40, проверка GOTO edges, диагональные corner cells, только GOTO caps
- navigation.md: Навигация на крышах — крыши = обычные MAV ячейки с высоким MAVHEIGHT; PAP_FLAG_HIDDEN + roof_face → height установлен; без roof_face → PAP_FLAG_NOGO; скайлайты (prim 226/227) убраны из nav
- characters.md: Подтверждено — people_functions[] маппит ВСЕ THUG → cop_states (fn_cop_init, не fn_thug_init); fn_thug_init с ASSERT(0) НИКОГДА не вызывается; fn_roper_normal() пуста — только PCOM; fn_cop_normal() #if 0 — только PCOM
- rendering.md: Crinkle система полностью задокументирована — микро-геометрический bump mapping; 3D mesh → баринцентрически маппится на квад + нормаль-выдавливание; ПОЛНОСТЬЮ ОТКЛЮЧЁН (`return NULL` в CRINKLE_load, `if(0)` в aeng.cpp); не переносить
- rendering.md: NIGHT_generate_walkable_lighting — МЁРТВЫЙ КОД (`return;` после roof_walkable); только NIGHT_generate_roof_walkable() реально вызывается

**ВЫПОЛНЕНО в этой итерации (Здания/Интерьеры — TODO пункты):**
- buildings_interiors.md: ID_generate_floorplan() — полный флоу: 32 seeds, find_good_layout, ID_generate_inside_walls (num_walls=area/16+1), ID_add_room_faces
- buildings_interiors.md: ID_find_rooms() — BFS flood-fill, queue 64, блокируется по WALL_XL/XS/ZL/ZS флагам
- buildings_interiors.md: ID_score_layout_house_ground() — ratio 256-шкала, цель=414 (golden ratio), прямоугольность +1000, одна связь → *3/4
- buildings_interiors.md: ID_assign_room_types() — bubble sort по площади; HOUSE: biggest=LOUNGE; WAREHOUSE: biggest=WAREHOUSE, LOO/OFFICE/MEETING
- buildings_interiors.md: STAIR_calculate scoring — детальные значения (+0x10000/-0x10000/+0x5000/+0xbabe/-INF)
- buildings_interiors.md: WARE_init() — полная структура WARE_Ware, пулы nav/height/rooftex, door coords, OB_FLAG_WAREHOUSE
- Аннотированы: ware.cpp (полный header блок), stair.cpp (scoring блок), id.cpp (ID_generate_floorplan header)

**ВЫПОЛНЕНО в этой итерации (World, Characters):**
- world_map.md раздел 9: build_quick_city() — DFacet+roof_faces4 в PAP_Lo.ColVectHead/Walkable; только roof_faces4 (-face) реально регистрируются
- world_map.md: ROAD_wander_calc() — граф нодов для авто-AI (ROAD_Node 8б, MAX 256), сканирует ROAD_is_middle(5×5), ROAD_add с пересечениями, граничные ноды в ROAD_edge[]
- world_map.md: ROAD_is_road() = PAP_Hi.Texture 323-356 (PC), ROAD_is_middle = 5×5 все road; ROAD_LANE=0x100; хардкод (121,33) на gpost3.iam
- world_map.md: WAND_init() — PAP_FLAG_WANDER на тротуарах ≤2 кл. от дороги + зебры (333/334) - исключает коллизионные примы y≤terrain+64
- world_map.md: WAND_get_next_place() — 4 направления±1 рандом → dot product scoring; fallback 16 случайных проб
- characters.md: InterruptFrame = МЁРТВЫЙ КОД — всегда 0, нигде не присваивается ненулевое значение
- characters.md: CMatrix33 = {SLONG M[3]} (3 packed SLONG, не float!); GameKeyFrameElement ULTRA_COMPRESSED = 8б {m00/m01/m10/m11:SBYTE + OffX/Y/Z:SBYTE + Pad:UBYTE}; scale=128; GetCMatrix: UCA_Lookup[a][b]=Root(16383-a²-b²), строка 2 = cross product >>7, clamped ±127

**ВЫПОЛНЕНО в предыдущей итерации (AI остатки + controls + форматы):**
- ai.md разделы 10f/10g/10h — PCOM_process_killing детали, PCOM_process_snipe, pcom_zone
- ai.md 10f: KILLING→FLEE ТОЛЬКО для FAKE_WANDER NPC через PCOM_should_fake_person_attack_darci(); regular→NAVTOKILL или NORMAL
- ai.md 10g: snipe = 3 substates (LOOK→AIMING→NOMOREAMMO), range 0x600, "holster when idle" закомментирован
- ai.md 10h: pcom_zone = 4-bit bitmask PAP_Hi.Flags>>10; NPC с zone!=0 не видят/слышат цели из других зон; level design error recovery
- controls.md: apply_button_input() полный флоу — ACTION tree, движение dispatch, NON_INT блокировки
- texture_format.md: .txc = FileClump архив (header+offsets+lengths+WriteSquished entries); ⚠️ size_t платформозависим
- texture_format.md: style.tma = 200×5 TXTY + flags; instyle.tma = count_x*count_y UBYTE; TXTY={Page,Tx,Ty,Flip}
- Аннотированы: pcom.cpp (killing+snipe обновлены, zone +1 блок), interfac.cpp (apply_button_input), io.cpp (TXTY исправлен)

**ВЫПОЛНЕНО в предыдущей итерации (физика + WMOVE):**
- physics.md: WMOVE система — moving walkable faces (wmove.cpp), лимиты, PSX ограничение, WMOVE_process/relative_pos
- physics.md: mount_ladder() полный флоу — ok_to_mount_ladder, set_person_climb_ladder (аним по PersonType)
- physics.md: ПРЕ-РЕЛИЗ: mount_ladder() из interfac.cpp закомментирован! Игрок не может залезать снизу
- physics.md: collide_against_objects() детали — PRIM_COLLIDE_BOX, sit_down trigger (prim IDs), ignore OnFace
- physics.md: water physics — просто invisible walls по краям, нет замедления/утопания
- vehicles.md: VEH_find_runover_things() точный алгоритм — 2 сферы, WheelAngle scaling, infront=[256,512]
- Аннотированы: wmove.cpp (~5 блоков), collide.cpp (mount_ladder + ok_to_mount_ladder)

**ВЫПОЛНЕНО в этой итерации (навигация):**
- navigation.md: WARE_mav_enter/inside/exit — паттерн MAV_nav swap, логика дверей с height check
- navigation.md: INSIDE2_mav_* — enter работает, inside=ASSERT(0), stair/exit=баг `>` вместо `>>`
- navigation.md: INSIDE2_mav_nav_calc баг (z-цикл по MinX/MaxX вместо MinZ/MaxZ)
- navigation.md: MAV при изменении уровня — только двери через turn_on/off; взрывы НЕ обновляют MAV
- navigation.md: MAV_turn_movement_on заменяет ВСЕ caps на GOTO (остальные теряются)
- Аннотированы: ware.cpp (WARE_mav_enter/inside/exit), inside2.cpp (INSIDE2_mav_nav_calc + все mav_* функции)

**ВЫПОЛНЕНО в предыдущей итерации (AI детали):**
- ai.md разделы 10..10e — PCOM_process_default dispatch таблица, PCOM_process_normal детали
- ai.md: MIB = каждый кадр can_a_see_b, group aggro radius 1280, PCOM_alert_nearby_mib_to_attack (MIB+GUARD+GANG+FIGHT_TEST)
- ai.md: BANE = SUMMON_START→SUMMON_FLOAT, электродуги к 4 телам, proximity electrocute Darci (25hp каждые ~2 сек)
- ai.md: DRIVING AI = FINDCAR если не за рулём, иначе STILL/PATROL/WANDER driving; COP_DRIVER arrest закомментирован
- ai.md: SEARCHING/SLEEPING = стабы (пустые); LOOKAROUND = только счётчик
- ai.md: resurrect bug — newpos вычисляется но не присваивается → гражданские воскресают на месте смерти

**ВЫПОЛНЕНО ранее (форматы ресурсов):**
- Создан `resource_formats/lighting_format.md` — .lgt формат (ED_Light 16б, NIGHT_Colour 3б, ambient)
- Обновлён `resource_formats/map_format.md` — исправлено .iam (не .pam!), SuperMap binary layout
- Обновлён `resource_formats/model_format.md` — .prm формат: PrimObject/PrimFace3/PrimFace4 структуры
- Обновлён `resource_formats/animation_format.md` — .all формат: keyframe chunk, pointer fixup
- Обновлён `subsystems/missions.md` раздел 11 — .ucm формат: EventPoint = 74 байта точно
- Обновлён `resource_formats/README.md` — исправлены расширения, добавлен .lgt

---

## interfac.cpp аннотирование — ИТОГ (выполнено)

**Ключевые открытия:**
- process_hardware_level_input_for_player(): камера сначала → EWAY стоп → form_leave_map стоп → InputDone mask → weapon keys → mode dispatch
- InputDone механизм: накапливает обработанные биты, сбрасывает отпущенные; кнопки срабатывают раз на нажатие (не каждый кадр)
- Главный диспетчер: DRIVING→apply_button_input_car; FIGHT→apply_button_input_fight; нормальный→apply_button_input
- player_apply_move(): STATE machine, только поворот; бег/прыжок/действие — в apply_button_input
- player_turn_left_right(): wMaxTurn=94 (idle), 12 (в воздухе), 70-Velocity (бег); клавиатура → накопительный поворот; стик → пропорциональный
- player_turn_left_right_analogue(): стик → Arctan(-dx,dz) → +camera_angle; Roll = (Velocity-9)*dx>>5 (наклон при беге)
- apply_button_input_fight(): MOVE без FORWARDS = выход из FIGHT в RUN режим (только без analogue)
- do_an_action() приоритеты: выйти из машины > арест > слезть с лестницы > сесть в машину > крюк > переключатель > разговор > подбор > обыск > обнять стену > кола > присесть
- should_i_jump(): проверяет WARE_which_contains по 4 точкам; dx/dz захардкожены (не зависят от угла персонажа — баг/упрощение!)
- Горячие клавиши: KB_1=убрать, KB_2=пистолет, KB_3..KB_8=SHOTGUN/AK47/GRENADE/C4/KNIFE/BAT
- Аннотированы: interfac.cpp ~50+ claude-ai блоков

---

## game_states.md + camera.md — ИТОГ (выполнено)

**game_states.md ключевые открытия:**
- Per-frame порядок: GAMEMENU → tutorial → controls → should_i_process (things+EWAY+FC) → draw_screen → OVERLAY → screen_flip → lock_frame_rate(30) → handle_sfx → GAME_TURN++
- Bench cooldown: кадр 314 из каждых 1024 (~34с) → сброс GF_DISABLE_BENCH_HEALTH
- Post-loop: park2.ucm→cutscene(1), Finale1.ucm→cutscene(3)+OS_hack()
- DarciDeadCivWarnings: 0-2 = предупреждения; >=3 = GS_LEVEL_LOST; персистирует между миссиями!
- GS_REPLAY = goto round_again (полный рестарт); нет чекпоинтов
- SMOOTH_TICK_RATIO: 4-кадровое скользящее среднее TICK_RATIO для машин

**camera.md ключевые открытия:**
- cam.cpp = МЁРТВЫЙ КОД (`#ifdef DOG_POO`); только fc.cpp активен
- FC_alter_for_pos: gun-out→ddist=200; InCar→ddist=356; idle height check → МЁРТВЫЙ КОД (`&& 0`)
- FC_calc_focus: focus_yaw = персонаж.Angle; peeking (HUG_WALL_LOOK) смещает focus_x/z; dangling → камера сбоку (±550)
- FC_focus_above: gun-out → lower 0xa000; InsideIndex → cam_height*1.5; машины разных типов по-разному
- FC_process: 8-шаговый raycast от want_pos к focus для collision + get-behind + distance clamp + smoothing (>>2 pos, >>3 Y, >>2 angles)
- Lens: всегда 0x28000*CAM_MORE_IN (FOV не меняется)
- Toonear 0x90000 = first-person mode (специальный случай)

---

## Mission.cpp / CRIME_RATE / Win-Lose — ИТОГ (выполнено)

**Ключевые открытия:**
- Mission.cpp = WayWind EDITOR файл, не игровой рантайм
- Трансляция WPT_* → EWAY_DO_* происходит в elev.cpp (строки 748-1654) при загрузке .ucm
- WPT_GOTHERE_DOTHIS (39) = НЕ РЕАЛИЗОВАН в пре-релизе (ASSERT(0) в default case)
- WPT_BONUS_POINTS → EWAY_DO_MESSAGE (очки = мёртвый код через `if(0)`)
- set_stats() (Person.cpp:412) = тривиальна: только `stat_game_time = GetTickCount() - stat_start_time`
- CRIME_RATE delta: kill guilty=-2, kill civ wander=+5, arrest guilty=-4, OBJECTIVE мёртв код
- EWAY_counter[7] = счётчик убитых копов (инкр. до 255, не декрементируется)
- DarciDeadCivWarnings (>= 3) → GS_LEVEL_WON превращается в GS_LEVEL_LOST!
- park2.ucm → cutscene 1 (MIB intro); Finale1.ucm → cutscene 3 + OS_hack()
- GROUP_LIFE/DEATH иммунны к себе — только другие WP той же colour+group меняются
- Аннотированы: elev.cpp (WPT→EWAY_DO mapping), eway.cpp (OBJECTIVE, GROUP_LIFE/DEATH, COUNTER), Person.cpp (CRIME_RATE update)
- missions.md обновлён: разделы 10, 10a, 10b — полные детали

---

## eway.cpp — ИТОГ (выполнено)

**Ключевые открытия:**
- File already had extensive annotations from prior sessions + added more this session
- EWAY_COND_PRESSURE и EWAY_COND_CAMERA = **always FALSE** (stubs, не реализованы)
- PSX оптимизация: step=2 (чётные/нечётные кадры), кроме GAME_TURN==0; PC всегда step=1
- EWAY_STAY_ALWAYS → сразу EWAY_FLAG_DEAD (fire-once); в EWAY_process() и в set_inactive
- zone byte в EWAY_create_enemy: bits 0-3=zone, bit 4=INVULNERABLE, bit 5=GUILTY, bit 6=FAKE_WANDER
- Roper bug (пре-релиз): использует Darci stats (Roper блок закомментирован)
- Непрерывные while-active: EMIT_STEAM, WATER_SPOUT (4x DIRT_new_water/кадр), SHAKE_CAMERA (+=3), VISIBLE_COUNT_UP
- Координаты вэйпойнта в mapsquare-единицах → ×256 (<<8) при создании Thing
- EWAY_created_last_waypoint: resolve IDs + count objectives/guilty + init timers + attach camera targets
- 54 функции в файле, детальная карта в KB создана
- missions.md обновлён: детали реализации, stay logic, stub conditions, spawn details

---

## Person.cpp — ИТОГ (выполнено)

**Ключевые открытия:**
- Per-frame порядок: `StateFn(thing)` → `general_process_person()` → `PCOM_process_person()`
- `people_functions[]`: Darci→darci_states, Roper→roper_states, ВСЕ NPC→cop_states
- `darci_states` — только 7 записей (INIT, NORMAL=NULL, HIT, REMOVE, MOVEING, IDLE). Прыжки/бой/лазание через generic_people_functions
- `fn_person_jumping()` — plant_feet() везде закомментирована! Y задаётся явно из PAP или projectile_move_thing
- `fn_person_dangling()` — единственное место активного plant_feet() при приземлении: SUB_STATE_DROP_DOWN_LAND → end==1
- `set_person_drop_down()` — флаги: KEEP_VEL, KEEP_DY, QUEUED, OFF_FACE; DY=-(4<<8), Velocity=-8 если не KEEP
- `general_process_person()` — ПОСЛЕ StateFn, делает: WMOVE, горение, кровь, гарпун, general_process_player()
- JUMPING→LAND: FLY→hit==1→LAND_FAST(если движется) или idle(если стоп) → STATE_MOVEING

---

## Статус фазы анализа

**Фаза 1 (ЗАВЕРШЕНА):** Детальный анализ оригинального кода → запись в `original_game_knowledge_base/`
- KB написана на ~99% — все геймплейные подсистемы покрыты + верификация проведена
- Исходники аннотированы: 80+ файлов с `// claude-ai:` комментариями
- ~180 неаннотированных файлов протриажены: 90%+ = editor/PSX/rendering (подтверждено)
- Перекрёстная верификация: combat.md, weapons_items.md, vehicles.md, characters.md обновлены
- Новые системы задокументированы: minor_systems.md (Water, Trip, SM, Balloon, Tracks, Spark)
- **ГОТОВО к фазе 2** (планирование new_game)

---

## Карта подсистем (1-2 строки + связи)

| Подсистема | Файл KB | Статус | Ключевой факт |
|-----------|---------|--------|---------------|
| **Физика/коллизии** | physics.md | ✅ Хорошо | Integer-only, slide_along без отскока, гравитация animation-driven для персонажей, HyperMatter для мебели |
| **Thing/Объекты** | game_objects.md | ✅ Хорошо | union+fnptr полиморфизм, MAX 700, MapWho хэш, state machine |
| **AI (PCOM)** | ai.md | ✅ Хорошо | 21 тип AI, 27 состояний, собаки инертны в пре-релизе |
| **Навигация** | navigation.md | ✅ Хорошо | MAV = greedy best-first (НЕ A*), горизонт 32, NAV = wallhug |
| **Персонажи/анимации** | characters.md | ✅ Хорошо | Vertex morphing (НЕ skeletal), DrawTween, Thug/Cop в #if 0 |
| **Управление/ввод** | controls.md | ✅ Хорошо | 18 кнопок INPUT_*, 52 ACTION_*, zipwire есть в финале |
| **Транспорт** | vehicles.md | ✅ Хорошо | 4 пружины подвески, 6 зон урона, мотоцикл НЕ переносить |
| **Бой** | combat.md | ✅ Хорошо | Урон из анимации (GameFightCol), knockback через анимации |
| **Оружие/предметы** | weapons_items.md | ✅ Хорошо | 30 типов SPECIAL_*, мины заглушены, C4 = 5 сек (не 10) |
| **Мир/карта** | world_map.md | ✅ Хорошо | PAP_Hi 128×128 + PAP_Lo 32×32, MapWho, здания |
| **Здания/интерьеры** | buildings_interiors.md | ✅ Хорошо | 21 тип STOREY_TYPE_*, DFacet, двери через MAV |
| **Миссии (EWAY)** | missions.md | ✅ Хорошо | 41 условие, 57 действий, .ucm ≠ MuckyBasic, polling каждый кадр |
| **Загрузка уровня** | level_loading.md | ✅ Хорошо | 9 шагов, MAV_precalculate самый тяжёлый, игрок создаётся через EWAY |
| **Рендеринг** | rendering.md | ✅ Достаточно | Полностью заменяем, DirectX6 → OpenGL; 386 строк |
| **Рендеринг/Меши** | rendering_mesh.md | ✅ Достаточно | Tom's Engine, vertex morphing, crumple, UV packing |
| **Рендеринг/Освещение** | rendering_lighting.md | ✅ Достаточно | NIGHT система детали, Crinkle (отключён в пре-релизе) |
| **Состояния игрока** | player_states.md | ✅ Хорошо | Полные списки STATE_* и SUB_STATE_* |
| **Эффекты** | effects.md | ✅ Хорошо | Частицы, огонь, POW(взрывы), PYRO(18 типов пиротехники), DIRT(16 типов debris), ткань отключена |
| **Форматы ресурсов** | resource_formats/ | ✅ Хорошо | .iam/.prm/.all/.lgt/.ucm задокументированы; .txc/.tma остались |
| **Камера** | camera.md | ✅ Хорошо | FC only (cam.cpp=мёртв), 8-шаг raycast collision, get-behind алгоритм |
| **Звук** | audio.md | ✅ Хорошо | Miles Sound System → miniaudio, 14 MUSIC_MODE_*, 5 биомов ambient |
| **UI** | ui.md | ✅ Хорошо | HUD, инвентарь, fonts, gamemenu (frontend вынесен в frontend.md) |
| **Frontend** | frontend.md | ✅ Хорошо | game→mission pipeline, FE_* режимы, STARTSCR_mission, Attract.cpp |
| **Прогресс/сохранения** | player_progress.md | ✅ Хорошо | .wag: var-str+CRLF, v0-3, hierarchy bits, best_found=анти-фарм |
| **Матем/утилиты** | math_utils.md | ✅ Хорошо | 2 стека (PSX int/PC float), glm для новой игры |
| **Игровые состояния** | game_states.md | ✅ Хорошо | per-frame порядок, DarciDeadCivWarnings, GS_REPLAY=goto, bench cooldown |
| **Взаимодействие** | interaction_system.md | ✅ Хорошо | find_grab_face 2-pass, cable params в DFacet, ladder, zipwire |
| **Малые подсистемы** | minor_systems.md | ✅ Хорошо | Water, Trip, SM, Balloon, Tracks, Spark, Command(legacy), drawxtra warning |
| **WayWind** | waywind.md | ❌ Не нужен | Редактор, не переносить |
| **MuckyBasic** | muckybasic.md | ❌ Не нужен | Не интегрирован с игрой |

---

## Карта связей (что читать вместе)

```
collide.cpp      → physics.md + navigation.md + characters.md
Person.cpp       → ai.md + characters.md + combat.md + controls.md + player_states.md
eway.cpp         → missions.md + game_objects.md
Mission.cpp      → missions.md + weapons_items.md
interfac.cpp     → controls.md + player_states.md + camera.md
Vehicle.cpp      → vehicles.md + physics.md
Special.cpp      → weapons_items.md + combat.md
Building.cpp     → buildings_interiors.md + world_map.md + navigation.md
interact.cpp     → interaction_system.md + physics.md + controls.md + characters.md
plat.cpp         → game_objects.md + missions.md (waypoints)
chopper.cpp      → game_objects.md + ai.md
```

---

## Аннотированные исходники (// claude-ai: комментарии)

| Файл | Кол-во строк | Статус |
|------|-------------|--------|
| hm.cpp | ~309 | ✅ |
| Furn.cpp | ~158 | ✅ |
| pcom.cpp | ~178 | ✅ |
| Special.cpp | ~477 | ✅ |
| bat.cpp | ~128 | ✅ (= Bane AI, не летучие мыши!) |
| canid.cpp | ~101 | ✅ (собаки инертны — dispatch switch закомментирован) |
| cutscene.cpp | ~102 | ✅ |
| ob.cpp | ~124 | ✅ |
| elev.cpp | ~82 | ✅ |
| Anim.cpp | ~111 | ✅ |
| mesh.cpp (game) | ~111 | ✅ |
| id.cpp | ~150 | ✅ |
| facet.cpp | ~109 | ✅ |
| supermap.cpp | ~109 | ✅ |
| io.cpp | ~308 | ✅ |
| Prim.cpp | ~175 | ✅ |
| Game.cpp | ~134 | ✅ |
| figure.cpp (DDEngine) | ~249 | ✅ |
| Controls.cpp | ~215 | ✅ |
| collide.cpp | ~77 ann. | ✅ (уточнено: move_thing порядок, plant_feet, height_above_anything) |
| walkable.cpp | ~20 ann. | ✅ (find_face_for_this_pos подробно аннотирован) |
| Person.cpp | ~8 блоков | ✅ (per-frame порядок, jumping FSM, drop_down, dangling landing) |
| eway.cpp | ~60+ блоков | ✅ (per-frame loop, spawn funcs, cond stubs, stay logic, 54-func map) |
| Mission.cpp | N/A | ✅ (это EDITOR файл, не рантайм; WPT→EWAY_DO в elev.cpp аннотировано) |
| elev.cpp | ~30+ ann. | ✅ (WPT→EWAY_DO mapping, WPT_BONUS_POINTS dead code, WPT_GOTHERE_DOTHIS ASSERT) |
| **interfac.cpp** | ~50+ блоков | ✅ (process_hardware_input, player_apply_move, InputDone, weapon hotkeys, mode dispatcher) |
| **ware.cpp** | ~4 блока | ✅ (WARE_mav_enter/inside/exit) |
| **inside2.cpp** | ~6 блоков | ✅ (INSIDE2_mav_nav_calc + все mav_* функции, включая баги пре-релиза) |
| **ware.cpp** | ~1 header блок | ✅ (WARE_init полный флоу, WARE_Ware структура, nav/height/rooftex пулы) |
| **stair.cpp** | +1 блок | ✅ (scoring блок STAIR_calculate добавлен к существующим аннотациям) |
| **gamemenu.cpp** | 1 header блок | ✅ (GAMEMENU типы, wait timer, WON/LOST структуры, DO_* коды) |
| **Attract.cpp** | +расширен | ✅ (ScoresDraw header: поля статистики, mucky times, DC/PC ветки) |
| **frontend.cpp** | +2 блока | ✅ (FRONTEND_level_won + FRONTEND_level_lost с полными деталями) |
| **id.cpp** | +1 блок | ✅ (ID_generate_floorplan header: 32-seed pipeline, scoring, assign_room_types) |
| **wmove.cpp** | ~5 блоков | ✅ (WMOVE система, create/process/relative_pos) |
| **collide.cpp (ladder)** | +3 блока | ✅ (mount_ladder, ok_to_mount_ladder, пре-релиз баг) |
| **Map.cpp** | +4 блока | ✅ (MapElement.Colour = мёртвый код в DDEngine; MAP_light_map не вызывается) |
| **night.cpp** | +2 блока | ✅ (полный pipeline header; BUG dz*nx→dz*nz аннотирован) |
| **frontend.cpp** | +5 блоков | ✅ (ParseMissionData, MissionHierarchy, FRONTEND_mode, FRONTEND_display, FE_START pipeline) |
| **Attract.cpp** | ~3 блока | ✅ (header overview, main loop, STARTS_START handler) |
| **startscr.cpp** | 1 блок | ✅ (header: game build = только 1 переменная; #ifdef EDITOR = dead) |
| **overlay.cpp** | 1 header блок | ✅ (OVERLAY_handle pipeline, PANEL_last layout, active/dead code) |
| **guns.cpp** | 1 header блок | ✅ (auto-aim scoring, weapon stats, find_target_new, snipe) |
| **grenade.cpp** | 1 header блок | ✅ (физика, CreateGrenadeExplosion, show_grenade_path) |
| **Nav.cpp** | 1 header блок | ✅ (wallhug обёртка, NAV_Waypoint pool, NAV_wall_in_way) |
| **Wallhug.cpp** | 1 header блок | ✅ (2D grid pathfinding: Bresenham+huggers, LOS cleanup, 4 release conditions) |
| **barrel.cpp** | 1 header блок | ✅ (2-sphere rigid body: 4 типа, gravity/damping/MAV walls, stacking, shoot→explode) |
| **pow.cpp** | 1 header блок | ✅ (POW: 8 типов взрывов, sprite sphere generation, cascade spawn, damping) |
| **pyro.cpp** | 1 header блок | ✅ (PYRO: 18 типов пиротехники, IMMOLATE -10HP, GAMEOVER 8 dlights, MergeSoundFX) |
| **dirt.cpp** | 1 header блок | ✅ (DIRT: 16 типов debris, ambient leaves/cans, throwable physics, pigeon DISABLED) |
| **fire.cpp** | ~99 ann. | ✅ (FIRE система: 8 fires, 256 flames, burn-out, bespoke renderer) |
| **psystem.cpp** | ~158 ann. | ✅ (Particle system: pool, physics, PFLAG_*, fire_pal) |
| **Vehicle.cpp** | ~309 ann. | ✅ (5159 строк, 52 функции, подвеска, collision, steering, crumple) |
| **Building.cpp** | ~141 ann. | ✅ (здания, DFacet, roof faces) |
| **mav.cpp** | ~152 ann. | ✅ (MAV navigation grid, precalculate, caps) |
| **sound.cpp** | ~103 ann. | ✅ (Miles Sound System, MFX, ambient biomes) |
| **door.cpp** | ~33 ann. | ✅ (двери, MAV_turn_on/off, height check) |
| **ribbon.cpp** | 1 header блок | ✅ (circular buffer trail: CONVECT/FADE/SLIDE, 64×32, triangle strip) |
| **bang.cpp** | 1 header блок | ✅ (hierarchical visual explosions: 4 cascade types, 64×4096, MapWho) |
| **interact.cpp** | 1 header блок | ✅ (grab/ladder/cable: 2-pass search, DFacet cable params, bone positions) |
| **plat.cpp** | 1 header блок | ✅ (moving platforms: waypoint paths, GOTO/PAUSE/STOP, collision) |
| **chopper.cpp** | 1 header блок | ✅ (helicopter enemy: CIVILIAN type, detection radii, rotor anim) |
| **Pjectile.cpp** | 1 header блок | ✅ (generic projectile pool: 3 functions, minimal wrapper) |

---

## TODO по подсистемам — что ещё проверить

### ФИЗИКА (physics.md) — TODO ✅ ВСЕ ЗАКРЫТЫ
- [x] `find_face_for_this_pos()` — ГОТОВО (в walkable.cpp! Порог 160ед, GRAB_FLOOR=0x50, FIND_ANYFACE/FIND_FACE_NEAR_BELOW)
- [x] `slide_along()` детали — ГОТОВО (NOGO push=0x5800, DFacet axis-aligned, SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT=-0x50)
- [x] `height_above_anything()` — ГОТОВО (if(1||...) делает FIND_ANYFACE мёртвым кодом, всегда FIND_FACE_NEAR_BELOW)
- [x] `collide_against_things()` + `collide_against_objects()` — ГОТОВО (детали в physics.md)
- [x] `move_thing()` реальный порядок операций — ГОТОВО (things → objects → slide_along → edges → find_face → move)
- [x] `mount_ladder()` — ГОТОВО: ok_to_mount_ladder QDIST2<75; set_person_climb_ladder анимы по PersonType; в пре-релизе interfac.cpp вызов закомментирован (игрок не может снизу!)
- [x] Коллизии транспорта с персонажами: `VEH_find_runover_things()` — ГОТОВО: 2 сферы, WheelAngle scaling, infront=[256,512]
- [x] Воды: `PAP_Lo.water` высота воды — ГОТОВО: water_level=-128 хардкод; PAP_Lo.water=-16 при загрузке; OB_height_fiddle_de_dee=МЁРТВЫЙ КОД; stop_movement_between активен
- [x] RWMOVE система — ГОТОВО: это WMOVE (moving walkable faces) в wmove.cpp; max=192; PSX ограничение
- [x] `collide_against_objects()` детали — ГОТОВО: PRIM_COLLIDE_BOX через OB_find; sit_down trigger на prim IDs
- [x] `plant_feet()` — ГОТОВО: вызывается из STATE FUNCTIONS, НЕ из move_thing. Активно: fn_person_dangling (SUB_STATE_DROP_DOWN_LAND end==1). В fn_person_jumping — везде закомментирована.

### AI/PCOM (ai.md) — TODO
- [x] Детали `PCOM_process_default()` — таблица dispatch в ai.md разделе 10; SEARCHING/SLEEPING=стабы, LOOKAROUND=пустой
- [x] `PCOM_AI_MIB` (18) поведение — каждый кадр can_a_see_b, mass aggro radius 1280 (MIB+GUARD+GANG+FIGHT_TEST)
- [x] `PCOM_AI_BANE` (19) — SUMMON loop, 4 тела в воздух, electricity arcs, electrocute Darci if idle nearby
- [x] Driving AI детали — FINDCAR если не за рулём; PATROL/WANDER/STILL driving modes; DRIVE_DOWN=road graph wander
- [x] Гражданские воскресают через >200 кадров — на месте смерти (баг пре-релиза: newpos не присваивается)
- [x] KILLING→FLEE threshold — ТОЛЬКО FAKE_WANDER NPC (Rasta/Cop); через PCOM_should_fake_person_attack_darci(); обычные→NAVTOKILL/NORMAL
- [x] PCOM_AI_SHOOT_DEAD (21) детали — 3 substates: LOOK/AIMING/NOMOREAMMO; range 0x600; holster code закомментирован
- [x] pcom_zone — 4-bit bitmask PAP_Hi.Flags>>10; vision+sound+navtokill фильтр; level design error recovery

### ПЕРСОНАЖИ/АНИМАЦИИ (characters.md) — TODO
- [x] Точные переходы состояний в Darci: JUMPING→LANDING — ГОТОВО (см. Person.cpp итог выше)
- [x] `InterruptFrame` — МЁРТВЫЙ КОД: всегда 0 в пре-релизе, нигде не используется
- [x] CMatrix33 декомпрессия — ГОТОВО: {SLONG M[3]} packed; GameKeyFrameElement 8б; scale=128; UCA_Lookup=Root(16383-a²-b²); cross product для строки 2
- [x] Roper финальное поведение — ГОТОВО: fn_roper_normal() пуста, всё через PCOM; people_functions[] = roper_states только для Roper
- [x] Cop финальное поведение — ГОТОВО: fn_cop_normal() #if 0; все NPC кроме Darci/Roper → cop_states (fn_cop_init реально работает)
- [x] Thug инициализация — ГОТОВО: people_functions[] маппит THUG → cop_states → fn_cop_init (НЕ fn_thug_init!); fn_thug_init с ASSERT(0) НИКОГДА не вызывается

### НАВИГАЦИЯ (navigation.md) — TODO
- [x] `WARE_mav_*()` детали — ГОТОВО: паттерн MAV_nav swap; enter→ближайшая дверь MAV_do; inside→warehouse local coords; exit→height check на дверь
- [x] `INSIDE2_mav_*()` детали — ГОТОВО: enter работает; inside=ASSERT(0) заглушка; stair/exit=баги `>`вместо`>>`; nav_calc баг z-цикла
- [x] Как MAV обновляется при изменении уровня — ГОТОВО: только двери (turn_on/off); взрывы НЕ обновляют MAV во время игры
- [x] Детали MAV_can_i_walk — ГОТОВО: нормализованный шаг ~0x40, cell-by-cell GOTO check, диагональ → ещё 2 угловые ячейки; только CAPS_GOTO
- [x] Что происходит с навигацией когда NPC на крыше? — ГОТОВО: крыши = обычные MAV ячейки, MAVHEIGHT из roof_face.Y/0x40; без roof_face → NOGO; скайлайты убраны

### МИССИИ/EWAY (missions.md) — TODO
- [x] Порядок разрешения зависимостей EventPoints — ГОТОВО (EWAY_created_last_waypoint linear pass, no cycle detection)
- [x] Как EWAY_process() итерирует по EP — ГОТОВО (по порядку создания 1..upto, linear array, NOT linked list)
- [x] EWAY_COND_PRESSURE / EWAY_COND_CAMERA — ГОТОВО (always FALSE, stubs)
- [x] Точный binary layout .ucm файла — ГОТОВО: missions.md раздел 11 (версионный заголовок+SkillLevels skip+BOREDOM_RATE+CarsRate+MusicWorld+messages)
- [x] WPT_GOTHERE_DOTHIS (39) — НЕ РЕАЛИЗОВАН в пре-релизе (ASSERT(0) в elev.cpp default)
- [x] WPT_GROUP_LIFE / GROUP_DEATH (33/34) — ГОТОВО (scans colour+group, иммунны сами к себе)
- [x] EWAY_COND_COUNTER_GTEQ (25) — ГОТОВО (EWAY_counter[subtype] += 1 при DO_INCREASE_COUNTER)
- [x] **АННОТИРОВАТЬ** eway.cpp — ВЫПОЛНЕНО (OBJECTIVE, GROUP_LIFE/DEATH, COUNTER, MISSION_COMPLETE/FAIL)
- [x] **АННОТИРОВАТЬ** Mission.cpp — N/A (это редакторский файл, не игровой рантайм)
- [x] **АННОТИРОВАТЬ** elev.cpp WPT→EWAY_DO mapping — ВЫПОЛНЕНО

### УПРАВЛЕНИЕ/ВВОД (controls.md) — TODO
- [x] Детали `process_hardware_level_input_for_player()` — ГОТОВО (8-шаговый порядок, InputDone, камера первая)
- [x] Как работает аналоговый стик — ГОТОВО (NOISE_TOLERANCE=8192 PC / 24 DC, ANALOGUE_MIN_VELOCITY=8 PC / 32 PSX)
- [x] `player_apply_move()` — ГОТОВО (STATE machine: turn, jump, вызов через player_interface_move)
- [x] `player_turn_left_right_analogue()` — ГОТОВО (стик → Arctan(-dx,dz) + camera angle + Roll visual)
- [x] Горячие клавиши KB_1..KB_8 — ГОТОВО (1=убрать, 2=пистолет, 3-8=спецоружие)
- [x] **АННОТИРОВАТЬ** interfac.cpp — ВЫПОЛНЕНО
- [x] Double-click 200ms — GetTickCount() (WIN32 мс!); окно 200мс; исправлена ошибочная аннотация (было "GAME_TURN") в interfac.cpp
- [x] `apply_button_input()` полный флоу — ACTION tree (ACTION_SHOOT/JUMP/KICK/PUNCH/HUG/FLIP/DROP), movement dispatch, NON_INT флаги

### ФОРМАТЫ РЕСУРСОВ — TODO (КРИТИЧНО для data pipeline)
- [x] **`.all` файлы** — ГОТОВО: binary layout в animation_format.md (keyframe chunk, pointer fixup, moj embeds)
- [x] **`.iam` файлы** — ГОТОВО: map_format.md обновлён (PAP_Hi + animated prims + SuperMap)
- [x] **`.ucm` файлы** — ГОТОВО: missions.md раздел 11, EventPoint = 74 байта (14+60)
- [x] **`.prm` файлы** — ГОТОВО: model_format.md (PrimObject, PrimFace3, PrimFace4 structs)
- [x] **`.lgt` файлы** — ГОТОВО: lighting_format.md создан (ED_Light 16б, NIGHT_Colour 3б)
- [x] **`.txc` файлы** — ГОТОВО: FileClump архив; header=[ULONG MaxID, Offsets[], Lengths[]]; entry=WriteSquished(TXTY 4:4:4:4 или 5:6:5); ⚠️ size_t платформозависим; для новой игры загружать TGA напрямую
- [x] **`style.tma`** — ГОТОВО: 200 стилей × 5 слотов × TXTY(4б); textures_flags[200][5]; instyle.tma=count_x*count_y*UBYTE; обновлено texture_format.md

### МИР/КАРТА (world_map.md) — TODO
- [x] `build_quick_city()` — ГОТОВО: DFacet→ColVectHead, roof_faces4→Walkable; mark_naughty_facets; только -face работает
- [x] `ROAD_wander_calc()` — ГОТОВО: ROAD_Node граф, ROAD_is_middle(5×5), ROAD_add с intersect/split, ROAD_edge[], хардкод gpost3.iam
- [x] `WAND_init()` — ГОТОВО: ≤2кл. от дороги + зебры + исключение collision prims; WAND_get_next_place() dot-product scoring
- [x] Как освещение `MapElement.Colour` применяется к вершинам при рендеринге — ГОТОВО: МЁРТВЫЙ КОД в DDEngine; Glide=прямо; DDEngine=NIGHT_light_mapsquare→NIGHT_cache→NIGHT_get_d3d_colour (rendering_lighting.md)

### ЗДАНИЯ/ИНТЕРЬЕРЫ (buildings_interiors.md) — TODO
- [x] `ID_generate_floorplan()` — ГОТОВО: 32 seeds, find_good_layout, ID_generate_inside_walls (num_walls=area/16+1 или /8+2 для апартаментов)
- [x] `RoomID scoring` → `ID_score_layout_house_ground()` — ГОТОВО: ratio 256-шкала, цель=414, +1000 прямоугольность, *3/4 если комната только из одной доступна
- [x] STAIR_calculate scoring — ГОТОВО: rand+0x10000/-0x10000/+0x5000/+0xbabe/-INF детали
- [x] `WARE_init()` — ГОТОВО: WARE_Ware полная структура, пулы nav/height/rooftex, door out/in coords

### РЕНДЕРИНГ (rendering.md) — TODO (низкий приоритет — заменяем)
- [x] Crinkle система — ГОТОВО: микро-геометрический bump mapping, mesh→квад проекция; отключён в пре-релизе, но **РАБОТАЕТ в PC релизе** (bump на ящиках); переносить через normal/parallax mapping
- [x] `POLY_frame_draw()` порядок сортировки — ГОТОВО: bucket sort [2048], sort_z=ZCLIP/vz, render FAR→NEAR; merge sort fallback в #else (отключён)
- [x] Как `NIGHT_generate_walkable_lighting()` работает — ГОТОВО: МЁРТВЫЙ КОД (`return;`), только NIGHT_generate_roof_walkable() реально вызывается
- [x] rendering.md разбит (446→386 строк): Tom's Engine + Mesh + UV пакинг → rendering_mesh.md

### НЕ ЧИТАНЫ (нужно прочесть KB файлы)
- [x] **camera.md** — ГОТОВО: FC only, 8-шаг raycast, get-behind, focus_yaw, toonear
- [x] **game_states.md** — ГОТОВО: per-frame порядок, DarciDeadCivWarnings, GS_REPLAY
- [x] **audio.md** — ГОТОВО: 14 MUSIC_MODE_*, 5 биомов, MFX→miniaudio, indoor/outdoor, footsteps
- [x] **ui.md** — ГОТОВО: HUD, enemy tracking, frontend modes, fonts, gamemenu
- [x] **math_utils.md** — ГОТОВО: PSX 0-2047 / PC radians, SinTable 2560 entries, glm замена

---

## Быстрые критичные факты (не забыть!)

- GAMEMENU wait delay: 64/кадр, порог 64*20*2=2560 (~40 кадров, ~1.3с) → тогда WON/LOST меню появляется
- WON-меню: НЕТ выбираемых пунктов (только X_LEVEL_COMPLETE); ESC/ENTER → DO_NEXT_LEVEL → GS_LEVEL_WON → FRONTEND_level_won()
- LOST-меню: Restart→DO_RESTART; Abandon→SURE→OKAY→DO_CHOOSE_NEW_MISSION → GS_LEVEL_LOST → FRONTEND_level_lost()
- FRONTEND_level_won(): иерархия bit2, best_found анти-фарм дельты, complete_point обновление, FE_SAVESCREEN+m_bGoIntoSaveScreen=TRUE
- FRONTEND_level_lost(): mission_launch=previous_mission_launch (restore), FE_MAINMENU
- ScoresDraw() (из GAMEMENU_draw при WON): killed/arrested/at-large/bonus found/missed/time; mucky times hash (DC активна, PC в #if 0)
- Mission.cpp = WayWind EDITOR файл (не игровой рантайм); WPT_* → EWAY_DO_* трансляция в elev.cpp:748-1654
- WPT_BONUS_POINTS → EWAY_DO_MESSAGE (очки = мёртвый код if(0)); WPT_GOTHERE_DOTHIS = ASSERT(0) не реализован
- set_stats() (Person.cpp:412) тривиальна: только записывает stat_game_time
- CRIME_RATE delta: kill guilty=-2, kill wander_civ=+5, arrest guilty=-4
- DarciDeadCivWarnings>=3 → GS_LEVEL_WON превращается в GS_LEVEL_LOST (накапл. между миссиями!)
- EWAY_counter[7] = счётчик убитых копов (cap 255)
- park2.ucm→cutscene 1 (MIB intro); Finale1.ucm→cutscene 3 + OS_hack()
- `bat.cpp` = Bane AI (главный антагонист), НЕ летучие мыши
- Собаки (canid.cpp) = инертны в пре-релизе, dispatch switch закомментирован
- C4 взрывчатка = 5 сек (не 10 как написано в комментарии кода)
- HyperMatter = собственная разработка MuckyFoot (НЕ Criterion middleware)
- HyperMatter: `gy=0` hardcoded — объекты отскакивают от плоскости Y=0, не от реального terrain
- `.ucm` файлы = EWAY Mission Data, НЕ MuckyBasic скрипты
- Roper Health = 400 (в 2× больше Darci = 200)
- MIB Health = 700, KO принципиально невозможен
- FLAG_FURN_DRIVING определён в Furn.h (не Vehicle.h!)
- Ночью всегда дождь (hardcoded: `if (!(NIGHT_FLAG_DAYTIME)) GAME_FLAGS |= GF_RAINING`)
- fn_cop_normal() = #if 0 в пре-релизе
- fn_thug_init() = ASSERT(0) в пре-релизе
- Мотоцикл (BIKE) = НЕ переносить (не вошёл в финал)
- Zipwire (тросы) = ПЕРЕНОСИТЬ (есть в финале, первый уровень)
- Гарпун = НЕ переносить (never shipped)
- Канализация = НЕ переносить (never shipped)
- process_controls() = НЕ обработка ввода игрока (это диспетчер подсистем!)
- InputDone: накапливает обработанные биты input, сбрасывает при отпускании — кнопка срабатывает один раз
- player_apply_move() → только поворот (STATE machine); бег/прыжок/action — в apply_button_input()
- player_turn_left_right(): wMaxTurn=94 idle / 12 в воздухе / (70-Velocity) при беге; клавиатура=накопительный, стик=пропорциональный
- apply_button_input_fight(): MOVE без FORWARDS = выход в RUN (в аналоговом режиме отключено)
- apply_button_input(): ACTION→do_an_action(); нет ACTION→SPRINT→RUN; find_best_action_from_tree()→REQUEST flags + jump/shoot/flip/hug/skid
- should_i_jump(): dx/dz захардкожены (не от угла персонажа!) — проверяет WARE_which_contains по 4 точкам
- Горячие клавиши PC: KB_1=убрать, KB_2=пистолет, KB_3=шотган, KB_4=AK47, KB_5=граната, KB_6=C4, KB_7=нож, KB_8=бита
- NOISE_TOLERANCE: PC=8192 (из 65535), DC=24 (из 128); ANALOGUE_MIN_VELOCITY: PC=8, PSX=32 (из 128)
- audio.md: 14 MUSIC_MODE_*, приоритет: CINEMATIC>FIGHT>DRIVING>SPRINT>CRAWL>AMBIENT; замена MSS32→miniaudio
- math_utils.md: PSX углы 0-2047, SinTable[2560] (не 2048!), CosTable = &SinTable[512]; PC float radians+glm
- Карты уровней: расширение `.iam` (не .pam!), лежат в `data/`, скрипты миссий `.ucm` — в `levels/`
- EventPoint binary = 74 байта (14 заголовок + 60 данных); MAX_EVENTPOINTS=512 → 37888 байт на миссию
- PRIM_START_SAVE_TYPE=5793: если save_type==base+1 → PrimPoint как SWORD (6б), иначе SLONG (12б)
- PrimFace4 FACE_FLAG_WALKABLE=(1<<6) — только эти квады образуют ходимые поверхности
- NIGHT_Colour диапазон 0-63 (не 0-255!); для OpenGL делить на 64.0f
- sizeof(ED_Light) = **20 байт** (не 16!) — 8 однобайт.полей + 3 SLONG; lighting_format.md исправлен
- sizeof(PrimFace3) = **28 байт** (не 22!) — 1+1+6+6+6+2+2+2+1+1
- sizeof(PrimFace4) = **34 байта** (не 26!) — 1+1+8+8+8(union)+2+2+2+1+1
- ED_Light::range,red,green,blue — radius/color для статического точечного источника света
- ambient direction в night.cpp везде = (110, -148, -177) — захардкожена во всём коде
- .all файл: save_type 2-5; save_type>2 → 4-byte count + count×moj blocks; затем keyframe chunk
- DoubleClick детекция: tick = GetTickCount() (WIN32 **мс**, НЕ GAME_TURN!); окно 200мс; LastReleased[i]=tick при отпускании; DoubleClick[i]++ если re-press в окне, иначе =1. Порт: SDL_GetTicks()
- .ucm header: 4(version)+4(flags)+260×5(names)+2+2+2(MapIndex/Used/Free UWORDs)+2(CrimeRate+FakeCivs)=1316б; потом 512×74б EventPoints; потом: skip 254б SkillLevels(if ver>5), read 1б BOREDOM_RATE, read 1+1б CarsRate+MusicWorld(if ver>8), messages section
- .all pointer fixup: сохранённые NextFrame/PrevFrame = runtime адреса; при загрузке пересчитываются через addr1/addr2/addr3
- TIMEOUT_DEMO=0 → demo_timeout() = мёртвый код
- briefing.cpp = МЁРТВЫЙ КОД (Dec 1998 прототип); OBEY_SCRIPT закомментирован → BRIEFING_mission_filename не используется
- Реальный мост frontend→миссия = STARTSCR_mission[] из startscr.cpp; BRIEFING_select() только из Attract.cpp (demo mode)
- SPECIAL_MINE подбор = ASSERT(0), метание = ASSERT(0)
- save slots: 1-based! (`save_slot = menu_state.selected + 1`)
- complete_point диапазон 0-24+
- CRIME_RATE: если 0 → дефолт 50
- startscr.cpp (game build) = только `CBYTE STARTSCR_mission[_MAX_PATH]`; весь остальной код = #ifdef EDITOR
- RIBBON: circular buffer trail renderer; MAX_RIBBONS=64, MAX_RIBBON_SIZE=32; CONVECT=+22 Y/frame (fire updrift)
- RIBBON_alloc → 1-based ID (0=fail); RIBBON_draw_ribbon() в drawxtra.cpp:2536 → triangle strip Tail→Head
- BANG: иерархические визуальные взрывы (НЕ урон!); 4 каскадных типа BIG→MIDDLE→NEARLY→END; BANG_Phwoar[4096]
- interact.cpp: find_grab_face() = 2-pass (hi-res roof, lo-res walkable); checks 4 edges per quad
- Cable/zipwire params PACKED в DFacet: StyleIndex=angle_step1, Building=angle_step2, Height=count (переиспользование полей!)
- valid_grab_angle() = ALWAYS 1 (валидация отключена в пре-релизе)
- plat.cpp: moving platforms АКТИВНЫ; waypoint GOTO/PAUSE/STOP; LOCK_X/Y/Z; collision с персонажами
- chopper.cpp: вертолёт-враг АКТИВЕН; CHOPPER_CIVILIAN; DETECTION_RADIUS=1024
- Switch.cpp: ПОЛУМЁРТВЫЙ — sphere proximity detect работает, group/class = стабы
- lead.cpp: верёвка/поводок НЕДОДЕЛАН; nd.cpp = ПУСТОЙ файл; morph.cpp = НЕ используется
- Триаж 188 файлов без аннотаций: ~118 Source + ~48 DDEngine + ~22 DDLibrary; 90%+ = editor/PSX/rendering
- STARTSCR_mission flow: FE_MAPSCREEN+ENTER→mode=100+N; ещё ENTER→FE_START→STARTSCR_mission set→STARTS_START→GS_PLAY_GAME→elev.cpp→ELEV_load_name→*STARTSCR_mission=0
- DONT_load = 0 жёстко в frontend.cpp → per-mission dontload bitmasks игнорируются
- this_level_has_bane = ТОЛЬКО index 27 в whattoload[] = Finale1.ucm
- VIOLENCE=FALSE для туториалов: FTutor1(1), Album1(31), Gangorder1(32) по индексам в whattoload[]
- game_attract_mode() в Attract.cpp = главный frontend loop (НЕ "attract" demo); старый demo полностью закомментирован
- level_won()/level_lost() в Attract.cpp = #if 0 dead code; победа/поражение → только через GAMEMENU
- Thing.cpp per-frame: StateFn → general_process_person → PCOM_process_person (для CLASS_PERSON)
- people_functions[]: Darci→darci_states, Roper→roper_states, ВСЕ остальные (Cop,Thug,MIB...) → cop_states
- darci_states: только 7 записей; STATE_NORMAL=NULL (игрок), прыжки/бой через generic_people_functions
- plant_feet() активно ТОЛЬКО в fn_person_dangling (SUB_STATE_DROP_DOWN_LAND, финальная анимация)
- В fn_person_jumping plant_feet() везде закомментирована — Y задаётся из PAP/projectile_move_thing
- set_person_drop_down() флаги: KEEP_VEL, KEEP_DY, QUEUED, OFF_FACE; DY=-(4<<8)=-1024, Vel=-8 if not KEEP
- JUMPING→LAND последовательность: RUNNING_JUMP → RUNNING_JUMP_FLY → (hit==1) → RUNNING_JUMP_LAND_FAST → STATE_MOVEING+RUNNING
- EWAY_COND_PRESSURE = always FALSE (stub), EWAY_COND_CAMERA = always FALSE (stub)
- PSX EWAY_process: step=2 (odd/even frames), PC: step=1 (all waypoints each frame)
- EWAY_STAY_ALWAYS → immediately EWAY_FLAG_DEAD (fire-once semantics)
- zone byte in EWAY_create_enemy: bits 0-3=zone, bit4=INVULNERABLE, bit5=GUILTY, bit6=FAKE_WANDER
- Roper stats bug (пре-релиз): EWAY_create_player(ROPER) использует Darci stats
- EWAY вэйпойнт coords = mapsquare units (×256 для WorldPos)
- cam.cpp = МЁРТВЫЙ КОД (`#ifdef DOG_POO` никогда не определён); FC only
- FC_alter_for_pos idle height check (`&& 0`) = МЁРТВЫЙ КОД — always dheight=0, ddist=256
- FC_process: 8-шаговый raycast collision (want→focus); push xforce/yforce/zforce<<4 к want_pos
- FC get-behind speed: vehicle>>5; entering>>3; у стены>>5+>>6; нормально>>3; gun-out добавляет >>4
- FC toonear_dist=0x90000 = first-person mode (специальный случай)
- Lens = всегда 0x28000*CAM_MORE_IN (FOV не меняется, не зависит от action)
- PCOM_process_default: SEARCHING=пусто(stub), SLEEPING=пусто(stub), LOOKAROUND=только счётчик
- MIB: каждый кадр can_a_see_b(me,Darci) → PCOM_alert_nearby_mib_to_attack (MIB+GUARD+GANG+FIGHT_TEST в radius 1280 → NAVTOKILL)
- Bane (PCOM_AI_BANE): всегда в SUMMON; левитирует 4 тела+электродуги; if Darci стоит рядом ~2с → -25hp + SPARK; Bane не двигается
- DRIVER/COP_DRIVER NORMAL: if !FLAG_PERSON_DRIVING → FINDCAR; COP_DRIVER arrest-from-car = закомментирован
- Resurrect bug пре-релиз: newpos(HomeX,HomeZ) вычисляется но НЕ присваивается → гражданские воскресают на месте смерти
- PCOM_process_normal LAZY: каждые 0x3f тиков ищет bench/sofa в radius 512; GUARD на домашней позиции рисует пистолет
- KILLING→FLEE: ТОЛЬКО для FAKE_WANDER NPC (Rasta/Cop) через PCOM_should_fake_person_attack_darci(); обычные враги из KILLING→NAVTOKILL(ушёл) или NORMAL(мёртв)
- PCOM_process_snipe: stationary only; LOOK→AIMING(draw gun, shoot if dist<0x600)→NOMOREAMMO; "put gun away after 10s" закомментирован
- pcom_zone: PAP_Hi.Flags bits 10-13 = 4-bit zone bitmask; NPC с zone!=0 игнорируют видимость/звуки/NAVTOKILL из других зон
- WARE_mav_enter: НЕ меняет MAV_nav; глобальный MAV_do() к door[best].out_x/z; dist==0 → GOTO к in_x/z
- WARE_mav_inside/exit: swap MAV_nav→WARE_nav[ww->nav]; coords в warehouse-local; exit добавляет height check (|h_in-h_out|<0x80)
- INSIDE2_mav_inside=ASSERT(0) (заглушка); INSIDE2_mav_stair/exit=баг `>` вместо `>>` → GOTO (0/1, 0/1)
- INSIDE2_mav_nav_calc: баг z-цикла (MinX/MaxX вместо MinZ/MaxZ) → nav-сетка некорректна
- MAV_turn_movement_on: устанавливает ТОЛЬКО MAV_CAPS_GOTO (остальные caps теряются!); взрывы стен НЕ обновляют MAV
- MAV_opt[0..15]: первые 16 записей = 4-bit bitmask (GOTO в каждом направлении) — для INSIDE2 nav
- WMOVE = moving walkable faces (wmove.cpp); персонажи стоят на крышах машин; PSX: только VAN/WILDCATVAN/AMBULANCE
- mount_ladder() пре-релиз баг: вызов из interfac.cpp закомментирован → игрок НЕ может залезть снизу; AI может (pcom.cpp:12549)
- water: PAP_FLAG_WATER = invisible collision walls по краям; нет плавания, нет утопания; огонь гасится
- PAP_Lo.water = -16 при загрузке (water_level=-128 >> 3); OB_height_fiddle_de_dee() = /* */ МЁРТВЫЙ КОД; объекты в воде ставятся y=water_level-miny только при загрузке
- stop_movement_between(): water→non-water → invisible DFacet стена (активный код); PAP_FLAG_SINK_POINT на водяной+3 соседних → для лужи/az
- FRONTEND FE_BACK=-2(pop stack), FE_MAINMENU=reset stackpos=0, mode≥100=mission brief (mode-100)
- FRONTEND_MissionHierarchy: menu_theme по complete_point, ANNOYING_HACK_FOR_SIMON активен, suggest_order[]→auto-select следующая миссия
- mission script формат v4: ObjID:GroupID:ParentID:ParentIsGroup:Type:Flags:District:fn:title:brief; @=\r в brief
- mission_hierarchy[ObjID] биты: 1=exists, 2=complete, 4=available; complete_point≥40 → принудительно flag|=2
- collide_against_objects(): prim IDs 89,95,101,102,105,110,126 = скамейки → sit_down при SUB_STATE_WALKING_BACKWARDS
- VEH_find_runover_things(): 2 сферы вперёд; infront = 512-WheelAngle*3 clamped [256,512]
- Per-frame game_loop: GAMEMENU→tutorial→controls→process_things→EWAY→FC→draw→OVERLAY→flip→lock_fps→sfx→GAME_TURN++
- Bench cooldown: GAME_TURN & 0x3ff == 314 → сброс GF_DISABLE_BENCH_HEALTH (~34с при 30fps)
- SMOOTH_TICK_RATIO = скользящее среднее TICK_RATIO по 4 кадрам (для машин)
- DarciDeadCivWarnings: 0/1/2=предупреждения (экран deadcivs.tga); >=3=GS_LEVEL_LOST; персистирует!
- apply_button_input(): ACTION→do_an_action(); нет ACTION→SPRINT→RUN; find_best_action_from_tree()→REQUEST flags + jump/shoot/flip/hug/skid; без движения: IDLE→turn, MOVEING→stop
- .txc = FileClump archive: [ULONG MaxID][Offsets[MaxID]][Lengths[MaxID]][data]; ⚠️ size_t=4/8б зависит от платформы; для новой игры читать TGA напрямую
- .wag layout: var-string+CRLF | complete_point(1) | version(1) | [v≥1: 9б stats] | [v≥2: 60б hierarchy] | [v≥3: 200б best_found]
- FRONTEND_SaveString: strlen(txt) байт + CRLF{13,10}; НЕ фиксированные 32 байта!
- mission_hierarchy биты: 1=exists, 2=complete, 4=available; [1]=3 (корень) при старте
- best_found[50][4]: [0]=Constitution, [1]=Strength, [2]=Stamina, [3]=Skill; анти-фарм: повторное прохождение даёт только улучшение рекорда
- complete_point пороги тем: <8→theme0, <16→theme1, <24→theme2, ≥24→theme3; max чит = 40
- Банки колы = DIRT объекты (НЕ SPECIAL!): TYPE_CAN/HELDCAN/THROWCAN; prob_can=1 из 101 → редко (~2-3 из 256 dirt slots); есть в финале PS1; ПЕРЕНОСИТЬ
- Банка: подбор через do_an_action() dist<0x80 idle Darci; бросок через PUNCH→player_activate_in_hand(); физика: гравитация+отскок; S_KICK_CAN звук+PCOM_SOUND_UNUSUAL (NPC alert)
- DIRT_create_cans(): 5 банок при столкновении бочек (barrel.cpp:1167)
- pigeon (голубь) = ВСЕГДА prob_pigeon=0 в DIRT_init (принудительно отключён)
- MapElement.Colour = МЁРТВЫЙ КОД в DDEngine (PC); MAP_light_set/get нигде не вызываются; Glide-only legacy
- DDEngine terrain vertex colour: NIGHT_light_mapsquare → colour[16] per lo-cell → NIGHT_cache → NIGHT_get_d3d_colour (×4 scale) → apply_cloud → fadeout
- ⚠️ БАГ в NIGHT_light_mapsquare:746: dprod=`dx*nx+dy*ny+dz*nx` — последний `dz*nx` должен быть `dz*nz`
- POLY_frame_draw: opaque страницы в порядке с COLOUR_PAGE первым; alpha → bucket sort [2048] sort_z=ZCLIP/vz → FAR→NEAR; PUDDLE и SHADOW — отдельно
- style.tma: 200 стилей × 5 слотов; TXTY{Page,Tx,Ty,Flip}=4б; textures_flags[200][5]=тип полигона; instyle.tma=count_x*count_y UBYTE индексов
- build_quick_city(): DFacet→PAP_Lo.ColVectHead (collision), roof_faces4→PAP_Lo.Walkable (negative index = roof face); положительные face indexes (prim_faces4) закомментированы
- ID_generate_floorplan(): 32 seeds (ID_MAX_FITS) если find_good_layout; num_walls=floor_area/16+1; apartments /8+2 с фикс. стартами у лестниц
- ID_find_rooms(): BFS queue=64; блокируется WALL_XL/XS/ZL/ZS; каждая связная компонента = room ID (1-based)
- ID_score_layout_house_ground(): ratio = max/min *256; цель=414 (золотое сечение *256); score+=(150+(414-ratio))*10; прям+1000; одна связь→*3/4; лестница в туалете → -score/2
- ID_assign_room_types(): bubble sort по размеру; HOUSE: biggest=LOUNGE, door-adj=LOBBY, 1=LOO, 2=KITCHEN, 3=DINING; WAREHOUSE: biggest=WAREHOUSE, door-adj=LOBBY, 1=LOO, middle=OFFICE, largest-rest=MEETING
- STAIR_calculate scoring: base=rand(0xffff); both-outside=+0x10000; one-outside=-0x10000; corner=+0x5000 per coord; opposite-hint=+0xbabe per cell; blocks-door=-INF
- WARE_Ware: door[4]{out_x/z, in_x/z}; nav_pitch=bz2-bz1; index=(x-minx)*nav_pitch+(z-minz); OB inside if y+maxy < MAVHEIGHT*64-0x20
- WARE_init(): loads .map (iam→map extension swap) for rooftop textures; MAV_calc_height_array(TRUE/FALSE) wraps the init; door prims #if 0 (disabled)
- ROAD_wander_calc(): граф для авто-AI; ROAD_Node{x,z:UBYTE, c[4]:UBYTE}=8б; MAX 256 нодов; сканирует ROAD_is_middle=5×5 квадрат дороги; ROAD_LANE=0x100; хардкод (121,33) блокирован для gpost3.iam
- ROAD_is_road: PAP_Hi.Texture 323-356 PC / 256-278 PSX / TextureSet7-8: 35,36,39
- WAND_init(): PAP_FLAG_WANDER = ≤2 кл. от дороги (не HIDDEN); зебры (tex 333/334); -prim collision y≤terrain+64 снимает флаг
- WAND_get_next_place(): 4 направления ±1 случайный сдвиг; scoring = dot product с текущим направлением; fallback = 16 проб ±15 кл.
- POW = ТОЛЬКО визуальные взрывы (спрайты), НЕ урон/shockwave; урон в PYRO_blast_radius
- POW_TICKS_PER_SECOND=2000; POW_DELTA_SHIFT=4; sprite dead at frame>=16; safety: POW_init() on overflow
- POW_create() API: LARGE_SEMI(0)→MULTISPAWN_SEMI, MEDIUM(1)→SPAWN_MEDIUM, LARGE(2)→MULTISPAWN
- PYRO_construct bitmask: &1=TWANGER &2=EXPLODE2+gust &4=DUST &8=STREAMER &16=BONFIRE &32=NEWDOME &64=FIREBOMB &128=FIREBOMB+WAVE
- PYRO IMMOLATE: -10HP/frame на victim; ribbons на костях; не убивает INVULNERABLE/PLAYERKILL
- PYRO GAMEOVER: 8 dlights по кругу; counter>242 → GS_LEVEL_LOST + knock_person_down Darci
- PYRO HITSPANG: max global_spang_count=5; person target=blood(HEAD); non-person=sparks
- DIRT pigeon = ВСЕГДА prob_pigeon=0 (hardcoded line 193, "no bug ridden pigeons for us!")
- DIRT_find_useless(): 8×UNUSED/DELETE_OK → 8×LEAF → random (recycling priority)
- DIRT THROWCAN→CAN при малом dy (<10<<TICK_SHIFT); S_KICK_CAN + PCOM_SOUND_UNUSUAL при bounce
- DIRT_create_brass: prim 253 = ejected shell (PC only)
- DIRT HELDCAN/HELDHEAD: droll field = owner thing index (tracks SUB_OBJECT_PREFERRED_HAND)
- InterruptFrame в DrawTween = МЁРТВЫЙ КОД: всегда NULL, нигде не присваивается ненулевое значение в пре-релизе
- people_functions[]: ALL THUG (Rasta/Grey/Red) → cop_states → fn_cop_init (НЕ fn_thug_init!); fn_thug_init с ASSERT(0) никогда не вызывается
- fn_roper_normal() = пустая функция (только return); fn_cop_normal() = #if 0 мёртвый код; весь NPC AI через PCOM
- Crinkle = микро-геометрический bump mapping; отключён в пре-релизе (CRINKLE_load→NULL, aeng.cpp if(0)), но **РАБОТАЕТ в финальном PC релизе** (bump на деревянных ящиках); переносить через normal/parallax mapping
- NIGHT_generate_walkable_lighting() = МЁРТВЫЙ КОД (`return;` после roof_walkable); только NIGHT_generate_roof_walkable() реально работает
- MAV_can_i_walk: шаг ~0x40 субпикс, проверяет CAPS_GOTO на каждом ребре; диагональ → +2 угловые ячейки; только GOTO (не jump/climb)
- Крыши в MAV: PAP_FLAG_HIDDEN + roof_face → MAVHEIGHT=roof_face.Y/0x40; без roof_face → PAP_FLAG_NOGO; скайлайты (prim 226/227) убраны из nav
- CMatrix33 = {SLONG M[3]}: каждый M[row] packed 3×10-bit elements; scale=128 (не 32768)
- GameKeyFrameElement (ULTRA_COMPRESSED) = 8 байт: {m00,m01,m10,m11: SBYTE; OffX,OffY,OffZ: SBYTE; Pad: UBYTE}
- GetCMatrix(): UCA_Lookup[a][b]=Root(16383-a²-b²); Pad bits 0-1=знак c; bits 2-3=позиция c в строке0; bits 4-5=позиция c в строке1; строка2=crossproduct>>7, clamp±127
- **ВЕРИФИКАЦИЯ:** Darci fall damage: `(-DY-20000)/100`, cap 250; death plunge Player=-12000 / NPC=-6000
- **ВЕРИФИКАЦИЯ:** Pistol=70hp, Shotgun=300-dist, AK47=100(player)/40(NPC); fire rates 140/400/64 ticks
- **ВЕРИФИКАЦИЯ:** Hit chance: `230 - abs(Roll)>>1 - Velocity + mods`; min 20/256
- **ВЕРИФИКАЦИЯ:** Vehicle friction bit-shift: `(vel<<f - vel)>>f`; terminal velocity emergent
- **ВЕРИФИКАЦИЯ:** ⚠️ drawxtra.cpp DRAWXTRA_MIB_destruct() мутирует ammo из рендерера → вынести
- **ВЕРИФИКАЦИЯ:** Water.cpp = полная динамическая вода (1024 points, gushing), НЕ "просто стены"
- **ВЕРИФИКАЦИЯ:** trip.cpp = растяжки (32 wires, 3D line collision, debounce 4f) — активная механика
- **РЕШЕНО:** Balloon.cpp — фича ВЫРЕЗАНА. load_prim_object(PRIM_OBJ_BALLOON) закомментировано (ob.cpp:757), меш не грузится. NO_MORE_HAPPY_BALOONS guard + нет шариков в PC лонгплее. Не переносить.
- **ВЕРИФИКАЦИЯ:** puddle.cpp = лужи на земле, **PC only** (пользователь подтвердил: есть в PC, нет в PS1). Переносить.
- **ВЕРИФИКАЦИЯ:** memory.cpp save_table = 67 entries; MAX_THINGS=256, RMAX_PEOPLE=1024, EWAY_MAX=512
