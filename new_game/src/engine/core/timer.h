#ifndef ENGINE_CORE_TIMER_H
#define ENGINE_CORE_TIMER_H

// High-resolution stopwatch using SDL_GetPerformanceCounter.
// StartStopwatch() captures the start time; StopStopwatch() returns elapsed seconds.

// No-op profiling markers (BreakTimer system disabled in Release build).
// uc_orig: BreakStart (fallen/DDEngine/Headers/BreakTimer.h)
#define BreakStart() 0
// uc_orig: BreakEnd (fallen/DDEngine/Headers/BreakTimer.h)
#define BreakEnd(X)
// uc_orig: BreakFacets (fallen/DDEngine/Headers/BreakTimer.h)
#define BreakFacets(N)
// uc_orig: BreakFrame (fallen/DDEngine/Headers/BreakTimer.h)
#define BreakFrame()
// uc_orig: BreakTime (fallen/DDEngine/Headers/BreakTimer.h)
#define BreakTime(X) 0

// uc_orig: StartStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
void StartStopwatch();
// uc_orig: StopStopwatch (fallen/DDEngine/Source/BreakTimer.cpp)
float StopStopwatch();

#endif // ENGINE_CORE_TIMER_H
