# Рендеринг — Urban Chaos (DDEngine)

**Ключевые файлы:**
- `fallen/DDEngine/aeng.cpp` — главный движок рендеринга (~16K строк)
- `fallen/DDEngine/poly.cpp` — батчинг и сортировка полигонов
- `fallen/DDEngine/mesh.cpp` — загрузка и рендеринг 3D мешей
- `fallen/DDEngine/texture.cpp`, `D3DTexture.cpp` — управление текстурами
- `fallen/DDEngine/light.cpp` — освещение
- `fallen/DDEngine/cam.cpp` — камера
- `fallen/DDEngine/sky.cpp` — небо
- `fallen/DDEngine/shadow.cpp` — тени (спрайты)
- `fallen/DDEngine/sprite.cpp` — 2D спрайты
- `fallen/DDLibrary/DDManager.cpp` — Direct3D (~49K строк)
- `fallen/DDLibrary/GDisplay.cpp` — управление дисплеем (~51K строк)

---

## 1. Общий пайплайн рендеринга

**Порядок за кадр:**

1. `POLY_frame_init()` — инициализация кадра, очистка буферов страниц
2. Трансформация и накопление полигонов:
   - `MESH_draw_poly()` — 3D модели (персонажи, машины, здания)
   - `SPRITE_draw()` — спрайты (тени, эффекты)
   - `SKY_draw_poly_*()` — небо (облака, луна, звёзды)
3. `POLY_frame_draw()` — рендеринг всех накопленных полигонов по страницам:
   - `SortBackFirst()` — Z-сортировка (bucket/merge sort)
   - `DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, ..., D3DDP_DONOTUPDATEEXTENTS | D3DDP_DONOTLIGHT)`
   - Флаги означают: трансформации и освещение уже выполнены на CPU

**Ключевые константы:**
- `POLY_NUM_PAGES = 1508` — страниц текстур (0–1407 обычные, 1408+ служебные)
- `MAX_VERTICES = 32 000` — глобальный пул вершин
- `NO_CLIPPING_TO_THE_SIDES_PLEASE_BOB = 0` на PC — screen-space clipping по краям включён

---

## 2. Форматы вершин

**Основная структура (D3DTLVERTEX из DirectX 6):**
```c
struct D3DTLVERTEX {
    float X, Y, Z;      // Экранные координаты (X, Y) + глубина (Z)
    float RHW;          // 1/W (инвертированное W для перспективы)
    D3DCOLOR diffuse;   // Диффузный цвет (ARGB)
    D3DCOLOR specular;  // Зеркальный/fog цвет (ARGB)
    float tu, tv;       // Текстурные координаты
};
```

**Трансформационный конвейер:**
1. Мировое пространство (SLONG координаты)
2. View space (через матрицу камеры — rotate_on_x/y/z)
3. Перспективная проекция → нормализованный куб [-1, 1]
4. Экранное пространство (0..DisplayWidth × 0..DisplayHeight)

**Фиксированный пайплайн D3D** — без шейдеров. Vertex lighting через `LIGHT_get_d3d_colour()`.

---

## 3. Culling и отсечение

**Frustum culling:**
- `POLY_sphere_visible()` — тест видимости сферы объекта
- `POLY_setclip()` — screen-space clipping по границам экрана
- `POLY_ZCLIP_PLANE = 1/64` — минимальное расстояние (near clip)

**Back-face culling:**
- В `POLY_valid_triangle()` через крестное произведение нормалей
- Если результат < 0 → треугольник не виден
- Отключается флагом `NO_BACKFACE_CULL_PLEASE_BOB` (для двусторонних мешей)

**Сортировка полигонов:**
- `WE_NEED_POLYBUFFERS_PLEASE_BOB = 1` на PC — нужна Z-сортировка (polypage.h)
- `WE_NEED_POLYBUFFERS_PLEASE_BOB = 0` на DC — без сортировки
- Bucket sort или merge sort по depth для прозрачных объектов

---

## 3b. Рендеринг персонажей (Tom's Engine)

