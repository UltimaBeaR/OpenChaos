# Stage 7 — Новый рендерер (девлог)

## Архитектура бэкенда

### Структура файлов

```
engine/graphics/graphics_engine/
├── graphics_engine.h               — контракт (единственное что видит игровой код)
├── graphics_engine_d3d.cpp         — реализация под D3D6 (первая)
└── graphics_engine_opengl.cpp      — реализация под OpenGL (вторая, позже)
```

- Игровой код включает только `graphics_engine.h`
- CMake-флаг (`GRAPHICS_BACKEND=D3D` / `=OPENGL`) выбирает какой .cpp компилировать
- Линкер подставляет нужную реализацию, никакого runtime dispatch
- Для сравнения: переключил флаг → пересобрал → сравнил визуально

### Принципы проектирования контракта

**⚠️ Эти принципы — руководство при выделении каждой функции бэкенда.**

1. **Абстрагировать в терминах "что нужно игре", не "что умеет D3D".**
   При создании каждой функции спрашивать: "а как бы я это реализовал на OpenGL?"
   Если ответ "с трудом" или "через костыль" — абстракция D3D-центричная, переделать.

2. **Таблица правильных vs неправильных абстракций:**

   | D3D-центричная (ПЛОХО) | Игро-центричная (ХОРОШО) | Почему |
   |---|---|---|
   | `lock_surface() / unlock_surface()` | `upload_pixels(texture, data, rect)` | OpenGL не имеет "поверхностей", но glTexSubImage2D делает то же самое |
   | `set_render_state(key, value)` | `set_blend_mode(Alpha)`, `set_depth_mode(ReadWrite)` | Группировка по смыслу, нативно в обоих API |
   | `create_surface(DDSD_*, DDSCAPS_*)` | `create_texture(width, height, format)` | DDraw флаги не имеют аналогов в OpenGL |
   | `blt(src, dst, rect, DDBLT_WAIT)` | `blit_to_screen(texture, rect)` | Blt — DDraw концепция, в OpenGL это draw quad |
   | `set_texture_stage_state(stage, op)` | `set_texture_filter(Nearest/Linear)` | Texture stages — D3D6 специфика |

3. **Непрозрачные хэндлы вместо API-специфичных указателей.**
   - `TextureHandle` (uint32 или struct) вместо `LPDIRECT3DTEXTURE2`
   - Бэкенд внутри себя маппит хэндл → GL texture ID / D3D pointer
   - Игровой код никогда не видит ни D3D ни GL типы

4. **Свои типы вместо D3D типов.**
   - Vertex structs вместо D3DTLVERTEX / D3DLVERTEX / D3DVERTEX
   - Enum'ы BlendMode, DepthMode, CullMode вместо D3DRENDERSTATE_*
   - Эти типы одинаково нативно реализуются и в D3D и в OpenGL

5. **Не проектировать "на вырост".** Абстрагировать только то что реально вызывается кодом игры. Не добавлять функции "на всякий случай".

---

## Инвентаризация D3D/DDraw вызовов

### Масштаб

- **44 файла** из 664 содержат D3D/DDraw вызовы (6.6% кодовой базы)
- **~1600+ ссылок** на D3D/DDraw API суммарно
- **SetRenderState — доминирует:** 802+ вхождений (половина всех ссылок)
- **DrawPrimitive — всего 29 вызовов** (основная точка отрисовки, их мало)

### Две параллельные D3D-системы

1. **Основная игра** — класс `Display` в `engine/graphics/graphics_api/`
2. **Outro/Demo** — независимый `OS_Framework` в `outro/` (61 D3D ссылка, полностью автономный)

### Три формата вершин

| Тип | Назначение | Вхождения |
|-----|-----------|-----------|
| `D3DTLVERTEX` (transformed & lit) | HUD, текст, 2D элементы | 21+ |
| `D3DLVERTEX` (lit, untransformed) | Динамическая геометрия (персонажи, эффекты) | 35+ |
| `D3DVERTEX` (untransformed, unlit) | Статические меши | 9+ |

### Классификация файлов по интенсивности D3D

**CRITICAL (50+ ссылок) — ядро:**
- `display.cpp` (67) — основной Display класс, управление поверхностями, flip, blit
- `outro_os.cpp` (61) — автономная D3D-система для демо/outro

**HIGH (15+ ссылок) — важные модули:**
- `dd_manager.cpp` (26) — перечисление устройств, форматов, режимов
- `truetype.cpp` (16) — рендер TrueType шрифтов в текстуры + альфа-блендинг
- `d3d_texture.cpp` (16) — создание/блокировка/освобождение текстур
- `gd_display.h` (17) — определение класса Display (хранит все D3D указатели)

