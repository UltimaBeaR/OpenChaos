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

### Crash в катсцене на чтении мусорного `current_frame_prev` (root cause найден)

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