**`USE_TOMS_ENGINE_PLEASE_BOB = 1`** (`DDEngine/Headers/aeng.h`) — всегда включён (и PC, и DC).
D3D-дружественный рендерер персонажей. Вся графика персонажей идёт через него.
В figure.cpp весь путь рендеринга под `#if USE_TOMS_ENGINE_PLEASE_BOB` — единственный активный путь.

```c
// figure.cpp
SLONG FIGURE_alpha = 255;  // альфа прозрачности персонажа
UBYTE body_part_upper[];   // таблица upper body parts для 15 типов персонажей
```

Освещение персонажей: `BuildMMLightingTable()` — использует NIGHT систему (`NIGHT_found[]`, `NIGHT_amb_r/g/b`).
**Anti-lights:** существуют отрицательные источники света — вычитают яркость (для тёмных зон).

**`HIGH_REZ_PEOPLE_PLEASE_BOB`** — закомментирован ("Now enabled only on a cheat!").
Высокополигональные модели в финале **не активны**.

---

## 4. Система мешей (mesh.cpp)

**Структура загруженного меша:**
```c
struct PrimObject {
    UWORD StartPoint, EndPoint;  // Диапазон вершин в глобальном массиве
    PrimFace3 *faces3;           // Треугольные грани
    PrimFace4 *faces4;           // Четырёхугольные грани
    // Каждая грань: индексы вершин + page (текстура) + флаги
};
```

**Флаги граней:**
- masked (маска/cutout), semi-transparent, self-illuminating и т.д.

**MESH_draw_poly() — процесс рендеринга:**
1. Трансформация вершин (локальное → мировое, матрица поворота 3×3)
2. Для каждой грани:
   - Back-face culling
   - Трансформация вершин в view space (`POLY_transform`)
   - Добавление в POLY buffer: `POLY_add_triangle()` / `POLY_add_quad()`
3. Освещение из предвычисленного `NIGHT_Colour` массива

**Морфинг (tweening):**
```c
MESH_draw_morph(prim, morph1, morph2, tween, ...):
// tween ∈ [0, 256], где 256 = полностью morph2
// Вершины линейно интерполируются между двумя ключевыми кадрами
```
- Нет скелетной анимации — только vertex morphing между keyframes
- Топология (грани) одинакова для всех morphs
- `DrawTween` = морфинг, `DrawMesh` = один конкретный кадр (быстрее)

**Деформация транспорта (Crumple):**
- `MESH_set_crumple()` — параметры деформации
- `MESH_car_crumples[]` — таблица предвычисленных смещений: **5 уровней урона × 8 вариантов × 6 точек**
- Смещения вершин применяются при трансформации

**Отражения в лужах:**
- `MESH_draw_reflection()` — специальный путь для рендеринга объектов как отражения в луже
- Используется вместе с `PUDDLE_in()` для определения зоны отражения

---

## 5. Система текстур

**Характеристики:**
- Формат исходников: TGA файлы (из `Debug/Textures/`)
- `TEXTURE_NUM_STANDARD = 22 × 64 = 1408` базовых страниц
- Каждая page: 256×256 или 512×512 пикселей в 16-bit или 32-bit
- Объём VRAM: ~128–256 MB (огромно для 1999)

**`D3DTexture` класс:**
- Оборачивает `LPDIRECT3DTEXTURE2` (DirectDraw Surface)
- `TEXTURE_texture[]` — массив всех загруженных текстур
- Динамическое обновление теневой текстуры: `TEXTURE_shadow_lock/unlock`
- Тень: 64×64 px (`TEXTURE_SHADOW_SIZE`)

**Флаги страниц (`POLY_page_flag[]`):**
```c
POLY_PAGE_FLAG_TRANSPARENT  // Alpha blending
POLY_PAGE_FLAG_WRAP         // Повторение текстуры
POLY_PAGE_FLAG_ADD_ALPHA    // Аддитивная альфа (огонь, свечение)
POLY_PAGE_FLAG_2PASS        // Двухпроходный рендеринг
POLY_PAGE_FLAG_SELF_ILLUM   // Самоосвещённая (небо, огонь)
POLY_PAGE_FLAG_NO_FOG       // Без fog
POLY_PAGE_FLAG_ALPHA        // Стандартная альфа
```

