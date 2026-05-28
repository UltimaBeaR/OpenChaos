# Новая раскладка клавиатуры и мыши

Полностью новая клавиатурно-мышиная схема управления, разработана с нуля
поверх готового геймпад-управления как референса. Цель — минимум различий
от геймпада в игровых ощущениях, при этом современные FPS-конвенции
(WASD + мышь, спринт на Shift, ADS на правой/средней кнопке мыши).

Документ ведётся **до и во время** реализации. После завершения — фиксируется
финальная схема (записывается также в `GAMEPLAY_CHANGES.md`).

## Финальная раскладка

### На ноге

| Действие | Геймпад | Клавиатура / Мышь |
|---|---|---|
| Движение (аналоговое) | Left stick | **W A S D** |
| Look (камера yaw/pitch) | Right stick | **Mouse motion** |
| Прыжок | ✕ (Cross) | **Space** |
| Удар рукой (punch) | R2 (analog trigger) | **Левая кнопка мыши** |
| Удар ногой (kick) | R1 | **Правая кнопка мыши** |
| Действие / Спринт-hold | ◯ (Circle) | **Left Shift** |
| Прицел от 1-го лица (aim hold) | L1 | **Средняя кнопка мыши** |
| Тактический шаг + перекаты (L2) | L2 (analog trigger) | **Left Ctrl** |
| Переключение оружия — циклический перебор того что есть | R3 | **Tab** |
| Переключение оружия — быстрый выбор конкретного (холстер / пистолет / дробовик / AK47 / граната / взрывчатка / нож / бита) | DPad (4 направления, подмножество) | **1 2 3 4 5 6 7 8** |
| Пауза | Options/Start | Esc |
| Консоль разработчика | — | F9 |
| Геймпад-чит (immortal / health / weapons / ammo) | Select+L1+L2+DPad | F9-команды (см. ниже) |

### В машине

| Действие | Геймпад | Клавиатура |
|---|---|---|
| Газ (accel) | R2 (digital threshold) | **W** |
| Тормоз (brake) | L2 (digital threshold) | **Space** |
| Руль влево/вправо | Левый стик (горизонталь) | **A / D** |
| Сирена / мигалка (только полицейская машина) | △ (Triangle) | **E** |
| Действие — выйти из машины | ◯ (Circle) | **Left Shift** |

**Газ/тормоз — binary везде.** Физика машины (`vehicle.cpp`) читает только
бит `VEH_ACCEL` / `VEH_DECEL` из `DControl`. Аналоговая величина триггера
L2/R2 в `apply_button_input_car` не используется — есть только порог
digital_held. Поэтому на геймпаде и на клаве модуляции «прижать педаль
на 30%» нет в принципе.

### Aim mode (прицеливание от 1-го лица)

В режиме aim (зажата средняя кнопка мыши) и WASD, и движение мыши **оба**
управляют направлением прицела. Аналогия двух стиков на геймпаде: левый
стик и правый стик в aim-режиме оба смотрят прицелом.

### Tanky controls (legacy)

Старое управление со стрелок (↑↓←→ = вперёд/назад/танковый поворот) **выпиливается
из main path, но обработка остаётся в коде закомментированной** с пометкой
почему. На случай если в будущем захочется добавить тумблер в настройки
для поклонников оригинала.

## Команды чита в F9-консоли

Геймпад-чит на клаве реализуется через консоль (F9). Команды одиночные,
в стиле `bangunsnotgames` — слитная фраза без пробелов.

| Эффект | Геймпад | Команда | Откуда |
|---|---|---|---|
| Бессмертие (toggle) | Select+L1+L2+Up | `bloodofkings` | Highlander — отсылка к сообщению чита |
| Полное здоровье (1000 HP) | Select+L1+L2+Down | `shieldofsteel` | Highlander — отсылка к сообщению чита |
| Спавн оружия (AK+SG+P+3 granate) | Select+L1+L2+Left | `weneedguns` | The Matrix — отсылка к сообщению чита |
| Макс. патроны (240 всех типов) | Select+L1+L2+Right | `losttrack` | Dirty Harry — отсылка к сообщению чита |

Все команды — фрагмент из сообщения которое чит показывает на экране
при срабатывании (см. `GAMEPLAY_CHANGES.md` → «Геймпад-читы»). Симметрия
«увидел → ввёл».

## Что удаляется

Эти клавиши **полностью удаляются** из main path:

| Клавиша | Что делала | Почему убираем |
|---|---|---|
| F5 | `INPUT_MASKM_CAM1 = 0` — no-op | мёртвая кнопка |
| F6 | rotate camera left | дубль Del + мышь крутит |
| F7 | rotate camera right | дубль PgDn + мышь крутит |
| End | snap camera behind | не нужен — автокамера сама доворачивается |
| Delete | rotate camera left | мышь крутит |
| PageDown | rotate camera right | мышь крутит |
| A (hold) | прицел от 1-го лица | A теперь = стрейф |
| ↑↓←→ | tanky-движение + look в aim | заменяется на WASD + мышь |
| Z | punch (foot) / accel (car) | заменяется на ЛКМ / W |
| X | kick (foot) / brake (car) | заменяется на ПКМ / Space |
| C | action (foot) | заменяется на Shift |
| Enter | колесо инвентаря | заменяется на Tab |