**MEDIUM (5-14 ссылок) — рендер-пайплайн:**
- `aeng.cpp` (14) — анимированные персонажи, DrawIndexedPrimitive
- `frontend.cpp` (13) — меню, фон, Blt
- `polypage.cpp` (9) — полигонный рендер, DrawIndexedPrimitive
- `vertex_buffer.cpp` (9) — управление вершинными буферами
- `render_state.cpp` (5) — кэш рендер-стейтов

**LOW (<5 ссылок) — точечное использование:**
- `flamengine.cpp` — Viewport->Clear
- `texture.cpp` — TEXTURE_get_handle возвращает LPDIRECT3DTEXTURE2
- `superfacet.cpp`, `farfacet.cpp`, `figure.cpp`, `fastprim.cpp` — геометрия
- `frontend.h`, `frontend_globals.h` — объявления surface указателей
- `host.cpp` — RestoreAllSurfaces
- Заголовки с глобалами: `*_globals.h/cpp`

### Паттерны использования D3D

**1. Управление поверхностями (DDraw):**
`CreateSurface → Lock → Write → Unlock → Blt/Flip → Release`
- `display.cpp`, `d3d_texture.cpp`, `truetype.cpp`, `frontend.cpp`

**2. Отрисовка примитивов (D3D):**
`SetTexture → SetRenderState(×N) → DrawPrimitive/DrawIndexedPrimitive`
- `aeng.cpp`, `polypage.cpp`, `farfacet.cpp`, `figure.cpp`, `fastprim.cpp`, `truetype.cpp`
- Только D3DPT_TRIANGLELIST и D3DPT_TRIANGLEFAN

**3. Текстуры:**
- Создание: `CreateSurface + QueryInterface(IID_IDirect3DTexture2)`
- Доступ: централизован через `TEXTURE_get_handle(page)` → LPDIRECT3DTEXTURE2
- Биндинг: `Display::SetTexture()` с кэшированием в `render_state.cpp`

**4. Render states (802+ вхождений!):**
Основные используемые стейты:
- Alpha blending: ALPHABLENDENABLE, SRCBLEND, DESTBLEND
- Z-buffer: ZENABLE, ZWRITEENABLE, ZFUNC
- Texture filtering: TEXTUREMAG, TEXTUREMIN
- Texture blending: TEXTUREMAPBLEND
- Fog: FOGENABLE
- Culling: CULLMODE
- Shading: SHADEMODE
- Misc: SPECULARENABLE, ANTIALIAS, TEXTUREPERSPECTIVE

### Пайплайн отрисовки (поток данных)

```
Assets (texture.cpp)
    ↓ LPDIRECT3DTEXTURE2
Graphics API (display.cpp, d3d_texture.cpp, render_state.cpp)
    ↓ SetTexture, SetRenderState, DrawPrimitive
Rendering Pipeline (aeng.cpp, polypage.cpp, poly.cpp, bucket)
    ↓ builds vertices, sorts by z/texture
Geometry (superfacet, farfacet, figure, fastprim)
    ↓ static/animated geometry → vertex arrays
Text (truetype.cpp)
Frontend (frontend.cpp → Blt backgrounds)
Outro (outro_os.cpp — independent pipeline)
```

---

## Выводы из инвентаризации

### Хорошие новости
1. **DrawPrimitive — всего 29 вызовов.** Основная точка отрисовки компактная.
2. **Текстуры уже централизованы** через `TEXTURE_get_handle()` — хороший кандидат для абстракции.
3. **Graphics API уже выделен** в `engine/graphics/graphics_api/` — ядро D3D в ~9 файлах.
4. **Только TRIANGLELIST и TRIANGLEFAN** — никаких strips, points, lines (кроме outro).

### Сложности
1. **SetRenderState — 802+ вхождений.** Размазаны по всему рендер-пайплайну. Но большинство — через макросы из `display_macros.h`, что упрощает замену.
2. **Outro — отдельная D3D-система.** Нужно заменять независимо от основной игры.
3. **D3D типы в сигнатурах функций** — `LPDIRECT3DTEXTURE2` торчит из `texture.h`, `frontend.h`, `polypage.h`, геометрических модулей. Нужно заменить на TextureHandle.
4. **Три формата вершин** — нужно три (или одну унифицированную) свою структуру.

---

## Валидация инвентаризации

Первый анализ искал по известным D3D идентификаторам из памяти модели.
Валидация: поиск от кода — все идентификаторы с префиксами D3D*, DD*, LPDIRECT*, IDirect*, IID_*.

