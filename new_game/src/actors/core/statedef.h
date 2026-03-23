#ifndef ACTORS_CORE_STATEDEF_H
#define ACTORS_CORE_STATEDEF_H

// State machine constants for the Thing system.
// Thing.State drives the active StateFn handler. Thing.SubState refines it.

// uc_orig: STATE_INIT (fallen/Headers/statedef.h)
#define STATE_INIT 0
// uc_orig: STATE_NORMAL (fallen/Headers/statedef.h)
#define STATE_NORMAL 1
// uc_orig: STATE_COLLISION (fallen/Headers/statedef.h)
#define STATE_COLLISION 2
// uc_orig: STATE_ABOUT_TO_REMOVE (fallen/Headers/statedef.h)
#define STATE_ABOUT_TO_REMOVE 3
// uc_orig: STATE_REMOVE_ME (fallen/Headers/statedef.h)
#define STATE_REMOVE_ME 4

// uc_orig: STATE_MOVEING (fallen/Headers/statedef.h)
#define STATE_MOVEING 5 // walking, running, siddling; also used for moving furniture
// uc_orig: STATE_FDRIVING (fallen/Headers/statedef.h)
#define STATE_FDRIVING 6 // furniture drives
// uc_orig: STATE_IDLE (fallen/Headers/statedef.h)
#define STATE_IDLE 6 // standing, waiting
// uc_orig: STATE_LANDING (fallen/Headers/statedef.h)
#define STATE_LANDING 7
// uc_orig: STATE_FDOOR (fallen/Headers/statedef.h)
#define STATE_FDOOR 7 // furniture that is a door
// uc_orig: STATE_JUMPING (fallen/Headers/statedef.h)
#define STATE_JUMPING 8
// uc_orig: STATE_FIGHTING (fallen/Headers/statedef.h)
#define STATE_FIGHTING 9
// uc_orig: STATE_FALLING (fallen/Headers/statedef.h)
#define STATE_FALLING 10
// uc_orig: STATE_USE_SCENERY (fallen/Headers/statedef.h)
#define STATE_USE_SCENERY 11
// uc_orig: STATE_DOWN (fallen/Headers/statedef.h)
#define STATE_DOWN 12
// uc_orig: STATE_HIT (fallen/Headers/statedef.h)
#define STATE_HIT 13
// uc_orig: STATE_CHANGE_LOCATION (fallen/Headers/statedef.h)
#define STATE_CHANGE_LOCATION 14 // entering/leaving buildings/vehicles
// uc_orig: STATE_DYING (fallen/Headers/statedef.h)
#define STATE_DYING 16
// uc_orig: STATE_DEAD (fallen/Headers/statedef.h)
#define STATE_DEAD 17
// uc_orig: STATE_DANGLING (fallen/Headers/statedef.h)
#define STATE_DANGLING 18
// uc_orig: STATE_CLIMB_LADDER (fallen/Headers/statedef.h)
#define STATE_CLIMB_LADDER 19
// uc_orig: STATE_HIT_RECOIL (fallen/Headers/statedef.h)
#define STATE_HIT_RECOIL 20
// uc_orig: STATE_CLIMBING (fallen/Headers/statedef.h)
#define STATE_CLIMBING 21
// uc_orig: STATE_GUN (fallen/Headers/statedef.h)
#define STATE_GUN 22
// uc_orig: STATE_SHOOT (fallen/Headers/statedef.h)
#define STATE_SHOOT 23
// uc_orig: STATE_DRIVING (fallen/Headers/statedef.h)
#define STATE_DRIVING 24
// uc_orig: STATE_NAVIGATING (fallen/Headers/statedef.h)
#define STATE_NAVIGATING 25
// uc_orig: STATE_WAIT (fallen/Headers/statedef.h)
#define STATE_WAIT 26
// uc_orig: STATE_FIGHT (fallen/Headers/statedef.h)
#define STATE_FIGHT 27
// uc_orig: STATE_STAND_UP (fallen/Headers/statedef.h)
#define STATE_STAND_UP 28
// uc_orig: STATE_MAVIGATING (fallen/Headers/statedef.h)
#define STATE_MAVIGATING 29
// uc_orig: STATE_GRAPPLING (fallen/Headers/statedef.h)
#define STATE_GRAPPLING 30 // using the grappling hook
// uc_orig: STATE_GOTOING (fallen/Headers/statedef.h)
#define STATE_GOTOING 31 // going to a point with no navigation
// uc_orig: STATE_CANNING (fallen/Headers/statedef.h)
#define STATE_CANNING 32 // coke-can stuff
// uc_orig: STATE_CIRCLING (fallen/Headers/statedef.h)
#define STATE_CIRCLING 33 // circling around someone about to attack
// uc_orig: STATE_HUG_WALL (fallen/Headers/statedef.h)
#define STATE_HUG_WALL 34
// uc_orig: STATE_SEARCH (fallen/Headers/statedef.h)
#define STATE_SEARCH 35
// uc_orig: STATE_CARRY (fallen/Headers/statedef.h)
#define STATE_CARRY 36
// uc_orig: STATE_FLOAT (fallen/Headers/statedef.h)
#define STATE_FLOAT 37 // a person floating in the air

