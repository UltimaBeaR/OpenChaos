---
name: asan
description: >
  AddressSanitizer (ASan) — runtime memory error detector for finding heap overflows,
  use-after-free, stack-use-after-scope, global buffer overflows, and other memory bugs.
  Use this skill whenever crash investigation suggests memory corruption: crash_log.txt
  shows heap corruption, access violations in system DLLs (NVIDIA, ucrtbase), CRT debug
  asserts on heap, inconsistent/variable crash symptoms, or crashes that move around between
  runs. Also use when the user says "memory bug", "heap corruption", "asan", "sanitizer",
  "buffer overflow", "use-after-free", or when you suspect memory issues yourself during
  debugging. Even if the user doesn't mention ASan explicitly, if the crash pattern looks
  like memory corruption (random crashes, heap asserts, driver DLL crashes), proactively
  suggest using this skill.
---

# AddressSanitizer (ASan) — memory error detection

## Quick reference

```bash
make configure-asan          # enable ASan (one-time)
make build-release           # build (prefer Release over Debug)
make run-release             # run — ASan output goes to stderr.log
# ... reproduce the bug, read stderr.log ...
make configure        # disable ASan when done
make build-release           # rebuild clean
```

ASan stops at the **first** error. Fix it, rebuild, re-run, repeat until you reach the target bug.

## When to use

- **Heap corruption**: CRT debug asserts like `_CrtIsValidHeapPointer`, crashes inside system DLLs (NVIDIA, ucrtbase), inconsistent crash symptoms
- **Access violations** with no obvious cause in our code
- **Use-after-free** suspicion (works with freed memory, random data corruption)
- **Buffer overflows** — both heap and stack/global
- **Variable crash locations** — crashes in different places each run suggest memory corruption elsewhere

## Environment

- **Compiler**: Clang 20+ (LLVM), installed at `C:\Program Files\LLVM\`
- **ASan runtime**: `C:\Program Files\LLVM\lib\clang\22\lib\windows\clang_rt.asan_dynamic-x86_64.dll`
- **Build system**: CMake + Ninja Multi-Config via Makefile
- **CMake option**: `ENABLE_ASAN` (ON/OFF) in `new_game/CMakeLists.txt`

## Setup details

### configure-asan

`make configure-asan` runs CMake with `-DENABLE_ASAN=ON` and copies the ASan DLL to build directories.

Under the hood:
- `CMakeLists.txt` adds `-fsanitize=address -fno-omit-frame-pointer` to compile and link flags
- ASan is applied to the `OpenChaos` target
- `clang_rt.asan_dynamic-x86_64.dll` is copied next to the exe

### Why Release, not Debug

Debug CRT (`ucrtbased.dll`) has its own allocator hooks that conflict with ASan, causing a false positive `bad-free` at process exit:
```
ERROR: AddressSanitizer: attempting free on address which was not malloc()-ed
(ucrtbased.dll / MSVCP140D.dll in stack trace)
```
This is NOT a real bug — it's the debug CRT's static destructors freeing memory ASan didn't track. The actual memory bugs are identical in both configs. **Always prefer Release** to avoid wasting time on this.

### Where ASan output goes

ASan writes to stderr, which the Makefile redirects to `stderr.log`. On error, ASan calls `abort()` — our crash handler also writes crash_log.txt, but the real diagnostic info is in **stderr.log**.

## Reading ASan output

```
==PID==ERROR: AddressSanitizer: <error-type> on address 0x... at pc 0x...
<READ|WRITE> of size N at 0x... thread T0
    #0 0x... in FunctionName (OpenChaos.exe+0x...)    <- crash site
    #1 0x... in CallerFunction (OpenChaos.exe+0x...)  <- call chain
    ...

0x... is located N bytes after M-byte region [0x..., 0x...)
allocated by thread T0 here:                       <- who allocated the buffer
    #0 0x... in allocator
    #1 0x... in AllocatingFunction
    ...
