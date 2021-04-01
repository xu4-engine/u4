/*
 * imageloader_u4.cpp
 */

#include "rle.h"
#include "lzw/u4decode.h"
#include "lzw/u6decode.h"

/**
 * Load an Ultima IV image and apply the standard U4 16 or 256 color palette.
 * This loader handles the original 4-bit images, as well as the 8-bit VGA
 * upgrade images.
 */
Image *loadImage_u4(U4FILE *file, int ftype, int width, int height, int bpp) {
    Image* image = NULL;
    unsigned char *raw = NULL;
    unsigned char *compressed = NULL;
    long rawLen, compLen;


    if (width == -1 || height == -1 || bpp == -1)
          errorFatal("dimensions not set for U4 image");

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32,
           "invalid bpp: %d", bpp);

    switch(ftype) {
    case FTYPE_U4RAW:
    {
        // Loader for U4 raw images.  Raw images are just an uncompressed
        // stream of pixel data with no palette information (e.g. shapes.ega,
        // charset.ega).

        rawLen = file->length();
        raw = (unsigned char *) malloc(rawLen);
        file->read(raw, 1, rawLen);

        long requiredLength = (width * height * bpp / 8);
        if (rawLen < requiredLength) {
            errorWarning("u4Raw Image of size %ld does not fit anticipated size %ld", rawLen, requiredLength);
            goto cleanup_raw;
        }
    }
        break;

    case FTYPE_U4RLE:
    case FTYPE_U4LZW:
        // Loader for U4 images with RLE or LZW compression.  Like raw images,
        // the data is just a stream of pixel data with no palette information
        // (e.g. start.ega, rune_*.ega).
        // (e.g. title.ega, tree.ega).

        compLen = file->length();
        compressed = (unsigned char *) malloc(compLen);
        file->read(compressed, 1, compLen);

        if (ftype == FTYPE_U4RLE)
            rawLen = rleDecompressMemory(compressed, compLen, (void**) &raw);
        else
            rawLen = decompress_u4_memory(compressed, compLen, (void**) &raw);
        free(compressed);

        if (rawLen != (width * height * bpp / 8))
            goto cleanup_raw;
        break;

    case FTYPE_U5LZW:
        // Loader for U5 images with LZW compression.  Similar to U4 LZW
        // images, but with a slightly different variation on the LZW algorithm.

        compLen = file->length();
        compressed = (unsigned char *) malloc(compLen);
        file->read(compressed, 1, compLen);

        rawLen = compressed[0] + (compressed[1]<<8) +
                 (compressed[2]<<16) + (compressed[3]<<24);
        raw = (unsigned char *) malloc(rawLen);
        U6Decode::lzw_decompress(compressed+4, compLen-4, raw, rawLen);
        free(compressed);
        break;
    }

    image = Image::create(width, height);
    if (image)
        setFromRawData(image, width, height, bpp, raw, stdPalette(bpp));

cleanup_raw:
    free(raw);
    return image;
}