// Movement substates.
// uc_orig: SUB_STATE_WALKING (fallen/Headers/statedef.h)
#define SUB_STATE_WALKING 1
// uc_orig: SUB_STATE_RUNNING (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING 2
// uc_orig: SUB_STATE_SIDELING (fallen/Headers/statedef.h)
#define SUB_STATE_SIDELING 3
// uc_orig: SUB_STATE_STOPPING (fallen/Headers/statedef.h)
#define SUB_STATE_STOPPING 4
// uc_orig: SUB_STATE_HOP_BACK (fallen/Headers/statedef.h)
#define SUB_STATE_HOP_BACK 5
// uc_orig: SUB_STATE_STEP_LEFT (fallen/Headers/statedef.h)
#define SUB_STATE_STEP_LEFT 6
// uc_orig: SUB_STATE_STEP_RIGHT (fallen/Headers/statedef.h)
#define SUB_STATE_STEP_RIGHT 7
// uc_orig: SUB_STATE_FLIPING (fallen/Headers/statedef.h)
#define SUB_STATE_FLIPING 8
// uc_orig: SUB_STATE_RUNNING_THEN_JUMP (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_THEN_JUMP 9
// uc_orig: SUB_STATE_STOPPING_OT (fallen/Headers/statedef.h)
#define SUB_STATE_STOPPING_OT 10
// uc_orig: SUB_STATE_WALKING_BACKWARDS (fallen/Headers/statedef.h)
#define SUB_STATE_WALKING_BACKWARDS 16
// uc_orig: SUB_STATE_RUNNING_SKID_STOP (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_SKID_STOP 17
// uc_orig: SUB_STATE_STOPPING_DEAD (fallen/Headers/statedef.h)
#define SUB_STATE_STOPPING_DEAD 18
// uc_orig: SUB_STATE_CRAWLING (fallen/Headers/statedef.h)
#define SUB_STATE_CRAWLING 19
// uc_orig: SUB_STATE_SNEAKING (fallen/Headers/statedef.h)
#define SUB_STATE_SNEAKING 20
// uc_orig: SUB_STATE_STOP_CRAWL (fallen/Headers/statedef.h)
#define SUB_STATE_STOP_CRAWL 21
// uc_orig: SUB_STATE_SLIPPING (fallen/Headers/statedef.h)
#define SUB_STATE_SLIPPING 22
// uc_orig: SUB_STATE_SLIPPING_END (fallen/Headers/statedef.h)
#define SUB_STATE_SLIPPING_END 23
// uc_orig: SUB_STATE_SIMPLE_ANIM (fallen/Headers/statedef.h)
#define SUB_STATE_SIMPLE_ANIM 24
// uc_orig: SUB_STATE_RUNNING_VAULT (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_VAULT 25
// uc_orig: SUB_STATE_SIMPLE_ANIM_OVER (fallen/Headers/statedef.h)
#define SUB_STATE_SIMPLE_ANIM_OVER 26
// uc_orig: SUB_STATE_RUNNING_HIT_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_HIT_WALL 27
// uc_orig: SUB_STATE_RUNNING_HALF_BLOCK (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_HALF_BLOCK 28
// uc_orig: SUB_STATE_RUN_STOP (fallen/Headers/statedef.h)
#define SUB_STATE_RUN_STOP 29

