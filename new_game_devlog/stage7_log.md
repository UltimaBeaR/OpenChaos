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

## Прогресс

### Шаг 1 — Отключение outro ✅

Выполнено:
- CMakeLists.txt: закомментированы все 18 outro .cpp файлов
- `game.cpp:931-937`: закомментирован `OS_hack()` после финальной миссии (Finale1.ucm)
- `frontend.cpp:3551-3552`: закомментирован `OS_hack()` из меню кредитов (FE_CREDITS)
- Точка входа в outro из основной игры — единственная функция `OS_hack()` (определена в `outro_os.cpp`)
- Сборка: 307/307, линковка ОК, exit code 0
- Проверка: игра запускается, outro не вызывается

---

## Анализ API surface (что игровой код вызывает)

### Текущая абстракция

Центр — класс `Display` (`gd_display.h`), глобал `the_display`.
Макросы в `display_macros.h` оборачивают основные вызовы.
Но многие файлы лезут напрямую в D3D-указатели на `the_display`.

### Что игровой код реально вызывает (вне graphics_api/)

**Через макросы (легко заменить):**
- `BEGIN_SCENE` / `END_SCENE` — 6 файлов
- `DRAW_PRIMITIVE` / `DRAW_INDEXED_PRIMITIVE` — aeng, truetype
- `REALLY_SET_RENDER_STATE` — aeng, poly, truetype
- `REALLY_SET_TEXTURE` / `REALLY_SET_NO_TEXTURE` — aeng, truetype
- `CLEAR_VIEWPORT` — aeng, qeng
- `FLIP` — aeng, qeng
- `SET_*_BACKGROUND` — aeng, qeng

**Через the_display методы (средняя сложность):**
- `RunCutscene()` — game.cpp, elev.cpp
- `create/blit/destroy_background_surface()` — game.cpp
- `SetGamma()`, `GetGamma()`, `IsGammaAvailable()` — game_tick, frontend
- `screen_lock()` / `screen_unlock()` — aeng (14 мест!), font
- `PlotPixel()`, `PlotFormattedPixel()`, `GetPixel()` — sky, font, game_tick
- `blit_back_buffer()` — aeng
- `SetUserColour()` — aeng
- `toGDI()` / `fromGDI()` — host, elev
- `RemoveAllLoadedTextures()` — texture.cpp
- `GetDeviceInfo()->...()` — poly_render, figure, aeng, texture (capability queries)

**Прямой доступ к D3D указателям (самое проблемное):**
- `the_display.lp_D3D_Device->SetRenderState()` — fastprim, figure
- `the_display.lp_D3D_Device->SetTransform()` — farfacet, poly
- `the_display.lp_D3D_Device->DrawIndexedPrimitive()` — aeng
- `the_display.lp_D3D_Device->SetTexture()` — fastprim
- `the_display.lp_D3D_Device` передаётся в `DrawIndPrimMM()` — aeng, fastprim, figure
- `the_display.lp_D3D_Viewport->SetViewport2()` — poly, attract, frontend, overlay
- `the_display.lp_D3D_Viewport->Clear()` — frontend, flamengine
- `the_display.lp_DD4->CreateSurface()` — frontend, truetype
- `the_display.lp_DD4->CreatePalette()` — truetype
- `the_display.lp_DD4->RestoreAllSurfaces()` — host
- `the_display.lp_DD_BackSurface->Blt()` — frontend, flamengine
- `the_display.lp_DD_BackSurface->GetSurfaceDesc()` — frontend
- `the_display.lp_DD_FrontSurface->Lock/Unlock()` — figure (screen capture?)

**Данные-члены (не методы):**
- `the_display.ViewportRect` — attract, frontend, flamengine
- `the_display.screen`, `screen_pitch`, `screen_width`, `screen_height` — font, wibble, sky
- `the_display.DisplayRect` — wind_procs
- `the_display.CurrMode->GetBPP()` — wibble
- `the_display.CurrDevice->IsHardware()` — elev
- `the_display.lp_DD_Background_use_instead` — frontend

### `DrawIndPrimMM` — отдельная проблема

Кастомная функция batched рендеринга с multi-matrix трансформацией.
5 вызовов: aeng, fastprim, figure. Все передают `the_display.lp_D3D_Device`.
Нужно обернуть в graphics_engine.

### Порядок миграции (Шаг 2)

Итеративно, от простого к сложному, каждый шаг компилируется:

**2a) Создать каркас graphics_engine.h + graphics_engine_d3d.cpp**
Пустые файлы, добавить в CMake, проверить компиляцию.

**2b) Типы и enum'ы**
Определить в graphics_engine.h: TextureHandle, BlendMode, DepthMode, CullMode, и т.д.
Пока не менять игровой код — только определить типы.

**2c) Макросы → функции graphics_engine**
Заменить `display_macros.h` → вызовы через graphics_engine.
BEGIN_SCENE/END_SCENE, CLEAR_VIEWPORT, FLIP, SET_*_BACKGROUND.
Это затрагивает все файлы что используют макросы, но замена механическая.

**2d) Render states**
Заменить REALLY_SET_RENDER_STATE → функции graphics_engine с enum'ами.
Сгруппировать по смыслу: set_blend_mode(), set_depth_mode(), set_texture_filter() и т.д.

**2e) Draw calls**
Заменить DRAW_PRIMITIVE/DRAW_INDEXED_PRIMITIVE → ge_draw_triangles() и т.п.
Заменить DrawIndPrimMM → обёртка в graphics_engine.

**2f) Текстуры**
TextureHandle вместо LPDIRECT3DTEXTURE2 в сигнатурах.
TEXTURE_get_handle() возвращает TextureHandle.
REALLY_SET_TEXTURE → ge_bind_texture(TextureHandle).

**2g) Viewport и трансформации**
Обернуть SetViewport2, SetTransform, Clear.

**2h) Surface operations**
Обернуть CreateSurface, Lock/Unlock, Blt — frontend, truetype, flamengine.

**2i) Device queries**
Обернуть GetDeviceInfo()->XxxSupported() → функции/константы.

**2j) Screen buffer access**
Обернуть screen_lock/unlock, PlotPixel, screen/screen_pitch.

**2k) Прямой D3D доступ**
Убрать оставшийся прямой доступ к lp_D3D_Device, lp_DD4 и т.д.
К этому моменту большинство уже обёрнуто предыдущими шагами.

**2l) Финальная проверка**
Убедиться что D3D хедеры не включаются нигде кроме graphics_engine_d3d.cpp
и файлов внутри graphics_api/ (которые станут частью бэкенда).

---

