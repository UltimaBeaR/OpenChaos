# Stage 12 — план парности с релизной PC-версией

Рабочий план Этапа 12 по результатам анализа `original_game_compare/docs/release_changes/` (~405 находок по 15 подсистемам).

Связанные документы:
- **[findings.md](findings.md)** — полный ID-индекс всех находок с per-item решениями (рабочая память, не для ревью)
- [stage12.md](../stage12.md) — критерии Релиза 1.0
- [prerelease_fixes.md](../prerelease_fixes.md) — уже пофикшенные пре-релизные баги
- [known_issues_and_bugs.md](../known_issues_and_bugs.md) — бессрочный список проблем

> **⚠️ СТАТУС: ПРЕДВАРИТЕЛЬНЫЙ.** Все решения о "портируем / не портируем / обсудить" — моё предварительное суждение. Документ не утверждён. Ревью проходит **блоками** (не каждый ID отдельно), каждое решение подтверждается явно.

---

## 🔄 ПЕРЕСМОТР СТРАТЕГИИ (обновлено в процессе ревью)

**Первоначальная гипотеза была неверна.** Report называет ветку `new/` "final Steam PC release", но накопленные доказательства при сверке с запускаемым retail Steam PC бинарём показывают обратное:

- `new/` содержит визуальные фичи которые в retail **не видны**: cloud shadows на фасадах, fog на мешах/людях, crumple deformation машин, floating 3D damage text над персонажами, анимированные текстуры
- `new/` удалил farfacet/superfacet/fastprim, хотя farfacet в retail **работает**
- `new/` массово удалил UI фичи (panel relocation, French localization, `whattoload[]`, mission hierarchy direction), при этом **дефолтный выбор стартовой миссии** в retail работает **как в `new/`**, а не как в нашей pre-release базе — значит часть изменений в `new/` всё-таки соответствует retail. (Изначально я ошибочно приводил тут же loading screen как аргумент — это оказалось нашим OpenGL-багом, не имеет отношения к pre/post-release различиям. См. [known_issues_and_bugs.md](../known_issues_and_bugs.md).)

**Новая гипотеза (почти точно):**
- `old/` = pre-release ветка ≈ **наша база в `new_game/`**, но она **не** retail. Это ветка которую потом допилили до retail.
- `new/` = **post-release dev branch**. MuckyFoot продолжали работать над кодом после шипа retail — добавляли новые фичи, рефакторили, пробовали идеи — но этот билд **никогда не шипнулся**. Остался как snapshot брошенной ветки.
- **Retail source-кода у нас нет вообще.** Ни `old/`, ни `new/` не являются retail.
- **Наша `new_game/` база (растёт из `old/`) ближе к retail чем `new/`**, но всё равно не retail — это pre-release до финальной полировки MuckyFoot.
- Некоторые вещи в `new/` сохранились как в retail (mission hierarchy direction — дефолтная стартовая миссия) — они были уже сделаны на момент шипа retail и остались в post-release ветке. Другие фичи в `new/` добавлены **после** шипа и никогда не выпускались.

### Новая цель Stage 12

**Цель:** повторить RETAIL PC поведение. Наша база `new_game/` уже **ближе** к retail чем `new/`. Значит:

1. **Default: НЕ портируем из `new/`.** Наша версия ближе к retail — если retail что-то работает, у нас скорее всего уже работает.
2. **Портируем только явные bug fixes:** undefined переменные, missing returns, typos (`mz`→`z`), null derefs, buffer overflows, memory corruption, compile-breaking stubs. Эти — баги независимо от ветки, и в retail они **точно** были починены (раз retail работает).
3. **Проверяем по retail, не по `new/`:** если сомнения работает ли фича в retail → игрок смотрит retail бинарь. Если работает — значит у нас скорее всего тоже должно работать, если не работает — чинить нашу сторону. Если в retail не работает → `new/` версия фичи была post-release, нам не нужна.
4. **Улучшения из `new/` — опционально.** Мы можем **выбрать** взять какие-то post-release улучшения если они делают игру лучше и не ломают retail feel. Но это **не обязанность** и не default. Каждый такой порт = отдельное решение "да, это улучшение которое мы хотим сверх retail".
5. **Inverted findings (где pre-release новее `new/`) становятся менее интересны** — если наша база = pre-release, то мы эти фичи уже имеем. Audit делаем только чтобы убедиться что Stage 2/4 cleanup'ы их не потеряли.

### Что это меняет для уже утверждённых блоков (1-8)

**Большинство блоков нужно пересмотреть заново** под новой призмой. Общий подход:

- **Блок 1 (P0 physics/engine):** ✅ Остаётся в силе. Все 4 пункта — явные code bugs (debug short-circuit, typo, missing return, dead guard). Портируем независимо от статуса `new/`.
- **Блок 2 (P0 graphics):** Частично. `FillFacetPoints` UV, `POLY_shadow` buffer overflow, `AENG_upper/lower` buffer overflow — реальные баги, портируем. `apply_cloud` — **SKIP**, retail не показывает cloud shadows. `sewer facets invisible` — нужна верификация (в retail канализация рендерится?).
- **Блок 3 (P0 animation/mesh):** Частично. `FIGURE_draw` null-guard, untextured ASSERT removal — crash fixes, портируем. Fog/cloud re-enable на меши/людей (~6 сайтов) — **SKIP**, retail не показывает mesh fog. `FACE_FLAG_ANIMATE` — **нужна верификация**. Grenade fade — SKIP (зависит от fog).
- **Блок 4 (P0 particles/fire):** Частично. `psystem` tick-rate группа + iterator advance — компилируется у нас значит уже пофикшено или фикс нужен. `FIRE_Flame` struct fill — **нужна верификация** (пламя анимируется в retail?). `pyro spang cap` — SKIP если retail не показывает unlimited sparks.
- **Блок 5 (P0 AI/gameplay):** ✅ Остаётся. Все 3 — gameplay bugs (follower immunity, Balrog stuck, treasure double-apply/swap). Портируем.
- **Блок 6 (P0 vehicles):** Частично. `VEH-02 crumple mesh` — **SKIP**, подтверждено что в retail нет деформации. `VEH-04 siren null-safety` — crash fix, портируем. `VEH-08 chopper MFX_QUEUED` — верификация (есть ли audio gap). `VEH-10/11 WMOVE` — memory/overflow fixes, портируем.
- **Блок 7 (P0 audio):** Частично. `AUD-01/02 rain+wind gates/queue` — bug fixes, портируем (с учётом OpenAL adapter). `AUD-07 wind index` — тоже bug fix. `AUD-03/04 data` — **верифицировать** в retail (есть ли dog sounds, какой wolf).
- **Блок 8 (P1 balance pack):** **ОТКАТ**. Числа в `new/` — WIP post-release баланс, который не шипнулся. Наши pre-release числа ближе к retail. Не портируем, разве что точечно если какой-то tweak подтверждён визуально / слышимо в retail.
- **Блок 9 (P1 UI additions):** Остальные 4 пункта (karma/compass/press-button, menufont effects, lang cleanup, mucky_times) — **все нужна верификация**. Если retail их не показывает — SKIP.

