---
name: work-tracking
description: >-
  Methodology for the project's persistent working memory: the task tracker (tasks/bugs/todos/improvements),
  PLANS (tasks of type "plan": a goal + an ordered list of step-tasks with statuses; open ones are listed in
  plans), the priority backlogs, the detailed process log (tasklog), the knowledge base (topics), task areas and
  the "what goes where" rule. Read it BEFORE writing to the project's working docs, opening/closing a task,
  creating or changing a plan (the user talks about the order of work, "what's next", "first… then…" — offer to
  make it a plan), editing the backlogs (main and background) or a log (any touch of the backlogs — run
  `backlog_tidy.py`), choosing a task type (analysis / feature / bug / plan), or when you need to know where
  things live and how they are organized. Portable system — the concrete paths and folder names of this project
  are in its CLAUDE.md.
---

# Project work organization system

A **portable methodology** for the project's persistent memory (primarily for you, the assistant — to restore
context quickly in a new session). Only the SYSTEM is described here, not tied to a specific project. **Where the
folders physically live and what they are called in the current project — see its `CLAUDE.md`.** Below, paths
use placeholder names (`plans`, `tasks/`, `tasklog/`, `topics/`, `backlog`).

**The tracker is split into two AREAS — `active/` and `closed/`.** Each has its own `tasks/`, `tasklog/`,
`original-prompts/` and `_tags.md`. Why: active tasks are usually looked at SEPARATELY from closed ones — mixing
them clutters the overview. Closing/reopening a task = MOVING its files between the areas (details — §Task
tracker and §Lifecycle).

## Six memory locations + decision rule

Write to exactly one place per the rule. Never duplicate a fact in two places.

| Location | What goes there | Granularity |
|---|---|---|
| **`CLAUDE.md`** | Short imperative behavior rules. Plus project specifics: where this system's folders live. | Always-on. |
| **plans** (list of plans) | The list of OPEN plans — tasks of type `plan` (§Plans): which one we follow now, which ones before it, one line per plan. Steps and statuses live in the plan file. | One short list per project. |
| **tasks** (tracker) | Tasks: bugs, todos, improvements, epics. **What** to do, status, important notes — YOUR wording. | One task = one number. |
| **original-prompts** | **The user's original requests VERBATIM** (1:1): the initial one + all refinements, chronologically. Yours — only meta-notes (when/under what conditions it was added). The source of truth for INTENT, separate from your interpretation. | One file per task (that has a user request). |
| **tasklog** | **Detailed process log** of a task: blow-by-blow what was done, to reconstruct the course of changes and history. | One file per task/epic that needs a detailed log. |
| **topics** | Reusable knowledge not tied to a moment: patterns, case studies, "how X works". | By topic, not by time. |

**Test before writing:**
1. A short imperative behavior rule? → `CLAUDE.md`.
2. An open task/bug/improvement (to be done)? → task (+ a line in the backlog).
3. **A verbatim user request/idea (initial or refinement)?** → that task's original-prompts.
4. A detailed chronicle "what I did on the task step by step"? → that task's tasklog.
5. Reusable knowledge/pattern for the future? → topics.
6. Order of work, "where we're heading", a sequence of tasks toward a goal? → **plan** (a task of type `plan`,
   §Plans); its line goes into `plans`.

## Task vs process log — THE MAIN separation

Keep two different kinds of records about one task apart:

- **Task file** (`<area>/tasks/<N>-slug.md`, where `N` is the task number) — the "header": **description +
  important notes** (key decisions, what's important to know, status). Compact, durable. NOT blow-by-blow.
- **Process log** (`<area>/tasklog/<N>-slug.md`, name 1:1 with the task) — **ALL the context of what was done**
  on the task: chronology of steps, what was tried, what didn't work, pitfalls. To reconstruct the course of
  changes and history. Append-only. (`<area>` = `active/` while open, `closed/` after closing.)

The rule is simple:

- **All process/chronology → tasklog.** Period. The task file holds only the durable part: description +
  important notes + a link to the tasklog. **There is NO separate log section in the task file.**
- The only exception — if the project has a **PRE-DEFINED** clear fixed criterion "data of kind X is kept in the
  task file for quick scanning" (the criterion makes it unambiguous what goes there and what doesn't). Then the
  criterion is written as an explicit rule (in the project's `CLAUDE.md`) and the section is kept strictly by it.
  **No pre-defined criterion → only the tasklog** (don't decide ad hoc per task — that's vague).

## The user's original requests — a separate `original-prompts/` area (mandatory)

**The user's original wording is stored VERBATIM in a separate file** `<area>/original-prompts/<N>-slug.md`
(name 1:1 with the task, like the tasklog). Separate — so that the ESSENCE of the task (your wording) doesn't mix
with the originals, and there is a clean source of truth for intent.

**What and how to write:**
- **Only requests that affect the ESSENCE of the task** (the idea / what we do / why / how — the intent and its
  substantive refinements). DON'T write process stuff here: priority / where in the backlog / "after X",
  requests about diagnostics/tooling/deploy, organizational remarks — these are not about the essence.
- When creating a task AND on EVERY substantive refinement — **append the request to this file as close to 1:1
  (verbatim) as possible**, chronologically: initial request, then all refinements of the intent.
- Never overwrite earlier wording — only append. Never edit the original to fit your plan.
- Your insertions are allowed ONLY as **meta-notes** (italic/in parentheses): when it was added (date), under what
  conditions/in response to what, a link to the related moment. Don't paraphrase the user's quote itself.
- Quotes stay in the language the user wrote them in (do not translate them).
- In the task file — DON'T duplicate the original; link to the original-prompts file (see the template).
- Tasks migrated from older systems, for which no verbatim request exists, have no original-prompts file — that's
  fine; never invent a "quote".

**When you MUST read a task's `original-prompts`:**
- before writing/changing your plan or the task description — check you're following the original idea;
- when you're **confused** about the task / feel you're going the wrong way;
- when the user **complains** that you're doing the wrong thing — re-read the original first;
- when the user **asks a question about the task** — they remember it by THEIR request, not by your
  interpretation (which you may have distorted) → answer after checking the original.

**Why:** your retelling/plan is already an interpretation; you may paraphrase (especially after iterations),
shift emphasis or lose a detail, and the user forgets the original thought over time. Don't edit the original to
fit the plan — fit the plan to the original (or explicitly record the discrepancy and ask).

## Task tracker

The tracker is split into two AREAS, each with its own `tasks/` folder (+ its own `tasklog/`):
- **`active/tasks/`** — open tasks;
- **`closed/tasks/`** — closed ones.

In EACH area, `tasks/` contains `_index.md` (the registry of its area), `_tags.md` (the glossary of ITS area) and
the heavy `<N>-slug.md` files.

- **`_index.md`** — the registry of tasks of ITS area.
  - `active/`: table `# | Area | Status | Tags | Description`.
  - `closed/`: table `# | Area | Closed | Tags | Outcome` — closing date + a compact outcome **in one line** (with
    a link to the log/commit). The number and area are preserved.
  - A **short** task lives ENTIRELY in its row.
  - A **heavy** one (long description / important notes) → a file `<N>-slug.md` in `tasks/` of the SAME area; the
    row holds a summary + a number-link. (The detailed process log is not here but in the tasklog.)
  - **Task ID = the next free integer** (max among open+closed, BOTH areas, + 1). Monotonic, **never reused**,
    **no leading zeros**. Number order ≠ priority ≠ chronology — it's just an ID.
  - Task statuses in `active/`: 🔵 open · 🟡 in progress · ❄️ deferred. In `closed/` there's no status — the
    closing date in the "Closed" column instead; reflect the nature of the closing (done / wontfix) in the outcome
    text.
- **`_tags.md`** — glossary of tags, **scoped per area** (each area has its own). **Type** — one to three, see
  "Task type" below; EVERY task has one, including epics and closed ones. **Topic** — 0..N (added as needed; not to
  be confused with the task area — see below). Special tag `epic` — a large multi-stage task whose process log is
  a separate tasklog file. Tag `gh-issue` — the task came from a GitHub issue (see the project `CLAUDE.md`).
  - **Scope rule:** an area's `_tags.md` lists only tags that occur in tasks of THAT area. A tag present in both
    active and closed is **duplicated** (with its description) in both. A tag was on a single task and it closed →
    the tag **moves** from `active/_tags.md` to `closed/_tags.md` (and back on reopen). **Update the `_tags.md` of
    both areas on every task move.**
- 🔴 **area — the task's main domain, EVERY task has exactly one** (in both `active/` and `closed/`; no task
  without an area). It's not a tag and not a topic: the area answers "what does the task belong to", so the
  backlog immediately shows groups that can be moved as a whole. Not necessarily a subproject — just a group name.
  - **The area registry is a separate tracker file** (where — in the project `CLAUDE.md`): a list of
    ``- `name` — description``. The name is one word, **never with spaces** (hyphens are fine). A new area is
    first added there, with a description, and only then assigned to a task.
  - A task belongs to several — pick the one it belongs to **more strongly**. If the same pair of areas keeps
    sharing tasks — create a **separate intersection area** for the two.
  - Where it's written: the `Area` column of the registry, the `- Area:` line in the task file, and the second
    word of the backlog line. The same everywhere.
- 🔴 **Task type — `analysis` / `feature` / `bug`; for a plan — `plan`; no other types.** `plan` — always alone,
  without other types (§Plans). A regular task has at least one of the three (epics and closed ones too); several
  may be present — then always in the order **analysis → feature → bug**. Written as the first words of the tags
  (the "Tags" column of the registry, the `- Tags:` line of the task file), in the backlog — as icons (see
  "Backlog"). The point — a human sees in the backlog where the bugs are and where the features are, and
  prioritizes by that. How to assign:
  - **`analysis` (🔵) — only a LARGE standalone investigation with a real reason**: study someone else's sources or
    product, choose a technology from options, check whether something is possible in principle, find the cause of
    an unclear problem. 🔴 NOT analysis: "build a feature and figure out how along the way", a small check or a
    decision inside the work, a measurement for a specific change — that's part of the feature or bug;
  - **`feature` (🟢)** — new or an improvement of existing: a new capability, refinement, optimization, tool,
    umbrella epic, a stash of future mechanics;
  - **`bug` (🔴)** — works incorrectly, a problem: defect, artifact, flicker, lag or slowdown where it should work
    normally. An "improvement" that actually fixes a defect is a bug, not a feature;
  - **combinations:** a bug in which something new is also added — `feature, bug`; a large investigation of the
    cause of a problem — `analysis, bug`; a large investigation that grows into an implementation —
    `analysis, feature`.
- **`<N>-slug.md`** — the file of a heavy task (`N` = number) in `tasks/` of its area. Template:

```markdown
# N — Title

- Status: 🔵 open
- Area: <area>
- Tags: <type: analysis/feature/bug>, <topic>
- Created: YYYY-MM-DD
- Original: [original-prompts/<N>-slug.md](../original-prompts/<N>-slug.md)  ← the user's verbatim requests
- Related: [links to tasklog/topic/other #NN, GitHub issue]

## Description
<what and why — YOUR wording, checked against the original. Don't duplicate the original here.>

## Important notes
<key decisions, what's important to know. NOT a process chronicle — that's the tasklog.>
```

- **`<N>-slug.md`** in **`original-prompts/`** — the user's verbatim requests (see §The user's original requests).
  Created together with a task that has a clear user request/idea; appended on every refinement. Free
  chronological format, but the user's quotes 1:1; your insertions only as meta-notes. Header example:

```markdown
# N — Title (the user's original requests)

Task: [../tasks/<N>-slug.md]

## Initial request (YYYY-MM-DD)
> <verbatim user quote>

## Refinement (YYYY-MM-DD, <in response to what / conditions — assistant meta-note>)
> <verbatim user quote>
```

**Lifecycle:**
- **Create** — next free number, **pick an area** (no fitting one — first add it to the area registry) → a row in
  `active/tasks/_index.md`; heavy → file `active/tasks/<N>-slug.md`; there's a user request/idea →
  `active/original-prompts/<N>-slug.md` (verbatim); a detailed log is needed → `active/tasklog/<N>-slug.md`. New
  tags → into `active/tasks/_tags.md`. The new task goes to the top of the background backlog (§Backlog).
- **Update** — status/tags in `active/tasks/_index.md`, the detailed course — in its tasklog; **a new
  refinement/idea from the user — append VERBATIM to its `original-prompts/<N>-slug.md`**.
- **Close** — move BETWEEN the areas:
  1. the row from `active/tasks/_index.md` → into `closed/tasks/_index.md` (format `# | Area | Closed | Tags |
     Outcome`: same area, closing date + a compact one-line outcome with a link to the log/commit); remove the
     task's line from the backlog it was in (and run `backlog_tidy.py`);
  2. the task file `<N>-slug.md`, its separate tasklog file and its `original-prompts/<N>-slug.md` (if any) —
     `mv` from `active/` to `closed/` (subtasks of an epic whose log is INSIDE the epic log don't move the log —
     the epic stays active);
  3. **update the `_tags.md` of both areas** per the scope rule (remove from active the tags no longer present in
     any active task; add to closed);
  4. **in the plans where the task is a step, put ✅** (`grep "#N"` over `tasks/`); when closing a plan — remove its
     line from `plans` (and from "Current" if we were following it);
  5. **fix cross-links:** links to the moved task from active files now lead to `../../closed/tasks/…`, and its own
     links to active neighbors — to `../../active/…` (and to the epic log — to `../../active/tasklog/…`).
