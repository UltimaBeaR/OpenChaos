# Заметки по портированию подсистем

## Аудио (из audio.md)

**MSS32 → miniaudio:**

| MSS32 / MFX | miniaudio |
|-------------|-----------|
| `MFX_play_xyz()` | `ma_sound_set_position()` + `ma_sound_start()` |
| `MFX_update_xyz()` | `ma_sound_set_position()` |
| `MFX_set_volume()` | `ma_sound_set_volume()` |
| `MFX_stop()` | `ma_sound_stop()` |
| 3D positional | `ma_engine` с `MA_SOUND_FLAG_SPATIALIZATION` |
| Fade transitions | `ma_sound_set_volume()` с timer |

**Кандидаты:** OpenAL Soft (предпочтительнее — зрелый 3D spatializer, прямой аналог MSS32), miniaudio (проще интегрировать, но 3D базовый).
Финальный выбор — при старте Фазы 3. Подробно → `tech_and_architecture.md`.

**Переносить 1:1:**
- 14 MUSIC_MODE_* с логикой переключения и приоритетами
- 3D позиционный звук (обновление позиции каждый кадр)
- FLAGS_HAS_ATTACHED_SOUND механизм
- Ambient система (5 биомов по texture_set)
- Система диалогов (WAV + субтитры из text/)
- Параметры громкости из config.ini

**Заменить:**
- MSS32 → miniaudio
- DirectSound3D driver → кросс-платформенная 3D аудио-система

---

## Математика (из math_utils.md)

| Компонент | Подход |
|-----------|--------|
| Matrix3x3 float | **glm::mat3** |
| Quaternion float | **glm::quat** |
| Vector3 float | **glm::vec3** |
| SLERP | Встроен в glm (`glm::slerp`) |
| PSX FMatrix / integer quaternion | **Не переносить** (только PSX) |
| CMatrix33 (PSX сжатая) | **Не переносить** |
| AtanTable / SinTable lookup | **Не нужны** — libm достаточно быстр |
| Fixed-point 16.16 (GameCoord) | Сохранить для физики (1:1 воспроизведение) |
| Root() wrapper | Не нужен — использовать `std::sqrt()` |

---

## Персонажи/анимации (из characters.md)

| Аспект | Подход |
|--------|--------|
| Vertex morphing | Загружаем GameKeyFrameElement[] из .all → декомпрессируем GetCMatrix → конвертируем в `glm::mat4` (×1/128) |
| Tweening | `CQuaternion::BuildTween` = quaternion slerp → `glm::mix()` на quaternions |
| DrawTween | Реализовать аналогичный animation controller (state, blend, queue) |
| Типы персонажей | Перенести 1:1 (15 типов, те же PERSON_TYPE константы) |
| Флаги состояний | Перенести 1:1 |
| API анимаций | Перенести: `set_anim`, `tween_to_anim`, `queue_anim` |
| Смерть/KO | Перенести 1:1, ragdoll НЕ добавлять |
| Гарпун/лазание | Перенести 1:1 |
| Плавание | Не реализовывать (в оригинале нет) |
| InterruptFrame | Не реализовывать (мёртвый код) |

---

## Рендеринг (из rendering.md)

**DirectX 6 → OpenGL 4.6 / Vulkan маппинг:**

| Оригинал | OpenGL 4.6 / Vulkan |
|----------|---------------------|
| D3DTLVERTEX | Custom vertex struct + VBO |
| LPDIRECT3DTEXTURE2 | GLuint texture / VkImage |
| SetTransform() | Uniform buffers (MVP матрицы) |
| SetRenderState() | glEnable/Disable или Pipeline state |
| DrawIndexedPrimitive() | glDrawElements / vkCmdDrawIndexed |

**Требует переработки:**
- Фиксированный пайплайн → шейдеры (vertex lighting, fog)
- Crinkle система → не переносить как есть, реализовать через normal/parallax mapping
- Vertex buffer pool → staging buffers / persistent mapping

**Сохранить идеи:**
- PolyPage — хорошая абстракция
- RenderState инкапсуляция
- Bucket sort для прозрачных

**Структура вершины:**
```cpp
struct Vertex {
    float x, y, z;
    uint32_t colour;     // ARGB
    uint32_t specular;   // ARGB fog
    float u, v;
};
```