### Откуда приходят D3D хедеры

D3D хедеры — системные (Windows SDK), не локальные. Включаются в 10 файлах:
- `engine/platform/uc_common.h` → `<d3dtypes.h>`, `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/graphics_api/dd_manager.h` → `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/graphics_api/d3d_texture.h` → `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/graphics_api/vertex_buffer.h` → `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/graphics_api/render_state.h` → `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/geometry/fastprim_globals.h` → `<d3d.h>`
- `engine/graphics/text/truetype_globals.h` → `<ddraw.h>`, `<d3d.h>`
- `engine/graphics/pipeline/bucket.h` → `<ddraw.h>`, `<d3d.h>`
- `outro/core/outro_os_globals.h` → `<ddraw.h>`, `<d3d.h>`
- `outro/core/outro_os.cpp` → `<ddraw.h>`, `<d3d.h>`, `<d3dtypes.h>`

### Что пропустил первый анализ

**Ложная тревога: DDEngine (187 файлов) и DDLibrary (56 файлов)**
Это НЕ классы — это имена оригинальных директорий (`fallen/DDEngine/`, `fallen/DDLibrary/`).
187 файлов содержат `// uc_orig: DDEngine/...` комментарии-трассировку, а не D3D вызовы.

**Реальные пропуски — идентификаторы не найденные первым проходом:**

Texture stage states (11 идентификаторов):
- `D3DTSS_COLOROP`, `D3DTSS_COLORARG1`, `D3DTSS_COLORARG2`
- `D3DTSS_ALPHAOP`, `D3DTSS_ALPHAARG1`, `D3DTSS_ALPHAARG2`
- `D3DTSS_TEXCOORDINDEX`, `D3DTSS_MINFILTER`, `D3DTSS_MAGFILTER`, `D3DTSS_MIPFILTER`
- `D3DTSS_ADDRESS`

Texture operations:
- `D3DTOP_DISABLE`, `D3DTOP_SELECTARG1`, `D3DTOP_MODULATE`
- `D3DTA_DIFFUSE`, `D3DTA_TEXTURE`, `D3DTA_CURRENT`
- `D3DTBLEND_MODULATE` (5 файлов), `D3DTBLEND_MODULATEALPHA` (5), `D3DTBLEND_DECAL` (3)

Texture filtering:
- `D3DTFG_POINT`, `D3DTFG_LINEAR`, `D3DTFN_LINEAR`, `D3DTFP_NONE`

Texture addressing:
- `D3DTADDRESS_WRAP` (4 файла), `D3DTADDRESS_CLAMP` (3)

Lock флаги:
- `DDLOCK_WAIT` (6 файлов), `DDLOCK_NOSYSLOCK` (3), `DDLOCK_WRITEONLY` (1), `DDLOCK_SURFACEMEMORYPTR` (1)

Flip:
- `DDFLIP_WAIT` (3 файла)

Transform states:
- `D3DTRANSFORMSTATE_WORLD`, `D3DTRANSFORMSTATE_VIEW`, `D3DTRANSFORMSTATE_PROJECTION` (2)

Comparison:
- `D3DCMP_ALWAYS`, `D3DCMP_NOTEQUAL`, `D3DCMP_LESS`, `D3DCMP_LESSEQUAL` (4), `D3DCMP_GREATER`, `D3DCMP_GREATEREQUAL`

Culling:
- `D3DCULL_NONE` (4), `D3DCULL_CCW` (3), `D3DCULL_CW` (1), `D3DFILL_SOLID` (1)

Blend modes (дополнительные):
- `D3DBLEND_ZERO` (4), `D3DBLEND_SRCCOLOR` (2), `D3DBLEND_INVSRCCOLOR` (2), `D3DBLEND_DESTCOLOR` (1)

Clear:
- `D3DCLEAR_TARGET` (3), `D3DCLEAR_ZBUFFER` (1)

Draw flags:
- `D3DDP_DONOTUPDATEEXTENTS` (3), `D3DDP_DONOTLIGHT` (3), `D3DDP_DONOTCLIP` (1)

DDraw surface caps (дополнительные):
- `DDSCAPS_PRIMARYSURFACE`, `DDSCAPS_BACKBUFFER`, `DDSCAPS_VIDEOMEMORY`, `DDSCAPS_ZBUFFER`
- `DDSCAPS_3DDEVICE`, `DDSCAPS_COMPLEX`, `DDSCAPS_FLIP`, `DDSCAPS_MIPMAP`, `DDSCAPS_OWNDC`
- `DDSCAPS2_TEXTUREMANAGE` (2), `DDSCAPS2_HINTDYNAMIC`, `DDSCAPS2_HINTSTATIC`

