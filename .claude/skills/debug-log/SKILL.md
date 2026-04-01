---
name: debug-log
description: >
  Temporary runtime debug logging to file for investigating rendering bugs,
  game state, variable values, call traces, etc. Use this skill whenever you
  need to see what's happening inside the game at runtime — values that only
  exist during gameplay, render state during specific frames, variable changes
  over time. Trigger when: you need runtime data to diagnose a bug, the user
  reports a visual glitch or wrong behavior that requires seeing internal values,
  or when crash_log.txt and debugger attach aren't enough because you need to
  observe values across many frames. Also use when you say to yourself "I wish
  I could see what X equals during gameplay" — that's exactly what this skill is for.
---

# Debug Logging — temporary runtime logging to file

## Quick reference

```cpp
// Temporary debug log — remove after investigation.
{
    static int s_dbg_count = 0;
    if (s_dbg_count < 20) {
        s_dbg_count++;
        FILE* dbg = fopen("debug_TOPIC.txt", "a");
        if (dbg) {
            fprintf(dbg, "[TAG] message: val=%d\n", val);
            fclose(dbg);
        }
    }
}
```

Build, ask user to run and reproduce, read the log file yourself.

## Why log to file

The game runs as a Windows GUI app (SUBSYSTEM:WINDOWS) — there's no console.
stderr is redirected to stderr.log by the Makefile, but adding fprintf(stderr)
means your output mixes with other stderr content. A dedicated file per investigation
keeps things clean, and the static counter prevents runaway output if the code runs
thousands of times per frame.

## Logging pattern details

- **File location**: appears next to the exe — `new_game/build/Debug/debug_TOPIC.txt` (or Release)
- **Filename**: descriptive per investigation (e.g., `debug_blit.txt`, `debug_vb_expand.txt`)
- **Counter limit**: `s_dbg_count < 20` (or more if needed) — prevents huge files
- **Append mode**: `"a"` so multiple runs accumulate
- **Close after each write**: the game may crash at any moment — unflushed data is lost
- **Static counter**: resets only on restart, not per frame

## Workflow

1. Add debug log code to the relevant function
2. Build: `make build-debug` (or `make build-release` if the bug is release-only)
3. Tell the user to run, reproduce the situation, and close the game
4. Read the log file yourself with the Read tool — don't ask the user to paste output
5. Analyze, iterate if needed (adjust what you log, increase counter, etc.)
6. Fix the bug
7. **Remove all debug logging code** — never commit it

## Choosing the right debug tool

| Situation | Tool |
|-----------|------|
| Need values across many frames/ticks | **debug-log** (this skill) |
| Game crashed, want to know where | crash_log.txt, then `asan` skill |
| Game froze/hung, need current state | `debugger` skill (cdb attach) |
| Need to see what's on screen | `screenshot` skill |
| Suspect memory corruption | `asan` skill |
