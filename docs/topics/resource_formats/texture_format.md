# Формат текстур — Urban Chaos

**Расположение:** `original_game/fallen/Debug/Textures/`, `original_game/fallen/Debug/clumps/`
**Функции загрузки:** `fallen/DDEngine/Source/texture.cpp`, `fallen/DDLibrary/D3DTexture.cpp`
**Заголовки:** `fallen/DDEngine/Headers/texture.h`

---

## .TGA — основной формат текстур

**Стандарт:** Targa Image Format (TGA)

### Характеристики
| Параметр | Значение |
|----------|---------|
| Цвет | 32-bit RGBA или 24-bit RGB |
| Размеры | 512×512, 256×256, 128×128, 64×64, 32×32 пикселей (степени двойки) |
| Мип-маппинг | Рассчитывается при загрузке, **не хранится** в файле |
| Сжатие | Нет (несжатый TGA) |

### Расположение
```
Debug/Textures/
├── *.tga       — глобальные текстуры (персонажи, объекты, UI)
└── ...
```

---

## .TXC — архивы текстур уровней (FileClump)

**Расположение:** `Debug/clumps/`
**Именование:** `<level_name>.txc` (например `level1.txc` из имени `level1.iam`)
**Реализация:** `DDLibrary/Source/FileClump.cpp`, `DDLibrary/Source/Tga.cpp`

Бинарный архив текстур уровня — pre-baked из TGA файлов для быстрой загрузки. Создаётся при первом запуске (если `TEXTURE_create_clump=1`), затем используется как read-only.

### Binary layout FileClump (.txc)

```
[4 байта]  ULONG MaxID — число слотов (= TEXTURE_MAX_TEXTURES + 64*8 ≈ ~600+)
[MaxID * sizeof(size_t)] Offsets[] — смещение каждого entry в файле (0 = пусто)
[MaxID * sizeof(size_t)] Lengths[] — длина в байтах каждого entry (0 = пусто)
[data blob] сжатые данные текстур по соответствующим смещениям
```

⚠️ **ПРОБЛЕМА:** `sizeof(size_t)` зависит от платформы (4 байта на 32-bit, 8 байт на 64-bit). Файлы созданные на 32-bit непортируемы для 64-bit парсинга.

### Binary layout каждого entry (до/после WriteSquished)

**Без сжатия:** прямой массив UWORD:
```
[2 байта] UWORD contains_alpha (0 или 1)
[2 байта] UWORD width
[2 байта] UWORD height
[width * height * 2 байта] пиксели:
  - contains_alpha=1: RGBA 4:4:4:4 в UWORD (биты 3..0=B, 7..4=G, 11..8=R, 15..12=A)
  - contains_alpha=0: RGB 5:6:5 в UWORD (биты 4..0=B, 10..5=G, 15..11=R)
```

**Со сжатием (WriteSquished):** палитра + битовое кодирование:
```
[2 байта] 0xFFFF — маркер сжатого формата
[2 байта] contains_alpha
[2 байта] width
[2 байта] height
[2 байта] UWORD total — число уникальных пикселей в палитре
[total * 2 байта] palette[] — уникальные UWORD значения пикселей
[bit-stream] индексы в палитру (bits=ceil(log2(total)) бит на пиксель, MSB first)
```

Сжатие применяется только если оно уменьшает размер.

### Загрузка в игре

- `TEXTURE_initialise_clumping()` → `OpenTGAClump(filename, maxid, readonly=true)`
- `TGA_load(file, ..., id)` → если clump открыт: `TGA_read_compressed(id)` → `ReadSquished(id)` → расшифровка → возврат RGBA пикселей
- `IndividualTextures=false` → читать из clump; `true` → читать отдельные TGA файлы (editor/debug режим)


---

## .TMA — таблицы тайловых текстур (style.tma / instyle.tma)

**Расположение:** `Debug/Textures/world*/`
**Функции загрузки:** `fallen/Source/io.cpp` — `load_texture_styles()`, `load_texture_instyles()`

Не .txc! Это отдельный формат — таблица соответствия стиль→текстура для тайлов зданий и города.

### style.tma — таблица городских тайлов

200 стилей тайлов × 5 слотов = UV/Page данные для поверхностей зданий.

