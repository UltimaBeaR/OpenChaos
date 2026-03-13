# Миссии — реализация и форматы

**Связанный файл:** [missions.md](missions.md) (EWAY runtime: структуры, условия, действия, режимы)

---

## 10. Важно: .ucm — НЕ MuckyBasic

**`.ucm` файлы = EWAY Mission Data**, не скомпилированные MuckyBasic скрипты.

Вся логика миссий описывается через EventPoint массив (до 512 вэйпойнтов) с условиями EWAY_COND_* и действиями WaypointType. Никаких вызовов MuckyBasic VM нет.

MuckyBasic — отдельная изолированная система (`MuckyBasic/` папка), не интегрированная с игрой. Вне скоупа анализа, см. [analysis_scope.md](../analysis_scope.md).

---

## 11. Трансляция WPT_* → EWAY_DO_* (elev.cpp)

> **Важно:** WPT_* значения (Mission.h) и EWAY_DO_* значения (eway.h) — **разные нумерации!**

- `.ucm` файл хранит **WPT_* значения** (редакторский формат)
- При загрузке уровня в `elev.cpp` (строки 748–1654) происходит трансляция в **EWAY_DO_*** (рантайм формат)
- `EWAY_create()` получает уже EWAY_DO_* значения

### Ключевые маппинги:
```
WPT_END_GAME_WIN    → EWAY_DO_MISSION_COMPLETE
WPT_END_GAME_LOSE   → EWAY_DO_MISSION_FAIL
WPT_GROUP_LIFE      → EWAY_DO_GROUP_LIFE
WPT_GROUP_DEATH     → EWAY_DO_GROUP_DEATH
WPT_INCREMENT       → EWAY_DO_INCREASE_COUNTER  (subtype = Data[1] = counter index)
WPT_RESET_COUNTER   → EWAY_DO_RESET_COUNTER     (subtype = Data[0], 0 = все счётчики)
WPT_CONVERSATION    → EWAY_DO_CONVERSATION или EWAY_DO_AMBIENT_CONV (если Data[3]!=0)
WPT_CUT_SCENE       → EWAY_DO_CUTSCENE
WPT_ACTIVATE_PRIM   → EWAY_DO_CONTROL_DOOR или EWAY_DO_ELECTRIFY_FENCE или ... (depends on prim type)
```

### НЕ реализованные типы (пре-релиз):
- **WPT_GOTHERE_DOTHIS (39)** — отсутствует в switch → hits `default: ASSERT(0)`. NPC scripting **не реализован**.
- **WPT_BONUS_POINTS (32)** → маппится в EWAY_DO_MESSAGE из-за `if(0)` блока:
  ```c
  if (0) {  // "Bonus points are just messages nowadays."
      ed.type = EWAY_DO_OBJECTIVE;  // ← МЁРТВЫЙ КОД
  } else {
      ed.type = EWAY_DO_MESSAGE;    // ← всегда
  }
  ```
  WPT_BONUS_POINTS = просто текстовое сообщение, очки не начисляются.

### Трансляция OT_* → EWAY_STAY_*:
```
OT_NONE / OT_ACTIVE   → EWAY_STAY_ALWAYS
OT_ACTIVE_WHILE       → EWAY_STAY_WHILE
OT_ACTIVE_TIME        → EWAY_STAY_TIME  (es.arg = AfterTimer)
OT_ACTIVE_DIE         → EWAY_STAY_DIE
```

---

## 11a. CRIME_RATE — детали обновления

### Загрузка:
- Загружается из `.iam` файла: `CRIME_RATE = junk[0]`
- Если 0 → устанавливается 50 (дефолт)

### CRIME_RATE_SCORE_MUL (pre-game, в EWAY_created_last_waypoint):
```c
for_guilty = num_guilty_people * 4;              // 4% за каждого guilty NPC
SATURATE(for_guilty, 0, CRIME_RATE >> 1);        // cap at 50% of CRIME_RATE
CRIME_RATE_SCORE_MUL = (CRIME_RATE - for_guilty) << 8 / total_objective_points;
// Если total_points == 0 → CRIME_RATE_SCORE_MUL = 0
```

### Динамическое обновление в игре (Person.cpp):
| Событие | Изменение |
|---------|-----------|
| Убийство guilty NPC | -2 |
| Убийство wandering civ (PCOM_AI_CIV + PCOM_MOVE_WANDER) | +5 |
| Арест guilty NPC | -4 |
| EWAY_DO_OBJECTIVE (мёртвый код, не вызывается) | -percent |

Все изменения: `SATURATE(CRIME_RATE, 0, 100)`.

