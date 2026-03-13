# Прогресс игрока и сохранения — Urban Chaos

**Ключевые файлы:**
- `fallen/Source/frontend.cpp` — save/load логика, mission_hierarchy, best_found
- `fallen/Headers/Game.h` — структура `struct Game` с характеристиками
- `fallen/Source/Game.cpp` — глобальные переменные прогресса

---

## 1. Файл сохранения (.wag)

**Путь:** `saves/slot{N}.wag` (N = **1, 2, 3** — три слота, 1-based!)
**Нумерация:** `save_slot = menu_state.selected + 1` — первый пункт меню = слот 1, не 0.
**Формат:** бинарный, sequential write/read
**Версия текущая:** `version = 3`

### Точный бинарный layout:

```
mission_name : variable-length string + CRLF (0x0D 0x0A)
              — FRONTEND_SaveString: strlen(txt) байт + 2 байта CRLF
              — FRONTEND_LoadString: читает до байта 0x0A (LF), переменная длина
complete_point : UBYTE (1 байт)
version        : UBYTE (1 байт) = 3  ← "strange place to have version, Historical Reasons"

--- если version >= 1 (добавлено в v1) ---
DarciStrength      : UBYTE (1 байт)
DarciConstitution  : UBYTE (1 байт)
DarciSkill         : UBYTE (1 байт)
DarciStamina       : UBYTE (1 байт)
RoperStrength      : UBYTE (1 байт)
RoperConstitution  : UBYTE (1 байт)
RoperSkill         : UBYTE (1 байт)
RoperStamina       : UBYTE (1 байт)
DarciDeadCivWarnings : UBYTE (1 байт)   — счётчик убийств мирных (0-255)

--- если version >= 2 (добавлено в v2) ---
mission_hierarchy[60] : UBYTE[60] (60 байт)

--- если version >= 3 (добавлено в v3) ---
best_found[50][4] : UBYTE[50][4] (200 байт)
```

**Итого** (version 3): len(mission_name) + 2 + 271 байт (фиксированная часть от complete_point).

**История версий:**
- **v0** — только mission_name string + complete_point + version=0 (stats/hierarchy/best_found = defaults)
- **v1** — добавлены stats (9 байт характеристик)
- **v2** — добавлен mission_hierarchy[60] (60 байт)
- **v3** — добавлен best_found[50][4] (200 байт) — финальный формат

---

## 2. complete_point — глобальный прогресс

```c
UBYTE complete_point = 0;   // Game.cpp — глобальная переменная
```

- Увеличивается при завершении ключевых сюжетных миссий
- **Пороги texture_set** (тема уровней темнеет по ходу игры, frontend.cpp:1711):
  ```c
  if (complete_point<24) newtheme=2;
  if (complete_point<16) newtheme=1;
  if (complete_point<8)  newtheme=0;
  // иначе newtheme = 3 (самая тёмная тема)
  ```
- **Практический максимум:** 40 (чит-код `KB_ASTERISK/complete_point=40`)
- Влияет на доступные миссии (через `FRONTEND_MissionHierarchy`)
- Если `complete_point >= 40` — специальное условие в миссионном дереве

---

## 3. mission_hierarchy[60] — ветвящаяся структура миссий

```c
UBYTE mission_hierarchy[60];  // глобальная, frontend.cpp:493
// Каждый элемент = статус одной из 60 возможных миссий (по ObjID)
```

**Биты (frontend.cpp:1676, 3134, 3149):**
- **bit 0 (1)** = миссия существует/разблокирована
- **bit 1 (2)** = миссия завершена (complete)
- **bit 2 (4)** = миссия доступна/ожидает (available/waiting)

**Важные моменты:**
- `mission_hierarchy[1] = 3` — корневая миссия (1=exists + 2=complete), старт (frontend.cpp:1760)
- `!complete_point` → `ZeroMemory(mission_hierarchy, 60)` при загрузке новой игры (frontend.cpp:4156)
- После завершения миссии: `mission_hierarchy[mission_launch] |= 2` (set complete)
- "waiting" (bit 4) вычисляется динамически при отображении меню миссий

---

## 4. best_found[50][4] — лучшие приросты статов

```c
UBYTE best_found[50][4];  // frontend.cpp:1505
// best_found[mission_id][stat_index]
// Инициализация: memset(&best_found[0][0], 50*4, 0)  ⚠️ баг: memset(ptr, size, value=0) — OK, но аргументы перепутаны местами
```

**Индексы статов (frontend.cpp:4248-4281):**
```
[0] = Constitution gain (прирост Constitution за эту миссию)
[1] = Strength gain
[2] = Stamina gain
[3] = Skill gain
```

**Логика** (при завершении миссии):
- `found = current_stat - saved_stat` (прирост за прохождение)
- Если `found > best_found[mission][i]`:
  - `the_game.DarciConstitution += found - best_found[mission][i]` (бонус = улучшение рекорда)
  - `best_found[mission][i] = found` (обновить рекорд)
- **Смысл:** повторное прохождение миссии даёт только РАЗНИЦУ с лучшим результатом — нельзя бесконечно фармить прокачку одной миссией.

---

## 5. Характеристики персонажей (RPG прокачка)

Хранятся в `struct Game` (`Game.h`) и сохраняются в .wag:

```c
struct Game {
    UBYTE DarciStrength;      // сила Darci
    UBYTE DarciConstitution;  // телосложение
    UBYTE DarciSkill;         // навык (Reflex в UI)
    UBYTE DarciStamina;       // выносливость

    UBYTE RoperStrength;      // то же для Roper
    UBYTE RoperConstitution;
    UBYTE RoperSkill;
    UBYTE RoperStamina;

    UBYTE DarciDeadCivWarnings;  // нарушения (убийства мирных)
};
```

**Прокачка:**
- Начальные значения выставляются при старте (frontend.cpp:4369-4374): все = `stat_up` (начальное значение)
- Растут при завершении миссий через `best_found` механику
- Roper прокачивается отдельно (только Strength, frontend.cpp:4297)
- Влияют на боевые возможности и диалоги

---

## 6. VIOLENCE флаг

```c
SLONG VIOLENCE_ALLOWED = 1;   // Game.cpp — кровь/гор разрешены по умолчанию
```

- Булевый флаг для крови и gore-эффектов
- В финальной версии включён по умолчанию


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

