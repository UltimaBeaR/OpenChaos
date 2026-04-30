# Coverage — что интерполируется и почему

> Архитектура → [`architecture.md`](architecture.md). Базовый принцип всей задачи → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md).

Решение **что** интерполировать вытекает из принципа: **physics-tick** вещи (двигаются дискретно по тикам) — нужны интерполяции, **wall-clock** вещи (уже плавные через `g_frame_dt_ms`) — не нужны.

## Что интерполируется (физика → визуальный плавный рендер)

### Things — массив `THINGS[MAX_THINGS=701]`

Capture идёт по `thing_class_head[CLASS_X]` linked list, для классов где **позиция меняется**:

| Класс | Что это | Что интерполируется |
|---|---|---|
| `CLASS_PERSON` | Дарси, NPC всех типов (cops, thugs, MIB, civilians) | WorldPos + Tweened Angle/Tilt/Roll |
| `CLASS_VEHICLE` | Машины (car, police, ambulance, jeep) | WorldPos. **Угол не интерполируется** (хранится в `Genus.Vehicle->Draw.Angle`, не в DrawTween) |
| `CLASS_PROJECTILE` | Пули | WorldPos |
| `CLASS_ANIMAL` | Крысы, собаки, кошки | WorldPos + Tweened углы (через DT_ANIM_PRIM) |
| `CLASS_PYRO` | Огонь, immolation | WorldPos + Tweened углы |
| `CLASS_CHOPPER` | Вертолёты | WorldPos + Tweened углы |
| `CLASS_PLAT` | Движущиеся платформы | WorldPos |
| `CLASS_BARREL` | Катящиеся бочки | WorldPos |
| `CLASS_BAT` | Bane (летающие) | WorldPos + Tweened углы (DT_ANIM_PRIM) |
| `CLASS_BIKE` | Велосипеды | WorldPos. Угол через DrawMesh, не интерполируется |
| `CLASS_SPECIAL` | Pickup'ы (обычно статичные, но granates thrown — двигаются) | WorldPos |

