# Original Game Knowledge Base

Детальная документация по оригинальному исходному коду Urban Chaos (1999).
Написана для понимания игры с целью создания новой версии.

**Источник:** `original_game/` (READ ONLY)
**Ресурсы:** `original_game/fallen/Debug/` (Steam-версия, рабочие)

## Статус анализа

| Подсистема | Статус | Файл |
|---|---|---|
| Обзор проекта и структура | ✅ Готово | [00_overview.md](00_overview.md) |
| Препроцессорные флаги | ✅ Готово | [preprocessor_flags.md](preprocessor_flags.md) |
| Физика и коллизии | ✅ Готово | [subsystems/physics.md](subsystems/physics.md) |
| Игровые объекты (Thing) | ✅ Готово | [subsystems/game_objects.md](subsystems/game_objects.md) |
| AI и поведение NPC | ✅ Готово | [subsystems/ai.md](subsystems/ai.md) |
| Скриптовый язык MuckyBasic | ✅ Готово | [subsystems/muckybasic.md](subsystems/muckybasic.md) |
| Форматы ресурсов | ✅ Готово | [resource_formats/README.md](resource_formats/README.md) |
| Рендеринг | ✅ Готово | [subsystems/rendering.md](subsystems/rendering.md) |
| Система миссий | ✅ Готово | [subsystems/missions.md](subsystems/missions.md) |
| Оружие и предметы | ✅ Готово | [subsystems/weapons_items.md](subsystems/weapons_items.md) |
| Звук | ✅ Готово | [subsystems/audio.md](subsystems/audio.md) |
| Игровой мир и карта | ✅ Готово | [subsystems/world_map.md](subsystems/world_map.md) |
| Персонажи и анимации | ✅ Готово | [subsystems/characters.md](subsystems/characters.md) |
| Транспорт | ✅ Готово | [subsystems/vehicles.md](subsystems/vehicles.md) |
| UI и фронтенд | ✅ Готово | [subsystems/ui.md](subsystems/ui.md) |
| Математика и утилиты | ✅ Готово | [subsystems/math_utils.md](subsystems/math_utils.md) |

## Структура папки

```
original_game_knowledge_base/
├── README.md                    — этот файл, индекс
├── 00_overview.md               — общий обзор архитектуры (entry point, системы, зависимости)
├── preprocessor_flags.md        — классификация всех #ifdef флагов, что нужно для PC release
├── resource_formats/
│   ├── README.md                — обзор всех форматов ресурсов
│   ├── map_format.md            — формат карт (.pap и др.)
│   ├── model_format.md          — формат 3D моделей (clumps)
│   ├── animation_format.md      — формат анимаций
│   ├── texture_format.md        — формат текстур
│   └── audio_format.md          — формат звука
└── subsystems/
    ├── physics.md               — ✅ коллизии, движение, fixed-point math, транспортная физика
    ├── ai.md                    — ✅ PCOM, типы AI, состояния, навигация (NAV+MAV), бой
    ├── game_objects.md          — ✅ Thing система, классы, State Machine, MapWho
    ├── muckybasic.md            — ✅ скриптовый движок, VM, опкоды, синтаксис, рекомендации замены
    ├── rendering.md             — ✅ DDEngine, poly, mesh, texture, light
    ├── missions.md              — ✅ EventPoint граф, TT_*/WPT_* триггеры, структуры миссий
    ├── weapons_items.md         — ✅ SPECIAL_* типы, оружие, инвентарь, гранаты
    ├── audio.md                 — ✅ Miles Sound System, 3D аудио, 14 музыкальных режимов
    ├── world_map.md             — ✅ карта (PAP_Hi/Lo), MapWho, здания, освещение
    ├── characters.md            — ✅ 15 типов персонажей, vertex morphing, DrawTween
    ├── vehicles.md              — ✅ 9 типов машин, физика, BIKE (2 мотоцикла)
    ├── ui.md                    — ✅ HUD, меню, шрифты, диалоги, frontend
    └── math_utils.md            — ✅ Matrix3x3, Quaternion, SLERP, arctan/sqrt tables
