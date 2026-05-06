# Input System — переработка системы управления

> ## ✅ ЗАДАЧА ПОЛНОСТЬЮ ЗАКРЫТА (2026-05-06)
>
> Все работы по унификации системы управления через модуль `input_frame`
> завершены. Все hardware input reads (клавиатура, gamepad buttons, sticks,
> triggers, D-Pad, mouse, DualSense touchpad) во всех потребителях идут через
> единое API в [`new_game/src/engine/input/input_frame.h`](../../new_game/src/engine/input/input_frame.h).
> Старая система удалена целиком: `Keys[256]`, `LastKey`, `joystick.{cpp,h}`,
> `ReadInputDevice/GetInputDevice`, same-turn release latching machinery.
>
> Полная сводка → последняя секция [changelog.md](changelog.md) "Phase D —
> финальная зачистка legacy input system".
>
> **Что НЕ входит в эту задачу** (и осталось как опциональное расширение):
> система actions, переназначение клавиш (remap UI), контекстные группы
> (на ходу / в машине), reverse-lookup glyph'ов для подсказок UI. Это
> отдельный большой план в [`full_plan_deferred.md`](full_plan_deferred.md) —
> не блокирует, может быть реализован позже как аддитивный слой над
> `input_frame` (API спроектирован чтобы actions можно было надстроить).

Все работы по новой/унифицированной системе управления собраны здесь. Папка
переехала из `fps_unlock/` потому что задача шире чем FPS unlock — затрагивает
меню, debug-клавиши, vehicle controls и общую структуру обработки ввода.

## Что здесь

| Файл | Что внутри |
|------|-----------|
| [current_plan.md](current_plan.md) | Финальное описание модуля `input_frame` — API, scope, фазы реализации. Статус: **полностью реализовано**. |
| [migration_checklist.md](migration_checklist.md) | Список граблей и решений собранных при миграции (combined-source 2× rate, antagonist suppression, stale auto-repeat из другого контекста, D-Pad как отдельный источник, и т.д.). Полезен для будущих расширений (новый consumer / actions слой / mouse buttons / etc.). |
| [changelog.md](changelog.md) | Журнал изменений по задаче (хронологически). Последняя секция — финальная сводка для handoff. |
| [full_plan_deferred.md](full_plan_deferred.md) | **Отложенный** план следующего шага — actions, remap UI, контексты, reverse-lookup glyph'ов. Не блокирует, реализуется когда возникнет потребность. API `input_frame` совместим с этим расширением. |

## Если задача возобновляется (расширение, не переделка)

Возможные направления расширения (если возникнет потребность):

1. **Actions / remap UI / контексты** — описано в [`full_plan_deferred.md`](full_plan_deferred.md). API `input_frame` спроектирован для аддитивности: маппинг "physical → action" надстраивается слоем сверху, потребители продолжают читать через `input_key_*` / `input_btn_*` / etc.
2. **Synthesise key event with timing semantics** для widget.cpp/gamemenu.cpp internal bridge — сейчас работает через `input_frame_inject_key_press`, рассинхрона timing нет, но если API будет нужен с tighter контролем — точка расширения там же.
3. **Mouse buttons расширения** (drag, double-click, scroll) — `input_mouse_*` есть базовый, при необходимости добавлять.
4. **DualSense touchpad finger position / gesture** — сейчас используется только `input_btn_just_pressed(17)` для click. Если потребуется finger tracking — добавить через `input_touchpad_*`.

## Связь с FPS unlock issues

**Все** input-related issue в `fps_unlock/` закрыты этой задачей:
- #5 (siren toggle) ✅ resolved
- #15 (отзывчивость управления на 20 Hz) ✅ resolved (2026-05-06)
- #20 (weapon switch responsiveness) ✅ resolved
- #23 (DualSense LED siren strobe) ✅ resolved
- [TASK] Переработка управления ✅ implemented (этот модуль)

Открытые input-related в [`new_game_planning/known_issues_and_bugs.md`](../../new_game_planning/known_issues_and_bugs.md) секция "Управление" — отдельные UX/калибровочные баги, не относятся к архитектуре input pipeline (deadzone в меню для Xbox drift, sensitivity вождения, R3 inventory snapshot, и т.п.).
