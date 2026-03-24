#include "ui/frontend/attract_globals.h"

// uc_orig: demo_text (fallen/Source/Attract.cpp)
CBYTE demo_text[] = "Urban Chaos utilises a ground breaking graphics engine which includes 3D volumetric\n"
                    "fog, true wall hugging shadows, atomic matter simulation for real-time physical\n"
                    "modelling of dynamic object collisions and so provides the perfect environment \n"
                    "for incredible feats of acrobatic skill and total scenery interaction. \n\n"
                    "The plot of Urban Chaos is still a closely guarded secret but revolves around end\n"
                    "of the millennium chaos as predictions for Armageddon become more than mere \n"
                    "fantasy.\n\n"
                    "Urban Chaos is being developed by Mucky Foot one of the UK's hottest new \n"
                    "development teams and will be published world-wide by Eidos Interactive.";

// uc_orig: playbacks (fallen/Source/Attract.cpp)
CBYTE* playbacks[] = {
    "Data\\Game.pkt",
    "Data\\Game2.pkt",
    "Data\\Game3.pkt"
};

// uc_orig: current_playback (fallen/Source/Attract.cpp)
SLONG current_playback = 0;

// uc_orig: go_into_game (fallen/Source/Attract.cpp)
SLONG go_into_game = 0;

// uc_orig: auto_advance (fallen/Source/Attract.cpp)
UBYTE auto_advance = 0;