// Idle substates.
// uc_orig: SUB_STATE_WAITING (fallen/Headers/statedef.h)
#define SUB_STATE_WAITING 11
// uc_orig: SUB_STATE_WATCHING (fallen/Headers/statedef.h)
#define SUB_STATE_WATCHING 12
// uc_orig: SUB_STATE_SCRATCHING (fallen/Headers/statedef.h)
#define SUB_STATE_SCRATCHING 13

// Landing substates.
// uc_orig: SUB_STATE_DROP_LAND (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_LAND 14
// uc_orig: SUB_STATE_RUNNING_LAND (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_LAND 15
// uc_orig: SUB_STATE_DEATH_LAND (fallen/Headers/statedef.h)
#define SUB_STATE_DEATH_LAND 42

// Jump substates.
// uc_orig: SUB_STATE_STANDING_JUMP (fallen/Headers/statedef.h)
#define SUB_STATE_STANDING_JUMP 30
// uc_orig: SUB_STATE_STANDING_GRAB_JUMP (fallen/Headers/statedef.h)
#define SUB_STATE_STANDING_GRAB_JUMP 31
// uc_orig: SUB_STATE_RUNNING_JUMP (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_JUMP 32
// uc_orig: SUB_STATE_RUNNING_JUMP_FLY (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_JUMP_FLY 33
// uc_orig: SUB_STATE_RUNNING_GRAB_JUMP (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_GRAB_JUMP 34
// uc_orig: SUB_STATE_RUNNING_JUMP_LAND (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_JUMP_LAND 35
// uc_orig: SUB_STATE_RUNNING_JUMP_LAND_FAST (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_JUMP_LAND_FAST 36
// uc_orig: SUB_STATE_RUNNING_JUMP_FLY_STOP (fallen/Headers/statedef.h)
#define SUB_STATE_RUNNING_JUMP_FLY_STOP 37
// uc_orig: SUB_STATE_STANDING_JUMP_FORWARDS (fallen/Headers/statedef.h)
#define SUB_STATE_STANDING_JUMP_FORWARDS 38
// uc_orig: SUB_STATE_STANDING_JUMP_BACKWARDS (fallen/Headers/statedef.h)
#define SUB_STATE_STANDING_JUMP_BACKWARDS 39
// uc_orig: SUB_STATE_FLYING_KICK (fallen/Headers/statedef.h)
#define SUB_STATE_FLYING_KICK 40
// uc_orig: SUB_STATE_FLYING_KICK_FALL (fallen/Headers/statedef.h)
#define SUB_STATE_FLYING_KICK_FALL 41

// Float substates.
// uc_orig: SUB_STATE_FLOAT_UP (fallen/Headers/statedef.h)
#define SUB_STATE_FLOAT_UP 43
// uc_orig: SUB_STATE_FLOAT_BOB (fallen/Headers/statedef.h)
#define SUB_STATE_FLOAT_BOB 44
// uc_orig: SUB_STATE_FLOAT_DOWN (fallen/Headers/statedef.h)
#define SUB_STATE_FLOAT_DOWN 45

// Ledge/rope substates.
// uc_orig: SUB_STATE_LATCHING_ON (fallen/Headers/statedef.h)
#define SUB_STATE_LATCHING_ON 50
// uc_orig: SUB_STATE_LATCHING_OFF (fallen/Headers/statedef.h)
#define SUB_STATE_LATCHING_OFF 51
// uc_orig: SUB_STATE_VAULTING (fallen/Headers/statedef.h)
#define SUB_STATE_VAULTING 52
// uc_orig: SUB_STATE_CLIMBING (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMBING 53

