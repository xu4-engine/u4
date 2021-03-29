/*
 * screen_sdl.cpp
 */

#include <SDL.h>

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "settings.h"
#include "screen.h"

#if defined(MACOSX)
#include "macosx/cursors.h"
#else
#include "cursors.h"
#endif

extern bool verbose;
extern Image* screenImage;
extern unsigned int refresh_callback(unsigned int, void*);

bool screenFormatIsABGR = true;

static SDL_Cursor *cursors[5] = {NULL, NULL, NULL, NULL, NULL};
static SDL_TimerID refreshTimer = NULL;
static int frameDuration = 0;

SDL_Cursor *screenInitCursor(const char * const xpm[]);


int u4_SDL_InitSubSystem(Uint32 flags) {
    int f = SDL_WasInit(SDL_INIT_EVERYTHING);
    if (f == 0) {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    }
    if (!SDL_WasInit(flags))
        return SDL_InitSubSystem(flags);
    else
        return 0;
}

void u4_SDL_QuitSubSystem(Uint32 flags) {
    if (SDL_WasInit(SDL_INIT_EVERYTHING) == flags)
        SDL_Quit();
    else
        SDL_QuitSubSystem(flags);
}

void screenInit_sys(const Settings* settings, int reset) {
    if (reset) {
        SDL_RemoveTimer(refreshTimer);
        refreshTimer = NULL;
    } else {
        /* start SDL */
        if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
            errorFatal("unable to init SDL: %s", SDL_GetError());
        SDL_EnableUNICODE(1);
        atexit(SDL_Quit);

        SDL_WM_SetCaption("Ultima IV", NULL);
#ifdef ICON_FILE
        SDL_WM_SetIcon(SDL_LoadBMP(ICON_FILE), NULL);
#endif
    }

    SDL_SetGamma(settings->gamma / 100.0f, settings->gamma / 100.0f, settings->gamma / 100.0f);

    if (!SDL_SetVideoMode(320 * settings->scale, 200 * settings->scale, 32, SDL_HWSURFACE | (settings->fullscreen ? SDL_FULLSCREEN : 0)))
        errorFatal("unable to set video: %s", SDL_GetError());

    if (verbose) {
        char driver[32];
        printf("screen initialized [screenInit()], using %s video driver\n", SDL_VideoDriverName(driver, sizeof(driver)));
    }

    /* enable or disable the mouse cursor */
    if (settings->mouseOptions.enabled) {
        SDL_ShowCursor(SDL_ENABLE);
        if (cursors[0] == NULL) {
            cursors[0] = SDL_GetCursor();
            cursors[1] = screenInitCursor(w_xpm);
            cursors[2] = screenInitCursor(n_xpm);
            cursors[3] = screenInitCursor(e_xpm);
            cursors[4] = screenInitCursor(s_xpm);
        }
    } else {
        SDL_ShowCursor(SDL_DISABLE);
    }

    {
    SDL_Surface* ss = SDL_GetVideoSurface();
#if 0
    printf( "SDL color masks: R:%08x G:%08x B:%08x A:%08x\n",
            ss->format->Rmask, ss->format->Gmask,
            ss->format->Bmask, ss->format->Amask );
#endif
    switch (ss->format->Rmask) {
        default:
            errorWarning("Unsupported SDL pixel format: %d:%08x",
                         ss->format->BitsPerPixel, ss->format->Rmask);
            // Fall through...
        case 0x00ff0000:
            screenFormatIsABGR = false;
            break;
        case 0x000000ff:
            screenFormatIsABGR = true;
            break;
    }
    }

    frameDuration = 1000 / settings->screenAnimationFramesPerSecond;
    refreshTimer = SDL_AddTimer(frameDuration, &refresh_callback, NULL);
}

void screenDelete_sys() {
    SDL_RemoveTimer(refreshTimer);
    refreshTimer = NULL;

    SDL_FreeCursor(cursors[1]);
    SDL_FreeCursor(cursors[2]);
    SDL_FreeCursor(cursors[3]);
    SDL_FreeCursor(cursors[4]);

    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    SDL_WM_IconifyWindow();
}

#if 0
void screenDeinterlaceCga(unsigned char *data, int width, int height, int tiles, int fudge) {
    unsigned char *tmp;
    int t, x, y;
    int tileheight = height / tiles;

    tmp = new unsigned char[width * tileheight / 4];

    for (t = 0; t < tiles; t++) {
        unsigned char *base;
        base = &(data[t * (width * tileheight / 4)]);

        for (y = 0; y < (tileheight / 2); y++) {
            for (x = 0; x < width; x+=4) {
                tmp[((y * 2) * width + x) / 4] = base[(y * width + x) / 4];
            }
        }
        for (y = tileheight / 2; y < tileheight; y++) {
            for (x = 0; x < width; x+=4) {
                tmp[(((y - (tileheight / 2)) * 2 + 1) * width + x) / 4] = base[(y * width + x) / 4 + fudge];
            }
        }
        for (y = 0; y < tileheight; y++) {
            for (x = 0; x < width; x+=4) {
                base[(y * width + x) / 4] = tmp[(y * width + x) / 4];
            }
        }
    }

    delete [] tmp;
}

