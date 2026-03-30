#ifndef ENGINE_IO_ASYNC_FILE_GLOBALS_H
#define ENGINE_IO_ASYNC_FILE_GLOBALS_H

#include "engine/io/async_file.h"
#include "engine/platform/threading_bridge.h"

// uc_orig: File (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern AsyncFile File[MAX_ASYNC_FILES];
// uc_orig: FreeList (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern AsyncFile FreeList;
// uc_orig: ActiveList (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern AsyncFile ActiveList;
// uc_orig: CompleteList (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern AsyncFile CompleteList;

// uc_orig: KillThread (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern int KillThread;
// uc_orig: CancelKey (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern void* CancelKey;

// uc_orig: csLock (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern ThreadMutex csLock;
// uc_orig: hEvent (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern ThreadCondVar cvEvent;
// uc_orig: hThread (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern ThreadHandle workerThread;

// uc_orig: BytesPerMillisecond (fallen/DDLibrary/Source/AsyncFile2.cpp)
extern const int BytesPerMillisecond;

#endif // ENGINE_IO_ASYNC_FILE_GLOBALS_H
