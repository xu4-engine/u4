/*
 * event_allegro.cpp
 */

#include "screen_allegro.h"
#include "event.h"
#include "screen.h"
#include "settings.h"
#include "xu4.h"

extern uint32_t getTicks();

extern bool verbose;


#define CTIMER(id)  ((ALLEGRO_TIMER*) id)

/**
 * Constructs a timed event manager object, which will drive all of the
 * timed events that this object controls.
 */
TimedEventMgr::TimedEventMgr(int msec) : baseInterval(msec) {
    ALLEGRO_TIMER* timer = al_create_timer(double(msec) * 0.001);
    id = timer;
    al_register_event_source(SA->queue, al_get_timer_event_source(timer));
    al_start_timer(timer);
}

/**
 * Destructs a timed event manager object.
 */
TimedEventMgr::~TimedEventMgr() {
    al_destroy_timer(CTIMER(id));   // Automatically unregisters.
    id = NULL;
    cleanupLists();
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

static void handleMouseMotionEvent(int x, int y) {
    if (! xu4.settings->mouseOptions.enabled)
        return;

    MouseArea* area = xu4.eventHandler->mouseAreaForPoint(x, y);
    screenSetMouseCursor(area ? area->cursor : MC_DEFAULT);
}

/*
static void handleActiveEvent(const ALLEGRO_EVENT* event, updateScreenCallback updateScreen) {
    if (event.active.state & SDL_APPACTIVE) {
        // application was previously iconified and is now being restored
        if (event.active.gain) {
            if (updateScreen)
                (*updateScreen)();
        }
    }
}
*/

static void handleMouseButtonDownEvent(EventHandler* handler, const ALLEGRO_EVENT* event, Controller *controller, updateScreenCallback updateScreen) {
    static const uint8_t buttonMap[4] = { 0, 0, 2, 1 };
    MouseArea* area;
    int xu4Button, keyCmd;

    if (! xu4.settings->mouseOptions.enabled)
        return;

    area = handler->mouseAreaForPoint(event->mouse.x, event->mouse.y);
    if (area) {
        // Map ALLEGRO button to what MouseArea uses.
        xu4Button = buttonMap[event->mouse.button & 3];

        keyCmd = area->command[xu4Button];
        if (keyCmd) {
            controller->keyPressed(keyCmd);
            if (updateScreen)
                (*updateScreen)();
        }
    }
}

static void handleKeyDownEvent(const ALLEGRO_EVENT* event, Controller *controller, updateScreenCallback updateScreen) {
    int processed;
    int key;
    int keycode = event->keyboard.keycode;

    switch( keycode ) {
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
            else if (keycode >= ALLEGRO_KEY_F1 && keycode <= ALLEGRO_KEY_F12)
                key = U4_FKEY + (keycode - ALLEGRO_KEY_F1);
            else
                key = 0;

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
    }
}

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * while some important event happens (e.g., getting hit by a cannon ball or
 * a spell effect).
 *
 * This method is not expected to handle msec values of less than the display
 * refresh interval.
 */
void EventHandler::wait_msecs(unsigned int msec) {
    Controller waitCon;     // Base controller consumes key events.
    ALLEGRO_EVENT event;
    EventHandler* eh = xu4.eventHandler;
    ScreenAllegro* sa = SA;
    uint32_t endTime = getTicks() + msec;
    bool sleeping = true;
    bool redraw = false;

    // Using getTicks() rather than an ALLEGRO_TIMER to avoid the overhead
    // to create/destroy & register/unregister it.

    eh->pushController(&waitCon);

    while (sleeping && ! eh->ended) {
        do {
            al_wait_for_event(sa->queue, &event);
            switch (event.type) {
            // Discard all key & button events but globals (e.g. Alt+x).
            case ALLEGRO_EVENT_KEY_CHAR:
                handleKeyDownEvent(&event, &waitCon, NULL);
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
                if (event.timer.source == sa->refreshTimer) {
                    redraw = true;
                } else {
                    eh->getTimer()->tick();
                }
                if (getTicks() >= endTime)
                    sleeping = false;
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                eh->quitGame();
                sleeping = false;
                break;
            }
        } while (! al_is_event_queue_empty(sa->queue));

        if (redraw) {
            redraw = false;
            screenSwapBuffers();
        }
    }

    eh->popController();
}

void EventHandler::run() {
    ALLEGRO_EVENT event;
    ScreenAllegro* sa = SA;
    bool redraw = false;

    if (updateScreen)
        (*updateScreen)();
    screenSwapBuffers();

    if (! sa->runRecursion)
        al_start_timer(sa->refreshTimer);
    ++sa->runRecursion;

    while (!ended && !controllerDone) {
        do {
            al_wait_for_event(sa->queue, &event);
            switch (event.type) {
            default:
                break;
            //case ALLEGRO_EVENT_KEY_DOWN:
            case ALLEGRO_EVENT_KEY_CHAR:
                handleKeyDownEvent(&event, getController(), updateScreen);
                break;
            case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonDownEvent(this, &event, getController(), updateScreen);
                break;
            case ALLEGRO_EVENT_MOUSE_AXES:
                handleMouseMotionEvent(event.mouse.x, event.mouse.y);
                break;
            case ALLEGRO_EVENT_TIMER:
                if (event.timer.source == sa->refreshTimer)
                    redraw = true;
                else
                    getTimer()->tick();
                break;
            /*
            case SDL_ACTIVEEVENT:
                handleActiveEvent(event, updateScreen);
                break;
            */
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                quitGame();
                break;
#if 0
            // For mobile devices...
            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
            case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
#endif
            }
        } while (! al_is_event_queue_empty(sa->queue));

        if (redraw) {
            redraw = false;
            screenSwapBuffers();
        }
    }

    --sa->runRecursion;
    if (! sa->runRecursion)
        al_stop_timer(sa->refreshTimer);
}

void EventHandler::setScreenUpdate(void (*updateFunc)(void)) {
    updateScreen = updateFunc;
}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int delay, int interval) {
    //return SDL_EnableKeyRepeat(delay, interval);
    return 0;
}
