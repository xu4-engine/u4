/*
 * $Id$
 */

#include <SDL.h>
#include "u4.h"

#include "event.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "xu4.h"

extern bool verbose, quit;
extern int eventTimerGranularity;

extern uint32_t getTicks();
extern int u4_SDL_InitSubSystem(Uint32 flags);
extern void u4_SDL_QuitSubSystem(Uint32 flags);

enum UserEventCode {
    UC_ScreenRefresh,
    UC_TimedEventMgr
};

unsigned int refresh_callback(unsigned int interval, void *param) {
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = UC_ScreenRefresh;
    event.user.data1 = param;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    return interval;
}

/**
 * Adds an SDL timer event to the message queue.
 */
static unsigned int tem_callback(unsigned int interval, void *param) {
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = UC_TimedEventMgr;
    event.user.data1 = param;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    return interval;
}

static unsigned int tem_instances = 0;

/**
 * Constructs a timed event manager object.
 * Adds a timer callback to the SDL subsystem, which
 * will drive all of the timed events that this object
 * controls.
 */
TimedEventMgr::TimedEventMgr(int i) : baseInterval(i) {
    /* start the SDL timer */
    if (tem_instances == 0) {
        if (u4_SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
            errorFatal("unable to init SDL: %s", SDL_GetError());
    }

    id = static_cast<void*>(SDL_AddTimer(i, &tem_callback, this));
    tem_instances++;
}

/**
 * Destructs a timed event manager object.
 * It removes the callback timer and un-initializes the
 * SDL subsystem if there are no other active TimedEventMgr
 * objects.
 */
TimedEventMgr::~TimedEventMgr() {
    SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
    id = NULL;

    if (tem_instances == 1)
        u4_SDL_QuitSubSystem(SDL_INIT_TIMER);

    if (tem_instances > 0)
        tem_instances--;

    cleanupLists();
}

/**
 * Re-initializes the timer manager to a new timer granularity
 */
void TimedEventMgr::reset(unsigned int interval) {
    baseInterval = interval;
    stop();
    start();
}

void TimedEventMgr::stop() {
    if (id) {
        SDL_RemoveTimer(static_cast<SDL_TimerID>(id));
        id = NULL;
    }
}

void TimedEventMgr::start() {
    if (!id)
        id = static_cast<void*>(SDL_AddTimer(baseInterval, &tem_callback, this));
}

/**
 * Constructs an event handler object.
 */
EventHandler::EventHandler() : timer(eventTimerGranularity), updateScreen(NULL) {
    controllerDone = ended = false;
}

static void handleMouseMotionEvent(const SDL_Event &event) {
    if (!xu4.settings->mouseOptions.enabled)
        return;

    MouseArea *area;
    area = xu4.eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (area)
        screenSetMouseCursor(area->cursor);
    else
        screenSetMouseCursor(MC_DEFAULT);
}

static void handleActiveEvent(const SDL_Event &event, updateScreenCallback updateScreen) {
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and is now being restored
        if (event.active.gain) {
            if (updateScreen)
                (*updateScreen)();
        }
    }
}

static void handleMouseButtonDownEvent(const SDL_Event &event, Controller *controller, updateScreenCallback updateScreen) {
    int button = event.button.button - 1;

    if (!xu4.settings->mouseOptions.enabled)
        return;

    if (button > 2)
        button = 0;
    MouseArea *area = xu4.eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (!area || area->command[button] == 0)
        return;
    controller->keyPressed(area->command[button]);
    if (updateScreen)
        (*updateScreen)();
}