```

### What to look at first

1. **Error type** — tells you the class of bug
2. **READ/WRITE of size N** — what operation and how many bytes
3. **Stack trace #0** — exact function where the bad access happens
4. **"located N bytes after M-byte region"** — buffer size and how far past the end
5. **"allocated by ... here"** — who allocated the buffer (crucial for size mismatch bugs)

### Error types

| Error type | Meaning | Typical cause |
|-----------|---------|---------------|
| `heap-buffer-overflow` | Write/read past end of malloc'd buffer | Wrong size calc, off-by-one, memcpy with wrong length |
| `stack-buffer-overflow` | Write/read past end of stack array | Array index out of bounds |
| `global-buffer-overflow` | Write/read past end of global array | Array index out of bounds, off-by-one in loop |
| `heap-use-after-free` | Access to freed memory | Dangling pointer, double free |
| `stack-use-after-scope` | Access to variable after its scope ended | Pointer to local variable stored and used later |
| `stack-use-after-return` | Access to stack frame after function returned | Returning pointer to local |
| `strcpy-param-overlap` | strcpy with overlapping src/dst | Use memmove instead |
| `bad-free` | free() on non-malloc'd address | Double free, freeing stack/global, or debug CRT false positive |

### Function names in Release builds

Release builds may show inaccurate function names (nearest symbol). If a function name looks wrong (e.g. `TGA_save` for what's actually `ReadSquished`), look at the call chain context to determine the real function. The RVA offset within the binary can help disambiguate.

## Workflow for multi-bug sessions

ASan aborts on the first error it finds. When hunting a specific bug (like "crash on mission X"), you may need to fix several unrelated bugs first to get past startup and reach the target code path. This is normal and expected — each bug you fix along the way is a real bug worth fixing.

1. Build with ASan, run, read stderr.log
2. Fix the reported bug (even if it's not the one you're looking for)
3. **Document the fix** (mandatory, every time):
   - Add a comment at the fix site in code explaining what was wrong and why (e.g. `// ASan fix: missing null terminator, original bug`)
   - Update `known_issues_and_bugs.md` — add the bug to the "Рандомные краши" entry or create a new entry
   - Add the bug to the "Bugs found with ASan" section at the bottom of this skill file
4. **Ask user to verify** — tell the user the simplest way to reproduce the situation that triggered the bug (e.g. "step on a mine", "restart a failed mission"). The user rebuilds and checks that ASan no longer aborts on the same action. Keep it short — one sentence with exact steps.
5. Rebuild, re-run
5. Repeat until you reach the target crash
6. When done, disable ASan (`make configure`) and verify the fix works in normal builds

## Performance

ASan adds ~2-3x overhead. The game will be noticeably slower. Timing-dependent bugs may behave differently: they become MORE likely if the bug is a race condition, LESS likely if it depends on fast execution.

## Bugs found with ASan (2026-04-02, Urban Shakedown investigation)

Real bugs in legacy code that ASan caught. These document patterns to watch for in this codebase.

### 1. ge_vb_expand heap-buffer-overflow (ROOT CAUSE of Urban Shakedown crash)
- **Bug**: `ge_vb_alloc` reuses a VB slot from a previous larger allocation, sets small `logsize` but keeps large `capacity`. `ge_vb_expand` computes new size from `logsize` (small), allocates small buffer, but `memcpy`s `capacity` bytes (large) -> write past end.
- **Size**: calloc 65536, memcpy 131072
- **Fix**: if `buf->capacity >= needed`, skip reallocation — buffer is already large enough.
- **File**: `backend_opengl/game/core.cpp`, `ge_vb_expand()`

### 2. ReadSquished overread (tga.cpp / file_clump.cpp)
- **Bug**: bit-stream decoder reads 1 UWORD past compressed data on final iteration.
- **Size**: 2 bytes past 83860-byte buffer
- **Fix**: +2 padding in `FileClump::Read` allocation (`new UBYTE[Lengths[id] + 2]()`).
- **Pattern**: benign overread in original 32-bit code, only caught by ASan.

### 3. strcpy-param-overlap (xlat_str.cpp)
- **Bug**: `strcpy(scan + 1, test)` where both point into same buffer.
- **Fix**: `memmove(scan + 1, test, strlen(test) + 1)`.
- **Pattern**: in-place string compaction — always needs memmove, not strcpy.

### 4. arctan_table off-by-one (vehicle.cpp)
- **Bug**: `for (ii = 0; ii <= 2 * WHEELTIME + 1; ii++)` reads `arctan_table[71]` on array of size 71.
- **Fix**: `ii < 2 * WHEELTIME + 1`.
- **Pattern**: classic `<=` vs `<` off-by-one on array bounds.

