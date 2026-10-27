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
  - **Сохраняет** живое значение `Thing.WorldPos` (+ `Genus.Vehicle->Angle/Tilt/Roll` для CLASS_VEHICLE) в `saved_*` поля snapshot'а
  - **Записывает в Thing напрямую** интерполированное значение
- Аналогично для `FC_cam[0]` и DIRT pool
- Помечает `applied=true`

**Деструктор** `~RenderInterpFrame()`:
- Проходит по тому же массиву
- Для каждого `applied=true`: восстанавливает saved_* обратно в живой Thing/Vehicle/FC_cam/DIRT

В результате **между ctor и dtor** живые поля `Thing.WorldPos` / `Vehicle->Angle/Tilt/Roll` / camera / DIRT содержат интерполированные значения. Это нужно для readers которые читают `Thing.WorldPos` напрямую (NIGHT_find lighting, shadow projection origin, EWAY targets, audio listener tracking).

**Per-bone pose интерполяция персонажей идёт через отдельный механизм** — pose snapshot, не через substitution. Substitute'ить `Draw.Tweened->Angle/Tilt/Roll/AnimTween/CurrentFrame/NextFrame` больше **не нужно**: render-функции figure.cpp читают per-bone world transform напрямую из pose snapshot. Подробно в секции «Pose snapshot — character body» ниже.

После dtor — снова actual values. Никакая логика/AI/физика между ctor и dtor не выполняется (всё уже отработало в physics-loop), поэтому никто кроме рендера не видит интерполированное состояние.

## Файлы

