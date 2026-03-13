# Оружие и предметы — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Special.cpp` — вся логика предметов (~41K строк)
- `fallen/Headers/Special.h` — структуры и константы
- `fallen/Source/guns.cpp` — прицеливание и дальность стрельбы
- `fallen/Headers/Person.h` — инвентарь (SpecialList, ammo_packs_*)
- `fallen/Source/grenade.cpp` — физика гранат (DIRT-система)
- `fallen/Source/dirt.cpp` — DIRT-система: листья, банки колы, гильзы, кровь, голубятники (отключены)
- `fallen/Headers/dirt.h` — DIRT типы, лимиты, API

---

## 1. Типы предметов (SPECIAL_*)

Полный список из `Special.h` строки 19–49:

```c
#define SPECIAL_NONE             0
#define SPECIAL_KEY              1   // Ключ (квест-предмет)
#define SPECIAL_GUN              2   // Пистолет
#define SPECIAL_HEALTH           3   // Аптечка (+100 здоровья)
#define SPECIAL_BOMB             4   // Бомба
#define SPECIAL_SHOTGUN          5   // Дробовик
#define SPECIAL_KNIFE            6   // Нож (ближний бой)
#define SPECIAL_EXPLOSIVES       7   // Взрывчатка (закладывается)
#define SPECIAL_GRENADE          8   // Граната (бросается)
#define SPECIAL_AK47             9   // АК-47
#define SPECIAL_MINE            10   // Мина (нельзя подобрать — заглушена)
#define SPECIAL_THERMODROID     11   // Термодроид (тип неизвестен, см. ниже)
#define SPECIAL_BASEBALLBAT     12   // Бейсбольная бита (ближний бой)
#define SPECIAL_AMMO_PISTOL     13   // Патроны для пистолета
#define SPECIAL_AMMO_SHOTGUN    14   // Патроны для дробовика
#define SPECIAL_AMMO_AK47       15   // Патроны для АК-47
#define SPECIAL_KEYCARD         16   // Карточка-ключ (квест)
#define SPECIAL_FILE            17   // Файл/досье (квест)
#define SPECIAL_FLOPPY_DISK     18   // Дискета (квест)
#define SPECIAL_CROWBAR         19   // Монтировка (инструмент)
#define SPECIAL_VIDEO           20   // Видеокассета (квест)
#define SPECIAL_GLOVES          21   // Перчатки
#define SPECIAL_WEEDAWAY        22   // Weed Away (специальный предмет)
#define SPECIAL_TREASURE        23   // Сокровище/значок (бонус к характеристикам)
#define SPECIAL_CARKEY_RED      24   // Ключ от машины (красной)
#define SPECIAL_CARKEY_BLUE     25   // Ключ от машины (синей)
#define SPECIAL_CARKEY_GREEN    26   // Ключ от машины (зелёной)
#define SPECIAL_CARKEY_BLACK    27   // Ключ от машины (чёрной)
#define SPECIAL_CARKEY_WHITE    28   // Ключ от машины (белой)
#define SPECIAL_WIRE_CUTTER     29   // Кусачки для проволоки
#define SPECIAL_NUM_TYPES       30
```

**SPECIAL_THERMODROID (11):** Нет поведения на земле (break; в special_normal). Не функционален ни в пре-релизе ни в финале. Не переносить.

**Чего НЕТ в финальной игре (несмотря на популярные мифы):**
- Нет `SPECIAL_SNIPER` (снайперской винтовки)
- Нет `SPECIAL_SILENCED_PISTOL`
- Нет `SPECIAL_GRAPPLE` — абордажный крюк не реализован как Special-предмет (анимации есть, предмета нет)
- Нет `SPECIAL_HANDCUFFS` — наручники не реализованы как Special-предмет
- Нет `SPECIAL_FIRE_EXTINGUISHER`

---

## 2. Группы предметов

