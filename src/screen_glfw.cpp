/*
 * XU4 - GLFW Screen Interface
 * Copyright (C) 2024  Karl Robillard
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"
#include "error.h"
#include "event.h"
#include "gpu_opengl.h"
#include "image.h"
#include "settings.h"
#include "screen.h"
#include "u4.h"
#include "xu4.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <GLFW/glfw3.h>

#define MAX_CURSORS 4

struct ScreenGLView {
    GLFWwindow* view;
    Controller* waitCon;
    updateScreenCallback update;
    GLFWcursor* fwCursor[MAX_CURSORS];
    MouseCursor currentCursor;
    int winPos[2];      // Position used when going from fullscreen to window.
    bool winFullscreen;
#if 0
    int     joyId;
    uint8_t joyButton[8];
    float   joyAxis[2];
    ClockMs joyDPadTime[2];
    uint8_t joyDPadKey[2];
#endif
    OpenGLResources gpu;
};

#define SGL ((ScreenGLView*) xu4.screenSys)


#define CURSORSIZE 24
#define XPMSIZE    32
#include "cursors.h"

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

static void _loadCursors(GLFWcursor** fwCursor)
{
#if 0
    GLFWimage gi;
    ImageInfo info;

    if (screen_loadCursors(&info)) {
        if (info.rectCount > 0) {
            int count = info.rectCount;
            if (count > MAX_CURSORS)
                count = MAX_CURSORS;

            gi.width = info.img.w;
            const ImageRect* ir = info.rect;
            for (int i = 0; i < count; ++i) {
                gi.height = ir->height;
                gi.pixels = (unsigned char*)
                            (info.img.pixels + info.img.w * ir->y);

                fwCursor[i] = glfwCreateCursor(&gi, ir->hotX, ir->hotY);
                ++ir;
            }
        }
        image_free(&info);
    }
#else
    static const uint8_t hotXY[2*MAX_CURSORS] = { 0,6,  6,0,  18,6,  6,18 };
    const uint8_t* hot = hotXY;
    GLFWimage gi;
    Image32 img;
    image32_allocPixels(&img, CURSORSIZE, CURSORSIZE);

    gi.width  = img.w;
    gi.height = img.h;
    gi.pixels = (unsigned char*) img.pixels;

#define CREATE_CURSOR(I, PM) \
    _copyCursor(&img, PM, 0); \
    fwCursor[I] = glfwCreateCursor(&gi, hot[0], hot[1]); \
    hot += 2

    // Match MouseCursor order.
    CREATE_CURSOR(0, w_xpm);
    CREATE_CURSOR(1, n_xpm);
    CREATE_CURSOR(2, e_xpm);
    CREATE_CURSOR(3, s_xpm);

    image32_freePixels(&img);
#endif
}

#if defined(__linux__)
extern Image* loadImage_png(U4FILE *file);

static void _setWindowIcon(GLFWwindow* view, const char* filename) {
    U4FILE* fp = u4fopen_stdio(filename);
    if (fp) {
        Image* img = loadImage_png(fp);
        u4fclose(fp);
        if (img) {
            GLFWimage gi;
            gi.width  = img->w;
            gi.height = img->h;
            gi.pixels = (unsigned char*) img->pixels;

            glfwSetWindowIcon(view, 1, &gi);
            delete img;
        }
    }
}
#endif


#include "gpu_opengl.cpp"


static void keyHandler(GLFWwindow* win, int token, int scancode, int action, int mods)
{
    static const char shiftNum_US[] = ")!@#$%^&*(";
    int key;
    (void) win;
    (void) scancode;

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    switch (token) {
        case GLFW_KEY_ESCAPE:
            key = U4_ESC;
            break;
        case GLFW_KEY_ENTER:
            key = U4_ENTER;
            break;
        case GLFW_KEY_TAB:
            key = U4_TAB;
            break;
        case GLFW_KEY_UP:
            key = U4_UP;
            break;
        case GLFW_KEY_DOWN:
            key = U4_DOWN;
            break;
        case GLFW_KEY_LEFT:
            key = U4_LEFT;
            break;
        case GLFW_KEY_RIGHT:
            key = U4_RIGHT;
            break;
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
            key = U4_BACKSPACE;
            break;
        case GLFW_KEY_PAUSE:
            key = U4_PAUSE;
            break;
        case GLFW_KEY_KP_ENTER:
            key = U4_KEYPAD_ENTER;
            break;
        default:
            // glfwSetCharCallback() is provided for text input.
            if (token >= GLFW_KEY_A && token <= GLFW_KEY_Z) {
                key = token;
                if (mods & GLFW_MOD_CONTROL) {
                    // Map to ASCII Control Code.
                    key -= GLFW_KEY_A - 1;
                } else if (! (mods & GLFW_MOD_SHIFT))
                    key += 'a' - 'A';
            }
            else if (token >= GLFW_KEY_0 && token <= GLFW_KEY_9) {
                if (mods & GLFW_MOD_SHIFT)
                    key = shiftNum_US[token - GLFW_KEY_0];
                else
                    key = token;
            }
            else if (token >= GLFW_KEY_F1 && token <= GLFW_KEY_F12)
                key = U4_FKEY + (token - GLFW_KEY_F1);
            else if (token <= GLFW_KEY_GRAVE_ACCENT)
                key = token;
            else
                return;
            break;
    }

    if (mods & GLFW_MOD_ALT)
        key += U4_ALT;
    else if (mods & GLFW_MOD_SUPER)
        key += U4_META;

#ifdef DEBUG
    xu4.eventHandler->recordKey(key);
#endif
    if (xu4.verbose)
        printf("key event: token %d, mod 0x%x; translated %d\n", token, mods, key);

    /* handle the keypress */
    if (SGL->waitCon->notifyKeyPressed(key)) {
        updateScreenCallback updateScreen = SGL->update;
        if (updateScreen)
            (*updateScreen)();
    }
}

