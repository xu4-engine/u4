/*
 * event_allegro.cpp
 */

#include <allegro5/allegro.h>
#include "u4.h"
#include "event.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"

extern bool verbose;
extern int eventTimerGranularity;
extern ALLEGRO_EVENT_QUEUE* sa_queue;
extern ALLEGRO_TIMER* sa_refreshTimer;


#define CTIMER(id)  ((ALLEGRO_TIMER*) id)

/**
 * Constructs a timed event manager object, which will drive all of the
 * timed events that this object controls.
 */
TimedEventMgr::TimedEventMgr(int msec) : baseInterval(msec) {
    ALLEGRO_TIMER* timer = al_create_timer(double(msec) * 0.001);
    id = timer;
    al_register_event_source(sa_queue, al_get_timer_event_source(timer));
    al_start_timer(timer);
}

/**
 * Destructs a timed event manager object.
 */
TimedEventMgr::~TimedEventMgr() {
    al_destroy_timer(CTIMER(id));   // Automatically unregisters.
    id = NULL;
}

/**
 * Re-initializes the timer manager to a new timer granularity
 */
void TimedEventMgr::reset(unsigned int interval) {
    baseInterval = interval;

    ALLEGRO_TIMER* at = CTIMER(id);
    al_stop_timer(at);
    al_set_timer_speed(at, double(interval) * 0.001);
    al_start_timer(at);
}

void TimedEventMgr::stop() {
    al_stop_timer(CTIMER(id));
}

void TimedEventMgr::start() {
    al_start_timer(CTIMER(id));
}


/**
 * Constructs an event handler object.
 */
EventHandler::EventHandler() : timer(eventTimerGranularity), updateScreen(NULL) {
}

static void handleMouseMotionEvent(int x, int y) {
    if (!settings.mouseOptions.enabled)
        return;

    MouseArea* area = eventHandler->mouseAreaForPoint(x, y);
    if (area)
        screenSetMouseCursor(area->cursor);
    else
        screenSetMouseCursor(MC_DEFAULT);
}

/*
static void handleActiveEvent(const ALLEGRO_EVENT* event, updateScreenCallback updateScreen) {
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and is now being restored
        if (event.active.gain) {
            if (updateScreen)
                (*updateScreen)();
            screenRedrawScreen();
        }
    }
}
*/

static void handleMouseButtonDownEvent(const ALLEGRO_EVENT* event, Controller *controller, updateScreenCallback updateScreen) {
    if (!settings.mouseOptions.enabled)
        return;

    int button = event->mouse.button - 1;
    if (button > 2)
        button = 0;
    MouseArea *area = eventHandler->mouseAreaForPoint(event->mouse.x, event->mouse.y);
    if (!area || area->command[button] == 0)
        return;
    controller->keyPressed(area->command[button]);
    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();
}

static void handleKeyDownEvent(const ALLEGRO_EVENT* event, Controller *controller, updateScreenCallback updateScreen) {
    int processed;
    int key;

    switch( event->keyboard.keycode ) {
        case ALLEGRO_KEY_UP:
            key = U4_UP;
            break;
        case ALLEGRO_KEY_DOWN:
            key = U4_DOWN;
            break;
        case ALLEGRO_KEY_LEFT:
            key = U4_LEFT;
            break;
        case ALLEGRO_KEY_RIGHT:
            key = U4_RIGHT;
            break;
        case ALLEGRO_KEY_BACKSPACE:
        case ALLEGRO_KEY_DELETE:
            key = U4_BACKSPACE;
            break;
        default:
            if (event->keyboard.unichar > 0)
                key = event->keyboard.unichar & 0x7F;
            else
                key = event->keyboard.keycode;

            if (event->keyboard.modifiers & ALLEGRO_KEYMOD_ALT)
                key += U4_ALT;
            else if (event->keyboard.modifiers & ALLEGRO_KEYMOD_LWIN)
                key += U4_META;
            break;
    }

    if (verbose) {
        printf("key event: unicode = %d, sym = %d, mod = %d; translated = %d\n",
               event->keyboard.unichar,
               event->keyboard.keycode,
               event->keyboard.modifiers,
               key);
    }

    /* handle the keypress */
    processed = controller->notifyKeyPressed(key);
    if (processed) {
        if (updateScreen)
            (*updateScreen)();
        screenRedrawScreen();
    }
}

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * While some important event happens (e.g., getting hit by a cannon ball or a spell effect).
 */
