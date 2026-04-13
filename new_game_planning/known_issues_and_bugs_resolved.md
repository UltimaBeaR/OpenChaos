# Исправленные баги и закрытые проблемы (архив)

Архив всех исправленных/закрытых записей из [`known_issues_and_bugs.md`](known_issues_and_bugs.md).
Полезно при расследовании регрессий — если симптомы знакомые, возможно баг уже был и описан здесь.

---

## Пре-релизные баги (исправленные)

| Баг | Файл | Описание |
|-----|------|----------|
| Баг освещения | night.cpp | `dz×nx` вместо `dz×nz` в расчёте статических источников света. **Исправлено:** оба места в `NIGHT_light_mapsquare`. Визуально незаметно — на плоской земле nx=nz=0, эффект только на крутых склонах с разными nx/nz. |
| Джойстик/геймпад не работает | DIManager.cpp | `OS_joy_init()` не дописана. **Исправлено на этапе 5.1** — DirectInput заменён на SDL3 + Dualsense-Multiplatform. |

---

## Управление (исправленные)

| Проблема | Описание |
|----------|----------|
| Маппинг кнопок в авто не соответствует PS1 | Было: Square=вперёд, Cross=мигалки. Исправлено: геймпад использует PSX маппинг (Cross=газ, Square=тормоз, Triangle=мигалка), клавиатура — PC маппинг (Z=газ, X=тормоз, Space=мигалка). Два набора макросов `INPUT_CAR_KB_*` / `INPUT_CAR_PAD_*`, выбор по `active_input_device`. |
| Автоскролл в меню паузы без геймпада | `gamepad_state` обнулялась через `memset` при disconnect — оси lX/lY становились 0 вместо 32768 (центр). Код меню интерпретировал 0 как "стик вверх". Фикс: инициализация и reset осей в 32768. |

---

## Визуальные проблемы (исправленные)