**Блоки 10-14** (Person.cpp guards, визуал draw-order, AI improvements, data/assets, P2 cleanups) — **все под пересмотр**, дефолт SKIP, порт только точечно на проверенных пунктах.

**Блоки 15-16** (AUDIT inverted / ambiguous) — **теряют большую часть смысла**. Если наша база ≈ retail, проверять inverted findings массово не нужно. Остаётся точечный audit только по пунктам где есть подозрение на regression после Stage 2/4 (например, loadscreen баг — см. known_issues).

**Блок 17 (SKIP)** — расширяется. Добавляются почти все фичи/визуал/баланс.

### Пошаговый план работы по новой стратегии

1. **Пройтись по блокам 1-8 заново**, пометить каждый пункт:
   - ✅ `PORT` — явный code bug, независимо от retail статуса
   - 🔍 `VERIFY` — нужна проверка в retail перед решением
   - ❌ `SKIP` — подтверждено что в retail не показывается, `new/` это post-release dev
2. **VERIFY группа → verify session с пользователем.** Группирую чтобы одним проходом в retail.
3. **PORT группа → делаю работу.** Блоки остаются, только короче.
4. **Дополнительные улучшения из `new/`** — рассматриваем в конце, после порта bug fixes. Опционально.

### Наименование файлов

В дальнейшем при обсуждении буду называть:
- `old/` → **pre-release branch** (ветка от которой растёт наша база)
- `new/` → **post-release dev branch** (не шипнулось)
- Наша `new_game/` → **наша база** (pre-release + Stage 1..11 работа)
- **Retail** → только запускаемая Steam PC версия (исходников нет)

---

## ⬇ Историческое: первоначальная стратегия (не актуально, оставлено для контекста)

**⚠ Ниже — первоначальный главный принцип и блоки решений, составленные до того как стала понятна post-release природа `new/`. Контент оставлен для понимания как менялся подход. Актуальные решения — выше.**

---

## 🎯 Главный принцип

**Эталон ощущений — retail Steam PC.** Каждое изменение оценивается по вопросу: "делает ли это игру лучше для игрока, или хотя бы матчит ощущения retail?"

### Правила

1. **Наши собственные улучшения не откатываем.** Fixed timestep, farfacet без глюков, character lighting, antialiasing, mipmap LOD bias, своя transparent sort.
2. **Default: портируем из `new/`**, но каждый пункт с умом — а не бездумно.
3. **Два исключения из default:**
   - **WIP в `new/`:** могли начать и не доделать — pre-release может быть более рабочим.
   - **Регрессия в `new/`:** фича есть в pre-release, удалена в `new/`, в retail работает.
4. **Визуал:** оцениваем **качество алгоритма**, не только наличие. Если наш путь объективно лучше — оставляем.
5. **Код-рефакторы:** портируем только если реально улучшают код.
6. **Невидимое — оценивается отдельно.** Детерминизм, save format rewrites, internal architecture — default скорее skip (игрок не заметит), но не автоматом. Смотрим на стоимость и что именно даёт каждый пункт. Некоторые могут иметь смысл даже без видимого эффекта (например если закладывают фундамент или чинят latent bug).
7. **Downgrade запрещён.**

---

## ⚠️ Что такое `new/` — не retail

Жёсткое предупреждение. Report называет `new/` "final Steam PC", но это неправда:

1. **farfacet / superfacet / fastprim удалены** в `new/`, хотя в retail Steam PC farfacet работает (подтверждено визуально).
2. **ui_frontend массово инвертирован**: десятки полезных фич (panel-relocation, French localization, auto-FMV timeout, `whattoload[]`, `FRONTEND_MissionHierarchy` direction, force-walk) есть только в `old/` (pre-release) и исчезли в `new/`.
3. **`new/` визуально деградирован** при запуске (нет теней, низкие текстуры) — похоже на недонастроенный промежуточный снапшот.

**Следствие:** арбитр спорных пунктов — **запускаемая retail Steam PC**, не `new/`. `new/` — источник подсказок где могут быть фиксы, не истина.

---

## Эталоны (priority order)

1. **Retail Steam PC** — главный арбитр поведения. Эталон ощущений.
2. **`original_game/`** — pre-release база из которой растём. Эталон пре-релизного состояния.
3. **`original_game_compare/old/`** — та же pre-release после Stage 2 cleanup.
4. **`original_game_compare/new/`** — **не эталон**, источник находок. Промежуточный снапшот, местами хуже pre-release, см. предупреждение выше.
5. **PieroZ v0.2.0** — дополнительный источник. Иногда подсматриваем его коммиты на предмет как он решал конкретные баги — может быть полезным референсом для отдельных случаев.

---

## Что уже сделано (не дублируем)

Stage 5 фиксы → см. [prerelease_fixes.md](../prerelease_fixes.md):
- Z-buffer mismatch, cola cans, leaves rotation, bump mapping flicker, GetTickCount overflow × 2.

Stage 4 реструктуризация: файлы в `src/engine/` + `src/game/`, поиск через `grep "uc_orig:.*<file>" src/`.
Stage 7: D3D6 вырезан, только OpenGL через GE контракт в `src/engine/graphics/graphics_engine/`.

---

## Общая статистика