## Правила работы на Этапе 7

1. **После каждой итерации — запись в девлог.** Что сделано, какие файлы затронуты, что изменилось.
2. **После каждой итерации — само-ревью.** Проверить: компиляция, diff корректен, нет сломанных include, нет случайных изменений логики.
3. **Каждая итерация компилируется.** Не накапливать сломанное состояние.

---

## Прогресс Шага 2 — Выделение graphics_engine

### 2a) Каркас ✅

Создано:
- `engine/graphics/graphics_engine/graphics_engine.h` — контракт
- `engine/graphics/graphics_engine/graphics_engine_d3d.cpp` — D3D реализация
- Добавлено в CMakeLists.txt
- Сборка: 308/308, линковка ОК

Что в контракте:
- Типы: GEVertexTL, GEVertexLit, GEVertex (с static_assert на совпадение layout с D3D), GETextureHandle
- Enum'ы: BlendMode, DepthMode, CullMode, TextureFilter, TextureBlend, TextureAddress, CompareFunc, PrimitiveType, Background
- Frame: ge_begin_scene, ge_end_scene, ge_clear, ge_flip
- Render state: ge_set_blend_mode, ge_set_depth_mode, ge_set_depth_func, ge_set_cull_mode, ge_set_texture_filter, ge_set_texture_blend, ge_set_texture_address, ge_set_fog_enabled, ge_set_specular_enabled, ge_set_perspective_correction
- Draw: ge_draw_primitive (TL), ge_draw_indexed_primitive (TL), ge_draw_indexed_primitive_lit, ge_draw_indexed_primitive_unlit
- Viewport: ge_set_viewport
- Background: ge_set_background, ge_set_background_color
- Texture: ge_bind_texture

Находки:
- D3DLVERTEX имеет dwReserved между z и color — GEVertexLit нужен _reserved padding
- SET_BLUE_BACKGROUND — мёртвый макрос (метод SetBlueBackground не существует в Display), убран из enum'а

### 2c) Замена макросов → ge_* вызовы ✅

Заменены макросы BEGIN_SCENE, END_SCENE, CLEAR_VIEWPORT, FLIP, SET_*_BACKGROUND, SetUserColour
на ge_begin_scene(), ge_end_scene(), ge_clear(), ge_flip(), ge_set_background(), ge_set_background_color().

Затронутые файлы:
- `aeng.cpp` — 13 замен (2× BEGIN_SCENE, 2× END_SCENE, 3× SET_BLACK_BACKGROUND, 2× CLEAR_VIEWPORT, 1× FLIP, 3× SetUserColour, 1× SetUserBackground → ge_set_background(User), 1× ClearViewport)
- `poly.cpp` — 6 замен (3× BEGIN_SCENE, 3× END_SCENE)
- `qeng.cpp` — 3 замены (FLIP, SET_BLACK_BACKGROUND, CLEAR_VIEWPORT). Полностью убран include display_macros.h — больше не нужен.
- `truetype.cpp` — 2 замены (BEGIN_SCENE, END_SCENE)

Все файлы получили `#include "engine/graphics/graphics_engine/graphics_engine.h"`.
Макросы REALLY_SET_*, DRAW_*PRIMITIVE пока остаются — мигрируются на следующих подшагах.

Проверка: вне graphics_api/ и graphics_engine/ эти макросы не используются (outro отключен).
Сборка: 308/308, линковка ОК.

### 2d) Замена REALLY_SET_RENDER_STATE → ge_set_* (частично) ✅

Заменены блоки render state настройки в трёх файлах:

- `truetype.cpp:551-558` → ge_set_texture_filter, ge_set_texture_blend, ge_set_depth_mode, ge_set_blend_mode
  (8 D3D вызовов → 4 ge_ вызова, семантически сгруппированы)
- `aeng.cpp:2895-2903` → ge_set_texture_blend, ge_set_depth_mode, ge_set_blend_mode
  (7 D3D вызовов → 3-4 ge_ вызова)
- `poly.cpp:1934-1937` → ge_set_blend_mode(Additive), ge_set_depth_bias(2)
  (4 D3D вызова → 2 ge_ вызова)

Добавлено в контракт: `ge_set_depth_bias(int32_t bias)` для D3DRENDERSTATE_ZBIAS.

Оставлено на потом: poly.cpp debug render path (FORCE_SET_RENDER_STATE) — интеграция с render state cache (RenderState::s_State), нужен отдельный подход.

Остаток REALLY_SET_RENDER_STATE вне бэкенда: 2 вхождения в poly.cpp (debug path).
Сборка: 308/308, линковка ОК.

### 2e-2f) Замена DRAW_INDEXED_PRIMITIVE и REALLY_SET_TEXTURE → ge_* ✅

Заменены:

- `truetype.cpp:612` — DRAW_INDEXED_PRIMITIVE → ge_draw_indexed_primitive (TL vertex, text quad)
- `truetype.cpp:558` — REALLY_SET_TEXTURE → ge_bind_texture
- `aeng.cpp:2904,2907` — DRAW_INDEXED_PRIMITIVE × 2 → ge_draw_indexed_primitive (water/effect)
- `aeng.cpp:3217` — REALLY_SET_TEXTURE → ge_bind_texture

TextureHandle пока = reinterpret_cast из LPDIRECT3DTEXTURE2 (временно, для D3D бэкенда).
Proper texture handle mapping — при полной миграции текстурной системы.

Остаток вне бэкенда: 1× FORCE_SET_TEXTURE в poly.cpp debug path (отложен).

### Прямой D3D доступ: SetRenderState, SetTransform, DrawIndexedPrimitive ✅

Заменены прямые `the_display.lp_D3D_Device->` вызовы:

- `fastprim.cpp:831-877` — 9× SetRenderState + 2× SetTexture + 1× DrawIndexedPrimitive → ge_set_blend_mode, ge_bind_texture, ge_draw_indexed_primitive_lit, ge_set_texture_blend
- `figure.cpp:2089,2166,2712,2779,4311,4397` — 6× SetRenderState(SPECULARENABLE) → ge_set_specular_enabled
- `farfacet.cpp:554,608` — 2× SetTransform(PROJECTION) → ge_set_transform
- `poly.cpp:280,300,546,579` — 4× SetTransform(VIEW/PROJECTION/WORLD) → ge_set_transform

Добавлено в контракт:
- `GEMatrix` struct (4×4 float, совпадает с D3DMATRIX по layout)
- `GETransform` enum (World, View, Projection)
- `ge_set_transform(GETransform, const GEMatrix*)`
- `ge_set_depth_bias(int32_t)`