// Sidle/crouch substates.
// uc_orig: SUB_STATE_SIDLE (fallen/Headers/statedef.h)
#define SUB_STATE_SIDLE 60
// uc_orig: SUB_STATE_IDLE_CROUTCH (fallen/Headers/statedef.h)
#define SUB_STATE_IDLE_CROUTCH 61
// uc_orig: SUB_STATE_IDLE_CROUTCHING (fallen/Headers/statedef.h)
#define SUB_STATE_IDLE_CROUTCHING 62
// uc_orig: SUB_STATE_IDLE_CROUTCH_ARREST (fallen/Headers/statedef.h)
#define SUB_STATE_IDLE_CROUTCH_ARREST 63
// uc_orig: SUB_STATE_IDLE_UNCROUCH (fallen/Headers/statedef.h)
#define SUB_STATE_IDLE_UNCROUCH 64

// Fall substate.
// uc_orig: SUB_STATE_FALLING_NORMAL (fallen/Headers/statedef.h)
#define SUB_STATE_FALLING_NORMAL 0

// Combat substates.
// uc_orig: SUB_STATE_PUNCH (fallen/Headers/statedef.h)
#define SUB_STATE_PUNCH 90
// uc_orig: SUB_STATE_KICK (fallen/Headers/statedef.h)
#define SUB_STATE_KICK 91
// uc_orig: SUB_STATE_BLOCK (fallen/Headers/statedef.h)
#define SUB_STATE_BLOCK 92
// uc_orig: SUB_STATE_GRAPPLE (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLE 93
// uc_orig: SUB_STATE_GRAPPLEE (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLEE 94
// uc_orig: SUB_STATE_WALL_KICK (fallen/Headers/statedef.h)
#define SUB_STATE_WALL_KICK 95
// uc_orig: SUB_STATE_STEP (fallen/Headers/statedef.h)
#define SUB_STATE_STEP 96
// uc_orig: SUB_STATE_STEP_FORWARD (fallen/Headers/statedef.h)
#define SUB_STATE_STEP_FORWARD 97
// uc_orig: SUB_STATE_GRAPPLE_HOLD (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLE_HOLD 98
// uc_orig: SUB_STATE_GRAPPLE_HELD (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLE_HELD 99
// uc_orig: SUB_STATE_ESCAPE (fallen/Headers/statedef.h)
#define SUB_STATE_ESCAPE 100
// uc_orig: SUB_STATE_GRAPPLE_ATTACK (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLE_ATTACK 101
// uc_orig: SUB_STATE_HEADBUTT (fallen/Headers/statedef.h)
#define SUB_STATE_HEADBUTT 102
// uc_orig: SUB_STATE_STRANGLE (fallen/Headers/statedef.h)
#define SUB_STATE_STRANGLE 103
// uc_orig: SUB_STATE_HEADBUTTV (fallen/Headers/statedef.h)
#define SUB_STATE_HEADBUTTV 104
// uc_orig: SUB_STATE_STRANGLEV (fallen/Headers/statedef.h)
#define SUB_STATE_STRANGLEV 105

// Carry substates.
// uc_orig: SUB_STATE_PICKUP_CARRY (fallen/Headers/statedef.h)
#define SUB_STATE_PICKUP_CARRY 110
// uc_orig: SUB_STATE_DROP_CARRY (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_CARRY 111
// uc_orig: SUB_STATE_STAND_CARRY (fallen/Headers/statedef.h)
#define SUB_STATE_STAND_CARRY 112
// uc_orig: SUB_STATE_PICKUP_CARRY_V (fallen/Headers/statedef.h)
#define SUB_STATE_PICKUP_CARRY_V 113
// uc_orig: SUB_STATE_DROP_CARRY_V (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_CARRY_V 114
// uc_orig: SUB_STATE_STAND_CARRY_V (fallen/Headers/statedef.h)
#define SUB_STATE_STAND_CARRY_V 115
// uc_orig: SUB_STATE_CARRY_MOVE_V (fallen/Headers/statedef.h)
#define SUB_STATE_CARRY_MOVE_V 116