| Категория | Кол-во | Что значит |
|---|---:|---|
| PORT P0 | ~30 | реальные баги в нашей базе — портируем без обсуждения |
| PORT P1 | ~80 | видимые улучшения — портируем пакетами |
| PORT P2 | ~25 | cleanup, trivial — фоном |
| OPEN | ~20 | требуют твоего решения при ревью |
| AUDIT | ~90 | сначала проверить текущее состояние нашей базы |
| SKIP | ~140 | out-of-scope (D3D6 / PSX / editor / dev / software / dead) |
| SELF / INVERT-KEEP / DONE | ~20 | наш путь или уже сделано |
| **Всего** | **~405** | |

---

## БЛОКИ ДЛЯ РЕВЬЮ

Ниже — блоки решений. Каждый блок = одно решение пользователя при ревью. Полные ID → см. [findings.md](findings.md).

### Блок 1. P0 физика/движок — реальные баги

**Что:** `height_above_anything` debug short-circuit, `calc_map_height_at` mz→z typo, `SUB_STATE_WALKING_BACKWARDS` edge-slide guard uncomment, `OB_remove` missing return.

**IDs:** PHY-01, PHY-02, PHY-05, ENG-18.

**Предварительное решение:** PORT — все trivial one-line fixes, явные баги, без risks.

---

### Блок 2. P0 графика — визуальные баги (некоторые через GE адаптацию)

**Что:** `FillFacetPoints` foundation==2 UV bug ("pants stretchy"), sewer facet invisibility, `apply_cloud` real impl (тени облаков на фасадах), `POLY_shadow` buffer size fix (potential crash), `AENG_upper/lower` buffer overflow двухшаговый фикс (resize + mask drop).

**IDs:** GFX-17, GFX-02, GFX-01, GFX-07, GFX-19+GFX-37.

**Предварительное решение:** PORT — все влияют на визуал / стабильность. `AENG_upper/lower` двухшаговый — портируем lock-step как указано в отчёте. `apply_cloud` — полная реализация, не stub.

**Caveat:** `POLY_shadow` buffer может быть связан с ASan крашами из known_issues, портируем первым из этого блока.

---

### Блок 3. P0 анимация/mesh — fog + cloud re-enable + FACE_FLAG_ANIMATE

**Что:** fog/cloud отключены на мешах и персонажах в pre-release (~8 сайтов в `figure.cpp` и `mesh.cpp`). Плюс `FACE_FLAG_ANIMATE` branch закомментирован (анимированные текстуры мертвы). Плюс untextured mesh ASSERT removal, grenade fade 0xff→0, FIGURE null-element guard.

**IDs:** ANM-02, ANM-05, ANM-09, ANM-10, ANM-11, ANM-12, ANM-13, ANM-14, ANM-15, ANM-03, ANM-04.

**Предварительное решение:** PORT — все trivial раскомментирования, видимые баги. Персонажи и машины перестанут выглядеть "пластилином на фоне".

---

### Блок 4. P0 частицы/огонь

**Что:** `FIRE_Flame` struct пустая (пламя не анимируется), `psystem` tick-rate группа (first_pass/prev_tick decl + local_ratio + iterator advance paths + PFLAG_HURTPEOPLE throttle), `pyro` global_spang cap removal.

**IDs:** PRT-01..04, PRT-07..11, PRT-15.

**Предварительное решение:** PORT пакетом — огонь/частицы системно сломаны. Иронично: `psystem` с pre-release даже **не должен компилиться** (undeclared `first_pass`), значит у нас уже какой-то фикс есть — верифицировать что он правильный.

---

### Блок 5. P0 AI / gameplay

**Что:** Follower NPC immunity to fall damage, Balrog PAP_FLAG_HIDDEN collision (stuck on buildings), SPECIAL_TREASURE double-apply + 71↔81 ID swap.

**IDs:** AI-02, AI-04, AI-08.

**Предварительное решение:** PORT — все три явные gameplay баги в pre-release.

---

### Блок 6. P0 vehicles

**Что:** Car crumple mesh `0xff`→`0`, Siren no-driver null-safety (краш), Chopper `MFX_QUEUED` drop (прерывистый звук винта), `WMOVE_relative_pos` fixed-point overflow fix, `WMOVE_remove(class)` bulk walkable cleanup.

**IDs:** VEH-02, VEH-04, VEH-08, VEH-10, VEH-11.

**Предварительное решение:** PORT — все видимые баги / крашы / физ. проблемы. `WMOVE_relative_pos` — единственный large (следую формуле из отчёта дословно).

---

### Блок 7. P0 audio

**Что:** rain/city ambience `GAME_TURN & 0x3f` gate removal, wind retry jitter + MFX_QUEUED, wind sample index ordering, snow wolf WOLF1→WOLF3, city_animals pool 7→11.

**IDs:** AUD-01, AUD-02, AUD-07, AUD-04, AUD-03.

**⚠ Caveat (OpenAL backend):** у нас MSS32 заменён на OpenAL через wrapper (miniaudio используется только в DualSense audio-to-haptic модуле, не в игровом звуке). Риски в 2 пунктах:
- **AUD-01** полагается на дедуп looped sample в MFX слое — если наш OpenAL-wrapper не дедуплицирует, unlimited re-issue даст артефакт. Сначала проверить поведение wrapper'а, при необходимости добавить дедуп.
- **AUD-02** использует `MFX_QUEUED` + `MFX_set_queue_gain`. Если семантика queue в wrapper'е другая или не реализована — нужна адаптация.
- AUD-03/04/07 — чистые данные (sample arrays + enum), бэкенд не затрагивают, портируются без нюансов.

**Предварительное решение:** PORT пакетом, но AUD-01 и AUD-02 сначала через audit wrapper'а.

---

### Блок 8. P1 balance pack

**Что:** числовые баланс-твики. По принципу "релиз — эталон ощущений" портируем одним пакетом, потом runtime test 2-3 миссии, откат точечно если ощущается хуже.

Пункты: TIMER_VANISH 100→50, dead vanish 30→40, respawn gate 2000→3200, roll tolerance 70→100, bodyguard `shoot_time >>= 2` removal, cop search 0x800→0x700, bodyguard search 0x800→0x500, grenade timers 120→80 / 70→30, person warmup 40→10 iters, bonfire sound removal, barrel cone penalty re-enable, NOISE_TOLERANCE 8192→1024.

**IDs:** CMB-09, CMB-10, CMB-15, PRT-51, PRT-24, PRT-25, ENG-12, PRT-17, PRT-39, UI-09.

