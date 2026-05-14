# Camera improvements — devlog

Назначение: память между сессиями. Контекст у Claude на этой задаче заканчивается (проверено опытом, две прошлые попытки провалились частично из-за этого). Следующий агент должен восстановить состояние **полностью** отсюда.

**Структура файла:**
- **Сводка для следующего агента** (ниже) — что делается, статус, что дальше, ключевые ссылки. Читать первой.
- **Договорённости по алгоритму коллизии** — собраны ответы пользователя по реализации (для предстоящего шага).
- **Хронология** (Записи 1–N) — лог сессии в порядке появления. Читать после сводки, для контекста решений.
- **Reference doc** по удалённым старым системам — отдельный файл [`legacy_wall_collision_reference.md`](legacy_wall_collision_reference.md). Открывать когда понадобится подсказка «как старая система решала X».

---

# Сводка для следующего агента

## Что это за задача

Третья попытка переделать поведение камеры при контакте со стенами/окружением (трекалось в [`known_issues_and_bugs.md` секция «Камера»](../../new_game_planning/known_issues_and_bugs.md)). Первые две попытки провалились — Claude не понимал систему достаточно глубоко, правил наобум, ломал работающую логику.

**Новая стратегия (нерушимая):** старая физическая логика камеры (`FC_process` в [`fc.cpp`](../../new_game/src/camera/fc.cpp)) **НЕ трогается**. Вся новая логика живёт в отдельном модуле [`vis_cam.h`/`.cpp`](../../new_game/src/camera/) поверх результата FC_process. FC_process пишет в `FC_cam[*]` как раньше; `VC_process` после него читает FC_cam и пишет в `VC_state[*]`; render-interp снэпшотит и lerp'ает VC_state.

Это даёт защиту: если новая логика ошибётся — старая система остаётся целой, rollback тривиален. Сломать накапливаемый accumulator FC_process физически невозможно (его поля не трогаются).

## Pipeline (одна строчка)

```
Physics tick:  FC_process()  →  VC_process()  →  render_interp_capture_camera(&FC_cam[0])
Render frame:  RenderInterpFrame ctor (substitute fc->x/y/z = lerp моих snap'ов)  →  draw  →  dtor restore
```

## Что уже сделано

**Подготовка (всё закоммичено пользователем по ходу):**
- Удалены старые системы wall-vs-camera / wall-vs-ground / L1-R1 rotate lock / emergency toonear / `FC_setup_initial_camera` LOS. Камера сейчас игнорирует геометрию полностью. Детали удалённого — в [`legacy_wall_collision_reference.md`](legacy_wall_collision_reference.md).
- Починены 2 бага по ходу: вертикальное кручение правым стиком триггерило get-behind (отсутствовал `fc->nobehind = 0x2000` в Y-ветке); F5/F6/F7/End/Del/PgDn теряли клики при FPS > physics rate (заменено `input_key_just_pressed` → `input_key_press_pending + input_key_consume`).

**Инфраструктура `vis_cam` (готова к коммиту в этой сессии):**
- [`vis_cam.h`](../../new_game/src/camera/vis_cam.h) — `VC_State`, `VC_state[FC_MAX_CAMS]`, `VC_process()`.
- [`vis_cam.cpp`](../../new_game/src/camera/vis_cam.cpp) — реализация: identity copy FC_cam → VC_state + (пустой пока) `vc_process_one(fc, vc, focus)`. Плюс `VC_debug_draw_focus()` (forward-declared в aeng.cpp, не в header — это debug helper, уйдёт со скаффолдом).
- [`render_interp.cpp`](../../new_game/src/engine/graphics/render_interp.cpp) — `render_interp_capture_camera` снэпшотит из `VC_state[idx]`, ctor/dtor не тронуты.
- [`game.cpp`](../../new_game/src/game/game.cpp) — вызов `VC_process()` сразу после `FC_process()`.
- [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp) — вызовы `VC_debug_draw_focus()` в `AENG_draw_city` и `AENG_draw_warehouse` (две render-loop функции для outdoor/indoor режимов соответственно).
- Test toggle на `\` уже удалён (пользователь подтвердил что pipeline работает end-to-end).

**Точка фокуса (raycast origin) найдена:**
- В `vc_process_one` приходит `VC_FocusPoint focus = {fc->focus_x, fc->focus_y + fc->lookabove, fc->focus_z}`.
- Это look-at want-target камеры (приблизительно голова Дарси / центр машины).
- Не путать: focus не находится визуально в центре кадра при движении — это **нормально** из-за smoothing камеры (`yaw += dyaw >> 2`). Готовой точки «центр кадра в мире» в коде нет и быть не может архитектурно.
- В struct VC_FocusPoint пока только x/y/z. Если для алгоритма потребуется ещё (например focus_yaw) — расширь struct.

**Debug helpers временно остаются** (удалить когда алгоритм заработает):
- `CONSOLE_text_at` рисует «center» в центре кадра.
- `VC_debug_draw_focus` рисует красное «FOCUS» в мировой точке `focus`.

## Что нужно делать дальше

**Шаг 2: реализовать алгоритм коллизии в `vc_process_one`.** Описан пользователем — раздел «Договорённости по алгоритму коллизии» ниже. Сейчас тело функции пустое (с комментарием-примером `// vc.y += 0x8000;`).

**Шаг 3 (позже, не в этой итерации):** скрытие модели Дарси / машины когда камера близко.

**Шаг 4 (исследовательский, не в этой итерации):** edge case — Дарси внутри стенки (лестница, прижатие к стене). Запись в договорённостях ниже.

## Структура VisCam (что где)

| Файл | Что |
|------|-----|
| [`vis_cam.h`](../../new_game/src/camera/vis_cam.h) | публичный API: `VC_State`, `VC_state[]`, `VC_process()` |
| [`vis_cam.cpp`](../../new_game/src/camera/vis_cam.cpp) | `VC_FocusPoint` (локальный), `vc_process_one` (тут алгоритм), `VC_process` (entry), `VC_debug_draw_focus` (TEMP) |
| [`render_interp.cpp`](../../new_game/src/engine/graphics/render_interp.cpp) | модифицирован `render_interp_capture_camera` — снэпшотит из VC_state |
| [`game.cpp:1037+`](../../new_game/src/game/game.cpp#L1037) | вызов `VC_process()` после `FC_process()` |
| [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp) | forward-decl + 2 call site для `VC_debug_draw_focus()` |

## Нерушимые правила задачи

1. **Не трогать `FC_process` / `fc.cpp` / FC_Cam struct.** Только читать. Любая ошибка в новой логике не должна сломать старую.
2. **Не угадывать.** Если решение не очевидно — спрашивать пользователя или искать в коде. Прошлые попытки провалились из-за гадания.
3. **«Вроде понял» = «не понял».** Перед действием подтвердить что система правда понятна.
4. **Никаких правок без явного «ок» пользователя.** Вопрос ≠ команда. Списком задач — не лететь вперёд, ждать ack по каждому шагу.
5. **Все важные детали — сразу в этот devlog.** Контекст закончится.

---

# Договорённости по алгоритму коллизии (перед стартом реализации)

Собраны из обсуждения с пользователем 2026-05-14. Когда начинаешь Шаг 2, это твой контракт.

## Базовый алгоритм (формулировка пользователя)

1. Рейкаст(ы) от **точки фокуса** (`focus` в `vc_process_one`) до **текущей позиции камеры** (`vc.x/y/z` — это копия `fc->x/y/z`).
2. Цель — определить нет ли на пути препятствий (стены, неровный пол, и т.д. — то что в старой системе ловилось).
3. Проверять **не только центр** камеры, но и **весь её видимый объём** (рейкаст по нескольким точкам — например 4 угла near-плоскости frustum'а, или другой sampling).
4. Если есть пересечение — **переместить камеру по существующей нормали** (по линии «камера → фокус») в сторону фокуса до точки пересечения **плюс некий offset** (вглубь, к фокусу — это убирает мелкие декоративные элементы со стен типа bump mapping в кадре).
5. **Только вдоль этой нормали**, не сбоку. Камера остаётся на той же оси «view→focus», просто становится ближе к фокусу.

## Уточнения от пользователя

**A) Камера может «пролететь» точку фокуса — это нормально.**
Если Дарси прижата к стенке вплотную, offset может оказаться больше расстояния до фокуса → камера пройдёт через focus и окажется с другой стороны. Не пытаться clamp'ить на focus point, это by design.

**B) Дарси может стоять внутри стенки (лестница, hug-wall, и т.п.).**
Тогда focus point может оказаться **внутри** геометрии и рейкаст «от focus к камере» начнётся уже внутри стены. Предположительный способ обработки (не в первой итерации, **только записать**):
- Если origin рейкаста внутри геометрии — пускать дополнительный рейкаст от Дарси **в противоположную сторону** от камеры; найти точку выхода из стены.
- Использовать эту точку выхода как новый origin рейкаста к камере.
- Точное решение пока неизвестно — оставить как открытый вопрос, не реализовывать в первой версии.

