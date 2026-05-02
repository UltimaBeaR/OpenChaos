# Архитектура системы интерполяции

> Базовый принцип всей задачи → [`../CORE_PRINCIPLE.md`](../CORE_PRINCIPLE.md).
> Контекст родительской задачи → [`../fps_unlock.md`](../fps_unlock.md).

## Зачем нужна

Физика тикает 20 Hz (`g_physics_hz`, дизайновая частота оригинала, `THING_TICK_BASE_MS = 1000 / UC_PHYSICS_DESIGN_HZ = 50ms`). Рендер бежит на любой частоте (60+ Hz). Без интерполяции render-кадры между physics-тиками показывают **идентичные позиции** — визуально это рывки, мир кажется тормозным даже при высоком FPS.

Решение — render-side interpolation: между двумя последними physics-снапшотами **рисовать** lerp(prev, curr, alpha), при этом **не трогая физику** (она остаётся 20 Hz, как требует `CORE_PRINCIPLE.md`).

## Принцип

Стандартный «render lag by 1 tick»:
- Физика обновила состояние в конце тика N → snapshot.curr = state_N, snapshot.prev = state_{N-1}
- Рендер показывает `lerp(state_{N-1}, state_N, alpha)` где `alpha = physics_acc_ms / phys_step_ms`
- При `alpha=0` (только что прошёл тик N) рендерим `state_{N-1}` (мы отстаём на 1 тик ≈ 50мс)
- При `alpha→1` (вот-вот придёт тик N+1) рендерим почти `state_N`
- На границе тиков: `alpha=1` показывает state_N → tick N+1 → `alpha=0` показывает new_prev = state_N (то же значение). Без скачка.

Цена: 50мс render lag (показываем прошлое). Это не уменьшает input latency, только улучшает визуальную плавность.

## Архитектура

### 1. Capture в конце physics-тика

В физическом loop'е [`game.cpp`](../../../new_game/src/game/game.cpp) после `process_things` и всех `*_process` функций — **в самом конце** каждого тика, перед `physics_acc_ms -= phys_step_ms`:

```cpp
// Walk per-class linked lists for movement-bearing classes only.
const UBYTE moving_classes[] = {
    CLASS_PERSON, CLASS_VEHICLE, CLASS_PROJECTILE,
    CLASS_ANIMAL, CLASS_PYRO, CLASS_CHOPPER,
    CLASS_PLAT, CLASS_BARREL, CLASS_BAT, CLASS_BIKE,
    CLASS_SPECIAL,
};
for (UBYTE c : moving_classes) {
    UWORD t_idx = thing_class_head[c];
    while (t_idx) {
        Thing* p = TO_THING(t_idx);
        render_interp_capture(p);
        t_idx = p->NextLink;
    }
}
render_interp_capture_camera(&FC_cam[0]);
```

Capture **сдвигает окно**: prev = old_curr, curr = текущее значение. Один проход на каждом physics-тике, не один раз в render-кадре. Если за один render-кадр случилось несколько physics-тиков (лаг или низкий FPS), окно сдвигается на каждом → к моменту рендера prev/curr всегда покрывают **последний** тик.

### 2. Расчёт alpha

После physics-loop, **перед** `draw_screen`:

```cpp
double a = physics_acc_ms / phys_step_ms;  // [0, 1)
if (a < 0.0) a = 0.0; else if (a > 1.0) a = 1.0;
g_render_alpha = float(a);
```

`physics_acc_ms` — остаток после while-loop, всегда в `[0, phys_step_ms)`. Clamp защищает от crazy edge cases.

### 3. Frame-scope в начале draw_screen

В [`game.cpp` `draw_screen()`](../../../new_game/src/game/game.cpp):

```cpp
void draw_screen(void) {
    RenderInterpFrame interp_frame;  // RAII
    AENG_draw(draw_3d);
    // ... destructor restores originals
}
```

**Конструктор** `RenderInterpFrame::RenderInterpFrame()`:
- Проходит по всему `g_thing_snaps[MAX_THINGS]`
- Для каждого `valid && p->Class != CLASS_NONE`:
  - **Сохраняет** живое значение `Thing.WorldPos` и `Tweened->Angle/Tilt/Roll` в `saved_*` поля snapshot'а
  - **Записывает в Thing напрямую** интерполированное значение
