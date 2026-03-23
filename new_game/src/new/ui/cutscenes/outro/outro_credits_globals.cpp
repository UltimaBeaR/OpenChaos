#include "ui/cutscenes/outro/outro_credits_globals.h"

// uc_orig: CREDITS_current_section (fallen/outro/credits.cpp)
SLONG CREDITS_current_section = 0;

// uc_orig: CREDITS_current_y (fallen/outro/credits.cpp)
float CREDITS_current_y = 0.0F;

// uc_orig: CREDITS_current_end_y (fallen/outro/credits.cpp)
float CREDITS_current_end_y = 0.0F;

// uc_orig: CREDITS_last (fallen/outro/credits.cpp)
SLONG CREDITS_last = 0;

// uc_orig: CREDITS_now (fallen/outro/credits.cpp)
SLONG CREDITS_now = 0;

// uc_orig: CREDITS_muckyfoot (fallen/outro/credits.cpp)
// Lines terminated by NULL (blank line) or "!" (end of section).
CBYTE* CREDITS_muckyfoot[] = {
    "Mucky Foot are Ashley Hampton, Barry Meade, Chris Knott, Eddie",
    "Edwards, Fin McGechie, Gary Carr, Guy Simmons, James 'Dudley'",
    "Watson, Jan Svarovsky, John Steels, Junior Walker, Justin Amore,",
    "Mark Smart, Mark Adami, Martin Oliver, Matthew Rosenfeld, Mike",
    "Burnham, Mike Diskett, Ollie Shaw, Richard Franke, Simon 'Grimmy'",
    "Keating, Stuart Black, Tom Forsyth, Tom Ireland and Wayne Imlach.",
    NULL,
    "Here's an effort to divide up what we did to make Urban Chaos!",
    NULL,

    "~BProgramming",
    "\tMike Diskett",
    "\tMark Adami",
    "\tMatthew Rosenfeld",
    "\tJames 'Dudley' Watson",
    "\tEddie Edwards",
    "\tGuy Simmons",
    "\tTom Forsyth",
    "~I\tAdditional programming",
    "\t\tJeremy Longley",
    NULL,

    "~BArt Direction",
    "\tFin McGechie",
    NULL,

    "~BArt",
    "\tStuart Black",
    "\tRichard Franke",
    "\tOllie Shaw",
    "\tFin McGechie",
    "\tJunior Walker",
    "\tChris Knott",
    "\tGary Carr",
    "~I\tAdditional art",
    "\t\tTerry Catrell",
    "\t\tJoe Rider",
    "\t\tSteve Brown",
    NULL,

    "~BAnimation",
    "\tOllie Shaw",
    "\tJunior Walker",
    "\tChris Knott",
    NULL,

    "~BLevel Design",
    "\tSimon 'Grimmy' Keating",
    "\tBarry Meade",
    NULL,

    "~BSound and Music",
    "\tMartin Oliver",
    NULL,

    "~BScripting",
    "\tBarry Meade",
    "\tSimon 'Grimmy' Keating",
    "\tFin McGechie",
    NULL,

    "~BTesting",
    "\tJustin (mucky)hands Amore",
    "~I\tAdditional testing",
    "\t\tSean Lamacraft",
    "\t\tMarie Colwell",
    "\t\tMark Rose",
    "\t\tDavid Harlow",
    "~I\tFurther testing",
    "\t\tChristopher Absolom",
    "\t\tMark Baker",
    "\t\tAlex Blackwood",
    "\t\tDahman Coombes",
    "\t\tTom Everard",
    "\t\tEamon Meadows",
    "\t\tAnthony Nicholson",
    "\t\tTom Patterson",
    "\t\tLawrence Phillips",
    "\t\tDaniel Purvis",
    "\t\tGary Reed",
    "\t\tAmy Ross",
    "\t\tPeter Ruscoe",
    "\t\tKraig Stone",
    "\t\tLorne Tietjen",
    "\t\tDavid Walker",
    "\t\tDavid Wright",
    NULL,
    "\tMichael Burnham",
    "\tTom Forsyth",
    "\tGary Carr",
    "\tJohn Steels",
    "\tTom Ireland",
    "\tJan Svarovsky",
    "\tand everyone at MuckyFoot",
    NULL,

    "~BIT and Mucky Website",
    "\tMichael Burnham",
    NULL,

    "~BPR",
    "\tCathy Campos at PanachePR",
    NULL,

    "~BFurry Friends",
    "\tSam",
    "\tLisa",
    "\tBarney",
    NULL,

    "Special thanks to Glenn Corpes and a big mucky \"Thank You\" from",
    "Mucky Foot to everyone at Eidos. Hello to the Wednesday night",
    "footballers!",
    "!"
};

