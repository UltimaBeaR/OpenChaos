#include "engine/io/async_file_globals.h"

// uc_orig: File (fallen/DDLibrary/Source/AsyncFile2.cpp)
AsyncFile File[MAX_ASYNC_FILES];
// uc_orig: FreeList (fallen/DDLibrary/Source/AsyncFile2.cpp)
AsyncFile FreeList;
// uc_orig: ActiveList (fallen/DDLibrary/Source/AsyncFile2.cpp)
AsyncFile ActiveList;
// uc_orig: CompleteList (fallen/DDLibrary/Source/AsyncFile2.cpp)
AsyncFile CompleteList;

// uc_orig: KillThread (fallen/DDLibrary/Source/AsyncFile2.cpp)
int KillThread;
// uc_orig: CancelKey (fallen/DDLibrary/Source/AsyncFile2.cpp)
void* CancelKey;

// uc_orig: csLock (fallen/DDLibrary/Source/AsyncFile2.cpp)
ThreadMutex csLock;
// uc_orig: hEvent (fallen/DDLibrary/Source/AsyncFile2.cpp)
ThreadCondVar cvEvent;
// uc_orig: hThread (fallen/DDLibrary/Source/AsyncFile2.cpp)
ThreadHandle workerThread;

// uc_orig: BytesPerMillisecond (fallen/DDLibrary/Source/AsyncFile2.cpp)
const int BytesPerMillisecond = 128 * 1024;
