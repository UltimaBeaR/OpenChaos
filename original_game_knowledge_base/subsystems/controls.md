# Управление и ввод — Urban Chaos

**Ключевые файлы:** `fallen/Headers/interfac.h`, `fallen/Source/interfac.cpp`, `fallen/Headers/Player.h`, `fallen/Headers/animate.h`

---

## 1. Структура Player

```c
// fallen/Headers/Player.h
typedef struct {
    COMMON(PlayerType)              // Type: PLAYER_DARCI, PLAYER_ROPER и т.д.

    ULONG  Input;                   // Текущее состояние ввода (bitmask)
    ULONG  InputDone;               // Флаг: ввод обработан в этом тике
    UWORD  PlayerID;                // ID игрока (0 или 1)
    UBYTE  Stamina;                 // Выносливость (0–255)
    UBYTE  Constitution;            // Стойкость/защита

    ULONG  LastInput;               // Ввод предыдущего тика
    ULONG  ThisInput;               // Ввод текущего тика
    ULONG  Pressed;                 // Кнопки, нажатые именно в этом тике
    ULONG  Released;                // Кнопки, отпущенные в этом тике
    ULONG  DoneSomething;           // Флаг: действие выполнено

    SLONG  LastReleased[16];        // GetTickCount() последнего отпускания (double-click)
    UBYTE  DoubleClick[16];         // Счётчик двойных нажатий на кнопку

    UBYTE  Strength;                // Сила удара
    UBYTE  RedMarks;                // Нарушения закона (10 = game over)
    UBYTE  TrafficViolations;       // Нарушения ПДД
    UBYTE  Danger;                  // Уровень угрозы: 0=нет, 1=макс, 3=мин

    UBYTE  PopupFade;               // Прозрачность попапа инвентаря
    SBYTE  ItemFocus;               // Выбранный предмет (-1 = ничего)
    UBYTE  ItemCount;               // Количество предметов в инвентаре
    UBYTE  Skill;                   // Уровень навыка

    struct Thing *CameraThing;      // Объект камеры
    struct Thing *PlayerPerson;     // Объект персонажа игрока
} Player;
```

**MAX_PLAYERS = 2** (split-screen поддерживается)

**Типы игроков:**
- `PLAYER_NONE` (0)
- `PLAYER_DARCI` (1) — основной персонаж
- `PLAYER_ROPER` (2)
- `PLAYER_COP` (3)
- `PLAYER_THUG` (4)

---

## 2. Кнопки управления (INPUT_*)

Определены в `interfac.h` как индексы (0–17):

```c
#define INPUT_FORWARDS      0   // Вперёд
#define INPUT_BACKWARDS     1   // Назад
#define INPUT_LEFT          2   // Влево
#define INPUT_RIGHT         3   // Вправо
#define INPUT_START         4   // Пауза/меню
#define INPUT_CANCEL        5   // Отмена/назад
#define INPUT_PUNCH         6   // Удар кулаком
#define INPUT_KICK          7   // Удар ногой
#define INPUT_ACTION        8   // Взаимодействие/использование
#define INPUT_JUMP          9   // Прыжок/подъём
#define INPUT_CAMERA        10  // Смена режима камеры
#define INPUT_CAM_LEFT      11  // Поворот камеры влево
#define INPUT_CAM_RIGHT     12  // Поворот камеры вправо
#define INPUT_CAM_BEHIND    13  // Камера за спину
#define INPUT_MOVE          14  // Бег/спринт (модификатор)
#define INPUT_SELECT        15  // Инвентарь/выбор
#define INPUT_STEP_LEFT     16  // Уклон влево
#define INPUT_STEP_RIGHT    17  // Уклон вправо
```

**Маски (bitmask-версии):**
```c
#define INPUT_MASK_FORWARDS     (1 << 0)
#define INPUT_MASK_BACKWARDS    (1 << 1)
// ... и т.д. для всех 18 кнопок
#define INPUT_MASK_ALL_BUTTONS  (0x3ffff)   // биты 0–17
```

**Флаги типа ввода (hardware-level):**
```c
#define INPUT_TYPE_KEY      (1<<0)   // Клавиатура
#define INPUT_TYPE_JOY      (1<<1)   // Джойстик/геймпад
#define INPUT_TYPE_GONEDOWN (1<<6)   // Только вновь нажатые кнопки
```

