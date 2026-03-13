# AI персонажей — Urban Chaos (PCOM система)

**Связанные файлы:**
- [ai_structures.md](ai_structures.md) — Person struct, типы персонажей, флаги
- [ai_behaviors.md](ai_behaviors.md) — детальные реализации MIB/Bane/Driving/Snipe/Zone
- [navigation.md](navigation.md) — MAV/NAV навигация
- [combat.md](combat.md) — боевая система

**Ключевые файлы кода:**
- `fallen/Headers/pcom.h` — типы AI, состояния, флаги
- `fallen/Source/Person.cpp` — AI логика (~22K строк)
- `fallen/Source/pcom.cpp` — PCOM обработчики

---

## 4. PCOM — система AI (Person Commands)

### Типы AI (PCOM_AI_*) — 22 типа (0-21)

```c
PCOM_AI_NONE          (0)   // Бездействует
PCOM_AI_CIV           (1)   // Гражданский (блуждает/патрулирует)
PCOM_AI_GUARD         (2)   // Охранник (защищает территорию)
PCOM_AI_ASSASIN       (3)   // Ассасин (убивает конкретного врага)
PCOM_AI_BOSS          (4)   // Босс (атакует после гибели группы)
PCOM_AI_COP           (5)   // Полицейский (арестовывает преступников)
PCOM_AI_GANG          (6)   // Банда (атакует при виде)
PCOM_AI_DOORMAN       (7)   // Охранник двери
PCOM_AI_BODYGUARD     (8)   // Телохранитель конкретного NPC
PCOM_AI_DRIVER        (9)   // Водитель (ищет машину, ездит)
PCOM_AI_BDISPOSER     (10)  // Сапёр (разряжает бомбы)
PCOM_AI_BIKER         (11)  // Байкер (#ifdef BIKE — не переносить)
PCOM_AI_FIGHT_TEST    (12)  // Боевой манекен
PCOM_AI_BULLY         (13)  // Задира (убивает всех не из банды)
PCOM_AI_COP_DRIVER    (14)  // Полицейский водитель
PCOM_AI_SUICIDE       (15)  // Самоубийца
PCOM_AI_FLEE_PLAYER   (16)  // Убегает от игрока
PCOM_AI_KILL_COLOUR   (17)  // Убивает всех одного цвета/группировки
PCOM_AI_MIB           (18)  // Men in Black (спецагент) — см. ai_behaviors.md
PCOM_AI_BANE          (19)  // Главный антагонист — см. ai_behaviors.md
PCOM_AI_HYPOCHONDRIA  (20)  // Ипохондрик (ищет здоровье)
PCOM_AI_SHOOT_DEAD    (21)  // Ассасин-снайпер — см. ai_behaviors.md
```

### Состояния AI (PCOM_AI_STATE_*) — 28 состояний (0-27)

```c
PCOM_AI_STATE_PLAYER        (0)   // Это игрок (не AI)
PCOM_AI_STATE_NORMAL        (1)   // Нормальное поведение
PCOM_AI_STATE_INVESTIGATING (2)   // Исследует подозрительный звук
PCOM_AI_STATE_SEARCHING     (3)   // Активный поиск нарушителя (STUB — пустой)
PCOM_AI_STATE_KILLING       (4)   // Пытается убить цель
PCOM_AI_STATE_SLEEPING      (5)   // Спит (STUB — пустой)
PCOM_AI_STATE_FLEE_PLACE    (6)   // Убегает из места
PCOM_AI_STATE_FLEE_PERSON   (7)   // Убегает от персонажа
PCOM_AI_STATE_FOLLOWING     (8)   // Следует за целью
PCOM_AI_STATE_NAVTOKILL     (9)   // Навигирует к цели для убийства
PCOM_AI_STATE_HOMESICK      (10)  // Возвращается домой
PCOM_AI_STATE_LOOKAROUND    (11)  // Осматривается (только счётчик, no code)
PCOM_AI_STATE_FINDCAR       (12)  // Ищет машину
PCOM_AI_STATE_BDEACTIVATE   (13)  // Разряжает бомбу
PCOM_AI_STATE_LEAVECAR      (14)  // Выходит из машины
PCOM_AI_STATE_SNIPE         (15)  // Снайперская позиция
PCOM_AI_STATE_WARM_HANDS    (16)  // Греет руки у огня
PCOM_AI_STATE_FINDBIKE      (17)  // Ищет байк (#ifdef BIKE)
PCOM_AI_STATE_KNOCKEDOUT    (18)  // Выбит из сознания
PCOM_AI_STATE_TAUNT         (19)  // Дразнит врага перед боем
PCOM_AI_STATE_ARREST        (20)  // Арестовывает нарушителя
PCOM_AI_STATE_TALK          (21)  // Разговаривает
PCOM_AI_STATE_GRAPPLED      (22)  // Удерживается в захвате
PCOM_AI_STATE_HITCH         (23)  // Садится как пассажир
PCOM_AI_STATE_AIMLESS       (24)  // Ищет машину, не находит
PCOM_AI_STATE_HANDS_UP      (25)  // Руки вверх (сдача)
PCOM_AI_STATE_SUMMON        (26)  // Вызывает подкрепление (Bane)
PCOM_AI_STATE_GETITEM       (27)  // Идёт за предметом
```

