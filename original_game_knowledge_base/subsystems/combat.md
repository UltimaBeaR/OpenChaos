# Боевая система (Combat)

## Обзор

Вся боевая система основана на **анимационных данных**, а не на отдельном физическом движке. Коллизия удара определяется через структуру `GameFightCol`, встроенную в кадры анимации. Отбрасывание — через выбор анимации, а не через векторы скорости.

Файлы: `fallen/Source/Combat.cpp`, `fallen/Editor/Headers/Anim.h`

## GameFightCol — зона удара

Встроена в `GameKeyFrame` (каждый кадр анимации может содержать данные удара):

```cpp
struct GameFightCol {
    UWORD Dist1;        // внутренняя граница дистанции удара
    UWORD Dist2;        // внешняя граница дистанции удара
    UBYTE Angle;        // угол центра зоны удара (в 1/256 круга)
    UBYTE Priority;     // приоритет удара (не используется в Combat.cpp)
    UBYTE Damage;       // базовый урон
    UBYTE Tween;        // смещение кадра анимации
    UBYTE AngleHitFrom; // угол относительно жертвы (0-255 = полный круг)
    UBYTE Height;       // высота: 0=низкая(ноги), 1=средняя(торс), 2=высокая(голова)
    UBYTE Width;        // радиус/ширина зоны в пространстве коллизий
    UBYTE Dummy;        // выравнивание
    struct GameFightCol *Next; // несколько зон удара на один кадр (linked list)
};
```

## Алгоритм определения попадания

Функция `check_combat_hit_with_person()`:

1. **Предварительные проверки:** жертва не мертва; если KO — только атака ANIM_FIGHT_STOMP
2. **Дистанция:** QDIST2 от таза атакующего до таза жертвы (только XZ плоскость), `abs(dy) > 100` = промах
3. **Угол:** конвертация в 2048-градусную систему, допуск ±200 единиц (FIGHT_ANGLE_RANGE = 400)
4. **Видимость (melee):** LOS-проверка через `there_is_a_los_things()` с флагами игнорирования забора и примов
5. **Финальная проверка дистанции:** dist < Dist2 + 0x30 (COMBAT_HIT_DIST_LEEWAY)

**Стомп (топтание лежащего):** особый случай — проверяется позиция правой ступни (SUB_OBJECT_RIGHT_FOOT) против 3 точек тела жертвы (таз, голова, правая голень), дистанция < 0x53.

## Формула урона

```
damage = fight->Damage  // базовый урон из анимации
```

**Модификаторы по типу атаки (полная таблица из кода):**
- KICK_NAD: +50 (мужчины) / +10 (женщины) + taunt_prob=75%
- KICK_RIGHT/LEFT: +30
- KICK_NEAR: damage=40 (заменяет, не прибавляет)
- BAT_HIT1/HIT2: +20 + taunt_prob+=10
- FLYKICK варианты: +20 + taunt_prob+=10
- KICK_BEHIND: +20
- KNIFE_ATTACK1/2/3: damage=30/50/70 (заменяет)
- STOMP: +50

**Модификаторы атакующего:**
- Roper стреляет: урон ×2; Roper ближний бой: +20
- Игрок стреляет: +player.Skill; Игрок ближний бой: +player.Strength
- Всегда: +GET_SKILL(attacker)

**Защита жертвы:**
```
if жертва — игрок: damage >>= 1  (половина урона)
else: damage -= GET_SKILL(victim), минимум 2
```

**Блок** (SUB_STATE_BLOCK или STEP_FORWARD + ANIM_FIGHT_STEP_S): только против melee (type==0 или type > HIT_TYPE_GUN_SHOT_L); guns игнорируют блок → damage = 0.

**AI вероятность блока:**
- Base: `60 + GET_SKILL(person) * 12`
- Если атакующий не виден: вероятность `/= 2`
- Cap: `150 + GET_SKILL(person) * 5`
- Финал: `Random() % cap < block_prob` → блок

**Применение:**
```
if !(FLAG2_PERSON_INVULNERABLE): victim.Health -= damage
victim.Agression -= damage  (жертва отступает)
attacker.Agression += damage  (атакующий воодушевляется)
```

## Отбрасывание (Knockback)

**Не физика!** Реализовано через выбор анимации:

```cpp
UBYTE take_hit[7][2] = {
    {ANIM_KD_FRONT_LOW,  1},  // 0: спереди-низко (нокдаун)
    {ANIM_HIT_FRONT_MID, 0},  // 1: спереди-средне (отдача)
    {ANIM_HIT_FRONT_HI,  0},  // 2: спереди-высоко
    {ANIM_KD_BACK_LOW,   1},  // 3: сзади-низко (нокдаун)
    {ANIM_HIT_BACK_MID,  0},  // 4: сзади-средне
    {ANIM_HIT_BACK_HI,   0},  // 5: сзади-высоко
    {0, 0}                     // 6: не используется
};
```

