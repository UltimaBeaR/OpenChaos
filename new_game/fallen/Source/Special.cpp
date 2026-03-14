// claude-ai: Special.cpp — Weapon and item system: spawning, pickup, drop, use, and per-tick update.
// claude-ai: "Special" = any pickupable/droppable item (weapon, ammo, health, key, explosive, etc.)
// claude-ai: KEY SYSTEMS IN THIS FILE:
// claude-ai:   SPECIAL_info[] — table mapping SPECIAL_* type -> display name + mesh ObjectId + group
// claude-ai:   special_normal() — per-tick StateFn for all specials (projectile physics, timer countdowns,
// claude-ai:                       grenade/explosive detonation, mine proximity check, pickup radius)
// claude-ai:   person_get_item() — executes item pickup (ammo stacking, inventory add, UI message)
// claude-ai:   should_person_get_item() — AI/player pickup eligibility (ammo caps, one-knife rule, etc.)
// claude-ai:   alloc_special() / free_special() — create/destroy a Special Thing in the world
// claude-ai:   special_pickup() / special_drop() — attach/detach from person's SpecialList linked list
// claude-ai:   SPECIAL_throw_grenade() / SPECIAL_prime_grenade() — grenade arming and throwing
// claude-ai:   SPECIAL_set_explosives() — plant a C4-style explosive (5-second fuse)
// claude-ai: PORT: port all of this. The item system is central to gameplay and mission scripting.
// claude-ai: NOTE: SPECIAL_MINE pickup is permanently disabled (ASSERT(0) in person_get_item).
// claude-ai:       SPECIAL_info[] maps to old type codes — check Special.h for SPECIAL_* constants.
//	Special.cpp
//	Guy Simmons, 28th March 1998.
//
//
// 
// WILLIAM SHAKESPEARE (1564-1616)
//
// Shall I compare thee to a summer's day?
// Thou art more lovely and more temperate.
// Rough winds do shake the darling buds of May,
// And summer's lease hath all too short a date.
// Sometime too hot the eye of heaven shines,
// And often is his gold complexion dimm'd;
// And every fair from fair sometime declines,
// By chance or nature's changing course untrimm'd;
// But thy eternal summer shall not fade
// Nor lose possession of that fair thou ow'st;
// Nor shall Death brag thou wander'st in his shade,
// When in eternal lines to time thou grow'st:
// So long as men can breathe or eyes can see,
// So long lives this, and this gives life to thee. 
// 


#include	"Game.h"
#include	"StateDef.h"
#include	"Special.h"
#include	"eway.h"
#include	"pcom.h"
#include	"night.h"
#include	"dirt.h"
#include	"animate.h"
#include	"cnet.h"
#include	"pow.h"
#include	"memory.h"
#include	"xlat_str.h"
#include	"sound.h"
#include	"grenade.h"
#ifndef		PSX
#include	"..\ddengine\headers\panel.h"
#else
#include	"..\psxeng\headers\panel.h"
#endif

extern	void	add_damage_text(SWORD x,SWORD y,SWORD z,CBYTE *text);




// claude-ai: SPECIAL_info[] — master table for all item types, indexed by SPECIAL_* enum.
// claude-ai: Columns: display name (for debug), PRIM_OBJ_* mesh ID (used to set dm->ObjectId), group flags.
// claude-ai: SPECIAL_GROUP_*: USEFUL=mission item, COOKIE=stat boost, ONEHANDED_WEAPON, TWOHANDED_WEAPON,
// claude-ai:                   AMMO=ammo pack, STRANGE=explosive/exotic.
// claude-ai: Note: index ordering here does NOT match the SPECIAL_* constants in Special.h from the
// claude-ai:       knowledge base — this appears to be a different/older enum mapping used in this pre-release.
// claude-ai: PORT: replicate this table to map item types to their 3D mesh IDs and behavioural groups.
SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES] =
{
	{"None",			0,							0								},
	{"Key",				PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"Gun",				PRIM_OBJ_ITEM_GUN,			SPECIAL_GROUP_ONEHANDED_WEAPON	},
	{"Health",			PRIM_OBJ_ITEM_HEALTH,		SPECIAL_GROUP_COOKIE			},
	{"Bomb",			PRIM_OBJ_THERMODROID,		SPECIAL_GROUP_STRANGE			},
	{"Shotgun",			PRIM_OBJ_ITEM_SHOTGUN,		SPECIAL_GROUP_TWOHANDED_WEAPON	},
	{"Knife",			PRIM_OBJ_ITEM_KNIFE,		SPECIAL_GROUP_ONEHANDED_WEAPON	},
	{"Explosives",		PRIM_OBJ_ITEM_EXPLOSIVES,	SPECIAL_GROUP_USEFUL			},
	{"Grenade",			PRIM_OBJ_ITEM_GRENADE,		SPECIAL_GROUP_ONEHANDED_WEAPON	},
	{"Ak47",			PRIM_OBJ_ITEM_AK47,			SPECIAL_GROUP_TWOHANDED_WEAPON	},
	{"Mine",			PRIM_OBJ_MINE,				SPECIAL_GROUP_STRANGE			},
	{"Thermodroid",		PRIM_OBJ_THERMODROID,		SPECIAL_GROUP_STRANGE			},
	{"Baseballbat",		PRIM_OBJ_ITEM_BASEBALLBAT,	SPECIAL_GROUP_TWOHANDED_WEAPON	},
	{"Ammo pistol",		PRIM_OBJ_ITEM_AMMO_PISTOL,	SPECIAL_GROUP_AMMO				},
	{"Ammo shotgun",	PRIM_OBJ_ITEM_AMMO_SHOTGUN, SPECIAL_GROUP_AMMO				},
	{"Ammo AK47",		PRIM_OBJ_ITEM_AMMO_AK47,	SPECIAL_GROUP_AMMO				},
	{"Keycard",			PRIM_OBJ_ITEM_KEYCARD,		SPECIAL_GROUP_USEFUL			},
	{"File",			PRIM_OBJ_ITEM_FILE,			SPECIAL_GROUP_USEFUL			},
	{"Floppy_disk",		PRIM_OBJ_ITEM_FLOPPY_DISK,	SPECIAL_GROUP_USEFUL			},
	{"Crowbar",			PRIM_OBJ_ITEM_CROWBAR,		SPECIAL_GROUP_USEFUL			},
	{"Video",			PRIM_OBJ_ITEM_VIDEO,		SPECIAL_GROUP_USEFUL			},
	{"Gloves",			PRIM_OBJ_ITEM_GLOVES,		SPECIAL_GROUP_USEFUL			},
	{"WeedAway",		PRIM_OBJ_ITEM_WEEDKILLER,	SPECIAL_GROUP_USEFUL			},
	{"Badge",			PRIM_OBJ_ITEM_TREASURE,		SPECIAL_GROUP_COOKIE			},
	{"Red Car Keys",	PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"Blue Car Keys",	PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"Green Car Keys",	PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"Black Car Keys",	PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"White Car Keys",	PRIM_OBJ_ITEM_KEY,			SPECIAL_GROUP_USEFUL			},
	{"Wire Cutters",	PRIM_OBJ_ITEM_WRENCH,		SPECIAL_GROUP_USEFUL			},
};



void free_special(Thing *s_thing);


//
// Adds/removes the special from a person's 
//

// claude-ai: special_pickup — attaches a Special to a person's SpecialList (singly-linked list).
// claude-ai: Sets OwnerThing and prepends to SpecialList. Clears PROJECTILE substate.
// claude-ai: NOTE: does NOT remove the special from the map — caller must call remove_thing_from_map().
// claude-ai: SPECIAL_MINE is explicitly forbidden here (mines cannot be picked up).
void special_pickup(Thing *p_special, Thing *p_person)
{
	ASSERT(p_special->Class == CLASS_SPECIAL);
	ASSERT(p_person ->Class == CLASS_PERSON);

	ASSERT(p_special->Genus.Special->SpecialType!=SPECIAL_MINE);

	p_special->Genus.Special->OwnerThing  = THING_NUMBER(p_person);
	p_special->Genus.Special->NextSpecial = p_person->Genus.Person->SpecialList;
	p_person->Genus.Person->SpecialList   = THING_NUMBER(p_special);

	if (p_special->SubState == SPECIAL_SUBSTATE_PROJECTILE)
	{
		p_special->SubState = SPECIAL_SUBSTATE_NONE;
	}
}

// claude-ai: special_drop — detaches special from person's SpecialList and places it in the world.
// claude-ai: Walks the linked list to find the item, splices it out, then calls find_nice_place_near_person()
// claude-ai: to choose a drop position (avoids dropping through walls).
// claude-ai: Dropped guns (for player only) get randomized partial ammo: shotgun 2-3, AK47 6-13 rounds.
// claude-ai: Velocity is set to 5 to prevent immediate re-pickup (5-gameturn cooldown).
// claude-ai: Also clears SpecialUse and SpecialDraw references on the person.
void special_drop(Thing *p_special, Thing *p_person)
{
	#ifndef NDEBUG
	SLONG count = 0;
	#endif

	ASSERT(p_special->Class == CLASS_SPECIAL);
	ASSERT(p_person ->Class == CLASS_PERSON );
	ASSERT(p_special->Genus.Special->SpecialType!=SPECIAL_MINE);

	UWORD  next =  p_person->Genus.Person->SpecialList;
	UWORD *prev = &p_person->Genus.Person->SpecialList;

	while(1)
	{
#ifndef NDEBUG
		ASSERT(count++ < 100);
#endif

		if (next == NULL)
		{
			//
			// We haven't found the special... is this an error? I guess so.
			//

//			ASSERT(0);

			return;
		}

		Thing* p_next = TO_THING(next);
		ASSERT(p_next->Class == CLASS_SPECIAL);

		if (next == THING_NUMBER(p_special))
		{
			//
			// This is the special we want to drop. Take it out the the linked list.
			//

		   *prev                                  = p_special->Genus.Special->NextSpecial;
		    p_special->Genus.Special->NextSpecial = NULL;
			p_special->Genus.Special->OwnerThing  = NULL;

			{
				SLONG gx;
				SLONG gy;
				SLONG gz;

				extern void find_nice_place_near_person(	// In person.cpp
								Thing *p_person,
								SLONG *nice_x,	// 8-bits per mapsquare
								SLONG *nice_y,
								SLONG *nice_z);

				find_nice_place_near_person(
					p_person,
				   &gx,
				   &gy,
				   &gz);

				p_special->WorldPos.X = gx << 8;
				p_special->WorldPos.Y = gy << 8;
				p_special->WorldPos.Z = gz << 8;
			}

			//
			// Put it on the map.
			//

			add_thing_to_map(p_special);

			// claude-ai: DROP AMMO RANDOMISATION — only applies to player-dropped weapons (PlayerID==0).
			// claude-ai: When player drops a shotgun: ammo set to (Random()&1)+2 = 2 or 3 shells.
			// claude-ai: When player drops an AK47:   ammo set to (Random()&7)+6 = 6 to 13 rounds.
			// claude-ai: Purpose: prevents player from exploiting drop/pickup cycle to keep full ammo.
			// claude-ai: Enemy-dropped weapons retain their original ammo (no randomisation).
			// claude-ai: Velocity=5 sets a 5-gameturn flag. counter=0 starts the ~1-second pickup cooldown (320 ticks).
			// claude-ai: NOTE: the Velocity=5 cooldown is actually the old system (now commented out in
			//            special_normal). The counter>16*20 check is the active cooldown mechanism.
			if (p_person->Genus.Person->PlayerID == 0)
			{
				//
				// Don't drop a gun with MAX ammo.
				//

				switch(p_special->Genus.Special->SpecialType)
				{
					case SPECIAL_SHOTGUN:
						p_special->Genus.Special->ammo = (Random() & 0x1) + 2;
						break;

					case SPECIAL_AK47:
						p_special->Genus.Special->ammo = (Random() & 0x7) + 6;
						break;
				}
			}
			p_special->Velocity               = 5; // can't pick it up for 5 gameturns!
			p_special->Genus.Special->counter = 0;	// Can't pick it up for one second.
			p_special->SubState               = SPECIAL_SUBSTATE_NONE;

			if (next == p_person->Genus.Person->SpecialUse)
			{
				p_person->Genus.Person->SpecialUse = NULL;
			}

			if (next == p_person->Genus.Person->SpecialDraw)
			{
				p_person->Genus.Person->SpecialDraw = NULL;
			}

			return;
		}

		prev = &p_next->Genus.Special->NextSpecial;
		next =  p_next->Genus.Special->NextSpecial;
	}
}


