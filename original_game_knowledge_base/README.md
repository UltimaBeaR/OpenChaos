# Original Game Knowledge Base

Детальная документация по оригинальному исходному коду Urban Chaos (1999).
Написана для понимания игры с целью создания новой версии.

**Источник:** `original_game/` (READ ONLY, пре-релизный snapshot)
**Ресурсы:** `original_game/fallen/Debug/` (Steam-версия, рабочие)

## С чего начать (порядок чтения)

1. [overview.md](overview.md) — архитектура, entry point, системы, файловая структура
2. [cut_features.md](cut_features.md) — что вырезано, что переносить, решения
3. [preprocessor_flags.md](preprocessor_flags.md) — флаги компилятора, что нужно для PC
4. Нужная подсистема из таблицы ниже

## Подсистемы

| Подсистема | Файл |
|---|---|
| Физика и коллизии | [subsystems/physics.md](subsystems/physics.md) |
| Игровые объекты (Thing) | [subsystems/game_objects.md](subsystems/game_objects.md) |
| AI и поведение NPC | [subsystems/ai.md](subsystems/ai.md) |
| Навигация NPC (MAV/NAV) | [subsystems/navigation.md](subsystems/navigation.md) |
| Персонажи и анимации | [subsystems/characters.md](subsystems/characters.md) |
| Управление и ввод | [subsystems/controls.md](subsystems/controls.md) |
| Транспорт | [subsystems/vehicles.md](subsystems/vehicles.md) |
| Боевая система | [subsystems/combat.md](subsystems/combat.md) |
| Рендеринг и освещение | [subsystems/rendering.md](subsystems/rendering.md) |
| Рендеринг: Tom's Engine, меши, морфинг | [subsystems/rendering_mesh.md](subsystems/rendering_mesh.md) |
| Рендеринг: NIGHT освещение, Crinkle | [subsystems/rendering_lighting.md](subsystems/rendering_lighting.md) |
| Игровой мир и карта | [subsystems/world_map.md](subsystems/world_map.md) |
| Здания и интерьеры | [subsystems/buildings_interiors.md](subsystems/buildings_interiors.md) |
| Загрузка уровней | [subsystems/level_loading.md](subsystems/level_loading.md) |
| Система миссий (EWAY) | [subsystems/missions.md](subsystems/missions.md) |
| Прогресс игрока и сохранения | [subsystems/player_progress.md](subsystems/player_progress.md) |
| Оружие и предметы | [subsystems/weapons_items.md](subsystems/weapons_items.md) |
| Звук | [subsystems/audio.md](subsystems/audio.md) |
| UI и фронтенд (HUD, шрифты, диалоги) | [subsystems/ui.md](subsystems/ui.md) |
| Frontend (меню, pipeline запуска миссии) | [subsystems/frontend.md](subsystems/frontend.md) |
| Визуальные эффекты | [subsystems/effects.md](subsystems/effects.md) |
| Камера | [subsystems/camera.md](subsystems/camera.md) |
| Состояния игры (GS_*, пауза, смерть) | [subsystems/game_states.md](subsystems/game_states.md) |
| Состояния игрока (STATE_*, SUB_STATE_*) | [subsystems/player_states.md](subsystems/player_states.md) |
| WayWind (редакторный UI, не игра) | [subsystems/waywind.md](subsystems/waywind.md) |
| MuckyBasic (не используется в игре) | [subsystems/muckybasic.md](subsystems/muckybasic.md) |
| Математика и утилиты | [subsystems/math_utils.md](subsystems/math_utils.md) |

## Форматы ресурсов

| Формат | Файл |
|---|---|
| Обзор + структура папок | [resource_formats/README.md](resource_formats/README.md) |
| Карты (.pam/.pi/.pp) | [resource_formats/map_format.md](resource_formats/map_format.md) |
| 3D модели (.IMP/.MOJ) | [resource_formats/model_format.md](resource_formats/model_format.md) |
| Анимации (в .IMP/.MOJ) | [resource_formats/animation_format.md](resource_formats/animation_format.md) |
| Текстуры (.TGA/.TXC) | [resource_formats/texture_format.md](resource_formats/texture_format.md) |
| Звук (.WAV) | [resource_formats/audio_format.md](resource_formats/audio_format.md) |