| Проблема | Описание |
|----------|----------|
| Здания появляются резко за пределами тумана | Pop-in. Первоначальный план был "увеличить дальность отрисовки + alpha fade на границе". **Исправлено (2026-04-13) иначе** — через систему `FARFACET` (дальние силуэты зданий): MuckyFoot уже написали её для релизной PC-версии, но в пре-релизных исходниках она была отключена на PC через `#ifndef TARGET_DC / if (!Keys[KB_R]) return;`. Включили, портировали под OpenGL (стейл world matrix → `POLY_set_local_rotation_none`; убран broken `MY_PROJ_MATRIX_SCALE` Z-bias hack; отключён depth test/write, alpha blend on; альфа в шейдере привязана к fog density, силуэт красится в `u_fog_color`; cull по дальнему углу 4×4 квадрата). Добавлен F10 debug-режим (после `bangunsnotgames`) с визуализацией silhouette body / fade band. Подробности → [`stage12_farfacets.md`](../new_game_devlog/stage12_farfacets.md). |
| Освещение персонажей (плоское, без светлой/тёмной стороны) | Software fallback `DrawIndPrimMM` Tom Forsyth'а в пре-релизных исходниках MuckyFoot был заглушкой (`// I don't actually do lighting yet` — его собственный коммент); реальное освещение делал hardware D3D6 driver через расширение `D3DDP_MULTIMATRIX`. Мы унаследовали заглушку при портировании в Stage 7. **Исправлено (2026-04-12):** реализовано per-vertex software освещение в `ge_draw_multi_matrix` — dot(normal, bone-local light dir) → index в `MM_pcFadeTable[128]`, собираемой `BuildMMLightingTable`. Дополнительно исправлен критический layout mismatch (каст `GEVertex*` → `GEVertexLit*`) и неправильный масштаб нормалей (нормали хранятся unit после scale × 1/256, не tiny 1/256-вектор). Применяется **half-Lambert** (Valve/HL2 wrap): `wrap = cos × 0.5 + 0.5`, индекс в лит-рампе 0..63 по всей модели без cutoff'а теневой стороны. Результат выглядит естественнее чем релиз/PieroZ (там хардкорное byte-quantized освещение даёт "пластиковых" персонажей, плохо вписанных в окружение). Второй симптом этого же бага — "слишком ярко ночью на Auto Destruct" — ожидает проверки на сцене. Расследование → [`stage12_character_lighting_investigation.md`](stage12_character_lighting_investigation.md). Devlog → [`character_lighting_fix.md`](../new_game_devlog/character_lighting_fix.md). |
| Меню Sound Options — бегунки громкости не видны | Бегунки (слайдеры) в меню Sound Options не отображались. **Исправлено:** та же корневая причина что невидимый пар — `DRAW2D_Box` ставил `specular = 0` → fog заменял цвет. Фикс: `specular = 0xFF000000` во всех DRAW2D функциях (Box, Box_Page, Tri, Sprite). |
| OpenGL: alpha sort — тормоза на macOS | Per-polygon `DrawSinglePoly` × 350+ = 70-85ms на macOS GL→Metal. **Исправлено:** batched alpha sort — накопление индексов по runs одинаковых PolyPage, один draw call на run вместо per-polygon. 350 draw calls → ~10-20. Подробности → [`stage8_log.md`](../new_game_devlog/stage8_log.md) (2026-04-02). |
| OpenGL: z-fighting теней | Тени персонажей и машин z-файтились с землёй. **Исправлено:** (1) `DepthBias` отсутствовал в `InitScene`/`SetChanged` → `ge_set_depth_bias()` никогда не вызывался; (2) `POLY_PAGE_SHADOW_OVAL`/`SHADOW_SQUARE` не имели `SetDepthBias(8)`. |
| OpenGL: шрифты меню (olyfont2) — ghost RGB фон | Ghost RGB в alpha=0 пикселях виден при additive blend. Причина: баг в `gl_bleed_edges()` — цепная реакция edge bleeding заполняла чёрный фон паразитным RGB. |
| OpenGL: HUD иконка (fist и др.) — ghost RGB фон | Ghost RGB в alpha=0 пикселях виден при additive blend. Та же причина: `gl_bleed_edges()` читала/писала в один массив, D3D6 использовал два раздельных буфера. |
| OpenGL: туман не применяется к стенам и объектам | Туман работал только на земле. **Исправлено:** добавлен D3D6-style linear table fog по Z-дистанции в шейдере (fog_near/fog_far). Terrain получает `min(z_fog, specular_fog)`, стены/объекты — z_fog. Незначительная разница в интенсивности с D3D — требуется тонкая настройка. |
| OpenGL: листья (POLY_PAGE_LEAF) не отображаются in-game | Листья не рисовались — единственная геометрия использовавшая lit vertex shader (GPU-transform). Все остальное в игре — CPU-transform → TL path. **Исправлено:** `ge_draw_indexed_primitive_lit` в OpenGL бэкенде теперь делает CPU-transform (World×Projection) и рисует через TL path, как `ge_draw_multi_matrix`. **Техдолг:** lit vertex shader (`lit_vert.glsl`) не работает из-за несовместимости D3D/GL clip space конвенций — починить при необходимости (glClipControl или ремап projection). |
| OpenGL: окно без рамки | Окно запускалось с `WS_POPUP` стилем (без заголовка и рамки), в отличие от D3D бэкенда. **Исправлено:** `gl_context.cpp` теперь меняет стиль на `WS_OVERLAPPED \| WS_CAPTION \| WS_THICKFRAME \| WS_MINIMIZEBOX`, как D3D бэкенд в `display.cpp`. |
| OpenGL: краш при gameplay | Краш в WIBBLE_simple при пробежке по лужам — wibble эффект читал за пределами dummy screen buffer. **Исправлено:** `ge_lock_screen()` возвращает NULL → wibble пропускается. Лужи не рябят (Phase 6). |
| OpenGL: нет анимации перехода между экранами меню | Задник появлялся резко. **Исправлено:** реализованы `ge_blit_surface_to_backbuffer` (блит rect'а текстуры на экран), `ge_load_screen_surface` / `ge_create_screen_surface` / `ge_destroy_screen_surface` (загрузка BMP фонов в GL текстуры), override-механизм в `ge_blit_background_surface`. UV маппинг: screen coords / viewport size (D3D масштабирует 640×480 текстуру до размера экрана через CopyBackground32). |
| OpenGL: засвет при обратном слайде в меню | При возврате из карты миссий в главное меню анимация слайда показывала засвеченную/наложенную картинку вместо чистой замены. **Исправлено:** `gl_blit_fullscreen_texture` и `ge_blit_surface_to_backbuffer` теперь отключают blend/alpha test/depth перед отрисовкой. D3D `Blt` копирует пиксели напрямую (игнорирует render state), а GL рисует через pipeline — нужен явный сброс состояний. |
| OpenGL: артефакты теней (квадратики, силуэты на дороге) | Тени персонажей ломались при прицеливании NPC или в debug overlay: серые квадраты поверх теней, артефакты-силуэты на дороге, пропадание тени Дарси. **Исправлено (2026-04-05):** геометрия добавленная в poly pipeline вне render pass (до `POLY_frame_init`) вызывала corruption. Фикс: (1) `s_in_render_pass` флаг в `AENG_world_line` блокирует game-tick геометрию; (2) deferred `draw_view_line` (overlay.cpp). Подробности расследования → [`shadow_corruption_investigation.md`](../new_game_devlog/shadow_corruption_investigation.md). |
| OpenGL: сильные тормоза при огне в кадре | На уровне Urban Shakedown — когда огонь в кадре, FPS резко падает. **Исправлено:** причина — per-draw `glBufferData` с разными размерами на одном shared VBO (сотни раз за кадр). Драйвер переаллоцировал внутренний буфер каждый раз. Фикс: per-slot VBO/EBO/VAO (каждый GLVertexBuf slot получил свой GL буфер), VBO аллоцируется по power-of-2 один раз и переиспользуется через `glBufferSubData`. |
| Лужи пропадают на краю камеры | Лужи итерируются по gamut-ячейкам. Когда камера поворачивается, ячейка с лужей выпадает из gamut и лужа не рисуется. Присутствует в оригинале. **Исправлено:** цикл луж в `aeng.cpp` теперь итерирует все 128 z-строк карты с полным x-диапазоном вместо gamut-диапазона. **Почему это оптимально:** `PUDDLE_mapwho` — массив 128 байт (одна-две кеш-линии CPU). На пустой z-строке — одно чтение байта и return. Суммарная стоимость полного прохода ~1-2 нс (~0.00006% кадра при 30 FPS). Margin-подход (расширение gamut на N ячеек) был опробован первым (margin=3) — визуально недостаточно, а подбор margin под угол камеры добавляет сложность без измеримого выигрыша. Лужи за frustum'ом отсекаются на `POLY_transform` → `MaybeValid()` → `goto ignore_this_puddle` — лишних полигонов в pipeline не попадает. |
| Нельзя забраться на полицейскую машину | **Не баг:** проверено в релизной PC-версии — там так же. Оригинальное поведение. |
| Отладочная линия при броске гранаты видна в обычном режиме | При прицеливании гранатой рисовался debug trace line (линия траектории). **Исправлено:** перенесено под `ControlFlag && allow_debug_keys` — видно только в debug overlay. |
| Неправильный порядок отрисовки объектов (лежаки, кусты, мебель) | Проблема sort order для полупрозрачных объектов: кусты, лежаки, вывески, провода, пар — неправильный порядок отрисовки на множестве миссий. **Исправлено (2026-04-12):** (1) bucket sort заменён на `std::stable_sort` — точная сортировка вместо 2048-bucket приближения; (2) sort_z (ключ сортировки) не записывался в 3 из 4 code paths (`POLY_add_triangle_fast`, `POLY_add_quad_fast`, near-clip) — исправлено; (3) двухфазный split (non-additive → additive) убран — одна очередь для всех sorted полигонов; (4) sort_z = average(Z) вместо max(Z) — устраняет z-fighting на mesh'ах с общей вершиной (ёлки-конусы). Расследование → [`zorder_investigation.md`](../new_game_devlog/zorder_investigation.md). |
| Газетки не отображаются + сломанные UV у листьев | На 1 миссии газетки не видны, а часть листьев рисуется с битыми UV. **Исправлено:** один баг, внесён при рефакторинге рендерера (этап 7) — газетки (rubbish, каждый 16-й слот DIRT) были ошибочно переписаны в dirt batch вместе с листьями, но батч использовал текстуру LEAF. В оригинале газетки рисовались через `POLY_add_quad(POLY_PAGE_RUBBISH)` — отдельно от листьев, со своей текстурой. UV газеток попадали на случайные участки текстуры листьев → "глючные листики". Фикс: возвращён `POLY_add_quad` как в оригинале. |
| Листья и трупы рисуются поверх огня/взрывов/света | Листья деревьев, ground debris, ограждения рисуются поверх additive эффектов (огонь, bloom фонарей, пар). **Изначальный баг пре-релизных исходников** — проявляется даже на самом раннем билде до любых наших изменений. **Исправлено:** ground leaves/rubbish переведены на deferred path + DepthWrite=true; sorted pass разделён на non-additive → additive фазы. Расследование → [`leaf_overdraw_investigation.md`](../new_game_devlog/leaf_overdraw_investigation.md). |
| Грани листвы ёлок рисуются в неправильном порядке | Target UC — листва ёлок состоит из 4 полупрозрачных граней, задние поверх передних. **Исправлено (2026-04-12):** sort_z = average(Z) вместо max(Z) + stable_sort. Подробности → [`zorder_investigation.md`](../new_game_devlog/zorder_investigation.md). |
| OpenGL: нет пара из люков (steam) | На Urban Shakedown пар из решёток люков не отображался. **Исправлено:** fog шейдер интерпретировал `specular.a = 0` как "CPU fog = 100%" → полностью заменял цвет пара на fog color. Причина: `SPRITE_draw_tex` передавал `specular = 0` (all bytes) вместо `specular = 0xFF000000` (alpha=1.0 = "no CPU fog"). Фикс: установлен `specular = 0xFF000000` во всех вызовах `SPRITE_draw_tex` с нулевым specular (draw_steam, PYRO_draw_fire, barbwire). Workaround `SetFogEnabled(false)` убран — fog для STEAM работает через table fog как в оригинале D3D6. |
| OpenGL: полоска 1px справа окна в меню и outro | В главном меню и outro — вертикальная полоска в 1 пиксель на правом краю окна (цвет очистки framebuffer). В геймплее отсутствует. В D3D-версии нет. **Исправлено (2026-04-12):** fullscreen quad функции (`gl_blit_fullscreen_texture`, screen buffer blit, `ge_blit_surface_to_backbuffer`) — наш код, не D3D6 — задавали координаты 0..w. Шейдер `tl_vert.glsl` вычитал -0.5 (D3D6 pixel center) → квад покрывал -0.5..w-0.5, правый столбец пикселей не закрашивался. Фикс: +0.5 offset к координатам вершин в трёх функциях. |
| OpenGL: артефакт рядом с иконкой выбранного оружия | При открытии списка оружия (HUD) — рядом с выбранным пистолетом виден небольшой графический артефакт. В D3D-версии артефакта нет. **Исправлено (2026-04-09):** D3D6 pixel center offset (-0.5) в `tl_vert.glsl` устранил артефакт. |
| OpenGL: артефакты-линии над буквами в меню | В меню (например текст "OPTIONS") над некоторыми буквами видна горизонтальная линия-артефакт. В D3D-версии линия тоже есть, но значительно меньше. **Исправлено (2026-04-09):** D3D6 pixel center offset (-0.5) в `tl_vert.glsl` устранил артефакты. Подробности → [`stage12_log.md`](../new_game_devlog/stage12_log.md). |
| Outro: титры с чёрным фоном букв | Шрифт титров рисовался с непрозрачным чёрным фоном вместо прозрачного. **Исправлено:** два бага OpenGL бэкенда outro: (1) outro текстуры создавались вне game texture pool → `ge_bind_texture` не находила флаг alpha → шейдер игнорировал alpha канал текстуры. Фикс: `ge_set_bound_texture_has_alpha()` вызывается в `oge_bind_texture()`. (2) При `OS_DRAW_ALPHABLEND` использовался texture blend `Modulate` (alpha=texture.a only) вместо `ModulateAlpha` (alpha=texture.a×vertex.a), как в D3D6. |
| Outro: 3D модель не отображается (x64) | После перехода на x64 3D модель в outro пропала. **Исправлено:** `IMP_binary_load`/`IMP_binary_save` делали `fread`/`fwrite` целых структур `IMP_Mesh` (10 указателей) и `IMP_Mat` (4 указателя) — sizeof на x64 не совпадает с on-disk форматом x86. Фикс: ручное чтение/запись полей с фиксированными on-disk размерами (IMP_Mesh=136, IMP_Mat=104 байт), указатели пропускаются. Аналог фикса `CRINKLE_read_bin`. |
| Дублированные inline функции в разных .cpp файлах | Оригинальный код содержал `inline` функции с одинаковым именем в разных .cpp файлах (например `screen_flip` в `game.cpp` и `playcuts.cpp`). **Исправлено:** при миграции/рефакторинге все inline функции в .cpp файлах были убраны — проверка всех 318 .cpp файлов в new_game/src/ подтвердила: ни одной inline функции в .cpp файлах не осталось. |
| x64: пропадающие стены и ящики вблизи | `CRINKLE_read_bin` читала `CRINKLE_Crinkle` (struct с указателями) через `sizeof` → 24 на x64 вместо 16 → сдвиг bptr на 8 байт → мусор в crinkle mesh данных → текстуры с bump (окна, двери, ящики) не рендерились при z < 0.3. Фикс: фиксированный on-disk size 16 байт. Удалена неиспользуемая `CRINKLE_write_bin` (level editor tool). |
| Чёрные дырки на ближнем клиппинге | Целые полигоны (пол, стены, машины, персонажи) пропадали на периферии зрения. Баг оригинального движка. **Исправлено:** причина — `POLY_add_nearclipped_triangle()` не вызывала `PolyBufAlloc()`, вершины записывались но полигоны не регистрировались. Также исправлены: distance check с пустым else в figure.cpp, bounding sphere cull per body part, деление на ноль в MultiMatrix. Подробности → [`black_holes_investigation.md`](../new_game_devlog/black_holes_investigation.md). |
| Пунктирные линии прицела | Два вида: (1) серые пунктиры показывающие что враг целится в нас огнестрельным оружием, (2) линия траектории при прицеливании гранатой (наш прицел). Есть в PC релизе. **Исправлено:** убраны оба вида. Линия гранаты под вопросом — возможно разрабы так и планировали как помощь игроку, может придётся вернуть. |

---

## Крэши (исправленные)

| Проблема | Описание |
|----------|----------|
| Крэш при загрузке Psycho Park | После прохождения лоадера игра вылетала без видимых ошибок. **Исправлено:** перестало воспроизводиться после ASan-фиксов (2026-04-06). Вероятно исправлено попутно с другими memory corruption багами. |
| Крэш при загрузке Stern Revenge | Аналогично Psycho Park — вылет после лоадера без ошибок. **Исправлено:** перестало воспроизводиться после ASan-фиксов (2026-04-06). **Обновление 2026-04-11:** отдельный крэш — `ASSERT(0)` в `EWAY_create_animal`, т.к. BAT (subtype 1) попадал в пустой case → default → ASSERT. Причина: в данных миссии 23 летучие мыши, но код создания BAT был вырезан MuckyFoot (`#ifdef UNUSED`). Решение: `BAT_create` обёрнут в feature flag `cut_bats` (OFF по умолчанию), ASSERT в default сохранён. В финальной PC-версии мышей нет. См. `FEATURE_FLAGS.md`. |
| Крэш на синематике Urban Shakedown | **Стабильно воспроизводился** на OpenGL бэкенде. **Исправлено (2026-04-02):** root cause — `ge_vb_expand` heap overflow (logsize/capacity mismatch). ASan нашёл точное место. Подробности → `stage8_urban_shakedown_crash.md`. |

