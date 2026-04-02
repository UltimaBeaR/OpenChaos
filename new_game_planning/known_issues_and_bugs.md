# Известные проблемы и баги

Бессрочный список — пополняется и фиксится по ходу разработки, не привязан к конкретному этапу.
Пофикшенные баги пре-релиза → [`prerelease_fixes.md`](prerelease_fixes.md)

---

## Незафикшенные пре-релизные баги (известна причина из анализа кода)

| Баг | Файл | Описание |
|-----|------|----------|
| Воскрешение гражданских | pcom.cpp | Телепорт на HomeX/HomeZ считается но не применяется — воскресают на месте смерти |
| Баг освещения | night.cpp | `dz×nx` вместо `dz×nz` в расчёте статических источников света |
| MAV INSIDE2 | mav.cpp | `INSIDE2_mav_inside()` → `ASSERT(0)`, NPC не ходят внутри зданий |
| MAV stair/exit | mav.cpp | `> 16` вместо `>> 16` — возвращает текущую позицию вместо целевой |
| Лестницы | interfac.cpp | `mount_ladder()` закомментирован |
| Crinkle | mesh.cpp | `return NULL` — отключён, но работает в финальной PC версии |
| Wibble воды | water.cpp | Закомментирован в пре-релизе, уточнить есть ли в финале |
| ~~Джойстик/геймпад не работает~~ | ~~DIManager.cpp~~ | ~~`OS_joy_init()` не дописана.~~ **Исправлено на этапе 5.1** — DirectInput заменён на SDL3 + Dualsense-Multiplatform. |

---

## Баги из PieroZ/MuckyFoot-UrbanChaos (требуют проверки)

Найдены при анализе коммитов репозитория `github.com/PieroZ/MuckyFoot-UrbanChaos`.
Не факт что актуальны для нашей кодовой базы — требуют проверки.

| Баг | Коммит (PieroZ repo) | Описание | Статус |
|-----|----------------------|----------|--------|
| OpenAL спам "Invalid source ID 0" | `c8b1d76` (PieroZ) | Спам сообщений в логах. Не видно без отладчика. | Проверить |
| Bat/Baalrog: выход за массив | `021c515` (PieroZ) | `BAT_STATE_TURRET_FOLLOW` (20) удалён, `BAT_STATE_NUMBER` уменьшен 21→20. Массив `BAT_state_name` был на 1 элемент короче → потенциальный краш на босс-уровнях. | Проверить при прохождении |

---

## Управление

| Проблема | Описание | Статус |
|----------|----------|--------|
| ~~Маппинг кнопок в авто не соответствует PS1~~ | Было: Square=вперёд, Cross=мигалки. Исправлено: геймпад использует PSX маппинг (Cross=газ, Square=тормоз, Triangle=мигалка), клавиатура — PC маппинг (Z=газ, X=тормоз, Space=мигалка). Два набора макросов `INPUT_CAR_KB_*` / `INPUT_CAR_PAD_*`, выбор по `active_input_device`. | ✅ Исправлено |
| ~~Автоскролл в меню паузы без геймпада~~ | `gamepad_state` обнулялась через `memset` при disconnect — оси lX/lY становились 0 вместо 32768 (центр). Код меню интерпретировал 0 как "стик вверх". Фикс: инициализация и reset осей в 32768. | ✅ Исправлено |

---

## Визуальные проблемы (замечены вживую)