//
// Returns TRUE if the person is carrying a two-handed weapon.
//
// claude-ai: person_has_twohanded_weapon — checks SpecialList for any two-handed weapon.
// claude-ai: Two-handed weapons (shotgun, AK47, bat) prevent certain actions while held.
// claude-ai: Returns non-NULL (truthy) if any of SHOTGUN, AK47, or BASEBALLBAT is in inventory.
// claude-ai: Used by: animation system (prevents drawing/holstering one-handed), AI weapon selection,
// claude-ai:   and driving check (can't enter vehicle holding two-handed weapon in some cases).
// claude-ai: KNIFE is NOT two-handed — it can be held alongside other items.
// claude-ai: PISTOL is NOT two-handed — stored as a flag (FLAGS_HAS_GUN), not in SpecialList.
SLONG person_has_twohanded_weapon(Thing *p_person)
{
	return (person_has_special(p_person, SPECIAL_SHOTGUN) ||
			person_has_special(p_person, SPECIAL_AK47   ) ||
			person_has_special(p_person, SPECIAL_BASEBALLBAT));
}


//
// Should this person pick up the item?
//

// claude-ai: should_person_get_item — pickup eligibility check for both player and AI.
// claude-ai: Checks whether p_person WANTS/CAN pick up p_special given current inventory.
// claude-ai: Rules:
// claude-ai:   GUN: always if no gun; otherwise only if ammo_packs_pistol has room (<255-15).
// claude-ai:   HEALTH: only if below max health.
// claude-ai:   SHOTGUN/AK47: pick up if not carrying one, or if carrying but has ammo space.
// claude-ai:   KNIFE/BASEBALLBAT: only one allowed.
// claude-ai:   EXPLOSIVES: max 4 charges.
// claude-ai:   GRENADE: max 8 grenades (ammo field on the grenade special).
// claude-ai:   AMMO_*: only if there's room in ammo_packs (below 255 - pack size).
// claude-ai:   MINE: always FALSE — can't be picked up by design.
// claude-ai:   Default (quest items etc.): TRUE.
// claude-ai: Also blocks pickup during RUNNING_SKID_STOP (slide deceleration) to prevent grab glitches.
SLONG should_person_get_item(Thing *p_person, Thing *p_special)
{
	Thing *p_grenade;
	Thing *p_explosives;
	Thing *p_has;

	/*
	if(p_special->Velocity)
		return(0);
	*/

	if (p_person->State    == STATE_MOVEING &&
		p_person->SubState == SUB_STATE_RUNNING_SKID_STOP)
	{
		return FALSE;
	}

	// claude-ai: should_person_get_item switch — returns TRUE/FALSE per item type.
	// claude-ai: Pistol (SPECIAL_GUN): pick up if no gun yet, OR if reserve packs have room for 15 more rounds.
	// claude-ai:   Old check (commented out): "if clip < 15 pick up". New check: ammo_packs cap.
	// claude-ai:   The old approach could leave ammo on the ground when clip was full but packs weren't.
	// claude-ai: Health: only pick up if below max health (so the item stays available for later).
	// claude-ai: SPECIAL_BOMB: never pickupable by anyone (return FALSE always).
	// claude-ai: AK47/Shotgun: pick up if not owned OR ammo_packs have room for one more magazine.
	// claude-ai: BASEBALLBAT: one at a time (two-handed constraint). KNIFE: only one allowed.
	// claude-ai: EXPLOSIVES: max 4 charges. GRENADE: max 8. MINE: permanently blocked (FALSE).
	// claude-ai: Default (mission items, keys, etc.): always pick up (TRUE).
	switch(p_special->Genus.Special->SpecialType)
	{
		case SPECIAL_GUN:

			if (p_person->Flags & FLAGS_HAS_GUN)
			{
				//return p_person->Genus.Person->Ammo < SPECIAL_AMMO_IN_A_PISTOL;		// If less than 15 rounds of ammo
				return (p_person->Genus.Person->ammo_packs_pistol<255-SPECIAL_AMMO_IN_A_PISTOL);
			}
			else
			{
				//
				// Always pick up a gun if you haven't got one.
				//

				return TRUE;
			}

		case SPECIAL_HEALTH:
			return (p_person->Genus.Person->Health < health[p_person->Genus.Person->PersonType]);	// If not at maximum health

		case SPECIAL_BOMB:
			return FALSE;

		case SPECIAL_AK47:

			p_has = person_has_special(p_person, SPECIAL_AK47);

			return (!p_has || p_person->Genus.Person->ammo_packs_ak47 < 255-SPECIAL_AMMO_IN_A_AK47);

		case SPECIAL_BASEBALLBAT:
			
			//
			// We can only carry one two-handed weapon at once.
			//

			return !person_has_special(p_person, SPECIAL_BASEBALLBAT);

		case SPECIAL_SHOTGUN:
			
			p_has = person_has_special(p_person, SPECIAL_SHOTGUN);

			return (!p_has || p_person->Genus.Person->ammo_packs_shotgun < 255-SPECIAL_AMMO_IN_A_SHOTGUN);

		case SPECIAL_KNIFE:

			//
			// We can only have one knife.
			//

			return !person_has_special(p_person, SPECIAL_KNIFE);

		case SPECIAL_EXPLOSIVES:

			p_explosives = person_has_special(p_person, SPECIAL_EXPLOSIVES);

			//
			// You can only carry 4 explosives.
			//

			return (p_explosives == NULL || p_explosives->Genus.Special->ammo < 4);

		case SPECIAL_GRENADE:

			p_grenade = person_has_special(p_person, SPECIAL_GRENADE);

			//
			// You can only carry 8 grenades.
			//

			return (p_grenade == NULL || p_grenade->Genus.Special->ammo < 8);

		//
		// Only pickup ammo when you have that weapon.
		//

		// claude-ai: The second return lines below are DEAD CODE — unreachable after the first return.
		// claude-ai: They are the old check (must own that weapon type) replaced by the ammo cap check above.
		case SPECIAL_AMMO_SHOTGUN:
			return(p_person->Genus.Person->ammo_packs_shotgun<255-SPECIAL_AMMO_IN_A_SHOTGUN);
			return (SLONG) person_has_special(p_person, SPECIAL_SHOTGUN);
		case SPECIAL_AMMO_AK47:
			return(p_person->Genus.Person->ammo_packs_ak47<255-SPECIAL_AMMO_IN_A_AK47);
			return (SLONG) person_has_special(p_person, SPECIAL_AK47);
		case SPECIAL_AMMO_PISTOL:
			return(p_person->Genus.Person->ammo_packs_pistol<255-SPECIAL_AMMO_IN_A_PISTOL);
			return p_person->Flags & FLAGS_HAS_GUN;

		case	SPECIAL_MINE:
			return(FALSE);  //added my MD

		default:
			return TRUE;
	}
}




//
// The player person has picked up the given item.
//