// Vehicle substates.
// uc_orig: SUB_STATE_ENTERING_VEHICLE (fallen/Headers/statedef.h)
#define SUB_STATE_ENTERING_VEHICLE 140
// uc_orig: SUB_STATE_INSIDE_VEHICLE (fallen/Headers/statedef.h)
#define SUB_STATE_INSIDE_VEHICLE 141
// uc_orig: SUB_STATE_EXITING_VEHICLE (fallen/Headers/statedef.h)
#define SUB_STATE_EXITING_VEHICLE 142
// uc_orig: SUB_STATE_MOUNTING_BIKE (fallen/Headers/statedef.h)
#define SUB_STATE_MOUNTING_BIKE 145
// uc_orig: SUB_STATE_RIDING_BIKE (fallen/Headers/statedef.h)
#define SUB_STATE_RIDING_BIKE 146
// uc_orig: SUB_STATE_DISMOUNTING_BIKE (fallen/Headers/statedef.h)
#define SUB_STATE_DISMOUNTING_BIKE 147

// Death/arrest substates.
// uc_orig: SUB_STATE_DEAD_ARREST_TURN_OVER (fallen/Headers/statedef.h)
#define SUB_STATE_DEAD_ARREST_TURN_OVER 170
// uc_orig: SUB_STATE_DEAD_CUFFED (fallen/Headers/statedef.h)
#define SUB_STATE_DEAD_CUFFED 171
// uc_orig: SUB_STATE_DEAD_ARRESTED (fallen/Headers/statedef.h)
#define SUB_STATE_DEAD_ARRESTED 172
// uc_orig: SUB_STATE_DEAD_INJURED (fallen/Headers/statedef.h)
#define SUB_STATE_DEAD_INJURED 173
// uc_orig: SUB_STATE_DEAD_RESPAWN (fallen/Headers/statedef.h)
#define SUB_STATE_DEAD_RESPAWN 174

// Circling substates.
// uc_orig: SUB_STATE_CIRCLING_CHANGE_POS (fallen/Headers/statedef.h)
#define SUB_STATE_CIRCLING_CHANGE_POS 150
// uc_orig: SUB_STATE_CIRCLING_ATTACK (fallen/Headers/statedef.h)
#define SUB_STATE_CIRCLING_ATTACK 151
// uc_orig: SUB_STATE_CIRCLING_CIRCLE (fallen/Headers/statedef.h)
#define SUB_STATE_CIRCLING_CIRCLE 152
// uc_orig: SUB_STATE_CIRCLING_HORROR (fallen/Headers/statedef.h)
#define SUB_STATE_CIRCLING_HORROR 153
// uc_orig: SUB_STATE_CIRCLING_BACK_OFF (fallen/Headers/statedef.h)
#define SUB_STATE_CIRCLING_BACK_OFF 154

// Wall hug substates.
// uc_orig: SUB_STATE_HUG_WALL_TURN (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_TURN 156
// uc_orig: SUB_STATE_HUG_WALL_STAND (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_STAND 157
// uc_orig: SUB_STATE_HUG_WALL_STEP_LEFT (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_STEP_LEFT 158
// uc_orig: SUB_STATE_HUG_WALL_STEP_RIGHT (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_STEP_RIGHT 159
// uc_orig: SUB_STATE_HUG_WALL_LOOK_L (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_LOOK_L 162
// uc_orig: SUB_STATE_HUG_WALL_LOOK_R (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_LOOK_R 163
// uc_orig: SUB_STATE_HUG_WALL_LEAP_OUT (fallen/Headers/statedef.h)
#define SUB_STATE_HUG_WALL_LEAP_OUT 164

// Search substates.
// uc_orig: SUB_STATE_SEARCH_PRIM (fallen/Headers/statedef.h)
#define SUB_STATE_SEARCH_PRIM 160
// uc_orig: SUB_STATE_SEARCH_CORPSE (fallen/Headers/statedef.h)
#define SUB_STATE_SEARCH_CORPSE 161
// uc_orig: SUB_STATE_SEARCH_GETUP (fallen/Headers/statedef.h)
#define SUB_STATE_SEARCH_GETUP 165