```c
#define SPECIAL_GROUP_USEFUL             1   // Полезные (ключи, инструменты)
#define SPECIAL_GROUP_ONEHANDED_WEAPON   2   // Одноручное оружие (пистолет, нож, граната)
#define SPECIAL_GROUP_TWOHANDED_WEAPON   3   // Двуручное оружие (дробовик, АК-47, бита)
#define SPECIAL_GROUP_STRANGE            4   // Странные предметы
#define SPECIAL_GROUP_AMMO               5   // Патроны
#define SPECIAL_GROUP_COOKIE             6
```

**Ограничение:** одновременно можно нести только одно двуручное оружие. `person_has_twohanded_weapon()` (Special.cpp:225) проверяет наличие перед подбором.

---

## 3. Структура Special

Из `Special.h` строки 79–106:

```c
typedef struct {
    COMMON(SpecialType)           // Class, State, OldState, SubState макрос

    THING_INDEX NextSpecial;      // Следующий предмет в инвентаре (linked list)
    THING_INDEX OwnerThing;       // Владелец (персонаж, несущий предмет)

    UWORD ammo;                   // Боезапас в магазине ИЛИ счётчик для активных предметов
    UWORD waypoint;               // Waypoint создавший этот предмет (для EWAY) ИЛИ
                                  // DIRT_dirt индекс для активированной мины
    UWORD counter;                // Счётчик вращения (анимация лежащего предмета)
    UWORD timer;                  // Таймер взрывателя (граната: запал, взрывчатка: обратный отсчёт)
} Special;
```

---

## 4. Субсостояния (SubState)

```c
#define SPECIAL_SUBSTATE_NONE          0   // Нормальное состояние (лежит на земле или в руке)
#define SPECIAL_SUBSTATE_ACTIVATED     1   // Активировано (граната с запалом, взрывчатка)
#define SPECIAL_SUBSTATE_IS_DIRT       2   // Граната в DIRT-физике (катится по земле)
#define SPECIAL_SUBSTATE_PROJECTILE    3   // В полёте (граната брошена вверх)
```

---

## 5. Инвентарь персонажа

### Поля Person (Person.h строки 123–125)
```c
THING_INDEX SpecialList;      // Голова linked list всех предметов
THING_INDEX SpecialUse;       // Текущее активное оружие
THING_INDEX SpecialDraw;      // Предмет в процессе извлечения/уборки
```

### Резервный боезапас (Person.h строки 226–231)
```c
UBYTE ammo_packs_pistol;      // Резерв патронов для пистолета (0–255)
UBYTE ammo_packs_shotgun;     // Резерв патронов для дробовика (0–255)
UBYTE ammo_packs_ak47;        // Резерв патронов для АК-47 (0–255)
UBYTE Ammo;                   // Текущий магазин пистолета (0–15)
```

---

## 6. Боезапас оружия

Вместимость магазина (Special.h строки 8–11):

```c
#define SPECIAL_AMMO_IN_A_PISTOL   15
#define SPECIAL_AMMO_IN_A_SHOTGUN   8
#define SPECIAL_AMMO_IN_A_GRENADE   3   // гранат в стаке при подборе
#define SPECIAL_AMMO_IN_A_AK47     30
```

Начальный боезапас при создании предмета (Special.cpp строки 1610–1621):
- Пистолет: 15 патронов
- Дробовик: 8 патронов
- АК-47: 30 патронов
- Граната: 3 штуки
- Взрывчатка: 1 штука
- Нож, бита: 0 (бесконечно)

Максимум стака:
- Гранаты: 8 (нельзя подобрать если уже 8)
- Взрывчатка: 4 (нельзя подобрать если уже 4)
- Резерв патронов: 255 (UBYTE)

---

## 7. Параметры стрельбы (guns.cpp)

`get_gun_aim_stats()` (guns.cpp:16) возвращает дальность и разброс:

| Оружие | Дальность (блоки) | Разброс |
|--------|------------------|---------|
| MIB (встроенный) | 8 | 20 |
| Пистолет | 7 | 15 |
| АК-47 | 8 | 20 |
| Дробовик | 6 | 30 |

