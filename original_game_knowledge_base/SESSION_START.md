# SESSION START — Urban Chaos KB Быстрый Ориентир

**Правило старта сессии:** Читать ТОЛЬКО этот файл + CLAUDE.md. Остальные файлы KB читать точечно по мере необходимости.
**Если задача касается подсистемы X → читать subsystems/X.md перед работой.**

---

## ⚠️ Скоуп анализа: PC + PS1

**Эталон — ОБЕ релизные версии: PC (Steam) и PS1.**
- Фичи, присутствующие в любой из версий, считаются частью финальной игры
- PC-only фичи (отсутствуют на PS1): **лужи** (puddle.cpp), **bump mapping/Crinkle** (на ящиках)

---

## ⚡ СТАТУС И СЛЕДУЮЩИЕ ИТЕРАЦИИ

**Фаза 1 (ЗАВЕРШЕНА):** Детальный анализ оригинального кода → `original_game_knowledge_base/`
- KB написана на ~99% — все геймплейные подсистемы покрыты + верификация проведена
- 80+ файлов аннотированы `// claude-ai:` комментариями
- ~180 неаннотированных файлов протриажены: 90%+ = editor/PSX/rendering (подтверждено)
- **ГОТОВО к Фазе 2** (планирование new_game)

**Итерации анализа:** см. `ANALYSIS_ITERATIONS_PLAN.md` — итерации 1-8 выполнены, осталась 9.
**Быстрые факты:** см. `QUICK_FACTS.md` (все ключевые числа, формулы, константы).

**Оставшиеся итерации:**
- **Итерация 9:** DENSE_SUMMARY.md — сверхкомпактная версия всей KB (~30-40K токенов)

---

## Карта подсистем

| Подсистема | Файл KB | Статус | Ключевой факт |
|-----------|---------|--------|---------------|
| **Физика/коллизии** | physics.md + physics_details.md | ✅ | Integer-only, slide_along без отскока, гравитация animation-driven |
| **Thing/Объекты** | game_objects.md | ✅ | union+fnptr полиморфизм, MAX 700, MapWho хэш, state machine |
| **AI (PCOM) — ядро** | ai.md | ✅ | 22 типа AI (PCOM_AI 0-21), 28 состояний (PCOM_AI_STATE 0-27) |
| **AI — структуры** | ai_structures.md | ✅ | Person struct, типы персонажей, флаги |
| **AI — поведения** | ai_behaviors.md | ✅ | MIB, Bane, Driving, Killing, Snipe, Zone |
| **Навигация** | navigation.md | ✅ | MAV = greedy best-first (НЕ A*), горизонт 32, NAV = wallhug |
| **Персонажи/анимации** | characters.md + characters_details.md | ✅ | Vertex morphing (НЕ skeletal), DrawTween, Thug/Cop в #if 0 |
| **Управление/ввод** | controls.md + psx_controls.md | ✅ | 18 кнопок INPUT_*, 52 ACTION_*, zipwire есть в финале; PSX: 4 раскладки + аналог |
| **Транспорт** | vehicles.md | ✅ | 4 пружины подвески, 6 зон урона, мотоцикл не в финале |
| **Бой** | combat.md | ✅ | Урон из анимации (GameFightCol), knockback через анимации |
| **Оружие/предметы** | weapons_items.md | ✅ | 30 типов SPECIAL_*, мины заглушены, C4 = 5 сек (не 10) |
| **Мир/карта** | world_map.md | ✅ | PAP_Hi 128×128 + PAP_Lo 32×32, MapWho, здания |
| **Здания/интерьеры** | buildings_interiors.md | ✅ | 21 тип STOREY_TYPE_*, DFacet, двери через MAV |
| **Миссии (EWAY)** | missions.md + missions_implementation.md | ✅ | 42 условия, 52 действия, .ucm ≠ MuckyBasic, polling каждый кадр |
| **Загрузка уровня** | level_loading.md | ✅ | 9 шагов, MAV_precalculate самый тяжёлый, игрок создаётся через EWAY |
| **Рендеринг** | rendering.md | ✅ | DirectX6 fixed-function pipeline; painter's algorithm (bucket sort) |
| **Рендеринг/Меши** | rendering_mesh.md | ✅ | Tom's Engine, vertex morphing, crumple, UV packing |
| **Рендеринг/Освещение** | rendering_lighting.md | ✅ | NIGHT система, Crinkle (отключён в пре-релизе, но работает в PC) |
| **Состояния игрока** | player_states.md | ✅ | Полные списки STATE_* и SUB_STATE_* |
| **Эффекты** | effects.md | ✅ | POW(взрывы), PYRO(18 типов), DIRT(16 типов debris), RIBBON, BANG |
| **Форматы ресурсов** | resource_formats/ | ✅ | .iam/.prm/.all/.lgt/.ucm/.txc/.tma задокументированы |
| **Камера** | camera.md | ✅ | FC only (cam.cpp=мёртв), 8-шаг raycast collision, get-behind |
| **Звук** | audio.md | ✅ | Miles Sound System (MSS32), 14 MUSIC_MODE_*, 5 биомов ambient |
| **UI** | ui.md | ✅ | HUD, инвентарь, fonts, gamemenu, frontend pipeline |
| **Frontend** | frontend.md | ✅ | game→mission pipeline, FE_* режимы, STARTSCR_mission, Attract.cpp |
| **Прогресс/сохранения** | player_progress.md | ✅ | .wag: var-str+CRLF, v0-3, hierarchy bits, best_found=анти-фарм |
| **Матем/утилиты** | math_utils.md | ✅ | 2 стека (PSX int/PC float), Matrix3x3/Quaternion/Vector3 |
| **Игровые состояния** | game_states.md | ✅ | per-frame порядок, DarciDeadCivWarnings, GS_REPLAY=goto |
| **Взаимодействие** | interaction_system.md | ✅ | find_grab_face 2-pass, cable params в DFacet, ladder, zipwire |
| **Малые подсистемы** | minor_systems.md | ✅ | Water, Trip, SM, Balloon, Tracks, Spark, Command(legacy) |
| **Скоуп анализа** | analysis_scope.md | — | Что анализируем/не анализируем и почему |
| **WayWind** | waywind.md | ❌ Вне скоупа | Редактор |

