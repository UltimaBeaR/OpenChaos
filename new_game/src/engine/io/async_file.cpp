// Asynchronous file loading implementation.
// Worker thread reads files in chunks, using mutex + condition_variable for synchronization.
// Three doubly-linked lists (Free, Active, Complete) manage AsyncFile blocks.

#include <cstring>

#include "engine/io/async_file.h"
#include "engine/io/async_file_globals.h"
#include "engine/io/file.h"

// uc_orig: Unlink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void Unlink(AsyncFile* file);
// uc_orig: FreeLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void FreeLink(AsyncFile* file);
// uc_orig: ActiveLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void ActiveLink(AsyncFile* file);
// uc_orig: CompleteLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void CompleteLink(AsyncFile* file);
// uc_orig: ThreadRun (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void ThreadRun(void* arg);

// Predicate for condition variable: returns true when worker should wake up.
static int ThreadShouldWake(void* /*arg*/)
{
    return KillThread == 1 ||
           CancelKey != nullptr ||
           ActiveList.next != &ActiveList;
}

// uc_orig: InitAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void InitAsyncFile(void)
{
    memset(File, 0, sizeof(File));
    memset(&FreeList, 0, sizeof(FreeList));
    memset(&ActiveList, 0, sizeof(ActiveList));
    memset(&CompleteList, 0, sizeof(CompleteList));

    FreeList.next = FreeList.prev = &FreeList;
    ActiveList.next = ActiveList.prev = &ActiveList;
    CompleteList.next = CompleteList.prev = &CompleteList;

    for (int ii = 0; ii < MAX_ASYNC_FILES; ii++) {
        FreeLink(&File[ii]);
    }

    csLock = thread_mutex_create();
    cvEvent = thread_condvar_create();

    KillThread = 0;
    CancelKey = 0;

    workerThread = thread_create(ThreadRun, NULL);
}

// uc_orig: TermAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void TermAsyncFile(void)
{
    thread_mutex_lock(csLock);
    KillThread = 1;
    thread_mutex_unlock(csLock);

    thread_condvar_notify(cvEvent);
    thread_join(workerThread);
    workerThread = NULL;

    thread_condvar_destroy(cvEvent);
    cvEvent = NULL;
    thread_mutex_destroy(csLock);
    csLock = NULL;
}

// uc_orig: ThreadRun (fallen/DDLibrary/Source/AsyncFile2.cpp)
void ThreadRun(void* /*arg*/)
{
    for (;;) {
        // Wait until there's work to do.
        thread_mutex_lock(csLock);
        thread_condvar_wait(cvEvent, csLock, ThreadShouldWake, NULL);

        if (KillThread == 1) {
            KillThread = 2;
            thread_mutex_unlock(csLock);
            return;
        }

        if (CancelKey) {
            AsyncFile* file = ActiveList.next;
            while (file != &ActiveList) {
                AsyncFile* next = file->next;
                if (file->hKey == CancelKey) {
                    Unlink(file);
                    CompleteLink(file);
                }
                file = next;
            }
            CancelKey = NULL;
        }

        AsyncFile* file = NULL;

        if (ActiveList.next != &ActiveList) {
            file = ActiveList.next;
            Unlink(file);
        }

        thread_mutex_unlock(csLock);

        if (file) {
            while (file->blen > BytesPerMillisecond) {
                fread(file->buffer, 1, BytesPerMillisecond, file->hFile);
                file->buffer += BytesPerMillisecond;
                file->blen -= BytesPerMillisecond;
                thread_yield();
            }

            if (file->blen) {
                fread(file->buffer, 1, file->blen, file->hFile);
            }

            thread_mutex_lock(csLock);
            CompleteLink(file);
            thread_mutex_unlock(csLock);
        }
    }
}

// uc_orig: LoadAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
bool LoadAsyncFile(char* filename, void* buffer, uint32_t blen, void* key)
{
    if (FreeList.next == &FreeList)
        return false;

    AsyncFile* file = FreeList.next;

    file->hFile = MF_Fopen(filename, "rb");
    if (!file->hFile)
        return false;

    file->hKey = key;
    file->buffer = (UBYTE*)buffer;
    file->blen = blen;

    Unlink(file);

    thread_mutex_lock(csLock);
    ActiveLink(file);
    thread_mutex_unlock(csLock);

    thread_condvar_notify(cvEvent);

    return true;
}

// uc_orig: GetNextCompletedAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void* GetNextCompletedAsyncFile(void)
{
    thread_mutex_lock(csLock);
    for (AsyncFile* file = CompleteList.next; file != &CompleteList; file = file->next) {
        Unlink(file);
        thread_mutex_unlock(csLock);

        if (file->hFile) {
            fclose(file->hFile);
            file->hFile = NULL;
        }
        FreeLink(file);

        return file->hKey;
    }
    thread_mutex_unlock(csLock);

    return NULL;
}

// uc_orig: CancelAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void CancelAsyncFile(void* key)
{
    if (!key)
        return;

    thread_mutex_lock(csLock);
    CancelKey = key;
    thread_mutex_unlock(csLock);

    thread_condvar_notify(cvEvent);

    // Wait for worker to process the cancellation.
    for (;;) {
        thread_mutex_lock(csLock);
        if (!CancelKey) {
            thread_mutex_unlock(csLock);
            break;
        }
        thread_mutex_unlock(csLock);
        thread_yield();
    }
}

// uc_orig: Unlink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void Unlink(AsyncFile* file)
{
    file->next->prev = file->prev;
    file->prev->next = file->next;
}

// uc_orig: FreeLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void FreeLink(AsyncFile* file)
{
    file->prev = &FreeList;
    file->next = FreeList.next;

    file->next->prev = file;
    file->prev->next = file;
}

// uc_orig: ActiveLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void ActiveLink(AsyncFile* file)
{
    file->prev = &ActiveList;
    file->next = ActiveList.next;

    file->next->prev = file;
    file->prev->next = file;
}

// uc_orig: CompleteLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void CompleteLink(AsyncFile* file)
{
    file->prev = &CompleteList;
    file->next = CompleteList.next;

    file->next->prev = file;
    file->prev->next = file;
}
