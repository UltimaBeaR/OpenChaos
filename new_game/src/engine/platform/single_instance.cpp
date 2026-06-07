#include "engine/platform/single_instance.h"

#ifdef _WIN32

#include <windows.h>

// Named mutex in the Local (per-session) namespace. A second launch in the same
// user session finds it already created and bails. The handle is held for the
// whole process lifetime (never closed) so the mutex exists as long as we run;
// Windows destroys it automatically when the process exits, including on a
// crash. (Use a "Global\\" prefix instead to block across user sessions / fast-
// user-switching — not done here; per-session is the usual game behaviour.)
static const char* const SINGLE_INSTANCE_MUTEX_NAME = "OpenChaos-SingleInstance";

bool single_instance_acquire(void)
{
    HANDLE mutex = CreateMutexA(NULL, FALSE, SINGLE_INSTANCE_MUTEX_NAME);
    if (mutex == NULL)
        return true; // guard malfunction -> fail open, allow launch

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return false; // another instance already owns the mutex
    }

    // Intentionally leak `mutex`: hold it for the whole process lifetime.
    return true;
}

#else // POSIX (macOS / Linux)

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

// Advisory whole-file lock on a fixed lock file. flock() is released
// automatically when the process exits (including on a crash), so a stale lock
// never blocks a relaunch — the lock file itself persisting is harmless. The
// file lives in the temp dir (TMPDIR, else /tmp), which is always user-writable
// even when the game is installed in a read-only location.
static const char* const SINGLE_INSTANCE_LOCK_NAME = "OpenChaos.lock";
// Buffer for "<tmpdir>/OpenChaos.lock" — temp paths are well under this.
static const int SINGLE_INSTANCE_PATH_MAX = 1024;

bool single_instance_acquire(void)
{
    const char* tmp = getenv("TMPDIR");
    if (tmp == NULL || tmp[0] == '\0')
        tmp = "/tmp";

    char path[SINGLE_INSTANCE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", tmp, SINGLE_INSTANCE_LOCK_NAME);

    int fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd < 0)
        return true; // guard malfunction -> fail open, allow launch

    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return false; // another instance holds the lock
    }

    // Intentionally leak `fd`: hold the lock for the whole process lifetime.
    return true;
}

#endif