**Предварительное решение:** PORT пакетом. Caveat: могут быть WIP-числа где pre-release рабочее (unlikely). Откат по одному если поднимется жалоба.

---

### Блок 9. P1 UI additions (release-only фичи — добавить)

**Что:** karma meter / press-button prompt / compass helpers в `panel.cpp`, menufont GLIMMER/SHAKE/HALOED effects на титульном экране, `lang_english.txt` PC cleanup (33 VMU строки удалить), mucky_times table real values.

**IDs:** UI-03, UI-08, UI-30, UI-24.

**Предварительное решение:** PORT — все видимые UI фичи, matches retail.

**⚠ ВАЖНОЕ правило по `add_damage_text` (floating 3D text над персонажами):**

Весь кластер находок из `new/` которые раскомментируют `add_damage_text(ax, ay, az, "...")` (floating 3D текст над головой цели) — **SKIP, не портируем**. Пользователь подтвердил: в retail Steam PC таких floating text'ов **нет нигде**, видели только в нашем debug режиме. Значит в `new/` эти вызовы — dev/debug код который либо не дошёл в retail, либо отключён на уровне build flag / rendering path.

Затронутые ID которые SKIP:
- **CMB-11** damage numbers popup — SKIP (retail показывает только panel bottom-left, не floating над целью)
- **CMB-16** `find_best_grapple` "Better not grapple!" — SKIP
- **CMB-V13** `perform_arrest` "Arrested" — SKIP (retail показывает в bottom-left panel, не floating над целью)
- **CMB-V18** `set_person_enter_vehicle` "Car Locked" — SKIP (bottom-left panel уже есть и в pre-release)
- **AI-11** `Special.cpp` "STA increased" / "FUSE SET" — SKIP

Это правило применяется глобально: любое раскомментирование `add_damage_text` из `new/` → SKIP.

---

### Блок 10. P1 Person.cpp systematic guard removal — ❗ ОТКРЫТО

**Что:** релиз систематически удаляет ~31 conservative guard в state-transition функциях (jump-on-slope, NOGO pull-up, vault-into-wall, slippy idle и т.д.).

Детали — в секции "Открытые вопросы" ниже (примеры на 4 guards, риски, батчи A/B/C).

**IDs (батчи):**
- Батч A (safe): CMB-V1, CMB-V2, CMB-V3, CMB-V7, CMB-V8, CMB-V11, CMB-V14..V17, CMB-D3, CMB-08, CMB-06 (частично), CMB-E1
- Батч B (medium): CMB-V10, CMB-D4, CMB-D5
- Батч C (high risk): CMB-D1
- **НЕ портируем:** CMB-V6, CMB-D2, CMB-E2

**Предварительное решение:** ОТКРЫТО. Нужно твоё слово: (A) порт батчами с runtime verification, или (B) оставить guards / отложить в Stage 13.

---

### Блок 11. P1 визуал / draw-order / GE адаптация — ⚠ требует сверки с OpenGL

**Что:** `POLY_frame_draw` fog colour tint, INDOORS alpha blend override, `POLY_frame_draw_sewater` API, `POLY_fadeout_point` re-enable в facet.cpp (~8 sites), AENG draw order shuffle, `AENG_draw_warehouse` facet filter removal, floor backface cull TRUE, moon distance scales.

**IDs:** GFX-26, GFX-27, GFX-25, GFX-28, GFX-24, GFX-51, GFX-48, GFX-49.

**⚠ Caveat:** у нас OpenGL и своя transparent sort (наша лучше пре-релизной bucket sort). Код `new/` возможно всё ещё использует пре-релизный забагованный sort. Для каждого пункта — проверить что делает наш GE слой, адаптировать через `ge_*` вместо copy-paste.

**Предварительное решение:** PORT с адаптацией. Начать с "чистых" (`POLY_fadeout_point` re-enable, floor backface, warehouse filter, moon distance — не трогают sort). Потом draw order и fog — со сверкой. После — runtime check.

---

### Блок 12. P1 AI/combat улучшения (без guard removal)

**Что:** Darci projectile_move NO_FLOOR + plunge stagger + slide_along (Roper floating fix), vehicle mount flag split, Shotgun/AK47 scatter stagger, pcom AI ASSERT tripwires removal (cop/darci targeting), pcom find_car frequency, pcom add_gang_member off-by-one, pcom navtokill home-zone branch drop, pcom process_person lookat idle (NPC glance), baseball-bat yomp removal extended, cable-fall damage + LOS, crawl/prone collapse, Roper arrest voice.

**IDs:** AI-03, CMB-02, AI-10, PRT-23, PRT-27, PRT-50, PRT-28, PRT-29, CMB-07, CMB-E1, CMB-14, CMB-17, CMB-V25.

**Предварительное решение:** PORT — небольшие улучшения AI/combat. NPC lookat glance — verify (игрок замечает?).

---

### Блок 13. P1 data / settings persistence

**Что:** `AENG_set_detail_levels` ENV persistence для stars / moon_reflection / people_reflection / filter / perspective. People3 textures если есть в ресурсах.

**IDs:** GFX-62, GFX-33.

**Предварительное решение:** PORT detail ENV — нужно для stage12 критерия "настройки работают через конфиг". People3 — AUDIT если есть файлы, тогда PORT.

---

### Блок 14. P2 trivial cleanups (фоном)

**Что:** API simplifications делающие код чище: `create_shockwave` без `just_people`, `pedals` без `Thing*`, `check_limb_pos_on_ladder` без `i_am_going_down`, `PCOM_call_cop_to_arrest_me` без `store_it`, DIRT encapsulation, DOOR_Door static array, `set_person_goto_xz` default ASSERT, MF_Fopen→fopen, MemAlloc→malloc, прочее.

**IDs:** PHY-03, PHY-07, VEH-05, CMB-13, CMB-03, PRT-05, PRT-06, MAP-16, CMB-V9, CMB-V24, CMB-V19, CMB-E3, ANM-20, ANM-21, ENG-13, ENG-15, PLT-12, PRT-47 и др.

**Предварительное решение:** PORT фоном — по мере работы с файлами, не отдельная итерация.

---

### Блок 15. AUDIT — inverted findings (ui_frontend + mission_game + map_world)