---

## 3. Маппинг кнопок (PC, по умолчанию)

**Джойстик (геймпад):**
```c
#define JOYPAD_BUTTON_KICK       0
#define JOYPAD_BUTTON_PUNCH      1
#define JOYPAD_BUTTON_JUMP       2
#define JOYPAD_BUTTON_ACTION     3
#define JOYPAD_BUTTON_MOVE       4   // Бег
#define JOYPAD_BUTTON_START      5
#define JOYPAD_BUTTON_SELECT     6
#define JOYPAD_BUTTON_CAMERA     7
#define JOYPAD_BUTTON_CAM_LEFT   8
#define JOYPAD_BUTTON_CAM_RIGHT  9
#define JOYPAD_BUTTON_1STPERSON  10  // Вид от первого лица
```

**Клавиатура (коды DirectInput):**
| Клавиша | Код | Действие |
|---------|-----|----------|
| Стрелка влево | 203 | INPUT_LEFT |
| Стрелка вправо | 205 | INPUT_RIGHT |
| Стрелка вверх | 200 | INPUT_FORWARDS |
| Стрелка вниз | 208 | INPUT_BACKWARDS |
| Z | 44 | PUNCH |
| X | 45 | KICK |
| C | 46 | ACTION |
| V | 47 | MOVE (бег) |
| Пробел | 57 | JUMP |
| Tab | 15 | START (пауза) |
| Enter | 28 | SELECT |
| Page Down | 207 | CAMERA |
| End | 211 | CAM_LEFT |
| Page Up | 209 | CAM_RIGHT |
| A | 30 | 1ST_PERSON |

Маппинг хранится в `config.ini` секции `[Keyboard]` / `[Joypad]` и загружается при старте.

---

## 4. Обработка ввода (пайплайн)

**Основная функция:** `interfac.cpp::process_hardware_level_input_for_player(Thing *p_player)` (line 7732)

**Порядок обработки каждого тика:**

1. **Получение сырого ввода:** `PACKET_DATA(player_id)` — сырое состояние кнопок
2. **Вычисление изменений:**
   ```c
   pl->LastInput  = pl->ThisInput;
   pl->ThisInput  = input;
   pl->Pressed    = pl->ThisInput & ~pl->LastInput;   // только что нажатые
   pl->Released   = ~pl->ThisInput & pl->LastInput;   // только что отпущенные
   ```
3. **Double-click:** для каждой из 16 кнопок: если нажата в течение 200ms после отпускания (через `LastReleased[i]` / GetTickCount()) — инкремент `DoubleClick[i]`
4. **Управление камерой:** `INPUT_MASK_CAMERA` / `CAM_BEHIND` / `CAM_LEFT` / `CAM_RIGHT`
5. **Движение:** `player_apply_move(Thing *p_thing, ULONG input)` — обрабатывает направление с учётом `Mode`
6. **Бой и действия:** `apply_button_input(Thing *p_player, Thing *p_person, ULONG input)` — см. ниже
7. **В машине:** `apply_button_input_car(Thing *p_furn, ULONG input)` — если `FLAG_PERSON_DRIVING`
8. **Вид от первого лица:** `apply_button_input_first_person(...)` — отдельная функция

### apply_button_input() — полный флоу (interfac.cpp:4382)

**Шаг 1 — ACTION кнопка:**
- Если `INPUT_MASK_ACTION` нажат → `do_an_action(p_person, input)` (взаимодействие с миром)
- Если НЕТ ACTION → SPRINT→RUN downgrade; если crouching → `set_person_idle_uncroutch()`

**Шаг 2 — блокировка:**
- Если STATE_CARRY и substate PICKUP_CARRY/DROP_CARRY → ранний return (анимация не прерывается)