// claude-ai: person_get_item — executes the actual pickup for one specific item.
// claude-ai: Called after should_person_get_item() returns TRUE.
// claude-ai: Handles each SPECIAL_* type with its own logic:
// claude-ai:   GUN: add ammo if already have gun; else grant gun + set ammo.
// claude-ai:       Excess ammo overflows into ammo_packs_pistol in packs of 15.
// claude-ai:   HEALTH: +100 HP, capped at character's max health.
// claude-ai:   SHOTGUN/AK47: if already owned, merge ammo (overflow to ammo_packs); else add to inventory.
// claude-ai:   GRENADE: merge ammo onto existing grenade, or add grenade to inventory.
// claude-ai:   EXPLOSIVES: add charge, or add to inventory.
// claude-ai:   AMMO_*: directly increment ammo_packs_* counters.
// claude-ai:   TREASURE (badge): pick stat to increase based on ObjectId (71=Skill, 94=Strength, 81=Stamina, 39=Con).
// claude-ai:   Default (knife, bat, key, video, etc.): call special_pickup() and keep in inventory.
// claude-ai: x_message is set to show a HUD info message for the player.
// claude-ai: If EWAY waypoint attached, notifies waypoint system of pickup.
// claude-ai: keep=FALSE means free_special() is called at the end; keep=TRUE preserves the Thing.
void person_get_item(Thing *p_person, Thing *p_special)
{
	Thing *p_gun;

	SLONG keep      = FALSE;
	SLONG x_message = 0;
	SLONG overflow  = 0; // ammo left over

//	PANEL_new_text(0,4000," PICKUP special %d \n",p_special->Genus.Special->SpecialType);

	// claude-ai: person_get_item — weapon-by-weapon ammo and inventory handling:
	// claude-ai:
	// claude-ai: AMMO STORAGE ARCHITECTURE:
	// claude-ai:   Pistol:  Person->Ammo (UBYTE, 0-255) = current clip; ammo_packs_pistol = reserve packs.
	// claude-ai:            Each pack = 15 rounds (SPECIAL_AMMO_IN_A_PISTOL). Pick up gun = 15 rounds.
	// claude-ai:   Shotgun: Special->ammo (on the shotgun Thing) = clip; ammo_packs_shotgun = reserve.
	// claude-ai:            Each pack/clip = 8 shells (SPECIAL_AMMO_IN_A_SHOTGUN).
	// claude-ai:   AK47:    Special->ammo = clip; ammo_packs_ak47 = reserve.
	// claude-ai:            Each pack/clip = 30 rounds (SPECIAL_AMMO_IN_A_AK47).
	// claude-ai:   Grenade: Special->ammo = count in inventory (max 8).
	// claude-ai:            Each grenade item carries SPECIAL_AMMO_IN_A_GRENADE=3 grenades.
	// claude-ai:   C4:      Special->ammo = charges (max 4); no separate packs.
	// claude-ai:
	// claude-ai: FIRING / RELOADING (in Person.cpp shoot_get_ammo_sound_anim_time()):
	// claude-ai:   Each shot: p_special->ammo -= 1. When ammo hits 0, pull from ammo_packs_*.
	// claude-ai:   Reload: take min(SPECIAL_AMMO_IN_A_*,  ammo_packs_*) → fill clip, decrement packs.
	// claude-ai:   Returns HAD_TO_CHANGE_CLIP if a reload happened (suppresses sound this frame).
	// claude-ai:   MIB enemies: unlimited AK47 ammo (ammo++ on each shot instead of decrement).
	// claude-ai:   Enemy NPCs: AK47 ammo never runs out (see shoot_get_ammo_sound_anim_time in Person.cpp).
	// claude-ai:
	// claude-ai: WEAPON FIRING DAMAGE (from get_shoot_damage() in Person.cpp):
	// claude-ai:   Pistol   (SPECIAL_GUN):     HIT_TYPE_GUN_SHOT_PISTOL,  damage = 70 HP.
	// claude-ai:   Shotgun  (SPECIAL_SHOTGUN):  HIT_TYPE_GUN_SHOT_SHOTGUN, damage = 300 - dist (dist scaled).
	// claude-ai:                                Saturated 0-250. At close range: ~250 HP (near one-shot).
	// claude-ai:                                Max useful range: ~0x600 map units (beyond that = 0 damage).
	// claude-ai:   AK47     (SPECIAL_AK47):     HIT_TYPE_GUN_SHOT_AK47,    damage = 100 (player) / 40 (NPC).
	// claude-ai:   All guns vs player target: damage >>= 1 (halved) to keep player alive longer.
	// claude-ai:
	// claude-ai: FIRE ANIMATION TIMING (Timer1 cooldown in ticks):
	// claude-ai:   Pistol:  *time = 140 ticks between shots.
	// claude-ai:   Shotgun: *time = 400 ticks between shots.
	// claude-ai:   AK47:    *time = 64 ticks between shots (rapid fire).
	// claude-ai:   MIB AK47: *time = 64 ticks (same rate, no ammo limit).
	// claude-ai:
	// claude-ai: HIT CHANCE FORMULA (get_shoot_damage → chance):
	// claude-ai:   Base chance for person: 230 - (abs(Roll)>>1) - Velocity.
	// claude-ai:   Player aiming: +64 bonus. AI vs player: -64 penalty. AI vs AI: +100 bonus.
	// claude-ai:   Distance penalty: for dist > 0x400, penalty = dist>>3 (pistol/AK47) or dist>>2 (shotgun).
	// claude-ai:   Flip/dodge action: chance halved. Skill >= 7: random dodge roll.
	// claude-ai:   Shotgun spread: 5 extra skill needed to dodge (shotgun harder to dodge than pistol).
	// claude-ai:   Always minimum 20/256 = ~8% chance to hit (never pure miss).
	switch(p_special->Genus.Special->SpecialType)
	{
		// claude-ai: SPECIAL_GUN pickup:
		// claude-ai:   Already have gun → merge ammo into clip; overflow into packs in chunks of 15.
		// claude-ai:   No gun yet → set FLAGS_HAS_GUN flag on the Thing, take clip ammo (15 rounds).
		// claude-ai:   The GUN itself is NOT stored in SpecialList — it's just a flag + Person->Ammo UBYTE.
		// claude-ai:   This is why should_person_get_item returns TRUE (pick up another pistol) only when
		//              ammo_packs_pistol has room — picking up the gun is really just picking up ammo.
		case SPECIAL_GUN:

			if (p_person->Flags & FLAGS_HAS_GUN)
			{
				x_message = X_AMMO;

				overflow=(SWORD)p_person->Genus.Person->Ammo + p_special->Genus.Special->ammo;
				while (overflow>15)
				{
					p_person->Genus.Person->ammo_packs_pistol += 15;
					overflow-=15;
				}
				p_person->Genus.Person->Ammo=overflow;

/*				if ((SWORD)p_person->Genus.Person->Ammo + p_special->Genus.Special->ammo<256)
					p_person->Genus.Person->Ammo += p_special->Genus.Special->ammo;
				else
					p_person->Genus.Person->Ammo = 255;*/
			}
			else
			{
				x_message = X_GUN;

				p_person->Genus.Person->Ammo = p_special->Genus.Special->ammo;
				p_person->Flags             |= FLAGS_HAS_GUN;
			}

			break;

		// claude-ai: SPECIAL_HEALTH pickup: restores 100 HP, capped at character's max health.
		// claude-ai: health[] is extern array indexed by PersonType (e.g. PersonType 0 = Darci = 200 HP max).
		// claude-ai: Old hardcoded cap of 200 is commented out — now uses per-person-type table.
		// claude-ai: Shows X_HEALTH HUD message. Item is destroyed (keep=FALSE by default).
		case SPECIAL_HEALTH:

			x_message = X_HEALTH;

			p_person->Genus.Person->Health += 100;

			if (p_person->Genus.Person->Health > health[p_person->Genus.Person->PersonType])
			{
				p_person->Genus.Person->Health = health[p_person->Genus.Person->PersonType];
//				p_person->Genus.Person->Health = 200;
			}

			break;

		// claude-ai: SPECIAL_AMMO_SHOTGUN pickup: adds SPECIAL_AMMO_IN_A_SHOTGUN (8) shells to reserve packs.
		// claude-ai: No cap check here — the cap is enforced in should_person_get_item (ammo_packs_shotgun < 255-8).
		// claude-ai: All ammo packs show generic X_AMMO HUD message, not weapon-specific message.
		case SPECIAL_AMMO_SHOTGUN:
			p_person->Genus.Person->ammo_packs_shotgun += SPECIAL_AMMO_IN_A_SHOTGUN;

			x_message = X_AMMO;

			break;

		// claude-ai: SPECIAL_AMMO_AK47 pickup: adds SPECIAL_AMMO_IN_A_AK47 (30) rounds to reserve packs.
		// claude-ai: Same pattern as shotgun ammo — cap enforced in should_person_get_item.
		case SPECIAL_AMMO_AK47:

			p_person->Genus.Person->ammo_packs_ak47 += SPECIAL_AMMO_IN_A_AK47;

			x_message = X_AMMO;

			break;

		// claude-ai: SPECIAL_AMMO_PISTOL pickup: adds SPECIAL_AMMO_IN_A_PISTOL (15) rounds to reserve packs.
		// claude-ai: Note: pistol packs go into ammo_packs_pistol (UBYTE), not directly into Person->Ammo (clip).
		// claude-ai: The reload system in shoot_get_ammo_sound_anim_time pulls from packs into the clip on empty.
		case SPECIAL_AMMO_PISTOL:

			p_person->Genus.Person->ammo_packs_pistol += SPECIAL_AMMO_IN_A_PISTOL;

			x_message = X_AMMO;
			
			break;

		// claude-ai: SPECIAL_MINE pickup — PERMANENTLY DISABLED.
		// claude-ai: ASSERT(0) fires if this case is ever reached — should never happen since
		// claude-ai:   should_person_get_item returns FALSE for MINE unconditionally.
		// claude-ai: The commented-out code below shows the original intent: mines were once pickupable.
		// claude-ai: They would have been carried (special_pickup) and then thrown (SPECIAL_throw_mine).
		// claude-ai: The SubState == ACTIVATED check was the trigger for arming when placed.
		// claude-ai: In the final game: mines are ONLY placed via alloc_special() from level scripts.
		case SPECIAL_MINE:

			//
			// You can't pick up mines any more.
			//

			ASSERT(0);

			/*
	
			ASSERT(p_special->SubState == SPECIAL_SUBSTATE_ACTIVATED);

			//
			// When the special comes to draw itself, it will draw itself at that
			// person's hand.  We don't remove it from the map or free up the special.
			//

			special_pickup(p_special, p_person);

			keep = TRUE;

			*/

			break;

		// claude-ai: SPECIAL_SHOTGUN pickup:
		// claude-ai:   If already carrying shotgun → merge ammo into the carried weapon's ammo field.
		// claude-ai:     Overflow goes to ammo_packs_shotgun in chunks of SPECIAL_AMMO_IN_A_SHOTGUN (8 shells).
		// claude-ai:   If no shotgun → call special_pickup() + remove from map + keep=TRUE.
		// claude-ai:     The shotgun Thing stays alive in SpecialList — it IS the weapon object.
		// claude-ai:     This differs from pistol (no Thing stored) and knife (also stored, but melee).
		// claude-ai:   The shotgun Thing's ammo field = shells left in the magazine.
		case SPECIAL_SHOTGUN:

			{
				Thing *p_gun = person_has_special(p_person, SPECIAL_SHOTGUN);

				if (p_gun)
				{
/*					p_has->Genus.Special->ammo += p_special->Genus.Special->ammo;

					if (p_has->Genus.Special->ammo > SPECIAL_AMMO_IN_A_SHOTGUN)
					{
						p_has->Genus.Special->ammo = SPECIAL_AMMO_IN_A_SHOTGUN;
					}*/
					overflow=(SWORD)p_gun->Genus.Special->ammo + p_special->Genus.Special->ammo;
					while (overflow>SPECIAL_AMMO_IN_A_SHOTGUN)
					{
						p_person->Genus.Person->ammo_packs_shotgun += SPECIAL_AMMO_IN_A_SHOTGUN;
						overflow-=SPECIAL_AMMO_IN_A_SHOTGUN;
					}
					p_gun->Genus.Special->ammo=overflow;


					x_message = X_AMMO;

					break;
				}
				else
				{
					special_pickup(p_special, p_person);

					//
					// Make sure it isn't drawn!
					//

					remove_thing_from_map(p_special);

					//
					// Dont free up the special.
					//

					keep = TRUE;

					x_message = X_SHOTGUN;

					break;
				}
			}

		// claude-ai: SPECIAL_AK47 pickup: identical pattern to SPECIAL_SHOTGUN.
		// claude-ai:   Already carrying AK47 → merge ammo; overflow → ammo_packs_ak47 in 30-round chunks.
		// claude-ai:   New AK47 → special_pickup() + remove from map + keep=TRUE (Thing stored in list).
		// claude-ai:   Animation: ANIM_AK_FIRE (64-tick cooldown). Sound: S_AK47_BURST (sounds better than S_AK47_SHOT).
		// claude-ai:   Muzzle flash: 3 brass casings ejected (vs 1 for pistol) — cosmetic emphasis on rapid fire.
		case SPECIAL_AK47:

			{
				Thing *p_gun = person_has_special(p_person, SPECIAL_AK47);

				if (p_gun)
				{
/*					p_has->Genus.Special->ammo += p_special->Genus.Special->ammo;

					if (p_has->Genus.Special->ammo > SPECIAL_AMMO_IN_A_AK47)
					{
						p_has->Genus.Special->ammo = SPECIAL_AMMO_IN_A_AK47;
					}*/
					overflow=(SWORD)p_gun->Genus.Special->ammo + p_special->Genus.Special->ammo;
					while (overflow>SPECIAL_AMMO_IN_A_AK47)
					{
						p_person->Genus.Person->ammo_packs_ak47 += SPECIAL_AMMO_IN_A_AK47;
						overflow-=SPECIAL_AMMO_IN_A_AK47;
					}
					p_gun->Genus.Special->ammo=overflow;


					x_message = X_AMMO;
				}
				else
				{
					special_pickup(p_special, p_person);

					//
					// Make sure it isn't drawn!
					//

					remove_thing_from_map(p_special);

					//
					// Dont free up the special.
					//

					keep = TRUE;

					x_message = X_AK;
				}

				break;
			}

		// claude-ai: SPECIAL_GRENADE pickup:
		// claude-ai:   Already have grenades → ammo += 3 (SPECIAL_AMMO_IN_A_GRENADE), capped at 100.
		// claude-ai:     (The cap of 100 is a safety guard, real max enforced by should_person_get_item = 8.)
		// claude-ai:   No grenades → special_pickup() + remove from map + keep=TRUE. The grenade Thing
		// claude-ai:     stays in SpecialList and its ammo field = count of grenades in inventory.
		// claude-ai: GRENADE USE (Person.cpp, shoot_get_ammo_sound_anim_time):
		// claude-ai:   NPC throwing: calls SPECIAL_prime_grenade() + set_person_can_release() for auto-throw.
		// claude-ai:   Player throwing: controlled via player input, timed cook by how long button is held.
		// claude-ai: GRENADE IN-FLIGHT (grenade.cpp, ProcessGrenade()):
		// claude-ai:   Physics: gravity = -2 * TICK_RATIO per tick (stronger than item drop gravity of -256).
		// claude-ai:   Max downward velocity: -0x2000. Bounces off floor (50% velocity) and walls (50% dx/dz).
		// claude-ai:   Timer counts down; on expiry: CreateGrenadeExplosion().
		// claude-ai:   Every 16 ticks: PCOM_oscillate_tympanum(PCOM_SOUND_GRENADE_FLY) — alerts NPCs.
		// claude-ai: GRENADE EXPLOSION (CreateGrenadeExplosion in grenade.cpp):
		// claude-ai:   Particle effects: smoke cloud + additive explosion sprites.
		// claude-ai:   create_shockwave: radius=0x300, maxdamage=500.
		// claude-ai:   Damage at edge of 0x300 radius: 0. At centre: 500 HP (lethal to all NPCs).
		// claude-ai:   DIRT_gust in 4 directions. DIRT_new_sparks (lots).
		case SPECIAL_GRENADE:

			{
				Thing *p_has = person_has_special(p_person, SPECIAL_GRENADE);

				if (p_has)
				{
					if(p_has->Genus.Special->ammo<100) //just to be on the safe side
						p_has->Genus.Special->ammo += SPECIAL_AMMO_IN_A_GRENADE;

					x_message = X_AMMO;
				}
				else
				{
					special_pickup(p_special, p_person);

					//
					// Make sure it isn't drawn!
					//

					remove_thing_from_map(p_special);

					//
					// Dont free up the special.
					//

					keep = TRUE;

					x_message = X_GRENADE;
				}

				break;
			}


		// claude-ai: SPECIAL_EXPLOSIVES (C4) pickup:
		// claude-ai:   Already carrying C4 → ammo += 1 (stacks charges, max 4 enforced by should_person_get_item).
		// claude-ai:   New C4 → special_pickup() + remove from map + keep=TRUE.
		// claude-ai:   ACTIVATED explosives that are picked up: SubState reset to NONE (defused automatically!).
		// claude-ai:   USE (SPECIAL_set_explosives in this file):
		// claude-ai:     ammo==1: drop in place + arm. ammo>1: alloc_special a new one + arm it.
		// claude-ai:     Timer = 1600 ticks = 5 seconds. No remote detonation in this pre-release.
		case SPECIAL_EXPLOSIVES:

			{
				Thing *p_has = person_has_special(p_person, SPECIAL_EXPLOSIVES);

				if (p_has)
				{
					p_has->Genus.Special->ammo += 1;
				}
				else
				{
					special_pickup(p_special, p_person);

					//
					// Make sure it isn't drawn!
					//

					remove_thing_from_map(p_special);

					//
					// Dont free up the special.
					//

					keep = TRUE;
				}
				if(p_special->SubState == SPECIAL_SUBSTATE_ACTIVATED)
				{
					//This is active lets make it inactive
					p_special->SubState = SPECIAL_SUBSTATE_NONE;  

				}

				x_message = X_EXPLOSIVES;

				break;
			}
			// claude-ai: SPECIAL_TREASURE — badge/statue collectible that permanently increases a player stat.
		// claude-ai: Stat determined by ObjectId: 81=Stamina, 94=Strength, 71=Skill(Reflex), 39=Constitution.
		// claude-ai: These are the "bonus" collectibles scattered in levels. stat_count_bonus tracks total found.
		// claude-ai: Pickup guard: FLAG_PERSON_DRIVING must be clear (player can't collect badges while in vehicle).
		// claude-ai: After pickup: keep=FALSE so free_special() is called — badge destroyed, not in inventory.
		// claude-ai: stat_count_bonus extern declared inline inside the case body (unusual C++ pattern, but valid).
		// claude-ai: PSX: PANEL_icon_time=30 forces the badge panel to display for 30 frames.
		case SPECIAL_TREASURE:

				{
					Thing *darci = NET_PERSON(0);

					//
					// Has a player picked us up?
					//

					if (!(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING))
					{

							switch(p_special->Draw.Mesh->ObjectId)
							{
								case	81:

									x_message = X_STA_INCREASED;

									NET_PLAYER(0)->Genus.Player->Stamina++;

									break;

								case	94:

									x_message = X_STR_INCREASED;

									NET_PLAYER(0)->Genus.Player->Strength++;

									break;

								case	71:

									x_message = X_REF_INCREASED;

									NET_PLAYER(0)->Genus.Player->Skill++;
									break;

								case	39:

									x_message = X_CON_INCREASED;
									NET_PLAYER(0)->Genus.Player->Constitution++;
									break;

								default:
									ASSERT(0);
									break;
							}
					}

#ifdef PSX
					PANEL_icon_time=30;
#endif

extern	SLONG	stat_count_bonus;
					stat_count_bonus++;
				}
				keep = FALSE;
				break;



	// claude-ai: DEFAULT case — all items NOT handled above: KNIFE, BASEBALLBAT, KEY, KEYCARD, FILE,
	// claude-ai:   FLOPPY_DISK, CROWBAR, VIDEO, GLOVES, WEEDAWAY, CARKEY_*, WIRE_CUTTER.
	// claude-ai: All are added to SpecialList via special_pickup() + remove from map + keep=TRUE.
	// claude-ai:
	// claude-ai: MELEE WEAPONS (KNIFE, BASEBALLBAT):
	// claude-ai:   Stored as Things in SpecialList. No ammo field during combat.
	// claude-ai:   Combat damage dispatched by apply_violence() → combat.cpp (fight tree), NOT here.
	// claude-ai:   Animation dispatch (Person.cpp): KNIFE → ANIM_KNIFE_ATTACK1, BAT → ANIM_BAT_HIT1.
	// claude-ai:   Shotgun/AK47 can also be used as a whip: ANIM_SHOTGUN_WHIP1/2 (gun-as-club).
	// claude-ai:   Mesh variant: Knife PersonID|=2<<5, Bat PersonID|=4<<5 (drives skinned prop selection).
	// claude-ai:   SPECIAL_BASEBALLBAT is two-handed (person_has_twohanded_weapon returns TRUE).
	// claude-ai:   SPECIAL_KNIFE is one-handed; can be drawn while holding with one hand.
	// claude-ai:   AI weapon priority (get_persons_best_weapon_with_ammo): AK47 > Shotgun > Gun > Bat > Knife.
	// claude-ai:
	// claude-ai: MISSION ITEMS (KEY, KEYCARD, VIDEO, etc.):
	// claude-ai:   Held in SpecialList; no combat use. Mission scripts check via person_has_special().
	// claude-ai:   EWAY_item_pickedup() fires below if waypoint was attached to this item.
	// claude-ai:   x_message set for KEY→X_KEYCARD, VIDEO→X_VIDEO only. Others: no HUD feedback.
	// claude-ai:   keep=TRUE means the Thing survives; free_special() is NOT called.
	default:

			if (p_special->Genus.Special->SpecialType == SPECIAL_KNIFE)       {x_message = X_KNIFE;}
			if (p_special->Genus.Special->SpecialType == SPECIAL_BASEBALLBAT) {x_message = X_BASEBALL_BAT;}
			if (p_special->Genus.Special->SpecialType == SPECIAL_KEY)         {x_message = X_KEYCARD;}
			if (p_special->Genus.Special->SpecialType == SPECIAL_VIDEO)       {x_message = X_VIDEO;}

			//
			// Use it by default? Nope.  Add it to the person's linked list of
			// items carried.
			//

			special_pickup(p_special, p_person);

			//
			// Make sure it isn't drawn!
			//

			remove_thing_from_map(p_special);

			//
			// Dont free up the special.
			//

			keep = TRUE;

			break;
	}

	// claude-ai: HUD feedback: PANEL_new_info_message only fires if x_message was set AND the picker
	// claude-ai:   is a player (PlayerID != 0 = player; 0 = AI NPC — NPCs don't get HUD messages).
	// claude-ai: Note: the add_damage_text() call above is commented out — floating text was removed.
	if (x_message && p_person->Genus.Person->PlayerID)
	{
/*
		add_damage_text(
			p_special->WorldPos.X          >> 8,
			p_special->WorldPos.Y + 0x2000 >> 8,
			p_special->WorldPos.Z          >> 8,
			XLAT_str_ptr(x_message));
*/

		PANEL_new_info_message(XLAT_str(x_message));
	}

	// claude-ai: EWAY waypoint notification: if this item was placed by a mission waypoint (e.g. "pickup
	// claude-ai:   this key to unlock the door"), EWAY_item_pickedup() marks the waypoint as fulfilled.
	// claude-ai:   This triggers any chained waypoints that depend on item collection.
	if (p_special->Genus.Special->waypoint)
	{
		//
		// Tell the waypoint system.
		//

		EWAY_item_pickedup(p_special->Genus.Special->waypoint);
	}

	if (!keep)
	{
		//
		// Don't need the special any more.
		//

		free_special(p_special);
	}
}


