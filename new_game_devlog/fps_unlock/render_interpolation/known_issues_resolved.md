# Known issues — resolved

Закрытые баги интерполяции рендеринга. Активные баги → [`known_issues.md`](known_issues.md).

Контекст → [`README.md`](README.md) | [`architecture.md`](architecture.md) | [`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md)

---

## ✅ 1. Мельтешащие педестрианы

**Был:** изредка по сцене на 1 кадр промелькивает педестриан в середине сцены и сразу пропадает. Бывает редко, но заметно. Связано с появлением (alloc) NPC.

**Гипотеза:** между alloc'ом нового thing'а в slot и заполнением его `WorldPos` caller'ом проходил physics-тик с capture'ом. Capture фиксировал stale данные от прошлого жильца этого slot'а (память не обнуляется в `alloc_thing`). Потом WorldPos устанавливался caller'ом, и следующий capture видел резкую смену позиции — на render-кадре это летело как «промельк».

**Закрыто** (подтверждено пользователем): вместе с фиксом машин-телепортов через всю карту (см. ниже «Машины и педестрианы летели через всю карту (PCOM wander-recycle teleports)»). Мельтешение было тем же классом бага — EWAY-подобная система допускала большие дельты `pos_prev → pos_curr` за один тик. Фиксы `render_interp_mark_teleport` в pcom.cpp + `render_interp_invalidate` в `free_thing` закрыли семейство кейсов целиком.

---

## ✅ 2. Резкие смены анимации в action-сценариях (jump-kick, fall-on-release, arrest spin, etc.) — 2026-05-03

**Был:** семейство симптомов на anim transitions / state-handler discrete writes:
- Удар ногой в прыжке (jump → air-kick → recovery → land): резкий
- Переход на падение при резком отпускании вперёд в прыжке: резкий visible переход
- Комбо ударов (punch combo) на 5 Hz: дерганые куски без сглаживания
- **Arrest cuffing spin:** при cuffing'е арестованного NPC state-handler одновременно менял `CurrentAnim` (на arrest-flip-anim, mesh не меняется) и писал `Draw.Tweened->Angle` со сдвигом ~90° (тело flip'ается лицом вверх). Старая интерполяция arc-lerp'ила Tween Angle через 90° за 50мс → visible spin на каждом cuffing'е.

**Корни** (старая substitution architecture):
1. Pose-change magnitude между соседними анимациями слишком большая для 1-tick blend window.
2. Backward AnimTween через combat code (`AnimTween = -twist`) — naive lerp визуально проигрывал в обратку; snap-on-backward создавал замороженные участки.
3. Adaptive blend duration не имела хорошей середины (1 tick → дёргается, 2 tick → U-shape на jump landing).
4. State-handler simultaneous writes (CurrentAnim + Tweened.Angle для arrest, или WorldPos.Y + bone_offset reset для ladder) — компоненты лерпились независимо, итоговая видимая поза не cancel-out'илась.

**Закрыто Phase 5 cleanup'ом** ([`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md)).
Pose snapshot решает архитектурно: composer выполняет full pose composition в physics tick (включая cancel-out + post-tick keyframe state), render лерпит per-bone world transforms между prev/curr. Anim transitions работают автоматически — composer выдаёт корректную post-tick pose независимо от того сколько keyframes между prev и curr; render видит smooth target вместо discrete snaps. Cancel-out cases (arrest spin, ladder cycles) решаются автоматически: composer выдаёт visible body transform после всех state-handler writes, render лerp'ит между двумя valid composed poses. Все hacks (cross-anim blend, virtual extended coordinate, snap-on-backward) удалены.

**Подтверждено пользователем 2026-05-03:** прыжки (включая phase transitions), ladder, walk/run/sprint, combat, arrest cuffing — без регрессий и без видимых jerk'ов / spin'ов.

---

## ✅ 2b. Тень Дарси не синхронизирована с телом на сменах анимации — 2026-05-03