**Шаг 3 — action tree** (`find_best_action_from_tree()` → switch по ACTION_*):
```
ACTION_KICK_FLAG      → FLAG_PERSON_REQUEST_KICK
ACTION_PUNCH_FLAG     → FLAG_PERSON_REQUEST_PUNCH
ACTION_BLOCK_FLAG     → FLAG_PERSON_REQUEST_BLOCK
ACTION_JUMP_FLAG      → FLAG_PERSON_REQUEST_JUMP
ACTION_HUG_LEFT/RIGHT → set_person_hug_wall_dir(0/1)
ACTION_UNSIT          → set_person_unsit()
ACTION_FLIP_LEFT/RIGHT → set_person_flip(0/1) [только если FrameIndex<3 при STANDING_JUMP]
ACTION_SHOOT          → set_player_shoot(p_person, 0)
ACTION_GUN_AWAY       → set_person_gun_away()
ACTION_DROP_DOWN      → set_person_drop_down(OFF_FACE) [если DEATH_SLIDE: MFX_stop zipwire сначала]
ACTION_RESPAWN        → processed (мёртвый код — реальный respawn закомментирован)
```

Только если НЕТ флагов NON_INT_M/NON_INT_C:
```
ACTION_SKID            → Velocity>25 → ANIM_SLIDER_START + MFX_SLIDE_START (looped)
ACTION_STANDING_JUMP   → should_i_jump() → вперёд/назад(backflip)/влево(flip)/вправо(flip)/нейтральный
ACTION_RUNNING_JUMP    → should_i_jump() → set_person_running_jump()
ACTION_TRAVERSE_LEFT/RIGHT → set_person_traverse(0/1)
ACTION_PULL_UP         → set_person_pulling_up()
ACTION_FIGHT_KICK      → если BACKWARDS: kick назад (dir=2, turn_to_target DONT_TURN); иначе: turn_to_target_and_kick()
ACTION_FIGHT_PUNCH     → если пистолет: set_person_shoot(); иначе: player_activate_in_hand() || turn_to_target_and_punch()
ACTION_DRAW_SPECIAL    → set_person_draw_special()
```

**Шаг 4 — движение** (если `INPUT_MOVEMENT_MASK` или мышь):
- Если НЕТ NON_INT_M — `player_interface_move()` для: IDLE, MOVEING, GUN, HIT_RECOIL, CARRY, HUG_WALL, CLIMBING, CLIMB_LADDER, DANGLING, JUMPING
- GRAPPLING → только если substate GRAPPLING_WINDUP
- NON_INT_M и ACTION_SIT_BENCH + FORWARDS → `set_person_idle()` (встать со скамейки)

**Шаг 4b — нет движения:**
- STATE_IDLE → `player_turn_left_right(p_person, 0)` (только поворот без движения)
- STATE_MOVEING:
  - SUB_STATE_CRAWLING → `set_person_idle_croutch()` (остановиться сидя)
  - иначе → `player_stop_move()` (торможение)

**Аналоговые стики (геймпад):**
```c
#define GET_JOYX(input)  (((input>>17)&0xfe)-128)   // X: биты 17–24
#define GET_JOYY(input)  (((input>>24)&0xfe)-128)   // Y: биты 24–31
```
Ось X/Y кодируется прямо в старших битах поля `Input`.

---

## 5. Режимы персонажа (PERSON_MODE_*)

```c
#define PERSON_MODE_RUN     0  // Бег (можно спринтовать)
#define PERSON_MODE_WALK    1  // Ходьба
#define PERSON_MODE_SNEAK   2  // Скрытность
#define PERSON_MODE_FIGHT   3  // Боевой режим (особая камера)
#define PERSON_MODE_SPRINT  4  // Спринт
#define PERSON_MODE_NUMBER  5
```

---

## 6. Действия персонажа (ACTION_*)

Определены в `animate.h` (строки 688–740):