**Служебные страницы (с индекса 1408):**
- `POLY_PAGE_SHADOW`, `POLY_PAGE_SHADOW_OVAL` — тени
- `POLY_PAGE_FLAMES`, `POLY_PAGE_SMOKE`, `POLY_PAGE_EXPLODE1/2` — эффекты
- `POLY_PAGE_MOON`, `POLY_PAGE_SKY` — небо

**TEX_EMBED флаг:**
- PC: `TEX_EMBED = 1` — текстуры встроены в страницы (независимые буферы)
- Этот флаг активен в обеих конфигурациях Fallen.vcxproj

---

## 6. Освещение — NIGHT система

**Vertex Lighting** — не per-pixel, освещение запекается в вершины (NIGHT система из `night.h`).

### Ёмкость NIGHT системы
```c
NIGHT_MAX_SLIGHTS   = 256    // статических источников света
NIGHT_MAX_DLIGHTS   = 64     // динамических источников света
NIGHT_MAX_SQUARES   = 256    // кэшированных lo-res квадратов освещения
NIGHT_MAX_WALKABLE  = 15000  // вершин walkable поверхностей с цветом
NIGHT_MAX_BRIGHT    = 64     // яркость канала (6-bit, 0-63 → ×4 → 0-255 для D3D)
NIGHT_MAX_FOUND     = 4      // максимум источников влияния на 1 точку
```

### Форматы данных
```c
struct NIGHT_Colour {
    UBYTE r, g, b;  // 6-bit каналы (0-63), конвертируются в D3D через ×4
};

// NIGHT_get_d3d_colour() — конвертирует NIGHT_Colour → D3D ARGB (0xFF, r×4, g×4, b×4)
```

### Типы источников
- **`NIGHT_Slight`** — статические (загружаются из `.lgt` файлов через `NIGHT_load_ed_file()`)
- **`NIGHT_Dlight`** — динамические (создаются в рантайме, до 64 одновременно)

### Флаги освещения уровня
```c
NIGHT_FLAG_LIGHTS_UNDER_LAMPOSTS   (1<<0)  // фонари
NIGHT_FLAG_DARKEN_BUILDING_POINTS  (1<<1)  // затемнение стен зданий
NIGHT_FLAG_DAYTIME                 (1<<2)  // дневное освещение
```

### Specular эффект
```c
NIGHT_specular_enable  // на PC: oversaturation создаёт pseudo-specular highlights
                       // на DC: отключён
```

### Статические тени (shadow.cpp)
Запекаются **при загрузке уровня**, не в рантайме:
- Направление солнца: `SHADOW_DIR_X=+147, SHADOW_DIR_Y=-148, SHADOW_DIR_Z=-147`
- Метод: ray-cast через `there_is_a_los()` в сторону солнца
- Результат: биты `PAP_FLAG_SHADOW_1/2/3` в PAP_Hi.Flags для каждого хай-рез квадрата
- `shadow[8] = {0, 1, 7, 5, 3, 2, 4, 5}` — lookup таблица для 3 соседних квадратов

### Затухание по расстоянию (дистанс-фейд)
```c
POLY_FADEOUT_START = 0.60F  // z/view_dist — начало затухания
POLY_FADEOUT_END   = 0.95F  // полностью прозрачно
```

**Динамические источники (light.cpp):**
```c
LIGHT_create(x, y, z, range, type):
// Типы: NORMAL, BROKEN (мерцание), PULSE, FADE
// LIGHT_range = 0x600 максимум (~1536 units)
```

**ВАЖНО: Динамических теней НЕТ.**
Тени под персонажами — flat sprite (`POLY_PAGE_SHADOW`), позиционируется под ногами.
Статическое освещение уровня запекается при загрузке, обновляется только при смене дня/ночи.

---

## 7. Камера

**Камера от третьего лица:**
```c
POLY_camera_set(x, y, z, yaw, pitch, roll, view_dist, lens):
// lens ~ 1.5 (zoom фактор, чем выше — уже FOV)
// view_dist ~ 6000 units (дальность видимости)
```