---

## CRT Debug Assertions (исправленные)

| Проблема | Описание |
|----------|----------|
| CRT heap corruption (debug build) | Debug Assertion Failed в `debug_heap.cpp:904-908`: `_CrtIsValidHeapPointer(block)` и `is_block_type_valid(header->_block_use)`. **Исправлено (2026-04-02):** root cause найден через ASan — `ge_vb_expand` аллоцировал 65536 байт, но `memcpy` копировал 131072 (logsize/capacity mismatch при переиспользовании VB слота). Фикс: skip realloc если буфер уже достаточно большой. Также исправлены 5 попутных багов (ReadSquished overread, strcpy overlap, arctan off-by-one, SIN overflow, tmat use-after-scope). Подробности → `stage8_urban_shakedown_crash.md`. |

---

## Сборка (закрытые)

| Проблема | Описание |
|----------|----------|
| D3D6 бэкенд — удалён | **Удалён (2026-04-08).** Runtime невозможен на x64 (Direct3D legacy interfaces не эмулируются). Удалены: `backend_directx6/` (25 файлов), `backend_stub/`, CMake/Make targets (`configure-d3d6`, `GRAPHICS_BACKEND`, `ddraw`/`dxguid` линковка, `VERSION_D3D`), документация обновлена. OpenGL 4.1 — единственный бэкенд. |

