# Game State Machine — Urban Chaos

**Ключевые файлы:**
- `fallen/Headers/Game.h` — GS_* константы, GF_* флаги
- `fallen/Source/Game.cpp` — главный цикл, переходы между состояниями

---

## 1. Глобальные состояния игры (GS_*)

`GAME_STATE` — глобальная переменная с битовыми флагами (Game.h строки 171–181):

```c
#define GS_ATTRACT_MODE    (1 << 0)   // Главное меню + демо-режим
#define GS_PLAY_GAME       (1 << 1)   // Активный геймплей
#define GS_CONFIGURE_NET   (1 << 2)   // Настройка сети (Dreamcast, не нужен)
#define GS_LEVEL_WON       (1 << 3)   // Миссия выполнена
#define GS_LEVEL_LOST      (1 << 4)   // Миссия провалена / игрок арестован
#define GS_GAME_WON        (1 << 5)   // Вся кампания завершена
#define GS_RECORD          (1 << 6)   // Запись демо-повтора
#define GS_PLAYBACK        (1 << 7)   // Воспроизведение демо
#define GS_REPLAY          (1 << 8)   // Перезапуск текущей миссии
#define GS_EDITOR          (1 << 16)  // Редактор уровней (только для разработки)
```

---

## 2. Главный внешний цикл

```c
game() {
    game_startup();
    while (SHELL_ACTIVE && GAME_STATE) {
        if (GAME_STATE & GS_ATTRACT_MODE)  game_attract_mode();
        if (GAME_STATE & GS_PLAY_GAME)     game_loop();
        if (GAME_STATE & GS_EDITOR)        editor_loop();       // НЕ НУЖЕН
        if (GAME_STATE & GS_CONFIGURE_NET) CNET_configure();    // НЕ НУЖЕН
    }
    game_shutdown();
}
```

**Игровой inner loop (game_loop) продолжается пока:**
```c
while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON)))
```

---

## 3. Порядок обновления за кадр (PC, game_loop)

```
[Начало кадра]
1. GAMEMENU_process()
   → Возвращает GAMEMENU_DO_NOTHING / DO_RESTART / DO_CHOOSE_NEW_MISSION / DO_NEXT_LEVEL
   → Если != 0: PANEL_fadeout_start() (затемнение)
   → После PANEL_fadeout_finished():
       DO_RESTART         → GS_REPLAY
       DO_CHOOSE_NEW_MISSION → GS_LEVEL_LOST (выход в меню)
       DO_NEXT_LEVEL      → GS_LEVEL_WON (чит/skip)
       Любой вариант      → break из inner loop

2. GS_LEVEL_WON проверка
   → Finale1.ucm → break немедленно (не ждать ввода)

3. EWAY_tutorial_string обработка
   → Пока != NULL: игра заморожена (should_i_process_game = FALSE)
   → counter < 64*40: кнопка ускоряет показ
   → counter >= 64*40: кнопка закрывает сообщение

4. demo_timeout(0) — мёртвый код (TIMEOUT_DEMO=0)

5. special_keys() — отладочные клавиши (single-step и т.д.)

6. process_controls()
   → Пропускается если GS_LEVEL_LOST | GS_LEVEL_WON | tutorial_string

7. check_pows() — power-up спавн

8. if (should_i_process_game()):
   → process_things(1)          — обновление всех Things (StateFn + PCOM)
   → PARTICLE_Run()             — частицы
   → OB_process()               — интерактивные объекты
   → TRIP_process()             — tripwires
   → DOOR_process()             — двери
   → EWAY_process()             — миссионная логика
   → SPARK_show_electric_fences()
   → RIBBON_process()
   → DIRT_process()
   → ProcessGrenades()
   → WMOVE_draw()               — движущиеся платформы
   → BALLOON_process()
   → MAP_process()
   → POW_process()
   → FC_process()               — обновление камеры

9. PUDDLE_process(), DRIP_process() — всегда (даже при паузе)

10. draw_screen()               — рендеринг 3D

11. OVERLAY_handle()            — HUD (здоровье, оружие, радар, тексты)

12. CONSOLE_draw()              — debug тексты (только если не на паузе)

13. GAMEMENU_draw()             — отрисовка меню паузы

14. PANEL_fadeout_draw()        — эффект затемнения при выходе/переходе

15. screen_flip()               — показ буфера

16. lock_frame_rate(30)         — spin-loop ожидание до 30 FPS (из config.ini)

17. handle_sfx()                — обновление звука (движущиеся SFX)

18. GAME_TURN++                 — глобальный счётчик кадров

19. Bench cooldown check:
    if ((GAME_TURN & 0x3ff) == 314):
        GAME_FLAGS &= ~GF_DISABLE_BENCH_HEALTH
    → каждые 1024 кадра (~34с при 30fps) — разрешает лечение на скамейке
```

**Примечание:** `should_i_process_game()` возвращает FALSE когда GF_PAUSED или показывается tutorial.

