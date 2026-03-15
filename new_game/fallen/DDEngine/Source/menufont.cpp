/************************************************************
 *
 *   menufont.cpp
 *   2D proportional-font text writer with poncey afterfx
 *
 */

#include "MenuFont.h"
#include "tga.h"
#include "poly.h"
#include "..\headers\noserver.h"



CharData FontInfo[256];
CBYTE FontName[_MAX_PATH];
SLONG FontPage;
SLONG FONT_TICK = 0;

inline BOOL Red(SLONG ofs, TGA_Pixel* data)
{
    return ((data[ofs].red > 200) && (!data[ofs].blue) && (!data[ofs].green));
}
inline BOOL Bloo(SLONG ofs, TGA_Pixel* data)
{
    return ((data[ofs].blue > 200) && (!data[ofs].red) && (!data[ofs].green));
}

inline BOOL Mata(SLONG ofs, TGA_Pixel* data)
{
    return (Red(ofs, data) && Red(ofs + 1, data) && (Red(ofs + 256, data)));
}

void MENUFONT_Page(SLONG page)
{
    FontPage = page;
}

//
// oh fuck
//
#ifdef NO_SERVER
#define TEXTURE_EXTRA_DIR "server\\textures\\extras\\"
#else
#define TEXTURE_EXTRA_DIR "u:\\urbanchaos\\textures\\extras\\"

#endif

/*void MENUFONT_Load(CBYTE *fn, SLONG page, CBYTE *fontlist) {
  TGA_Pixel *temp;
  UBYTE *pt;
  UWORD x,y,ox,oy,ofs, yofs;
  CBYTE tmp[_MAX_PATH];

  if (!stricmp(fn,FontName)) return;
  strcpy(FontName,fn);
  FontPage=page;

  // scan the tga...
  temp = new TGA_Pixel[256*256];
  strcpy(tmp,TEXTURE_EXTRA_DIR);
  strcat(tmp,fn);
  TGA_load(tmp,256,256,temp,-1);

  pt=(UBYTE*)fontlist;

  ZeroMemory(FontInfo,sizeof(FontInfo));

  for (y=0;y<255;y++)
        for (x=0;x<255;x++)
          if (Mata(ofs=x|(y<<8),temp)) {
                FontInfo[*pt].x=ox=x+1;
                FontInfo[*pt].y=oy=y+1;
                ofs+=257; yofs=0;
                while (!Red(ofs+1,temp)) {
                        ofs++; ox++;
                }
                FontInfo[*pt].ox=ox;
                while (!Red(ofs+256,temp)) {
                        ofs+=256; oy++;
                        if (Bloo(ofs+1,temp)) {
                                FontInfo[*pt].yofs=yofs;
                        }
                        yofs++;
                }
                FontInfo[*pt].oy=oy;

                // extra border space -- avoid redness
                FontInfo[*pt].x++;
                FontInfo[*pt].y++;
//		FontInfo[*pt].ox--;
//		FontInfo[*pt].oy--;
//		FontInfo[*pt].ox++;
//		FontInfo[*pt].oy++;


//		FontInfo[*pt].width =ox-(x+1);
//		FontInfo[*pt].height=oy-(y+1);
                FontInfo[*pt].width =ox-(x-1);
                FontInfo[*pt].height=oy-(y);

                FontInfo[*pt].x/=256.0;
                FontInfo[*pt].y/=256.0;
                FontInfo[*pt].ox/=256.0;
                FontInfo[*pt].oy/=256.0;
                pt++;
          }

  delete [] temp;

  if (!FontInfo[32].width) FontInfo[32].width=FontInfo[65].width;


}
*/
void MENUFONT_Load(CBYTE* fn, SLONG page, CBYTE* fontlist)
{
    TGA_Pixel* temp;
    UBYTE* pt;
    UWORD x, y, ox, oy, ofs, yofs;
    CBYTE tmp[_MAX_PATH];

    if (!stricmp(fn, FontName))
        return;
    strcpy(FontName, fn);
    FontPage = page;

    // scan the tga...
    temp = new TGA_Pixel[256 * 256];
    ASSERT(temp != NULL);
    strcpy(tmp, TEXTURE_EXTRA_DIR);
    strcat(tmp, fn);
    TGA_load(tmp, 256, 256, temp, -1, FALSE);

    pt = (UBYTE*)fontlist;

    ZeroMemory(FontInfo, sizeof(FontInfo));

    for (y = 0; y < 255; y++)
        for (x = 0; x < 255; x++)
            if (Mata(ofs = x | (y << 8), temp)) {
                FontInfo[*pt].x = ox = x + 1;
                FontInfo[*pt].y = oy = y + 1;
                ofs += 257;
                yofs = 0;
                while (!Red(ofs + 1, temp)) {
                    ofs++;
                    ox++;
                }
                FontInfo[*pt].ox = ox;
                while (!Red(ofs + 256, temp)) {
                    ofs += 256;
                    oy++;
                    yofs++;
                }
                FontInfo[*pt].oy = oy;

                FontInfo[*pt].width = ox - (x - 1);
                FontInfo[*pt].height = oy - (y);

                FontInfo[*pt].x /= 256.0f;
                FontInfo[*pt].y /= 256.0f;
                FontInfo[*pt].ox /= 256.0f;
                FontInfo[*pt].oy /= 256.0f;
                pt++;
                if (!*pt)
                    break;
            }

    delete[] temp;

    if (!FontInfo[32].width)
        FontInfo[32].width = FontInfo[65].width;
}


