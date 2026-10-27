# 33 — A wall disappears at the edge of the camera frustum (RTA)

- Status: 🔵 open
- Area: render
- Tags: bug
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
**Repro (RTA):** exit the police station onto the street, go left along the road — **the first wall on the left** (with 3D bump-mapping). When rotating the camera, you can sometimes see how **a chunk/the whole wall right on the left at the edge of the frame disappears** — and through the resulting hole you can see the sky (that is, the entire wall object disappears, not part of it). Looks like **an inaccuracy of the frustum-containment check** for the wall during the culling of invisible geometry (the frustum cull of the bounding object triggers too early at edge angles). Not very noticeable, manifests in edge cases at the camera edge. Possibly related to the object's bbox/radius in the visibility test.

Old-list status: *Investigate*.
