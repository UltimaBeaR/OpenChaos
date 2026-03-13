# Original Game Knowledge Base

Детальная документация по оригинальному исходному коду Urban Chaos (1999).
Написана для понимания игры с целью создания новой версии.

**Источник:** `original_game/` (READ ONLY, пре-релизный snapshot)
**Ресурсы:** `original_game/fallen/Debug/` (Steam-версия, рабочие)

## С чего начать (порядок чтения)

1. [SESSION_START.md](SESSION_START.md) — быстрый ориентир по всей KB (карта подсистем, связи, аннотированные файлы)
2. [QUICK_FACTS.md](QUICK_FACTS.md) — все ключевые числа, формулы и константы одним файлом
3. [overview.md](overview.md) — архитектура, entry point, системы, файловая структура
4. [analysis_scope.md](analysis_scope.md) — что анализируем / не анализируем и почему
5. [cut_features.md](cut_features.md) — вырезанные и отключённые фичи в оригинальном коде
6. Нужная подсистема из таблицы ниже

## Подсистемы

| Подсистема | Файл |
|---|---|
| Физика и коллизии | [subsystems/physics.md](subsystems/physics.md) |
| Физика — вода, гравитация, HyperMatter, WMOVE | [subsystems/physics_details.md](subsystems/physics_details.md) |
| Игровые объекты (Thing) | [subsystems/game_objects.md](subsystems/game_objects.md) |
| Игровые объекты — Barrel, Platform, Chopper | [subsystems/game_objects_details.md](subsystems/game_objects_details.md) |
| AI и поведение NPC (PCOM ядро) | [subsystems/ai.md](subsystems/ai.md) |
| AI — структуры данных Person | [subsystems/ai_structures.md](subsystems/ai_structures.md) |
| AI — детальные поведения (MIB/Bane/Snipe/Zone) | [subsystems/ai_behaviors.md](subsystems/ai_behaviors.md) |
| Навигация NPC (MAV/NAV) | [subsystems/navigation.md](subsystems/navigation.md) |
| Персонажи и анимации | [subsystems/characters.md](subsystems/characters.md) |
| Персонажи — детали (типы, CMatrix33, Darci physics) | [subsystems/characters_details.md](subsystems/characters_details.md) |
| Управление и ввод | [subsystems/controls.md](subsystems/controls.md) |
| Транспорт | [subsystems/vehicles.md](subsystems/vehicles.md) |
| Боевая система | [subsystems/combat.md](subsystems/combat.md) |
| Рендеринг и освещение | [subsystems/rendering.md](subsystems/rendering.md) |
| Рендеринг: Tom's Engine, меши, морфинг | [subsystems/rendering_mesh.md](subsystems/rendering_mesh.md) |
| Рендеринг: NIGHT освещение, Crinkle | [subsystems/rendering_lighting.md](subsystems/rendering_lighting.md) |
| Игровой мир и карта | [subsystems/world_map.md](subsystems/world_map.md) |
| Здания и интерьеры | [subsystems/buildings_interiors.md](subsystems/buildings_interiors.md) |
| Здания — id.cpp рендеринг, WARE склады | [subsystems/buildings_interiors_details.md](subsystems/buildings_interiors_details.md) |
| Загрузка уровней | [subsystems/level_loading.md](subsystems/level_loading.md) |
| Система миссий (EWAY runtime) | [subsystems/missions.md](subsystems/missions.md) |
| Миссии — WPT трансляция, CRIME_RATE, binary .ucm | [subsystems/missions_implementation.md](subsystems/missions_implementation.md) |
| Прогресс игрока и сохранения | [subsystems/player_progress.md](subsystems/player_progress.md) |
| Оружие и предметы | [subsystems/weapons_items.md](subsystems/weapons_items.md) |
| Оружие — гранаты, автоприцел, DIRT банки | [subsystems/weapons_items_details.md](subsystems/weapons_items_details.md) |
| Звук | [subsystems/audio.md](subsystems/audio.md) |
| UI и фронтенд (HUD, шрифты, диалоги) | [subsystems/ui.md](subsystems/ui.md) |
| Frontend (меню, pipeline запуска миссии) | [subsystems/frontend.md](subsystems/frontend.md) |
| Визуальные эффекты | [subsystems/effects.md](subsystems/effects.md) |
| Взрывы/Пиро/Мусор (POW/PYRO/DIRT) | [subsystems/effects_pyro_pow_dirt.md](subsystems/effects_pyro_pow_dirt.md) |
| Камера | [subsystems/camera.md](subsystems/camera.md) |
| Состояния игры (GS_*, пауза, смерть) | [subsystems/game_states.md](subsystems/game_states.md) |
| Состояния игрока (STATE_*, SUB_STATE_*) | [subsystems/player_states.md](subsystems/player_states.md) |
| WayWind (редакторный UI, не игра) | [subsystems/waywind.md](subsystems/waywind.md) |
| Математика и утилиты | [subsystems/math_utils.md](subsystems/math_utils.md) |
| Взаимодействие (grab/ladder/cable) | [subsystems/interaction_system.md](subsystems/interaction_system.md) |
| Малые подсистемы (вода, растяжки, SM, шары, следы, искры, Command) | [subsystems/minor_systems.md](subsystems/minor_systems.md) |

## Форматы ресурсов

| Формат | Файл |
|---|---|
| Обзор + структура папок | [resource_formats/README.md](resource_formats/README.md) |
| Карты (.pam/.pi/.pp) | [resource_formats/map_format.md](resource_formats/map_format.md) |
| 3D модели (.IMP/.MOJ) | [resource_formats/model_format.md](resource_formats/model_format.md) |
| Анимации (в .IMP/.MOJ) | [resource_formats/animation_format.md](resource_formats/animation_format.md) |
| Текстуры (.TGA/.TXC) | [resource_formats/texture_format.md](resource_formats/texture_format.md) |
| Освещение (.lgt) | [resource_formats/lighting_format.md](resource_formats/lighting_format.md) |
| Звук (.WAV) | [resource_formats/audio_format.md](resource_formats/audio_format.md) |
