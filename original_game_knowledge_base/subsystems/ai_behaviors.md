# AI — Детальные реализации поведений

**Связанные файлы:** [ai.md](ai.md) (PCOM ядро), [ai_structures.md](ai_structures.md) (структуры данных)

---

## 10b. MIB AI детали

**PCOM_AI_MIB (18)** — самый агрессивный AI, всегда убивает игрока:

```
PCOM_process_state_change → case PCOM_AI_MIB:
    if pcom_ai_state == NORMAL:
        if can_a_see_b(me, Darci) && !stealth_debug:
            PCOM_alert_nearby_mib_to_attack(me)  // mass aggro
    PCOM_process_default(me)  // обычные состояния
```

**PCOM_alert_nearby_mib_to_attack()** — массовый aggro trigger (pcom.cpp:10825):
- `THING_find_sphere(radius=0x500=1280 units)` — все персонажи поблизости
- Для каждого NPC с pcom_ai ∈ {MIB, GUARD, GANG, FIGHT_TEST}:
  - Если NOT HELPLESS и в состоянии NORMAL/HOMESICK/INVESTIGATING:
  - → `PCOM_set_person_ai_navtokill(found, Darci)` (немедленно начинает преследование)

**Ключевые отличия MIB от других AI:**
- Не ждёт звука/события — проверяет can_a_see_b каждый кадр в NORMAL state
- Групповой aggro: один MIB увидел → все MIB/GUARD/GANG/FIGHT_TEST в радиусе 1280 атакуют
- Нет агрессивного порога — убивает всегда (не как COP/GANG с условиями)
- KO невозможен (Health=700, FLAG2_PERSON_INVULNERABLE)

---

## 10c. Bane AI детали

**PCOM_AI_BANE (19)** — всегда в состоянии SUMMON:

```
case PCOM_AI_BANE:
    if pcom_ai_state != SUMMON: PCOM_set_person_ai_summon(me)
    PCOM_process_summon(me)
```

**PCOM_process_summon()** — 2 подсостояния (pcom.cpp:5280):

**SUMMON_START:** ждёт анимацию SUB_STATE_FLOAT_BOB, затем:
- Поднимает до 4 тел (PCOM_summon[0..3]) в воздух (`set_person_float_up()`)
- Создаёт SPARK электродуги от конечностей Bane к телам (лимбы: L_HAND, R_HAND, L_FOOT, R_FOOT → таз жертвы)
- Переходит в SUMMON_FLOAT

**SUMMON_FLOAT:** каждые `PCOM_get_duration(50)=160 тиков` пересоздаёт SPARK дуги.

Близость Darci → электрошок:
- `dx+dz < 0x60000` (Darci рядом) → `pcom_move_counter` растёт
- Если Darci смещается → `pcom_move_counter >>= 1` (сброс при движении)
- `pcom_move_counter >= PCOM_get_duration(20)=640 тиков (~2 сек на месте)` → electrocute:
  - `set_person_recoil(Darci, ANIM_HIT_FRONT_MID, 0)` + `Health -= 25`
  - SPARK дуга Bane.PELVIS → Darci.PELVIS (intensity=50)
  - `pcom_move_counter = 0` (сброс)

**Вывод:** Bane не атакует напрямую — он статичен, поднимает тела, и если игрок стоит рядом 2 секунды — бьёт током.

---

## 10d. Driving AI детали

**PCOM_AI_DRIVER (9) / PCOM_AI_COP_DRIVER (14):**

COP_DRIVER → fallthrough в DRIVER логику.

```
NORMAL state:
    if !FLAG_PERSON_DRIVING:
        PCOM_set_person_ai_findcar(me, NULL)  // ищет любую машину
    else:
        dispatch по pcom_move:
            STILL  → PCOM_process_driving_still()
            PATROL/PATROL_RAND → PCOM_process_driving_patrol()
            WANDER → PCOM_process_driving_wander()
            FOLLOW → (пусто — не реализовано)
non-NORMAL states: PCOM_process_default()
```

**Важно:** логика COP_DRIVER (выйти из машины и арестовать) закомментирована (`/* ... */`).

**Driving movement states** (в PCOM_process_movement):
- `PCOM_MOVE_STATE_DRIVETO (8)` — едет к конкретной точке
- `PCOM_MOVE_STATE_DRIVE_DOWN (11)` — едет по дорожному графу (ROAD wander)
- Завершение DRIVE_DOWN → переход к следующему NAV-узлу дороги

---

## 10e. Воскрешение гражданских

(pcom.cpp:13104)

```c
if (pcom_ai == PCOM_AI_CIV && pcom_move == PCOM_MOVE_WANDER && !IN_VIEW):
    pcom_ai_counter++
    if pcom_ai_counter > 200:
        newpos = (HomeX<<16+0x8000, HomeZ<<16+0x8000, terrain_height)
        // ⚠️ БАГИ пре-релиза: newpos вычисляется но НЕ присваивается p_person->WorldPos!
        plant_feet(p_person)       // просто снижает к полу на ТЕКУЩЕЙ позиции
        Health = health[PersonType] // восстанавливает здоровье
        clear FLAGS_BURNING, BurnIndex = 0
        PCOM_set_person_ai_normal(p_person)
```

