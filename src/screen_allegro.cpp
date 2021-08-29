/*
 * screen_allegro.cpp
 */

#include "screen_allegro.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "settings.h"
#include "screen.h"
#include "xu4.h"

extern bool verbose;

#if defined(MACOSX)
#define CURSORSIZE 16
#define XPMSIZE    CURSORSIZE
#include "macosx/cursors.h"
#else
#define CURSORSIZE 24
#define XPMSIZE    32
#include "cursors.h"
#endif

/**
 * Create an Allegro cursor object from an xpm.
 */
static ALLEGRO_MOUSE_CURSOR* screenInitCursor(ALLEGRO_BITMAP* bmp, const char * const xpm[]) {
    ALLEGRO_COLOR white, black, empty;
    int row, col;
    int hot_x, hot_y;

    white = al_map_rgb(255, 255, 255);
    black = al_map_rgb(0, 0, 0);
    empty = al_map_rgba(0, 0, 0, 0);

    al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
    al_set_target_bitmap(bmp);

    for (row=0; row < CURSORSIZE; row++) {
        for (col=0; col < CURSORSIZE; col++) {
            switch (xpm[4+row][col]) {
                case 'X':
                    al_put_pixel(col, row, black);
                    break;
                case '.':
                    al_put_pixel(col, row, white);
                    break;
                case ' ':
                    al_put_pixel(col, row, empty);
                    break;
            }
        }
    }
    sscanf(xpm[4+XPMSIZE], "%d,%d", &hot_x, &hot_y);

    al_unlock_bitmap(bmp);
    return al_create_mouse_cursor(bmp, hot_x, hot_y);
}


#ifdef USE_GL
#include "gpu_opengl.cpp"
#include <allegro5/allegro_opengl.h>
#endif


void screenInit_sys(const Settings* settings, int reset) {
    ScreenAllegro* sa;
#ifdef USE_GL
    // NOTE: _FORWARD_COMPATIBLE requires al_set_current_opengl_context()
    int dflags = ALLEGRO_OPENGL | ALLEGRO_OPENGL_3_0 |
                 ALLEGRO_OPENGL_FORWARD_COMPATIBLE;
#else
    int dflags = 0;
#endif
    int dw = 320 * settings->scale;
    int dh = 200 * settings->scale;

    if (reset) {
        sa = SA;

        // On reset sa->queue is not touched as the TimedEventMgr has a timer
        // registered and that needs to keep running.

        // Halt refresh timer as display re-creation may take a moment.
        al_stop_timer(sa->refreshTimer);

        al_unregister_event_source(sa->queue, al_get_display_event_source(sa->disp));

#ifdef USE_GL
        gpu_free(&sa->gpu);
#endif
        al_destroy_display(sa->disp);
        sa->disp = NULL;

        ALLEGRO_EVENT_SOURCE* source = al_get_mouse_event_source();
        if (source)
            al_unregister_event_source(sa->queue, source);
    } else {
        xu4.screenSys = sa = new ScreenAllegro;
#ifdef USE_GL
        xu4.gpu = &sa->gpu;
#endif
        memset(sa, 0, sizeof(ScreenAllegro));

        if (! al_init())
            goto fatal;

        if (!al_install_keyboard())
            goto fatal;

        sa->queue = al_create_event_queue();
        if (!sa->queue)
            goto fatal;
    }

    if (settings->fullscreen)
        dflags |= ALLEGRO_FULLSCREEN;
    al_set_new_display_flags(dflags);

    sa->disp = al_create_display(dw, dh);
    if (! sa->disp)
        goto fatal;

    al_set_window_title(sa->disp, "Ultima IV");  // configService->gameName()
    //al_set_display_icon(sa->disp, ALLEGRO_BITMAP*);  LoadBMP(ICON_FILE));

    // Can settings->gamma be applied?

#ifndef USE_GL
    // Default bitmap format is ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA.
    //printf("KR fmt %d\n", al_get_new_bitmap_format());
    //al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888);
    //al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);

    {
    ScreenState* state = screenState();
    int format = al_get_display_format(sa->disp);
    switch (format) {
        default:
            errorWarning("Unsupported Allegro pixel format: %d", format);
            // Fall through...
        case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
        case ALLEGRO_PIXEL_FORMAT_XRGB_8888:
            state->formatIsABGR = false;
            break;
        case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
        case ALLEGRO_PIXEL_FORMAT_XBGR_8888:
            state->formatIsABGR = true;
            break;
    }
    }
#endif

    /* enable or disable the mouse cursor */
    if (settings->mouseOptions.enabled) {
        if (!al_install_mouse())
            goto fatal;

        if (sa->cursors[1] == NULL) {
            // Create a temporary bitmap to build the cursor in.
            al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
            ALLEGRO_BITMAP* bm = al_create_bitmap(CURSORSIZE, CURSORSIZE);
            if (bm) {
                sa->cursors[0] = NULL;
                sa->cursors[1] = screenInitCursor(bm, w_xpm);
                sa->cursors[2] = screenInitCursor(bm, n_xpm);
                sa->cursors[3] = screenInitCursor(bm, e_xpm);
                sa->cursors[4] = screenInitCursor(bm, s_xpm);

                al_destroy_bitmap(bm);
            }
        }

        al_register_event_source(sa->queue, al_get_mouse_event_source());
        al_show_mouse_cursor(sa->disp);
    } else {
        al_hide_mouse_cursor(sa->disp);
    }

#ifdef USE_GL
    // NOTE: The GL context is made current after creating the mouse cursors
    // as the context is lost when mucking with bitmaps.
    al_set_current_opengl_context(sa->disp);

    if (! gpu_init(&sa->gpu, dw, dh, settings->scale))
        errorFatal("Unable to initialize OpenGL resources");
#endif

    sa->refreshRate = 1.0 / settings->screenAnimationFramesPerSecond;
    if (reset) {
        al_set_timer_speed(sa->refreshTimer, sa->refreshRate);
    } else {
        sa->refreshTimer = al_create_timer(sa->refreshRate);

        al_register_event_source(sa->queue, al_get_timer_event_source(sa->refreshTimer));
        al_register_event_source(sa->queue, al_get_keyboard_event_source());
    }

    al_register_event_source(sa->queue, al_get_display_event_source(sa->disp));
    al_start_timer(sa->refreshTimer);
    return;

fatal:
    errorFatal("Unable to initialize Allegro");
}

