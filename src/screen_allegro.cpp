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

#ifdef __linux__
extern Image* loadImage_png(U4FILE *file);

static ALLEGRO_BITMAP* loadBitmapPng(const char* filename) {
    U4FILE* uf = u4fopen_stdio(filename);
    if (uf) {
        Image* img = loadImage_png(uf);
        u4fclose(uf);
        if (img) {
            ALLEGRO_BITMAP* bm;
            const RGBA* col;
            int x, y;

            al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
            bm = al_create_bitmap(img->w, img->h);

            al_lock_bitmap(bm, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
            al_set_target_bitmap(bm);

            col = (const RGBA*) img->pixels;
            for (y=0; y < img->h; ++y) {
                for (x=0; x < img->w; ++x) {
                    al_put_pixel(x, y, al_map_rgb(col->r, col->g, col->b));
                    ++col;
                }
            }

            al_unlock_bitmap(bm);
            delete img;
            return bm;
        }
    }
    return NULL;
}
#endif


#include "gpu_opengl.cpp"
#include <allegro5/allegro_opengl.h>


void screenInit_sys(const Settings* settings, ScreenState* state, int reset) {
    ScreenAllegro* sa;
    const char* gpuError;
#ifdef _WIN32
    // ALLEGRO_OPENGL_3_0 causes al_create_display() to fail on Windows 7 and
    // with Wine.  We check that version 3.3 calls are available below.
    int dflags = ALLEGRO_OPENGL;
#else
    // NOTE: _FORWARD_COMPATIBLE requires al_set_current_opengl_context()
    int dflags = ALLEGRO_OPENGL | ALLEGRO_OPENGL_3_0 |
                 ALLEGRO_OPENGL_FORWARD_COMPATIBLE;
#endif
    int dw = 320 * settings->scale;
    int dh = 200 * settings->scale;

    if (reset) {
        sa = SA;

        al_unregister_event_source(sa->queue, al_get_display_event_source(sa->disp));

        gpu_free(&sa->gpu);
        al_destroy_display(sa->disp);
        sa->disp = NULL;

        ALLEGRO_EVENT_SOURCE* source = al_get_mouse_event_source();
        if (source)
            al_unregister_event_source(sa->queue, source);
    } else {
        xu4.screenSys = sa = new ScreenAllegro;
        xu4.gpu = &sa->gpu;
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
        dflags |= ALLEGRO_FULLSCREEN_WINDOW;
    al_set_new_display_flags(dflags);

    //al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 8, ALLEGRO_REQUIRE);

    sa->disp = al_create_display(dw, dh);
    if (! sa->disp) {
#if 0
        if (dflags & ALLEGRO_FULLSCREEN) {
            // Fullscreen mode is picky about resolutions, so fallback to
            // using a window if it fails.
            al_set_new_display_flags(dflags & ~ALLEGRO_FULLSCREEN);
            sa->disp = al_create_display(dw, dh);
            if (! sa->disp)
                goto fatal;
            xu4.errorMessage = "Fullscreen failed! Try another scale.";
        } else
#endif
            goto fatal;
    }

#ifdef _WIN32
    {
    uint32_t ver = al_get_opengl_version();
    int major = ver>>24;
    int minor = (ver>>16) & 255;
    if (major < 3 || (major == 3 && minor < 3))
        errorFatal("OpenGL 3.3 is required (version %d.%d.%d)",
                   major, minor, (ver>>8) & 255);

    if (! reset) {
        al_set_current_opengl_context(sa->disp);
        if (! gladLoadGL())
            errorFatal("Unable to get OpenGL function addresses");
    }
    }
#endif

    state->displayW = al_get_display_width(sa->disp);
    state->displayH = al_get_display_height(sa->disp);
    state->aspectW  = dw;
    state->aspectH  = dh;
    state->aspectX  = (state->displayW - dw) / 2;
    state->aspectY  = (state->displayH - dh) / 2;

    al_set_window_title(sa->disp, "Ultima IV");  // configService->gameName()

#ifdef __linux__
    {
    ALLEGRO_BITMAP* bm =
        loadBitmapPng("/usr/share/icons/hicolor/48x48/apps/xu4.png");
    if (bm) {
        al_set_display_icon(sa->disp, bm);
        al_destroy_bitmap(bm);
    }
    }
#endif

    // Can settings->gamma be applied?

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

    // NOTE: The GL context is made current after creating the mouse cursors
    // as the context is lost when mucking with bitmaps.
    al_set_current_opengl_context(sa->disp);

    gpuError = gpu_init(&sa->gpu, dw, dh, settings->scale, settings->filter);
    if (gpuError)
        errorFatal("Unable to obtain OpenGL resource (%s)", gpuError);

    sa->refreshRate = 1.0 / settings->screenAnimationFramesPerSecond;
    if (! reset) {
        al_register_event_source(sa->queue, al_get_keyboard_event_source());
    }

    al_register_event_source(sa->queue, al_get_display_event_source(sa->disp));
    return;

fatal:
    errorFatal("Unable to initialize Allegro");
}

void screenDelete_sys() {
    ScreenAllegro* sa = SA;

    gpu_free(&sa->gpu);

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

extern void screenRender();
extern void musicUpdate();

void screenSwapBuffers() {
    musicUpdate();

    CPU_START()
    screenRender();
    al_flip_display();
    CPU_END("ut:")
}

// Private function for Allegro backend.
float screenFrameDuration() {
    return SA->refreshRate;
}

void screenWait(int numberOfAnimationFrames) {
#ifndef GPU_RENDER
    screenUploadToGPU();
#endif

    // Does this wait need to handle world animation or input (e.g. user
    // quits game)?

    assert(numberOfAnimationFrames >= 0);

    screenSwapBuffers();
    al_rest(SA->refreshRate * numberOfAnimationFrames);
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