#define SC(a) (SIN(a & 2047) >> 15)
#define CC(a) (COS(a & 2047) >> 15)

void MENUFONT_DrawFlanged(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG alpha, SLONG rgb, UBYTE flags)
{
    SLONG width = 0, height, c0, len = strlen(msg), i;
    UBYTE* pt;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };

    pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xff000000;
    pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = rgb | (alpha << 24);
    pp[0].Z = pp[1].Z = pp[2].Z = pp[3].Z = 0.5f;

    alpha >>= 2;

    pt = (UBYTE*)msg;
    for (c0 = 0; c0 < len; c0++) {
        width = (FontInfo[*pt].width * scale) >> 8;
        height = (FontInfo[*pt].height * scale) >> 8;

        if ((width > 0) && (height > 0)) {

            pp[0].u = FontInfo[*pt].x;
            pp[0].v = FontInfo[*pt].y;
            pp[1].u = FontInfo[*pt].ox;
            pp[1].v = FontInfo[*pt].y;
            pp[2].u = FontInfo[*pt].x;
            pp[2].v = FontInfo[*pt].oy;
            pp[3].u = FontInfo[*pt].ox;
            pp[3].v = FontInfo[*pt].oy;

            if (flags & MENUFONT_SINED)
                for (i = 0; i < 4; i++) {
                    pp[0].X = x;
                    pp[0].Y = y + (SIN(((x + FONT_TICK) << i) & 2047) >> 13);
                    pp[1].X = x + width;
                    pp[1].Y = y + (SIN(((x + width + FONT_TICK) << i) & 2047) >> 13);
                    pp[2].X = x;
                    pp[2].Y = y + height;
                    pp[3].X = x + width;
                    pp[3].Y = y + height;
                    POLY_add_quad(quad, FontPage, FALSE, TRUE);
                }
            else
                for (i = 1; i < 4; i++) {
                    pp[0].X = x + SC((x + (i * 128) + FONT_TICK) << 2);
                    pp[0].Y = y + SC((x + (i * 194) + FONT_TICK) << 2);
                    pp[1].X = x + width + CC((x + width + FONT_TICK) << 2);
                    pp[1].Y = y + SC((x + width + FONT_TICK) << 2);
                    pp[2].X = x + CC((x + (i * 128) + FONT_TICK) << 2);
                    pp[2].Y = y + height + SC((x + (i * 194) + FONT_TICK) << 2);
                    pp[3].X = x + width + CC((x + width - (i * 128) + FONT_TICK) << 2);
                    pp[3].Y = y + height + CC((x + width + FONT_TICK) << 2);
                    POLY_add_quad(quad, FontPage, FALSE, TRUE);
                }
        }

        pt++;
        x += width - 1;
    }
}

