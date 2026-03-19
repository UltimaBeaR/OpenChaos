// BreakTimer.cpp
//
// break timer for timing portions of code

#include <MFStdLib.h>
#include <cmath>
#include "BreakTimer.h"

// Windows timer access functions

static inline ULONG GetFineTimerFreq()
{
    LARGE_INTEGER freq;

    QueryPerformanceFrequency(&freq);

    return ULONG(freq.u.LowPart);
}

static inline ULONG GetFineTimerValue()
{
    LARGE_INTEGER time;

    QueryPerformanceCounter(&time);

    return ULONG(time.u.LowPart);
}

// basic timing

static ULONG stopwatch_start;

void StartStopwatch()
{
    stopwatch_start = GetFineTimerValue();
}

float StopStopwatch()
{
    ULONG time = GetFineTimerValue() - stopwatch_start;

    float secs = float(time) / float(GetFineTimerFreq());

    return secs;
}
