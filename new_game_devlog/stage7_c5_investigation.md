# C5 — Загрузочный экран просвечивает через первые кадры

## Описание бага

- На 1-2 кадра после загрузки уровня на **небе и заднике (backdrop)** виден белёсый оверлей
- Оверлей содержит **очертания загрузочного экрана** (луна + силуэт Дарси из `e3load.tga`)
- Проявляется **ТОЛЬКО на первом запуске миссии** после старта приложения
- При перезапуске (выход в меню → повторный вход) — нет
- **В старой версии (stage_5_1_done) НЕ воспроизводится**
- Уровневая геометрия (здания, земля, персонажи) рисуется **нормально** поверх глюка
- Глюк **только на небе и заднике** — это ключевая деталь

## Почему небо критично

Небо рисуется в `POLY_frame_draw_odd()` с **аддитивным блендингом** (SrcBlend=One, DstBlend=One).
Это значит: финальный цвет = содержимое_буфера + sky_texture × vertex_color.

Если в буфере — корректный sky_clear_color → всё нормально.
Если в буфере — остатки загрузочного экрана → они **аддитивно складываются** с небом → баг.

Здания рисуются опак (Modulate, без alpha blend) → полностью перезаписывают буфер → глюк не виден.

## Что было проверено

### 1. ge_* обёртки — все 1:1 passthrough
Каждая ge_* функция в `graphics_engine_d3d.cpp` просто вызывает соответствующий Display метод:
- `ge_clear(true, true)` → `the_display.lp_D3D_Viewport->Clear(1, &ViewportRect, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER)` = `ClearViewport()`
- `ge_flip()` → `FLIP(NULL, DDFLIP_WAIT)` → `Display::Flip()`
- `ge_set_background(User)` → `SET_USER_BACKGROUND` → `SetBackground(user_handle)`
- `ge_set_background_color(r,g,b)` → `SetUserColour(r,g,b)`
- `ge_show_back_image()` → `ShowBackImage()` → `blit_background_surface()`
- `ge_reset_back_image()` → `ResetBackImage()` → `destroy_background_surface()`
- `ge_blit_back_buffer()` → `the_display.blit_back_buffer()`

### 2. AENG_clear_viewport — идентичен старому коду
Старый:
```
the_display.SetUserColour(sky_colour);
the_display.SetUserBackground();
the_display.ClearViewport();
```
Новый:
```
ge_set_background_color(sky_colour);
ge_set_background(GEBackground::User);
ge_clear(true, true);
```
Функционально тождественно.

### 3. ViewportRect — корректен
Устанавливается в SetDisplay → (0, 0, screen_width, screen_height).
Затем обновляется `ge_set_viewport(0, 0, w, h)` в attract.cpp.
Между загрузкой и первым кадром игры — не изменяется.
AENG_clear_viewport использует полноэкранный rect.

### 4. Рендер неба — идентичен
`SKY_draw_poly_sky_old` (aeng.cpp:5295) добавляет полигоны в `POLY_PAGE_SKY`.
Sky page проверяется в двух местах:
- `POLY_frame_draw()` (poly.cpp:1897) — рисует sky первым с `RS.SetChanged()`,
  НО к этому моменту sky polys ещё не добавлены → `NeedsRendering()` = false → пропуск.
- `POLY_frame_draw_odd()` (poly.cpp:2093) — рисует sky с **additive blend (One, One)**.
  Это ЕДИНСТВЕННОЕ место где небо реально рисуется.
Код poly.cpp и poly_render.cpp — механические замены (REALLY_SET → ge_set).

### 5. Loading screen flow — идентичен
```
attract mode: ATTRACT_loadscreen_init() → loads e3load.tga, shows, flips ×2
attract loop exits → ge_show_back_image + AENG_flip → ge_reset_back_image
game_init() → ELEV_load_user → ELEV_game_init → ATTRACT_loadscreen_draw (×N)
  (но ge_show_back_image no-op: lp_DD_Background = NULL после ge_reset_back_image)
game_loop → first frame → AENG_draw → AENG_clear_viewport → rendering
```
Этот flow одинаков в старой и новой версии.

### 6. Display::Flip / blit_back_buffer — идентичны
Pre-flip callback (PreFlipTT, PANEL_ResetDepthBodge, PANEL_screensaver_draw) —
в старом коде были hardcoded в Display::Flip, в новом через s_pre_flip_callback.
Функционально идентично.
`screen_flip()` проверяет `ge_is_primary_driver()` = `the_display.GetDriverInfo()->IsPrimary()`.

