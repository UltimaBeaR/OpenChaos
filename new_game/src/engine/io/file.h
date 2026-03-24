#ifndef ENGINE_IO_FILE_H
#define ENGINE_IO_FILE_H

#include "engine/core/types.h"

// Windows API for HANDLE, BOOL.
// Temporary dependency until Stage 8 (cross-platform).
#include <windows.h>

#include <cstdio>

// Opaque file handle wrapping Windows HANDLE.
// uc_orig: MFFileHandle (MFStdLib/Headers/StdFile.h)
typedef HANDLE MFFileHandle;

// Sentinel values returned on failure.
// uc_orig: FILE_OPEN_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_OPEN_ERROR ((MFFileHandle) - 100)

// uc_orig: FILE_CLOSE_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_CLOSE_ERROR ((MFFileHandle) - 101)

// uc_orig: FILE_CREATION_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_CREATION_ERROR ((MFFileHandle) - 102)

// uc_orig: FILE_SIZE_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_SIZE_ERROR ((SLONG) - 103)

// uc_orig: FILE_READ_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_READ_ERROR ((SLONG) - 104)

// uc_orig: FILE_WRITE_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_WRITE_ERROR ((SLONG) - 105)

// uc_orig: FILE_SEEK_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_SEEK_ERROR ((SLONG) - 106)

// uc_orig: FILE_LOAD_AT_ERROR (MFStdLib/Headers/StdFile.h)
#define FILE_LOAD_AT_ERROR ((SLONG) - 107)

// Seek origin constants for FileSeek().
// uc_orig: SEEK_MODE_BEGINNING (MFStdLib/Headers/StdFile.h)
#define SEEK_MODE_BEGINNING 0

// uc_orig: SEEK_MODE_CURRENT (MFStdLib/Headers/StdFile.h)
#define SEEK_MODE_CURRENT 1

// uc_orig: SEEK_MODE_END (MFStdLib/Headers/StdFile.h)
#define SEEK_MODE_END 2

// All file paths are resolved relative to a base path set via FileSetBasePath().

// uc_orig: FileExists (MFStdLib/Source/StdLib/StdFile.cpp)
BOOL FileExists(CBYTE* file_name);

// uc_orig: FileOpen (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileOpen(CBYTE* file_name);

// uc_orig: FileClose (MFStdLib/Source/StdLib/StdFile.cpp)
void FileClose(MFFileHandle file_handle);

// uc_orig: FileCreate (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileCreate(CBYTE* file_name, BOOL overwrite);

// uc_orig: FileDelete (MFStdLib/Source/StdLib/StdFile.cpp)
void FileDelete(CBYTE* file_name);

// uc_orig: FileSize (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSize(MFFileHandle file_handle);

// uc_orig: FileRead (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileRead(MFFileHandle file_handle, void* buffer, ULONG size);

// uc_orig: FileWrite (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileWrite(MFFileHandle file_handle, void* buffer, ULONG size);

// uc_orig: FileSeek (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSeek(MFFileHandle file_handle, const int mode, SLONG offset);

// Read entire file into a pre-allocated buffer. Returns file size or FILE_LOAD_AT_ERROR.
// uc_orig: FileLoadAt (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileLoadAt(CBYTE* file_name, void* buffer);

// Set the base directory prepended to all file paths.
// uc_orig: FileSetBasePath (MFStdLib/Source/StdLib/StdFile.cpp)
void FileSetBasePath(CBYTE* path_name);

// fopen/fclose wrappers that resolve paths through the base path system.
// uc_orig: MF_Fopen (MFStdLib/Source/StdLib/StdFile.cpp)
FILE* MF_Fopen(const char* file_name, const char* mode);

// uc_orig: MF_Fclose (MFStdLib/Source/StdLib/StdFile.cpp)
int MF_Fclose(FILE* stream);

#endif // ENGINE_IO_FILE_H