Выбор: `behind` (проверяется через dot product), `Height` из GameFightCol → индекс в таблице.

**KO** (нокаут, от combo3/combo3b): `set_person_dead(PERSON_DEATH_TYPE_STAY_ALIVE, ...)`, `Agression = -60`.
**Height == 0** (подсечка): `sweep_feet()`.
**Иначе:** `set_person_recoil()` с нужной анимацией.

MIB-персонажи (PERSON_MIB1/2/3) не могут быть нокаутированы. **Исключение:** PCOM_AI_FIGHT_TEST (тренировочные манекены) могут умирать от комбо даже если MIB/invulnerable.

## Fight Tree — дерево комбо

Дерево переходов атак в зависимости от ввода (структура массива `fight_tree[][10]`):
- Колонки: `[0]=Anim, [1]=Finish, [2-5]=NextState(P1/P2/K1/K2), [6]=NextJump, [7]=NextBlock, [8]=Damage, [9]=HitType`
- Punch combo: PUNCH_COMBO1(dmg 10) → COMBO2(30) → COMBO3(60) — KO
- Kick combo: KICK_COMBO1(10) → COMBO2(30) → COMBO3(60) — KO
- Cross-combo финишеры: 30, 80, 80 (nodes 11-13)
- Knife: 30 → 60 → 80 (nodes 14, 16, 18)
- Bat: 60 → 90 (nodes 19, 21)

## Граплинг

```cpp
struct Grapples {
    UWORD Anim;    // анимация
    SWORD Dist;    // желаемая дистанция
    SWORD Range;   // допуск по дистанции
    SWORD Angle;   // угол
    SWORD DAngle;  // допуск по углу
    SWORD Peep;    // тип персонажа (1=Darci, 2=Roper)
};

// Доступные граплинги:
{ANIM_PISTOL_WHIP, 75, 65, 1024, 0, 1}  // удар пистолетом (только Darci)
{ANIM_STRANGLE,    50, 20,    0, 0, 2}  // удушение
{ANIM_HEADBUTT,    65, 20,    0, 0, 2}  // удар головой
{ANIM_GRAB_ARM,    60, 20,    0, 0, 1}  // захват руки
```

Проверка попадания граплинга (`check_combat_grapple_with_person()`):
- Дистанция: `|dist - pg->Dist| < pg->Range`
- Y-разница: < 50
- Жертва смотрит в ту же сторону (угол 160–1888 = промах)
- Жертва не в граплинге, не KO, жива

Смерть от граплинга: SNAP_KNECK → мгновенная смерть (ANIM_DIE_KNECK).
Strangle/Headbutt: жертва может вырваться (счётчик Escape).

## Дальний бой vs ближний

**Melee (GameFightCol):** требует LOS через стены.
**Ranged:** типы HIT_TYPE_GUN_SHOT_H/M/L, _PISTOL/_SHOTGUN/_AK47. Roper: урон ×2. Нет LOS-проверки. Нет нокдауна стоя (кроме особых условий). Не блокируется обычным блоком.

## Правила атаки (faction rules)

Функция `people_allowed_to_hit_each_other()`:
- DARCI: не бьёт копов/Roper без вины
- ROPER: не бьёт Darci/копов
- COP: только меченых преступников/виновных
- THUGS: бьют разные гангстерские цвета, не своих
- MIBs: не бьют других MIB

**Gang attack tracking** (до 8 атакующих с 8 компасных направлений):
```cpp
struct GangAttack { UWORD Owner; UWORD Perp[8]; UWORD LastUsed; UBYTE Index; UBYTE Count; };
```

## Взрыв (Shockwave)

`create_shockwave()` в Collide.cpp:
- Урон с затуханием: `hitpoints = maxdamage * (radius - dist) / radius`
- Нокдаун если hitpoints > 30
- Прыгающие персонажи: урон ×0.5
- Вызывает `knock_person_down(person, damage, x, z, attacker)`
- Ищет до 16 сущностей в радиусе (SHOCKWAVE_FIND = 16)

## Ключевые константы

| Константа             | Значение | Назначение                     |
|-----------------------|----------|-------------------------------|
| FIGHT_ANGLE_RANGE     | 400      | Допуск угла удара (±200 ед.) |
| COMBAT_HIT_DIST_LEEWAY | 0x30    | Допуск дистанции              |
| SHOCKWAVE_FIND        | 16       | Макс. сущностей в взрыве     |
| Минимальный урон      | 2        | Нижняя граница               |
| Макс. здоровье        | 200      | Стандарт для большинства     |

## Важно для переноса

- **Нет физики отбрасывания** — только анимации. В новой игре сохранить этот подход.
- Урон зашит в анимационные данные (GameFightCol.Damage) — при переносе надо читать из .all файлов.
- Блок работает только против melee — это геймплейное решение, не упростить.
- KO у MIB принципиально заблокирован — важная балансная особенность.
