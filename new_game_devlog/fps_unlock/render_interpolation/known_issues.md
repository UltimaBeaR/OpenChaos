# Known issues — текущие проблемы и что пробовали

> Архитектура → [`architecture.md`](architecture.md). Coverage → [`coverage.md`](coverage.md). Планы → [`plans.md`](plans.md).

## Активные баги

### 1. Мельтешащие педестрианы — открыт

**Симптом:** изредка по сцене на 1 кадр промелькивает педестриан в середине сцены и сразу пропадает. Бывает редко, но заметно. Скорее всего связано с появлением (alloc) NPC, реже — с пропаданием.

**Гипотеза:** между alloc'ом нового thing'а в slot и заполнением его `WorldPos` caller'ом может пройти physics-тик с capture'ом. Capture фиксирует **stale данные** от прошлого жильца этого slot'а (память не обнуляется в `alloc_thing`). Потом WorldPos устанавливается caller'ом, и следующий capture видит резкую смену позиции — на render-кадре между двумя capture'ами это летит как «промельк».

**Что мы про это знаем:**
- `alloc_primary_thing` ([thing.cpp:92-112](../../../new_game/src/things/core/thing.cpp#L92)) **сразу** добавляет slot в `thing_class_head[]` linked list — то есть thing виден моему capture'у до того как `alloc_thing` выставит даже Class.
- В `alloc_thing` ([thing.cpp:601-619](../../../new_game/src/things/core/thing.cpp#L601)) ставятся только Class/State/Flags/StateFn. WorldPos не трогается.
- WorldPos заполняет caller (например `alloc_person`).
- Между alloc и установкой WorldPos может пройти capture (если alloc происходит вне physics-tick logic, например в state handler одного thing'а — а capture идёт после всех state handler'ов).

**Что пробовали:**
- Hook `render_interp_invalidate(t_thing)` в `free_thing` — обнуляет snapshot при освобождении slot'а. **Помогло частично** — закрыл случай «slot переиспользован → snapshot.valid=true со stale данными». Но не закрыл «alloc → capture с stale WorldPos → caller потом ставит правильную WorldPos → следующий capture видит большую дельту».

**Что НЕ сработало:**
- **Position teleport detector** с порогом 1000-5000 units. Гипотеза была — детектировать большие скачки позиции и подавлять lerp на этом тике. На пороге 1000 плавность пропала **глобально** (что-то нормальное попадало под порог и подавляло lerp везде). На пороге 5000 — не тестировалось. Метод признан неточным (угадывание порога) и удалён из кода.

**Что планируется** (см. [`plans.md`](plans.md)):
- Hook на момент когда `Class` устанавливается из `CLASS_NONE` — перехватить spawn до первого capture
- Альтернатива: hook на `add_thing_to_map` — это вызывается caller'ом **после** заполнения WorldPos
- Альтернатива: в capture проверять `if (!s.valid && p->Class != s.last_seen_class)` → инвалидация перед capture

**Дебаг:** включить `RENDER_INTERP_LOG=1` в [render_interp.cpp](../../../new_game/src/engine/graphics/render_interp.cpp) — в `stderr.log` появятся строки `[render_interp] FIRST_CAPTURE` и `[render_interp] FREE`, можно увидеть последовательность жизни slot'ов. Сейчас лог **включён** для активной отладки.

### 2. Резкие смены анимации в action-сценариях (jump-kick, fall-on-release, etc.) — открыт

**Семейство симптомов:**

- **Удар ногой в прыжке** (jump → air-kick → recovery → land): резкий на 5Hz и на 20Hz одинаково. Без плавности от blend'а, конечности «дёргаются» сквозь дикие позы.
- **Переход на падение при резком отпускании вперёд в прыжке** (бежишь+прыгаешь+отпустил W → анимация падения с раскинутыми руками): резкий visible переход.
- **Комбо ударов (punch combo)** на 5 Hz: дерганые куски без сглаживания, при ZIP визуально кадров меньше чем без интерполяции. На 20Hz примерно ОК (после snap-on-backward fix).

**Корни проблем (не одна причина):**

1. **Pose-change magnitude** — между двумя соседними анимациями (например anim 280 → 281 в jump-kick) body root offset скачет 57600 → 28928 + матрицы конечностей дико отличаются. 50мс (1 физ-тик при 20Hz) или 200мс (на 5Hz) blend window визуально недостаточно, чтобы глаз увидел плавный переход — изменение слишком большое.

2. **Adaptive blend duration** жёстко 1 физ-тик чтобы не было U-shape (живая новая анимация прокручивается мид-блендом). Расширение до 2 тиков (MIN=100мс при 20Hz) пробовали — вернуло U-shape на jump landing. Нет хорошей середины.

3. **Backward AT через geymplay-код:** combat анимации напрямую модифицируют `AnimTween` (включая `AnimTween = -twist` в person.cpp:10898) — между physics ticks AT может прыгать назад на дикую дельту (например 156→35 за тик при 20Hz). Текущий фикс — snap-on-any-backward (если `curr_AT < prev_AT`, не лерпим, используем curr напрямую). Это устраняет «smooth обратное проигрывание» (которое юзер ненавидел), но создаёт `замороженные` визуальные участки на длительность тика. На 20Hz freeze ≈ 50мс (приемлемо), на 5Hz ≈ 200мс (видно как «дерганые кадры»).

4. **Direction-reversal-only snap** (попытка быть умнее) пропускает smooth backward после флипа — вернула старые глюки punch'а. Все-backward-snap нужен для combat.

**Что НЕ сработало:**
- Direction-reversal heuristic (snap только на смене знака, плавный backward внутри).
- MIN blend duration 100ms (улучшил jump-kick маргинально, добавил U-shape на jump landing).
- Frozen-new pose в blend (улучшил U-shape на jump-up но регресс на idle→run).

**Приоритет:** средний — без IP всё работает (просто на physics rate визуально), с IP осталось семейство action-багов.

**Возможные направления для будущего (если решат довести):**
- Per-body-part offset lerp в child limbs (сейчас только matrix slerp лерпится; offset CF.O константа в кадре). Может смягчить резкие изменения положения конечностей внутри одной анимации.
- Селективное отключение blend для конкретных anim ID диапазонов (combat 146-281, fall, etc.).
- Two-phase blend (сначала cross-fade на frozen new, в конце catchup до live).
- Доработка underлying anim/physics код чтобы не было `AnimTween = -twist` или подобных AT-as-parameter трюков.

### 2a. Регресс инпута комбо-ударов — открыт

**Симптом:** при комбо ударов нажатия как будто плохо регистрируются — последовательные нажатия могут не срабатывать. До недавних изменений (в render_interp / fps_unlock в целом) такого не было.

**Гипотеза:** скорее всего НЕ render-interp баг — это input/state handling. Возможно где-то логика проверяет animation state вместо input event'а, и из-за изменённого темпа физики/анимации временные окна для комбо нарушены. Или обработка input в state handler'ах где-то завязана на какой-то рендер-сторонний параметр.

**Что нужно:** воспроизвести с известным набором нажатий, проверить регистрируются ли input event'ы (input_actions), сравнить с initial_working_d3d_build/last_working_d3d_build версиями для понимания регресса.

**Приоритет:** средний — мешает играть в комбатах.

### 2b. Тень Дарси не синхронизирована с телом на сменах анимации — открыт

**Симптом:** в обычной езде/беге тень визуально **плавная** — позиция и углы лерпятся синхронно с телом. **Проблема — на границах смены анимации** (jump → land, idle → run, любая anim transition). На моменте перехода тень рисует уже новую pose, в то время как тело ещё в blend'е между старой и новой (per-body-part cross-fade через `render_interp_get_blend`). На приземлении прыжка особенно заметно — тень "приземлилась" в финальной позе, тело ещё в air-pose с морфом.

**Root cause гипотеза:** shadow rendering path **не повторяет полный комплект interp-трюков** что используется для основной модели в `FIGURE_draw`:

1. **WorldPos** — лерпится через `RenderInterpFrame` (substitution в `Thing.WorldPos`). У теней работает (pos-плавность есть).
2. **Tween-углы** (`Draw.Tweened->Angle/Tilt/Roll`) — substituted в ctor. Скорее всего работает.
3. **AnimTween + CurrentFrame + NextFrame** (vertex morph fraction + keyframe pointer pair) — substituted в ctor. Если shadow path не использует эти поля, либо использует другие копии — баг.
4. **Cross-anim blend per-body-part** через `render_interp_get_blend()` — это **активный query** который надо звать в render path. `FIGURE_draw` его зовёт, shadow path вероятно нет.

**Что нужно:** повторить в shadow rendering path полный набор:
- Использовать те же `dt->AnimTween/CurrentFrame/NextFrame` (мы их подменяем)
- Запросить `render_interp_get_blend(p_thing, &out)` и если active — composить shadow pose так же как тело (`lerp(old_pose, new_pose, blend_t)` per-body-part)

Кандидаты shadow rendering files (искать):
- `AENG_detail_shadows` (city) / `AENG_shadows_on` (warehouse) в [`aeng.cpp`](../../../new_game/src/engine/graphics/pipeline/aeng.cpp) — выбор 4 ближайших actors + detail shadow рисовка.
- `FIGURE_draw_shadow` или аналог — может рисовать через own shadow projection.
- Shadow path для blob-теней — отдельная история, blob уже не зависит от pose.

**Приоритет:** средний — заметно на каждом приземлении, прыжке, переходе в combat.

### 2c. Дёрганые анимации Дарси на лестнице (вверх редко, вниз почти постоянно) — открыт

**Симптом:** проявляется только при включённой интерполяции (`g_render_interp_enabled = true`, IP: on). При карабкании Дарси по лестнице:
- **Вверх:** иногда (не на каждый шаг) verхняя часть модели резко дёргается куда-то и возвращается. Выглядит как 1-кадровый артефакт.
- **Вниз:** почти **постоянное** "тяжёлое" дёргание туда-сюда — анимация конечностей рывками, не плавно. Существенно более выражено чем на подъёме.

При выключенной интерполяции (toggle 3) — анимация лестницы работает корректно (дискретно на physics rate, без artifact'ов). Значит баг в интерполяции, не в самом ladder state-handler'е.

**Гипотеза:** state-handler лестницы (вероятно `STATE_USE_SCENERY` substate ladder, либо отдельный `SUB_STATE_LADDER_*`) **дискретно перепозиционирует** Дарси на каждом step (snap-to-rung) или меняет **AnimTween** нелинейно (e.g. AnimTween reset to 0 на каждом step + immediate large advance к ~256). Intra-anim AnimTween jumps уже частично закрыты `snap-on-any-backward` (см. баг #2 / closed combat AnimTween fixes), но ladder-down может иметь специфические forward jumps которые тоже дают visual mush после lerp'а.

Возможные конкретные кандидаты:
- **AnimTween reset на каждый rung step:** ladder-down anim проигрывается incrementally, и каждый rung — full anim cycle (`FrameIndex` wraps to 0). Loop-wrap detection в render-interp есть, но если wrap случается чаще чем раз в N тиков, multi-keyframe-per-tick path не отрабатывает (известно из architecture, "Случай 4: FrameIndex продвинулся больше чем на 1 за тик — пропускаем").
- **Position snap к ladder rungs:** если каждый physics tick body snaps к next rung position, а render lерпит между snaps — видно "ездит" между rungs.
- **Cross-anim transition spam:** если ladder-down меняет MeshID/CurrentAnim каждые N тиков, anim-transition детектор каждый раз ставит `skip_once` + cross-anim blend window открывается и закрывается часто — может дать дёрганый blend.

**Что нужно:** воспроизвести с RENDER_INTERP_LOG=1, посмотреть что доминирует — `ANIM_TRANS` спам, `BIG_ANGLE_DELTA`, `BIG_DELTA` (pos), либо AnimTween jumps. На ladder-down эффект сильнее → корреляция с rung step direction. По данным решить: добавить snap-on-large-AnimTween-jump аналогично snap-on-backward, либо collapse position на anim transition (см. также bug 2b/2c обсуждение conditional collapse).

**Приоритет:** средний — лестницы используются нечасто, но когда используются — заметно.

## Известные ограничения (не баги — отсутствие покрытия)

### 3. Партиклы рывками

**Симптом:** искры, glitter, дым могут двигаться зернисто на 60 Hz рендере.

**Причина:** PARTICLE pool — отдельная структура от Things, движется через `TICK_RATIO` на physics-tick. Не интерполируется.

**Приоритет:** низкий. Из-за короткого lifetime партиклов и flicker'а — обычно не бросается в глаза. Отложено до подтверждения что заметно.

### 4. Короткий путь lerp угла при быстром вращении — направление может перепутаться

**Симптом (предполагаемый, требует проверки на практике):** при очень быстром вращении объекта lerp угла может крутить **в обратную сторону** или дёргаться. Заметно на сценах где объект раскручен на большой угловой скорости — например, конусы которые задели машиной и они отлетают, быстро крутясь вокруг своей оси.

**Причина:** `lerp_angle_2048` (и `lerp_angle_cam` для камеры) использует **short-path** интерполяцию — выбирает кратчайший путь по окружности (≤180° / ≤1024 единиц). Если за один physics-тик объект реально провернулся на больше чем 180°, кратчайший путь между snap.prev и snap.curr идёт **в противоположном направлении** от настоящего вращения.

Пример: за тик угол изменился с 0 до 1500 (физика провернула на 1500 единиц по часовой). Short-path считает: 1500 → 1500-2048 = -548 — то есть «548 единиц против часовой». Lerp на рендере крутит против часовой, хотя физически объект крутится по часовой. Между двумя последовательными такими тиками рендер показывает «дёргается туда-сюда».

**Когда проявляется:**
- Угловая скорость > 1024 ед/тик = >180° за 50ms = >3600°/сек = >10 оборотов в секунду
- Это **очень быстрое** вращение, бывает только при резких физ-импульсах (отлетающие объекты, перевёрнутые машины)

**Решение для CLASS_VEHICLE — закрыто через `Vehicle->VelR` direction hint** (`lerp_angle_2048_directed`). Знак VelR определяет на какой стороне круга идёт `prev → curr`, обходя short-path heuristic. Vehicle снимает Angle/Tilt/Roll своим snapshot веткой (см. [`architecture.md`](architecture.md), `coverage.md`).

**Для других классов проблема остаётся открытой** в той части где быстро крутящиеся объекты используют Tween-углы (`lerp_angle_2048` short-path). Кандидаты:
- `CLASS_PROJECTILE` — пули обычно по прямой, угол не критичен.
- `CLASS_PYRO`, `CLASS_BARREL` — могут быстро крутиться (катящиеся бочки, отлетающие при взрыве). Если станет видимо, нужен angular velocity hint в физике этих классов либо selective skip.

**Возможные решения для оставшихся классов:**
- **Хранить знак угловой скорости рядом со снапшотом.** Аналогично Vehicle, но требует чтобы класс физически хранил angular velocity где-то в Genus-структуре.
- **Просто отключить угловую интерполяцию для быстрых объектов** (`skip_once` на каждом тике пока скорость > threshold). Лучше дёрганье на 20Hz чем визуальный реверс направления.

**Приоритет:** низкий до подтверждения на практике. Быстрое вращение — короткий эффект (объект быстро остановится), большинство игроков может не заметить.

## Закрытые баги

### Кажущаяся дёрганость анимаций персонажей в катсценах

**Был:** в in-game катсценах казалось что анимации персонажей дёрганые. На разных моментах разных катсцен по-разному.

**Закрыто (наблюдение):** после фикса дёрганости EWAY камеры (см. ниже) кажущаяся дёрганость анимаций тоже исчезла. Скорее всего восприятие было одно — резкие cut'ы / дёрги камеры визуально портили ощущение от анимаций. Сами анимационные пути (AnimTween, FC_cam tracking) на персонажей-актёров в катсцене работали корректно изначально. Если баг всё-таки проявится в специфических сценах — открыть заново с конкретным репро.

### Дёрганость камеры в in-game катсценах (EWAY)

**Симптом:** в любой in-game катсцене (EWAY scripts — начало миссии, диалоговые сцены) камера двигается ступеньками на physics rate (20 Hz), несмотря на `IP: on` в overlay. Toggle 3 (g_render_interp_enabled) на симптом не влиял. В обычном геймплее player-camera при этом интерполировалась корректно.

**Root cause:** in-game катсцены используют **отдельную** камеру со state в глобалах `EWAY_cam_x/y/z/yaw/pitch/lens` (см. [eway_globals.h](../../../new_game_planning/...) и `EWAY_process_camera` в `eway.cpp`). В render path (`AENG_draw`) функция `EWAY_grab_camera` копирует эти raw post-tick глобалы в `fc->x/y/z/...` (т.е. в `FC_cam[0]`) **поверх** нашего FC_cam apply. Render видит raw EWAY state, наш lerp затирается. Toggle 3 не имел эффекта потому что apply вообще не действовал — между ctor и AENG_draw в катсцене всё равно перезаписывалось.

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

**Defensive guards остаются** на всякий случай (canonicity check `render_interp_addr_looks_valid`, `is_loop_wrap` сужен до `frame_diff >= 200`) — для других потенциальных edge case'ов которые не угадаешь заранее.

### Поворот машин рывками

**Был:** машины на повороте вращались ступеньками 20 Hz, хотя позиция была плавная. Угол машины хранится в `Genus.Vehicle->Angle/Tilt/Roll` (SLONG), отдельно от `Draw.Tweened`. Прежняя версия системы покрывала только Tween-углы.

**Закрыто:** в `ThingSnap` добавлена отдельная ветка `has_vehicle_angles` + `veh_angle/tilt/roll_prev/curr` + `veh_velr_curr` + `veh_ptr_curr` (cached `Genus.Vehicle`). `render_interp_capture` для CLASS_VEHICLE снимает `Genus.Vehicle->Angle/Tilt/Roll` и `VelR`. `RenderInterpFrame::ctor` подменяет их на интерполированные через `lerp_angle_2048_directed` (Angle, использует знак VelR как direction hint для long-path при быстром вращении) и `lerp_angle_2048_slong` (Tilt/Roll). `dtor` восстанавливает live values. Закрывает также подмножество #4 (short-path при быстром вращении) для CLASS_VEHICLE.

**Pointer-identity guard:** ctor/dtor применяют lerp **только** если `p->Genus.Vehicle == s.veh_ptr_curr` (cached pointer). Был замечен crash в начальной катсцене миссии: между capture и render-frame `Genus.Vehicle` мог быть rebind'нут (slot переиспользован под нового актёра, либо partial bind где Class уже CLASS_VEHICLE а pointer ещё не финализирован). Без identity-check ctor разыменовывал stale/garbage pointer → access violation на `0xffffffffffffffff`. С identity-check mismatched frames просто рендерятся без интерполяции (no crash, no stutter — следующий tick восстановит окно).

### Camera-body desync

**Был:** интерполированное тело Дарси «снапилось» в screen-space на каждой границе тика, потому что камера не интерполировалась. Возникало впечатление что body двигается плавно сам по себе, но «не там где ожидаешь относительно мира».

**Закрыто:** добавлена интерполяция `FC_cam[0]` через тот же механизм. Тело и камера на одной render-time-шкале.

### Глюки анимации при прыжке Дарси

**Был:** при переключении state (idle → jump → land) поза «съёживалась» в момент перехода — lerp интерполировал между двумя позами с разными MeshID/CurrentAnim.

**Закрыто:** anim-transition детектор в `render_interp_capture` — при изменении `dt->MeshID` или `dt->CurrentAnim` ставит `skip_once=true`. На тике перехода рендерится poseN без интерполяции из poseN-1.

### Slot reuse теплепортационные перелёты

**Был:** при alloc нового thing'а в slot где раньше был умерший — snapshot оставался valid со старыми pos_prev/pos_curr. Lerp летел между мёртвой и новой позицией.

**Закрыто:** hook `render_interp_invalidate(t_thing)` в `free_thing`. Snapshot обнуляется при освобождении slot'а. Первый capture нового жильца идёт по ветке `!valid` → `prev=curr=current`.

Однако этот фикс **не закрыл** «мельтешащих педестрианов» — это другой кейс (см. баг 1 выше).

### Машины и педестрианы летели через всю карту (PCOM wander-recycle teleports)

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

**Также исправлен сам `render_interp_mark_teleport`** ([`render_interp.cpp`](../../../new_game/src/engine/graphics/render_interp.cpp)): раньше она ставила `s.skip_once = true`, но `skip_once` collapse'ит **только** anim-keyframe поля (AnimTween, FrameIndex, CurrentFrame, NextFrame) и **не** collapse'ит pos/angles — это специально, потому что тот же флаг переиспользуется anim-transition детектором, где pose меняется но WorldPos должен продолжать лерпиться. Для настоящего teleport'а нужен полный wipe → теперь `mark_teleport` делегирует `render_interp_invalidate` (полный сброс snapshot'а; на следующем capture идёт `!valid` ветка которая ставит `prev=curr=current`).

**Подтверждено пользователем 2026-05-02:** машины и пешеходы перестали летать через карту в RTA intro и в обычном геймплее.

**Не закрыто** этим фиксом (отдельные кейсы для будущих итераций):
- spawn-stale (см. баг 1, "Мельтешащие педестрианы") — alloc Thing в slot → physics-tick capture со stale WorldPos от прошлого жильца → caller выставляет настоящую WorldPos позже → следующий tick видит большую дельту. Не PCOM-driven, baseline pre-release bug.
- Vehicle slot reuse без `free_thing` (`THING_kill` no-op для CLASS_VEHICLE) — теоретический, на практике не наблюдался.

**Diagnostic infrastructure:** `RENDER_INTERP_LOG` флаг (default 0) в `render_interp.cpp` пишет в stderr.log события CLASS_CHG / VEH_REBIND / FIRST_CAPTURE / BIG_DELTA для post-mortem анализа. Поднимать на 1 при поиске новых кейсов teleport'а — паттерн "BIG_DELTA в одном и том же slot повторяющийся" указывает на recycle-mechanism, "CLASS_CHG" указывает на slot reuse без proper invalidate.

### Листья / DIRT pool — рывки целых "блоков" при движении игрока

**Симптом:** при движении персонажа по карте (особенно бег/езда) на сцене заметно **дёргание целых групп листьев на земле** — будто блок листьев на 1 кадр телепортируется на другое место. На месте (стоя) проблемы нет — тянется только при движении. Также проявляется на снегу, банках, других DIRT-объектах в разной мере.

**Root cause:** `DIRT_process` использует **focus-based recycle** ([`dirt.cpp:340-475`](../../../new_game/src/world_objects/dirt.cpp)) — каждый physics tick проверяется ~`DIRT_MAX_DIRT/8` слотов. Если slot дальше `DIRT_focus_radius` от focus point (~camera position), он **перепозиционируется** на новое случайное место около focus point. При движении игрока focus сдвигается → много слотов сразу выходят за radius → recycle'ятся группой в одном тике → render-interp лерпит каждый из них через всю карту от старой позиции к новой за один тик. **Тип DIRT не меняется** (LEAF→LEAF) — мой capture-side `last_type` change check не срабатывает, идёт стандартный window-shift.

Второй recycle path — `DIRT_tree`-driven leaf spawn ([`dirt.cpp:282-326`](../../../new_game/src/world_objects/dirt.cpp)): когда дерево становится in-range, листья spawn'ятся вокруг него. Если кандидат slot имел `DIRT_FLAG_DELETE_OK` (LEAF marked для удаления) и реинкарнируется как LEAF на новой позиции — тоже type-stays-LEAF, тот же баг.

**Фикс — `render_interp_mark_dirt_teleport(slot_idx)` в обоих recycle paths:**
- [`dirt.cpp:472-486`](../../../new_game/src/world_objects/dirt.cpp) — после switch'а в focus-based recycle, гейтован `if (dd->type != DIRT_TYPE_UNUSED)` (UNUSED handled by capture-side type-change auto-detect).
- [`dirt.cpp:325`](../../../new_game/src/world_objects/dirt.cpp) — внутри tree-based DELETE_OK reuse path, после успешного set'а new x/y/z.

**API:** новый `render_interp_mark_dirt_teleport(int idx)` в [`render_interp.h`](../../../new_game/src/engine/graphics/render_interp.h) — wipes `g_dirt_snaps[idx]`, на следующем capture идёт `!valid` (first-capture) path, prev=curr=new. Аналог `render_interp_mark_teleport(Thing*)` для Things.

**Подтверждено пользователем 2026-05-02:** дёргания "блоков" листьев исчезли. Bounce-back collision pos write'ы (line 1080/1265/1395 в dirt.cpp) — small per-cell bounce, не teleport, mark не нужен. Held-can/head positioning (line 1340) — отдельный потенциальный body-vs-held-item desync, не наблюдался, не трогаем.
