# Coverage — что интерполируется и почему

> Архитектура → [`architecture.md`](architecture.md). Базовый принцип всей задачи → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md).

Решение **что** интерполировать вытекает из принципа: **physics-tick** вещи (двигаются дискретно по тикам) — нужны интерполяции, **wall-clock** вещи (уже плавные через `g_frame_dt_ms`) — не нужны.

## Что интерполируется (физика → визуальный плавный рендер)

### Things — массив `THINGS[MAX_THINGS=701]`

Capture идёт по `thing_class_head[CLASS_X]` linked list, для классов где **позиция меняется**:

| Класс | Что это | Что интерполируется |
|---|---|---|
| `CLASS_PERSON` | Дарси, NPC всех типов (cops, thugs, MIB, civilians) | WorldPos + per-bone world pose (15-bone hierarchy через pose snapshot) |
| `CLASS_VEHICLE` | Машины (car, police, ambulance, jeep) | WorldPos + `Genus.Vehicle->Angle/Tilt/Roll` (отдельная ветка снапшота, не через DrawTween) |
| `CLASS_PROJECTILE` | Пули | WorldPos |
| `CLASS_ANIMAL` | Собаки (мёртвый код в pre-release, dispatch закомментирован) | WorldPos + per-element world pose (flat skeleton через pose snapshot) если когда-нибудь оживёт |
| `CLASS_PYRO` | Огонь, immolation. State в Genus.Pyro, не Tween | WorldPos. Углов нет |
| `CLASS_CHOPPER` | Вертолёты. Углы в Draw.Mesh, не Tween | WorldPos + Draw.Mesh->Angle/Tilt/Roll (DrawMesh-ветка) |
| `CLASS_PLAT` | Движущиеся платформы | WorldPos + Draw.Mesh->Angle/Tilt/Roll |
| `CLASS_BARREL` | Катящиеся бочки и конусы | WorldPos + Draw.Mesh->Angle/Tilt/Roll (DrawMesh-ветка с per-axis vel_hint для fast tumble) |
| `CLASS_BAT` | Bat / Gargoyle / Balrog / Bane (DT_ANIM_PRIM, flat skeleton) | WorldPos + per-element world pose (flat skeleton через pose snapshot) |
| `CLASS_BIKE` | Велосипеды | WorldPos + Draw.Mesh->Angle/Tilt/Roll (DrawMesh-ветка) |
| `CLASS_SPECIAL` | Pickup'ы (обычно статичные, но granates thrown — двигаются) | WorldPos |

