# Task: World-space pose interpolation for characters

> **Контекст для агента:** прочитай эту задачу целиком, потом
> [`architecture.md`](architecture.md), потом [`coverage.md`](coverage.md),
> потом [`known_issues.md`](known_issues.md). После — приступай. Не торопись:
> главная сложность здесь не сам lerp, а понимание pose composition в
> [figure.cpp](../../../new_game/src/engine/graphics/geometry/figure.cpp) и
> [interact.cpp](../../../new_game/src/things/core/interact.cpp).

## Постановка проблемы

Текущая render-interp архитектура лерпит **независимо**:

1. `Thing.WorldPos` (мировая позиция корня тела).
2. `Draw.Tweened->Angle/Tilt/Roll` (поворот тела целиком, "Tween-углы").
3. `Draw.Tweened->AnimTween + CurrentFrame + NextFrame` (vertex morph внутри
   одной анимации — фракция перехода между keyframe pair).

Финальная мировая позиция кости (sub-object) собирается **на лету в `figure.cpp` draw path** как
`WorldPos + bone_local_offset(AnimTween, CurrentFrame, NextFrame, Tween-углы)`.

**Это даёт визуальные глюки когда state-handler одновременно меняет
WorldPos и keyframes так, что итог в мировой системе не меняется**:

### Канонический пример: лестница

State handler (внутри `process_things` физ-тика):
- В пределах цикла анимации карабкания keyframes describe, как тело
  поднимается **относительно корня** на ~0.3 м к концу цикла.
- В конце цикла anim wrap'нулся (`FrameIndex` сбросился к 0,
  `CurrentFrame`/`NextFrame` указывают на начальные keyframes — bone
  offset = 0).
- На том же физ-тике `WorldPos.Y += 0.3 m` (state handler "застолбил"
  поднятый рост).

