/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

#include "config.h"
#include "debug.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "imageloader_u5.h"
#include "lzw/u6decode.h"

using std::vector;

ImageLoader *U5LzwImageLoader::instance = ImageLoader::registerLoader(new U5LzwImageLoader, "image/x-u5lzw");

/**
 * Loads in the lzw-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U5LzwImageLoader::load(U4FILE *file) {
    ASSERT(width != -1 && height != -1 && bpp != -1, "dimensions not set");
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

    Image *image = Image::create(width, height, bpp == 4 || bpp == 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            delete [] raw;
        return NULL;
    }

    U4PaletteLoader paletteLoader;
    if (bpp == 8)
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    else if (bpp == 4)
        image->setPalette(paletteLoader.loadEgaPalette(), 16);

    setFromRawData(image, width, height, bpp, raw);

    delete [] raw;

    return image;
}
