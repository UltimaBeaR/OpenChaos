#include "assets/tga.h"
#include "assets/tga_globals.h"
#include "engine/io/file.h"
#include <cstring>

// uc_orig: tclump (fallen/DDLibrary/Source/Tga.cpp)
static FileClump* tclump = NULL;

// uc_orig: writing (fallen/DDLibrary/Source/Tga.cpp)
static bool writing;

// uc_orig: init_convert (fallen/DDLibrary/Source/Tga.cpp)
static bool init_convert = false;

// Forward declarations.
// uc_orig: TGA_load_from_file (fallen/DDLibrary/Source/Tga.cpp)
TGA_Info TGA_load_from_file(const CBYTE* file, SLONG max_width, SLONG max_height, TGA_Pixel* data, BOOL bCanShrink = TRUE);

// uc_orig: TGA_make_conversion_tables (fallen/DDLibrary/Source/Tga.cpp)
static void TGA_make_conversion_tables(void);

// uc_orig: TGA_write_compressed (fallen/DDLibrary/Source/Tga.cpp)
static void TGA_write_compressed(const TGA_Info& ti, TGA_Pixel* data, ULONG id);

// uc_orig: TGA_read_compressed (fallen/DDLibrary/Source/Tga.cpp)
static TGA_Info TGA_read_compressed(TGA_Pixel* data, ULONG id, SLONG max_width, SLONG max_height);

// Open a texture clump archive. Initialises colour conversion tables on first call.
// uc_orig: OpenTGAClump (fallen/DDLibrary/Source/Tga.cpp)
void OpenTGAClump(const char* clumpfn, size_t maxid, bool readonly)
{
    if (!init_convert) {
        TGA_make_conversion_tables();
        init_convert = true;
    }

    delete tclump;
    tclump = NULL;

    writing = !readonly;

    tclump = new FileClump(clumpfn, maxid, readonly);
}

// Free the active texture clump.
// uc_orig: CloseTGAClump (fallen/DDLibrary/Source/Tga.cpp)
void CloseTGAClump()
{
    delete tclump;
    tclump = NULL;
}

// Return the active texture clump pointer.
// uc_orig: GetTGAClump (fallen/DDLibrary/Source/Tga.cpp)
FileClump* GetTGAClump()
{
    return tclump;
}

// Check whether a TGA exists. In write mode checks the filesystem; in read mode checks the clump.
// uc_orig: DoesTGAExist (fallen/DDLibrary/Source/Tga.cpp)
bool DoesTGAExist(const char* filename, ULONG id)
{
    if (writing) {
        FILE* fd = MF_Fopen(filename, "rb");
        if (fd) {
            MF_Fclose(fd);
            return true;
        }
        return false;
    } else {
        return tclump->Exists(id);
    }
}

// Dispatch TGA load: read directly from file, compress into clump, or read from clump.
// uc_orig: TGA_load (fallen/DDLibrary/Source/Tga.cpp)
TGA_Info TGA_load(const CBYTE* file, SLONG max_width, SLONG max_height, TGA_Pixel* data, ULONG id, BOOL bCanShrink)
{
    if (!tclump || (id == -1)) {
        // read directly from file
        return TGA_load_from_file(file, max_width, max_height, data, bCanShrink);
    }

    if (writing) {
        // read from file, then write compressed
        TGA_Info ti = TGA_load_from_file(file, max_width, max_height, data);
        TGA_write_compressed(ti, data, id);
        return ti;
    }

    // read compressed
    return TGA_read_compressed(data, id, max_width, max_height);
}

