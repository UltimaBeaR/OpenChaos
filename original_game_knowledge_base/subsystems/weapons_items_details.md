# Оружие — детали реализации (гранаты, автоприцел, DIRT)

**Связанный файл:** [weapons_items.md](weapons_items.md) — основные данные оружия

---

## 8. Физика гранат (grenade.cpp)

MAX_GRENADES = 6, struct Grenade (~44 байт): owner, xyz, yaw/pitch, dxyz, dyaw/dpitch, timer.

### Активация (Special.cpp:1734)
`SPECIAL_prime_grenade()` → timer = 16 × 20 × 6 = 1920 тиков (**6 секунд** при 20 fps).

### Бросок (grenade.cpp:105)
`CreateGrenadeFromPerson()`:
- Вектор из `FMATRIX_vector(Angle, 0)`: dx/dz = -vec × 181 >> 11, dy = 181 << 6
- NPC с близкой целью (dist < 0x500): скорость /= 2
- Стартовая позиция: LEFT_HAND bone

### Физика каждый кадр (grenade.cpp:208)
`ProcessGrenade()`:
- Timer: -= 16 × TICK_RATIO (при 0 → взрыв)
- Гравитация: dy -= TICK_RATIO × 2, cap -0x2000
- Движение: pos += vel × TICK_RATIO
- Отскок от пола: dy = abs(dy) >> 1; если dy < 0x400 → полная остановка (dx=dz=0)
- Отскок от стен: reverse + halve (при смене mapsquare + under > 0x4000)
- Alert AI: PCOM_oscillate_tympanum(GRENADE_FLY) каждые 16 кадров

### Взрыв (grenade.cpp:326)
`CreateGrenadeExplosion()`:
- Звук S_EXPLODE_START
- 2 основных частицы (EXPLODE1/2_ADDITIVE) + ~20 доп. (дым, огонь, осколки)
- DIRT_new_sparks × 3, DIRT_gust × 4 направления
- **create_shockwave(radius=0x300, force=500)** — повреждения по площади

### Прицел гранаты (grenade.cpp:441)
`show_grenade_path()` — рисует траекторию при удержании гранаты:
- Условие: STATE_IDLE + нет паузы + нет slowdown + не в warehouse
- Симуляция: CreateGrenadeFromPerson(timer=6000) → ProcessGrenade(explode=0) в цикле
- Отрисовка: пунктирная зелёная линия (0x8000af00) + PANEL_draw_gun_sight(scale=400) в точке приземления

---

## 8a. Автоприцеливание (guns.cpp)

### Параметры оружия — get_gun_aim_stats()

| Оружие | Range | Spread | Примечание |
|--------|-------|--------|-----------|
| MIB (встроенный) | 2048 (8<<8) | 20 | "ninja shooting machines" |
| Pistol (GUN_OUT) | 1792 (7<<8) | 15 | |
| AK47 | 2048 (8<<8) | 20 | |
| Shotgun | 1536 (6<<8) | 30 | Ближе, но шире конус |

### Scoring целей — calc_target_score_new() (для ИГРОКА)

Конус: MAX_NEW_TARGET_DANGLE = 2048/7 ≈ 292 (±~51°). При беге (SUB_STATE_RUNNING) сужается ×4 (±~13°).
Вертикальный лимит: 45° (abs(dy) > dist → skip).
Фильтры: dead/float=skip, people_allowed_to_hit_each_other, can_a_see_b (persons), there_is_a_los (non-persons).
CLASS_SPECIAL → только MINE.

**Score = (8<<8 - dist) × (MAX_DANGLE - dangle) >> 3**

Модификаторы scoring:
- Враг, атакующий игрока: ×4
- Thugs / MIB: ×2
- Копы (для Darci/Roper): ×0.5
- Гражданские: ×180/256 (~70%)
- Hostages: ×170/256 (~66%)
- Slag: ×210/256 (~82%)
- Barrels: ×220/256 (~86%), stacked: -0x80, cones/bins=skip
- Hands-up лежащие: score=0
- Dying: >>1
- Standing on vehicle: score=0
- First-person look: доп. вертикальный scoring

### Поиск цели — find_target_new()
- **Игрок:** THING_find_sphere(range, Person|Bat|Barrel|Vehicle|Special) → best score
- **NPC:** PCOM_person_wants_to_kill → если цель в машине → таргетит машину

### Снайпер NPC — calc_snipe_target_score() / find_snipe_target()
Для PCOM_AI_SHOOT_DEAD: range 19×256=4864, angle ±56, linear scan, нет LOS.

