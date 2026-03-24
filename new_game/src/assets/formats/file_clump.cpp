// Container file (.gob/.ilf/.txc) reader/writer.
// Stores multiple files by numeric ID with offset/length index.

#include "assets/formats/file_clump.h"
#include "engine/io/file.h"

#include <cstring>

// uc_orig: FileClump::FileClump (fallen/DDLibrary/Source/FileClump.cpp)
FileClump::FileClump(const char* clumpfn, ULONG max_id, bool readonly)
{
    ClumpFD = NULL;
    MaxID = max_id;
    Offsets = NULL;
    Lengths = NULL;
    NextOffset = 0;
    ReadOnly = readonly;

    if (ReadOnly) {
        if (ClumpFD = MF_Fopen(clumpfn, "rb")) {
            fread(&MaxID, sizeof(ULONG), 1, ClumpFD);
            Offsets = new size_t[MaxID];
            Lengths = new size_t[MaxID];
            fread(Offsets, sizeof(size_t), MaxID, ClumpFD);
            fread(Lengths, sizeof(size_t), MaxID, ClumpFD);
        } else {
            MaxID = 0;
        }
    } else {
        if (ClumpFD = MF_Fopen(clumpfn, "wb")) {
            fwrite(&MaxID, sizeof(ULONG), 1, ClumpFD);
            Offsets = new size_t[MaxID];
            Lengths = new size_t[MaxID];
            for (ULONG ii = 0; ii < MaxID; ii++) {
                Offsets[ii] = Lengths[ii] = 0;
            }
            fwrite(Offsets, sizeof(size_t), MaxID, ClumpFD);
            fwrite(Lengths, sizeof(size_t), MaxID, ClumpFD);
            NextOffset = sizeof(ULONG) + 2 * MaxID * sizeof(size_t);
        } else {
            MaxID = 0;
        }
    }
}

// uc_orig: FileClump::~FileClump (fallen/DDLibrary/Source/FileClump.cpp)
FileClump::~FileClump()
{
    if (ClumpFD) {
        if (!ReadOnly) {
            fseek(ClumpFD, 0, SEEK_SET);
            fwrite(&MaxID, sizeof(ULONG), 1, ClumpFD);
            fwrite(Offsets, sizeof(size_t), MaxID, ClumpFD);
            fwrite(Lengths, sizeof(size_t), MaxID, ClumpFD);

            for (ULONG ii = 0; ii < MaxID; ii++) {
            }
        }
        MF_Fclose(ClumpFD);
    }
    delete[] Offsets;
    delete[] Lengths;
}

// uc_orig: FileClump::Exists (fallen/DDLibrary/Source/FileClump.cpp)
bool FileClump::Exists(ULONG id)
{
    if (id >= MaxID)
        return false;
    if (!Offsets[id])
        return false;

    return true;
}

// uc_orig: FileClump::Read (fallen/DDLibrary/Source/FileClump.cpp)
UBYTE* FileClump::Read(ULONG id)
{
    if (id >= MaxID)
        return NULL;
    if (!Offsets[id])
        return NULL;

    UBYTE* buffer = new UBYTE[Lengths[id]];

    if (buffer) {
        fseek(ClumpFD, Offsets[id], SEEK_SET);
        if (fread(buffer, 1, Lengths[id], ClumpFD) != Lengths[id]) {
            delete[] buffer;
            buffer = NULL;
        }
    }

    return buffer;
}

// uc_orig: FileClump::Write (fallen/DDLibrary/Source/FileClump.cpp)
bool FileClump::Write(void* buffer, size_t nbytes, ULONG id)
{
    if (id >= MaxID)
        return false;
    if (ReadOnly)
        return false;
    if (Offsets[id])
        return false;

    fseek(ClumpFD, NextOffset, SEEK_SET);
    if (fwrite(buffer, 1, nbytes, ClumpFD) != nbytes)
        return false;

    Offsets[id] = NextOffset;
    Lengths[id] = nbytes;
    NextOffset += nbytes;

    return true;
}
