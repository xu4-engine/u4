/*
 * $Id$
 */

#ifndef IMAGE_H
#define IMAGE_H

typedef enum {
    IMTYPE_HW,
    IMTYPE_SW
} ImageType;

typedef struct _Image {
    int w, h, scale;
    int indexed;
#ifdef _SDL_video_h
    SDL_Surface *surface;
#else
    void *surface;
#endif
} Image;

typedef struct _RGB {
    int r, g, b;
} RGB;

Image *imageNew(int w, int h, int scale, int indexed, ImageType type);
void imageDelete(Image *im);
void imageSetPaletteFromImage(Image *im, Image *src);
void imagePutPixel(Image *im, int x, int y, int r, int g, int b);
void imagePutPixelIndex(Image *im, int x, int y, unsigned int index);
void imagePutPixelScaled(Image *im, int x, int y, int r, int g, int b);
void imageGetPixel(Image *im, int x, int y, int *r, int *g, int *b);
void imageGetPixelIndex(Image *im, int x, int y, unsigned int *index);
void imageFillRect(Image *im, int x, int y, int w, int h, int r, int g, int b);
void imageDraw(const Image *im, int x, int y);
void imageDrawSubRect(const Image *im, int x, int y, int rx, int ry, int rw, int rh);

#endif /* IMAGE_H */