static void handleKeyDownEvent(const SDL_Event &event, Controller *controller, updateScreenCallback updateScreen) {
    int processed;
    int key;

    if (event.key.keysym.unicode != 0)
        key = event.key.keysym.unicode & 0x7F;
    else
        key = event.key.keysym.sym;

    if (event.key.keysym.mod & KMOD_ALT)
#if defined(MACOSX)
        key = U4_ALT + event.key.keysym.sym; // macosx translates alt keys into strange unicode chars
#else
    key += U4_ALT;
#endif
    if (event.key.keysym.mod & KMOD_META)
        key += U4_META;

    if (event.key.keysym.sym == SDLK_UP)
        key = U4_UP;
    else if (event.key.keysym.sym == SDLK_DOWN)
        key = U4_DOWN;
    else if (event.key.keysym.sym == SDLK_LEFT)
        key = U4_LEFT;
    else if (event.key.keysym.sym == SDLK_RIGHT)
        key = U4_RIGHT;
    else if (event.key.keysym.sym == SDLK_BACKSPACE ||
             event.key.keysym.sym == SDLK_DELETE)
        key = U4_BACKSPACE;

#if defined(MACOSX)
    // Mac OS X translates function keys weirdly too
    if ((event.key.keysym.sym >= SDLK_F1) && (event.key.keysym.sym <= SDLK_F15))
        key = U4_FKEY + (event.key.keysym.sym - SDLK_F1);
#endif

    if (verbose)
        printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
               event.key.keysym.unicode,
               event.key.keysym.sym,
               event.key.keysym.mod,
               key);

    /* handle the keypress */
    processed = controller->notifyKeyPressed(key);

    if (processed) {
        if (updateScreen)
            (*updateScreen)();
    }

}

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * While some important event happens (e.g., getting hit by a cannon ball or a spell effect).
 *
 * This method is not expected to handle msec values less than one game frame.
 */
void EventHandler::sleep(unsigned int msec) {
    SDL_Event event;
    uint32_t endTime = getTicks() + msec;
    bool sleeping = true;
    bool redraw = false;

    while (sleeping) {
        do {
            SDL_WaitEvent(&event);
            switch (event.type) {
            default:
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                // Discard the event.
                break;
            case SDL_MOUSEMOTION:
                handleMouseMotionEvent(event);
                break;
            case SDL_ACTIVEEVENT:
                handleActiveEvent(event, xu4.eventHandler->updateScreen);
                break;
            case SDL_USEREVENT:
                if (event.user.code == UC_ScreenRefresh) {
                    redraw = true;
                } else if (event.user.code == UC_TimedEventMgr) {
                    xu4.eventHandler->getTimer()->tick();
                }
                if (getTicks() >= endTime)
                    sleeping = false;
                break;
            case SDL_QUIT:
                xu4.eventHandler->quitGame();
                sleeping = false;
                break;
            }
        } while (SDL_PollEvent(NULL));

        if (redraw) {
            redraw = false;
            screenSwapBuffers();
        }
    }
}

void EventHandler::run() {
    SDL_Event event;
    bool redraw = false;

    if (updateScreen)
        (*updateScreen)();
    screenSwapBuffers();

    while (!ended && !controllerDone) {
        do {
            SDL_WaitEvent(&event);
            switch (event.type) {
            default:
                break;
            case SDL_KEYDOWN:
                handleKeyDownEvent(event, getController(), updateScreen);
                break;

            case SDL_MOUSEBUTTONDOWN:
                handleMouseButtonDownEvent(event, getController(), updateScreen);
                break;

            case SDL_MOUSEMOTION:
                handleMouseMotionEvent(event);
                break;

            case SDL_USEREVENT:
                if (event.user.code == UC_ScreenRefresh)
                    redraw = true;
                else
                    xu4.eventHandler->getTimer()->tick();
                break;

            case SDL_ACTIVEEVENT:
                handleActiveEvent(event, updateScreen);
                break;

            case SDL_QUIT:
                quitGame();
                break;
            }
        } while (SDL_PollEvent(NULL));

        if (redraw) {
            redraw = false;
            screenSwapBuffers();
        }
    }
}

void EventHandler::setScreenUpdate(void (*updateScreen)(void)) {
    this->updateScreen = updateScreen;
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    return SDL_EnableKeyRepeat(delay, interval);
}
