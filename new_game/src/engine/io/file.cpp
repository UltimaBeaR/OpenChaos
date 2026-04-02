#include "engine/io/file.h"
#include "engine/io/file_globals.h"

#include <cstring>
#include <cstdio>

// Prepend cBasePath to filename. Returns pointer to static buffer.
// Normalizes backslashes to forward slashes for cross-platform compatibility.
// uc_orig: MakeFullPathName (MFStdLib/Source/StdLib/StdFile.cpp)
static CBYTE* MakeFullPathName(const CBYTE* cFilename)
{
    strcpy(cTempFilename, cBasePath);
    strcat(cTempFilename, cFilename);
    for (CBYTE* p = cTempFilename; *p; ++p) {
        if (*p == '\\') *p = '/';
    }
    return (cTempFilename);
}

// uc_orig: FileExists (MFStdLib/Source/StdLib/StdFile.cpp)
BOOL FileExists(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);

    FILE* f = fopen(file_name, "rb");
    if (f) {
        fclose(f);
        return UC_TRUE;
    }
    return UC_FALSE;
}

// uc_orig: FileOpen (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileOpen(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);

    FILE* f = fopen(file_name, "rb");
    if (!f) return FILE_OPEN_ERROR;
    return f;
}

// uc_orig: FileClose (MFStdLib/Source/StdLib/StdFile.cpp)
void FileClose(MFFileHandle file_handle)
{
    if (file_handle) fclose(file_handle);
}

// uc_orig: FileCreate (MFStdLib/Source/StdLib/StdFile.cpp)
MFFileHandle FileCreate(CBYTE* file_name, BOOL overwrite)
{
    file_name = MakeFullPathName(file_name);

    if (!overwrite) {
        // CREATE_NEW semantics: fail if file already exists.
        FILE* test = fopen(file_name, "rb");
        if (test) {
            fclose(test);
            return FILE_CREATION_ERROR;
        }
    }

    FILE* f = fopen(file_name, "w+b");
    if (!f) return FILE_CREATION_ERROR;
    return f;
}

// uc_orig: FileDelete (MFStdLib/Source/StdLib/StdFile.cpp)
void FileDelete(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);
    remove(file_name);
}

// uc_orig: FileSize (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSize(MFFileHandle file_handle)
{
    if (!file_handle) return FILE_SIZE_ERROR;

    long pos = ftell(file_handle);
    if (fseek(file_handle, 0, SEEK_END) != 0) return FILE_SIZE_ERROR;
    long size = ftell(file_handle);
    fseek(file_handle, pos, SEEK_SET);

    if (size < 0) return FILE_SIZE_ERROR;
    return (SLONG)size;
}

// uc_orig: FileRead (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileRead(MFFileHandle file_handle, void* buffer, ULONG size)
{
    if (!file_handle) return FILE_READ_ERROR;

    size_t bytes_read = fread(buffer, 1, size, file_handle);
    if (bytes_read == 0 && ferror(file_handle))
        return FILE_READ_ERROR;
    return (SLONG)bytes_read;
}

// uc_orig: FileWrite (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileWrite(MFFileHandle file_handle, void* buffer, ULONG size)
{
    if (!file_handle) return FILE_WRITE_ERROR;

    size_t bytes_written = fwrite(buffer, 1, size, file_handle);
    if (bytes_written == 0 && ferror(file_handle))
        return FILE_WRITE_ERROR;
    return (SLONG)bytes_written;
}

// uc_orig: FileSeek (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSeek(MFFileHandle file_handle, const int mode, SLONG offset)
{
    if (!file_handle) return FILE_SEEK_ERROR;

    int whence;
    switch (mode) {
    case SEEK_MODE_BEGINNING: whence = SEEK_SET; break;
    case SEEK_MODE_CURRENT:   whence = SEEK_CUR; break;
    case SEEK_MODE_END:       whence = SEEK_END; break;
    default:                  return FILE_SEEK_ERROR;
    }

    if (fseek(file_handle, offset, whence) != 0)
        return FILE_SEEK_ERROR;
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
    cBasePath[MAX_LENGTH_OF_BASE_PATH - 1] = '\0';

    // Add trailing path separator if needed.
    CBYTE* pch = cBasePath;
    if (*pch != '\0') {
        while (*++pch != '\0') { }
        pch--;
        if (*pch != '\\' && *pch != '/') {
            *pch++ = '/';
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
    return fopen(file_name, mode);
}

// uc_orig: MF_Fclose (MFStdLib/Source/StdLib/StdFile.cpp)
int MF_Fclose(FILE* stream)
{
    return fclose(stream);
}
