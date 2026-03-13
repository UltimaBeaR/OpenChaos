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

## 1. Типы объектов рендеринга (DT_*)

Все объекты имеют тип, определяющий путь рендеринга (drawtype.h):

```c
#define DT_NONE              0
#define DT_BUILDING          1   // Здания (DFacet)
#define DT_PRIM              2   // Статичный прим-объект (DrawMesh)
#define DT_MULTI_PRIM        3   // Составной прим
#define DT_ROT_MULTI         4   // Вращающийся составной прим
#define DT_EFFECT            5   // Расширяющееся кольцо (Effect.cpp)
#define DT_MESH              6   // Меш без анимации (DrawMesh)
#define DT_TWEEN             7   // Персонаж с vertex morphing (DrawTween)
#define DT_SPRITE            8   // 2D спрайт в 3D пространстве
#define DT_VEHICLE           9   // Транспортное средство
#define DT_ANIM_PRIM        10   // Анимированный прим
#define DT_CHOPPER          11   // Вертолёт
#define DT_PYRO             12   // Огонь/взрывы
#define DT_ANIMAL_PRIM      13   // Животные
#define DT_TRACK            14   // Рельсы/колея
#define DT_BIKE             15   // Велосипед
```

**DrawMesh** — статический объект:
```c
typedef struct {
    UWORD Angle;        // Yaw (0xfafa = не используется)
    UWORD Roll;
    UWORD Tilt;
    UWORD ObjectId;     // Индекс в глобальном массиве примов
    CACHE_Index Cache;  // Кэш освещения
    UBYTE Hm;           // 255 = NULL
} DrawMesh;
```

---

## 1b. Общий пайплайн рендеринга

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

**Внутренняя структура POLY_Point (poly.cpp):**
```c
typedef struct {
    float x, y, z;      // 3D мировые координаты (view space)
    float X, Y, Z;      // 2D экранные координаты + depth (Z = 1/view_z)
    UWORD clip;         // Флаги отсечения (POLY_CLIP_*)
    UWORD user;         // Зарезервировано
    float u, v;         // Текстурные координаты
    ULONG colour;       // Диффузный цвет (xxRRGGBB)
    ULONG specular;     // Зеркальный/fog цвет (alpha = fog factor в старших 8 битах)
} POLY_Point;
```

**Финальный формат для D3D (D3DTLVERTEX из DirectX 6):**
```c
struct D3DTLVERTEX {
    float X, Y, Z;      // Экранные координаты (X, Y) + глубина (Z)
    float RHW;          // 1/W (инвертированное W для перспективы)
    D3DCOLOR diffuse;   // Диффузный цвет (ARGB)
    D3DCOLOR specular;  // Зеркальный/fog цвет (ARGB)
    float tu, tv;       // Текстурные координаты
};
```

**POLY_transform() — трансформация вершины:**
```c
// Input: world_x, world_y, world_z (мировые координаты)
// 1. Translate к камере:
vx = world_x - POLY_cam_x;
vy = world_y - POLY_cam_y;
vz = world_z - POLY_cam_z;

// 2. Rotate в view space (через матрицу камеры 3×3)
MATRIX_MUL(POLY_cam_matrix, vx, vy, vz);

// 3. Near/far clip:
if (vz < POLY_ZCLIP_PLANE) → POLY_CLIP_NEAR
if (vz > 1.0F)             → POLY_CLIP_FAR

// 4. Перспективная проекция:
Z  = POLY_ZCLIP_PLANE / vz;   // Reciprocal depth
pp->X = POLY_screen_mid_x - POLY_screen_mul_x * vx * Z;
pp->Y = POLY_screen_mid_y - POLY_screen_mul_y * vy * Z;
pp->z = vz;
pp->Z = Z;
```

**Трансформационный конвейер:**
1. Мировое пространство (SLONG fixed-point координаты, × 1/256)
2. View space (через матрицу камеры — rotate_on_x/y/z)
3. Перспективная проекция + reciprocal depth
4. Экранное пространство (0..DisplayWidth × 0..DisplayHeight)

**Фиксированный пайплайн D3D** — без шейдеров. Vertex lighting через `NIGHT_get_d3d_colour()`.

---

## 2b. UV упаковка в PrimFace4

Детали UV упаковки: **[rendering_mesh.md](rendering_mesh.md)** (раздел 6).

Краткое: `UV[i][0]` — 6 бит U + 2 бита grid-X; `UV[i][1]` — 8 бит V. Scale 1/32.
Atlas = 8×8 тайлов по 64×64 px. `FACE_PAGE_OFFSET = 8*64 = 512`.

---

## 2c. Gamut — отсечение по видимости

`AENG_calc_gamut()` вычисляет frustum в пространстве карты:

```c
// NGAMUT хранит для каждой строки Z видимый диапазон X:
for (z = NGAMUT_lo_zmin; z <= NGAMUT_lo_zmax; z++) {
    for (x = NGAMUT_lo_gamut[z].xmin; x <= NGAMUT_lo_gamut[z].xmax; x++) {
        NIGHT_cache_create(x, z);  // Только видимые ячейки получают освещение
    }
}
```

Gamut — coarse culling (по lo-res клеткам карты). Затем facets дополнительно проверяются по AABB.

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