// claude-ai: special_activate_mine — triggers mine explosion. Creates fire/dust pyro effects and shockwave.
// claude-ai: Shockwave radius 0x200, force 250. Clears player target if targeting this mine.
// claude-ai: PS1 vs PC difference: PSX uses POW_create (simpler), PC uses PYRO_create (more detailed).
// claude-ai: Calls free_special() to destroy the mine Thing after detonation.
void special_activate_mine(Thing *p_mine)
{
	ASSERT(p_mine->Class == CLASS_SPECIAL);
	ASSERT(p_mine->Genus.Special->SpecialType == SPECIAL_MINE);

	if (NET_PERSON(0)->Genus.Person->Target == THING_NUMBER(p_mine))
	{
		//
		// Make the player choose a new target.
		//

		NET_PERSON(0)->Genus.Person->Target = NULL;
	}

	/*

	PYRO_construct(
		p_mine->WorldPos,
	   -1,
		0xa0);
	*/

#ifdef PSX

	POW_create(
		POW_CREATE_LARGE_SEMI,
		p_mine->WorldPos.X,
		p_mine->WorldPos.Y,
		p_mine->WorldPos.Z,
		0,0,0);
	PYRO_create(p_mine->WorldPos,PYRO_DUSTWAVE);

#else

	PYRO_create(p_mine->WorldPos,PYRO_FIREBOMB);
	PYRO_create(p_mine->WorldPos,PYRO_DUSTWAVE);

#endif

	MFX_play_xyz(THING_NUMBER(p_mine),SOUND_Range(S_EXPLODE_START,S_EXPLODE_END),0,p_mine->WorldPos.X,p_mine->WorldPos.Y,p_mine->WorldPos.Z);

	create_shockwave(
		p_mine->WorldPos.X >> 8,
		p_mine->WorldPos.Y >> 8,
		p_mine->WorldPos.Z >> 8,
		0x200,
		250,
		NULL);

	free_special(p_mine);
}



DIRT_Info special_di;

