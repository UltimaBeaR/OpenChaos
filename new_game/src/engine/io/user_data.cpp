// Per-user writable data folder ("overlay" root) — see engine/io/user_data.h.

#include "engine/io/user_data.h"
#include "engine/core/types.h" // oc_mkdir
#include "engine/platform/crash_log_file.h" // OC_crash_log_path declaration

#include <SDL3/SDL.h>
#include <cstdio>

// File written next to the writable data; kept identical to the historical name
// so existing tooling/docs still apply, just relocated into the user folder.
#define USERDATA_CRASH_LOG_NAME "OpenChaos.crash_log.txt"

// Root with a trailing '/', forward slashes. Empty until USERDATA_init succeeds.
static char g_root[512] = "";
// Absolute crash-log path, precomputed so crash handlers never call SDL/alloc.
// Defaults to a bare working-dir name in case a crash happens before init.
static char g_crash_log_path[512] = USERDATA_CRASH_LOG_NAME;

static bool path_is_absolute(const char* p)
{
    if (!p || !p[0])
        return false;
    if (p[0] == '/' || p[0] == '\\')
        return true;
#ifdef _WIN32
    if (p[1] == ':') // drive-letter path, e.g. C:\...
        return true;
#endif
    return false;
}

// Create every parent directory of `path` (every prefix ending in '/').
// Existing directories are left as-is — mkdir errors are intentionally ignored.
static void make_parent_dirs(char* path)
{
    for (char* p = path; *p; ++p) {
        if (*p == '/' && p != path) {
            *p = '\0';
            oc_mkdir(path);
            *p = '/';
        }
    }
}

void USERDATA_init()
{
    // SDL_GetPrefPath creates the folder and returns it with a trailing native
    // separator. It does not require SDL to be initialized.
    char* pref = SDL_GetPrefPath("", "OpenChaos");
    if (pref && pref[0]) {
        size_t n = 0;
        for (const char* s = pref; *s && n < sizeof(g_root) - 2; ++s)
            g_root[n++] = (*s == '\\') ? '/' : *s;
        if (n == 0 || g_root[n - 1] != '/')
            g_root[n++] = '/';
        g_root[n] = '\0';
    }
    if (pref)
        SDL_free(pref);

    // Precompute the crash-log path while it is safe to do so.
    if (g_root[0])
        snprintf(g_crash_log_path, sizeof(g_crash_log_path), "%s%s", g_root, USERDATA_CRASH_LOG_NAME);
}

const char* USERDATA_root()
{
    return g_root;
}

char* USERDATA_resolve_write(const char* rel, char* out, size_t out_size)
{
    if (!g_root[0] || path_is_absolute(rel)) {
        // No user folder (fall back to the working dir) or the caller passed an
        // absolute path — use it verbatim.
        snprintf(out, out_size, "%s", rel);
        return out;
    }
    snprintf(out, out_size, "%s%s", g_root, rel);
    for (char* p = out; *p; ++p)
        if (*p == '\\')
            *p = '/';
    make_parent_dirs(out);
    return out;
}

extern "C" const char* OC_crash_log_path(void)
{
    return g_crash_log_path;
}
