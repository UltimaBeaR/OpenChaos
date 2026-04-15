# Система виброотдачи и adaptive trigger для оружия (2026-04-15)

Живой документ про то как сейчас устроена вибрация при стрельбе и поведение
adaptive trigger на DualSense. Смежный документ про почему "true" audio-haptic
через Sony API не вышел: [`dualsense_audio_haptic_investigation.md`](dualsense_audio_haptic_investigation.md).

## Что реализовано

### Вибрация при стрельбе (envelope-based)

Вместо "true" Sony audio-haptic — считаем амплитудную **огибающую** из реального
WAV-сэмпла оружия один раз при старте, и драйвим ею slow-мотор DualSense
синхронно с выстрелами.

Модуль: [`new_game/src/engine/input/gamepad_haptic.cpp`](../new_game/src/engine/input/gamepad_haptic.cpp).

- При `gamepad_haptic_init()` загружается WAV для каждого оружия через
  `sdl3_load_wav` (отдельно от MFX, чтобы не трогать звуковой движок). Считается
  RMS-огибающая с окном 5ms, усекается до per-weapon `max_seconds`, масштабируется
  на per-weapon `gain`, клампится на per-weapon `ceiling`. Сохраняется как
  `std::vector<uint8_t>` — единицы сотен байт, околонулевой расход памяти.
- `gamepad_haptic_weapon_fire(weapon_type)` вызывается из
  [`actually_fire_gun()`](../new_game/src/things/characters/person.cpp) на
  каждый выстрел игрока. Гейт: `INPUT_DEVICE_DUALSENSE` + `PlayerID`. NPC-шные
  выстрелы игнорируются.
- `gamepad_haptic_tick()` вызывается из `gamepad_rumble_tick` раз в кадр.
  Берёт `MAX` по диапазону `[prev_index, curr_index]` — чтобы не
  alias'ить пики при коарсном тике (33мс) поверх 5мс разрешения огибающей.
- Результат max-merge'ится со старым PS1-style shock мотором
  (`gamepad_set_shock` для урона/тряски) — не затирает, а складывается.

### Per-weapon параметры

Таблица в `gamepad_haptic_init()`:

| Оружие | `gain` | `ceiling` | `max_seconds` | Ощущение |
|--------|--------|-----------|---------------|----------|
| **AK47** | 0.4 | 90 | 40ms | короткий резкий "bah", ритмично на авто-огне |
| **Пистолет** | 0.05 | 18 | 35ms | лёгкий "поп", слабее AK47 |
| **Дробовик** | 0.55 | 140 | 90ms | тяжёлый "бам" с долгим спадом |

`max_seconds` обязательно короче cooldown'а между выстрелами — иначе огибающие
перекрываются и превращаются в сплошной buzz без текстуры.

### Adaptive trigger (Weapon25 mode)

Реализация в [`gamepad.cpp`](../new_game/src/engine/input/gamepad.cpp) через
`TRIGGER_MODE_AIM_GUN` → `ds_trigger_weapon(7, 2, 0, 0, 1)`.

- Сейчас **один статичный режим щелчка для всех оружий** — `StartZone=7`,
  `Amplitude=2` (значения 4-битные, 0..15; предыдущие 20 и 180 овершоотили и
  на DualSense приходили как 4/4 из-за overflow в
  [`Weapon25`](../new_game/libs/dualsense-multiplatform/Dualsense-Multiplatform/Source/Public/GImplementations/Utils/GamepadTrigger.h)).
- Режим активен только когда `weapon_ready == true`, считается в
  [`game.cpp`](../new_game/src/game/game.cpp) из: `has_gun` && `!on_cooldown`
  (Timer1 == 0) && не в "non-firing" state (jump, fall, climb, dying, hit, fight
  и т.п.) && не целишься в сдающегося / невиновного копа.
- **R2 position gate**: `AIM_GUN` можно (пере-)активировать только если текущее
  положение R2 ≤ `AIM_GUN_ENTRY_MAX_R2` (80). Иначе при переходе из `NONE` в
  `AIM_GUN` на почти-нажатом триггере DualSense выдаёт "фантомный щелчок" без
  соответствующего выстрела.
- `gamepad_triggers_lockout()` вызывается из `gamepad_haptic_weapon_fire` на
  single-shot оружиях — мгновенно переключает режим в `NONE` на кадре выстрела,
  чтобы не было гонки между Timer1 и HID-латентностью переключения режима.

### Fire logic (input_actions.cpp)

- `fire_threshold = 200` (~78% хода R2) — совпадает с точкой где Weapon25
  щёлкает, чтобы выстрел и щелчок происходили на одном и том же движении
  триггера.
