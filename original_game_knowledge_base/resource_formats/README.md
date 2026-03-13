# Форматы ресурсов — Urban Chaos

Ресурсы: `original_game/fallen/Debug/` (Steam-версия, рабочая).
Загрузка: `fallen/Source/io.cpp`, `fallen/Source/Level.cpp`, `fallen/Headers/io.h`

## Структура папки ресурсов

```
Debug/
├── levels/        — скрипты миссий (.ucm — EWAY Mission Data, НЕ MuckyBasic!)
├── data/          — карты уровней (.iam), анимации (.all), освещение (Lighting/*.lgt)
├── data/Lighting/ — файлы освещения (.lgt) — точечные огни + ambient уровня
├── Meshes/        — 3D модели (.SEX исходники, .IMP/.MOJ скомпилированные)
├── Textures/      — текстуры (.TGA, 32-bit RGBA)
├── clumps/        — архивы текстур уровней (.TXC)
├── server/prims/  — статические примы мира (.prm — фонари, баки, урны, ~265 типов)
├── data/sfx/      — звуки WAV: Ambience/, Footsteps/, General/, music01-08/
├── talk2/         — диалоги (WAV)
├── text/          — строки UI (TXT)
├── bink/          — Bink-видео (.bik)
├── stars/         — текстуры неба
└── config.ini     — конфиг игры
```

## Детальная документация по форматам

| Формат | Файл |
|--------|------|
| Карты уровней (.iam) | [map_format.md](map_format.md) |
| 3D модели (.IMP, .MOJ, .SEX, .TXC) + примы (.prm) | [model_format.md](model_format.md) |
| Анимации (.all, встроены в модели) | [animation_format.md](animation_format.md) |
| Текстуры (.TGA, .TXC) | [texture_format.md](texture_format.md) |
| Звук (.WAV, музыкальные режимы) | [audio_format.md](audio_format.md) |
| Освещение (.lgt) | [lighting_format.md](lighting_format.md) |
| Скрипты миссий (.ucm — EventPoint формат) | [../subsystems/missions.md](../subsystems/missions.md) |
| Конфиг (config.ini) | [../subsystems/player_progress.md](../subsystems/player_progress.md) |

## Приоритет загрузчиков в новой версии

| Приоритет | Формат | Сложность |
|-----------|--------|-----------|
| 1 | Текстуры (.TGA) | Низкая — стандартный формат |
| 2 | Текстовые строки | Низкая |
| 3 | Конфиг (config.ini) | Низкая |
| 4 | Карты уровней (.iam) | Высокая — PAP_Hi + SuperMap |
| 5 | 3D модели (.IMP, .MOJ, .prm) | Средняя |
| 6 | Анимации (.all) | Средняя — pointer fixup |
| 7 | Звук (.WAV) | Низкая |
| 8 | Освещение (.lgt) | Низкая |
| 9 | Скрипты миссий (.ucm) | Высокая — EventPoint → EWAY трансляция |