### Движение
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_IDLE` | 0 | Стоит |
| `ACTION_WALK` | 1 | Ходьба |
| `ACTION_RUN` | 2 | Бег |
| `ACTION_WALK_BACK` | 34 | Ходьба назад |
| `ACTION_CRAWLING` | 41 | Ползком/пригнувшись |
| `ACTION_STOPPING` | 30 | Торможение |
| `ACTION_SKID` | 42 | Скользящий удар (при беге) |
| `ACTION_CROUTCH` | 40 | Присесть/спрятаться |

### Прыжки и подъёмы
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_STANDING_JUMP` | 3 | Прыжок с места |
| `ACTION_STANDING_JUMP_GRAB` | 4 | Прыжок с захватом края |
| `ACTION_RUNNING_JUMP` | 5 | Прыжок при беге |
| `ACTION_DANGLING` | 6 | Свисает с края |
| `ACTION_PULL_UP` | 7 | Подтягивается на край |
| `ACTION_GRABBING_LEDGE` | 9 | Хватается за край |
| `ACTION_DROP_DOWN` | 10 | Спрыгивает |
| `ACTION_FALLING` | 11 | Падение |
| `ACTION_LANDING` | 12 | Приземление |
| `ACTION_CLIMBING` | 13 | Карабкание по лестнице/стене |
| `ACTION_DANGLING_CABLE` | 17 | Свисает на кошке |
| `ACTION_TRAVERSE_LEFT` | 36 | Движение влево в висе |
| `ACTION_TRAVERSE_RIGHT` | 37 | Движение вправо в висе |

### Бой
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_FIGHT_IDLE` | 16 | Боевая стойка |
| `ACTION_FIGHT_PUNCH` | 14 | Удар кулаком |
| `ACTION_FIGHT_KICK` | 15 | Удар ногой |
| `ACTION_GRAPPLE` | 43 | Захват |
| `ACTION_GRAPPLEE` | 44 | В захвате |
| `ACTION_HOP_BACK` | 18 | Уклон назад |
| `ACTION_SIDE_STEP` | 19 | Уклон в сторону |
| `ACTION_FLIP_LEFT` | 27 | Кувырок влево |
| `ACTION_FLIP_RIGHT` | 28 | Кувырок вправо |
| `ACTION_BLOCK_FLAG` | 39 | Блок/защита (флаг в цепи) |
| `ACTION_PUNCH_FLAG` | 38 | Флаг комбо-удара |
| `ACTION_KICK_FLAG` | 32 | Флаг удара ногой |

### Оружие
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_DRAW_SPECIAL` | 21 | Достать оружие |
| `ACTION_AIM_GUN` | 22 | Прицелиться |
| `ACTION_SHOOT` | 23 | Выстрел |
| `ACTION_GUN_AWAY` | 24 | Убрать оружие |

### Zipwire / тросы
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_DANGLING_CABLE` | 17 | Висит на тросе |
| `ACTION_TRAVERSE_LEFT` | 36 | Движение влево по тросу |
| `ACTION_TRAVERSE_RIGHT` | 37 | Движение вправо по тросу |
| `ACTION_DEATH_SLIDE` | 35 | Скольжение вниз по тросу |

**Подсистема zipwire** (`interact.cpp`, `Person.cpp::person_death_slide()`):
- Тросы — часть геометрии уровня (`STOREY_TYPE_CABLE = 9`), рендерятся в `facet.cpp::cable_draw()`
- `FLAG_PERSON_ON_CABLE (1<<12)` — игрок на тросе
- Sub-states: `SUB_STATE_DANGLING_CABLE` (185), `_FORWARD` (186), `_BACKWARD` (187), `SUB_STATE_DEATH_SLIDE` (189)
- Анимации: `ANIM_WIRE_SLIDE_HANG` (247), `ANIM_WIRE_SLIDE_PULLUP` (246)
- Звук: `S_ZIPWIRE` — цикличный, pitch = 224 + velocity/4
- `find_cable_y_along()` — высота троса в точке (учитывает провисание)
- `get_cable_along()` — позиция вдоль троса (0–4096), отступ от концов: 4–252/256
- Ввод на тросе: Вперёд/Move = подтянуться, Назад = отпуститься, Влево/Вправо = траверс
- **Подтверждено в финальной игре** (первый уровень, полицейская тренировочная полоса)
- В этой кодовой базе (пре-релиз) код захвата закомментирован — это артефакт

### Транспорт и окружение
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_ENTER_VEHICLE` | 45 | Садится в машину |
| `ACTION_INSIDE_VEHICLE` | 46 | Едет в машине |
| `ACTION_SIT_BENCH` | 47 | Сидит на скамейке |
| `ACTION_HUG_WALL` | 48 | Прижимается к стене |
| `ACTION_HUG_LEFT` | 49 | Прижимается к левому углу |
| `ACTION_HUG_RIGHT` | 50 | Прижимается к правому углу |
| `ACTION_UNSIT` | 51 | Встаёт |

