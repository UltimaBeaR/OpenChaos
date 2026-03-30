// Threading bridge implementation.
// Compiled with /Zp8 to avoid alignment issues with MSVC's STL headers.

#include "engine/platform/threading_bridge.h"

#include <condition_variable>
#include <mutex>
#include <thread>

// --- Mutex ---

ThreadMutex thread_mutex_create(void)
{
    return new std::mutex();
}

void thread_mutex_destroy(ThreadMutex m)
{
    delete static_cast<std::mutex*>(m);
}

void thread_mutex_lock(ThreadMutex m)
{
    static_cast<std::mutex*>(m)->lock();
}

void thread_mutex_unlock(ThreadMutex m)
{
    static_cast<std::mutex*>(m)->unlock();
}

// --- Condition variable ---

ThreadCondVar thread_condvar_create(void)
{
    return new std::condition_variable();
}

void thread_condvar_destroy(ThreadCondVar cv)
{
    delete static_cast<std::condition_variable*>(cv);
}

void thread_condvar_notify(ThreadCondVar cv)
{
    static_cast<std::condition_variable*>(cv)->notify_one();
}

void thread_condvar_wait(ThreadCondVar cv, ThreadMutex m,
                         int (*predicate_func)(void* arg), void* predicate_arg)
{
    auto* mtx = static_cast<std::mutex*>(m);
    auto* cond = static_cast<std::condition_variable*>(cv);

    std::unique_lock<std::mutex> lock(*mtx, std::adopt_lock);
    cond->wait(lock, [&] { return predicate_func(predicate_arg) != 0; });
    lock.release(); // Caller still owns the lock.
}

// --- Thread ---

ThreadHandle thread_create(ThreadFunc func, void* arg)
{
    return new std::thread(func, arg);
}

void thread_join(ThreadHandle t)
{
    auto* thr = static_cast<std::thread*>(t);
    if (thr->joinable()) {
        thr->join();
    }
    delete thr;
}

void thread_yield(void)
{
    std::this_thread::yield();
}
