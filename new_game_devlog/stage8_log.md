# Stage 8 — Кросс-платформа (девлог)

## 2026-03-30: SDL3 окно + GL контекст + event loop

### Что сделано

Заменили Win32 оконную систему на SDL3. Окно, GL контекст и event loop теперь через SDL3 bridge.

**Изменённые файлы:**

| Файл | Что изменилось |
|------|---------------|
| `sdl3_bridge.h/cpp` | Добавлены bridge-функции: окно (`create/destroy/show/set_size/get_size/get_drawable_size/get_native_handle`), GL контекст (`create/destroy/swap/get_proc_address`), event loop (`SDL3_Callbacks` + `poll_events`), таблица маппинга SDL_Scancode → Win32 Set 1 scancodes |
| `gl_context.cpp/h` | Полностью переписан: WGL → SDL3 bridge. Из ~180 строк Win32-specific стало ~60 строк |
| `host.cpp/h` | `SetupHost`: SDL3 окно вместо `RegisterClass`+`CreateWindowEx`. `LibShellActive`: `sdl3_poll_events` вместо `PeekMessage`/`GetMessage`. Event callbacks конвертируют SDL в формат `KeyboardProc`/`MouseProc`. `HOST_run` принимает `(int argc, char* argv[])` вместо WinMain параметров |
| `main.cpp` | `WinMain` → обычный `main()` + `#undef main` (макрос в uc_common.h) |
| `mouse.cpp` | `RecenterMouse` через bridge (`sdl3_window_get_center` + `sdl3_warp_mouse_global`) вместо Win32 API |
| `CMakeLists.txt` | `/ENTRY:mainCRTStartup` для main() при SUBSYSTEM:WINDOWS (без консоли) |

**Ключевые решения:**

- **Bridge-паттерн сохранён:** SDL3 хедеры по-прежнему только в `sdl3_bridge.cpp` (из-за `/Zp1`). Все новые функции через `sdl3_bridge.h`
- **hDDLibWindow сохранён:** устанавливается через `sdl3_window_get_native_handle()` — legacy код (game_tick, widget, elev) продолжает работать. Полная очистка — follow-up
- **Scancode маппинг:** SDL_Scancode (USB HID) → Win32 Set 1 (KB_* коды). ~80 клавиш. Keyboard/Mouse proc пока принимают Win32 message codes (WM_KEYDOWN и т.д.) — полная кросс-платформенность input будет следующим шагом
- **Gamepad events:** `sdl3_poll_events` теперь единый event loop (SDL_PollEvent). Gamepad connect/disconnect events сохраняются во внутреннюю очередь и читаются через `sdl3_gamepad_poll_event` как раньше
- **Окно фиксированного размера** (без SDL_WINDOW_RESIZABLE)

### Что осталось для полной кросс-платформенности

- Input: keyboard.cpp / mouse.cpp всё ещё используют Win32 типы (WM_*, LPARAM)
- hDDLibWindow: 4 места за пределами окна/GL (game_tick, widget, elev, mouse — mouse уже мигрирован)
- Time(): пока через GetLocalTime/GetTickCount
- host.cpp: windows.h включен безусловно
- wind_procs.cpp: DDLibShellProc мёртвый код (никто не вызывает), убрать при cleanup
- host_globals: часть глобалов (DDLibClass, hDDLibAccel и др.) больше не нужна

### Замечено

- Периодически появляются assertion failure диалоги. Наш `ASSERT` макрос — no-op (`{}`). Стандартный `assert()` нигде не вызывается. Источник не определён — возможно сторонние библиотеки (SDL3, OpenAL, CRT). Нужен скриншот при следующем появлении для идентификации
