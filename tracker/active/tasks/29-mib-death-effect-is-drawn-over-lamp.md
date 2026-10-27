# 29 — MIB death effect is drawn over lamp posts

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
When a MIB dies, during the death animation a characteristic **blue triangular light effect** appears. This effect is drawn **over the lamp posts**, although the posts themselves should be in front relative to it (they're closer to the camera). Reproduces well on **Headline Hostage** — a MIB stands right next to the posts at the start of the mission. Bug class: post-process / additive sprite without depth test. Most likely the effect goes through `POLY_PAGE_BLOOM*` / `POLY_PAGE_LENSFLARE` with `SetDepthEnabled(false)` (like a normal bloom/lens flare — intentionally over everything). Here this is by-design for most post-processes, but for **this particular death effect** it should respect scene depth (the posts should occlude it). **Verify against the retail PC version** — if the posts occlude it there, it's a regression (or a deliberate pre-release difference); if it's the same, it's the original behavior. Not a blocker.

Old-list status: *Investigate (verify)*.
