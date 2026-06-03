#ifndef ENGINE_PLATFORM_CRASH_LOG_FILE_H
#define ENGINE_PLATFORM_CRASH_LOG_FILE_H

// Name of the crash/exit log written next to the executable on ANY abnormal or
// fatal termination — crash (exception/signal), abort(), failed ASSERT, or a
// fatal initialization failure (e.g. the display/GL context could not be
// created). Kept in its own dependency-free header so it can be included from
// crash_handler_win.cpp, which is deliberately isolated from project headers to
// avoid windows.h type clashes.
#define OC_CRASH_LOG_FILE "OpenChaos.crash_log.txt"

#endif // ENGINE_PLATFORM_CRASH_LOG_FILE_H