Соответствующие `ACT_FOOT_CAM_*_KKEY`, `ACT_FOOT_MOVE_*_KKEY`,
`ACT_FOOT_PUNCH_KKEY`, etc. **удаляются** (не переименовываются).
Tanky-обработка стрелок — комментируется в `apply_button_input` с TODO.

## Что добавляется

Функциональность, которая есть на геймпаде, но раньше отсутствовала на клаве:

1. **Аналоговое движение через WASD** — каждая клавиша при удержании
   эмулирует полное отклонение левого стика в одну сторону. Диагонали
   (две клавиши вместе) клампятся до единичного вектора — как настоящий
   стик не выходит за единичный круг, так и W+D даёт магнитуду 1 на
   45°, не sqrt(2). Точная реализация — в `get_hardware_input`
   (`INPUT_TYPE_KEY` ветка). Downstream-код уже умеет работать со
   стик-аналогом.

2. **L2 тактический режим на Ctrl** — Ctrl зажат → медленный шаг,
   первое отклонение WASD после нажатия Ctrl фиксирует режим (как
   стик на геймпаде). Перекаты и сальто — Space в окне 600 мс после
   нажатия Ctrl, направление — текущие WASD.

3. **Спринт на Shift** — Shift зажат + движение через WASD →
   `INPUT_MASK_ACTION` непрерывно (как Circle hold на геймпаде).

4. **Punch/kick на мыши** — ЛКМ и ПКМ читаются через `input_mouse_btn_*`
   API (нужно добавить, сейчас mouse-buttons только подсасываются для
   capture). LMB → `INPUT_MASK_PUNCH`, RMB → `INPUT_MASK_KICK`.
   LMB маппится в trigger-эквивалент R2 для weapon-feel (digital,
   но достаточно).

5. **Aim hold на MMB** — средняя кнопка мыши, hold → `INPUT_MASK_CAMERA`
   + first-person-aim state. WASD + mouse motion в этом режиме оба
   управляют yaw/pitch прицела.

6. **Чит через F9-консоль** — четыре команды (см. таблицу выше).
   Реализуются как обычные dev-console команды, не требуют
   `bangunsnotgames` (это полноправный геймплей-чит, не debug).

## Баги найденные при тестировании (фиксятся в конце задачи)

Собираем здесь, чтобы не разбираться по одному во время основной работы.
Все баги выявлены при ручном тестировании после первого батча шагов 1-3
и батча 4/5/7/8. Чиним после того как остальные TODO в этой задаче
закрыты.

### BUG-1 ✅ FIXED: WASD — перс «летит» во время прыжка после отпускания клавиши
Было: при прыжке + отпускании W в полёте Дарси продолжала лететь до
приземления, плюс делала ещё пару шагов после landing.
Стало: после серии изменений по камере и input pipeline проблема
ушла. Прыжок останавливается корректно при отпускании клавиши, и
после landing нет лишних шагов. Точный фикс — побочный эффект одной
из правок (вероятно input edge-detect через press_pending + consume
+ упрощение мышиных блоков). Не воспроизводится после серии правок.

### BUG-2: Tab + Shift замораживает Дарси в странной позе
**Воспроизведение:** жмёшь Tab (открывает inventory wheel) — пока wheel
на экране, зажимаешь Shift → Дарси застывает в странной позе. На
закрытии wheel поведение восстанавливается.
**Гипотеза:** Shift = ACTION в новом layout. ACTION в gameplay делает
много разных вещей в зависимости от состояния. Во время открытого
inventory wheel перс уже в специальном state — ACTION на нём ловится
не туда. Возможно нужно гейтить ACTION пока wheel открыт.

### BUG-3 ✅ FIXED: D → "There is room behind Darci"
Было: чтение `ACT_BANG_ROOM_BEHIND_CHECK_KKEY` в `game_tick.cpp:1407`
не было загейтлено через `allow_debug_keys`. D вне debug-режима всё
равно показывал debug-сообщение.

Стало: чтение обёрнуто в `input_debug_modifier_active()` (часть
TODO-1 ремапа). Теперь D-room-behind фолбэк работает только при
**F1+D** в debug-режиме. В обычной игре D = только стрейф вправо.

### BUG-4: лёгкая подкрутка модели после остановки
**Воспроизведение:** разогнался WASD, навёл камеру сбоку, отпустил
WASD. На остановке моделька Дарси чуть-чуть докручивается (видно еле-
еле). Старого поведения на стрелках такого не было.
**Гипотеза:** при digital-WASD packing вектор стика моментально схло-
пывается в центр, но gameplay-сторона ожидает плавное затухание (как
у настоящего стика). Возможно нужно ramping вниз вместо мгновенного
сброса до 32768, или нужно правильнее переводить character-frame.

