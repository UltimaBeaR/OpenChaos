# Original Game Knowledge Base

Детальная документация по оригинальному исходному коду Urban Chaos (1999).
Написана для понимания игры с целью создания новой версии.

**Источник:** `original_game/` (READ ONLY)
**Ресурсы:** `original_game/fallen/Debug/` (Steam-версия, рабочие)

## Статус анализа

| Подсистема | Статус | Файл |
|---|---|---|
| Обзор проекта и структура | ✅ Базово | [00_overview.md](00_overview.md) |
| Форматы ресурсов | 🔲 Не начато | [resource_formats/README.md](resource_formats/README.md) |
| Физика и коллизии | 🔲 Не начато | [subsystems/physics.md](subsystems/physics.md) |
| AI и поведение NPC | 🔲 Не начато | [subsystems/ai.md](subsystems/ai.md) |
| Рендеринг | 🔲 Не начато | [subsystems/rendering.md](subsystems/rendering.md) |
| Игровые объекты (Thing) | 🔲 Не начато | [subsystems/game_objects.md](subsystems/game_objects.md) |
| Игровой мир и карта | 🔲 Не начато | [subsystems/world_map.md](subsystems/world_map.md) |
| Персонажи и анимации | 🔲 Не начато | [subsystems/characters.md](subsystems/characters.md) |
| Транспорт | 🔲 Не начато | [subsystems/vehicles.md](subsystems/vehicles.md) |
| Оружие и предметы | 🔲 Не начато | [subsystems/weapons_items.md](subsystems/weapons_items.md) |
| Система миссий | 🔲 Не начато | [subsystems/missions.md](subsystems/missions.md) |
| Скриптовый язык MuckyBasic | 🔲 Не начато | [subsystems/muckybasic.md](subsystems/muckybasic.md) |
| Звук | 🔲 Не начато | [subsystems/audio.md](subsystems/audio.md) |
| UI и фронтенд | 🔲 Не начато | [subsystems/ui.md](subsystems/ui.md) |
| Препроцессорные флаги | 🔲 Не начато | [preprocessor_flags.md](preprocessor_flags.md) |
| Математика и утилиты | 🔲 Не начато | [subsystems/math_utils.md](subsystems/math_utils.md) |

## Структура папки

```
original_game_knowledge_base/
├── README.md                    — этот файл, индекс
├── 00_overview.md               — общий обзор архитектуры
├── preprocessor_flags.md        — какие флаги что делают, нужны ли для PC release
├── resource_formats/
│   ├── README.md                — обзор всех форматов ресурсов
│   ├── map_format.md            — формат карт (.pap и др.)
│   ├── model_format.md          — формат 3D моделей (clumps)
│   ├── animation_format.md      — формат анимаций
│   ├── texture_format.md        — формат текстур
│   └── audio_format.md          — формат звука
└── subsystems/
    ├── physics.md
    ├── ai.md
    ├── rendering.md
    ├── game_objects.md
    ├── world_map.md
    ├── characters.md
    ├── vehicles.md
    ├── weapons_items.md
    ├── missions.md
    ├── muckybasic.md
    ├── audio.md
    ├── ui.md
    └── math_utils.md
```

## Ключевые файлы оригинала (быстрый доступ)

### Основные
- `fallen/Source/Game.cpp` — главный игровой цикл (~53K строк)
- `fallen/Source/Mission.cpp` — система миссий (~43K строк)
- `fallen/Source/Person.cpp` — AI персонажей (~22K строк)
- `fallen/Source/Special.cpp` — оружие/предметы (~41K строк)
- `fallen/Headers/Game.h` — главные структуры данных
- `fallen/Headers/Thing.h` — базовый класс всех игровых объектов

### Рендеринг
- `fallen/DDLibrary/DDManager.cpp` — DirectDraw/Direct3D (~49K строк)
- `fallen/DDLibrary/GDisplay.cpp` — управление дисплеем (~51K строк)
- `fallen/DDLibrary/DIManager.cpp` — DirectInput (~62K строк)
- `fallen/DDEngine/aeng.cpp` — движок анимации (~16K строк)

### Физика/мир
- `fallen/Source/collide.cpp` — коллизии (~11K строк)
- `fallen/Source/building.cpp` — генерация зданий (~11K строк)
- `fallen/Headers/pap.h` — формат карты (heightmap)