static inline void dispatchEvent(InputEvent* ie)
{
    SGL->waitCon->inputEvent(ie);
}

static void mouseMotionHandler(GLFWwindow* win, double x, double y)
{
    InputEvent ie;
    (void) win;

    ie.type = IE_MOUSE_MOVE;
    ie.state = 0;
    ie.x = (int16_t) x;
    ie.y = (int16_t) y;

    dispatchEvent(&ie);
}

static void mouseButtonHandler(GLFWwindow* win, int button, int action,
                               int mods)
{
    static const uint16_t _mouseButtonMap[3] = {
        CMOUSE_LEFT, CMOUSE_RIGHT, CMOUSE_MIDDLE
    };
    InputEvent ie;
    double xpos, ypos;

    ie.type = (action == GLFW_PRESS) ? IE_MOUSE_PRESS : IE_MOUSE_RELEASE;
    if (button > GLFW_MOUSE_BUTTON_3)
        ie.n = button + 1;
    else
        ie.n = _mouseButtonMap[button];
    ie.state = 0;

    glfwGetCursorPos(win, &xpos, &ypos);
    ie.x = (int16_t) xpos;
    ie.y = (int16_t) ypos;

    dispatchEvent(&ie);
}

static void scrollHandler(GLFWwindow* win, double x, double y)
{
    InputEvent ie;
    (void) win;

    ie.type = IE_MOUSE_WHEEL;
    ie.n = 0;
    ie.x = (int16_t) x;
    ie.y = (int16_t) y;
    ie.state = 0;

    dispatchEvent(&ie);
}

extern int screenInitState(ScreenState*, const Settings*, int dw, int dh);

