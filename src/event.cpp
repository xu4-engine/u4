/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cctype>
#include <list>

#include "event.h"

#include "context.h"
#include "debug.h"
#include "location.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "textview.h"
#include "xu4.h"

using namespace std;

#ifdef IOS
// Seems that iOS hands-off event loop means we need to fire a bit more slowly.
int eventTimerGranularity = 300;
#else
int eventTimerGranularity = 250;
#endif

void EventHandler::runController(Controller* con) {
    pushController(con);
    run();
    popController();
    setControllerDone(false);
}

/**
 * Waits a given number of milliseconds before continuing
 */
void EventHandler::wait_msecs(unsigned int msecs) {
    int msecs_per_cycle = (1000 / xu4.settings->gameCyclesPerSecond);
    int cycles = msecs / msecs_per_cycle;

    if (cycles > 0) {
        WaitController waitCtrl(cycles);
        xu4.eventHandler->pushController(&waitCtrl);
        waitCtrl.wait();
    }
    // Sleep the rest of the msecs we can't wait for
    EventHandler::sleep(msecs % msecs_per_cycle);
}

/**
 * Waits a given number of game cycles before continuing
 */
void EventHandler::wait_cycles(unsigned int cycles) {
    WaitController waitCtrl(cycles);
    xu4.eventHandler->pushController(&waitCtrl);
    waitCtrl.wait();
}

void EventHandler::setControllerDone(bool done)
{
    controllerDone = done;
#if defined(IOS)
    if (done)
        controllerStopped_helper();
#endif
}     /**< Sets the controller exit flag for the event handler */

/** Returns the current value of the controller exit flag */
bool EventHandler::getControllerDone() {
    return controllerDone;
}

/** Signals that the game should immediately exit. */
void EventHandler::quitGame() {
    ended = true;
    xu4.stage = StageExitGame;
}

TimedEventMgr* EventHandler::getTimer()  { return &timer;}

Controller *EventHandler::pushController(Controller *c) {
    controllers.push_back(c);
    getTimer()->add(&Controller::timerCallback, c->getTimerInterval(), c);
    return c;
}

Controller *EventHandler::popController() {
    if (controllers.empty())
        return NULL;

    Controller *controller = controllers.back();
    getTimer()->remove(&Controller::timerCallback, controller);
    controllers.pop_back();

    return getController();
}

Controller *EventHandler::getController() const {
    if (controllers.empty())
        return NULL;

    return controllers.back();
}

