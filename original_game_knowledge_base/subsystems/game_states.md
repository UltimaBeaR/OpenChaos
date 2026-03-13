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

---

## 10. Что переносить

- Все GS_* флаги с той же семантикой
- Точный порядок per-frame обновления (пункты 1-19 выше)
- Логику GS_REPLAY (полный рестарт без чекпоинтов)
- DarciDeadCivWarnings поведение (персистируемый счётчик, экраны предупреждений)
- Bench health cooldown: 1024-кадровый цикл
- SMOOTH_TICK_RATIO (4-кадровое среднее для машин)
- Статистику: killed/arrested counts, crime rate
- Условия победы/поражения (определяются через EWAY, не хардкодом)
- Катсцены привязаны к конкретным файлам: park2.ucm, Finale1.ucm
