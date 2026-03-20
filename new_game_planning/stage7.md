# Этап 7 — Новый рендерер

**Цель:** заменить DirectX рендерер на OpenGL/Vulkan через абстракцию.

## Стратегия

- Ввести интерфейс `IRenderer` на границе DDEngine → DDManager
- DDEngine (bucket sort, pages, polygon submission) остаётся
- DDManager заменяется реализацией `IRenderer` на OpenGL или Vulkan
- Параллельно: перенос мутаций стейта из рендерера в update фазу
  (конкретные места: `drawxtra.cpp`, `PYRO_draw_armageddon()`)

**Выбор API:** OpenGL 4.1 vs Vulkan+MoltenVK — решить перед стартом этапа.
Подробно → [`tech_and_architecture.md`](tech_and_architecture.md)

**Критерий:** тестовая инфраструктура из этапа 6 проходит без регрессий.
