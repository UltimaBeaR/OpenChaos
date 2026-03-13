# Формат файлов освещения — Urban Chaos (.lgt)

**Расположение:** `original_game/fallen/Debug/data/Lighting/*.lgt`
**Функция загрузки:** `fallen/Source/night.cpp::NIGHT_load_ed_file()`
**Структура ED_Light:** `fallen/Ledit/Headers/ed.h`

---

## Назначение

`.lgt` файлы = предварительно размещённые динамические источники света для уровня.
Создаются в редакторе освещения (Ledit). Загружаются в начале каждой миссии.

---

## Бинарный формат

```
[4 байта] SLONG sizeof_ed_light  — размер struct ED_Light (проверка совместимости)
                                    Старший UWORD = версия (0 = старая, 1+ = новая)
                                    Младший UWORD = sizeof(ED_Light)
[4 байта] SLONG ed_max_lights    — количество записей ED_Light в файле
[4 байта] SLONG sizeof_night_colour — размер struct NIGHT_Colour (проверка)

[ed_max_lights * sizeof(ED_Light)] ED_Light[]  — массив источников света

[4 байта] SLONG ed_light_free    — количество неиспользуемых слотов (игнорируется при загрузке)

── Глобальное освещение уровня ──
[4 байта] ULONG flag             — NIGHT_FLAG_* битовые флаги (день/ночь и др.)
[4 байта] ULONG amb_d3d_colour   — D3D-формат ambient цвета (ARGB packed, игнорируется в новой игре)
[4 байта] ULONG amb_d3d_specular — D3D-формат specular (игнорируется в новой игре)
[4 байта] ULONG amb_red          — ambient красный (0-100, масштабируется ×820>>8 = ×3.2)
[4 байта] ULONG amb_green        — ambient зелёный
[4 байта] ULONG amb_blue         — ambient синий
[1 байт]  SBYTE lampost_red      — цвет фонарей (красный)
[1 байт]  SBYTE lampost_green    — цвет фонарей (зелёный)
[1 байт]  SBYTE lampost_blue     — цвет фонарей (синий)
[1 байт]  UBYTE padding          — выравнивание
[4 байта] SLONG lampost_radius   — радиус действия фонарей (world units)
[sizeof(NIGHT_Colour)] sky_colour — цвет неба (3 байта: red, green, blue, range 0-63)
```

---

## Структура ED_Light (12 байт)

```c
// fallen/Ledit/Headers/ed.h
typedef struct
{
    UBYTE range;    // радиус влияния источника (world units / масштаб)
    SBYTE red;      // красный компонент (signed, -128..127)
    SBYTE green;    // зелёный
    SBYTE blue;     // синий
    UBYTE next;     // linked list: следующий свет в слоте (редакторный)
    UBYTE used;     // 1 = активный источник, 0 = пустой слот
    UBYTE flags;    // LIGHT_FLAGS_INSIDE (1) = только внутри зданий
    UBYTE padding;
    SLONG x;        // позиция X в world units
    SLONG y;        // позиция Y
    SLONG z;        // позиция Z
} ED_Light;         // sizeof = 12 байт (4 UBYTE/SBYTE + 3 SLONG = 4 + 12 = 16? нет...)
```

**Важно:** `sizeof(ED_Light)` = 4 (UBYTE×4 + SBYTE×3 + pad×1) + 12 (SLONG×3) = **16 байт**.
Файл проверяет совместимость через `sizeof_ed_light` поле в заголовке.

---

## NIGHT_Colour (PC платформа = 3 байта)

```c
// fallen/Headers/Night.h
typedef struct {
    UBYTE red;    // 0..63 (NIGHT_MAX_BRIGHT = 64)
    UBYTE green;  // 0..63
    UBYTE blue;   // 0..63
} NIGHT_Colour;   // sizeof = 3 байта на PC
```

Диапазон 0–63 (не 0–255). При конвертации в OpenGL: `r_f = col.red / 64.0f`.

---

## Что загружается из .lgt

После загрузки NIGHT_load_ed_file() устанавливает глобали:
- `NIGHT_flag` — определяет режим освещения (день/ночь, дождь)
- `NIGHT_sky_colour` — цвет неба для рендеринга
- `NIGHT_lampost_radius` / `NIGHT_lampost_red/green/blue` — параметры фонарей
- NIGHT_ambient() — нормирует ambient: `amb_red * 820 >> 8`
- NIGHT_slight_create() — создаёт активные статические точечные огни

**Жёстко заданная директория освещения:**
`ambient_dir = (110, -148, -177)` — используется везде в коде как константа.

---

## Что переносить

| Компонент | Перенос | Примечание |
|-----------|---------|------------|
| Парсер .lgt | 1:1 | Формат прост, не менять |
| ED_Light → point light | 1:1 | range/color → OpenGL point light |
| NIGHT_Colour (0-63) | Нормализовать | Делить на 64.0f для OpenGL |
| amb_d3d_colour | **НЕТ** | D3D-специфичный packed формат, пересчитать из amb_red/green/blue |
| LIGHT_FLAGS_INSIDE | 1:1 | Для indoor/outdoor раздельного освещения |
| sky_colour | 1:1 | → clear color или skybox tint |
| lampost_* | 1:1 | Отдельный класс ambient для ночных фонарей |
