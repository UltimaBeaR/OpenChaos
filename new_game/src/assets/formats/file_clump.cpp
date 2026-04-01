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
            // Clump files store offsets/lengths as 32-bit values (written by original 32-bit game).
            // Read as uint32_t and expand to size_t for 64-bit compatibility.
            uint32_t* tmp = new uint32_t[MaxID];
            fread(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            for (ULONG i = 0; i < MaxID; i++) Offsets[i] = tmp[i];
            fread(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            for (ULONG i = 0; i < MaxID; i++) Lengths[i] = tmp[i];
            delete[] tmp;
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
            // Write as uint32_t for compatibility with 32-bit clump format.
            uint32_t* tmp = new uint32_t[MaxID]();
            fwrite(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            fwrite(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            delete[] tmp;
            NextOffset = sizeof(ULONG) + 2 * MaxID * sizeof(uint32_t);
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
            // Write as uint32_t for 32-bit clump format compatibility.
            uint32_t* tmp = new uint32_t[MaxID];
            for (ULONG ii = 0; ii < MaxID; ii++) tmp[ii] = (uint32_t)Offsets[ii];
            fwrite(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            for (ULONG ii = 0; ii < MaxID; ii++) tmp[ii] = (uint32_t)Lengths[ii];
            fwrite(tmp, sizeof(uint32_t), MaxID, ClumpFD);
            delete[] tmp;
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

    // +2 padding: ReadSquished bit-stream decoder may read 1 UWORD past the
    // compressed data on the final iteration (benign overread in original code).
    UBYTE* buffer = new UBYTE[Lengths[id] + 2]();

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