---

## 4. Пауза (GF_PAUSED)

Флаг `GF_PAUSED` (Game.h строка 186, бит 2) управляет паузой.

**PC:** Меню паузы через GAMEMENU (gamemenu.cpp) — 2+ пункта с анимацией, затемнением.

**PSX:** `PAUSE_handler()` — простой обработчик кнопки Start. Возвращает TRUE = выход из игры.

---

## 5. Победа/поражение — поток

### Во время игры (inner loop)

**Победа:**
- Триггер `WPT_END_GAME_WIN` в EWAY → `GAME_STATE = GS_LEVEL_WON`
- `MUSIC_mode(MUSIC_MODE_GAMEWON)`

**Поражение:**
- Триггер `WPT_END_GAME_LOSE` или здоровье ≤ 0 → `GAME_STATE = GS_LEVEL_LOST`
- Если `player->Health > 0`: `MUSIC_mode(MUSIC_MODE_CHAOS)` (провал без гибели)
- Если `player->Health <= 0`: `MUSIC_mode(MUSIC_MODE_GAMELOST)` (гибель)

### GAMEMENU WON/LOST меню (gamemenu.cpp)

**Структуры меню:**
```c
PAUSE = {X_GAME_PAUSED,  X_RESUME_LEVEL, X_RESTART_LEVEL, X_ABANDON_GAME}
WON   = {X_LEVEL_COMPLETE}          // ← только заголовок, items[1..7] = NULL!
LOST  = {X_LEVEL_LOST, X_RESTART_LEVEL, X_ABANDON_GAME}
SURE  = {X_ARE_YOU_SURE, X_OKAY, X_CANCEL}   // подтверждение Abandon
```

**WON-меню:** нет выбираемых пунктов. Нажатие ENTER/SPACE на NULL-пункте → `GAMEMENU_DO_NEXT_LEVEL`. ESC → тоже `DO_NEXT_LEVEL`.

**LOST-меню:** RESTART_LEVEL → `DO_RESTART`; ABANDON_GAME → SURE → OKAY → `DO_CHOOSE_NEW_MISSION`.

**Таймер показа:** `GAMEMENU_wait` инкрементируется на 64/кадр (пока не играет MFX_QUICK speech); порог `64*20*2 = 2560` ≈ 40 кадров = ~1.3 сек → только тогда показывается меню. DC: дополнительная блокировка ввода ещё 2.5 сек (`64*20*4 = 5120`).

**Выход из inner loop (PC):**
```
exit_game_loop = GAMEMENU_process()   // != 0 → fadeout start
→ PANEL_fadeout_finished():
    DO_RESTART            → GAME_STATE = GS_REPLAY; break
    DO_CHOOSE_NEW_MISSION → GAME_STATE = GS_LEVEL_LOST; break
    DO_NEXT_LEVEL         → GAME_STATE = GS_LEVEL_WON; break
```

### После inner loop (PC)

```
if (GS_LEVEL_WON):
    if "park2.ucm"    → RunCutscene(1)   // MIB intro
    if "Finale1.ucm"  → RunCutscene(3) + OS_hack()  // финал + credits

DarciDeadCivWarnings check (если RedMarks > 1):
    0 → предупреждение 1 (X_CIVSDEAD_WARNING_1)
    1 → предупреждение 2 (X_CIVSDEAD_WARNING_2)
    2 → предупреждение 3 (X_CIVSDEAD_WARNING_3)
    3 → GAME_STATE = GS_LEVEL_LOST + сообщение 4 (X_CIVSDEAD_WARNING_4)
    DarciDeadCivWarnings += 1 после показа (персистирует между миссиями!)

switch(GAME_STATE):
    GS_REPLAY     → GAME_STATE = GS_PLAY_GAME|GS_REPLAY; goto round_again (полный рестарт)
    GS_LEVEL_WON  → FRONTEND_level_won()
    GS_LEVEL_LOST → FRONTEND_level_lost()
    0             → выход
```

### FRONTEND_level_won() — детали (frontend.cpp:4284)

```
1. mission_hierarchy[mission_launch] |= 2          // пометить как выполненную
2. if (!cheated):                                   // g_bPunishMePleaseICheatedOnThisLevel
       Для каждого стата (Constitution/Strength/Stamina/Skill):
           found = Player->Stat - the_game.DarciStat  // дельта за миссию
           if found > best_found[mission_launch][i]:   // анти-фарм!
               the_game.DarciStat += (found - best_found[..][i])
               best_found[mission_launch][i] = found   // новый рекорд
3. FRONTEND_kibble_init()
4. menu_state сброшен (mode=-1)
5. if complete_point < mission_launch: complete_point = mission_launch
6. FRONTEND_MissionHierarchy(MISSION_SCRIPT)       // пересчитать дерево доступности
7. FRONTEND_mode(FE_SAVESCREEN)                    // показать экран сохранения
8. m_bGoIntoSaveScreen = TRUE                      // флаг для FRONTEND_init()
```

