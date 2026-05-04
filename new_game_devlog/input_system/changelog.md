# Input System — журнал работ

Хронологический лог изменений по задаче переработки input-системы. Самые свежие записи сверху.

Контекст и план → [current_plan.md](current_plan.md). Большой план с actions/ремапом → [full_plan_deferred.md](full_plan_deferred.md).

---

## 2026-05-05 — Папка создана, scope согласован

- Создана папка `new_game_devlog/input_system/` для всех работ по переработке управления.
- Старый `fps_unlock/input_system.md` (полный план с actions/remap/контекстами) перенесён сюда как [`full_plan_deferred.md`](full_plan_deferred.md) — отложен.
- Написан [`current_plan.md`](current_plan.md) под текущую (урезанную) задачу: модуль `input_frame` с edge-detect, sticky-флагами, stick-as-direction emulation и universal auto-repeat.
- Кросс-ссылки в `fps_unlock/*.md` (5 мест) обновлены на новый путь.
- Implementation ещё не начат — следующий шаг: Phase 1 (скелет модуля).