// Climbing/dangling substates.
// uc_orig: SUB_STATE_GRAB_TO_DANGLE (fallen/Headers/statedef.h)
#define SUB_STATE_GRAB_TO_DANGLE 180
// uc_orig: SUB_STATE_DANGLING (fallen/Headers/statedef.h)
#define SUB_STATE_DANGLING 181
// uc_orig: SUB_STATE_PULL_UP (fallen/Headers/statedef.h)
#define SUB_STATE_PULL_UP 182
// uc_orig: SUB_STATE_DROP_DOWN (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_DOWN 183
// uc_orig: SUB_STATE_DROP_DOWN_LAND (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_DOWN_LAND 184
// uc_orig: SUB_STATE_DANGLING_CABLE (fallen/Headers/statedef.h)
#define SUB_STATE_DANGLING_CABLE 185
// uc_orig: SUB_STATE_DANGLING_CABLE_FORWARD (fallen/Headers/statedef.h)
#define SUB_STATE_DANGLING_CABLE_FORWARD 186
// uc_orig: SUB_STATE_DANGLING_CABLE_BACKWARD (fallen/Headers/statedef.h)
#define SUB_STATE_DANGLING_CABLE_BACKWARD 187
// uc_orig: SUB_STATE_DROP_DOWN_OFF_FACE (fallen/Headers/statedef.h)
#define SUB_STATE_DROP_DOWN_OFF_FACE 188
// uc_orig: SUB_STATE_DEATH_SLIDE (fallen/Headers/statedef.h)
#define SUB_STATE_DEATH_SLIDE 189
// uc_orig: SUB_STATE_TRAVERSE_LEFT (fallen/Headers/statedef.h)
#define SUB_STATE_TRAVERSE_LEFT 190
// uc_orig: SUB_STATE_TRAVERSE_RIGHT (fallen/Headers/statedef.h)
#define SUB_STATE_TRAVERSE_RIGHT 191

// Ladder substates.
// uc_orig: SUB_STATE_MOUNT_LADDER (fallen/Headers/statedef.h)
#define SUB_STATE_MOUNT_LADDER 192
// uc_orig: SUB_STATE_ON_LADDER (fallen/Headers/statedef.h)
#define SUB_STATE_ON_LADDER 193
// uc_orig: SUB_STATE_CLIMB_UP_LADDER (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_UP_LADDER 194
// uc_orig: SUB_STATE_CLIMB_DOWN_LADDER (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_DOWN_LADDER 195
// uc_orig: SUB_STATE_CLIMB_OFF_LADDER_BOT (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_OFF_LADDER_BOT 196
// uc_orig: SUB_STATE_CLIMB_OFF_LADDER_TOP (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_OFF_LADDER_TOP 197
// uc_orig: SUB_STATE_CLIMB_DOWN_ONTO_LADDER (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_DOWN_ONTO_LADDER 198

// Wall climbing substates.
// uc_orig: SUB_STATE_CLIMB_LANDING (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_LANDING 210
// uc_orig: SUB_STATE_CLIMB_AROUND_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_AROUND_WALL 211
// uc_orig: SUB_STATE_CLIMB_UP_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_UP_WALL 212
// uc_orig: SUB_STATE_CLIMB_DOWN_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_DOWN_WALL 213
// uc_orig: SUB_STATE_CLIMB_OVER_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_OVER_WALL 214
// uc_orig: SUB_STATE_CLIMB_OFF_BOT_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_OFF_BOT_WALL 215
// uc_orig: SUB_STATE_CLIMB_LANDING2 (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_LANDING2 216
// uc_orig: SUB_STATE_CLIMB_LEFT_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_LEFT_WALL 217
// uc_orig: SUB_STATE_CLIMB_RIGHT_WALL (fallen/Headers/statedef.h)
#define SUB_STATE_CLIMB_RIGHT_WALL 218

