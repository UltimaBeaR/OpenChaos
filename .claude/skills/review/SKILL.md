---
name: review
description: >
  General self-review for OpenChaos — runs after ANY code changes, not just
  before commits. Use this skill whenever work is done: after a migration
  iteration, after fixes, after any edits. The user commits manually and may
  not commit at all — review is still mandatory every time code is touched.
  Also use when the user explicitly asks for review.
  Trigger on: "проверяй", "ревью", "готово", "самопроверка", "review", "check",
  "проверь", "посмотри что поменял", or automatically after completing any
  batch of code changes. For Stage 4 migration, also trigger stage4-review.
---

# Self-Review — mandatory after any code changes

Run this after finishing ANY batch of edits. Not just before commits — after every piece of work.
No shortcuts. Every file, every hunk, fresh eyes.

## Step 1: Gather all changes

```bash
git diff          # staged + unstaged changes in existing files
git status        # new untracked files — also part of the review scope
```

Read the full output. Open and read **every changed and new file**.
Review as a **reviewer**, not as the author.

## Step 2: General checks

### Dangling if/else — the silent killer

Before ANY line deletion, check: is it the sole body of `if`/`else`/`while`/`for` without braces?

```cpp
if (condition)
    deleted_line();   // you delete this
next_statement();     // now captured by the if! Logic silently changed.
```

The compiler won't warn. This silently changes program logic.

When deleting the body of a braceless `if`/`else` — delete the `if`/`else` itself too, or add `{}` / `;`.

This is especially dangerous in mass operations. Never delete lines in bulk without checking every context (+-5 lines).

### Code correctness
- Logic unchanged — not simplified, not "improved", not refactored beyond what was requested
- No accidental deletions of needed code
- No broken control flow from removed lines
- String/path literals correct

### Compilation
- If code was changed: `make build-release` must pass
- If only docs/config changed: compilation check not needed

## Step 3: Verify completeness

- Every file that should have been changed — was changed
- Every file that should NOT have been changed — was not changed
- No stray debug prints or temporary code left behind

## Step 4: Report

After confirming all checks pass:

"Самопроверка пройдена: [N] файлов проверено, всё ок."

If any issue is found — fix it first. Never suggest committing with known issues.

## Stage-specific extensions

On **Stage 4 (migration)**: also run `stage4-review` which adds checks A-H
(uc_orig, globals, entity mapping, include guards, comments, CMake, empty files, DAG order).
