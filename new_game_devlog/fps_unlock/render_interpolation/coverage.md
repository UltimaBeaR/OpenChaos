# Coverage — что интерполируется и почему

> Архитектура → [`architecture.md`](architecture.md). Базовый принцип всей задачи → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md).

Решение **что** интерполировать вытекает из принципа: **physics-tick** вещи (двигаются дискретно по тикам) — нужны интерполяции, **wall-clock** вещи (уже плавные через `g_frame_dt_ms`) — не нужны.

## Что интерполируется (физика → визуальный плавный рендер)

### Things — массив `THINGS[MAX_THINGS=701]`

Capture идёт по `thing_class_head[CLASS_X]` linked list, для классов где **позиция меняется**:

| Класс | Что это | Что интерполируется |
|---|---|---|
| `CLASS_PERSON` | Дарси, NPC всех типов (cops, thugs, MIB, civilians) | WorldPos + Tweened Angle/Tilt/Roll |
| `CLASS_VEHICLE` | Машины (car, police, ambulance, jeep) | WorldPos + `Genus.Vehicle->Angle/Tilt/Roll` (отдельная ветка снапшота, не через DrawTween) |
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

### EWAY катсценная камера — `EWAY_cam_x/y/z/yaw/pitch/lens`

Параллельный snapshot для **in-game катсценной камеры** — она хранится не в `FC_cam[0]`, а в отдельных глобалах из `eway_globals.h`. `EWAY_process_camera` обновляет их в physics-tick; render-path функция `EWAY_grab_camera` копирует raw post-tick state в `fc->*`, перетирая FC_cam apply.

`render_interp_capture_eway_camera` снимает EWAY-глобалы рядом с capture FC_cam[0]. `render_interp_apply_eway_camera` подменяет на lerp в render path сразу после `EWAY_grab_camera` (в `aeng.cpp` и `bloom.cpp`). Apply **не вызывается** для audio (`MFX_set_listener`) и input — там должен быть real post-tick state без render-lag.

`EWAY_cam_roll` всегда константа `1024 << 8` (хардкод в `EWAY_grab_camera`); lerp по нему не делается, apply пишет ту же константу.

## Углы по DrawType

Углы хранятся в разных местах в зависимости от DrawType — поэтому интерполируются избирательно.

### Покрыто (Tween-семейство)

`Draw.Tweened->Angle/Tilt/Roll` (SWORD, диапазон 0..2047 циклически):

| DrawType | Класс/назначение |
|---|---|
| `DT_TWEEN` | Generic tween-mesh |
| `DT_ROT_MULTI` | CLASS_PERSON (Дарси, NPC) |
| `DT_ANIM_PRIM` | CLASS_ANIMAL, CLASS_BAT |

### Покрыто (Vehicle — отдельная ветка)

`Genus.Vehicle->Angle/Tilt/Roll` (SLONG, диапазон 0..2047 циклически для Angle; Tilt/Roll обычно небольшие и не нормализованы):

| DrawType | Класс/назначение |
|---|---|
| `DT_VEHICLE` | CLASS_VEHICLE (car, police, ambulance, jeep, ...) |

`Vehicle->Angle` использует **direction-aware lerp**: знак `Vehicle->VelR` (rotational velocity) служит hint'ом, чтобы при очень быстром вращении (>180°/тик, например конусы/мусор после удара машиной — если бы они были vehicles) короткий путь не выбирал обратное направление. Для Tilt/Roll — обычный short-path (нет per-axis angular velocity в Vehicle struct, но они и не накапливаются за полный круг).

### Не покрыто (требует расширения системы)

