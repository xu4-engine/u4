/*
 * screen_glv.cpp
 */

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "gpu_opengl.h"
#include "image.h"
#include "settings.h"
#include "screen.h"
#include "u4.h"
#include "xu4.h"

#include <assert.h>
#include <glv.h>
#include <glv_keys.h>

#define CURSORSIZE 20
#define XPMSIZE    32
#include "cursors.h"

struct ScreenGLView {
    GLView* view;
    Controller* waitCon;
    updateScreenCallback update;
    int currentCursor;
    bool cursorShown;
    OpenGLResources gpu;
};

#define SA  ((ScreenGLView*) xu4.screenSys)

#ifndef ANDROID
/**
 * Copy xpm into an Image32.
 */
static void _copyCursor(Image32* img, const char* const xpm[], int destY)
{
    RGBA white, black, empty;
    RGBA* dp;
    int row, col;
    //int hot_x, hot_y;

    rgba_set(white, 255, 255, 255, 255);
    rgba_set(black, 0, 0, 0, 255);
    rgba_set(empty, 0, 0, 0, 0);

    dp = (RGBA*) (img->pixels + (img->w * destY));

    for (row=0; row < CURSORSIZE; row++) {
        for (col=0; col < CURSORSIZE; col++) {
            switch (xpm[4+row][col]) {
                case 'X':
                    *dp++ = black;
                    break;
                case '.':
                    *dp++ = white;
                    break;
                case ' ':
                    *dp++ = empty;
                    break;
            }
        }
    }
    //sscanf(xpm[4+XPMSIZE], "%d,%d", &hot_x, &hot_y);
}

static int _loadCursors(GLView* view)
{
    static const int cursorCount = 4;
    static const short areas[6*cursorCount] = {
        // x, y, w, h, hotX, hotY
        0, 0, 20,20,  0, 6,
        0,20, 20,20,  6, 0,
        0,40, 20,20, 18, 6,
        0,60, 20,20,  6,18
    };
    int ok;
    Image32 img;
    image32_allocPixels(&img, CURSORSIZE, CURSORSIZE*cursorCount);

    // Match MouseCursor order.
    _copyCursor(&img, w_xpm,  0);
    _copyCursor(&img, n_xpm, 20);
    _copyCursor(&img, e_xpm, 40);
    _copyCursor(&img, s_xpm, 60);

    ok = glv_loadCursors(view, areas, cursorCount,
                         (const uint8_t*) img.pixels, img.w, 0);
    image32_freePixels(&img);
    return ok;
}
#endif

#if defined(__linux__) && ! defined(ANDROID)
extern Image* loadImage_png(U4FILE *file);

static void _setX11Icon(GLView* view, const char* filename) {
    U4FILE* uf = u4fopen_stdio(filename);
    if (uf) {
        Image* img = loadImage_png(uf);
        u4fclose(uf);
        if (img) {
            glv_setIcon(view, img->w, img->h, (const uint8_t*) img->pixels, 0);
            delete img;
        }
    }
}
#endif


#include "gpu_opengl.cpp"


static void handleKeyDownEvent(const GLViewEvent* event,
                               Controller *controller,
                               updateScreenCallback updateScreen) {
    int key;
    int keycode = event->code;

    switch (keycode) {
        case KEY_Up:
            key = U4_UP;
            break;
        case KEY_Down:
            key = U4_DOWN;
            break;
        case KEY_Left:
            key = U4_LEFT;
            break;
        case KEY_Right:
            key = U4_RIGHT;
            break;
        case KEY_Back_Space:
        case KEY_Delete:
            key = U4_BACKSPACE;
            break;
        case KEY_Pause:
            key = U4_PAUSE;
            break;
        case KEY_KP_Enter:
            key = U4_KEYPAD_ENTER;
            break;
        default:
            if (keycode >= KEY_F1 && keycode <= KEY_F12)
                key = U4_FKEY + (keycode - KEY_F1);
            else
                key = KEY_ASCII(event);

            if (event->state & GLV_MASK_ALT)
                key += U4_ALT;
            else if (event->state & GLV_MASK_CMD)
                key += U4_META;
            /*
            else if (event->state & GLV_MASK_CTRL)
                key += U4_CTRL:
            */
            break;
    }

#ifdef DEBUG
    xu4.eventHandler->recordKey(key);
#endif

    if (xu4.verbose) {
        printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
               KEY_ASCII(event),
               event->code,
               event->state,
               key);
    }

    /* handle the keypress */
    if (controller->notifyKeyPressed(key)) {
        if (updateScreen)
            (*updateScreen)();
    }
}