Хиттипы от оружия (combat.h строки 9–21):
```c
HIT_TYPE_GUN_SHOT_PISTOL   = 10
HIT_TYPE_GUN_SHOT_SHOTGUN  = 11
HIT_TYPE_GUN_SHOT_AK47     = 12
```

### Урон оружия (Special.cpp:471+):
| Оружие | Урон | Примечание |
|--------|------|-----------|
| Пистолет | 70 | |
| Дробовик | 300 - dist | Saturated 0-250; max range ~0x600 |
| АК-47 (игрок) | 100 | |
| АК-47 (NPC) | 40 | |
| Все guns vs игрок | damage >>= 1 | Половина для баланса |

### Скорострельность (тики между выстрелами):
| Оружие | Тики | ~секунды (30fps) |
|--------|------|-----------------|
| Пистолет | 140 | ~4.7с |
| Дробовик | 400 | ~13.3с |
| АК-47 | 64 | ~2.1с |
| MIB АК-47 | 64 | unlimited ammo |

### Hit chance формула:
```
base = 230 - abs(Roll)>>1 - Velocity
+ 64 (если игрок целится)
- 64 (AI стреляет в игрока)
+ 100 (AI vs AI)
- dist>>3 (pistol/AK, если dist > 0x400)
- dist>>2 (shotgun, если dist > 0x400)
/ 2 (если цель делает flip/dodge)
Skill >= 7: random dodge roll
Min: 20/256 (~8%, никогда чистый промах)
```

### Параметры взрывов (сравнительная таблица):
| Тип | Радиус | Max урон | Aggressor |
|-----|--------|----------|-----------|
| Mine | 0x200 | 250 | NULL |
| Grenade | 0x300 | 500 | thrower |
| C4 | 0x500 | 500 | placer |
| Bomb | нет shockwave | 0 | visual only |

Формула: `hitpoints = maxdamage × (radius - dist) / radius`
Knockdown: hitpoints > 30. Jumping/flipping: hitpoints /= 2 + 1.

---

**Физика гранат и автоприцеливание:** → [weapons_items_details.md](weapons_items_details.md) (секции 8, 8a)

---

## 9. Взрывчатка (SPECIAL_EXPLOSIVES)

`SPECIAL_set_explosives()` (Special.cpp:1781) — закладка:
- Отнимает 1 от стака, создаёт новый Special на позиции игрока
- Активирует: `SubState = ACTIVATED`, `timer = 16 * 20 * 5` (**5 секунд**)
- **Баг в комментарии:** комментарий в коде говорит "10 second fuse" — но формула `16*20*5 = 1600` тиков, при 20 fps логики = **ровно 5 секунд**. Финальная игра также 5 секунд.

Обратный отсчёт (Special.cpp строки 992–1050):
- Тикает со скоростью 0x10 за кадр
- При `timer <= 0x10` → взрыв:
  - Шоквейв с силой 500, радиус 0x500
  - Эффект огня (pyro)

---

## 10. Логика подбора предметов

`should_person_get_item()` (Special.cpp:237) определяет нужен ли предмет:

| Предмет | Условие подбора |
|---------|----------------|
| Пистолет | Нет пистолета, ИЛИ есть но ammo_packs < 240 |
| Дробовик | Нет, ИЛИ есть но ammo_packs < 247 |
| АК-47 | Нет, ИЛИ есть но ammo_packs < 225 |
| Нож / Бита | Нет такого оружия |
| Аптечка | Здоровье < максимума |
| Граната | Нет, ИЛИ < 8 штук |
| Взрывчатка | Нет, ИЛИ < 4 штук |
| Патроны | Есть соответствующее оружие + есть место |
| Бомба / Мина | **Никогда** (возвращает FALSE) |
| Остальное | Всегда |

`person_get_item()` (Special.cpp:355) выполняет подбор:
1. Объединяет боезапас — переполнение уходит в `ammo_packs_*`
2. Если подобрано новое оружие — сразу экипирует
3. Аптечка: +100 здоровья (до максимума)
4. Прочие: добавляет в linked list инвентаря