// Read a TGA directly from disk into data[]. Returns TGA_Info with valid=0 on failure.
// Supports 32-bit RGBA, 24-bit RGB, and 8-bit paletted images.
// The image is vertically flipped to match expected top-down orientation.
// uc_orig: TGA_load_from_file (fallen/DDLibrary/Source/Tga.cpp)
TGA_Info TGA_load_from_file(const CBYTE* file, SLONG max_width, SLONG max_height, TGA_Pixel* data, BOOL bCanShrink)
{

    SLONG i;
    SLONG x;
    SLONG y;
    SLONG y1;
    SLONG y2;

    SLONG tga_pixel_depth;
    SLONG tga_image_type;
    SLONG tga_id_length;

    SLONG tga_pal;

    UBYTE header[18];
    UBYTE definitely_no_alpha;

    FILE* fd;

    TGA_Info ans;
    UBYTE pal[256 * 3];

    fd = MF_Fopen(file, "rb");
    if (!fd) {
        ans.valid = 0;
        return ans;
    }

    if (fread(header, 1, 18, fd) != 18)
        goto file_error;

    tga_id_length = header[0x0];
    tga_pal = header[1];
    tga_image_type = header[0x2];
    tga_width = header[0xc] + header[0xd] * 256;
    tga_height = header[0xe] + header[0xf] * 256;
    tga_pixel_depth = header[0x10];

    ans.valid = 0;
    ans.width = tga_width;
    ans.height = tga_height;
    ans.contains_alpha = 0;

    if (tga_image_type != 2 && tga_pal != 1) {
        MF_Fclose(fd);
        return ans;
    }

    if (tga_pixel_depth != 32 && tga_pixel_depth != 24 && tga_pal != 1) {
        MF_Fclose(fd);
        return ans;
    }

    if (tga_width > max_width || tga_height > max_height) {
        MF_Fclose(fd);
        return ans;
    }

    ans.valid = 1;

    fseek(fd, tga_id_length, SEEK_CUR);

    if (header[0x1]) {
        UBYTE col;

        SLONG entries = header[5] + header[6] * 256;
        SLONG bitsperentry = header[7];
        ULONG length;

        length = (entries * bitsperentry + 7) >> 3;

        if (fread(&pal[0], 1, length, fd) != length)
            goto file_error;

        UBYTE* buffer = new UBYTE[tga_width * tga_height];
        if (fread(buffer, 1, tga_width * tga_height, fd) != tga_width * tga_height)
            goto file_error;
        UBYTE* bp = buffer;

        for (i = 0; i < tga_width * tga_height; i++) {
            col = *bp++;

            data[i].red = pal[col * 3 + 2];
            data[i].green = pal[col * 3 + 1];
            data[i].blue = pal[col * 3 + 0];
            data[i].alpha = 255;

            definitely_no_alpha = 1;
        }

        delete[] buffer;
    } else {

        if (tga_pixel_depth == 32) {
            if (fread(data, 1, sizeof(TGA_Pixel) * tga_width * tga_height, fd) != sizeof(TGA_Pixel) * tga_width * tga_height)
                goto file_error;

            definitely_no_alpha = 0;
        } else {
            // Load 24-bit pixels one at a time, inserting a full alpha channel.
            UBYTE* buffer = new UBYTE[3 * tga_width * tga_height];
            if (fread(buffer, 1, 3 * tga_width * tga_height, fd) != 3 * tga_width * tga_height)
                goto file_error;
            UBYTE* bp = buffer;

            for (i = 0; i < tga_width * tga_height; i++) {
                data[i].blue = *bp++;
                data[i].green = *bp++;
                data[i].red = *bp++;
                data[i].alpha = 255;
            }

            delete[] buffer;

            definitely_no_alpha = 1;
        }
    }

    MF_Fclose(fd);

    if (definitely_no_alpha == 0) {
        for (i = 0; i < tga_width * tga_height; i++) {
            if (data[i].alpha != 255) {
                ans.contains_alpha = 1;
                break;
            }
        }
    } else {
        ans.contains_alpha = 0;
    }

    // TGA files are stored bottom-up; flip vertically to get top-down order.
    TGA_Pixel spare;

    for (y = 0; y < tga_height / 2; y++) {
        y1 = y;
        y2 = tga_height - 1 - y;

        for (x = 0; x < tga_width; x++) {
            spare = data[x + y1 * tga_width];
            data[x + y1 * tga_width] = data[x + y2 * tga_width];
            data[x + y2 * tga_width] = spare;
        }
    }

    return ans;

file_error:;

    MF_Fclose(fd);
    ans.valid = 0;

    return ans;
}

