# Вырезанные и отключённые фичи в оригинальном коде

Факты о статусе фич в пре-релизной кодовой базе.
Решения о том, что анализировать → [analysis_scope.md](analysis_scope.md).

---

## Статус кодовой базы

`original_game/` — **пре-релизный snapshot**, не финальный релиз.
Из README Mike Diskett:
> *"This is a snapshot of the source code recently retreived from an old SourceSafe backup"*
> *"the source data used by the editor/Engine is likely somewhat different to the data used by the final disk image"*

- Некоторые фичи закомментированы здесь, но **работают в финальной PS1/PC игре**
- Баги и артефакты рендеринга в этой версии могут быть исправлены в финале
- **Нельзя считать "закомментированный код = вырезанная фича"**
- Эталон: Steam PC ресурсы + воспоминания пользователя о PS1 финале
- При необходимости — YouTube лонгплей или PS1 эмулятор (пользователь проверит конкретный уровень)

---

## Фичи, подтверждённые Mike Diskett как "never shipped"

> *"Theres code for features that never shipped like a motorbike and a grappling hook with rope physics"*
> *"once upon a time the building all had procedural internals, also sewer systems.."*

| Фича | Код в базе | Статус |
|------|-----------|--------|
| Мотоцикл (BIKE) | bike.cpp, ~15 файлов | Не вошёл в финальную игру |
| Кошка-гарпун | hook.cpp (256-точечная верёвка) | Не вошёл в финальную игру |
| Канализация | sewer.cpp, ns.cpp | Не вошла в финальную игру |
| Процедурные интерьеры | building.cpp (частично) | Не вошли в финальную игру. ⚠️ Не путать с вручную размещёнными интерьерами уровней (полицейский участок и т.д.) — они в данных уровня и работают |

---

## Другие вырезанные/отключённые фичи

| Фича | Статус в кодовой базе |
|------|----------------------|
| Мультиплеер / split-screen | Спланирован (CTF mode, MAX_PLAYERS=2), в финале отключён |
| GS_REPLAY (demo/replay режим) | Есть в коде, в игру не вошло |
| Zipwire / тросы | Закомментирован в пре-релизе — **артефакт пре-релиза, есть в финальной игре** (подтверждено: первый уровень, тренировочная полоса) |
| Симуляция ткани (cloth.cpp) | `CLOTH_process()` — тело функции в `#if 0`. Verlet-интеграция, пружинные связи. Структуры и API есть, логика отключена |
| Indoor navigation AI (inside2.cpp) | `INSIDE2_mav_nav_calc()` — ~400 строк в `#if 0`. Навигация NPC внутри зданий |
| Heartbeat HUD (panel.cpp) | `PANEL_do_heartbeat()` — ~260 строк в `#if 0`. Монитор сердечного ритма. Заменён обычной шкалой HP (подтверждено TCRF) |
| 3D компас (panel.cpp) | `PANEL_draw_compass()` + `PANEL_draw_compass_angle()` — ~100 строк в `#if 0`. HUD компас |
| Портреты врагов (overlay.cpp) | `track_enemy()` — тело в `#ifdef OLD_POO`. Старая HUD-система: портреты лиц + HP-полоска в углу экрана. Заменена floating HP bar над головой (PANEL_draw_local_health) |
| Цифры урона (DAMAGE_TEXT) | `#ifdef DAMAGE_TEXT` блоки в aeng.cpp. Плавающие числа урона над персонажами в 3D. В финале убраны (подтверждено TCRF) |
| TCP/IP консоль (console.cpp) | ~215 строк в `#if 0 / #ifndef FINAL`. Удалённая отладочная консоль |
| 3D шрифты (Font3D) | font3d.h/font3d.cpp — весь класс в `#if 0`. Использовался в briefing, pause, startscr — все вызовы мёртвые |
| Aureal A3D звук | `USE_A3D` в Sound.h, A3DManager.cpp. 3D-аудио Aureal. Компания обанкротилась в 2000, технология мертва |
| Force Feedback | FFManager.h/FFManager.cpp — не реализован (пустые файлы, только скелет) |
| Ray система | ray.cpp — ~1400 строк в `#if 0` ("Intel C compiler barfs"). RAY_ функции нигде не вызываются |
| ENV система | Env.cpp — всё содержимое (ENV_load и др.) в `#if 0` |
| Attract mode (DDEngine) | DDEngine/Source/Attract.cpp — режим демонстрации, вырезан |
| Шарики (Balloon.cpp) | Код полный, но `load_prim_object(PRIM_OBJ_BALLOON)` закомментировано в ob.cpp. Пользователь не видел шариков в PC лонгплее |
| Старая камера (cam.cpp) | Весь файл в `#ifdef DOG_POO` — никогда не компилировался. Заменена FC (Final Camera) в fc.cpp |
| Старая AI команда (Command.cpp) | Legacy AI-командная система. Заменена PCOM в финале. Не в vcxproj |
| Software renderer (sw.cpp/tom.cpp) | Включал tom.cpp 3 раза для генерации вариантов. Не в vcxproj, мёртвый |
| Thug old AI (Thug.cpp) | `fn_thug_normal()` — тело в `#if 0`. Старая waypoint AI для бандитов |

---

## Неиспользуемые объекты (файлы есть, в игре недоступны)

- **Bat** (`anim001.all`) — есть AI, нет текстур; не спаунится (только в демо)
- **Gargoyle** (`anim002.all`, `gargoyle1.all`) — работает, не используется в миссиях
- **Tank** (`prim099`), **Plane** (`prim388`) — модели без нативных текстур

---

## Особенности прототипа vs финал (из TCRF)

- Прототип: монитор сердечного ритма + цветная рамка урона → финал: обычная шкала HP
- Прототип: цифры урона над врагами → убраны в финале
- Прототип: neck snap из-за спины → финал: обычный удар по шее
- Прототип: treasure items → финал: полицейские значки
- Прототип: мультиплеер в главном меню → финал: убран
