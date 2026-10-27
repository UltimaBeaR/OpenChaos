# 35 — Police car shadow with a see-through square

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
At the very start of the **Headline Hostage** mission there's a police car. Its shadow glitches: in the middle a **rectangular square** appears, through which it's as if you can see the floor under the shadow (a spot lighter than the surrounding shadow, like a "hole" in the shadow). Looks like **z-fighting** — two shadow polygons at the same height compete by depth, and at a certain camera angle one starts winning in the center. Noticed 2026-05-20. **Version comparison (2026-05-23):** in the **retail PC version this bug is NOT present**; in the **PieroZ version it IS**; in our version the manifestation **differs from PieroZ** — when the camera approaches, the square **shrinks slightly** (dependence on proximity to the camera), whereas in PieroZ it's of **constant size**. Conclusion: the bug originates from the pre-release sources (MuckyFoot fixed it in the final release), and on top of that we have an **additional** dependence of the square's size on the distance to the camera — probably a renderer regression (proximity-dependent depth-bias / LOD path). **Second repro (Driving Bronze, 2026-05-24):** at the very start of the mission — if you go in the direction **opposite to the police car's direction**, you'll run into a **STOP** sign in front of which stand **2 cones**. The shadows of these cones **partially climb onto** some invisible "thing" that occludes the shadows — visible from the cones' clipped shadows. If you drive into this spot in the police car — there's the **same rectangle** occluding its shadow, with the **same proximity dependence** (narrows when the camera approaches). Most likely the **same bug class**, manifesting not only on Headline Hostage — somewhere in the level data / in the shadow-receiver generation code there are "ghost" rectangular shadow planes whose z drifts with the camera.

Old-list status: *Investigate*.