### Типы движения (PCOM_MOVE_*)

```c
PCOM_MOVE_STILL         (1)  // Стоит
PCOM_MOVE_PATROL        (2)  // Патрулирует по waypoints
PCOM_MOVE_PATROL_RAND   (3)  // Случайный патруль
PCOM_MOVE_WANDER        (4)  // Блуждает по улицам
PCOM_MOVE_FOLLOW        (5)  // Следует за NPC
PCOM_MOVE_WARM_HANDS    (6)  // Греет руки у огня
PCOM_MOVE_FOLLOW_ON_SEE (7)  // Следует если видит цель
PCOM_MOVE_DANCE         (8)  // Танцует
PCOM_MOVE_HANDS_UP      (9)  // Руки вверх (статичный)
PCOM_MOVE_TIED_UP       (10) // Связан
```

### Черты персонажа (PCOM_BENT_*)

```c
PCOM_BENT_LAZY             (0)  // Движется медленнее; ищет скамейку каждые 0x3f тиков
PCOM_BENT_DILIGENT         (1)  // Движется быстрее
PCOM_BENT_GANG             (2)  // Член банды (защищает товарищей)
PCOM_BENT_FIGHT_BACK       (3)  // Может драться в ответ
PCOM_BENT_ONLY_KILL_PLAYER (4)  // Убивает только игрока
PCOM_BENT_ROBOT            (5)  // Игнорирует всё вокруг
PCOM_BENT_RESTRICTED       (6)  // Не прыгает и не лазит
PCOM_BENT_PLAYERKILL       (7)  // Убить может только игрок (NPCшники не трогают)
```

---

## 5. Восприятие — видимость

`can_a_see_b(Thing *observer, Thing *target, SLONG range=0, SLONG no_los=0)`

**Дальность видения (range=0 → default):**
- NPC по умолчанию: `8 << 8` = 2048 world-units
- Игрок: `256 << 8` = 65536 (видит всегда на max дистанцию)
- Отрицательный range: игнорировать освещение

**Освещение влияет на дальность NPC:**
```c
light_at_target = NIGHT_get_light_at(target_pos);
view_range = (R+G+B) + (R+G+B)<<3 + (R+G+B)>>2 + 256;
// Хорошее освещение → дальше видят; тёмные зоны → ближе
```

**Коррекции дальности:**
- Цель приседает (`crouching`): view_range >>= 1 (половина)
- Цель движется: view_range += 256 (проще заметить)
- Высотная разница: если `dy >= 0x80` (128 units), добавляет `dy << 2` к эффективной дистанции

**Угол зрения (FOV), из 2048 = 360°:**
- Близко (dist < 0xc0 = 192 units): FOV = 700 (≈123°)
- Далеко (dist ≥ 192): FOV = 420 (≈74°)
- "Краем глаза" (250/2048 ≈ 44°): видит на половинную дистанцию только если цель не движется

**Line of Sight:**
- Eye height standing: 0x60 (96 units); crouching: 0x20 (32 units)
- Проверяется через `there_is_a_los()` от головы наблюдателя к голове цели

---

## 5b. Восприятие — звуки

`PCOM_oscillate_tympanum(type, originator, x, y, z, store_it)`

Звуки распространяются с радиусом (в world-units):

