# Urban Shakedown Cinematic Crash — Investigation Log

Сессия: 2026-04-01/02. Баг в known_issues_and_bugs.md.

## Симптомы

- **Миссия:** Urban Shakedown, вступительный синематик (EWAY camera)
- **Когда:** примерно в конце синематика, воспроизводится стабильно
- **Только OpenGL бэкенд** — на D3D6 не крашит
- **Проявления нестабильны:** ACCESS_VIOLATION в NVIDIA DLL, CRT debug assert (heap corruption), тихое завершение процесса
- Если проскипать и перезагрузить миссию — при повторном просмотре не крашит
- При сильных тормозах (glFinish на каждый draw) не воспроизводится — timing-dependent

## Исследование

### 1. Crash handler не ловил краш

Crash handler (`SetUnhandledExceptionFilter`) не срабатывал при "тихом" закрытии окна.
**Причина:** `exit()` и clean return из main не проходят через exception filter.
**Фикс:** добавлены `atexit`, `signal(SIGABRT)`, `SetConsoleCtrlHandler` — теперь crash_log.txt пишется при ЛЮБОМ завершении.

### 2. ASSERT был пустышкой

`ASSERT(e)` в `uc_common.h` и `outro_always.h` раскрывался в `{}` (no-op). ~492 вызова по кодовой базе никогда не проверяли условия.
**Фикс:** рабочий ASSERT — пишет в crash_log.txt (файл, строка, условие) и abort(). При активации обнаружились ~10 файлов с мёртвыми ассертами (ссылки на переменные из другого scope) — пофикшено (удалены или добавлены includes).

### 3. CRT assert не ловился

Стандартный CRT assert (`_CrtDbgReport`) показывал диалог и вызывал abort(). Текст ассерта терялся.
**Фикс:** `_CrtSetReportHook` перехватывает CRT reports → пишет в crash_log.txt + stderr. `_set_abort_behavior(0, _WRITE_ABORT_MSG)` подавляет abort-диалог.

### 4. Crash handler не записывал при stack overflow

Exception handler использовал fprintf/localtime (CRT) — при stack overflow CRT недоступен, файл создавался пустым.
**Фикс:** первичная запись через Win32 API (CreateFileA + WriteFile + GetLocalTime + wsprintfA) — минимум стека, без CRT.

### 5. `ge_vb_expand` — use-after-free

CRT assert `_CrtIsValidHeapPointer(block)` в `free_dbg` → `ge_vb_expand`.
**Причина:** `ge_vb_expand` делал `free(buf->data)` на указателе, который мог быть уже freed или повреждён.
**Контекст:** в D3D6 `ExpandBuffer` получает **новый** VertexBuffer объект и **ReleaseBuffer** старый. В OpenGL — мутирует тот же slot, меняя `data` pointer. Если кто-то кешировал старый pointer — use-after-free.
**Фикс:** убран `free()` из `ge_vb_expand` и `ge_vb_alloc`. Старая память leak'ается (bounded: 32KB per expand, pool максимум 512 slots).

### 6. Shared VBO hammering

OpenGL бэкенд использовал один `s_vbo` для всех draw calls. `glBufferData` вызывалась сотни раз за кадр с разными размерами на одном VBO. В D3D6 каждый VertexBuffer — отдельный IDirect3DVertexBuffer.
**Фикс:** per-slot VBO/EBO/VAO — каждый GLVertexBuf slot получает свой GL буфер. VBO аллоцируется по power-of-2 и переиспользуется через `glBufferSubData`.

### 7. Uninit heap data

`malloc` в `ge_vb_alloc` не инициализировал буфер. В debug — `0xCDCDCDCD`, при отправке в GPU — краш.
**Фикс:** `calloc` вместо `malloc`.

### 8. Корневая причина — ~~heap corruption в legacy коде~~ **НАЙДЕНА: ge_vb_expand overflow**

**ASan нашёл root cause (2026-04-02):**
- `ge_vb_alloc` переиспользует VB слот от предыдущей крупной аллокации
- Устанавливает маленький `logsize` (запрошенный), но `capacity` остаётся большой (от прошлого использования)
- `ge_vb_expand` вычисляет `needed = (1 << (logsize+1)) * 32` — маленький буфер
- `calloc(needed)` аллоцирует 65536 байт
- `memcpy(new_data, buf->data, buf->capacity)` копирует 131072 байт — **вдвое больше!**
- Overflow портит CRT heap metadata → краши в произвольных местах

**Фикс:** в `ge_vb_expand` — если `buf->capacity >= needed`, буфер уже достаточно большой, просто обновить `logsize` без реаллокации.

### 9. Попутные баги найденные ASan

ASan останавливается на первой ошибке, поэтому по пути к целевому крашу пришлось исправить 5 других багов:

1. **ReadSquished overread** (tga.cpp) — чтение 2 байт за концом буфера при распаковке текстур. Фикс: +2 padding в FileClump::Read.
2. **strcpy overlap** (xlat_str.cpp) — `strcpy(scan+1, test)` с перекрывающимися буферами. Фикс: `memmove`.
3. **arctan_table off-by-one** (vehicle.cpp) — `<=` вместо `<` в цикле инициализации. Фикс: `ii < 2 * WHEELTIME + 1`.
4. **SIN table overflow** (figure.cpp, draw_flames) — индекс до 10000 при размере таблицы 2560. Фикс: маска `& 2047`.
5. **CMatrix33 tmat use-after-scope** (figure.cpp) — указатель на локальную переменную внутри цикла, используемый после выхода из scope. Фикс: массив `tmat[MAX_RECURSION]` на уровне функции.

## Что проверяли и исключили

| Гипотеза | Результат |
|----------|-----------|
| Uninit vertex data (0xCDCDCDCD, NaN) | Нет — calloc, проверки чисты |
| Индексы за пределами буфера | Нет — max_idx < alloc_verts |
| VB buffer overrun (canary) | Нет — canary цел (corruption была в ge_vb_expand, не в VB data) |
| Невалидный VB handle (magic) | Нет — magic совпадает |
| Невалидный buf->data (page touch) | Нет — все страницы читаемы |
| Невалидная текстура (glIsTexture) | Нет — текстуры валидны |
| GL errors перед крашем | Нет — glGetError чист |
| VB data freed (use-after-free в expand) | Да, пофикшено, но краш остался |
| Shared VBO orphaning | Пофикшено (per-slot), краш остался |
| PointAlloc overrun | Нет — ASSERT(m_VBUsed + num_points <= GetVBSize()) не сработал |
| **ge_vb_expand logsize/capacity mismatch** | **ДА — ROOT CAUSE. ASan поймал.** |

## Архитектурные изменения (оставлены)

1. **Per-slot VBO/EBO/VAO** — каждый GLVertexBuf slot имеет свои GL объекты (как D3D6)
2. **calloc** вместо malloc для VB data
3. **Нет free()** в expand/alloc — leak bounded by pool size
4. **Max-index upload** — `glBufferData` загружает только `(max_idx+1)*32` байт
5. **Exit/crash logging** — crash_log.txt при любом завершении
6. **ASSERT** — рабочий макрос вместо no-op
7. **CRT report hook** — перехват CRT assert текста
8. **ge_vb_expand capacity check** — skip realloc if buffer already large enough
9. **ASan support** — `make configure-asan` для поиска memory bugs (скилл `.claude/skills/asan/`)