**Вывод:** в пре-релизной версии гражданские воскресают **на месте смерти**, а не на домашней позиции (teleport в newpos — мёртвый код). В финале может быть исправлено.

---

## 10f. PCOM_process_killing() — ближний бой

**FAKE_WANDER проверка** (каждые 4 тика):
- Если NPC с флагом FLAG2_PERSON_FAKE_WANDER (Thug Rasta или Cop):
  - `PCOM_should_fake_person_attack_darci()` → если false: `PCOM_set_person_ai_flee_person()` и выход
  - Это единственный путь из KILLING → FLEE напрямую

**Если цель мертва:**
- Если арестована и не SUB_STATE_DEAD_ARRESTED → ждём завершения анимации ареста
- Иначе → `PCOM_set_person_ai_normal()` (домой)

**Если цель в машине:**
- Есть пистолет → `PCOM_set_person_ai_navtokill()` (стреляет по машине)
- Нет пистолета → `PCOM_set_person_ai_taunt()` (только издевается)

**Каждые 2 тика — проверка дистанции:**
- Если цель НЕ в нокауте:
  - Если цель убегает (FLEE_PERSON): `too_far = 0x150` (~340 units — ближе держится)
  - Иначе: `too_far = 0x250` (~592 units)
  - Если дистанция > too_far ИЛИ нет LOS → `PCOM_set_person_ai_navtokill()` (догоняет)

**Каждые 64 тика:** агрессивный звук (`S_WTHUG1_ALERT_START`)
**Каждые 256 тиков (PSX: 128):** ищет оружие поблизости → `PCOM_set_person_ai_getitem()`

**Вывод:** KILLING НЕ переходит в FLEE по жизням. FLEE только для FAKE_WANDER NPC. Обычные враги: NORMAL (цель мертва) или NAVTOKILL (цель ушла).

---

## 10g. PCOM_process_snipe() — снайпер AI

**PCOM_AI_SHOOT_DEAD (21)** — статичный снайпер, не перемещается, только стреляет.

Если нет цели (`pcom_ai_arg == NULL`) → `PCOM_set_person_ai_normal()` и выход.

**3 подсостояния** (`pcom_ai_substate`):

**PCOM_AI_SUBSTATE_LOOK (ожидание цели):**
- Нет патронов → ничего не делает
- `can_a_see_b(me, target)` → если да: переход в AIMING, counter=0
- Код "убрать пистолет после 10 сек без цели" — **закомментирован**

**PCOM_AI_SUBSTATE_AIMING (прицеливается/стреляет):**
- Если пистолет не достан → `PCOM_set_person_move_draw_gun()`
- Если дистанция < `0x600` (≈1536 units) И `can_a_see_b`:
  - `shoot_time = get_rate_of_fire() - (SKILL × 4) / 100`
  - Если counter > shoot_time ИЛИ AI_type == PCOM_AI_SHOOT_DEAD:
    - Нет патронов → `set_person_shoot(click)` + убрать пистолет → NOMOREAMMO
    - Есть патроны → `PCOM_set_person_move_shoot()`, counter=0
  - Всегда: `set_face_thing(me, target)` — поворачивается к цели
- Если цель вышла из дистанции/LOS → назад в LOOK

**PCOM_AI_SUBSTATE_NOMOREAMMO (нет патронов):**
- Ждёт пока `pcom_move_state == STILL` (анимация уборки завершена)
- Затем → LOOK, counter=0

**Ключевые параметры:**
- Максимальная дистанция стрельбы: **0x600 units** (1536)
- Скорость стрельбы: `get_rate_of_fire(p_person) - GET_SKILL(p_person)×4/100`
- Снайпер НЕ двигается никогда (нет навигации)
- Стреляет мгновенно если `pcom_ai == PCOM_AI_SHOOT_DEAD` и aiming (счётчик игнорируется)

---

## 10h. pcom_zone — зональная изоляция AI

**Хранение зон в навмеше:**
- PAP_Hi.Flags bits 10-13 = 4-bit zone ID для каждого навмеш-квадрата
- `PCOM_get_zone_for_position(Thing*)` → читает зону из текущего/целевого навмеш-квадрата

**Person.pcom_zone — бitmask зон NPC:**
- Если `pcom_zone == 0` → NPC не ограничен зонами (реагирует на всё)
- Если `pcom_zone != 0` → NPC реагирует только на события в своих зонах

**Фильтрация видимости** (`PCOM_can_i_see_person_to_attack`):
```c
if (pcom_zone && !(PCOM_get_zone_for_position(target) & pcom_zone)):
    return NULL  // цель в другой зоне — игнорировать
```

**Фильтрация звуков** (в `PCOM_oscillate_tympanum`):
```c
if (p_found->pcom_zone && !(PCOM_get_zone_for_position(sound_x, sound_z) & p_found->pcom_zone)):
    continue  // звук из другой зоны — не слышать
```

**Фильтрация NAVTOKILL** (pcom.cpp:8669):
- Если цель ушла из зоны NPC → `PCOM_set_person_ai_homesick()` (вернуться домой)
- Если дом тоже не в зоне (level design ошибка) → `pcom_zone = 0` (сбросить ограничение)

**Вывод:** pcom_zone — 4-bit bitmask. Назначается через zone byte в EWAY_create_enemy (биты 0-3). Если дом NPC не в его же зоне — зона автоматически сбрасывается.
