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

## 2. Главный цикл

Из `Game.cpp`:

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

**Игровой цикл (game_loop) продолжается пока:**
```c
while (SHELL_ACTIVE && (GAME_STATE & (GS_PLAY_GAME | GS_LEVEL_LOST | GS_LEVEL_WON)))
```

---

## 3. Переходы состояний

### Запуск → игра

```
game_startup()
    → Инициализация дисплея, ввода, звука
    → PC/Editor: GAME_STATE = GS_PLAY_GAME
    → Frontend: GAME_STATE = GS_ATTRACT_MODE (загрузка меню)

game_init()
    → Загрузка миссии + создание всех объектов
```

### Победа / поражение

**Победа:**
- Триггер `WPT_END_GAME_WIN` в EWAY → `GAME_STATE = GS_LEVEL_WON`
- Музыка: `MUSIC_mode(MUSIC_MODE_VICTORY)`
- Финальная миссия (`Finale1.ucm`) → выход из игры

**Поражение:**
- Триггер `WPT_END_GAME_LOSE` или здоровье ≤ 0 → `GAME_STATE = GS_LEVEL_LOST`
- Музыка:
  ```c
  if (player->Health > 0)
      MUSIC_mode(MUSIC_MODE_CHAOS);    // Провал без гибели
  else
      MUSIC_mode(MUSIC_MODE_FAIL);     // Гибель игрока
  ```

### Повтор миссии (GS_REPLAY)

```c
if (hardware_input_replay()) {
    #ifdef PSX
        GAME_STATE = GS_LEVEL_LOST;   // На PSX → возврат в меню
    #else
        GAME_STATE = GS_REPLAY;        // На PC → перезапуск миссии
    #endif
}
```

**ВАЖНО: Чекпоинтов нет.** При `GS_REPLAY` вся миссия инициализируется с нуля:
- Все объекты пересоздаются
- Все event points сбрасываются в начальное состояние
- Игрок спавнится на стартовой позиции из файла миссии

Миссии могут содержать `WPT_AUTOSAVE` точки (Mission.h строка 41), но это дополнительный механизм сохранения, зависящий от конкретной миссии.

---

## 4. Пауза (GF_PAUSED)

Флаг `GF_PAUSED` (Game.h строка 186, бит 2) управляет паузой.

Меню паузы (Game.cpp строки 463–477):
- 2 пункта: **"CONTINUE GAME"** и **"EXIT"**
- Клавиши:
  ```c
  PAUSED_KEY_START   (1 << 0)
  PAUSED_KEY_UP      (1 << 1)
  PAUSED_KEY_DOWN    (1 << 2)
  PAUSED_KEY_SELECT  (1 << 3)
  ```

---

## 5. Условия провала миссии

Из Game.cpp строки 2276–2346:

```
Миссия проваливается если:
  1. Здоровье игрока ≤ 0 → переход в STATE_DEAD
  2. EWAY выполняет WPT_END_GAME_LOSE
  3. Конкретный NPC-цель убит (определяется в EWAY)
  4. CrimeRate превышает порог (если включён флаг миссии)
  5. Истёк таймер (WPT_COUNT_UP_TIMER в EWAY)
```

---

## 6. Счёт и статистика

Поля из Game.h строки 304–310:

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

## 7. Demo timeout (отключён в релизе)

В коде есть закомментированный таймаут демо:
```c
#define TIMEOUT_DEMO 0   // При значении > 0 — принудительный выход через N секунд
```

В финальном релизе отключён.

---

## 8. Что переносить

- Все GS_* флаги с той же семантикой
- Логику GS_REPLAY (полный рестарт, не чекпоинт)
- Паузу с двумя пунктами меню
- Статистику: killed/arrested counts, crime rate
- Условия победы/поражения (определяются через EWAY, не хардкодом)