---

## Карта связей (что читать вместе)

```
collide.cpp      → physics.md + navigation.md + characters.md
Person.cpp       → ai.md + ai_structures.md + ai_behaviors.md + combat.md + controls.md + player_states.md
pcom.cpp         → ai.md + ai_behaviors.md
eway.cpp         → missions.md + game_objects.md
Mission.cpp      → missions.md + weapons_items.md
interfac.cpp     → controls.md + player_states.md + camera.md
Vehicle.cpp      → vehicles.md + physics.md
Special.cpp      → weapons_items.md + combat.md
Building.cpp     → buildings_interiors.md + world_map.md + navigation.md
interact.cpp     → interaction_system.md + physics.md + controls.md + characters.md
plat.cpp         → game_objects.md + missions.md (waypoints)
chopper.cpp      → game_objects.md + ai.md
```

---

## Аннотированные исходники (// claude-ai: комментарии)

| Файл | Статус |
|------|--------|
| hm.cpp, Furn.cpp | ✅ |
| pcom.cpp (~178 ann.) | ✅ |
| Special.cpp (~477 ann.) | ✅ |
| bat.cpp (~128 ann.) | ✅ (Bane AI, НЕ летучие мыши!) |
| canid.cpp (~101 ann.) | ✅ (собаки инертны — dispatch закомментирован) |
| cutscene.cpp, ob.cpp, elev.cpp | ✅ |
| Anim.cpp, mesh.cpp (game), id.cpp | ✅ |
| facet.cpp, supermap.cpp, io.cpp, Prim.cpp | ✅ |
| Game.cpp, figure.cpp (DDEngine) | ✅ |
| Controls.cpp (~215 ann.) | ✅ |
| collide.cpp (~77 ann.) | ✅ (move_thing порядок, ladder, gravity=-51) |
| walkable.cpp (~20 ann.) | ✅ |
| Person.cpp (~8 блоков) | ✅ (per-frame порядок, jumping FSM, drop_down) |
| eway.cpp (~60+ блоков) | ✅ (per-frame loop, spawn funcs, cond stubs) |
| elev.cpp (~30+ ann.) | ✅ (WPT→EWAY_DO mapping) |
| interfac.cpp (~50+ блоков) | ✅ (process_hardware_input, InputDone, weapon hotkeys) |
| ware.cpp (~5 блоков), inside2.cpp (~6 блоков) | ✅ |
| stair.cpp, gamemenu.cpp | ✅ |
| Attract.cpp, frontend.cpp (~7+ блоков) | ✅ |
| wmove.cpp (~5 блоков) | ✅ |
| collide.cpp (ladder: +3 блока) | ✅ |
| Map.cpp (+4 блока), night.cpp (+2 блока) | ✅ |
| overlay.cpp, guns.cpp, grenade.cpp | ✅ (header блоки) |
| Nav.cpp, Wallhug.cpp, barrel.cpp | ✅ (header блоки) |
| pow.cpp, pyro.cpp, dirt.cpp | ✅ (header блоки) |
| fire.cpp (~99 ann.), psystem.cpp (~158 ann.) | ✅ |
| Vehicle.cpp (~309 ann.) | ✅ (5159 строк, 52 функции) |
| Building.cpp (~141 ann.), mav.cpp (~152 ann.) | ✅ |
| sound.cpp (~103 ann.), door.cpp (~33 ann.) | ✅ |
| ribbon.cpp, bang.cpp, interact.cpp | ✅ (header блоки) |
| plat.cpp, chopper.cpp, Pjectile.cpp | ✅ (header блоки) |
| startscr.cpp | ✅ (header: game build = только 1 переменная) |
| psxlib/GHost.cpp | ✅ (PSX controls: PAD_cfg0..3, аналог, вибрация) |