```c
PCOM_SOUND_FOOTSTEP      = 0x280   // 640 units / 2.5 squares
PCOM_SOUND_VAN           = 0x180   // 384 units (пугает только гражданских)
PCOM_SOUND_DROP          = 0x200   // 512 units
PCOM_SOUND_MINE          = 0x300   // 768 units
PCOM_SOUND_GRENADE_HIT   = 0x300   // 768 units (нужен LOS к гранате)
PCOM_SOUND_GRENADE_FLY   = 0x300
PCOM_SOUND_LOOKINGATME   = 0x400
PCOM_SOUND_WANKER        = 0x400
PCOM_SOUND_DROP_MED      = 0x400   // 1024 units
PCOM_SOUND_UNUSUAL       = 0x600   // 1536 units (банка колы!)
PCOM_SOUND_HEY           = 0x600
PCOM_SOUND_DRAW_GUN      = 0x600   // визуальный триггер (требует LOS!)
PCOM_SOUND_DROP_BIG      = 0x600
PCOM_SOUND_BANG          = 0x700   // 1792 units
PCOM_SOUND_ALARM         = 0x800   // 2048 units
PCOM_SOUND_FIGHT         = 0x900   // 2304 units
PCOM_SOUND_GUNSHOT       = 0xa00   // 2560 units (самый громкий)
```

**Фильтры распространения:**
- Zone check: NPC с `pcom_zone` слышат только звуки из своей зоны
- Warehouse boundary: звуки не проникают через стены склада
- PCOM_SOUND_DRAW_GUN требует LOS (это визуальный триггер, несмотря на название)

**Реакция на звуки:**
- Большинство звуков → `PCOM_AI_STATE_INVESTIGATING`
- Бой/выстрел → `PCOM_AI_STATE_SEARCHING` или сразу KILLING

---

## 8. Режимы и скорости движения

```c
// Режимы
PERSON_MODE_RUN    (0)
PERSON_MODE_WALK   (1)
PERSON_MODE_SNEAK  (2)
PERSON_MODE_FIGHT  (3)  // Боевой (сниженная скорость)
PERSON_MODE_SPRINT (4)  // Спринт (требует выносливости)

// Скорости
PERSON_SPEED_WALK   (1)
PERSON_SPEED_RUN    (2)
PERSON_SPEED_SNEAK  (3)
PERSON_SPEED_SPRINT (4)
PERSON_SPEED_YOMP   (5)  // Быстрая ходьба (марш)
PERSON_SPEED_CRAWL  (6)
```

---

## 8b. Собаки (canid) — статус в пре-релизе

**Файл:** `fallen/Source/canid.cpp` (~1475 строк)

**ВАЖНО: В этой сборке собаки полностью инертны!**

Главный dispatch switch в `CANID_fn_normal()` закомментирован — ни одно состояние собаки не обрабатывается. Все остальные функции (`CANID_6sense`, `CANID_can_see`, `CANID_LOS`, `CANID_Homing`) реализованы, но не вызываются.

---

## 9. Групповое поведение (банды и полиция)

### Оповещение банды

```c
PCOM_alert_my_gang_to_a_fight(attacker, target)
// → все члены банды (same pcom_colour) в состояниях
//   NORMAL/WARM_HANDS/FOLLOWING/SEARCHING/TAUNT/INVESTIGATING
//   переходят в KILLING с тем же target

PCOM_alert_my_gang_to_flee(threat, source)
// → не-GANG_flagged члены начинают флить вместе
```

- Группировка по `pcom_colour` (0–15), не по внешнему цвету
- `PCOM_BENT_GANG` (бит 2): NPC защищает членов своей банды

### Гражданские: воскрешение

Блуждающие гражданские (PCOM_MOVE_WANDER) воскресают если мертвы:
- Не в поле зрения игрока (флаг `FLAGS_IN_VIEW` = 0)
- Мертвы 200+ кадров → home позиция с полным здоровьем
- **Баг пре-релиза:** teleport не работает, воскресают на месте смерти (см. ai_behaviors.md 10e)

### Полиция: арест

1. Нарушитель получает флаг `FLAG_PERSON_FELON` через `PCOM_call_cop_to_arrest_me()`
2. Каждые 4 кадра коп проверяет: сканирует сферу радиуса 0x800 (2048 units)
3. Если находит `FLAG_PERSON_FELON` + есть LOS → переходит в `PCOM_AI_STATE_ARREST`
4. Водитель-коп (COP_DRIVER): паркует машину, выходит, арестовывает
5. Другие копы слышат `PCOM_SOUND_HEY` (радиус 0x600) → сходятся на место