D3D6 SetTransform принимает non-const LPD3DMATRIX → const_cast в D3D реализации.

Также заменены:
- `aeng.cpp:1943,2317` — 2× DrawIndexedPrimitive(LVERTEX, dirt particles) → ge_draw_indexed_primitive_lit
- `attract.cpp:66-104` — полная инициализация render state (30 строк D3D) → 11 ge_ вызовов

Оставшийся lp_D3D_Device доступ (13 вхождений) — всё внутренности D3D бэкенда:
- 5× DrawIndPrimMM (fastprim, figure×3, aeng) — custom multi-matrix D3D batching
- 8× PolyPage::Render/DrawSinglePoly (poly.cpp) — vertex buffer D3D path

Эти файлы — **часть D3D бэкенда**, не игровой код. При OpenGL будут переписаны целиком.

Сборка: 308/308.

### Viewport миграция ✅

Все `lp_D3D_Viewport` вызовы вне бэкенда заменены на ge_set_viewport / ge_clear:
- `flamengine.cpp:480` — Viewport->Clear(TARGET) → ge_clear(true, false)
- `overlay.cpp:261-274` — SetViewport2 (15 строк D3D) → ge_set_viewport (1 строка). Убран include gd_display.h.
- `frontend.cpp:2352-2366` — SetViewport2 + Clear → ge_set_viewport + ge_clear
- `frontend.cpp:3144` — Clear(ZBUFFER) → ge_clear(false, true)
- `poly.cpp:320` — SetViewport2 → ge_set_viewport (g_viewData остаётся как глобал — используется в GenerateMMMatrix)

Результат: lp_D3D_Viewport = 0 вхождений вне бэкенда.

### Анализ DDraw surface operations — часть бэкенда, не абстрагируем

Проанализированы все 17 оставшихся DDraw обращений:

- **figure.cpp `DeadAndBuried()`** — debug, рисует на FrontSurface. Часть D3D рендерера персонажей.
- **host.cpp** — RestoreAllSurfaces, toGDI/fromGDI. Платформенный D3D код, при OpenGL не нужен.
- **truetype.cpp** — CreateSurface (8-bit paletted для GDI font). При OpenGL — FreeType/stb прямо в текстуру.
- **frontend.cpp** — CreateSurface, Blt, Background_use_instead. DDraw surface для фонов меню.
- **flamengine.cpp** — Blt с BackSurface (feedback эффект).

**Решение:** не абстрагировать через ge_. Это D3D-центричные операции (surfaces, palettes, Blt, device-lost) — при OpenGL переписываются целиком.

### Итог Шага 2 ✅

**Мигрировано на ge_*:**
- ✅ Frame: begin/end scene, clear, flip
- ✅ Background: set_background, set_background_color
- ✅ Render state: blend, depth, depth_func, depth_bias, cull, texture filter/blend/address, fog, specular, perspective
- ✅ Textures: bind_texture
- ✅ Draw: draw_primitive (TL), draw_indexed_primitive (TL/Lit/Unlit)
- ✅ Transforms: set_transform (World/View/Projection)
- ✅ Viewport: set_viewport

**D3D бэкенд internals (переписываются при OpenGL, не абстрагируются):**
- PolyPage::Render/DrawSinglePoly (8) — vertex buffer batching
- DrawIndPrimMM (5) — multi-matrix batching
- FORCE_SET_* debug path (2) — render state cache
- DDraw surface ops (17) — surfaces, Blt, palettes, device-lost
- figure DeadAndBuried (2) — debug front surface draw
- host device management (6) — toGDI, RestoreAll, IsDisplayChanged

### Изоляция D3D хедеров из uc_common.h ✅

Убраны `<d3dtypes.h>`, `<ddraw.h>`, `<d3d.h>` из `uc_common.h` (umbrella хедер, 145 файлов).
Ожидали массовые поломки — оказалось только 2 хедера нуждались в прямых D3D includes:
- `figure_globals.h` — добавлено `<ddraw.h>` + `<d3d.h>` (D3DVERTEX, D3DMATRIX, D3DCOLOR)
- `frontend.h` — добавлено `<ddraw.h>` (LPDIRECTDRAWSURFACE4)

Все остальные файлы бэкенда уже имели свои D3D includes через graphics_api/ хедеры.
Сборка: 308/308, 0 ошибок.

### Реструктуризация: graphics_api/ → graphics_engine/d3d/ ✅

Перенесены все файлы из `engine/graphics/graphics_api/` в `engine/graphics/graphics_engine/d3d/`.
`graphics_engine_d3d.cpp` тоже перемещён в `d3d/`.

Итоговая структура:
```
engine/graphics/graphics_engine/
├── graphics_engine.h           — контракт (игровой код включает только это)
└── d3d/                        — D3D бэкенд целиком
    ├── graphics_engine_d3d.cpp — адаптер ge_* → D3D
    ├── display.cpp             — Display класс
    ├── dd_manager.cpp          — DDraw device enumeration
    ├── d3d_texture.cpp         — D3D texture management
    ├── render_state.cpp        — render state caching
    ├── vertex_buffer.cpp       — vertex buffer management
    ├── work_screen.cpp         — offscreen rendering
    └── *_globals.*             — глобалы
```

Обновлено: 60 файлов (include paths), CMakeLists.txt.
Папка `graphics_api/` удалена.

### Попытка замены D3D типов → GE типы в geometry/pipeline

Попробовал заменить `D3DLVERTEX` → `GEVertexLit` в файлах вне d3d/.
**Не работает** — эти файлы используют D3D union alias members (`dvX`, `dvY`, `dvZ`, `dvTU`, `dvTV`,
`dvSX`, `dvSY`, `dcColor`, `dcSpecular`, `dvRHW`). GEVertexLit имеет только `x`, `y`, `z`, `u`, `v`.

Замена потребовала бы переименование ВСЕХ member access — по сути переписывание рендер-пайплайна.
При OpenGL он и так будет переписан. Откачено.

### Реальная граница D3D бэкенда

D3D код живёт не только в `graphics_engine/d3d/`, но и в:
- `engine/graphics/geometry/` — farfacet, fastprim, figure, superfacet, sprite, sky, shape
- `engine/graphics/pipeline/` — aeng, poly, polypage, poly_render, qeng
- `engine/graphics/text/` — truetype, font, text
- `engine/graphics/postprocess/` — wibble, bloom
- `engine/effects/` — flamengine
- `ui/frontend/` — frontend, attract
- `engine/platform/` — host, wind_procs
- `assets/` — texture

