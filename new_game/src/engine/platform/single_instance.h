#ifndef ENGINE_PLATFORM_SINGLE_INSTANCE_H
#define ENGINE_PLATFORM_SINGLE_INSTANCE_H

// Cross-platform single-instance guard.
//
// Call once at startup. Returns true if this is the only running instance (the
// lock is acquired and held for the rest of the process lifetime), or false if
// another instance is already running — in which case the caller should exit
// silently, without showing any window or message.
//
// Fails open: if the underlying OS primitive can't be created, returns true
// (allow the launch) rather than blocking the game on a guard malfunction.
bool single_instance_acquire(void);

#endif // ENGINE_PLATFORM_SINGLE_INSTANCE_H