void screenInit_sys(const Settings* settings, ScreenState* state, int reset)
{
    ScreenGLView* ss;
    GLFWmonitor* monitor;
    const GLFWvidmode* mode;
    const char* gpuError;
    int scale = settings->scale;
    int dw = U4_SCREEN_W * scale;
    int dh = ((settings->filter == FILTER_POINT_43) ? 240 : U4_SCREEN_H) * scale;
    char title[MOD_NAME_LIMIT];

    xu4.config->gameTitle(title);

    if (reset) {
        ss = SGL;
        gpu_free(&ss->gpu);

        if (settings->fullscreen) {
            monitor = glfwGetPrimaryMonitor();
            mode    = glfwGetVideoMode(monitor);
            dw = mode->width;
            dh = mode->height;

            if (! ss->winFullscreen) {
                ss->winFullscreen = true;
                // Save current window position if transitioning.
                glfwGetWindowPos(ss->view, ss->winPos, ss->winPos+1);
                glfwSetWindowMonitor(ss->view, monitor, 0, 0, dw, dh, mode->refreshRate);
            }
        } else {
            if (ss->winFullscreen) {
                ss->winFullscreen = false;
                glfwSetWindowMonitor(ss->view, NULL, ss->winPos[0], ss->winPos[1],
                                     dw, dh, GLFW_DONT_CARE);
            } else {
                glfwSetWindowSize(ss->view, dw, dh);
            }
        }

        glfwSetWindowTitle(ss->view, title);
    } else {
        xu4.screenSys = ss = (ScreenGLView*) calloc(1, sizeof(ScreenGLView));

        if (! glfwInit()) {
            errorFatal("Unable to initialize GLFW");
        }

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#if defined(USE_GLES)
        const int glVersion = 0x301;
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

        ss->winFullscreen = settings->fullscreen;
        if (settings->fullscreen) {
            // Borderless full screen window.
            monitor = glfwGetPrimaryMonitor();
            mode    = glfwGetVideoMode(monitor);
            glfwWindowHint(GLFW_RED_BITS,     mode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS,   mode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS,    mode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

            ss->winPos[0] = (mode->width  - dw) / 2;
            ss->winPos[1] = (mode->height - dh) / 2;
            dw = mode->width;
            dh = mode->height;
        } else {
            monitor = NULL;
        }

        ss->view = glfwCreateWindow(dw, dh, title, monitor, NULL);
        if (! ss->view) {
            errorFatal("Unable to create GLFW window");
        }

        glfwSetKeyCallback(ss->view, keyHandler);
        glfwSetCursorPosCallback(ss->view, mouseMotionHandler);
        glfwSetMouseButtonCallback(ss->view, mouseButtonHandler);
        glfwSetScrollCallback(ss->view, scrollHandler);

#if defined(__linux__)
        _setWindowIcon(ss->view, "/usr/share/icons/hicolor/48x48/apps/xu4.png");
#endif
        _loadCursors(ss->fwCursor);

#if 0
        if ((settings->options & SOPT_USE_JOYSTICK) &&
            glfwJoystickPresent(GLFW_JOYSTICK_1)) {
            ss->joyId = GLFW_JOYSTICK_1;
            memset(ss->joyButton, 0, sizeof(ss->joyButton));
            ss->joyAxis[0] = ss->joyAxis[1] = 0.0f;
        } else {
            ss->joyId = GLFW_JOYSTICK_1 - 1;
        }
#endif
    }

    //----------------------------------

    // NOTE: Calling glfwGetWindowSize() here does not return the values
    // passed to glfwSetWindowMonitor() above.  So we just use the requested
    // dimensions and hope for the best.
#if 0
    glfwGetWindowSize(ss->view, &state->displayW, &state->displayH);
    printf("KR dim %d,%d (%d,%d) %d\n",
            dw, dh, state->displayW, state->displayH, glfwGetError(NULL));
#endif
    scale = screenInitState(state, settings, dw, dh);

    /* enable or disable the mouse cursor */
    screenShowMouseCursor(settings->mouseOptions.enabled);

    glfwMakeContextCurrent(ss->view);
#ifdef _WIN32
    if (! reset) {
        if (! gladLoadGL())
            errorFatal("Unable to get OpenGL function addresses");
    }
#endif

    xu4.gpu = &ss->gpu;
    gpuError = gpu_init(&ss->gpu, state->aspectW, state->aspectH, scale,
                        settings->filter);
    if (gpuError)
        errorFatal("Unable to obtain OpenGL resource (%s)", gpuError);
}

void screenDelete_sys()
{
    ScreenGLView* ss = SGL;

    gpu_free(&ss->gpu);

    glfwDestroyWindow(ss->view);
    glfwTerminate();    // Destroys all fwCursor for us.

    free(ss);
    xu4.screenSys = NULL;
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
    glfwIconifyWindow(SGL->view);
}

//#define CPU_TEST
#include "cpuCounter.h"

extern void screenRender();

void screenSwapBuffers()
{
    CPU_START()
    screenRender();
    glfwSwapBuffers(SGL->view);
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

void screenSetMouseCursor(MouseCursor cursor)
{
    ScreenGLView* sg = SGL;
    if (cursor != sg->currentCursor) {
        GLFWcursor* fwc = (cursor == MC_DEFAULT) ? NULL
                                                 : sg->fwCursor[cursor - 1];
        glfwSetCursor(sg->view, fwc);
        sg->currentCursor = cursor;
    }
}

void screenShowMouseCursor(bool visible)
{
    ScreenGLView* sg = SGL;
    glfwSetInputMode(sg->view, GLFW_CURSOR,
                     visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

#if 0
static void joystick_handleEvents(ScreenGLView* sg)
{
    static const uint8_t joyButtonKey[4] = { HKEY_E, HKEY_A, HKEY_J, HKEY_T };
    InputEvent ie;
    int count;
    Stage* st = stage_current();
    const ClockMs repeatDelay = 300;

    ie.type = IE_KEY_PRESS;
    ie.key.unicode = 0;
    ie.key.modifier = 0;

    const float* axes = glfwGetJoystickAxes(sg->joyId, &count);
    if (count > 1) {
        if (sg->joyAxis[0] != axes[0]) {
            sg->joyAxis[0] = axes[0];

            if (axes[0] < -0.9f) {
                ie.key.uid = HKEY_LeftArrow;
dpad0:
                sg->joyDPadKey[0] = ie.key.uid;
                sg->joyDPadTime[0] = gs.rclock + repeatDelay;
                st->dispatch(st, &ie);
            } else if (axes[0] > 0.9f) {
                ie.key.uid = HKEY_RightArrow;
                goto dpad0;
            } else {
                sg->joyDPadTime[0] = -1;
            }
        }
        if (sg->joyAxis[1] != axes[1]) {
            sg->joyAxis[1] = axes[1];

            if (axes[1] < -0.9f) {
                ie.key.uid = HKEY_UpArrow;
dpad1:
                sg->joyDPadKey[1] = ie.key.uid;
                sg->joyDPadTime[1] = gs.rclock + repeatDelay;
                st->dispatch(st, &ie);
            } else if (axes[1] > 0.9f) {
                ie.key.uid = HKEY_DownArrow;
                goto dpad1;
            } else {
                sg->joyDPadTime[1] = -1;
            }
        }

        for (int i = 0; i < 2; ++i) {
            if (gs.rclock > sg->joyDPadTime[i]) {
                sg->joyDPadTime[i] += repeatDelay;
                ie.key.uid = sg->joyDPadKey[i];
                st->dispatch(st, &ie);
            }
        }
    }

    const uint8_t* btn = glfwGetJoystickButtons(sg->joyId, &count);
    if (count) {
        if (count > 4)
            count = 4;
        for (int i = 0; i < count; ++i) {
            if (sg->joyButton[i] != btn[i] && btn[i] == GLFW_PRESS) {
                ie.key.uid = joyButtonKey[i];
                st->dispatch(st, &ie);
            }
        }
        memcpy(sg->joyButton, btn, count);
    }
}
#endif

/*
 * \param waitCon  Input events are passed to this controller if not NULL.
 *                 Otherwise EventHandler::getController() (the currently
 *                 active one) will be used.
 */
void EventHandler::handleInputEvents(Controller* waitCon,
                                     updateScreenCallback update)
{
    ScreenGLView* ss = SGL;
    Controller* prevCon = ss->waitCon;
    updateScreenCallback prevUpdate = ss->update;

    ss->waitCon = waitCon ? waitCon : getController();
    ss->update  = update;

#if 0
    if (ss->joyId >= GLFW_JOYSTICK_1)
        joystick_handleEvents(ss);
#endif

    glfwPollEvents();

    ss->waitCon = prevCon;
    ss->update  = prevUpdate;

    if (glfwWindowShouldClose( ss->view ))
        quitGame();
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    return 0;
}
