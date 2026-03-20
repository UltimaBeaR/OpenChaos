#include "engine/io/file.h"

#include <cstring>

// uc_orig: MAX_LENGTH_OF_BASE_PATH (MFStdLib/Source/StdLib/StdFile.cpp)
#define MAX_LENGTH_OF_BASE_PATH 64

// uc_orig: cBasePath (MFStdLib/Source/StdLib/StdFile.cpp)
static CBYTE cBasePath[MAX_LENGTH_OF_BASE_PATH + 1];

// uc_orig: MAX_LENGTH_OF_FULL_NAME (MFStdLib/Source/StdLib/StdFile.cpp)
#define MAX_LENGTH_OF_FULL_NAME (MAX_LENGTH_OF_BASE_PATH + 128)

// uc_orig: cTempFilename (MFStdLib/Source/StdLib/StdFile.cpp)
static CBYTE cTempFilename[MAX_LENGTH_OF_FULL_NAME + 1];

// Prepend cBasePath to filename. Returns pointer to static buffer.
// uc_orig: MakeFullPathName (MFStdLib/Source/StdLib/StdFile.cpp)
static CBYTE* MakeFullPathName(const CBYTE* cFilename)
{
    strcpy(cTempFilename, cBasePath);
    strcat(cTempFilename, cFilename);
    return (cTempFilename);
}

// uc_orig: FileExists (MFStdLib/Source/StdLib/StdFile.cpp)
BOOL FileExists(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);

    if (GetFileAttributes(file_name) == 0xffffffff)
        return FALSE;
    else
        return TRUE;
}

// uc_orig: FileOpen (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileOpen(CBYTE* file_name)
{
    MFFileHandle result = FILE_OPEN_ERROR;

    if (FileExists(file_name)) {
        file_name = MakeFullPathName(file_name);

        result = CreateFile(
            file_name,
            (GENERIC_READ),
            (FILE_SHARE_READ),
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (result == INVALID_HANDLE_VALUE) {
            result = FILE_OPEN_ERROR;
        }
    }
    return result;
}

// uc_orig: FileClose (MFStdLib/Source/StdLib/StdFile.cpp)
void FileClose(MFFileHandle file_handle)
{
    CloseHandle(file_handle);
}

// uc_orig: FileCreate (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileCreate(CBYTE* file_name, BOOL overwrite)
{
    DWORD creation_mode;
    MFFileHandle result;

    file_name = MakeFullPathName(file_name);

    if (overwrite) {
        creation_mode = CREATE_ALWAYS;
    } else {
        creation_mode = CREATE_NEW;
    }
    result = CreateFile(
        file_name,
        (GENERIC_READ | GENERIC_WRITE),
        0,
        NULL,
        creation_mode,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (result == INVALID_HANDLE_VALUE)
        result = FILE_CREATION_ERROR;

    return result;
}

// uc_orig: FileDelete (MFStdLib/Source/StdLib/StdFile.cpp)
void FileDelete(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);
    DeleteFile(file_name);
}

// uc_orig: FileSize (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSize(MFFileHandle file_handle)
{
    DWORD result;

    result = GetFileSize(file_handle, NULL);
    if (result == 0xffffffff)
        return FILE_SIZE_ERROR;
    else
        return (SLONG)result;
}

// uc_orig: FileRead (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileRead(MFFileHandle file_handle, void* buffer, ULONG size)
{
    SLONG bytes_read;

    if (ReadFile(file_handle, buffer, size, (LPDWORD)&bytes_read, NULL) == FALSE)
        return FILE_READ_ERROR;
    else
        return bytes_read;
}

// uc_orig: FileWrite (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileWrite(MFFileHandle file_handle, void* buffer, ULONG size)
{
    SLONG bytes_written;

    if (WriteFile(file_handle, buffer, size, (LPDWORD)&bytes_written, NULL) == FALSE)
        return FILE_WRITE_ERROR;
    else
        return bytes_written;
}

// uc_orig: FileSeek (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSeek(MFFileHandle file_handle, const int mode, SLONG offset)
{
    DWORD method;

    switch (mode) {
    case SEEK_MODE_BEGINNING:
        method = FILE_BEGIN;
        break;
    case SEEK_MODE_CURRENT:
        method = FILE_CURRENT;
        break;
    case SEEK_MODE_END:
        method = FILE_END;
        break;
    }
    if (SetFilePointer(file_handle, offset, NULL, method) == 0xffffffff)
        return FILE_SEEK_ERROR;
    else
        return 0;
}

// uc_orig: FileLoadAt (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileLoadAt(CBYTE* file_name, void* buffer)
{
    SLONG size;
    MFFileHandle handle;

    handle = FileOpen(file_name);
    if (handle != FILE_OPEN_ERROR) {
        size = FileSize(handle);
        if (size > 0) {
            if (FileRead(handle, buffer, size) == size) {
                FileClose(handle);
                return size;
            }
        }
        FileClose(handle);
    }
    return FILE_LOAD_AT_ERROR;
}

// uc_orig: FileSetBasePath (MFStdLib/Source/StdLib/StdFile.cpp)
void FileSetBasePath(CBYTE* path_name)
{
    strncpy(cBasePath, path_name, MAX_LENGTH_OF_BASE_PATH - 1);
    // Add trailing backslash if needed.
    CBYTE* pch = cBasePath;
    if (*pch != '\0') {
        while (*++pch != '\0') { }
        pch--;
        if (*pch != '\\') {
            *pch++ = '\\';
            *pch = '\0';
        }
    }
}

// uc_orig: MF_Fopen (MFStdLib/Source/StdLib/StdFile.cpp)
FILE* MF_Fopen(const char* file_name, const char* mode)
{
    if (!FileExists((char*)file_name)) {
        return NULL;
    }
    file_name = MakeFullPathName(file_name);
    return (fopen((char*)file_name, (char*)mode));
}

// uc_orig: MF_Fclose (MFStdLib/Source/StdLib/StdFile.cpp)
int MF_Fclose(FILE* stream)
{
    return (fclose(stream));
}