---

## macOS (исправленные)

| Проблема | Описание |
|----------|----------|
| Очень низкий FPS (ощущение как software rendering) | Причина: alpha sort рендерил каждый полигон отдельным GL draw call (350+ вызовов за кадр). Фикс: batched alpha sort — накопление индексов + один draw call на PolyPage. Frame time с ~80ms → ~5ms. |
| DualSense: огромная задержка ввода (~1 секунда) | Причина: macOS IOKit HID бэкенд (через SDL3 hidapi) возвращает репорты из FIFO-очереди (самый старый первым). DualSense по BT шлёт ~133 репортов/сек, игра читает ~30/сек → очередь росла на ~100/сек → через секунду мы читали данные секундной давности. На Windows HID бэкенд ведёт себя иначе. Фикс: drain всей очереди за кадр, использовать последний репорт (ds_bridge.cpp `read()`). Также: dirty-check для LED/vibration output (gamepad.cpp), skip PlugAndPlay re-enumeration при подключённом устройстве (ds_bridge.cpp). |

---

## Linux (исправленные)

| Проблема | Описание |
|----------|----------|
| Case-insensitive file I/O | Linux FS case-sensitive, ресурсы на диске mixed case (`TITLE LEAVES1.TGA`), код ищет lowercase (`title leaves1.tga`). Меню без фона, миссии не грузились, outro крашился. **Исправлено:** `fopen_ci()` в `file.cpp` — рекурсивный CI-резолвинг каждого компонента пути через `opendir`/`readdir`/`strcasecmp`. Все `fopen` в проекте заменены на `fopen_ci` (файловый движок, outro, tga, env). `stat()` → `fstat()` по открытому handle. |

