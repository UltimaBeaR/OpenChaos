# Система виброотдачи и adaptive trigger для оружия (2026-04-15)

Живой документ про то как сейчас устроена вибрация при стрельбе и поведение
adaptive trigger на DualSense. Смежный документ про почему "true" audio-haptic
через Sony API не вышел: [`dualsense_audio_haptic_investigation.md`](dualsense_audio_haptic_investigation.md).

---

## 🔧 2026-05-16 — Граната: фидбек фазы 1 (pistol-like) + фаза 2 (лёгкий триггер)

Реализован фидбек, ранее помеченный как «будущая работа» в записи про
фикс детонации.

**Спека (пользователь).** Граната — отдельное поведение, не melee и не
огнестрел:
- **Фаза 1** (граната в руке, ещё не примирована, `SubState !=
  ACTIVATED`) — взвод фитиля должен ощущаться как клик пистолета:
  DualSense = Weapon25-клик, Xbox = короткий rumble (симуляция клика).
  Переиспользована пистолетная реализация.
- **Фаза 2** (примирована, ждёт броска) — без эффектов, просто лёгкий
  триггер (как пистолет, но без клика).
- Детекция в обеих фазах — пистолетная (лёгкий аналог 200/80), **не**
  melee-клик в полу хода.

**Реализация (4 точки):**
1. `weapon_feel.cpp` — `weapon_feel_register(SPECIAL_GRENADE, pistol)`:
   граната получает пистолетный профиль (Weapon25 + xbox rumble + haptic).
2. `weapon_feel_evaluate_fire` — граната исключена из melee-ветки
   (`current_weapon != SPECIAL_GRENADE`), уходит в firearm-threshold путь;
   `weapon_drawn==false` → act-path не берётся → чистый порог 200/80
   (pistol-like, без melee-клика). Обе фазы детектятся одинаково.
3. `game.cpp` — `grenade_phase1` (граната в руке && `SubState !=
   ACTIVATED`) добавлен в `weapon_ready`. Фаза 1 → AIM_GUN/Weapon25
   включён (DS клик). Фаза 2 → `weapon_ready=false` → NONE (клика нет,
   лёгкий триггер). Эффект-параметры берутся из gren профиля (pistol).
4. `input_actions.cpp` — на взводе (`SPECIAL_prime_grenade`, player only)
   вызывается `weapon_feel_on_shot_fired(SPECIAL_GRENADE, 0)`: проигрывает
   пистолетный rumble/haptic-всплеск. Xbox: это и есть «короткий rumble =
   клик». DS: добавка к уже активному непрерывному Weapon25.

Огнестрел / удары / нож / бита / C4 / мина — не затронуты (граната
дискриминируется явно по `SPECIAL_GRENADE` + фазе).

---

## 🔧 2026-05-16 — Граната: фикс детонации в руке (port-баг)

**Баг.** Примированная граната в руке игрока не взрывалась по истечении
6-сек фитиля (в релизной версии — взрывается). Причина — расхождение
порта в `special.cpp` (carried cook-off): оригинал
([`original Special.cpp:1160-1205`]) **всегда** вызывает
`CreateGrenadeExplosion`, и лишь потом разбирает инвентарь
(`ammo==1` → drop+free, иначе `SubState=NONE; ammo--`). Наш порт
взрывал **только при `ammo==1`**, а при `ammo>1` молча сбрасывал
`SubState` без взрыва. У игрока гранат обычно несколько
(`SPECIAL_AMMO_IN_A_GRENADE = 3`) → `ammo>1` → не взрывалось.

**Фикс.** Переписано 1:1 по оригиналу: взрыв всегда (в точке
`WorldPos.Y + 2256` — высота руки, uc_orig), `SpecialUse=NULL` всегда,
затем ветка инвентаря; условие `timer < ticks` (строгое, как в
оригинале). Файл: `special.cpp`.

(Двухфазность гранаты — клик 1 = взвод+таймер, клик 2 = бросок —
подтверждена KB, это оригинал, не трогалось. Фидбек 1-й фазы
DualSense-клик / Xbox-вибро — отдельная будущая работа.)

---

## 🔧 2026-05-16 — Melee (не-огнестрел): отдельный аналоговый порог

**Проблема.** Удар рукой/ножом/битой/гранатой с триггера R2 шёл через
аналоговый порог-фоллбэк пистолетного профиля (`SPECIAL_NONE`
зарегистрирован на pistol): `fire = 200`, `reset = 80` из 255. Огромная
полоса гистерезиса (продавить >~78%, отпустить <~31% чтобы перевзвести).
Для пушек так задумано (слабый зажим не стреляет), для рукопашки
неудобно — частые тычки теряются.

**Промежуточный заход (отменён).** Сначала melee пробовали посадить на
цифровой бит триггера (`input_btn_held(16)`). По ощущению не то — откатили
к аналогу. (Цифровой бит не имел отношения к багу гранаты — тот
оказался отдельным port-багом детонации, см. запись выше.)

**Финальное решение (правило пользователя).** Огнестрел = пистолет /
дробовик / AK47 → старый аналог/adaptive-click (без изменений). Всё
остальное → **отдельный аналоговый порог, без эффектов DualSense**:
- срабатывание у самого конца хода `MELEE_FIRE_THRESHOLD_R2 = 225` (~88%)
- короткий перевзвод `MELEE_RESET_THRESHOLD_R2 = 200`
- т.е. игрок вжимает курок в пол (клац), чуть отпускает, снова вжимает —
  быстрый дробный melee. Одинаково на DualSense и Xbox (оба аналог
  0..255).

**Реализация.** Дискриминатор — существующий `weapon_drawn` (ставится
только для трёх огнестрелов). В `weapon_feel_evaluate_fire` ветка под
`!weapon_drawn` использует `MELEE_FIRE/RESET_THRESHOLD_R2`; огнестрельный
путь обёрнут в `else` и **не тронут**. Сигнатуру не меняли (цифровой
параметр из промежуточного захода убран). L2-канал не трогался.
Пороги — именованные константы, тюнятся по ощущению.

Файлы: `weapon_feel.h`, `weapon_feel.cpp`, `input_actions.cpp`.

---

## 🔧 2026-04-19 — AK47 reload click + release-gate между перезарядкой и следующим выстрелом

Финальная полировка AK47: закрыты три связанных бага про перезарядку.
Все три — одним пакетом, т.к. логически неразделимы.

### Баг 1 — Machine эффект оставался активным когда магазин пустел

Раньше `weapon_ready` для auto-fire оружий был `has_gun && !non_firing_state`
без учёта того что Timer1 > 0 (идёт реальная стрельба). Игрок держал R2,
магазин кончался, игра прекращала выстрелы, но Machine-пульс на триггере
продолжал дрожать. Фикс — новая функция
`weapon_feel_trigger_effect_should_run(weapon, in_shot_cycle, mag_empty)`:
политика по типу оружия живёт внутри weapon_feel. Auto-fire: эффект ON
только если идёт shot cycle (Timer1 > 0 от реального выстрела) ИЛИ
магазин пуст с сконфигурированным reload click-ом. Single-shot: ON
только когда НЕ в shot cycle (как раньше). game.cpp передаёт готовые
примитивы (`on_cooldown` как in_shot_cycle, `ammo==0` как mag_empty),
weapon_feel не знает про Thing*/State.

### Баг 2 — перезарядка не требовала отпускания R2 (авто-стрельба из нового магазина)

Стоя (SUB_STATE_AIM_GUN handler) и на бегу (fn_person_moveing) имели
асимметричную логику. Бегом: `if (Timer1)` гейт → после HAD_TO_CHANGE_CLIP
(Timer1=0) ничего не делает → игроку нужно реально отпустить и нажать.
Стоя: AIM_GUN handler каждый тик звал `continue_firing`, без Timer1
гейта → после HAD_TO_CHANGE_CLIP в том же тике видел ammo=30 + PUNCH
зажат → `set_person_shoot` → выстрел. Итог: стоя первое нажатие
перезаряжало И стреляло одновременно. Лог показал: `set_person_shoot
HAD_TO_CHANGE_CLIP → ammo=30 → тут же второй set_person_shoot ammo=29`
в одном и том же тике T771.

Фикс — единый reload gate в `input_actions.cpp`:

```cpp
static bool s_ak47_reload_gate = false;
void input_actions_mark_ak47_reload_gate()  { s_ak47_reload_gate = true;  }
void input_actions_clear_ak47_reload_gate() { s_ak47_reload_gate = false; }
bool input_actions_ak47_reload_gate_set()   { return s_ak47_reload_gate;  }
```

Флаг выставляется в `set_person_shoot` / `set_person_running_shoot` на
`HAD_TO_CHANGE_CLIP`. Проверяется в `continue_firing` — если флаг стоит
и PUNCH зажат, возвращает UC_FALSE (блокирует auto-re-fire через
SUB_STATE_AIM_GUN и SUB_STATE_SHOOT_GUN). Очищается в двух местах:
(a) в person.cpp после `actually_fire_gun` (реальный выстрел прошёл);
(b) в `weapon_feel_evaluate_fire` когда `r2 ≤ reset_threshold` (игрок
физически отпустил курок).

**Критично:** очистка НЕ по падению PUNCH bit в `continue_firing`. В
reload-press режиме (act-bit path) PUNCH ставится ТОЛЬКО на тике
rising-edge. На следующий тик при зажатом триггере PUNCH=0 → очистка
по PUNCH сняла бы гейт сразу после reload-нажатия, и auto-fire
запускался бы следом. Нужна очистка по физическому состоянию курка,
не по edge-триггерному биту.

### Баг 3 — не было hardware щелчка на триггере при перезарядке

Хотели чтобы reload-нажатие ощущалось как Weapon25 клик (как у
пистолета), без вибрации. Проблема: для AK47 эффект всегда был Machine,
а Weapon25 в профиль не было. Добавлены поля `reload_click_start_zone`,
`reload_click_end_zone`, `reload_click_strength` в `WeaponFeelProfile`.
Для AK47 — те же параметры что у пистолета (start=4, end=6, strength=5).
Для пистолета/шотгана — 0 (пистолет уже имеет главный Weapon25
эффект).

Новый параметр `mag_empty` пошёл сквозь всю цепочку:
- `game.cpp` считает: `mag_empty = (ammo==0) OR reload_gate_set`.
  Расширение через gate критично: иначе на тике HAD_TO_CHANGE_CLIP
  ammo уже 30, mag_empty=0, эффект снимается ДО того как hardware
  отработал клик.
- `weapon_feel_trigger_effect_should_run` возвращает TRUE для
  auto-fire при mag_empty даже если in_shot_cycle=FALSE.
- `gamepad_triggers_update(..., mag_empty)` при `mag_empty && reload_click_strength!=0`
  переопределяет effect на Weapon25 с reload-click параметрами вместо
  Machine.

### Синхронизация клика с моментом перезарядки (PUNCH в правильном r2)

