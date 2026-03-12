# Состояния игрока и персонажей — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/statedef.h` — все STATE_* и SUB_STATE_* константы
- `fallen/Headers/Person.h` — поля State, SubState в PersonStruct
- `fallen/Source/interfac.cpp` — обработка ввода → переходы состояний

---

## 1. Главные состояния (STATE_*)

Из `statedef.h` (строки 5–45):

```c
#define STATE_INIT              0    // Инициализация объекта
#define STATE_NORMAL            1    // Стоит, ждёт (основное)
#define STATE_COLLISION         2    // Обработка коллизий
#define STATE_MOVEING           5    // Движение (ходьба/бег/бег в сторону)
#define STATE_IDLE              6    // Простой (перекрывается с STATE_DRIVING)
#define STATE_LANDING           7    // Приземление после прыжка
#define STATE_JUMPING           8    // Прыжок
#define STATE_FIGHTING          9    // Боевой режим
#define STATE_FALLING           10   // Падение вниз
#define STATE_USE_SCENERY       11   // Взаимодействие с объектом сцены
#define STATE_DOWN              12   // Лежит (сбит / скользит)
#define STATE_HIT               13   // Получает удар
#define STATE_CHANGE_LOCATION   14   // Вход/выход из транспорта или здания
#define STATE_DYING             16   // Анимация смерти
#define STATE_DEAD              17   // Труп (нет дальнейших анимаций)
#define STATE_DANGLING          18   // Висит на краю / зипвайре
#define STATE_CLIMB_LADDER      19   // На лестнице
#define STATE_CLIMBING          21   // Лазание по стене
#define STATE_GUN               22   // Пистолет достан
#define STATE_SHOOT             23   // Стрельба
#define STATE_DRIVING           24   // Водитель транспорта
#define STATE_NAVIGATING        25   // Движение по маршруту (AI)
#define STATE_WAIT              26   // Скриптованное ожидание
#define STATE_GRAPPLING         30   // Использует абордажный крюк
#define STATE_CANNING           32   // Держит банку/предмет
#define STATE_CIRCLING          33   // Кружение в бою
#define STATE_HUG_WALL          34   // Прижат к стене
#define STATE_SEARCH            35   // Обыскивает труп/зону
#define STATE_CARRY             36   // Несёт труп/объект (только Roper)
#define STATE_FLOAT             37   // В воде или левитация
```

---

## 2. Подсостояния движения (SUB_STATE_* — Movement)

```c
#define SUB_STATE_WALKING               1
#define SUB_STATE_RUNNING               2
#define SUB_STATE_SIDELING              3    // Шаг в сторону
#define SUB_STATE_WALKING_BACKWARDS     16
#define SUB_STATE_RUNNING_SKID_STOP     17   // Торможение после бега
#define SUB_STATE_SNEAKING              20   // Тихое крадение на корточках
#define SUB_STATE_CRAWLING              19   // Движение лёжа
#define SUB_STATE_SLIPPING              22   // Скольжение (лёд/вода)
#define SUB_STATE_RUNNING_VAULT         25   // Прыжок через препятствие
#define SUB_STATE_RUNNING_HIT_WALL      27   // Столкновение со стеной на бегу
```

---

## 3. Подсостояния прыжков (SUB_STATE_* — Jumping)

```c
#define SUB_STATE_STANDING_JUMP                30
#define SUB_STATE_STANDING_GRAB_JUMP           31   // Прыжок с висением
#define SUB_STATE_RUNNING_JUMP                 32
#define SUB_STATE_RUNNING_JUMP_FLY             33   // Фаза полёта
#define SUB_STATE_RUNNING_GRAB_JUMP            34
#define SUB_STATE_RUNNING_JUMP_LAND            35
#define SUB_STATE_STANDING_JUMP_FORWARDS       38
#define SUB_STATE_STANDING_JUMP_BACKWARDS      39
#define SUB_STATE_FLYING_KICK                  40   // Удар ногой в воздухе
#define SUB_STATE_FLYING_KICK_FALL             41
```

---

## 4. Подсостояния боя (SUB_STATE_* — Combat)