**Был:** в обычной езде/беге тень визуально плавная — позиция и углы лерпятся синхронно с телом. Проблема — на границах смены анимации (jump → land, idle → run, любая anim transition). На моменте перехода тень рисует уже новую pose, в то время как тело ещё в blend'е между старой и новой. На приземлении прыжка особенно заметно — тень "приземлилась" в финальной позе, тело ещё в air-pose с морфом.

**Закрыто Phase 4** ([`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md)). Решение оказалось проще предполагаемого: shadow path рендерится через `SMAP_add_tweened_points` ([smap.cpp:263](../../../new_game/src/engine/graphics/lighting/smap.cpp#L263)), который тоже композирует per-bone pose из raw post-tick keyframe data (build_tween_matrix + linear lerp). Подключили его к тому же `render_interp_get_cached_pose()` API что и main body draw — override (x, y, z, mat_final) перед vertex transform. Тень теперь читает идентичные snapshot pose values что main body, синхронизация автоматическая во всех transitions без specialized blend logic.

---

## ✅ 2c. Дёрганые анимации Дарси на лестнице — 2026-05-03

**Был:** при включённой интерполяции на лестнице — дёрги верхней части тела вверх (1-кадровый артефакт) и почти постоянное дёрганье вниз. Корень: state-handler лестницы делает discrete WorldPos snap-ы (`WorldPos.Y += 0.3` в конце цикла анимации, при том что bone offset reset to 0) — старая интерполяция linearly lerp'ила через эти snap'ы → visible mid-alpha jerk.

**Закрыто Phase 5** ([`world_pose_snapshot_plan.md`](world_pose_snapshot_plan.md)). Pose snapshot захватывает **visible pelvis world position** (= `WorldPos + R · bone_local`), который через cancel-out остаётся плавным даже когда raw WorldPos снапит. Render видит smooth target вместо discrete snap'ов в pos.

**Подтверждено пользователем 2026-05-03:** лестница вверх/вниз — тело движется плавно, никаких jerk'ов.

---

## ✅ Кажущаяся дёрганость анимаций персонажей в катсценах

**Был:** в in-game катсценах казалось что анимации персонажей дёрганые. На разных моментах разных катсцен по-разному.

**Закрыто (наблюдение):** после фикса дёрганости EWAY камеры (см. ниже) кажущаяся дёрганость анимаций тоже исчезла. Скорее всего восприятие было одно — резкие cut'ы / дёрги камеры визуально портили ощущение от анимаций. Сами анимационные пути (AnimTween, FC_cam tracking) на персонажей-актёров в катсцене работали корректно изначально. Если баг всё-таки проявится в специфических сценах — открыть заново с конкретным репро.

---

## ✅ Дёрганость камеры в in-game катсценах (EWAY)

**Симптом:** в любой in-game катсцене (EWAY scripts — начало миссии, диалоговые сцены) камера двигается ступеньками на physics rate (20 Hz), несмотря на `IP: on` в overlay. Toggle 3 (g_render_interp_enabled) на симптом не влиял. В обычном геймплее player-camera при этом интерполировалась корректно.

**Root cause:** in-game катсцены используют **отдельную** камеру со state в глобалах `EWAY_cam_x/y/z/yaw/pitch/lens` (см. eway_globals.h и `EWAY_process_camera` в `eway.cpp`). В render path (`AENG_draw`) функция `EWAY_grab_camera` копирует эти raw post-tick глобалы в `fc->x/y/z/...` (т.е. в `FC_cam[0]`) **поверх** нашего FC_cam apply. Render видит raw EWAY state, наш lerp затирается. Toggle 3 не имел эффекта потому что apply вообще не действовал — между ctor и AENG_draw в катсцене всё равно перезаписывалось.

Подтверждено через diagnostic logging: в катсцене capture работал нормально (alpha варьировался, valid_cams=1, FC_cam apply писал lerp правильно), но render использовал EWAY-перезапись.

**Фикс:**
1. Новый snapshot `EwayCamSnap` (один экземпляр) в `render_interp.cpp` хранит prev/curr для `EWAY_cam_x/y/z/yaw/pitch/lens` (roll константа `1024 << 8`).
2. `render_interp_capture_eway_camera()` снимает EWAY-глобалы в physics tick (после `EWAY_process()`, рядом с `render_interp_capture_camera(&FC_cam[0])`). Если катсцена не активна — snapshot инвалидируется (re-seed prev=curr на следующей активации).
3. `render_interp_apply_eway_camera(SLONG* x, SLONG* y, SLONG* z, SLONG* yaw, SLONG* pitch, SLONG* roll, SLONG* lens)` — переписывает указатели на интерполированные значения, если interp enabled и snapshot valid.
4. В `aeng.cpp` (одиночная-камера branch + splitscreen branch dead code) и в `bloom.cpp` (BLOOM_flare_draw, BLOOM line-of-sight) — после `EWAY_grab_camera` вызывается apply, raw post-tick state заменяется на lerp.
5. **Не трогаем** другие места `EWAY_grab_camera`: `fc.cpp:1365` (camera physics), `input_actions.cpp:1389` (input — должен видеть real state), `game.cpp:791` (audio listener — interp lag не подходит для звука).

После фикса toggle 3 в катсцене работает, камера плавная.

**Cut detection (резкие переходы между сценами):** internal катсцены содержат cut'ы (смена waypoint'а / scene → scene → переключение говорящего в conversation). Без обработки render lerp плавно «пролетает» камеру через cut за 50ms — выглядит как медленный sweep между сценами вместо мгновенного переключения. EWAY-script использует разные пути для cut (signal `EWAY_cam_jumped = 10` ставится не во всех случаях — например, не ставится в `EWAY_create_camera` или `EWAY_cam_converse`-target-change), так что enumeration manual mark-точек хрупкое.

Решение — **delta-based auto-detect** в `render_interp_capture_eway_camera`: после обновления prev/curr считаем дельту между ними. Smooth EWAY motion (waypoint chase via velocity, midpoint двух speakers) физически не превышает несколько world units за physics tick, тогда как cut даёт 50000+ sub-units (≈5m+) или >30° по yaw. Если delta превышает threshold — `skip_once = true`, и в этом же capture prev копируется в curr → render frames между этим тиком и следующим лерпят `lerp(curr, curr, alpha) = curr` (камера зафиксирована на post-cut позиции). Cut виден на экране как hard transition между двумя render frames, без интерполяционного «полёта». Threshold'ы (5m / 30°) с большим запасом над максимальной скоростью smooth-EWAY motion на 5Hz physics; false-positive не наблюдается на практике.

---

## ✅ Cutscene crash — non-canonical pointer в RenderInterpFrame ctor

**Симптом:** ASan access-violation на адресе `0xffffffffffffffff` в `RenderInterpFrame::RenderInterpFrame`, ближе к концу начальной катсцены миссии. Регистры идентичны от запуска к запуску — детерминированный.

**Анализ:** дизассемблирование показало, crash в `is_loop_wrap`:
```cpp
bool is_loop_wrap = (frame_diff != 0 && frame_diff != 1
                     && s.current_frame_prev
                     && (s.current_frame_prev->Flags & ANIM_FLAG_LAST_FRAME));
```
`s.current_frame_prev` = `0xff00007800000000` — non-canonical pointer.

**Root cause** (найдено через диагностический лог в `render_interp_capture`):
- `DT_CHOPPER` (вертолёт) и `DT_PYRO` ошибочно были включены в `draw_type_uses_tween()`.
- Реально chopper'ы используют `Draw.Mesh` (DrawMesh) — `alloc_chopper` ([chopper.cpp:85](../../../new_game/src/things/vehicles/chopper.cpp#L85)) ставит `p_thing->Draw.Mesh = dm`. Pyro не использует поле `Draw` вообще (state в `Genus.Pyro`).
- Через union `Draw.Tweened` я читал DrawMesh-память как DrawTween → `dt->CurrentFrame`/`NextFrame` мапились на DrawMesh-поля (UWORD Angle/Roll/Tilt/ObjectId/Cache/Hm) и out-of-bounds в DRAW_MESHES pool.
- Conclusive evidence: лог показал `slot=143 class=12 (CLASS_CHOPPER) drawtype=11 (DT_CHOPPER)`, и `NextFrame` инкрементировался ровно на `0x20` каждый physics-тик (`0x20 → 0x40 → 0x60 → 0x80`) — это какое-то DrawMesh поле / соседний slot pool, не keyframe pointer.
- Save/load convert_thing_to_pointer ([memory.cpp:371-373](../../../new_game/src/missions/memory.cpp#L371)) подтверждает: `DT_CHOPPER` идёт в `convert_drawtype_to_pointer(p_thing, DT_MESH)` — официально DrawMesh.

**Фикс:** `DT_PYRO` и `DT_CHOPPER` убраны из `draw_type_uses_tween()`. Эти DrawType теперь не интерполируются в render-interp (как уже было для `DT_VEHICLE` до отдельной ветки). Если потребуется плавность поворотов вертолётов — расширить DrawMesh-ветку аналогично Vehicle.

---

## ✅ Поворот машин рывками

**Был:** машины на повороте вращались ступеньками 20 Hz, хотя позиция была плавная. Угол машины хранится в `Genus.Vehicle->Angle/Tilt/Roll` (SLONG), отдельно от `Draw.Tweened`. Прежняя версия системы покрывала только Tween-углы.

**Закрыто:** в `ThingSnap` добавлена отдельная ветка `has_vehicle_angles` + `veh_angle/tilt/roll_prev/curr` + `veh_velr_curr` + `veh_ptr_curr` (cached `Genus.Vehicle`). `render_interp_capture` для CLASS_VEHICLE снимает `Genus.Vehicle->Angle/Tilt/Roll` и `VelR`. `RenderInterpFrame::ctor` подменяет их на интерполированные через `lerp_angle_2048_directed` (Angle, использует знак VelR как direction hint для long-path при быстром вращении) и `lerp_angle_2048_slong` (Tilt/Roll). `dtor` восстанавливает live values. Закрывает также подмножество #4 (short-path при быстром вращении) для CLASS_VEHICLE.

**Pointer-identity guard:** ctor/dtor применяют lerp **только** если `p->Genus.Vehicle == s.veh_ptr_curr` (cached pointer). Был замечен crash в начальной катсцене миссии: между capture и render-frame `Genus.Vehicle` мог быть rebind'нут (slot переиспользован под нового актёра, либо partial bind где Class уже CLASS_VEHICLE а pointer ещё не финализирован). Без identity-check ctor разыменовывал stale/garbage pointer → access violation на `0xffffffffffffffff`. С identity-check mismatched frames просто рендерятся без интерполяции (no crash, no stutter — следующий tick восстановит окно).

---

## ✅ Camera-body desync

**Был:** интерполированное тело Дарси «снапилось» в screen-space на каждой границе тика, потому что камера не интерполировалась. Возникало впечатление что body двигается плавно сам по себе, но «не там где ожидаешь относительно мира».

**Закрыто:** добавлена интерполяция `FC_cam[0]` через тот же механизм. Тело и камера на одной render-time-шкале.

---

## ✅ Глюки анимации при прыжке Дарси (старый, до Phase 5)

**Был:** при переключении state (idle → jump → land) поза «съёживалась» в момент перехода — старый substitution lerp интерполировал между двумя позами с разными MeshID/CurrentAnim.

**Закрыто:** anim-transition детектор в `render_interp_capture` ставил `skip_once=true` при изменении `dt->MeshID` или `dt->CurrentAnim`. На тике перехода рендерилась poseN без интерполяции из poseN-1.

**Note (Phase 5 update):** оба механизма (substitution + skip_once для anim-transitions) удалены при cleanup'е. Pose snapshot path архитектурно решает проблему без специального skip — composer выдаёт correct post-tick pose, render лерпит между двумя valid composed poses плавно. См. также bug 2 (resolved).

---

## ✅ Slot reuse теплепортационные перелёты

**Был:** при alloc нового thing'а в slot где раньше был умерший — snapshot оставался valid со старыми pos_prev/pos_curr. Lerp летел между мёртвой и новой позицией.

**Закрыто:** hook `render_interp_invalidate(t_thing)` в `free_thing`. Snapshot обнуляется при освобождении slot'а. Первый capture нового жильца идёт по ветке `!valid` → `prev=curr=current`.

Однако этот фикс **не закрыл** «мельтешащих педестрианов» — это другой кейс (см. баг 1 в [`known_issues.md`](known_issues.md)).

---

## ✅ Машины и педестрианы летели через всю карту (PCOM wander-recycle teleports)

**Симптом:** в in-game катсценах (особенно RTA intro) и в обычном геймплее с активным трафиком — машины с пассажирами и пешеходы периодически дёргаются мгновенным полётом через всю карту за один физ-тик. Render-interp линейно лерпит pos между двумя несвязанными точками в течение тика → визуально модель проносится через всё игровое пространство и оседает на новом месте.

**Root cause (`pcom.cpp`):** Urban Chaos использует **wander recycle** — когда NPC (driver/passenger или pedestrian) уходит за `DRAW_DIST` и не виден игроку (`!FLAGS_IN_VIEW`) дольше определённого времени, он переселяется на новый wander-point рядом с Darci через семейство `WAND_find_*` функций. Это позволяет городу всегда казаться населённым без бесконечного спавна. NPC + его машина (если есть) переcпавниваются мгновенно. Render-interp видит огромный `pos_prev → pos_curr` и lerp'ит между ними весь тик.

Подтверждение: bracket-trace через `process_things` показал что все `BIG_DELTA` события `|d| ≈ 1M-5M sub-units` происходят **внутри `PCOM_process_person`** (не в EWAY как казалось изначально). Парные `slot=N (PERSON) + slot=N+1 (VEHICLE)` объясняются тем что passenger и его car telep'ятся вместе.

**Фикс — `render_interp_mark_teleport(thing)` сразу после каждого WorldPos write в pcom.cpp:**

1. [`pcom.cpp:707`](../../../new_game/src/ai/pcom.cpp) — общий person teleport (с гейтом `dont_teleport` capping distance к 0x5000, но безгейтовый путь не cap'ит).
2. [`pcom.cpp:4211/4215/4217`](../../../new_game/src/ai/pcom.cpp) — `FLAG2_PERSON_FAKE_WANDER` traffic recycle через `WAND_find_good_start_point_for_car`. **Главный источник** наблюдавшегося jitter'а в RTA intro. Mark на vehicle и passenger.
3. [`pcom.cpp:4258`](../../../new_game/src/ai/pcom.cpp) — vehicle end-of-road recycle через `ROAD_find_me_somewhere_to_appear`. Mark на vehicle и driver.
4. [`pcom.cpp:4499`](../../../new_game/src/ai/pcom.cpp) — pedestrian respawn через `WAND_find_good_start_point` (KO/helpless/arrested cleanup).
5. [`pcom.cpp:6267`](../../../new_game/src/ai/pcom.cpp) — `PCOM_teleport_home` (явное "go home").

**Дополнительно — превентивный фикс в EWAY** ([`eway.cpp`](../../../new_game/src/missions/eway.cpp)):
- `EWAY_DO_CREATE_ENEMY` end-handler — telep актёра off-stage в `(0x8000, _, 0x8000)` после remove_thing_from_map.
- `EWAY_DO_MOVE_THING` — scene-transition move, обе ветки (on-map / off-map). В наблюдавшейся катсцене не triggered, но семантически тоже teleport.

**Также исправлен сам `render_interp_mark_teleport`** ([`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp)): раньше она ставила `s.skip_once = true`, но `skip_once` (старая семантика) collapse'ит только anim-keyframe поля и не collapse'ит pos/angles. Для настоящего teleport'а нужен полный wipe → `mark_teleport` делегирует `render_interp_invalidate` (полный сброс snapshot'а; на следующем capture идёт `!valid` ветка которая ставит `prev=curr=current`).

