#include "engine/platform/uc_common.h"
#include "game/game_types.h"
#include "things/core/switch.h"

// uc_orig: init_switches (fallen/Source/Switch.cpp)
void init_switches(void)
{
    memset((UBYTE*)SWITCHES, 0, sizeof(SWITCHES));
    SWITCH_COUNT = 0;
}

// uc_orig: free_switch (fallen/Source/Switch.cpp)
void free_switch(Thing* switch_thing)
{
    switch_thing->Genus.Switch->SwitchType = SWITCH_NONE;
    remove_thing_from_map(switch_thing);
    free_thing(switch_thing);
}

// uc_orig: create_switch (fallen/Source/Switch.cpp)
// Stub — creation from level definitions is commented out in pre-release.
THING_INDEX create_switch(void)
{
    return 0;
}

// uc_orig: process_switch_sphere (fallen/Source/Switch.cpp)
// Tests if the scanee coordinate is within the switch's radius and sets the triggered flag.
static void process_switch_sphere(Thing* s_thing, GameCoord* scanee_coord)
{
    SLONG distance;
    Switch* the_switch;

    the_switch = s_thing->Genus.Switch;
    distance = SDIST3(
        (scanee_coord->X - s_thing->WorldPos.X) >> 8,
        (scanee_coord->Y - s_thing->WorldPos.Y) >> 8,
        (scanee_coord->Z - s_thing->WorldPos.Z) >> 8);
    if (distance <= the_switch->Radius) {
        the_switch->Flags |= SWITCH_FLAGS_TRIGGERED;
    } else if (the_switch->Flags & SWITCH_FLAGS_RESET) {
        the_switch->Flags &= ~SWITCH_FLAGS_TRIGGERED;
    }
}

// uc_orig: fn_switch_player (fallen/Source/Switch.cpp)
// Triggers when player 0's person is within radius.
void fn_switch_player(Thing* s_thing)
{
    process_switch_sphere(s_thing, &NET_PERSON(0)->WorldPos);
}

// uc_orig: fn_switch_thing (fallen/Source/Switch.cpp)
// Triggers when the tracked thing (by index Scanee) is within radius.
void fn_switch_thing(Thing* s_thing)
{
    process_switch_sphere(s_thing, &TO_THING(s_thing->Genus.Switch->Scanee)->WorldPos);
}

// uc_orig: fn_switch_group (fallen/Source/Switch.cpp)
// Stub — group scanning not implemented in pre-release.
void fn_switch_group(Thing* s_thing)
{
}

// uc_orig: fn_switch_class (fallen/Source/Switch.cpp)
// Stub — class scanning is commented out in pre-release.
void fn_switch_class(Thing* s_thing)
{
}
