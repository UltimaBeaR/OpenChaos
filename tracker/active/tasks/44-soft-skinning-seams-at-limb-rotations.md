# 44 — Soft skinning: seams at limb rotations

- Status: 🔵 open
- Area: render
- Tags: feature
- Created: 2026-10-27
- Related: migrated from the old list `archive/known_issues_and_bugs/known_issues_and_bugs.md`

## Description
Phase 2 skeletal skinning. Currently only minor blending is enabled at the neck↔head joint (HEAD parent: band=20, w_max=1.0) — slightly smooths the kink on most models. **Universally fixing all joints doesn't work** because: (1) different PersonTypes have different mesh layouts — for some the neck is in the TORSO piece, for others in the HEAD piece; the same band/w_max parameters break one group when fixing another. The same applies to wrists and ankles. (2) The skeleton has **no intermediate upper-body / neck bone** — the HEAD bone sits at the base of the neck, the TORSO bone at the pelvis, between them is one long bone. One could **synthesize a virtual upper-body bone** between TORSO and HEAD (slightly below the HEAD joint) — then there would be a natural pivot for the upper part of the torso and the blending would be anatomically correct. But this requires reworking the FK chain / pose composer / bind palette — a large separate task. Current state: the 5-pair bidirectional blend algorithm is written, 3 groups (HEAD/HANDS/FEET) × parent/child × (band, w_max), the values are hardcoded in `bind_palette.cpp`. Only HEAD parent is filled; the rest are zeros = hard binding. To finish — add the virtual upper-body bone, tune constants for all groups / models.

Old-list status: *Post-1.0*.
