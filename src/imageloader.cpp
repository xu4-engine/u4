/*
 * $Id$
 */

#include "debug.h"
#include "image.h"
#include "imageloader.h"

std::map<std::string, ImageLoader *> *ImageLoader::loaderMap = NULL;

ImageLoader::ImageLoader() : width(-1), height(-1), bpp(-1) {}

void ImageLoader::setDimensions(int width, int height, int bpp) {
    this->width = width;
    this->height = height;
    this->bpp = bpp;
}

ImageLoader *ImageLoader::getLoader(const std::string &fileType) {
    ASSERT(loaderMap != NULL, "loaderMap not initialized");
    if (loaderMap->find(fileType) == loaderMap->end())
        return NULL;
    return (*loaderMap)[fileType];
}

ImageLoader *ImageLoader::registerLoader(ImageLoader *loader, const std::string &type) {
    if (loaderMap == NULL) {
        loaderMap = new std::map<std::string, ImageLoader *>;
    }
    (*loaderMap)[type] = loader;
    return loader;
}

void ImageLoader::setFromRawData(Image *image, int width, int height, int bpp, unsigned char *rawData) {
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
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixelIndex(x, y, rawData[y * width + x]);
        }
        break;

    case 4:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=2) {
                image->putPixelIndex(x, y, rawData[(y * width + x) / 2] >> 4);
                image->putPixelIndex(x + 1, y, rawData[(y * width + x) / 2] & 0x0f);
            }
        }
        break;

    default:
        ASSERT(0, "invalid bits-per-pixel (bpp): %d", bpp);
    }
}
