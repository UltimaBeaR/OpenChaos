// BreakTimer.h
//
// break timer

// Interface:
//
// Simply call BreakTime(name) at each breakpoint and BreakFrame() after the screen flip call
// Call BreakStart() and BreakEnd() at start/end of game replay code
//
// The library reports on the times between frames and breakpoints
// by storing peak times (min and max) and average time between points

#define BreakStart() 0
#define BreakEnd(X) 0
#define BreakTime(X) 0
#define BreakFacets(N) 0
#define BreakFrame() 0

extern void StartStopwatch();
extern float StopStopwatch();