| Проблема | Описание | Статус |
|----------|----------|--------|
| Здания появляются резко за пределами тумана | Дистанция появления зданий/объектов слишком маленькая — туман уже прошёл, а здание резко pop-in'ится. Возможно настраивается через ini/config. | Исследовать |
| ~~OpenGL: alpha sort — тормоза на macOS~~ | ~~Per-polygon `DrawSinglePoly` × 350+ = 70-85ms на macOS GL→Metal.~~ **Исправлено:** batched alpha sort — накопление индексов по runs одинаковых PolyPage, один draw call на run вместо per-polygon. 350 draw calls → ~10-20. Подробности → [`stage8_log.md`](../new_game_devlog/stage8_log.md) (2026-04-02). | ✅ Исправлено |
| ~~OpenGL: z-fighting теней~~ | ~~Тени персонажей и машин z-файтились с землёй.~~ **Исправлено:** (1) `DepthBias` отсутствовал в `InitScene`/`SetChanged` → `ge_set_depth_bias()` никогда не вызывался; (2) `POLY_PAGE_SHADOW_OVAL`/`SHADOW_SQUARE` не имели `SetDepthBias(8)`. | ✅ Исправлено |
| OpenGL: шрифты меню (olyfont2) — ghost RGB фон | Ghost RGB в alpha=0 пикселях виден при additive blend (POLY_PAGE_NEWFONT_INVERSE). В D3D6 ghost не виден, только 1px окантовка вокруг букв. Подробности → [`stage7_ghost_rgb_investigation.md`](../new_game_devlog/stage7_ghost_rgb_investigation.md) (баг 1). | Исследовать |
| OpenGL: HUD иконка (fist и др.) — ghost RGB фон | Ghost RGB в alpha=0 пикселях виден при additive blend (POLY_PAGE_LASTPANEL2_ADD). Нельзя обнулить RGB по alpha — иконка сама alpha=0. В D3D6 ghost не виден, только 1px окантовка. Подробности → [`stage7_ghost_rgb_investigation.md`](../new_game_devlog/stage7_ghost_rgb_investigation.md) (баг 2). | Исследовать |
| ~~OpenGL: туман не применяется к стенам и объектам~~ | ~~Туман работал только на земле.~~ **Исправлено:** добавлен D3D6-style linear table fog по Z-дистанции в шейдере (fog_near/fog_far). Terrain получает `min(z_fog, specular_fog)`, стены/объекты — z_fog. Незначительная разница в интенсивности с D3D — требуется тонкая настройка. | ✅ Исправлено (tuning pending) |
| ~~OpenGL: листья (POLY_PAGE_LEAF) не отображаются in-game~~ | ~~Листья не рисовались — единственная геометрия использовавшая lit vertex shader (GPU-transform). Все остальное в игре — CPU-transform → TL path.~~ **Исправлено:** `ge_draw_indexed_primitive_lit` в OpenGL бэкенде теперь делает CPU-transform (World×Projection) и рисует через TL path, как `ge_draw_multi_matrix`. **Техдолг:** lit vertex shader (`lit_vert.glsl`) не работает из-за несовместимости D3D/GL clip space конвенций — починить при необходимости (glClipControl или ремап projection). | ✅ Исправлено (техдолг: GPU-path) |
| ~~OpenGL: окно без рамки~~ | ~~Окно запускалось с `WS_POPUP` стилем (без заголовка и рамки), в отличие от D3D бэкенда.~~ **Исправлено:** `gl_context.cpp` теперь меняет стиль на `WS_OVERLAPPED \| WS_CAPTION \| WS_THICKFRAME \| WS_MINIMIZEBOX`, как D3D бэкенд в `display.cpp`. | ✅ Исправлено |
| ~~OpenGL: краш при gameplay~~ | ~~Краш в WIBBLE_simple при пробежке по лужам — wibble эффект читал за пределами dummy screen buffer.~~ **Исправлено:** `ge_lock_screen()` возвращает NULL → wibble пропускается. Лужи не рябят (Phase 6). | ✅ Исправлено |
| ~~OpenGL: нет анимации перехода между экранами меню~~ | ~~Задник появлялся резко.~~ **Исправлено:** реализованы `ge_blit_surface_to_backbuffer` (блит rect'а текстуры на экран), `ge_load_screen_surface` / `ge_create_screen_surface` / `ge_destroy_screen_surface` (загрузка BMP фонов в GL текстуры), override-механизм в `ge_blit_background_surface`. UV маппинг: screen coords / viewport size (D3D масштабирует 640×480 текстуру до размера экрана через CopyBackground32). | ✅ Исправлено |
| OpenGL: экран брифинга не убирается при загрузке миссии | При старте миссии загрузочный экран (e3load.tga) рисуется, но поверх него остаётся задник брифинга (background override не сбрасывается). В D3D версии загрузочный экран виден. Вероятно `FRONTEND_scr_unload_theme` не вызывается до `ATTRACT_loadscreen_init`, либо порядок вызовов отличается. | Исследовать |
| ~~OpenGL: засвет при обратном слайде в меню~~ | ~~При возврате из карты миссий в главное меню анимация слайда показывала засвеченную/наложенную картинку вместо чистой замены.~~ **Исправлено:** `gl_blit_fullscreen_texture` и `ge_blit_surface_to_backbuffer` теперь отключают blend/alpha test/depth перед отрисовкой. D3D `Blt` копирует пиксели напрямую (игнорирует render state), а GL рисует через pipeline — нужен явный сброс состояний. | ✅ Исправлено |
| OpenGL: краш при смене миссии | Access violation reading 0x00000008 (null + offset 8) в системной DLL. Стек: `BARREL_alloc` (barrel.cpp:1055) → `slide_around_sausage` (collide.cpp:4681) → `plant_feet` (collide.cpp:1412) → `Arctan` (math.cpp:68) → крах в ntdll. Вероятно не связано с OpenGL — физика/коллизии при переходе между уровнями. | Исследовать |
| OpenGL: тень глючит когда враг целится | Когда враг целится огнестрельным оружием — появляются пунктирные линии прицела. В этот момент тень персонажа рисуется серым прямоугольником (того же цвета что пунктиры). Когда пунктиры пропадают — тень возвращается в норму. Похоже на незачищенный render state или неправильно привязанную текстуру — после отрисовки пунктиров остаётся их текстура/состояние, и тень рисуется с ним. В D3D не воспроизводится (предположительно). Та же категория что баг с drip-кружками на средней дистанции (пофикшен) — render state leak между poly pages. Вероятно poly page пунктиров не выставляет какой-то state (текстура, blend mode), и следующий poly page (тень) наследует неправильное значение. Фикс drip'ов был в `ge_unlock_screen` (save/restore state) — здесь причина другая, нужно искать какой poly page рисует пунктиры и какой state утекает. | Исследовать |
| ~~OpenGL: сильные тормоза при огне в кадре~~ | ~~На уровне Urban Shakedown — когда огонь в кадре, FPS резко падает.~~ **Исправлено:** причина — per-draw `glBufferData` с разными размерами на одном shared VBO (сотни раз за кадр). Драйвер переаллоцировал внутренний буфер каждый раз. Фикс: per-slot VBO/EBO/VAO (каждый GLVertexBuf slot получил свой GL буфер), VBO аллоцируется по power-of-2 один раз и переиспользуется через `glBufferSubData`. | ✅ Исправлено |
| Дублированные inline функции в разных .cpp файлах | Оригинальный код содержит `inline` функции с одинаковым именем в разных .cpp файлах (например `screen_flip` в `game.cpp` и `playcuts.cpp`). По стандарту C++ все определения `inline` функции должны быть идентичны — иначе UB. Линкер молча выбирает одно определение, и вызывается не та версия. Обнаружено на `screen_flip`: game.cpp вызывал версию из playcuts.cpp (без `AENG_screen_shot`), debug лог внутри game.cpp версии не появлялся. **Нужно:** найти ВСЕ такие дублированные `inline` функции в кодовой базе и сделать их `static` (file-local) или переименовать. Потенциально источник скрытых багов. | Исправить |
| ~~x64: пропадающие стены и ящики вблизи~~ | `CRINKLE_read_bin` читала `CRINKLE_Crinkle` (struct с указателями) через `sizeof` → 24 на x64 вместо 16 → сдвиг bptr на 8 байт → мусор в crinkle mesh данных → текстуры с bump (окна, двери, ящики) не рендерились при z < 0.3. Фикс: фиксированный on-disk size 16 байт. Удалена неиспользуемая `CRINKLE_write_bin` (level editor tool). | ✅ Исправлено |
| Чёрные дырки на ближнем клиппинге | Целые полигоны (пол + стены) пропадают при приближении камеры (L1 zoom). Баг оригинального движка (был до миграций). Исследовано 7 гипотез — ни одна не подтвердилась: **NOT** near-plane (POLY_ZCLIP_PLANE 1/64→1/8192), **NOT** AABB rejection в FACET_draw, **NOT** D3DDP_DONOTCLIP, **NOT** gamut culling (расширение +2 тайла), **NOT** POLY_valid_quad, **NOT** DrawIndPrimMM near-Z guard, **NOT** dprod threshold (8→1). Затрагивает оба рендер-пути (POLY pipeline для стен, DrawIndPrimMM для пола). ~~Вероятная причина: D3D7 compatibility layer на Windows 11 некорректно обрабатывает pre-transformed vertices с экранными координатами далеко за пределами viewport (guardband clipping). **Проверить после порта на OpenGL (этап 7)** — если баг исчезнет, подтвердится D3D7-гипотеза.~~ **Проверено на этапе 7:** баг воспроизводится и в OpenGL бэкэнде — гипотеза D3D7 guardband clipping **опровергнута**. Причина в игровом коде (CPU-side culling или клиппинг), не в графическом API. | Исследовать |

---

## CRT Debug Assertions

| Проблема | Описание | Статус |
|----------|----------|--------|
| ~~CRT heap corruption (debug build)~~ | ~~Debug Assertion Failed в `debug_heap.cpp:904-908`: `_CrtIsValidHeapPointer(block)` и `is_block_type_valid(header->_block_use)`.~~ **Исправлено (2026-04-02):** root cause найден через ASan — `ge_vb_expand` аллоцировал 65536 байт, но `memcpy` копировал 131072 (logsize/capacity mismatch при переиспользовании VB слота). Фикс: skip realloc если буфер уже достаточно большой. Также исправлены 5 попутных багов (ReadSquished overread, strcpy overlap, arctan off-by-one, SIN overflow, tmat use-after-scope). Подробности → `stage8_urban_shakedown_crash.md`. | ✅ Исправлено |

---

## Крэши

| Проблема | Описание | Статус |
|----------|----------|--------|
| Крэш при загрузке Psycho Park | После прохождения лоадера игра вылетает без видимых ошибок. Проверить воспроизводится ли в оригинальной (немодифицированной) сборке. | Исследовать |
| Крэш при загрузке Stern Revenge | Аналогично Psycho Park — вылет после лоадера без ошибок. Возможно общая причина. | Исследовать |
| Крэш при zoom-камере (A) вверх | Access violation reading 0x00000008 (null + offset 8). Стек: `FIGURE_draw_reflection` (figure.cpp) → `MSMesh::SetSize` → `MemFree`. `p_thing->Draw.Tweened` содержит невалидный указатель или `MSMesh` вызывается на null объекте. **Не стабильно** — воспроизводился при zoom-камере (A) вверх, и в других ситуациях. Один и тот же стек в разных крашах. Не связано с OpenGL — игровая логика. | Исследовать |
| Крэш при езде на машине (reflection) | Access violation reading 0x00000008 (null + offset 8). Стек: `FIGURE_draw_prim_tween_reflection` (figure.cpp:3338-3352), также `SUPERFACET_redo_lighting` (superfacet.cpp:758) / `SUPERFACET_init` (superfacet.cpp:713) в стеке. Тот же паттерн (null+8) что и крэш zoom-камеры — вероятно общая причина в коде отрисовки reflection. Воспроизведён 2026-03-30 на миссии 1 при езде на машине. Не связано с SDL3 миграцией окна. | Исследовать |
| Крэш в ge_draw_indexed_primitive_vb (uninit heap) | Access violation writing 0xCDCDCE09 (MSVC uninit heap pattern `0xCDCDCDCD` + offset). Стек: `PolyPage::Render` (polypage.cpp:300) → `ge_draw_indexed_primitive_vb` (core.cpp:1631) — `buf->data` указывает на freed/uninit память. VB pool slot был освобождён или не инициализирован. **Не стабильно.** | Исследовать |
| ~~Крэш на синематике Urban Shakedown~~ | ~~**Стабильно воспроизводился** на OpenGL бэкенде.~~ **Исправлено (2026-04-02):** root cause — `ge_vb_expand` heap overflow (logsize/capacity mismatch). ASan нашёл точное место. Подробности → `stage8_urban_shakedown_crash.md`. | ✅ Исправлено |
| Бесконечный скролл меню (stick drift) | В главном меню иногда начинается непрерывное прокручивание пунктов вниз, как будто зажата клавиша. Подозрение: нет deadzone для стиков геймпада в обработке меню. Проявилось на Xbox контроллере (сильный drift), один раз на DualSense. В геймплее проблемы нет — значит deadzone применяется только в игровом инпуте, но не в меню. | Исследовать |
| Рандомные краши во время геймплея | Игра периодически крашится в произвольных местах при обычной игре. Не привязано к конкретной миссии или действию. Скорее всего это **несколько разных багов** — memory corruption, uninit pointers, out-of-bounds в legacy коде. ASan (`.claude/skills/asan/`) показал себя эффективно на Urban Shakedown — план: прогонять игру под ASan, фиксить баги один за одним. Часть перечисленных выше крашей (reflection, uninit heap) могут быть проявлениями этих же проблем. | Исследовать (ASan) |

---

## Сборка D3D6 бэкенда

| Проблема | Описание | Статус |
|----------|----------|--------|
| D3D6 не собирается на x64 с clang++ | Три категории ошибок: (1) `ULONG` — `types.h` определяет `typedef uint32_t ULONG`, `minwindef.h` — `typedef unsigned long ULONG`, нет guard'а `#ifndef _WINDEF_` для ULONG; (2) `DWORD` — guard `#ifndef _WINDEF_` есть, но в некоторых TU `types.h` включается до `windows.h` → guard не срабатывает; (3) `_BitScanForward`, `__readgsbyte` и др. — конфликт builtins `clang++` (GNU frontend) с объявлениями в `winnt.h`. `clang-cl` (MSVC frontend) умеет обходить эти конфликты, `clang++` — нет. **Варианты фикса:** (a) компилировать DX6 файлы через `clang-cl` (отдельный target с другим компилятором), (b) вернуть DX6 на x86 тулчейн (`clang-x86-windows.cmake`), (c) добавить guard'ы и workaround'ы для `clang++` + Windows SDK. | Отложен |
| D3D6 не тестировался на x64 | Помимо проблем компиляции, DX6 API на x64 не проверялся — могут быть runtime проблемы (DirectDraw/Direct3D COM интерфейсы, pointer-size вопросы в DX6 structures). DX6 бэкенд остаётся legacy для x86, на x64 основной бэкенд — OpenGL. | Отложен |

---

## macOS-специфичные проблемы

| Проблема | Описание | Статус |
|----------|----------|--------|
| ~~Очень низкий FPS (ощущение как software rendering)~~ | **ИСПРАВЛЕНО.** Причина: alpha sort рендерил каждый полигон отдельным GL draw call (350+ вызовов за кадр). Фикс: batched alpha sort — накопление индексов + один draw call на PolyPage. Frame time с ~80ms → ~5ms. | ✅ Исправлено |
| ~~DualSense: огромная задержка ввода (~1 секунда)~~ | **ИСПРАВЛЕНО.** Причина: macOS IOKit HID бэкенд (через SDL3 hidapi) возвращает репорты из FIFO-очереди (самый старый первым). DualSense по BT шлёт ~133 репортов/сек, игра читает ~30/сек → очередь росла на ~100/сек → через секунду мы читали данные секундной давности. На Windows HID бэкенд ведёт себя иначе. Фикс: drain всей очереди за кадр, использовать последний репорт (ds_bridge.cpp `read()`). Также: dirty-check для LED/vibration output (gamepad.cpp), skip PlugAndPlay re-enumeration при подключённом устройстве (ds_bridge.cpp). | ✅ Исправлено |

---

## Отложенный функционал (DualSense, из этапа 5.1)

| Фича | Описание | Приоритет |
|------|----------|-----------|
| B5 — Тачпад | `EnableTouch()`, читать TouchPosition/TouchRelative. Варианты: камера, карта, инвентарь. Требует дизайн-решения. | Низкий |
| B6 — Гироскоп | `EnableMotionSensor()`, читать Gyroscope. Точное прицеливание. Требует калибровку и UI. | Средний |
| B7 — Audio-to-haptic | `AudioHapticUpdate()` через miniaudio. Конвертация звуков (стрельба, взрывы, двигатель) в тактильную обратную связь. Требует интеграцию с MFX. | Средний |
| Вибрация в видеовставках | MDEC_vibra[] — frame-synchronized vibration для intro/endgame. Требует интеграцию с видеоплеером. | Низкий |
| Вибрация в меню | Тест-вибрация при включении опции в настройках. Опция пока не в UI. | Низкий |

---

## Желаемый функционал (из PS1 версии)

| Фича | Описание | Статус |
|------|----------|--------|
| Подсказки кнопок геймпада в меню | На PS1 в главном меню (и возможно в других местах) показываются подсказки: ✕ — выбрать, △ — назад. Хотелось бы вернуть для PC — показывать только при подключённом DualSense. Нужно: (1) вытащить код из PS1 версии (`original_game/fallen/PSXENG/`), посмотреть как рисовалось, (2) портировать отрисовку, (3) показывать условно по `active_input_device == gamepad`. | Реализовать |

---

## Отсутствующий функционал (вырезан при модернизации)

| Проблема | Описание | Статус |
|----------|----------|--------|
| ~~Видеоролики не воспроизводятся~~ | **ИСПРАВЛЕНО.** FFmpeg (libavcodec) заменил проприетарный Bink SDK. Все 6 роликов работают: 3 интро (Eidos, logo, intro) + 3 катсцены. Видео: FFmpeg decode → GL текстура → fullscreen quad. Аудио: FFmpeg decode → OpenAL streaming. A/V sync по wall clock. Скип: клавиатура, мышь, SDL gamepad, DualSense (edge-triggered). Кросс-платформенно. Девлог: `new_game_devlog/bink_video_playback.md`. | ✅ Исправлено |