### Спец. счётчик: EWAY_counter[7]
- Инкрементируется (до max 255) при каждом убийстве PERSON_COP

---

## 11b. Win/Lose flow и GROUP логика

Полный Win/Lose flow → см. [game_states.md](game_states.md) раздел 5.

### Статистика миссии (Person.cpp):
```c
stat_killed_thug, stat_killed_innocent      // убийства guilty/non-guilty
stat_arrested_thug, stat_arrested_innocent  // аресты
stat_count_bonus                            // бонусные предметы
stat_game_time                              // мс от start до set_stats()
```

### GROUP_LIFE / GROUP_DEATH детали:
- **GROUP_LIFE**: clears EWAY_FLAG_DEAD на всех WP с тем же `colour` AND `group`; иммунны сами WP GROUP_LIFE/GROUP_DEATH
- **GROUP_DEATH**: sets EWAY_FLAG_DEAD на всех WP с тем же `colour` AND `group`; иммунны сами WP GROUP_LIFE/GROUP_DEATH

---

## 12. Глобальный прогресс и сохранение

Формат .wag файла, complete_point, mission_hierarchy, характеристики персонажей → см. [player_progress.md](player_progress.md).

### CRIME_RATE в .ucm:
```c
// elev.cpp: CRIME_RATE = junk[0]; — загружается из файла уровня
// Если 0 → устанавливается 50 (дефолт)
TT_CRIME_RATE_ABOVE   // сработает если CRIME_RATE превысит порог
TT_CRIME_RATE_BELOW   // сработает если CRIME_RATE упадёт ниже порога
```

---

## 13. Точный бинарный формат .ucm файла

`.ucm` = EWAY Mission Data. Загружается `ELEV_load_level()` в `elev.cpp`.

### Заголовок миссии

```
[4 байта]  SLONG version
[4 байта]  SLONG flag               — bit0=USED, bit1=SHOW_CRIMERATE, bit2=CARS_WITH_ROAD_PRIMS
[_MAX_PATH] char BriefName[]        — путь к файлу брифинга (260 байт)
[_MAX_PATH] char LightMapName[]     — путь к .lgt файлу освещения
[_MAX_PATH] char MapName[]          — имя .iam файла карты (без пути)
[_MAX_PATH] char MissionName[]      — имя миссии
[_MAX_PATH] char CitSezMapName[]    — путь к файлу "cit-sez"
[2 байта]  UWORD MapIndex           — игнорируется при загрузке
[2 байта]  UWORD UsedEPoints        — игнорируется при загрузке
[2 байта]  UWORD FreeEPoints        — игнорируется при загрузке
[1 байт]   UBYTE CrimeRate          — CRIME_RATE (0-100; если 0 → дефолт 50)
[1 байт]   UBYTE FakeCivs           — количество "фоновых" гражданских
```

Итого заголовок: **4+4+260×5+2+2+2+1+1 = 1316 байт**

### EventPoint записи

Цикл `for (i = 0; i < MAX_EVENTPOINTS; i++)`:
```
[14 байт] EventPoint header:
  UBYTE Colour, Group, WaypointType, Used    (4 байта)
  UBYTE TriggeredBy, OnTrigger, Direction, Flags (4 байта)
  UWORD EPRef, EPRefBool, AfterTimer         (6 байт)
[60 байт] EventPoint data:
  SLONG Data[10]                             (40 байт)
  SLONG Radius, X, Y, Z                     (16 байт)
  UWORD Next, Prev                           (4 байт)
```

**EventPoint = ровно 74 байта** (14 + 60).
`MAX_EVENTPOINTS = 512` → **37 888 байт** EventPoint секция.

Только записи с `Used != 0` обрабатываются.

### После EventPoints

```
if (version > 5):
  FileSeek(+254)        — пропуск SkillLevels[254], не читается в runtime

FileRead(1 byte)        — BOREDOM_RATE (min 4 enforced)

if (version > 8):
  FileRead(1 byte)      — FAKE_CARS (CarsRate) — трафик NPC
  FileRead(1 byte)      — MUSIC_WORLD (1-based)
```

После этого — секция сообщений (строки брифингов/диалогов):
- `if version < 8`: только сообщения
- `if version >= 8`: сообщения + катсцены
- Каждая строка: если `version > 4` — SLONG длина + N байт текста; иначе фикс. _MAX_PATH байт

Итого полный .ucm: **≈1316 + 37888 + ~260 ≈ 39 460 байт** (плюс строки сообщений).

### Путь к .iam карте

```c
sprintf(fname_map, "%s%s", DATA_DIR, ucm.MapName);
// → открывает data/MAPNAME.iam через load_game_map()
```