### BUG-5 ✅ FIXED: ЛКМ/ПКМ удары через раз
**Корень:** архитектурная дыра — гемпадная ветка в `get_hardware_input`
всегда пакует CENTER stick в bits 18-31, даже когда гемпад не активное
устройство. KBM-ветка пакует bits 18-31 ТОЛЬКО когда WASD зажат. Без
зажатых WASD И без гемпада → bits 18-31 = 0 → `GET_JOYY` декодирует
raw zero как **-128** (= полное отклонение стика назад). Это
trigger'ит `process_analogue_movement` → character forces в
`STATE_MOVEING/ACTION_RUN` → `action_run` table маппит PUNCH →
ACTION_SHOOT → без оружия `set_player_shoot` silently не делает ничего
→ удар не проходит.
Связано: при подключенном гемпаде проблемы НЕТ т.к. он давал baseline
CENTER packing.
**Найдено через лог:** `JOYX=-128 JOYY=-128 input=0x00000040 state=5
sub=2 action=2 new_action=23 CASE ACTION_SHOOT has_gun=0` — без
WASD/гемпада character всегда в RUN, PUNCH → SHOOT путь → no-op.
**Фикс:**
1. Гейт всего гемпадного блока в `get_hardware_input` условием
   `active_input_device != KBM` — при KBM-режиме гемпад НЕ читается
   вообще (ни stick, ни кнопки, ни L2, ни cheat combo).
2. KBM-блок else-ветка: когда WASD не зажат И KBM активен — пакую
   CENTER stick в bits 18-31 (`0x81000000` эквивалент), сбрасываю
   `analogue=0`. Это neutral baseline для KBM — `GET_JOYX/Y` возвращает 0.
**Аудит других мест** где гемпад читается без device-гейта — на отдельный
проход (могут быть связанные баги в меню, виджетах и т.д.).

### BUG-6: в машине W + Space не работают одновременно
**Воспроизведение:** в машине зажат W (газ). Жмёшь Space (тормоз) —
не реагирует. Отпускаешь W — тормоз срабатывает.
**Гипотеза:** в `apply_button_input_car` логика "if (input &
ACCELERATE) accel; else if (input & DECELERATE) decel;" — это else-
if. Газ имеет приоритет, тормоз игнорируется пока газ зажат. Та же
проблема и на гемпаде (R2+L2 одновременно — газ выигрывает).
**Подозрение:** возможно проблема шире и проявится в других местах
с двумя зажатыми клавишами. Проверить.

### BUG-8 ✅ FIXED: Shift не позволяет сесть в машину
Было: Shift возле машины — Дарси не садилась.
Стало: после серии правок по input pipeline / камере фикс получился
побочным эффектом. Shift у машины теперь стабильно отрабатывает
посадку. Не воспроизводится.

### BUG-7 ✅ DONE (доработка, не баг): меню-навигация на WASD
**Запрос:** во всех меню (фронтенд, pause, mission-select etc.)
навигация сейчас по стрелкам. Хочется WASD добавить как альтернативу
(стрелки остаются).
**Решение:** добавлены alt-константы `ACT_MENU_NAV_*_ALT_KKEY`
(W/S/A/D) в `act_menu.h`, подключены через OR рядом со стрелками во
всех меню-нав сайтах: `widget.cpp` (FORM_KeyProc up/down),
`gamemenu.cpp` (pause up/down), `frontend.cpp` (held + just_pressed
все 4 направления), `attract.cpp` (wake-up). Стрелки не тронуты.
В Form-виджетах буква сначала уходит в `Char`-обработчик фокуса —
поэтому ввод WASD в текстовое поле печатает символы, а не навигирует
(навигация срабатывает только когда виджет символ не съел).

