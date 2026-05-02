# Plan: World-space pose snapshot — реализация

> Источник идеи и контекст: [task_new_characters_interpolation_method.md](task_new_characters_interpolation_method.md).
> Архитектура текущей render-interp: [architecture.md](architecture.md).
> Известные баги: [known_issues.md](known_issues.md).

## Принцип

Physics tick доставляет полное VALID render-state в snapshot (per-bone world transforms). Render между тиками лерпит prev_target → curr_target в **мировом** пространстве — не "inputs" (AnimTween, keyframes, body angles), а уже **скомпонованную** мировую позу.

Это эквивалент того что уже работает для DIRT, Vehicles, Camera, EWAY-cam. Characters — единственный laggard, потому что pose composition сложнее (per-bone). Этой задачей подтягиваем characters к тому же принципу.

## ⚠️ Critical: visible root vs Thing.WorldPos

`WorldPos` в Thing — это engine tracking point. На лестнице state-handler пишет `WorldPos.Y += 0.3` в конце цикла анимации, тогда как видимое положение тела (pelvis bone в мире) на этом тике стабильно (cancel-out: `WorldPos += 0.3` компенсирует `bone_local[0].Y -= 0.3`).

Если snapshot захватит **raw WorldPos**, prev/curr будут отличаться на 0.3, и лерп даст jerk даже там где визуальное положение pelvis должно быть стабильно.

**Якорь snapshot — это VISIBLE pelvis world position**, то есть:

```
visible_pelvis_world = WorldPos + R(body_angles) * bone_local[0]_offset
```

Это та точка которая на лестнице действительно смотрится стабильной во время cancel-out тика. Все остальные кости хранятся **относительно** этой точки (через иерархию parent → child).

**Перед стартом реализации:** сделать debug-визуализацию visible pelvis world position и убедиться что на ladder up/down она движется плавно (не прыгает). Если прыгает — найти другую "якорную" точку (например, голова или torso) с действительно гладкой траекторией.

## Snapshot структура

Per bone:
| Поле | Размер | Назначение |
|---|---|---|
| Position | 3 × int32 = 12 байт | bone[0]: VISIBLE world pos; bones[1..N]: local-to-parent offset |
| Rotation | quaternion 4 × int16 fixed-point = 8 байт | bone[0]: world rot; bones[1..N]: local-to-parent rot |

Per Thing (max 15 bones × prev+curr): **600 байт**.
× MAX_THINGS=701 → **~420 KB static**.

Все pose-данные складывать в отдельный пул `g_pose_snaps[MAX_THINGS]` (не в `ThingSnap`) — иначе ThingSnap раздувается с ~60 байт до ~750 байт и iteration по slot'ам становится cache-unfriendly.

## Hierarchy

`body_part_children[15][N]` уже существует в figure_globals или figure.cpp — определяет дерево скелета (PELVIS → TORSO → HEAD; PELVIS → LEFT_FEMUR → LEFT_TIBIA → LEFT_FOOT; и так далее). Inverse `body_part_parent[15]` либо уже есть, либо derive'нем один раз при init.

При apply pose walk идёт **в порядке иерархии**: сначала pelvis (root), потом дети, потом внуки — каждому ребёнку нужно уже посчитанное world-transform его родителя.

## Capture (per physics tick)

В `render_interp_capture` для DT_TWEEN family things:

1. Compute pelvis VISIBLE world transform (= WorldPos + R(body_angles) · bone_local[0]).
2. For each child bone in hierarchy order: compute local-to-parent transform (offset + rotation).
3. Window-shift snapshot (prev ← curr).
4. Write composed values to curr.

Логика composition — это то что figure.cpp делает per-frame. Extract в pure function:

```cpp
void compose_full_skeletal_pose(Thing* p_thing,
                                BoneTransform out[MAX_BONES],
                                int* out_count);
```

— читает `Thing.WorldPos`, `Draw.Tweened->Angle/Tilt/Roll/AnimTween/CurrentFrame/NextFrame`, character_scale (для CLASS_PERSON), компонирует pose, выписывает 15 (или меньше) трансформов. Никаких global state mutation.