// claude-ai: special_normal — per-tick StateFn for all CLASS_SPECIAL Things. Called every game tick.
// claude-ai: This function handles ALL item behaviours while on the ground or carried:
// claude-ai:
// claude-ai: PROJECTILE substate (item thrown/dropped from height):
// claude-ai:   Simulates bouncing physics (velocity stored in ss->timer as UWORD offset from 0x8000).
// claude-ai:   Gravity = -256 units/tick. Bounces with 50% velocity retention. Stops when velocity < 0x100.
// claude-ai:
// claude-ai: Carried (OwnerThing set, not EXPLOSIVES):
// claude-ai:   WorldPos tracks owner's WorldPos each tick.
// claude-ai:   If GRENADE + ACTIVATED substate: countdown timer; detonate via CreateGrenadeExplosion when expired.
// claude-ai:   Grenade with ammo>1: subtract 1 ammo and reset (multi-grenade inventory).
// claude-ai:
// claude-ai: On-ground (not carried):
// claude-ai:   Items rotate (Angle += 32) unless mine or activated explosive.
// claude-ai:   BOMB: if EWAY waypoint activates, detonates via PYRO_construct.
// claude-ai:   EXPLOSIVES (ACTIVATED): countdown, then PYRO+shockwave (radius 0x500, force 500).
// claude-ai:   MINE: proximity check every other tick; activates if person/vehicle/Balrog within 0x3000/0x50/0x6000.
// claude-ai:         Checks player's bodyguard is NOT triggered by mine (bodyguards don't trigger mines).
// claude-ai:   TREASURE: proximity auto-pickup within 0xa0 map units (Manhattan distance).
// claude-ai:   Default (weapons, ammo, health, keys): pickup radius check (counter>16*20 = ~1.25 seconds cooldown).
// claude-ai:     Player within 0xa0 units -> call should_person_get_item / person_get_item.
// claude-ai:
// claude-ai: PORT: replicate the full tick logic including all countdowns and proximity checks.
void special_normal(Thing *s_thing)
{
	SLONG i;

	SLONG dx;
	SLONG dy;
	SLONG dz;

	SLONG dist;
	
	// claude-ai: Counter increment — tracks time since the item was placed/dropped.
	// claude-ai: counter is NOT incremented when the item is FLAG_SPECIAL_HIDDEN (stored inside an object).
	// claude-ai: When hidden, the counter field is repurposed: it holds the containing-object's ID (!)
	// claude-ai:   This is noted by the "ob this specail is inside" comment — a dual-use of the field.
	// claude-ai: 16 * TICK_RATIO >> TICK_SHIFT = 16 ticks per frame at normal speed.
	// claude-ai: try_pickup only fires when counter > 16*20 = 320 = ~1.0 second after drop (PCOM_TICKS_PER_SEC=320).
	if (s_thing->Flags & FLAG_SPECIAL_HIDDEN)
	{
		//
		// Counter is the ob this specail is inside!
		//
	}
	else
	{
		s_thing->Genus.Special->counter += 16 * TICK_RATIO >> TICK_SHIFT;
	}

	// claude-ai: PROJECTILE substate — item was dropped from height (e.g. dropped gun spawned mid-air).
	// claude-ai: timer UWORD encodes velocity: 0x8000=zero, values below=falling, above=rising.
	// claude-ai: Ground = PAP_calc_map_height_at + 0x30 (slightly above terrain surface).
	if (s_thing->SubState == SPECIAL_SUBSTATE_PROJECTILE)
	{
		SpecialPtr ss;
		SLONG      velocity;
		SLONG      ground;

		ss        = s_thing->Genus.Special;
		velocity  = ss->timer;
		velocity -= 0x8000;
		velocity -= 256 * TICK_RATIO >> TICK_SHIFT;

		s_thing->WorldPos.Y += velocity * TICK_RATIO >> TICK_SHIFT;
		ground               = PAP_calc_map_height_at(s_thing->WorldPos.X >> 8, s_thing->WorldPos.Z >> 8) + 0x30 << 8;

		if (s_thing->WorldPos.Y < ground)
		{
			velocity              = abs(velocity);
			velocity            >>= 1;
			s_thing->WorldPos.Y   = ground;

			if (velocity < 0x100)
			{
				s_thing->SubState             = SPECIAL_SUBSTATE_NONE;
				s_thing->Genus.Special->timer = 0;

				return;
			}
		}

		velocity += 0x8000;

		SATURATE(velocity, 0, 0xffff);

		ss->timer= velocity;

		return;
	}
	else
	{
		/*
		ASSERT(s_thing->Velocity>=0 &&s_thing->Velocity<10);
		if(s_thing->Velocity)
			s_thing->Velocity--;   // dropped things cant be picked up for a few gameturns.
		*/

	}

	// claude-ai: CARRIED branch: OwnerThing is set AND item is not EXPLOSIVES (C4 tracks separately).
	// claude-ai: SPECIAL_MINE and SPECIAL_NONE are illegal to carry — ASSERTs guard these.
	// claude-ai: EXPLOSIVES is excluded here because it stays in the world even after being "planted";
	//            its OwnerThing records who placed it (for blame attribution on shockwave), not who is
	//            holding it. When first picked up the explosive goes in the SpecialList normally, but
	//            after SPECIAL_set_explosives() drops/places it, OwnerThing is set for explosion credit.
	if (s_thing->Genus.Special->OwnerThing && s_thing->Genus.Special->SpecialType != SPECIAL_EXPLOSIVES)
	{
		Thing *p_owner = TO_THING(s_thing->Genus.Special->OwnerThing);
		ASSERT(s_thing->Genus.Special->SpecialType != SPECIAL_MINE);
		ASSERT(s_thing->Genus.Special->SpecialType != SPECIAL_NONE);

		ASSERT((s_thing->Flags&FLAGS_ON_MAPWHO)==0);


		//
		// This special is being carried by someone- it can't be on the mapwho
		//

		// claude-ai: All carried specials shadow their owner's WorldPos every tick.
		// claude-ai: This keeps the item logically "inside" the owner — no physics, no rotation.
		// claude-ai: The draw system reads SpecialDraw to place weapon in the hand bone position.
		s_thing->WorldPos = p_owner->WorldPos;

		// claude-ai: GRENADE COOK-OFF: when the player (or NPC) primes a grenade and holds it,
		// claude-ai: the ACTIVATED substate is set and the timer counts down here, while carried.
		// claude-ai: Timer starts at 1920 ticks (6 seconds). Each tick deducts 16 ticks.
		// claude-ai: On expiry: CreateGrenadeExplosion() is called AT THE OWNER'S POSITION (+2256 Y
		// claude-ai:   offset = ~0x8D0 sub-mapsquare units, roughly waist height to raise blast center).
		// claude-ai: PC vs PSX: PC uses the particle-based CreateGrenadeExplosion (grenade.cpp);
		//            PSX uses POW_create (simpler effect) + create_shockwave (radius 0x300, force 500).
		// claude-ai: After explosion: if ammo==1 (last grenade), drop + free; else decrement ammo,
		//            reset SubState to NONE so the next grenade in inventory can be primed again.
		if (s_thing->Genus.Special->SpecialType == SPECIAL_GRENADE &&
			s_thing->SubState                   == SPECIAL_SUBSTATE_ACTIVATED)
		{
			SLONG ticks = 16 * TICK_RATIO >> TICK_SHIFT;

			if (s_thing->Genus.Special->timer < ticks)
			{
				//
				// The grenade has gone off!
				//
#ifndef PSX
				CreateGrenadeExplosion(s_thing->WorldPos.X, s_thing->WorldPos.Y + 2256, s_thing->WorldPos.Z, p_owner);
#else
				POW_create(
					POW_CREATE_LARGE_SEMI,
					s_thing->WorldPos.X,
					s_thing->WorldPos.Y,
					s_thing->WorldPos.Z,0,0,0);

				create_shockwave(
					s_thing->WorldPos.X >> 8,
					s_thing->WorldPos.Y >> 8,
					s_thing->WorldPos.Z >> 8,
					0x300,
					500,
					p_owner);
#endif

				p_owner->Genus.Person->SpecialUse = NULL;

				if (s_thing->Genus.Special->ammo == 1)
				{
					special_drop(s_thing, p_owner);
					free_special(s_thing);
				}
				else
				{
					s_thing->SubState = SPECIAL_SUBSTATE_NONE;	// let's reset the grenade
					s_thing->Genus.Special->ammo -= 1;
				}
			}
			else
			{
				s_thing->Genus.Special->timer -= ticks;
			}
		}
	}
	else
	{
		//
		// This special is lying on the ground. Mostly they rotate.
		//

		if (s_thing->Genus.Special->SpecialType == SPECIAL_BOMB &&
			s_thing->SubState                   == SPECIAL_SUBSTATE_NONE)
		{
			//
			// Deactivated bombs don't rotate.
			//
		}
		else
		{
			// claude-ai: Item rotation: Angle += 32 per tick (2047 max = 360 degrees in 64 steps).
			// claude-ai: Rotation creates the spinning/bobbing visual on lying items in the world.
			// claude-ai: Mines and armed C4 (ACTIVATED) do NOT rotate — they are meant to look stationary.
			if (s_thing->Genus.Special->SpecialType!=SPECIAL_MINE) // mines no longer rotate
				if (s_thing->Genus.Special->SpecialType!=SPECIAL_EXPLOSIVES || s_thing->SubState != SPECIAL_SUBSTATE_ACTIVATED) // explosives waiting to go off no longer rotate
					s_thing->Draw.Mesh->Angle = (s_thing->Draw.Mesh->Angle+32)&2047;
		}

		// claude-ai: Per-type on-ground behaviours (most items just get auto-pickup below at try_pickup:)
		// claude-ai: WEAPON GROUND BEHAVIOUR SUMMARY:
		// claude-ai:   SPECIAL_GUN / SHOTGUN / AK47 / KNIFE / BASEBALLBAT — all fall-through to try_pickup.
		// claude-ai:   SPECIAL_AMMO_PISTOL / SHOTGUN / AK47               — fall-through to try_pickup.
		// claude-ai:   SPECIAL_GRENADE (unactivated)                       — fall-through to try_pickup.
		// claude-ai:   SPECIAL_GRENADE (ACTIVATED, on ground)              — NO pickup (break before try_pickup).
		// claude-ai:   SPECIAL_HEALTH                                      — only try_pickup if player < max HP.
		// claude-ai:   SPECIAL_BOMB                                        — waits for EWAY waypoint to go active.
		// claude-ai:   SPECIAL_EXPLOSIVES (unarmed, no owner)              — goto try_pickup.
		// claude-ai:   SPECIAL_EXPLOSIVES (ACTIVATED)                      — countdown to detonation (no pickup).
		// claude-ai:   SPECIAL_MINE                                        — proximity scan (no pickup).
		// claude-ai:   SPECIAL_THERMODROID                                 — no-op (break; just sits there).
		// claude-ai:   SPECIAL_TREASURE                                    — auto-pickup by proximity (own logic).
		// claude-ai:   All mission items (SPECIAL_KEY, KEYCARD, FILE, etc) — default fall-through to try_pickup.
		switch(s_thing->Genus.Special->SpecialType)
		{
			// claude-ai: SPECIAL_BOMB — scripted mission explosive, detonated via EWAY waypoint activation.
			// claude-ai: Unlike SPECIAL_EXPLOSIVES (C4, player-placed), the BOMB is placed by level script.
			// claude-ai: It only explodes when its linked EWAY waypoint becomes active.
			// claude-ai: SubState must be ACTIVATED (set by mission script) before the waypoint check runs.
			// claude-ai: Uses PYRO_construct(pos, -1, 256) on PC — radius 256 map units.
			// claude-ai: No shockwave call here (unlike EXPLOSIVES/GRENADE/MINE): BOMB is purely visual effect.
			// claude-ai: Deactivated BOMB (SubState==NONE) does not rotate (see rotation code above).
			// claude-ai: PORT: BOMB type is mission-critical; needed for scripted level set-pieces.
			case SPECIAL_BOMB:

				//
				// Has the waypoint that created us gone active?
				//

				if (s_thing->Genus.Special->waypoint)
				{
					if (s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED)
					{
						if (EWAY_is_active(s_thing->Genus.Special->waypoint))
						{
							//
							// Explode this bomb.
							//
#ifndef PSX
							PYRO_construct(
								s_thing->WorldPos,
							   -1,
								256);
#else					   
							POW_create(
								POW_CREATE_LARGE_SEMI,
								s_thing->WorldPos.X,
								s_thing->WorldPos.Y,
								s_thing->WorldPos.Z,0,0,0);
#endif
							//
							// Don't need the special any more.
							//

							free_special(s_thing);
						}
					}
				}

				break;

			// claude-ai: SPECIAL_EXPLOSIVES (C4 brick) — player-placed timed explosive.
			// claude-ai: Two on-ground states:
			// claude-ai:   ACTIVATED: counting down to explosion (timer set to 1600 ticks = 5 seconds).
			// claude-ai:   Unarmed (no OwnerThing): acts like a pickupable item → goto try_pickup.
			// claude-ai: Explosion parameters:
			// claude-ai:   PYRO_construct flags = 1|4|8|64 (fire + dustcloud + shockwave + sparks).
			// claude-ai:   PYRO_construct radius = 0xa0 (160 map units).
			// claude-ai:   create_shockwave: radius=0x500, maxdamage=500.
			// claude-ai:   Damage formula (from create_shockwave): damage = 500*(0x500-dist)/0x500.
			// claude-ai:   At edge of radius (0x500 = 1280 map units): 0 damage.
			// claude-ai:   At centre: 500 damage (enough to one-shot any NPC and usually kill the player).
			// claude-ai: DIRT_gust is called in 2 directions (±0xa0 on X axis) for dust splash effect.
			// claude-ai: Sound: SOUND_Range(S_EXPLODE_MEDIUM, S_EXPLODE_BIG) — random medium-to-large blast.
			// claude-ai: After explosion: free_special() destroys the Thing.
			// claude-ai: PORT: explosion radius 0x500 and force 500 are the C4 signature values — document these.
			case SPECIAL_EXPLOSIVES:

				if (s_thing->SubState == SPECIAL_SUBSTATE_ACTIVATED)
				{
					SLONG tickdown = 0x10 * TICK_RATIO >> TICK_SHIFT;


					//
					// Time to explode?
					//

					if (s_thing->Genus.Special->timer <= tickdown)
					{
						//
						// Bang!
						//

#ifndef PSX
						PYRO_construct(
							s_thing->WorldPos,
						    1|4|8|64,
							0xa0);
#else
						POW_create(
							POW_CREATE_LARGE_SEMI,
							s_thing->WorldPos.X,
							s_thing->WorldPos.Y,
							s_thing->WorldPos.Z,0,0,0);
						PYRO_construct(
							s_thing->WorldPos,PYRO_DUSTWAVE,0xa0);
#endif
						MFX_play_xyz(THING_NUMBER(s_thing),SOUND_Range(S_EXPLODE_MEDIUM,S_EXPLODE_BIG),0,s_thing->WorldPos.X,s_thing->WorldPos.Y,s_thing->WorldPos.Z);

						{
							SLONG cx,cz;
							cx=s_thing->WorldPos.X>>8;
							cz=s_thing->WorldPos.Z>>8;
							DIRT_gust(s_thing,cx,cz,cx+0xa0,cz);
							DIRT_gust(s_thing,cx+0xa0,cz,cx,cz);
						}

						create_shockwave(
							s_thing->WorldPos.X >> 8,
							s_thing->WorldPos.Y >> 8,
							s_thing->WorldPos.Z >> 8,
							0x500,
							500,
							TO_THING(s_thing->Genus.Special->OwnerThing));

						//
						// This is the end of the explosives.
						//

						free_special(s_thing);
					}
					else
					{
						s_thing->Genus.Special->timer -= tickdown;
					}
				}
				else
				{
					if(!s_thing->Genus.Special->OwnerThing )
					{
						//
						// not being carried round may as well see if anyone wants to pick me up
						//
						goto	try_pickup; //I love goto
					}
				}

				break;

			// claude-ai: SPECIAL_MINE proximity logic:
			// claude-ai:   ammo > 0: countdown timer (ammo ticks down each frame, explodes at ammo==1).
			// claude-ai:   ammo == 0: every other tick (GAME_TURN&1 == THING_NUMBER&1), scan nearby entities:
			// claude-ai:     - CLASS_BAT (Balrog type): activates if dist < 0x6000.
			// claude-ai:     - CLASS_PERSON: activates if foot height <= mine height + 0x2000>>8 AND dist < 0x3000.
			// claude-ai:     - CLASS_VEHICLE: checks all 4 wheel positions, activates if wheel dist < 0x50.
			// claude-ai:   Bodyguards of the player do NOT trigger mines.
			// claude-ai:
			// claude-ai: MINE EXPLOSION PARAMETERS (special_activate_mine()):
			// claude-ai:   PC:  PYRO_create(PYRO_FIREBOMB) + PYRO_create(PYRO_DUSTWAVE) — two effects.
			// claude-ai:   PSX: POW_create(POW_CREATE_LARGE_SEMI) + PYRO_create(PYRO_DUSTWAVE).
			// claude-ai:   Sound: SOUND_Range(S_EXPLODE_START, S_EXPLODE_END) — random explosion variant.
			// claude-ai:   create_shockwave: radius=0x200, maxdamage=250, aggressor=NULL.
			// claude-ai:   Damage at edge (0x200 = 512 units): 0. At centre: 250 HP.
			// claude-ai:   Compared to C4 (radius 0x500, force 500): mine is smaller radius, lower damage.
			// claude-ai:   Grenade shockwave: radius 0x300, force 500 — bigger radius than mine, same force as C4.
			// claude-ai:   Player target reset: if player was targeting the mine, Target cleared.
			// claude-ai:
			// claude-ai: EXPLOSION SUMMARY TABLE:
			// claude-ai:   MINE:       radius 0x200 (512),  maxdamage 250, aggressor NULL.
			// claude-ai:   GRENADE:    radius 0x300 (768),  maxdamage 500, aggressor = thrower.
			// claude-ai:   C4 (EXPLOSIVES): radius 0x500 (1280), maxdamage 500, aggressor = placer.
			// claude-ai:   BARREL:     (see barrel.cpp — varies, usually radius 0x200-0x400).
			// claude-ai:   BOMB:       no shockwave — visual/scripted only (PYRO_construct only).
			// claude-ai:   Damage formula: hitpoints = maxdamage * (radius - dist) / radius.
			// claude-ai:   Knockdown threshold: hitpoints > 30 → knock_person_down() in collide.cpp.
			// claude-ai:   Jumping/flipping at moment of explosion: hitpoints halved + 1 (reduced impact).
			case SPECIAL_MINE:

				if (s_thing->Genus.Special->ammo > 0)
				{
					//
					// This special has a counter ticking down to explode.
					//

					if (s_thing->Genus.Special->ammo == 1)
					{
						special_activate_mine(s_thing);
					}
					else
					{
						s_thing->Genus.Special->ammo -= 1;
					}
				}
				else
				// claude-ai: Staggered alternating-tick mine proximity scan: runs every other tick per mine.
			// claude-ai: Broad sphere scan radius 0x300; per-entity exact thresholds applied per class.
			if ((GAME_TURN&1) == (THING_NUMBER(s_thing)&1))
				{
					SLONG i;
					SLONG j;

					SLONG wx;
					SLONG wy;
					SLONG wz;

					SLONG num_found = THING_find_sphere(
											s_thing->WorldPos.X >> 8,
											s_thing->WorldPos.Y >> 8,
											s_thing->WorldPos.Z >> 8,
											0x300,
											THING_array,
											THING_ARRAY_SIZE,
											(1 << CLASS_PERSON) | (1 << CLASS_VEHICLE) | (1 << CLASS_BAT));

					Thing *p_found;

					for (i = 0; i < num_found; i++)
					{
						p_found = TO_THING(THING_array[i]);

						switch(p_found->Class)
						{
							case CLASS_BAT:
								
								if (p_found->Genus.Bat->type == BAT_TYPE_BALROG)
								{
									dx = abs(p_found->WorldPos.X - s_thing->WorldPos.X);
									dz = abs(p_found->WorldPos.Z - s_thing->WorldPos.Z);

									dist = QDIST2(dx,dz);

									if (dist < 0x6000)
									{
										special_activate_mine(s_thing);

										return;
									}
								}

								break;

							case CLASS_PERSON:

								{
									Thing *p_person = p_found;

									dx = abs(p_person->WorldPos.X - s_thing->WorldPos.X);
									dz = abs(p_person->WorldPos.Z - s_thing->WorldPos.Z);

									dist = QDIST2(dx,dz);

									if (dist < 0x3000)
									{
										SLONG fx;
										SLONG fy;
										SLONG fz;

										calc_sub_objects_position(
											p_person,
											p_person->Draw.Tweened->AnimTween,
											SUB_OBJECT_LEFT_FOOT,
										   &fx,
										   &fy,
										   &fz);

										fx += p_person->WorldPos.X >> 8;
										fy += p_person->WorldPos.Y >> 8;
										fz += p_person->WorldPos.Z >> 8;

										if (fy > (s_thing->WorldPos.Y + 0x2000 >> 8))
										{
											//
											// This person has jumped over the mine!
											//
										}
										else
										{
											//
											// Activate the mine.
											//

											if (p_person->Genus.Person->pcom_ai== PCOM_AI_BODYGUARD && TO_THING(EWAY_get_person(p_person->Genus.Person->pcom_ai_other))->Genus.Person->PlayerID)
											{
												//
												// claude-ai: Bodyguard mine immunity: pcom_ai_other = their ward. If ward is a player → skip.
											// players bodyguards dont trigger mines
												//
											}
											else
											{
												special_activate_mine(s_thing);

												return;
											}
										}
									}
								}

								break;

							case CLASS_VEHICLE:

								{
									SLONG wx;
									SLONG wy;
									SLONG wz;

									Thing *p_vehicle = p_found;

									//
									// claude-ai: CLASS_VEHICLE mine check: iterate all 4 wheel positions.
									// claude-ai: vehicle_wheel_pos_get() returns each wheel's position in map-square units.
									// claude-ai: Activation threshold: QDIST3 < 0x50 (80 units) = near-contact required.
									// claude-ai: Much tighter than person (0x3000) or Balrog (0x6000) — models pressure plate.
									// claude-ai: PORT: vehicle wheel position API must be implemented before this works.
									// Find out where the wheels are.
									//

									vehicle_wheel_pos_init(p_vehicle);

									for (j = 0; j < 4; j++)
									{
										vehicle_wheel_pos_get(
											j,
										   &wx,
										   &wy,
										   &wz);
										
										dx = abs(wx - (s_thing->WorldPos.X >> 8));
										dy = abs(wy - (s_thing->WorldPos.Y >> 8));
										dz = abs(wz - (s_thing->WorldPos.Z >> 8));

										dist = QDIST3(dx,dy,dz);

										if (dist < 0x50)
										{
											special_activate_mine(s_thing);

											return;
										}
									}
								}

								break;

							default:
								ASSERT(0);
								break;
						}
					}
				}
				break;

			// claude-ai: SPECIAL_THERMODROID — placeholder type; no behaviour implemented on the ground.
			// claude-ai: The THERMODROID is an NPC enemy (bat-like creature), not a true item.
			// claude-ai: PRIM_OBJ_THERMODROID is shared with SPECIAL_BOMB in SPECIAL_info[].
			// claude-ai: Appears to have been an early concept that was never completed in this pre-release.
			// claude-ai: PORT: skip — not functional in the final game either.
			case SPECIAL_THERMODROID:
				break;

			// claude-ai: SPECIAL_TREASURE — stat-boosting collectible badge (Skill/Strength/Stamina/Constitution).
			// claude-ai: UNLIKE most items, TREASURE has its OWN proximity check here (not try_pickup).
			// claude-ai: Pickup radius: Manhattan distance < 0xa0 (same as try_pickup threshold).
			// claude-ai: Does NOT use should_person_get_item() — unconditional pickup.
			// claude-ai: Directly modifies Player struct: Player->Stamina/Strength/Skill/Constitution++.
			// claude-ai: Also increments stat_count_bonus (extern SLONG, tracks total badges found for score).
			// claude-ai: Note: SPECIAL_TREASURE also has pickup code in person_get_item() (also via ObjectId).
			//            Having the logic in BOTH places is intentional — player can trigger it either way.
			// claude-ai: Plays HUD message: X_STA_INCREASED / X_STR_INCREASED / X_REF_INCREASED / X_CON_INCREASED.
			// claude-ai: Calls add_damage_text() to show floating "+Stamina" text at item position (Y+0x6000).
			// claude-ai: After pickup: free_special() (item destroyed, not kept in inventory).
			case SPECIAL_TREASURE:

				for (i = 0; i < NO_PLAYERS; i++)
				{
					Thing *darci = NET_PERSON(i);

					//
					// Has a player picked us up?
					//

					if (!(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING))
					{
						dx = darci->WorldPos.X - s_thing->WorldPos.X >> 8;
						dy = darci->WorldPos.Y - s_thing->WorldPos.Y >> 8;
						dz = darci->WorldPos.Z - s_thing->WorldPos.Z >> 8;

						dist = abs(dx) + abs(dy) + abs(dz);

						if (dist < 0xa0)
						{
							SLONG x_message;

							//
							// Near enough to pick it up.
							// 

							switch(s_thing->Draw.Mesh->ObjectId)
							{
								case	81:

									x_message = X_STA_INCREASED;

									add_damage_text(
										s_thing->WorldPos.X          >> 8,
										s_thing->WorldPos.Y + 0x6000 >> 8,
										s_thing->WorldPos.Z          >> 8,
										XLAT_str(X_STA_INCREASED));

									NET_PLAYER(0)->Genus.Player->Stamina++;

									break;

								case	94:

									x_message = X_STR_INCREASED;

									add_damage_text(
										s_thing->WorldPos.X          >> 8,
										s_thing->WorldPos.Y + 0x6000 >> 8,
										s_thing->WorldPos.Z          >> 8,
										XLAT_str(X_STR_INCREASED));

									NET_PLAYER(0)->Genus.Player->Strength++;

									break;

								case	71:

									x_message = X_REF_INCREASED;

									add_damage_text(
										s_thing->WorldPos.X          >> 8,
										s_thing->WorldPos.Y + 0x6000 >> 8,
										s_thing->WorldPos.Z          >> 8,
										XLAT_str(X_REF_INCREASED));
									NET_PLAYER(0)->Genus.Player->Skill++;
									break;

								case	39:

									x_message = X_CON_INCREASED;

									add_damage_text(
										s_thing->WorldPos.X          >> 8,
										s_thing->WorldPos.Y + 0x6000 >> 8,
										s_thing->WorldPos.Z          >> 8,
										XLAT_str(X_CON_INCREASED));
									NET_PLAYER(0)->Genus.Player->Constitution++;
									break;

								default:
									ASSERT(0);
									break;
							}

							PANEL_new_info_message(XLAT_str(x_message));
#ifdef PSX
							PANEL_icon_time=30;
#endif

	//						CONSOLE_text(XLAT_str(X_FUSE_SET));
							free_special(s_thing);

extern	SLONG	stat_count_bonus;
							stat_count_bonus++;

	//						NET_PLAYER(i)->Genus.Player->Treasure += 1;
	//						darci->Genus.Person->Health = health[darci->Genus.Person->PersonType];

	/*
							{
								CBYTE str[64];

								sprintf(str, "Badge %d", NET_PLAYER(i)->Genus.Player->Treasure);

								CONSOLE_text(str);
							}
	*/


							//
							// Reduce crime rate by 10%
							//

	//						CRIME_RATE -= 10;

	//						SATURATE(CRIME_RATE, 0, 100);
						}
					}
				}

				break;
			// claude-ai: SPECIAL_HEALTH — pickup guarded: only tries if player is more than 100 HP below max.
			// claude-ai: health[] is extern from Person.cpp; indexed by PersonType (Darci's max HP ~200).
			// claude-ai: The check is: Health > maxHealth - 100 → break (don't pick up, not worth it).
			// claude-ai: Falls through into the grenade/weapon case below otherwise.
			case SPECIAL_HEALTH:

// claude-ai: health[] extern declared inline in case body — unusual but valid C++. Defined in Person.cpp.
extern	SWORD health[];
				if(NET_PERSON(0)->Genus.Person->Health>health[NET_PERSON(0)->Genus.Person->PersonType]-100)
				{
					break;
				}

			// claude-ai: SPECIAL_GRENADE on the ground:
			// claude-ai:   If ACTIVATED (primed but thrown/dropped) → do NOT pick up (safety: live grenade).
			// claude-ai:   Otherwise → fall-through to try_pickup like a normal item.
			// claude-ai: Note: a grenade in-flight is handled by the PROJECTILE substate (bouncing physics),
			//            not by this code — the Grenade struct in grenade.cpp handles in-flight physics.
			// claude-ai: When a grenade lands (velocity < 0x100), it becomes a regular on-ground item and
			//            this case governs its behaviour (still ticking down via its SubState==ACTIVATED).
			case SPECIAL_GRENADE:

				if (s_thing->Genus.Special->SpecialType == SPECIAL_GRENADE &&
					s_thing->SubState                   == SPECIAL_SUBSTATE_ACTIVATED)
				{
					//
					// Don't pickup activated grenades.
					//

					break;
				}

			// claude-ai: All remaining weapon/ammo types — fall-through to try_pickup.
			// claude-ai: SPECIAL_GUN (pistol), SPECIAL_SHOTGUN, SPECIAL_AK47: just lie on the ground and wait.
			// claude-ai: SPECIAL_KNIFE / SPECIAL_BASEBALLBAT: melee weapons, same — passive pickup.
			// claude-ai: SPECIAL_AMMO_*: ammo packs, passive pickup.
			// claude-ai: default: catches all mission items (KEY, KEYCARD, FILE, FLOPPY_DISK, CROWBAR,
			//            VIDEO, GLOVES, WEEDAWAY, CARKEY_*, WIRE_CUTTER) — all use try_pickup.
			case SPECIAL_AK47:
			case SPECIAL_SHOTGUN:
			case SPECIAL_GUN:
			case SPECIAL_KNIFE:
			case SPECIAL_BASEBALLBAT:
			case SPECIAL_AMMO_AK47:
			case SPECIAL_AMMO_SHOTGUN:
			case SPECIAL_AMMO_PISTOL:
			default:

// claude-ai: try_pickup — the generic pickup check for most items lying on the ground.
// claude-ai: counter > 16*20 = ~1.25 second cooldown after drop before item can be grabbed.
// claude-ai: Player (darci) must be within 0xa0 Manhattan distance units (not driving) to trigger.
// claude-ai: Calls should_person_get_item() then person_get_item() for eligible pickups.
// claude-ai: Note: only checks player (NET_PERSON(0)) — AI NPC pickup is handled via PCOM pathfinding.
// claude-ai: Item must still be on the map (FLAGS_ON_MAPWHO set) — carried items bypass this entirely.
// claude-ai: Pickup distance 0xa0 = 160 map units (same as SPECIAL_TREASURE proximity check).
// claude-ai: Manhattan distance (abs(dx)+abs(dy)+abs(dz)) not Euclidean — cheaper but slightly larger.
try_pickup:;
				if (s_thing->Genus.Special->counter > 16 * 20)
				{
					if (s_thing->Flags & FLAGS_ON_MAPWHO)
					{
						Thing *darci = NET_PERSON(0);

						//
						// Has a player picked us up?
						//

						if (!(darci->Genus.Person->Flags & FLAG_PERSON_DRIVING))
						{
							dx = darci->WorldPos.X - s_thing->WorldPos.X >> 8;
							dy = darci->WorldPos.Y - s_thing->WorldPos.Y >> 8;
							dz = darci->WorldPos.Z - s_thing->WorldPos.Z >> 8;

							dist = abs(dx) + abs(dy) + abs(dz);

							if (dist < 0xa0)
							{
								if (should_person_get_item(darci, s_thing))
								{
									person_get_item(darci, s_thing);
								}
							}
						}
					}
				}

				break;
		}
	}
}


