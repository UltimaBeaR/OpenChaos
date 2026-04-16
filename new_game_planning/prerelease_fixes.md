# Пре-релизные баги оригинала — выполненные фиксы

Пре-релизная кодовая база содержала баги которых нет в финальной игре.
Фиксы применялись на **Этапе 5**.

Незафикшенные баги и другие проблемы → [`known_issues_and_bugs.md`](known_issues_and_bugs.md)

---

## Пофикшенные баги

| Баг | Файл | Описание | Фикс |
|-----|------|----------|------|
| ✅ Порядок отрисовки персонаж/машина/стена | polypage.cpp | `DrawIndPrimMM` использовал линейное z (`view_z * P`) вместо обратного (`1 - P/view_z`), несовместимого с POLY software path. Персонажи/полы через hardware path всегда «выигрывали» z-тест. | Фикс z-формулы в `DrawIndPrimMM` (одна строка). Детали → `new_game_devlog/stage5_bug_zbuffer_mismatch.md` |
| ✅ Персонаж виден сквозь стену | polypage.cpp | Та же корневая причина что и "Порядок отрисовки" — z-buffer mismatch в `DrawIndPrimMM`. | Исправлено тем же фиксом (OC-ZBUF-MISMATCH). |
| ✅ Банки с колой не видны | aeng.cpp | `AENG_draw_dirt()` вызывается **после** финального `POLY_frame_draw()` — банки/гильзы через `MESH_draw_poly()` попадают в deferred PolyPage буфер, но flush уже прошёл. В Dreamcast-порте (оригинал `aeng.cpp:13844`) flush после dirt есть — баг исправлен в более поздних билдах. | Добавлен `POLY_frame_draw(UC_TRUE, UC_TRUE)` после `AENG_draw_dirt()` в `AENG_draw_city()`. Заодно починены гильзы (`DIRT_TYPE_BRASS`). |
| ✅ Листья/растительность пропадают | aeng.cpp | `POLY_set_local_rotation_none()` был пустым макросом `{}` (оригинал: `#ifndef TARGET_DC`). `MESH_draw_poly` для банок/гильз внутри `AENG_draw_dirt()` вызывает `POLY_set_local_rotation()` → `SetTransform(WORLD, матрица_объекта)`, но сброс не происходил. `DrawIndexedPrimitive` для листьев использовал world transform от последней банки → листья рисовались в неправильных координатах, мерцали группами (по батчам ~64 шт) при повороте камеры. | Удалён пустой макрос `#define POLY_set_local_rotation_none() {}` в aeng.cpp — теперь используется реальная функция из poly.cpp, которая сбрасывает D3D world transform. |
| ✅ Бампмеппинг мигает | facet.cpp | `(GAME_TURN & 0x20)` — оригинальная оптимизация для железа 1999 года: crinkle (бампмеппинг) рисовался только каждые 32 такта из 64, чтобы сэкономить производительность. Визуально — мигание ~раз в секунду. | Убрано условие `(GAME_TURN & 0x20)` — crinkle теперь рисуется каждый кадр. |
| ✅ Зависание на старте (spin-loop) | game.cpp | `lock_frame_rate()` использовал `SLONG` для `GetTickCount()`. При аптайме >24.8 дней значение переполняется в отрицательное → `timet = tick2 - tick1` отрицательный → условие выхода из цикла никогда не срабатывает. Баг проявляется с Windows Fast Startup. | `SLONG` → `DWORD` для tick1/tick2/timet. Пофикшено на Этапе 4. PieroZ (`67bb6c6`) фиксит через `GetTickCount64`, мы через unsigned 32-bit. Детали → `new_game_devlog/stage4_bug_tick_overflow.md` |
| ✅ Знак разворота постоянно на экране | panel.cpp | `PANEL_sign_time` (SLONG) = 0 по умолчанию. При аптайме >24.8 дней: `dtime = GetTickCount() - 0` переполняется в отрицательное → `dtime < 3000` всегда TRUE → знак рисуется постоянно. Та же корневая причина (GetTickCount + SLONG). | `SLONG` → `DWORD`. Пофикшено на Этапе 4. Та же корневая причина, тот же коммит PieroZ. Детали → `new_game_devlog/stage4_bug_tick_overflow.md` |
| ✅ `MSG_draw` глобальный buffer overflow (ASan) | message.cpp | `MSG_MAX_MESSAGES = 1000`, валидные индексы 0..999. `MSG_draw` делал `if (pos > MSG_MAX_MESSAGES)` — при `pos == 1000` условие НЕ срабатывало, читался `MSG_message[1000]` (один элемент за концом массива). Оригинальный off-by-one, тихо дремал 26 лет. Триггер: F9 → `bangunsnotgames` → Ctrl (включить debug-оверлей) → поворот камеры (`KB_PPLUS/PMINUS` сдвигает `draw_message_offset`) → `pos` доходит до 1000 → краш под ASan. | `>` → `>=` в двух местах в `MSG_draw` (обёртка `pos` и wrap после инкремента). Обнаружено 2026-04-17 при тестовом запуске под ASan с debug-оверлеем. |

---

Пользователь может принести фиксы из чужих веток —
проверить корректность по знанию оригинальной логики перед применением.