void EventHandler::setController(Controller *c) {
    while (popController() != NULL) {}
    pushController(c);
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


//----------------------------------------------------------------------------


/* TimedEvent functions */
TimedEvent::TimedEvent(TimedEvent::Callback cb, int i, void *d) :
    callback(cb),
    data(d),
    interval(i),
    current(0)
{}

TimedEvent::Callback TimedEvent::getCallback() const    { return callback; }
void *TimedEvent::getData()                             { return data; }

/**
 * Advances the timed event forward a tick.
 * When (current >= interval), then it executes its callback function.
 */
void TimedEvent::tick() {
    if (++current >= interval) {
        (*callback)(data);
        current = 0;
    }
}

/**
 * Returns true if the event queue is locked
 */
bool TimedEventMgr::isLocked() const {
    return locked;
}

void TimedEventMgr::cleanupLists() {
    while(! events.empty()) {
        delete events.front(), events.pop_front();
    }
    while(! deferredRemovals.empty()) {
        delete deferredRemovals.front(), deferredRemovals.pop_front();
    }
}

/**
 * Adds a timed event to the event queue.
 */
void TimedEventMgr::add(TimedEvent::Callback callback, int interval, void *data) {
    events.push_back(new TimedEvent(callback, interval, data));
}

/**
 * Removes a timed event from the event queue.
 */
TimedEventMgr::List::iterator TimedEventMgr::remove(List::iterator i) {
    if (isLocked()) {
        deferredRemovals.push_back(*i);
        return i;
    }
    else {
        delete *i;
        return events.erase(i);
    }
}

void TimedEventMgr::remove(TimedEvent* event) {
    List::iterator i;
    for (i = events.begin(); i != events.end(); i++) {
        if ((*i) == event) {
            remove(i);
            break;
        }
    }
}

void TimedEventMgr::remove(TimedEvent::Callback callback, void *data) {
    List::iterator i;
    for (i = events.begin(); i != events.end(); i++) {
        if ((*i)->getCallback() == callback && (*i)->getData() == data) {
            remove(i);
            break;
        }
    }
}

/**
 * Runs each of the callback functions of the TimedEvents associated with this manager.
 */
void TimedEventMgr::tick() {
    List::iterator i;
    lock();

    for (i = events.begin(); i != events.end(); i++)
        (*i)->tick();

    unlock();

    // Remove events that have been deferred for removal
    for (i = deferredRemovals.begin(); i != deferredRemovals.end(); i++)
        events.remove(*i);
}

void TimedEventMgr::lock()      { locked = true; }
void TimedEventMgr::unlock()    { locked = false; }

void EventHandler::pushMouseAreaSet(MouseArea *mouseAreas) {
    mouseAreaSets.push_front(mouseAreas);
}

void EventHandler::popMouseAreaSet() {
    if (mouseAreaSets.size())
        mouseAreaSets.pop_front();
}

/**
 * Get the currently active mouse area set off the top of the stack.
 */
MouseArea* EventHandler::getMouseAreaSet() const {
    if (mouseAreaSets.size())
        return mouseAreaSets.front();
    else
        return NULL;
}

/**
 * @param maxlen the maximum length of the string
 * @param screenX the screen column where to begin input
 * @param screenY the screen row where to begin input
 * @param accepted_chars a string characters to be accepted for input
 */
ReadStringController::ReadStringController(int maxlen, int screenX, int screenY, const string &accepted_chars) {
    this->maxlen = maxlen;
    this->screenX = screenX;
    this->screenY = screenY;
    this->view = NULL;
    this->accepted = accepted_chars;
}

ReadStringController::ReadStringController(int maxlen, TextView *view, const string &accepted_chars) {
    this->maxlen = maxlen;
    this->screenX = view->getCursorX();
    this->screenY = view->getCursorY();
    this->view = view;
    this->accepted = accepted_chars;
}

bool ReadStringController::keyPressed(int key) {
    int valid = true,
        len = value.length();
    string::size_type pos = string::npos;

    if (key < U4_ALT)
         pos = accepted.find_first_of(key);

    if (pos != string::npos) {
        if (key == U4_BACKSPACE) {
            if (len > 0) {
                /* remove the last character */
                value.erase(len - 1, 1);

                if (view) {
                    view->textAt(screenX + len - 1, screenY, " ");
                    view->setCursorPos(screenX + len - 1, screenY, true);
                } else {
                    screenHideCursor();
                    screenTextAt(screenX + len - 1, screenY, " ");
                    screenSetCursorPos(screenX + len - 1, screenY);
                    screenShowCursor();
                }
            }
        }
        else if (key == '\n' || key == '\r') {
            doneWaiting();
        }
        else if (len < maxlen) {
            /* add a character to the end */
            value += key;

            if (view) {
                view->textAt(screenX + len, screenY, "%c", key);
            } else {
                screenHideCursor();
                screenTextAt(screenX + len, screenY, "%c", key);
                screenSetCursorPos(screenX + len + 1, screenY);
                c->col = len + 1;
                screenShowCursor();
            }
        }
    }
    else valid = false;

    return valid || KeyHandler::defaultHandler(key, NULL);
}

string ReadStringController::get(int maxlen, int screenX, int screenY, EventHandler *eh) {
    if (!eh)
        eh = xu4.eventHandler;

    ReadStringController ctrl(maxlen, screenX, screenY);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

string ReadStringController::get(int maxlen, TextView *view, EventHandler *eh) {
    if (!eh)
        eh = xu4.eventHandler;

    ReadStringController ctrl(maxlen, view);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

ReadIntController::ReadIntController(int maxlen, int screenX, int screenY) : ReadStringController(maxlen, screenX, screenY, "0123456789 \n\r\010") {}

int ReadIntController::get(int maxlen, int screenX, int screenY, EventHandler *eh) {
    if (!eh)
        eh = xu4.eventHandler;

    ReadIntController ctrl(maxlen, screenX, screenY);
    eh->pushController(&ctrl);
    ctrl.waitFor();
    return ctrl.getInt();
}

int ReadIntController::getInt() const {
    return static_cast<int>(strtol(value.c_str(), NULL, 10));
}

ReadChoiceController::ReadChoiceController(const string &choices) {
    this->choices = choices;
}

bool ReadChoiceController::keyPressed(int key) {
    // isupper() accepts 1-byte characters, yet the modifier keys
    // (ALT, SHIFT, ETC) produce values beyond 255
    if ((key <= 0x7F) && (isupper(key)))
        key = tolower(key);

    value = key;

    if (choices.empty() || choices.find_first_of(value) < choices.length()) {
        // If the value is printable, display it
        if (!isspace(key))
            screenMessage("%c", toupper(key));
        doneWaiting();
        return true;
    }

    return false;
}

char ReadChoiceController::get(const string &choices, EventHandler *eh) {
    if (!eh)
        eh = xu4.eventHandler;

    ReadChoiceController ctrl(choices);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

ReadDirController::ReadDirController() {
    value = DIR_NONE;
}

bool ReadDirController::keyPressed(int key) {
    Direction d = keyToDirection(key);
    bool valid = (d != DIR_NONE);

    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        value = DIR_NONE;
        doneWaiting();
        return true;

    default:
        if (valid) {
            value = d;
            doneWaiting();
            return true;
        }
        break;
    }

    return false;
}

WaitController::WaitController(unsigned int c) : Controller(), cycles(c), current(0) {}

void WaitController::timerFired() {
    if (++current >= cycles) {
        current = 0;
        xu4.eventHandler->setControllerDone(true);
    }
}

bool WaitController::keyPressed(int key) {
    return true;
}

void WaitController::wait() {
    Controller_startWait();
}

void WaitController::setCycles(int c) {
    cycles = c;
}


//----------------------------------------------------------------------------


KeyHandler::KeyHandler(Callback func, void *d, bool asyncronous) :
    handler(func),
    async(asyncronous),
    data(d)
{}

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
    //case U4_META + 'Q':   // Handle shifted version?
    //case U4_META + 'X':
#endif
    case U4_ALT + 'x': /* Alt+x */
    //case U4_ALT + 'X':    // Handle shifted version?
#if defined(WIN32)
    case U4_ALT + U4_FKEY + 3:
#endif
        xu4.eventHandler->quitGame();
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
