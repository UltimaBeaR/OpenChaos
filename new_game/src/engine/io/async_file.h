#ifndef ENGINE_IO_ASYNC_FILE_H
#define ENGINE_IO_ASYNC_FILE_H

#include "engine/core/types.h"
#include <cstdint>
#include <cstdio>

// Asynchronous file loading using a background worker thread.
// Files are queued for loading, read in chunks by the worker, and completed
// files can be polled via GetNextCompletedAsyncFile().

// uc_orig: AsyncFile (fallen/DDLibrary/Headers/AsyncFile2.h)
struct AsyncFile {
    FILE* hFile;
    UBYTE* buffer;
    int blen;
    void* hKey;
    AsyncFile* prev;
    AsyncFile* next;
};

// uc_orig: MAX_ASYNC_FILES (fallen/DDLibrary/Headers/AsyncFile2.h)
#define MAX_ASYNC_FILES 16

// uc_orig: InitAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void InitAsyncFile(void);
// uc_orig: TermAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void TermAsyncFile(void);
// uc_orig: LoadAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
bool LoadAsyncFile(char* filename, void* buffer, uint32_t blen, void* key);
// uc_orig: GetNextCompletedAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void* GetNextCompletedAsyncFile(void);
// uc_orig: CancelAsyncFile (fallen/DDLibrary/Source/AsyncFile2.cpp)
void CancelAsyncFile(void* key);

#endif // ENGINE_IO_ASYNC_FILE_H