| Файл | Назначение |
|---|---|
| [`new_game/src/engine/graphics/render_interp.h`](../../../new_game/src/engine/graphics/render_interp.h) | API: `g_render_alpha`, `render_interp_capture/_invalidate/_mark_teleport`, `render_interp_capture_camera/_mark_camera_teleport`, `render_interp_capture_dirt/_mark_dirt_teleport`, `render_interp_reset`, `render_interp_compute_pose`, `render_interp_get_cached_pose`, класс `RenderInterpFrame` |
| [`new_game/src/engine/graphics/render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp) | Реализация: snapshot-массивы `g_thing_snaps[MAX_THINGS]`, `g_pose_snaps[MAX_THINGS]`, `g_cam_snaps[FC_MAX_CAMS]`, `g_dirt_snaps[DIRT_MAX_DIRT]`, `g_eway_cam_snap`; capture/lerp функции; ctor/dtor `RenderInterpFrame`; per-frame pose cache |
| [`new_game/src/engine/graphics/geometry/pose_composer.h/.cpp`](../../../new_game/src/engine/graphics/geometry/pose_composer.cpp) | Pure pose composition: `compose_full_skeletal_pose` (15-bone person hierarchy, iterative DFS) и `compose_flat_skeletal_pose` (flat skeleton для DT_ANIM_PRIM bat / Bane / Balrog / Gargoyle и DT_TWEEN с non-15 ElementCount). Mirrors figure.cpp's per-bone math; no globals mutated |
| [`new_game/src/engine/graphics/geometry/figure.cpp`](../../../new_game/src/engine/graphics/geometry/figure.cpp) | Override блоки в `FIGURE_draw_prim_tween_person_only_just_set_matrix`, `FIGURE_draw_prim_tween_person_only`, `FIGURE_draw_prim_tween`, `FIGURE_draw_prim_tween_reflection` — читают `render_interp_get_cached_pose(p_thing)[bone_idx]` и заменяют (off_x/y/z, mat_final, fmatrix) перед `POLY_set_local_rotation` |
| [`new_game/src/engine/graphics/lighting/smap.cpp`](../../../new_game/src/engine/graphics/lighting/smap.cpp) | Override в `SMAP_add_tweened_points` (shadow projection) — тот же pose snapshot что body draw |
| [`new_game/src/game/game.cpp`](../../../new_game/src/game/game.cpp) | Capture-loop в конце physics-тика, расчёт alpha, frame-scope в `draw_screen`, `render_interp_reset()` перед main loop |
| [`new_game/src/things/core/thing.cpp`](../../../new_game/src/things/core/thing.cpp) | Hook `render_interp_invalidate(t_thing)` в `free_thing()` |
| [`new_game/CMakeLists.txt`](../../../new_game/CMakeLists.txt) | Подключение `render_interp.cpp`, `pose_composer.cpp` |

## Структуры данных

### `ThingSnap` (per-Thing snapshot — позиция и vehicle углы)

```cpp
struct ThingSnap {
    GameCoord pos_prev, pos_curr;
    bool valid;            // false до первого capture после reset/invalidate

    // Skeleton tracking — capture_pose использует для invalidate per-bone
    // snapshot при смене mesh / anim / skeleton-type.
    UBYTE last_mesh_id;
    SLONG last_anim;
    UBYTE last_seen_class;  // диагностика slot-reuse через RENDER_INTERP_LOG

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
    SLONG saved_veh_angle, saved_veh_tilt, saved_veh_roll;
    bool applied;
    bool veh_angles_applied;  // ctor wrote interp values into Genus.Vehicle->
                              // Angle/Tilt/Roll; dtor restores them.
};
```

Массив `g_thing_snaps[MAX_THINGS]`, индексируемый по `Thing - THINGS` (pointer arithmetic против глобального массива `THINGS[]`).

### `PoseSnap` (per-Thing per-bone pose snapshot — для тела персонажа)

```cpp
struct BoneSnap {
    SLONG x, y, z;       // bone[0]: WORLD pos; bones[1..N-1] hierarchical: parent-local offset
                         // flat skeleton: WORLD pos для всех bones
    CMatrix33 cmat;      // bone[0]: WORLD rot;  bones[1..N-1] hierarchical: parent-local rot
                         // flat: WORLD rot для всех. CMatrix33 = packed 10-bit-per-component
                         // (тот же формат что keyframes — половина storage vs Matrix33)
};

struct PoseSnap {
    BoneSnap bones_prev[POSE_MAX_BONES];   // POSE_MAX_BONES = 20
    BoneSnap bones_curr[POSE_MAX_BONES];
    UBYTE last_mesh_id;     // invalidate when this changes (different skeleton)
    UBYTE bone_count;       // 15 для person hierarchy; 1..20 для flat
    bool  flat_skeleton;    // true: bones[i] = WORLD; false: bone[0]=WORLD + bones[1..14]=parent-local
    bool  valid;
};
```

Массив `g_pose_snaps[MAX_THINGS]`, ~670 KB total. Two skeleton types:

- **Hierarchical (person rig, 15 bones)** — для `CLASS_PERSON && DT_ROT_MULTI && ElementCount == 15`. `bone[0]` = visible PELVIS world transform (smooth anchor — преодолевает ladder-cancel-out / jump-snap WorldPos дискретности); `bones[1..14]` = parent-local (offset + rotation в parent's local frame). На apply иерархия восстанавливается chain'ом `parent_world × local_to_parent → child_world` — preserves rigid joint connections даже при rotational motion.
- **Flat (animals/bats/Bane/Balrog/Gargoyle)** — для `DT_ANIM_PRIM` и `DT_TWEEN` с `ElementCount != 15`. Каждая кость capture'ится как world transform независимо (mirrors `FIGURE_draw_prim_tween` parent==NULL ветку: `WorldPos + R(angles) × keyframe_offset_lerped × scale`). Apply lерпит world pos / slerps world rot per element без иерархии.

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

### Углы 0..2047 (DIRT pool yaw/pitch/roll)

`SWORD`-углы DIRT debris (yaw/pitch/roll). Вход может быть отрицательным (`-1 == 2047`). Выход всегда в `[0, 2047]`.

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

(Person/animal/bat угол body больше не лерпится через эту функцию — это часть pose snapshot composer'а, который выдаёт world rotation per bone напрямую.)

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
- `void render_interp_mark_teleport(Thing*)` — полный сброс snapshot'а thing'а через `render_interp_invalidate`. Wipes ОБА snap pool'а: `g_thing_snaps[idx]` (pos + vehicle angles) и `g_pose_snaps[idx]` (per-bone pose). На следующем capture идёт `!valid` ветка, prev=curr=current — никакого лерпа через teleport. Используется в EWAY-обработчиках ([`eway.cpp`](../../../new_game/src/missions/eway.cpp) — `EWAY_DO_CREATE_ENEMY`, `EWAY_DO_MOVE_THING`) и PCOM wander-recycle ([`pcom.cpp`](../../../new_game/src/ai/pcom.cpp) — 5 callsites для passenger/vehicle teleports across the map).
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

## Pose snapshot — character body

Тело Tween-семейства (CLASS_PERSON + DT_ANIM_PRIM bat/Bane/Balrog/Gargoyle + generic DT_TWEEN) интерполируется **per-bone в world space**, не через substitution `dt->Angle/Tilt/Roll/AnimTween/CF/NF`.

### Зачем pose snapshot вместо substitution

Старый подход: render-frame ctor подменяет `Draw.Tweened->Angle/Tilt/Roll/AnimTween/CurrentFrame/NextFrame` на интерполированные значения, дальше figure.cpp заново композирует pose из подменённых полей. Это работало для базовой плавности, но имело структурные проблемы:

1. **Cancel-out cases** (лестница, прыжки): state-handler делает `WorldPos.Y += 0.3` одновременно с reset bone offset обратно в 0 в keyframe — итоговая видимая поза не меняется, но компоненты лерпятся независимо → визуальный jerk в середине alpha. Подробно в [`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md) "Канонический пример: лестница".
2. **Anim transitions** (idle→sprint, jump-kick) требовали costly cross-anim blend hack (per-bone морф lerp(old_pose, new_pose, blend_t) c hard-coded blend duration ~50ms). Edge cases (jump landing U-shape, combat snap-on-backward, virtual extended coordinate trick для frame transitions) накапливались.
3. **`AnimTween` имеет gameplay-induced backward jumps** (combat code пишет `AnimTween = -twist`) — naive lerp визуально проигрывал анимации в обратку.