Эти файлы — **D3D рендер-пайплайн**, при OpenGL переписываются целиком.
`graphics_engine/d3d/` содержит ядро D3D, geometry/pipeline/text — потребители ядра.

ge_* абстракция работает на границе: игровой код (things/, game/, combat/, ai/, missions/) →
ge_* → D3D бэкенд. Вызовы ge_* в pipeline файлах — точки соприкосновения.

### Замена D3D типов на GE типы ✅

Добавлены union aliases в GEVertexTL, GEVertexLit, GEVertex (sx/dvSX/dvX/tu/dvTU и т.д.)
и GEMatrix (_11.._44) для совместимости с legacy member access.

Массовая замена sed'ом:
- `D3DLVERTEX` → `GEVertexLit` (13 файлов, ~50 вхождений)
- `D3DTLVERTEX` → `GEVertexTL` (bucket.h, polypage.cpp, polypoint.h)
- `D3DVERTEX` → `GEVertex` (figure_globals.h/cpp)
- `D3DMATRIX` → `GEMatrix` (farfacet, fastprim, figure, aeng, poly — ~20 вхождений)

Пограничные casts на стыке GE ↔ D3D типов:
- `GEMatrix*` → `LPD3DMATRIX` при заполнении `D3DMULTIMATRIX` (fastprim, figure×3, aeng)
- `uint32_t*` → `ULONG*` при вызове `NIGHT_get_d3d_colour` (fastprim, superfacet)
- `D3DTLVERTEX*` → `GEVertexTL*` при получении из vertex buffer pool (polypage)

Добавлены `#include graphics_engine.h` в: farfacet_globals.h, superfacet_globals.h,
superfacet.cpp, aeng_globals.h, polypage.cpp, figure_globals.h, fastprim_globals.h.
Убраны D3D includes из bucket.h и polypoint.h.

Оставшиеся D3D идентификаторы вне d3d/ (следующая итерация):
- D3DTexture класс, LPDIRECT3DTEXTURE2, D3DTEXTURE_TYPE_* — текстурная система
- D3DRENDERSTATE_*, D3DPT_*, D3DFVF_*, D3DDP_* — константы в farfacet, fastprim
- D3DMULTIMATRIX — struct в fastprim, figure, aeng
- D3DCOLOR, D3DVECTOR — в figure_globals.h
- DDraw surface ops — flamengine, frontend

Сборка: 308/308, 0 ошибок.

### Анализ оставшихся D3D идентификаторов

~700+ вхождений D3D констант вне d3d/ — подавляющее большинство через
`RenderState::SetRenderState(D3DRENDERSTATE_*, D3DBLEND_*)` в poly_render.cpp.

RenderState — кэширующий слой D3D render states. Game code (poly_render) настраивает
per-page render states, PolyPage flush'ит их в D3D device.

Чтобы убрать D3D отсюда нужен один из подходов:
A) RenderState принимает GE enum'ы вместо D3D констант
B) RenderState целиком в d3d/, game code ходит через ge_set_*
Оба требуют переписывания poly_render.cpp.

Также остаются: D3DTexture класс (14 файлов), D3DMULTIMATRIX (11), LPDIRECT3DTEXTURE2 (16),
D3DCOLOR/D3DVECTOR (figure), D3DVIEWPORT2 globals (10), DDraw surface ops (frontend, flame, truetype).

### Замена D3DCOLOR, D3DVECTOR, D3DVIEWPORT2, D3DMULTIMATRIX, texture types ✅

- `D3DCOLOR` → `ULONG` (figure_globals)
- `D3DVECTOR` → `GEVector` (figure, figure_globals) — добавлен `GEVector{x,y,z}`
- `D3DVIEWPORT2` → `GEViewport` (poly, polypage, figure) — добавлен `GEViewport` с legacy dwX/dwY aliases
- `D3DMULTIMATRIX` → `GEMultiMatrix` (fastprim, figure, aeng, polypage) — заменён D3D pointer types на GE*
- `D3DTEXTURE_TYPE_UNUSED` → `GE_TEXTURE_TYPE_UNUSED` (texture.cpp)
- `TEXTURE_get_handle()` — возвращает `GETextureHandle` вместо `LPDIRECT3DTEXTURE2`
- Убраны лишние `reinterpret_cast<GETextureHandle>` в caller'ах (poly_render, attract)

Оставшиеся D3D идентификаторы вне d3d/ (~90 вхождений):
- D3DTexture класс (14) — D3D texture management, переписывается при OpenGL
- DDLibClass (12) + DDLibShellProc (3) — window management (host/wind_procs)
- LPDIRECT3DTEXTURE2 (7) — в fastprim.h, polypage, text (returns from D3DTexture methods)
- DDraw surface ops (~20) — truetype font surface, frontend backgrounds
- D3D constants (~15) — D3DFVF_*, D3DPT_* in DrawIndPrimMM, farfacet
- D3DObj/D3DPeopleObj (8) — figure draw cache objects
- Device info types (5) — DDDriverInfo, DDModeInfo, D3DDeviceInfo queries

Всё оставшееся — D3D бэкенд internals: texture class, surface creation, device enum,
DrawIndPrimMM parameters. При OpenGL эти модули переписываются целиком.

Сборка: 308/308.

---

## Требования к архитектуре (от пользователя, обязательны)

1. **Публичный контракт графического движка — API agnostic.** Ничего D3D/OpenGL-специфичного.
2. **Весь D3D код — в d3d/ папке.** Ни один файл вне d3d/ не включает D3D хедеры напрямую или транзитивно.
3. **Никакой игровой логики внутри реализации графического API.** Только код отрисовки.

### Что нужно сделать для выполнения требований

**Проблема:** `texture.h` включает `d3d_texture.h` → D3D типы утекают в 200+ файлов.
`D3DTexture` класс (D3D-specific) используется в публичном API: `TEXTURE_texture[]`, `TEXTURE_get_D3DTexture()`.

**Решение:**

**A. Разрезать texture.h:**
- Публичная часть (`texture.h`): только `GETextureHandle`, `TEXTURE_get_handle()`, `TEXTURE_load()` и т.д.
  Без `D3DTexture*`, без `d3d_texture.h` include.
- D3D internal: `TEXTURE_get_D3DTexture()` и `TEXTURE_texture[]` → доступны только из d3d/.

**B. Убрать прямые D3D SDK includes из 5 файлов вне d3d/:**
- `fastprim_globals.h` → `<d3d.h>` нужен для... проверить, может уже не нужен
- `figure_globals.h` → `<ddraw.h>` `<d3d.h>` — нужны для D3DTexture-related types?
- `font.cpp` → `<d3d.h>` — для D3DTexture font access?
- `truetype_globals.h` → `<ddraw.h>` `<d3d.h>` — для DDraw surface types
- `frontend.h` → `<ddraw.h>` — для LPDIRECTDRAWSURFACE4

