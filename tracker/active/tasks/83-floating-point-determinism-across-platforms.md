# 83 — Floating-point determinism across platforms

- Status: ❄️ deferred
- Area: platform
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Float results can differ across platforms/compilers due to: fast-math flags (`-ffast-math` reorders/relaxes ops), FMA contraction (`a*b+c` fused into one rounding step on some targets, two on others), and x87-vs-SSE intermediate precision. Any float that feeds a branch, an int cast, or accumulates over time can diverge Windows vs Linux/macOS. Mostly cosmetic here (the gameplay-critical math is integer/fixed-point), but a possible parity source. **Fix direction (when a symptom appears):** ensure the same float settings on all platforms — verify no `-ffast-math` mismatch between build configs, consider `-ffp-contract=off` for parity, keep IEEE strictness consistent. **Risk:** float flags affect performance and can subtly change output on the reference platform too; must not be flipped blindly (target hardware is weak — perf-sensitive). **Not free** — investigate build flags and measure before changing anything.

Old-list status: *Deferred (only if a symptom appears)*.
