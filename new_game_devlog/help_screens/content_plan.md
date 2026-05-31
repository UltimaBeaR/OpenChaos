# Help content plan — topic roadmap

The "How to Play" list is split into one cheat-sheet topic plus a few per-mode
topics.

Writing rules (learned from review):
- **REFERENCE (topic 1) only** uses the flat `{glyph} - label` column layout
  (a button map). The help-list itself is titled "Controls".
- **Every other topic is written mechanic-first, in prose**: describe the game
  capability in plain words and name the button only to explain HOW to do it.
  Do NOT restate the button map.
- **No flowery / editorial wording** ("Darci is at her best on the move", "her
  fastest dodge", etc.). Plain, but not telegraphic — just describe what you can
  do and how.
- **Use ALL-CAPS sub-headers** inside a topic to chunk it for quick visual
  navigation (e.g. MOVING / JUMPING / ROLLING / STEALTH / CAMERA / CLIMBING).
  The body font has no bold/colour, so caps is the only header cue.
- Group strongly-related mechanics so the TOPIC count stays small; split into
  sub-headers within a topic rather than adding more topics.
- Buttons are shown via inline glyph tokens. Combos use `glyph+glyph` (no spaces).

Topics live in `help_content_{kbm,xbox,ps}.cpp` (one body per device), wired in
`help_content.cpp`. See [overview.md](overview.md) for the infrastructure.

## Topics

1. **REFERENCE** (the button map) — ✅ done. Flat button reference: every
   button used during gameplay, one line each, all situations at once. A cheat
   sheet, not a guide.

2. **MOVEMENT** — on foot. The densest topic. Climbing folds in here (it's
   locomotion, not its own topic). Sub-section order is by importance:
   MOVING → CAMERA → JUMPING → CLIMBING → ROLLING → STEALTH → WALL HUG
   (basic movement + camera first; wall hug is rarely needed, so it's last).
   Exact mechanics (confirmed by the user):

   Moving:
   - Run: stick / WASD. One speed, always runs — no analog speed ramp.
   - Sprint: hold the sprint button while running.
   - Slow walk: HOLD the slow/roll button (L2 / Ctrl). In this mode it is ONLY
     slow walk — no rolls.
   - Roll: its OWN mechanic — describe it independently, NOT as "the slow-walk
     button also rolls". Press (not hold) the roll button (L2 / Ctrl — yes, the
     same physical key as slow walk, but don't frame it that way) together with
     a movement direction → Darci rolls that way. Her fastest dodge. Keep it
     visually separated from the slow-walk line in the body text.
   - Jump: on the spot, or along your run direction. A jump done while moving
     backwards is a backflip. (Mid-air you can steer the arc and kill momentum
     by releasing everything — deliberately NOT documented, too fiddly; the
     player discovers it.)
   - Back-step: hold the zoom modifier (E / L1) and push BACK (away from where
     Darci faces) → she walks backwards still facing front. Pushing toward her
     facing is ignored (she just runs forward as if nothing were held).
   - Stealth: from a STANDSTILL (not running) hold stealth → Darci crouches.
     Then start moving → she crawls on all fours. That crawl is the stealth move.
   - Camera: mouse / right stick rotates the camera, and Darci turns to face the
     way the camera looks (always faces the camera direction). Movement is
     camera-relative: hold forward and swing the camera to steer where she runs
     (slight delay).
   - Zoom: pulls the camera in to just above Darci's head, facing the same way.
     Only works from a STANDSTILL — holding it while running does not zoom.
     Same button as back-step (E / L1): held alone = zoom, held + back = back-step.
   - Camera-steer is an OPTION, not the only way to move: phrase it as "you can
     also steer by swinging the camera", not as the primary movement method.

   Wall hug (its OWN sub-section — NOT climbing):
   - Press Action to flatten against a wall; edge left/right along it
     (HUG_WALL state).

   Climbing:
   - Ledge: jump toward a ledge; if Darci reaches it she grabs on automatically.
     Press jump again to pull up. While hanging you can edge left/right
     (DANGLING state).
   - Ladders: to go UP, walk into the ladder (face it) and Darci grabs on. To
     get on heading DOWN, approach REAR-FIRST (back up to it via back-step) and
     press Action — facing it head-on won't start a descent. Once on the ladder
     (no matter how you got on), push forward / back to climb up / down
     (manual — Darci does NOT auto-climb).
   - Zip-line: jump onto a wire (can be anywhere, usually strung between
     buildings) — Darci grabs it and slides down. (In an early movement
     tutorial mission.)

3. **COMBAT** — melee. Combat mode auto-engages near enemies. Punches / kicks,
   slide (подкат), flying/jump kick (удар ногой на лету), combos (partly random),
   grapple / throw, block, rolling out of a fight, melee weapons (bat, knife).
   - **Arrest lives here** (decision): it's the resolution of a fight (subdue →
     arrest), part of the combat cycle, not a separate world interaction.

4. **WEAPONS & SHOOTING** — guns. The four ranged weapons (pistol, M16, shotgun,
   grenade), firing, aiming / zoom, ammo, selecting / cycling / drawing-holstering.

5. **DRIVING** — car. Gas / brake / reverse, steering, getting in and out, siren.
   - Car entry/exit is documented HERE only (not duplicated in INTERACTION).

6. **INTERACTION** — world interactions NOT covered by other topics. The Action
   button is context-sensitive: search bodies, press wall buttons, talk to NPCs,
   levers, etc. Only unique items that don't belong to another topic. No car
   entry (→ DRIVING), no arrest (→ COMBAT).

## Out of scope

- **Radar / objectives** — deliberately omitted. The help is focused on CONTROLS
  (how to operate the game), not on reading the HUD or mission info.
- Menu navigation — never described (CONTROLS lists gameplay buttons only).

## Status

- [x] 1. REFERENCE
- [ ] 2. MOVEMENT  ← next
- [ ] 3. COMBAT
- [ ] 4. WEAPONS & SHOOTING
- [ ] 5. DRIVING
- [ ] 6. INTERACTION

Placeholder topics still in the table (MOVEMENT/COMBAT/CLIMBING/WEAPONS) get
replaced/restructured into the above as each is written.