**Сортировка полигонов (POLY_frame_draw):**
- `WE_NEED_POLYBUFFERS_PLEASE_BOB = 1` на PC — нужна Z-сортировка (polypage.h)
- `WE_NEED_POLYBUFFERS_PLEASE_BOB = 0` на DC — без сортировки (нет полибуферов)

**Детали POLY_frame_draw сортировки (poly.cpp:3283):**

Два типа страниц:
- **Непрозрачные** (`!RS.NeedsSorting()`): рендерятся в порядке индекса, с `POLY_PAGE_COLOUR` ПЕРВЫМ
- **Прозрачные** (`RS.NeedsSorting()`): bucket sort (global), затем render back-to-front

Bucket sort (основной путь, `#if 1` в коде):
```
buckets[2048] — глобальный массив, индекс = глубинный bucket
sort_z = max(pp->Z всех вершин полигона) = ZCLIP_PLANE/vz (reciprocal depth)
  → Z больше = ближе к камере (reciprocal!)
bucket = int(sort_z * 2048), clamp [0, 2047]
  → bucket[0]   = далёкие полигоны (sort_z мал)
  → bucket[2047] = близкие полигоны (sort_z велик)
Render loop: for i=0..2047 → порядок FAR TO NEAR = BACK-TO-FRONT ✓
```

Исключения из bucket sort:
- `POLY_PAGE_SHADOW`: скипается если `!draw_shadow_page`
- `POLY_PAGE_PUDDLE`: скипается из bucket sort, рендерится отдельно (`POLY_frame_draw_puddles()`)

Альтернативный путь (`#else`, ОТКЛЮЧЁН): `SortBackFirst()` — merge sort per page (ascending sort_z = far first). Заменён bucket sort как более быстрый.

После bucket render: `pa->RS.ResetTempTransparent()` для каждой alpha-страницы.

Финальная очистка: 3 страницы VB/IB освобождаются per frame (rolling cleanup, `iPageNumberToClear`).

**Fog colour:**
- Outdoor: `fog_colour = NIGHT_sky_colour.{red,green,blue}` → ARGB; night sky colour
- GF_SEWERS / GF_INDOORS: `fog_colour = 0` (чёрный)
- draw_3d mode: fog = average RGB (grayscale sky)
- Fog передаётся в `DefRenderState.InitScene(fog_colour)`

---

## 3b. Рендеринг персонажей и система мешей

Детальная документация: **[rendering_mesh.md](rendering_mesh.md)**

Краткое summary:
- `USE_TOMS_ENGINE_PLEASE_BOB = 1` — всегда включён, единственный активный путь в figure.cpp
- Vertex morphing (DrawTween): `tween ∈ [0,256]`, линейная интерполяция между двумя keyframes
- Anti-lights: отрицательные источники света вычитают яркость (тёмные зоны)
- `MESH_car_crumples[5уровней×8вариантов×6точек]` — предвычисленная деформация машин
- `MESH_draw_reflection()` + `PUDDLE_in()` — отражения в лужах (отдельный проход)
- `HIGH_REZ_PEOPLE_PLEASE_BOB` = закомментирован, высокополигональные модели не активны
- `InterruptFrame` = МЁРТВЫЙ КОД (всегда NULL в пре-релизе)

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

Детальная документация: **[rendering_lighting.md](rendering_lighting.md)**

Краткое summary:
- Vertex lighting (не per-pixel), запекается в вершины; `NIGHT_Colour` {r,g,b} 6-bit (0-63) → D3D через ×4
- Статические источники: NIGHT_Slight, из `.lgt` файлов; динамические: NIGHT_Dlight, до 64
- Статические тени: запекаются при загрузке, ray-cast к солнцу (dir +147,-148,-147) → PAP_FLAG_SHADOW
- LIGHT_range max = 0x600; типы: NORMAL/BROKEN/PULSE/FADE
- **Динамических теней НЕТ** — тени = flat sprite под ногами
- `NIGHT_generate_walkable_lighting()` = **МЁРТВЫЙ КОД** (return сразу после roof_walkable)

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

**Crinkle система:** — детали в [rendering_lighting.md](rendering_lighting.md)

Micro-geometry bump mapping (mesh → проекция на квад + выдавливание вдоль нормали).
**⚠️ ПОЛНОСТЬЮ ОТКЛЮЧЕНА** (`CRINKLE_load` = return NULL; `if(0)` в aeng.cpp). Не переносить.

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
- **Crinkle система** — не переносить (полностью отключена в оригинале)
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

---

## ⚠️ Game Logic в рендерере (drawxtra.cpp)

**КРИТИЧНО для портирования.** Два места где рендер-код МУТИРУЕТ game state:

1. **`DRAWXTRA_MIB_destruct()`** — модифицирует `ammo_packs_pistol`, создаёт `PYRO_TWANGER` и `SPARK`
2. **`PYRO_draw_armageddon()`** — создаёт `PYRO_NEWDOME`, `PARTICLE_Add()`, `MFX_play_xyz()`

При портировании эту логику **ВЫНЕСТИ из рендерера в game update loop**.
`engineMap.cpp` — безопасен (read-only для рендеринга).
