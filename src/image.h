/*
 * $Id$
 */

#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _RGBA {
    unsigned int r, g, b, a;
} RGBA;

#define IM_OPAQUE 255
#define IM_TRANSPARENT 0

Image *imageNew(int w, int h, int scale, int indexed, ImageType type);
void imageDelete(Image *im);
void imageSetPaletteFromImage(Image *im, Image *src);
int imageGetTransparentIndex(Image *im, unsigned int *index);
void imageSetTransparentIndex(Image *im, unsigned int index);
void imagePutPixel(Image *im, int x, int y, int r, int g, int b, int a);
void imagePutPixelIndex(Image *im, int x, int y, unsigned int index);
void imagePutPixelScaled(Image *im, int x, int y, int r, int g, int b, int a);
void imageGetPixel(Image *im, int x, int y, int *r, int *g, int *b, int *a);
void imageGetPixelIndex(Image *im, int x, int y, unsigned int *index);
void imageFillRect(Image *im, int x, int y, int w, int h, int r, int g, int b);
void imageDraw(const Image *im, int x, int y);
void imageDrawSubRect(const Image *im, int x, int y, int rx, int ry, int rw, int rh);
void imageDrawSubRectInverted(const Image *im, int x, int y, int rx, int ry, int rw, int rh);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_H */