**Binary layout (save_type >= 5 — актуальный формат):**
```
[4 байта] SLONG save_type
// if save_type 2..3: TXextrainfo block — пропускается через seek
[2 байта] UWORD = 200  (число стилей)
[2 байта] UWORD = 5    (слотов на стиль)
[200 * 5 * 4 байта] textures_xy[200][5] — TXTY:
  struct TXTY { UBYTE Page; UBYTE Tx; UBYTE Ty; UBYTE Flip; }
  Page = индекс текстурного атласа (страницы)
  Tx/Ty = координаты тайла внутри страницы
  Flip = флаги поворота/зеркала текстуры
[2 байта] UWORD = 200
[2 байта] UWORD = 21   (длина имени стиля в символах)
[200 * 21 байт] texture_style_names[200][21] — имена (только редактор; в-игре пропускается seek)
[2 байта] UWORD = 200
[2 байта] UWORD = 5    (слотов на стиль)
[200 * 5 * 1 байт] textures_flags[200][5] — тип полигона:
  POLY_GT = Gouraud+Textured, POLY_FT = Flat+Textured
```

**Слоты (5 на стиль):**
Соответствуют 5 поверхностям городского тайла (крыша, стены по 4 сторонам или похожее).

### instyle.tma — таблица интерьерных текстур

```
[4 байта] SLONG save_type
[2 байта] UWORD count_x
[2 байта] UWORD count_y
[count_x * count_y * 1 байт] inside_tex[count_x][count_y] — индексы стилей интерьера
[2 байта] UWORD name_x
[2 байта] UWORD name_y
[name_x * name_y байт] inside_names[][] — текстовые имена (только редактор)
```

### Как используются

Оба .tma файла нужно парсить для правильного отображения городских тайлов.
TXTY хранит **индексы тайлов** (Page, Tx, Ty, Flip — всё UBYTE), не UV-координаты. Финальные UV вычисляются в рантайме из индексов тайла и размера страницы.
DXTXTY (DirectX версия) хранит только Page (UWORD) и Flip (UWORD) — без Tx/Ty.

---

## Структура текстурных страниц (page layout)

**`TEXTURE_MAX_TEXTURES = 22*64 + 160 = 1568`** — всего слотов страниц.

| Диапазон | Кол-во | Назначение |
|----------|--------|-----------|
| 0..255 (4×64) | 256 | World текстуры (world0..world3) |
| 256..511 (4×64) | 256 | Shared текстуры |
| 512..575 (1×64) | 64 | Interior текстуры |
| 576..703 (2×64) | 128 | People текстуры |
| 704..1151 (7×64) | 448 | Prims/static mesh текстуры |
| 1152..1407 (4×64) | 256 | People2 текстуры |
| 1408..1567 (160) | 160 | Спецэффекты (снег, взрывы, лица, туман) |
| 1568..1759 (+192) | 192 | Crinkle/normal-map данные для world страниц |

**Директории текстур:**
```
textures/world0/    — tex000.tga, tex000hi.tga, tex000to.tga, sex000hi.sex (crinkle)
textures/shared/
textures/inside/
textures/people/
textures/prims/
textures/people2/
clumps/             — level1.txc, level2.txc, ...
```

**Разрешение** — 3 варианта (движок пробует по убыванию качества):
1. `tex%03dto.tga` — 128×128 (super-res)
2. `tex%03dhi.tga` — 64×64 (hi-res, предпочтительно на PC)
3. `tex%03d.tga` — 32×32 (низкое)

**Crinkle данные** (`TEXTURE_crinkle[page]`):
- Хранятся отдельно при ID = `TEXTURE_MAX_TEXTURES + page`
- Загружаются через `CRINKLE_load()` из `.sex` файлов
- Normal-map эффект (bump текстуры для city tiles)

**2-pass страницы** (`POLY_PAGE_FLAG_2PASS`):
- Страница N рисуется в 2 прохода
- Проход 1: страница N (цвет)
- Проход 2: страница N+1 (illumination/glow маска)

---

## Наборы текстур (texture_set)

```c
// texture.cpp
TEXTURE_choose_set(SLONG texture_set)   // выбирает набор по ID (0–21)
```

- 22 набора (0–21), каждый определяет визуальный стиль уровня
- Разные наборы: городской асфальт, снег, лес, промзона, порт и т.д.
- `texture_set` загружается из файла карты (поле save_type >= 20)

---

## Загрузка в D3D (оригинал)

```c
// D3DTexture.cpp — загрузка и управление текстурами в DirectX
// TEX_EMBED флаг (активен в Release и Debug) — встраивание текстур
```

- Текстуры создаются как D3D Texture объекты
- Мип-маппинг генерируется на CPU при загрузке
- `TEX_EMBED` — текстуры встроены в память (не перезагружаются с диска)