## Apply (per render frame)

В `RenderInterpFrame::ctor` для каждого валидного DT_TWEEN snapshot:

```
PoseSnap& s = g_pose_snaps[i];
if (!s.valid) skip;

// Bone 0 (pelvis): world-space lerp
BoneTransform pelvis_lerped;
pelvis_lerped.pos = lerp(s.bones_prev[0].pos, s.bones_curr[0].pos, alpha);
pelvis_lerped.rot = quat_slerp(s.bones_prev[0].rot, s.bones_curr[0].rot, alpha);

// Bones 1..N: local lerp + hierarchy reconstruction
BoneTransform world[MAX_BONES];
world[0] = pelvis_lerped;
for (int i = 1; i < bone_count; i++) {
    int parent = body_part_parent[i];
    BoneTransform local;
    local.pos = lerp(s.bones_prev[i].pos, s.bones_curr[i].pos, alpha);
    local.rot = quat_slerp(s.bones_prev[i].rot, s.bones_curr[i].rot, alpha);
    
    world[i].pos = world[parent].pos + rotate(world[parent].rot, local.pos);
    world[i].rot = quat_mul(world[parent].rot, local.rot);
}
```

`figure.cpp` draw paths читают world[i] для каждой кости через новый API:

```cpp
bool render_interp_get_bone_world_transform(Thing*, int bone_idx,
                                            BoneTransform* out);
```

## Что вырезаем (финальный cleanup)

После того как новый путь работает и протестирован:

| Файл | Что |
|---|---|
| `figure.cpp` | `figure_morph_root_offset` blend branch (~lines 1682-1706) |
| `figure.cpp` | `figure_morph_matrix` blend branch (~lines 1745-1775) |
| `render_interp.h` | `RenderInterpBlend` struct + `render_interp_get_blend` decl |
| `render_interp.cpp` | `render_interp_get_blend` impl |
| `render_interp.cpp` | ThingSnap.blend_active/blend_old_cf/_nf/_at/blend_start_ms/blend_duration_ms |
| `render_interp.cpp` | Anim-transition blend setup в `render_interp_capture` (~lines 518-564) |
| `render_interp.cpp` | AnimTween virtual-extended-coord cases 1/2/3 + snap-on-backward в ctor (~lines 1044-1131) |
| `render_interp.cpp` | Substitution `dt->Angle/Tilt/Roll/AnimTween/CF/NF` в ctor + restore в dtor |
| `render_interp.cpp` | ThingSnap.angle/tilt/roll_prev/curr, anim_tween_*, frame_index_*, current_frame_*, next_frame_*, has_angles, anim_tween_applied |
| `render_interp.cpp` | `skip_once` для anim-only fields (для teleport остаётся отдельно) |
| `debug_interpolation_config.h` | `INTERP_THING_TWEEN_ANGLE`, `INTERP_THING_ANIM_MORPH`, `INTERP_THING_CROSS_ANIM_BLEND` flags + dead branches |

`render_interp_mark_teleport` остаётся (full snapshot wipe — корректно).
Substitution `Thing.WorldPos` остаётся (нужно для других render readers — NIGHT_find lighting, shadows projection origin, EWAY targets, etc).

## Phasing

**Phase 0 (✅ DONE — verified 2026-05-03):** Debug visualisation двух точек на каждом персе:
- `PEL` — visible pelvis world position (= `WorldPos + R(angles) * bone_local_offset[0]`)
- `_____ROOT` — raw `Thing.WorldPos` (engine tracking point)

Per-character colour: Дарси белая, остальные — pseudo-random per-slot stable colour.

