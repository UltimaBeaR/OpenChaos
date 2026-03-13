# Thing System — детали (Barrel, Platform, Chopper)

**Связанный файл:** [game_objects.md](game_objects.md) — Thing struct, классы, флаги, state machine, MapWho

---

## 10. Barrel System (barrel.cpp) — деструктивные объекты

**Ключевой файл:** `barrel.cpp` (1937 строк), `barrel.h`

### Архитектура: 2-sphere rigid body

Каждый барель моделируется как **2 связанных сферы** (`BARREL_Sphere`) на фиксированном расстоянии `BARREL_SPHERE_DIST=50`. Это простейший rigid body — позиция бареля = среднее двух сфер, ориентация из вектора между ними.

### Состояния

| Состояние | Описание |
|-----------|---------|
| **STACKED** | На другом бареле (`on=THING_INDEX`), не обрабатывается. Каскадное пробуждение |
| **STILL** | На земле, не обрабатывается. Отличие от STACKED: `on=NULL` |
| **Moving** (без флагов STACKED/STILL) | Активная физика: гравитация, коллизии, damping |
| **HELD** | Персонаж держит — не обрабатывать физику (МЁРТВЫЙ КОД в пре-релизе!) |

### Физика (BARREL_process_sphere)

- Гравитация: `dy -= 0x80` каждый кадр
- Затухание: `d* -= d*/32` (3.125% за кадр)
- Коллизия с землёй: `PAP_calc_map_height_at` + bounce (`dy = abs(dy)/2`)
- Коллизия со стенами: проверка MAV_opt по 4 направлениям (XS/XL/ZS/ZL), отскок от стен
- Коллизия с другими барелями: sphere-sphere, push apart `ddist/4`
- Покой: `abs(dx)+abs(dy)+abs(dz) < 0x200` → still++; оба still>64 → BARREL_convert_moving_to_stationary

### Standing-up коррекция

Когда барель на земле (GROUNDED) и tilt близок к вертикали (в пределах 224 от 512 или 1536):
- Конусы/бины стоят только "правильной" стороной (проверка bs1.y > bs2.y)
- Остальные: сферы стягиваются к оси в XZ → барель постепенно встаёт

### 4 типа барелей

| Тип | Описание | При стрельбе |
|-----|---------|-------------|
| NORMAL | Стандартный барель | Взрыв (PYRO_FIREBOMB) + shockwave(0x200, 250) + удаление |
| CONE | Трафик-конус, меньший радиус сфер, НЕ стакается | Только толкает соседей (мелкий sphere hit) |
| BURNING | Горящий барель (PYRO_IMMOLATE при создании) | Как NORMAL |
| BIN | Мусорный бак, содержит мусор + банки | Только толкает соседей |

### Cans (банки из мусорных баков)

Если бак (BIN) упал на бок (tilt далеко от вертикали) и остановился → `DIRT_create_cans()` — рассыпаются банки.
Одноразовое: после рассыпания `BARREL_FLAG_CANS` снимается.

### BARREL_alloc — создание

- Аллоцирует Thing + DrawMesh + Barrel структуру (пул из BARREL_MAX_BARRELS=300)
- **Экстренный recycling:** если нет свободных Thing → ищет БЛИЖАЙШИЙ существующий барель и переиспользует!
- Стакинг: ищет барели в радиусе `BARREL_STACK_RADIUS=45`, ставит поверх (on = THING_INDEX нижнего)
- Небольшой рандом позиции: ±0x1ff к X/Y/Z (дрожание для visual variety)
- Burning barrel: создаёт PYRO_IMMOLATE сразу при аллокации

### BARREL_shoot — стрельба по барелю

Для NORMAL/BURNING:
1. `BARREL_dissapear()` — удаление с каскадом (стакнутые барели сверху → moving)
2. `PYRO_create(PYRO_FIREBOMB)` — огненный эффект
3. `create_shockwave(radius=0x200, damage=250)` — урон ближайшим
4. `PCOM_oscillate_tympanum(PCOM_SOUND_BANG)` — алерт для AI
5. `BARREL_hit_with_sphere(radius=0x60)` — толкает другие барели

Для CONE/BIN: только толкает соседние барели маленьким sphere hit (radius=0x15).

### МЁРТВЫЙ КОД

- `BARREL_position_on_hands()` + `BARREL_throw()` (строки ~1505-1690) — подбор/бросок, полностью в `/* */`
- Cone penalty (EWAY_count_up += 500) — закомментировано в convert_stationary_to_moving
- Старые particle-эффекты взрыва (строки ~1863-1890) — в `/* */`

### Коллизии с примитивами (BARREL_hit_with_prim)

Oriented box test: prim→bounding box + yaw rotation → transform barrel sphere в object space → AABB test. При коллизии — push sphere к ближайшему краю бокса.

---

## 11. Moving Platforms (plat.cpp)

**Файл:** `fallen/Source/plat.cpp` (`#ifndef PSX`)

Динамически движущиеся меш-примитивы по waypoint-маршрутам. Игроки могут стоять на них.

**Состояния:** PLAT_STATE_NONE / GOTO / PAUSE / STOP
- GOTO: вектор к целевому waypoint, ускорение/замедление smooth
- Arrival: overshoot correction; задержка 10 сек = "остановиться навсегда"
- Коллизии: PLAT_MAX_FIND=8 ближайших персонажей, bounding box check
- Визуал: rocket exhaust particles в GOTO состоянии (PC only)

**Флаги:** PLAT_FLAG_LOCK_X / LOCK_Y / LOCK_Z — блокировка осей

---

## 12. Helicopter (chopper.cpp)

**Файл:** `fallen/Source/chopper.cpp` (`#ifndef PSX`)

Вертолёт-враг с вращающимися лопастями и AI pathfinding.

**Лимиты:** MAX_CHOPPERS (пул), CLASS_CHOPPER Thing
- HARDWIRED_RADIUS = 8192, DETECTION_RADIUS = 1024, IGNORAMUS_RADIUS = 6144
- PRIM_OBJ_CHOPPER = 74, PRIM_OBJ_CHOPPER_BLADES = 75
- Dispatch: CHOPPER_functions[type] — CHOPPER_CIVILIAN

---

## 13. Другие мелкие системы (триаж)

| Файл | Назначение | Статус |
|------|-----------|--------|
| Pjectile.cpp | Generic projectile pool (alloc/free Thing+Projectile) | Минимальный (~50 строк); гранаты отдельно в grenade.cpp |
| Switch.cpp | Proximity trigger (sphere check) | ПОЛУМЁРТВЫЙ: sphere detect работает, group/class = стабы |
| lead.cpp | Верёвка/поводок (8-32 точек) | НЕДОДЕЛАН: создание работает, физика/симуляция неполная |
| State.cpp | State function dispatch → char/vehicle/animal tables | Роутер, логика в characters.md |
| Player.cpp | Аллокация Darci/Roper Thing | Минимальный обёртка |
| animal.cpp | Canid entities | `#ifndef TARGET_DC`, стабы |
| nd.cpp | Пустой файл | Только `#include "game.h"` |
| morph.cpp | Shape keys (bird shapes) | НЕ используется в игре |