Pose snapshot решает всё три класса автоматически: capture делает full pose composition в physics tick (composer mirrors figure.cpp's per-bone math), render просто лерпит per-bone world transforms между prev/curr. Cancel-out проявляется в композиции: visible pelvis pos стабилен через ladder cycle, render лерпит между двумя стабильными значениями = плавно. Anim transitions работают так же — composer выдаёт post-tick pose независимо от того сколько keyframes между prev и curr; render видит smooth target.

### Capture (per physics tick)

В `render_interp_capture` для tween-family Things вызывается `capture_pose(idx, p_thing)`:

1. **Path selection:** `is_hierarchical = (CLASS_PERSON && DT_ROT_MULTI && ElementCount == 15)`. Иначе flat path.
2. **Skeleton-type / mesh-id change:** invalidate snapshot — bone count / layout мог измениться (`s.flat_skeleton != !is_hierarchical || s.last_mesh_id != dt->MeshID` ⇒ `s.valid = false`).
3. **Hierarchical:** вызвать `compose_full_skeletal_pose(p_thing, &pose)`, скопировать `bones[0]` как world (compress_matrix → CMatrix33), для `bones[1..14]` derive parent-local через `local_pos[i] = parent_rot_inv × (end_pos[i] - end_pos[parent])` + `local_rot[i] = parent_rot_inv × end_mat[i]`. Identities cancel out R_body/WorldPos — данные purely от anim keyframes, лерп через rotations gives smooth joint connections.
4. **Flat:** вызвать `compose_flat_skeletal_pose(p_thing, &pose)`, скопировать каждую кость как world (compress_matrix). `bone_count = pose.bone_count` (1..20). No parent walk.
5. **Window shift:** `s.bones_prev[i] = s.bones_curr[i]; s.bones_curr[i] = new_bones[i]`. Первый capture: prev = curr = new (no lerp).

### Apply (per render frame)

`render_interp_compute_pose(p_thing, BoneInterpTransform out[POSE_MAX_BONES])` лерпит per-bone:

- **Hierarchical:** `out[0]` = direct world lerp(prev[0], curr[0], alpha) + slerp(prev[0].cmat, curr[0].cmat). Для `i=1..14`: lerp parent-local pos + slerp parent-local rot, затем chain через parent's уже-вычисленный world transform: `out[i].pos = out[parent].pos + (out[parent].rot × local_pos)/256` + `out[i].rot = out[parent].rot × local_rot`. Iteration order body_part_parent[i] < i гарантирует parent готов до child.
- **Flat:** прямой world lerp + slerp per element, без иерархии.

### Cache + override

`render_interp_get_cached_pose(p_thing)` — keyed по `(Thing*, g_render_interp_frame_counter)`. Все потребители (main body draw, gun/muzzle, reflection, shadow, NIGHT_find lighting, grenade-in-hand, HUD selection oval, gun sights, lens flare) ходят через один кэш — иерархия рассчитывается не более одного раза на Thing на render frame, регардлесс сколько render paths трогают этот Thing.

В figure.cpp draw functions (`FIGURE_draw_prim_tween_person_only_just_set_matrix`, `FIGURE_draw_prim_tween_person_only`, `FIGURE_draw_prim_tween`, `FIGURE_draw_prim_tween_reflection`) перед `POLY_set_local_rotation` стоит override block:

```cpp
if constexpr (ri_cfg::INTERP_THING_WORLD_POSE) {
    const BoneInterpTransform* pose = render_interp_get_cached_pose(p_thing);
    if (pose) {
        SLONG part = FIGURE_dhpr_rdata1[recurse_level].part_number;  // или part_number arg для flat
        if (part >= 0 && part < POSE_MAX_BONES) {
            off_x = pose[part].pos_x;
            off_y = pose[part].pos_y;
            off_z = pose[part].pos_z;
            mat_final = pose[part].rot;
            // recompute fmatrix from mat_final
        }
    }
}
```

Override fall-through к legacy math если pose недоступен (interp off / world-pose flag off / snapshot invalid right after teleport).

### Reflection / shadow

- **Reflection** (`FIGURE_draw_prim_tween_reflection`): override pose из snapshot + геометрическое water mirror через y=H плоскость. Position: `y_reflected = 2 × FIGURE_reflect_height - y_world`. Rotation: левое умножение `M_reflect × mat_final` где `M_reflect = diag(1, -1, 1)` = негирование Y row (M[1][*]) в world rotation.
- **Shadow** (`SMAP_add_tweened_points` в smap.cpp): override (x, y, z, mat_final) из snapshot перед vertex transform. Тень читает идентичные pose values что и тело — синхронизация автоматическая во всех transitions без specialized blend logic. Закрывает старый bug 2b (тень desync).

## Vehicle yaw/tilt/roll — отдельная ветка

CLASS_VEHICLE хранит yaw/tilt/roll в `Genus.Vehicle->Angle/Tilt/Roll` (SLONG), не в `Draw.Tweened`. Это поле читает `make_car_matrix` ([vehicle.cpp:125](../../../new_game/src/things/vehicles/vehicle.cpp#L125)) и `draw_car`. `ThingSnap` снимает `veh_angle/tilt/roll_prev/curr` плюс `veh_velr_curr` (`Vehicle->VelR`, rotational velocity). При apply: `lerp_angle_2048_directed` (знак VelR как direction hint long-path при very fast spin > 180°/tick) для Angle, обычный short-path `lerp_angle_2048_slong` для Tilt/Roll. Pointer-identity guard в ctor/dtor (`p->Genus.Vehicle == s.veh_ptr_curr`) защищает от cutscene rebind crash'ей.

## Не покрыто

- `DT_MESH` — статичные mesh. Углы в `Draw.Mesh->Angle/Tilt/Roll` (UWORD). Большинство таких объектов не вращаются.
- `DT_BIKE` — велосипеды. Через DrawMesh.
- `DT_CHOPPER` — вертолёты. Через DrawMesh, не Tween. Если потребуется плавность поворотов — расширить DrawMesh-ветку аналогично Vehicle.
- `DT_PYRO` — pyro-эффекты, state в `Genus.Pyro`, нет direct angle storage.

`DT_CHOPPER` и `DT_PYRO` исторически ошибочно числились в Tween-семействе (читали DrawMesh / stale память через union как DrawTween) — крашило ctor на non-canonical pointer'е в катсценах. Сейчас они в `draw_type_uses_tween()` НЕ входят. См. [`coverage.md`](coverage.md).

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

### Skeleton tracking (внутри `capture_pose`)

```cpp
bool is_hierarchical = (p_thing->Class == CLASS_PERSON
                        && p_thing->DrawType == DT_ROT_MULTI
                        && dt->TheChunk->ElementCount == 15);

if (s.valid && (s.last_mesh_id != dt->MeshID
                || s.flat_skeleton == is_hierarchical)) {
    s.valid = false;  // skeleton-type change → invalidate
}
```

Цель: при смене скелета (другой mesh / тип скелета изменился, e.g. человек→животное в одном slot) bone count или layout могут отличаться — данные предыдущих bones meaningless. Invalidate ⇒ next capture идёт по `!valid` ветке, prev=curr=new.

**Anim transitions сами по себе не требуют skip_once** в pose snapshot path — composer выдаёт correct post-tick pose независимо от того сколько keyframes между prev и curr; render лерпит prev/curr world transforms плавно через transition. Это resolution Phase 5 cleanup'а: старые hacks (skip_once для anim-transitions, cross-anim blend, virtual extended coordinate) больше не нужны.

## Производительность

**Память:** `~700 KB` статически. Никакой динамики (`malloc`/`new`/vector resize) каждый кадр.

| Структура | Размер |
|---|---|
| `g_thing_snaps[MAX_THINGS=701]` | ~50 байт × 701 ≈ 35 KB |
| `g_pose_snaps[MAX_THINGS=701]` | ~960 байт × 701 ≈ 670 KB (POSE_MAX_BONES=20 × prev+curr × 24 байт BoneSnap) |
| `g_cam_snaps[FC_MAX_CAMS=2]` | ~80 байт × 2 ≈ 160 B |
| `g_dirt_snaps[DIRT_MAX_DIRT=1024]` | ~30 байт × 1024 ≈ 30 KB |

**Время:**
| Операция | Частота | Стоимость |
|---|---|---|
| `render_interp_capture` per tick | 20 Hz | ~100-200 things × pose composition + parent-local derivation ≈ ~50-100µs total на тик (большая часть — pose composer math) |
| `RenderInterpFrame` ctor | per render frame | iterate 701 slots, lerp WorldPos + vehicle angles ≈ ~2µs |
| `RenderInterpFrame` dtor | per render frame | restore saved ≈ ~1µs |
| `render_interp_compute_pose` (per Thing per render frame) | render rate | 15-bone hierarchy walk OR flat lerp loop ≈ ~3µs/Thing. Cached per render frame — каждый Thing считается max 1 раз регардлесс number of render paths |

На 60 FPS типичная сцена (~50 visible characters): pose composition ~150µs/render frame (=0.9% на 60Hz, ~5% на 144Hz). Capture ~100µs / 50ms physics tick = 0.2% CPU. Steam Deck overhead приемлем.

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

## Что НЕ делает эта система

- **Не меняет физику.** Physics работает как всегда, на 20 Hz, с тем же `process_things` и тем же state.
- **Не интерполирует партиклы** (`PARTICLE_*` pool). Они в отдельной структуре, движутся через TICK_RATIO. На фоне их короткого lifetime и flicker'а — обычно не бросается в глаза. См. [`plans.md`](plans.md).
- **Не интерполирует wall-clock эффекты** (rain, drip ripples, ribbons, sparks от заборов, sky, fire). Они и так плавные через `g_frame_dt_ms` — см. `CORE_PRINCIPLE.md`.
- **Не уменьшает input latency.** Render lag ~50мс остаётся (показываем интерполированное прошлое). Если нужна латенси — другая задача (поднятие physics Hz с timer scaling), см. [`../fps_unlock.md`](../fps_unlock.md).
