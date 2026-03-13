# Персонажи — детали реализации

**Связанный файл:** [characters.md](characters.md) (анимационная система, архитектура)

---

## 10. Специфика по типам персонажей (из анализа исходников)

### Таблица здоровья

| Тип | PERSON_TYPE | Health | AnimType | MeshID | Статус кода |
|-----|-------------|--------|----------|--------|------------|
| Darci | 0 | 200 | DARCI(0) | 0 | Полная реализация |
| Roper | 1 | **400** | ROPER(2) | 0 | fn_roper_normal() **пустая** |
| Cop | 2 | 200 | CIV(1) | 4 | fn_cop_normal() **#if 0** |
| CIV | 3 | 130 | CIV(1) | 7 | через PCOM |
| Thug (все) | 4-6 | 200 | CIV(1) | 0-2 | **ASSERT(0)** в init |
| Slag/Hostage/etc | 7-11 | 200 | DARCI(0) | 1-3 | через PCOM |
| MIB 1/2/3 | 12-14 | **700** | CIV(1) | 5 | через PCOM |

### Darci (PERSON_DARCI = 0) — единственный полностью реализованный

- **Падение:** `DY < -30000` → мгновенная смерть; `DY < -20000` → `damage = (-DY - 20000) / 100`
- Звуки падения: PCOM_SOUND_DROP (низкое), DROP_MED, DROP_BIG; ScreamFallSound при смерти
- Физика на крышах складов: скольжение по WARE_inside()
- Коллизия с заборами: обнаружение + лазание
- Гравитация: 4<<8 = 1024 ед/тик, терминальная скорость: -30000
- Анимация при инициализации: `ANIM_STAND_READY`

### Roper (PERSON_ROPER = 1) — почти пуст

- Health = **400** (в 2× больше Darci)
- `fn_roper_normal()` — **полностью пустая функция**, всё поведение через общий Person.cpp
- Начальная скорость: 10 единиц
- Roper: урон огнестрельным оружием ×2, ближний бой +20

### Cop (PERSON_COP = 2) — отключён в пре-релизе

- `fn_cop_normal()` — **обёрнута в `#if 0`** (весь код закомментирован)
- Внутри: патрулирование по вэйпойнтам, угловая интерполяция (сдвиг >>4), случайное idle (50%)
- Система звуковых оповещений: PCOM_SOUND_GUNSHOT(6) > FIGHT(5) > ALARM(4) > HEY(3) > UNUSUAL(2) > FOOTSTEP(1)
- AI-типы: PCOM_AI_COP(5), PCOM_AI_COP_DRIVER(14), PCOM_AI_ARREST(20)

### Thug (PERSON_THUG_* = 4-6) — сломан в пре-релизе

- `fn_thug_init()` содержит **ASSERT(0)** — инициализация упадёт
- **В финальной игре**: `people_functions[]` маппит PERSON_THUG_* → `cop_states`, поэтому запускается `fn_cop_init()` (не fn_thug_init). fn_thug_init никогда не вызывается в рабочем коде.

### Архитектурное подтверждение (people_functions[] таблица)

```cpp
// Person.cpp
GenusFunctions people_functions[] = {
    { PERSON_DARCI,          darci_states },
    { PERSON_ROPER,          roper_states },
    { PERSON_COP,            cop_states   },
    { PERSON_CIV,            cop_states   },
    { PERSON_THUG_RASTA,     cop_states   },  // НЕ thug_states!
    { PERSON_THUG_GREY,      cop_states   },
    { PERSON_THUG_RED,       cop_states   },
    { PERSON_SLAG_TART,      cop_states   },
    // ... все остальные → cop_states
    { PERSON_MIB1/2/3,       cop_states   },
};
```

