# Gamepad improvements (part of Stage 13)

Generic gamepad improvements (generic via SDL3 + things common to all controllers).

DualSense-specific topics (adaptive triggers, LED, touchpad, gyroscope, haptics) → [stage13_dualsense.md](stage13_dualsense.md)
Technical documentation for the DualSense API → [stage5_1_dualsense.md](stage5_1_dualsense.md)

---

## Gamepad settings via UI
- In 1.0 the joystick settings are removed from the menu
- After 1.0 — a full UI: button binding, stick sensitivity, deadzones, etc.

## Button icons in the UI
- The `InputDeviceType` infrastructure is ready (we detect the controller type)
- Device-aware button glyphs are **implemented** for the in-game "How to Play"
  help (embedded Kenney atlases, see `new_game_devlog/help_screens/`). Hints directly in
  the HUD (over gameplay) — not yet implemented, a candidate for the future.

### Steam Deck detection (after 1.0)
- Right now `active_input_device` distinguishes only KEYBOARD_MOUSE / XBOX / DUALSENSE.
  There is no "Steam Deck" value → on the Deck the help will show the keyboard or Xbox style
  of glyphs, not the native Deck style.
- **The glyph graphics for the Steam Deck are already ready** (the `deck` atlas is embedded and loaded in
  `input_glyphs_init`, coordinates in `input_glyph_coords`), it's just never selected —
  atlas selection in `atlas_for_active_device()` (input_glyphs.cpp) has no Deck branch.
- TODO: detect running on a Steam Deck (SDL/ENV/device) and add
  the corresponding `InputDeviceType`, so the `deck` atlas is selected automatically.

## Quick weapon select: D-pad

**Idea:** D-pad directly (without a modifier) — instant weapon selection by slot, bypassing cycling. R3 we **leave alone** — keeping the current behavior (cyclic cycling through the whole inventory) as is.

**Layout:**
| D-pad | Weapon |
|-------|--------|
| ↑ | Pistol |
| ← | Machine gun (AK47) |
| → | Shotgun |
| ↓ | Melee / fists (see below) |

**Logic for ↑ / ← / →:**
- If the needed weapon is in the inventory and not in hand → instant switch, no cycling animation.
- If the weapon is already in hand → don't re-equip (don't replay the draw animation), but the popup still shows as confirmation of the press.
- If the weapon is not in the inventory → no switch happens, but the popup shows the **current** weapon (visual feedback that the button was pressed — without it the player can't tell whether the game reacted).

**Logic for ↓ (melee):**
- If any weapon **other than** melee is in hand → switch to the **empty fist**.
- If a fist or a melee weapon is already in hand → cycle through the melee slots in the order: **fist → bat → knife → fist → …** (cyclically, whatever isn't in the inventory is skipped; the fist is always present).
- The popup shows on every press (just like for ↑/←/→).

**Popup HUD:** any D-pad press opens the same popup as for R3 (lives ~3 sec, then fades). It highlights the slot selected by the D-pad (or the current slot if the weapon isn't in the inventory).

**R3 (unchanged):** cyclic cycling through the whole inventory, exactly as now. All weapons not assigned to the D-pad (grenade, C4, etc.) are still accessible via R3. The R3 path code we **leave alone**.

**Note on naming:** in the sources the weapon is called `AK47` (`PRIM_OBJ_WEAPON_AK47`), even though it's visually an M16. In the code and docs we keep "AK47" as in the original, to avoid breeding discrepancies.

**Implementation:** handled in `game_tick.cpp` next to the R3 path (the same gating — `can_darci_change_weapon` + not paused). On `dpad_just_pressed(dir)` it calls select-by-type (for ↑/←/→) or melee-toggle (for ↓), using `CONTROLS_set_inventory` for equipping — the same as the R3 path. Gated on the cheat combo (Select+L1+L2) so as not to duplicate D-pad weapon switching with the D-pad cheats in `input_actions.cpp`. The R3 path is not modified.

## Stick drift in menus
- The main menu has no deadzone for the sticks — infinite scroll on controllers with drift
- In gameplay there is a deadzone, in menus — none
- **This is a bug for 1.0**, not for stage 13 — recorded in [known_issues](../known_issues_and_bugs/known_issues_and_bugs.md) — "Infinite menu scroll (stick drift)"
