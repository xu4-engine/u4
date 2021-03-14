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

KeyHandler::KeyHandler(Callback func, void *d, bool asyncronous) :
    handler(func),
    async(asyncronous),
    data(d)
{}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int KeyHandler::setKeyRepeat(int delay, int interval) {
    return SDL_EnableKeyRepeat(delay, interval);
}

/**
 * Handles any and all keystrokes.
 * Generally used to exit the application, switch applications,
 * minimize, maximize, etc.
 */
bool KeyHandler::globalHandler(int key) {
    switch(key) {
#if defined(MACOSX)
    case U4_META + 'q': /* Cmd+q */
    case U4_META + 'x': /* Cmd+x */
#endif
    case U4_ALT + 'x': /* Alt+x */
#if defined(WIN32)
    case U4_ALT + U4_FKEY + 3:
#endif
        quit = true;
        EventHandler::end();
        return true;
    default: return false;
    }
}

/**
 * A default key handler that should be valid everywhere
 */
bool KeyHandler::defaultHandler(int key, void *data) {
    bool valid = true;

    switch (key) {
    case '`':
        if (c && c->location)
            printf("x = %d, y = %d, level = %d, tile = %d (%s)\n", c->location->coords.x, c->location->coords.y, c->location->coords.z, c->location->map->translateToRawTileIndex(*c->location->map->tileAt(c->location->coords, WITH_OBJECTS)), c->location->map->tileTypeAt(c->location->coords, WITH_OBJECTS)->getName().c_str());
        break;
    default:
        valid = false;
        break;
    }

    return valid;
}

/**
 * A key handler that ignores keypresses
 */
bool KeyHandler::ignoreKeys(int key, void *data) {
    return true;
}

/**
 * Handles a keypress.
 * First it makes sure the key combination is not ignored
 * by the current key handler. Then, it passes the keypress
 * through the global key handler. If the global handler
 * does not process the keystroke, then the key handler
 * handles it itself by calling its handler callback function.
 */ 
bool KeyHandler::handle(int key) {
    bool processed = false;
    if (!isKeyIgnored(key)) {
        processed = globalHandler(key);
        if (!processed)
            processed = handler(key, data);
    }
    
    return processed;
}

/**
 * Returns true if the key or key combination is always ignored by xu4
 */
bool KeyHandler::isKeyIgnored(int key) {
    switch(key) {
    case U4_RIGHT_SHIFT:
    case U4_LEFT_SHIFT:
    case U4_RIGHT_CTRL:
    case U4_LEFT_CTRL:
    case U4_RIGHT_ALT:
    case U4_LEFT_ALT:
    case U4_RIGHT_META:
    case U4_LEFT_META:
    case U4_TAB:
        return true;
    default: return false;
    }
}

bool KeyHandler::operator==(Callback cb) const {
    return (handler == cb) ? true : false;        
}

KeyHandlerController::KeyHandlerController(KeyHandler *handler) {
    this->handler = handler;
}

KeyHandlerController::~KeyHandlerController() {
    delete handler;
}

bool KeyHandlerController::keyPressed(int key) {
    ASSERT(handler != NULL, "key handler must be initialized");
    return handler->handle(key);
}

KeyHandler *KeyHandlerController::getKeyHandler() {
    return handler;
}

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
 * Adds a key handler to the stack.
 */ 
void EventHandler::pushKeyHandler(KeyHandler kh) {
    KeyHandler *new_kh = new KeyHandler(kh);
    KeyHandlerController *khc = new KeyHandlerController(new_kh);
    pushController(khc);
}

/**
 * Pops a key handler off the stack.
 * Returns a pointer to the resulting key handler after
 * the current handler is popped.
 */ 
void EventHandler::popKeyHandler() {
    if (controllers.empty())
        return;

    popController();
}

/**
 * Returns a pointer to the current key handler.
 * Returns NULL if there is no key handler.
 */ 
KeyHandler *EventHandler::getKeyHandler() const {
    if (controllers.empty())
        return NULL;

    KeyHandlerController *khc = dynamic_cast<KeyHandlerController *>(controllers.back());
    ASSERT(khc != NULL, "EventHandler::getKeyHandler called when controller wasn't a keyhandler");
    if (khc == NULL)
        return NULL;

    return khc->getKeyHandler();
}

/**
 * Eliminates all key handlers and begins stack with new handler.
 * This pops all key handlers off the stack and adds
 * the key handler provided to the stack, making it the
 * only key handler left. Use this function only if you
 * are sure the key handlers in the stack are disposable.
 */ 
void EventHandler::setKeyHandler(KeyHandler kh) {
    while (popController() != NULL) {}
    pushKeyHandler(kh);
}

MouseArea* EventHandler::mouseAreaForPoint(int x, int y) {
    int i;
    MouseArea *areas = getMouseAreaSet();

    if (!areas)
        return NULL;

    for (i = 0; areas[i].npoints != 0; i++) {
        if (screenPointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return NULL;
}