void EventHandler::sleep(unsigned int msec) {
    // Make this static so that all instance stop. (e.g., sleep calling sleep).
    static bool stopUserInput = true;
    ALLEGRO_EVENT event;
    bool redraw = false;

    // Start a timer for the amount of time we want to sleep from user input.
    ALLEGRO_TIMER* sleepTimer = al_create_timer(double(msec) * 0.001);
    al_register_event_source(sa_queue, al_get_timer_event_source(sleepTimer));
    al_start_timer(sleepTimer);

    stopUserInput = true;
    while (stopUserInput) {
        do {
            al_wait_for_event(sa_queue, &event);
            switch (event.type) {
            default:
                // Discard any key & button events.
                // TODO: But handle Alt+x Alt+q keys...
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                handleMouseMotionEvent(event.mouse.x, event.mouse.y);
                break;
            /*
            case SDL_ACTIVEEVENT:
                handleActiveEvent(event, updateScreen);
                break;
            */
            case ALLEGRO_EVENT_TIMER:
                if (event.timer.source == sleepTimer) {
                    // NOTE: This will terminate all nested sleep calls.
                    stopUserInput = false;
                } else if (event.timer.source == sa_refreshTimer) {
                    redraw = true;
                } else {
                    eventHandler->getTimer()->tick();
                }
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                EventHandler::quitGame();
                stopUserInput = false;
                break;
            }
        } while (! al_is_event_queue_empty(sa_queue));

        if (redraw) {
            redraw = false;
            //printf( "KR EventHandler updateScreen\n" );
            screenRedrawScreen();
        }
    }

    al_destroy_timer(sleepTimer);   // Automatically unregisters.
}

void EventHandler::run() {
    ALLEGRO_EVENT event;
    bool redraw = false;

    if (updateScreen)
        (*updateScreen)();
    screenRedrawScreen();

    while (!ended && !controllerDone) {
        do {
            al_wait_for_event(sa_queue, &event);
            switch (event.type) {
            default:
                break;
            //case ALLEGRO_EVENT_KEY_DOWN:
            case ALLEGRO_EVENT_KEY_CHAR:
                handleKeyDownEvent(&event, getController(), updateScreen);
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonDownEvent(&event, getController(), updateScreen);
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                handleMouseMotionEvent(event.mouse.x, event.mouse.y);
                break;
            case ALLEGRO_EVENT_TIMER:
                if (event.timer.source == sa_refreshTimer)
                    redraw = true;
                else
                    eventHandler->getTimer()->tick();
                break;
            /*
            case SDL_ACTIVEEVENT:
                handleActiveEvent(event, updateScreen);
                break;
            */
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                EventHandler::quitGame();
                break;
#if 0
            // For mobile devices...
            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
#endif
            }
        } while (! al_is_event_queue_empty(sa_queue));

        if (redraw) {
            redraw = false;
            //printf( "KR EventHandler updateScreen\n" );
            screenRedrawScreen();
        }
    }
}

void EventHandler::setScreenUpdate(void (*updateFunc)(void)) {
    updateScreen = updateFunc;
}

/**
 * Returns true if the queue does not contain any timer events.
 * (Used to skip screen refresh in game.cpp & intro.cpp)
 */
bool EventHandler::timerQueueEmpty() {
    ALLEGRO_EVENT event;
    // There is no Allegro equivalent to SDL_PeepEvents so we can only look
    // at one event, not the entire queue.
    if (al_peek_next_event(sa_queue, &event)) {
        if (event.type == ALLEGRO_EVENT_TIMER)
            return false;
    }
    return true;
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    //return SDL_EnableKeyRepeat(delay, interval);
    return 0;
}