// Write a 16-bit pixel buffer to the clump, applying bit-packing compression if it saves space.
// The buffer header (words 0-2) is always written verbatim; only pixel data (words 3+) is compressed.
// uc_orig: WriteSquished (fallen/DDLibrary/Source/Tga.cpp)
static void WriteSquished(UWORD* buffer, size_t nwords, ULONG id)
{
    UWORD used[65536];
    UWORD mapping[65536];

    memset(used, 0, 65536 * 2);

    UWORD total = 0;

    for (size_t ii = 3; ii < nwords; ii++) {
        if (!used[buffer[ii]]) {
            used[buffer[ii]] = total + 1;
            mapping[total] = buffer[ii];
            total++;
        }
    }

    int bits = 1;

    while (total > (1 << bits))
        bits++;

    int bits_required = bits * (nwords - 3) + (5 + total) * 16;
    int bits_normal = 16 * nwords;

    if (bits_required < bits_normal) {
        UWORD* squished = new UWORD[(bits_required + 15) / 16];
        UWORD* sptr = squished;

        *sptr++ = 0xFFFF; // marker for compressed file
        *sptr++ = buffer[0];
        *sptr++ = buffer[1];
        *sptr++ = buffer[2];
        *sptr++ = total;

        for (size_t ii = 0; ii < total; ii++) {
            *sptr++ = mapping[ii];
        }

        // Write pixel indices as a packed bit stream.
        // e.g. with 10-bit values A,B,C,D packed into 16-bit words:
        // 0xAAAAAAAAAABBBBBB 0xBBBBCCCCCCCCCCDD 0xDDDDDDDD00000000

        UWORD cword = 0; // current output word being filled
        UWORD cbits = 0; // number of valid bits accumulated in cword

        for (size_t ii = 3; ii < nwords; ii++) {
            UWORD encoded = used[buffer[ii]] - 1;

            if (cbits + bits > 16) {
                // fill the current word and flush it, carry remaining bits over
                cword <<= (16 - cbits);
                cword |= encoded >> (bits - (16 - cbits));
                *sptr++ = cword;
                cword = encoded;
                cbits = bits - (16 - cbits);
            } else {
                // append bits to the current word
                cword <<= bits;
                cword |= encoded;
                cbits += bits;
            }
        }

        if (cbits)
            *sptr++ = (cword << (16 - cbits));

        tclump->Write(squished, 2 * (sptr - squished), id);
        delete[] squished;
    } else {
        tclump->Write(buffer, 2 * nwords, id);
    }
}

// Read a compressed or uncompressed pixel buffer from the clump.
// Returns a heap-allocated buffer that must be freed by the caller.
// uc_orig: ReadSquished (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE* ReadSquished(ULONG id)
{
    UBYTE* buffer = tclump->Read(id);
    if (!buffer)
        return NULL;

    UWORD* bptr = (UWORD*)buffer;

    if (*bptr != 0xFFFF)
        return buffer; // not compressed, return as-is

    bptr++;

    // compute the uncompressed pixel count from the stored width and height
    size_t nwords = bptr[1] * bptr[2];

    UBYTE* output = new UBYTE[2 * (nwords + 3)];
    UWORD* optr = (UWORD*)output;

    // copy the three-word header (contains_alpha, width, height)
    *optr++ = *bptr++;
    *optr++ = *bptr++;
    *optr++ = *bptr++;

    // read the palette mapping table
    UWORD total = *bptr++;
    UWORD mapping[65536];

    for (size_t ii = 0; ii < total; ii++) {
        mapping[ii] = *bptr++;
    }

    int bits = 1;

    while (total > (1 << bits))
        bits++;

    // decode the packed bit stream
    int cbits = 16;
    UWORD cword = *bptr++;

    for (size_t ii = 0; ii < nwords; ii++) {
        UWORD encoded;

        if (cbits > bits) {
            // enough bits available in the current word
            encoded = cword >> (16 - bits);
            cword <<= bits;
            cbits -= bits;
        } else {
            // straddles word boundary; read remaining bits from next word
            int xbits = bits - cbits;
            encoded = cword >> (16 - bits);
            cword = *bptr++;
            cbits = 16;
            encoded |= (cword >> (16 - xbits));
            cword <<= xbits;
            cbits -= xbits;
        }

        *optr++ = mapping[encoded];
    }

    delete[] buffer;

    return output;
}

// Colour conversion lookup tables used when compressing pixels to 16-bit formats.
// uc_orig: C8to4 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C8to4[256];

// uc_orig: C8to5 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C8to5[256];

// uc_orig: C8to6 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C8to6[256];

// uc_orig: C4to8 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C4to8[16];

// uc_orig: C5to8 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C5to8[32];

// uc_orig: C6to8 (fallen/DDLibrary/Source/Tga.cpp)
static UBYTE C6to8[64];

// Populate the 8-bit <-> 4/5/6-bit conversion lookup tables.
// uc_orig: TGA_make_conversion_tables (fallen/DDLibrary/Source/Tga.cpp)
static void TGA_make_conversion_tables(void)
{
    int ii;

    for (ii = 0; ii < 256; ii++) {
        C8to4[ii] = (((ii * 30) / 256) + 1) / 2;
        C8to5[ii] = (((ii * 62) / 256) + 1) / 2;
        C8to6[ii] = (((ii * 126) / 256) + 1) / 2;
    }

    for (ii = 0; ii < 16; ii++)
        C4to8[ii] = (ii << 4) | ii;
    for (ii = 0; ii < 32; ii++)
        C5to8[ii] = (ii << 3) | (ii >> 2);
    for (ii = 0; ii < 64; ii++)
        C6to8[ii] = (ii << 2) | (ii >> 4);
}

