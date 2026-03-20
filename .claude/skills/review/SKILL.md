---
name: review
description: >
  General self-review for OpenChaos — runs after ANY code changes, not just before commits.
  Use this skill whenever work is done: after a migration iteration, after fixes, after any edits.
  Trigger on: "проверяй", "ревью", "готово", "самопроверка", "review", "check", or automatically
  after completing any batch of code changes. ALWAYS run this after finishing work — never skip it.
  The user commits manually and may not commit at all — review is still mandatory.
  For Stage 4 migration work, also trigger the stage4-review skill which adds Stage 4-specific checks.
---

# Pre-Commit Self-Review

After finishing ANY batch of edits you MUST go through this review.
This is not just a "pre-commit" step — review happens after every completed piece of work:
after a migration iteration, after fixes requested by the user, after any code changes.
The user commits manually and may not commit at all — review is still mandatory.
No shortcuts. No "looks good". Every file, every hunk, fresh eyes.

## Step 1: Gather All Changes

```bash
git diff          # staged + unstaged changes in existing files
git status        # new untracked files — also part of the review scope
```

Read the full output. Then open and read **every changed and every new file**.
Review as a **reviewer**, not as the author.

## Step 2: General Checks

### Dangling if/else — THE SILENT KILLER

Before ANY line deletion, check: is it the sole body of `if`/`else`/`else if`/`while`/`for`
without braces?

If the deleted line was the body of a braceless `if`:
```cpp
if (condition)
    deleted_line();   // ← you delete this
next_statement();     // ← now captured by the if! Logic silently changed.
```

The compiler will NOT warn. This **silently changes program logic**.

**Rule:** when deleting the body of a braceless `if`/`else` — delete the `if`/`else` itself too
(or add `{}` / `;`).

This applies to ALL line deletions, not just debug calls. Especially dangerous in mass operations.
**Never delete lines in bulk via script** without manually checking every context afterward.
Line deletion — only via Edit with surrounding code review (±5 lines).

### Code correctness
- Logic unchanged — not simplified, not "improved", not refactored beyond what was requested
- No accidental deletions of needed code
- No broken control flow from removed lines
- String/path literals correct (no typos, no wrong slashes)

### Compilation
- If code was changed: `make build-release` must pass
- If only docs/config changed: compilation check not needed

## Step 3: Verify Completeness

- Every file that should have been changed — was changed
- Every file that should NOT have been changed — was not changed
- No stray debug prints or temporary code left behind

## Step 4: Report

Only after confirming all checks pass, tell the user. Be specific:

"Самопроверка пройдена: [N] файлов проверено, всё ок."

If any issue is found — fix it first. Do not suggest committing with known issues.

---

## Stage-Specific Extensions

On **Stage 4 (migration)**: also run the `stage4-review` skill which adds checks A–H
(uc_orig, globals, entity mapping, include guards, comments, CMake, empty files, DAG order).

Future stages will have their own review extensions.