void screenDelete_sys() {
    ScreenAllegro* sa = SA;

    al_destroy_timer(sa->refreshTimer);
    sa->refreshTimer = NULL;

#ifdef USE_GL
    gpu_free(&sa->gpu);
#endif

    for( int i = 1; i < 5; ++i )
        al_destroy_mouse_cursor(sa->cursors[i]);

    al_destroy_event_queue(sa->queue);
    sa->queue = NULL;

    al_destroy_display(sa->disp);
    sa->disp = NULL;

    delete sa;
    xu4.screenSys = NULL;
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    //SDL_WM_IconifyWindow();
}

//extern uint32_t getTicks();

//#define CPU_TEST
#include "support/cpuCounter.h"

#ifndef USE_GL
/*
 * Show screenImage on the display.
 * If w is zero then the entire display is updated.
 */
static void updateDisplay(int x, int y, int w, int h) {
    const ALLEGRO_LOCKED_REGION* lr;
    uint32_t* drow;
    uint32_t* dp;
    const uint32_t* srow;
    const uint32_t* send;
    const uint32_t* sp;
    int dpitch, cr;
    int screenImageW = xu4.screenImage->width();
    int offset = screenState()->vertOffset;

#if 0
    static uint32_t dt = 0;
    uint32_t ms = getTicks();
    printf("KR ud %d (%d)\n", ms, ms-dt);
    dt = ms;
#endif

    CPU_START()

    if (w == 0) {
        w = xu4.screenImage->width();
        h = xu4.screenImage->height();
    }

    ALLEGRO_BITMAP* backBuf = al_get_backbuffer(SA->disp);

    lr = al_lock_bitmap(backBuf, ALLEGRO_PIXEL_FORMAT_ANY,
                        ALLEGRO_LOCK_WRITEONLY);
    assert(lr);
#if 0
    printf("KR updateDisplay format:%d psize:%d pitch:%d\n",
            lr->format, lr->pixel_size, lr->pitch);
#endif
    dpitch = lr->pitch / sizeof(uint32_t);
    drow = ((uint32_t*) lr->data) + y*dpitch + x;
    srow = xu4.screenImage->pixelData() + y*screenImageW + x;

    if (offset > 0) {
        h -= offset;
        while (offset) {
            memset(drow, 0, w * sizeof(uint32_t));
            drow += dpitch;
            --offset;
        }
    }

    for (cr = 0; cr < h; ++cr) {
        dp = drow;
        sp = srow;
        send = srow + w;
        while (sp != send) {
            *dp++ = *sp++;
        }
        drow += dpitch;
        srow += screenImageW;
    }
    al_unlock_bitmap(backBuf);

    al_flip_display();

    CPU_END("ut:")
}
#endif

void screenSwapBuffers() {
#ifdef USE_GL
    CPU_START()
#ifndef GPU_RENDER
    const ScreenAllegro* sa = SA;
    gpu_viewport(0, 0, al_get_display_width(sa->disp),
                       al_get_display_height(sa->disp));
    gpu_background(xu4.gpu, NULL, xu4.screenImage);
#endif
    al_flip_display();
    CPU_END("ut:")
#else
    updateDisplay(0, 0, 0, 0);
#endif
}

void screenWait(int numberOfAnimationFrames) {
    ScreenAllegro* sa = SA;

    // Does this wait need to handle world animation or input (e.g. user
    // quits game)?

    assert(numberOfAnimationFrames >= 0);

    // Stop refresh timer to prevent events from accumulating in queue.
    al_stop_timer(sa->refreshTimer);
    screenSwapBuffers();
    al_rest(sa->refreshRate * numberOfAnimationFrames);
    al_start_timer(sa->refreshTimer);
}

void screenSetMouseCursor(MouseCursor cursor) {
    ScreenAllegro* sa = SA;

    if (cursor != sa->currentCursor) {
        if (cursor == MC_DEFAULT)
            al_set_system_mouse_cursor(sa->disp, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
        else
            al_set_mouse_cursor(sa->disp, sa->cursors[cursor]);
        sa->currentCursor = cursor;
    }
}

void screenShowMouseCursor(bool visible) {
    if (visible)
        al_show_mouse_cursor(SA->disp);
    else
        al_hide_mouse_cursor(SA->disp);
}