**Что:** ~90 находок где pre-release новее `new/`. Нельзя просто "оставляем". Для каждой 3-шаговая проверка:
1. `grep` в нашей базе — есть ли фича (должна быть, раз растём из pre-release)
2. Retail Steam — работает ли визуально/функционально
3. `new/`→retail — нет ли альтернативного механизма

**Особо важные для аудита:**
- **UI-35** FRONTEND_MissionHierarchy direction — critical, бьёт по map screen
- **UI-55** whattoload[] 36-entry mission table — critical, per-mission flags
- **UI-02** panel relocation + health clamp + AK47 + wraparound
- **UI-50** FRONTEND_init music reset
- **UI-22** Attract language-change reinit
- **MIS-06** game_loop cleanups
- **MIS-07** save.cpp fixes (MF_Fopen + ULONG + dup LOAD_open leak)
- **MIS-09** GS_LEVEL_WON null-deref guard
- **MAP-07** EWAY_evaluate_condition negate
- Все остальные UI-* с пометкой AUDIT (~55 штук)

**Предварительное решение:** запустить аудит через subagent'ов (по 2-3 parallel). Результаты → в `findings.md`. Показать тебе только те где что-то не так, не всё подряд.

---

### Блок 16. AUDIT — ambiguous items

**Что:** пункты где без чтения текущего кода не решить:
- farfacet / superfacet / fastprim статус (ADD-03/04/05) — farfacet осознанно держим, остальные возможно dead
- build.cpp / BUILD_draw call sites (ADD-07)
- Camera.h gameplay entity (ADD-17)
- save_table reserve counts (ENG-03)
- `flag_v_faces` / FACE_FLAG_VERTICAL source (ENG-05)
- AENG CurDrawDistance representation (GFX-38)
- `apply_cloud` текущее состояние (GFX-01)
- People3 textures в ресурсах (GFX-33)
- Thing Timer1/BuildingList/CameraMan (AI-01)
- psystem (уже должен иметь какой-то фикс раз собирается)

**Предварительное решение:** subagent аудит пакетом. Результат определит финальное решение.

---

### Блок 17. SKIP — out-of-scope (не требует обсуждения)

Если хочешь по какой-то категории иначе — скажи.

- **D3D6-specific:** renderstate fix_directx, USE_D3D_VBUF, polypage TEX_EMBED, Display::Flip depth-bodge, Permedia 2 workarounds, render state caching switches.
- **Software rasterizer (sw_hack / SOFTWARE):** sw.cpp, tom.cpp, все `if (sw_hack)` ветки, все SOFTWARE gates.
- **PSX-specific:** save_psx / iamapsx / build_psx branches, PSX-ресурсы gates.
- **Dreamcast:** save_dreamcast_wad, DCLowLevel, DT_PAL/NTSC/VGA enum.
- **Editor-only:** walk_links pool, edit_info.HideMap, `#if 0` editor helpers.
- **Network / multiplayer cut:** net.cpp, AsyncFile, noserver.h.
- **Dev tooling / debug:** FFManager DI8 test button, outro standalone exe, CALC_CAR_CRUMPLE, MIKE macros, MAKE_FPM_TRACK_ENEMY, `#ifdef UNUSED` blocks, debug floating labels, debug green lines.
- **API layers мы не используем:** Miles Sound System, Aureal3D, DirectInput8 manager, DirectPlay, Bink client, Win32 menu bar, DDraw enum.
- **Dev scaffolding / Stage-2 artifacts:** outline.h, america.h / demo.h / font3d.h / bike.h placeholders, snd_type.h, resource.h, _asm int 3.
- **Determinism cluster:** PTIME→GAME_TURN cascade, Root() bit-exact sqrt, COMMON(TYPE) 4→8 expansion, WaveParams struct API. **См. Открытые вопросы — может измениться.**
- **Already handled differently:** AENG_draw_dirt rewrite (Stage 5), POLY scaffolding (Stage 5), overlay viewport reset (GE), polypage TEX_EMBED (OpenGL).

**Всего:** ~140 пунктов.

---

### Блок 18. SELF / уже сделано

**Что:** пункты где у нас свой путь лучше или сделано на Stage 5/7.

- GFX-23 AENG_draw_dirt rewrite → Stage 5 flush-after проще
- GFX-60 POLY scaffolding → Stage 5 фикс (реальная функция)
- PRT-31/32 Overlay viewport / POLY_flush_local_rot → OpenGL GE корректен
- GFX-05 POLY_add_poly unified → свой GE
- GFX-13 polypage TEX_EMBED → OpenGL
- UI-01 Panel depth-bodge → OpenGL
- Плюс все из [prerelease_fixes.md](../prerelease_fixes.md)

**Предварительное решение:** ничего не делаем.

---

## 🔍 Открытые вопросы — требуют твоего решения