**C) LOS API.** Пользователь не уверен. Цель — игнорировать мелкие объекты (фонари, мусорки), машины тоже игнорировать (на первой итерации). Картинка должна быть «красивой», не должно быть возможности заглянуть на другой конец карты через стенку. Возможно нужен микс API. Действие для тебя: **разобраться** какой LOS API что покрывает (стены/заборы/прозрачные стены/полы/etc), и решить — возможно `there_is_a_los` с конкретным набором `LOS_FLAG_*` подойдёт. Опции:
- `there_is_a_los(x1,y1,z1, x2,y2,z2, flags)` — точная геометрия. Флаги: `LOS_FLAG_IGNORE_PRIMS` (мелкие объекты → нам нужно их игнорить), `LOS_FLAG_IGNORE_SEETHROUGH_FENCE_FLAG`, `LOS_FLAG_IGNORE_UNDERGROUND_CHECK`.
- `MAV_height_los_slow(warehouse_id, x1,y1,z1, x2,y2,z2)` — height-aware MAV-навмеш; заполняет глобалы `MAV_height_los_fail_x/y/z` точкой пересечения.

Принципиально для рейкаста нужна **точка пересечения** (чтобы переместить камеру к ней). У `MAV_height_los_slow` это в готовом виде. У `there_is_a_los` — придётся итеративно сужать (binary search вдоль луча). Возможно стоит начать с MAV_height_los_slow + флаг для игнора мелких; если он не игнорит мелочёвку — переключиться на there_is_a_los или комбинацию. **Решение — за тобой, в коде после анализа API.**

**D) Многоточечный sampling.** Без разницы какой именно — пользователь сказал «бери тот что дешевле по перфу». Варианты:
- (a) Одна точка центра + safe-radius offset в сторону focus (минимум вычислений).
- (b) 4 угла near-плоскости frustum'а (4 рейкаста).
- (c) Дискретная сетка 3×3 / 4×4 (9-16 рейкастов, дороже).
Я склоняюсь к (b) — 4 рейкаста, симметричный покрывает корнер-кейсы. **Перф: помни про целевое железо** (Steam Deck, слабые ПК) — не делать больше чем достаточно.

**E) Машина (CLASS_VEHICLE).** Тот же алгоритм без особенностей. Скрытие машины (когда камера близко) — позже.

**F) Offset «вплотную к стене + немного».** «Полметра» по словам пользователя. В world units — Дарси ростом ~`0x10000` (≈ 1.7m), значит полметра ≈ `0x4800-0x5000`. Стартовое — берём ~`0x4000–0x5000`. Подкрутим по факту визуально.

**G) Скрытие модели Дарси / машины** при близкой камере — **НЕ в этой итерации**. Сначала только collision. Скрытие — отдельный шаг.

**H) Y-составляющая.** Не обсуждалось напрямую. По смыслу «полы / уступы» тоже могут быть препятствием — алгоритм должен работать в 3D, не только XZ. Возможно пол под камерой будет ловиться при низком pitch. Если станет проблемой — обсудить отдельно.

**I) Кинематики (EWAY/PLAYCUTS) — не трогать.** Сценарные камеры идут своим pipeline через EWAY_grab_camera + render_interp_apply_eway_camera. Применять collision-логику только когда **не** активна cutscene-камера (по флагу `EWAY_cam_active` или результату `EWAY_grab_camera`).

---

# Хронология

## Запись 1 — 2026-05-13 — стартовая фиксация контекста

### Что хотим сделать