void MENUFONT_DrawFutzed(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG alpha, SLONG rgb, UBYTE flags)
{
    SLONG width = 0, height, c0, len = strlen(msg), i, j, k;
    UBYTE* pt;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    float uys, ys, uyc, yc;

    pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xff000000;
    pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = rgb | (alpha << 24);
    pp[0].Z = pp[1].Z = pp[2].Z = pp[3].Z = 0.5f;

    j = (!(rand() & 0x1f));

    pt = (UBYTE*)msg;
    for (c0 = 0; c0 < len; c0++) {
        width = (FontInfo[*pt].width * scale) >> 8;
        height = (FontInfo[*pt].height * scale) >> 8;

        if ((width > 0) && (height > 0)) {

            pp[0].u = FontInfo[*pt].x;
            pp[1].u = FontInfo[*pt].ox;
            pp[2].u = FontInfo[*pt].x;
            pp[3].u = FontInfo[*pt].ox;
            pp[0].X = x;
            pp[1].X = x + width;
            pp[2].X = x;
            pp[3].X = x + width;

            uys = FontInfo[*pt].oy - FontInfo[*pt].y;
            uys *= 0.125f; // 1/8               0.0625; // 1/16
            ys = height * 0.125f;

            uyc = FontInfo[*pt].y;
            yc = y;

            //			j=SIN(TICK<<3)>>8;
            //			j=(!(rand()&0xff));

            for (i = 0; i < 8; i++) {

                //				if (abs(i-j)<8)
                //				if (!(rand()%0xff))
                if (j) {
                    //					k=SIN(((i-j)<<3)&2047)>>14;
                    k = rand() % 7;
                    pp[0].X = x + k;
                    pp[1].X = x + width + k;
                    //					k=SIN(((i+1-j)<<3)&2047)>>14;
                    k = rand() % 7;
                    pp[2].X = x + k;
                    pp[3].X = x + width + k;
                }

                pp[0].v = pp[1].v = uyc;
                uyc += uys;
                pp[2].v = pp[3].v = uyc;
                pp[0].Y = pp[1].Y = yc;
                yc += ys;
                pp[2].Y = pp[3].Y = yc;
                POLY_add_quad(quad, FontPage, FALSE, TRUE);
            }
        }

        pt++;
        x += width - 1;
    }
}