После включения Weapon25 эффекта клик всё равно приходил не в тот
момент что перезарядка. Причина: для AK47 `fire_threshold = зона 4
(r2=112)`, а клик hardware фаерится на зоне 6 (r2=168). Курок идёт
вверх, при r2=112 игра ловит PUNCH → HAD_TO_CHANGE_CLIP → ammo=30 →
эффект снимается до того как курок доходит до r2=168.

Решение: для auto-fire в `mag_empty`-состоянии переключаемся на
act-bit путь детекции (как у пистолета). Новый параметр `mag_empty`
добавлен в `weapon_feel_evaluate_fire`. Когда оно true + auto_fire +
reload_click_strength>0, эффективные параметры меняются на
single-shot Weapon25: `eff_effect=Weapon`, `eff_auto=false`,
`eff_end_zone=reload_click_end_zone`. `use_act_path` становится true,
PUNCH фаерится на `reload_click_end_zone * R2_UNITS_PER_ZONE = 168`
— синхронно с hardware кликом.

### Сводка кода

**Новые API:**
- `weapon_feel_trigger_effect_should_run(weapon, in_shot_cycle, mag_empty)` — решает включён ли эффект.
- `WeaponFeelProfile::reload_click_{start_zone,end_zone,strength}` — параметры клика на перезарядке.
- `gamepad_triggers_update(..., mag_empty)` — параметр для overrride на Weapon25.
- `weapon_feel_evaluate_fire(..., mag_empty)` — параметр для reload-press mode.
- `input_actions_mark_ak47_reload_gate / clear / set` — трио для гейта.

**Файлы:** `new_game/src/engine/input/weapon_feel.{h,cpp}`,
`new_game/src/engine/input/gamepad.{h,cpp}`, `new_game/src/game/game.cpp`,
`new_game/src/game/input_actions.{h,cpp}`,
`new_game/src/things/characters/person.cpp`.

### Поведение после всех фиксов

1. Игрок держит R2 с AK47, стреляет. Machine-пульс идёт. ammo↓.
2. Последний выстрел, ammo=0. Timer1 set, Machine держится пока
   cooldown идёт.
3. Timer1=0, нового выстрела нет (ammo=0). in_shot_cycle=false, но
   mag_empty=true → эффект переключается на Weapon25 клик
   (параметры как у пистолета).
4. Игрок отпускает R2 (r2→0) — клик не фаерится (Weapon25 клик только
   на crossing up).
5. Игрок зажимает R2. Курок пересекает зону 6 (r2=168) — hardware
   фаерит клик. На том же r2=168 fire detection ловит PUNCH (act-bit
   path) → set_person_shoot → HAD_TO_CHANGE_CLIP → ammo=30, reload gate
   установлен. DRY звук играет.
6. gamepad_triggers_update: reload_gate_set → mag_empty остаётся true
   → Weapon25 держится активным пока игрок не отпустит R2.
7. Игрок отпустил R2 (r2 ниже reset_threshold=80) → evaluate_fire
   сбрасывает reload gate → mag_empty=false → эффект снимается.
8. Игрок зажимает R2. mag_empty=false → обычный threshold path с
   auto_fire=true → PUNCH фаерится в зоне 4 → set_person_shoot → ammo>0
   → actually_fire_gun → реальный выстрел. Machine эффект включается.
9. Дальше обычная auto-fire стрельба.

---

## 🔧 2026-04-19 — AK47 Machine effect, Xbox rumble, унификация rate стоя/бегом

Три связанные правки поверх финального состояния пистолета ниже.

**1. AK47 adaptive trigger — Machine effect (0x27).** Раньше AK47 имел
`trigger_strength=0` и триггер оставался свободным. Теперь для AK47
`trigger_effect = Machine` с зонами `[PISTOL_CLICK_START=4, 9]` — пульс
начинается в той же точке что Weapon25 клик пистолета и тянется **до
последней зоны** (включая полное вдавливание триггера). Amp_a=7, amp_b=3,
frequency=8, period=80 — взято из input-debug Machine тестера (известно
что на этих значениях эффект уверенно ощущается на железе). Более
слабые значения (ampA=5, ampB=2, period=4) были не ощутимы.
`fire_threshold` выведен из `trigger_start_zone * R2_UNITS_PER_ZONE`
(= 112), так что game начинает стрелять в ту же точку где включается
эффект. Fire detection — штатный threshold path (auto_fire=true,
стреляет пока R2 выше порога).

**Побочный фикс в `game.cpp`:** `FLAG_PERSON_GUN_OUT` это **pistol-only**
флаг — для AK47/shotgun он очищается в `set_person_draw_item`, оружие
хранится только в `Person->SpecialUse`. Изначально `has_gun` в
`gamepad_triggers_update`-call-site читал только флаг → для AK47
`weapon_ready=false` → mode=NONE → триггер молчал. Фикс: `has_gun =
has_pistol_out || (SpecialUse in {AK47, SHOTGUN})`. Аналогично в
`input_actions.cpp` для `weapon_drawn` параметра `evaluate_fire`.

