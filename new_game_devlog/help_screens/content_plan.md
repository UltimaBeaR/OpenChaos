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

3. **COMBAT** — melee. The other dense topic. Mechanics (confirmed by the user):

   Fighting stance — BODY KEEPS THIS BRIEF. The player only needs "a stance
   activates by itself in close combat; switch target with Action". The
   trigger/movement specifics below are REFERENCE, not for the screen — don't
   spell out technical detail unless it's how to perform a move (button combos).
   - Enters automatically when an enemy holding a melee weapon is close, OR the
     instant you swing at any enemy in melee (even a miss counts). A gunman keeps
     firing until you land a hit on him.
   - Movement is TARGET-RELATIVE (boxer stance): step toward / away / sidestep the
     current target, each with a cooldown (can't spam steps). No jumping, can't
     just run away. Switch target with Action.
   - Hitting an armed enemy knocks their weapon to the ground; getting hit while
     you're armed drops yours.

   Strikes & combos — BODY KEEPS THIS SIMPLE: just "press the attack buttons a
   few times or alternate them to chain a combo; if a follow-up doesn't come
   out that's normal (not a missed input)". Hit counts / proc odds / bat-knife
   internals / weapon-drop are reference only, not for the screen.
   - Punch (hands) or kick (feet). Press repeatedly for a combo of up to 3 hits:
     1st always lands, each follow-up has a diminishing chance to continue. Can
     mix hands and feet.
   - In the stance, hold a direction (left / right / back) + kick → kick that way.
   - Bat/knife attack with the PUNCH button; kicks still work. Knife = 3 distinct
     strikes; bat = 1st hit with the bat, the other 2 combo hits are hand blows
     (not the bat). Drawing a gun leaves the stance; bat/knife can be drawn in it.

   Grapple:
   - Tap punch + move INTO a target right in front and close → grab. From the
     hold: punch → throw to ground (can fail → shoves enemy aside); kick → knee
     to gut (up to 2 if quick). Then Darci releases.

   Dodge (body section DODGE):
   - Push BACK as the enemy attacks → Darci ducks under it. Hold to stay crouched
     (auto-stands after a moment). Kick while ducking → sweep that knocks down
     nearby enemies. (A dodge, NOT a block.)

   Escape (in the body this lives INSIDE the FIGHTING STANCE block, since it's
   how you leave the stance):
   - Any roll cancels the stance and gives a chance to flee. In a fight the roll
     also works on the JUMP button (not only the roll button); still needs a
     direction held.

   Running moves (work outside the stance too):
   - Slide (подкат): kick while running → Darci slides; sliding into an enemy
     knocks them down.
   - Jump kick: jump while running (won't work from a standstill), then kick in
     the air.

   Downed enemies (works in stance and normal on-foot):
   - Arrest: stand over a knocked-down (still alive) enemy and press Action.
   - Stomp: facing a downed enemy up close, press kick → downward stomp.

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
- [x] 3. COMBAT
- [ ] 4. WEAPONS & SHOOTING
- [ ] 5. DRIVING
- [ ] 6. INTERACTION

Placeholder topics still in the table (MOVEMENT/COMBAT/CLIMBING/WEAPONS) get
replaced/restructured into the above as each is written.