### 7. GERenderState cache — не может быть причиной
InitScene() в POLY_frame_init устанавливает ВСЕ render states через ge_* вызовы
на каждом кадре. s_State синхронизируется. Первый кадр получает полную инициализацию.

### 8. POLY_init_render_states — идентичен
RenderStates_OK = false при инициализации → первый вызов всегда инициализирует.
POLY_PAGE_SKY setup: SET_TEXTURE(sky) + DepthEnabled + !DepthWrite + !Fog — одинаково.

## Единственное существенное отличие (attract.cpp init)

В старом attract.cpp перед attract-loop был блок из ~25 явных D3D вызовов:
```cpp
dev->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
dev->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, UC_TRUE);
dev->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, UC_FALSE);
// ... ещё ~20 вызовов ...
dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
// ...
dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);  // ← Отключение stage 1!
dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
```

В новом коде заменено на:
```cpp
ge_set_viewport(0, 0, ge_get_screen_width(), ge_get_screen_height());
ge_set_perspective_correction(true);
ge_set_specular_enabled(false);
ge_set_depth_mode(GEDepthMode::ReadWrite);
ge_set_depth_func(GECompareFunc::LessEqual);
ge_set_cull_mode(GECullMode::None);
ge_set_fog_enabled(false);
ge_set_texture_filter(Linear, Linear);
ge_set_texture_address(Wrap);
ge_set_texture_blend(Modulate);
ge_set_blend_mode(Opaque);
```

**Что потеряно:**
- FILLMODE, SHADEMODE, STIPPLEDALPHA, SUBPIXEL, ANTIALIAS — не критично (set-once defaults)
- D3DTSS для stage 0 (COLOROP/ARG, ALPHAOP/ARG) — не установлены явно через TSS
- **D3DTSS stage 1 DISABLE** — НЕ установлен в новом коде
- Примечание: в `ge_init()` есть комментарий: "Do NOT set TSS here — explicit TSS disables
  the legacy D3DRENDERSTATE_TEXTUREMAPBLEND that the POLY pipeline uses"

Это различие происходит ПЕРЕД attract loop (при входе в attract mode), не непосредственно
перед первым кадром игры. К моменту первого кадра InitScene() переустанавливает все states.
Но это единственное найденное отличие в render state initialization.

## Что НЕ проверено / возможные направления

1. **Viewport Clear возвращает ошибку?** — ge_clear не проверяет HRESULT. Можно добавить проверку.
2. **Тройная буферизация?** — dgVoodoo может использовать triple buffering. Loading flips +
   game blit → один из трёх буферов может сохранить загрузочный экран на 2-3 кадра.
3. **Порядок вызовов D3D** — хотя обёртки 1:1, порядок вызовов из game code мог
   незначительно измениться (напр. attract mode init). Полный diff ревью может это выявить.
4. **Диагностический подход** — добавить ge_clear + ge_flip перед первым кадром game_loop
   (после game_init, до main while) чтобы принудительно очистить все буферы.
5. **Полная сверка Stage 7 diff** — может выявить неочевидную ошибку переноса.

## Файлы для ревью (приоритет)

- `attract.cpp` — init render states (единственное найденное отличие)
- `poly.cpp` — POLY_frame_init, POLY_frame_draw, POLY_frame_draw_odd
- `poly_render.cpp` — POLY_init_render_states, per-page RS setup
- `aeng.cpp` — AENG_clear_viewport, AENG_draw, AENG_draw_city
- `graphics_engine_d3d.cpp` — ge_clear, ge_flip, ge_set_background*
- `display.cpp` — Display::Flip, blit_background_surface, SetDisplay

## Ключевой вывод

Баг проявляется ТОЛЬКО на небе/заднике потому что небо — единственная крупная область
экрана, которая рисуется с **аддитивным блендингом** (POLY_frame_draw_odd: One, One).
Все остальные объекты рисуются опак → полностью перезаписывают буфер → скрывают проблему.

Это значит: **корневая причина — dirty back buffer на первом кадре**.
Viewport clear (ge_clear) либо не срабатывает, либо чистит не тот буфер,
либо чистит не полностью. Код clear идентичен старому — значит разница
в том, в каком состоянии D3D device/surfaces находятся К МОМЕНТУ первого clear.
Что-то в новом коде оставляет device/surfaces в состоянии, при котором
первый Viewport::Clear не полностью очищает back buffer.

## Статус

🔍 Расследование приостановлено.