**C. Для каждого файла с прямым D3D SDK include — варианты:**
1. Убрать D3D include если тип уже заменён на GE*
2. Если файл реально D3D-зависим (truetype DDraw surface, frontend DDraw surface) — перенести в d3d/
3. Если файл содержит смесь game logic + D3D — разрезать на две части

**D. Файлы которые нужно разрезать или перенести:**
- `truetype.cpp` — D3D font rendering → d3d/ (при OpenGL: FreeType/stb)
- `frontend.cpp` — DDraw surface management для backgrounds → extract D3D part → d3d/
- `flamengine.cpp` — DDraw Blt feedback → extract D3D part → d3d/
- `fastprim.cpp` — DrawIndPrimMM + D3D texture access → extract D3D calls → ge_*
- `figure.cpp` — DrawIndPrimMM → extract D3D calls → ge_*
- `farfacet.cpp` — DrawIndexedPrimitive через PolyPage → verify D3D removed
- `host.cpp` — RestoreAllSurfaces, toGDI → platform D3D code → d3d/
- `polypage.cpp` — DrawIndexedPrimitiveVB, DrawIndPrimMM → d3d/

**E. Добавить в ge_* контракт:**
- `ge_draw_multi_matrix()` — обёртка DrawIndPrimMM
- `ge_create_offscreen_texture()` / `ge_upload_pixels()` — обёртка DDraw surface ops
- `ge_blit_to_backbuffer()` / `ge_blit_from_backbuffer()` — обёртка Blt
- `ge_restore_surfaces()` — device-lost recovery (no-op в OpenGL)
- `ge_lock_screen()` / `ge_unlock_screen()` — framebuffer pixel access
- Font rendering abstraction (truetype → ge_create_font_texture?)

### Порядок работы

1. Разрезать texture.h (убрать D3DTexture из публичного API) — разблокирует 200 файлов
2. Добавить ge_draw_multi_matrix() — разблокирует fastprim, figure, aeng
3. Перенести polypage.cpp в d3d/ (DrawIndexedPrimitiveVB — чисто D3D)
4. Перенести truetype D3D part в d3d/
5. Вычистить frontend.cpp (DDraw Blt → ge_blit)
6. Вычистить flamengine.cpp (DDraw Blt → ge_blit)
7. Вычистить host.cpp (RestoreAll → ge_restore)
8. Убрать оставшиеся D3D includes из fastprim_globals.h, figure_globals.h, font.cpp
9. Финальная проверка: grep D3D вне d3d/ = 0 вхождений

### Замена RenderState на GERenderState ✅

Создан новый API-агностичный `GERenderState` (`ge_render_state.h/cpp`):
- Хранит GE enum'ы (GEBlendFactor, GETextureBlend, GECullMode...) вместо D3D DWORD
- Типизированные setters: SetBlendMode(), SetFogEnabled(), SetDepthWrite(), SetSrcBlend(), SetTextureBlend()...
- Flush через ge_* (InitScene → ge_set_cull_mode/ge_set_fog_params/ge_set_blend_factors/..., SetChanged → дельта)
- Legacy aliases: `using RenderState = GERenderState`, `#define RS_None GE_EFFECT_NONE` и т.д.

Добавлено в контракт:
- `GEBlendFactor` enum (Zero, One, SrcAlpha, InvSrcAlpha, SrcColor, InvSrcColor, DstColor...)
- `ge_set_blend_factors(GEBlendFactor src, GEBlendFactor dst)` — раздельные src/dst blend
- `ge_set_blend_enabled(bool)` — вкл/выкл blending отдельно от mode
- `ge_set_fog_params(color, near, far)` — fog distance setup

Старый d3d/render_state.h заменён форвардом на ge_render_state.h.
Старый render_state.cpp удалён.

Обновлены потребители:
- `poly_render.cpp` — ~550 вызовов SetRenderState(D3D_X, D3D_Y) → typed setters
- `farfacet.cpp` — SetRenderState → SetFogEnabled
- `fastprim.cpp` — LPDIRECT3DTEXTURE2 → GETextureHandle casts
- `superfacet.cpp/h` — LPDIRECT3DTEXTURE2 → GETextureHandle
- `aeng.cpp/h` — LPDIRECT3DTEXTURE2 → GETextureHandle, SetRenderState → typed setters
- `figure.cpp` — SetRenderState → SetCullMode, SetAlphaBlendEnabled, SetTextureBlend
- `poly.cpp` — FORCE_SET_* debug path обновлён

Сборка: 308/308.

Сборка: 308/308.

---

### frontend.cpp — GEScreenSurface abstraction ✅

Добавлено в ge_* контракт:
- `GEScreenSurface` opaque handle type (uintptr_t, maps to LPDIRECTDRAWSURFACE4 in D3D backend)
- `ge_create_screen_surface(pixels)`, `ge_load_screen_surface(filename)` — создание surface
- `ge_destroy_screen_surface()`, `ge_restore_screen_surface()` — lifecycle
- `ge_set_background_override()`, `ge_get_background_override()` — active background
- `ge_blit_surface_to_backbuffer(surface, x, y, w, h)` — blit surface

Изменения:
- `frontend.h` — убран `#include <ddraw.h>`, убраны `FRONTEND_scr_add`/`FRONTEND_scr_img_load_into_screenfull` (→ ge_*)
- `frontend_globals.h/cpp` — `LPDIRECTDRAWSURFACE4` → `GEScreenSurface` (6 глобалов)
- `frontend.cpp` — все 16 the_display вызовов заменены:
  - Surface creation/destruction → ge_create/load/destroy_screen_surface
  - `lp_DD_Background_use_instead` → ge_set/get_background_override
  - `UseBackSurface()` → ge_set_background_override (13 мест)
  - `Blt()` → ge_blit_surface_to_backbuffer
  - `ViewportRect` → ge_set_viewport(0, 0, w, h)

Остаётся: второй include block (line ~840) с dd_manager.h + gd_display.h для видеонастроек
(DDDriverManager/DDModeInfo/D3DDeviceInfo enumeration). Это D3D-specific UI — абстрагируется позже.

Сборка: 308/308.

---

### Display mode + texture + blit abstraction — ещё 5 файлов ✅

