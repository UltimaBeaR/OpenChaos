#ifndef ENGINE_PLATFORM_CRASH_LOG_FILE_H
#define ENGINE_PLATFORM_CRASH_LOG_FILE_H

// Absolute path to the crash/exit log, inside the per-user data folder (see
// engine/io/user_data.h). Written on ANY abnormal or fatal termination — crash
// (exception/signal), abort(), failed ASSERT, or a fatal initialization failure
// (e.g. the display/GL context could not be created). The path is computed once
// at startup (USERDATA_init) into a static buffer, so this is safe to call from
// crash/signal handlers — no allocation, no SDL. Before USERDATA_init runs it
// returns a plain working-dir fallback name.
//
// Kept dependency-free so it can be included from crash_handler_win.cpp, which
// is deliberately isolated from project headers to avoid windows.h type clashes.
#ifdef __cplusplus
extern "C" {
#endif
const char* OC_crash_log_path(void);
#ifdef __cplusplus
}
#endif

#define OC_CRASH_LOG_FILE OC_crash_log_path()

#endif // ENGINE_PLATFORM_CRASH_LOG_FILE_H
