/*
 * imageloader.cpp
 */

#include <assert.h>
#include "debug.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "screen.h"
#include "xu4.h"


// A black & white palette
static RGBA bwPalette[2] = {{0,0,0,0}, {255,255,255,255}};

static const RGBA* stdPalette(int bpp)
{
    switch(bpp) {
        case 8:
            return xu4.imageMgr->vgaPalette();
        case 4:
            return xu4.config->egaPalette();
        case 1:
            return bwPalette;
        default:
            return NULL;
    }
}

/**
 * Fill in the image pixel data from an uncompressed string of bytes.
 *
 * If bpp is 1, 4, or 8, then palette must not be NULL and must have enough
 * entries for that depth (i.e. 2, 16, and 256 respectively).
 */
static void setFromRawData(Image *image, int width, int height, int bpp, unsigned char *rawData, const RGBA *palette) {
    const RGBA* col;
    uint32_t* row;
    uint32_t* rowEnd;
    uint32_t* cpix;
    int rowAdvance;
    int y, i;
    bool screenFormatIsABGR = screenState()->formatIsABGR;

// SDL format->Rmask = 0x00ff0000
#define PIXEL_ARGB_4B(bp)   ((bp[3]<< 24) | (bp[0]<< 16) | (bp[1]<< 8) | bp[2])
#define PIXEL_ARGB_3B(bp)   (0xff000000   | (bp[0]<< 16) | (bp[1]<< 8) | bp[2])
#define PIXEL_ARGB_U32(c)   ((c->a << 24) | (c->r << 16) | (c->g << 8) | c->b)

// ALLEGRO_PIXEL_FORMAT_ABGR_8888
#define PIXEL_ABGR_4B(bp)   ((bp[3]<< 24) | (bp[2]<< 16) | (bp[1]<< 8) | bp[0])
#define PIXEL_ABGR_3B(bp)   (0xff000000   | (bp[2]<< 16) | (bp[1]<< 8) | bp[0])
#define PIXEL_ABGR_U32(c)   ((c->a << 24) | (c->b << 16) | (c->g << 8) | c->r)

    //printf("KR setFromRawData bpp:%d\n", bpp);

    row = image->pixels;
    rowAdvance = image->w;

    switch (bpp) {
    case 32:
        if (screenFormatIsABGR) {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    *cpix++ = PIXEL_ABGR_4B(rawData);
                    rawData += 4;
                }
                row += rowAdvance;
            }
        } else {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    *cpix++ = PIXEL_ARGB_4B(rawData);
                    rawData += 4;
                }
                row += rowAdvance;
            }
        }
        break;

    case 24:
        if (screenFormatIsABGR) {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    *cpix++ = PIXEL_ABGR_3B(rawData);
                    rawData += 3;
                }
                row += rowAdvance;
            }
        } else {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    *cpix++ = PIXEL_ARGB_3B(rawData);
                    rawData += 3;
                }
                row += rowAdvance;
            }
        }
        break;

    case 8:
        if (screenFormatIsABGR) {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    i = *rawData++;
                    col = palette + i;
                    *cpix++ = PIXEL_ABGR_U32(col);
                }
                row += rowAdvance;
            }
        } else {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    i = *rawData++;
                    col = palette + i;
                    *cpix++ = PIXEL_ARGB_U32(col);
                }
                row += rowAdvance;
            }
        }
        break;

    case 4:
        assert((width & 1) == 0);
        if (screenFormatIsABGR) {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    i = *rawData++;

                    col = palette + (i >> 4);
                    *cpix++ = PIXEL_ABGR_U32(col);

                    col = palette + (i & 15);
                    *cpix++ = PIXEL_ABGR_U32(col);
                }
                row += rowAdvance;
            }
        } else {
            for (y = 0; y < height; ++y) {
                cpix = row;
                rowEnd = row + width;
                while (cpix != rowEnd) {
                    i = *rawData++;

                    col = palette + (i >> 4);
                    *cpix++ = PIXEL_ARGB_U32(col);

                    col = palette + (i & 15);
                    *cpix++ = PIXEL_ARGB_U32(col);
                }
                row += rowAdvance;
            }
        }
        break;

    case 1:
    {
        uint32_t black, white;
        int mask;

        col = palette;
        if (screenFormatIsABGR) {
            black = PIXEL_ABGR_U32(col);
            ++col;
            white = PIXEL_ABGR_U32(col);
        } else {
            black = PIXEL_ARGB_U32(col);
            ++col;
            white = PIXEL_ARGB_U32(col);
        }

        assert((width & 7) == 0);
        for (y = 0; y < height; ++y) {
            cpix = row;
            rowEnd = row + width;
            while (cpix != rowEnd) {
                i = *rawData++;
                for (mask = 0x80; mask; mask >>= 1) {
                    *cpix++ = (i & mask) ? white : black;
                }
            }
            row += rowAdvance;
        }
    }
        break;

    default:
        ASSERT(0, "Invalid rawData bits-per-pixel (bpp): %d", bpp);
    }
}


#include "imageloader_fmtowns.cpp"
#include "imageloader_png.cpp"
#include "imageloader_u4.cpp"


Image* loadImage(U4FILE *file, int ftype, int width, int height, int bpp) {
    switch(ftype) {
    case FTYPE_PNG:
        return loadImage_png(file);

    case FTYPE_U4RAW:
    case FTYPE_U4RLE:
    case FTYPE_U4LZW:
    case FTYPE_U5LZW:
        return loadImage_u4(file, ftype, width, height, bpp);

    case FTYPE_FMTOWNS:
    case FTYPE_FMTOWNS_PIC:
    case FTYPE_FMTOWNS_TIF:
        return loadImage_fmTowns(file, width, height, bpp);
    }
    return NULL;
}