// Quantise RGBA pixel data to 16-bit (4:4:4:4 or 5:6:5) and write it into the clump.
// Skips the write if the clump entry already exists.
// uc_orig: TGA_write_compressed (fallen/DDLibrary/Source/Tga.cpp)
static void TGA_write_compressed(const TGA_Info& ti, TGA_Pixel* data, ULONG id)
{
    if (!ti.valid)
        return;
    if (tclump->Exists(id))
        return;

    UWORD* buffer = new UWORD[ti.width * ti.height + 3];
    UWORD* bptr = buffer;

    *bptr++ = (UWORD)ti.contains_alpha;
    *bptr++ = (UWORD)ti.width;
    *bptr++ = (UWORD)ti.height;

    if (ti.contains_alpha) {
        // compress to 4:4:4:4
        for (int ii = 0; ii < ti.width * ti.height; ii++) {
            UWORD pix;

            pix = C8to4[data->blue];
            pix |= C8to4[data->green] << 4;
            pix |= C8to4[data->red] << 8;
            pix |= C8to4[data->alpha] << 12;

            *bptr++ = pix;
            data++;
        }
    } else {
        // compress to 5:6:5
        for (int ii = 0; ii < ti.width * ti.height; ii++) {
            UWORD pix;

            pix = C8to5[data->blue];
            pix |= C8to6[data->green] << 5;
            pix |= C8to5[data->red] << 11;

            *bptr++ = pix;
            data++;
        }
    }

    WriteSquished(buffer, ti.width * ti.height + 3, id);
    delete[] buffer;
}

// Decompress a clump entry back to 32-bit RGBA pixels.
// uc_orig: TGA_read_compressed (fallen/DDLibrary/Source/Tga.cpp)
static TGA_Info TGA_read_compressed(TGA_Pixel* data, ULONG id, SLONG max_width, SLONG max_height)
{
    TGA_Info ti;

    ti.valid = 0;

    if (!tclump->Exists(id))
        return ti;

    UBYTE* buffer = ReadSquished(id);
    if (!buffer)
        return ti;
    UWORD* bptr = (UWORD*)buffer;

    ti.valid = 1;
    ti.contains_alpha = *bptr++;
    ti.width = *bptr++;
    ti.height = *bptr++;

    if (ti.contains_alpha) {
        // expand from 4:4:4:4
        for (int ii = 0; ii < ti.width * ti.height; ii++) {
            UWORD pix = *bptr++;

            data->blue = C4to8[pix & 0xF];
            data->green = C4to8[(pix >> 4) & 0xF];
            data->red = C4to8[(pix >> 8) & 0xF];
            data->alpha = C4to8[(pix >> 12) & 0xF];
            data++;
        }
    } else {
        // expand from 5:6:5
        for (int ii = 0; ii < ti.width * ti.height; ii++) {
            UWORD pix = *bptr++;

            data->blue = C5to8[pix & 0x1F];
            data->green = C6to8[(pix >> 5) & 0x3F];
            data->red = C5to8[(pix >> 11) & 0x1F];
            data->alpha = 0;
            data++;
        }
    }

    delete[] buffer;
    return ti;
}