void MENUFONT_Draw(SWORD x, SWORD y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags, SWORD max)
{
    const UBYTE haloscale = 3;
    SLONG width = 0, height, c0, len = strlen(msg);
    // SLONG col;
    UBYTE* pt;
    //	float fscale=(float)scale/256.0;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    UBYTE hchar = (flags & MENUFONT_SUPER_YCTR) ? (UBYTE)*msg : 'M';
    SLONG yofs;

    if (max == -1)
        max = len;



    y -= (FontInfo[hchar].height * scale) >> 9;

    if (flags & (MENUFONT_CENTRED | MENUFONT_RIGHTALIGN)) {
        pt = (UBYTE*)msg;
        for (c0 = 0; c0 < len; c0++) {
            {
                width += ((FontInfo[*(pt++)].width - 1) * scale) >> 8;
            }
        }
        x -= (flags & MENUFONT_CENTRED) ? width >> 1 : width;
    }

    {

        SLONG a = (rgb >> 24) & 0xff;
        SLONG r = (rgb >> 16) & 0xff;
        SLONG g = (rgb >> 8) & 0xff;
        SLONG b = (rgb >> 0) & 0xff;

        r = r * a >> 8;
        g = g * a >> 8;
        b = b * a >> 8;

        rgb = (r << 16) | (g << 8) | (b << 0);
    }

    ASSERT((flags & (MENUFONT_GLIMMER | MENUFONT_SHAKE | MENUFONT_FUTZING | MENUFONT_FLANGED | MENUFONT_SINED | MENUFONT_HALOED)) == 0);

    pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xff000000;
    pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = rgb;
    pp[0].Z = pp[1].Z = pp[2].Z = pp[3].Z = 0.5f;

    pt = (UBYTE*)msg;
    for (c0 = 0; c0 < max; c0++) {

        {
            width = (FontInfo[*pt].width * scale) >> 8;
            height = (FontInfo[*pt].height * scale) >> 8;

            yofs = (FontInfo[*pt].yofs * scale) >> 8;
            y += yofs;

            if ((width > 0) && (height > 0)) {

                pp[0].u = FontInfo[*pt].x;
                pp[0].v = FontInfo[*pt].y;
                pp[1].u = FontInfo[*pt].ox;
                pp[1].v = FontInfo[*pt].y;
                pp[2].u = FontInfo[*pt].x;
                pp[2].v = FontInfo[*pt].oy;
                pp[3].u = FontInfo[*pt].ox;
                pp[3].v = FontInfo[*pt].oy;

                if (!(flags & MENUFONT_ONLY)) {
                    pp[0].X = x;
                    pp[0].Y = y;
                    pp[1].X = x + width;
                    pp[1].Y = y;
                    pp[2].X = x;
                    pp[2].Y = y + height;
                    pp[3].X = x + width;
                    pp[3].Y = y + height;

                    POLY_add_quad(quad, FontPage, FALSE, TRUE);
                }
            }

            y -= yofs;

            pt++;
            x += width - 1;
        }
    }
}

// I don't want to have to code a VMU->VM skipping thing for every rout,
// so here's a checker.
#ifdef DEBUG
#define CHECKTHEREARENOVMUSINHERE(string) ASSERT(strstr(string, "VMU") == NULL)
#else
#define CHECKTHEREARENOVMUSINHERE(string) sizeof(string)
#endif

void MENUFONT_Draw_floats(float x, float y, UWORD scale, CBYTE* msg, SLONG rgb, UWORD flags)
{
    const UBYTE haloscale = 3;
    SLONG c0, len = strlen(msg);
    // SLONG col;
    float width = 0, height;
    UBYTE* pt;
    float fscale = (float)scale / 256.0f;
    POLY_Point pp[4];
    POLY_Point* quad[4] = { &pp[0], &pp[1], &pp[2], &pp[3] };
    UBYTE hchar = (flags & MENUFONT_SUPER_YCTR) ? (UBYTE)*msg : 'M';

    CHECKTHEREARENOVMUSINHERE(msg);

    if (flags & MENUFONT_HSCALEONLY) {
        y -= FontInfo[hchar].height * 0.5f;
    } else {
        y -= (FontInfo[hchar].height * fscale) * 0.5f;
    }

    if (flags & (MENUFONT_CENTRED | MENUFONT_RIGHTALIGN)) {
        pt = (UBYTE*)msg;
        for (c0 = 0; c0 < len; c0++)
            width += ((float)FontInfo[*(pt++)].width - 1) * fscale;
        x -= (flags & MENUFONT_CENTRED) ? width * 0.5f : width;
    }

    {
        SLONG a = (rgb >> 24) & 0xff;
        SLONG r = (rgb >> 16) & 0xff;
        SLONG g = (rgb >> 8) & 0xff;
        SLONG b = (rgb >> 0) & 0xff;

        r = r * a >> 8;
        g = g * a >> 8;
        b = b * a >> 8;

        rgb = (r << 16) | (g << 8) | (b << 0);
    }

    pp[0].specular = pp[1].specular = pp[2].specular = pp[3].specular = 0xff000000;
    pp[0].colour = pp[1].colour = pp[2].colour = pp[3].colour = rgb;
    pp[0].Z = pp[1].Z = pp[2].Z = pp[3].Z = 0.5f;

    pt = (UBYTE*)msg;
    for (c0 = 0; c0 < len; c0++) {
        width = ((float)FontInfo[*pt].width) * fscale;
        if (flags & MENUFONT_HSCALEONLY) {
            height = ((float)FontInfo[*pt].height);
        } else {
            height = ((float)FontInfo[*pt].height) * fscale;
        }

        y += FontInfo[*pt].yofs;

        if ((width > 0) && (height > 0)) {

            pp[0].u = FontInfo[*pt].x;
            pp[0].v = FontInfo[*pt].y;
            pp[1].u = FontInfo[*pt].ox;
            pp[1].v = FontInfo[*pt].y;
            pp[2].u = FontInfo[*pt].x;
            pp[2].v = FontInfo[*pt].oy;
            pp[3].u = FontInfo[*pt].ox;
            pp[3].v = FontInfo[*pt].oy;

            if (!(flags & MENUFONT_ONLY)) {
                pp[0].X = x;
                pp[0].Y = y;
                pp[1].X = x + width;
                pp[1].Y = y;
                pp[2].X = x;
                pp[2].Y = y + height;
                pp[3].X = x + width;
                pp[3].Y = y + height;

                POLY_add_quad(quad, FontPage, FALSE, TRUE);
            }
        }

        y -= FontInfo[*pt].yofs;

        pt++;
        x += width - 1;
    }
}

