# Этап 12 — Доработки к релизу 1.0

**Цель:** итеративные доработки и выпуск промежуточных релизов (0.2, 0.3, ...) вплоть до стабильной версии 1.0. После 1.0 — расширение функционала поверх оригинального релиза (новый контент, расширение карты и т.д.).

**Критерий завершения:** все миссии проходимы без крэшей, визуал соответствует оригиналу, релиз 1.0.

---

## Стабильность

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| ASan прогоны — систематический поиск memory corruption | ⏳ | [stage8.md](stage8.md), [known_issues](known_issues_and_bugs.md) | ASan уже нашёл 5+ багов на Urban Shakedown |
| Рандомные крэши (reflection null+8, uninit heap, смена миссии) | ⏳ | [known_issues](known_issues_and_bugs.md) (Крэши) | Вероятно несколько разных багов, ASan должен помочь |
| Тормоза при вступительной катсцене миссии | ⏳ | [known_issues](known_issues_and_bugs.md) (Крэши) | Плавающий баг, не привязан к конкретной миссии |
| ~~Дублированные inline функции → сделать `static`~~ | ✅ | [known_issues](known_issues_and_bugs.md) | Уже исправлено при миграции — 0 inline в .cpp файлах |

## Outro (OpenGL бэкенд)

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| 3D модель не отображается (регрессия x64) | ⏳ | [stage8.md](stage8.md), [known_issues](known_issues_and_bugs.md) | Причина найдена: `fread` struct с указателями, sizeof мисматч x86/x64. Фикс: ручное чтение полей с on-disk размерами |
| `oge_texture_create` — BGRA → GL текстура | ⏳ | [stage7.md](stage7.md) (Outro backend) | Linked list дедуп-кеш, поддержка invert |
| `oge_texture_create_blank` — пустая текстура | ⏳ | [stage7.md](stage7.md) (Outro backend) | |
| `oge_texture_size` — вернуть размер | ⏳ | [stage7.md](stage7.md) (Outro backend) | |
| Vertex shader (`outro_vert.glsl`) — два texcoord | ⏳ | [stage7.md](stage7.md) (Outro backend) | Для bump mapping |
| Fragment shader (`outro_frag.glsl`) — dual-texture multiply | ⏳ | [stage7.md](stage7.md) (Outro backend) | `OS_DRAW_TEX_MUL`, additive, alpha blend, decal |
| VAO для 40-byte vertex | ⏳ | [stage7.md](stage7.md) (Outro backend) | position, rhw, color, specular, texcoord0, texcoord1 |
| `oge_bind_texture` — привязка к stage 0/1 | ⏳ | [stage7.md](stage7.md) (Outro backend) | |
| `oge_draw_indexed` — upload + draw | ⏳ | [stage7.md](stage7.md) (Outro backend) | |
| `oge_change_renderstate` / `oge_undo_renderstate_changes` | ⏳ | [stage7.md](stage7.md) (Outro backend) | OS_DRAW_* → GL state |

## Визуал

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Ghost RGB на шрифтах/HUD иконках | ⏳ | [known_issues](known_issues_and_bugs.md), [ghost_rgb_investigation](../new_game_devlog/stage7_ghost_rgb_investigation.md) | OpenGL, additive blend, в D3D6 не видно |
| ~~Чёрные дырки на ближнем клиппинге~~ | ✅ | [known_issues](known_issues_and_bugs.md) | Отсутствовал `PolyBufAlloc` в near-clip path + фиксы figure.cpp |
| Здания pop-in за туманом | ⏳ | [known_issues](known_issues_and_bugs.md) | Возможно настраивается через config |
| Резкое переключение bump mapping на стенах | ⏳ | [known_issues](known_issues_and_bugs.md) | Релизный баг, виден на лонгплеях |
| Bump mapping fade-in дистанция слишком короткая | ⏳ | [known_issues](known_issues_and_bugs.md) | Crinkle появляется слишком резко при приближении |
| ~~Лужи пропадают на краю камеры~~ | ✅ | [known_issues](known_issues_and_bugs.md) | Полный проход 128 z-строк вместо gamut, ~1-2 нс/кадр |
| Экран брифинга не убирается при загрузке миссии | ⏳ | [known_issues](known_issues_and_bugs.md) | OpenGL-специфичный |
| Листья/трупы поверх огня/взрывов | ⏳ | [known_issues](known_issues_and_bugs.md) | Сверить с оригиналом |
| Z-buffer лежаков у бассейна | ⏳ | [known_issues](known_issues_and_bugs.md) | Estate of Emergency, листья сквозь лежаки + неправильный порядок между двумя лежаками |
| Тонкая настройка тумана | ⏳ | [known_issues](known_issues_and_bugs.md) | Незначительная разница с D3D |
| Меню Sound Options — бегунки громкости не видны | ⏳ | [known_issues](known_issues_and_bugs.md) | Сверить с оригиналом |
| Glow звёзд — ореол вокруг ярких звёзд | ⏳ | [stage7_opengl_verification.md](stage7_opengl_verification.md) | `ge_plot_formatted_pixel` |
| Legacy шрифт `FONT_draw` — попиксельный текст | ⏳ | [stage7_opengl_verification.md](stage7_opengl_verification.md) | Через `ge_lock_screen` |
| `ge_restore_screen_surface` — device-lost recovery | ⏳ | [stage7.md](stage7.md) (TODO) | |
| `ge_update_display_rect` — resize окна | ⏳ | [stage7.md](stage7.md) (TODO) | |
| `ge_is_gamma_available` + gamma коррекция | ⏳ | [stage7.md](stage7.md) (TODO) | Настройки яркости в меню |
| Lit vertex shader (`lit_vert.glsl`) — техдолг | ⏳ | [known_issues](known_issues_and_bugs.md) | D3D/GL clip space несовместимость |

