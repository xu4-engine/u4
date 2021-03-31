/*
 * $Id$
 */

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "rle.h"
#include "lzw/u4decode.h"

ImageLoader *U4RawImageLoader::instance = ImageLoader::registerLoader(new U4RawImageLoader, "image/x-u4raw");
ImageLoader *U4RleImageLoader::instance = ImageLoader::registerLoader(new U4RleImageLoader, "image/x-u4rle");
ImageLoader *U4LzwImageLoader::instance = ImageLoader::registerLoader(new U4LzwImageLoader, "image/x-u4lzw");


/**
 * Loads in the raw image and apply the standard U4 16 or 256 color
 * palette.
 */
Image *U4RawImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          errorFatal("dimensions not set for u4raw image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long rawLen = file->length();
    unsigned char *raw = (unsigned char *) malloc(rawLen);
    file->read(raw, 1, rawLen);

    long requiredLength = (width * height * bpp / 8);
    if (rawLen < requiredLength) {
        if (raw)
            free(raw);
        errorWarning("u4Raw Image of size %ld does not fit anticipated size %ld", rawLen, requiredLength);
        return NULL;
    }

    Image *image = Image::create(width, height);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    setFromRawData(image, width, height, bpp, raw, stdPalette(bpp));
    free(raw);

    return image;
}

/**
 * Loads in the rle-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4RleImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          errorFatal("dimensions not set for u4rle image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

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

    Image *image = Image::create(width, height);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    setFromRawData(image, width, height, bpp, raw, stdPalette(bpp));
    free(raw);

    return image;
}

/**
 * Loads in the lzw-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4LzwImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          errorFatal("dimensions not set for u4lzw image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

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

    Image *image = Image::create(width, height);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    setFromRawData(image, width, height, bpp, raw, stdPalette(bpp));
    free(raw);

    return image;
}