Концептуально **нет реального движения кости в мировом пространстве**:
смещение от корня просто перешло из bone-local в World, итог тот же.
Без интерполяции это так и работает (все рендеры до wrap'а были на
последнем keyframe → bone Y world ≈ root_Y + 0.3 м; первый рендер после
wrap'а → bone Y world = (root_Y + 0.3) + 0 = ровно та же мировая Y).

С нашей текущей интерполяцией:
- Lerp WorldPos.Y: 0 → 0.3, на alpha=0.5 = **+0.15 м**.
- Lerp morph: virtual extended coord даёт плавный переход keyframe pair
  → bone offset на alpha=0.5 ≈ **+0.033 м**.
- Composed bone Y = **0.183 м** (а должно быть стабильно 0.3 м во
  всём диапазоне alpha).

Кость в render frame **проседает на ~0.12 м в середине alpha**, потом
доходит до 0.3 м к alpha=1. Это и есть наблюдаемый "прыжок дарси
вниз и подтяжка вверх" на лестнице. На замедленной физике (5 Hz)
длительность тика 200 мс — диссонанс отлично видно. На 20 Hz короткий
jerk.

### Канонический пример 2: arrest spin

State handler (DYING / DEAD with substate DEAD_CUFFED) на одном тике:
- Смена `CurrentAnim` на arrest-flip-anim (mesh не меняется).
- **Одновременная** запись `Draw.Tweened->Angle` со сдвигом ~90° (тело
  flip'ается лицом вверх для cuffing).

Render-interp arc-lerps Tween Angle через 90° за 50мс. Затем те же 90°
на возврате через многие тики позже — снова arc. Visible spin.

Здесь cancel-out не такой чистый как с лестницей (два события
разнесены на много секунд). Но **тот же класс багов**: state handler
делает дискретный snap, render лerp'ит его как непрерывное движение.

Гипотеза: **per-bone world-space pose lerp** даёт правильное
поведение для **обоих** случаев — render видит "куда кость финально
оказалась после physics tick" и идёт к этой точке плавно. Если в
итоговом world-space ничего не сдвинулось (cancel-out), render просто
показывает константную позу. Если сдвинулось (legit motion), render
плавно тянется.

## Текущее состояние кода (что уже сделано)

- Render-interp infrastructure готова — capture в конце physics tick,
  apply через `RenderInterpFrame::ctor/dtor` в `draw_screen()`. Подмена
  live `Thing.WorldPos`, `Draw.Tweened->*` на интерполированные значения,
  потом восстановление в dtor.
- Snapshot хранит prev/curr `WorldPos`, prev/curr `AnimTween + FrameIndex
  + CurrentFrame + NextFrame` для anim morph через keyframe boundaries.
- Есть отдельный путь `render_interp_get_blend()` — query'ится из
  figure.cpp при cross-anim transitions. Возвращает per-body-part fade
  state из старой анимации в новую (50–200 мс window).
- DIRT pool, FC_cam, EWAY_cam, vehicle angles покрыты отдельно — не
  трогать в этой задаче.
- **Конфиг флагов**:
  [`new_game/src/debug_interpolation_config.h`](../../../new_game/src/debug_interpolation_config.h).
  Все 8 флагов `ri_cfg::INTERP_*` сейчас `true` (default behaviour).
  Использовать для A/B testing — выключить отдельные пути и убедиться,
  где именно артефакт.

## Что предлагается — два варианта

Оба варианта используют **общий** базовый refactor: вытащить pose
composition из figure.cpp в reusable function. После этого:

- **Вариант A (per-bone snapshot):** в conце physics-тика для каждого
  Tween-Thing скомпонировать **полную** скелетную позу
  (per-bone world transforms — позиция + матрица), сохранить в snapshot
  prev/curr. В render frame `figure.cpp` читает уже скомпонированный
  prev/curr per-bone, лerp'ит pos и rotation между ними.

- **Вариант B (double pose evaluation):** snapshot хранит **то что
  хранит сейчас** (prev/curr WorldPos + prev/curr keyframe pointers и
  AnimTween). В `figure.cpp` draw-pathи (FIGURE_draw / FIGURE_draw_*) при
  каждом render frame compose'им позу **дважды** — один раз с prev
  state, второй раз с curr state. Лerp'им per-bone финальные мировые
  позиции/повороты между этими двумя композициями.

Cancel-out работает в обоих:
- Pose_prev = WorldPos_prev + bone_local_via_prev_keyframes(AnimTween_prev).
- Pose_curr = WorldPos_curr + bone_local_via_curr_keyframes(AnimTween_curr).
- Если physics сделал cancel-out (root +0.3 м, bone offset −0.3 м),
  оба composed pose дают одинаковое world Y. Lerp(same, same) = same.
  Render показывает константную позу через wrap.

## Сравнение по сложности и рискам

Оба варианта разделяют **общий refactor** — extract pose composition
logic из `figure.cpp` в reusable function. Это **основная масса работы**
(см. ниже "Where to start"). После этого:

| | Вариант A | Вариант B |
|---|---|---|
| Доп. работа сверх baseline | ~40–50% | ~15–20% |
| Доп. memory | ~640 KB (per-bone × all slots × prev/curr) | 0 (snapshot не расширяется) |
| Per-tick capture cost | full pose eval × visible Things | 0 |
| Per-render-frame cost | lerp pre-composed (cheap) | 2× pose eval per Thing (~7 M ops/sec total на 60 FPS × 100 Things — pasable) |
| Архитектурная чистота | "render reads pre-composed state" — clean | "compose twice + lerp" — менее clean но проще |

### Доп. риски варианта A
1. **Snapshot lifecycle.** Per-bone array sized per mesh. На смене MeshID
   надо invalidate (уже есть `last_mesh_id` check в capture, но array
   sizing требует max-bone budget или dynamic resize → новые edge
   cases).
2. **Memory layout & cache.** 640 KB extra в snapshot pool. При линейном
   проходе по slots — новый cache footprint. Не страшно, но мерять.
3. **Capture timing.** Pose eval в physics-tick context (после
   `process_things`, рядом с другими `render_interp_capture_*`). Если
   что-то ещё мутирует `dt->*` между capture и render frame, snapshot
   станет stale. У нас `RenderInterpFrame::ctor` — единственный writer
   live `Thing.WorldPos / dt->*` в render path; должно быть ОК но
   проверить.
4. **Skeleton edge cases.** Некоторые DT_TWEEN things (animals, bats)
   имеют разные bone counts. Sized buffer должен покрывать максимум.

### Доп. риски варианта B
1. **Composer purity.** `calc_sub_objects_position` ([interact.cpp:752](../../../new_game/src/things/core/interact.cpp#L752))
   читает из `p_mthing->Draw.Tweened->CurrentFrame/NextFrame/Angle/...`,
   плюс глобальные `offset` и `matrix` (через `FMATRIX_calc`,
   `FMATRIX_MUL_BY_TRANSPOSE`). Static mutable globals — потенциальный
   скрытый side effect при двойном вызове в один render frame. Также
   `MSG_add` debug в interact.cpp:808. Нужен audit:
   - `offset`, `matrix` — static? thread-local? per-call temporaries?
   - `MSG_add` гейтится `allow_debug_keys` — OK, но lazy log могут
     удвоиться.
   - `FMATRIX_calc` mutate'ит global state? (Скорее всего только
     output buffer.)
2. **Lerp semantics.** Per-bone pos — линейный lerp_vec3 OK. Bone
   rotation тоньше. Если кость хранит matrix3x3 — нужен либо
   slerp(quaternion, quaternion, alpha) (точно, но тяжелее) либо
   lerp matrix-elements + reorthogonalize (быстро, но плывёт на больших
   углах). Текущий код хранит pose как pos + matrix через
   `FMATRIX_calc(Angle, Tilt, Roll)`. На больших дельтах углов
   element-wise lerp матриц может дать non-orthogonal result.
3. **Существующий cross-anim blend.** `render_interp_get_blend()` уже
   делает per-body-part смешивание между двумя позами. Возможен conflict
   с новым double-pose-eval подходом. Audit: либо новый подход заменяет
   blend полностью, либо живут вместе (одно для intra-anim wrap'ов,
   другое для anim transitions).

## Where to start (общий baseline)

Главная работа в **обоих** вариантах — вытащить pose composition в
reusable function. Сейчас она размазана:

- **`calc_sub_objects_position(p_mthing, tween, object, *x, *y, *z)`** в
  [`interact.cpp:752`](../../../new_game/src/things/core/interact.cpp#L752).
  Compose'ит позицию **одного** sub-object (LEFT_HAND, RIGHT_FOOT, etc.)
  относительно root, читая `p_mthing->Draw.Tweened->CurrentFrame/NextFrame`,
  `Angle/Tilt/Roll`. Output — offset relative to thing.
- **`calc_sub_objects_position_global(cur_frame, next_frame, tween, object, *x, *y, *z)`** в
  [`interact.cpp:869`](../../../new_game/src/things/core/interact.cpp#L869).
  То же, но принимает keyframe pointers напрямую (не через Thing).
  **Очень полезно для варианта B** — можно подставить prev и curr
  pointers из snapshot, не трогая live `Draw.Tweened`.
- **`FIGURE_draw_prim_tween`** в
  [`figure.cpp:1786`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L1786).
  Main draw routine для DT_TWEEN family. Читает live `Draw.Tweened->*`
  (которые уже перезаписаны нашим RenderInterpFrame ctor'ом интерполированными
  значениями), вызывает `calc_sub_objects_position` и проектирует bones
  на экран.
- **`FIGURE_draw`** в
  [`figure.cpp:2710`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L2710),
  **`FIGURE_draw_prim_tween_reflection`** на
  [`figure.cpp:2991`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L2991),
  **`FIGURE_draw_reflection`** на
  [`figure.cpp:3289`](../../../new_game/src/engine/graphics/geometry/figure.cpp#L3289)
  — другие entry points draw path. Все используют тот же compose
  механизм.

Бекстейдж — `MultiMatrix` (mmesh.cpp) — это аппарат для рисования
нескольких bone-attached sub-meshes с собственными матрицами. Тоже
читает `Draw.Tweened->*` через цепочку. Pose composition spread'ит
через несколько уровней.

### Рекомендуемая последовательность (для обоих вариантов)

1. **Прочитать** `interact.cpp` (calc_sub_objects_position* и siblings),
   `figure.cpp` (FIGURE_draw* family), `mmesh.cpp` (MultiMatrix bone
   draw). Цель: понимать, какие глобальные переменные используются как
   temporaries (`offset`, `matrix`), какие функции purity-friendly,
   какие нет.
2. **Записать список sub-objects** для DT_TWEEN family. Из
   `things/characters/`: вероятно `SUB_OBJECT_HEAD`, `SUB_OBJECT_LEFT_HAND`,
   `SUB_OBJECT_RIGHT_HAND`, `SUB_OBJECT_LEFT_FOOT`,
   `SUB_OBJECT_RIGHT_FOOT`, может ещё. Найди константы (`SUB_OBJECT_*`)
   в headers и составь полный список.
3. **Изолировать pose compose в reusable function.** Цель — `pose_t
   compose_pose(GameCoord world_pos, SWORD angle, SWORD tilt, SWORD roll,
   SLONG anim_tween, GameKeyFrame* curr_frame, GameKeyFrame* next_frame,
   ...)` — pure function, no global state mutation. Для variant B это
   будет вызываться 2× per render frame; для A — 1× per physics tick.
4. **Audit purity.** Прогнать composer 2× с одинаковыми входами и
   убедиться что output одинаковый, никакого global state не изменено.

После baseline ветвление:

### Если делать вариант B (рекомендуемый)

5. В `RenderInterpFrame::ctor`: **не подменять** `Draw.Tweened->*`
   (либо подменять, но figure.cpp ignor'ит). Возможно проще оставить
   текущую подмену, но в `figure.cpp` draw path читать prev/curr из
   snapshot напрямую через новое API.
6. Новое API: `bool render_interp_get_thing_pose_endpoints(Thing*,
   PoseSnapshot* out_prev, PoseSnapshot* out_curr, float* out_alpha)` —
   возвращает то что нужно для compose 2× + alpha.
7. В `FIGURE_draw_prim_tween` и siblings: query API → compose 2× →
   per-bone lerp → draw.
8. **Cross-anim blend** (`render_interp_get_blend`) — переосмыслить.
   Возможно полностью заменяется новым подходом (если двойной compose
   корректно обрабатывает anim transition тоже). Либо оставить как
   short-window override на anim swap.

### Если делать вариант A

5. Расширить `ThingSnap` в
   [`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp):
   добавить `pose_prev[N_BONES]`, `pose_curr[N_BONES]`. Размер N_BONES =
   max возможный bone count для DT_TWEEN family.
6. В `render_interp_capture(Thing*)`: после shift snapshot prev←curr,
   compose pose из live `Thing.WorldPos`/`dt->*` и сохранить в
   `pose_curr`. Compose использует ту же reusable function что в B.
7. В `RenderInterpFrame::ctor`: вместо substitution `Draw.Tweened->*` —
   передать lerped pose в `figure.cpp` через override. Либо остальной
   путь подменяет live state на интерполированные per-bone значения.
8. `figure.cpp` draw path — читать lerped pose-from-snapshot.

## Что нужно сохранить (regressions checklist)

- Обычная walk/run motion плавная.
- Прыжки (jump, jump-fall, jump-land) плавные.
- Vehicle motion плавный (через отдельный `INTERP_VEHICLE_ANGLE`).
- Cross-anim blend на normal anim transitions (idle→walk и подобные)
  визуально лучше чем без блёнда (как сейчас).
- DIRT pool / FC_cam / EWAY_cam — отдельные пути, не трогать.

## Что нужно увидеть (success criteria)

- **Лестница** — ни одного visible "прыжка дарси вниз" при подъёме/
  спуске. На замедленной физике (debug-клавиша 1, ~5 Hz) тело идёт
  плавно вверх без артефактов.
- **Arrest sequence** — нет visible spin при flip'е тела перед cuffing.
- Walk/run/turn по карте — без регрессий.
- Combat действия (jump-kick, fall-on-release, punch combos) — без
  регрессий относительно текущего состояния.

## Diagnostic infrastructure

В [`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp)
есть `RENDER_INTERP_LOG` (default 0) — пишет в `stderr.log`:
- FIRST_CAPTURE / FREE / CLASS_CHG / VEH_REBIND / BIG_DELTA — события
  жизни snapshot'ов.

При работе над этой задачей включить =1 и собирать post-mortem на
проблемных сценах для проверки гипотез.

[`debug_interpolation_config.h`](../../../new_game/src/debug_interpolation_config.h)
— compile-time toggle для каждого пути интерполяции отдельно. После
implementation добавить новый флаг (или переиспользовать
`INTERP_THING_ANIM_MORPH` / `INTERP_THING_CROSS_ANIM_BLEND`) для нового
world-space подхода — чтобы юзер мог откатить если что-то пошло не так.

## Тестовые сценарии (обязательно прогнать всё)

1. **Лестница на любой миссии** — карабкаться вверх и вниз. На обычной
   физике (20 Hz) и на замедленной (5 Hz через клавишу 1).
2. **Arrest** — knock'аем гражданского, подходим, арестовываем
   (рука двигает наручники к перпу). На обоих rate'ах physics.
3. **Прыжок** — на месте и на бегу. Падение с высоты. Все 3 фазы
   (jump-up, fall, land).
4. **Combat combo** — punch, jump-kick. Не должно стать **хуже** чем
   сейчас (известно что combo и так дёрганые — см. known_issues #2).
5. **Walk/run/sprint** — стандартное передвижение, повороты.
6. **Cross-anim transitions** — idle→walk→run→walk→idle.
7. **Катсцены** — RTA intro, любая cinematic. Особенно где много
   персонажей в кадре.
8. **Vehicles** — езда, повороты. Должна остаться без регрессий
   (vehicle angle path отдельный).
9. **DIRT** — побегать рядом с листьями (Psycho Park park area, RTA
   downtown). Должно остаться без регрессий.

## Recommended choice

Иди **вариант B** (double pose evaluation). Меньше работы (~120% от
baseline vs ~150% у A), меньше surface для багов, не нужны новые
структуры в memory, легче откатить если упрёшься во что-то.

Главный скрытый риск B (composer purity) проверяется единожды при
extract'е reusable function — если composer purity-friendly после
extract'а, остальное идёт легко.

## Если упрёшься во что-то

- Composer не purity-friendly после extract'а (мутирует global state) →
  попытаться сделать пер-вызов snapshot/restore globals (save/copy/
  restore). Если совсем упёрто — переходи на вариант A.
- Per-bone matrix lerp даёт визуально странные angles (не reorthogonal)
  → попробовать quaternion slerp (extract Tween-углы → quat per bone →
  slerp → matrix). Тяжелее но точнее.
- На action-scene anims (combat) видны регрессии — проверь сначала с
  выключённым `INTERP_THING_CROSS_ANIM_BLEND` через
  `debug_interpolation_config.h`. Возможно blend и новый pose lerp
  конфликтуют → blend disable / window обнуление.

## Files to read first (in order)

1. [`architecture.md`](architecture.md) — общая архитектура render-interp.
2. [`coverage.md`](coverage.md) — что покрывается, что нет.
3. [`known_issues.md`](known_issues.md) — известные баги, особенно
   #2 (action anims), 2b (тень pose), 2c (лестница).
4. [`new_game/src/engine/graphics/render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp)
   — текущая реализация capture/apply.
5. [`new_game/src/things/core/interact.cpp`](../../../new_game/src/things/core/interact.cpp)
   — `calc_sub_objects_position` family.
6. [`new_game/src/engine/graphics/geometry/figure.cpp`](../../../new_game/src/engine/graphics/geometry/figure.cpp)
   — `FIGURE_draw_*` family. Большой файл — ищи через grep, не читай
   целиком.
7. [`new_game/src/debug_interpolation_config.h`](../../../new_game/src/debug_interpolation_config.h)
   — флаги для A/B testing на проблемных сценах.

## Не трогать

- DIRT pool interpolation (отдельный путь, работает).
- FC_cam / EWAY_cam interpolation (отдельный путь, работает).
- Vehicle angles (отдельный путь, работает).
- pcom.cpp / eway.cpp `render_interp_mark_teleport` calls (предотвращают
  long-distance teleport jitter — отдельная задача, закрыта).
