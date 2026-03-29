---
name: debug-log
description: >
  Temporary runtime debug logging to file for investigating rendering bugs,
  game state, variable values etc. Use when the user needs to run the game
  to reproduce an issue and you need to see runtime values.
---

# Debug Logging — temporary runtime logging to file

## When to use
When you need runtime debug output from the game (values of variables, call traces, render state, etc.)
and the user needs to run the game manually to reproduce the situation.

## How to log

**Always log to a file, never to stderr.** The user can't easily redirect stderr on Windows.

```cpp
// Temporary debug log — remove after investigation.
{
    static int s_dbg_count = 0;
    if (s_dbg_count < 20) {
        s_dbg_count++;
        FILE* dbg = fopen("debug_TOPIC.txt", "a");
        if (dbg) {
            fprintf(dbg, "[TAG] your message here: val=%d\n", val);
            fclose(dbg);
        }
    }
}
```

- File appears next to the exe: `new_game/build/Debug/debug_TOPIC.txt`
- Use a descriptive filename (e.g., `debug_blit.txt`, `debug_leaf.txt`)
- Limit output with a static counter to avoid huge files
- Use `"a"` (append) mode so multiple runs accumulate
- Always close the file after each write (game may crash)

## Workflow

1. Add debug log code, build (`make build-debug`)
2. Tell the user to run, reproduce the situation, and close the game
3. Tell the user: "готово, можно смотреть лог"
4. **Read the log file yourself** with the Read tool: `new_game/build/Debug/debug_TOPIC.txt`
5. Analyze, fix, remove debug code

## After investigation

**Always remove debug logging code.** Never commit it.

## Don'ts

- Don't use `fprintf(stderr, ...)` — user can't see it without stderr redirect
- Don't ask the user to run with `2> file.txt` — inconvenient on Windows
- Don't ask the user to copy-paste log output — read the file yourself