**2. Xbox (и любой SDL-gamepad) rumble при стрельбе.** Раньше envelope
запускался только на DualSense из-за early-return в
`weapon_feel_on_shot_fired` на `device != DUALSENSE`. Убрал гейт —
envelope теперь гоняется device-agnostic, а `gamepad_rumble_tick` уже
max-merges `haptic_slow` в low-freq моторе через `sdl3_gamepad_rumble`.
Никакого отдельного Xbox-пайплайна не понадобилось. Добавлено
per-профильное поле `haptic_xbox_boost` (умножитель, применяется только
на non-DS устройствах) — для пистолета 4.0x (DS-калиброванный ceiling=18
на Xbox почти не ощущается без boost'а), AK47/shotgun оставлены на 1.0
(их амплитуды достаточны). DS путь envelope не меняется.

**3. Унификация fire-rate AK47 стоя и бегом.** До 2026-04-19 AK47 стрелял
на бегу в ~2× быстрее чем стоя (101мс vs 204мс). Причина: оригинальный
MuckyFoot дизайн использовал Timer1 в двух ролях одновременно — в
`SUB_STATE_SHOOT_GUN` как cooldown-декремент и в `SUB_STATE_AIM_GUN` как
accumulator (`Timer1 += TICK_TOCK`, fire при `> 100`). Стоя цикл проходил
обе фазы (декремент 2 тика + accumulator 2 тика = 4 тика), бег — только
декремент (2 тика). Баг в симметрии, rate разный.

**Финальный дизайн (после обсуждения с юзером):**

- **Одна унифицированная точка** установки cooldown — `weapon_feel_consume_shot_cooldown_timer1`. Для auto-fire weapons (AK47) значение масштабируется × `AUTO_FIRE_COOLDOWN_SCALE = 2` — компенсирует убранный из state machine AIM_GUN accumulator. Результат: Timer1 = 60 (anim-derived 30 × 2), декремент ~10/тик, цикл = 6 тиков ≈ 204мс. Пистолет не auto-fire, scale=1, его цикл не трогаем.
- **Одна унифицированная точка** декремента Timer1 и ре-файра — in-place в SHOOT_GUN (player path) и в `fn_person_moveing` (running). Оба делают одно и то же: декрементируют Timer1 одинаковым шагом, при достижении 0 — либо `set_person_running_shoot` (running), либо `set_person_shoot` direct call (standing AK47 с зажатым R2), либо `set_person_aim` (отпустили R2). Итоговый rate одинаков на обоих путях: ~204мс.
- **AIM_GUN AK47 accumulator удалён** для игрока. В AIM_GUN для AK47 при зажатом R2 — сразу `set_person_shoot`, никаких накоплений.
- **Анимационный «колбасится»-эффект сохранён через profile-driven feature.** Оригинальный визуальный эффект возникал из чередования анимаций SHOOT ↔ AIM при пинг-понге state machine (`set_person_aim` при выходе из SHOOT_GUN ставил `ANIM_SHOTGUN_AIM`). Поскольку унифицированный декремент держит persona в SHOOT_GUN весь цикл, анимация зависала на последнем кадре SHOOT. Фикс: добавлено поле `WeaponFeelProfile::aim_interlude_anim` (0 = выключено). В SHOOT_GUN handler при `end==1` (завершение анимации выстрела) handler сам свапает анимацию на `aim_interlude_anim` если профиль её указал. Для AK47 стоит `ANIM_SHOTGUN_AIM` — та же что ставил оригинальный `set_person_aim`. Плюс в том же месте снимаются `FLAG_PERSON_NON_INT_M | FLAG_PERSON_NON_INT_C` (persona становится управляемой между выстрелами — можно начать бежать, повернуться, отменить).
- Код: никаких per-weapon бранчей типа «if AK47 do X». Weapon специфика вся в профиле: `auto_fire` флаг → scale в weapon_feel; `aim_interlude_anim` → swap в handler. Механизм один, работает для любого оружия которое захочет такое поведение.

**Профиль расширен (итог).** `WeaponFeelProfile` теперь содержит:
- `TriggerEffectType trigger_effect` (None / Weapon / Machine) + Machine-поля (`machine_amp_a/b`, `machine_frequency`, `machine_period`).
- `float haptic_xbox_boost` — множитель envelope'а на non-DS устройствах.
- `int32_t aim_interlude_anim` — вторичная анимация для пост-выстрельной «aim pose» фазы.
- (существующие) `haptic_*`, `trigger_start_zone/end_zone/strength`, `fire_threshold/reset_threshold`, `auto_fire`.

Пистолет использует Weapon effect (unchanged), AK47 — Machine + interlude, shotgun — None.
`gamepad.cpp apply_trigger_mode` dispatch'ит по `trigger_effect` в
AIM_GUN режиме; params_changed-гейт перевыставляет эффект при смене
оружия без прохода через NONE.

**Известные open-issues на AK47 feel (для следующих сессий):** эффект
не пропадает при 0 патронов; нет Weapon25-щелчка при смене магазина;
асимметричный баг перехода стоя→бегом с зажатым R2 (только в одну
сторону, обратный переход ок). Детали — в
[`known_issues_and_bugs.md`](../new_game_planning/known_issues_and_bugs.md)
секция «Управление».

---

## ✅ ФИНАЛЬНОЕ СОСТОЯНИЕ (2026-04-18 evening) — единый таймер из анимации

Эта секция описывает **рабочее финальное состояние**. Всё что ниже
(HANDOFF 2, HANDOFF, расследования) — исторический контекст, как мы
к этому пришли. Код сверять только по этой секции.

### Что это делает

Система щелчка adaptive trigger и темпа стрельбы для игрока. Работает
одинаково стоя и на бегу, на клавиатуре и на контроллере. Таймер кулдауна
выводится из длительности анимации выстрела — никаких per-weapon магических
чисел в коде.

### Ключевые принципы

**1. Один таймер для всего.** `Person::Timer1` — единый counter кулдауна
для игрока. Значение считается один раз из данных анимации при первом
обращении и кэшируется. Для пистолета ≈ 120 Timer1 units (эквивалент 140
в оригинале). Для AK47 / shotgun аналогично из соответствующих анимаций.

**2. Стоя и на бегу — одна логика.**
- Стоя: `set_person_shoot` ставит Timer1 в calculated value.
  `SUB_STATE_SHOOT_GUN` для игрока каждый тик декрементирует Timer1
  (как `fn_person_moveing` делает на бегу) и выходит в aim при Timer1=0.
- На бегу: `set_person_running_shoot` ставит Timer1, `fn_person_moveing`
  декрементирует.
- Вход в оба: строго `Timer1 == 0`. Симметрично.

**3. Мотор DualSense синхронизирован с игровым gate'ом.**
- `on_cooldown = Timer1 > pre_release` в game.cpp.
- Pre-release = 0 тиков — мотор включается ровно когда игра готова
  принять выстрел.
- Никаких окон где мотор включён а игра ещё в кулдауне (никаких "кликов
  без выстрела").

**4. Клавиатура работает без изменений.** INPUT_MASK_PUNCH обрабатывается
игрой одинаково независимо от источника. Gate стоит в `set_person_*_shoot`
и `SUB_STATE_SHOOT_GUN` — не в weapon_feel. Темп стрельбы с клавы и с
контроллера идентичен (ограничен одним и тем же Timer1).

**5. NPC не трогаем.** Все изменения гейчены на `PlayerID`. NPC продолжают
использовать хардкоды Timer1=140/64/400 (важно для burst-стрельбы
AK47/MIB которая использует Timer1 как счётчик очереди).

### Почему именно так

**Почему Timer1 а не свой счётчик:** Timer1 уже декрементируется в
`fn_person_moveing`, уже используется как кулдаун running shot'а в
оригинале. Использовать его же для стоя даёт симметрию бесплатно и
минимально меняет структуру.

**Почему анимация как источник значения:** Оригинал прятал хардкоды
Timer1=140/64/400 которые MuckyFoot подбирали "на глаз" под длительность
анимации. Если художник поменяет анимацию — хардкоды надо вручную
подкручивать. Считать из asset'а даёт автоматическую адаптацию.

**Почему pre-release=0:** пробовали 2 и 4. 2 работало "почти", но на
бегу быстрый тап через 1 тик после включения мотора давал выстрел без
щелчка (мотор не успел физически прогреться). 4 прогрев решает, но
открывает окно где мотор активен а игра ещё в кулдауне → щелчки без
выстрелов. 0 — мотор и gate включаются синхронно, никаких рассогласований.
Побочка: очень быстрое нажатие в момент включения мотора может не дать
тактильного клика (HID latency ~30мс), но это редкий случай.

**Почему строгий Timer1=0 а не pre-release окно для fire:** pre-release
fire позволял стрелять раньше, с debt-компенсацией следующего кулдауна.
Математика правильная, но пользователь на практике чувствовал "клики
во время кулдауна" потому что в pre-release окне мотор активен а игра
частично активна. Строгий Timer1=0 проще и предсказуемее.

**Почему анимация выстрела стоя прерывается:** `SUB_STATE_SHOOT_GUN`
выходит когда Timer1=0, а не когда анимация доиграла свои кадры до
конца. Timer1 выведен из длительности анимации, так что выход
происходит близко к концу анимации — глитч "рука дёрнется" если
выход на последних кадрах, но это ≤2 кадра и незаметно. Принято как
допустимая цена за симметрию running/standing.

**Почему отказались от pending буфера:** pending буферизовал клики во
время кулдауна и выстреливал их позже. На стоя он не проявлялся (там
Action=SHOOT жёстко блокировал input до конца состояния). На бегу
приводил к "отложенным выстрелам без клика" — хардкорный антипаттерн
click ⇔ shot синхронизации. Убран.

**Почему отказались от debt компенсации:** debt имел смысл только
при pre-release fire. Сейчас fire строго при Timer1=0, debt всегда 0.
Функция `weapon_feel_consume_shot_cooldown_timer1` ещё выдаёт base+debt
но debt всегда 0 — код можно упростить в будущем.

### Файлы

- `new_game/src/engine/input/weapon_feel.{h,cpp}`:
  - `calc_anim_ticks()` — симуляция anim ticker.
  - `weapon_to_shoot_anim_id()` — mapping weapon → anim_id.
  - `weapon_feel_get_shoot_anim_ticks()` — lazy caching.
  - `weapon_feel_consume_shot_cooldown_timer1()` — public API для
    person.cpp (возвращает Timer1 value).
  - `weapon_feel_pre_release_timer1()` — public pre-release порог
    (сейчас возвращает 0).
  - `weapon_feel_on_shot_fired(special, remaining_timer1)` — callback
    с parameter'ом для debt capture.
- `new_game/src/things/characters/person.cpp`:
  - `set_person_running_shoot` — anim-derived Timer1 для игрока,
    строгий fire gate.
  - `set_person_shoot` — для игрока не зануляет Timer1, ставит
    anim-derived после fire.
  - `actually_fire_gun` — передаёт Timer1 в weapon_feel.
  - `fn_person_gun` SUB_STATE_SHOOT_GUN — для игрока декрементит Timer1
    и выходит по 0.
- `new_game/src/game/game.cpp`:
  - `on_cooldown` = Timer1 > pre_release, унифицированный для обоих
    состояний.

### Известные open vопросы

- **Ксбокс контроллер:** нет adaptive trigger, поведение должно быть
  как на клавиатуре (просто нажатие = PUNCH). Проверить вживую.
- **AK47 (auto-fire):** сейчас использует старый threshold path в
  weapon_feel_evaluate_fire. Должно работать как было. Pre-release /
  debt механика не применима (auto_fire обходит armed gate).
- **Shotgun:** нет adaptive trigger click (trigger_strength=0 в профиле),
  работает через threshold path.

---

## 🚨 HANDOFF 2 — session continuation (2026-04-18 late)

**Предыдущий AI перезапущен из-за захламлённого контекста. Эта секция —
актуальный снапшот состояния на момент перезапуска.**

### Текущая архитектура (на момент перезапуска)

**Единый pending-based подход для running и standing:**

1. **game.cpp cooldown gates** (game-state-derived, без магических чисел):
   ```cpp
   in_running_cooldown = (State == STATE_MOVEING && Timer1 > 0);
   in_standing_cooldown = (State == STATE_GUN && SubState == SUB_STATE_SHOOT_GUN);
   on_cooldown = in_running || in_standing;
   ```
   `on_cooldown` → `weapon_ready=false` → `mode=NONE`. Мотор молчит в
   cooldown — это было твёрдое требование юзера.

2. **weapon_feel API** (минимум):
   ```cpp
   void weapon_feel_set_game_state(bool in_cooldown, bool weapon_ready);
   ```
   Вызывается из game.cpp каждый тик.

3. **Click detection** в `weapon_feel_evaluate_fire` (act-path):
   - По r2-position: `prev_r2 < zone_upper_r2 && r2 >= zone_upper_r2`.
   - `zone_upper_r2 = end_zone * R2_UNITS_PER_ZONE` (R2_UNITS_PER_ZONE = 28).
   - Работает **независимо от mode** — click фиксируется даже при mode=NONE.
   - Если `in_cooldown` → `s_pending_shot = true`.
   - Если нет → `out.shoot = true` (immediate).

4. **Pending flush**:
   - `!in_cooldown && weapon_ready` → emit PUNCH, clear pending.
   - `!weapon_ready` (jump/cutscene/holster) → cancel pending.

5. **Lockout на fire frame** возвращён (`gamepad_triggers_lockout(0)` в
   `weapon_feel_on_shot_fired`).

6. **Compensation убрана полностью**. `weapon_feel_consume_timer1_compensation`
   больше нет. Running теперь работает через pending как и standing —
   нет per-shot Timer1 manipulation.

### Магические числа / чистота кода

- `R2_UNITS_PER_ZONE = 28`, `WEAPON25_ZONE_COUNT = 9`, `DEFAULT_FIRE_THRESHOLD_R2 = 200`,
  `DEFAULT_RESET_THRESHOLD_R2 = 80` — константы в `weapon_feel.cpp` namespace.
- **Запрет магических чисел записан в CLAUDE.md** (требование #0 для
  любого кода, не только weapon feel).

### Актуальная проблема (причина перезапуска)

Юзер пожаловался: **"два раза быстро кликаю и 2-й без клика но он стреляет"**.

Трассировка:
- Fire #1 → `lockout(0)` → `mode=NONE`. `Timer1=140`.
- Игрок тапает второй раз, Timer1 всё ещё > 0 (cooldown). `mode=NONE`
  (gated). Hardware мотор **не щёлкает** (mode=NONE) — это то что юзер
  требовал.
- Наш r2-based detector ловит пересечение `zone_upper`. `s_in_cooldown=true`
  → `s_pending_shot=true`. Tactile не даётся (motor off).
- Timer1 → 0. Pending flush → PUNCH → shot. Motor активируется с задержкой
  HID, к моменту fire motor не готов → tactile отсутствует.

**Итого**: pending-flushed shot **всегда** без tactile feedback (потому
что motor был выключен всю cooldown). Юзер жалуется что это некрасиво.

### Что предложил юзер (ЗАПИСАТЬ И ОБДУМАТЬ)

> **"Нельзя просто взять то же значение кулдауна что при беге и так же
> запускать таймер?"**

Идея: для стоя использовать **тот же Timer1-like механизм** что и для
running. После standing fire запустить counter (Timer1 или свой),
декрементировать его и использовать как gate. Это даст unified cooldown
mechanism с одним set of logic.

Это возможно проще чем два разных signal'а (Timer1 vs SubState).

Однако `set_person_shoot` (standing) сейчас устанавливает `Timer1 = 0`
и не использует его. Animation sama rate-limiter. То есть game не имеет
Timer1 counter для standing.

**Варианты:**
- A. Завести свой wall-clock counter в weapon_feel при каждом fire
  (аналог Timer1 но на нашей стороне). Значение = параметр профиля
  (per-weapon). Это возвращает wall-clock magic number в профиль —
  то от чего юзер отказывался.
- B. Использовать `time` из `shoot_get_ammo_sound_anim_time` (для pistol
  = 140 и для running и для standing). Не привязано к реальной длине
  анимации (374мс empirically, а Timer1=140 даёт 583мс), но это
  game-defined константа, не magic.
- C. Использовать running Timer1 даже для standing — модифицировать
  `set_person_shoot` чтобы устанавливал Timer1=time как running. Тогда
  единый signal. Но `set_person_shoot` — uc_orig функция, менять нужно
  аккуратно.

Вариант **C** кажется наиболее чистым — unified Timer1 signal, minimal
game-side changes.

### План для следующей сессии

1. Прочитать эту секцию + handoff выше.
2. Обсудить с юзером вариант C: установить `Timer1 = time` в `set_person_shoot`
   (для player'а only, чтобы не сломать AI).
3. Затем game.cpp gate упростится: `on_cooldown = (Timer1 > 0)` — одна
   проверка для обоих случаев.
4. `SUB_STATE_SHOOT_GUN` check можно будет убрать.
5. Pending механизм остаётся как есть.
6. Remaining bug "pending flush без tactile" — обсудить с юзером,
   принимать trade-off или искать решение.

### Что **не** предпринимать

- ❌ Wall-clock cooldowns в профиле (fire_cooldown_ms=300/400 и т.п.).
  Юзер явно отказался.
- ⚠️ Retry с удержанием PUNCH bit несколько тиков. Изначально юзер
  отверг из-за риска зависания. **Переосмыслено 2026-04-18:** можно
  попробовать, но не бесконечно — держать флаг **ограниченное число
  тиков** (например 1-5), если за это окно выстрел не применился —
  снимать. Это страхует от hang. Идея покрывает случай "click прилетел
  за тик-два до конца cooldown" — без retry этот клик теряется.
  Параллельно юзер готов принять 5% случаев "тактильность без выстрела"
  (мотор может оставаться включённым в cooldown). Не делаем пока не
  обсудим.
- ❌ Pre-release через магические числа. Только через game-derived
  signals.
- ❌ mode=AIM_GUN всегда во время cooldown. Юзер сказал «опять клики во
  время кулдауна» — строгое НЕТ.

### Состояние файлов в коде

- `new_game/src/game/game.cpp`: on_cooldown = running || standing, оба
  derived from game state.
- `new_game/src/engine/input/weapon_feel.h`: один API `weapon_feel_set_game_state`.
- `new_game/src/engine/input/weapon_feel.cpp`: pending-based evaluate_fire
  act-path. Lockout возвращён.
- `new_game/src/things/characters/person.cpp`: compensation logic убрана,
  `set_person_running_shoot` вернулся к оригинальному Timer1=time.
- `CLAUDE.md`: добавлено правило #0 про именованные константы.

Build проходит. Проблема сейчас — юзерский опыт (tactile missing на
pending-flushed shots), не билд-блокер.

---

## 🚨 HANDOFF — adaptive trigger click sync не готов (2026-04-18)

**Статус: НЕ РАБОТАЕТ корректно. Пользователь будет микроконтролить
следующую сессию. Не предпринимай изменений без явной команды — только
объясняй как что работает.**

### Что пользователь хочет (твёрдо, не переспрашивать)

0. **Система должна быть устойчива к изменениям параметров.** На этапе
   стабилизации (после того как базовая логика заработает) нужно убрать
   хардкоды: магические числа завязанные на конкретные Timer1 values,
   декремент Timer1 (сейчас 16/тик), конкретные виды оружия. Система
   должна автоматически подстраиваться под разные Timer1 (пистолет=140,
   AK47=64, shotgun=400) и под любые tick rates / декременты без ручной
   подгонки. Известные хардкоды на 2026-04-18: `Timer1 > 31` в
   [game.cpp](../new_game/src/game/game.cpp) (pre-release 2 тика,
   завязан на декремент 16).
1. **Click ⇔ shot строго синхронизированы.** Каждый реальный выстрел
   должен сопровождаться физическим щелчком на триггере. Каждый
   физический щелчок должен соответствовать реальному выстрелу. Никаких
   clicks без shots и никаких shots без clicks.
2. **Rate стрельбы gamepad'ом должен совпадать с rate стрельбы на
   клавиатуре.** Если на клаве игрок жмёт быстро и получает много
   выстрелов — на gamepad'е должно быть так же.
3. **"Допустимо" НЕ равно "выполнено".** Юзер явно сказал не принимать
   trade-off'ы. Делаем пока не заработает идеально.

### Текущее состояние кода (не трогать без команды)

Файлы с правками:
- [`new_game/src/engine/input/weapon_feel.cpp`](../new_game/src/engine/input/weapon_feel.cpp) / `.h`
- [`new_game/src/engine/input/gamepad.cpp`](../new_game/src/engine/input/gamepad.cpp) / `.h`
- [`new_game/src/game/game.cpp`](../new_game/src/game/game.cpp)
- [`new_game/src/game/input_actions.cpp`](../new_game/src/game/input_actions.cpp)

Ключевые моменты текущей версии:

1. **Act-bit detection (`weapon_feel_evaluate_fire`)** — fire event
   только на hardware `act 1→0` edge с `r2 > reset_threshold`.
   **Threshold fallback убран** (юзер жаловался что он стреляет без
   кликов).
2. **`gamepad_triggers_lockout(0)`** — вызывается в
   `weapon_feel_on_shot_fired` на fire frame → мгновенно шлёт
   `mode=NONE` HID пакет, минуя ожидание следующего тика.
3. **Running gate (`game.cpp`)**:
   ```cpp
   const bool in_running_cooldown =
       (darci_t->State == STATE_MOVEING &&
        darci_t->Genus.Person->Timer1 > 15);
   bool on_cooldown = in_running_cooldown || weapon_feel_is_in_fire_cooldown();
   ```
   Pre-release на 1 тик (`> 15` вместо `> 0`) — чтобы HID пакет
   `NONE→AIM_GUN` успел пропагировать к моменту `Timer1=0`.
4. **`weapon_drawn` параметр** у `weapon_feel_evaluate_fire` —
   различает bare-hand (threshold path) от пистолета (act-bit path).
5. **`fire_cooldown_ms` у всех профилей = 0**, машинерия оставлена
   как инструмент для post-1.0 кастомных оружий.
6. **`STATE_GUN` НЕ добавлен** в `non_firing_state` (пробовал, не помогло).

### Всё что пробовали и почему каждый вариант провалился

| # | Что сделали | Симптом провала |
|---|-------------|-----------------|
| 1 | act-bit + threshold fallback + no gate | Shot без click (fallback стрелял когда hardware не успел) |
| 2 | 600мс wall-clock gate (Timer1-matching) | Стреляет медленно, клик нечёткий (mode toggle jitter) |
| 3 | 100мс gate + pulse_reset (motor reset hack) | Работало на vendored либе. С libDualsense — не нужно было, но тогда было хорошо |
| 4 | No gate, mode=AIM_GUN всегда | I2: clicks без shots во время running Timer1 |
| 5 | `STATE_MOVEING && Timer1 > 0` gate | Стоя клики без выстрела + бегом shots без clicks после gate release (HID окно) |
| 6 | **Текущее:** lockout + `Timer1 > 15` + no fallback | "все херня опять" — точные симптомы не описаны юзером |

### Твёрдые факты про hardware / game

- **Timer1 для пистолета = 140 unit, ~583 мс wall-clock** на running
  shot (`set_person_running_shoot`).
- **Timer1 декрементируется ТОЛЬКО в `fn_person_moveing`** (STATE_MOVEING).
  Если игрок выходит из STATE_MOVEING с ненулевым Timer1 — он замирает.
- **Standing fire (`set_person_shoot`) не использует Timer1** —
  устанавливает его в 0, скорость ограничена только анимацией.
- **libDualsense с правильным Weapon25 packing** → hardware motor
  стабильно щёлкает на каждое нажатие когда mode=AIM_GUN continuously.
- **Mode toggle NONE↔AIM_GUN** создаёт HID RTT окно (~30мс) где
  поведение мотора undefined → jitter.
- **Act-bit sampling** на 15fps game tick (~67мс на тик) может
  пропускать fast press если R2 проходит зону за один тик — тогда
  мы не видим `act=1` state, только `act=0→0` и edge не детектится.
- **Game tick order**:
  1. `gamepad_poll`
  2. `get_hardware_input` → `weapon_feel_evaluate_fire` (step 2)
  3. game simulation → `actually_fire_gun` → `weapon_feel_on_shot_fired` (step 3)
  4. `gamepad_rumble_tick`
  5. `gamepad_triggers_update` (step 5) — читает `on_cooldown`, форсит mode

### Открытые вопросы (для исследования под микроконтролем)

- Что ИМЕННО в текущей версии симптомами показалось юзеру "херня"?
  Standing или running? Clicks без shots? Shots без clicks? Медленный
  rate? Нужны точные симптомы от пользователя.
- Реально ли `STATE_MOVEING` всегда означает "running" с точки зрения
  игровой логики? Возможно есть подстаты (`SUB_STATE_WALKING_BACKWARDS`
  etc) где Timer1 gate не должен применяться.
- Действительно ли mode=AIM_GUN остаётся активным после 1-тикового
  `gamepad_triggers_lockout(0)`? Возможно есть race где lockout шлёт
  NONE, а следующий `gamepad_triggers_update` сразу шлёт AIM_GUN
  обратно (особенно если Timer1 ещё не установлен или подхватывается
  не из того кадра).
- Правильно ли работает `weapon_feel_is_in_fire_cooldown()` если
  у всех `fire_cooldown_ms=0`? (Всегда возвращает false — эта проверка
  в `on_cooldown` просто дописана как OR для будущего использования,
  сейчас no-op).

### План работы со следующей сессией

1. Юзер воспроизводит конкретный симптом.
2. Ты объясняешь что сейчас делает код в этом конкретном сценарии
   (не начинаешь переписывать).
3. Юзер решает что менять.
4. Ты делаешь одну конкретную правку по команде юзера.
5. Повтор.

**НЕ** предлагай архитектурные решения сам. **НЕ** делай изменения
без явной команды. **Объясняй** как работает текущий код — вплоть до
построчной трассировки если надо.

### Файлы которые стоит читать при входе в задачу

- Этот файл (сверху донизу, но handoff-секция — самая свежая).
- Module manual в самом начале [`weapon_feel.cpp`](../new_game/src/engine/input/weapon_feel.cpp).
- [`dualsense_adaptive_trigger_facts.md`](dualsense_libs_reference/dualsense_adaptive_trigger_facts.md) —
  hardware факты.
- [`gamepad_triggers_update` в gamepad.cpp](../new_game/src/engine/input/gamepad.cpp) —
  state machine режимов.

---

## ⚠️ Начни отсюда

Этот файл накапливался итерациями. Актуальное состояние живёт в секции
[**"Расследование adaptive trigger синхронизации"**](#расследование-adaptive-trigger-синхронизации-2026-04-15-сессия)
ниже. Она содержит:

- **Чеклист требований** (R1-R5 геймплей + P1-P5 процессные) — всегда
  сверяться перед любой правкой в эту систему.
- **Факты vs предположения** про hardware поведение.
- **Все подходы которые пробовали** и почему каждый не работал.
- **Текущее стабильное состояние кода** с описанием каждого файла.
- **Известные проблемы** (I1 rapid-tap, I2 click-during-cooldown).
- **План** (hardware delay test, далее).

Блоки выше "Расследование" — исторические, описывают промежуточные
состояния и могут ссылаться на файлы которых уже нет (например
`gamepad_haptic.cpp` — переехал в `weapon_feel.cpp` и `weapon_feel.h`
в ходе рефакторинга). Читать для контекста, но код сверять только
по актуальной секции.

Главный исходник системы: [`new_game/src/engine/input/weapon_feel.cpp`](../new_game/src/engine/input/weapon_feel.cpp) —
в самом начале файла развёрнутый module manual с описанием per-tick
flow, текущего дизайна, известных проблем, и требований. Это вторая
точка входа помимо этого devlog.

## Что реализовано (актуально на 2026-04-15)

Всё живёт в едином модуле [`new_game/src/engine/input/weapon_feel.{h,cpp}`](../new_game/src/engine/input/weapon_feel.cpp).
В начале `.cpp` файла — развёрнутый module manual с per-tick flow и
дизайном. Читать при входе в систему.

### 1. Вибрация при стрельбе (envelope-based)

Вместо "true" Sony audio-haptic считаем амплитудную RMS-огибающую из
WAV-сэмпла оружия один раз при `weapon_feel_init()`, сохраняем как
`std::vector<uint8_t>` и проигрываем на slow-моторе DualSense через
`weapon_feel_tick_haptic()` (вызывается из `gamepad_rumble_tick`).
Max-merge со старым PS1-style shock мотором. Гейт на игрока —
`PlayerID && INPUT_DEVICE_DUALSENSE`.

Per-weapon параметры живут в `WeaponFeelProfile` — добавление нового
оружия = одна регистрация в `weapon_feel_init`, не трогая другие файлы.

Текущая таблица:

| Оружие | `gain` | `ceiling` | `max_seconds` | Ощущение |
|--------|--------|-----------|---------------|----------|
| **AK47** | 0.4 | 90 | 40ms | короткий резкий "bah", ритмично на авто-огне |
| **Пистолет** | 0.05 | 18 | 35ms | лёгкий "поп", слабее AK47 |
| **Дробовик** | 0.55 | 140 | 90ms | тяжёлый "бам" с долгим спадом |

`max_seconds` должен быть короче cooldown'а между выстрелами в очереди —
иначе огибающие сливаются в сплошной buzz.

### 2. Adaptive trigger (Weapon25)

State machine в [`gamepad.cpp`](../new_game/src/engine/input/gamepad.cpp)
`gamepad_triggers_update`. Три режима: NONE, AIM_GUN, CAR.

- `TRIGGER_MODE_AIM_GUN` = `ds_trigger_weapon(start_zone, amplitude, 0, 0, 1)`
  где параметры берутся из `WeaponFeelProfile` активного оружия.
  Все поля Weapon25 4-битные (0..15), overflow тихий.
- Сейчас для всех трёх оружий `start_zone=7, amplitude=2` — единый
  "лёгкий щелчок" feel, но каждое оружие хранит свою копию и его
  можно тюнить независимо.
- **Mode=AIM_GUN включается безусловно** когда weapon_ready && has_click.
  Нет R2 gate, нет armed gate, нет cooldown gate. Обоснование и
  trade-off'ы см. в секции "Текущее стабильное состояние кода" ниже.

### 3. Fire detection

`weapon_feel_evaluate_fire(weapon_type, r2, l2)` вызывается из
[`input_actions.cpp`](../new_game/src/game/input_actions.cpp)
`get_hardware_input`. Возвращает `{shoot, kick}` — эти события
пушатся в input pipeline игры как `INPUT_MASK_PUNCH` / `INPUT_MASK_KICK`.

Логика — чистый rising-edge: `R2 ≤ reset_threshold (80)` → `armed=true`;
`R2 > fire_threshold (200) && armed` → `shoot=true, armed=false`.
Для auto-fire оружий (AK47) armed игнорируется — стреляет пока R2
удерживается. L2 kick всегда rising-edge, не auto-fire.

Никаких wall-clock таймеров в fire logic. Rate limiting — через game
Timer1 внутри `actually_fire_gun`.

### 4. Callback on_shot_fired

[`actually_fire_gun`](../new_game/src/things/characters/person.cpp)
в game-side логике вызывает `weapon_feel_on_shot_fired(weapon_type)`
**только на реальных фиксациях выстрела** (Timer1 был 0). Это
единственный источник истины про "реальный выстрел случился". Внутри
запускается envelope playback и вызывается `gamepad_triggers_lockout`
для single-shot оружий (legacy, blink-mode — см. комментарий в
gamepad.cpp).

## Что работает хорошо в текущем стабильном состоянии

Актуально на 2026-04-15 для версии описанной в секции "Текущее
стабильное состояние кода" ниже.

- Per-weapon вибрация с текстурой (не сплошной buzz), юзер-тюнед для
  пистолета / AK47 / дробовика.
- Щелчок триггера синхронизирован с моментом реального выстрела при
  **нормальном темпе** стрельбы (прицельные одиночные, очереди с
  естественными паузами).
- Non-firing states (прыжок, падение, лазание, драка) — щелчка нет,
  mode=NONE корректно гейтит.
- Dry fire (пустой магазин) — щелчок работает, как у реального
  пистолета.
- Auto-fire у AK47 работает как должен (held-down, envelope retrigger
  на каждый шот очереди).
- Естественный темп стрельбы (game Timer1) соблюдается — без
  искусственных кулдаунов.

Известные остаточные проблемы (I1/I2) описаны ниже в секции
"Результаты теста стабильной версии".

## Расследование adaptive trigger синхронизации (2026-04-15, сессия)

Живые заметки по итогам длинной сессии отладки. **Важно**: факты отмечены
как "**знаем**", предположения — как "**предположение**". Не путать.

### ⚠️ ЧЕКЛИСТ ТРЕБОВАНИЙ — сверяться при КАЖДОЙ правке

**Перед тем как коммитить/предлагать правку в adaptive trigger систему —
пройти по этому списку и убедиться что ни одно требование не нарушено.
Пользователь явно просил не забывать про одно когда работаешь над другим.**

#### Геймплейные требования

- **R1. Естественный темп стрельбы.** Fire rate должен определяться
  game Timer1, не искусственными wall-clock таймерами. Замедление fire
  rate недопустимо. *Источник: "если задержку между выстрелами вернуть
  к стандартной..."* → "Нам нужно сделать какую то систему которая будет
  работать на дефолтной задержке".
- **R2. Поведение быстрого тыка как на клаве.** Press 1 → shot, press 2
  во время cooldown → **тихо игнорируется** без побочных эффектов,
  press 3 после cooldown → shot. Быстрый тап не должен "ломать"
  систему так чтобы следующие нажатия не срабатывали. *Источник:
  "не так что полностью все выстрелы блокируются а так что блокируются
  только те что в момент кулдауна жмутся".*
- **R3. Никаких "выстрелов без щелчка".** Если игра фиксирует выстрел —
  hardware обязан отдать тактильный щелчок. Это главная жалоба всей
  сессии.
- **R4. "Щелчки без выстрела" во время cooldown — допустимы.**
  Пользователь явно сказал "в целом тут тоже ок". *Не приоритизировать
  устранение этого за счёт R3.*
- **R5. Между выстрелами триггер свободный (mode=NONE).** Желательно.
  *Источник: "курок опускается без реакции и без натяжения".* Менее
  приоритетно чем R1-R3 — можно пересмотреть если конфликтует с ними.

#### Процессные требования

- **P1. Не решать за юзера что оставить как есть или отложить в post-1.0.**
  Даже если кажется что баг непоправим. *Источник: "post 1.0 мы это не
  оставим. нехуй за меня решать".*
- **P2. Никакой самодеятельности когда юзер сказал "подожди".**
  Если юзер прерывает или говорит "не делай ничего" — останавливаться
  полностью, ждать явного разрешения.
- **P3. Не билдить самому.** Юзер собирает сам. Исключение: явная
  просьба.
- **P4. Debug logs в weapon_feel.cpp не убирать** пока юзер явно не
  скажет "закончили".
- **P5. Факты vs предположения.** При записи в документацию помечать
  что **знаю** (проверено/логом/кодом), а что **предположение**.
  Не путать одно с другим.

### Что **знаем** про hardware

- **Weapon25 click однократный**: после срабатывания железо "разряжает"
  click-state, и для следующего щелчка триггер должен физически
  вернуться в release zone (точная позиция — **предположение**: где-то
  около reset_threshold=80 или глубже, **не проверено**).
- **HID BT latency**: типично 7-30мс round-trip для DualSense через
  Bluetooth. Источник: общеизвестная характеристика BT HID, не
  измеряли на этом контроллере конкретно.
- **Game tick rate**: ~30Hz (~34мс на тик), установлено из логов где
  tick timestamps идут с интервалом 33-34мс.
- **Пистолет Timer1 = 140** (внутренние единицы игры), в реальном
  времени **предположительно** ~150-250мс. Точное соотнесение
  внутренних единиц к мс не проверено.
- **R2 семплируется раз в game tick**. Между тиками мы не видим
  промежуточных значений, физические движения триггера быстрее одного
  тика для нас происходят "мгновенно" (0→254 за один poll).

### Что **предполагаем** (не проверено)

- **Мотор Weapon25 имеет физическое время engagement'а** после
  активации AIM_GUN — возможно десятки мс, возможно больше.
  **Не доказано.** Возможно задержки нет вообще и причина missing
  кликов в чём-то другом. Пользователь предложил отдельный тест
  на это измерить — пока не сделали.
- **HID race реальна**: если mode транзитит NONE→AIM_GUN в момент
  когда пользователь уже физически двигает триггер past click point,
  hardware клик не сработает. Косвенно подтверждается логами
  (в которых при 100мс MODE_PRE_ENABLE происходили missing clicks,
  а при 300мс — пропадали), но **не доказано строго**.

### Подходы которые пробовали и почему они не сработали

#### 1. Armed gate (mode=NONE когда armed=false)
Mode следует за rising-edge armed state. Сразу после выстрела
armed=false → mode=NONE. Release → armed=true → mode=AIM_GUN
(HID отправляется). Press → выстрел + щелчок.

**Проблема**: HID race. Time between release dip detection и press
past click point может быть короче HID RTT. Mode команда приходит
когда триггер уже за click point → hardware click не срабатывает.
**Симптом**: "выстрел есть, клика нет" при быстром release-press.

#### 2. Unconditional mode (mode=AIM_GUN всегда)
Mode постоянно включен пока weapon_ready. HID не флипается — race
невозможен в принципе.

**Проблема**: hardware click физически срабатывает на **любом**
пересечении click point, включая во время game cooldown (Timer1).
Game блокирует выстрел через Timer1, но hardware уже щёлкнул.
**Симптом**: "клик есть, выстрела нет" во время cooldown.

#### 3. Release-settle таймер (блокирует fire если release→press быстрее N мс)
Мысль: если юзер тыкает слишком быстро — блокировать выстрел, чтобы
дать hardware время.

**Проблема 1 (ранний вариант с delayed fire)**: таймер откладывал
выстрел вместо блокировки — через 500мс пользователь всё равно
получал выстрел если держал триггер, причём без клика.
**Проблема 2 (consumed вариант)**: при release-press быстрее таймера
блокировка "прилипала" (s_armed_consumed оставался true) пока
не случался новый глубокий release. Быстрый тап = все прессы
блокировались подряд. Это то что пользователь описал как
"вообще не стреляет при быстром тапе".

#### 4. Post-fire cooldown (wall-clock N мс после выстрела)
Ограничивает частоту выстрелов wall-clock таймером. Блокирует
fire логику и mode (через is_r2_armed).

**Проблема**: требует POST_FIRE_COOLDOWN ≥ MODE_PRE_ENABLE, иначе
логика split cooldown'а ломается. Значит минимум 300-400мс
между выстрелами — **медленнее чем естественный Timer1**.
Пользователь явно сказал что это недопустимо — нужен натуральный
темп.

#### 5. MIN_PRESS_DURATION (блокирует если press быстрее N мс)
Пытался блокировать "быстрые" нажатия измеряя время от
RISE-past-reset до fire_threshold.

**Проблема**: при poll rate 30Hz естественные пресс-ы занимают
1-2 тика (33-68мс). Порог 80мс блокировал **все** пресс-ы, не
только быстрые. При tick rate 30Hz мы принципиально не можем
различить "нормальный" и "слишком быстрый" press — данных нет.

#### 6. Split mode/fire lockout (mode возвращается раньше fire)
Mode lockout короче fire lockout на `MODE_PRE_ENABLE`, давая mode
HID время пропагировать перед тем как fire будет разрешён.

**Проблема**: работает только пока POST_FIRE_COOLDOWN достаточно
большой (≥ MODE_PRE_ENABLE). При естественном Timer1 окно становится
отрицательным или нулевым, mode lockout исчезает → возвращается
HID race подхода (1).

### Ограничения которые **знаем** железные

- **Game tick rate 30Hz = 34мс на тик**. Любые timing-based проверки
  имеют granularity 34мс — невозможно точно измерить события короче
  этого.
- **R2 sampled только в момент poll'а**. Физические motion'ы между
  tick'ами невидимы.
- **Hardware state (click consumed / ready)** не наблюдаем со стороны
  PC. Мы не знаем щёлкнуло ли железо на конкретном пересечении.

### ✅ Текущее стабильное состояние кода (2026-04-15, коммит-кандидат)

**Принцип**: самая простая рабочая версия, без искусственных wall-clock
кулдаунов. Все попытки добавить свою rate-limit логику приводили к
нарушению R1 (натуральный темп) или R2 (keyboard-like) или возвращали
R3 (выстрел без щелчка). Оставляем только необходимый минимум.

**Что делает код сейчас:**

1. **`gamepad.cpp` — `gamepad_triggers_update`**:
   `mode=AIM_GUN` включается безусловно когда `weapon_ready &&
   weapon_has_click`. Никаких проверок на armed/consumed/cooldown.
   Hardware Weapon25 эффект держится непрерывно пока оружие готово,
   hardware сам рулит click state (однократный клик на физическом
   пересечении, автоматический rearm на глубоком release).

2. **`weapon_feel.cpp` — `evaluate_fire`**:
   Чистый rising-edge fire detector. R2 падает ≤ reset_threshold (80) —
   `s_r2_armed=true`. R2 > fire_threshold (200) и armed — выстрел,
   `s_r2_armed=false`. Для auto-fire (AK47) игнорирует armed и стреляет
   на любом пересечении. Никаких wall-clock таймеров, никакого consume,
   никакого post_fire_cooldown.

3. **`weapon_feel.cpp` — `weapon_feel_is_r2_armed`**:
   Возвращает просто `auto_fire || s_r2_armed`. **Не используется**
   сейчас gamepad.cpp — оставлен в API для будущих экспериментов.

4. **`game.cpp` — `weapon_ready`**:
   `has_gun && !non_firing_state && target_not_surrendered`. **Без**
   проверки Timer1 cooldown — cooldown пусть блокирует только реальный
   выстрел в `actually_fire_gun`, hardware эффект остаётся активен.

5. **`weapon_feel.cpp` — `on_shot_fired`**:
   Вызывает `gamepad_triggers_lockout(0)` на single-shot оружиях.
   *Полу-legacy*: форсит mode=NONE на фрейме выстрела, потом следующий
   tick `gamepad_triggers_update` обратно включает AIM_GUN. Эффективно
   это даёт кратковременный "blink" mode'а. Оставлено чтобы не
   ломать поведение envelope retrigger. **Возможно стоит убрать**
   — вызывает лишний HID write, пользы от него при unconditional mode
   нет. *На заметку для следующей итерации.*

6. **Rate limiting**: целиком через game Timer1. `actually_fire_gun`
   не вызывает `weapon_feel_on_shot_fired` если Timer1 > 0, так что
   наш envelope + lockout срабатывают только на реальных выстрелах.

**Debug logging**: весь логгинг в `weapon_feel_debug_log()` остаётся
в коде (пишет в `weapon_feel_debug.log` рядом с exe, flush на каждой
строке). **Не удалять** пока пользователь явно не скажет.

### Известные проблемы этого стабильного состояния

- **I1. Rapid tapping → некоторые клики пропадают**. На очень быстрых
  тапах hardware Weapon25 click physical rate limit (точное значение
  не знаем — **предположение**) не успевает за темпом. Игра стреляет
  нормально, hardware иногда не щёлкает. **При нормальной прицельной
  стрельбе не проявляется.** Нарушает R3 в пограничных случаях.
- **I2. Клик во время game Timer1 cooldown**. Игрок жмёт во время
  cooldown — hardware щёлкает (mode=AIM_GUN), игра не стреляет
  (Timer1 блок). Результат: "щелчок без выстрела". Пользователь
  раньше сказал что это "в целом ок" (R4), но стоит проверить заново
  что это не слишком раздражает на натуральном Timer1.
- **I3. Возможная hardware motor engagement delay**. **Не проверено.**
  Логи показывали missing clicks когда mode=AIM_GUN был уже активен
  (cur_mode=1 в triggers_lockout), что указывает либо на hardware
  задержку, либо на то что моё понимание hardware click state
  неверно. Планируется отдельный тест для измерения.

### Hardware rate limit измерен (2026-04-15)

Тест в observation-mode (`weapon_feel_test.cpp`, активируется `\`): триггер
удерживается в `mode=AIM_GUN` непрерывно, персонаж не стреляет, на экран
выводится `last gap` между press'ами. Игрок тапает R2 в разных темпах и
слышит/чувствует пропадают ли клики, коррелируя с `last gap`.

**Результат** (уточнён 2026-04-17): hardware Weapon25 не успевает
перевзводиться при быстром тапе.
- **≥ 300 мс между press'ами** — клик срабатывает стабильно.
- **200-300 мс** — серая зона, клик может быть а может и не быть.
- **< 200 мс** — клик **не срабатывает никогда**, даже сопротивление в
  триггере пропадает совсем (мотор не успевает вообще ничего).

Это **факт** про hardware, не предположение. Объясняет I1 (rapid tap missing
clicks) — это не баг нашей логики, это физический предел моторчика внутри
DualSense. Любая стрельба с интервалом быстрее ~300 мс принципиально не
может иметь щелчок на каждом выстреле.

**Следствия для дизайна**:
- Для AK47 (auto-fire ~100-150 мс между выстрелами очереди) — щелчка на
  каждый шот быть не может. Нужен другой эффект (MachineGun26 вибрация?
  или просто принять отсутствие щелчка в очереди).
- Для пистолета Timer1=140 (internal units) — надо сопоставить с реальным
  временем. Если натуральный темп < 300 мс, I1 проявляется на быстрой
  стрельбе одиночными. Если ≥ 300 — вообще не проявляется.
- I1 **нельзя починить** внутри Weapon25. Вариант "притворяться что щелчок
  был" через замену на вибро-импульс (MachineGun26 или rumble burst) —
  возможный путь для быстрой стрельбы.

### Результаты теста стабильной версии (2026-04-15, пистолет)

Пользователь протестировал текущую версию (unconditional mode + plain
rising-edge). Подтверждено:

1. **I1 подтверждено**: быстрый тап → можно получить выстрел без щелчка.
2. **I2 подтверждено**: нажатие во время Timer1 cooldown → щелчок без
   выстрела.
3. **Non-firing states (прыжок, падение, драка, и т.д.)**: всё ок,
   щелчков там нет как и должно быть.

Пока тестировали только пистолет. Дробовик и AK47 не проверяли в
этой сессии.

### План на продолжение

1. **Hardware motor delay test**. Ключевая неизвестная: существует ли
   физическая задержка между HID mode=AIM_GUN и моментом когда мотор
   действительно готов выдать щелчок, и если да — сколько она. Без
   этих данных любая попытка элиминировать I1 — наугад.
   Возможные подходы:
   - Автоматический, через клавишу: код форсит mode=NONE, ждёт N мс,
     включает mode=AIM_GUN, просит пользователя нажать R2. Варьируем N.
   - Ручной, через экран: дебаг overlay показывает "mode=AIM_GUN,
     T=N мс назад", пользователь жмёт и мысленно замечает есть ли
     клик. Коррелируем T с ощущением.
   Пользователь согласился на тест, пока не сделан.
2. **Если hardware delay подтвердится** — задокументировать её
   величину прямо в этом devlog и сесть думать над механизмом
   который её учитывает, не нарушая R1/R2. Идеи пока нет.
3. **Если не подтвердится** — I1 и I2 остаются нерешёнными
   принципиально из-за hardware click rate limit, прорабатываем
   альтернативные эффекты триггера (не Weapon25), или документируем
   как ограничение "post-1.0 investigation".
4. **Когда adaptive trigger стабилизируется** — разобраться с
   `gamepad_triggers_lockout` blink'ом (строка в gamepad.cpp): сейчас
   это бесполезный HID-write на каждом выстреле, должен уйти или
   переосмыслиться.
5. **Побочная наблюдение из сессии**: пользователь видел случаи
   "клик есть, но полу-клик, не чёткий". Может быть артефакт
   hardware delay, может что-то ещё. Стоит держать в голове при
   тестах.

### Миграция на act-bit детект щелчка (2026-04-17)

После того как собственная `libDualsense` стабилизировалась и feedback
status стал надёжно репортиться для full-name эффектов (Weapon25/Feedback/
Vibration/Bow/Machine/Galloping), заменили в `weapon_feel_evaluate_fire`
empirical r2-threshold-based детект щелчка на прямой hardware-сигнал —
бит 4 байта 0x29 (act flag), который контроллер сам выставляет когда
трекает мотор эффекта.

**Что конкретно поменялось:**

- `weapon_feel_evaluate_fire` теперь имеет две ветки:
  - **Act-bit path** — DualSense + `trigger_strength>0` + не auto-fire:
    fire на переходе `act 1→0` при `r2 > reset_threshold`. Нет armed gate,
    нет wall-clock таймеров — hardware сам эмитит максимум один клик на
    физический цикл нажатия.
  - **Threshold fallback** — всё остальное (AK47 auto-fire, shotgun без
    click, Xbox, клавиатура): старая threshold + armed rising-edge логика
    без изменений.
- Сигнатура функции не менялась, act bit берётся из
  `gamepad_get_right_trigger_effect_active()` (который уже обновляется
  per-poll в `gamepad.cpp`).
- Debug log расширен: в `tick` добавлено `act=N`, добавлены события
  `FIRE-CLICK` и `act 1->0 release` (последнее — когда edge
  совпадает с отпусканием курка внутри зоны без щелчка).

**Ожидаемое влияние на I1/I2:**

- **I1 (выстрел без щелчка)** — должен уйти. Мы больше не гадаем по
  r2-позиции, fire идёт ровно от hardware click-сигнала. Если hardware
  не щёлкнул (быстрый тап < 200мс), мы не стреляем — это совпадает
  с требованием R3 "не стрелять без щелчка". Потенциально может
  проявиться другой симптом на ОЧЕНЬ быстром нажатии: если весь цикл
  `act=0 → act=1 → act=0` укладывается в один 33мс тик, мы не
  увидим промежуточный `act=1` и не детектим edge → клика нет →
  выстрела нет. Это корректнее чем старое I1 (тогда был выстрел без
  щелчка, теперь — ни того ни другого, что консистентно с R2
  "keyboard-like"). Hardware Weapon25 rate limit всё равно не даст
  щёлкнуть чаще ~300мс.
- **I2 (щелчок без выстрела)** — **сохраняется** в прежнем виде.
  Timer1 gate срабатывает в `actually_fire_gun`, hardware клик мы
  детектируем и вызываем shoot, Timer1 молча отказывает. Это явно
  помечено как R4-допустимо пользователем.

**Требует проверки:**

- Пистолет — основной кейс act-path. Живое тестирование: одиночные
  прицельные, быстрые тапы, нажатие во время Timer1 cooldown.
- AK47 — остаётся на threshold-path (auto_fire=true). Не должен
  регрессировать.
- Shotgun — остаётся на threshold-path (trigger_strength=0). Не должен
  регрессировать.
- Xbox / клавиатура — fallback на threshold-path. Не должен
  регрессировать.
- macOS / Steam Deck — проверить что act bit приходит корректно
  (feedback offsets уже валидированы cross-platform, см.
  `dualsense_adaptive_trigger_facts.md`).

**Результаты тестирования (2026-04-17, пистолет, Windows DualSense):**

- ✅ **I1 устранён.** Пользователь подтвердил: "нет кейса когда тыкаю и
  нет щелчка". Старый сценарий "выстрел без щелчка при быстром тапе"
  больше не воспроизводится — действительно fire теперь идёт только
  на hardware-сигнале щелчка.
- ⚠️ **I2 сохранился** (щелчок во время Timer1 cooldown) — пользователь
  пересмотрел требование R4 ("в целом ок") и попросил убрать. См.
  следующий раздел.
- macOS / Steam Deck / Xbox / клавиатура — не протестированы в этой
  сессии.

### Фикс I2 через cooldown-gate на mode (2026-04-17)

Пользователь отменил R4 как "tolerable" — кликов во время cooldown
быть не должно. Пересмотрели и реализовали cooldown gate. С act-bit
путём старая HID-race проблема больше не влияет на fire-detection.

#### Первая попытка: Timer1 — НЕ СРАБОТАЛО

Первый подход — использовать игровой `Person::Timer1` (задаётся в
`set_person_running_shoot` как per-weapon cooldown: pistol=140,
AK47=64, shotgun=400). Изменили в `game.cpp` `on_cooldown = (Timer1 > 0)`.

**Регрессия:** игрок мог попасть в состояние где выстрелы и клики
полностью пропадали. Репро: пробежал, выстрелил из пистолета на
бегу, остановился, жму R2 — ничего. Сколько ни жди, ни жми — не
стреляет. Помогал только пробег заново.

**Причина** — Timer1 имеет 3 серьёзных проблемы как signal cooldown'а:

1. **Декрементируется только в `fn_person_moveing`** (STATE_MOVEING).
   Если игрок выстрелил на бегу и остановился — Timer1 застревает
   на значении cooldown'а (140 единиц) навсегда. Пока не побежит
   снова, Timer1 не уменьшится → наш gate вечно true → mode вечно
   NONE → выстрелов и кликов нет.
2. **Standing-still fire (`set_person_shoot`) не использует Timer1
   вообще.** Он выставляет Timer1=0 и полагается на animation state
   машину как rate limit. То есть для стрельбы стоя у Timer1 нет
   смысла post-fire cooldown'а.
3. **Timer1 переиспользуется для других таймеров** на том же Person:
   `set_person_idle` (random 400-911 когда gun НЕ out — но в карьере
   может быть доставание/убирание оружия где Timer1 всё ещё ненулевой),
   `set_person_hop_back` (=3), и другие.

#### Вторая попытка: локальный wall-clock timer в weapon_feel — РАБОТАЕТ

Вместо чтения Timer1 ведём собственный cooldown в `weapon_feel`:

- Добавлено поле `fire_cooldown_ms` в `WeaponFeelProfile`
  (пистолет=600мс, AK47/shotgun=0 потому что у них нет adaptive click'а).

  **Про 600мс:** сначала поставили 200 и получили регрессию — rapid-tap
  со средней скоростью (~300-500мс между нажатиями) давал "2-й клик без
  выстрела". Причина: неверно оценили длительность игрового Timer1.
  Правильный расчёт для running-shot пистолета:
  - `Timer1 = 140` (из `shoot_get_ammo_sound_anim_time`)
  - Декремент за тик = `16 * TICK_RATIO >> TICK_SHIFT = 16` на нормальной скорости
  - Тиков до нуля: `140 / 16 = 8.75`
  - `NORMAL_TICK_TOCK = 1000/15 ≈ 66.67 мс` на тик
  - **Реальное время: `8.75 × 66.67 ≈ 583 мс`**

  600мс = 583 + 17мс margin, чтобы после истечения нашего gate'а
  HID-пропагация mode=AIM_GUN (~30мс) не успевала включить эффект
  раньше чем game Timer1 дойдёт до нуля.
- `weapon_feel_on_shot_fired` при каждом реальном выстреле
  устанавливает `s_fire_cooldown_end = now + fire_cooldown_ms`.
- Новая публичная функция `bool weapon_feel_is_in_fire_cooldown()` —
  возвращает `now < s_fire_cooldown_end`.
- `game.cpp` использует `weapon_feel_is_in_fire_cooldown()` вместо
  Timer1 для формирования `on_cooldown`.

Преимущества:
- Wall-clock — не зависит от того что делает персонаж (бежит, стоит,
  в воздухе). Тикается независимо от state машины.
- Старт только на реальных выстрелах (callback от
  `actually_fire_gun` где выстрел реально состоялся).
- Per-weapon тюнинг через профиль.
- Не интерферирует с другими таймерами на Person.

#### Почему HID race теперь не проблема

1. Игрок стреляет. Mode становится NONE (на fire frame через
   `gamepad_triggers_lockout` + последующие кадры через cooldown gate).
2. Cooldown 200мс для пистолета.
3. Cooldown истекает. На следующем кадре `on_cooldown=false`,
   `gamepad_triggers_update` отправляет `mode=AIM_GUN`. Hardware
   начинает принимать command через HID RTT (~7-30мс на BT).
4. Если игрок жмёт в этом HID-гап окне — hardware ещё не знает что
   click-effect активен → никакого клика. **Но и выстрела нет** (fire
   идёт от act-edge, act тоже не поднимется без активного эффекта).
   Игрок отпустит и нажмёт ещё раз. Это keyboard-like refusal, тот же
   поведенческий паттерн что "тап во время cooldown'а" — привычное
   поведение с клавиатуры.
5. Если игрок жмёт уже ПОСЛЕ HID пропагации → mode активен → клик
   → fire. Всё как ожидается.

**AK47 и shotgun не затронуты**: их профили с `trigger_strength=0`
не получают AIM_GUN mode вообще (`weapon_has_click` гейт в
`gamepad_triggers_update`) и `fire_cooldown_ms=0`. Когда у этих
оружий появятся собственные adaptive trigger эффекты — выставить
соответствующие cooldown_ms в их профилях.

### Регрессия: bare-hand punch через R2 не работал (2026-04-17)

После ввода act-bit пути обнаружилась ещё одна регрессия — нажатие
R2 **без оружия** (bare-hand melee punch) не срабатывает.

**Причина:** `SPECIAL_NONE` в регистре профилей маппится на пистолет
(`weapon_feel_register(SPECIAL_NONE, pistol)`). Это сделано потому
что пистолет — дефолтное оружие без отдельного `SpecialUse` Thing'а,
и `SpecialUse==null` одновременно означает и "безоружный" и "пистолет".

Когда у игрока **нет** оружия:
- `current_weapon=SPECIAL_NONE` → профиль пистолета → `has_click=true`.
- Act-path условие срабатывает.
- Но в `gamepad_triggers_update` игра видит `has_gun=false` →
  `weapon_ready=false` → mode=NONE. Hardware Weapon25 не активирован.
- Игрок жмёт R2 → hardware не щёлкает (mode=NONE) → act bit остаётся
  0 → act edge никогда не наступает → fire не происходит.
- R2 бесполезен для bare-hand punch.

**Фикс:** добавлен параметр `weapon_drawn` в
`weapon_feel_evaluate_fire`. Caller (`input_actions.cpp`) теперь
передаёт `FLAG_PERSON_GUN_OUT`. Act-path дополнительно гейтится на
`weapon_drawn` — когда оружие не достано, fire-detection падает в
threshold path, и R2 > fire_threshold даёт INPUT_MASK_PUNCH →
bare-hand melee через action tree.

**Почему не испортило I2 fix** (cooldown gate):
- Во время cooldown'а: `has_gun=true` (пистолет в руке, просто
  cooldown'ится) → `weapon_drawn=true` → act-path продолжает
  использоваться → mode=NONE → click не приходит → fire не
  происходит. I2 fix сохраняется.
- Bare-hand: `has_gun=false` → `weapon_drawn=false` → threshold path →
  R2 > 200 → fire работает. ✓

**Почему профиль `SPECIAL_NONE` не переделали в "bare hand":**
потенциально можно было бы разделить (SPECIAL_NONE для bare-hand,
SPECIAL_GUN для пистолета без SpecialUse), но это требует изменений
на стороне game code где все проверки "SpecialUse==null =&gt; pistol".
Параметр `weapon_drawn` проще и локальнее.

### Регрессия: gamepad fire rate меньше keyboard fire rate (2026-04-17)

При бешеном тапе по клавиатурной кнопке выстрел игра стреляет
заметно быстрее, чем при таком же тапе по R2 триггеру. Это неприемлемо
— управление должно ощущаться паритетно.

**Причина:** hardware Weapon25 мотор имеет физический click rate limit
~300мс (см. `dualsense_adaptive_trigger_facts.md`). Act-bit путь
fires только на hardware-сигнале щелчка, поэтому gamepad фактически
не мог стрелять чаще чем раз в ~300мс даже если игрок готов жать
быстрее. Плюс наш cooldown gate (600мс для пистолета) — но он в
основном лимитировал на игровом Timer1 который и так срабатывает.

Клавиатура идёт по `INPUT_MASK_PUNCH` напрямую каждый тик когда клавиша
зажата — rate ограничен только game Timer1 для running shot'а и
ничем для standing shot'а (`set_person_shoot` не проверяет Timer1,
полагается на state machine).

**Фикс: threshold fallback внутри act-path.**

Добавлена проверка: если act edge не сработал в этом тике И
адаптивный клик активен на железе (`gamepad_is_adaptive_click_active()`),
используется classic rising-edge threshold fire:
`r2 > fire_threshold && s_r2_armed` → fire. Это покрывает быстрые
тапы быстрее hardware rate limit — щелчка физически не будет
(motor не успевает), но выстрел срабатывает как на клавиатуре.

Для этого добавлена функция `gamepad_is_adaptive_click_active()` в
`gamepad.h/cpp` — возвращает true iff `s_trigger_mode == TRIGGER_MODE_AIM_GUN`.

**Почему не сломает I2 fix:** threshold fallback GATED на
`gamepad_is_adaptive_click_active()`. Во время cooldown'а mode=NONE,
fallback не запускается, fire не происходит. Сохраняется "нет клика,
нет выстрела" как обещали.

**Новое поведение:**
- Медленный тап (>300мс): act edge → fire с кликом. ✅
- Быстрый тап (<300мс, mode активен): hardware не щёлкает → act
  edge не срабатывает → **threshold fallback срабатывает** → fire
  без клика. Rate как у клавиатуры. ✅
- Нажатие во время cooldown'а: mode=NONE → neither act ни fallback
  не срабатывают → нет ни клика ни выстрела. ✅
- Bare-hand: `weapon_drawn=false` → весь act-path пропускается,
  обычный threshold path. ✅

**Проверка 2026-04-17 (после внедрения threshold fallback):** при быстром
нажатии выстрелы идут, но **тактильного клика не чувствуется**. Юзер:
"Надо чтобы были ощущения только в момент клика и чтобы при этом можно
было выстрелы получать так же часто как если долбишь по клаве максимально
быстро". То есть признать "нет клика при быстром тапе" — неприемлемо.

### ❌ Мотор-ресет оказался не нужен с своей либой (2026-04-17, после переосмысления)

После того как pulse_reset выпилили для проверки — клики работают и
**без** него: быстрые тапы дают клик каждый раз, независимо от темпа.

**Вывод:** ~300мс rate limit что мы намерили ранее был **артефактом
кривой vendored либы** (`dualsense-multiplatform`) — она слала
сломанные HID пакеты для Weapon25 packing'а, и железо реагировало
частично некорректно. После миграции на свой `libDualsense` с
правильным packing'ом мотор реагирует стабильно на каждое нажатие
без необходимости сбрасывать эффект.

Убрали соответственно: `gamepad_triggers_pulse_reset()`,
`gamepad_triggers_lockout()` (legacy blink который был leftover от
старой логики). Оставили cooldown-gate через `fire_cooldown_ms` в
профиле — нужен чтобы mode=NONE во время game cooldown'а (чтобы
нажатие во время Timer1 не давало клик без выстрела).

### Итерации 2026-04-17 — что пробовали и почему откатили

**Опробованные варианты и почему ни один cooldown gate не сработал:**

1. **600мс wall-clock gate на любой shot** — gamepad чувствовался
   медленнее клавиатуры на standing-still, клики нестабильные (mode
   токглится NONE↔AIM_GUN ровно пока игрок жмёт).
2. **100мс gate + pulse_reset** — клики на каждый выстрел работают,
   но оказалось что с своей libDualsense pulse_reset не нужен.
3. **STATE_MOVEING+Timer1>0 gate** (только для running): standing-still
   получил I2 обратно (mode=AIM_GUN во время STATE_GUN анимации →
   клики без выстрела); running получил shots без clicks "очень часто"
   — после Timer1 mode возвращается в AIM_GUN с HID latency, user
   жмёт в это окно → motor не успевает щёлкнуть → threshold fallback
   стреляет без клика.

**Вывод:** с корректным libDualsense (без кривых HID пакетов) mode
лучше держать **постоянно AIM_GUN** пока оружие готово. Любое toggle
NONE↔AIM_GUN вносит timing jitter который ломает синхронизацию клика
с нажатием.

### Текущее стабильное состояние (2026-04-18)

**Цель** — полная "click ⇔ shot" синхронизация: ни кликов без выстрелов,
ни выстрелов без кликов. Достигается тремя механизмами одновременно.

**1. Immediate mode=NONE на fire frame.** `weapon_feel_on_shot_fired`
вызывает `gamepad_triggers_lockout(0)` который сразу шлёт NONE HID
пакет (не ждёт следующий тик). Это закрывает окно HID-RTT (~30мс)
между выстрелом и тем как `gamepad_triggers_update` на step 5 тика
выставил бы NONE — иначе быстрый re-press в этом окне мог бы поймать
ещё активный мотор и щёлкнуть без выстрела.

**2. Running Timer1 gate с pre-release** (в `game.cpp`):
```cpp
const bool in_running_cooldown =
    (darci_t->State == STATE_MOVEING &&
     darci_t->Genus.Person->Timer1 > 15);
bool on_cooldown = in_running_cooldown || weapon_feel_is_in_fire_cooldown();
```
Пока игрок в STATE_MOVEING и Timer1 > 15 (≈ 1 тик до полного
истечения) — mode=NONE. Гейт отпускается за 1 тик до Timer1=0, чтобы
HID-пакет NONE→AIM_GUN успел пропагировать к моменту когда игра начнёт
принимать выстрелы. Без этого pre-release первое нажатие сразу после
Timer1=0 попадало на мотор в transitioning state → silent shot.

**3. Никакого threshold fallback в act-path.** Act-path теперь fires
**ТОЛЬКО** на hardware act-edge. Если мотор не щёлкнул (fast press
сквозь зону мимо семплирования, или mode=NONE) — выстрела нет. Это
гарантирует что каждый выстрел имеет тактильный клик. Проигрываем
скорость в редких случаях когда игрок жмёт быстрее чем hardware успевает
реагировать — выигрываем консистентный "click ⇔ shot".

**Что это даёт:**

- **Standing**: mode=AIM_GUN всегда (Timer1 gate не срабатывает т.к.
  state != STATE_MOVEING). Каждое нажатие → клик + выстрел. Скорость
  ограничена только тем как быстро игрок физически жмёт + hardware
  motor click rate.
- **Running + первый выстрел**: mode=AIM_GUN. Жмёшь → клик + выстрел →
  Timer1=140 стартует. `weapon_feel_on_shot_fired` моментально шлёт
  NONE (lockout), мотор гасится.
- **Running + в cooldown**: mode=NONE. Любое нажатие R2 не даёт ни
  клика ни выстрела. Тихое игнорирование как клавиатура.
- **Running + выход из cooldown**: за 1 тик до Timer1=0 gate отпускает,
  NONE→AIM_GUN HID летит ~30мс, мотор активируется к моменту когда
  Timer1=0 и игра готова. Первый press после этого → клик + выстрел.
  Цикл повторяется на running rate.

**`fire_cooldown_ms` в профиле** — оставлен как инструмент на случай
если конкретному оружию понадобится своя wall-clock задержка (post-1.0
custom weapons). Для всех текущих оружий 0.

**Открытые вопросы для тестирования:**
- Standing-still rapid tap — должен дать клик+выстрел на каждый press.
- Running rapid tap — должен дать клик+выстрел на Timer1-rate (~1.7/сек),
  никаких silent shots, никаких clicks-without-shot.
- Прыжок / падение / сцена — mode=NONE (non_firing_state), никаких кликов.

### (legacy) Мотор-ресет через кратковременное mode=NONE — неактуально

**Исторически** (когда ещё с vendored либой жили):

Юзер предложил: **форсить mode=NONE сразу после клика, через минимальное
время (~100мс) опять включать AIM_GUN**. Идея — hardware motor внутри
себя держит "cooldown state", и кратковременный сброс эффекта может
сбросить этот state быстрее чем natural ~300мс self-rearm.

**Что изменили:** `pistol.fire_cooldown_ms` с 600 → 100. После каждого
выстрела mode=NONE держится ~100мс, потом mode=AIM_GUN.

**Результат тестирования (2026-04-17):** **гипотеза подтвердилась**.
Юзер: "быстрые клики стали работать. даже при быстрых (насколько я могу
физически) кликах я чувствую физически клик. не увидел разницы между
бегом и стоянием".

Итоговое поведение:
- Каждый выстрел сопровождается физическим щелчком, включая при rapid tap'е.
- Rate сравним с клавиатурным маш'ингом.
- Опасение про I2 на running-shot (окно между 100мс gate и 583мс Timer1)
  **на практике не проявилось** — юзер не заметил разницы между running
  и standing. Возможные причины: (a) HID re-enable mode пропагирует с
  ~30мс задержкой, плюс игрок редко жмёт в коротком окне 100-583мс; (b)
  hardware motor после ресета перевзводится не мгновенно, что сглаживает
  timing; (c) просто редкий кейс.

Если I2 когда-то всплывёт на специфическом timing window — можно будет
поднять cooldown или сделать гибридный gate (running vs standing), но
пока 100мс эмпирически оптимально.

Threshold fallback внутри act-path остался как safety net — в норме теперь
каждый fire идёт через act edge, но если по какой-то причине мотор не
успеет щёлкнуть — fallback сработает (shot без click'а).

**Требует теста:**

- Пистолет rapid-tap во время и сразу после cooldown'а — не должно
  быть кликов во время cooldown'а.
- Пистолет: выстрел на бегу, остановка, жму снова — **должно
  работать** (это была регрессия первой попытки).
- Пистолет стоя — rapid tap не должен плодить стали клики пока
  анимация играется.
- 200мс достаточно? Если анимация пистолета >200мс, то после 200мс
  mode активируется, но game всё ещё в STATE_GUN → нажатие может
  дать click без выстрела. Возможно нужно увеличить до 300-400мс.
  Пробовать вживую.

### Про другие смежные баги

- **Стрельба на бегу не работала** (обнаружено в этой сессии):
  оказалось связано с `MIN_PRESS_DURATION` блокировавшем все
  пресс-ы. После удаления константы — работает. **Знаем**.
- **R3 weapon select inventory bug** (не про adaptive trigger):
  найдено что воспроизводится при подборе нового оружия пока popup
  уже открыт. Записано отдельно в known_issues_and_bugs.md.

---

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

### ✅ Выполнено в ходе сессии

- Рефакторинг: вся система per-weapon feel перенесена из
  `gamepad_haptic.cpp` (удалён) + вкраплений в `gamepad.cpp` /
  `input_actions.cpp` / `game.cpp` / `person.cpp` в единый модуль
  [`weapon_feel.{h,cpp}`](../new_game/src/engine/input/weapon_feel.cpp).
  Добавление кастомного оружия после 1.0 теперь = одна регистрация
  `WeaponFeelProfile` в `weapon_feel_init()`.
- Выход из тупика с перегруженной логикой (armed gate + wall-clock
  cooldowns + consumed flags + mode pre-enable) — откат к простейшей
  стабильной версии (unconditional mode + plain rising-edge).
- Подробный module manual написан в начале `weapon_feel.cpp` и
  аккумулированное расследование + чеклист требований здесь.
