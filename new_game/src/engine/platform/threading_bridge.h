#pragma once

// Threading bridge — wraps std::thread/mutex/condition_variable behind C functions.
//
// Like sdl3_bridge, this exists because the project compiles with /Zp1
// (1-byte struct packing), which breaks alignment static_asserts in MSVC's
// <thread>, <mutex>, and <condition_variable> headers. The bridge .cpp is
// compiled with /Zp8.

// Opaque handle to a mutex.
typedef void* ThreadMutex;
// Opaque handle to a condition variable.
typedef void* ThreadCondVar;
// Opaque handle to a thread.
typedef void* ThreadHandle;

// Thread entry point signature.
typedef void (*ThreadFunc)(void* arg);

// --- Mutex ---

ThreadMutex thread_mutex_create(void);
void thread_mutex_destroy(ThreadMutex m);
void thread_mutex_lock(ThreadMutex m);
void thread_mutex_unlock(ThreadMutex m);

// --- Condition variable ---

ThreadCondVar thread_condvar_create(void);
void thread_condvar_destroy(ThreadCondVar cv);
// Wake one waiting thread.
void thread_condvar_notify(ThreadCondVar cv);
// Wait until predicate returns true. predicate_func is called with predicate_arg.
void thread_condvar_wait(ThreadCondVar cv, ThreadMutex m,
                         int (*predicate_func)(void* arg), void* predicate_arg);

// --- Thread ---

ThreadHandle thread_create(ThreadFunc func, void* arg);
// Wait for thread to finish. Cleans up the handle.
void thread_join(ThreadHandle t);
// Yield current thread's time slice.
void thread_yield(void);