//---------------------------------------------------------------
#ifndef PSX
// claude-ai: init_specials — zero out the SPECIALS[] pool and reset count. Called at level load.
// claude-ai: SPECIALS[] is a static pool of Special structs (not Things). MAX_SPECIALS = capacity.
// claude-ai: Note: the full memset (commented out) was replaced with a targeted one for just the pool.
// claude-ai:   This avoids zeroing any trailing allocator metadata that may follow SPECIALS[].
// claude-ai: PSX version is excluded (#ifndef PSX) — PSX has its own init path.
// claude-ai: PORT: at new level init, zero the special pool and reset SPECIAL_COUNT to 0.
void	init_specials(void)
{
	//memset((UBYTE*)SPECIALS,0,sizeof(SPECIALS));

	memset((UBYTE*)SPECIALS,0,sizeof(Special) * MAX_SPECIALS);
	SPECIAL_COUNT	=	0;
}

#endif
//---------------------------------------------------------------

// claude-ai: find_empty_special — linear scan for an unused slot in SPECIALS[] pool.
// claude-ai: Returns index (1-based; 0 = NULL/unused). Returns NULL if pool is full.
// claude-ai: Pool scan starts at index 1 (0 = reserved as NULL sentinel), goes to MAX_SPECIALS-1.
// claude-ai: SpecialType == SPECIAL_NONE indicates free slot (set by free_special and init_specials).
// claude-ai: O(MAX_SPECIALS) — not a problem since pool is small (typically 64-128 items).
// claude-ai: PORT: replicate pool with 1-based indexing to match SPECIAL_NUMBER/TO_SPECIAL macros.
SLONG	find_empty_special(void)
{
	SLONG	c0;
	for(c0=1;c0<MAX_SPECIALS;c0++)
	{
		if(SPECIALS[c0].SpecialType==SPECIAL_NONE)
		{
			return(c0);
		}
	}

	return NULL;
}