static void eventHandler(GLView* view, GLViewEvent* event)
{
    InputEvent ie;
    ScreenGLView* sa = (ScreenGLView*) view->user;
    Controller* controller = sa->waitCon;

    switch (event->type)
    {
        case GLV_EVENT_KEY_DOWN:
            if (! sa->waitCon)
                controller = xu4.eventHandler->getController();
            handleKeyDownEvent(event, controller, sa->update);
            break;

#ifdef ANDROID
        case GLV_EVENT_KEY_UP:
            if (event->code == KEY_Back)
                xu4.eventHandler->quitGame();
            break;
#endif

        case GLV_EVENT_BUTTON_DOWN:
            ie.type = IE_MOUSE_PRESS;
mouse_button:
            // GLV button order matches ControllerMouseButton.
            ie.n = event->code;
mouse_pos:
            ie.x = event->x;
            ie.y = event->y;
            ie.state = 0;
            if (! sa->waitCon)
                controller = xu4.eventHandler->getController();
            controller->inputEvent(&ie);
            break;

        case GLV_EVENT_BUTTON_UP:
            ie.type = IE_MOUSE_RELEASE;
            goto mouse_button;

        case GLV_EVENT_MOTION:
            ie.type = IE_MOUSE_MOVE;
            goto mouse_pos;

        case GLV_EVENT_WHEEL:
            ie.n = 0;
            ie.type = IE_MOUSE_WHEEL;
            goto mouse_pos;

        case GLV_EVENT_EXPOSE:
            xu4.eventHandler->expose();
            break;

        case GLV_EVENT_CLOSE:
            xu4.eventHandler->quitGame();
            break;
/*
#ifdef ANDROID
        case GLV_EVENT_APP:
            switch (event->code) {
                case 1:             // APP_CMD_INIT_WINDOW
                    //resetGraphics();
                    break;
                case 2:             // APP_CMD_TERM_WINDOW
                    break;
            }
            break;
#endif
*/
        default:
            break;
    }
}

extern int screenInitState(ScreenState*, const Settings*, int dw, int dh);

