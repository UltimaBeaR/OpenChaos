#ifndef ASSETS_FILE_CLUMP_H
#define ASSETS_FILE_CLUMP_H

#include "engine/core/types.h"
#include <cstdio>

// Container format for bundling multiple files into a single archive (.gob/.ilf/.txc).
// Header: ULONG max_id + offset/length arrays. Data follows sequentially.

// uc_orig: FileClump (fallen/DDLibrary/Headers/FileClump.h)
class FileClump {
public:
    // uc_orig: FileClump::FileClump (fallen/DDLibrary/Source/FileClump.cpp)
    FileClump(const char* clumpfn, ULONG max_id, bool readonly);
    // uc_orig: FileClump::~FileClump (fallen/DDLibrary/Source/FileClump.cpp)
    ~FileClump();

    // uc_orig: FileClump::Exists (fallen/DDLibrary/Source/FileClump.cpp)
    bool Exists(ULONG id);

    // uc_orig: FileClump::Read (fallen/DDLibrary/Source/FileClump.cpp)
    UBYTE* Read(ULONG id);
    // uc_orig: FileClump::Write (fallen/DDLibrary/Source/FileClump.cpp)
    bool Write(void* buffer, size_t nbytes, ULONG id);

private:
    FILE* ClumpFD;
    ULONG MaxID;
    size_t* Offsets;
    size_t* Lengths;
    size_t NextOffset;
    bool ReadOnly;
};

#endif // ASSETS_FILE_CLUMP_H
