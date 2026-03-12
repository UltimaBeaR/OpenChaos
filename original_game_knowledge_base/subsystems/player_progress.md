# Прогресс игрока и сохранения — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/frontend.cpp` — save/load логика
- `fallen/Headers/Game.h` — структура Game с характеристиками
- `fallen/Source/Game.cpp` — глобальные переменные прогресса
- `fallen/Source/elev.cpp` — CRIME_RATE загрузка

---

## 1. Файл сохранения (.wag)

**Путь:** `saves/slot{N}.wag` (N = **1, 2, 3** — три слота, 1-based!)
**Нумерация:** `save_slot = menu_state.selected + 1` — первый пункт меню = слот 1, не 0.
**Формат:** бинарный, sequential write/read
**Версия текущая:** `version = 3`

### Структура файла:
```c
// FRONTEND_save_savegame() / FRONTEND_load_savegame() — frontend.cpp

char   mission_name[32];   // текущая/последняя миссия
UBYTE  complete_point;     // глобальный прогресс (0–24+)
UBYTE  version;            // = 3

// Характеристики Darci (RPG stats):
UBYTE  DarciStrength;
UBYTE  DarciConstitution;
UBYTE  DarciSkill;
UBYTE  DarciStamina;

// Характеристики Roper:
UBYTE  RoperStrength;
UBYTE  RoperConstitution;
UBYTE  RoperSkill;
UBYTE  RoperStamina;

UBYTE  DarciDeadCivWarnings;   // счётчик убийств мирных

UBYTE  mission_hierarchy[60];  // статус 60 миссий
UBYTE  best_found[50][4];      // лучшие результаты 50 миссий
```

**Итого:** ~32 + 1 + 1 + 8 + 8 + 1 + 60 + 200 ≈ 303+ байт на слот (точно зависит от формата строки в FRONTEND_SaveString)

**История версий сохранений:**
- Версия 0, 1, 2 — старые форматы (код проверяет при загрузке и пропускает недостающие поля)
- Версия 3 — текущий финальный формат
- При загрузке версий < 3 некоторые поля не читаются, используются значения по умолчанию

---

## 2. complete_point — глобальный прогресс

```c
UBYTE complete_point = 0;   // Game.cpp
```

- Увеличивается при завершении ключевых сюжетных миссий
- Диапазон: 0 → 24+ (точное максимальное значение зависит от структуры кампании)
- Влияет на:
  - Открытые миссии (branching)
  - Визуальный стиль уровней (texture_set — уровни "темнеют" по ходу игры)
  - Доступность контента

---

## 3. mission_hierarchy[60] — ветвящаяся структура миссий

```c
UBYTE mission_hierarchy[60];
// Каждый элемент = статус одной из 60 миссий
// bit 2 (значение 4) = миссия завершена
```

Обеспечивает нелинейную структуру прохождения — игрок может выбирать миссии из доступных.

---

## 4. best_found[50][4] — лучшие результаты

```c
UBYTE best_found[50][4];
// Для каждой из 50 миссий: 4 байта лучшего результата
// Содержимое: вероятно время, очки или флаги прохождения
```

---

## 5. Характеристики персонажей (RPG прокачка)

Хранятся в `struct Game` (`Game.h`) и сохраняются в .wag:

```c
struct Game {
    // ...
    UBYTE DarciStrength;      // сила Darci
    UBYTE DarciConstitution;  // телосложение
    UBYTE DarciSkill;         // навык
    UBYTE DarciStamina;       // выносливость

    UBYTE RoperStrength;      // то же для Roper
    UBYTE RoperConstitution;
    UBYTE RoperSkill;
    UBYTE RoperStamina;

    UBYTE DarciDeadCivWarnings;  // нарушения (убийства мирных)
    // ...
};
```

Прокачиваются в ходе прохождения, влияют на боевые возможности и диалоги.

---

## 6. VIOLENCE флаг

```c
SLONG VIOLENCE_ALLOWED = 1;   // Game.cpp — кровь/гор разрешены по умолчанию
```

- Булевый флаг для крови и гор-эффектов
- Может быть установлен per-mission
- В финальной версии включён по умолчанию
- **В новой версии: всегда = 1 (hardcode), без настройки.** Если когда-либо понадобится опция — добавить позже.

---

## 7. Игровые настройки (config.ini)

```ini
[Render]
max_frame_rate=30       # cap FPS
draw_distance=22        # дальность видимости
detail_shadows=1        # статические тени
detail_puddles=1        # лужи
detail_rain=1           # дождь
detail_stars=1          # звёзды

[Game]
language=text\lang_english.txt
scanner_follows=1
```

---

## 8. Что переносить

| Аспект | Подход |
|--------|--------|
| Формат .wag (версия 3) | Перенести 1:1 для совместимости с оригинальными сейвами |
| complete_point логика | Перенести 1:1 |
| mission_hierarchy[60] | Перенести 1:1 |
| best_found[50][4] | Перенести 1:1 |
| RPG характеристики 8×UBYTE | Перенести 1:1 |
| VIOLENCE флаг | Перенести, сделать настраиваемым |
| config.ini формат | Можно заменить на современный (TOML/JSON), сохранив те же параметры |