Добавлено в ge_* контракт:
- Display mode: `ge_to_gdi()`, `ge_from_gdi()`, `ge_restore_all_surfaces()`,
  `ge_is_display_changed()`, `ge_clear_display_changed()`, `ge_update_display_rect()`
- Texture: `ge_remove_all_loaded_textures()`
- Blit: `ge_blit_texture_to_backbuffer(page, w, h)`
- Loading screens: `ge_init_back_image()`, `ge_show_back_image()`, `ge_reset_back_image()`
- `ge_is_fullscreen()`, `ge_is_primary_driver()`, `ge_run_cutscene()`

Очищены от gd_display.h:
- `elev.cpp` — RunCutscene → ge_run_cutscene, toGDI → ge_to_gdi
- `host.cpp` — 8 calls: toGDI/fromGDI, RestoreAllSurfaces, IsDisplayChanged/Off, IsFullScreen → ge_*
- `wind_procs.cpp` — DisplayRect + IsFullScreen → ge_update_display_rect
- `flamengine.cpp` — DDraw Blt → ge_blit_texture_to_backbuffer (D3DTexture deps stay via d3d_texture.h)
- `texture.cpp` — RemoveAllLoadedTextures → ge_remove_all_loaded_textures

gd_display.h вне d3d/: **3 файла** (frontend.cpp + figure.cpp + farfacet.cpp).
figure.cpp и farfacet.cpp — D3D рендер-пайплайн, при OpenGL переписываются целиком.
frontend.cpp — единственный "чужой" файл с DDraw surface ops.
Сборка: 308/308.

---

### Массовая очистка d3d/ includes ✅

**render_state.h** (9 файлов → 0): заменены на `ge_render_state.h` (публичный хедер).
**dd_manager.h** (3 файла → 0): farfacet_globals, superfacet_globals, poly_globals → `graphics_engine.h`.
**gd_display.h** (figure, farfacet → 0): DeadAndBuried → ge_debug_paint_block, DrawIndexedPrimitive → ge_draw_indexed_primitive_lit.
**polypage.h**: vertex_buffer.h → forward declarations. Render/DrawSinglePoly: убран IDirect3DDevice3* параметр.
**aeng.cpp**: display_macros.h убран. TheVPool → ge_reclaim_vertex_buffers.
**poly.cpp**: display_macros.h + vertex_buffer.h убраны. REALLY_SET_* → ge_set_*. Render(device) → Render().
**qeng.cpp**: TheVPool → ge_reclaim_vertex_buffers.
**game_tick.cpp**: TheVPool → ge_dump_vpool_info.
**pause.cpp**: d3d_texture.h убран.
**flamengine.cpp**: D3DTexture → ge_lock/unlock_texture_pixels + ge_get_texture_pixel_format.
**texture_globals.h**: TEXTURE_texture[] перенесён в d3d/d3d_texture_globals.h/cpp.
**figure.cpp**: D3DCOLORVALUE → GEColorValue.

Сборка: 308/308. d3d/ includes вне d3d/: **15** (было 33).

---

## Оставшиеся d3d/ includes вне d3d/ (15 штук)

| Файл | Include | Нужно |
|------|---------|-------|
| texture.cpp | d3d_texture.h/globals | Перенести .cpp в d3d/ (D3D texture loader) |
| text.cpp | d3d_texture.h + vertex_buffer.h | Перенести .cpp в d3d/ (D3D text renderer) |
| truetype.cpp | display_macros.h | Перенести .cpp в d3d/ (D3D TrueType) |
| truetype.h | d3d_texture.h | Forward-decl D3DTexture или opaque handle |
| truetype_globals.h | SDK + d3d_texture.h | Перенести в d3d/ (D3DTexture tt_Texture[]) |
| polypage.cpp | vertex_buffer_globals + gd_display | Перенести .cpp в d3d/ (D3D renderer) |
| wind_procs.h | dd_manager.h | Абстрагировать ChangeDDInfo |
| frontend.cpp | dd_manager + gd_display | ge_enumerate_modes() или секция в d3d/ |

### Стратегия

**Batch 1 — перенос .cpp в d3d/:**
texture.cpp, text.cpp, truetype.cpp, polypage.cpp — чистый D3D render код.
Их публичные API (.h) остаются на месте.
CMakeLists.txt обновляется.

**Batch 2 — headers:**
truetype.h, truetype_globals.h, wind_procs.h, frontend.cpp video section.

### Правила переноса → `new_game_planning/stage7_renderer_rules.md`

---

### Массовая очистка gd_display.h — ещё 7 файлов ✅

Добавлено в ge_* контракт:
- `ge_is_fullscreen()` — windowed/fullscreen mode query
- `ge_init_back_image()`, `ge_show_back_image()`, `ge_reset_back_image()` — loading screens
- `ge_create/blit/destroy_background_surface()` — background surface ops
- `ge_run_cutscene()` — FMV playback
- `ge_is_primary_driver()` — DDraw driver query

Очищены от gd_display.h:
- `shape.cpp` — не использовал ничего, include убран
- `sprite.cpp` — DisplayWidth/Height → local #define
- `mouse.cpp` — hDDLibWindow → local extern
- `widget.cpp` — IsFullScreen → ge_is_fullscreen
- `attract.cpp` — ViewportRect → ge_set_viewport(0,0,w,h), InitBackImage/ShowBackImage/ResetBackImage → ge_*
- `game.cpp` — backgrounds/cutscene/driver → ge_*
- `eng_map.cpp` — DisplayWidth/Height → local #define

gd_display.h вне d3d/: 15 → **8 файлов** (frontend, texture, elev, figure, farfacet, wind_procs, host, flamengine).
Оставшиеся — DDraw surface ops, platform code, D3D рендер-пайплайн internals.
Сборка: 308/308.

---

### Screen access + device caps + gamma API ✅

Добавлено в graphics_engine.h + D3D реализация:

**Screen buffer:**
- `ge_lock_screen()`, `ge_unlock_screen()` — lock/unlock back buffer
- `ge_get_screen_buffer()`, `ge_get_screen_pitch()`, `ge_get_screen_width/height()`, `ge_get_screen_bpp()`
- `ge_plot_pixel()`, `ge_plot_formatted_pixel()`, `ge_get_pixel()`, `ge_get_formatted_pixel()`
- `ge_blit_back_buffer()`

**Device capabilities:**
- `ge_supports_dest_inv_src_color()`, `ge_supports_modulate_alpha()`, `ge_supports_adami_lighting()`
- `ge_is_hardware()`

**Gamma:**
- `ge_set_gamma(black, white)`, `ge_get_gamma()`, `ge_is_gamma_available()`