- Аналогично для `FC_cam[0]` (всех `FC_MAX_CAMS` слотов в общем случае)
- Помечает `applied=true`

**Деструктор** `~RenderInterpFrame()`:
- Проходит по тому же массиву
- Для каждого `applied=true`: восстанавливает saved_* обратно в живой Thing/FC_cam

В результате **между ctor и dtor** живые поля содержат интерполированные значения. Все читатели в render path (FIGURE_draw, AENG_set_camera_radians, тени, отражения, минимап, прицел, gun-flash, lighting/NIGHT_find) автоматически получают подменённое состояние без точечных правок.

После dtor — снова actual values. Никакая логика/AI/физика между ctor и dtor не выполняется (всё уже отработало в physics-loop), поэтому никто кроме рендера не видит интерполированное состояние.

## Файлы

| Файл | Назначение |
|---|---|
| [`new_game/src/engine/graphics/render_interp.h`](../../../new_game/src/engine/graphics/render_interp.h) | API: `g_render_alpha`, `render_interp_capture/_invalidate/_mark_teleport`, `render_interp_capture_camera/_mark_camera_teleport`, `render_interp_reset`, класс `RenderInterpFrame` |
| [`new_game/src/engine/graphics/render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp) | Реализация: snapshot-массивы `g_thing_snaps[MAX_THINGS]`, `g_cam_snaps[FC_MAX_CAMS]`; lerp-функции; ctor/dtor `RenderInterpFrame` |
| [`new_game/src/game/game.cpp`](../../../new_game/src/game/game.cpp) | Capture-loop в конце physics-тика, расчёт alpha, frame-scope в `draw_screen`, `render_interp_reset()` перед main loop |
| [`new_game/src/things/core/thing.cpp`](../../../new_game/src/things/core/thing.cpp) | Hook `render_interp_invalidate(t_thing)` в `free_thing()` |
| [`new_game/CMakeLists.txt`](../../../new_game/CMakeLists.txt) | Подключение `render_interp.cpp` |

## Структуры данных

### `ThingSnap` (per-Thing snapshot)

```cpp
struct ThingSnap {
    GameCoord pos_prev, pos_curr;
    SWORD angle_prev, angle_curr;
    SWORD tilt_prev, tilt_curr;
    SWORD roll_prev, roll_curr;
    bool valid;            // false до первого capture после reset/invalidate
    bool skip_once;        // запросить prev=curr на следующем capture (teleport)
    bool has_angles;       // были ли захвачены Tweened-углы

    // Anim-transition detection
    UBYTE last_mesh_id;
    SLONG last_anim;

    // Vertex-morph fraction (Draw.Tweened->AnimTween) + указатели keyframe'ов
    // которые она индексирует. AnimTween продвигается на physics-tick'е,
    // не per-frame. Указатели CurrentFrame/NextFrame нужны чтобы лерпить
    // через границу keyframe — на быстрых анимациях FrameIndex меняется
    // почти каждый тик.
    SLONG anim_tween_prev, anim_tween_curr;
    UBYTE frame_index_prev, frame_index_curr;
    struct GameKeyFrame* current_frame_prev;
    struct GameKeyFrame* current_frame_curr;
    struct GameKeyFrame* next_frame_prev;
    struct GameKeyFrame* next_frame_curr;

    // Vehicle yaw/tilt/roll — отдельная ветка для CLASS_VEHICLE.
    // veh_velr_curr — Vehicle->VelR (rotational velocity), используется
    // как direction hint в lerp_angle_2048_directed.
    // veh_ptr_curr — закэшированный Genus.Vehicle на момент capture.
    // Используется в ctor/dtor для identity-check: если live
    // p->Genus.Vehicle != veh_ptr_curr → пропускаем apply (vehicle slot
    // был перепривязан в катсцене; mismatch может означать stale capture
    // или невалидный live pointer).
    bool  has_vehicle_angles;
    Vehicle* veh_ptr_curr;
    SLONG veh_angle_prev, veh_angle_curr;
    SLONG veh_tilt_prev,  veh_tilt_curr;
    SLONG veh_roll_prev,  veh_roll_curr;
    SLONG veh_velr_curr;

    // Frame-scope save (заполняется RenderInterpFrame ctor, читается dtor)
    GameCoord saved_pos;
    SWORD saved_angle, saved_tilt, saved_roll;
    SLONG saved_anim_tween;
    struct GameKeyFrame* saved_current_frame;
    struct GameKeyFrame* saved_next_frame;
    SLONG saved_veh_angle, saved_veh_tilt, saved_veh_roll;
    bool applied;
    bool anim_tween_applied;  // covers AnimTween + CurrentFrame + NextFrame
                              // override. Skipped on >1-keyframe jumps per
                              // tick (нет промежуточной истории указателей).
    bool veh_angles_applied;  // ctor wrote interp values into Genus.Vehicle->
                              // Angle/Tilt/Roll; dtor restores them.
};
```

Массив `g_thing_snaps[MAX_THINGS]`, индексируемый по `Thing - THINGS` (pointer arithmetic против глобального массива `THINGS[]`).

### `CamSnap` (per-FC_Cam snapshot)

```cpp
struct CamSnap {
    FC_Cam* cam;
    SLONG x_prev, x_curr;
    SLONG y_prev, y_curr;
    SLONG z_prev, z_curr;
    SLONG yaw_prev, yaw_curr;
    SLONG pitch_prev, pitch_curr;
    SLONG roll_prev, roll_curr;
    bool valid;
    bool skip_once;
    SLONG saved_x, saved_y, saved_z;
    SLONG saved_yaw, saved_pitch, saved_roll;
    bool applied;
};
```

Массив `g_cam_snaps[FC_MAX_CAMS]`. Splitscreen в shipped game не используется, фактически работает только slot 0.

## Формулы lerp

### Линейная (позиция)

```cpp
inline SLONG lerp_i32(SLONG a, SLONG b, float t) {
    return a + SLONG(float(b - a) * t);
}
```

### Углы Tween (диапазон 0..2047)

`SWORD`-углы DrawTween (Angle, Tilt, Roll). Вход может быть отрицательным (`-1 == 2047`). Выход всегда в `[0, 2047]`.

```cpp
inline SWORD lerp_angle_2048(SWORD a, SWORD b, float t) {
    int ai = int(a) & 2047;
    int bi = int(b) & 2047;
    int diff = bi - ai;
    if (diff > 1024) diff -= 2048;
    else if (diff < -1024) diff += 2048;
    int result = ai + int(float(diff) * t);
    return SWORD(((result % 2048) + 2048) % 2048);
}
```

Short-path: интерполяция идёт по короткой стороне круга. Например, 2047 → 0 идёт через +1, не через -2047.

### Углы FC_Cam (диапазон 0..2048×256)

Камера хранит углы в fixed-point (`1 angle unit = 256 sub-units`). Полная окружность = 524288 (2048 × 256). Та же short-path логика на расширенном диапазоне.

### Углы Vehicle (SLONG, диапазон 0..2047 для Angle, любой — для Tilt/Roll)

`Genus.Vehicle->Angle/Tilt/Roll` хранятся как SLONG. `Angle` нормализуется `& 2047` в физике, Tilt/Roll — нет (но обычно небольшие).

Для Angle используется `lerp_angle_2048_directed(a, b, t, vel_hint)` — short-path с tie-break по vel_hint в ambiguous-зоне.

- `|diff| <= 1024`: лерп идёт по реальному `diff` как есть, vel_hint игнорируется. Это критично — иначе micro-correction назад (например, diff=-1 после столкновения) при положительном VelR превратился бы в почти полный круг вперёд.
- `|diff| > 1024`: physics реально провернула >180° за тик, short-path выбрал бы противоположный знак. Используем vel_hint: положительный → выбираем положительную ветку (long-path forward), отрицательный → отрицательную. При `vel_hint == 0` — обычный short-path (нет предпочтения).

В нормальной езде `|VelR|` маленький и `|diff|` тоже маленький — hint никогда не активируется. Hint срабатывает только при импульсном раскручивании (удар, перевёрнутая машина).

Для Tilt/Roll используется `lerp_angle_2048_slong` — обычный short-path с маскировкой к [0, 2047]. У Vehicle нет per-axis angular velocity для Tilt/Roll, и они не накапливаются за полный круг (машина не делает кувырок через 360° по pitch).

## API

### Capture (вызывается из physics-loop)

- `void render_interp_capture(Thing*)` — снимок одного Thing'а
- `void render_interp_capture_camera(FC_Cam*)` — снимок камеры

### Invalidation (вызывается извне)

- `void render_interp_invalidate(Thing*)` — обнулить snapshot. Сейчас: hook на `free_thing` (slot reuse). Может вызываться в любой момент когда thing «начинает новую жизнь».
- `void render_interp_mark_teleport(Thing*)` — полный сброс snapshot'а thing'а через `render_interp_invalidate`. На следующем capture идёт `!valid` ветка, prev=curr=current — никакого лерпа через teleport. Используется в EWAY-обработчиках которые двигают actors / vehicles между сценами катсцены ([`eway.cpp`](../../../new_game/src/missions/eway.cpp) — `EWAY_DO_CREATE_ENEMY`, `EWAY_DO_MOVE_THING`). **Важно:** ранее реализация была через `s.skip_once = true`, но `skip_once` collapse'ит только anim-keyframe поля (AnimTween, FrameIndex, CurrentFrame, NextFrame) и НЕ collapse'ит pos/angles — это специально, потому что тот же флаг переиспользуется anim-transition детектором, где pose меняется но WorldPos должен продолжать лерпиться. Для настоящего teleport'а нужен полный wipe → invalidate.
- `void render_interp_mark_camera_teleport(FC_Cam*)` — то же для камеры. **Сейчас нигде не вызывается**.
- `void render_interp_reset(void)` — обнулить всё. Вызывается перед main loop в `game_loop()`.

### Frame-scope

- `class RenderInterpFrame` — RAII. Создаётся в `draw_screen()`, в ctor применяет интерполяцию ко всем valid snapshot'ам (Things + камеры), в dtor восстанавливает.

### Глобалы

- `extern float g_render_alpha;` — текущий коэффициент интерполяции `[0, 1)`. Обновляется один раз за render-frame после physics-loop.
- `extern bool g_render_interp_enabled;` — мастер-флаг включения. По умолчанию `true`. Когда `false`, `RenderInterpFrame::ctor` делает early return — рендер читает actual (post-tick) state напрямую, мир дёргается на physics-rate. Capture продолжает работать, переключение на `true` сразу даёт плавность. Toggle hotkey **3** (см. [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md)).

## Captured классы (фактический список)

В capture-loop попадают только классы где **позиция меняется**:
- `CLASS_PERSON`
- `CLASS_VEHICLE`
- `CLASS_PROJECTILE`
- `CLASS_ANIMAL`
- `CLASS_PYRO`
- `CLASS_CHOPPER`
- `CLASS_PLAT`
- `CLASS_BARREL`
- `CLASS_BAT`
- `CLASS_BIKE`
- `CLASS_SPECIAL` (granates thrown)

Не входят: `CLASS_BUILDING`, `CLASS_SWITCH`, `CLASS_FURNITURE`, `CLASS_TRACK`, `CLASS_PLAYER`, `CLASS_CAMERA`, `CLASS_ANIM_PRIM`. Это статичные либо нерелевантные для движения.

## DIRT pool (отдельная подсистема — листья, гильзы, банки и т.п.)

`DIRT_dirt[1024]` ([world_objects/dirt.h](../../../new_game/src/world_objects/dirt.h)) — отдельный pool ambient-debris не в `THINGS[]`. Включает все `DIRT_TYPE_*`: leaves, brass casings, cans, blood, snow, sparks, urine, head props и др. Updates per physics tick через `DIRT_process` (`world_objects/dirt.cpp`) — gravity, friction, waft, scatter от игрока через `DIRT_gust`. Render — `AENG_draw_dirt` (`engine/graphics/pipeline/aeng.cpp`).

**Snapshot инфраструктура:** отдельный `DirtSnap g_dirt_snaps[DIRT_MAX_DIRT]` (~30 KB) с prev/curr для `x/y/z` (SWORD), `yaw/pitch/roll` (SWORD, диапазон 0..2047). `last_type` отслеживается для детекции slot reuse — когда `DIRT_TYPE_UNUSED` сменяется на любой не-UNUSED тип (или один тип на другой), snapshot обнуляется и идёт через !valid first-capture path.

**Capture:** `render_interp_capture_dirt()` — один проход на 1024 слота в physics tick после `DIRT_process` (рядом с другими capture-вызовами). Свободные слоты (`type == DIRT_TYPE_UNUSED`) early-skip — стоимость прохода ничтожная даже когда pool почти пуст.

**Apply/restore:** в `RenderInterpFrame::ctor/dtor` после Things и cameras — write lerped pos/angles прямо в `DIRT_dirt[i]`, save originals в snapshot для восстановления. Render-path читает интерполированные значения автоматически (как для Things).

**Lerp функции:**
- Позиция SWORD — `lerp_i16(a, b, t)` (математика идёт через SLONG чтобы diff не overflow'нул на extremes; результат cast'ится обратно в SWORD).
- Углы 0..2047 — `lerp_angle_2048` (тот же short-path что для Tween-углов Things).

## Углы — какие DrawType покрыты

Углы интерполируются **только для Tween-семейства DrawType** (углы лежат в `Draw.Tweened->{Angle,Tilt,Roll}`):

```cpp
inline bool draw_type_uses_tween(UBYTE dt) {
    switch (dt) {
        case DT_TWEEN:
        case DT_ROT_MULTI:    // CLASS_PERSON
        case DT_ANIM_PRIM:    // animals/bats
            return true;
        default:
            return false;
    }
}
```

**`DT_CHOPPER` и `DT_PYRO` сюда НЕ входят** — вертолёты используют `Draw.Mesh` (DrawMesh), а pyro вообще не использует поле `Draw` (state в `Genus.Pyro`). Включение их в Tween-семейство приводило к чтению DrawMesh / stale памяти как DrawTween через union, что давало non-canonical pointer'ы в `current_frame_curr`/`next_frame_curr` snapshot fields и крашило ctor на дереференсе. Подтверждено через диагностический лог в катсцене (slot=143 chopper, NextFrame инкрементировался ровно на 0x20 каждый тик — DrawMesh поле, не keyframe pointer).

**Vehicle — отдельная ветка (вне `draw_type_uses_tween`):**

CLASS_VEHICLE хранит yaw/tilt/roll в `Genus.Vehicle->Angle/Tilt/Roll` (SLONG). Это поле — то самое что читает `make_car_matrix` ([vehicle.cpp:125](../../../new_game/src/things/vehicles/vehicle.cpp#L125)) и `draw_car`. `Vehicle.Draw.Angle` (внутри вложенного DrawTween Draw) — другое поле и не интерполируется.

Snapshot хранит `veh_angle/tilt/roll_prev/curr` плюс `veh_velr_curr` (`Vehicle->VelR`, rotational velocity). При apply используется direction-aware lerp `lerp_angle_2048_directed`: знак `VelR` определяет на какой стороне круга идёт `prev → curr`. Это закрывает кейс «very fast spin > 180°/tick», где обычный short-path выбрал бы обратное направление.

**Не покрыты углы у:**
- `DT_MESH` — статичные mesh. Углы в `Draw.Mesh->Angle/Tilt/Roll` (UWORD). Большинство таких объектов не вращаются, но если будут — нужно отдельная ветка.
- `DT_BIKE` — велосипеды. Также через DrawMesh.
- `DT_CHOPPER` — вертолёты. Через DrawMesh, аналогично Vehicle. Если потребуется плавность поворотов — расширить DrawMesh-ветку.
- `DT_PYRO` — pyro-эффекты, pyro state в `Genus.Pyro`, нет direct angle storage.

Подробнее → [`coverage.md`](coverage.md).

## Hook'и в коде вне render_interp.cpp

### `free_thing` ([thing.cpp:623](../../../new_game/src/things/core/thing.cpp))

```cpp
void free_thing(Thing* t_thing) {
    if (t_thing->Flags & FLAGS_HAS_ATTACHED_SOUND)
        MFX_stop_attached(t_thing);
    extern void render_interp_invalidate(Thing*);
    render_interp_invalidate(t_thing);  // <-- hook
    if (is_class_primary(t_thing->Class)) ...
}
```

Цель: сбросить snapshot когда thing освобождается. Защищает от случая «slot переиспользован новым thing'ом, snapshot.valid=true со stale данными прошлого жильца».

### Anim-transition detection (внутри `render_interp_capture`)

```cpp
if (dt) {
    if (s.last_mesh_id != dt->MeshID || s.last_anim != dt->CurrentAnim) {
        s.skip_once = true;
        s.last_mesh_id = dt->MeshID;
        s.last_anim = dt->CurrentAnim;
    }
}
```

Цель: на смене анимации (например `STATE_IDLE` → `STATE_JUMPING`) поза дискретно меняется, и lerp'ить через эту границу = визуальное «съёживание». Детектор сравнивает `MeshID`/`CurrentAnim` с прошлым capture; при изменении — `skip_once=true`, на этом тике prev=curr (поза рендерится без интерполяции в момент смены).

Зафиксировал баг с глюками анимаций при прыжке Дарси.

## Производительность

**Память:** `~42 KB` статически. Никакой динамики (`malloc`/`new`/vector resize) каждый кадр. На Steam Deck/телефоне — те же 42 KB.

| Структура | Размер |
|---|---|
| `g_thing_snaps[MAX_THINGS=701]` | ~60 байт × 701 ≈ 42 KB |
| `g_cam_snaps[FC_MAX_CAMS=2]` | ~80 байт × 2 ≈ 160 B |

**Время:**
| Операция | Частота | Стоимость |
|---|---|---|
| Capture per tick | 20 Hz | ~100-200 things × ~30 байт R/W ≈ <1µs |
| `RenderInterpFrame` ctor | per render frame | iterate 701 slots, ~100 valid (6 lerps each) ≈ ~2µs |
| `RenderInterpFrame` dtor | per render frame | restore ~100 saved ≈ ~1µs |

На 60 FPS — суммарно `<5µs / 16ms` = `~0.03% CPU`. Cache: 42 KB полностью fits L2 (на любом современном железе, Steam Deck включительно), линейный доступ — prefetcher отрабатывает идеально.

## Debug controls

### Runtime toggle (клавиша 3)

`g_render_interp_enabled` тогглится клавишей **3** через `check_debug_timing_keys()`
в [`game.cpp`](../../../new_game/src/game/game.cpp) (зеркальный switch в
[`video_player.cpp`](../../../new_game/src/engine/video/video_player.cpp) для FMV).
Работает в игре, главном меню, attract, cutscenes, outro и FMV.

Состояние выводится в overlay [`debug_timing_overlay.cpp`](../../../new_game/src/game/debug_timing_overlay.cpp)
как `IP: on` / `IP: off`. Пример:
```
phys: 20  lock: unlim  IP: on  fps: 144.3
```

Полный набор debug-клавиш и формат overlay → [`../debug_physics_render_rate.md`](../debug_physics_render_rate.md).

### Logging

В [`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp) есть compile-time флаг:

```cpp
#define RENDER_INTERP_LOG 1   // 0 чтобы выключить
```

При `RENDER_INTERP_LOG=1` пишутся в stderr строки:

- `[render_interp] FIRST_CAPTURE slot=X class=Y dt=Z pos=(...)` — slot впервые попал в capture (после reset/invalidate)
- `[render_interp] FREE slot=X class=Y was_pos=(...)` — slot освобождён через `render_interp_invalidate` (`free_thing` hook)

Используется для отладки spawn/despawn багов. Через grep `[render_interp]` в `stderr.log` видна последовательность жизни slot'ов.

## AnimTween (vertex-morph fraction)

`Draw.Tweened->AnimTween` — это фракция `[0, 256)` морфа между двумя соседними keyframe'ами текущей анимации (`CurrentFrame` и `NextFrame`). Продвигается анимационными хендлерами **на physics-тике** (например `person_normal_animate_speed` в [person.cpp:2848](../../../new_game/src/things/characters/person.cpp#L2848) масштабирует `tween_step` на `TICK_RATIO`). При 20 Hz physics + 60 Hz render одно значение AnimTween держится 3 render-кадра подряд → морф рывками, хотя позиция плавная.

> ⚠️ Раньше документация утверждала «AnimTween обновляется per render-frame» — это было неверно. Тиковый источник подтверждён в коде.