**Результаты теста:**
- **Лестница (ladder up/down):** PEL движется плавно, ROOT снапит на каждой границе цикла анимации. Подтверждает что **видимый pelvis — правильный anchor** для snapshot (не WorldPos).
- **Прыжки:** ROOT прыгает на стыках фаз прыжка тоже. Это **объясняет почему текущие jump-баги (#2 в [known_issues.md](known_issues.md)) требовали костылей** на 20 Hz и всё равно остаются на 5 Hz: state-handler прыжка делает discrete WorldPos snap-ы, текущая интерполяция linerly lerp'ит через них → визуальный jerk. World-pose snapshot должен это закрыть автоматически (visible pelvis движется плавно через все фазы прыжка, render видит smooth target).

**Debug visualisation оставлена активной** в коде ([aeng.cpp](../../../new_game/src/engine/graphics/pipeline/aeng.cpp)) до завершения Phase 5. На каждом этапе можно визуально проверять: PEL должен оставаться плавным, ROOT — снапить (это правильное поведение). Удалить debug блоки в самом конце вместе с другими hacks.

**Phase 1:** Extract `compose_full_skeletal_pose` — pure function. Golden test: composer output == current figure.cpp per-bone values на нескольких сэмпл-сценах (idle, walk, jump, ladder).

**Phase 2:** Snapshot extension + capture call. На этом этапе snapshot захватывается per-tick, но render path его не читает — продолжает работать через старый substitution path.

**Phase 3:** New apply path за флагом `INTERP_THING_WORLD_POSE`. Когда флаг on, figure.cpp читает from snapshot. Когда off — старый путь (для отката).

**Phase 4:** Test (см. ниже).

**Phase 5:** Cleanup всех hacks (см. таблицу выше) **+ debug visualisation в aeng.cpp**. Удалить старый флаг.

**Phase 6:** Подключить shadow rendering path к тому же snapshot API. Закрывает [bug 2b](known_issues.md#2b).

## Test scenarios (success criteria)

| Сценарий | Критерий |
|---|---|
| Ladder up/down @ 20Hz | Никаких видимых jerks; pelvis движется плавно |
| Ladder up/down @ 5Hz (debug key 1) | То же самое, более выпукло |
| Arrest cuffing | Нет видимого spin'а во время flip'а (если анимация cancel-out designed) |
| Walk / run / sprint | Без регрессий vs current (оба корректные) |
| Cross-anim transitions (idle→walk→run→walk→idle) | Плавные, не хуже current |
| Combat (jump-kick, punch combo) | Не хуже current state. Если станет хуже — оценить, может потребоваться extra blend window поверх pose snapshot |
| Cutscenes (RTA intro, mission cinematics) | Без регрессий |
| Vehicles | Без регрессий (отдельный путь, не трогаем) |
| DIRT pool (leaves) | Без регрессий (отдельный путь, не трогаем) |
| Shadow synchronization | После Phase 6: тень синхронна с телом во всех transitions |

## Risks (recap)

| Risk | Severity | Mitigation |
|---|---|---|
| Quat slerp обязателен для rotation interp | Mechanical | Использовать существующую CQuaternion::BuildTween / Slerp |
| Engineering risk в pose composer extract | Medium | Golden test (Phase 1) до integration |
| CPU cost capture (~1.5%@20Hz Steam Deck) | Medium | Frustum cull capture для invisible characters (follow-up если нужно) |
| `INTERP_VEHICLE_ANGLE` cancel-out edge cases? | Low | Vehicle angles интерполируются independent from pose snapshot — никакого conflict |
| Anim transition cases с large pose change в world space | Low | World-space lerp всё равно гладкий, если станет хуже — addressable post-Phase 5 |

## Что не трогаем

- Vehicle yaw/tilt/roll path — уже работает по принципу world-space.
- DIRT pool — уже работает по принципу world-space.
- Camera (FC_cam, EWAY_cam) — уже работает.
- Particles — отдельная задача (см. plans.md).
- DrawMesh angles (DT_BIKE, DT_CHOPPER) — отдельная задача.

## Diagnostic infrastructure

`RENDER_INTERP_LOG` flag остаётся (capture/free events).
`FIGURE_MORPH_LOG` (root bone CSV log в figure.cpp:1708) — может быть полезен в Phase 1 для golden test.

Возможно добавить новый `POSE_SNAPSHOT_LOG` для отладки самого snapshot capture (per-bone values, hierarchy walk).