**Очищены от gd_display.h (7 файлов):**
- `aeng.cpp` — 27 the_display вызовов → ge_* (screen lock/unlock, blit, device caps)
- `font.cpp` — PlotPixel, screen_lock/unlock, screen dimensions → ge_*
- `sky.cpp` — PlotPixel, PlotFormattedPixel, GetFormattedPixel → ge_*
- `wibble.cpp` — screen buffer/pitch/bpp → ge_* (extern для RealDisplayWidth/Height)
- `bloom.cpp` — DisplayWidth/Height → ge_get_screen_width/height
- `game_tick.cpp` — SetGamma, GetPixel → ge_* (extern для hDDLibWindow, DisplayWidth/Height)
- `poly_render.cpp` — GetDeviceInfo()->XxxSupported() → ge_supports_*

**Частично очищены (ge_* вместо GetDeviceInfo, but gd_display.h kept):**
- `figure.cpp` — AdamiLightingSupported → ge_supports_adami_lighting (2 места)
- `texture.cpp` — DestInvSourceColourSupported → ge_supports_dest_inv_src_color
- `elev.cpp` — IsHardware → ge_is_hardware
- `frontend.cpp` — SetGamma/GetGamma/IsGammaAvailable → ge_*

gd_display.h вне d3d/: было 20+ файлов → осталось 15.
Сборка: 308/308.

---

### Убраны LPDIRECT3DTEXTURE2 и D3D SDK includes из fastprim, figure_globals ✅

- `fastprim_globals.h`: `LPDIRECT3DTEXTURE2 texture` → `GETextureHandle texture` в struct FASTPRIM_Call.
  Убран `#include <d3d.h>`.
- `fastprim.cpp`: все 5 LPDIRECT3DTEXTURE2 → GETextureHandle. Убраны reinterpret_cast'ы при вызове
  ge_bind_texture() и pp->RS.GetTexture().
- `figure_globals.h`: убраны `#include <ddraw.h>` и `#include <d3d.h>` — в файле не осталось
  D3D SDK типов (DWORD приходит из Windows, D3DObj/D3DPeopleObj — наши имена).

Оставшиеся D3D SDK includes вне d3d/ (и вне outro/):
- `truetype_globals.h` — `<ddraw.h>` + `<d3d.h>` (D3DTexture tt_Texture[] — DDraw font surfaces)
- `frontend.h` — `<ddraw.h>` (LPDIRECTDRAWSURFACE4 в параметрах)

Оба — D3D-зависимые файлы, переписываются при OpenGL.
Сборка: 308/308.

---

### ge_draw_multi_matrix — замена DrawIndPrimMM ✅

DrawIndPrimMM — CPU-side multi-matrix transform + D3D DrawIndexedPrimitive. Превращён в API-agnostic:

**Новая сигнатура:**
```cpp
enum class GEMMVertexType { Lit, Unlit };
void ge_draw_multi_matrix(GEMMVertexType, GEMultiMatrix*, uint16_t num_verts, uint16_t* indices, uint32_t num_indices);
```

**Изменения:**
- polypage.h: убрана старая `DrawIndPrimMM(LPDIRECT3DDEVICE3, DWORD dwFVFType, ...)` сигнатура.
  Добавлены `GEMMVertexType` enum и `ge_draw_multi_matrix()`.
- polypage.cpp: реализация заменена — `lpDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, ...)`
  → `ge_draw_indexed_primitive(TriangleList, ...)`. D3D типы (DWORD, WORD, HRESULT) → stdint.
- fastprim.cpp: 1 call site — `DrawIndPrimMM(the_display.lp_D3D_Device, D3DFVF_LVERTEX, ...)`
  → `ge_draw_multi_matrix(Lit, ...)`. Убран `#include "gd_display.h"`.
- figure.cpp: 3 call sites — `DrawIndPrimMM(the_display.lp_D3D_Device, D3DFVF_VERTEX, ...)`
  → `ge_draw_multi_matrix(Unlit, ...)`.
- aeng.cpp: 1 call site — `DrawIndPrimMM(the_display.lp_D3D_Device, D3DFVF_LVERTEX, ...)`
  → `ge_draw_multi_matrix(Lit, ...)`.

Результат: DrawIndPrimMM больше не принимает D3D-указатели. CPU-transform код API-agnostic.
figure.cpp и aeng.cpp ещё используют `the_display` для других вещей (screen lock, device info).
Сборка: 308/308.

---

### Разрезание texture.h — убрали D3DTexture из публичного API ✅

**Проблема:** texture.h включал d3d_texture.h → D3DTexture утекал в ~28 файлов (и транзитивно дальше).
`TEXTURE_get_D3DTexture()` торчал в публичном API.

**Решение:**
- Добавлена `TEXTURE_get_tex_offset(page, &uScale, &uOffset, &vScale, &vOffset)` — публичная функция,
  скрывает D3DTexture::GetTexOffsetAndScale внутри texture.cpp
- `PolyPage::SetTexOffset(D3DTexture*)` → `SetTexOffset(SLONG page)` — принимает page index
- poly_render.cpp: `SET_TEXTURE(PAGE)` теперь вызывает `SetTexOffset(PAGE)` вместо `SetTexOffset(TEXTURE_get_D3DTexture(PAGE))`
- Удалены из texture.h: `#include "d3d_texture.h"`, `TEXTURE_get_D3DTexture()` декларация
- Удалены из polypage.h: forward declaration `class D3DTexture`
- polypage.cpp: заменён `#include "d3d_texture.h"` → `#include "assets/texture.h"`

Затронутые файлы: texture.h, texture.cpp, polypage.h, polypage.cpp, poly_render.cpp.
Сборка: 308/308, 0 ошибок.

---

### Batch 1+2: перенос .cpp в d3d/ + очистка headers ✅

**Batch 1 — перенос .cpp файлов:**
- `texture.cpp` → `d3d/texture.cpp` (1418 строк, D3D texture loader)
- `text.cpp` → `d3d/text.cpp` (233 строки, D3D text renderer)
- `truetype.cpp` → `d3d/truetype.cpp` (613 строк, D3D TrueType renderer)
- `polypage.cpp` → `d3d/polypage.cpp` (648 строк, D3D polypage renderer)
- `truetype_globals.h/.cpp` → `d3d/truetype_globals.h/.cpp` (D3D-specific globals: D3DTexture[], IDirectDrawSurface4*, etc.)

**Batch 2 — очистка headers:**
- `truetype.h`: убран `#include d3d_texture.h`, CacheLine (D3D-specific) перенесён в d3d/truetype_globals.h
- `wind_procs.h`: убран `#include dd_manager.h`, удалён неиспользуемый `ChangeDDInfo` struct
- `frontend.cpp`: убраны dd_manager.h, dd_manager_globals.h, gd_display.h

