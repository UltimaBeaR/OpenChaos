#include "engine/io/file.h"
#include "engine/io/file_globals.h"
#include "engine/io/user_data.h"

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
        if (comp_len >= sizeof(component))
            return false;
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
        if (!d)
            return false;

        bool found = false;
        struct dirent* entry;
        while ((entry = readdir(d)) != NULL) {
            if (strcasecmp(entry->d_name, component) == 0) {
                size_t name_len = strlen(entry->d_name);
                if (pos + name_len + 1 >= resolved_size) {
                    closedir(d);
                    return false;
                }
                memcpy(resolved + pos, entry->d_name, name_len);
                pos += name_len;
                found = true;
                break;
            }
        }
        closedir(d);
        if (!found)
            return false;

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
    if (f)
        return f;

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
// This is the GAME-FOLDER (working-dir) path — the read-only install.
// uc_orig: MakeFullPathName (MFStdLib/Source/StdLib/StdFile.cpp)
static CBYTE* MakeFullPathName(const CBYTE* cFilename)
{
    strcpy(cTempFilename, cBasePath);
    strcat(cTempFilename, cFilename);
    for (CBYTE* p = cTempFilename; *p; ++p) {
        if (*p == '\\')
            *p = '/';
    }
    return (cTempFilename);
}

// Build the user-data-folder path for `rel` (no directory creation — for reads).
// Returns false when there is no user folder, in which case the caller should
// use the game-folder path. See engine/io/user_data.h for the overlay model.
static bool MakeUserPathName(const char* rel, char* out, size_t out_size)
{
    const char* root = USERDATA_root();
    if (!root[0])
        return false;
    snprintf(out, out_size, "%s%s", root, rel);
    for (char* p = out; *p; ++p)
        if (*p == '\\')
            *p = '/';
    return true;
}

// uc_orig: FileExists (MFStdLib/Source/StdLib/StdFile.cpp)
// Overlay read: exists if the file is present in the user folder OR the game folder.
BOOL FileExists(CBYTE* file_name)
{
    char upath[512];
    if (MakeUserPathName(file_name, upath, sizeof(upath))) {
        FILE* uf = fopen_ci(upath, "rb");
        if (uf) {
            fclose(uf);
            return UC_TRUE;
        }
    }

    FILE* f = fopen_ci(MakeFullPathName(file_name), "rb");
    if (f) {
        fclose(f);
        return UC_TRUE;
    }
    return UC_FALSE;
}

// uc_orig: FileOpen (MFStdLib/Source/StdLib/StdFile.cpp)
// Overlay read: try the user folder first, then fall back to the game folder.
MFFileHandle FileOpen(CBYTE* file_name)
{
    char upath[512];
    if (MakeUserPathName(file_name, upath, sizeof(upath))) {
        FILE* uf = fopen_ci(upath, "rb");
        if (uf)
            return uf;
    }

    FILE* f = fopen_ci(MakeFullPathName(file_name), "rb");
    if (!f)
        return FILE_OPEN_ERROR;
    return f;
}

// uc_orig: FileClose (MFStdLib/Source/StdLib/StdFile.cpp)
void FileClose(MFFileHandle file_handle)
{
    if (file_handle)
        fclose(file_handle);
}

// uc_orig: FileCreate (MFStdLib/Source/StdLib/StdFile.cpp)
// Writes always go to the user folder (the install dir may be read-only).
MFFileHandle FileCreate(CBYTE* file_name, BOOL overwrite)
{
    char wpath[512];
    USERDATA_resolve_write(file_name, wpath, sizeof(wpath));

    if (!overwrite) {
        // CREATE_NEW semantics: fail if file already exists.
        FILE* test = fopen_ci(wpath, "rb");
        if (test) {
            fclose(test);
            return FILE_CREATION_ERROR;
        }
    }

    FILE* f = fopen_ci(wpath, "w+b");
    if (!f)
        return FILE_CREATION_ERROR;
    return f;
}

// uc_orig: FileDelete (MFStdLib/Source/StdLib/StdFile.cpp)
// Deletes the user-folder copy only — the game folder is read-only and is never
// touched (a file present only there reappears via the read overlay).
void FileDelete(CBYTE* file_name)
{
    char upath[512];
    const char* target = MakeUserPathName(file_name, upath, sizeof(upath))
        ? upath
        : MakeFullPathName(file_name);
#ifndef _WIN32
    char resolved[512];
    if (resolve_path_ci(target, resolved, sizeof(resolved))) {
        remove(resolved);
    } else {
        remove(target);
    }
#else
    remove(target);
#endif
}

// uc_orig: FileSize (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSize(MFFileHandle file_handle)
{
    if (!file_handle)
        return FILE_SIZE_ERROR;

    long pos = ftell(file_handle);
    if (fseek(file_handle, 0, SEEK_END) != 0)
        return FILE_SIZE_ERROR;
    long size = ftell(file_handle);
    fseek(file_handle, pos, SEEK_SET);

    if (size < 0)
        return FILE_SIZE_ERROR;
    return (SLONG)size;
}

// uc_orig: FileRead (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileRead(MFFileHandle file_handle, void* buffer, ULONG size)
{
    if (!file_handle)
        return FILE_READ_ERROR;

    size_t bytes_read = fread(buffer, 1, size, file_handle);
    if (bytes_read == 0 && ferror(file_handle))
        return FILE_READ_ERROR;
    return (SLONG)bytes_read;
}

// uc_orig: FileWrite (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileWrite(MFFileHandle file_handle, void* buffer, ULONG size)
{
    if (!file_handle)
        return FILE_WRITE_ERROR;

    size_t bytes_written = fwrite(buffer, 1, size, file_handle);
    if (bytes_written == 0 && ferror(file_handle))
        return FILE_WRITE_ERROR;
    return (SLONG)bytes_written;
}

// uc_orig: FileSeek (MFStdLib/Source/StdLib/StdFile.cpp)
SLONG FileSeek(MFFileHandle file_handle, const int mode, SLONG offset)
{
    if (!file_handle)
        return FILE_SEEK_ERROR;

    int whence;
    switch (mode) {
    case SEEK_MODE_BEGINNING:
        whence = SEEK_SET;
        break;
    case SEEK_MODE_CURRENT:
        whence = SEEK_CUR;
        break;
    case SEEK_MODE_END:
        whence = SEEK_END;
        break;
    default:
        return FILE_SEEK_ERROR;
    }

    if (fseek(file_handle, offset, whence) != 0)
        return FILE_SEEK_ERROR;
    return 0;
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
// Write/append modes go to the user folder; read modes use the overlay
// (user folder first, then game folder).
FILE* MF_Fopen(const char* file_name, const char* mode)
{
    const bool writing = mode && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'));
    if (writing) {
        char wpath[512];
        USERDATA_resolve_write(file_name, wpath, sizeof(wpath));
        return fopen_ci(wpath, mode);
    }

    char upath[512];
    if (MakeUserPathName(file_name, upath, sizeof(upath))) {
        FILE* uf = fopen_ci(upath, mode);
        if (uf)
            return uf;
    }
    return fopen_ci(MakeFullPathName(file_name), mode);
}

// uc_orig: MF_Fclose (MFStdLib/Source/StdLib/StdFile.cpp)
int MF_Fclose(FILE* stream)
{
    return fclose(stream);
}