// Load a paletted TGA and remap its lower 4-bit palette indices using an external remap palette.
// uc_orig: TGA_load_remap (fallen/DDLibrary/Source/Tga.cpp)
TGA_Info TGA_load_remap(const CBYTE* file, const CBYTE* pname, SLONG max_width, SLONG max_height, TGA_Pixel* data)
{
    SLONG i;
    SLONG x;
    SLONG y;
    SLONG y1;
    SLONG y2;

    SLONG tga_pixel_depth;
    SLONG tga_image_type;
    SLONG tga_id_length;

    SLONG tga_pal;

    UBYTE header[18];
    UBYTE junk;
    UBYTE definitely_no_alpha;

    FILE *handle, *phandle;

    TGA_Info ans;
    UBYTE pal[256 * 3];
    UBYTE remap_pal[256 * 4];

    phandle = MF_Fopen(pname, "rb");
    if (phandle == NULL) {
        ans.valid = 0;
        return ans;
    }

    if (fread(&pal[0], 1, 24, phandle) != 24)
        goto file_error;

    if (fread(&remap_pal[0], 1, 256 * 4, phandle) != 256 * 4)
        goto file_error;

    MF_Fclose(phandle);

    {
        // If the upper 16 palette entries sum higher than the lower 16, shift upper into lower.
        SLONG pal1 = 0, pal2 = 0, c0;
        for (c0 = 0; c0 < 16 * 4; c0++)
            pal1 += remap_pal[c0];

        for (c0 = 240 * 4; c0 < 256 * 4; c0++)
            pal2 += remap_pal[c0];

        if (pal2 > pal1) {
            for (c0 = 0; c0 < 16 * 4; c0++) {
                remap_pal[c0] = remap_pal[240 * 4 + c0];
            }
        }
    }

    handle = MF_Fopen(file, "rb");

    if (handle == NULL) {
        ans.valid = 0;
        return ans;
    }

    if (fread(header, sizeof(UBYTE), 18, handle) != 18)
        goto file_error;

    tga_id_length = header[0x0];
    tga_pal = header[1];
    tga_image_type = header[0x2];
    tga_width = header[0xc] + header[0xd] * 256;
    tga_height = header[0xe] + header[0xf] * 256;
    tga_pixel_depth = header[0x10];

    ans.valid = 0;
    ans.width = tga_width;
    ans.height = tga_height;
    ans.contains_alpha = 0;

    if (tga_pal != 1) {
        MF_Fclose(handle);
        return ans;
    }

    if (tga_width > max_width || tga_height > max_height) {
        MF_Fclose(handle);
        return ans;
    }

    ans.valid = 1;

    for (i = 0; i < tga_id_length; i++) {
        if (fread(&junk, sizeof(UBYTE), 1, handle) != 1)
            goto file_error;
    }

    if (header[0x1]) {
        UBYTE col;

        SLONG entries = header[5] + header[6] * 256;
        SLONG bitsperentry = header[7];
        SLONG length;

        length = (entries * bitsperentry + 7) >> 3;

        {
            if (fread(&pal[0], sizeof(UBYTE), length, handle) != length)
                goto file_error;
        }
        for (i = 0; i < tga_width * tga_height; i++) {
            if (fread(&col, sizeof(UBYTE), 1, handle) != 1)
                goto file_error;
            col = 255 - col;

            col &= 15;

            data[i].red = remap_pal[col * 4 + 0];
            data[i].green = remap_pal[col * 4 + 1];
            data[i].blue = remap_pal[col * 4 + 2];
            data[i].alpha = 255;
        }
        definitely_no_alpha = 1;
    }

    MF_Fclose(handle);

    {
        ans.contains_alpha = 0;
    }

    // TGA files are stored bottom-up; flip vertically to get top-down order.
    TGA_Pixel spare;

    for (y = 0; y < tga_height / 2; y++) {
        y1 = y;
        y2 = tga_height - 1 - y;

        for (x = 0; x < tga_width; x++) {
            spare = data[x + y1 * tga_width];
            data[x + y1 * tga_width] = data[x + y2 * tga_width];
            data[x + y2 * tga_width] = spare;
        }
    }

    return ans;

file_error:;

    MF_Fclose(handle);
    ans.valid = 0;

    return ans;
}

// Write pixel data to a TGA file. Rows are written bottom-up as required by the TGA spec.
// Pass contains_alpha=FALSE to save as 24-bit RGB (no alpha channel).
// uc_orig: TGA_save (fallen/DDLibrary/Source/Tga.cpp)
void TGA_save(
    const CBYTE* file,
    SLONG width,
    SLONG height,
    TGA_Pixel* data,
    SLONG contains_alpha)
{
    SLONG x;
    SLONG y;

    UBYTE header[18];

    FILE* handle;

    handle = fopen(file, "wb");

    if (handle == NULL) {
        return;
    }

    SLONG i;

    for (i = 0; i < 18; i++) {
        header[i] = TGA_header[i];
    }

    header[0xc] = width & 0xff;
    header[0xd] = width >> 8;
    header[0xe] = height & 0xff;
    header[0xf] = height >> 8;

    header[0x10] = (contains_alpha) ? 32 : 24;

    fwrite(&header, sizeof(UBYTE), 18, handle);

    // Write rows from bottom to top to match the TGA bottom-up convention.
    for (y = height - 1; y >= 0; y--)
        for (x = 0; x < width; x++) {
            if (contains_alpha) {
                fwrite(&data[x + y * width].alpha, sizeof(UBYTE), 1, handle);
            }

            fwrite(&data[x + y * width].blue, sizeof(UBYTE), 1, handle);
            fwrite(&data[x + y * width].green, sizeof(UBYTE), 1, handle);
            fwrite(&data[x + y * width].red, sizeof(UBYTE), 1, handle);
        }

    MF_Fclose(handle);
}
