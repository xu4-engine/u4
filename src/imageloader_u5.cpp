/*
 * $Id$
 */

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u5.h"
#include "lzw/u6decode.h"

ImageLoader *U5LzwImageLoader::instance = ImageLoader::registerLoader(new U5LzwImageLoader, "image/x-u5lzw");

/**
 * Loads in the lzw-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U5LzwImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          errorFatal("dimensions not set for u5lzw image");
    }

    ASSERT(bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = new unsigned char[compressedLen];
    file->read(compressed, 1, compressedLen);

    long rawLen = compressed[0] + (compressed[1]<<8) + (compressed[2]<<16) + (compressed[3]<<24);
    unsigned char *raw = new unsigned char[rawLen];

    U6Decode::lzw_decompress(compressed+4, compressedLen-4, raw, rawLen);
    delete [] compressed;

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            delete [] raw;
        return NULL;
    }

    Image *image = Image::create(width, height, bpp == 4 || bpp == 8);
    if (!image) {
        if (raw)
            delete [] raw;
        return NULL;
    }

    setFromRawData(image, width, height, bpp, raw, stdPalette(bpp));
    delete [] raw;

    return image;
}