**Что делаем:** capture'им `AnimTween`, `FrameIndex` и указатели `CurrentFrame`/`NextFrame` рядом с углами Tween. В `RenderInterpFrame::ctor` подменяем все три (`dt->AnimTween`, `dt->CurrentFrame`, `dt->NextFrame`) на правильные для текущего alpha значения. Дальше FIGURE_draw / `calc_sub_objects_position` / smap читают подменённые поля и вертексный морф идёт плавно на render-rate.

**Случай 1: FrameIndex стабилен (`frame_index_prev == frame_index_curr`).** За тик анимация не перешла keyframe границу. CurrentFrame/NextFrame одни и те же. Прямой `lerp_i32(prev_AT, curr_AT, alpha)`.

**Случай 2: FrameIndex продвинулся ровно на 1** (`(UBYTE)(curr - prev) == 1`). За тик пересекли одну границу keyframe — CurrentFrame/NextFrame сменились. Используем virtual extended координату:

```
total = (256 - prev_AT) + curr_AT       // фракционных keyframe-units пройдено за тик
v = prev_AT + alpha * total
если v < 256:
    рендер с prev's (CurrentFrame, NextFrame), AnimTween = v
иначе:
    рендер с curr's (CurrentFrame, NextFrame), AnimTween = v - 256
```

