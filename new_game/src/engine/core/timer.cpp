#include "engine/core/timer.h"
#include "engine/core/timer_globals.h"
#include "engine/platform/sdl3_bridge.h"

// Captures the current timer value as the start of a timed interval.
// uc_orig: StartStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
void StartStopwatch()
{
    stopwatch_start = sdl3_get_performance_counter();
}

// Returns elapsed seconds since StartStopwatch() was called.
// uc_orig: StopStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
float StopStopwatch()
{
    uint64_t elapsed = sdl3_get_performance_counter() - stopwatch_start;
    return (float)elapsed / (float)sdl3_get_performance_frequency();
}