- `reset_threshold = 80` — игра переармирует rising-edge детектор когда
  R2 опускается до этого порога. **Этот же флаг armed используется
  adaptive-trigger state machine'ом** через `weapon_feel_is_r2_armed()` —
  единый источник истины для fire detection и триггерного эффекта.

### Unified armed state (2026-04-15)

Между fire detection (`weapon_feel_evaluate_fire`) и adaptive trigger
state machine в `gamepad_triggers_update` используется **один флаг
`s_r2_armed`** в модуле weapon_feel. Поведение:

- `armed=true` (после deep release ≤ reset_threshold): mode=AIM_GUN,
  триггер с сопротивлением и щелчком, следующий press выстрелит.
- `armed=false` (после выстрела, пока не было deep release): mode=NONE,
  триггер **полностью свободный** — нет сопротивления, нет щелчка.
  Следующий press **не выстрелит** (rising-edge детектор заблокирован).

Эффект: быстрое полу-отпускание + polу-нажатие не даёт никакой реакции
вообще — триггер скользит без ощущения. Чтобы сделать следующий выстрел
нужно отпустить триггер глубоко, до зоны переармирования. Это убирает
прежний баг "shot without click on rapid shallow tap": теперь если
выстрел не может сработать по игровой логике, то и триггерный эффект
отключён, так что в принципе не может быть ситуации "выстрел есть,
щелчка нет" или наоборот.

Для auto-fire оружий (AK47) `weapon_feel_is_r2_armed()` возвращает true
всегда — у них нет rising-edge гейта, эффект триггера держится
непрерывно пока оружие готово.
- **Auto-fire детект**: если текущее оружие = `SPECIAL_AK47`, используется
  held-down поведение (фишка бесконечно пока держишь R2). Иначе — rising-edge
  (каждый выстрел требует отпустить R2 ≤ `RESET_THRESHOLD`).
- **L2 (kick)** — тоже rising-edge с теми же порогами для консистентности.

## Что сейчас работает хорошо

- Per-weapon вибрация с текстурой (не сплошной buzz), юзер-тюнед для пистолета
  / AK47 / дробовика.
- Щелчок триггера синхронизирован с моментом реального выстрела (раньше
  стрелял до щелчка).
- Cooldown между выстрелами не даёт фальшивых щелчков при rapid fire.
- Non-firing states (прыжок, падение, лазание, драка) — щелчка нет.
- Dry fire (пустой магазин) — щелчок работает, как у реального пистолета.
- Auto-fire у AK47 работает как и должен (held-down).

## Известные баги

### Rapid release-press → выстрел без щелчка (редкий)

Сценарий: полностью нажал R2 (выстрел с щелчком) → очень быстро и коротко
отпустил R2 (не до 0, а куда-то в район 60-80) → сразу нажал обратно.
Примерно в 1-2 случаях за 2 обоймы получается что игра делает выстрел но
щелчка на триггере не происходит, триггер идёт полностью свободным ходом.

**Вероятная причина:** релиз-пулл цикл короче одного кадра (~16мс). R2
семплируется один раз за кадр, момент "низкого значения" либо пропускается
полностью, либо оказывается выше `AIM_GUN_ENTRY_MAX_R2` (но ниже старого
`RESET_THRESHOLD=130`, что раньше давало обратный баг "щелчок без выстрела").

**Возможные решения (ещё не пробовано):**

1. **Hysteresis** на уровне gamepad.cpp — запоминать минимум R2 между
   вызовами `gamepad_triggers_update`, использовать его для can_enter-гейта.
   Тогда короткий dip ниже 80 между фреймами будет пойман.
2. **Wall-clock lockout после выстрела** длительностью хотя бы 1 HID-RTT
   (~30мс) — мёртвая зона в которую физически не может попасть rapid
   release-press.
3. **Поллить R2 чаще** чем раз в кадр (через side-channel poll внутри
   ds_bridge).

Записано в [`known_issues_and_bugs.md`](../new_game_planning/known_issues_and_bugs.md)
как "DualSense adaptive trigger: 'быстрый release + быстрый re-press'".

## TODO — планы на развитие

### Per-weapon adaptive trigger профили

Сейчас все оружия используют один статичный `Weapon25(7, 2)`. Хочется:

- **AK47** — эмуляция автоматической отдачи: не статичный щелчок а ритмичное
  "отдача-возврат-отдача" на каждый выстрел в очереди. Возможно через
  `MachineGun26` режим или кастомную последовательность HID выходных
  репортов.
- **Пистолет** — чёткий острый щелчок с коротким возвратом (текущее плюс-минус
  устраивает).
- **Дробовик** — тяжёлый медленный импульс с большим сопротивлением и
  ленивым возвратом (под стать физической отдаче дробовика).
