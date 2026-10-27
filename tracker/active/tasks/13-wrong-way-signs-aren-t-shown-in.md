# 13 — "Wrong way" signs aren't shown in car missions

- Status: 🔵 open
- Area: gameplay
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
In the original (PS1, and presumably retail PC) during car-driving missions, when moving against the road direction, a warning sign should blink on the screen. Currently in our build these signs don't appear at all. **User clarification:** I don't remember exactly how it should be, need to **compare with retail**. Important: we **do show** the "turn left/right now" signs (a different class of signs with the same drawing mechanism) — so the mechanism itself isn't broken, the problem is specifically in the wrong-way trigger. Possibly: (1) the wrong-way signal should go not through `EWAY_DO_SIGN` but through separate road-direction check code (see `ROAD_signed_dist` / road-network) which we lost during porting; (2) the mission script doesn't call `EWAY_DO_SIGN` for wrong-way (there's no such signal in the scripts at all).

Old-list status: *Investigate (compare with retail)*.
