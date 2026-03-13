# AI — Структуры данных персонажей

**Связанные файлы:** [ai.md](ai.md) (PCOM поведение), [ai_behaviors.md](ai_behaviors.md) (детальные реализации)

**Ключевые файлы:**
- `fallen/Headers/pcom.h` — типы AI, состояния, флаги (~250 строк)
- `fallen/Headers/Person.h` — структура Person (~249 строк)

---

## 1. Структура Person

Расширение Thing для персонажей (NPC и игрок).

```c
typedef struct {
    // Базовые
    UBYTE Action;           // Текущее действие
    UBYTE SubAction;        // Подействие
    SWORD Health;           // Здоровье (0 = мёртв)

    // Цель и навигация
    THING_INDEX Target;     // Враг в поле зрения
    THING_INDEX InWay;      // Препятствие на пути
    THING_INDEX InCar;      // В какой машине
    THING_INDEX InBike;     // На каком байке
    UWORD NavIndex;         // Индекс текущей навигационной точки
    SWORD NavX, NavZ;       // Целевая позиция навигации
    SWORD TargetX, TargetZ; // Позиция цели

    // Инвентарь
    THING_INDEX SpecialList; // Список спецпредметов
    THING_INDEX SpecialUse;  // Используемый предмет
    THING_INDEX SpecialDraw; // Отображаемый предмет
    SWORD Hold;              // Удерживаемый объект
    UBYTE Ammo;              // Боеприпасы в пистолете

    // Характеристики
    UBYTE Mode;             // Режим движения (бег/ходьба/крадение)
    UBYTE Power;            // Сила удара (0–255)
    UBYTE Stamina;          // Выносливость
    UBYTE GotoSpeed;        // Скорость перемещения
    UBYTE FightRating;      // Навык боя + группировка
    SWORD Agression;        // Агрессивность

    // Позиция и ориентация
    UWORD HomeX, HomeZ;     // Домашняя позиция
    UWORD GotoX, GotoZ;     // Целевая позиция
    UBYTE HomeYaw;          // Домашнее направление
    UBYTE AttackAngle;      // Угол атаки (0–255)

    // AI система (высокоуровневая)
    UBYTE pcom_ai;          // Тип AI (PCOM_AI_*)
    UBYTE pcom_ai_state;    // Состояние AI (PCOM_AI_STATE_*)
    UBYTE pcom_ai_substate; // Подсостояние
    UWORD pcom_ai_counter;  // Таймер для AI
    UWORD pcom_ai_arg;      // Аргумент для AI
    UWORD pcom_ai_other;    // Вспомогательная переменная

    // AI система (движение)
    UBYTE pcom_move;          // Тип движения (PCOM_MOVE_*)
    UBYTE pcom_move_state;    // Состояние движения
    UBYTE pcom_move_substate; // Подсостояние
    UWORD pcom_move_counter;  // Таймер
    UWORD pcom_move_arg;      // Аргумент
    UWORD pcom_move_follow;   // Точка следования

    // Прочее
    UBYTE BurnIndex;        // Индекс pyro если горит
    UWORD UnderAttack;      // 0 = нет, иначе таймер
    UBYTE Ware;             // Индекс хранилища (если внутри)
    UBYTE pcom_zone;        // Зона патрулирования
    SBYTE CombatNode;       // Позиция в дереве боевых комбо
    GameCoord GunMuzzle;    // Позиция дула пистолета (PC)
} PersonStruct;
```

---

## 2. Типы персонажей (PERSON_*)

```c
PERSON_DARCI         (0)   // Главная героиня (игрок)
PERSON_ROPER         (1)   // Второй персонаж (игрок)
PERSON_COP           (2)   // Полицейский
PERSON_THUG_RASTA    (4)   // Головорез (растаман)
PERSON_THUG_GREY     (5)   // Головорез (серый)
PERSON_THUG_RED      (6)   // Головорез (красный)
PERSON_SLAG_TART     (7)   // Девушка
PERSON_SLAG_FATUGLY  (8)   // Толстая женщина
PERSON_HOSTAGE       (9)   // Заложник
PERSON_MECHANIC      (10)  // Механик
PERSON_TRAMP         (11)  // Бродяга
PERSON_MIB1/2/3      (12-14) // Men In Black
```

---

## 3. Флаги Person (FLAG_PERSON_*)

```c
FLAG_PERSON_NON_INT_M         // Не интерактивен (мужчина)
FLAG_PERSON_NON_INT_C         // Не интерактивен (женщина)
FLAG_PERSON_LOCK_ANIM_CHANGE  // Анимация заблокирована
FLAG_PERSON_GUN_OUT           // Пистолет выведен
FLAG_PERSON_DRIVING           // Управляет машиной
FLAG_PERSON_SLIDING           // Скользит (tackling)
FLAG_PERSON_NO_RETURN_TO_NORMAL // Не возвращается в нормальное состояние
FLAG_PERSON_HIT_WALL          // Столкнулся со стеной
FLAG_PERSON_USEABLE           // Можно взаимодействовать (клавиша U)
FLAG_PERSON_REQUEST_KICK      // AI запросила удар ногой
FLAG_PERSON_REQUEST_JUMP      // AI запросила прыжок
FLAG_PERSON_NAV_TO_KILL       // Навигирует к цели для убийства
FLAG_PERSON_ON_CABLE          // На кабеле
FLAG_PERSON_GRAPPLING         // Использует grappling hook
FLAG_PERSON_HELPLESS          // Не может выполнять AI команды
FLAG_PERSON_CANNING           // Держит банку
FLAG_PERSON_REQUEST_PUNCH     // AI запросила удар кулаком
FLAG_PERSON_REQUEST_BLOCK     // AI запросила блокировку
FLAG_PERSON_FALL_BACKWARDS    // Падает назад
FLAG_PERSON_BIKING            // Катается на байке
FLAG_PERSON_PASSENGER         // Пассажир в машине
FLAG_PERSON_HIT               // Был поражён
FLAG_PERSON_SPRINTING         // Спринт
FLAG_PERSON_FELON             // Разыскивается полицией
FLAG_PERSON_SEARCHED          // Обыскан полицией
FLAG_PERSON_ARRESTED          // Арестован
FLAG_PERSON_BARRELING         // Держит бочку
FLAG_PERSON_KO                // Выбит из сознания
FLAG_PERSON_WAREHOUSE         // Внутри склада

// Flags2:
FLAG2_PERSON_INVULNERABLE     // Неуязвим
FLAG2_PERSON_IS_MURDERER      // Убийца
FLAG2_PERSON_CARRYING         // Несёт объект
FLAG2_PERSON_GUILTY           // Виновен в преступлении
FLAG2_PERSON_FAKE_WANDER      // Притворяется гражданским (Rasta/Cop)
```
