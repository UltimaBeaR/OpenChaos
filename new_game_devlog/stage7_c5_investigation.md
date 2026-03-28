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

---

## Расследование: что было проверено и исключено

### 1. ge_* обёртки (ИСКЛЮЧЕНО)

Каждая ge_* функция — прямой passthrough к Display/D3D методу:
- `ge_clear(true, true)` → `lp_D3D_Viewport->Clear(1, &ViewportRect, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER)`
- `ge_set_background_color(r,g,b)` → `the_display.SetUserColour(r,g,b)`
- `ge_set_background(User)` → `the_display.SetUserBackground()` → `SetBackground(user_handle)`

**Прямая подстановка D3D вызовов:** заменили весь `AENG_clear_viewport` на оригинальные
`SET_BLACK_BACKGROUND`, `the_display.SetUserColour()`, `the_display.ClearViewport()` напрямую
(минуя ge_*). **Баг остался.** → ge_* обёртки не виноваты.

### 2. AENG_clear_viewport — код идентичен (ИСКЛЮЧЕНО)

Старый код:
```cpp
the_display.SetUserColour(sky_colour);
the_display.SetUserBackground();
the_display.ClearViewport();
```
Новый код (через ge_*):
```cpp
ge_set_background_color(sky_colour);
ge_set_background(GEBackground::User);
ge_clear(true, true);
```
Транслируется в те же D3D вызовы. Подтверждено прямой подстановкой.

### 3. ViewportRect (ИСКЛЮЧЕНО)

Устанавливается в SetDisplay → (0, 0, 640, 480). Обновляется в `ge_set_viewport()`.
Между загрузкой и первым кадром не изменяется. Полноэкранный.

### 4. Loading screen flow (ИСКЛЮЧЕНО)

Полностью идентичен старой версии:
```
ATTRACT_loadscreen_init() → loads e3load.tga, shows+flips ×2
attract loop exits → ge_show_back_image + AENG_flip → ge_reset_back_image
game_init() → ATTRACT_loadscreen_draw (×N, но ge_show_back_image = no-op)
game_loop → first frame → AENG_draw → AENG_clear_viewport → rendering
```

### 5. Display::Flip / pre-flip callback (ИСКЛЮЧЕНО)

Старый Display::Flip содержал hardcoded `PreFlipTT()`, `PANEL_ResetDepthBodge()`,
`PANEL_screensaver_draw()`. Новый — через `s_pre_flip_callback` = `game_pre_flip()`,
которая вызывает те же три функции. Callback зарегистрирован до `OpenDisplay()`.
`PreFlipTT()` → `BlitText()` имеет `ASSERT(UC_FALSE)` — путь рендера текста не выполняется.

### 6. GERenderState cache (ИСКЛЮЧЕНО)

`InitScene()` вызывается в `POLY_frame_init()` на каждом кадре, устанавливает ВСЕ
render states через `REALLY_SET_RENDER_STATE`. Кэш синхронизируется полностью.

### 7. Пропущенные TSS из attract.cpp init (ИСКЛЮЧЕНО)

В старом attract.cpp перед attract loop было ~25 D3D вызовов, включая TSS stage 0 setup
и TSS stage 1 DISABLE. В новом — 10 ge_* вызовов (TSS не устанавливаются).

**Проверено:**
- Добавили только TSS (stage 0 + stage 1 disable) в `ge_init()` → баг остался
- Перенесли TSS в attract.cpp (то же место что в старом коде) → баг остался
- Добавили ВСЕ ~30 D3D вызовов из старого attract.cpp init → **баг остался**

TSS не являются причиной.

### 8. Stars ge_lock_screen (ИСКЛЮЧЕНО)

`SKY_draw_stars` рисует звёзды через `ge_lock_screen()` (прямой pixel access к back buffer)
в `AENG_draw_city` ДО первого `BeginScene`. Гипотеза: lock возвращает stale (до-clear) буфер.
Пропустили звёзды на первом кадре → **баг остался**. Не причина.

### 9. POLY_frame_init / POLY_frame_draw (ИСКЛЮЧЕНО)

`POLY_frame_init` не делает clear — только очищает списки полигонов, ставит render states,
вызывает `BeginScene`. `POLY_frame_draw` рендерит опак-геометрию и вызывает `EndScene`.
Между ними множество POLY_frame_init/frame_draw циклов — все идентичны старому коду.
Double BeginScene (frame_init → draw_odd) присутствует и в старом коде — не причина.

