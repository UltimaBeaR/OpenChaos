# CLAUDE.md — OpenChaos (Urban Chaos Modernization)

## What this project is

A modernization of **Urban Chaos (1999, MuckyFoot Productions)**.
The original source code is in `original_game/` — it is never changed, only read.
Goal: a new version of the game on a modern stack, keeping the original gameplay 1:1.

The original code was copied to `new_game/` and refactored iteratively (renderer, platform code, structure).
Physics and game logic are ported 1:1. `original_game/` is the behavior reference. The original resources are reused
as is. Release 1.0 has shipped (patches 1.0.x); work continues post-1.0 via the tracker.

## Repository layout

```
original_game/      — original source code (READ ONLY)
new_game/           — the modernized game (C++, OpenGL, SDL3, x64; Windows / macOS / Linux)
libDualsense/       — OpenChaos-owned standalone DualSense lib (SDL3 hidapi), add_subdirectory from new_game/CMakeLists.txt.
                      Consumers: `#include <libDualsense/ds_*.h>`. Docs: libDualsense/README.md, libDualsense/API.md
tracker/            — task tracker (skill `work-tracking`), see "Task system" below
docs/topics/        — reusable knowledge: how things work, references (debug keys, DualSense protocol, resource
                      formats, original cut features, x64 porting patterns, …)
archive/            — history of the work up to 1.0, READ ONLY reference: old stage plans (new_game_planning/),
                      devlogs and investigations (new_game_devlog/), the old issue lists (known_issues_and_bugs/,
                      incl. the resolved archive), finished stage-4 skills and tools. Not translated, not maintained.
release/            — release packaging (scripts, per-platform assets, dist/)
make/               — Makefile modules (platform.mk, configure.mk)
tools/              — helper tools
legal/              — rights history, attribution, positioning
.claude/skills/     — Claude Code skills (trigger automatically)
```

## Task system

The work is organized by the **`work-tracking` skill** — read it before writing to the tracker, opening/closing a
task, creating/changing a plan or touching the backlogs. The full methodology lives only there; here — only where
things are in this repo:

- `tracker/plans.md` — open plans ("Current" = the plan we follow); `tracker/backlog.txt` — main backlog (the user's
  focus); `tracker/backlog-background.txt` — background backlog (everything else open); `tracker/areas.md` — area
  registry; `tracker/active/` and `tracker/closed/` — `tasks/` (`_index.md`, `_tags.md`, `N-slug.md`), `tasklog/`,
  `original-prompts/`.
- `docs/topics/` — the "topics" of the skill.

Project-specific additions to the skill:
- **GitHub issues:** a task created from a GitHub issue gets the tag `gh-issue` and a link to the issue in its
  `- Related:` line. The task is the source of truth; the issue is only the input.
- **Pre-defined task-file section `## How to verify`** (the skill's exception for task files): a short, exact
  in-game check scenario — which mission/save, where to go, what to look at, what is expected. Write it for every
  bug/feature that needs an in-game check by the user, before handing the work over. Goal: the user's check takes
  a minute or two, never "play around and see".
- Bug-fixing approach: reproduce ourselves first; ask the issue author for help when unsure; prefer cheap, locally
  verifiable fixes.

How to restore context in a new session — see the skill. Don't read `archive/` proactively — only when a task needs
the history of a specific subsystem or decision.

## Target hardware and performance

**Don't optimize for a modern powerful desktop.** Target hardware:
- **Handhelds** — Steam Deck first of all, possibly a phone (Android) port in the future.
- **Weak/old PCs** — integrated graphics, old laptops, budget builds.

**Practical consequences:**
- "It's unnoticeable on my desktop" is **not an argument**. Always think "how will this be on a Steam Deck or
  integrated graphics?".
- Any CPU/GPU overhead adds up and shows exactly on weak hardware. Even "negligible" 1-2% from several small
  optimizations add up.
- Avoid unnecessary work: culling, early-outs, no CPU-transform for GPU-capable things, avoid per-draw state
  changes, batch where possible.
- When proposing a tuning value (margin, threshold, range, …) — **explicitly lean toward performance** if the
  visual quality doesn't noticeably degrade.
