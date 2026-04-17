# DualSense reference & working directory

Рабочая папка для всего что связано с DualSense: документация по
протоколу, эмпирические факты про железо, планы по переписыванию
своей DualSense-библиотеки, и локальные клоны референсных open-source
проектов для сверки packing'ов и layout'ов.

## Как работать с этой папкой

Вся документация внутри этой папки — **single source of truth** по
DualSense для проекта. Если что-то связано с DualSense протоколом,
либой, или adaptive trigger эффектами — читай и пиши здесь, не в
[`../`](../).

В `../` остаётся только [`weapon_haptic_and_adaptive_trigger.md`](../weapon_haptic_and_adaptive_trigger.md) —
потому что это документ про **игровую weapon систему** (как OpenChaos
использует haptic + trigger), а не про сам DualSense протокол.

## Навигация (по приоритету для чистой сессии)

### 🎯 Если пришёл писать свою DualSense либу
→ начни с [`own_dualsense_lib_plan.md`](own_dualsense_lib_plan.md) —
это **self-contained план** с архитектурой, пошаговыми фазами,
ссылками на все референсы и критериями готовности.

### 📖 Если нужна справка по DualSense протоколу
→ [`dualsense_protocol_reference.md`](dualsense_protocol_reference.md) —
общая справка по USB/BT transport, input/output report layout,
trigger effect modes, статус реализации в вендоренной либе.

### 🔬 Если нужны эмпирические факты про железо
→ [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md) —
измеренные hardware константы (click rate limit, feedback offsets),
таблица значений state nybble, cross-platform validation status.

### 🛠 Если готовишь PR в rafaelvaloto/Dualsense-Multiplatform
→ [`dualsense_lib_pr_notes.md`](dualsense_lib_pr_notes.md) —
подробное описание двух PR (feedback status reading + Weapon25
packing rewrite), аудит всех сломанных эффектов в либе, готовые
patches с пояснениями.

### 📦 Если надо скачать референсные либы
→ [`libs_contents.md`](libs_contents.md) — точные инструкции что
и откуда качать, куда класть. `libs/` внутри этой папки под
gitignore и содержит все клоны.

## Структура папки

```
dualsense_libs_reference/
├── README.md                            -- этот файл (навигация)
├── libs_contents.md                     -- инструкция по скачиванию libs/
├── own_dualsense_lib_plan.md            -- план переписывания на свою либу
├── dualsense_protocol_reference.md      -- справка по DualSense протоколу
├── dualsense_adaptive_trigger_facts.md  -- эмпирические факты про железо
├── dualsense_lib_pr_notes.md            -- PR notes для апстрима
├── .gitignore                           -- игнорит libs/
└── libs/                                -- [GITIGNORED] клоны референсных репо
    ├── duaLib/                          -- ⭐ главный источник (Nielk1, MIT)
    ├── DualSense-Windows/               -- ⭐ input parser reference (DS5W)
    ├── nondebug-dualsense/              -- HID offsets documentation
    ├── rafaelvaloto-pristine/           -- чистая копия вендоренной либы
    ├── rafaelvaloto-patched/            -- наш патченый форк (копия из new_game/libs/)
    ├── nielk1_trigger_gist.txt          -- backup gist
    └── dogtopus_hid_descriptor.txt      -- backup HID descriptor
```

## Связь с остальным проектом

**Игровой код и либа**:
- Текущая вендоренная (патченая) DualSense либа — [`../../new_game/libs/dualsense-multiplatform/`](../../new_game/libs/dualsense-multiplatform/)
  (маркеры правок `OPENCHAOS-PATCH`)
- Наш bridge layer между игрой и либой — [`../../new_game/src/engine/platform/ds_bridge.cpp`](../../new_game/src/engine/platform/ds_bridge.cpp)
- Использование триггеров в игре — [`../../new_game/src/engine/input/gamepad.cpp`](../../new_game/src/engine/input/gamepad.cpp) и [`../../new_game/src/engine/input/weapon_feel.cpp`](../../new_game/src/engine/input/weapon_feel.cpp)

**Смежная документация в devlog**:
- [`../weapon_haptic_and_adaptive_trigger.md`](../weapon_haptic_and_adaptive_trigger.md) —
  история отладки adaptive trigger в игре, план использования
  `act` bit для fire detection

**Тестовый инструмент** (только в ветке `dualsence_test`):
- `../../new_game/src/engine/input/weapon_feel_test.cpp` — observation
  mode для валидации feedback signals и тактильных ощущений.
  Активируется `\` или L3 клик.