**Почему именно эти классы:** `THING_FIND_MOVING` в [thing.h:241](../../../new_game/src/things/core/thing.h#L241) использует похожий список (`PERSON | ANIMAL | PROJECTILE`) — это «классы которые двигаются по сцене». Расширили до 11 классов на основе фактических игровых сущностей, у которых WorldPos меняется во время игры.

### Камера — `FC_cam[FC_MAX_CAMS]`

Поля интерполируются: `x, y, z, yaw, pitch, roll`. Только slot 0 фактически используется в shipped game (splitscreen вырезан). Структура поддерживает до `FC_MAX_CAMS` слотов на случай если splitscreen вернётся.

**Почему важно:** камера обновляется в `FC_process` внутри physics-loop — то есть сама дискретна на 20 Hz. Если её не интерполировать — interp'нутая Дарси начинает «дёргаться относительно камеры» (рендеримся в frame камеры из end-of-tick state, а тело — из интерполированного состояния). Получается screen-space jitter.

## Углы по DrawType

Углы хранятся в разных местах в зависимости от DrawType — поэтому интерполируются избирательно.

### Покрыто (Tween-семейство)

`Draw.Tweened->Angle/Tilt/Roll` (SWORD, диапазон 0..2047 циклически):

| DrawType | Класс/назначение |
|---|---|
| `DT_TWEEN` | Generic tween-mesh |
| `DT_ROT_MULTI` | CLASS_PERSON (Дарси, NPC) |
| `DT_ANIM_PRIM` | CLASS_ANIMAL, CLASS_BAT |
| `DT_PYRO` | CLASS_PYRO |
| `DT_CHOPPER` | CLASS_CHOPPER |

### Не покрыто (требует расширения системы)

- **`DT_VEHICLE`** — машины. Угол в `Genus.Vehicle->Draw.Angle` (отдельная структура). Позиция плавная, поворот зернистый (рывками 20 Hz). Заметно при поворотах машины.
- **`DT_MESH`** — статичные mesh-объекты. Углы в `Draw.Mesh->Angle/Tilt/Roll` (UWORD). Часть таких объектов (`CLASS_SPECIAL` static pickups) — не вращаются вообще; часть — вращается (барабанящие предметы, эффекты). Сейчас не покрыто.
- **`DT_BIKE`** — велосипеды. Через DrawMesh.

См. [`plans.md`](plans.md) — задача расширения на DrawMesh-углы и Vehicle-углы.

## Что НЕ нужно интерполировать (уже плавно)

### AnimTween (вертексный morph)

Анимации персонажей в Urban Chaos — это **per-vertex morph между keyframes**, а не skeletal. Tween-фракция `dt->AnimTween` обновляется **каждый render-кадр** (не каждый physics-тик), так что vertex positions уже плавно tweens. Эта анимация плавная независимо от render rate.

Мы интерполируем `Angle/Tilt/Roll` (общий поворот всего меша), но **не сами вершины** — этим занимается morph-pipeline в `FIGURE_draw`.

### Wall-clock эффекты (см. `CORE_PRINCIPLE.md`)

Уже плавные через `g_frame_dt_ms`:

- **Дождь** (`AENG_draw_rain`) — bucket-strobe pattern на `UC_VISUAL_CADENCE_TICK_MS` = 33 ms (специальная техника, см. CORE_PRINCIPLE)
- **Drip ripples / Puddle splash** (`DRIP_process`, `PUDDLE_process`) — wall-clock 30 Hz accumulator с шагом `UC_VISUAL_CADENCE_TICK_MS`
- **Ribbons** (электрические провода, `RIBBON_process`) — wall-clock 30 Hz accumulator (тот же паттерн что drip)
- **Sparks** (электрические заборы, `SPARK_show_electric_fences`) — wall-clock period accumulator (~1067 ms = 32 visual ticks)
- **Sky / Cloud / Moon wibble** — параметризован `VISUAL_TURN` (визуальная каденция 30 Hz)
- **Fire** (`FIRE_*`) — отдельный issue, в pre-release нет вызова, статус неясен (см. summary.md open question 5)
- **Touch fire** (фонарик Дарси, `BLOOM` dlight colour alternation) — параметризован `VISUAL_TURN` (`game_tick.cpp:1706`)

### Аудио

`MFX_set_listener` (audio listener) обновляется по wall-clock. **НЕ должен** видеть интерполированную камеру (50мс лага в звуке слышимо). Сейчас MFX работает вне `draw_screen` frame-scope — получает actual values. Это правильно.

### HUD / UI / меню / прицел

В screen-space, не зависит от world-rate. Прицел в FPP, иконки, меню, HUD — рисуются после интерпо-frame по своей логике.

### Партиклы

`PARTICLE_*` pool — двигаются через `TICK_RATIO` на physics-tick. **Технически дёргаются на 60 Hz рендере**, но из-за короткого lifetime и flicker'а это обычно незаметно. Сейчас не интерполируются. См. [`plans.md`](plans.md) — может потребоваться добавить если станет видно.

## Что выпадает из правила «physics → interp, wall-clock → уже плавно»

Это **серая зона**, где нужно решать индивидуально:

- **MIB self-destruct effect** (`DRAWXTRA_MIB_destruct`) — рисуется по `Timer1`. Параметризован physics-tick'ами. Если визуально дёргается — нужна отдельная обработка. Сейчас не трогаем.
- **Drip-spawn от puddles** — `g_frame_dt_ms` (wall-clock), уже плавно. Но spawn position читает `darci->WorldPos` — **подменено** через интерпо-frame (получает интерполированное), что правильно.
- **Cone/torch (фонарик Дарси)** — рисуется в render-path с углами на `VISUAL_TURN` (30 Hz wall-clock после миграции в шаге 2) и позицией Дарси (интерполированной через frame-scope). Микро-расхождение между «направление фонарика на 30 Hz» и «положение Дарси на render-rate» — теоретически возможно, но визуально незаметно.

## Сводка

```
Physics-rate state (нужна интерполяция):
  ├── Things (movement classes) → WorldPos + Tween-углы
  ├── Camera (FC_cam) → x/y/z/yaw/pitch/roll
  └── НЕ покрыто: Vehicle Angle, DrawMesh углы, particles

Wall-clock state (уже плавно, не трогаем):
  ├── AnimTween (vertex morph)
  ├── Эффекты погоды/окружения (rain, drip, ribbon, sparks, sky, fire)
  ├── Аудио listener
  ├── HUD/UI/menu
  └── DRIP/PUDDLE/wall-clock effects
```
