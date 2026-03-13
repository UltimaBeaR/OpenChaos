# Quick Facts — Urban Chaos Ключевые Числа и Факты

Компактный справочник критичных констант, формул, флагов и особенностей.
**Подробности** — в subsystems/*.md. Этот файл = только суть, без объяснений.

---

## Игровой цикл и время

- Per-frame порядок: `GAMEMENU → tutorial → controls → process_things → EWAY → FC → draw → OVERLAY → flip → lock_fps → sfx → GAME_TURN++`
- Lock fps = 30 (lock_frame_rate)
- PCOM_TICKS_PER_SEC = 320 (16 тиков/кадр × 20 кадров/сек)
- POW_TICKS_PER_SECOND = 2000
- Bench cooldown: GAME_TURN & 0x3ff == 314 → сброс GF_DISABLE_BENCH_HEALTH (~34с при 30fps)
- SMOOTH_TICK_RATIO = скользящее среднее TICK_RATIO по 4 кадрам (для машин)
- GAMEMENU wait delay: 64/кадр, порог 2560 → WON/LOST меню (~40 кадров, ~1.3с)
- TIMEOUT_DEMO=0 → demo_timeout() = мёртвый код
- DoubleClick: GetTickCount() (WIN32 **мс**!); окно 200мс; не GAME_TURN

## Карта и физика

- MAP = 128×128 mapsquares, MAP_SIZE = 128×256 = 32768 world-units per axis
- PAP_Hi 128×128 (hi-res), PAP_Lo 32×32 (lo-res)
- water_level = -128 хардкод; PAP_Lo.water = -16 при загрузке (-128>>3)
- PAP_LO_NO_WATER = -127
- GRAVITY = -51 (units/frame²); НЕ -5120!
- Darci fall damage: `(-DY - 20000) / 100`; death plunge Player=-12000 / NPC=-6000
- HyperMatter: `gy=0` hardcoded — объекты отскакивают от Y=0, не от реального terrain
- NOGO push = 0x5800; SLIDE_ALONG_DEFAULT_EXTRA_WALL_HEIGHT = -0x50
- water: PAP_FLAG_WATER = invisible collision walls; нет плавания, нет утопания; огонь гасится
- WMOVE: max=192 движущихся поверхностей; PSX: только VAN/WILDCATVAN/AMBULANCE
- Ночью всегда дождь: hardcoded `if (!(NIGHT_FLAG_DAYTIME)) GAME_FLAGS |= GF_RAINING`

## Урон и бой

- Pistol = 70hp урона; fire rate = 140 тиков (≈2.3с между выстрелами)
- Shotgun = 300-dist; fire rate = 400 тиков
- AK47 = 100hp (player) / 40hp (NPC); fire rate = 64 тиков
- Grenade explosion radius = 0x300 (768 units); шарит в шаре
- C4 = 5 секунд задержки (не 10 как в комментарии кода!)
- Hit chance: `230 - abs(Roll)>>1 - Velocity + modifiers`; min 20/256 (~8%)
- PYRO IMMOLATE: -10HP/frame; GAMEOVER: 8 dlights, counter>242 → GS_LEVEL_LOST
- PYRO HITSPANG: max global_spang_count=5
- MIB Health=700, KO невозможен (FLAG2_PERSON_INVULNERABLE)
- Roper Health=400 (в 2× больше Darci=200)
- PCOM_BENT_LAZY: ищет скамейку каждые 0x3f тиков в radius 512
- KILLING→FLEE: ТОЛЬКО для FAKE_WANDER NPC (Rasta/Cop); обычные враги → NAVTOKILL/NORMAL

## AI и навигация

- PCOM_AI: 22 типа (0-21); PCOM_STATE: 28 состояний (0-27)
- SEARCHING/SLEEPING = STUB (пустой), LOOKAROUND = только счётчик
- MIB: can_a_see_b каждый кадр; mass aggro radius=1280 (MIB+GUARD+GANG+FIGHT_TEST)
- PCOM_ARRIVE_DIST = 0x40 = 64 units
- RMAX_PEOPLE = 180; MAX_THINGS = 700 (memory.cpp save_table = 67 entries, но здесь MAX_THINGS=700)
- Гражданские воскресают через >200 кадров без LOS; баг пре-релиза: на месте смерти (не home)
- MAV: greedy best-first (НЕ A*); горизонт 32; шаг ~0x40 субпикселей
- MAV_turn_movement_on: ТОЛЬКО GOTO caps (остальные теряются!)
- Крыши в MAV: MAVHEIGHT=roof_face.Y/0x40; без roof_face → NOGO
- pcom_zone: PAP_Hi.Flags bits 10-13 = 4-bit bitmask; zone!=0 → фильтр видения/звука/NAVTOKILL
- zone byte в EWAY_create_enemy: bits 0-3=zone, bit4=INVULNERABLE, bit5=GUILTY, bit6=FAKE_WANDER
- Resurrect bug пре-релиз: newpos вычисляется но НЕ присваивается → воскрес на месте смерти
- NAV: wallhug; NAV_Waypoint pool max=512
- WARE_mav_enter: не меняет MAV_nav; WARE_mav_inside/exit: swap MAV_nav→WARE_nav
- INSIDE2_mav_inside=ASSERT(0); INSIDE2_mav_nav_calc: баг z-цикла (MinX/MaxX вместо MinZ/MaxZ)

## Персонажи и анимация

- people_functions[]: Darci→darci_states, Roper→roper_states, ВСЕ NPC (Cop,Thug,MIB...) → cop_states
- darci_states: только 7 записей; STATE_NORMAL=NULL
- fn_cop_normal() = #if 0; fn_thug_init() = ASSERT(0); fn_roper_normal() = пустая
- plant_feet() активно ТОЛЬКО в fn_person_dangling (SUB_STATE_DROP_DOWN_LAND)
- mount_ladder() пре-релиз: вызов из interfac.cpp закомментирован → игрок НЕ может залезть снизу; AI может
- InterruptFrame = МЁРТВЫЙ КОД: всегда 0
- CMatrix33 = {SLONG M[3]}: каждый M[row] packed 3×10-bit; scale=128 (не 32768)
- GameKeyFrameElement (ULTRA_COMPRESSED) = 8 байт: {m00,m01,m10,m11:SBYTE; OffX,OffY,OffZ:SBYTE; Pad:UBYTE}
- JUMPING→LAND: RUNNING_JUMP → RUNNING_JUMP_FLY → (hit==1) → RUNNING_JUMP_LAND_FAST → STATE_MOVEING+RUNNING
- set_person_drop_down(): DY=-(4<<8)=-1024, Velocity=-8 if not KEEP_VEL

## Управление и ввод

- process_controls() = диспетчер подсистем (НЕ обработка ввода игрока!)
- InputDone: накапливает обработанные биты; кнопка срабатывает один раз на нажатие
- player_turn_left_right(): wMaxTurn=94 idle / 12 в воздухе / (70-Velocity) при беге
- should_i_jump(): dx/dz захардкожены (не зависят от угла персонажа — баг/упрощение!)
- Горячие клавиши PC: KB_1=убрать, KB_2=пистолет, KB_3=шотган, KB_4=AK47, KB_5=граната, KB_6=C4, KB_7=нож, KB_8=бита
- NOISE_TOLERANCE: PC=8192 (из 65535), DC=24; ANALOGUE_MIN_VELOCITY: PC=8, PSX=32 (из 128)
- apply_button_input(): ACTION→do_an_action(); нет ACTION→SPRINT→RUN
- do_an_action() приоритеты: выйти из машины > арест > слезть с лестницы > сесть в машину > крюк > переключатель > разговор > подбор > обыск > обнять стену > кола > присесть

## Транспорт и машины

- VEH_find_runover_things(): 2 сферы вперёд; infront = 512-WheelAngle×3 clamped [256,512]
- Vehicle friction bit-shift: `(vel<<f - vel)>>f`; terminal velocity emergent
- FLAG_FURN_DRIVING определён в Furn.h (не Vehicle.h!)
- 4 пружины подвески; 6 зон урона; мотоцикл не в финале (подтверждено Mike Diskett)
- WMOVE: персонажи стоят на крышах машин (PSX: только 3 типа)

## Миссии и EWAY

- EWAY: 42 условия (COND 0-41), 52 действия (DO 0-52), polling каждый кадр
- EWAY_COND_PRESSURE = always FALSE; EWAY_COND_CAMERA = always FALSE (stubs)
- PSX EWAY_process: step=2 (odd/even frames); PC: step=1
- EWAY_STAY_ALWAYS → immediately EWAY_FLAG_DEAD (fire-once semantics)
- EWAY вэйпойнт coords = mapsquare units (×256 для WorldPos = <<8)
- EWAY_counter[7] = счётчик убитых копов (cap 255)
- WPT_BONUS_POINTS → EWAY_DO_MESSAGE (очки = мёртвый код if(0))
- WPT_GOTHERE_DOTHIS (39) = ASSERT(0) не реализован в пре-релизе
- GROUP_LIFE/DEATH иммунны к себе — только другие WP той же colour+group

## Win/Lose и Frontend

- DarciDeadCivWarnings >= 3 → GS_LEVEL_WON превращается в GS_LEVEL_LOST! Персистирует!
- CRIME_RATE delta: kill guilty=-2, kill wander_civ=+5, arrest guilty=-4
- park2.ucm → cutscene 1 (MIB intro); Finale1.ucm → cutscene 3 + OS_hack()
- this_level_has_bane = ТОЛЬКО index 27 в whattoload[] = Finale1.ucm
- VIOLENCE=FALSE для туториалов: FTutor1(1), Album1(31), Gangorder1(32)
- DONT_load = 0 жёстко → per-mission dontload bitmasks игнорируются
- STARTSCR_mission flow: FE_MAPSCREEN+ENTER → mode=100+N → FE_START → STARTSCR_mission set → STARTS_START → GS_PLAY_GAME → elev.cpp → *STARTSCR_mission=0
- FRONTEND_level_won(): hierarchy bit 2, best_found анти-фарм, complete_point++, FE_SAVESCREEN
- FRONTEND_level_lost(): mission_launch=previous_mission_launch (restore), FE_MAINMENU
- ScoresDraw(): killed/arrested/at-large/bonus found/missed/time; mucky times DC активна (PC в #if 0)
- complete_point пороги тем: <8→0, <16→1, <24→2, ≥24→3; max чит = 40
- mission_hierarchy биты: 1=exists, 2=complete, 4=available; [1]=3 (корень) при старте
- best_found[50][4]: [0]=Constitution [1]=Strength [2]=Stamina [3]=Skill; анти-фарм
- save slots: 1-based! (`save_slot = menu_state.selected + 1`)
- ANNOYING_HACK_FOR_SIMON активен (policeID требует fight1+assault1+testdrive1a)

## Форматы файлов

- .wag layout: var-string+CRLF | complete_point(1) | version(1) | [v≥1: 9б stats] | [v≥2: 60б hierarchy] | [v≥3: 200б best_found]
- FRONTEND_SaveString: strlen(txt) байт + CRLF{13,10}; НЕ фиксированные 32 байта!
- EventPoint binary = 74 байта (14 заголовок + 60 данных); MAX_EVENTPOINTS=512 → 37888 байт
- .ucm header: 4(version)+4(flags)+260×5(names)+2+2+2(MapIndex/Used/Free)+2(CrimeRate+FakeCivs) = 1316б; потом 512×74б EventPoints; потом версионный хвост
- .ucm версионный хвост: skip 254б SkillLevels(if ver>5), read 1б BOREDOM_RATE, read 1+1б CarsRate+MusicWorld(if ver>8), messages section
- .txc = FileClump archive: [ULONG MaxID][Offsets[MaxID]][Lengths[MaxID]][data]; ⚠️ size_t платформозависим
- .all pointer fixup: сохранённые NextFrame/PrevFrame = runtime адреса; при загрузке пересчитываются
- sizeof(ED_Light) = **20 байт** (8 однобайт.полей + 3 SLONG)
- sizeof(PrimFace3) = **28 байт** (1+1+6+6+6+2+2+2+1+1)
- sizeof(PrimFace4) = **34 байта** (1+1+8+8+8(union)+2+2+2+1+1)
- Карты уровней: расширение `.iam` (не .pam!); скрипты миссий `.ucm` — в `levels/`
- ROAD_is_road: PAP_Hi.Texture 323-356 (PC) / 256-278 (PSX)
- style.tma: 200 стилей × 5 слотов; TXTY{Page,Tx,Ty,Flip}=4б; instyle.tma=count_x*count_y UBYTE

## Освещение и рендеринг

- NIGHT_Colour диапазон 0-63 (не 0-255!)
- ambient direction везде = (110, -148, -177) — захардкожена
- ⚠️ БАГ night.cpp:746: dprod=`dz*nx` вместо `dz*nz`
- POLY_frame_draw: bucket sort [2048] sort_z=ZCLIP/vz → FAR→NEAR; PUDDLE и SHADOW — отдельно
- MapElement.Colour = МЁРТВЫЙ КОД в DDEngine (PC); MAP_light_set/get нигде не вызываются
- Crinkle (bump mapping): отключён в пре-релизе (CRINKLE_load→NULL), но работает в финальном PC
- NIGHT_generate_walkable_lighting() = МЁРТВЫЙ КОД (`return;` в начале); только roof_walkable активна
- DRAWXTRA_MIB_destruct() мутирует ammo_packs_pistol/PYRO/SPARK из рендерера (drawxtra.cpp) → вынести!
- FC toonear=0x90000 = first-person mode; Lens = всегда 0x28000*CAM_MORE_IN (FOV не меняется)

## Эффекты

- RIBBON: circular buffer trail; MAX_RIBBONS=64, MAX_RIBBON_SIZE=32; CONVECT=+22 Y/frame
- BANG: BIG→MIDDLE→NEARLY→END; BANG_Phwoar[4096]; урон НЕ отсюда
- DIRT pigeon = ВСЕГДА prob_pigeon=0 (hardcoded "no bug ridden pigeons for us!")
- Банки колы = DIRT объекты (TYPE_CAN/HELDCAN/THROWCAN); prob_can=1/101; есть в PS1
- DIRT_create_brass: prim 253 = ejected shell (PC only)
- build_quick_city(): DFacet→PAP_Lo.ColVectHead; roof_faces4→PAP_Lo.Walkable

## Вырезанное / Пре-релизные особенности

- bat.cpp = Bane AI (главный антагонист), НЕ летучие мыши
- Собаки (canid.cpp) = инертны в пре-релизе (dispatch switch закомментирован)
- Мотоцикл (BIKE) = не в финальной игре (подтверждено Mike Diskett)
- Zipwire (тросы) = ЕСТЬ в финальной игре (первый уровень)
- Гарпун = не в финале; Канализация = не в финале; Balloon.cpp = ВЫРЕЗАН
- cam.cpp = МЁРТВЫЙ КОД (`#ifdef DOG_POO`); только fc.cpp активен
- briefing.cpp = МЁРТВЫЙ КОД (Dec 1998 прототип); OBEY_SCRIPT закомментирован
- CRIME_RATE: если 0 → дефолт 50; CRIME_RATE delta через Person.cpp
- puddle.cpp = лужи, PC only (подтверждено пользователем); переносить
- Balloon.cpp: load_prim_object(PRIM_OBJ_BALLOON) закомментировано → не грузится
- Switch.cpp: sphere proximity detect работает, group/class = стабы
- lead.cpp: верёвка/поводок НЕДОДЕЛАН; nd.cpp = ПУСТОЙ файл; morph.cpp = НЕ используется
- SPECIAL_MINE подбор = ASSERT(0), метание = ASSERT(0)
- OB_height_fiddle_de_dee() в ob.cpp = /* */ МЁРТВЫЙ КОД
- is_semtex = (index==20) = skymiss2.ucm (вероятно ошибка в пре-релизе)
- Roper stats bug: EWAY_create_player(ROPER) использует Darci stats (Roper блок закомментирован)
- fn_cop_normal() = #if 0 в пре-релизе; fn_thug_init() = ASSERT(0) в пре-релизе
- COP_DRIVER arrest-from-car = закомментирован (/* ... */)
- STARTSCR_notify_gameover() закомментирован в Game.cpp → auto_advance никогда не ставится
- valid_grab_angle() = ALWAYS 1 (валидация отключена в пре-релизе)
- FC_alter_for_pos idle height check (`&& 0`) = МЁРТВЫЙ КОД
- PRIM_START_SAVE_TYPE=5793: если save_type==base+1 → PrimPoint как SWORD (6б), иначе SLONG (12б)
- PrimFace4 FACE_FLAG_WALKABLE=(1<<6) — только эти квады образуют ходимые поверхности
- DrawMesh.Angle = 0-2047 (полные 360°)