### BUG-CAM-VERT-LIMIT ✅ FIXED: вертикаль мыши без лимита, глюк в верхней точке
Было: pitch уходил до ~90° (bird's-eye), камера зависала на оси Y
фокуса, дальше только горизонтальное orbiting.
Исправлено в `apply_pitch_y_delta` (`fc.cpp`):
1. Clamp по pitch-углу `[11°, 53°]` (= `KBM_PITCH_MIN/MAX = 64/302` game-units,
   соответствует эффективному диапазону гемпада при default cam_dist).
   Y-offset выводится из угла: `off_y = dist3d * sin(pitch)`.
2. Tighter из двух: pitch-derived и гемпадный `FC_CAM_Y_MIN/MAX_ABOVE_FOCUS`
   — чтобы при малом dist3d (после спринт-шринка XZ) min не уходил ниже
   гемпадного floor.
3. Degenerate recovery: если XZ был 0 (legacy state), ставим камеру за
   спину фокуса через `focus_yaw`.

### BUG-CAM-JUMP-ROT-JERK ✅ FIXED: дёрг камеры при приземлении если крутишь мышью
Было: при mdx/mdy в один тик с landing-induced `focus_y_delta` ловился
дёрг.
Стало: после упрощения мышиных блоков (убраны явные `fc->yaw -= angle`
в mdx и `fc->pitch = Arctan(...)` snap в mdy — они были избыточны,
дублировали работу `FC_look_at_focus` + angle smoothing snap) конфликт
устранён. Теперь один источник истины: mouse-блок двигает только
позиции, далее `FC_look_at_focus` пересчитывает want-углы из новой
геометрии (уже включающей `focus_y_delta`), angle smoothing snap копирует
в `fc->yaw/pitch/roll`. Никаких per-tick конфликтов между мышью и
landing.

### BUG-CAM-HANG-NEAR-CLIP: при висении на стенке near-plane режет стену вокруг персонажа
**Воспроизведение:** Дарси висит на выступе стенки. Камера снапается
на неё (т.к. min_d_hit от пробы стенки < VC_WALL_OFFSET — стенка
вплотную к фокусу). Камера в позиции Дарси. Near-clip plane в её
направлении взгляда режет геометрию стенки рядом, виден задник карты
через дыру.
**Попытки фикса — НЕ СРАБОТАЛИ:**
1. `VC_MIN_FOCUS_DIST` — держать камеру на дистанции 0.5m по
   focus→camera направлению вместо snap. Не сработало: focus→camera
   направление часто идёт ВНУТРЬ стенки (камера натурально хочет быть
   позади Дарси = с обратной от стенки стороны = внутрь). Камера
   оказывалась внутри материала стенки, near-plane резал ещё сильнее
   (всё прозрачное). Дополнительно: при подходе к стенке лицом
   (камера спереди от Дарси) — то же самое, камера в стенке.
2. Flip-opposite (трекать min hit sample, ставить камеру в
   ПРОТИВОПОЛОЖНОМ направлении от стенки) — не сработало: при
   подходе к стенке камера автоматически разворачивалась в другую
   сторону. Нарушает "manual rotation only" правило, юзер не хочет.

**Текущее состояние:** оригинальный snap-to-focus (камера на Дарси,
near-plane режет геометрию вокруг). Меньшее зло из тех что пробовали.

**Что нужно (для правильного фикса):** определить НА КАКОЙ СТОРОНЕ
стенки сейчас свободно (где обычно ходит Дарси) **БЕЗ автоматического
разворота камеры**. Варианты:
- Расширить probe чтобы возвращал wall normal или multiple hit info.
- Использовать FC_calc_focus's STATE_DANGLING handling — там focus_yaw
  специально rotates чтобы камера была сбоку. Может на KBM этот rotate
  применяется не полностью. Проверить.
- Специальный handling для STATE_DANGLING / STATE_CLIMBING etc — может
  программный override камеры (как при посадке в машину) — это явный
  сценический режим где manual не работает.

### BUG-CAM-MMB-AIM-JERK: приближение вида (MMB) дёрганое и со смещением вбок
**Воспроизведение:** на клаве жмёшь среднюю кнопку мыши (aim/zoom-in)
— переход в режим приближения идёт резко, иногда с одиночным дёргом
вбок на 1 кадр перед тем как камера встанет в центр. На геймпаде
(L1-hold для aim) переход плавный.
**Гипотеза:** связано с отключением сглаживания поворотов на KBM. Aim-
mode меняет позицию/угол камеры (over-the-shoulder), и без angle
smoothing переход идёт скачком вместо интерполяции. Один-кадр сбоку —
вероятно промежуточное состояние между mouse mdx (если двигалась мышь
на тике активации) и aim-снапом.
**Возможный фикс:** для aim-mode перехода временно вернуть angle
smoothing на 1-2 тика, либо явно проинтерполировать camera к aim-
state. Это не противоречит "manual rotation" принципу — переход в aim
это явный программный режим.

### BUG-CAM-PLATFORM-AUTO-ROT ✅ FIXED: стоя на едущей машине камера авто-поворачивается (KBM)
**Причина:** двойное применение vehicle motion на want для KBM.
- Platform inheritance (`fc.cpp` ~line 917) делает rigid rotate+translate
  `want_x/z` вокруг pivot машины каждый тик.
- Translation tracking (~line 1346) на KBM делал `want += focus_dx` (full).
- Оба применялись → vehicle motion в want учитывался дважды → want
  over-shoots focus → `FC_look_at_focus` считает другой want_yaw →
  KBM-snap копирует его в `fc->yaw` → видимый auto-rotation.
- На геймпаде бага НЕТ т.к. translation tracking = excess-speed only
  (= 0 для типичных скоростей машины), platform inheritance — единственный
  источник vehicle motion → корректно.
**Фикс:** на KBM в translation tracking добавлен гейт
`if (!fc->platform_thing)` — пропускаем когда стоим на платформе.
Platform inheritance делает работу один раз. Углы стабильны.

### BUG-CAM-CAR-ENTRY-NO-LOCK: при посадке в машину камера не доворачивается
**Воспроизведение:** на клаве подходишь к машине, жмёшь Shift — начи-
нается анимация посадки. Камера в этот момент должна программно
доворачиваться (lock-rotation к ракурсу машины), но НЕ доворачивается.
На геймпаде доворачивается.
**Контекст:** это одно из немногих исключений где камера ДОЛЖНА
программно поворачиваться даже в "manual" режиме. Во время анимации
посадки юзеру полностью убирается контроль камеры (отдельный
сценический режим) — поэтому правило "поворот только руками" не
нарушается.
**Гипотеза:** скорее всего отключённое angle smoothing или get-behind
гейт ловит этот случай тоже. Нужно явно разрешить программную ротацию
для `SubState == SUB_STATE_ENTERING_VEHICLE` (или аналогичного
сценического флага). Возможно блок get-behind на это завязан и его
надо разгейтить для KBM в этом конкретном случае.

### BUG-CAM-VERT-LIMIT-DRIFT ✅ FIXED: на упоре вертикального лимита кручение мышью даёт смещение
**Корень:** `apply_pitch_y_delta` вызывался ДВАЖДЫ в mdy блоке — один
раз для `want`, второй раз для `fc`. Каждый вызов вычислял
`pitch_max_off_y` из СВОЕГО dist3d. Want и fc расходятся со временем
(position smoothing lag, want-only translation tracking, want-only
Y focus tracking), их dist3d разные → разные лимиты pitch. На упоре
want корректно возвращал eff_dy=0, но fc — с большим dist3d — видел
более высокий pitch_max и продолжал ехать вверх по 2-7к fc-units
каждый тик. Position smoothing затем дёргал fc обратно к want, что
выглядело как drift.
**Лог подтвердил:**
```
WANT y_delta=14863 dist3d_sq=21677410445 max=117690 new=117690 eff_dy=0    ← OK
FC   y_delta=14863 dist3d_sq=28250296433 max=134269 new=134269 eff_dy=7123 ← BUG
```
**Фикс (`fc.cpp` mdy mouse block):** хелпер теперь вызывается ТОЛЬКО
для `want`, потом полученный world-space delta (`want_after - want_before`)
зеркалится на `fc.x/y/z` напрямую. Want и fc движутся в lockstep при
mdy (так же как mdx через `ORBIT_PT` для обоих). Другие источники
расхождения want/fc (translation tracking, Y focus tracking, position
smoothing) работают как раньше.

### BUG-JUMP-FROM-IDLE-WRONG-DIR ✅ FIXED: прыжок с места «вперёд+jump» иногда идёт в старое направление перса
**Воспроизведение:** перс стоит на месте. Прокручиваешь камеру в другую
сторону (перс лицом «куда было раньше»). Почти одновременно: «вперёд»
(чуть раньше) + jump. Ожидаемо: перс развернётся к камере и прыгнет
бегом в направлении камеры. Реально — **через раз**:
- Иногда: перс разворачивается к камере, бежит в направлении камеры,
  прыгает на бегу в направлении камеры (правильно).
- Иногда: перс бежит в направлении куда смотрел РАНЬШЕ (не в направлении
  камеры), прыжок на бегу в это старое направление (неправильно).
- Это НЕ "стрелочка не успела": в неправильном случае прыжок именно
  **на бегу**, не с места.

Уже бежал + резкий разворот камеры в другую сторону + jump не отпуская
бега — это работает корректно (перс продолжает бежать в текущую сторону,
прыжок туда же; резкого разворота не делает). Бажит только сценарий
"из стояния".

**Воспроизведение на устройствах:**
- Клавиатура: воспроизводится.
- Геймпад: тоже воспроизводится, **«через раз»** — реже чем на клаве,
  но баг общий, не KBM-специфика.

**Корень:** action tree dispatch в `apply_button_input` бежит ДО
rotation pass (`player_interface_move` → `player_turn_left_right_analogue`)
в том же тике. На тике первого нажатия движения+jump из stand перс
ещё не повернут к камере — `Tweened->Angle` старый. Jump-обработчики
(`set_person_standing_jump_forwards` / `set_person_running_jump`) читают
этот старый Angle → прыжок летит в старое направление.

Кейс race-prone из-за timing: если W и Space прилетели в один тик из
IDLE → ACTION_STANDING_JUMP, не было snap-а; если W прилетел тик-два
раньше Space → state уже MOVEING, action_run → ACTION_RUNNING_JUMP,
также без snap. Также воспроизводилось после landing (DROP_DOWN_LAND
end → set_person_idle → continue_moveing → set_person_running) — counter
не армировался т.к. путь минует `process_analogue_movement`.

**Фикс (3 части):**
1. `ACTION_STANDING_JUMP` FORWARDS branch — добавил snap angle к
   stick+camera direction перед `set_person_standing_jump_forwards`
   (зеркало существующего snap-а в side/back branch).
2. Глобал-counter `g_player_run_entry_ticks` (5 тиков). Армируется
   через `player_arm_run_entry_snap()` внутри `set_person_running`
   (когда prior State != MOVEING для player) — покрывает ВСЕ пути
   входа в MOVEING (process_analogue_movement, set_person_idle-continue,
   dangling drop, etc.) одной точкой. Декремент — в начале
   `apply_button_input`.
3. `ACTION_RUNNING_JUMP` body — если counter > 0, snap angle перед
   `set_person_running_jump`. Тот же formula что в STANDING_JUMP.

Counter-window (5 тиков ≈ 100ms) маленький — преднамеренный
«бежал → flip камеры → jump» (где perse продолжает бежать в свою
сторону, не разворачивается резко) сохраняет существующее поведение
(counter=0 после установившегося бега).

### BUG-CAM-SMALL-ROT-SHIFT: мелкое кручение мыши даёт смещение, а не поворот
**Воспроизведение:** на клаве делаешь очень маленькое движение мышью —
визуально воспринимается не как поворот камеры, а как небольшое
СМЕЩЕНИЕ. На среднем/сильном кручении эффекта нет — выглядит нормальным
поворотом.
**Возможная связь:** может быть та же проблема, что и **jitter ближних
объектов** (вероятно описана где-то в known_issues). Скорее всего
проблема в точности int math на маленьких углах: ORBIT_PT с углом 1-2
game-units даёт SIN/COS близко к 0/1, частное `(rx*c - rz*s) >> 16` при
маленьком s теряет точность → дельта по rx маленькая, по rz микро-
смещение от потерь округления. На больших углах это размывается.
**Где смотреть:** `ORBIT_PT` макрос в mdx-блоке `FC_process` и orbital
helper `apply_pitch_y_delta`.

## Что делаем в конце (после основной раскладки)

Это **часть задачи**, но делается после того как новая клавиатурная
схема готова и проверена в игре. Не отложено в смысле «когда-нибудь
потом» — это последние шаги той же задачи.

### TODO-1 ✅ DONE: ремап `bangunsnotgames` и debug-клавиш

**Решение**: добавлен глобальный **F1-модификатор**. Все хоткеи в
`act_bangunsnotgames.h` теперь требуют удержания F1 (вместе с
`allow_debug_keys`). Реализация — два helper'а в input_frame:

- `input_debug_modifier_active()` = `allow_debug_keys && F1_held` — гейт + F1 alone = легенда (overlay auto-hide на первом F1+key)
  для всех debug-input call sites
- `input_gameplay_enabled()` = `!input_debug_modifier_active()` (с заделом
  на будущие "suppress gameplay" условия) — гейт для gameplay-input
  entry points (`process_controls` инвентарь/weapon block, и т.д.)

**Что было сделано**:
- 25+ `if (allow_debug_keys && input_key_*)` сайтов → `input_debug_modifier_active()`
- gameplay-input блок в `process_controls` обёрнут в `input_gameplay_enabled()`
- CRT остался на **F2** (F1 — модификатор; F1+F2 = CRT toggle)
- Numpad-bangunsnotgames → Home/End кластер + K + R + Delete:
  - Numpad +/-/Enter (msg scroll) → PageUp / PageDown / Home
  - Numpad 7/5/2/3 (pyro/immolate/firepool) → End / K / Delete / R
- Perf-debug (compile-time, не bangunsnotgames):
  - Phys 1/2/3/9/0 → Numpad 1/2/3/9/0 (compile-time dev, numpad ok)
  - Perf panel 4/5 → F6 / F7
- Frontend cheats `ACT_MENU_FE_CHEAT_*` → KKEY_EQUALS / KKEY_8 (off numpad)
- WARE debug T → теперь загейтлено F1-модификатором (раньше leak без gate)
- D = room-behind → теперь загейтлено F1-модификатором (BUG-3 fixed)
- F1 alone = легенда (5с), F1+key = debug-действие (overlay сразу прячется)
- F9 (console) НЕ под F1-модификатором — универсальная клавиша
- F1 legend в debug_help.cpp полностью переписан под новый формат «F1+key → action»

**Drawing/render-only call sites** (типа `if (ControlFlag && allow_debug_keys) draw_debug_thing()`) оставлены с `allow_debug_keys` — это не input, ничего не блокирует.

### TODO-2 ✅ DONE: фикс 1–8 weapon hotkeys

Было: `apply_button_input` использовал `input_key_just_pressed` (single-frame
edge), а вызывался из physics-tick (20 Hz) — не render-frame (60+ Hz).
Ребро могло теряться между тиками.

Стало: переведено на `input_key_press_pending` + `input_key_consume`.
Sticky pending переживает gap между тиками. Consume происходит up-front
для всех 8 клавиш сразу — press «потрачен» на тике когда прочитан
(не очередится при блокировке), что сохраняет старую just_pressed
семантику.

### TODO-CAMERA-MODE ✅ DONE: два режима камеры

**Реализация** (`new_game/src/camera/camera_mode.{h,cpp}` + рефактор `fc.cpp`/`vis_cam.cpp`):

- Enum `CameraMode { CAMERA_MODE_AUTO, CAMERA_MODE_MANUAL }`.
- Compile-time дефолты по device:
  `KBM_DEFAULT_CAMERA_MODE = CAMERA_MODE_MANUAL`,
  `GAMEPAD_DEFAULT_CAMERA_MODE = CAMERA_MODE_AUTO`.
- `get_active_camera_mode()` — единственный источник истины (читает
  `active_input_device` + дефолты).
- `CAMERA_RUBBERNESS_DEFAULT = 0.5f` — текущее «классическое» ощущение
  (rotation 25%/tick, wall 60/40). 0.0 = snap, 1.0 = очень лениво.
- `keep_for_rubberness(default_keep)` — piecewise-linear lerp: r=0.5
  возвращает default_keep (bit-near-identical к шиппинг-поведению),
  r=0 → 0 (snap), r=1 → very lazy.

**Заменённые гейты** (8 мест по аудиту):
- A) Rotation smoothing (`fc.cpp`) → mode + rubberness.
- B1) Wall sustained-collision blend (`vis_cam.cpp`) → mode + rubberness.
- B2) Wall collision entry — fc-snap vs raw_vc (`vis_cam.cpp`) → mode.
- B3) Wall exit smoothing (`vis_cam.cpp`) → mode + rubberness × fade.
- C) Get-behind / fight-behind / idle-return (`fc.cpp`) → mode.
- D) Translation tracking 1:1 vs excess-speed (`fc.cpp`) → mode.
- E) Distance min-clamp (`fc.cpp`) → mode.
- H) Y-convergence vs delta-tracking (`fc.cpp`) → mode + per-tick
  manual_y_input flag (mouse mdy и stick Y оба сигнализируют).