1. **PTIME / Root() determинизм cluster** — порт ~100+ изменений ради replay/multiplayer. Игрок не заметит визуально. Склон — skip, вернёмся при ревью.
2. **Person.cpp systematic guard removal (~31 сайт)** — release агрессивно убирает conservative guards. Что такое guards: защитные early-return'ы в state-transition функциях (jump→не прыгнуть со склона, idle→не встать на скользкий склон, vault→не перепрыгнуть если за забором стена). Retail игрок знает игру без них. Варианты: (A) порт батчами A/B/C, (B) оставить / отложить.
3. **Grab-edge 4→2 point test (MIS-01)** — pre-release строже, release мягче. Что в retail?
4. **MUSIC per-Ware override removal (AUD-06)** — inverted. В retail музыка меняется в warehouse?
5. **process_things noise+arrest queue removal (AI-05, PRT-21, PRT-22)** — API simplification меняющее AI timing.
6. **process_things NextLink advance order (AI-07)** — inverted, предварительно INVERT-KEEP.
7. **`fn_person_fighting` headbutt copy-paste bug (CMB-E2)** — НЕ портируем (баг new/).
8. **`SUB_STATE_WALKING` footstep deletion (CMB-D2)** — НЕ портируем (регрессия).
9. **`fn_person_gun` safety path (CMB-V6)** — INVERT-KEEP (pre-release безопаснее). Согласен?
10. **oval.cpp Z-fight removal (GFX-15)** — cause unclear.
11. **Mesh fade 0xff→0 (GFX-22)** — семантика неясна.
12. **Car color palette swap (GFX-20)** — verify retail.
13. **Fight mode persist while running (CMB-V11)** — verify.
14. **NPC idle glance at player (PRT-29)** — verify.
15. **Hands-up civilians targeting (CMB-04)** — verify.
16. **find_arrestee cop scoring (CMB-V12)** — verify.
17. **Roper/tramp estate guard (CMB-05)** — verify.
18. **Wall hit anim (CMB-08)** — verify.
19. **Slider alpha 128 vs 192 (UI-47)** — verify.
20. **Enemy health percent vs Health>>1 (PRT-34)** — cause unclear.
21. **Bonfire periodic sound (PRT-17)** — verify.
22. **Special drop pointer cleanup (AI-09)** — UAF risk, verify.
23. **RunCutscene(2) vs (3) (MIS-33)** — verify against .cpk layout.
24. **PSystem menu-pause early-out (PRT-14), ribbon menu early-out (PRT-38)** — verify intent.
25. **`pcom SOUND_FIGHT` bent test (PRT-30)** — unclear if regression.
26. **Load_needed_anim_prims per-level gating (MIS-04)** — inverted memory optimization.
27. **PHY-01 `height_above_anything` Ware branch (collide.cpp)** — оказался НЕ bugfix, а behavior change. Pre-release: `if (1 || Ware)` — мёртвая else-ветка, всегда `FIND_FACE_NEAR_BELOW`. Release: убрали `1||`, non-Ware персонажи теперь получают `FIND_ANYFACE` (более общий поиск). Наш код: при портировании выкинули мёртвую ветку, семантически = pre-release. Что делать: (1) понять что такое флаг `Ware` — grep где выставляется; (2) проверить в retail: есть ли разница в приземлении/падении для не-Ware персонажей; (3) не факт что правка вообще дошла до retail Steam (`new/` post-release dev branch). **Отложено 2026-04-14** до явного retail-feel check.

---

## 📌 Наблюдения пользователя в retail (отдельно проверить при порте соответствующих блоков)

### Mission loadscreen — картинка отличается

**Симптом:** в retail PC загрузочный экран миссий показывает **другую картинку** чем в нашей текущей версии. Ресурсы те же самые. Значит отличается либо вызов (`ATTRACT_loadscreen_init(N)` с другим N), либо сам механизм выбора ассета.

**Где проверять:**
- Связано с MIS-10 (game_startup loadscreen order — inverted)
- Связано с GFX-12 (`TEXTURE_load_needed` signature simplified)
- Возможно связано с UI-55 (`whattoload[]` 36-entry table) — если ассет loadscreen определяется по индексу миссии
- Grep `ATTRACT_loadscreen_init` / `ATTRACT_loadscreen_draw` во всех трёх деревьях (`old/`, `new/`, наш `new_game/`) — сравнить параметры и call sites

**Когда разбирать:** при работе с Блоком 15 AUDIT inverted (обходим MIS-10 и UI-55).

### Default selected mission на map screen

**Симптом:** в retail при старте новой игры дефолтно выбрана **беговая** миссия (с препятствиями), не миссия с полицейской машиной. Обе доступны сразу, но курсор изначально на беговой. В нашей текущей версии — наоборот, дефолт на машине.

**Где проверять:**
- **UI-35** `FRONTEND_MissionHierarchy` comparator direction — почти наверняка причина. Pre-release: `if (order < best_score)` → наименьший order (раньше в последовательности). `new/`: `if (order > best_score)` → наибольший order. Инверсия выбора.
- Надо определить какое направление даёт retail поведение ("беговая миссия по умолчанию") и синхронизировать с ним.

**Когда разбирать:** при работе с Блоком 15 AUDIT inverted (UI-35).

---

## 🎯 Verify-in-Steam session (буду заполнять по мере утверждения блоков)

Список сгруппирован по локациям в игре — чтобы ты мог пройти одной сессией.

### Main menu / HUD
- Karma meter есть в HUD?
- Compass (линия направления) есть?
- Press-button prompt при интеракции есть?
- Menufont effects (glimmer/shake/halo) на "Urban Chaos" логотипе?
- Slider alpha — sliders прозрачные (128) или плотные (192)?
- Map screen: при suggestion следующей миссии — идёт по возрастанию order (pre) или убыванию (new)?
- Save/Load screen: пустые slots — grey (disabled) или selectable?

### Любая миссия (визуал)
- Тени облаков видны на фасадах зданий? (GFX-01 `apply_cloud`)
- **Дистанционный туман применяется к машинам / персонажам / мешам?** Смотреть далёкую машину или NPC — становится ли более туманной по мере удаления (как далёкие дома)? (ANM-02/05/09/10/11/12 — fog re-enable на меши/людей)
- Пол зданий виден снизу?
- Горизонт: туман тонирован в цвет неба или hard black?
- Вход в здание: пол плавно исчезает/появляется?
- Вода в канализации: выглядит правильно?

### Любая миссия (аудио)
- Walking footsteps слышны?
- Rain/city ambience не пропадает долго?
- Wind gusts: не перекрывают друг друга жёстко?
- Bonfire: звучит (каждые 8 turns) или молчит?

### Любая миссия (combat)
- Damage popups (floating numbers) видны при ударе?
- "Arrested" / "STA increased" / "Car Locked" / "FUSE SET" текст всплывает?
- Wall-hit: при лобовом в стену играется анимация разворота?
- При беге — выходит ли игрок из fight mode автоматически?
- Follower NPC: получает fall damage?
- NPCs: косятся на игрока (glance) при прохождении?

### Combat balance
- Bodyguard fire rate: быстро или медленно стреляет по цивилам?
- Cop search radius: замечает на каком расстоянии?
- Cops: можно ли арестовать не-guilty cop?

### Mission: Estate of Emergency (Balrog)
- Balrog: залипает на стенках зданий?

### Mission: Gatecrasher (nightclub)
- context_music: в клубе музыка меняется на mode 0?

### Сниперка / бинокль
- Draw distance сжимается при zoom?
- Moon визуально сжимается?

### Warehouse
- Музыка меняется при заходе в warehouse/interior?

### Grab edges
- Схватился за уступ: строгий или мягкий допуск?

### Cones (mission с traffic cones)
- "Time Penalty" сообщение показывается при hit cone?

---

## ✅ VERIFY checklist — итоговый (retail Steam PC session)

