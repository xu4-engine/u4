/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <SDL.h>
#include "u4.h"

#include "event.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"

extern bool verbose, quit;
extern int eventTimerGranularity;

extern int u4_SDL_InitSubSystem(Uint32 flags);
extern void u4_SDL_QuitSubSystem(Uint32 flags);

/**
 * Constructs a timed event manager object.
 * Adds a timer callback to the SDL subsystem, which
 * will drive all of the timed events that this object
 * controls.
 */
TimedEventMgr::TimedEventMgr(int i) : baseInterval(i) {
    /* start the SDL timer */
    if (instances == 0) {
        if (u4_SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
            errorFatal("unable to init SDL: %s", SDL_GetError());
    }

    id = static_cast<void*>(SDL_AddTimer(i, &TimedEventMgr::callback, this));
    instances++;
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

    if (instances == 1)
        u4_SDL_QuitSubSystem(SDL_INIT_TIMER);

    if (instances > 0)
        instances--;
}

/**
 * Adds an SDL timer event to the message queue.
 */
unsigned int TimedEventMgr::callback(unsigned int interval, void *param) {
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = param;
    event.user.data2 = NULL;
    SDL_PushEvent(&event);

    return interval;
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
        id = static_cast<void*>(SDL_AddTimer(baseInterval, &TimedEventMgr::callback, this));
}

/**
 * Constructs an event handler object.
 */
EventHandler::EventHandler() : timer(eventTimerGranularity), updateScreen(NULL) {
}

static void handleMouseMotionEvent(const SDL_Event &event) {
    if (!settings.mouseOptions.enabled)
        return;

    MouseArea *area;
    area = eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
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
            screenRedrawScreen();
        }
    }
}

static void handleMouseButtonDownEvent(const SDL_Event &event, Controller *controller, updateScreenCallback updateScreen) {
    int button = event.button.button - 1;

    if (!settings.mouseOptions.enabled)
        return;

    if (button > 2)
        button = 0;
    MouseArea *area = eventHandler->mouseAreaForPoint(event.button.x, event.button.y);
    if (!area || area->command[button] == 0)
        return;
    controller->keyPressed(area->command[button]);
    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();
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
        screenRedrawScreen();
    }

}

static Uint32 sleepTimerCallback(Uint32 interval, void *) {
    SDL_Event stopEvent;
    stopEvent.type = SDL_USEREVENT;
    stopEvent.user.code = 1;
    stopEvent.user.data1 = 0;
    stopEvent.user.data2 = 0;
    SDL_PushEvent(&stopEvent);
    return 0;
}

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * While some important event happens (e.g., getting hit by a cannon ball or a spell effect).
 */
void EventHandler::sleep(unsigned int usec) {
    // Start a timer for the amount of time we want to sleep from user input.
    static bool stopUserInput = true; // Make this static so that all instance stop. (e.g., sleep calling sleep).
    SDL_TimerID sleepingTimer = SDL_AddTimer(usec, sleepTimerCallback, 0);

    stopUserInput = true;
    while (stopUserInput) {
        SDL_Event event;
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
            handleActiveEvent(event, eventHandler->updateScreen);
            break;
        case SDL_USEREVENT:
            if (event.user.code == 0) {
                eventHandler->getTimer()->tick();
            } else if (event.user.code == 1) {
                SDL_RemoveTimer(sleepingTimer);
                stopUserInput = false;
            }
            break;
        case SDL_QUIT:
            ::exit(0);
            break;
        }
    }
}

void EventHandler::run() {
    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();

    while (!ended && !controllerDone) {
        SDL_Event event;

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
            eventHandler->getTimer()->tick();
            break;

        case SDL_ACTIVEEVENT:
            handleActiveEvent(event, updateScreen);
            break;

        case SDL_QUIT:
            ::exit(0);
            break;
        }
    }

}

void EventHandler::setScreenUpdate(void (*updateScreen)(void)) {
    this->updateScreen = updateScreen;
}

/**
 * Returns true if the queue is empty of events that match 'mask'.
 */
 bool EventHandler::timerQueueEmpty() {
    SDL_Event event;

    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
        return false;
    else
        return true;
}


/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    return SDL_EnableKeyRepeat(delay, interval);
}