**Приоритет цели для ареста:**
- Расстояние + высота: ближе = выше приоритет
- Игрок: score <<= 1 (деприоритет)
- С флагом GUILTY: score >>= 2 (высокий приоритет)

---

## 9b. Числовые константы

```c
PCOM_TICKS_PER_TURN    = 16           // Тиков за кадр
PCOM_TICKS_PER_SEC     = 16 * 20 = 320  // Тиков/сек (NOTE: 320, не 256!)
PCOM_ARRIVE_DIST       = 0x40 = 64   // Units — считается "прибыл"
RMAX_PEOPLE            = 180          // Максимум NPC в сцене
PCOM_MAX_GANGS         = 16           // Максимум банд (цвета 0–15)
PCOM_MAX_GANG_PEOPLE   = 64           // Суммарно членов банд
PCOM_MAX_FIND          = 16           // Максимум результатов sphere-поиска
PCOM_get_duration(n)   = n * 32 тиков // Конвертация из десятых секунды
```

---

## 10. Поток AI за кадр

```
PCOM_process_person(Thing *p_person):  // pcom.cpp:13003
    StateFn(p_person)                   // анимационный state machine

    if PlayerID:
        idle → pcom_move_counter++ → если >PCOM_get_duration(30) и рядом танцующий NPC:
            Darci копирует его танец (dance join логика)
        return  // игрок: больше ничего не делаем

    PCOM_process_movement(p_person)     // низкоуровневое движение

    if HELPLESS: return

    if DEAD/DYING:
        if AI==CIV && MOVE==WANDER && !IN_VIEW:
            pcom_ai_counter++ → если >200: воскресить
    else:
        PCOM_process_state_change(p_person)  // главный AI dispatcher
```

### PCOM_process_default() — диспетчер состояний

Вызывается большинством AI-типов. Switch по `pcom_ai_state`:

| Состояние | Обработчик |
|-----------|-----------|
| NORMAL | PCOM_process_normal() |
| INVESTIGATING | PCOM_process_investigating() |
| **SEARCHING** | **пусто (stub, break)** |
| KILLING | PCOM_process_killing() |
| **SLEEPING** | **пусто (stub, break)** |
| FLEE_PLACE / FLEE_PERSON | PCOM_process_fleeing() |
| FOLLOWING | PCOM_process_following() |
| NAVTOKILL | PCOM_process_navtokill() |
| HOMESICK | PCOM_process_homesick() |
| **LOOKAROUND** | **только счётчик (no code)** |
| FINDCAR | PCOM_process_findcar() |
| BDEACTIVATE | PCOM_process_bdeactivate() |
| LEAVECAR | PCOM_process_leavecar() |
| SNIPE | PCOM_process_snipe() |
| WARM_HANDS | PCOM_process_warm_hands() |
| KNOCKEDOUT | PCOM_process_knockedout() |
| TAUNT | PCOM_process_taunt() |
| ARREST | PCOM_process_arrest() |
| TALK | PCOM_process_talk() |
| HITCH | PCOM_process_hitch() |
| AIMLESS | PCOM_process_wander() |
| HANDS_UP | PCOM_process_hands_up() |
| GETITEM | PCOM_process_getitem() |
| default | ASSERT(0) |

FINDBIKE обёрнут в `#ifdef BIKE` (не переносится).

### PCOM_process_normal() — диспетчер NORMAL state

- **STILL / DANCE / HANDS_UP / TIED_UP:**
  - Если далеко от дома (>256 units) → HOMESICK
  - GUARD на домашней позиции: рисует пистолет если не нарисован
  - LAZY-флаг: каждые 0x3f тиков ищет скамейку/диван в радиусе 0x200=512, садится если нашёл
- **PATROL / PATROL_RAND** → `PCOM_process_patrol()`
- **WANDER** → `PCOM_process_wander()`
- **FOLLOW**: если CANTFIND substate — каждые 16 кадров пробует `can_a_see_b` к цели, иначе wander
- **FOLLOW_ON_SEE**: если дистанция < 0x200 И `can_a_see_b` → переключается на FOLLOW
- **WARM_HANDS**: если далеко от дома (>0x200) → HOMESICK