- Before "extend the radius/distance/range" — think about the cost and tell the user, don't hide it.

This does NOT mean pinching pennies on complex features — but "cheap and sufficient" always beats "expensive with
a margin".

## Communication style

- **Language:** talk to the user in **Russian**. Everything written into the repo (docs, tracker, skills, code
  comments, README) is in **English**.
- **A question ≠ a call to action.** If the user asks "why X?" or "can we Y?" — it's a question for understanding,
  not a request to change something. Answer it. Don't start editing until the user explicitly asks. If unsure —
  ask: "Do you want me to do it, or are we just discussing?"
- **No code edits without explicit approval.** Even an "obvious one-line fix". First describe what you propose,
  wait for "ok"/"do it"/"patch"/etc., only then edit. This applies to any project files, not only code.
- **When working through a list (review, bugs, checklist) — don't rush ahead.** After finishing one item (fix /
  discussion / decision), **stop and wait for an explicit "ok" / "next"** before moving on. No "while you test,
  let's discuss the next one" without explicit permission. The user sets the pace.
- **Explain simply, at an everyday level.** The user is a developer and understands programming, but **doesn't
  read the code** and **doesn't want to dig into implementation details** (variable/function names, file
  structure, how a mechanism works inside). Give the essence: what we do, how it changes the game's behavior, what
  the options and trade-offs are — at the level of gameplay and logic, not code. Shorter is better, as long as no
  important details are lost. If the user explicitly asks for implementation details — then go deep.

## Key rules

- **⚠️ Magic numbers — ALWAYS as named constants.** Any numeric value that carries meaning (threshold, delay,
  multiplier, count, offset, …) must be a named constant with a comment on where it came from and why this value.
  If a value derives from another (e.g. threshold = decrement × 2) — write the formula, not the computed result.
  Exceptions: obvious meaningless literals (0, 1, indices of short arrays).
  - *Why:* hardcoded numbers can't be adapted to parameter changes (new weapon, different tick rate, different
    delay) without manual hunting in many places. A constant breaks explicitly — the compiler finds it, not the
    user during testing.
  - *Applied right when writing code, not "I'll clean it up later".*
- **Any claim about the original game — only after checking. No assumptions.**
  - Sources of truth: the code in `original_game/` (read it), and the behavior of the retail releases (the user can
    check them — see "Available game versions").
  - "Most likely", "typical for games of that time", "usually in such cases" — not acceptable. Either it's verified
    in the code / by the user — or say "I don't know, need to check".
- `original_game/` — read only. **The only exception:** analytical comments with the prefix `// claude-ai:` may be
  added to ease navigation.
- Existing `// uc_orig:` comments in `new_game/` map code to its original entity — keep them when moving code.
- **⚠️ Mandatory self-review after any code changes** → skill `review`. Always, not only before a commit.
- Commits are made by the user manually — never commit yourself.
- **⚠️ Git — READ ONLY.** No mutating commands: `git checkout`, `git reset`, `git restore`, `git commit`,
  `git stash`, `git revert`, `git clean`, `git mv` — FORBIDDEN. Only reading: `git status`, `git diff`, `git log`,
  `git show`.
- **⚠️⚠️⚠️ IMPORTANT: Building — rules:**
  - The user **ALWAYS** builds themself. **DON'T build after every edit** — it's unnecessary.
  - If the user says they already built — **DON'T run the build again, EVER**.
  - If the user says it **didn't build** — build yourself and look at the errors.
  - If compile errors are likely (complex edits, new symbols, struct changes) — build yourself proactively.
  - Commands: `make build-release` / `make build-debug`, `make r` = build + run Release, `make d` = build + run Debug.
  - **⚠️ Checking the build result (clang + ninja):** DON'T use `| grep "error:"` — it misses linker errors and
    other problems. **Always** check: (1) the exit code (`echo $?` or `&& echo OK`), (2) the last lines of the
    output (`| tail -5`) — there must be a `Linking CXX executable` line. If `ninja: build stopped: subcommand
    failed` — the build failed even if the word `error` isn't found.
