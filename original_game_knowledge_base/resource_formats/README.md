# Форматы ресурсов — Urban Chaos

Ресурсы: `original_game/fallen/Debug/` (Steam-версия, рабочая).
Загрузка: `fallen/Source/io.cpp`, `fallen/Source/Level.cpp`, `fallen/Headers/io.h`

## Структура папки ресурсов

```
Debug/
├── levels/      — скрипты уровней (.ucm — скомпилированный MuckyBasic)
├── Meshes/      — 3D модели (.SEX исходники, .IMP/.MOJ скомпилированные)
├── Textures/    — текстуры (.TGA, 32-bit RGBA)
├── clumps/      — архивы текстур уровней (.TXC)
├── data/sfx/    — звуки WAV: Ambience/, Footsteps/, General/, music01-08/
├── data/Lighting/ — файлы освещения (.lgt)
├── talk2/       — диалоги (WAV)
├── text/        — строки UI (TXT)
├── bink/        — Bink-видео (.bik)
├── stars/       — текстуры неба
└── config.ini   — конфиг игры
```

## Детальная документация по форматам

| Формат | Файл |
|--------|------|
| Карты уровней (.pam/.pi/.pp) | [map_format.md](map_format.md) |
| 3D модели (.IMP, .MOJ, .SEX, .TXC) | [model_format.md](model_format.md) |
| Анимации (встроены в .IMP/.MOJ) | [animation_format.md](animation_format.md) |
| Текстуры (.TGA, .TXC) | [texture_format.md](texture_format.md) |
| Звук (.WAV, музыкальные режимы) | [audio_format.md](audio_format.md) |
| Скрипты (.ucm) | [../subsystems/muckybasic.md](../subsystems/muckybasic.md) |
| Конфиг (config.ini) | [../subsystems/player_progress.md](../subsystems/player_progress.md) |

## Приоритет загрузчиков в новой версии

| Приоритет | Формат | Сложность |
|-----------|--------|-----------|
| 1 | Текстуры (.TGA) | Низкая — стандартный формат |
| 2 | Текстовые строки | Низкая |
| 3 | Конфиг (config.ini) | Низкая |
| 4 | Карты уровней (.pam) | Высокая |
| 5 | 3D модели (.IMP, .MOJ) | Средняя |
| 6 | Звук (.WAV) | Низкая |
| 7 | Скрипты (.ucm) | Очень высокая |
| 8 | Анимации (в .MOJ/.IMP) | Средняя |