**Новые ge_* функции:**
- `ge_texture_loading_done()` — освобождение temp буферов после загрузки текстур (заменяет `NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit`)
- `ge_is_ntsc()` — проверка NTSC режима (заменяет `eDisplayType == DT_NTSC`)
- `ge_enumerate_drivers(callback, ctx)` — перечисление видеодрайверов через callback (заменяет прямой доступ к DDDriverInfo linked list)

**Результат:**
- d3d/ includes вне d3d/ (без outro): **0** (было 15)
- D3D SDK includes вне d3d/ (без outro): **0** (было 2)
- **Правило 1 полностью выполнено** для основной игры
- Сборка: 308/308

---

### polypage.h — убраны D3D типы из публичного header'а ✅

- `IDirect3DVertexBuffer* m_VB` → `void* m_VB` (opaque, D3D backend кастит через `RAW_VB()`)
- `VertexBuffer* m_VertexBuffer` → `void* m_VertexBuffer` (opaque, D3D backend кастит через `VB()`/`SET_VB()`)
- Удалён forward-decl `struct IDirect3DVertexBuffer` и `class VertexBuffer` из polypage.h
- В d3d/polypage.cpp добавлены cast-макросы `VB()`, `SET_VB()`, `RAW_VB()`

Сборка: 308/308.

---

### ge_render_state.h → merged в graphics_engine.h ✅

`GERenderState` (render state cache, delta-flush) перенесён в `graphics_engine.h` — один файл абстракции.
`ge_render_state.h` удалён. `ge_render_state.cpp` → `graphics_engine.cpp`. 11 файлов обновлены.
`d3d/render_state.h` оставлен как legacy redirect.

Итого абстракция: `graphics_engine.h` + `graphics_engine.cpp`. Бэкенд: `backend_directx6/` (29 файлов).
Сборка: 308/308.

---

### Очистка бэкенда от игровой логики ✅

**texture.cpp** — вернулся в `assets/`. Все D3DTexture обращения заменены на ge_* (~12 новых функций).
Файл больше не зависит от бэкенда. Можно реализовать текстурный менеджмент для любого API.

Новые ge_* функции: `ge_texture_load_tga`, `ge_texture_destroy`, `ge_texture_free_all`,
`ge_texture_create_user_page`, `ge_texture_font_on/font2_on`, `ge_texture_change_tga`,
`ge_texture_set_greyscale`, `ge_texture_get_size`, `ge_texture_get/set_type`,
`ge_get_texture_handle`, `ge_get_texture_offset`, `ge_texture_loading_begin`.

**display.cpp** — убраны все игровые includes (panel.h, input_actions.h, poly.h, polypage.h).
Прямые вызовы PreFlipTT, PANEL_ResetDepthBodge, PANEL_screensaver_draw, PolyPage::SetScaling
заменены на callback-механизм: `ge_set_pre_flip_callback`, `ge_set_mode_change_callback`.
Callbacks регистрируются из game.cpp при старте.

**polypage.cpp** — `extern int AENG_total_polys_drawn` заменён на `ge_set_polys_drawn_callback`.

**Результат:** в бэкенде `backend_directx6/` нет ни одного include игровой логики.
Только engine/graphics/, engine/platform/, engine/core/, engine/io/, assets/formats/tga.

**Повторный полный аудит (28 файлов бэкенда + контракт):**
- 0 игровых типов (Thing, Person, Vehicle, Mission)
- 0 игровых подсистем (AI, physics, input, sound, combat, pathfinding)
- 0 игровых includes (panel, frontend, input_actions, game_types, buildings, map)
- Внешние зависимости бэкенда: только engine infrastructure + assets/texture + assets/formats/tga
- Связь с игрой: 3 callback'а через контракт (pre-flip, mode change, polys drawn)
- `POLY_reset_render_states()` — graphics pipeline, не game logic
- `g_dw3DStuffHeight/Y` — viewport letterbox state

Сборка: 308/308.

---

### Проверка заменяемости на OpenGL (Шаг 2.5B) ✅

Аудит 9 .cpp файлов бэкенда. Вердикт: **контракт чист, бэкенд заменяем, блокеров нет.**

Сложность по файлам: vertex_buffer/polypage/text — лёгкие (уже абстрагированы),
graphics_engine_d3d/d3d_texture/dd_manager — средние (1:1 маппинг API),
display/truetype/work_screen — сложные (DDraw lifecycle, surface locking).

Контракт `graphics_engine.h` **не требует изменений** для OpenGL реализации.
Потенциальные улучшения (GETextureBlend → шейдеры, vertex padding, fog/specular) —
решено делать по месту при написании OpenGL бэкенда, а не заранее в вакууме.
Для modern GL (3.3+) понадобится шейдерный слой (D3D6 = fixed-function pipeline).

---

### d3d/ → backend_directx6/ ✅

Переименована папка бэкенда: `d3d/` → `backend_directx6/` (там и D3D6 и DirectDraw).
25 файлов обновлены (include пути + CMakeLists.txt).
Сборка: 308/308.

---

### Шаг 2 основной игры — ЗАВЕРШЁН ✅

**Полный аудит D3D изоляции (9 категорий):**

| Категория | Вне backend/outro |
|-----------|-------------------|
| D3D SDK includes (`<d3d.h>`, `<ddraw.h>`) | 0 |
| Внутренние backend includes | 0 |
| D3D SDK interface типы (`IDirect3D*`, `IDirectDraw*`) | 0 |
| D3D vertex типы (`D3DTLVERTEX`, `D3DLVERTEX`) | 0 (только комментарии) |
| DDraw surface типы (`DDSURFACEDESC`, `LPDIRECTDRAWSURFACE4`) | 0 |
| D3D state/blend enums (`D3DRENDERSTATE_*`) | 0 активного кода (мёртвый `/* */` в poly.cpp) |
| Проектные D3D типы (`D3DTexture`, `VertexBuffer`) | 0 (только комментарии) |
| D3D глобалы (`the_display.`, `TheVPool`, `REALLY_SET_`) | 0 |
| DDraw функции (`DrawIndexedPrimitiveVB`, `Blt`) | 0 |

Весь D3D/DirectDraw код — **исключительно** в `engine/graphics/graphics_engine/backend_directx6/`.
Остальной код видит только `graphics_engine.h` с чистыми типами.
Можно переходить к Шагу 3 (outro) или сразу к Шагу 4 (OpenGL).

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