// claude-ai: alloc_special — creates a new Special+Thing in the world at (world_x,world_y,world_z).
// claude-ai: world_xyz is in 8-bits-per-mapsquare units; stored as <<8 (fixed-point 16.16).
// claude-ai: Allocates from SPECIALS[] pool + alloc_draw_mesh() + alloc_thing(CLASS_SPECIAL).
// claude-ai: If all three allocations fail OR pool is full, tries to HIJACK an existing unimportant
// claude-ai:   on-map special (lowest priority: knife/bat, then ammo, then guns/grenades/health).
// claude-ai:   Hijack preference: far from player (distance > 12 blocks), priority-scored by type.
// claude-ai: Sets up StateFn = special_normal, assigns ammo defaults per weapon type.
// claude-ai: If spawned above terrain (world_y > height + 0x50), auto-sets PROJECTILE substate to drop.
// claude-ai: TREASURE type gets random ObjectId from {71,94,81,39} (Skill/Strength/Stamina/Constitution badge).
// claude-ai: PORT: replicate pool + hijack logic carefully — it prevents crashes when item pool is full.
Thing *alloc_special(
		UBYTE type,
		UBYTE substate,
		SLONG world_x,
		SLONG world_y,
		SLONG world_z,
		UWORD waypoint)
{
	SLONG	  c0;
	DrawMesh *dm;
	Special	 *new_special;
	Thing    *special_thing	= NULL;
	SLONG     special_index;

	ASSERT(WITHIN(type, 1, SPECIAL_NUM_TYPES - 1));

	// Run through the special array & find an unused one.

	special_index = find_empty_special();
	dm            = alloc_draw_mesh();
	special_thing = alloc_thing(CLASS_SPECIAL);

	if (!special_index || !dm || !special_thing)
	{
		//
		// Oh dear! Dealloc anythings we've got.
		//

		if (special_index)
		{
			TO_SPECIAL(special_index)->SpecialType = SPECIAL_NONE;
		}

		if (dm)
		{
			free_draw_mesh(dm);
		}

		if (special_thing)
		{
			free_thing(special_thing);
		}

		//
		// Find another special and hijack it!
		//


		SLONG  score;
		SLONG  best_score = -1;
		Thing *best_thing =  NULL;

		SLONG list = thing_class_head[CLASS_SPECIAL];

		while(list)
		{
			Thing *p_hijack = TO_THING(list);

			ASSERT(p_hijack->Class == CLASS_SPECIAL);

			list = p_hijack->NextLink;

			//
			// Good special to hijack?
			//

			score = -1;

			if (!(p_hijack->Flags & FLAGS_ON_MAPWHO))
			{
				continue;
			}

			if (p_hijack->Genus.Special->OwnerThing)
			{
				continue;
			}

			switch(p_hijack->Genus.Special->SpecialType)
			{
				case SPECIAL_KNIFE:
				case SPECIAL_BASEBALLBAT:
					score += 4000;
					break;

				case SPECIAL_AMMO_PISTOL:
				case SPECIAL_AMMO_SHOTGUN:
				case SPECIAL_AMMO_AK47:
					score += 3000;
					break;

				case SPECIAL_GUN:
				case SPECIAL_GRENADE:
				case SPECIAL_HEALTH:
				case SPECIAL_SHOTGUN:
				case SPECIAL_AK47:
					score += 2000;
					break;

				default:

					//
					// Never hijack one of these...
					//

					continue;
			}

			SLONG dx = abs(p_hijack->WorldPos.X - NET_PERSON(0)->WorldPos.X) >> 16;
			SLONG dz = abs(p_hijack->WorldPos.Z - NET_PERSON(0)->WorldPos.Z) >> 16;

			if (dx + dz > 12)
			{
				score += dx;
				score += dz;

				if (score > best_score)
				{
					best_score = score;
					best_thing = p_hijack;
				}
			}
		}

		// claude-ai: HIJACK PRIORITY SCORING (when pool is full):
		// claude-ai:   Score = type_base + distance_from_player.
		// claude-ai:   Type base:  Knife/Bat = 4000 (lowest gameplay value, safe to recycle).
		// claude-ai:               Ammo packs = 3000 (less important than weapons).
		// claude-ai:               Gun/Grenade/Health/Shotgun/AK47 = 2000 (only if nothing else).
		// claude-ai:   Distance bonus: dx + dz (map-square units) — prefers items far from player.
		// claude-ai:   Minimum distance: must be > 12 map squares from player to be eligible.
		// claude-ai:   Preference: highest total score wins. Picked up / carried items: never hijacked.
		// claude-ai:   Items with OwnerThing (carried): excluded (skip if OwnerThing set).
		// claude-ai:   Items NOT on map (FLAGS_ON_MAPWHO clear): excluded.
		// claude-ai:   Mission-critical types (BOMB, MINE, THERMODROID, EXPLOSIVE, TREASURE, KEY*): excluded.
		// claude-ai:   If no eligible item found: returns NULL (caller must handle gracefully).
		// claude-ai: PORT: this is critical for pool overflow safety — must be replicated.
		if (best_thing)
		{
			remove_thing_from_map(best_thing);

			special_index = SPECIAL_NUMBER(best_thing->Genus.Special);
			special_thing = best_thing;
			dm            = special_thing->Draw.Mesh;
		}
		else
		{
			return NULL;
		}
	}

	// claude-ai: alloc_special post-allocation setup — wire together the Special pool slot and the Thing.
	// claude-ai: new_special->Thing and special_thing->Genus.Special form a two-way link.
	// claude-ai: StateFn = special_normal — all items share the same per-tick state function.
	// claude-ai: substate passed in by caller: usually SPECIAL_SUBSTATE_NONE; ACTIVATED for armed explosives.
	// claude-ai: counter starts at 0 → item can NOT be picked up for the first 16*20 = 320 ticks (~1 second).
	new_special					= TO_SPECIAL(special_index);
	new_special->SpecialType	= type;
	new_special->Thing			= THING_NUMBER(special_thing);
	new_special->waypoint       = waypoint;
	new_special->counter        = 0;
	new_special->OwnerThing     = NULL;

	special_thing->Genus.Special = new_special;
	special_thing->State		 = STATE_NORMAL;
	special_thing->SubState      = substate;
	special_thing->StateFn		 = special_normal;

	// Create the visible object.

	// claude-ai: DrawMesh setup: Angle/Tilt/Roll all 0. Items rotate each tick in special_normal via Angle+=32.
	// claude-ai: DT_MESH draw type — rendered as a static mesh, no skinned animation.
	// claude-ai: ObjectId is set from SPECIAL_info[type].prim (mesh index in the Primlist/object table).
	// claude-ai: PRIM_OBJ_ITEM_TREASURE gets a random ObjectId from {71,94,81,39} to select badge variant.
	special_thing->Draw.Mesh	=	dm;
	special_thing->DrawType		=	DT_MESH;
	dm->Angle					=	0;
	dm->Tilt					=	0;
	dm->Roll					=	0;

	// claude-ai: Default ammo values for newly spawned weapons.
	// claude-ai: SPECIAL_AMMO_IN_A_PISTOL/SHOTGUN/AK47/GRENADE are defined in Special.h.
	// claude-ai: EXPLOSIVES start with ammo=1 (one charge per placed C4).
	switch(type)
	{
		case SPECIAL_GUN:        new_special->ammo = SPECIAL_AMMO_IN_A_PISTOL;  break;
		case SPECIAL_SHOTGUN:    new_special->ammo = SPECIAL_AMMO_IN_A_SHOTGUN; break;
		case SPECIAL_AK47:       new_special->ammo = SPECIAL_AMMO_IN_A_AK47;    break;
		case SPECIAL_GRENADE:    new_special->ammo = SPECIAL_AMMO_IN_A_GRENADE; break;
		case SPECIAL_EXPLOSIVES: new_special->ammo = 1;                         break;

		default:
			new_special->ammo = 0;
			break;
	}

	dm->ObjectId = SPECIAL_info[type].prim;

	if(dm->ObjectId==PRIM_OBJ_ITEM_TREASURE)
	{
		switch(Random()&3)
		{
			case	0:
				dm->ObjectId=71;
				break;
			case	1:
				dm->ObjectId=94;
				break;
			case	2:
				dm->ObjectId=81;
				break;
			case	3:
				dm->ObjectId=39;
				break;

		}

	}

	// claude-ai: World position stored as fixed-point 8.8 (world_x in mapsquares, <<8 shifts to sub-unit precision).
	// claude-ai: add_thing_to_map() registers the item in the spatial grid (mapwho) for proximity queries.
	// claude-ai: If item spawns above terrain (world_y > ground height + 0x50), auto-enable drop physics:
	// claude-ai:   SubState = SPECIAL_SUBSTATE_PROJECTILE; timer = 0x8000 (encodes velocity = 0 in UWORD bias).
	// claude-ai:   The PROJECTILE physics in special_normal() then takes over for falling/bouncing.
	// claude-ai:   This handles dropped guns, thrown grenades that start mid-air, etc.
	special_thing->WorldPos.X = world_x << 8;
	special_thing->WorldPos.Y = world_y << 8;
	special_thing->WorldPos.Z = world_z << 8;

	add_thing_to_map(special_thing);

	if (world_y > PAP_calc_map_height_at(world_x, world_z) + 0x50)
	{
		//
		// Make this item start dropping to the ground...
		//

		special_thing->SubState             = SPECIAL_SUBSTATE_PROJECTILE;
		special_thing->Genus.Special->timer = 0x8000;	// 0x8000 => 0! (Its a UWORD)
	}

	return	special_thing;

}