- **Снайпер** (если когда-нибудь будет) — "точка" — очень жёсткое сопротивление
  до щелчка, мгновенный спуск.

### Xbox вибрация при стрельбе

Сейчас envelope-based рамбл работает только на DualSense. Xbox контроллеры
получают только существующую PS1-style shock вибрацию (урон, тряска камеры), но
не на стрельбу.

Нужно добавить аналогичную вибрацию для Xbox. Скорее всего **проще** чем для
DualSense: ERM-моторы Xbox низкочастотные, не могут следовать детальной
амплитудной огибающей как voice-coil'ы DualSense. Достаточно обобщённого
шаблона "короткий импульс на выстрел" с per-weapon scaling. Без всяких RMS
огибающих и 5ms разрешения — одно значение на фрейм выстрела + decay.

### ✅ Модуль weapon_feel (сделано 2026-04-15)

Вся система инкапсулирована в единый модуль
[`new_game/src/engine/input/weapon_feel.{h,cpp}`](../new_game/src/engine/input/weapon_feel.h).
Раньше логика per-weapon поведения была размазана по 4 файлам со связкой
через две константы, которые должны были сидеть в синхроне (`RESET_THRESHOLD`
в input_actions.cpp vs `AIM_GUN_ENTRY_MAX_R2` в gamepad.cpp).

**Что внутри модуля:**

- `WeaponFeelProfile` — единая структура на оружие: haptic envelope params
  (wave_id/gain/ceiling/max_seconds) + adaptive trigger params
  (start_zone/amplitude) + fire model (fire_threshold/reset_threshold/auto_fire).
- Реестр `SPECIAL_*` → profile + precomputed envelope (`std::unordered_map`).
  Дефолтный профиль для неизвестных оружий: нет haptic, нет adaptive click,
  стандартные пороги 200/80 rising-edge.
- `weapon_feel_init()` регистрирует встроенные профили (AK47, пистолет,
  дробовик) и строит envelopes из WAV через `sdl3_load_wav`. Envelope-код
  (RMS window 5ms, gain+ceiling clamp, max-merge scan) переехал сюда из
  gamepad_haptic.cpp без изменений.
- `weapon_feel_register(special_type, profile)` — регистрация/переопределение
  профиля в рантайме (для кастомных оружий). Если init уже прошёл, envelope
  строится сразу.
- `weapon_feel_on_shot_fired(special_type)` — вызывается из
  [`actually_fire_gun()`](../new_game/src/things/characters/person.cpp).
  Запускает envelope playback + дёргает trigger lockout для single-shot.
- `weapon_feel_tick_haptic(&active)` — per-frame, возвращает значение для
  slow-мотора. Вызывается из `gamepad_rumble_tick`.
- `weapon_feel_evaluate_fire(weapon, r2, l2)` — rising-edge/held-down детект
  по профилю текущего оружия. Возвращает `{shoot, kick}`. Вызывается из
  [`input_actions.cpp`](../new_game/src/game/input_actions.cpp) вместо
  прежнего блока с constexpr порогами.
- `weapon_feel_get_profile(weapon)` — лукап, используется в
  [`gamepad.cpp`](../new_game/src/engine/input/gamepad.cpp)
  `gamepad_triggers_update` для получения Weapon25 params и entry_max_r2
  для текущего оружия игрока (сигнатура теперь принимает `current_weapon`).

**Что снаружи модуля осталось:**

- `weapon_ready` computation в game.cpp (список non-firing states — это про
  состояние игрока, не про оружие).
- Реальный вызов `actually_fire_gun()` и PS1-style shock-мотор — max-merge
  с выходом модуля в `gamepad_rumble_tick`.
- Trigger state machine в gamepad.cpp (NONE/AIM_GUN/CAR) — CAR-режим не
  относится к оружию, так что полноценный перенос был бы оверскоупом.
  Машина состояний теперь просто спрашивает weapon_feel что отправить.

**Добавление кастомного оружия после 1.0:**

```cpp
WeaponFeelProfile my_laser = { S_LASER_SHOT, 0.3f, 120, 0.06f,
                               10, 3, 200, 80, false };
weapon_feel_register(SPECIAL_CUSTOM_LASER, my_laser);
```

Одна строка — и вибрация, adaptive trigger click, fire detection начинают
работать для нового оружия. Никаких правок в gamepad.cpp / input_actions.cpp /
person.cpp / game.cpp.

**Что константа `RESET_THRESHOLD`/`AIM_GUN_ENTRY_MAX_R2` больше не fragile:**
обе подсистемы (fire detection + trigger state machine) читают один и тот же
`profile->reset_threshold`. Сломать синхрон по забывчивости теперь
невозможно.