## Управление

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Stick drift в меню | ⏳ | [known_issues](known_issues_and_bugs.md) (Крэши) | Нет deadzone для стиков в меню |

## Платформы

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Linux: сборка, тестирование, пакет | ⏳ | [stage11.md](stage11.md) (Linux) | |
| Linux: case-insensitive file I/O | ⏳ | [stage8.md](stage8.md) | Блокер для Linux |
| Linux: тулчейн CMake | ⏳ | [stage8.md](stage8.md) | `clang-x64-linux.cmake` |
| Steam Deck: проверка | ⏳ | [stage11.md](stage11.md) (Steam Deck) | Возможно отдельный конфиг-профиль |
| Steam Input API | ⏳ | [stage11.md](stage11.md) (Steam Input) | Исследовать доступность для non-Steam games |
| macOS: тулчейны CMake | ⏳ | [stage8.md](stage8.md) | `clang-x64-macos.cmake`, `clang-universal-macos.cmake` |

## Техдолг

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| ~~Удалить D3D6 бэкенд~~ | ✅ | [known_issues](known_issues_and_bugs.md) (Сборка D3D6) | Удалён целиком: backend_directx6/, backend_stub/, CMake/Make targets, документация обновлена |

## Инфраструктура

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Собственный файл настроек | ⏳ | [stage11.md](stage11.md) (Планы) | openchaos.cfg / JSON |
| Лаунчер | ⏳ | [stage11.md](stage11.md) (Планы) | Выбор папки ресурсов через file dialog |
| CI/CD | ⏳ | [stage11.md](stage11.md) (Планы) | GitHub Actions, автосборка при пуше тега |
| macOS: .app bundle + .dmg | ⏳ | [stage11.md](stage11.md) (Планы) | Запуск без Terminal |
| Windows: установщик | ⏳ | [stage11.md](stage11.md) (Планы) | Опционально, portable zip остаётся |
| Проверить лицензии | ⏳ | [stage11.md](stage11.md) (Лицензии) | LICENSE в корне, vendored libs |
| Тестирование на чистой машине | ⏳ | [stage11.md](stage11.md) (Тестирование) | Без dev-зависимостей |
| Changelog / release notes | ⏳ | [stage11.md](stage11.md) (Документация) | |

## Функционал (пре-релизные баги оригинала)

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Crinkle (bump mapping стен) | ⏳ | [known_issues](known_issues_and_bugs.md) | `return NULL` отключён, работает в финале |
| Wibble воды | ⏳ | [known_issues](known_issues_and_bugs.md) | Закомментирован, уточнить наличие в финале |
| Лестницы | ⏳ | [known_issues](known_issues_and_bugs.md) | `mount_ladder` закомментирован |
| Воскрешение гражданских | ⏳ | [known_issues](known_issues_and_bugs.md) | Телепорт на HomeX/HomeZ не применяется |
| MAV INSIDE2 | ⏳ | [known_issues](known_issues_and_bugs.md) | `ASSERT(0)`, NPC не ходят внутри зданий |
| MAV stair/exit | ⏳ | [known_issues](known_issues_and_bugs.md) | `> 16` вместо `>> 16` |
| ~~Баг освещения~~ | ✅ | [known_issues](known_issues_and_bugs.md) | Визуально незаметно, но математически корректно |

## Функционал (новый / из PS1)

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Подсказки кнопок геймпада в меню | ⏳ | [known_issues](known_issues_and_bugs.md) (Желаемый) | Фича из PS1: ✕ выбрать, △ назад |

## DualSense доработки (не всё обязательно для 1.0, но возможно стоит сделать)

| Пункт | Статус | Референс | Комментарии |
|-------|--------|----------|-------------|
| Weapon-specific feedback: дробаш | ⏳ | [stage5_1.md](stage5_1.md) (Weapon-specific) | Adaptive triggers + сильная короткая вибрация |
| Weapon-specific feedback: автомат | ⏳ | [stage5_1.md](stage5_1.md) (Weapon-specific) | Trigger vibration при зажатом R2, серия импульсов |
| Audio-to-haptic | ⏳ | [stage5_1.md](stage5_1.md) (B7), [known_issues](known_issues_and_bugs.md) | Конвертация звуков (стрельба, взрывы, двигатель) в haptic через miniaudio |
| Вибрация в видеовставках | ⏳ | [known_issues](known_issues_and_bugs.md) (Отложенный) | MDEC_vibra[], frame-synchronized |
| Вибрация в меню | ⏳ | [known_issues](known_issues_and_bugs.md) (Отложенный) | Тест-вибрация при включении опции |
| Тачпад | ⏳ | [stage5_1.md](stage5_1.md) (B5), [known_issues](known_issues_and_bugs.md) | Нет дизайн-решения, низкий приоритет |
| Гироскоп | ⏳ | [stage5_1.md](stage5_1.md) (B6), [known_issues](known_issues_and_bugs.md) | Gyro aiming, требует калибровку |

---

Бессрочный список всех багов: [known_issues_and_bugs.md](known_issues_and_bugs.md)
