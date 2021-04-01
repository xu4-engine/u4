/*
 * imageloader.h
 */

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include "image.h"
#include "u4file.h"

enum ImageFiletype {
    FTYPE_UNKNOWN,
    FTYPE_PNG,
    FTYPE_U4RAW,
    FTYPE_U4RLE,
    FTYPE_U4LZW,
    FTYPE_U5LZW,
    FTYPE_FMTOWNS,
    FTYPE_FMTOWNS_PIC,
    FTYPE_FMTOWNS_TIF
};

Image* loadImage(U4FILE *file, int ftype, int width, int height, int bpp);

#endif /* IMAGELOADER_H */
