# Лог Этапа 4.1 — Удаление мёртвого кода

План и правила: [`new_game_planning/stage4.md`](../new_game_planning/stage4.md) → секция 4.1

---

## Шаг 1, итерация 1 — Удаление целых файлов (2026-03-24)

Анализ файлов по названиям + сверка с KB + подтверждение пользователем.
Удалено **25 файлов**, скомпилировано, проверено в рантайме.

### Удалённые файлы

| Группа | Файлы | Причина |
|--------|-------|---------|
| `assets/sound_id.cpp` | 1 | Пустой файл, не в CMakeLists |
| `actors/animals/lead.h`, `lead_globals.h` | 2 | Неиспользуемые хедеры, нет .cpp |
| `world/map/qedit.*` | 4 | Редактор карт, `QEDIT_loop()` нигде не вызывается |
| `actors/items/dike.*` | 4 | Вырезанная физика мотоцикла (BIKE), вызовы только в закомментированном коде |
| `actors/animals/canid.*` | 4 | Собаки — dispatch закомментирован в оригинале, нет внешних ссылок |
| `engine/io/drive.*` | 4 | Функции путей к CD-ROM. Вызовы в 5 файлах заменены на `".\\"` |
| `assets/bink_client.*` | 2 | Bink video. `PlayQuickMovie()` оставлен как пустой stub в display.cpp |
| `engine/net/*` | 4 | Сетевой код (все функции были пустыми стабами). Вызовы убраны, `CNET_configure` заглушен |

### Правки в вызывающем коде

- **drive:** `missions/main.cpp` — убран `LocateCDROM()`. `texture.cpp`, `mfx.cpp`, `eway.cpp`, `frontend.cpp` — пути заменены на `".\\"`
- **bink:** `wind_procs.cpp` — убран `BinkMessage()`. `display.cpp` — `PlayQuickMovie` → пустой stub, убраны FMV defines и `mirror` переменная
- **net:** `game.cpp` — убран `NET_kill()`. `attract.cpp` — убран `NET_init()`. `cnet.cpp` — `CNET_configure()` → stub (return FALSE). `thing.cpp` — убрана сетевая ветка в `do_packets()`
- **canid:** `animal_globals.h` — убран extern `CANID_state_function[]`. `animal_globals.cpp` — убрана запись из `ANIMAL_functions[]`
- **qedit:** `game.cpp` — убраны includes
- **CMakeLists.txt:** убраны все удалённые .cpp + дубликат bat.cpp/bat_globals.cpp (был дважды)

### Проверено но НЕ удалено (оказалось в использовании)

bat (летучая мышь — 7+ файлов ссылаются), balloon (loading screens, cutscenes), truetype (рендеринг текста),
chopper (полноценный вертолёт с AI и рендерингом), snipe (режим прицела), attract (система loading screens),
furn.h (struct Furniture встроена в Thing), startscr (коды действий меню), cloth.h (симуляция ткани в controls.cpp)