- **`DT_MESH`** — статичные mesh-объекты. Углы в `Draw.Mesh->Angle/Tilt/Roll` (UWORD). Часть таких объектов (`CLASS_SPECIAL` static pickups) — не вращаются вообще; часть — вращается (барабанящие предметы, эффекты). Сейчас не покрыто.
- **`DT_BIKE`** — велосипеды. Через DrawMesh.
- **`DT_CHOPPER`** — вертолёты. `alloc_chopper` ставит `Draw.Mesh = dm` (DrawMesh), не Tween. Углы в `Draw.Mesh->Angle/Roll/Tilt` (UWORD). Раньше ошибочно числился в Tween-семействе — приводило к чтению DrawMesh-памяти как DrawTween и появлению non-canonical pointer'ов в snapshot (catscene crash на slot=143). Если потребуется плавность поворотов вертолёта — нужно расширение DrawMesh-ветки аналогично Vehicle.
- **`DT_PYRO`** — pyro-эффекты. `alloc_pyro` не присваивает `Draw.X` вообще (state в `Genus.Pyro`). Раньше ошибочно числился в Tween-семействе, читал stale memory предыдущего жильца slot'а.

См. [`plans.md`](plans.md) — задача расширения на DrawMesh-углы.

## Что НЕ нужно интерполировать (уже плавно)

### AnimTween (вертексный morph) — теперь интерполируется

Анимации персонажей в Urban Chaos — это **per-vertex morph между keyframes**, не skeletal. Tween-фракция `dt->AnimTween` определяет где на оси `[CurrentFrame, NextFrame]` мы сейчас.

> ⚠️ Раньше тут было написано «AnimTween обновляется per render-frame, плавная сама по себе» — **это было неверно**. AnimTween продвигается анимационными хендлерами **на physics-тике** (e.g. [`person_normal_animate_speed` person.cpp:2848](../../../new_game/src/things/characters/person.cpp#L2848) масштабирует `tween_step` на `TICK_RATIO`). При 20 Hz physics + 60 Hz render значение держится 3 кадра подряд → морф рывками.

`AnimTween` (фракция [0, 256) внутри keyframe пары) + указатели `CurrentFrame`/`NextFrame` теперь capture'ятся и подменяются в frame-scope:
- Capture рядом с Angle/Tilt/Roll (когда `dt = Draw.Tweened` валиден)
- Apply: при стабильном FrameIndex — прямой lerp; при продвижении на 1 keyframe за тик — virtual extended coordinate с переключением пары указателей; при >1 keyframes за тик — пропуск (редко)
- Это нужно потому что на быстрых анимациях (running, sprint) FrameIndex меняется почти каждый тик и одного только лерпа AnimTween недостаточно

Подробнее → [`architecture.md`](architecture.md), секция «AnimTween».

Углы (`Angle/Tilt/Roll`, общий поворот меша) интерполируются параллельно. Сами вершины не трогаем — этим занимается morph-pipeline в `FIGURE_draw`, ему достаточно правильного `AnimTween`.

**Не покрывает:** cross-fade между **разными** анимациями (`CurrentAnim`/`MeshID` change). На таком переходе срабатывает `skip_once` — поза дискретно скачет с старой на новую. Отдельный баг → [`known_issues.md`](known_issues.md) #2.

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
  ├── Things (movement classes) → WorldPos + Tween-углы (DT_TWEEN family)
  ├── Vehicles → Genus.Vehicle->Angle/Tilt/Roll (SLONG, отдельная ветка с VelR direction hint)
  ├── Camera (FC_cam) → x/y/z/yaw/pitch/roll
  ├── EWAY катсценная камера → EWAY_cam_x/y/z/yaw/pitch/lens (отдельный snapshot, render-time substitution после EWAY_grab_camera)
  └── НЕ покрыто: DrawMesh углы (DT_MESH/DT_BIKE), particles

Wall-clock state (уже плавно, не трогаем):
  ├── AnimTween (vertex morph)
  ├── Эффекты погоды/окружения (rain, drip, ribbon, sparks, sky, fire)
  ├── Аудио listener
  ├── HUD/UI/menu
  └── DRIP/PUDDLE/wall-clock effects
```