```c
#define SUB_STATE_PUNCH                 90
#define SUB_STATE_KICK                  91
#define SUB_STATE_BLOCK                 92
#define SUB_STATE_GRAPPLE               93   // Захватывает врага
#define SUB_STATE_GRAPPLEE              94   // Находится в захвате (жертва)
#define SUB_STATE_WALL_KICK             95   // Удар от стены
#define SUB_STATE_STEP                  96   // Маленький шаг
#define SUB_STATE_STEP_FORWARD          97
#define SUB_STATE_GRAPPLE_HOLD          98   // Удерживает захват
#define SUB_STATE_GRAPPLE_HELD          99   // Удерживается (жертва)
#define SUB_STATE_ESCAPE               100   // Вырывается из захвата
#define SUB_STATE_GRAPPLE_ATTACK       101   // Бьёт удерживаемого врага
#define SUB_STATE_HEADBUTT             102
#define SUB_STATE_STRANGLE             103
#define SUB_STATE_HEADBUTTV            104   // Вариант в транспорте
#define SUB_STATE_STRANGLEV            105   // Вариант в транспорте
```

**Кнопки во время захвата:**
- Атакующий: ACTION_KICK_FLAG / ACTION_PUNCH_FLAG
- Жертва: ACTION_BLOCK_FLAG → попытка вырваться

---

## 5. Подсостояния смерти (SUB_STATE_* — Death/Arrest)

```c
// Смерть:
#define SUB_STATE_DYING_INITIAL_ANI      230  // Начало анимации падения
#define SUB_STATE_DYING_FINAL_ANI        231  // Финальная анимация
#define SUB_STATE_DYING_ACTUALLY_DIE     232  // Переход в STATE_DEAD
#define SUB_STATE_DYING_KNOCK_DOWN       233  // Нелетальный нокдаун
#define SUB_STATE_DYING_GET_UP_AGAIN     234  // Подъём после нокдауна
#define SUB_STATE_DYING_PRONE            235  // Падение как снаряд (mid-animation)
#define SUB_STATE_DYING_KNOCK_DOWN_WAIT  236  // Ожидание перед подъёмом

// Арест (персонаж жив):
#define SUB_STATE_DEAD_ARREST_TURN_OVER  170  // Разворачивается к арестующему
#define SUB_STATE_DEAD_CUFFED            171  // Анимация надевания наручников
#define SUB_STATE_DEAD_ARRESTED          172  // Стоит со скованными руками
#define SUB_STATE_DEAD_INJURED           173  // Ранен (альтернативная анимация)
#define SUB_STATE_DEAD_RESPAWN           174  // Сброс состояния
```

**Разница арест vs смерть:**
- Арестованный: `FLAG_PERSON_ARRESTED` (бит 26) → может быть отпущен через EWAY
- Мёртвый: `STATE_DEAD` → необратимо

---

## 6. Подсостояния лазания (SUB_STATE_* — Climbing)

```c
// Уступ / карниз:
#define SUB_STATE_GRAB_TO_DANGLE         180  // Переход к висению
#define SUB_STATE_DANGLING               181  // Висит на краю
#define SUB_STATE_PULL_UP                182  // Подтягивается на уступ
#define SUB_STATE_DROP_DOWN              183  // Отпускается
#define SUB_STATE_DROP_DOWN_LAND         184  // Приземление после отпускания
#define SUB_STATE_DROP_DOWN_OFF_FACE     188  // Прыжок-падение со стены

// Зипвайр:
#define SUB_STATE_DANGLING_CABLE         185  // Висит на зипвайре
#define SUB_STATE_DANGLING_CABLE_FORWARD 186  // Движется по зипвайру вперёд
#define SUB_STATE_DANGLING_CABLE_BACKWARD 187 // Движется назад

// Лестница:
#define SUB_STATE_MOUNT_LADDER           192  // Хватается за лестницу
#define SUB_STATE_ON_LADDER              193  // Стоит на лестнице
#define SUB_STATE_CLIMB_UP_LADDER        194
#define SUB_STATE_CLIMB_DOWN_LADDER      195
#define SUB_STATE_CLIMB_OFF_LADDER_BOT   196  // Слезает снизу
#define SUB_STATE_CLIMB_OFF_LADDER_TOP   197  // Перелезает сверху

// Стена:
#define SUB_STATE_CLIMB_LANDING          210
#define SUB_STATE_CLIMB_AROUND_WALL      211
#define SUB_STATE_CLIMB_UP_WALL          212
#define SUB_STATE_CLIMB_DOWN_WALL        213
#define SUB_STATE_CLIMB_OVER_WALL        214
#define SUB_STATE_CLIMB_OFF_BOT_WALL     215
#define SUB_STATE_CLIMB_LEFT_WALL        217
#define SUB_STATE_CLIMB_RIGHT_WALL       218
```