**Параметры:**
- Yaw — горизонтальный поворот
- Pitch — вертикальный наклон
- Roll — наклон камеры

**Collision avoidance:**
- Raycasting от игрока к позиции камеры
- При пересечении стены → камера придвигается к игроку

**Splitscreen:**
- `POLY_SPLITSCREEN_NONE`, `TOP`, `BOTTOM` — поддержка раздельного экрана

---

## 8. Спрайты и 2D

**3D спрайты (SPRITE_draw):**
```c
SPRITE_draw(world_x, world_y, world_z, size, colour, page, ...):
// Трансформируется в мировое → screen space
// Рендерится как quad (два треугольника)
// Два типа: обычный и distorted (с искажением)
```

**HUD и текст:**
- `FONT_draw()` — текст (TrueType шрифты)
- `PANEL_draw_*()` — элементы интерфейса (health bar, таймер)
- `MSG_add()` — экранные сообщения

---

## 9. Специальные эффекты

**Skybox:**
- Простой куб вокруг камеры (всегда следует за ней)
- `SKY_draw_poly_sky()` — текстура неба (POLY_PAGE_SKY + POLY_PAGE_MOON)
- `SKY_draw_poly_clouds()` — полигоны облаков на расстоянии

**Частицы:**
- `Effect.cpp`, `psystem.cpp` — система эмиттеров
- Огонь: `POLY_PAGE_FLAMES` с аддитивной альфой (POLY_PAGE_FLAG_ADD_ALPHA)
- Дым: полупрозрачные спрайты
- Взрывы: `POLY_PAGE_EXPLODE1/2` — анимированные спрайты

**Crinkle система:**
- Нечто похожее на bump mapping через перемещение текстур
- Использует нормаль и view space
- Специфично для DirectX — при портировании нужно переосмыслить

---

## 10. Портирование на OpenGL/Vulkan

**Легко маппится:**

| Оригинал (DirectX 6) | OpenGL 4.6 / Vulkan |
|----------------------|---------------------|
| D3DTLVERTEX | Custom vertex struct + VBO |
| LPDIRECT3DTEXTURE2 | GLuint texture / VkImage |
| SetTransform() | Uniform buffers (MVP матрицы) |
| SetRenderState() | glEnable/Disable или Pipeline state |
| DrawIndexedPrimitive() | glDrawElements / vkCmdDrawIndexed |
| D3DRENDERSTATETYPE (ZEnable, AlphaBlend, CullMode) | glDepthMask, glBlendFunc, glCullFace |

**Требует переработки:**

- **Фиксированный пайплайн** → вершинный + фрагментный шейдеры
  - Vertex lighting — реализовать в vertex shader
  - Fog — реализовать в fragment shader
- **Crinkle система** — переосмыслить как normal mapping
- **Vertex buffer pool** — переработать под staging buffers / persistent mapping
- **Multi-matrix DrawIndPrimMM** — кастомный механизм

**Сохранить идеи из оригинала:**
- PolyPage система — хорошая абстракция, адаптировать
- RenderState инкапсуляция — хорошая практика, оставить
- Bucket sort для прозрачных полигонов — стандарт, оставить

---

## 11. Итоговые данные для нового рендерера

**Структура вершины для нового рендерера:**
```cpp
struct Vertex {
    float x, y, z;      // Позиция
    uint32_t colour;     // ARGB диффузный
    uint32_t specular;   // ARGB зеркальный/fog
    float u, v;          // UV координаты
};
```

**Ключевые render state transitions:**
- Прозрачность: SRC_ALPHA + ONE_MINUS_SRC_ALPHA (standard) или ONE + ONE (additive для огня)
- Z-buffer: включён везде, запись выключена для прозрачных
- Culling: back-face по умолчанию, front-face или off для двусторонних

**Освещение в новой версии:**
- Можно улучшить до per-pixel (fragment shader) — визуально лучше
- Динамические тени — можно добавить (shadow maps) — не было в оригинале
- Оригинальная vertex lighting легко воспроизводится в vertex shader
