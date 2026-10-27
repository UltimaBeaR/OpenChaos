# 82 — Sort stability differs across platforms (qsort / std::sort)

- Status: ❄️ deferred
- Area: platform
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
`qsort` and `std::sort` do **not** guarantee a stable order for elements that compare equal, and the actual order of equal elements differs between the Windows CRT / libstdc++ / libc++ implementations. Anywhere the game sorts by a key that has ties (e.g. draw-order by depth, entity lists), the tie-break order can differ Windows vs Linux/macOS → different draw order / iteration order → subtle visual or gameplay divergence. **Suspected relevance:** possibly connected to some of the z-fighting / draw-order visual bugs (leaves & newspapers, puddles) that behave differently across platforms. **Fix direction (when a symptom appears):** switch the offending sort(s) to `std::stable_sort`, or add an explicit tie-breaker (e.g. object id / index) to the comparator so the order is fully determined by the key. **Risk:** changing a comparator/sort can shift behavior on ALL platforms, including the Windows reference — needs per-site verification, not a blind sweep. **Not free** — do only when a concrete divergence is traced to a sort.

Old-list status: *Deferred (only if a symptom appears)*.
