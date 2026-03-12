# Оружие и предметы — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/Special.cpp` — логика спецпредметов и оружия
- `fallen/Headers/Special.h` — структуры и константы
- `fallen/Source/Person.cpp` — инвентарь персонажа
- `fallen/Headers/Person.h` — SpecialList инвентарь

---

## 1. Типы предметов (SPECIAL_*)

30 типов специальных объектов:

| Категория | Типы |
|-----------|------|
| Огнестрельное | `SPECIAL_PISTOL`, `SPECIAL_SHOTGUN`, `SPECIAL_AK47`, `SPECIAL_SNIPER`, `SPECIAL_SILENCED_PISTOL` |
| Взрывчатка | `SPECIAL_GRENADE`, `SPECIAL_MOLOTOV`, `SPECIAL_PROXIMITY_MINE`, `SPECIAL_TIMED_MINE` |
| Ближний бой | `SPECIAL_KNIFE`, `SPECIAL_CROWBAR`, `SPECIAL_BASEBALL_BAT` |
| Специальное | `SPECIAL_GRAPPLE` — абордажный крюк, `SPECIAL_FIRE_EXTINGUISHER` |
| Предметы | `SPECIAL_HEALTH_PACK`, `SPECIAL_ARMOR`, `SPECIAL_KEY`, `SPECIAL_HANDCUFFS` |
| Машины | `SPECIAL_CAR_KEYS` |
| Разное | `SPECIAL_BRIEFCASE`, `SPECIAL_DRUG_PACKAGE`, `SPECIAL_EVIDENCE` |

---

## 2. Структура Special (предмет)

```c
struct Special {
    UBYTE  Type;         // SPECIAL_* тип
    UBYTE  Flags;        // Флаги состояния
    UWORD  Ammo;         // Текущий боезапас
    UWORD  MaxAmmo;      // Максимальный боезапас
    SLONG  X, Y, Z;      // Позиция (если лежит на земле)
    UWORD  Owner;        // THING_INDEX владельца (0 = никто)
    UWORD  NextInList;   // Следующий в инвентаре (linked list)
};
```

---

## 3. Дефолтный боезапас

| Оружие | Начальный | Максимум |
|--------|-----------|---------|
| Pistol | 15 | 60 |
| Shotgun | 8 | 32 |
| Grenade | 3 | 6 |
| AK47 | 30 | 120 |
| Sniper | 5 | 20 |
| Molotov | 2 | 4 |

---

## 4. Скорость стрельбы (в кадрах, 30fps)

| Оружие | Задержка кадров |
|--------|----------------|
| Pistol | 10 (0.33 сек) |
| Shotgun | 20 (0.67 сек) |
| AK47 | 5 (0.17 сек) — полуавтомат |
| Sniper | 30 (1.0 сек) |

---

## 5. Система гранат

```c
// Максимум одновременно активных гранат:
MAX_ACTIVE_GRENADES = 6

// Таймер до взрыва:
GRENADE_TIMER = 0x1F00  // ~8000 тиков = ~6 секунд при 30fps

// Радиус взрыва:
GRENADE_RADIUS = 512    // world units

// Урон в эпицентре:
GRENADE_DAMAGE_MAX = 100

// Граната — это CLASS_SPECIAL объект с:
//   State = STATE_JUMPING (пока летит)
//   DY = начальная вертикальная скорость
//   DogPoo1 = обратный отсчёт таймера
```

---

## 6. Инвентарь персонажа (SpecialList)

Инвентарь реализован как односвязный список внутри PersonStruct:

```c
struct PersonStruct {
    // ...
    UWORD SpecialList;     // Голова linked list предметов (THING_INDEX)
    UWORD ActiveSpecial;   // Текущий активный предмет
    // ...
};
```

**Операции:**
- `PERSON_pickup_item(person, item)` — добавить в список
- `PERSON_drop_item(person, item)` — убрать из списка, положить на землю
- `PERSON_use_item(person)` — использовать ActiveSpecial

---

## 7. Оружие как Thing

Оружие, лежащее на земле — это `Thing` с `Class = CLASS_SPECIAL`.
При подборе:
1. `Thing` удаляется из мира (`free_primary_thing`)
2. Создаётся запись в `SpecialList` персонажа
3. `Owner` устанавливается в THING_INDEX персонажа

---

## 8. Абордажный крюк (SPECIAL_GRAPPLE)

```c
// Особый предмет с отдельной логикой:
// - При использовании создаёт CLASS_PROJECTILE (крюк летит)
// - При попадании в здание (FLAGS_CABLE_BUILDING) — крепится
// - Персонаж переходит в STATE_GRAPPLING
// - Подтягивание: DY += GRAPPLE_PULL_FORCE каждый кадр
// - Максимальная длина троса: ~2048 world units
```

---

## 9. Огнетушитель (SPECIAL_FIRE_EXTINGUISHER)

```c
// При использовании около горящего объекта (FLAGS_BURNING):
// - Гасит огонь (убирает FLAG_BURNING)
// - Создаёт облако дыма (спрайт-эффект)
// Критично для некоторых миссий
```

---

## 10. Наручники (SPECIAL_HANDCUFFS)

```c
// Арест врага:
// - Персонаж должен быть в STATE_DOWN (лежит)
// - При применении враг переходит в PERSON_ARRESTED
// - Очки за арест > очки за убийство (игровая механика)
```

---

## 11. Что переносить в новую версию

**Переносить 1:1:**
- Все SPECIAL_* типы и их поведение
- SpecialList инвентарь (linked list в Person)
- Таймеры и параметры оружия (для совместимости с балансом)
- Гранатную систему (таймер, радиус, MAX_ACTIVE_GRENADES)
- Логику ареста (наручники + STATE_DOWN)
- Абордажный крюк (STATE_GRAPPLING)

**Можно улучшить:**
- Инвентарь → modern container (но сохранить семантику)
- Параметры оружия → data-driven (JSON/XML вместо хардкода)
