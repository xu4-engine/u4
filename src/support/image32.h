/*
 * image32.h
 */

#ifndef IMAGE32_H
#define IMAGE32_H

#include <stdint.h>

typedef struct RGBA  RGBA;

struct RGBA {
    uint8_t r, g, b, a;
};

#define rgba_set(S,R,G,B,A)     S.r = R; S.g = G; S.b = B; S.a = A
#define rgba_setp(S,R,G,B,A)    S->r = R; S->g = G; S->b = B; S->a = A

typedef struct {
    uint32_t* pixels;
    uint16_t w, h;
} Image32;

#ifdef __cplusplus
//extern "C" {
#endif

void     image32_init(Image32*);
int      image32_allocPixels(Image32*, uint16_t w, uint16_t h);
void     image32_freePixels(Image32*);
int      image32_duplicatePixels(Image32* dest, const Image32* src);
void     image32_fill(Image32*, const RGBA* color);
void     image32_fillRect(Image32*, int x, int y, int rw, int rh,
                          const RGBA* color);
void     image32_blit(Image32* dest, int dx, int dy,
                      const Image32* src, int blend);
void     image32_blitRect(Image32* dest, int dx, int dy,
                          const Image32* src, int sx, int sy, int sw, int sh,
                          int blend);
//void     image32_loadPPM(Image32*, const char *filename);
void     image32_savePPM(const Image32*, const char *filename);

#ifdef __cplusplus
//}
#endif

#endif /* IMAGE32_H */