void screenInit_sys(const Settings* settings, ScreenState* state, int reset) {
    ScreenGLView* sa;
    const char* gpuError;
    int scale = settings->scale;
#ifdef ANDROID
    const int glVersion = 0x301;
    const int attrib = GLV_ATTRIB_DOUBLEBUFFER;
#elif defined(USE_GLES)
    const int glVersion = 0x301;
    const int attrib = GLV_ATTRIB_DOUBLEBUFFER | GLV_ATTRIB_ES;
#else
    const int glVersion = 0x303;
    const int attrib = GLV_ATTRIB_DOUBLEBUFFER;
#endif

    if (reset) {
        sa = SA;

        gpu_free(&sa->gpu);
    } else {
        xu4.screenSys = sa = new ScreenGLView;
        xu4.gpu = &sa->gpu;

        memset(sa, 0, sizeof(ScreenGLView));
        sa->currentCursor = MC_DEFAULT;
        sa->cursorShown = true;

        sa->view = glv_create(attrib, glVersion);
        if (! sa->view)
            goto fatal;

        sa->view->user = sa;
        glv_setEventHandler(sa->view, eventHandler);

#if defined(__linux__) && ! defined(ANDROID)
        _setX11Icon(sa->view, "/usr/share/icons/hicolor/48x48/apps/xu4.png");
#endif
#ifndef ANDROID
        if (! _loadCursors(sa->view))
            errorWarning("Unable to load window cursors");
#endif
    }

    {
    char buf[MOD_NAME_LIMIT];
    glv_setTitle(sa->view, xu4.config->gameTitle(buf));
    }

    {
    GLViewMode mode;
    mode.id = settings->fullscreen ? GLV_MODEID_FULL_WINDOW
                                   : GLV_MODEID_FIXED_WINDOW;
    mode.width  = U4_SCREEN_W * scale;
    mode.height = ((settings->filter == FILTER_POINT_43) ? 240 : U4_SCREEN_H) * scale;

    glv_changeMode(sa->view, &mode);
    }

    scale = screenInitState(state, settings, sa->view->width, sa->view->height);

    /* enable or disable the mouse cursor */
    screenShowMouseCursor(settings->mouseOptions.enabled);

    glv_makeCurrent(sa->view);
#ifdef _WIN32
    if (! reset) {
        if (! gladLoadGL())
            errorFatal("Unable to get OpenGL function addresses");
    }
#endif

    gpuError = gpu_init(&sa->gpu, state->aspectW, state->aspectH, scale,
                        settings->filter);
    if (gpuError)
        errorFatal("Unable to obtain OpenGL resource (%s)", gpuError);

    return;

fatal:
    errorFatal("Unable to initialize GLView");
}

void screenDelete_sys() {
    ScreenGLView* sa = SA;

    gpu_free(&sa->gpu);

    glv_destroy(sa->view);
    sa->view = NULL;

    delete sa;
    xu4.screenSys = NULL;
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    glv_iconify(SA->view);
}

//#define CPU_TEST
#include "support/cpuCounter.h"

extern void screenRender();

void screenSwapBuffers() {
    CPU_START()
    screenRender();
    glv_swapBuffers(SA->view);
    CPU_END("ut:")
}


extern void msecSleep(uint32_t);

void screenWait(int numberOfAnimationFrames) {
#ifndef GPU_RENDER
    screenUploadToGPU();
#endif

    // Does this wait need to handle world animation or input (e.g. user
    // quits game)?

    assert(numberOfAnimationFrames >= 0);

    screenSwapBuffers();
    msecSleep((1000 * numberOfAnimationFrames) /
              xu4.settings->screenAnimationFramesPerSecond);
}

void screenSetMouseCursor(MouseCursor cursor) {
#ifndef ANDROID
    ScreenGLView* sa = SA;
    if (cursor != sa->currentCursor) {
        sa->currentCursor = cursor;
        glv_setCursor(sa->view, (cursor == MC_DEFAULT) ? GLV_CURSOR_ARROW
                                                       : cursor - 1);
    }
#endif
}

void screenShowMouseCursor(bool visible) {
    ScreenGLView* sa = SA;
    if (visible != sa->cursorShown) {
        sa->cursorShown = visible;
        glv_showCursor(sa->view, (int) visible);
    }
}

/*
 * \param waitCon  Input events are passed to this controller if not NULL.
 *                 Otherwise EventHandler::getController() (the currently
 *                 active one) will be used.
 */
void EventHandler::handleInputEvents(Controller* waitCon,
                                     updateScreenCallback update) {
    ScreenGLView* sa = SA;
    Controller* prevCon = sa->waitCon;
    updateScreenCallback prevUpdate = sa->update;

    sa->waitCon = waitCon;
    sa->update  = update;

    glv_handleEvents(sa->view);

    sa->waitCon = prevCon;
    sa->update  = prevUpdate;
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    //return SDL_EnableKeyRepeat(delay, interval);
    return 0;
}
