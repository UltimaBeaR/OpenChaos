#include "actors/items/special_globals.h"
#include "world/environment/prim_types.h" // PRIM_OBJ_ITEM_* constants
// Temporary: dirt.h needed for DIRT_Info type used in dead global special_di
#include "fallen/Headers/dirt.h"

// uc_orig: SPECIAL_info (fallen/Source/Special.cpp)
SPECIAL_Info SPECIAL_info[SPECIAL_NUM_TYPES] = {
    { "None",         0,                        0                            },
    { "Key",          PRIM_OBJ_ITEM_KEY,        SPECIAL_GROUP_USEFUL         },
    { "Gun",          PRIM_OBJ_ITEM_GUN,        SPECIAL_GROUP_ONEHANDED_WEAPON },
    { "Health",       PRIM_OBJ_ITEM_HEALTH,     SPECIAL_GROUP_COOKIE         },
    { "Bomb",         PRIM_OBJ_THERMODROID,     SPECIAL_GROUP_STRANGE        },
    { "Shotgun",      PRIM_OBJ_ITEM_SHOTGUN,    SPECIAL_GROUP_TWOHANDED_WEAPON },
    { "Knife",        PRIM_OBJ_ITEM_KNIFE,      SPECIAL_GROUP_ONEHANDED_WEAPON },
    { "Explosives",   PRIM_OBJ_ITEM_EXPLOSIVES, SPECIAL_GROUP_USEFUL         },
    { "Grenade",      PRIM_OBJ_ITEM_GRENADE,    SPECIAL_GROUP_ONEHANDED_WEAPON },
    { "Ak47",         PRIM_OBJ_ITEM_AK47,       SPECIAL_GROUP_TWOHANDED_WEAPON },
    { "Mine",         PRIM_OBJ_MINE,            SPECIAL_GROUP_STRANGE        },
    { "Thermodroid",  PRIM_OBJ_THERMODROID,     SPECIAL_GROUP_STRANGE        },
    { "Baseballbat",  PRIM_OBJ_ITEM_BASEBALLBAT, SPECIAL_GROUP_TWOHANDED_WEAPON },
    { "Ammo pistol",  PRIM_OBJ_ITEM_AMMO_PISTOL, SPECIAL_GROUP_AMMO          },
    { "Ammo shotgun", PRIM_OBJ_ITEM_AMMO_SHOTGUN, SPECIAL_GROUP_AMMO         },
    { "Ammo AK47",    PRIM_OBJ_ITEM_AMMO_AK47,  SPECIAL_GROUP_AMMO           },
    { "Keycard",      PRIM_OBJ_ITEM_KEYCARD,    SPECIAL_GROUP_USEFUL         },
    { "File",         PRIM_OBJ_ITEM_FILE,       SPECIAL_GROUP_USEFUL         },
    { "Floppy_disk",  PRIM_OBJ_ITEM_FLOPPY_DISK, SPECIAL_GROUP_USEFUL        },
    { "Crowbar",      PRIM_OBJ_ITEM_CROWBAR,    SPECIAL_GROUP_USEFUL         },
    { "Video",        PRIM_OBJ_ITEM_VIDEO,      SPECIAL_GROUP_USEFUL         },
    { "Gloves",       PRIM_OBJ_ITEM_GLOVES,     SPECIAL_GROUP_USEFUL         },
    { "WeedAway",     PRIM_OBJ_ITEM_WEEDKILLER, SPECIAL_GROUP_USEFUL         },
    { "Badge",        PRIM_OBJ_ITEM_TREASURE,   SPECIAL_GROUP_COOKIE         },
    { "Red Car Keys", PRIM_OBJ_ITEM_KEY,        SPECIAL_GROUP_USEFUL         },
    { "Blue Car Keys",PRIM_OBJ_ITEM_KEY,        SPECIAL_GROUP_USEFUL         },
    { "Green Car Keys",PRIM_OBJ_ITEM_KEY,       SPECIAL_GROUP_USEFUL         },
    { "Black Car Keys",PRIM_OBJ_ITEM_KEY,       SPECIAL_GROUP_USEFUL         },
    { "White Car Keys",PRIM_OBJ_ITEM_KEY,       SPECIAL_GROUP_USEFUL         },
    { "Wire Cutters", PRIM_OBJ_ITEM_WRENCH,     SPECIAL_GROUP_USEFUL         },
};

// Dead file-scope global from the original; declared but never referenced.
// uc_orig: special_di (fallen/Source/Special.cpp)
DIRT_Info special_di;
