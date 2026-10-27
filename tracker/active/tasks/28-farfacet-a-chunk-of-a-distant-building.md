# 28 — Farfacet: a chunk of a distant building's facade is missing

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro 1 (Urban Shakedown):** exit the police station, go left along the building to the corner (turn left). From this point a **distant building** is visible in the frame with a dark rectangular chunk of facade in its upper part (stands out in tone from the rest of the wall). If you move slightly away from the corner — this dark facade chunk **disappears** (in its place you can see what's behind the building — a hole in the facade). If you come closer — the chunk returns and the facade is drawn whole. **Repro 2 (RTA):** next to the bearded biker NPC (the one who "knew Darci's father") there's also a building with a similar hole in the far representation — so the bug isn't a one-off on Urban Shakedown, but a systemic class. **Update 2026-04-24:** the far geometry is **not rebuilt** on the fly — it's wrong the whole time (with the hole). There's no "LOD threshold triggering incorrectly"; the far representation of this building contains the hole from the start and stays that way. The visual "transition whole → with a hole" is not re-LOD, but simply a swap: when approaching, the far facet placeholder is replaced with the building's real (full) geometry, and the eye catches the moment of the **jump from the broken far to the correct near**. So the bug is in the construction of the far representation (somewhere during the generation of these buildings' far facets one polygon/facet is lost), not in the per-frame culling/visibility logic. **Check the retail PC version** — if it reproduces identically there, it's an original MuckyFoot bug.

Old-list status: *Investigate (verify)*.