**Переименования**: `KBM_PITCH_MIN/MAX` → `MOUSE_PITCH_MIN/MAX` (это
mouse vertical input math, не зависит от mode). Удалены
`VC_SMOOTH_ALPHA_NUM/DEN_KBM` (заменены формулой через rubberness).

**Оба режима поддерживаются для обоих устройств**: меняешь
`KBM_DEFAULT_CAMERA_MODE = CAMERA_MODE_AUTO` — мышь работает в авто-
резиновом режиме (включая get-behind, idle Y-convergence и т.д.).
Меняешь `GAMEPAD_DEFAULT_CAMERA_MODE = CAMERA_MODE_MANUAL` — стик
становится snappy без get-behind.

**Out of scope** (намеренно не заменено): `fc.cpp:1216` — гейт чтения
гамепад-стика; это input handling, а не camera behavior.

**Не зависит от mode**: position smoothing X/Z >>2, Y >>3 — осталось
неизменным (камера всё равно плавно следует за персом в обоих режимах).

После: следующий шаг — добавить runtime control rubberness (UI/console),
тогда юзер сможет тюнить в живую.

### TODO-CAMERA-MODE-LEGACY: исходный план (для истории)

После того как закроем оставшиеся камера-баги (вертикальный лимит,
дёрг при приземлении + поворот мышью одновременно), вводим явный
camera-mode как настройку:

**1) Автоматический режим** (geympad default):
- Повороты камеры сглаживаются (классические 25%/тик).
- Get-behind при движении (камера сама подстраивается за спину).
- Get-behind при неактивности (после паузы возвращается за спину).
- В fight mode get-behind триггерится при смене врага + периодически.
- Wall smoothing активен (60/40 VC_SMOOTH_ALPHA).
- Position smoothing активен (>>2 X/Z, >>3 Y).
- В целом — текущая гемпад-логика.

**2) Ручной режим** (KBM default — это текущее состояние после
правок rotation snap):
- Все повороты камеры ТОЛЬКО явным вводом (mouse motion / стик).
- Сглаживание ПОВОРОТОВ полностью отключено (упрощение).
- Get-behind отключён (нет авто-подстройки за спину).
- Get-behind по неактивности отключён.
- Fight-mode behind-triggers отключены.
- Wall smoothing отключён (keep=0).
- Position smoothing ОСТАЁТСЯ (плавное следование за персонажем).
- Translation tracking — 1:1 (камера жёстко за персонажем).
- Distance MAX clamp активен (sprint recovery).
- Distance MIN clamp отключён (не отменяет орбитальный pitch).

**Переключение режимов:**
- Дефолт по device: gamepad → automatic, mouse → manual.
- Compile-time override через константу в коде (тип
  `CAMERA_MODE_OVERRIDE` или флаг на FC_Cam).