DDraw blit (дополнительные):
- `DDBLT_COLORFILL` (2), `DDBLTFX` (2)

Palette/Gamma:
- `DDPCAPS_8BIT`, `DDPCAPS_INITIALIZE`, `DDGAMMARAMP`
- `IDirectDrawPalette`, `IDirectDrawGammaControl`

Pixel format (дополнительные):
- `DDPF_PALETTEINDEXED1`, `DDPF_PALETTEINDEXED2`, `DDPF_PALETTEINDEXED4`

Enumeration:
- `D3DENUMRET_OK`, `D3DENUMRET_CANCEL`, `DDENUMRET_OK`, `DDENUMRET_CANCEL`

Error codes:
- `DDERR_GENERIC`, `DDERR_INVALIDPARAMS`, `DDERR_SURFACELOST`, `DD_OK` (8), `D3D_OK`

Interfaces (дополнительные):
- `LPDIRECT3D3`, `LPDIRECT3DVIEWPORT3`, `LPDIRECT3DMATERIAL3`
- `IDirect3D3`, `IDirect3DDevice3`, `IDirect3DVertexBuffer`
- `IDirectDraw`, `IDirectDraw4`, `IDirectDrawSurface4`
- `IID_IDirectDraw4`, `IID_IDirect3D3`, `IID_IDirect3DRGBDevice`, `IID_IDirect3DMMXDevice`
- `IID_IDirectDrawGammaControl`

Матрицы и типы (дополнительные):
- `D3DMATRIX` (20 файлов!), `D3DMULTIMATRIX` (7), `D3DVIEWPORT2` (10)
- `D3DDEVICEDESC` (3), `D3DVECTOR` (3), `D3DCOLORVALUE` (1), `D3DCOLOR` (3)
- `D3DRECT` (1), `D3DMATERIAL` (1), `D3DMATERIALHANDLE` (1), `D3DVAL` (1)
- `DDCOLORKEY` (1), `DDCAPS` (2)

Проектные классы на базе D3D:
- `D3DTexture` (19 файлов) — класс с `LPDIRECT3DTEXTURE2` и `LPDIRECTDRAWSURFACE4` внутри
- `D3DPage` — упакованные текстурные страницы
- `D3DDeviceInfo` (7), `DDDriverInfo` (6), `DDModeInfo` (6), `DDDriverManager` (4)
- `DD_DRIVER_*` флаги (16 штук) — кастомные, не из SDK

### Уточнённый масштаб

- **D3DMATRIX в 20 файлах** — это больше чем ожидалось, нужна своя матричная структура
- **D3DTexture в 19 файлах** — проектный класс, его интерфейс нужно абстрагировать
- **D3DVIEWPORT2 в 10 файлах** — viewport данные используются широко
- **D3DLVERTEX в 13 файлах** (было 35+ вхождений, но в 13 уникальных файлах)

---

## План работы

### Шаг 1 — Отключить outro
Закомментировать outro .cpp файлы в CMakeLists.txt. Убрать вызовы outro из игрового кода
(stub/ifdef). Проверить что игра компилируется и работает без outro.

### Шаг 2 — Выделить graphics_engine для основной игры
Итеративно вытаскивать D3D вызовы из игрового кода в `graphics_engine.h` / `graphics_engine_d3d.cpp`.
Конечная цель: D3D хедеры включаются ТОЛЬКО в `graphics_engine_d3d.cpp` и в `graphics_api/` (внутренности бэкенда).
Остальной код игры видит только `graphics_engine.h` с чистыми типами.
Проверка: игра компилируется, запускается, работает как раньше.

### Шаг 3 — Включить outro, повторить для него
Раскомментировать outro. Выделить его D3D вызовы в `graphics_engine.h` / `graphics_engine_d3d.cpp`
(возможно с дополнительными функциями для outro-специфичных нужд).
Проверка: игра + outro работают как раньше, D3D не торчит наружу.

### Шаг 4 — OpenGL реализация для основной игры
Отключить outro снова. Создать `graphics_engine_opengl.cpp`, реализующий тот же контракт.
Переключить CMake на OpenGL бэкенд. Запустить, проверить, фиксить проблемы.

### Шаг 5 — OpenGL реализация для outro
Включить outro. Дописать OpenGL реализацию outro-специфичных функций.
Проверить всё вместе.

### Шаг 6 — Финализация
Убедиться что оба бэкенда работают. Удалить D3D бэкенд (или оставить для референса).
Убрать DirectX зависимости из CMake/vcpkg.
