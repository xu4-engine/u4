/*
 * $Id$
 */

#include "debug.h"
#include "config.h"
#include "image.h"
#include "imageloader.h"

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
        const Config *config = Config::getInstance();

        egaPalette = new RGBA[16];

        std::vector<ConfigElement> paletteConf = config->getElement("egaPalette").getChildren();
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
    int x, y;

    switch (bpp) {

    case 32:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixel(x, y,
                                rawData[(y * width + x) * 4],
                                rawData[(y * width + x) * 4 + 1],
                                rawData[(y * width + x) * 4 + 2],
                                rawData[(y * width + x) * 4 + 3]);
        }
        break;

    case 24:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixel(x, y,
                                rawData[(y * width + x) * 3],
                                rawData[(y * width + x) * 3 + 1],
                                rawData[(y * width + x) * 3 + 2],
                                IM_OPAQUE);
        }
        break;

    case 8:
        if (palette)
            image->setPalette(palette, 256);
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixelIndex(x, y, rawData[y * width + x]);
        }
        break;

    case 4:
        if (palette)
            image->setPalette(palette, 16);
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=2) {
                image->putPixelIndex(x, y, rawData[(y * width + x) / 2] >> 4);
                image->putPixelIndex(x + 1, y, rawData[(y * width + x) / 2] & 0x0f);
            }
        }
        break;

    case 1:
        if (palette)
            image->setPalette(palette, 2);
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=8) {
                image->putPixelIndex(x + 0, y, (rawData[(y * width + x) / 8] >> 7) & 0x01);
                image->putPixelIndex(x + 1, y, (rawData[(y * width + x) / 8] >> 6) & 0x01);
                image->putPixelIndex(x + 2, y, (rawData[(y * width + x) / 8] >> 5) & 0x01);
                image->putPixelIndex(x + 3, y, (rawData[(y * width + x) / 8] >> 4) & 0x01);
                image->putPixelIndex(x + 4, y, (rawData[(y * width + x) / 8] >> 3) & 0x01);
                image->putPixelIndex(x + 5, y, (rawData[(y * width + x) / 8] >> 2) & 0x01);
                image->putPixelIndex(x + 6, y, (rawData[(y * width + x) / 8] >> 1) & 0x01);
                image->putPixelIndex(x + 7, y, (rawData[(y * width + x) / 8] >> 0) & 0x01);
            }
        }
        break;

    default:
        ASSERT(0, "invalid bits-per-pixel (bpp): %d", bpp);
    }
}