---

## 7. Подсостояния транспорта и оружия

```c
// Транспорт:
#define SUB_STATE_ENTERING_VEHICLE       140
#define SUB_STATE_INSIDE_VEHICLE         141
#define SUB_STATE_EXITING_VEHICLE        142
#define SUB_STATE_MOUNTING_BIKE          145
#define SUB_STATE_RIDING_BIKE            146
#define SUB_STATE_DISMOUNTING_BIKE       147

// Оружие:
#define SUB_STATE_DRAW_GUN               220  // Достаёт пистолет
#define SUB_STATE_AIM_GUN                221  // Прицеливается
#define SUB_STATE_SHOOT_GUN              222  // Стреляет
#define SUB_STATE_GUN_AWAY               223  // Убирает пистолет
#define SUB_STATE_DRAW_ITEM              224  // Достаёт предмет
#define SUB_STATE_ITEM_AWAY              225  // Убирает предмет
```

---

## 8. Перенос трупа / арест (особые для Darci/Roper)

### Арест (только Darci)

Функция `find_arrestee()` (pcom.cpp:3083) находит ближайшего арестуемого.
`set_person_arrest()` — переводит цель в анимационную последовательность ареста.
EWAY триггер: `TT_PERSON_ARRESTED` — срабатывает когда персонаж надел наручники.

Флаг владельца: `FLAG_PERSON_ARRESTED` (PersonFlags бит 26).

### Перенос трупа (только Roper)

```c
// Состояния переноса:
#define SUB_STATE_PICKUP_CARRY      110  // Поднимает тело
#define SUB_STATE_DROP_CARRY        111  // Кладёт тело
#define SUB_STATE_STAND_CARRY       112  // Стоит с телом
#define SUB_STATE_PICKUP_CARRY_V    113  // Вариант для транспорта
#define SUB_STATE_DROP_CARRY_V      114
#define SUB_STATE_STAND_CARRY_V     115
#define SUB_STATE_CARRY_MOVE_V      116  // Движение с телом у транспорта
```

Флаг: `FLAG2_PERSON_CARRYING` (PersonFlags2 бит 7).
Поиск: `find_corpse()` — ближайший труп в радиусе взаимодействия.

### Абордажный крюк

```c
#define STATE_GRAPPLING              30  // Использует крюк

// Подсостояния крюка:
#define SUB_STATE_GRAPPLING_PICKUP   244  // Берёт крюк
#define SUB_STATE_GRAPPLING_WINDUP   245  // Раскручивает
#define SUB_STATE_GRAPPLING_RELEASE  246  // Бросает

// Состояния объекта-крюка (hook.h):
HOOK_STATE_STILL   (0)
HOOK_STATE_SPINNING (1)
HOOK_STATE_FLYING  (2)
```

---

## 9. Типы персонажей

```c
// Player.h — управляемые:
#define PLAYER_NONE     0
#define PLAYER_DARCI    1   // Женщина-полицейский, может арестовывать
#define PLAYER_ROPER    2   // Мужчина, может переносить тела
#define PLAYER_COP      3   // Дополнительный вариант
#define PLAYER_THUG     4   // Бандит (не используется в финале)

// Person.h — NPC:
#define PERSON_DARCI        0
#define PERSON_ROPER        1
#define PERSON_COP          2
#define PERSON_CIV          3   // Гражданский
#define PERSON_THUG_RASTA   4
#define PERSON_THUG_GREY    5
#define PERSON_THUG_RED     6
#define PERSON_SLAG_TART    7   // Женщина-гражданская
#define PERSON_SLAG_FATUGLY 8   // Полная женщина
#define PERSON_HOSTAGE      9
#define PERSON_MECHANIC    10
#define PERSON_TRAMP       11
#define PERSON_MIB1        12
#define PERSON_MIB2        13
#define PERSON_MIB3        14
```