**Почему именно эти классы:** `THING_FIND_MOVING` в [thing.h:241](../../../new_game/src/things/core/thing.h#L241) использует похожий список (`PERSON | ANIMAL | PROJECTILE`) — это «классы которые двигаются по сцене». Расширили до 11 классов на основе фактических игровых сущностей, у которых WorldPos меняется во время игры.

### Камера — `FC_cam[FC_MAX_CAMS]`

Поля интерполируются: `x, y, z, yaw, pitch, roll`. Только slot 0 фактически используется в shipped game (splitscreen вырезан). Структура поддерживает до `FC_MAX_CAMS` слотов на случай если splitscreen вернётся.

**Почему важно:** камера обновляется в `FC_process` внутри physics-loop — то есть сама дискретна на 20 Hz. Если её не интерполировать — interp'нутая Дарси начинает «дёргаться относительно камеры» (рендеримся в frame камеры из end-of-tick state, а тело — из интерполированного состояния). Получается screen-space jitter.

### EWAY катсценная камера — `EWAY_cam_x/y/z/yaw/pitch/lens`

Параллельный snapshot для **in-game катсценной камеры** — она хранится не в `FC_cam[0]`, а в отдельных глобалах из `eway_globals.h`. `EWAY_process_camera` обновляет их в physics-tick; render-path функция `EWAY_grab_camera` копирует raw post-tick state в `fc->*`, перетирая FC_cam apply.

`render_interp_capture_eway_camera` снимает EWAY-глобалы рядом с capture FC_cam[0]. `render_interp_apply_eway_camera` подменяет на lerp в render path сразу после `EWAY_grab_camera` (в `aeng.cpp` и `bloom.cpp`). Apply **не вызывается** для audio (`MFX_set_listener`) и input — там должен быть real post-tick state без render-lag.

`EWAY_cam_roll` всегда константа `1024 << 8` (хардкод в `EWAY_grab_camera`); lerp по нему не делается, apply пишет ту же константу.

## Pose / углы по DrawType

Поза персонажей и углы Things хранятся в разных местах в зависимости от DrawType.

### Tween-семейство — через pose snapshot (per-bone world transforms)

| DrawType | Класс/назначение | Skeleton | Pose path |
|---|---|---|---|
| `DT_ROT_MULTI` | CLASS_PERSON (Дарси, NPC) | Hierarchical 15-bone | `compose_full_skeletal_pose` — DFS walk по `body_part_children[][]` (PELVIS → TORSO → HEAD; PELVIS → FEMUR → TIBIA → FOOT; etc) |
| `DT_ANIM_PRIM` | CLASS_BAT (Bat / Gargoyle / Balrog / Bane), CLASS_ANIMAL (собаки, мёртвый код) | Flat (1..20 elements) | `compose_flat_skeletal_pose` — каждый element body-frame, mirrors `FIGURE_draw_prim_tween` parent==NULL ветку |
| `DT_TWEEN` | Generic tween-mesh | Hierarchical если ElementCount==15, иначе flat | По ElementCount |

Pose snapshot capture'ит world transform каждой кости (per-tick), apply лерпит world pos / slerps world rot per render frame. Это **архитектурно корректно** для cancel-out cases (лестница, прыжки) и anim transitions — composer выдаёт post-tick pose независимо от того сколько keyframes между prev и curr; render лerp'ит prev/curr world transforms плавно.

Подробно → [`architecture.md`](architecture.md), секция «Pose snapshot — character body».

### Vehicle — отдельная ветка

`Genus.Vehicle->Angle/Tilt/Roll` (SLONG, диапазон 0..2047 циклически для Angle; Tilt/Roll не нормализованы):

| DrawType | Класс/назначение |
|---|---|
| `DT_VEHICLE` | CLASS_VEHICLE (car, police, ambulance, jeep, ...) |

`Vehicle->Angle` использует **direction-aware lerp**: знак `Vehicle->VelR` (rotational velocity) служит hint'ом, чтобы при очень быстром вращении (>180°/тик) короткий путь не выбирал обратное направление. Для Tilt/Roll — обычный short-path.

### DrawMesh-семейство — отдельная ветка mesh angles (2026-05-04)

`Draw.Mesh->Angle/Tilt/Roll` (UWORD, 0..2047 циклически):

| DrawType | Класс/назначение |
|---|---|
| `DT_MESH` | CLASS_BARREL (бочки + конусы — один struct), CLASS_SPECIAL (pickup'ы), CLASS_PLAT (платформы) |
| `DT_BIKE` | Велосипеды (через DrawMesh, тот же путь) |
| `DT_CHOPPER` | Вертолёты (через DrawMesh) |

Расширено `ThingSnap` полями `mesh_angle/tilt/roll_prev/curr`, `mesh_*_vel_hint`, `mesh_ptr_curr` (для restore через cached pointer аналогично Vehicle). Capture в обоих ветках (first capture + regular path) с identity check на `DrawMesh*`. Apply через `lerp_angle_2048_directed(prev, curr, alpha, vel_hint)` — **per-axis** vel_hint (signed unwrapped diff `curr-prev`), что предотвращает backward-rotation через 0/2048 wrap при fast tumble (>180°/тик у бочек/конусов после удара). Compile-time flag `INTERP_MESH_ANGLE` в [`debug_interpolation_config.h`](../../../new_game/src/debug_interpolation_config.h).

**Static DT_MESH** (specials, неподвижные платформы): vel_hint=0, prev==curr → lerp возвращает curr → unchanged behavior, нулевой стоимость в release-build.

### Не покрыто (требует расширения системы)

- **`DT_PYRO`** — pyro-эффекты. `alloc_pyro` не присваивает `Draw.X` вообще (state в `Genus.Pyro`). Раньше ошибочно числился в Tween-семействе, читал stale memory.

## Что НЕ нужно интерполировать (уже плавно)

### AnimTween / vertex morph — покрыты pose snapshot'ом

Анимации персонажей в Urban Chaos — **per-vertex morph между keyframes** (не skeletal). Tween-фракция `dt->AnimTween` определяет где на оси `[CurrentFrame, NextFrame]` мы сейчас. AnimTween продвигается на physics-тике, при 20 Hz physics + 60 Hz render значение держится 3 кадра подряд → морф рывками.

Pose snapshot решает это **на уровне выше**: composer выполняет full pose composition в physics tick (включая vertex morph between AnimTween keyframes) и капчит world transform каждой кости. Render видит уже-композированную позу — два snapshot'а на границах physics ticks, между которыми лерпится `world_pose(t-1)` → `world_pose(t)`. AnimTween явно подменять не нужно.

То же касается **anim transitions** (idle→sprint, jump-kick, jump landing) — composer выдаёт корректную post-tick pose независимо от того что произошло между prev и curr; render лerp'ит между двумя valid composed poses плавно. Старые hacks (cross-anim blend, virtual extended coordinate, skip-on-backward) удалены.

Подробнее → [`architecture.md`](architecture.md), секция «Pose snapshot — character body».

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
  ├── Things WorldPos (movement classes, ~11 классов)
  ├── Persons / bats / animals → per-bone world pose snapshot (g_pose_snaps)
  │   ├── Hierarchical: 15-bone person rig (Дарси, NPC), parent-local + chain
  │   └── Flat: bat / Bane / Balrog / Gargoyle, world transform per element
  ├── Vehicles → Genus.Vehicle->Angle/Tilt/Roll (SLONG, отдельная ветка с VelR direction hint)
  ├── Camera (FC_cam) → x/y/z/yaw/pitch/roll
  ├── EWAY катсценная камера → EWAY_cam_x/y/z/yaw/pitch/lens (отдельный snapshot)
  ├── DIRT pool (leaves, brass, cans, blood, snow, etc — DIRT_dirt[1024]) → x/y/z/yaw/pitch/roll (SWORD)
  ├── DrawMesh углы (DT_MESH / DT_BIKE / DT_CHOPPER) → Draw.Mesh->Angle/Tilt/Roll (UWORD, отдельная ветка с per-axis vel_hint)
  └── НЕ покрыто: particles (PARTICLE_* pool)

Покрыто pose snapshot'ом автоматически (через композицию post-tick pose):
  ├── Vertex morph (AnimTween) — composer включает в pose composition
  ├── Anim transitions — composer выдаёт correct post-tick pose
  └── Cancel-out cases (ladder up/down, jump phases) — visible pelvis world stable

Wall-clock state (уже плавно, не трогаем):
  ├── Эффекты погоды/окружения (rain, drip, ribbon, sparks, sky, fire)
  ├── Аудио listener
  ├── HUD/UI/menu
  └── DRIP/PUDDLE/wall-clock effects
```