---

## Эксперименты с позицией clear (диагностика dirty buffer)

### Фаза 1: Внешние clear+flip (НЕ РАБОТАЮТ)

| Эксперимент | Результат |
|---|---|
| 3× ge_clear+ge_flip перед game loop | ❌ баг остался |
| 30 красных кадров (ge_clear красным + screen_flip + Sleep) | ❌ красный НЕ виден на экране, но Sleep замедляет загрузку → код выполняется |
| ge_flip вместо screen_flip (blit) | ❌ красный всё равно не виден |

**Вывод:** ge_clear+flip/blit без полного рендер-пайплайна (BeginScene→Draw→EndScene)
не отображаются на экране. Невозможно диагностировать через этот путь.

### Фаза 2: Диагностика внутри рендер-пайплайна (РАБОТАЮТ)

| Эксперимент | Результат |
|---|---|
| Opaque sky (AlphaBlendEnabled=false) на первом кадре | ✅ баг пропал (луна с чёрной подложкой — ожидаемо) |
| DstBlend=Zero для sky+moon на первом кадре | ✅ баг пропал (луна с чёрной подложкой) |
| ge_clear(true, false) внутри draw_odd (после BeginScene) | ✅ баг пропал |

**Вывод:** буфер грязный в момент рисования неба. Аддитивный бленд выявляет остатки.

### Фаза 3: Бинарный поиск позиции clear

| Позиция clear | Результат | Пояснение |
|---|---|---|
| AENG_clear_viewport (out-of-scene, начало кадра) | ❌ | Стандартный clear, не помогает |
| AENG_clear_viewport обёрнутый в BeginScene/EndScene | ❌ | Отдельная сцена до рендера |
| POLY_frame_init (после BeginScene, до рендера) | ❌ | Начало первой render-сцены |
| **Между последним EndScene и draw_odd** | ✅ | После основного рендера, до аддитивного неба |
| Внутри draw_odd (после BeginScene) | ✅ | Прямо перед рендером неба |

**Ключевой вывод:** clear работает ТОЛЬКО после завершения основной render-сцены
(после EndScene из POLY_frame_draw). Clear до этого момента (в любой позиции —
out-of-scene, в собственной сцене, в начале первой сцены) — не помогает.

Это значит: back buffer получает "грязные" пиксели где-то ВНУТРИ основной render-сцены,
и повторный clear до EndScene не помогает. Только clear после EndScene (когда сцена
"зафиксирована") эффективно очищает буфер.

### Фаза 4: Проверка AENG_clear_viewport с прямыми D3D вызовами

Заменили весь `AENG_clear_viewport` на прямые D3D вызовы из stage_5_1_done
(`SET_BLACK_BACKGROUND`, `the_display.SetUserColour()`, `the_display.ClearViewport()`)
минуя ge_* обёртки. **Баг остался.** → проблема не в обёртках.

---

## Нерешённый вопрос

Точная строка ошибки переноса НЕ найдена. Все D3D вызовы 1:1 идентичны
(подтверждено полным ревью Stage 7 + прямой подстановкой D3D вызовов).
Тем не менее, на первом кадре после загрузки back buffer содержит остатки
загрузочного экрана, которые не очищаются стандартным `Viewport::Clear`.

Возможные причины, которые не удалось проверить:
- Разный порядок компиляции/линковки (Clang vs MSVC) влияет на порядок
  статической инициализации, что меняет D3D device state
- Тонкое различие в порядке D3D вызовов на переходе loading→game,
  невидимое при покомпонентном сравнении
- Различие в выравнивании/паддинге структур при смене компилятора,
  влияющее на D3D surface locking

---

## Фикс

**Файл:** `engine/graphics/pipeline/aeng.cpp`, в `AENG_draw_city()`,
перед вызовом `POLY_frame_draw_odd()`.

Дополнительный `ge_clear(true, false)` (color only) между EndScene основного рендера
и BeginScene аддитивного неба. Только на первом кадре после загрузки (static bool).

Это гарантирует что буфер чист перед аддитивным блендингом sky/moon,
независимо от того, что произошло с буфером во время основного рендера.

## Статус

✅ **ИСПРАВЛЕН (проверено).** Корневая причина не найдена, но фикс
протестирован, минимален (одна строка, один раз), и не влияет на
остальной рендер.