В Urban Chaos есть задача улучшения поведения камеры (трекается в [`new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md), секция «Камера»):

- Камера должна всегда вращаться вокруг персонажа, даже если мешают стены.
- При коллизии со стеной — приближаться к персонажу (вплоть до позиции внутри модели).
- Когда камера слишком близко / внутри персонажа — скрывать модель Дарси.
- Сейчас камера блокируется стенами и перестаёт вращаться, плюс проходит сквозь объекты и видны «кишки» (см. также «Объекты, прорезаемые near clipping plane» там же).

### История — почему третья попытка

Это **третья попытка** переписать поведение камеры начиная с текущего коммита.

Первые две попытки провалились. Главная причина (формулировка пользователя):

> «У тебя то ли контекст заканчивался то ли ты всю систему понять не мог и ты не проверял то что надо проверять. Делал наобум. Чето правил — ломалось в другом месте. Как будто архитектуру не понимал.»

И:

> «Я просил тебя полностью подробно проанализировать как СЕЙЧАС работает перед тем как что то делать, чтобы ты понял систему. Но это тебе не помогло.»

То есть глубинная причина провалов — **я не понимал систему**, при этом всё равно правил, фиксил одно — ломалось другое, по кругу.

Дополнительно: в обеих попытках я каждый раз **переписывал всю систему работы с камерой** (зачем — пользователь уже не помнит, видимо чтобы «правильно сделать»). Это усугубляло: ломалась рабочая логика, и при этом своя — не работала как надо.

### Новая стратегия — старая логика НЕ трогается

Принципиальное изменение подхода:

1. **Существующая физическая логика камеры — НЕ трогается вообще.** Это нерушимое правило этой попытки.
2. Заводим **отдельный набор переменных** для новой системы камеры.
3. Наша логика встраивается как **новый этап**, идущий ПОСЛЕ всей старой физики камеры и ПЕРЕД фазой интерполяции (которая сейчас уже работает).
4. **Интерполяция переключается** на чтение наших новых переменных вместо старых.
5. Между кадрами можно хранить накапливающийся state в наших переменных.
6. **Кинематики (cutscenes) — не трогаем**, пусть работают как раньше; интерполяция нужна только для геймплейной камеры.

Ключевая защита: даже если я не до конца понимаю старую систему — я её не ломаю. Худший сценарий — наша новая логика считает ерунду, интерполяция читает ерунду. Старая система при этом остаётся целой; rollback = «вернуть интерполяцию на старые переменные».

### План (этапы)

**Этап 1 — этот devlog.** Заведён, в него пишется всё подряд.

**Этап 2 — итеративное удаление существующих ограничителей камеры от окружения:**
- Удалить системы wall-vs-camera, wall-vs-ground, и прочее что ограничивает движение камеры из-за окружающих объектов.
- По итогу — камера ведёт себя полностью дефолтно, можно крутить как угодно, ни за какие объекты не задевается. Как будто вокруг пустое пространство.
- Гранулярность: можно удалять сразу всё что **чётко** идентифицируется как такая система. Если в чём-то не уверен — отдельная итерация и спросить пользователя.
- После каждой итерации — пользователь проверяет в игре что ничего не сломалось.
- **Параллельно** (или в процессе) — отдельный файл-референс в этой же папке (`new_game_devlog/camera_improvements/{название}.md`): описание того, как старые системы wall-vs-camera / wall-vs-ground работали. Это референс для будущего построения новой системы — на случай если нужны подсказки. Название придумаю в момент создания.

**Этап 3 — анализ pipeline камеры (БЛОКЕР перед Этапом 4):**
- Найти где в коде идут физические расчёты камеры (game tick).
- Найти где идёт интерполяция камеры.
- Проверить **нет ли между физикой и интерполяцией ничего связанного с камерой, что считается на render-тике или в другой неявной фазе.** Если такое есть — это потенциально ломает план: моя вставка «после физики, перед интерполяцией» не сработает чисто, надо будет разбираться отдельно.
- Зафиксировать в devlog: конкретные файлы, функции, переменные, и pipeline в виде «X считается в Y в момент Z».
- **Показать пользователю результат анализа** до того как начну что-то строить. Дать ему возможность сказать «вот тут ты не разобрался» — ДО правок, а не после.

**Этап 4 — построение новой системы (только после ack пользователя по Этапу 3):**
- Завести новые переменные для камеры.
- Вычислять их после физики камеры.
- Переключить интерполяцию на новые переменные.
- Конкретный алгоритм коллизии — пользователь объяснит когда дойдём до этой точки.

### Ключевые установки от пользователя (правила этой попытки)

- **НЕ трогать старую логику камеры вообще.** Нерушимо.
- **НЕ гадать.** Если не уверен — фиксировать вопрос в devlog или спрашивать.
- **«Вроде понял» ≠ «понял».** Прежде чем что-то делать — проверять что реально разобрался. «Вроде понял» = «не понял».
- Удалять можно сразу всё что **чётко** определяется как система ограничения камеры окружением. Спорные случаи — отдельной итерацией.
- На вопросы пользователя — сначала отвечать, а не сразу лезть править код. Вопрос ≠ команда.
- На каждом шаге работы по списку — останавливаться и ждать явного «ок» / «дальше», не лететь вперёд.
- Все важные детали по ходу — записывать в этот devlog сразу. Память между сессиями.

### Для следующего агента (если контекст закончится посреди задачи)

Если ты читаешь это после потери контекста:

1. Прочитай [`CLAUDE.md`](../../CLAUDE.md) и эту запись полностью.
2. Прочитай **весь** devlog подряд — последние записи могут переопределять старые.
3. **Не доверяй памяти/предположениям** — проверяй текущее состояние файлов через `git diff` / чтение. История двух провалов — именно из-за «вроде помню как должно быть».
4. Не лезь в код пока пользователь явно не подтвердит на каком ты этапе и можно ли продолжать. Спроси «я понял что мы на Этапе X, идём дальше или есть корректировки?» — это нормально, лучше переспросить чем сломать.
5. Если уже создан файл-референс по старым системам (см. Этап 2) — прочитай его тоже.

### Текущее состояние

- Этап 1 (этот devlog): ✅ создан с этой записью.
- Этап 2: не начат, жду ack пользователя на старт.
- Этап 3: не начат.
- Этап 4: не начат.
- Файл-референс по старым системам: ещё не создан.

### Открытые вопросы

Пока нет.

---

## Запись 2 — 2026-05-13 — аудит существующих ограничителей камеры

Прочитан целиком [`new_game/src/camera/fc.cpp`](../../new_game/src/camera/fc.cpp) (1550 строк) — основной файл камеры. Также прочитаны [`fc.h`](../../new_game/src/camera/fc.h), [`cam.h`](../../new_game/src/camera/cam.h), [`cam_globals.h`](../../new_game/src/camera/cam_globals.h), [`fc_globals.h`](../../new_game/src/camera/fc_globals.h).

Проверены callers ключевых функций — [`game_tick.cpp`](../../new_game/src/game/game_tick.cpp), [`input_actions.cpp`](../../new_game/src/game/input_actions.cpp), [`memory.cpp`](../../new_game/src/missions/memory.cpp), [`eway.cpp`](../../new_game/src/missions/eway.cpp), [`elev.cpp`](../../new_game/src/assets/formats/elev.cpp).

Замечание: `cam.h` декларирует `CAM_process` / `CAM_set_mode` / `CAM_set_focus` / `CAM_set_collision` и т.д., но реализаций нигде в `new_game/src` нет (только outro имеет свой одноимённый `CAM_process`). Это мёртвый header, его не трогаем — вне scope задачи. Реальная камера живёт в `fc.cpp` (Final Camera, FC_*).

### Чёткие кандидаты на удаление (wall-vs-camera / wall-vs-ground ограничители)

**A. fc.cpp 1059–1203 — 8-шаговый raycast collision push (внутри `FC_process`)**
- Блок `if (!fc->toonear) { ... }`, содержит xforce/yforce/zforce, проверки `MAV_inside`, fence detection через `there_is_a_los`, `WARE_get_caps`, `MAV_get_caps`, `MAVHEIGHT` (крыша).
- Две ветки: warehouse (1075–1112) и outdoor (1113–1198).
- Эффект: текущая камера упирается в стены/заборы/крышу.
- Удаление чистое: блок самоизолирован, после него идёт другой `if (!fc->toonear)`.

**B. fc.cpp 1229–1239 — wall-reactive get-behind boost (внутри get-behind в `FC_process`)**
- `if (xforce || zforce) { ...сильное прижатие... } else { ...обычное... }` — усиление get-behind когда камера ударилась о стену.
- После удаления A, xforce/zforce всегда 0 → всегда else. Эти переменные станут не нужны.
- Удаляем if/else, оставляем тело else.

**C. fc.cpp 684–746 — `FC_allowed_to_rotate` + использование в L1/R1 rotate**
- Функция проверяет 4 угла позиции после поворота через `MAV_inside` (warehouse наоборот).
- Используется в `FC_rotate_left` (755) и `FC_rotate_right` (767).
- Это и есть «камера блокируется стенами и перестаёт вращаться по L1/R1».
- Удаляем функцию; в rotate_left/right убираем `if(allowed_to_rotate)`, всегда выставляем `fc->rotate = ±0x600`.

**D. fc.cpp 546–638 — `FC_force_camera_behind`: внутренняя wall-aware логика**
- Функция вызывается извне (`game_tick.cpp` x2, `input_actions.cpp` x5, `memory.cpp` x1, и внутри `FC_setup_initial_camera`). **Саму функцию не удаляем.**
- Внутри неё: `MAV_height_los_slow` проверка, fallback на 4 альтернативных угла (±100, ±200), emergency toonear setup с saved state.
- Удаляем wall-collision части: LOS check + 4 альтернативы + emergency toonear setup.
- Остаётся: посчитать gox/goy/goz behind focus и присвоить fc->want_x/y/z. Простое присваивание.

**E. fc.cpp 641–680 — `FC_setup_initial_camera`: initial LOS check + fallback**
- На старте миссии: вызывает `there_is_a_los(...)`. Если OK — ставит в default behind. Если нет — `FC_force_camera_behind`.
- Удаляем LOS check + fallback. Всегда ставим в default behind position (тело then-ветки).

### Что НЕ трогаем (это не wall-vs-camera)

- `FC_calc_focus` (focus position, не camera position)
- `FC_look_at_focus` (aim yaw/pitch/roll)
- `FC_can_see_person`, `FC_can_see_point` — это **gameplay-LOS** (используется gameplay-кодом «виден ли NPC из камеры»), камеру не двигает
- `FC_explosion`, shake-блок
- Get-behind core (1213–1248 за вычетом wall-reactive части B)
- Height tracking Y (1252–1300)
- Distance clamp FC_DIST_MIN/MAX (1302–1342)
- Smoothing position/angles (1344–1389)
- Vehicle platform inheritance (881–923)
- Warehouse transition snap (925–938)
- `FC_setup_camera_for_warehouse` (entry positioning, не collision)
- `FC_position_for_lookaround` (first-person look — отдельный режим, не wall avoidance)
- `FC_alter_for_pos`, `FC_focus_above` — distance/height presets
- `toonear` механизм в целом — он используется first-person look'ом

### Спорное / вынести в отдельную итерацию

**F. fc.cpp 1256–1260 — `GAME_FLAGS & GF_NO_FLOOR` Y floor clamp**
- `if (goto_y < 0x28000) goto_y = 0x28000;` — не даёт камере опуститься ниже на уровнях без пола.
- Это не «wall vs camera», это safety против ухода под землю на специальных уровнях.
- Не уверен что это надо удалять — спросить пользователя. По умолчанию **оставляю.**

### План удаления

Можно одной итерацией — все A–E. Пользователь сказал «можешь хоть сразу все удалить, главное не гадай». А–E чётко идентифицированы. F отдельно (или вообще оставляем).

### Текущее состояние

- Этап 1: ✅
- Этап 2: аудит сделан, жду ack пользователя на удаление A–E одной итерацией. По F — отдельный вопрос пользователю.
- Этап 3 (pipeline analysis): не начат.
- Этап 4: не начат.

---

## Запись 3 — 2026-05-13 — удаление A–E выполнено

Пользователь подтвердил A–E одной итерацией. F отложил («скажу после теста»).

### Что сделано в [`new_game/src/camera/fc.cpp`](../../new_game/src/camera/fc.cpp)

Diff: +18 / −361.

- **A** (8-шаговый raycast push в `FC_process`) — удалён целый блок `if (!fc->toonear) { ... }` (с warehouse и outdoor ветками). Следующий `if (!fc->toonear)` остался нетронут.
- **B** (wall-reactive boost в get-behind) — удалён `if (xforce||zforce){stronger}else{normal}`, оставлено только тело normal. После удаления A это эквивалентно прежнему поведению в случае «без wall hit», и теряет реакцию на wall hit (которая по смыслу больше не возможна, xforce/zforce всегда 0).
- **C** — функция `FC_allowed_to_rotate` удалена целиком. `FC_rotate_left/right` теперь безусловно выставляют `fc->rotate = ±0x600`.
- **D** — внутри `FC_force_camera_behind` удалены: `MAV_height_los_slow` check, fallback на 4 угла (±100, ±200), emergency toonear setup, метка `found_place:;`. Осталась чистая постановка want_x/y/z behind focus.
- **E** — в `FC_setup_initial_camera` удалена `there_is_a_los(...)` проверка и else-ветка с `FC_force_camera_behind`. Теперь всегда default behind position.

Сопутствующие cleanup'ы:
- Убраны includes: `engine/physics/collide.h` (LOS_FLAG_*), `map/pap.h` (PAP_calc_map_height_at). MAV_inside ещё используется в `FC_can_see_point` (gameplay-LOS), подгружается транзитивно через `buildings/ware.h` → `ai/mav.h`.
- Убраны unused `#define`s: `FC_PUSH_XS/XL/ZS/ZL`, `FC_ROTATE_NEAR`, `FC_ROTATE_DIR_LEFT/RIGHT`. Оставлены `FC_CSP_HEIGHT/RADIUS` — нужны для `FC_can_see_person`.
- Убраны unused локалы в `FC_process`: `i`, `x, y, z`, `xforce, yforce, zforce`.
- Шапка `FC_process` обновлена: убраны пункты «8. Collision: 8-step raycast» и «9. Get-behind with configurable strength», нумерация сдвинута.

### Что НЕ тронуто (по плану)

`FC_calc_focus`, `FC_look_at_focus`, `FC_can_see_person`, `FC_can_see_point`, `FC_explosion`, get-behind core, height tracking Y, distance clamp, smoothing position/angles, vehicle platform inheritance (881–923), warehouse transition snap, `FC_setup_camera_for_warehouse`, `FC_position_for_lookaround` (first-person look), `FC_alter_for_pos`, `FC_focus_above`, `toonear` механизм, F (GF_NO_FLOOR floor clamp — отложен до решения пользователя).

### Валидация

- `make build-release`: `[297/297] Linking CXX executable Release\OpenChaos.exe`. Только pre-existing warnings, не связанные с правками.
- Review skill: dangling if/else проверен, completeness проверена, нет stray кода. ✅
- Игровое тестирование: ожидается от пользователя.

### Что ожидать игроку (для контекста, не для GAMEPLAY_CHANGES.md пока)

Камера теперь должна проходить сквозь стены, заборы, крыши warehouse, не упираться при L1/R1 повороте. Камера никогда не должна «застревать» из-за окружения. Если есть кейс когда камера ведёт себя как раньше (упирается, не вращается) — это баг, в этой итерации этого быть не должно.

**Это переходное состояние** — новая система коллизий камеры будет надстроена сверху на Этапе 4. GAMEPLAY_CHANGES.md обновим тогда, не сейчас.

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅ удалено A–E. Открыто F (ждём решения пользователя после теста).
- Этап 3 (pipeline analysis): не начат.
- Этап 4 (новая система): не начат.

### Открытые вопросы

- **F** (GF_NO_FLOOR Y floor clamp, fc.cpp 1256–1260 до удаления — сейчас сместилось на меньшие номера строк): «удалять или нет». Пользователь сказал что решит после игрового теста.

---

## Запись 4 — игровое тестирование удаления A–E пройдено

Пользователь протестировал в игре результат удаления A–E. Все ситуации показали правильное поведение — камера полностью игнорирует геометрию окружения.

### Проверенные ситуации (все pass)

- Против стен зданий, в том числе при приближении через L1.
- Внутри помещений, против внутренних стен.
- Сверху-вниз на элементах зданий-уступах (первая миссия с полицейской машиной) — камера поворачивается сверху-вниз без проблем. **Замечание пользователя:** это **отдельная система**, не общий алгоритм против стен. Мы её не трогали, она продолжает работать корректно.
- Против гор на неровной земле (миссия Target UC).

Везде камера просто игнорирует геометрию — это и есть желаемое поведение для текущего шага.

Дополнительно проверено: поведение камеры само по себе (без коллизий) не сломалось — везде ведёт себя как и раньше.

### Замечание про «сверху-вниз на уступах»

Пользователь явно отметил что это **отдельная подсистема**, не часть удалённого общего алгоритма wall-vs-camera. Скорее всего речь про какую-то Y-компоненту (pitch over terrain / step) — не уверен, при необходимости разобраться позже. Сейчас это работает корректно и удаление A–E её не задело — фиксирую как факт.

### Решение по F (GF_NO_FLOOR floor clamp)

**Оставляем как есть.** Пользователь сказал: «не нашёл ситуаций где оно мешает; меньше удаляем — меньше риска всё поломать».

Это **не** wall-vs-camera, это safety на спец-уровнях без пола. Соответствует принципу «не трогать живой код без необходимости».

### Этап 2 — закрыт

A–E удалены, тест пройден, F оставлен. Этап 2 закрыт. Пользователь сейчас закоммитит результат.

### Следующий шаг

Этап 2bis (параллельно/в процессе): **отдельный файл-референс** с описанием того как удалённые системы (A–E) работали. Это референс для построения новой системы — куда смотреть когда понадобится подсказка про оригинальные алгоритмы. Имя файла придумаю при создании.

Затем Этап 3 — анализ pipeline камеры (физика → интерполяция → render). Это блокер перед построением новой системы.

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅ (удаление A–E, F оставлен)
- Этап 2bis (reference doc по удалённым системам): не начат
- Этап 3 (pipeline analysis): не начат
- Этап 4 (новая система): не начат

---

## Запись 5 — reference doc по удалённым системам создан

Создан [`legacy_wall_collision_reference.md`](legacy_wall_collision_reference.md) — справочник по системам A–E (плюс описание F которая не удалена, и список того что не тронуто и почему).

Структура файла:
- Архитектура «want vs actual» (фундамент)
- Toonear режим (общая инфра, использовалась в wall-collision и first-person)
- A: 8-шаговый raycast push (outdoor + warehouse ветки, fence detection)
- B: wall-reactive get-behind boost
- C: FC_allowed_to_rotate + L1/R1 lock
- D: FC_force_camera_behind wall-aware часть (LOS + 4 альтернативы + emergency toonear)
- E: FC_setup_initial_camera LOS + fallback (с упоминанием бага MuckyFoot `focus_x` вместо `focus_y` во втором аргументе)
- F: GF_NO_FLOOR Y floor clamp (НЕ удалено)
- Что не тронуто и почему: gameplay-LOS, first-person, camera core
- Подсказки для построения новой системы (чего переиспользовать, чего избегать)

Назначение — справочник для будущих сессий когда понадобится «как старое решало X»: открыть файл, найти нужную секцию, прочитать алгоритм + магические числа + зависимости.

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅ (удаление A–E, F оставлен)
- Этап 2bis: ✅ (reference doc создан)
- **Промежуточная задача от пользователя:** «один мерзкий баг в камере» — детали ещё не сказаны, ждём от пользователя
- Этап 3 (pipeline analysis): не начат
- Этап 4 (новая система): не начат

---

## Запись 6 — фикс бага «get-behind активируется при вертикальном кручении правым стиком»

### Симптом

В состоянии покоя (Дарси стоит) если правым стиком отклонить камеру от дефолтного положения «за спиной»:
- **Горизонтальное кручение стика** (`stick_x`) — система get-behind не активируется, камера держится в той ориентации куда её отклонили. ✅
- **Вертикальное кручение стика** (`stick_y` — height adjust) — система get-behind **активируется прямо во время кручения** и тянет камеру обратно за спину Дарси. ❌

Гипотеза пользователя: горизонтальная ветка ставит какой-то «активность управления» флаг, который подавляет get-behind, а вертикальная — нет.

### Причина (подтверждена кодом)

Флаг — `fc->nobehind` (это и означает «не пытайся развернуть камеру за спину»). Используется в [`FC_process` get-behind блоке](../../new_game/src/camera/fc.cpp): пока `fc->nobehind > 0`, get-behind пропускается и nobehind decay'ется (`-= 0x80 * TICK_RATIO >> TICK_SHIFT` каждый тик).

Места где `nobehind = 0x2000` выставляется (т.е. «пользователь сейчас управляет камерой, get-behind не лезь»):
- L1/R1 rotate decay-блок ([fc.cpp:790](../../new_game/src/camera/fc.cpp#L790))
- Right stick **horizontal** orbit ([fc.cpp:819](../../new_game/src/camera/fc.cpp#L819))
- Right stick **vertical** height adjust — **отсутствовало**, в этом и был баг

При горизонтальном кручении nobehind постоянно «обновлялся» до 0x2000 → get-behind подавлялся. При вертикальном — nobehind decay'ался до 0 → get-behind включался даже пока стик отклонён.

### Фикс

Добавлена одна строка `fc->nobehind = 0x2000;` в ветку `if (abs(stick_y) > 8000)` (после Y clamp), [fc.cpp:835](../../new_game/src/camera/fc.cpp#L835).

### Валидация

- `make build-release`: passed.
- Игровое тестирование пользователем: симптом пропал, при вертикальном кручении get-behind не активируется (ровно как при горизонтальном).

### Note для будущего агента

Это **не** часть удаления wall-collision систем (Этап 2). Это отдельный pre-existing баг в нашем добавленном right-stick orbit коде. Пользователь попросил починить между Этапом 2bis и Этапом 3 — «по дороге, пока в контексте».

Раз я в принципе сижу в fc.cpp и пишу right-stick orbit код, **на будущее проверять**: любая ветка нового пользовательского управления камерой должна выставлять `fc->nobehind = 0x2000;`, иначе get-behind будет вмешиваться в это управление.

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅
- Этап 2bis: ✅
- Bugfix вертикального стика: ✅
- Этап 3 (pipeline analysis): не начат
- Этап 4 (новая система): не начат

---

## Запись 7 — фикс класса бага «edge-triggered camera keys теряются на FPS > physics rate»

### Симптом

F5/F6/F7 (set camera type 1/2/3 + behind) на 60+ FPS с интерполяцией срабатывали 1 из 10–15 нажатий. На 30 FPS cap без интерполяции — стабильно. То же касается Delete/PgDn (rotate left/right на клаве) и End (force behind на клаве) — но они нажимаются часто, потеря одного клика не замечалась.

### Причина (architecture-level)

Rate mismatch между render frame и physics tick:

- [`input_frame_update`](../../new_game/src/engine/input/input_frame.cpp#L169) копирует `prev = curr` и обновляет `curr` **каждый render frame** (через `LibShellActive` в [host.cpp:241](../../new_game/src/engine/platform/host.cpp#L241)).
- [`input_key_just_pressed(k)`](../../new_game/src/engine/input/input_frame.cpp#L264) = `curr[k] && !prev[k]` — true только на одном snapshot'е.
- F5/F6/F7 опрашиваются внутри [`get_hardware_input`](../../new_game/src/game/input_actions.cpp#L3338), которая вызывается из `do_packets` ([thing.cpp:506](../../new_game/src/things/core/thing.cpp#L506)) **только когда physics accumulator дотягивает до tick** ([game.cpp:1008-1015](../../new_game/src/game/game.cpp#L1008-L1015)).

Сценарий потери: нажал F5 на render frame N → `prev=0, curr=1`. Physics accumulator < tick → process_things пропущен → клик не прочитан. Render frame N+1 → `input_frame_update` → `prev=1, curr=1` или `0`. Edge стёрт. Когда physics tick наконец отработает — `just_pressed = false`. Клик потерян.

При physics_hz=30, render=60+ → ~50% кадров без physics tick → ~50% кликов теряется.

### Решение

Заменено `input_key_just_pressed` на `input_key_press_pending` + `input_key_consume` (известный паттерн в проекте — уже используется в [siren](../../new_game/src/game/input_actions.cpp#L2979-L2981), [JUMP](../../new_game/src/game/input_actions.cpp#L3808) в той же функции, в [widget.cpp](../../new_game/src/ui/menus/widget.cpp#L69-L70), [gamemenu.cpp](../../new_game/src/ui/menus/gamemenu.cpp#L206), [playcuts.cpp](../../new_game/src/missions/playcuts.cpp#L511)).

`s_keys_press_pending` латчит rising edge и **переживает** frame_update'ы, сбрасывается только явным `input_key_consume`. Подходит для consumer'а, работающего на ритме отличном от frame_update.

### Affected — 6 клавиатурных edge-triggered camera actions

Все в `get_hardware_input`, [`input_actions.cpp:3338-3375`](../../new_game/src/game/input_actions.cpp#L3338-L3375):

| Клавиша | Действие | Default scancode |
|---------|----------|------------------|
| F5 | set camera type 1 + behind | hard-coded `KB_F5` |
| F6 | set camera type 2 + behind | hard-coded `KB_F6` |
| F7 | set camera type 3 + behind | hard-coded `KB_F7` |
| End | camera behind (без смены type) | `keybrd_button_use[JOYPAD_BUTTON_CAMERA]`, default 207 (KB_END), config.ini `[Keyboard] camera` |
| Delete | rotate left (как L1) | `keybrd_button_use[JOYPAD_BUTTON_CAM_LEFT]`, default 211, config.ini `[Keyboard] cam_left` |
| PgDn | rotate right (как R1) | `keybrd_button_use[JOYPAD_BUTTON_CAM_RIGHT]`, default 209, config.ini `[Keyboard] cam_right` |

### НЕ affected (по архитектуре)

- **Геймпад LB (L1, button 9)** — set type 0 + behind через [`input_btn_held`](../../new_game/src/game/input_actions.cpp#L3129). Level semantic, не edge, race нет.
- **Геймпад L2/R2** — для камеры не маппятся (комментарий [input_actions.cpp:3134](../../new_game/src/game/input_actions.cpp#L3134) «L2/R2 no longer map to camera rotation on gamepad»). На foot — PUNCH/KICK, в машине — gas/brake.
- **Правый стик геймпада** — отдельный код в [fc.cpp:806-836](../../new_game/src/camera/fc.cpp#L806-L836). Не трогался.
- На геймпаде установка type 1/2/3 (как F5/F6/F7) не реализована — нет.

### Удалён неверный комментарий

В [input_actions.cpp:3358-3362](../../new_game/src/game/input_actions.cpp) (до фикса) был комментарий:
> «input_frame edge-detect doesn't need consume»

Это утверждение **ложно** в условиях decoupled physics/render rate и фактически являлось источником этого бага — кто-то (видимо я в прошлой миграции) предполагал что не нужно. Удалён, заменён комментарием объясняющим почему нужен press_pending+consume.

### Валидация

Пользователь протестировал:
- F5, F6, F7 — работают каждое нажатие. ✅
- Delete, PgDn — работают каждое нажатие. ✅
- End — «делает то же самое что F5, разницы не увидел; считаем что ок». ✅
  - Note: по коду F5 = `FC_change_camera_type(1)` + force behind (меняет presets cam_dist/cam_height на type 1: dist=0x280, height=0x20000), End = только force behind (type не меняется). Если type уже был 1 после F5 — эффекты идентичны. Если был дефолт type 0 — должны отличаться. Тонкая разница которую пользователь не стал разбирать, не блокер.
- Регрессы (правый стик, LB геймпад) — не проверены отдельно, но архитектурно не affected.

### Note для будущего агента

**Любое новое edge-triggered key input в `get_hardware_input` (или другом коде, читаемом из `do_packets`/physics tick) ДОЛЖНО использовать `input_key_press_pending + input_key_consume`, не `input_key_just_pressed`.**

`input_key_just_pressed` корректно только если consumer работает на том же ритме что `input_frame_update` (т.е. на render-тике). Для game-tick / physics-tick consumer'ов — потеря edges гарантирована при render_fps > physics_fps.

Тот же паттерн распространяется на `input_btn_just_pressed` для gamepad button — там есть `input_btn_press_pending` + `input_btn_consume`. Применять симметрично, если есть gamepad-кнопки опрашиваемые на physics tick через edge.

### Связь с предыдущей задачей

Этот баг **не** связан с удалением wall-collision (Этап 2) и не связан с bugfix вертикального стика (Запись 6). Это отдельная архитектурная проблема в input-слое, которая просто всплыла по дороге.

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅
- Этап 2bis: ✅
- Bugfix вертикального стика: ✅
- Bugfix edge-triggered camera keys: ✅
- Этап 3 (pipeline analysis): не начат
- Этап 4 (новая система): не начат

---

## Запись 8 — анализ pipeline камеры (Этап 3, блокер перед Этапом 4)

### Карта pipeline

**Physics tick (30Hz design, `g_physics_hz`):**

Внутри [`game.cpp:1008-1114`](../../new_game/src/game/game.cpp#L1008) — `while (physics_acc_ms >= phys_step_ms)` loop:

1. `process_things(1, phys_tick_diff)` — внутри `do_packets()` → `get_hardware_input()` → опрос F5/Del/PgDn/End и т.д. Также: AI, физика, движение.
2. PARTICLE_Run / OB_process / TRIP_process / DOOR_process / EWAY_process.
3. WMOVE_draw / BALLOON_process / MAP_process / POW_process.
4. **`FC_process();`** ([`game.cpp:1037`](../../new_game/src/game/game.cpp#L1037)) — **физический шаг камеры**, вся старая логика FC_Cam в [`fc.cpp`](../../new_game/src/camera/fc.cpp). Включает: FC_calc_focus, vehicle inheritance, warehouse transition, get-behind, right-stick orbit, Y-tracking, distance clamp, smoothing want→actual для xz и углов, lens, shake apply + decay.
5. GAME_TURN++.
6. **render-interp snapshot block** ([`game.cpp:1056-1111`](../../new_game/src/game/game.cpp#L1056-L1111)):
   - `render_interp_capture(p)` для каждого moving Thing (CLASS_PERSON, VEHICLE, PROJECTILE и т.д.).
   - **`render_interp_capture_camera(&FC_cam[0])`** ([`game.cpp:1091`](../../new_game/src/game/game.cpp#L1091)) — snapshot CURR состояния FC_cam[0] (x/y/z/yaw/pitch/roll/etc).
   - `render_interp_capture_eway_camera()` — snap EWAY_cam_* globals (cutscene camera).
   - `render_interp_capture_dirt()`, `render_interp_capture_grenades()`.
7. `physics_acc_ms -= phys_step_ms`.

После цикла:
- `g_render_alpha = physics_acc_ms / phys_step_ms` (clamped [0,1]).

**Render frame (free-running, render_fps_cap):**

8. `draw_screen()`:
   - **`RenderInterpFrame` ctor** ([`render_interp.h`](../../new_game/src/engine/graphics/render_interp.h)):
     - Walks all valid snapshots.
     - Writes interpolated `lerp(prev, curr, g_render_alpha)` directly into `Thing.WorldPos / Tweened` angles AND **directly into FC_cam[0] fields** (x/y/z/yaw/pitch/roll).
   - All render code (AENG_draw, FIGURE_draw, panel HUD, bloom, etc) читает FC_cam[0] как обычно — видит интерполированные значения.
   - **EWAY/PLAYCUTS path** на render-тике (cutscene only):
     - `aeng.cpp:6069` — `EWAY_grab_camera` overrides fc->x/y/z в FC_cam (cutscene).
     - `render_interp_apply_eway_camera` substitutes EWAY raw values с интерполированными EWAY values.
     - `playcuts.cpp:374,448` — пишет fc->lens (PLAYCUTS only).
     - `aeng.cpp:559-565` (`AENG_set_camera`) — пишет в FC_cam (PLAYCUTS only).
   - **`RenderInterpFrame` dtor** — restores authoritative post-tick values back в FC_cam[0].

9. `handle_sfx()` ([`game.cpp:1342`](../../new_game/src/game/game.cpp#L1342)):
   - `MFX_set_listener` использует `FC_cam[0].yaw/pitch/roll` (authoritative, post-RenderInterpFrame-dtor).
   - `MFX_update()` → внутри для `MFX_CAMERA` voices читает `FC_cam[0].x/y/z` ([`mfx.cpp:1003-1006`](../../new_game/src/engine/audio/mfx.cpp#L1003-L1006)).

### Ответ на ключевой вопрос пользователя: «есть ли расчёты камеры на render-тике вне интерполяции?»

**В обычном gameplay — нет.** Все write'ы в FC_cam[0] на render-тике принадлежат cutscene-path'у (EWAY / PLAYCUTS), и они уже корректно обрабатываются через `render_interp_apply_eway_camera`. Read'еры на render-тике (panel HUD, MFX, bloom) — это только чтения, не двигают камеру.

**В cutscenes — да**, но пользователь явно сказал «кинематики не трогать», и их substitution path уже работает корректно отдельной системой.

**Вывод: точка интеграции для новой системы существует и чистая.**

### Точка интеграции для Этапа 4

**Между [`FC_process()` (game.cpp:1037)](../../new_game/src/game/game.cpp#L1037) и [`render_interp_capture_camera(&FC_cam[0])` (game.cpp:1091)](../../new_game/src/game/game.cpp#L1091).**

Любая строка в диапазоне 1038-1090 подойдёт. Самая чистая — сразу после `FC_process();`. На этой точке:
- FC_process уже записал свои значения в `FC_cam[0].x/y/z/yaw/pitch/roll/etc` (включая shake, smoothing, get-behind, всё).
- render-interp ещё не сделал snapshot — значит мои записи попадут в snapshot.

### Архитектурный вопрос на ack пользователя

Пользователь явно сказал: «отдельный набор переменных» + «интерполяция от своих переменных».

Я вижу два варианта реализации этого:

**Вариант A — отдельные поля в FC_Cam (или отдельная struct):**
- Старая система FC_process пишет в fc->x/y/z (как сейчас).
- Моя логика читает fc->x/y/z (результат старой), пишет в fc->cam_x/y/z (новые поля).
- `render_interp_capture_camera` модифицирован: снэпшотит мои fc->cam_x/y/z, **не** fc->x/y/z.
- `RenderInterpFrame` ctor: substitutes interpolated моих snap'ов в **fc->x/y/z** (там где их читают render / MFX / panel).
- `RenderInterpFrame` dtor: восстанавливает fc->x/y/z **к моим non-interp значениям**, не к старым (значениям FC_process).
- Между render frame'ами `handle_sfx`/`MFX_update` читают мои non-interp значения через fc->x/y/z.

Эффект: «снаружи» FC_cam[0].x/y/z всегда мои (collision-aware) значения. Старая система продолжает работать в своих параллельных полях — её логика не сломается потому что она читает свои.

**Вариант B — перетирание fc->x/y/z поверх FC_process:**
- Старая система FC_process пишет в fc->x/y/z.
- Моя логика читает fc->x/y/z, считает свою позицию, **перетирает** fc->x/y/z своими значениями.
- Никаких изменений в render-interp / RenderInterpFrame не нужно.
- Старые промежуточные значения теряются (никому не нужны).

Эффект тот же что и Вариант A снаружи. Но **формально нарушает** правило «отдельные переменные».

Я бы предложил **Вариант A** (формально соответствует тому что ты сказал), но он требует менять `render_interp.cpp` (модификация существующего render-interp кода). Вариант B проще, но нарушает буквальное «отдельные переменные».

### План работы по Этапу 4 (после ack)

1. Решить Вариант A или B (после твоего ответа).
2. Создать каркас новой системы (имена полей / функции / файл — fc_collision.cpp или внутри fc.cpp отдельным блоком). Только подменяет fc->x/y/z своими значениями, идентичными результату FC_process. Билд + тест — поведение не меняется.
3. Только потом начинаем строить collision algorithm (ты опишешь как должна работать).

### Текущее состояние

- Этап 1: ✅
- Этап 2: ✅
- Этап 2bis: ✅
- Bugfix вертикального стика: ✅
- Bugfix edge-triggered camera keys: ✅
- Этап 3 (pipeline analysis): ✅ — анализ выше. Точка интеграции найдена, render-time перезаписей в gameplay нет.
- Этап 4 (новая система): не начат, ждём ack по Варианту A/B.

---

## Запись 9 — решение по архитектуре, план шага 1 (скаффолд)

### Решение пользователя

**Вариант A в строгом виде. Никаких перетираний fc->x/y/z (или других полей старой системы).**

Ключевое уточнение пользователя:
> «там может быть накапливаемый эффект когда прошлая итерация зависит от результатов предыдущей»

Это важный нюанс который я не сформулировал явно в Варианте A: FC_process **использует свои fc->x/y/z как accumulator между tick'ами** (smoothing `actual += (want - actual) >> 2` читает actual из прошлого тика). Если перетереть fc->x/y/z своими значениями — следующий тик FC_process получит мои значения как стартовую точку smoothing'а, и его накапливаемая траектория сломается.

**Следствие:** fc->x/y/z/yaw/pitch/roll/lens — это **read-only для моей системы**, и **read-only для всех read'еров на render-вне-substitution** (handle_sfx, MFX_CAMERA voices, etc) тоже должны постепенно мигрировать на мои поля — но не в первом шаге.

### Архитектура (финал)

**Новые поля** — добавляю в `FC_Cam` (или nested struct внутри неё). Рабочее имя — `fc->vis.*` («visible / output camera», то что реально видно игроку):

```cpp
struct FC_Cam {
    // ... existing fields (не трогаем) ...
    struct {
        SLONG x, y, z;
        SLONG yaw, pitch, roll;
        SLONG lens;
    } vis;  // computed by FC_collision_process, snapshotted/lerped by render-interp
};
```

(Имя `vis` обсуждаемо. Альтернативы: `out`, `final`, `coll`, отдельный массив `g_vis_cam[FC_MAX_CAMS]`. Я склоняюсь к nested struct внутри FC_Cam для удобства передачи.)

**Pipeline новой системы:**

1. **Physics tick:**
   - FC_process отрабатывает как раньше → пишет в fc->x/y/z и т.д. Никаких изменений в fc.cpp кроме добавления полей в struct.
   - **`FC_collision_process()`** (новая функция, чистая, без побочных эффектов на FC_process state) читает fc->x/y/z и focus + другие данные, считает collision-aware значения, пишет в fc->vis.x/y/z и т.д.
   - На шаге 1 (скаффолд) — просто копирует fc->x/y/z → fc->vis.x/y/z. Алгоритм коллизии — отдельным шагом потом.
   - `render_interp_capture_camera` модифицируется: снэпшотит fc->vis.* (не fc->x/y/z).

2. **Render frame:**
   - `RenderInterpFrame` ctor — substitutes interpolated моих snap'ов в **fc->x/y/z** (там где их читают render-time consumers).
   - **`RenderInterpFrame` dtor — НЕ restores к старым FC_process значениям.** Restore к **моим non-interp** (fc->vis.x/y/z). Это «правильное» состояние «снаружи» — мои значения.
   - **ВАЖНО — это всё ещё не нарушает накапливание FC_process**, потому что FC_process читает **только fc->x/y/z**, и **на следующем physics tick'е** перед FC_process'ом старые значения нужны.
   - Подожди. Перечитываю свой план — это **противоречие**. dtor restores к моим = fc->x/y/z = mine. Следующий physics tick FC_process читает fc->x/y/z = mine = НЕ его accumulator. Сломал.

### Re-thinking

Чтобы НЕ перетирать FC_process accumulator, fc->x/y/z **должны** оставаться значениями FC_process даже после render. То есть:
- В RenderInterpFrame ctor: **save** старые fc->x/y/z в temp, **substitute** fc->x/y/z = lerp(my_prev, my_curr).
- В RenderInterpFrame dtor: **restore** fc->x/y/z = saved (старые FC_process значения).

Это **ровно как сейчас работает RenderInterpFrame** — только snap'ы теперь мои.

После dtor: fc->x/y/z = FC_process значения (нетронуты, accumulator работает).
Между ctor и dtor (внутри draw_screen): fc->x/y/z = lerp(my) — render видит мои.

**Что с handle_sfx, MFX и др.?** Они вне substitution (после dtor) → читают FC_process значения, не мои. Это **расхождение** между визуальной камерой (мои) и audio/HUD-yaw (FC_process).

Для шага 1 (скаффолд) — расхождения нет (мои = копия FC_process). Тестируется идентичное поведение.

Для будущих шагов (с реальной collision logic) — расхождение появится. **Решение** — на тот момент менять отдельных читателей чтобы читали fc->vis.* вместо fc->x/y/z. Это правка существующего кода, но это правка **читателей-адаптеров** (audio/HUD), не FC_process логики. Уточню с пользователем в момент когда понадобится.

### План шага 1 — скаффолд (без collision algorithm)

Цель — построить инфраструктуру, в которой моя система **присутствует** и работает в pipeline, но **функционально идентична** старой (потому что копирует значения). Билд + игровой тест должны показать **неизменное** поведение камеры.

Конкретные изменения:

1. **`new_game/src/camera/fc.h`** — добавить nested struct `vis` в `FC_Cam`:
   ```cpp
   struct {
       SLONG x, y, z, yaw, pitch, roll, lens;
   } vis;
   ```

2. **`new_game/src/camera/fc.cpp`** — добавить функцию `FC_collision_process()`:
   ```cpp
   void FC_collision_process(void) {
       for (SLONG cam = 0; cam < FC_MAX_CAMS; cam++) {
           FC_Cam* fc = &FC_cam[cam];
           if (fc->focus == NULL) continue;
           // Scaffold: identity copy. Collision logic will go here.
           fc->vis.x = fc->x;
           fc->vis.y = fc->y;
           fc->vis.z = fc->z;
           fc->vis.yaw = fc->yaw;
           fc->vis.pitch = fc->pitch;
           fc->vis.roll = fc->roll;
           fc->vis.lens = fc->lens;
       }
   }
   ```

3. **`new_game/src/camera/fc.h`** — задекларировать `FC_collision_process`.

4. **`new_game/src/game/game.cpp`** — вызвать `FC_collision_process()` сразу после `FC_process()` (строка 1037-1038).

5. **`new_game/src/engine/graphics/render_interp.cpp`** — модифицировать:
   - `render_interp_capture_camera(fc)`: снэпшотить fc->vis.x/y/z/yaw/pitch/roll/lens вместо fc->x/y/z/...
   - `RenderInterpFrame` ctor: substitutes lerp(prev_vis, curr_vis) в fc->x/y/z/yaw/pitch/roll/lens (как сейчас, но из моих snap'ов).
   - `RenderInterpFrame` dtor: restore fc->x/y/z/etc к saved-original (как сейчас, не меняется).
   - `render_interp_mark_camera_teleport(fc)`: использует мои snap'ы.

6. **Билд + тест.** Поведение должно быть **визуально идентично** до и после. Если есть расхождение — баг в скаффолде, не в алгоритме.

### Что НЕ делается в шаге 1

- Никакого реального collision algorithm.
- Не трогаем handle_sfx / MFX / HUD compass / другие читатели FC_cam[0].x/y/z. Расхождения нет.
- Не трогаем fc->shake / accumulator поля FC_process.

### Scope ограничение от пользователя

**Камера новой системы — только gameplay-режим (где управляется правым стиком, включая driving / in-car).**

Вне scope:
- EWAY cutscenes (in-game cinematics) — EWAY camera ходит своим pipeline через `EWAY_grab_camera` + `render_interp_apply_eway_camera`. Не трогаем.
- PLAYCUTS cutscenes (videos / story scenes) — пишет в FC_cam через `AENG_set_camera`. Не трогаем.
- Outro — отдельная подсистема. Не трогаем.
- Frontend / menus — камера не активна.

**Следствие для FC_collision_process:** когда `EWAY_grab_camera` возвращает true (cutscene активна) — collision logic skip'ается (или сводится к identity copy). В скаффолде (шаг 1) — копируем безусловно, потому что cutscene path всё равно перетрёт fc->x/y/z через AENG_set_camera/EWAY_grab_camera уже после моей системы → значения моего vis в этом случае «безвредны», просто не используются. Cutscene gate можно добавить во втором шаге когда появится реальная логика и важно её **не выполнять** в cutscene.

### Текущее состояние

- Этап 1–3: ✅
- Этап 4, шаг 1 (скаффолд): план описан, жду ack пользователя.
- Этап 4, шаг 2+ (реальная collision logic): после ack по шагу 1 и подтверждения что скаффолд работает.

---

## Запись 10 — инфраструктура vis_cam готова + test toggle

### Файлы

**Новые:**
- [`new_game/src/camera/vis_cam.h`](../../new_game/src/camera/vis_cam.h) — API: `VC_State` struct, `VC_state[FC_MAX_CAMS]`, `VC_test_offset_enabled`, `VC_process()`, `VC_toggle_test_offset()`.
- [`new_game/src/camera/vis_cam.cpp`](../../new_game/src/camera/vis_cam.cpp) — реализация: identity copy `FC_cam[*]` → `VC_state[*]` + опциональный Y offset.

**Изменения в существующих:**
- [`render_interp.cpp`](../../new_game/src/engine/graphics/render_interp.cpp) — `render_interp_capture_camera` теперь читает из `VC_state[idx]` (где `idx = fc - &FC_cam[0]`) вместо `fc->*`. RenderInterpFrame ctor/dtor не тронуты — substitution цели остаются `fc->x/y/z` и restore теми же saved values.
- [`game.cpp`](../../new_game/src/game/game.cpp) — `#include "camera/vis_cam.h"` + вызов `VC_process()` сразу после `FC_process()` ([строка 1037 + 5](../../new_game/src/game/game.cpp#L1037)).
- [`input_actions.cpp`](../../new_game/src/game/input_actions.cpp) — `#include "camera/vis_cam.h"` + обработка `KB_BACKSLASH` (`\\`) через `press_pending + consume` → `VC_toggle_test_offset()`. Безусловная, без debug-key gate (тест временный).
- [`CMakeLists.txt`](../../new_game/CMakeLists.txt) — добавлен `src/camera/vis_cam.cpp`.

**НЕ тронуто:** `fc.h`, `fc.cpp`, `fc_globals.*`, `FC_Cam` struct, RenderInterpFrame ctor/dtor, handle_sfx, panel, mfx, bloom — никаких расхождений в скаффолде, читатели вне substitution path продолжают видеть значения FC_process.

### Что произошло в pipeline

```
─── Physics tick ───
FC_process()         // legacy, без изменений; пишет в FC_cam[*]
VC_process()         // НОВОЕ: читает FC_cam[*], пишет в VC_state[*]
                     //  - identity copy (+ test Y offset если toggle on)
GAME_TURN++
...
render_interp_capture_camera(&FC_cam[0])
                     // ИЗМЕНЕНО: снэпшотит VC_state[0], не FC_cam[0]

─── Render frame ───
RenderInterpFrame ctor:
  // БЕЗ ИЗМЕНЕНИЙ: substitutes lerp(prev,curr) в fc->x/y/z
  // Источник lerp — теперь мой VC_state snapshot, цели те же fc->*
draw_screen
RenderInterpFrame dtor:
  // БЕЗ ИЗМЕНЕНИЙ: restore fc->* к saved-original (FC_process values)
```

### Test toggle

**Клавиша:** `\` (backslash).
**Поведение:** нажатие → `VC_test_offset_enabled` toggled.
**Эффект когда включён:** `VC_process` добавляет `+0x8000` к `VC_state[*].y` → визуальная камера лифтится на ~0.5m вверх в мировых координатах.
**Плавность:** благодаря render-interp lerp моих VC_state значений.

**Значение offset 0x8000** — оценка «полметра» по grep'ам world unit масштабов. Точное соответствие — на глаз пользователя. Если визуально слишком много/мало — корректируем числом.

### Что протестить

1. Запустить миссию (или загрузить save).
2. Камера должна вести себя как раньше — `VC_test_offset_enabled = false` при старте, identity copy в VC_state → render видит то же что и при отсутствии моей системы.
3. Нажать `\` — камера плавно но быстро поднимается на ~0.5m вверх в мировых координатах. Все механики (вращение, get-behind, право-стик) продолжают работать поверх — просто всё сдвинуто наверх.
4. Нажать `\` ещё раз — плавно возвращается на исходную высоту.
5. Поведение должно работать **независимо от FPS** (тот же фикс press_pending+consume что для F5/F6/F7).

### Текущее состояние

- Этап 1–3: ✅
- Этап 4 шаг 1 (скаффолд + тест): ✅ инфраструктура сделана, билд OK, ждём результат игрового теста.
- Этап 4 шаг 2+ (реальная collision logic): после успешного теста скаффолда и описания алгоритма пользователем.

---

## Запись 11 — 2026-05-14 — поиск focus point, refactor processing layer, удаление test toggle, договорённости

### Поиск точки фокуса для рейкаста

Цель — определить откуда пускать рейкаст для будущего collision. Несколько итераций:

1. Сначала отрисовал маркер `FOCUS` через `AENG_world_text` на `fc->focus_x/y/z` (без lookabove). Пользователь: «это тупо root дарси, не фокус».
2. Добавил `+ fc->lookabove` (как делает `FC_look_at_focus` для want_yaw/pitch). Пользователь: «больше на голову похоже. отклоняется при беге вбок от центра экрана».
3. Попробовал гадание `lookabove * 2` — пользователь жёстко остановил («не занимайся хуйней, я просил НАЙТИ существующую, не подгонять»).
4. Подтвердил пользователю: готовой переменной «точка в мире, всегда в центре кадра» в коде **нет и быть не может архитектурно** — это направление `(yaw, pitch)`, не точка. То что есть — `focus + lookabove` это **want-target** для look-at; реальная камера к нему стремится со smoothing-лагом, поэтому при движении точка визуально съезжает от центра экрана. Это by design.
5. Пользователь принял: используем `focus_x, focus_y + lookabove, focus_z` как точку фокуса. **Маркер FOCUS остался на этой формуле в коде.**

### Refactor processing layer

Пользователь попросил: завести `VC_FocusPoint focus` локальной переменной в `VC_process`, вынести логику обработки в отдельную функцию принимающую этот focus как параметр. Цель — если позже сменится источник focus point, signature функции остаётся стабильным, только инициализация focus меняется.

Реализовано в [`vis_cam.cpp`](../../new_game/src/camera/vis_cam.cpp):
```cpp
struct VC_FocusPoint { SLONG x, y, z; };

static void vc_process_one(FC_Cam* fc, VC_State& vc, const VC_FocusPoint& focus)
{
    (void)fc; (void)vc; (void)focus;
    // Example: vc.y += 0x8000;
}

void VC_process(void) {
    for cam in cams:
        // identity copy fc->* → vc.*
        VC_FocusPoint focus = { fc->focus_x, fc->focus_y + fc->lookabove, fc->focus_z };
        vc_process_one(fc, vc, focus);
}
```

`(void)fc; (void)vc; (void)focus;` — подавление `-Wunused-parameter` пока тело пустое. Уйдут когда тело начнёт реально использовать параметры.

### Удалён test toggle

Пользователь: «убираем VC_test_offset_enabled полностью, кнопку тоже». Поскольку pipeline уже подтверждённо работает (Запись 6 — пользователь тестил toggle и убедился что камера лифтится), мы убрали:
- `VC_test_offset_enabled` глобал из `vis_cam.cpp/h`.
- `VC_toggle_test_offset()` функцию.
- `VC_TEST_OFFSET_Y` константу.
- Обработку `KB_BACKSLASH` из [`input_actions.cpp`](../../new_game/src/game/input_actions.cpp) (и `#include "camera/vis_cam.h"` оттуда — больше не нужен).
- Пример процессинга остался как комментарий внутри тела `vc_process_one`: `// vc.y += 0x8000;`

Также `VC_debug_draw_focus()` убрана из публичного API ([`vis_cam.h`](../../new_game/src/camera/vis_cam.h)) — в [`aeng.cpp`](../../new_game/src/engine/graphics/pipeline/aeng.cpp) сейчас локальный forward declaration. Это правильно — функция чисто debug, в публичный header светить нечего.

### Договорённости по алгоритму коллизии

Пользователь описал основной алгоритм + дал ответы на мои открытые вопросы + добавил два edge case'а (камера пролетает focus, Дарси внутри стенки). Всё свёрнуто в раздел **«Договорённости по алгоритму коллизии»** в начале файла — это контракт для следующей сессии (Шаг 2).

### Подготовка к коммиту

В этой сессии готово к коммиту: инфраструктура vis_cam + debug маркеры (FOCUS в мире, "center" на экране). Логика коллизии **не реализована**, тело `vc_process_one` пустое. Следующий агент возьмётся за реализацию алгоритма по договорённостям.

Также записан в `known_issues_and_bugs.md` регрессированный баг про combat tutorial hints (нашёл/не нашёл прошлый фикс — не нашёл, описал заново с предложенным решением).

### Текущее состояние

- Подготовительная работа (удаление старых систем, bugfix'ы): ✅
- Инфраструктура vis_cam + render-interp подключение: ✅
- Точка фокуса определена и проброшена через `VC_FocusPoint`: ✅
- Refactor processing layer (стабильный signature): ✅
- Договорённости по алгоритму зафиксированы: ✅
- **Следующий шаг — реализация алгоритма в `vc_process_one`** (по договорённостям).