**Подтверждено пользователем 2026-05-02:** машины и пешеходы перестали летать через карту в RTA intro и в обычном геймплее.

**Не закрыто** этим фиксом (отдельные кейсы для будущих итераций):
- spawn-stale (см. баг 1, "Мельтешащие педестрианы" в [`known_issues.md`](known_issues.md)) — alloc Thing в slot → physics-tick capture со stale WorldPos от прошлого жильца → caller выставляет настоящую WorldPos позже → следующий tick видит большую дельту. Не PCOM-driven, baseline pre-release bug.
- Vehicle slot reuse без `free_thing` (`THING_kill` no-op для CLASS_VEHICLE) — теоретический, на практике не наблюдался.

**Diagnostic infrastructure:** `RENDER_INTERP_LOG` флаг (default 0) в `render_interp.cpp` пишет в stderr.log события CLASS_CHG / VEH_REBIND / FIRST_CAPTURE / BIG_DELTA для post-mortem анализа. Поднимать на 1 при поиске новых кейсов teleport'а.

---

## ✅ Листья / DIRT pool — рывки целых "блоков" при движении игрока

**Симптом:** при движении персонажа по карте (особенно бег/езда) на сцене заметно **дёргание целых групп листьев на земле** — будто блок листьев на 1 кадр телепортируется на другое место. На месте (стоя) проблемы нет — тянется только при движении. Также проявляется на снегу, банках, других DIRT-объектах в разной мере.

