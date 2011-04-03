/*
 * $Id$
 */

#ifndef IMAGELOADER_PNG_H
#define IMAGELOADER_PNG_H

#include "imageloader.h"

/**
 * Loader for PNG images.  All PNG images should be supported: indexed
 * images with palette, or true color, with or without an alpha
 * channel.
 */
class PngImageLoader : public ImageLoader {
    static ImageLoader *instance;

public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp);
    
};

#endif /* IMAGELOADER_PNG_H */