---

## 11. Выпадение предметов

`special_drop()` (Special.cpp:110):
- Убирает из linked list персонажа
- Вызывает `find_nice_place_near_person()` для позиционирования
- Ставит `Velocity = 5` (нельзя подобрать 5 тиков — защита от мгновенного переподбора)
- **Только PLAYER-dropped weapons** рандомизируются:
  - Дробовик: `(Random()&1)+2` = 2-3 патрона
  - АК-47: `(Random()&7)+6` = 6-13 патронов
- Enemy-dropped: сохраняют оригинальный ammo (без рандомизации)
- Cooldown после дропа: `counter > 16×20` (~1.25с) до pickup

---

## 12. Глобальные лимиты

```c
MAX_SPECIALS     = 260   // RMAX_SPECIALS — максимум объектов-предметов в мире
```

При переполнении — вытеснение (hijack) менее важных предметов:
- Вытесняются первыми: патроны, бита, нож (score 4000+)
- Вытесняются вторыми: пакеты патронов (score 3000+)
- Защищены от вытеснения: ключи, quest-предметы

---

## 13. Лежащие предметы — поведение

`special_normal()` (Special.cpp:802) каждый кадр:
1. Счётчик вращения: `counter += 32` → вращение предмета лёжа
2. Если `SUBSTATE_PROJECTILE` → физика полёта (гранаты)
3. Если предмет в руке — следует позиции владельца
4. Если активировано и есть таймер → взрыв по истечении

---

## 14. Ключевые функции

| Функция | Файл:строка | Назначение |
|---------|-------------|------------|
| `alloc_special()` | Special.cpp:1452 | Создать предмет в мире |
| `free_special()` | Special.cpp:1668 | Уничтожить предмет |
| `special_pickup()` | Special.cpp:93 | Добавить в инвентарь |
| `special_drop()` | Special.cpp:110 | Выбросить из инвентаря |
| `person_get_item()` | Special.cpp:355 | Обработать подбор |
| `should_person_get_item()` | Special.cpp:237 | Нужен ли предмет |
| `person_has_special()` | Special.cpp:1682 | Найти предмет в инвентаре |
| `SPECIAL_prime_grenade()` | Special.cpp:1734 | Активировать гранату (6 сек) |
| `SPECIAL_throw_grenade()` | Special.cpp:1701 | Бросить гранату |
| `SPECIAL_set_explosives()` | Special.cpp:1781 | Заложить взрывчатку (5 сек) |
| `special_activate_mine()` | Special.cpp:746 | Взрыв мины |
| `person_has_twohanded_weapon()` | Special.cpp:225 | Проверить двуручное |
| `get_gun_aim_stats()` | guns.cpp:16 | Дальность и разброс оружия |
| `calc_target_score_new()` | guns.cpp:212 | Scoring цели (для игрока) |
| `find_target_new()` | guns.cpp:595 | Поиск лучшей цели (sphere) |
| `find_snipe_target()` | guns.cpp:766 | Поиск цели для NPC-снайпера |
| `CreateGrenadeFromPerson()` | grenade.cpp:105 | Бросок гранаты |
| `ProcessGrenade()` | grenade.cpp:208 | Физика гранаты per-frame |
| `CreateGrenadeExplosion()` | grenade.cpp:326 | Взрыв гранаты |
| `show_grenade_path()` | grenade.cpp:441 | Прицел траектории |

---

## 15. Интеграция с EWAY

EWAY-действие `EWAY_DO_CREATE_ITEM` (тип 4) создаёт предмет:
- `arg1` = тип предмета (SPECIAL_*)
- `arg2` = waypoint создателя
- Поле `waypoint` в Special ссылается на создавший waypoint → EWAY может отслеживать факт подбора

---

**Банки колы (DIRT система), DIRT типы, физика банки:** → [weapons_items_details.md](weapons_items_details.md) (секция 16)