- Позже — в конфиг / меню settings.

**Реализация:**
- Все текущие `if (active_input_device == INPUT_DEVICE_KEYBOARD_MOUSE)`
  гейты заменить на `if (camera_mode == MANUAL)`.
- Добавить runtime-функцию `fc_get_camera_mode()` или подобную.
- Дефолт-функция: `if (gamepad_connected && !override) return AUTOMATIC;
                  if (!gamepad_connected && !override) return MANUAL;
                  return override_value;`

### TODO-3: Tactical-mode (Ctrl) интеграция с WASD-as-stick

**Что готово:**
- Константа `ACT_FOOT_TACTICAL_MODE_KKEY = KKEY_LEFT_CONTROL` объявлена
  в `act_foot.h`.
- WASD пакуется в bits 18-31 input mask как реальный стик (TODO-2 не
  related, имеется в виду шаг 7 раскладки).

**Чего нет:**
- L2 state-machine (engagement hysteresis, regime classifier
  FORWARD/BACKWARD, side-rolls по Cross в окне 600 мс, m_bForceWalk,
  player_relative) живёт ИНУТРИ `if (input_gamepad_connected())` ветки
  в `input_actions.cpp::get_hardware_input` (~3804–3884). При отсутствии
  геймпада весь блок скипается. Для клавы нужно вынести логику наружу,
  читать engagement signal как `max(L2_trigger_raw, KKEY_LCTRL_held ? 255 : 0)`,
  направление — из bits 18-31 input mask (там уже WASD-пакет лежит после
  TYPE_KEY ветки).
