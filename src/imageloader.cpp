/*
 * $Id$
 */

#include <assert.h>
#include "debug.h"
#include "config.h"
#include "image.h"
#include "imageloader.h"
#include "xu4.h"

extern bool screenFormatIsABGR;

std::map<std::string, ImageLoader *> *ImageLoader::loaderMap = NULL;

// TODO: Delete these on program exit!
static RGBA* bwPalette = NULL;
static RGBA* egaPalette = NULL;
static RGBA* vgaPalette = NULL;

/**
 * This class method returns the registered concrete subclass
 * appropriate for loading images of a type given by fileType.
 */
ImageLoader *ImageLoader::getLoader(const std::string &fileType) {
    ASSERT(loaderMap != NULL, "ImageLoader::getLoader loaderMap not initialized");
    if (loaderMap->find(fileType) == loaderMap->end())
        return NULL;
    return (*loaderMap)[fileType];
}

/**
 * Register an image loader.  Concrete subclasses should register an
 * instance at startup.  This method is safe to call from a global
 * object constructor or static initializer.
 */
ImageLoader *ImageLoader::registerLoader(ImageLoader *loader, const std::string &type) {
    if (loaderMap == NULL) {
        loaderMap = new std::map<std::string, ImageLoader *>;
    }
    (*loaderMap)[type] = loader;
    return loader;
}

/**
 * Loads a simple black & white palette
 */
static RGBA* loadBWPalette() {
    if (bwPalette == NULL) {
        bwPalette = new RGBA[2];

        bwPalette[0].r = 0;
        bwPalette[0].g = 0;
        bwPalette[0].b = 0;

        bwPalette[1].r = 255;
        bwPalette[1].g = 255;
        bwPalette[1].b = 255;
    }
    return bwPalette;
}

/**
 * Loads the basic EGA palette from egaPalette.xml
 */
static RGBA* loadEgaPalette() {
    if (egaPalette == NULL) {
        int index = 0;

        egaPalette = new RGBA[16];

        std::vector<ConfigElement> paletteConf = xu4.config->getElement("egaPalette").getChildren();
        for (std::vector<ConfigElement>::iterator i = paletteConf.begin(); i != paletteConf.end(); i++) {

            if (i->getName() != "color")
                continue;

            egaPalette[index].r = i->getInt("red");
            egaPalette[index].g = i->getInt("green");
            egaPalette[index].b = i->getInt("blue");

            index++;
        }
    }
    return egaPalette;
}

/**
 * Load the 256 color VGA palette from a file.
 */
static RGBA* loadVgaPalette() {
    if (vgaPalette == NULL) {
        U4FILE *pal = u4fopen("u4vga.pal");
        if (!pal)
            return NULL;

        vgaPalette = new RGBA[256];

        for (int i = 0; i < 256; i++) {
            vgaPalette[i].r = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].g = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].b = u4fgetc(pal) * 255 / 63;
        }
        u4fclose(pal);
    }
    return vgaPalette;
}

RGBA* ImageLoader::stdPalette(int bpp)
{
    switch(bpp) {
        case 8:
            return loadVgaPalette();
        case 4:
            return loadEgaPalette();
        case 1:
            return loadBWPalette();
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
void ImageLoader::setFromRawData(Image *image, int width, int height, int bpp, unsigned char *rawData, RGBA *palette) {
    const RGBA* col;
    uint32_t* row;
    uint32_t* rowEnd;
    uint32_t* cpix;
    int rowAdvance;
    int y, i;

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
