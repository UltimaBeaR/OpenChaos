# Формат карт уровней — Urban Chaos

**Расширения:** `.pam`, `.pi`, `.pp`
**Функция загрузки:** `fallen/Source/io.cpp::load_game_map()`
**Заголовки:** `fallen/Headers/pap.h`, `fallen/Headers/io.h`

---

## Версии формата (save_type)

| save_type | Значение |
|-----------|----------|
| < 18 | PSX-формат — другой layout объектов |
| >= 18 | PC-формат — LoadGameThing struct |
| > 23 | Перед PAP_Hi: [4 байта] размер OB-данных |
| >= 20 | После объектов: [4 байта] SLONG texture_set |
| >= 25 | psx_textures_xy данные (PSX only — не переносить) |
| >= 26 | Текстуры крыши 128×128 × 2 байта (PSX only — не переносить) |

---

## Бинарный формат (PC, save_type >= 18)

```
[4 байта]  SLONG save_type              — версия файла

[если save_type > 23]:
  [4 байта] SLONG ob_size              — размер OB-данных

══ PAP_Hi: высокое разрешение (128 × 128 ячеек) ══
struct PAP_Hi {         // 6 байт на ячейку (UWORD + UWORD + SBYTE + SBYTE)
    UWORD Texture;      // индекс текстуры (3 старших бита зарезервированы)
    UWORD Flags;        // битовые флаги (PAP_FLAG_*)
    SBYTE Alt;          // высота: реальная высота = Alt * 8
    SBYTE Height;       // не используется в финальной игре
};
// Итого: 128 × 128 × 6 = 98 304 байт

══ PAP_Lo: низкое разрешение (32 × 32 ячейки) ══
struct PAP_Lo {          // 8 байт на ячейку
    UWORD MapWho;        // голова linked list объектов в ячейке
    SWORD Walkable;      // >0 = prim_face4 (пол), <0 = крыша
    UWORD ColVectHead;   // голова linked list коллизионных векторов
    SBYTE water;         // высота воды (-127 = нет воды)
    UBYTE Flag;          // warehouse flags
};
// Итого: 32 × 32 × 8 = 8 192 байт

══ Объекты уровня (save_type >= 18) ══
struct LoadGameThing {
    UWORD Type;          // CLASS_* (класс объекта)
    UWORD SubStype;      // подтип
    SLONG X, Y, Z;       // позиция в мировых координатах (fixed-point 32.8)
    ULONG Flags;         // флаги объекта
    UWORD IndexOther;    // индекс связанного объекта
    UWORD AngleX;        // угол поворота X
    UWORD AngleY;        // угол поворота Y
    UWORD AngleZ;        // угол поворота Z
    ULONG Dummy[4];      // резерв (16 байт)
};

[если save_type >= 20]:
  [4 байта] SLONG texture_set          — набор текстур (0–21)

══ SuperMap ══
// load_super_map(handle, save_type) — дополнительные данные уровня
```

---

## Флаги PAP_Hi (Flags поле)

```c
// pap.h
#define PAP_FLAG_SHADOW_1      (1<<0)   // запечённая тень, направление 1
#define PAP_FLAG_SHADOW_2      (1<<1)   // запечённая тень, направление 2
#define PAP_FLAG_SHADOW_3      (1<<2)   // запечённая тень, направление 3
#define PAP_FLAG_REFLECTIVE    (1<<3)   // отражающая поверхность (лужи)
#define PAP_FLAG_HIDDEN        (1<<4)   // ячейка скрыта от рендеринга
```

Тени запекаются при загрузке в `shadow.cpp::SHADOW_do()`.
Направление источника тени: SHADOW_DIR_X=+147, Y=-148, Z=-147.

---

## Наборы текстур (texture_set 0–21)

Определяют визуальный стиль уровня (город, снег, лес, порт и т.д.).
`TEXTURE_choose_set(texture_set)` в `texture.cpp` — загружает нужный набор.

---

## Расположение файлов

```
original_game/fallen/Debug/levels/
├── *.ucm    — скрипт миссии (MuckyBasic bytecode)
├── *.pam    — основной файл карты
├── *.pi     — дополнительные данные карты
└── *.pp     — альтернативный вариант
```

---

## Что переносить

| Компонент | Перенос | Примечание |
|-----------|---------|------------|
| PAP_Hi (128×128, 6 байт/ячейка) | 1:1 | Критично: рендеринг, тени |
| PAP_Lo (32×32, 8 байт/ячейка) | 1:1 | Критично: коллизии, AI, вода |
| LoadGameThing | 1:1 | Все объекты уровня |
| texture_set | 1:1 | Визуальный стиль |
| PSX данные (>= 25, >= 26) | НЕТ | PSX only |
