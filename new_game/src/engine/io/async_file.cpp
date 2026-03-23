// Asynchronous file loading implementation.
// Worker thread reads files in chunks, using critical sections for synchronization.
// Three doubly-linked lists (Free, Active, Complete) manage AsyncFile blocks.

#include <windows.h>
#include <process.h>

#include "engine/io/async_file.h"
#include "engine/io/async_file_globals.h"

// uc_orig: Unlink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void Unlink(AsyncFile* file);
// uc_orig: FreeLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void FreeLink(AsyncFile* file);
// uc_orig: ActiveLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void ActiveLink(AsyncFile* file);
// uc_orig: CompleteLink (fallen/DDLibrary/Source/AsyncFile2.cpp)
static void CompleteLink(AsyncFile* file);
// uc_orig: ThreadRun (fallen/DDLibrary/Source/AsyncFile2.cpp)
static DWORD WINAPI ThreadRun(LPVOID arg);

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

    InitializeCriticalSection(&csLock);

    hEvent = CreateEvent(NULL, UC_FALSE, UC_FALSE, NULL);

    KillThread = 0;
    CancelKey = 0;

    DWORD tid;
    hThread = CreateThread(NULL, 0, ThreadRun, NULL, 0, &tid);
}

// uc_orig: TermAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void TermAsyncFile(void)
{
    EnterCriticalSection(&csLock);
    KillThread = 1;
    LeaveCriticalSection(&csLock);

    SetEvent(hEvent);

    EnterCriticalSection(&csLock);
    while (KillThread != 2) {
        LeaveCriticalSection(&csLock);
        Sleep(0);
        EnterCriticalSection(&csLock);
    }
    LeaveCriticalSection(&csLock);

    CloseHandle(hThread);
    CloseHandle(hEvent);
}

// uc_orig: ThreadRun (fallen/DDLibrary/Source/AsyncFile2.cpp)
DWORD WINAPI ThreadRun(LPVOID arg)
{
    for (;;) {
        WaitForSingleObject(hEvent, INFINITE);

        EnterCriticalSection(&csLock);

        if (KillThread == 1) {
            KillThread = 2;
            LeaveCriticalSection(&csLock);
            return 0;
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

        LeaveCriticalSection(&csLock);

        if (file) {
            DWORD amount;

            while (file->blen > BytesPerMillisecond) {
                ReadFile(file->hFile, file->buffer, BytesPerMillisecond, &amount, NULL);
                file->buffer += BytesPerMillisecond;
                file->blen -= BytesPerMillisecond;
                Sleep(0);
            }

            if (file->blen) {
                ReadFile(file->hFile, file->buffer, file->blen, &amount, NULL);
            }

            EnterCriticalSection(&csLock);
            CompleteLink(file);
            LeaveCriticalSection(&csLock);
        }
    }

    return 0;
}

// uc_orig: LoadAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
bool LoadAsyncFile(char* filename, void* buffer, DWORD blen, void* key)
{
    if (FreeList.next == &FreeList)
        return false;

    AsyncFile* file = FreeList.next;

    file->hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (!file->hFile)
        return false;

    file->hKey = key;
    file->buffer = (UBYTE*)buffer;
    file->blen = blen;

    Unlink(file);

    EnterCriticalSection(&csLock);
    ActiveLink(file);
    LeaveCriticalSection(&csLock);

    SetEvent(hEvent);

    return true;
}

// uc_orig: GetNextCompletedAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void* GetNextCompletedAsyncFile(void)
{
    EnterCriticalSection(&csLock);
    for (AsyncFile* file = CompleteList.next; file != &CompleteList; file = file->next) {
        Unlink(file);
        LeaveCriticalSection(&csLock);

        CloseHandle(file->hFile);
        FreeLink(file);

        return file->hKey;
    }
    LeaveCriticalSection(&csLock);

    return NULL;
}

// uc_orig: CancelAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void CancelAsyncFile(void* key)
{
    if (!key)
        return;

    EnterCriticalSection(&csLock);
    CancelKey = key;
    LeaveCriticalSection(&csLock);

    SetEvent(hEvent);

    EnterCriticalSection(&csLock);
    while (CancelKey) {
        LeaveCriticalSection(&csLock);
        Sleep(0);
        EnterCriticalSection(&csLock);
    }
    LeaveCriticalSection(&csLock);
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