### 5. SIN table overflow (figure.cpp, draw_flames)
- **Bug**: `SIN(trans)` where `trans = abs(dy - y) * 20` can reach 10000. `SIN(a)` = `SinTable[a]` with no masking, table size 2560.
- **Fix**: `SIN(trans & 2047)`.
- **Pattern**: SIN/COS macros are raw array lookups — all callers must mask the index. Missing mask = out-of-bounds read.

### 6. CMatrix33 tmat use-after-scope (figure.cpp)
- **Bug**: `CMatrix33 tmat` declared inside `if` block, pointer stored in `pDHPR1Inc->parent_base_mat`. Used in next loop iteration after scope ends.
- **Fix**: array `CMatrix33 tmat[MAX_RECURSION]` at function scope, indexed by `recurse_level`.
- **Pattern**: pointer to local variable survives the scope. Watch for any `&local_var` stored into a struct that outlives the block.

### 7. action_dying missing null terminator (input_actions.cpp) — 2026-04-11
- **Bug**: `action_dying[]` had only one entry `{ ACTION_BLOCK_FLAG, 0, INPUT_MASK_ACTION }` without a `{ 0, 0, 0 }` terminator. `find_best_action_from_tree()` iterates `while (action_options->Action)` — reads 1 byte past array into global redzone.
- **Repro**: step on a mine (player enters dying state) → `find_best_action_from_tree(ACTION_DYING, ...)` → overread.
- **Fix**: added `{ 0, 0, 0 }` terminator to `action_dying[]`.
- **Original bug**: same in `original_game/fallen/Source/interfac.cpp:558` — no terminator. Worked on x86 by luck (padding/next global contained zeros).
- **Pattern**: all `ActionInfo[]` arrays must be null-terminated. If adding new action arrays, always end with `{ 0, 0, 0 }`.

### 8. ELEV_load_name strcpy self-overlap (elev.cpp) — 2026-04-11
- **Bug**: `strcpy(ELEV_fname_level, fname_level)` where `fname_level == ELEV_fname_level` — strcpy of buffer into itself. Original comment: `// I hope this is OK` (it wasn't).
- **Repro**: fail a mission → restart mission → `game_init()` calls `ELEV_load_name(ELEV_fname_level)` → strcpy self-overlap.
- **Fix**: skip copy when `fname_level == ELEV_fname_level`.
- **Original bug**: same in `original_game/fallen/Source/elev.cpp`. Worked on MSVC x86 where strcpy tolerates overlapping identical pointers.
- **Pattern**: any function that copies its argument into a global — check if callers ever pass that same global as the argument.

### 9. MUSIC_play_the_mode lookup_table overflow (music.cpp) — 2026-04-11
- **Bug**: `lookup_table[14][3]` (modes 1-14), but `MUSIC_mode_process` sets `music_current_mode = 14 + amb` for warehouse ambience → `lookup_table[14+]` reads past end.
- **Repro**: enter a building/club with warehouse ambience (e.g. club on Gatecrasher mission).
- **Fix**: early return in `MUSIC_play_the_mode` when `mode > 14`.
- **Original bug**: same in `original_game/fallen/Source/music.cpp:160,295`. Table has 14 entries, mode can exceed 14.
- **Pattern**: static lookup tables indexed by mode/type — always verify the index is within bounds, especially when callers can set the index to dynamic values.

### 10. PCOM_set_person_ai_kill_person ASSERT(0) for civilians (pcom.cpp) — 2026-04-11
- **Bug**: `ASSERT(0)` when `p_person->PersonType == PERSON_CIV`. Civilians can be sent into combat AI (e.g. inside clubs). Original ASSERT was empty in release builds.
- **Repro**: enter the club on Gatecrasher mission, bандит стреляет внутри клуба рядом с гражданскими → civilian попадает в combat AI.
- **Fix**: replaced `ASSERT(0)` with `return` — civilians skip combat logic.
- **Original bug**: same in original code. ASSERT was no-op in release, civilian just fell through into combat code (harmless).
- **Pattern**: ASSERT(0) in original code often guards "shouldn't happen" cases that DO happen in gameplay. Consider replacing with early return or skip instead of keeping fatal ASSERT.