**Освещение:** можно улучшить до per-pixel; динамические тени (shadow maps) — опционально.

---

## Управление (из controls.md + psx_controls.md)

| Компонент | Перенос |
|-----------|---------|
| 18 кнопок INPUT_* | 1:1 |
| ACTION_* (52 действия) | 1:1 |
| FLAG_PERSON_* | 1:1, кроме FLAG_PERSON_BIKING |
| Double-click (200ms) | 1:1, порт: SDL_GetTicks() |
| Аналоговые стики | DirectInput → SDL3 |
| Клавиатурные коды | DirectInput → SDL3 scancodes |
| Расщепление экрана | НЕ ПЕРЕНОСИТЬ |
| FLAG_PERSON_BIKING | НЕ ПЕРЕНОСИТЬ |

**Геймпад (SDL3) — маппинг PSX кнопок:**

| PSX | SDL3 |
|-----|------|
| D-Pad | `SDL_GAMEPAD_BUTTON_DPAD_*` |
| Triangle | `SDL_GAMEPAD_BUTTON_NORTH` |
| Cross | `SDL_GAMEPAD_BUTTON_SOUTH` |
| Circle | `SDL_GAMEPAD_BUTTON_EAST` |
| Square | `SDL_GAMEPAD_BUTTON_WEST` |
| L1/R1 | `SDL_GAMEPAD_BUTTON_LEFT_SHOULDER/RIGHT_SHOULDER` |
| L2/R2 | `SDL_GAMEPAD_AXIS_LEFT_TRIGGER/RIGHT_TRIGGER` |
| Start | `SDL_GAMEPAD_BUTTON_START` |
| Select | `SDL_GAMEPAD_BUTTON_BACK` |
| Left Stick | `SDL_GAMEPAD_AXIS_LEFTX/LEFTY` |

**Default layout (на основе PSX cfg0):** левый стик = движение; Cross=JUMP, Circle=ACTION, Square=PUNCH, Triangle=KICK; L1=camera, R1=strafe, L2/R2=cam rotate. Сохранить 4 варианта раскладки как в оригинале.
Аналог PSX: порог 96/128 (75%) → для SDL3 использовать ~0.75f.

---

## Эффекты (из effects.md)

- **Ribbon** (circular buffer trails): перенести логику 1:1, рендеринг — modern API
- **Bang** (каскадные взрывы): перенести каскадную логику и тайминги, рендеринг → GPU particles
- **POW/PYRO/DIRT**: перенести логику, рендеринг заменить

---

## Физика (из physics.md)

**Критично (1:1):**
1. Точные пороги переходов: 30/60 юнитов
2. Алгоритм slide_along
3. Система face detection (`find_face_for_this_pos()`)
4. Физика подвески транспорта
5. Гравитация транспорта: -5120 юнит/тик²
6. Animation-driven падение персонажей
7. Все координатные сдвиги (`>> 8`, `>> 16`)

**Можно переосмыслить:**
- Linked-list spatial hash → quadtree/octree (но сохранить точное поведение collision detection)

**Float precision:**
- Оригинал = integer (SLONG)
- При использовании float → `-mfpmath=sse -msse2`
- Рассмотреть: сохранить integer-физику для детерминизма

---

## Форматы ресурсов

### Освещение (.lgt — из lighting_format.md)

| Компонент | Перенос |
|-----------|---------|
| Парсер .lgt | 1:1 |
| ED_Light → point light | range/color → OpenGL point light |
| NIGHT_Colour (0-63) | Делить на 64.0f для OpenGL |
| amb_d3d_colour | **НЕТ** — D3D-специфичный, пересчитать из amb_red/green/blue |
| sky_colour | → clear color или skybox tint |

### Текстуры (.txc — из texture_format.md)

**НЕ парсить .txc** (проблема с size_t). Вместо этого:
- Читать TGA из `Textures/` напрямую (stb_image)
- .txc — runtime optimization оригинала, не нужен

### Анимации (.all — из animation_format.md)

| Компонент | Перенос |
|-----------|---------|
| GameKeyFrame / GameKeyFrameElement | 1:1 (парсер) |
| DrawTween система | 1:1 |
| AnimBank / load_anim_system | 1:1 |
| Vertex morphing | 1:1 (принципиальное решение) |
| TweenStep interpolation | 1:1 |

---

## Frontend (из frontend.md)