// Gun substates.
// uc_orig: SUB_STATE_DRAW_GUN (fallen/Headers/statedef.h)
#define SUB_STATE_DRAW_GUN 220
// uc_orig: SUB_STATE_AIM_GUN (fallen/Headers/statedef.h)
#define SUB_STATE_AIM_GUN 221
// uc_orig: SUB_STATE_SHOOT_GUN (fallen/Headers/statedef.h)
#define SUB_STATE_SHOOT_GUN 222
// uc_orig: SUB_STATE_GUN_AWAY (fallen/Headers/statedef.h)
#define SUB_STATE_GUN_AWAY 223
// uc_orig: SUB_STATE_DRAW_ITEM (fallen/Headers/statedef.h)
#define SUB_STATE_DRAW_ITEM 224
// uc_orig: SUB_STATE_ITEM_AWAY (fallen/Headers/statedef.h)
#define SUB_STATE_ITEM_AWAY 225

// Dying substates.
// uc_orig: SUB_STATE_DYING_INITIAL_ANI (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_INITIAL_ANI 230
// uc_orig: SUB_STATE_DYING_FINAL_ANI (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_FINAL_ANI 231
// uc_orig: SUB_STATE_DYING_ACTUALLY_DIE (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_ACTUALLY_DIE 232
// uc_orig: SUB_STATE_DYING_KNOCK_DOWN (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_KNOCK_DOWN 233 // knocked down but gets back up
// uc_orig: SUB_STATE_DYING_GET_UP_AGAIN (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_GET_UP_AGAIN 234
// uc_orig: SUB_STATE_DYING_PRONE (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_PRONE 235 // falling as a projectile while dying
// uc_orig: SUB_STATE_DYING_KNOCK_DOWN_WAIT (fallen/Headers/statedef.h)
#define SUB_STATE_DYING_KNOCK_DOWN_WAIT 236

// Hit/attack substates.
// uc_orig: SUB_STATE_TAKE_HIT (fallen/Headers/statedef.h)
#define SUB_STATE_TAKE_HIT 240
// uc_orig: SUB_STATE_ATTACKING (fallen/Headers/statedef.h)
#define SUB_STATE_ATTACKING 241
// uc_orig: SUB_STATE_ACROBATIC (fallen/Headers/statedef.h)
#define SUB_STATE_ACROBATIC 242

// Entering vehicle on foot.
// uc_orig: SUB_STATE_DRIVING_ENTER_CAR (fallen/Headers/statedef.h)
#define SUB_STATE_DRIVING_ENTER_CAR 243

// Grappling substates.
// uc_orig: SUB_STATE_GRAPPLING_PICKUP (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLING_PICKUP 244
// uc_orig: SUB_STATE_GRAPPLING_WINDUP (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLING_WINDUP 245
// uc_orig: SUB_STATE_GRAPPLING_RELEASE (fallen/Headers/statedef.h)
#define SUB_STATE_GRAPPLING_RELEASE 246

// Canning (picking up cans/items) substates.
// uc_orig: SUB_STATE_CANNING_PICKUP (fallen/Headers/statedef.h)
#define SUB_STATE_CANNING_PICKUP 247
// uc_orig: SUB_STATE_CANNING_RELEASE (fallen/Headers/statedef.h)
#define SUB_STATE_CANNING_RELEASE 248
// uc_orig: SUB_STATE_CANNING_GET_SPECIAL (fallen/Headers/statedef.h)
#define SUB_STATE_CANNING_GET_SPECIAL 249 // bending down to pick up a special (not a can/head)
// uc_orig: SUB_STATE_CANNING_GET_BARREL (fallen/Headers/statedef.h)
#define SUB_STATE_CANNING_GET_BARREL 250
// uc_orig: SUB_STATE_CANNING_GOT_SPECIAL (fallen/Headers/statedef.h)
#define SUB_STATE_CANNING_GOT_SPECIAL 251

#endif // ACTORS_CORE_STATEDEF_H