---

## 16. Банки колы (DIRT система) — отдельная от SPECIAL

**Ключевое:** банки колы — это **НЕ SPECIAL предметы**, а объекты DIRT-системы.

### DIRT типы (dirt.h):
```c
DIRT_TYPE_UNUSED    0
DIRT_TYPE_LEAF      1   // листья — ambient мусор
DIRT_TYPE_CAN       2   // банка на земле — подбирается
DIRT_TYPE_PIGEON    3   // голубь — ВСЕГДА ОТКЛЮЧЁН (prob_pigeon=0 принудительно)
DIRT_TYPE_WATER     4   // брызги воды
DIRT_TYPE_HELDCAN   5   // банка в руке Darci
DIRT_TYPE_THROWCAN  6   // банка в полёте (projectile)
DIRT_TYPE_HEAD      7   // отрубленная голова (отдельная механика)
DIRT_TYPE_HELDHEAD  8   // голова в руке
DIRT_TYPE_THROWHEAD 9   // голова в полёте
DIRT_TYPE_BRASS     10  // гильзы (было grenade slot)
DIRT_TYPE_MINE      11  // мина как DIRT объект
DIRT_TYPE_URINE     12  // ???
DIRT_TYPE_SPARKS    13
DIRT_TYPE_BLOOD     14
DIRT_TYPE_SNOW      15
// DIRT_MAX_DIRT = 256 (PC) — максимальный пул DIRT объектов
```

### Спавн банок:
- **`DIRT_init(prob_leaf=100, prob_can=1, prob_pigeon=0, ...)` — вызывается при загрузке уровня (elev.cpp:2425)**
- Банки спавнятся **динамически** вокруг позиции Darci через DIRT_set_focus (Controls.cpp, каждый кадр)
- prob_can=1 vs prob_leaf=100 → примерно **1 банка на ~100 листьев** — очень редко! (~2-3 банки из 256 DIRT slots)
- **Также:** `DIRT_create_cans(x, z, angle)` спавнит 5 банок при ударе бочки о бочку (barrel.cpp:1167)
- **Почему банок не видно в пре-релизном запуске:** вероятнее всего, редкость (prob_can=1) или рендер-баг в этой сборке. В бинарнике pieroZ они видны — значит механика рабочая.

### Подбор:
```c
// interfac.cpp:2138-2157 — в do_an_action()
// Только Darci в idle (PersonID>>5 == 0) и dist < 0x80 от ближайшей банки
set_person_can_pickup(p_thing);    // → DIRT_pick_up_can_or_head()
// Банка переходит TYPE_CAN → TYPE_HELDCAN; dd->droll = THING_NUMBER(p_person)
// FLAG_PERSON_CANNING |= (1<<15); Person.Hold = DIRT index
// STATE_CANNING = 32
```

### Бросок:
```c
// PUNCH кнопка → set_player_punch() → player_activate_in_hand()
// Если FLAG_PERSON_CANNING → set_person_can_release(p_person, power=128)
// → DIRT_release_can_or_head(person, 128)
// Банка TYPE_HELDCAN → TYPE_THROWCAN
// Скорость: dx/dz из матрицы поворота персонажа × power >> 18; dy = power >> 2 (вверх)
```

### Физика полёта (TYPE_THROWCAN):
- Гравитация: `dy -= TICK_RATIO` per tick, max падение -0x20
- Отскок от стен: `dx = -dx >> 1`, `dz = -dz >> 1` (половина скорости)
- Отскок от пола: `dy = abs(dy) >> 1` (потеря энергии)
- Когда `dy < 10 << TICK_SHIFT` → становится TYPE_CAN (ложится на пол и катится)
- При ударе о пол (если скорость достаточна): **`S_KICK_CAN` звук** + alert NPC через `PCOM_oscillate_tympanum(PCOM_SOUND_UNUSUAL, ...)`

### Реакция NPC:
- Брошенная банка, ударившаяся о пол → `PCOM_SOUND_UNUSUAL` = "unusual noise" = NPC идут проверять
- NPC слышат звук банки (в отличие от тихих действий)
- Тактическое применение: отвлечь охрану

### Флаги и состояния:
```c
FLAG_PERSON_CANNING = (1<<15)  // Person.Flags — держит банку/голову
SWORD Person.Hold              // DIRT index удерживаемого объекта
STATE_CANNING = 32             // стейт персонажа при поднятии/броске
```