Это даёт непрерывный визуальный переход через границу: при `alpha = (256 - prev_AT) / total` мы ровно на стыке двух пар keyframe'ов — pose в обоих интерпретациях одна и та же (последний кадр прошлой пары = первый кадр следующей).

**Случай 3: Loop wrap** — в конце цикла анимации `ANIM_FLAG_LAST_FRAME`-флагнутый keyframe сбрасывает FrameIndex `N→0` (а не инкремент). UBYTE diff в этом случае `256-N`, не 1, и Случай 2 не сработает. Детект: `current_frame_prev->Flags & ANIM_FLAG_LAST_FRAME` (предыдущий тик был на терминальном keyframe). Логика та же что в Случае 2 — формула virtual extended координаты, переключение пары указателей. Зрительно: на стыке (last frame's NextFrame ↔ wrap-around start frame) обе пары дают одну и ту же позу (loop continuity), v=256 граница совпадает.

**Случай 4: FrameIndex продвинулся больше чем на 1 за тик** (TweenStep > 256, очень быстрая анимация). У нас нет промежуточных указателей — пропускаем подмену, рендер использует live curr. Редкий случай.

**Покрытие:** работает для всех `draw_type_uses_tween()` → CLASS_PERSON (Дарси, NPC), CLASS_ANIMAL, CLASS_BAT, CLASS_PYRO, CLASS_CHOPPER, generic DT_TWEEN.

**Что НЕ покрывает:** cross-fade между **разными** анимациями (idle→sprint и т.п.). Когда меняется `CurrentAnim`/`MeshID`, anim-transition детектор ставит `skip_once` → AnimTween prev=curr, никакой смешанной позы. Это отдельный баг → [`known_issues.md`](known_issues.md) #2.

## Что НЕ делает эта система

- **Не меняет физику.** Physics работает как всегда, на 20 Hz, с тем же `process_things` и тем же state.
- **Не делает cross-fade между разными анимациями.** AnimTween интерполируется только в рамках одной анимации (одного `CurrentAnim`/`MeshID`). На границе смены анимации — дискретный скачок (см. [`known_issues.md`](known_issues.md) #2).
- **Не интерполирует партиклы** (`PARTICLE_*` pool). Они в отдельной структуре, движутся через TICK_RATIO. На фоне их короткого lifetime и flicker'а — обычно не бросается в глаза. См. [`plans.md`](plans.md).
- **Не интерполирует wall-clock эффекты** (rain, drip ripples, ribbons, sparks от заборов, sky, fire). Они и так плавные через `g_frame_dt_ms` — см. `CORE_PRINCIPLE.md`.
- **Не уменьшает input latency.** Render lag ~50мс остаётся (показываем интерполированное прошлое). Если нужна латенси — другая задача (поднятие physics Hz с timer scaling), см. [`../fps_unlock.md`](../fps_unlock.md).
