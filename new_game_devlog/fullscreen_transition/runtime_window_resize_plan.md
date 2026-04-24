# План: runtime-ресайз окна

Задача: сделать окно игры ресайзабельным руками (юзер тянет за край OS
handle), и чтобы рендеринг подхватывал новый размер без ребилда.
`OC_WINDOWED_WIDTH/HEIGHT` в config.h становятся **стартовым** размером,
дальше юзер тянет как хочет. На fullscreen не влияет.

Этот файл — **handoff plan для свежего агента** (контекст разговора, где
он родился, недоступен). Прочти целиком, инструкции самодостаточные.

Связанные доки:
- [`concepts.md`](concepts.md) — глоссарий (ScreenWidth, g_frame_scale,
  uniform_scale и т.п.); прочти если любой термин ниже незнаком.
- [`issues.md`](issues.md) — секция «🟢 TODO — runtime window resize»
  (первое описание задачи, короткое; здесь — развёрнутый план).
- [`fbo_as_virtual_screen_plan.md`](fbo_as_virtual_screen_plan.md) §E4
  и §"Post-refactor surface" — историческая справка про `SetDisplay`.

---

## Хорошая новость: большая часть инфраструктуры уже есть

Не надо писать с нуля. Проверено в кодовой базе — следующие куски **уже
работают**:

| Что | Где | Статус |
|-----|-----|--------|
| SDL3 событие `SDL_EVENT_WINDOW_RESIZED` обрабатывается, есть callback-слот | [`sdl3_bridge.cpp:531-532`](../../new_game/src/engine/platform/sdl3_bridge.cpp#L531-L532) | ✅ готово |
| `SDL3_Callbacks::on_window_resized` зарегистрирован | [`host.cpp:74-77, 114`](../../new_game/src/engine/platform/host.cpp#L74-L114) | ✅ есть хук, но внутри только `ge_update_display_rect` — **нужно дописать ресайз-логику** |
| `composition_resize(w, h)` — пересоздаёт scene FBO, color/depth текстуры, обновляет `s_scene_w/h` | [`composition.cpp:176`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp#L176) | ✅ готово, no-op если размер не поменялся |
| `game_mode_changed(w, h)` — пересчитывает `PolyPage::s_XScale/s_YScale` и `ui_coords::recompute(w, h)` | [`game.cpp:176-187`](../../new_game/src/game/game.cpp#L176-L187) | ✅ готово, идёт через `ge_set_mode_change_callback` |
| `s_mode_change_callback` выстреливает при вызове в backend | [`core.cpp:2615-2617`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2615-L2617) | ✅ готово |
| `SetDisplay(w, h, depth)` — заглушка: пишет в `ScreenWidth/Height`, не трогает GL | [`core.cpp:2622-2629`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2622-L2629) | ⚠️ stub, либо заполнить, либо заменить прямым вызовом нового хелпера |

Всё ядро алгоритма (аспект-клэмп, HiDPI, `OC_RENDER_SCALE`,
`composition_init`) живёт в
[`core.cpp::OpenDisplay`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp#L2504).
Задача — вынести внутренности в переиспользуемый хелпер + вызвать его
из `on_window_resized`.

---

## Целевая архитектура (коротко)

```
                  resize event (SDL)
                         │
                         ▼
   sdl3_bridge: SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED → on_window_resized()
                         │
                         ▼
   host.cpp on_window_resized:
     - (optional) debounce ~150ms
     - query new drawable size (sdl3_window_get_drawable_size)
     - call graphics backend reconfigure hook
                         │
                         ▼
   core.cpp reconfigure_display_for_size(w, h):
     - recompute fbo_w/fbo_h (render scale + aspect clamp,
       идентично OpenDisplay)
     - composition_resize(fbo_w, fbo_h)
     - ScreenWidth = fbo_w; ScreenHeight = fbo_h
     - composition_bind_scene()
     - s_mode_change_callback(fbo_w, fbo_h)  // game_mode_changed
```

Рекомендация: **извлечь из OpenDisplay общий хелпер**
`static void compute_and_apply_fbo_size(int phys_w, int phys_h)` который
делает всё начиная с render-scale до callback'а. Тогда `OpenDisplay`
вызывает его после создания GL context, а `reconfigure_display_for_size`
вызывает его же без создания контекста. DRY, одна формула клэмпа в одном
месте.

---

## Шаги реализации

### Шаг 1: Сделать окно ресайзабельным

В [`sdl3_bridge.cpp:44`](../../new_game/src/engine/platform/sdl3_bridge.cpp#L44)
добавить флаг:

```cpp
Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN
             | SDL_WINDOW_HIGH_PIXEL_DENSITY
             | SDL_WINDOW_RESIZABLE;  // ← новое
```

Проверить: на fullscreen SDL игнорирует RESIZABLE, так что безопасно для
обоих режимов.

**После шага 1** — окно уже можно тянуть, но рендер остаётся в старом
размере, только виден масштабированный клочок. Нормально, переходим
дальше.

### Шаг 2: Переключить на `PIXEL_SIZE_CHANGED` событие

`SDL_EVENT_WINDOW_RESIZED` стреляет при изменении **логических точек**,
`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` — при изменении **физических
пикселей** (то что нужно для FBO). На не-HiDPI дисплеях они совпадают;
на Retina — нет.

В [`sdl3_bridge.cpp`](../../new_game/src/engine/platform/sdl3_bridge.cpp):
рядом с веткой `SDL_EVENT_WINDOW_RESIZED` (строка 531-532) добавить
case для `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` и вызывать тот же
callback. Либо переключить существующий case на `PIXEL_SIZE_CHANGED`,
если хочешь минимум кода — RESIZED в HiDPI не даёт точный сигнал.

### Шаг 3: Извлечь хелпер из OpenDisplay

В [`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp):

```cpp
// Runs the aspect-clamp + render-scale math, applies the resulting
// FBO size to composition + ScreenWidth/Height + ui_coords. Called
// both from OpenDisplay (first init) and from resize handler.
static bool apply_fbo_for_physical(int phys_w, int phys_h)
{
    // 1. render-scale
    float scale = OC_RENDER_SCALE;
    if (scale < OC_RENDER_SCALE_MIN) scale = OC_RENDER_SCALE_MIN;
    if (scale > 1.0f) scale = 1.0f;

    int scaled_w = std::max(1, int(phys_w * scale + 0.5f));
    int scaled_h = std::max(1, int(phys_h * scale + 0.5f));

    // 2. aspect clamp
    const float a = float(scaled_w) / float(scaled_h);
    int fbo_w, fbo_h;
    if (a > OC_FOV_CAP_ASPECT) {
        fbo_h = scaled_h;
        fbo_w = int(scaled_h * OC_FOV_CAP_ASPECT + 0.5f);
    } else if (a < OC_FOV_MIN_ASPECT) {
        fbo_w = scaled_w;
        fbo_h = int(scaled_w / OC_FOV_MIN_ASPECT + 0.5f);
    } else {
        fbo_w = scaled_w; fbo_h = scaled_h;
    }
    fbo_w = std::max(1, fbo_w);
    fbo_h = std::max(1, fbo_h);

    // 3. reallocate FBO + textures
    if (!composition_resize(fbo_w, fbo_h)) return false;

    // 4. publish new sizes
    ScreenWidth  = fbo_w;
    ScreenHeight = fbo_h;
    composition_bind_scene();

    // 5. pull in all UI / scale consumers
    if (s_mode_change_callback) {
        s_mode_change_callback(fbo_w, fbo_h);
    }
    return true;
}
```

Затем в `OpenDisplay`:
- удалить дублирующий код (render-scale + aspect clamp + `ScreenWidth=`
  + `composition_init` + callback) **после** создания GL context
- вместо этого: `composition_init(1, 1)` + `apply_fbo_for_physical(phys_w, phys_h)`.
  Либо оставить первичный `composition_init(fbo_w, fbo_h)` и вызвать
  `apply_fbo_for_physical` только для последующих ресайзов — **но**
  тогда есть риск расхождения формулы; лучше DRY.

**Важно:** печать `fprintf(stderr, "Display init: ...")` должна
остаться только в `OpenDisplay` (один раз), НЕ повторяться на каждом
resize событии — иначе логи забьются при активном drag'е.

### Шаг 4: Публичная точка входа для ресайза

В [`core.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp):

```cpp
void ge_resize_display()
{
    int phys_w = 0, phys_h = 0;
    sdl3_window_get_drawable_size(&phys_w, &phys_h);
    if (phys_w <= 0 || phys_h <= 0) return;
    apply_fbo_for_physical(phys_w, phys_h);
}
```

Добавить декларацию в [`game_graphics_engine.h`](../../new_game/src/engine/graphics/graphics_engine/game_graphics_engine.h).

Опционально: заменить тело `SetDisplay` на `ge_resize_display` и
удалить stub-комментарий `// TODO: resize GL context.` — либо оставить
SetDisplay как есть (он умер из D3D-эпохи), новый вход чище.

### Шаг 5: Дёрнуть из `on_window_resized`

В [`host.cpp:74-77`](../../new_game/src/engine/platform/host.cpp#L74-L77):

```cpp
static void on_window_resized()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
    ge_resize_display();  // ← новое
}
```

**После шага 5** в теории всё работает. Если сбилдить и потянуть окно —
рендер должен подстраиваться. В реальности надо пройти Шаги 6-7 для
надёжности.

### Шаг 6: Дебаунс (опционально на MVP, важно на слабом GPU)

Проблема: при перетаскивании края SDL шлёт event'ы почти каждый пиксель
(десятки в секунду). Каждый event = destroy+create двух GL-текстур и
FBO. На интегрированной графике / Steam Deck это вызовет заметные
лаги/заикания.

Простое решение: вместо немедленного `ge_resize_display()` поставить
флажок «pending resize» + timestamp, и обработать его в game loop
если прошло ≥150мс с последнего события.

Скелет:
```cpp
// host.cpp (or dedicated module)
static bool     s_resize_pending = false;
static uint64_t s_resize_last_event_ms = 0;
static constexpr uint64_t RESIZE_DEBOUNCE_MS = 150;

static void on_window_resized()
{
    ge_update_display_rect(sdl3_window_get_native_handle(), ge_is_fullscreen());
    s_resize_pending = true;
    s_resize_last_event_ms = sdl3_get_ticks();
}

// Called once per frame from game loop (or from an existing idle hook).
void host_process_pending_resize()
{
    if (!s_resize_pending) return;
    if (sdl3_get_ticks() - s_resize_last_event_ms < RESIZE_DEBOUNCE_MS) return;
    ge_resize_display();
    s_resize_pending = false;
}
```

Точка вызова `host_process_pending_resize()` — где-то в
[`game.cpp game_loop`](../../new_game/src/game/game.cpp) в начале
кадра до `AENG_draw`. Или в
[`process_controls`](../../new_game/src/game/input_actions.cpp) тике,
или после `sdl3_pump_events()` в bridge.

**Альтернатива — без дебаунса:** применять ресайз сразу. На современном
desktop GPU это в пределах миллисекунд и не заметно; на Steam Deck
проверить вручную и решить.

### Шаг 7: Ревизия кэшированных размеров

После `ge_resize_display()` глобалы `ScreenWidth`/`ScreenHeight`
обновлены, `composition` пересоздан, `game_mode_changed` дернулся →
`PolyPage::SetScaling` и `ui_coords::recompute` применились. Большая
часть game-кода читает эти глобалы каждый кадр, так что подхватится
само.

Но есть места где размер **кэшируется при инициализации** — после
ресайза они могут остаться с устаревшими значениями. Пройдись по
кандидатам:

- [`composition.cpp`](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/postprocess/composition.cpp) —
  `s_scene_w/s_scene_h` и `s_color_tex/s_depth_tex` обновятся через
  `composition_resize`. ✅ безопасно по построению.
- Любой aspect-fit dst rect кэшируется? Нет — в
  `composition_blit` он вычисляется каждый кадр из `window_w/h` и
  `s_scene_w/h`. ✅
- [`frontend.cpp`](../../new_game/src/ui/frontend/frontend.cpp) —
  FRONTEND_init_xition снимает `g_frame_w_px`/`g_frame_h_px` при
  старте перехода. Если ресайз случается во время перехода — переход
  может рисоваться в старых координатах. Проверить визуально. На
  практике редкий edge case.
- Скриншот-оверлей / loading screen background — если кэшируют
  размеры до start_level вызова, надо обновлять.
- Wibble post-process — ничего не кэширует, читает `ScreenWidth`
  каждый кадр.
- Шрифты / HUD — не кэшируют.

**Метод проверки:** `git grep -n 'RealDisplayWidth\|ScreenWidth\|ScreenHeight\|g_frame_' src/ --include='*.cpp'`,
пройтись по результатам, отметить места где значение читается **один
раз** и кэшируется в static/member. Таких скорее всего <5.

### Шаг 8: Fullscreen toggle (опционально, post-MVP)

Сейчас `OC_FULLSCREEN` — compile-time константа. Для полной фичи
«F11 — toggle fullscreen» — добавить:
- `sdl3_window_set_fullscreen(bool)` в bridge.
- После toggle вызвать тот же `ge_resize_display()`.
- Horkey через обычный input-actions путь.

Это уже скорее Stage 13, не блокер для ресайза.

---

## Тестирование

### Базовые сценарии

1. **Drag edge в widescreen**: потянуть за правый край 1920×1080 →
   900×1080 → 1920×1080. Рендер должен плавно следовать. Радар остаётся
   слева, msg-стек центруется в rubber gap'е (см. HUD layout в Resolved
   секции `issues.md`).
2. **Drag в portrait**: 1920×1080 → 500×1080 → 300×1200. HUD ужимается
   uniform'но. 3D letterbox-полосы в катсцене должны совпадать с UI-бaром.
3. **Пересечение aspect-clamp границ**: ультраширокий 2560×500 (выйдет
   за `OC_FOV_CAP_ASPECT=2.0`) → pillarbox (красные полосы слева/справа
   если `OC_DEBUG_COMPOSITION_BARS_RED`). Узкий 300×800 (за
   `OC_FOV_MIN_ASPECT=0.667`) → letterbox сверху/снизу.
4. **HiDPI**: на Mac Retina потянуть — drawable size в пикселях
   должен совпадать с window size × pixel_density. Отлаживай через
   `fprintf(stderr, "phys %d×%d\n", phys_w, phys_h)` временно в
   `ge_resize_display`.
5. **Drag во время катсцены**: не должно быть артефактов с диалог-барами
   (их layout уже привязан к UI-баам через CENTER_TOP / CENTER_BOTTOM,
   см. Resolved в `issues.md`).
6. **Drag во время меню паузы / main menu**: проверить что inner-bar
   клип для 4:3 framed region перестраивается.
7. **Быстрый ритмичный drag**: без дебаунса — проверить фреймрейт.
   С дебаунсом — проверить что ресайз применяется после отпускания.

### Ожидаемые визуальные инварианты

- Радар и msg-стек всегда пропорциональны (см. HUD layout в
  Resolved).
- Нет чёрного gap'а между кат-бaром и 3D сценой.
- Пилларбокс/летербокс композиционного слоя всегда = красный (если
  `OC_DEBUG_COMPOSITION_BARS_RED=true`), иначе чёрный.
- Крэшей / GL error'ов в консоли нет (проверить через
  `glGetError` после `composition_resize` для отладки).

---

## Риски / подводные камни

1. **GL object leak**. `composition_resize` корректно удаляет старые
   текстуры (`glDeleteTextures`) перед созданием новых — проверено в
   его реализации. Если когда-нибудь добавится ещё FBO attachment —
   не забыть в destroy path'е.
2. **Кадр в полёте**. SDL может доставить resize event посреди
   game_loop тика. После `composition_resize` старый scene FBO
   уничтожен — если game code ещё рисовал в него в этом кадре, будет
   GL error / чёрный кадр. Митигация: debounce + обрабатывать
   `s_resize_pending` в начале кадра (не внутри input pump).
3. **HiDPI ratio меняется**. Пользователь перетаскивает окно между
   мониторами разной плотности — SDL шлёт `PIXEL_SIZE_CHANGED`. Должно
   работать из коробки если всегда читаем `sdl3_window_get_drawable_size`.
4. **Минимальный размер окна**. Очень маленькое окно (например 10×10)
   → fbo_w/h = 1 после clamp → возможные деления на ноль вниз по
   стеку. `SDL_SetWindowMinimumSize(320, 240)` в bridge как защита.
5. **Несовместимость с `g_viewData` state**. POLY-pipeline записывает
   `g_viewData.dwWidth/Height` в `POLY_camera_set` из ScreenWidth/Height —
   это происходит каждый кадр, так что безопасно. Проверить нет ли
   другого кода, который читает `g_viewData` размеры между кадрами
   (grep `g_viewData.dwWidth`).
6. **Конфликт с `OC_FULLSCREEN=true`**. При fullscreen ресайз
   невозможен (SDL игнорирует drag), но API не должен ломаться.
   Протестировать что с `OC_FULLSCREEN=true` всё по-старому.

---

## Критерии приёмки

- [ ] Окно можно тянуть за любой край в любом направлении.
- [ ] Рендер следует за новым размером (после дебаунса если включён).
- [ ] На 4:3 / widescreen / portrait визуально всё как было до
  ресайза — HUD, катсцены, меню.
- [ ] Не хуже 30 fps во время активного drag'а с дебаунсом на
  Steam Deck (проверка если есть доступ; иначе — на любой
  интегрированной графике).
- [ ] Нет GL errors в stderr.
- [ ] `OC_FULLSCREEN=true` поведение идентично до-ресайзному.
- [ ] Нет крэшей при перетаскивании во время видео / кат-сцены /
  меню / геймплея.

---

## Estimate

- Шаги 1-5 (MVP): **~2-3 часа**.
- Шаг 6 (debounce): **~30-45 минут**.
- Шаг 7 (ревизия кэшей): **~30-60 минут**.
- Тестирование по чек-листу: **~1 час**.
- Запас на edge-кейсы и полировку: **~1 час**.

Итого **~5-6 часов** работы на одного человека. Post-1.0 фича.
