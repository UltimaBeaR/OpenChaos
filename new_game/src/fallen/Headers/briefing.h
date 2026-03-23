//
// briefing.h
// mission selection, briefings
// 14 dec 98
//

#ifndef FALLEN_HEADERS_BRIEFING_H
#define FALLEN_HEADERS_BRIEFING_H

#include "MFStdLib.h"

extern CBYTE BRIEFING_mission_filename[_MAX_PATH];

SBYTE BRIEFING_select(void);
SBYTE BRIEFING_next_mission(); // 0 = run out of missions

#endif // FALLEN_HEADERS_BRIEFING_H