//---------------------------------------------------------------

// claude-ai: free_special — destroys a Special Thing: marks pool slot as NONE, frees mesh, removes from map.
// claude-ai: Does NOT call special_drop() first — caller must handle inventory removal if item was carried.
// claude-ai: Sequence: SpecialType = NONE (frees pool slot), free_draw_mesh, remove_thing_from_map, free_thing.
// claude-ai: If caller forgets to call special_drop() first, the person's SpecialList will have a dangling ref.
// claude-ai: PORT: replicate this exact destruction order; all four steps are necessary.
void	free_special(Thing *special_thing)
{
	// Set the special type to none & free the thing.

	special_thing->Genus.Special->SpecialType	=	SPECIAL_NONE;
	free_draw_mesh(special_thing->Draw.Mesh);
	remove_thing_from_map(special_thing);
	free_thing(special_thing);
}


//---------------------------------------------------------------


// claude-ai: person_has_special — walks person's SpecialList linked list looking for given type.
// claude-ai: Returns pointer to the Thing if found, NULL if not. Use for inventory checks.
// claude-ai: SpecialList is a singly-linked list via Special->NextSpecial (UWORD thing index, 0 = end).
// claude-ai: Called by: should_person_get_item, person_get_item, shoot_get_ammo_sound_anim_time, AI code.
// claude-ai: Only returns FIRST matching item — only one of each type allowed (enforced by should_person_get_item).
// claude-ai: Exception: SPECIAL_EXPLOSIVES can stack up to 4 charges in the ammo field of ONE Thing.
// claude-ai: PORT: straightforward linked list walk; SpecialList starts at Person->SpecialList (UWORD index).
Thing *person_has_special(Thing *p_person, SLONG special_type)
{
	SLONG  special;
	Thing *p_special;

	for (special = p_person->Genus.Person->SpecialList; special; special = p_special->Genus.Special->NextSpecial)
	{
		p_special = TO_THING(special);

		if (p_special->Genus.Special->SpecialType == special_type)
		{
			return p_special;
		}
	}

	return NULL;
}


// claude-ai: SPECIAL_throw_grenade — converts a carried grenade into a physical projectile.
// claude-ai: Calls CreateGrenadeFromPerson() which handles the grenade physics Thing in grenade.cpp.
// claude-ai: p_special->Genus.Special->timer = cook time (how long button was held = remaining fuse on landing).
// claude-ai: CreateGrenadeFromPerson() returns FALSE if grenade array is full (no room) — silently aborts.
// claude-ai: Ammo management after throw:
// claude-ai:   ammo == 1 (last grenade): special_drop() + free_special() + clear SpecialUse pointer.
// claude-ai:   ammo > 1: decrement ammo, reset SubState to NONE (ready for next throw).
// claude-ai: SpecialUse is the pointer in Person that tracks the currently-active special in use.
// claude-ai: PORT: grenade throwing requires the grenade physics subsystem (grenade.cpp) to be ported.
void SPECIAL_throw_grenade(Thing *p_special)
{
	Thing *p_person = TO_THING(p_special->Genus.Special->OwnerThing);

	//
	// Convert the grenade to some dirt.
	//

	if (!CreateGrenadeFromPerson(p_person, p_special->Genus.Special->timer))
	{
		// no room in grenade array
		return;
	}

	if (p_special->Genus.Special->ammo == 1)
	{
		//
		// Get rid of our special.
		//

		special_drop(p_special, p_person);
		free_special(p_special);

		p_person->Genus.Person->SpecialUse = NULL;
	}
	else
	{
		p_special->Genus.Special->ammo -= 1;
		p_special->SubState             = SPECIAL_SUBSTATE_NONE;
	}
}


// claude-ai: SPECIAL_prime_grenade — arms the grenade (sets ACTIVATED substate and 6-second timer).
// claude-ai: Timer = 16*20*6 = 1920 ticks (at 16 ticks/frame * 20 frames/sec * 6 sec = 6 seconds).
// claude-ai: The timer counts down in special_normal() when SubState==ACTIVATED and item is carried.
void SPECIAL_prime_grenade(Thing *p_special)
{
	p_special->SubState             = SPECIAL_SUBSTATE_ACTIVATED;
	p_special->Genus.Special->timer = 16 * 20 * 6;			// 6 second so self destruct.
}

// claude-ai: SPECIAL_throw_mine — DISABLED (commented out entirely in this pre-release).
// claude-ai: Originally: would have created a DIRT physics object to handle the mine's arc through the air.
// claude-ai: SubState IS_DIRT + waypoint = dirt index was the design: dirt physics Thing drives position.
// claude-ai: After landing the dirt would have placed the mine and started the proximity trigger.
// claude-ai: In the final game mines are pre-placed by level scripts, never thrown by the player.
// claude-ai: should_person_get_item returns FALSE for MINE; person_get_item has ASSERT(0) for MINE.
// claude-ai: special_pickup has ASSERT for MINE. Mine can only be placed via alloc_special().
// claude-ai: PORT: skip SPECIAL_throw_mine — mine throwing was cut. Mines are map-placed only.

/*

void SPECIAL_throw_mine(Thing *p_special)
{
	UWORD dirt;
	UWORD owner;

	ASSERT(p_special->Genus.Special->SpecialType == SPECIAL_MINE);
	ASSERT(p_special->SubState                   == SPECIAL_SUBSTATE_ACTIVATED);
	ASSERT(p_special->Genus.Special->OwnerThing);

	//
	// Remember the owner...
	//

	owner = p_special->Genus.Special->OwnerThing;

	//
	// Create some dirt that is going to process the physics of the mine 
	// being thrown through the air.
	//

	dirt = DIRT_create_mine(TO_THING(p_special->Genus.Special->OwnerThing));

	//
	// Take it out of the person's special list.
	//

	special_drop(p_special, TO_THING(p_special->Genus.Special->OwnerThing));

	//
	// Link the special to the dirt that is going to process its physics.
	//

	p_special->SubState                  = SPECIAL_SUBSTATE_IS_DIRT;
	p_special->Genus.Special->waypoint   = dirt;
	p_special->Genus.Special->OwnerThing = owner;	// So we know who to blame for the explosion.
}

*/

// claude-ai: SPECIAL_set_explosives — plants C4 explosive at person's current position and arms it.
// claude-ai: If person has only 1 explosive: drops it in place and arms it (5-second timer: 16*20*5=1600 ticks).
// claude-ai: If person has >1 explosive: decrement ammo by 1, alloc_special() for a new one at person's pos.
// claude-ai: Timer = 16*20*5 = 1600 ticks = 5 seconds. OwnerThing set so shockwave knows who placed it.
// claude-ai: Shows "FUSE_SET" HUD message. The explosion is handled in special_normal() EXPLOSIVES path.
// claude-ai: Note: the add_damage_text call for floating "Fuse set..." text is commented out — only HUD message.
// claude-ai: Note: timer comment says "10 seconds" but the actual value 1600 = 5 seconds (comment is wrong).
// claude-ai: After placing, the item is on the map with ACTIVATED substate and OwnerThing pointing to placer.
// claude-ai: If pool is full (alloc_special returns NULL) when placing last-minus-one charge: bomb not placed.
// claude-ai: PORT: must match 5-second fuse exactly for gameplay parity.
void SPECIAL_set_explosives(Thing *p_person)
{
	Thing *p_special;

	p_special = person_has_special(p_person, SPECIAL_EXPLOSIVES);

	ASSERT(p_special->Genus.Special->SpecialType == SPECIAL_EXPLOSIVES);

	if (p_special)
	{
		if (p_special->Genus.Special->ammo == 1)
		{
			//
			// Drop the special.
			//

			p_special->WorldPos = p_person->WorldPos;

			special_drop(p_special, p_person);

			if (p_person->Genus.Person->SpecialUse == THING_NUMBER(p_special))
			{
				p_person->Genus.Person->SpecialUse = NULL;
			}
		}
		else
		{
			p_special->Genus.Special->ammo -= 1;

			//
			// Create a new explosives special...
			//
			
			p_special = alloc_special(
							SPECIAL_EXPLOSIVES,
							SPECIAL_SUBSTATE_NONE,
							p_person->WorldPos.X >> 8,
							p_person->WorldPos.Y >> 8,
							p_person->WorldPos.Z >> 8,
							NULL);
		}

		//
		// Prime it!
		//

		p_special->SubState                  = SPECIAL_SUBSTATE_ACTIVATED;
		p_special->Genus.Special->timer      = 16 * 20 * 5;				// 10 seconds so self destruct.
		p_special->Genus.Special->OwnerThing = THING_NUMBER(p_person);	// So we know who is responsible for the explosion.

		//CONSOLE_text("Five second fuse set...");
/*
		add_damage_text(
			p_special->WorldPos.X          >> 8,
			p_special->WorldPos.Y + 0x2000 >> 8,
			p_special->WorldPos.Z          >> 8,
			XLAT_str(X_FUSE_SET));
*/

		PANEL_new_info_message(XLAT_str(X_FUSE_SET));
	}
}

