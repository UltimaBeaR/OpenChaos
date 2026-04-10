#include "engine/io/file.h"
#include "engine/io/file_globals.h"

#include <cstring>
#include <cstdio>

#ifndef _WIN32
#include <dirent.h>
#include <strings.h>

// Resolve a full path case-insensitively on Linux/macOS.
// Walks each path component, scanning the directory for a case-insensitive match.
// Returns true and writes the resolved path to `resolved` (must be >= 512 bytes).
static bool resolve_path_ci(const char* path, char* resolved, size_t resolved_size)
{
    // Preserve leading absolute path prefix.
    size_t pos = 0;
    const char* rest = path;
    if (path[0] == '/') {
        resolved[0] = '/';
        pos = 1;
        rest = path + 1;
    }

    while (*rest) {
        // Extract next path component.
        const char* slash = strchr(rest, '/');
        size_t comp_len = slash ? (size_t)(slash - rest) : strlen(rest);

        char component[256];
        if (comp_len >= sizeof(component)) return false;
        memcpy(component, rest, comp_len);
        component[comp_len] = '\0';

        // Build current directory prefix for opendir.
        char dir_so_far[512];
        if (pos == 0) {
            strcpy(dir_so_far, ".");
        } else {
            memcpy(dir_so_far, resolved, pos);
            // Remove trailing slash for opendir.
            if (pos > 1 && dir_so_far[pos - 1] == '/')
                dir_so_far[pos - 1] = '\0';
            else
                dir_so_far[pos] = '\0';
        }

        // Try exact match first (fast path).
        char candidate[512];
        snprintf(candidate, sizeof(candidate), "%s/%s", dir_so_far, component);
        // For intermediate dirs we could stat(), but just try opendir scan on mismatch.

        DIR* d = opendir(dir_so_far);
        if (!d) return false;

        bool found = false;
        struct dirent* entry;
        while ((entry = readdir(d)) != NULL) {
            if (strcasecmp(entry->d_name, component) == 0) {
                size_t name_len = strlen(entry->d_name);
                if (pos + name_len + 1 >= resolved_size) { closedir(d); return false; }
                memcpy(resolved + pos, entry->d_name, name_len);
                pos += name_len;
                found = true;
                break;
            }
        }
        closedir(d);
        if (!found) return false;

        // Add slash separator if more components follow.
        if (slash) {
            resolved[pos++] = '/';
            rest = slash + 1;
        } else {
            break;
        }
    }

    resolved[pos] = '\0';
    return true;
}

// Case-insensitive fopen fallback for Linux/macOS.
// If exact path fails, resolve case-insensitively and retry.
FILE* fopen_ci(const char* path, const char* mode)
{
    FILE* f = fopen(path, mode);
    if (f) return f;

    char resolved[512];
    if (resolve_path_ci(path, resolved, sizeof(resolved))) {
        return fopen(resolved, mode);
    }
    return NULL;
}
#else
// On Windows the filesystem is case-insensitive, plain fopen is fine.
FILE* fopen_ci(const char* path, const char* mode) { return fopen(path, mode); }
#endif

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

    FILE* f = fopen_ci(file_name, "rb");
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

    FILE* f = fopen_ci(file_name, "rb");
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
        FILE* test = fopen_ci(file_name, "rb");
        if (test) {
            fclose(test);
            return FILE_CREATION_ERROR;
        }
    }

    FILE* f = fopen_ci(file_name, "w+b");
    if (!f) return FILE_CREATION_ERROR;
    return f;
}

// uc_orig: FileDelete (MFStdLib/Source/StdLib/StdFile.cpp)
void FileDelete(CBYTE* file_name)
{
    file_name = MakeFullPathName(file_name);
#ifndef _WIN32
    char resolved[512];
    if (resolve_path_ci(file_name, resolved, sizeof(resolved))) {
        remove(resolved);
    } else {
        remove(file_name);
    }
#else
    remove(file_name);
#endif
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
    return fopen_ci(file_name, mode);
}

// uc_orig: MF_Fclose (MFStdLib/Source/StdLib/StdFile.cpp)
int MF_Fclose(FILE* stream)
{
    return fclose(stream);
}