Этот чеклист — **то что пользователь проверяет запуская retail Steam PC**. Результаты определяют финальное решение по VERIFY пунктам. Для пунктов где "да" — portируем. Для "нет" — SKIP (но остаются задокументированными здесь, чтобы если когда-то потом захотим реализовать, знали куда смотреть).

### 🎬 Main menu / title screen / score screen

- **Menufont effects (UI-08):** на title screen логотип "Urban Chaos" — есть glimmer (шимер-эффект) / shake (трясётся) / halo (ореол)?
- **Mucky_times (UI-24):** войти в score screen после прохождения миссии — target times реалистичные (Mark Rose / Fin / Mike B.) или placeholder ("Beat me" / dev-имена Alex Hood / Joe Neate)?
- **Slider alpha (UI-47):** открыть options menu — sliders плотные или прозрачные насквозь?

### 📊 HUD общий (любая миссия)

- **Karma meter (UI-03):** есть в HUD индикатор karma (ангельский/дьявольский статус)?
- **Press-button prompt (UI-03):** подойти к интерактивному объекту (дверь, лестница, бочка, дверь машины) — появляется prompt "press X"?
- **Compass (UI-03):** видна compass-линия (указатель направления) где-то на HUD?

### 🏙 Любая миссия — базовый визуал

- **Stretchy textures (GFX-17):** пройти вдоль каналов / лестниц / высоких бордюров / наклонных оснований зданий — есть ли визуально растянутые текстуры, или всё нормально пропорциональное?
- **Sewer visibility (GFX-02):** спуститься в канализацию — вся геометрия видна корректно, или части facet'ов прозрачные/невидимые?
- **Animated scrolling textures (ANM-13/14):** искать анимированные текстуры — бегущие строки на баннерах, скроллящиеся вывески, движущиеся декали. Есть такое в retail?
- **Car colors (GFX-20):** машины на улицах — muted реалистичные цвета (green, blue, gold, teal, mustard) или saturated debug (yellow, magenta, cyan, bright red)?
- **Floor backface cull (GFX-48):** если есть доступ посмотреть на пол здания снизу — пол виден или нет?
- **Fire animation (PRT-01 + FIRE_Flame):** поджечь что-нибудь (бочка, molotov, pyro эффект) — пламя визуально движется (языки огня меняют форму, извиваются), или статичный sprite с мерцанием?

### 🔊 Любая миссия — audio

- **Rain ambience (AUD-01):** в миссии с дождём, постоять/поиграть минуту-две — rain ambience слышен постоянно или пропадает на секунду-другую?
- **Wind gusts (AUD-02):** слышны ли wind gusts, и если да — наслаиваются плавно (смешиваются) или один резко прерывает другой?
- **City dogs (AUD-03):** в BusyCity локациях слышен периодический лай собак? Должно быть 4 разных звука (S_DOG1..4).
- **Snow wolf (AUD-04):** в snow миссиях (если в campaign есть) слышен редкий wolf howl?
- **Bonfire periodic sound (PRT-17):** подойти к костру — звук огня проигрывается периодически (каждые ~8 игровых тиков) или молчит (только визуальный огонь без доп.звука)?

### 🔫 Любая миссия — combat / feel

- **Unlimited sparks (PRT-15):** стрелять очередью из автомата/шотгана в стену — много искр одновременно (больше 5)?
- **NPC glance (PRT-29):** civs на улице косятся головой в сторону Darci когда она проходит мимо, или игнорируют?
- **Baseball-bat run animation (CMB-07/E1):** взять бейсбольную биту, побежать — есть специальная анимация бега с битой (`ANIM_YOMP_BAT`), или обычный run?
- **Cable-fall damage (CMB-14):** проехаться на зиплайне до конца и упасть — получаешь damage от падения?
- **Roper arrest voice (CMB-V25):** арестовать Roper'а — играется озвучка при аресте?
- **Vehicle mount (CMB-02):** войти в машину которую уже ведёт NPC-водитель — NPC убегает (driver-swap) или остаётся / выталкивается?
- **Walking backwards snap-slide (PHY-05):** NPC идёт спиной по краю платформы/угла — snap-slide скользит к центру, или нет?
- **Fight mode persist running (CMB-V11):** в fight mode начать бежать — выходишь из fight mode автоматически или остаёшься в fight mode?
- **Hands-up civs autoaim (CMB-04):** civilians с поднятыми руками (арестабельные/сдавшиеся) — autoaim их таргетит или пропускает?
- **Cop arrestable (CMB-V12):** cops можно арестовать (не только убить) если они не-guilty?
- **Roper/tramp estate (CMB-05):** на estate-миссии Roper может ударить tramp?
- **Wall hit animation (CMB-08):** бежать лбом в стену — проигрывается анимация разворота / отшатывания (`ANIM_HIT_WALL`)?

### 🎯 Специфические миссии / локации

- **Chopper rotor sound (VEH-08):** в миссии с вертолётом — звук винта continuous loop или прерывистый / с разрывами?
- **Follower fall damage (AI-02):** миссия с hostage/follower NPC, ведёшь за собой, hostage прыгает с высоты — получает fall damage или immune?
- **Warehouse music change (AUD-06):** войти в warehouse interior (есть такие внутри некоторых миссий) — музыка меняется на другой mode?
- **Grab edges strictness (MIS-01):** проходить много edge-grab'ов (roof-to-roof), ощущается **строгий** (часто отказывается ловить когда могло бы) или **мягкий** допуск?
- **Barrel cone penalty (PRT-39):** миссия с traffic cones (с флагом GF_CONE_PENALTIES — miss test / race) — наехать на cone, появляется ли панельное сообщение "Time Penalty"?
- **RunCutscene indices (MIS-33):** катсцены проигрываются корректно и в нужные моменты?
- **Special drop UAF (AI-09):** уронить текущий used/drawn предмет, подобрать другой, быстро дропнуть — нет крашей?

### 🔭 Бинокль / снайперка

- **Moon distance zoom (GFX-49):** зайти в бинокль/снайперку, посмотреть на луну — сжимается (меньше) при zoom in или размер не меняется?
- **Binocular draw distance (GFX-36):** зоомировать в бинокль — far draw distance сжимается (город "приближается" через обрезку геометрии)?

### ⚙ Options menu

- **Settings persistence (GFX-62):** в options menu переключить stars / moon reflection / people reflection / filter / perspective, перезапустить игру — настройки сохранились? (Критично для нашего Stage 12 критерия "настройки через конфиг".)

