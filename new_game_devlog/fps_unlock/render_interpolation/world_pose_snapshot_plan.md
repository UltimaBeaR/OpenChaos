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

**Phase 1 (✅ DONE — verified 2026-05-03):** Extract `compose_full_skeletal_pose` — pure function в [`pose_composer.h`](../../../new_game/src/engine/graphics/geometry/pose_composer.h)/[`.cpp`](../../../new_game/src/engine/graphics/geometry/pose_composer.cpp). Iterative pre-order DFS по `body_part_children[][]`, mirrors figure.cpp:1851 (HIERARCHY) + figure.cpp:1859 (root offset) + figure.cpp:1868-1881 (world pos with character_scale) + figure.cpp:1894-1905 (mat_final). No globals mutated.

Golden test через `__________PEL_NEW` debug label в [`aeng.cpp`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp). Verified visually на ladder, wall climbing, jumps:
- PEL_NEW всегда совпадает с PEL (визуально one label) на каждом персе.
- Diff 1-2 пикселя в некоторых фазах анимации — precision delta между путями (FMATRIX_calc int vs FIGURE_rotate_obj float, разный Roll handling, разный scale application path). PEL_NEW = ground truth (то что figure.cpp реально рендерит); PEL — приближение через `calc_sub_objects_position`.
- С интерполяцией body + PEL + PEL_NEW глючат синхронно (текущий bug), expected — composer читает substituted state.

**Phase 2:** Snapshot extension + capture call. На этом этапе snapshot захватывается per-tick, но render path его не читает — продолжает работать через старый substitution path.

**Phase 3 (✅ DONE — verified 2026-05-03):** New apply path за флагом `INTERP_THING_WORLD_POSE` в [`debug_interpolation_config.h`](../../../new_game/src/debug_interpolation_config.h). Реализация:
- `render_interp_compute_pose()` в [`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp) — лерпит per-bone из snapshot (bone[0] world pos+rot, bones[1..14] parent-local + hierarchy reconstruction).
- `g_render_interp_frame_counter` — bumped в `RenderInterpFrame::ctor`, для cache invalidation в figure.cpp.
- В [`figure.cpp`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L3520) `FIGURE_draw_prim_tween_person_only_just_set_matrix` — override `(off_x/y/z, mat_final, fmatrix)` из snapshot перед `POLY_set_local_rotation`.
- Bug fix `compress_matrix`: clamp 10-bit signed (избегает sign-flip identity matrix → reflected body).
- Debug labels (PEL/ROOT/PEL_NEW/PEL_SNAP) переехали под `ri_cfg::DEBUG_POSE_LABELS` (off by default).

**Тесты пройдены:**
- Лестница вверх/вниз — тело движется плавно, никаких jerk'ов.
- Прыжки (включая phase transitions) — плавно, без снапов root.
- Ходьба/бег — без регрессий.
- PEL_NEW и тело синхронны (оба читают через snapshot).

**Известные нерешённые баги ВНЕ scope Phase 3** (нужны отдельные фазы):
- Shadow desync ([`known_issues.md`](known_issues.md) #2b) — shadow rendering path не вызывает render_interp_compute_pose, использует legacy. Phase 6.
- MIB destruct body rise ([`../fps_unlock_issues.md` #19](../fps_unlock_issues.md)) — render-path '+=' в WorldPos затирается substitute path. Не относится к pose snapshot — отдельный фикс нужен.

**Что НЕ покрыто в Phase 3** (legacy path остался, выглядит как раньше):
- `FIGURE_draw_prim_tween` (animals/bats/generic DT_TWEEN, не 15-bone).
- `FIGURE_draw_prim_tween_reflection` (water reflections).
- `FIGURE_draw_prim_tween_person_only` (gun + muzzle flash).

**Phase 4 (✅ DONE — verified 2026-05-03):** Расширение snapshot path на reflection + shadow.
- **Refactor:** общий per-frame pose cache переехал из figure.cpp в render_interp.cpp как `render_interp_get_cached_pose(Thing*)` API. Все потребители (main draw, reflection, shadow) ходят через один кэш — hierarchy walk выполняется не более одного раза на Thing на render frame регардлесс сколько путей рендеринга.
- **Reflection** ([figure.cpp `FIGURE_draw_prim_tween_reflection`](../../../new_game/src/engine/graphics/geometry/figure.cpp)): override pose из snapshot + геометрическое water mirror через y=H плоскость:
  - position: `y_reflected = 2 * FIGURE_reflect_height - y_world`.
  - rotation: `M_reflect × mat_final` где `M_reflect = diag(1, -1, 1)`. Левое умножение = негирование **Y row** (M[1][*]) в финальной world rotation. ⚠️ Не Y col — это сначала пробовал, давало visual desync (ноги отдельно от тела); legacy негирует Y col у R_BODY (body-frame matrix) что работает для upright тел но не для пер-кости WORLD rotation.
- **Shadow** ([smap.cpp `SMAP_add_tweened_points`](../../../new_game/src/engine/graphics/lighting/smap.cpp)): override (x, y, z, mat_final) из snapshot перед vertex transform. Закрывает [bug 2b](known_issues.md#2b-тень-дарси-не-синхронизирована-с-телом-на-сменах-анимации) полностью — тень теперь читает тот же snapshot что и тело, синхронизирована во всех transitions.

Тесты пройдены: тень синхронна с телом во всех scenarios. Reflection корректное и плавное во всех направлениях (после fix Y row vs Y col).

**Phase 5:** Cleanup всех hacks (см. таблицу выше) **+ debug visualisation в aeng.cpp**. Удалить старый флаг.

**Phase 6 — частично переехал в Phase 4:** Shadow path интегрирован в Phase 4 (см. выше). Если найдутся другие render paths которые используют per-bone pose (ANIM_obj_draw для animals/bats, gun/muzzle path) — расширить аналогично.

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
