# 43 — Missing "fire pillar" above a burning character (retail improvement)

- Status: 🔵 open
- Area: render
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In the final PC retail version, when a character is burning (clearly visible on Darci with the invulnerability cheat enabled — she runs around in the burning state for a long time) **a characteristic vertical pillar of rising fire is drawn above the character** — a separate visual effect on top of the normal flame distributed over the body. **We don't have this pillar.** It's also absent in the pre-release sources (our base) and in PieroZ — this is a **MuckyFoot improvement for the final release**, separate from the calibration of the general immolate fire. After the 2026-05-23 fix the overall flame size on the character became correct — but this "pillar" on top was never there and still isn't. In normal play the burning is short and the effect is barely noticeable; the most visible manifestation is the invulnerability cheat + a long stay in the fire. Non-blocking. Possible directions: an additional pyro emitter with vertical velocity, an additional ribbon (like the immolate branches), a separate sprite/billboard above the character. Compare with retail and reverse-engineer if needed.

Old-list status: *Post-1.0*.