### Смерть и восстановление
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_DYING` | 20 | Анимация смерти |
| `ACTION_DEAD` | 26 | Мёртв |
| `ACTION_DEATH_SLIDE` | 35 | Скользит по земле после удара |
| `ACTION_RESPAWN` | 25 | Возрождение |

### Прочее
| Константа | Значение | Описание |
|-----------|----------|----------|
| `ACTION_STAND_RELAX` | 8 | Расслабленная стойка |
| `ACTION_RUN_JUMP` | 31 | Альтернативный прыжок при беге |

---

## 7. Флаги персонажа (FLAG_PERSON_*)

Критические флаги структуры Person (`Flags` поле):

```c
// Внутренние/AI флаги:
FLAG_PERSON_NON_INT_M   (1<<0)   // Не прерывать анимацию движения
FLAG_PERSON_NON_INT_C   (1<<1)   // Не прерывать анимацию боя
FLAG_PERSON_LOCK_ANIM_CHANGE (1<<2) // Блок смены анимации
FLAG_PERSON_GUN_OUT     (1<<3)   // Оружие наготове
FLAG_PERSON_DRIVING     (1<<4)   // Ведёт машину
FLAG_PERSON_SLIDING     (1<<5)   // Скользящий удар
FLAG_PERSON_NO_RETURN_TO_NORMAL (1<<6) // Не возвращаться в STATE_NORMAL
FLAG_PERSON_HIT_WALL    (1<<7)   // Врезался в стену
FLAG_PERSON_USEABLE     (1<<8)   // Можно взаимодействовать
FLAG_PERSON_REQUEST_KICK  (1<<9)  // AI: запросил удар ногой
FLAG_PERSON_REQUEST_JUMP  (1<<10) // AI: запросил прыжок
FLAG_PERSON_NAV_TO_KILL   (1<<11) // AI: движется чтобы убить
// bit 12 = FLAG_PERSON_ON_CABLE (zipwire)
FLAG_PERSON_GRAPPLING   (1<<13)  // Держит кошку (grappling hook)
FLAG_PERSON_HELPLESS    (1<<14)  // Недееспособен
FLAG_PERSON_CANNING     (1<<15)  // Держит банку с колой
FLAG_PERSON_REQUEST_PUNCH (1<<16) // AI: запросил удар кулаком
FLAG_PERSON_REQUEST_BLOCK (1<<17) // AI: запросил блок
FLAG_PERSON_FALL_BACKWARDS (1<<18) // Падает назад
FLAG_PERSON_BIKING      (1<<19)  // Едет на велосипеде (незавершённая фича)
FLAG_PERSON_PASSENGER   (1<<20)  // Пассажир в машине
FLAG_PERSON_HIT         (1<<21)  // Только что получил удар
FLAG_PERSON_SPRINTING   (1<<22)  // Бежит спринтом
FLAG_PERSON_FELON       (1<<23)  // Разыскивается полицией
FLAG_PERSON_PEEING      (1<<24)  // Анимация мочеиспускания (NPC idle)
FLAG_PERSON_SEARCHED    (1<<25)  // Уже обыскан
FLAG_PERSON_ARRESTED    (1<<26)  // Под арестом
FLAG_PERSON_BARRELING   (1<<27)  // Держит бочку
FLAG_PERSON_MOVE_ANGLETO (1<<28) // AI: движется к углу
FLAG_PERSON_WAREHOUSE   (1<<30)  // Находится в складском здании
FLAG_PERSON_KILL_WITH_A_PURPOSE (1<<31) // AI: целенаправленное убийство
```

---

## 8. Здоровье: игрок vs NPC

**NPC:** одно поле `Person.Health` (SWORD, −32768..32767) — уменьшается от урона

**Игрок:** те же `Person.Health` + дополнительные поля в Player:
- `Stamina` — энергия для бега и действий (отдельно от здоровья)
- `Constitution` — защита
- `Strength` — сила атаки
- `RedMarks` — счётчик нарушений закона: **10 нарушений = немедленный GS_LEVEL_LOST** (независимо от здоровья!)
- `Danger` (0–3) — уровень обнаружения/угрозы

