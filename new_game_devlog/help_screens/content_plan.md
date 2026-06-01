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
- **Stay GENERAL; don't enumerate edge cases** when there could be many. State
  the capability, not every exception — e.g. "you can fire on the move", NOT a
  list of every state where shooting is/isn't allowed (there are dozens: ladder,
  ledge-hang, jumping, backing up...). The player finds the limits in play.
  Detail is for how to PERFORM a move (button combos), not for listing cases.
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
   - Drawing a GUN mid-fight drops the stance (a bat or knife does NOT — they
     work within the stance). Gun out = no auto-stance until hit, when the gun
     flies from her hands. BODY: brief note in the stance section. Use one verb
     for ending the stance — "drops" (not "cancels").

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

4. **WEAPONS & SHOOTING** — guns. Mechanics (confirmed by the user):

   Shooting:
   - Darci shoots wherever she faces. With no target near her line of sight the
     shot just goes nowhere ("you can, but it hits nothing").
   - If an enemy / civilian / car / barrel (barrels explode when shot) — maybe
     more — is near where she looks, it is auto-locked (narrow cone, best-target
     pick) and the shot connects. BODY KEEPS GENERAL: "people and certain
     objects" — no enumeration, no barrel detail (the player sees the lock-on
     and figures it out).
   - Swinging the camera alone does nothing; swing it then push forward and Darci
     turns to face the camera, lining up the target you want. BODY OMITS this —
     the player works out how to turn toward a target on their own.
   - Can fire standing / walking / running. NOT in mid-air, NOT during other
     actions, NOT while walking backwards.
   - Pistol & shotgun: holding attack = one shot. M16: holding attack = automatic
     fire.

   Reload & ammo:
   - Out of ammo: the next attack press RELOADS a spare magazine (no shot); press
     again to fire. No spares left → just empty clicks.
   - Spare ammo / dropped guns are picked up automatically by walking over them
     (user not 100% sure on per-weapon clip distinction — keep it general).

   Grenade (non-obvious — describe carefully):
   - With the grenade selected, dotted trailing lines show where it will land.
   - 1st attack press PULLS THE PIN — a countdown shows by the grenade HUD icon.
     2nd attack press THROWS it. It explodes when the timer hits 0, wherever it
     is (in hand or thrown).
   - The pin CANNOT be un-pulled: switching weapons (timer then hidden) or holding
     too long → it blows up in Darci's hands. Can't throw without pulling the pin.
     With the right timing it can burst in mid-air.

   Drawing & switching (SELF-CONTAINED — spell the buttons out here, don't make
   the player go to the REFERENCE; the weapon-list cycle combo is non-obvious):
   - Draw/holster toggles the last active weapon (holster = empty hands). KBM:
     middle mouse. Pad: a TAP of R3.
   - Quick-select: KBM 1/2/3/4 (pistol/M16/shotgun/grenade), Tab = bat/knife
     toggle. Pad D-pad: up pistol, left M16, right shotgun, up+right grenade,
     down = bat/knife toggle.
   - Weapon-list cycle: KBM mouse wheel. Pad: HOLD R3 + D-pad up/down (the hold
     combo is the non-obvious bit — must be spelled out).
   - (The stance interaction — drawing a gun drops the stance, gun-out = no
     stance until hit — is covered in COMBAT's FIGHTING STANCE, not here.)

5. **DRIVING** — car. Gas / brake / reverse, steering, getting in and out, siren.
   - Car entry/exit is documented HERE only (not duplicated in INTERACTION).

6. **INTERACTION** — world interactions NOT covered by other topics. The Action
   button is context-sensitive: search bodies, press wall buttons, talk to NPCs,
   levers, etc. Only unique items that don't belong to another topic. No car
   entry (→ DRIVING), no arrest (→ COMBAT).
   - Health packs: medkits lie around but do NOT auto-pickup — press Action next
     to one and Darci uses it with an animation, healing if not at full HP (won't
     pick it up at full HP). (Lives here, not in WEAPONS — it's an Action pickup.)

## Out of scope

- **Radar / objectives** — deliberately omitted. The help is focused on CONTROLS
  (how to operate the game), not on reading the HUD or mission info.
- Menu navigation — never described (CONTROLS lists gameplay buttons only).

## Status

- [x] 1. REFERENCE
- [ ] 2. MOVEMENT  ← next
- [x] 3. COMBAT
- [x] 4. WEAPONS & SHOOTING
- [ ] 5. DRIVING
- [ ] 6. INTERACTION

Placeholder topics still in the table (MOVEMENT/COMBAT/CLIMBING/WEAPONS) get
replaced/restructured into the above as each is written.
