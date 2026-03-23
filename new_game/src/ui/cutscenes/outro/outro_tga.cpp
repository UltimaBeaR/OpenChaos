#include "ui/cutscenes/outro/outro_tga.h"

// uc_orig: OUTRO_TGA_FileLoad_Error (fallen/outro/OutroTga.cpp)
OUTRO_TGA_Info OUTRO_TGA_FileLoad_Error(OUTRO_TGA_Info& ans, FILE*& handle, const CBYTE*& file)
{
    fclose(handle);
    ans.valid = UC_FALSE;
    return ans;
}

// uc_orig: OUTRO_TGA_load (fallen/outro/OutroTga.cpp)
OUTRO_TGA_Info OUTRO_TGA_load(
    const CBYTE* file,
    SLONG max_width,
    SLONG max_height,
    OUTRO_TGA_Pixel* data)
{
    SLONG i;

    UBYTE red;
    UBYTE green;
    UBYTE blue;

    SLONG tga_width;
    SLONG tga_height;
    SLONG tga_pixel_depth;
    SLONG tga_image_type;
    SLONG tga_id_length;

    UBYTE header[18];
    UBYTE rubbish;
    UBYTE no_alpha;

    FILE* handle;

    OUTRO_TGA_Info ans {};

    handle = fopen(file, "rb");

    if (handle == NULL) {
        ans.valid = UC_FALSE;
        return ans;
    }

    if (fread(header, sizeof(UBYTE), 18, handle) != 18)
        return OUTRO_TGA_FileLoad_Error(ans, handle, file);

    tga_id_length = header[0x0];
    tga_image_type = header[0x2];
    tga_width = header[0xc] + header[0xd] * 256;
    tga_height = header[0xe] + header[0xf] * 256;
    tga_pixel_depth = header[0x10];

    ans.valid = UC_FALSE;
    ans.width = tga_width;
    ans.height = tga_height;
    ans.flag = 0;

    if (tga_image_type != 2) {
        fclose(handle);
        return ans;
    }

    if (tga_pixel_depth != 32 && tga_pixel_depth != 24) {
        fclose(handle);
        return ans;
    }

    if (tga_width > max_width || tga_height > max_height) {
        fclose(handle);
        return ans;
    }

    ans.valid = UC_TRUE;

    // Skip past the image identification field.
    for (i = 0; i < tga_id_length; i++) {
        if (fread(&rubbish, sizeof(UBYTE), 1, handle) != 1)
            return OUTRO_TGA_FileLoad_Error(ans, handle, file);
    }

    if (tga_pixel_depth == 32) {
        if (fread(data, sizeof(OUTRO_TGA_Pixel), tga_width * tga_height, handle) != tga_width * tga_height)
            return OUTRO_TGA_FileLoad_Error(ans, handle, file);

        no_alpha = UC_FALSE;
    } else {
        // 24-bit: load pixel-by-pixel, adding a null alpha channel.
        for (i = 0; i < tga_width * tga_height; i++) {
            if (fread(&blue, sizeof(UBYTE), 1, handle) != 1)
                return OUTRO_TGA_FileLoad_Error(ans, handle, file);
            if (fread(&green, sizeof(UBYTE), 1, handle) != 1)
                return OUTRO_TGA_FileLoad_Error(ans, handle, file);
            if (fread(&red, sizeof(UBYTE), 1, handle) != 1)
                return OUTRO_TGA_FileLoad_Error(ans, handle, file);

            data[i].red = red;
            data[i].green = green;
            data[i].blue = blue;
            data[i].alpha = 255;
        }

        no_alpha = UC_TRUE;
    }

    fclose(handle);

    if (!no_alpha) {
        ans.flag |= TGA_FLAG_ONE_BIT_ALPHA;

        for (i = 0; i < tga_width * tga_height; i++) {
            if (data[i].alpha != 255) {
                ans.flag |= TGA_FLAG_CONTAINS_ALPHA;

                if (ans.flag != 0) {
                    ans.flag &= ~TGA_FLAG_ONE_BIT_ALPHA;
                    break;
                }
            }
        }

        if (!(ans.flag & TGA_FLAG_CONTAINS_ALPHA)) {
            ans.flag &= ~TGA_FLAG_ONE_BIT_ALPHA;
        }
    }

    // Detect grayscale: all pixels satisfy r == g == b.
    ans.flag |= TGA_FLAG_GRAYSCALE;

    for (i = 0; i < tga_width * tga_height; i++) {
        if (data[i].red != data[i].green || data[i].red != data[i].blue || data[i].green != data[i].blue) {
            ans.flag &= ~TGA_FLAG_GRAYSCALE;
            break;
        }
    }

    return ans;
}
