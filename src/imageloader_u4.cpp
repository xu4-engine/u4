/*
 * $Id$
 */

#include "debug.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "rle.h"
#include "lzw/u4decode.h"

extern RGBA egaPalette[];
extern RGBA vgaPalette[];

ImageLoader *U4RawImageLoader::instance = ImageLoader::registerLoader(new U4RawImageLoader, "image/x-u4raw");
ImageLoader *U4RleImageLoader::instance = ImageLoader::registerLoader(new U4RleImageLoader, "image/x-u4rle");
ImageLoader *U4LzwImageLoader::instance = ImageLoader::registerLoader(new U4LzwImageLoader, "image/x-u4lzw");

Image *U4RawImageLoader::load(U4FILE *file) {
    ASSERT(width != -1 && height != -1 && bpp != -1, "dimensions not set");
    ASSERT(bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long rawLen = file->length();
    unsigned char *raw = (unsigned char *) malloc(rawLen);
    file->read(raw, 1, rawLen);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            free(raw);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp == 4 || bpp == 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    if (bpp == 8)
        image->setPalette(vgaPalette, 256);
    else if (bpp == 4)
        image->setPalette(egaPalette, 16);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

Image *U4RleImageLoader::load(U4FILE *file) {
    ASSERT(width != -1 && height != -1 && bpp != -1, "dimensions not set");
    ASSERT(bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = rleDecompressMemory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            free(raw);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp == 4 || bpp == 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    if (bpp == 8)
        image->setPalette(vgaPalette, 256);
    else if (bpp == 4)
        image->setPalette(egaPalette, 16);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

Image *U4LzwImageLoader::load(U4FILE *file) {
    ASSERT(width != -1 && height != -1 && bpp != -1, "dimensions not set");
    ASSERT(bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = decompress_u4_memory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            free(raw);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp == 4 || bpp == 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    if (bpp == 8)
        image->setPalette(vgaPalette, 256);
    else if (bpp == 4)
        image->setPalette(egaPalette, 16);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