| Компонент | Перенос |
|-----------|---------|
| STARTSCR_mission | `std::string` |
| FE_* режимы | Перенести логику, переработать визуально |
| Стек меню | 1:1 |
| whattoload[] / DONT_load | Упростить (DONT_load=0 уже) |
| VIOLENCE флаг | Перенести (для туториалов) |
| DirectDraw фонов | → OpenGL текстуры |

---

## Здания/интерьеры (из buildings_interiors.md)

- `ID_draw()` → полная замена (D3D5 → OpenGL)
- `ID_get_face_texture()` → маппинг тип-грани → UV в OpenGL
- Процедурная генерация интерьеров → перенести 1:1 (алгоритм)
- Интерьеры = те же DFacets с типами 17/19/20, рендер переключается фильтрацией
- Двери: Open поле (0–255) синхронизировано с MAV-навигацией
- STOREY_TYPE_CABLE (тип 9) = тросы/death-slide — есть в финале, переносить
- Дороги определяются текстурами (ID диапазон), не явными данными — логику определения поверхности сохранить

---

## Навигация (из navigation.md)

- MAV — неявный граф (caps вычисляются из MAV_opt на лету, рёбра не хранятся)
- MAV greedy BFS, горизонт 32 шагов + случайное смещение (+0–3) для непредсказуемости
- MAV_precalculate() при каждой загрузке уровня (тяжёлая операция)
- INSIDE2 навигация: в пре-релизе заглушки — mav_inside/stair/exit нужно дописать
- Склады: паттерн MAV_nav swap (save → redirect → MAV_do → restore)
- Взрывы стен НЕ обновляют MAV в оригинале — решить нужна ли динамическая перестройка

---

## Препроцессорные флаги (из preprocessor_flags.md)

**Безопасно удалить:** `TARGET_DC`, `PSX`, `VERSION_GLIDE`, `EDITOR`, `BUILD_PSX`, все отладочные артефакты, региональные флаги, `EIDOS`.

**Заменить аналогами:**
- `VERSION_D3D`, `MF_DD2`, `TEX_EMBED` → OpenGL/Vulkan
- `_MF_WINDOWS`, `_WINDOWS`, `_WIN32` → CMake platform detection
- `FINAL`/`DEBUG`/`NDEBUG` → концепция Debug/Release остаётся

---

## Технические решения (из cut_features.md)

| Аспект | Решение |
|--------|---------|
| PSX / Dreamcast | Не переносить |
| Редакторы | Не переносить |
| Glide / SW renderer | Не переносить |
| Miles Sound System | → miniaudio или SDL_mixer |
| DirectX 6 / DirectInput | → OpenGL + SDL3 |
| MuckyBasic VM | → не нужен (EWAY реализуется на C++) |
| EIDOS флаг | Убрать |
| VIOLENCE_ALLOWED | Всегда = 1 |
| MARKS_PRIVATE_VERSION | Не переносить |
| USE_PASSWORD (чит-коды) | Перенести концептуально |
| Debug-режим | Опционально |

---

## AI/PCOM (из ai.md)

**1:1 (критично для геймплея):**
- Все типы AI (PCOM_AI_*) и поведение
- Все состояния AI (PCOM_AI_STATE_*) и переходы
- Система восприятия (can_a_see_b)
- Комбо система и BlockingHistory
- Двухуровневая навигация MAV + NAV
- Боевые константы (HIT_TYPE, урон, дистанции)

**Можно модернизировать:** связные списки → эффективные структуры; таблицы состояний → виртуальные функции; PCOM_BENT → гибкая система характеристик.

---

## Game Objects (из game_objects.md)

**1:1:** CLASS_* иерархия, State Machine, SubState, FLAGS_*, MapWho.
**Модернизировать:** связные списки → modern containers; Union → variant/polymorphism; лимит 700 → пересмотреть.

---

## Game States (из game_states.md)

Перенести: все GS_* флаги, точный порядок per-frame обновления, GS_REPLAY (полный рестарт), DarciDeadCivWarnings, bench cooldown (1024-кадровый цикл), SMOOTH_TICK_RATIO, статистику, условия победы/поражения через EWAY, катсцены park2/Finale1.

---

## Камера (из camera.md)