- Размер рефактора ~80–100 строк. Нужно аккуратно — state-machine
  имеет много transient'ов (l2_engaged, l2_regime, окно 600 мс) которые
  должны работать одинаково для гемпада и клавы.

**Приоритет:** последний крупный недоделанный шаг основной задачи.
После него — финальный review + GAMEPLAY_CHANGES.md update.

### TODO-4: GAMEPLAY_CHANGES.md — обновить под финальную F1-схему

`GAMEPLAY_CHANGES.md` секции «Управление» / «Геймпад-читы» / «Чит-команды
через консоль F9» уже обновлены, но обновлялись ещё под старую схему
(до F1-модификатора bangunsnotgames). Финальный проход:
- F1+key для всех debug-биндингов (хотя bangunsnotgames в GAMEPLAY_CHANGES
  скорее всего не упоминается — это dev-only)
- Сверить таблицу клавиатуры + машины (на ноге + в машине)
- Сверить cheat-команды
- Записать ChangeMode если что-то изменилось

## Шаги реализации (порядок)

Делаем итеративно, каждый шаг — отдельный коммит-кандидат.

1. **Удалить камера-клавиши** (F5/F6/F7/End/Del/PgDn) — самый безопасный
   шаг, ничего не ломает. Удалить `ACT_FOOT_CAM_MODE_*_KKEY`,
   `ACT_FOOT_CAM_BEHIND_KKEY`, `ACT_FOOT_CAM_LEFT_KKEY`,
   `ACT_FOOT_CAM_RIGHT_KKEY` и их обработку в `input_actions.cpp`.

2. **Переключить движение со стрелок на WASD** — переименовать
   `ACT_FOOT_MOVE_*_KKEY` биндинги на `KKEY_W/A/S/D`. Закомментировать
   tanky-обработку стрелок в `apply_button_input`.

3. **Перенести punch/kick/action/jump/inventory** — обновить
   `ACT_FOOT_PUNCH_KKEY/KICK_KKEY/ACTION_KKEY/INVENTORY_KKEY` и
   `ACT_FOOT_AIM_KKEY`. Удалить старые Z/X/C/Enter/A биндинги.

4. **Добавить mouse-button API** в `input_frame` — `input_mouse_btn_held`,
   `input_mouse_btn_press_pending`, `input_mouse_btn_consume`. Подключить
   SDL3 mouse button events в `host.cpp`.

5. **Punch/kick на мыши + Aim на MMB** — подключить LMB/RMB/MMB к
   соответствующим путям в `get_hardware_input` и aim state.

6. **Спринт-Shift и Tactical-Ctrl** — добавить keyboard-эквиваленты
   L2-engagement и Circle-hold-sprint в `get_hardware_input`. Логика
   та же что для геймпада (`l2_regime` стейт-машина и т.д.).

7. **WASD как аналоговое движение** — в `INPUT_TYPE_KEY` ветке
   `get_hardware_input` эмулировать stick deflection. Точнее — вместо
   digital-движения через `INPUT_MASK_FORWARDS` и т.п. сразу подавать
   в analog-path.

8. **Car controls — W=газ, Space=тормоз, A/D=руль, E=сирена** — обновить
   `ACT_CAR_*` биндинги, обработка остаётся та же.

9. **Чит-команды в F9-консоли** — четыре команды, реализованные через
   стандартный console-command-handler.

10. **Финальный self-review + GAMEPLAY_CHANGES.md update** — записать
    в `GAMEPLAY_CHANGES.md` новую раскладку (заменяет/дополняет
    существующие записи про геймпад).

## Открытые вопросы

Нет — все ключевые решения по раскладке зафиксированы выше. Aim-mode
look на WASD — взять ту же математику что уже есть на стрелках для
тангового look в первом плане; экспериментально подкрутить если будет
ощущаться не так.
