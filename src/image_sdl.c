/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>

#include "debug.h"
#include "image.h"

Image *imageNew(int w, int h, int scale, int indexed, ImageType type) {
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = malloc(sizeof(Image));

    im->w = w;
    im->h = h;
    im->scale = scale;
    im->indexed = indexed;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    if (type == IMTYPE_HW)
        flags = SDL_HWSURFACE;
    else
        flags = SDL_SWSURFACE;
    
    if (indexed)
        im->surface = SDL_CreateRGBSurface(flags, w, h, 8, rmask, gmask, bmask, amask);
    else
        im->surface = SDL_CreateRGBSurface(flags, w, h, 32, rmask, gmask, bmask, amask);

    if (!im->surface) {
        free(im);
        return NULL;
    }

    return im;
}

void imageDelete(Image *im) {
    SDL_FreeSurface(im->surface);
    free(im);
}

void imageSetPaletteFromImage(Image *im, Image *src) {
    ASSERT(im->indexed && src->indexed, "imageSetPaletteFromImage called on non-indexed image");
    memcpy(im->surface->format->palette->colors, 
           src->surface->format->palette->colors, 
           sizeof(SDL_Color) * src->surface->format->palette->ncolors);
}

void imagePutPixel(Image *im, int x, int y, int r, int g, int b) {
    imagePutPixelIndex(im, x, y, SDL_MapRGB(im->surface->format, r, g, b));
}

void imagePutPixelIndex(Image *im, int x, int y, unsigned int index) {
    int bpp;
    Uint8 *p;

    bpp = im->surface->format->BytesPerPixel;
    p = (Uint8 *)im->surface->pixels + y * im->surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = index;
        break;

    case 2:
        *(Uint16 *)p = index;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (index >> 16) & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = index & 0xff;
        } else {
            p[0] = index & 0xff;
            p[1] = (index >> 8) & 0xff;
            p[2] = (index >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = index;
        break;
    }
}

void imagePutPixelScaled(Image *im, int x, int y, int r, int g, int b) {
    int xs, ys;

    for (xs = 0; xs < im->scale; xs++) {
        for (ys = 0; ys < im->scale; ys++) {
            imagePutPixel(im, x * im->scale + xs, y * im->scale + ys, r, g, b);
        }
    }
}

void imageGetPixel(Image *im, int x, int y, int *r, int *g, int *b) {
    unsigned int index;
    Uint8 r1, g1, b1;

    imageGetPixelIndex(im, x, y, &index);

    SDL_GetRGB(index, im->surface->format, &r1, &g1, &b1);
    *r = r1;
    *g = g1;
    *b = b1;
}

void imageGetPixelIndex(Image *im, int x, int y, unsigned int *index) {
    int bpp = im->surface->format->BytesPerPixel;

    Uint8 *p = (Uint8 *) im->surface->pixels + y * im->surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        (*index) = *p;
        break;

    case 2:
        (*index) = *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            (*index) =  p[0] << 16 | p[1] << 8 | p[2];
        else
            (*index) =  p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        (*index) = *(Uint32 *)p;

    default:
        return;
    }
}

void imageFillRect(Image *im, int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect dest;
    Uint32 pixel;

    pixel = SDL_MapRGB(im->surface->format, r, g, b);
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;

    SDL_FillRect(im->surface, &dest, pixel);
}

void imageDraw(const Image *im, int x, int y) {
    SDL_Rect r;

    r.x = x;
    r.y = y;
    r.w = im->w;
    r.h = im->h;
    SDL_BlitSurface(im->surface, NULL, SDL_GetVideoSurface(), &r);
}