- **Reopen** — the same in reverse (`closed/` → `active/`).

### 🔴 "Done ↔ closed" reconciliation (regular, otherwise it DOESN'T HAPPEN)

Tasks get finished and stay hanging open: the work is done, but the row is still in `active/`. This breaks the
tracker's main property — you can no longer tell from it what's really left. It doesn't surface by itself:
finished, switched, forgot.

So **periodically (not "when I remember") go through `active/tasks/_index.md` and check: was the thing the task was
created for done?** Good moments — when you finish a portion of work on a task, when the user confirmed a check,
when you take the next task, when you restore context in a new session.

- Done and confirmed → **close** per the procedure above. Don't wait for a separate command.
- Done, but waiting for a live check by the user → keep it in `active/` with status 🟡 and an explicit "awaiting
  verification" note in the task file, so it doesn't read as "not done yet".
- **Not sure whether to close** (part is done, acceptance criterion is vague, the intent changed) — **ask the user**
  in plain text: briefly list what's done on the task and ask whether to close it. Don't close "just in case"
  yourself and don't leave it silently hanging.

**Hard points — not "periodically" but every time:**
1. **You reported "done" on a task** → in THE SAME reply: status 🟡 "done, awaiting verification" in the registry and,
   as the last line of the reply, the question "close #N?" (the task's number and title in words).
2. **The user confirmed** (explicitly — "close it", or in substance — "fine", "works", "ok", moved on to the next
   plan step) → close **in this same turn**, together with ✅ in the plans. Don't postpone "until the commit".
3. **You take the next plan step** → first look at the status of the previous one: 🟡 — ask whether to close.
4. **"Prepare for commit" / self-review** → go through the 🟡 tasks of the affected area and ask about each.

Check the reverse too: the row in `active/` describes what's really left, not what the task started with. The
intent changed or a part split off into a separate task — fix the row/file instead of accumulating the mismatch.

## Plans

**A plan is a task of type `plan`** (like an epic in Jira): a **goal** and a **sequential list of steps** with
statuses. That's the core of it; notes go around it. Why: keep the order of work toward a goal and see what's done
and what isn't.

- 🔴 **EVERY plan has a goal**: why these steps at all and why in this order, and when the plan counts as done. Two
  kinds (written in the plan file):
  - **targeted** — with a clear goal and completion criterion ("a big game map on which a game can be built");
  - **temporary** — just a way to plan the nearest tasks ("what we do this week"); the goal — a short phrase, the
    criterion — "the steps ran out or the plan was replaced".
- **A step** is one of three:
  - **a task** — a link `[#N](…)` + a short title; ✅ — when the task is closed (done), otherwise ⬜; 🔴 when closing a
    task — find it in all plans (`grep "#N"` over `tasks/`) and put ✅ there;
  - **another plan** — a link to the plan-task (plans nest: step "execute plan #M");
  - **a step without its own task** — text right in the plan (small, or not yet formalized). It grew, became needed
    in another plan, or needs a log — create a task and replace the text with a link.
- 🔴 **The same task may be in several plans** — that's normal (a quick plan and a full plan of one topic).
- 🔴 **Two statuses — ✅ done and ⬜ not done** (the icon at the start of the step). No intermediate ones ("in
  progress", "deferred") — neither for steps nor for the plan itself: what's started is visible from the steps
  inside, and there are few plans. Decided not to do a step — remove it (or leave it as a note `> …` if it's
  important to remember why).
- 🔴 **Done steps stay in the plan** (✅) — they show what was done in the plan at all. And **a fully executed plan**
  is closed and leaves `plans`: only created and not yet executed ones are there.
- **Step order = order of work.** The user moves and edits the plan — in words, through you (doesn't edit by hand):
  "move X up", "insert after Y", "remove Z" — you reorder, renumber, and write it verbatim into the plan's
  original-prompts.
- **Notes and comments — anywhere**, even in the middle of the list: a quote line `> …` under a step or between
  steps (a condition, "discuss after this", a link to an analysis).
- **The plan file** — like a heavy task file (always a file, even for a small plan), template:

```markdown
# N — Plan: <name>

- Area: <area>
- Tags: plan, <topic>
- Kind: targeted | temporary
- Called: "how the user calls the plan in conversation" (if they do)
- Created: YYYY-MM-DD
- Original: [original-prompts/<N>-slug.md](../original-prompts/<N>-slug.md)
- Related: [parent plan, neighboring plans, tasklog]

## Goal
<why this plan, why this order, when it's done>

## Steps
1. ✅ [#101](101-slug.md) — short title
2. ⬜ [Plan #200](200-slug.md) — nested plan
   > note on the step
3. ⬜ A step without its own task — what to do

## Notes
<decisions and important points about the plan as a whole; the chronicle — in the plan's tasklog, if needed>
```

- **In the registry** a plan is an ordinary row (`Tags: plan, …`, status `—`: open until executed); **plans are NOT
  put into the backlogs** — they're not work but the order of work; the list of open plans is `plans` (§below).
- **Closing a plan** — the goal is reached (or the plan was replaced/cancelled — say so in the outcome) → like any
  task (move to `closed/`) and its line — out of `plans`. Unfinished steps that are still needed — move to another
  plan or leave them as tasks in the backlog, don't lose them.

**When to offer a plan (proactively, without a separate request):** the user talks about the order of work
("first… then…", "what's next", "let's plan it", "we'll get back to this after X"), or it's clear a set of tasks is
better kept as a sequence toward a goal — **offer in one sentence to make it a plan** (new, or a step in an existing
one) and do it after agreement.

🔴 **Which plan to take.** "What's on the plan", "next on the plan" without clarification — the plan from the
"Current" line in `plans`; name it out loud ("on plan #N '…', step K"). The user calls the plan their own way ("the
editor plan") — search by "Called". Doesn't match (clearly about another one, or "Current" is empty, or two fit) —
ask which one, listing them briefly.

🔴 **Remind how it works** when it's clear the user doesn't remember (asks where the plan is, confuses the plan with
the backlog, talks about steps that aren't in the plan): in two or three sentences — plans are tasks with a goal and
steps, the list is in `plans`, steps are moved through me, one task can be in several plans.

## Backlog (priority)

**Priority is NOT stored in the task.** Priority is **the order of lines in the backlog file**.

🔴 **There are TWO backlogs — main and background** (where they live — in the project `CLAUDE.md`). The point is to
focus on a handful of related tasks and control only them:
- **main** — what the human keeps in focus right now and moves forward;
- **background** ("unassigned") — everything else that's open. 🔴 **A new task goes to the VERY TOP OF THE
  BACKGROUND one**; the human moves it into the main one. Any rule about "the backlog" without clarification (top =
  priority, "what's the priority" when restoring context) — is about the MAIN one;
- format, group and icon rules are common to both. A task is in exactly one of them. An open task that's in
  neither is a mess: the script reports it, its place is the background one.

- A flat text list, **one line = one task**. No tables, header, sections — so lines are easy to drag in an editor
  (dragged up = raised the priority).
- **Line format: `number area icons description`** — number, space, area, space, type icons, space, then free text
  (a short summary + a priority note). Example: `100500 render 🟢🔴 this is the most important task`.
- 🔴 **Type icons** — circles: 🔵 analysis, 🟢 feature, 🔴 bug; several — in a row without spaces, always in the order
  🔵🟢🔴. The same as the type in the task registry. **These circles are NOT used anywhere else in backlog lines**
  (write "important" in words), otherwise they can't be told apart from the type icons.
- 🔴 **Area groups are visible via blank lines:** tasks of ONE area in a row are written without blank lines between
  them, between tasks of DIFFERENT areas — **exactly 4 blank lines**. The point — a human immediately sees the groups
  and moves priorities by whole groups. The order is set only by the human: one area may appear in several chunks in
  different places of the backlog.
- Top = more important. The human edits it by hand. Blocked/deferred ones are here too (at the bottom).
- 🔴 **The tidy script is `backlog_tidy.py` next to this skill; it tidies BOTH backlogs. Run it EVERY time you touch
  the backlogs** (read, edit, create/close a task): the human edits them by hand and may forget blank lines, add
  extra ones, not set the area or icons — the script fixes that. What it does: puts exactly 4 blank lines between
  groups (fewer — adds, more — trims), removes blank lines inside a group and at the start of the file, normalizes
  spaces in `number area icons description`, sets the area or icons of a line that lacks them from the active
  registry, reorders icons to 🔵🟢🔴; it does NOT change the order of lines. What it reports: ERROR — no number, area
  not in the area registry, area or icons differ from the task registry, a number repeats (including across both
  backlogs), circles in the line text, an active task has no type; WARNING — the task is closed or not in the
  registry; INFO — active tasks that are in neither backlog. Errors and warnings — fix them (differs from the
  registry — check and fix whichever side is wrong). `--check` — only check, write nothing.

## plans / tasklog / topics

- **plans** — 🔴 **only the list of open plans** (tasks of type `plan`, §Plans): the update date as the first line,
  then — **which plan we follow now and which ones before it** (the user will often ask "what's on the plan" — you
  must know which plan is meant):
  - `**Current:** [#N](…) — "how the user calls it" (since date)` — exactly one, the CURRENT one: "what's on the
    plan", "next on the plan" without clarification — it's this one;
  - `**Previously:** [#M](…) — "…" (from date to date); …` — a few recent ones, newest first;
  - then one line per open plan — `- [#N](…) <area> — **name** (kind) — called "…"`. The order is set by the user.
  Changed the plan we follow — immediately rewrite "Current" and "Previously". How the user calls a plan ("the
  editor plan") — record it both here and in the plan file (the `- Called:` line). No steps, statuses or reasoning
  here — they're in the plan files. A nested plan also has its own line in the list (marked "step of plan #M").
  - **Update date — the first line**: if it's weeks off from reality, the file can't be trusted.
  - Update on every creation, closing and change of plan focus. The `backlog_tidy.py` script checks: every open
    plan is in `plans`, and `plans` contains only open plans.
  - 🔴 **Reconcile when restoring context:** read `plans` and see a mismatch with the plan files or the registry —
    FIRST fix it, then work.
  - The roadmap of a specific topic, if it's the source of truth, may live in a topic — then the plan links to it.
- **tasklog** — an append-only chronicle of the process of a task/epic. **Name the file like the task —
  `<N>-slug.md`; the task number at the start of the file** (a link to it in the registry). Append a new stage to the
  end, don't rewrite the old (it's history; only stale links may be fixed when files move).
- **topics** — knowledge by topic, reusable. Learned something useful along the way → move it here (instead of
  burying it in a log).

## Cross-links

Relative markdown links between files. When moving a file, check the depth: links to neighbors in the folder survive
the move, links outward (to another folder) — fix them. **Special case — closing/reopening a task** (moving
active↔closed): fix links in BOTH directions (who linked to the task and whom it linked to), including links to an
epic log from another area.

## How to restore context in a new session

1. `CLAUDE.md` (always-on) — rules + where the folders are + a pointer here.
2. `plans` — open plans; "Current" — the current one (its file — goal and steps: what's done, what's next).
3. The top of the main backlog — what's the priority (run `backlog_tidy.py` first); what the areas mean — the area
   registry.
4. `active/tasks/_index.md` — which tasks are open (closed ones — `closed/tasks/_index.md`). This is also the moment
   for the "done ↔ closed" reconciliation (see §Lifecycle): among the open ones there are usually already-done ones.
5. The needed tasklog / topic — by links, when you go into a specific topic/history.

## Discipline

- **One fact — one place.** A task as a whole is in the tracker; in source docs — only a pointer `→ #NN`. Important
  stuff about a task — in its file; the full process — in its tasklog. Don't mix.
- **The description of the system lives ONLY in this skill. Period.** How the tracker works, what the
  areas/files/tags mean, the rules of keeping it — lives EXCLUSIVELY here. DON'T duplicate it anywhere: no
  README/placeholders in the tracker folders, no explanatory inserts in `_index`/`_tags`/logs, nor in `CLAUDE.md`
  (there — only the physical location of the folders + a pointer to the skill). An empty area folder (e.g.
  `closed/tasklog/` while there are no closed logs) just stays empty — don't create a description file in it, it
  will be created by itself on the first move.
- Write so it's useful to yourself in the next session: short, to the point, with dates.