**Root cause:** `DIRT_process` использует **focus-based recycle** ([`dirt.cpp:340-475`](../../../new_game/src/world_objects/dirt.cpp)) — каждый physics tick проверяется ~`DIRT_MAX_DIRT/8` слотов. Если slot дальше `DIRT_focus_radius` от focus point (~camera position), он **перепозиционируется** на новое случайное место около focus point. При движении игрока focus сдвигается → много слотов сразу выходят за radius → recycle'ятся группой в одном тике → render-interp лерпит каждый из них через всю карту от старой позиции к новой за один тик. **Тип DIRT не меняется** (LEAF→LEAF) — мой capture-side `last_type` change check не срабатывает, идёт стандартный window-shift.

Второй recycle path — `DIRT_tree`-driven leaf spawn ([`dirt.cpp:282-326`](../../../new_game/src/world_objects/dirt.cpp)): когда дерево становится in-range, листья spawn'ятся вокруг него. Если кандидат slot имел `DIRT_FLAG_DELETE_OK` (LEAF marked для удаления) и реинкарнируется как LEAF на новой позиции — тоже type-stays-LEAF, тот же баг.

**Фикс — `render_interp_mark_dirt_teleport(slot_idx)` в обоих recycle paths:**
- [`dirt.cpp:472-486`](../../../new_game/src/world_objects/dirt.cpp) — после switch'а в focus-based recycle, гейтован `if (dd->type != DIRT_TYPE_UNUSED)` (UNUSED handled by capture-side type-change auto-detect).
- [`dirt.cpp:325`](../../../new_game/src/world_objects/dirt.cpp) — внутри tree-based DELETE_OK reuse path, после успешного set'а new x/y/z.

**API:** новый `render_interp_mark_dirt_teleport(int idx)` в [`render_interp.h`](../../../new_game/src/engine/graphics/render_interp.h) — wipes `g_dirt_snaps[idx]`, на следующем capture идёт `!valid` (first-capture) path, prev=curr=new. Аналог `render_interp_mark_teleport(Thing*)` для Things.

**Подтверждено пользователем 2026-05-02:** дёргания "блоков" листьев исчезли. Bounce-back collision pos write'ы (line 1080/1265/1395 в dirt.cpp) — small per-cell bounce, не teleport, mark не нужен. Held-can/head positioning (line 1340) — отдельный потенциальный body-vs-held-item desync, не наблюдался, не трогаем.
