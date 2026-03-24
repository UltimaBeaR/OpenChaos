#include "engine/platform/platform.h"
#include "missions/game_types.h"
#include "actors/characters/person.h"  // create_person
#include "actors/core/player.h"
#include "actors/core/player_globals.h"
#include "actors/core/statedef.h"
#include "actors/vehicles/furn.h"
#include "ui/menus/cnet.h"
#include "ui/menus/cnet_globals.h"
#include "ui/interfac.h"
#include "ui/interfac_globals.h"

// uc_orig: set_up_camera (fallen/Source/Player.cpp)
extern void set_up_camera(Thing* t_camera, GameCoord* start_pos, Thing* track_thing);

// uc_orig: init_players (fallen/Source/Player.cpp)
void init_players(void)
{
    memset((UBYTE*)PLAYERS, 0, sizeof(Player) * MAX_PLAYERS);
    PLAYER_COUNT = 0;
}

// uc_orig: alloc_player (fallen/Source/Player.cpp)
// Allocates a free Player slot and a corresponding Thing. Returns the player Thing.
Thing* alloc_player(UBYTE type)
{
    SLONG c0;
    Player* new_player;
    Thing* player_thing = NULL;

    for (c0 = 0; c0 < MAX_PLAYERS; c0++) {
        if (PLAYERS[c0].PlayerType == PLAYER_NONE) {
            player_thing = alloc_thing(CLASS_PLAYER);
            if (player_thing) {
                new_player = TO_PLAYER(c0);
                if (new_player) {
                    player_thing->Genus.Player = new_player;
                    player_thing->StateFn = process_hardware_level_input_for_player;

                    new_player->PlayerType = type;
                    new_player->Thing = THING_NUMBER(player_thing);
                } else {
                    free_thing(player_thing);
                    player_thing = NULL;
                }
            }
            break;
        }
    }
    return player_thing;
}

// uc_orig: free_player (fallen/Source/Player.cpp)
void free_player(Thing* player_thing)
{
    player_thing->Genus.Player->PlayerType = PLAYER_NONE;
    remove_thing_from_map(player_thing);
    free_thing(player_thing);
}

// uc_orig: create_player (fallen/Source/Player.cpp)
// Creates a player of the given type at the given map position.
// NOTE: Returns the person Thing, not the player Thing — required by the calling code in
// elev.cpp / eway.cpp which expects a person Thing index.
Thing* create_player(UBYTE type, MAPCO16 x, MAPCO16 y, MAPCO16 z, SLONG player_id)
{
    UBYTE person_type;
    SLONG person_index;
    Thing *person_thing = NULL,
          *player_thing = NULL;

    player_thing = alloc_player(type);
    if (player_thing) {
        switch (type) {
        case PLAYER_NONE:
            return NULL;
        case PLAYER_DARCI:
            person_type = PERSON_DARCI;
            break;
        case PLAYER_ROPER:
            person_type = PERSON_ROPER;
            break;
        case PLAYER_COP:
            person_type = PERSON_COP;
            break;
        case PLAYER_THUG:
            person_type = PERSON_THUG_RASTA;
            break;
        }

        person_index = create_person(
            person_type,
            0,
            x << 8,
            y << 8,
            z << 8);

        if (person_index) {
            person_thing = TO_THING(person_index);

            NET_PLAYER(player_id) = player_thing;

            player_thing->Genus.Player->PlayerPerson = person_thing;
            player_thing->Genus.Player->PlayerID = player_id;

            GAME_SCORE(player_id) = 0;

            person_thing->Genus.Person->PlayerID = player_id + 1;
        } else {
            free_player(player_thing);
            player_thing = NULL;
        }
    }

    return person_thing;
}

// uc_orig: store_player_pos (fallen/Source/Player.cpp)
// Stub — player position capture was commented out in pre-release.
void store_player_pos(void)
{
}

// uc_orig: PLAYER_redmark (fallen/Source/Player.cpp)
void PLAYER_redmark(SLONG playerid, SLONG dredmarks)
{
    SLONG redmarks;
    Thing* p_player;

    if (NO_PLAYERS != 1) {
        return;
    }

    p_player = NET_PLAYER(playerid);

    ASSERT(playerid == 0);
    ASSERT(p_player);
    ASSERT(p_player->Class == CLASS_PLAYER);

    redmarks = p_player->Genus.Player->RedMarks;
    redmarks += dredmarks;

    SATURATE(redmarks, 0, 10);

    p_player->Genus.Player->RedMarks = redmarks;

    if (redmarks == 10) {
        GAME_STATE = GS_LEVEL_LOST;
    }
}