void MENUFONT_Dimensions(CBYTE* fn, SLONG& x, SLONG& y, SWORD max, SWORD scale)
{
    UBYTE* fn2 = (UBYTE*)fn;


    if (!fn2[1]) {
        x = FontInfo[*fn2].width;
        y = FontInfo[*fn2].height;
        x *= scale;
        x >>= 8;
        y *= scale;
        y >>= 8;
        return;
    }
    x = 0;
    y = FontInfo[*fn2].height;
    while (max && *fn2) {

        {
            if (FontInfo[*fn2].height > y)
                y = FontInfo[*fn2].height;
            x += FontInfo[*fn2].width - 1;
            fn2++;
            max--;
        }
    }
    x *= scale;
    x >>= 8;
    y *= scale;
    y >>= 8;
}

SLONG MENUFONT_CharWidth(CBYTE fn, UWORD scale)
{
    return (FontInfo[(UBYTE)fn].width * scale) >> 8;
}

SLONG MENUFONT_CharFit(CBYTE* fn, SLONG x, UWORD scale)
{
    SLONG ctr = 0, width = 0;
    UBYTE* fn2 = (UBYTE*)fn;

    if (!*fn)
        return 0;

    while (*fn2 && (width < x)) {
        width += ((FontInfo[*(fn2++)].width * scale) >> 8) - 1;
        ctr++;
    }
    if (width > x)
        ctr--;
    return ctr;
}

void MENUFONT_Free()
{
    FontName[0] = 0;
}

void MENUFONT_MergeLower()
{
    UBYTE c;

    for (c = 'a'; c <= 'z'; c++) {
        FontInfo[c] = FontInfo[c - 32];
    }
    /*	for (c='à';c<='ý';c++) {
              FontInfo[c]=FontInfo[c-32];
            }*/
    for (c = 224; c <= 252; c++) {
        FontInfo[c] = FontInfo[c - 32];
    }
}

// ========================================================
//
// Cool line-fade-in text.
//
// ========================================================

//
// The line where we fade in.
//

float MENUFONT_fadein_x;

//
// The width of the fading in line.
//

#define MENUFONT_FADEIN_LEFT 256
#define MENUFONT_FADEIN_RIGHT 16

void MENUFONT_fadein_init()
{
    MENUFONT_fadein_x = 640.0F;
}

void MENUFONT_fadein_line(SLONG x)
{
    MENUFONT_fadein_x = float(x) * (1.0F / 256.0F);
}

//
// Draws a single character to the left of the fadein line. Returns
// the width of the character.
//

