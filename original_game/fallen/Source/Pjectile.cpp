// claude-ai: PROJECTILE POOL — minimal allocator: init/alloc/free for CLASS_PROJECTILE Things
// claude-ai: PROJECTILES[MAX_PROJECTILES] pool; alloc_projectile(type) → alloc_thing + link
// claude-ai: Grenades have separate system (grenade.cpp); this is generic pool wrapper
// Pjectile.cpp
// Guy Simmons, 4th January 1998

#include	"Game.h"
#include	"statedef.h"


//---------------------------------------------------------------

void	init_projectiles(void)
{
	memset((UBYTE*)PROJECTILES,0,sizeof(PROJECTILES));
	PROJECTILE_COUNT	=	0;
}

//---------------------------------------------------------------

Thing	*alloc_projectile(UBYTE type)
{
	SLONG			c0;
	Projectile		*new_proj;
	Thing			*proj_thing	=	NULL;


	// Run through the projectile array & find an unused one.
	// This is bloody inefficient for projectiles, I'll do a Used/Unused
	// at some point - Guy.
	for(c0=0;c0<MAX_PROJECTILES;c0++)
	{
		if(PROJECTILES[c0].ProjectileType==PROJECTILE_NONE)
		{
			proj_thing	=	alloc_thing(CLASS_PROJECTILE);
			if(proj_thing)
			{
				new_proj					=	TO_PROJECTILE(c0);
				new_proj->ProjectileType	=	type;
				new_proj->Thing				=	THING_NUMBER(proj_thing);

				proj_thing->Genus.Projectile=	new_proj;

				set_state_function(proj_thing,STATE_INIT);
			}
			break;
		}
	}
	return	proj_thing;
}

//---------------------------------------------------------------

void	free_projectile(Thing *proj_thing)
{
	// Set the projectile type to none & free the thing.
	proj_thing->Genus.Projectile->ProjectileType	=	PROJECTILE_NONE;
	free_thing(proj_thing);
}

//---------------------------------------------------------------
