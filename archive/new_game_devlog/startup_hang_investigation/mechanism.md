# mechanism — как работает lock/unlock в коде

Факты о коде игры: что делает `ge_lock_screen` / `ge_unlock_screen`, где вызывается. Источник — `new_game/src/` на main.

## `ge_lock_screen` — [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)

```cpp
void* ge_lock_screen()
{
    if (s_screen_locked) return s_dummy_screen;
    uint8_t* buf = ensure_screen_buffer();  // RGBA w×h×4
    // ...
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    // Y-flip in place (OpenGL bottom-up → game's top-down).
    // ...
    s_screen_locked = true;
    return buf;
}
```

- Читает из **default framebuffer** (`GL_BACK`). Никаких `glBindFramebuffer` / `glReadBuffer` в коде нет — FBO / render-to-texture не используется.
- Transfer size: `w × h × 4` bytes. При 640×480 = 1.2 MB per call — полный pipeline stall (GPU sync).
- После readpixels делает in-place Y-flip 16-byte chunks (CPU memory access на весь buffer).

## `ge_unlock_screen` — [core.cpp](../../new_game/src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp)

```cpp
void ge_unlock_screen()
{
    if (!s_screen_locked) return;
    s_screen_locked = false;

    GLuint tex = 0;
    glGenTextures(1, &tex);                  // новая texture object каждый раз
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, s_dummy_screen);
    // ... save/restore GL state ...
    ge_draw_indexed_primitive(TriangleList, verts, 4, indices, 6); // fullscreen quad
    glDeleteTextures(1, &tex);
}
```

- Создаёт новую texture object **каждый вызов** (`glGenTextures`), полный `glTexImage2D` upload (не `glTexSubImage2D`), рисует fullscreen textured quad, удаляет texture (`glDeleteTextures`).
- Выполняется безусловно при каждом lock'е — нет guard'ов, нет dedup'а.

## Per-frame call-sites в gameplay flow

| # | Файл:строка | Caller | Когда активен |
|---|-------------|--------|---------------|
| 1 | [aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp) `AENG_draw_city` | `SKY_draw_stars` | `AENG_detail_stars && !(NIGHT_flag & NIGHT_FLAG_DAYTIME)` — ночные миссии |
| 2 | [aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp) `WIBBLE` в цикле по отражениям | `WIBBLE_simple` | когда в кадре есть puddle tiles с bbox intersection |
| 3 | [font.cpp](../../new_game/src/engine/graphics/text/font.cpp) `FONT_buffer_draw` из `screen_flip` | pixel plot глифов | `FONT_message_upto > 0` (накоплены отложенные text messages) |

### Что ещё вызывает `ge_lock_screen` но не триггерит hang

- **Screenshot / video record** ([aeng.cpp](../../new_game/src/engine/graphics/pipeline/aeng.cpp) `AENG_screen_shot`) — только по нажатию `KB_S` или при `record_video=1`. Не per-frame.
- **Debug FPS overlay** (`AENG_draw_messages`) — только при `ControlFlag && allow_debug_keys`. В release flow не активен.
- **Benchmark** — вне gameplay-пути.

### Dead API

- `AENG_lock()` / `AENG_unlock()` / `AENG_flush()` — wrapper'ы над `ge_lock_screen` / `ge_unlock_screen`. Grep по `new_game/src/` не находит ни одного каллера.
