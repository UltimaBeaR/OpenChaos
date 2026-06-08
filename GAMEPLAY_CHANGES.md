# Gameplay Changes — OpenChaos vs Original

How OpenChaos differs from the **retail PC version** of Urban Chaos in terms of gameplay.
This covers changes to gameplay, controls, cheats, and mechanics — anything that affects how the game plays.
Pure graphics/rendering changes (renderer, effects) are not listed here.

---

## General

- Game speed is slightly slower than the retail version. The in-game timer now matches real-world time: 5 minutes on the in-game timer equals 5 real minutes.

- Camera — rotating the camera is now a mandatory control element (mouse or stick). The camera can now be rotated freely and won't go inside walls and other objects.

- The gamepad cheats are carried over from the PS1 version, but the button combinations used to trigger them have been changed, and one new cheat was added. These cheats are also available on keyboard through the F9 console.

- The old `.ini` config file is now ignored, except for the one setting that selects the localization language — everything else in it is no longer read. OpenChaos keeps its own settings in a separate file in your user folder (on Windows: `%APPDATA%\OpenChaos\OpenChaos.config.json`). Some settings carry over from the old config, some were removed, and some were added. Some options were also removed from the in-game menu — graphics settings and control rebinding — though a few of them are still available, only through the new config file.

- Save games are no longer stored in the `saves` folder inside the game directory — they are now written to your user folder (on Windows: `%APPDATA%\OpenChaos\saves`). When loading, the game reads saves from both your user folder and the game's own folder.

## Controls

- Keyboard controls have been changed to keyboard + mouse. Moving the mouse only rotates the camera around your character; the mouse buttons are also used in-game. The keyboard control scheme has been completely changed and rethought. For now you can't rebind the keys or mouse buttons.

- Gamepad support has been added — Xbox-compatible controllers and DualSense. Gamepads work over USB and Bluetooth. Rumble is supported on both. DualSense also supports LED lighting and trigger effects when firing. The gamepad control scheme has been rethought compared to the PS1 version. For now buttons can't be rebound.

- Character movement no longer uses the old "tanky" controls. It now works as a directional, analog movement mode — you point where you want to go and the character turns and moves that way — like the analog (stick) mode on the PS1. The old tanky controls can't be enabled at the moment.

- A "Help" menu has been added in pause mode (always in English) where you can view the button layout and descriptions of some game mechanics.

## On foot

- Rolls — you can now roll in any direction, including while running.

- Added a slow-walk mode, as on the PS1 version.

- Sprint, crawl (stealth mode) and the action button are now three separate buttons instead of one.

## Melee combat

- Melee combos — the old timing mechanic is gone: you no longer have to land the next press within a window after the previous swing's animation to keep a combo going. Combo hits now continue automatically as you press attack (you can just spam the button), but to compensate, the 2nd and 3rd hits now have a chance to fail.

- Melee target switching — targets now switch automatically: press attack while aiming the camera toward a new enemy and you'll retarget them. The target-switch button (the action button) now picks enemies primarily by the direction the camera is facing.

- After a dodge in melee you can no longer stay crouched indefinitely — you can hold the low pose for at most 1 second, then your character stands back up.

- Grappling (grabbing) an enemy now has a cooldown — after a grab you can't start another grab for 5 seconds.

- Throwing a grabbed enemy down to the ground now has a small chance to fail; on a failure you simply shove them away instead.

- Arresting can now be done at most once every 5 seconds.

- Kicking a downed enemy is now slightly easier (the kick has a bit more range).

## Weapons & shooting

- The M16's automatic fire rate may have changed slightly.

- Starting to run no longer interrupts your M16 fire.

- The aiming dotted lines (shown when an enemy is aiming at you) have been removed. The target marker itself stays.

## Vehicles

- Cars — spinning out is harder to trigger (it takes more to lose control and spin out), and steering, acceleration and braking may behave a little differently. You can now brake without spinning out, using a newly added dedicated reverse button, and the brake button no longer automatically puts the car into reverse after it has come to a stop. You can now also get out of the car while it's still moving, as long as it has almost stopped.

- You can now climb onto the ordinary police car (the non-SUV one).