### ❓ Открытые / cause unclear — потребуют дополнительного обсуждения после проверки

- **oval.cpp Z-fight (GFX-15):** декали на дорогах и на земле (следы крови, пятна от взрывов, тормозные следы) — стабильно рендерятся или z-fight'ят/мерцают?
- **Mesh fade 0xff→0 (GFX-22):** посмотреть разные меши (машины, props) — есть ли объекты где fade/alpha значение выглядит странно?
- **Enemy health % vs >>1 (PRT-34):** health bar над врагом в бою — если нанёс ~половину урона, показывает ~50% или странное значение?
- **SOUND_FIGHT bent reaction (PRT-30):** civs с bent характером (готовые драться) услышали звуки драки рядом — присоединяются или игнорируют?

### 🎮 Person.cpp guards — общий feel check (заменяет 31 отдельную проверку)

**Большой вопрос:** управление Darci в retail ощущается **отзывчивым** (X) или **консервативным** (Y)?

- **X (отзывчивое):**
  - Можно прыгать с крутых склонов (slope > 50 не блокирует jump)
  - Pull-up работает на любые ledges, включая края с "NOGO" зонами за ними
  - Vault работает вплотную к стенам (за забором стена — всё равно прыгает)
  - Idle на скользких склонах ставится без скатывания вниз
  - Скид-стоп проскальзывает дальше (не стопает velocity в ноль)
  - Recoil/hit animations прерываются новыми hit'ами сразу (не "защищены" текущей анимацией)
  - Crouch на скользком склоне не блокируется
  - NPC которые должны куда-то бежать — запускают yomp-анимацию сразу, не "ловят" анимации-замены

- **Y (консервативное):**
  - Жмёшь прыжок на наклоне — не прыгает
  - Pull-up в край с NOGO — блокируется без feedback
  - Vault вплотную к стене — не перепрыгивает
  - Idle на скользком — скатывается
  - Управление ощущается "липким" в edge cases

**Ответ на этот один вопрос определяет весь кластер Person.cpp guard removal (31 сайт).** Если X — портируем батчами. Если Y — оставляем наши (pre-release) guards, дальнейший разбор не нужен.

---

## 🔍 Code audits — для меня (через subagents, не retail)

Это проверки нашей текущей базы `new_game/` через grep и чтение файлов. Я запущу 2-3 параллельных subagent'а и соберу результаты. Retail не нужен.

1. **psystem state (PRT-07/08):** как у нас сейчас в `psystem.cpp` реализованы `first_pass`, `prev_tick`, `local_ratio`, `tick_diff` reset в `PARTICLE_Run`. Raw pre-release версия не должна собираться — значит фикс у нас есть, но какой именно?
2. **FIRE_Flame struct (PRT-01):** в `fire.cpp` — struct `FIRE_Flame` заполнена полями (`dx, dz, die, counter, height, next, points, angle[], offset[]`) или пустая shell?
3. **farfacet / superfacet / fastprim (ADD-03/04/05):** какие из этих трёх подсистем у нас live (используются), какие dead code. farfacet мы знаем что используем осознанно. superfacet и fastprim — проверить.
4. **build.cpp / BUILD_draw (ADD-07):** есть ли в нашей `new_game/` файл `build.cpp` или эквивалент, есть ли вызовы `BUILD_draw` / `BUILD_draw_inside` где-то в коде.
5. **Camera.h CameraMan (ADD-17 / AI-01):** есть ли в нашей базе `CameraMan` struct, `MAX_CAMERAS`, `alloc_camera`, `create_camera`, `process_t_camera` / `process_f_camera`.
6. **flag_v_faces / FACE_FLAG_VERTICAL (ENG-05):** где в нашем коде выставляется `FACE_FLAG_VERTICAL`. Если нигде — потеряно, если при build-time или runtime — найти место.
7. **save_table reserve counts (ENG-03):** какие значения Extra для SAVE_TABLE_VEHICLE, PEOPLE, SPECIAL, BAT, DTWEEN, DMESH, PLAT, WMOVE у нас сейчас.
8. **Thing struct members (AI-01):** в нашей `Thing.h` — есть `Timer1`, `BuildingList` union, `CameraMan* Camera` в Genus union.
9. **height_above_anything debug (PHY-01):** в нашем `collide.cpp` функция `height_above_anything` — есть ли там `if (1 || ...)` debug short-circuit.
10. **apply_cloud stub (GFX-01):** в нашем `facet.cpp` функция `apply_cloud` — это real реализация (walks `cloud_data[32][32]`) или no-op stub. (Это важно: если у нас stub, но retail тоже не показывает cloud shadows, тогда stub — осознанное состояние, не баг.)
11. **CurDrawDistance representation (GFX-38):** в нашем `aeng.cpp` — `CurDrawDistance` хранится как 8.8 fixed-point (`22 << 8`) или plain integer (`22`)?

---

## Процесс работы (после утверждения)

1. Блок утверждается → обновляю `findings.md` статус на конкретных ID.
2. Порядок: P0 physics → P0 engine → P0 vehicles → P0 AI → P0 audio → P0 particles → P0 animation → P0 graphics → P1 balance → P1 UI → P1 визуал → P1 AI → AUDIT subagent batches → P2 cleanups → verify-in-Steam → final QA.
3. Блок = одна итерация. Не батчу коммиты: один блок (или подблок) = один коммит.
4. AUDIT пакеты запускаю через subagents (2-3 parallel max).
5. Если внутри блока натыкаюсь на непонятное — останавливаю, спрашиваю.
6. После всех блоков — verify против [stage12.md](../stage12.md), фикс regression'ов.

### Правила верификации

- **Все визуальные и аудио изменения — обязательный before/after test.** Перед коммитом блока с визуалом/аудио: (1) сделать скрин/запись текущего состояния, (2) применить фикс, (3) сделать скрин/запись после, (4) сравнить с retail Steam. Без этого коммит не уходит. Эффект должен быть виден — если не видно, возможно фикс ложный или у нас уже что-то сделано.
- **P0 баги физики/коллизий/AI** — runtime test на 1-2 миссиях после применения, без скриншотов.
- **Балансовые числа** — весь пакет применяется одним коммитом, потом прогон 2-3 миссий, откат точечный если что-то ощущается явно хуже.