float MENUFONT_fadein_char(float x, float y, UBYTE ch, UBYTE fade)
{
    CharData* cd = &FontInfo[ch];

    if (x < 0 || x + cd->width >= 640.0F || y < 0 || y + cd->height >= 480.0F) {
        //
        // No clipping allowed.
        //

        return cd->width;
    }

    void PANEL_draw_quad(
        float left,
        float top,
        float right,
        float bottom,
        SLONG page,
        ULONG colour = 0xffffffff,
        float u1 = 0.0F,
        float v1 = 0.0F,
        float u2 = 1.0F,
        float v2 = 1.0F);

    if (x + cd->width < MENUFONT_fadein_x - MENUFONT_FADEIN_LEFT) {
        //
        // This character is totally to the left of the fadein line.
        //

        PANEL_draw_quad(
            x, y,
            x + cd->width,
            y + cd->height,
            FontPage,
            (fade << 0) | (fade << 8) | (fade << 16) | (fade << 24),
            cd->x,
            cd->y,
            cd->ox,
            cd->oy);
    } else if (x > MENUFONT_fadein_x + MENUFONT_FADEIN_RIGHT) {
        //
        // Totally to the right of the fadein line.
        //
    } else {
        //
        // Crosses the fadein line.
        //

#define MENUFONT_FADEIN_SEGMENTS 16

        SLONG i;

        float x1;
        float y1;
        float x2;
        float y2;

        float u1;
        float v1;
        float u2;
        float v2;

        float mx;
        float my;

        float expand;

        SLONG bright;
        SLONG colour;

        for (i = 0; i < MENUFONT_FADEIN_SEGMENTS; i++) {
            x1 = x + float(i * cd->width) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            y1 = y;
            x2 = x1 + float(cd->width) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            y2 = y1 + float(cd->height);

            u1 = cd->x + float(i * (cd->ox - cd->x)) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            v1 = cd->y;
            u2 = u1 + float(cd->ox - cd->x) * (1.0F / MENUFONT_FADEIN_SEGMENTS);
            v2 = cd->oy;

            //
            // Expand this segment a bit...
            //

#define MENUFONT_FADEIN_MAX_EXPAND 16.0F

            if (x1 > MENUFONT_fadein_x) {
                expand = 1.0F - (x1 - MENUFONT_fadein_x) * (1.0F / MENUFONT_FADEIN_RIGHT);
                expand *= expand;
                expand *= expand;
                expand *= expand;
                bright = expand * 16.0F;

                expand *= MENUFONT_FADEIN_MAX_EXPAND;

                SATURATE(bright, 0, 16);
            } else {
                expand = 1.0F - (MENUFONT_fadein_x - x1) * (1.0F / MENUFONT_FADEIN_LEFT);
                bright = 255 - expand * 224;
                expand *= expand;
                expand *= expand;
                expand *= expand;

                expand *= MENUFONT_FADEIN_MAX_EXPAND;
            }

            SATURATE(expand, 0.0F, MENUFONT_FADEIN_MAX_EXPAND);

            mx = (x1 + x2) * 0.5F;
            my = (y1 + y2) * 0.5F;

            x1 = x1 + (x1 - mx) * expand * 0.5F;
            y1 = y1 + (y1 - my) * expand;
            x2 = x2 + (x2 - mx) * expand * 0.25F;
            y2 = y2 + (y2 - my) * expand;

            SATURATE(bright, 0, 255);

            colour = bright * fade >> 8;
            colour |= colour << 8;
            colour |= colour << 8;
            colour |= colour << 8;

            PANEL_draw_quad(
                x1, y1,
                x2, y2,
                FontPage,
                colour,
                u1, v1,
                u2, v2);
        }
    }

    return cd->width;
}

//
// Draws centred text.
//

void MENUFONT_fadein_draw(SLONG x, SLONG y, UBYTE fade, CBYTE* msg)
{
    CBYTE* ch;

    float tx;

    //
    // Centre the text.
    //

    tx = x;

    if (msg == NULL) {
        msg = "<NULL>";
    }

    for (ch = msg; *ch; ch++) {
        tx -= float(FontInfo[*ch].width) * 0.5F;
    }

    //
    // Draw it.
    //

    for (ch = msg; *ch; ch++) {
        tx += MENUFONT_fadein_char(tx, y, *ch, fade);
    }
}
