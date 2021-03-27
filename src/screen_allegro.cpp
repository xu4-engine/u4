/*
 * screen_allegro.cpp
 */

#include <allegro5/allegro5.h>

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "settings.h"
#include "screen.h"
#include "xu4.h"

extern bool verbose;


ALLEGRO_EVENT_QUEUE* sa_queue = NULL;
ALLEGRO_DISPLAY* sa_disp = NULL;
ALLEGRO_TIMER* sa_refreshTimer = NULL;
bool screenFormatIsABGR = true;

static ALLEGRO_MOUSE_CURSOR* cursors[5];
static int frameDuration = 0;


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


void screenInit_sys() {
    const Settings& settings = *xu4.settings;
    int format;
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

    al_set_window_title(sa_disp, "Ultima IV");  // configService->gameName()
    //al_set_display_icon(sa_disp, ALLEGRO_BITMAP*);  LoadBMP(ICON_FILE));

    // Can settings.gamma be applied?

    // Default bitmap format is ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA.
    //printf("KR fmt %d\n", al_get_new_bitmap_format());
    //al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888);
    //al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);

    format = al_get_display_format(sa_disp);
    switch (format) {
        default:
            errorWarning("Unsupported Allegro pixel format: %d", format);
            // Fall through...
        case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
            screenFormatIsABGR = false;
            break;
        case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
            screenFormatIsABGR = true;
            break;
    }

    /* enable or disable the mouse cursor */
    if (settings.mouseOptions.enabled) {
        if (!al_install_mouse())
            goto fatal;

        al_register_event_source(sa_queue, al_get_mouse_event_source());

        // Create a temporary bitmap to build the cursor in.
        al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
        ALLEGRO_BITMAP* bm = al_create_bitmap(CURSORSIZE, CURSORSIZE);
        if (bm) {
            cursors[0] = NULL;
            cursors[1] = screenInitCursor(bm, w_xpm);
            cursors[2] = screenInitCursor(bm, n_xpm);
            cursors[3] = screenInitCursor(bm, e_xpm);
            cursors[4] = screenInitCursor(bm, s_xpm);

            al_destroy_bitmap(bm);
            al_show_mouse_cursor(sa_disp);
        }
    } else {
        al_hide_mouse_cursor(sa_disp);
    }

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

    for( int i = 1; i < 5; ++i )
        al_destroy_mouse_cursor(cursors[i]);

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

    ALLEGRO_BITMAP* backBuf = al_get_backbuffer(sa_disp);

    lr = al_lock_bitmap(backBuf, ALLEGRO_PIXEL_FORMAT_ANY,
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
    al_unlock_bitmap(backBuf);

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