---

## Отсутствующий функционал (закрытые)

| Проблема | Описание |
|----------|----------|
| Видеоролики не воспроизводятся | FFmpeg (libavcodec) заменил проприетарный Bink SDK. Все 6 роликов работают: 3 интро (Eidos, logo, intro) + 3 катсцены. Видео: FFmpeg decode → GL текстура → fullscreen quad. Аудио: FFmpeg decode → OpenAL streaming. A/V sync по wall clock. Скип: клавиатура, мышь, SDL gamepad, DualSense (edge-triggered). Кросс-платформенно. Девлог: `new_game_devlog/bink_video_playback.md`. |
| Outro: bump mapping не работает | `OS_DRAW_TEX_MUL` (dual-texture multiply) не реализован в OpenGL бэкенде. 3D модель в outro отображается, но без bump mapping. **Won't fix:** side-by-side сравнение с D3D показало что разница еле заметна — видна только на полицейском значке. Не стоит отдельного шейдера. |
| Порядок отрисовки пара/гейзеров и листвы | Target UC — гейзеры с паром: (1) когда несколько гейзеров перекрывают друг друга, задние иногда рисуются поверх передних, (2) листва деревьев (ёлки) рисуется поверх гейзеров, хотя деревья позади. Тот же класс проблем с sort order полупрозрачных эффектов. **Исправлено (2026-04-12):** пофикшено вместе с z-order рефактором (stable_sort, sort_z = average(Z), исправлена запись sort_z). Расследование → [`zorder_investigation.md`](../new_game_devlog/zorder_investigation.md). |
| OpenGL: огонь на полицейском участке не как в D3D | На Urban Shakedown (геймплей) — огонь на горящем полицейском участке визуально отличался от D3D-версии. **Исправлено:** пофикшено пакетом фиксов полупрозрачных (разделение на 2 очереди non-additive → additive) + z-order рефактор (2026-04-12). |
| Resize окна в рантайме | `ge_update_display_rect` — сейчас фиксированный размер окна, нельзя менять в рантайме. **Won't fix:** resize окна заблокирован. Если вернём resize — пересмотреть этот пункт. |