// uc_orig: CREDITS_eidos_uk (fallen/outro/credits.cpp)
CBYTE* CREDITS_eidos_uk[] = {
    "~BSenior Producer",
    "\tDarren Hedges",
    NULL,
    "~BLocalisation Team",
    "\tHolly Andrews",
    "\tFlavia Timiani",
    NULL,

    "~BMarketing",
    "\tKaren Ridley",
    "\tLorna Evans",
    NULL,

    "~BPR",
    "\tJonathan Rosenblatt",
    "\tSteve Starvis",
    NULL,

    "~BCreative Services",
    "\tCaroline Simon",
    NULL,

    "~BLocalisation QA",
    "\tMichael Weissmuller",
    "\tAlex Lepoureau",
    "\tAlessandro Mantelli",
    NULL,

    "~BQA Manager",
    "\tTony Bourne",
    NULL,

    "~BLead QA",
    "\tJean Duret",
    NULL,

    "~BQA",
    "\tLinus Dominique (Assistant Lead)",
    "\tJason Walker (Hardware Specialist)",
    "\tMichael Hanley",
    "\tAndrew Christopher",
    "\tAnthony Fretwell",
    "\tBJ Samuel Kil",
    "\tChris Ince",
    "\tClint Nembhard",
    "\tEsmond Ferns",
    "\tGuy Cooper",
    "\tLouis Farnham",
    "\tMarlon Grant",
    "\tNoel Cowan",
    "\tPatrick Cowan",
    "\tTyrone O'Neil",
    NULL,

    "~BWith thanks for the last leg",
    "\tIain McNeil",
    "\tGrant Dean",
    "!"
};

// uc_orig: CREDITS_eidos_usa (fallen/outro/credits.cpp)
CBYTE* CREDITS_eidos_usa[] = {
    "~BAssociate Producer",
    "\tEric Adams",
    NULL,

    "~BQA Manager",
    "\tMike McHale",
    NULL,

    "~BProduct Manager",
    "\tJennifer Fitzsimmons",
    NULL,

    "~BMarketing Manager",
    "\tSusan Boshkoff",
    NULL,

    "~BPR",
    "\tBrian Kemp",
    "\tGreg Rizzer",
    NULL,

    "~BTesting",
    "\tCorey Fong",
    "\tKjell Vistad",
    "\tRalph Ortiz",
    "\tJohn Arvay",
    "!"
};

// uc_orig: CREDITS_eidos_france (fallen/outro/credits.cpp)
CBYTE* CREDITS_eidos_france[] = {
    "~BChef de produit",
    "\tOlivier Salomon",
    NULL,

    "~BResponsable localisation",
    "\tSt"
    "\xe9"
    "phan Gonizzi",
    NULL,

    "~BResponsable RP",
    "\tPriscille Demoly",
    NULL,

    "~BTesteur de la VF",
    "\tGuillaume Mahouin",
    NULL,

    "~BTraduction",
    "\tAround the Word, Paris",
    NULL,

    "~BEnregistrement des voix fran"
    "\xe7"
    "aises",
    "\tLe Lotus Rose, Paris",
    "!"
};

// uc_orig: CREDITS_eidos_germany (fallen/outro/credits.cpp)
CBYTE* CREDITS_eidos_germany[] = {
    "~BLeiter Produktentwicklung",
    "\tBeco Mulderij",
    NULL,

    "~BProdukt-Manager",
    "\tLars Wittkuhn",
    NULL,

    "~BLeiter Marketing",
    "\tKnut-Jochen Bergel",
    NULL,

    "~BPR-Manager",
    "\tSascha Denise Green-Kaiser",
    NULL,

    "~BLokalisierungs-Manager",
    "\tHeidi Maria Kohne",
    NULL,

    "~BQA-Manager",
    "S"
    "\xf6"
    "ren Winterfeldt",
    NULL,

    "~B"
    "\xdc"
    "bersetzung",
    "\tViolet Media, Isabel Sterner",
    NULL,

    "~BAudio",
    "\tHastings International Audio Network",
    NULL,

    "~BTonmeister",
    "\tCedric Hopf",
    "!"
};

// uc_orig: CREDITS_voice_production (fallen/outro/credits.cpp)
CBYTE* CREDITS_voice_production[] = {
    "~BCasting",
    "\tPhil Morris at AllintheGame",
    NULL,

    "~BVoice Production",
    "\tBarry Meade",
    "\tMartin Oliver",
    "\tChris O'Saughanessy",
    "\tPhil Morris",
    NULL,

    "~BVoice Actors",
    "\tJohnnie Fiori",
    "\tDan Russell",
    "\tSharon Holm",
    "\tKerry Shale",
    "\tJulienne Davis",
    "\tColin McFarlane",
    "\tTed Maynard",
    "\tBrad Lavelle",
    "\tTogo Igawa",
    NULL,

    "~BTranslation",
    "\tAround the Word, Paris",
    NULL,

    "~BFrench Voices",
    "\tLe Lotus Rose, Paris",
    "!"
};

// uc_orig: CREDITS_bands (fallen/outro/credits.cpp)
CBYTE* CREDITS_bands[] = {
    "Way Out West - Urban Chaos",
    "The 3 Jays - Feeling it too",
    "Tour De Force ",
    "Asian Dub Foundation and Audioactive - Psycho Buds",
    "Infidels - Everything",
    "Infidels - Ooma Gabba",
    "Special Forces 'Something Else... The bleep tune' courtesy of Photek Productions",
    NULL,
    "Many Thanks to Miles Jacobson at Anglo.",
    "!"
};

// uc_orig: CREDITS_section (fallen/outro/credits.cpp)
// All credits sections indexed for sequential display.
CREDITS_Section CREDITS_section[CREDITS_NUM_SECTIONS] = {
    { "MUCKYFOOT",
        CREDITS_muckyfoot },

    { "EIDOS UK",
        CREDITS_eidos_uk },

    { "EIDOS USA",
        CREDITS_eidos_usa },

    { "EIDOS FRANCE",
        CREDITS_eidos_france },

    { "EIDOS GERMANY",
        CREDITS_eidos_germany },

    { "VOICE PRODUCTION",
        CREDITS_voice_production },

    { "ORIGINAL CD MUSIC",
        CREDITS_bands }
};