**Итог:**
- Все NPC кроме Darci/Roper → `fn_cop_init()` (реально работает), потом STATE_NORMAL = `fn_cop_normal()` (#if 0, мёртвый)
- Весь AI через PCOM_process_person, state functions = заглушки после init
- `fn_roper_normal()` пуста — Roper поведение только через PCOM
- `fn_thug_init()` с ASSERT(0) — НИКОГДА не вызывается (thug_states не в people_functions!)

### Canid/Dog — сломан в пре-релизе

- Большинство функций содержат **ASSERT(0)**
- Архитектура: CANID_SUBSTATE_SLEEP(1), PROWL(2), CHASE(3), FLEE(4), BARK(5)
- Радиус обнаружения: 0xd0 (~208 ед.), угол зрения: 256 единиц дуги
- **Не переносить** (не было в финальной игре — Mike Diskett подтверждает)

### SubClass.cpp — НЕ система подклассов персонажей

Это инструмент **редактора** (`DeleteCivs()`, `DeleteCars()`, `save_prim_map()` и т.д.). Не влияет на gameplay.

### AnimBank — загрузка анимаций

```cpp
// global_anim_array[4][450] — индекс AnimType × AnimID
load_anim_system(&game_chunk[0], "data\\darci1.all");
load_anim_system(&game_chunk[1], "data\\hero.all");      // Roper
load_anim_system(&game_chunk[3], "data\\bossprtg.all");  // CIV
append_anim_system(&game_chunk[1], "police1.all", 200, 0);  // доп. анимации копа
append_anim_system(&game_chunk[3], "newciv.all", CIV_M_START, 1);
```

Darci заимствует некоторые анимации CIV (например, вход в такси).

---

## 11. CMatrix33 — сжатый формат матриц анимации

### Структуры (Editor/Headers/Prim.h):

```c
struct CMatrix33 {
    SLONG M[3];  // 3 упакованных 32-bit слова, не 3×3 float!
};
// Каждый M[row] = (elem0 << 22) | (elem1 << 12) | (elem2 << 2)
// 10-bit элементы, range -512..511 (маски: CMAT0_MASK=0x3ff00000, CMAT1_MASK=0x000ffc00, CMAT2_MASK=0x000003ff)
```

**ВНИМАНИЕ:** В DDEngine/Headers/Quaternion.h есть своя `class FloatMatrix { float M[3][3]; }` — это ДРУГАЯ структура, только для рендерера.

### ULTRA_COMPRESSED_ANIMATIONS — хранилище (8 байт на элемент):

```c
struct GameKeyFrameElement {
    SBYTE m00, m01;  // 2 из 3 значений строки 0 (scale=128)
    SBYTE m10, m11;  // 2 из 3 значений строки 1
    SBYTE OffsetX, OffsetY, OffsetZ;  // Смещение кости
    UBYTE Pad;       // Битовые поля для декомпрессии
};
```

### Декомпрессия GetCMatrix():

```c
// UCA_Lookup[a][b] = Root(16383 - a² - b²)  -- pre-computed table (128×128 entries)
// Scale: значения в диапазоне [-127,127] где 127 ≈ 1.0 (scale factor = 128)

void GetCMatrix(GameKeyFrameElement *e, CMatrix33 *cm) {
    c = UCA_Lookup[a][b];         // c = sqrt(1 - a² - b²) в той же шкале
    if (Pad & 1) c = -c;          // знак c для строки 0
    // Pad bits 2-3 = позиция c в строке 0: 0→col0, 4→col2, 8→col1
    // Pad bits 4-5 = позиция c в строке 1: 0→col0, 16→col2, 32→col1
    // Строка 2 = cross product строк 0 и 1 (с >>7 для деления на scale)
    m20 = (m01*m12 >> 7) - (m02*m11 >> 7)  // clamp [-127,127]
    cm->M[0] = (m00 << 22) | (m01 << 12) | (m02 << 2)
    // ...
}
```

**InterruptFrame = МЁРТВЫЙ КОД**: поле объявлено в DrawTween, но везде `InterruptFrame = 0`. Нигде не присваивается ненулевое значение.

---

## 12. Физика Darci (Darci.cpp — 1431 строк)

`projectile_move_thing()` — основной цикл airborne-физики (576 строк).

**Гравитация и падение:**
- GRAVITY = `(4<<8)` = 1024 units/tick
- Terminal velocity: DY = -30000 (hard clamp)
- **Fall damage формула:** `damage = (-DY - 20000) / 100` (линейная, НЕ экспоненциальная)
- DY ≤ -30000 → damage = 250 (instant kill); DY ≥ -20000 → damage = 0

**Death plunge (спецанимация падения):**
- Player порог: DY < -12000; NPC порог: DY < -6000
- Lookahead: проверяет 3 шага вперёд; если нет пола в 0x600 units → ANIM_PLUNGE_START
- Velocity обнуляется при plunge

**Velocity scaling:** все velocity inputs: `(vel*3)>>2` (×0.75, конвертация FPS)
- `change_velocity_to()`: half-step per frame (asymptotic)
- `change_velocity_to_slow()`: 1 unit/frame
- `trickle_velocity_to()`: 1 unit/frame in either direction

**Wall collision при прыжке:** `slide_along(SLIDE_ALONG_FLAG_JUMPING)`
- Fence landing → `set_person_land_on_fence()` (если не unclimbable)
- Wall kick-off: **#if 0** (мёртвый код)
- Jump-grab: **#ifdef DOG_POO** (мёртвый код)

**Barrel collision:** `BARREL_hit_with_sphere()` radius 0x70 при каждом движении

**Return values projectile_move_thing:** 0=no collision, 1=landed, 2=fence/wall, 100=death
