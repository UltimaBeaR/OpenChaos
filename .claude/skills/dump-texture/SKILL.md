---
name: dump-texture
description: >
  Dump and visualize game resource images (textures from clump/TGA).
  Use when you need to see what a texture actually looks like — debug rendering
  bugs, check alpha channels, inspect atlas layouts, compare OpenGL vs D3D6
  output, verify texture loading. Trigger when: investigating visual glitches
  (wrong colors, missing alpha, ghost pixels), the user says "покажи текстуру",
  "что за текстура", "dump texture", or when you need to inspect pixel data
  to understand a rendering issue. Also useful when comparing how a texture
  appears in-game vs what's stored in the clump file.
---

# Dump Texture — debug visualization of GPU textures

## How to dump a texture

### 1. Add temporary dump code

Insert in `backend_opengl/game/core.cpp` BEFORE the `gl_upload_texture(tex, pixels, ...)` call in `gl_load_tga()`:

```cpp
// DEBUG: dump texture as raw BGRA file.
if (tex.file_id == PAGE_NUMBER && load_w == EXPECTED_SIZE) {
    FILE* f = fopen("TEXNAME_dump.raw", "wb");
    if (f) { fwrite(pixels, 4, count, f); fclose(f); }
    fprintf(stderr, "[DEBUG] Dumped %s %dx%d\n", tex.name, load_w, load_h);
}
```

Replace `PAGE_NUMBER` with the texture page ID. The page ID = `TEXTURE_NUM_STANDARD (1408) + offset`, where offset comes from texture.cpp constants.

### 2. Build, run, close game

```bash
make build-debug
```
The .raw file appears next to the executable.

### 3. Convert to PNG with Python

```python
from PIL import Image
with open('TEXNAME_dump.raw', 'rb') as f: data = f.read()
W = H = 256  # adjust to actual size
px = [(data[i+2], data[i+1], data[i], data[i+3]) for i in range(0, len(data), 4)]
for mode, name, sel in [('RGB','rgb',lambda p:(p[0],p[1],p[2])), ('L','alpha',lambda p:p[3]), ('RGBA','rgba',lambda p:p)]:
    img = Image.new(mode, (W,H)); img.putdata([sel(p) for p in px]); img.save(f'tex_{name}.png')
```

### 4. View the PNGs

Use `Read` tool on the PNG files — Claude can see images directly.

- **tex_rgb.png** — RGB without alpha. Shows "ghost" pixels invisible with alpha blend but visible with additive blend.
- **tex_alpha.png** — Alpha channel only. White = opaque, black = transparent.
- **tex_rgba.png** — Full RGBA composite.

### 5. Clean up

Remove debug dump code after investigation. Don't commit it.

## Quick alpha stats (without full dump)

Add to `gl_load_tga()` after pixel data is loaded:

```cpp
{
    int32_t a0 = 0, a255 = 0, aother = 0, a0_rgb_nz = 0;
    uint8_t mr = 0, mg = 0, mb = 0;
    for (int32_t i = 0; i < count; i++) {
        uint8_t a = pixels[i].alpha;
        if (a == 0) {
            a0++;
            if (pixels[i].red | pixels[i].green | pixels[i].blue) {
                a0_rgb_nz++;
                if (pixels[i].red > mr) mr = pixels[i].red;
                if (pixels[i].green > mg) mg = pixels[i].green;
                if (pixels[i].blue > mb) mb = pixels[i].blue;
            }
        } else if (a == 255) a255++; else aother++;
    }
    fprintf(stderr, "[TEX] page=%d '%s' %dx%d alpha=%s a0=%d(ghost=%d max=%d/%d/%d) a255=%d other=%d\n",
            tex.file_id, tex.name, load_w, load_h,
            load_alpha?"Y":"N", a0, a0_rgb_nz, mr, mg, mb, a255, aother);
}
```

## Known texture page IDs

| Texture | Page | File |
|---------|------|------|
| TEXTURE_page_font2d | 1425 | multifontPC.tga |
| TEXTURE_page_lcdfont | 1458 | olyfont2.tga |
| TEXTURE_page_lastpanel | 1473 | PCdisplay.tga |
| TEXTURE_page_lastpanel2 | 1477 | PCdisplay01.tga |
| TEXTURE_page_ic | 1454 | (from clump) |

Formula: `TEXTURE_NUM_STANDARD (1408) + offset` (offset from texture.cpp).
