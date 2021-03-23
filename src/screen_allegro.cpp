/*
 * screen_allegro.cpp
 */

#include <allegro5/allegro5.h>

#include "config.h"
#include "context.h"
#include "debug.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "image.h"
#include "imagemgr.h"
#include "intro.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"

#if defined(MACOSX)
#include "macosx/cursors.h"
#else
#include "cursors.h"
#endif

extern bool verbose;


ALLEGRO_EVENT_QUEUE* sa_queue = NULL;
ALLEGRO_DISPLAY* sa_disp = NULL;
#ifdef FBUF
ALLEGRO_BITMAP* sa_frameBuffer = NULL;
#endif
ALLEGRO_TIMER* sa_refreshTimer = NULL;
bool screenFormatIsABGR = true;

static ALLEGRO_MOUSE_CURSOR* cursors[5];
static Scaler filterScaler;
static int frameDuration = 0;


/**
 * Create an Allegro cursor object from an xpm.
 */
#if defined(MACOSX)
#define CURSORSIZE 16
#else
#define CURSORSIZE 32
#endif
static ALLEGRO_MOUSE_CURSOR* screenInitCursor(const char * const xpm[]) {
    return NULL;
#if 0
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

    ALLEGRO_BITMAP* bmp
    return al_create_mouse_cursor(bmp, hot_x, hot_y);
#endif
}


void screenInit_sys() {
    int w = 320 * settings.scale;
    int h = 200 * settings.scale;

    if (! al_init())
        goto fatal;

    if (!al_install_keyboard())
        goto fatal;

    sa_queue = al_create_event_queue();
    if (!sa_queue)
        goto fatal;

    if (settings.fullscreen)
        al_set_new_display_flags(ALLEGRO_FULLSCREEN);

    sa_disp = al_create_display(w, h);
    if(!sa_disp)
        goto fatal;

    // Can settings.gamma be applied?

    // Default is ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA.
    //printf("KR fmt %d\n", al_get_new_bitmap_format());
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888);
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
#ifdef FBUF
    sa_frameBuffer = al_create_bitmap(w, h);
    al_set_target_backbuffer(sa_disp);
#endif

    al_set_window_title(sa_disp, "Ultima IV");  // configService->gameName()
    //al_set_display_icon(sa_disp, ALLEGRO_BITMAP*);  LoadBMP(ICON_FILE));

    /* enable or disable the mouse cursor */
    if (settings.mouseOptions.enabled) {
        al_show_mouse_cursor(sa_disp);

        cursors[0] = NULL;
        cursors[1] = screenInitCursor(w_xpm);
        cursors[2] = screenInitCursor(n_xpm);
        cursors[3] = screenInitCursor(e_xpm);
        cursors[4] = screenInitCursor(s_xpm);
    } else {
        al_hide_mouse_cursor(sa_disp);
    }

    filterScaler = scalerGet(settings.filter);
    if (!filterScaler)
        errorFatal("%s is not a valid filter", settings.filter.c_str());

    sa_refreshTimer = al_create_timer(1.0 / settings.screenAnimationFramesPerSecond);

    al_register_event_source(sa_queue, al_get_keyboard_event_source());
    al_register_event_source(sa_queue, al_get_display_event_source(sa_disp));
    al_register_event_source(sa_queue, al_get_timer_event_source(sa_refreshTimer));
    al_start_timer(sa_refreshTimer);
    return;

fatal:
    errorFatal("Unable to initialize Allegro");
}

void screenDelete_sys() {
    al_destroy_timer(sa_refreshTimer);
    sa_refreshTimer = NULL;

    /*
    for( int i = 1; i < 5; ++i )
        al_destroy_mouse_cursor(cursors[i]);
    */

#ifdef FBUF
    al_destroy_bitmap(sa_frameBuffer);
    sa_frameBuffer = NULL;
#endif

    al_destroy_display(sa_disp);
    sa_disp = NULL;

    al_destroy_event_queue(sa_queue);
    sa_queue = NULL;
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    //SDL_WM_IconifyWindow();
}

extern Image* screenImage;
//extern uint32_t getTicks();

//#define CPU_TEST
#include "support/cpuCounter.h"

