#include "core/timer.h"
#include "core/timer_globals.h"
#include "core/types.h"
#include <windows.h>

// Returns the performance counter frequency (ticks per second, low 32 bits).
// uc_orig: GetFineTimerFreq (fallen/DDEngine/Source/BreakTimer.cpp)
static inline ULONG GetFineTimerFreq()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return ULONG(freq.u.LowPart);
}

// Returns the current performance counter value (low 32 bits).
// uc_orig: GetFineTimerValue (fallen/DDEngine/Source/BreakTimer.cpp)
static inline ULONG GetFineTimerValue()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return ULONG(time.u.LowPart);
}

// Captures the current timer value as the start of a timed interval.
// uc_orig: StartStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
void StartStopwatch()
{
    stopwatch_start = GetFineTimerValue();
}

// Returns elapsed seconds since StartStopwatch() was called.
// uc_orig: StopStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
float StopStopwatch()
{
    ULONG time = GetFineTimerValue() - stopwatch_start;
    float secs = float(time) / float(GetFineTimerFreq());
    return secs;
}