/**
 * Load an image from an ".pic" CGA image file.
 */
int screenLoadImageCga(Image **image, int width, int height, U4FILE *file, CompressionType comp, int tiles) {
    Image *img;
    int x, y;
    unsigned char *compressed_data, *decompressed_data = NULL;
    long inlen, decompResult;

    inlen = u4flength(file);
    compressed_data = (Uint8 *) malloc(inlen);
    u4fread(compressed_data, 1, inlen, file);

    switch(comp) {
    case COMP_NONE:
        decompressed_data = compressed_data;
        decompResult = inlen;
        break;
    case COMP_RLE:
        decompResult = rleDecompressMemory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    case COMP_LZW:
        decompResult = decompress_u4_memory(compressed_data, inlen, (void **) &decompressed_data);
        free(compressed_data);
        break;
    default:
        ASSERT(0, "invalid compression type %d", comp);
    }

    if (decompResult == -1) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    screenDeinterlaceCga(decompressed_data, width, height, tiles, 0);

    img = Image::create(width, height, true);
    if (!img) {
        if (decompressed_data)
            free(decompressed_data);
        return 0;
    }

    img->setPalette(egaPalette, 16);

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x+=4) {
            img->putPixelIndex(x, y, decompressed_data[(y * width + x) / 4] >> 6);
            img->putPixelIndex(x + 1, y, (decompressed_data[(y * width + x) / 4] >> 4) & 0x03);
            img->putPixelIndex(x + 2, y, (decompressed_data[(y * width + x) / 4] >> 2) & 0x03);
            img->putPixelIndex(x + 3, y, (decompressed_data[(y * width + x) / 4]) & 0x03);
        }
    }
    free(decompressed_data);

    (*image) = img;

    return 1;
}
#endif

/*
 * Show screenImage on the display.
 * If w is zero then the entire display is updated.
 */
// This would be static but its an Image friend to access screenImage->pixels.
void updateDisplay( int x, int y, int w, int h ) {
    SDL_Surface* ss = SDL_GetVideoSurface();
    if (ss) {
        uint32_t* dp;
        uint32_t* drow;
        const uint32_t* sp;
        const uint32_t* send;
        const uint32_t* srow;
        int dpitch = ss->pitch / sizeof(uint32_t);
        int cr;

#if 0
        printf( "KR redraw format:(%d %08x %08x %d,%d pitch:%d\n",
                ss->format->BitsPerPixel,
                ss->format->Rmask, ss->format->Amask,
                ss->w, ss->h, ss->pitch );
#endif

        if (w == 0) {
            w = ss->w;
            h = ss->h;
        }

        SDL_LockSurface(ss);
        srow = screenImage->pixels + y*screenImage->w + x;
        drow = ((uint32_t*) ss->pixels) + y*dpitch + x;
        for (cr = 0; cr < h; ++cr) {
            dp = drow;
            sp = srow;
            send = srow + screenImage->w;
            while (sp != send)
                *dp++ = *sp++;
            srow += screenImage->w;
            drow += dpitch;
        }
        SDL_UnlockSurface(ss);

        SDL_UpdateRect(ss, 0, 0, 0, 0);
    }
}

//#define CPU_TEST
#include "support/cpuCounter.h"

void screenSwapBuffers() {
    CPU_START()
    updateDisplay(0, 0, 0, 0);
    CPU_END("ut:")
}

void screenWait(int numberOfAnimationFrames) {
    SDL_Delay(numberOfAnimationFrames * frameDuration);
}

/**
 * Create an SDL cursor object from an xpm.  Derived from example in
 * SDL documentation project.
 */
#if defined(MACOSX)
#define CURSORSIZE 16
#else
#define CURSORSIZE 32
#endif
SDL_Cursor *screenInitCursor(const char * const xpm[]) {
    int i, row, col;
    Uint8 data[(CURSORSIZE/8)*CURSORSIZE];
    Uint8 mask[(CURSORSIZE/8)*CURSORSIZE];
    int hot_x, hot_y;

    i = -1;
    for (row=0; row < CURSORSIZE; row++) {
        for (col=0; col < CURSORSIZE; col++) {
            if (col % 8) {
                data[i] <<= 1;
                mask[i] <<= 1;
            } else {
                i++;
                data[i] = mask[i] = 0;
            }
            switch (xpm[4+row][col]) {
            case 'X':
                data[i] |= 0x01;
                mask[i] |= 0x01;
                break;
            case '.':
                mask[i] |= 0x01;
                break;
            case ' ':
                break;
            }
        }
    }
    sscanf(xpm[4+row], "%d,%d", &hot_x, &hot_y);
    return SDL_CreateCursor(data, mask, CURSORSIZE, CURSORSIZE, hot_x, hot_y);
}

void screenSetMouseCursor(MouseCursor cursor) {
    static int current = 0;

    if (cursor != current) {
        SDL_SetCursor(cursors[cursor]);
        current = cursor;
    }
}

void screenShowMouseCursor(bool visible) {
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}