/*
 * Show screenImage on the display.
 * If w is zero then the entire display is updated.
 */
// This would be static but its an Image friend to access screenImage->pixels.
void updateDisplay(int x, int y, int w, int h) {
    const ALLEGRO_LOCKED_REGION* lr;
    uint32_t* drow;
    uint32_t* dp;
    const uint32_t* srow;
    const uint32_t* send;
    const uint32_t* sp;
    int dpitch, cr;

#if 0
    static uint32_t dt = 0;
    uint32_t ms = getTicks();
    printf("KR ud %d (%d)\n", ms, ms-dt);
    dt = ms;
#endif

    CPU_START()

    if (w == 0) {
        w = screenImage->w;
        h = screenImage->h;
    }

#ifndef FBUF
    ALLEGRO_BITMAP* bbmap = al_get_backbuffer(sa_disp);
#define sa_frameBuffer  bbmap
#endif

    lr = al_lock_bitmap(sa_frameBuffer, ALLEGRO_PIXEL_FORMAT_ANY,
                        ALLEGRO_LOCK_WRITEONLY);
    assert(lr);
#if 0
    printf("KR updateDisplay format:%d psize:%d pitch:%d\n",
            lr->format, lr->pixel_size, lr->pitch);
#endif
    dpitch = lr->pitch / sizeof(uint32_t);
    drow = ((uint32_t*) lr->data) + y*dpitch + x;
    srow = screenImage->pixels + y*screenImage->w + x;

    for (cr = 0; cr < h; ++cr) {
        dp = drow;
        sp = srow;
        send = srow + w;
        while (sp != send) {
            *dp++ = *sp++;
        }
        drow += dpitch;
        srow += screenImage->w;
    }
    al_unlock_bitmap(sa_frameBuffer);

#ifdef FBUF
    if (w)
        al_draw_bitmap_region(sa_frameBuffer, x, y, w, h, x, y, 0);
    else
        al_draw_bitmap(sa_frameBuffer, 0, 0, 0);
#endif
    al_flip_display();

    CPU_END("ut:")
}

void screenSwapBuffers() {
    updateDisplay(0, 0, 0, 0);
}

void screenWait(int numberOfAnimationFrames) {
    // Does this wait need to handle world animation or input (e.g. user
    // quits game)?

    assert(numberOfAnimationFrames >= 0);

    // Stop refresh timer to prevent events from accumulating in queue.
    al_stop_timer(sa_refreshTimer);
    screenSwapBuffers();
    al_rest(0.001 * (numberOfAnimationFrames * frameDuration));
    al_start_timer(sa_refreshTimer);
}

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered
 * seperately. filter determines whether or not to filter the
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest = NULL;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();

    if (n == 0)
        n = 1;

    isTransparent = src->getTransparentIndex(transparentIndex);
    src->alphaOff();

    while (filter && filterScaler && (scale % 2 == 0)) {
        dest = (*filterScaler)(src, 2, n);
        src = dest;
        scale /= 2;
    }
    if (scale == 3 && scaler3x(settings.filter)) {
        dest = (*filterScaler)(src, 3, n);
        src = dest;
        scale /= 3;
    }

    if (scale != 1)
        dest = (*scalerGet("point"))(src, scale, n);

    if (!dest)
        dest = Image::duplicate(src);

    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);

    if (alpha)
        src->alphaOn();

    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();

    isTransparent = src->getTransparentIndex(transparentIndex);

    src->alphaOff();

    dest = Image::create(src->width() / scale, src->height() / scale, src->isIndexed());
    if (!dest)
        return NULL;

    if (dest->isIndexed())
        dest->setPaletteFromImage(src);

    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }

    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);

    if (alpha)
        src->alphaOn();

    return dest;
}

void screenSetMouseCursor(MouseCursor cursor) {
    static int current = 0;

    if (cursor != current) {
        if (cursor == MC_DEFAULT)
            al_set_system_mouse_cursor(sa_disp, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
        else
            al_set_mouse_cursor(sa_disp, cursors[cursor]);
        current = cursor;
    }
}

void screenShowMouseCursor(bool visible) {
    if (visible)
        al_show_mouse_cursor(sa_disp);
    else
        al_hide_mouse_cursor(sa_disp);
}