**Примечание:** Roper stats обновляются в else-ветке (DC: ASSERT(FALSE)) — на DC не поддерживается.

### FRONTEND_level_lost() — детали (frontend.cpp:4273)

```
1. mission_launch = previous_mission_launch    // восстановить (до запуска был сохранён)
2. FRONTEND_kibble_init()
3. menu_state сброшен (mode=-1)
4. FRONTEND_mode(FE_MAINMENU)                 // сразу в главное меню
```

### FRONTEND_init() после выигрыша (frontend.cpp:4244)

После вызова `FRONTEND_level_won()` → `FRONTEND_init()` инициализирует фронтенд:
```
if m_bGoIntoSaveScreen:
    FRONTEND_mode(FE_SAVESCREEN)     // показать сохранение
    m_bGoIntoSaveScreen = FALSE
else:
    FRONTEND_mode(FE_MAINMENU)

if mission_launch != 0:
    return                           // ранний выход — уже настроено
```

### ScoresDraw() — содержимое экрана победы (Attract.cpp:702)

Вызывается из `GAMEMENU_draw()` при `GAMEMENU_MENU_TYPE_WON`.

**Статистика (для Darci/Roper):**
- Убито (X_WON_KILLED): `stat_killed_thug` — счётчик из Person.cpp (ThugRasta/Grey/Red, MIB1/2/3)
- Арестовано (X_WON_ARRESTED): `stat_arrested_thug`
- На свободе (X_WON_AT_LARGE): живые Thug/MIB Types (сканирует все Things)
- Бонусов найдено (X_WON_BONUS_FOUND): `stat_count_bonus` (SPECIAL_TREASURE подобраны)
- Бонусов пропущено (X_WON_BONUS_MISSED): живые SPECIAL_TREASURE в Things
- Время (X_WON_TIMETAKEN): `h:mm:ss` из `stat_game_time = GetTickCount() - stat_start_time`

**Mucky Times (X_WON_MUCKYTIME):**
- Хардкодированный список ~35 миссий с рекордами разработчиков (имена: Mark Rose, Dave, Marie…)
- Два варианта: `#if 0` (старые PC времена), активный = DC времена (значительно медленнее)
- Если ваше время < mucky_time И нет читов → генерируется код: `hash = (i+1)*(m+1)*(s+1)*3141 % 12345 + 0x9a2f`; на DC: `hash ^= 0xffff`
- Формат кода: `: CODE <XXXX> ` (hex)
- Мёртвый план: отправить код TomF по email для верификации спидрана

**Позиции на экране:** текст-слева (X=10), значения-справа (X=300); двойной рендер (+2px чёрный shadow)

### Чекпоинты и GS_REPLAY

**Чекпоинтов нет.** При `GS_REPLAY`:
- `goto round_again` → повторный вход в `game_init()`
- Все объекты пересоздаются с нуля
- Все Event Points сбрасываются в начальное состояние
- Игрок спавнится на стартовой позиции из файла миссии

---

## 6. DarciDeadCivWarnings — детали

- `the_game.DarciDeadCivWarnings` — поле Game структуры, персистирует между миссиями
- Проверяется только для `PERSON_DARCI` (основной персонаж)
- `Player->RedMarks > 1` — порог: больше одного убитого мирного
- **Экран:** `deadcivs.tga` фон + текст из XLAT
- **Счётчик 3:** `GAME_STATE = GS_LEVEL_LOST` — уровень помечается проваленным!

---

## 7. Таймирование кадра (TICK_RATIO / SmoothTicks)

- `TICK_RATIO` — масштаб скорости на основе реального времени кадра
- `SMOOTH_TICK_RATIO` — 4-кадровое скользящее среднее из TICK_RATIO (SmoothTicks в Game.cpp):
  - кольцевой буфер из 4 значений
  - первые 3 кадра: возвращает raw_ticks напрямую
  - с 4-го кадра: sum >> 2 (среднее)
  - Используется для движения машин (чтобы не было рывков)
- `lock_frame_rate(env_frame_rate)`: spin-loop busy-wait; `max_frame_rate` из config.ini, дефолт 30

---

## 8. Счёт и статистика

Поля из Game.h:

```c
Scores[16]               // Очки по игрокам
CrimeRate                // Текущий уровень преступности (0–100%)
CrimeRateScoreMul        // Множитель очков от crime rate
stat_killed_thug         // Убито бандитов
stat_killed_innocent     // Убито гражданских
stat_arrested_thug       // Арестовано бандитов
stat_arrested_innocent   // Ошибочных арестов
```

---

## 9. Demo timeout (отключён)

```c
#define TIMEOUT_DEMO 0   // При значении > 0 — принудительный выход через N секунд
```
`demo_timeout()` = мёртвый код в финале.