- **Debugging:** skills `debugger` (attach to the process, stacks, variables), `debug-log` (temporary logging to a
  file), `text-rendering` (text on screen), `screenshot` (screenshot of the game window), `debugger-install`
  (install cdb.exe), `asan` (memory errors), `perf-diag` (rendering performance).
  - **Pick the tool by situation** — the goal is the minimum of actions from the user. Crash →
    OpenChaos.crash_log.txt. Freeze → debugger attach. Need to see the screen → screenshot. Need values per tick →
    debug-log. Combine as needed.
  - **Debug output priority:** to show values/state **to the user live** — **on-screen text first** (skill
    `text-rendering`). To check something yourself without the user — `debug-log` (file) or `fprintf(stderr, ...)`.
  - **Debug keys** (`bangunsnotgames` mode, F1 legend) → `docs/topics/debug_keys.md` (list, how to enable, where to
    add new ones).
  - **Exit/crash logging:** `OpenChaos.crash_log.txt` is written next to the exe on **any** program exit:
    - ASSERT() fail (**debug build only**) → `uc_assert_fail()` in `host.cpp` → "Crash (ASSERT)" + condition + file +
      line, then abort(). In release ASSERT is **not fatal** (see below) and doesn't write the crash_log.
    - Normal exit (return/exit) → `atexit` handler in `host.cpp` → "Clean exit"
    - abort() → `SIGABRT` handler in `host.cpp` → "Crash (abort)"
    - Access violation, div-by-zero → exception filter in `crash_handler_win.cpp` → "Crash (exception)" + registers
      + stack trace
    - Ctrl+C, console close → `SetConsoleCtrlHandler` in `crash_handler_win.cpp` → "Terminated (console event)"
    - SIGSEGV/SIGFPE (Linux/Mac) → signal handler in `host.cpp` → "Crash (signal)"
    - The only thing NOT caught: `TerminateProcess()` / `kill -9` (the OS kills instantly)
    - `stderr.log` (redirected by the Makefile) starts with a timestamp `=== Session started: ... ===`
    - The `g_exit_log_written` flag prevents overwriting between handlers
  - **ASSERT()** — a working macro (not a stub). Defined in `uc_common.h` and `outro_always.h`, implementation
    `uc_assert_fail()` in `host.cpp`. **Behavior depends on the build** (flag `OC_ASSERT_FATAL` in `debug_config.h`,
    tied to `NDEBUG`):
    - **Debug — fatal:** writes OpenChaos.crash_log.txt (condition, file, line) + stderr, then abort(). When the
      assert window appears — press Abort, the details are in crash_log automatically.
    - **Release — NOT fatal:** the game doesn't crash. The record `ASSERTION FAILURE: <condition> (<file>:<line>)`
      is written **once per site** (dedup by file:line so a per-frame assert doesn't flood the log) to TWO places:
      (1) the persistent file `OpenChaos.asserts.log` in the user folder (`%APPDATA%\OpenChaos\` etc., append) —
      this is what we ask a player to send, since a player's stderr goes nowhere; (2) `stderr` (→ `stderr.log` only
      when run via make). A temporary "ASSERTION FAILURE" pops up on screen via `CONSOLE_text_en` (drawn by the
      standard message path — gameplay/pause/cutscene letterbox; in the pure menu only the log). Then execution
      continues. Draining to the screen: `uc_assert_take_pending()` (host.cpp) → drain in `ui_render.cpp`.
      **crash_log is NOT used** for non-fatal asserts (crash/exit handlers overwrite it).
    - To debug a specific assert — build debug, then it stops in the debugger.
  - **Symbolization:** `llvm-symbolizer -e build/Debug/OpenChaos.exe --relative-address <RVA>` (the
    `--relative-address` flag is required!)
- **Typical 64-bit bugs and fix patterns** → `docs/topics/x64_porting_notes.md`
- **Skills are living documents:** if useful new information, a technique or know-how worth recording in a skill
  (new or existing) comes up — offer the user to update the skill.
- **Creating and improving skills:** always use the `skill-creator` plugin (via the Skill tool).
- Before analyzing any code, check whether there is already documentation on that part (`docs/topics/`, the task's
  files, and if needed `archive/`).
- **Installing software:** never install anything without the user's **explicit, unambiguous** permission. Always
  ask and wait for confirmation.
- **Everything about this project is written inside the project** — **NEVER write to Claude's memory files**.
  Memory files aren't committed. The context must be restorable exclusively from the repository.
- **README.md → "Modernization status"** — when a significant feature or change is finished (new subsystem, bug
  fix, technology switch), update the "Modernization status" section of `README.md` right away. It's the public face
  of the project.

## Documentation rules

- Where things go — skill `work-tracking` (tracker, tasklog, original-prompts, topics) + this file (behavior rules).
- **File size:** one file = one topic, target ~100-350 lines. If a file grows past 400 lines — split it.
- **No duplication:** if a topic is described in one file, another file only links to it.
- **File names** — no numbering (not `00_overview.md` — just `overview.md`).
- Record important decisions right away in the right place — the conversation context is lost between sessions.

## IMPORTANT: the codebase is a pre-release version!

`original_game/` is **NOT the final release** but an earlier pre-release version of the code.
- The final PS1/PC game was finished after this codebase was frozen
- Some features are commented out or unfinished here, but **work** in the final release
- Some bugs of this version are fixed in the final
- **Don't assume "commented-out code = cut feature"** (see `docs/topics/original_cut_features.md`)
- Reference: **both release versions: Steam PC AND PS1**
- When in doubt — ask the user (played the final on PS1, can check PC via longplays)

## Available game versions (for comparison and debugging)

**Resources** (textures, models, levels) — the same for ALL versions, from the retail PC game. Fully compatible with
all builds.

### Retail PC (final disc, Steam)
- **Sources:** no. **Binaries:** yes, the user has them.
- **Purpose:** the reference for visuals and behavior. The final version, most pre-release bugs fixed by MuckyFoot.
  The user can run it and check any moment.

### PieroZ v0.2.0
- **Sources:** yes, locally `C:\WORK\MuckyFoot-UrbanChaos_piero` (at tag `v0.2.0`). **Don't build** here — read only.
- **Binaries:** yes, downloaded from the GitHub releases page; match tag `v0.2.0`.
- **GitHub:** `github.com/PieroZ/MuckyFoot-UrbanChaos`
- **Base:** the same pre-release sources as our `original_game/` (commits up to `74749db` are identical), plus
  PieroZ's work on top (D3D6, x86, MSVC).
- **Purpose:** a reference of fixes. PieroZ fixed a number of pre-release bugs (z-order, leaves, characters).

### original_game/ (MuckyFoot pre-release sources)
- **Sources:** yes, `original_game/` (READ ONLY).
- **Binaries:** theoretically buildable, but not recommended — painful, and may crash at startup (no
  `GetTickCount` overflow fix, which we have).
- **Purpose:** code comparison only.

### Branch `initial_working_d3d_build`
- **Sources and binaries:** yes, builds and runs.
- **Inside:** a practically unchanged original — only the switch to Clang and dead-code removal. D3D6, x86.
- **Purpose:** baseline for bugs. If a bug reproduces here → it comes from MuckyFoot's pre-release sources.

### Branch `last_working_d3d_build`
- **Sources and binaries:** yes, builds and runs.
- **Inside:** D3D6, x86. Includes: the render-engine abstraction, the source restructuring (stage 4), bug fixes.
  **Does NOT include:** the x64 switch and everything after it.
- **Purpose:** the last D3D6 build. To check regressions from the OpenGL/x64 switch — if a bug is here but not in
  `initial_working_d3d_build`, it was introduced by the refactoring (stages 3-7).

### Branch `main` (current)
- **Sources and binaries:** yes, `make build-release` / `make build-debug`.
- **Inside:** OpenGL, x64, Windows + macOS + Linux. All of the project's work.

**When to use:** when investigating bugs — compare behavior between versions. If a bug is in
`initial_working_d3d_build` → it's from the pre-release sources. If only in `main` → introduced by the refactoring.
Suggest the user check a specific version when it can speed up debugging.
