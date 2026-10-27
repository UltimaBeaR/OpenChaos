# Формат карт уровней — Urban Chaos (.iam)

**Расширения:** `.iam` (PC), `.pi`/`.pp` (PSX альтернативы)
**Функция загрузки:** `fallen/Source/io.cpp::load_game_map()`
**Заголовки:** `fallen/Headers/pap.h`, `fallen/Headers/io.h`
**Расположение:** `original_game/fallen/Debug/data/*.iam`

**Важно:** Файлы карт = `.iam`, файлы миссий = `.ucm` (уровни/). Это разные форматы!

---

## Версии формата (save_type)

| save_type | Значение |
|-----------|----------|
| < 18 | PSX-формат — другой layout объектов (MapThingPSX) |
| >= 18 | PC-формат — LoadGameThing struct |
| > 23 | Перед PAP_Hi: [4 байта] ob_size (размер OB struct); OB данные в SuperMap, не здесь |
| >= 20 | После SuperMap: [4 байта] SLONG texture_set |
| >= 25 | psx_textures_xy данные (PSX only — не переносить) |
| >= 26 | Текстуры крыши 128×128 × 2 байта (PSX only — не переносить) |

---

## Бинарный формат (PC, save_type >= 18)

```
[4 байта]  SLONG save_type              — версия файла (18..28 встречается)

[если save_type > 23]:
  [4 байта] SLONG ob_size              — размер OB_Ob struct (проверка совместимости)

══ PAP_Hi: высокое разрешение (128 × 128 ячеек) ══
struct PAP_Hi {         // 6 байт на ячейку
    UWORD Texture;      // индекс текстуры тайла
    UWORD Flags;        // битовые флаги (PAP_FLAG_*)
    SBYTE Alt;          // высота: реальная высота = Alt * 8
    SBYTE Height;       // не используется в финальной игре
};
// Итого: 128 × 128 × 6 = 98 304 байт

[если PSX и save_type >= 26]:
  [4 байта] ULONG check                 — must == sizeof(UWORD)*128*128
  [32768 байт] UWORD WARE_roof_tex[128][128] — PSX текстуры крыши (SKIP)
  [4 байта] ULONG check2                — must == 666

══ Animated Prims ══
[2 байта]  UWORD anim_prim_count        — количество анимированных примов в уровне
[anim_prim_count * sizeof(LoadGameThing)] LoadGameThing[]
  — только записи TYPE=MAP_THING_TYPE_ANIM_PRIM
  — при чтении: load_anim_prim_object(IndexOther) + create_anim_prim()

══ OB (объекты окружения: фонари, баки и т.д.) ══
[если save_type < 23]:
  [4 байта] SLONG OB_ob_upto            — количество OB объектов
  [OB_ob_upto * sizeof(OB_Ob)] OB_ob[]  — массив объектов
  [OB_SIZE*OB_SIZE * sizeof(OB_Mapwho)] OB_mapwho[]  — spatial hash
[если save_type >= 23]: OB данные находятся в конце SuperMap секции (см. ниже)

══ SuperMap ══
// load_super_map(handle, save_type) — здания, фасеты, интерьеры, walkables, OB
// Содержимое описано в секции SuperMap ниже

[если save_type >= 20]:
  [4 байта] SLONG texture_set           — набор текстур (0–21)

[если save_type >= 25 и PSX]:
  [2*200*5 байт] psx_textures_xy        — UV таблица для PSX (SKIP)
```

---

## Структура LoadGameThing (PC, save_type >= 18)

```c
struct LoadGameThing {
    UWORD Type;          // MAP_THING_TYPE_* (тип объекта)
    UWORD SubStype;      // подтип
    SLONG X, Y, Z;       // позиция в world units (fixed-point)
    ULONG Flags;         // флаги объекта
    UWORD IndexOther;    // индекс связанного ресурса (номер anim prim)
    UWORD AngleX;        // угол поворота X
    UWORD AngleY;        // угол поворота Y
    UWORD AngleZ;        // угол поворота Z
    ULONG Dummy[4];      // резерв (16 байт)
};
// sizeof = 2+2+4+4+4+4+2+2+2+2+16 = 44 байта
```

---

## SuperMap бинарный формат

Загружается функцией `load_super_map(handle, save_type)` в `supermap.cpp`.

```
[2 байта] UWORD next_dbuilding         — количество зданий
[2 байта] UWORD next_dfacet            — количество фасетов (внешних стен)
[2 байта] UWORD next_dstyle            — количество стилей отделки

[если save_type >= 17]:
  [2 байта] UWORD next_paint_mem       — байты кастомных текстур
  [2 байта] UWORD next_dstorey         — количество этажей (DStorey)

[next_dbuilding * sizeof(DBuilding)]   — массив зданий
[next_dfacet    * sizeof(DFacet)]      — массив фасетов (внешние грани зданий)
[next_dstyle    * sizeof(UWORD)]       — таблица стилей

[если save_type >= 17]:
  [next_paint_mem * sizeof(UBYTE)]     — данные кастомных текстур
  [next_dstorey   * sizeof(DStorey)]   — данные этажей зданий

[если save_type >= 21]:
  [sizeof(next_inside_storey)] UWORD   — количество внутренних этажей
  [sizeof(next_inside_stair)]  UWORD   — количество лестниц
  [sizeof(next_inside_block)]  SLONG   — байты комнатных карт
  [next_inside_storey * sizeof(InsideStorey)]  — интерьеры зданий
  [next_inside_stair  * sizeof(Staircase)]     — лестничные связи
  [next_inside_block  * sizeof(UBYTE)]         — room-id карты по mapsquare

[walkables data] — load_walkables(handle, save_type)

[если save_type >= 23]:
  [sizeof(OB_ob_upto)] SLONG           — количество OB объектов
  [OB_ob_upto * sizeof(OB_Ob)]         — OB объекты (фонари, баки, урны)
  [OB_SIZE*OB_SIZE * sizeof(OB_Mapwho)] — spatial hash OB объектов
```

---

## PAP_Lo — low resolution map (32 × 32)

PAP_Lo **не сохраняется** в файл. Строится при загрузке:
- `MapWho` — обнуляется, заполняется при создании Thing
- `Walkable` — берётся из PAP_Hi → supermap walkables
- `ColVectHead` — строится из dfacets при load_super_map
- `water`, `Flag` — из PAP_Hi.Flags

---

## Флаги PAP_Hi

```c
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