Перенести FC целиком (единственная активная система). cam.cpp = мёртвый код, не переносить.
Ключевое: режимы 0-3, CAM_MORE_IN=0.75, gun-out ddist/lower, 8-step raycast collision, get-behind, toonear→first-person (0x90000), shake decay.

---

## Миссии/EWAY (из missions.md)

**1:1:** EventPoint+Data[10], все TT_*/WPT_* типы, бинарный .ucm формат, граф активации, .wag формат v3, mission_hierarchy[60], complete_point, best_found[50][4], CRIME_RATE, RPG характеристики.
**Можно улучшить:** typed parameters вместо raw Data[10], debug визуализация графа EP.
**Критично:** порядок активации EventPoints — основа сценария миссий.

---

## Прогресс/сохранения (из player_progress.md)

| Аспект | Подход |
|--------|--------|
| .wag (версия 3) | 1:1 для совместимости |
| complete_point | 1:1 (пороги 8/16/24) |
| mission_hierarchy[60] | 1:1 (биты 0/1/2) |
| best_found[50][4] | 1:1 (анти-фарм) |
| RPG характеристики | 1:1 |
| VIOLENCE | Перенести, hardcode = 1 |
| config.ini | Можно заменить на TOML/JSON |

---

## Транспорт (из vehicles.md)

1:1: 9 типов, VehicleStruct, подвеска (4 пружины), руль, двигатели, 6 зон повреждения, посадка/высадка, переезд персонажей.
**НЕ переносить:** BIKE (не в финале), DirectDraw/PSX специфика.

---

## UI (из ui.md)

| Компонент | Подход |
|-----------|--------|
| HUD | 1:1 по позициям, рендер через новый API |
| Tracking (4 врага) | 1:1 |
| Frontend меню | Переработать визуально, логика 1:1 |
| GameMenu | 1:1 |
| MenuFont эффекты | Основные (futzing, centred, shake) |
| Font2D | → TrueType (FreeType/stb_truetype) |
| Фоны | Те же TGA ресурсы |
| DirectDraw Surfaces | → текстуры OpenGL/SDL |

---

## Мир/карта (из world_map.md)

1:1: PAP_Hi (128×128), PAP_Lo/MapWho, высоты с билинейной интерполяцией, здания (Building/Storey/Wall), загрузчик .lev.
Улучшить: освещение (динамические тени).
MapElement: только LIGHT_Colour, остальное выбросить.

---

## Оружие/предметы (из weapons_items.md)

1:1: все 30 SPECIAL_*, SpecialList, ammo_packs, граната (парабола+отскок+6с), C4 (5с), подбор should_person_get_item(), защитный таймер 5 тиков.
**Не переносить:** SPECIAL_THERMODROID (нет поведения), SPECIAL_MINE (ASSERT(0)).

---

## Бой (из combat.md)

- Нет физики отбрасывания — только анимации (сохранить этот подход)
- Урон из GameFightCol.Damage в .all файлах
- Блок только против melee
- KO у MIB заблокирован (балансная особенность)

---

## Взаимодействие (из interaction_system.md)

Переносить: find_grab_face (двухпроходный поиск), cable физика, ladder механика, calc_sub_objects_position.
Cable параметры: сохранить кодирование в DFacet (или отдельная структура).
valid_grab_angle(): в оригинале отключена.
Канализация: НЕ переносить (cut feature).

---

## Малые подсистемы (из minor_systems.md)

- Trip (растяжки): 1:1 (активная геймплейная механика)
- SM (soft body): low priority, можно заменить на современный soft-body
- Command.cpp: НЕ переносить (кроме COM_PATROL_WAYPOINT если используется)
- drawxtra.cpp game logic: вынести из рендерера в game update loop

---

## Форматы ресурсов (дополнительные)

### Карта (.iam — из map_format.md)
1:1: PAP_Hi (6 б/ячейка), LoadGameThing, SuperMap (DBuilding, DFacet), InsideStorey/Staircase, texture_set.
PSX данные (versions >=25, >=26): НЕТ.

### Модели (.prm — из model_format.md)
1:1: .IMP, .MOJ, .TXC, .PRM парсеры. PrimPoint как SWORD — критично. FACE_FLAG_WALKABLE — критично.
.SEX: только исходник для редактора, не переносить.

### Аудио формат (из audio_format.md)
WAV файлы (SFX + диалоги): 1:1. 14 музыкальных режимов: 1:1. MSS32 → miniaudio/SDL.
