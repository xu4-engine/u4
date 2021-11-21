/*
 * event.cpp
 */

#include <assert.h>
#include <cctype>
#include <cstring>
#include <list>

#include "event.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "location.h"
#include "savegame.h"
#include "screen.h"
#include "textview.h"
#include "xu4.h"

/**
 * Constructs the event handler object.
 */
EventHandler::EventHandler(int gameCycleDuration, int frameDuration) :
    timerInterval(gameCycleDuration),
    frameInterval(frameDuration),
    runRecursion(0),
    updateScreen(NULL)
{
    controllerDone = ended = false;
    anim_init(&flourishAnim, 64, NULL, NULL);
    anim_init(&fxAnim, 32, NULL, NULL);
#ifdef DEBUG
    recordFP = -1;
    recordMode = 0;
#endif
}

EventHandler::~EventHandler() {
#ifdef DEBUG
    endRecording();
#endif
    anim_free(&flourishAnim);
    anim_free(&fxAnim);
}

void EventHandler::setTimerInterval(int msecs) {
    timerInterval = msecs;
}

void EventHandler::runController(Controller* con) {
    if (con->present()) {
        pushController(con);
        run();
        popController();
        setControllerDone(false);
        con->conclude();
    }
}

/** Sets the controller exit flag for the event handler */
void EventHandler::setControllerDone(bool done)
{
    controllerDone = done;
#if defined(IOS)
    if (done)
        controllerStopped_helper();
#endif
}

/** Returns the current value of the controller exit flag */
bool EventHandler::getControllerDone() {
    return controllerDone;
}

/** Signals that the game should immediately exit. */
void EventHandler::quitGame() {
    ended = true;
    xu4.stage = StageExitGame;
}

TimedEventMgr* EventHandler::getTimer() { return &timedEvents; }

Controller *EventHandler::pushController(Controller *c) {
    controllers.push_back(c);
    int interval = c->getTimerInterval();
    if (interval)
        timedEvents.add(&Controller::timerCallback, interval, c);
    return c;
}

Controller *EventHandler::popController() {
    if (controllers.empty())
        return NULL;

    Controller* con = controllers.back();
    if (con->getTimerInterval())
        timedEvents.remove(&Controller::timerCallback, con);

    controllers.pop_back();
    if (con->deleteOnPop())
        delete con;

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

void EventHandler::setScreenUpdate(void (*updateFunc)(void)) {
    updateScreen = updateFunc;
}

#include "support/getTicks.c"

/**
 * Delays program execution for the specified number of milliseconds.
 * This doesn't actually stop events, but it stops the user from interacting
 * while some important event happens (e.g., getting hit by a cannon ball or
 * a spell effect).
 *
 * This method is not expected to handle msec values of less than the display
 * refresh interval.
 *
 * \return true if game should exit.
 */
bool EventHandler::wait_msecs(unsigned int msec) {
    Controller waitCon;     // Base controller consumes key events.
    EventHandler* eh = xu4.eventHandler;
    uint32_t waitTime = getTicks() + msec;
    uint32_t now, elapsed;

    assert(eh->runRecursion);

    while (! eh->ended) {
        eh->handleInputEvents(&waitCon, NULL);
#ifdef DEBUG
        int key;
        while ((key = eh->recordedKey()))
            waitCon.notifyKeyPressed(key);
        eh->recordTick();
#endif
        if (eh->runTime >= eh->timerInterval) {
            eh->runTime -= eh->timerInterval;
            eh->timedEvents.tick();
        }
        eh->runTime += eh->frameInterval;

        screenSwapBuffers();

        now = getTicks();
        elapsed = now - eh->realTime;
        eh->realTime = now;
        if (now >= waitTime)    // Break only after realTime is updated.
            break;
        if (elapsed+2 < eh->frameInterval)
            msecSleep(eh->frameInterval - elapsed);
    }

    return eh->ended;
}

/*
 * Execute the game with a deterministic loop until the current controller
 * is done or the game exits.
 *
 * \return true if game should exit.
 */
bool EventHandler::run() {
    uint32_t now, elapsed;

    if (updateScreen)
        (*updateScreen)();

    if (! runRecursion) {
        runTime = 0;
        realTime = getTicks();
    }
    ++runRecursion;

    while (! ended && ! controllerDone) {
        handleInputEvents(getController(), updateScreen);
#ifdef DEBUG
        int key;
        while ((key = recordedKey())) {
            if (getController()->notifyKeyPressed(key) && updateScreen)
                (*updateScreen)();
        }
        recordTick();
#endif
        if (runTime >= timerInterval) {
            runTime -= timerInterval;
            timedEvents.tick();
        }
        runTime += frameInterval;

        screenSwapBuffers();

        now = getTicks();
        elapsed = now - realTime;
        realTime = now;
        if (elapsed+2 < frameInterval)
            msecSleep(frameInterval - elapsed);
    }

    --runRecursion;
    return ended;
}

#ifdef DEBUG
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#define close   _close
#define read    _read
#define write   _write
#else
#include <unistd.h>
#endif

#ifndef CDI32
#include "cdi.h"
#endif

#define RECORD_CDI  CDI32(0xDA,0x7A,0x4F,0xC0)
#define HDR_SIZE    8

enum RecordMode {
    MODE_DISABLED,
    MODE_RECORD,
    MODE_REPLAY,
};

enum RecordCommand {
    RECORD_NOP,
    RECORD_KEY,
    RECORD_KEY1,
    RECORD_END = 0xff
};

struct RecordKey {
    uint8_t op, key;
    uint16_t delay;
};

bool EventHandler::beginRecording(const char* file, uint32_t seed) {
    uint32_t head[2];

    recordClock = recordLast = 0;
    recordMode = MODE_DISABLED;

    if (recordFP >= 0)
        close(recordFP);
#ifdef _WIN32
    recordFP = _open(file, _O_WRONLY | _O_CREAT, _S_IWRITE);
#else
    recordFP = open(file, O_WRONLY | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    if (recordFP < 0)
        return false;

    head[0] = RECORD_CDI;
    head[1] = seed;
    if (write(recordFP, head, HDR_SIZE) != HDR_SIZE)
        return false;
    recordMode = MODE_RECORD;
    return true;
}

/**
 * Stop either recording or playback.
 */
void EventHandler::endRecording() {
    if (recordFP >= 0) {
        if (recordMode == MODE_RECORD) {
            char op = RECORD_END;
            write(recordFP, &op, 1);
        }
        close(recordFP);
        recordFP = -1;
        recordMode = MODE_DISABLED;
    }
}

//void EventHandler::recordMouse(int x, int y, int button) {}

void EventHandler::recordKey(int key) {
    if (recordMode == MODE_RECORD) {
        RecordKey rec;
        rec.op    = (key > 0xff) ? RECORD_KEY1 : RECORD_KEY;
        rec.key   = key & 0xff;
        rec.delay = recordClock - recordLast;

        recordLast = recordClock;
        write(recordFP, &rec, 4);
    }
}

/**
 * Update for both recording and playback modes.
 *
 * \return XU4 key code or zero if no key was pressed. When recording, a zero
 *         is always returned.
 */
int EventHandler::recordedKey() {
    int key = 0;
    if (recordMode == MODE_REPLAY) {
        if (replayKey) {
            if (recordClock >= recordLast) {
                key = replayKey;
                replayKey = 0;
            }
        } else {
            RecordKey rec;
            if (read(recordFP, &rec, 4) == 4 &&
                (rec.op == RECORD_KEY || rec.op == RECORD_KEY1)) {
                int fullKey = rec.key;
                if (rec.op == RECORD_KEY1)
                    fullKey |= 0x100;

                if (rec.delay)
                    replayKey = fullKey;
                else
                    key = fullKey;
                recordLast = recordClock + rec.delay;
            } else {
                endRecording();
            }
        }
    }

    return key;
}

/**
 * Begin playback from recorded input file.
 *
 * \return Random seed or zero if the recording file could not be opened.
 */
uint32_t EventHandler::replay(const char* file) {
    uint32_t head[2];

    recordClock = recordLast = 0;
    recordMode = MODE_DISABLED;
    replayKey = 0;

    if (recordFP >= 0)
        close(recordFP);
#ifdef _WIN32
    recordFP = _open(file, _O_RDONLY);
#else
    recordFP = open(file, O_RDONLY);
#endif
    if (recordFP < 0)
        return 0;
    if (read(recordFP, head, HDR_SIZE) != HDR_SIZE || head[0] != RECORD_CDI)
        return 0;

    recordMode = MODE_REPLAY;
    return head[1];
}
#endif


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
 * Destructs a timed event manager object.
 */
TimedEventMgr::~TimedEventMgr() {
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

    // Lock the event list so it cannot be modified during iteration.
    locked = true;

    for (i = events.begin(); i != events.end(); i++)
        (*i)->tick();

    locked = false;

    // Remove events that have been deferred for removal
    for (i = deferredRemovals.begin(); i != deferredRemovals.end(); i++)
        events.remove(*i);
}

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

#define TEST_BIT(bits, c)   (bits[c >> 3] & 1 << (c & 7))
#define MAX_BITS    128

static void addCharBits(uint8_t* bits, const char* cp) {
    int c;
    while ((c = *cp++))
        bits[c >> 3] |= 1 << (c & 7);
}

/**
 * @param maxlen the maximum length of the string
 * @param screenX the screen column where to begin input
 * @param screenY the screen row where to begin input
 * @param textView  Pointer to associated TextView or NULL.
 * @param accepted_chars a string characters to be accepted for input
 */
ReadStringController::ReadStringController(int maxlen, int screenX, int screenY, TextView* textView, const char* accepted_chars) {
    // Set 0-9, A-Z, a-z, backspace, new line, carriage return, & space.
    static const uint8_t alphaNumBitset[MAX_BITS/8] = {
        0x00, 0x25, 0x00, 0x00, 0x01, 0x00, 0xFF, 0x03,
        0xFE, 0xFF, 0xFF, 0x07, 0xFE, 0xFF, 0xFF, 0x07
    };

    this->maxlen = maxlen;
    this->screenX = screenX;
    this->screenY = screenY;
    view = textView;

    if (accepted_chars) {
        memset(accepted, 0, MAX_BITS/8);
        addCharBits(accepted, accepted_chars);
    } else {
        memcpy(accepted, alphaNumBitset, MAX_BITS/8);
    }
}

bool ReadStringController::keyPressed(int key) {
    bool valid = true;

    if (key < MAX_BITS && TEST_BIT(accepted, key)) {
        int len = value.length();

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
        else if (key == U4_ESC) {
            value.erase(0, value.length());
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

string ReadStringController::get(int maxlen, int screenX, int screenY, const char* extraChars) {
    ReadStringController ctrl(maxlen, screenX, screenY);
    if (extraChars)
        addCharBits(ctrl.accepted, extraChars);

    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

string ReadStringController::get(int maxlen, TextView *view, const char* extraChars) {
    ReadStringController ctrl(maxlen, view->getCursorX(), view->getCursorY(),
                              view);
    if (extraChars)
        addCharBits(ctrl.accepted, extraChars);

    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

ReadIntController::ReadIntController(int maxlen, int screenX, int screenY) : ReadStringController(maxlen, screenX, screenY, NULL, "0123456789 \n\r\010") {}

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
        if (key > ' ')
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
        if (c && c->location) {
            const Location* loc = c->location;
            const Tile* tile = loc->map->tileTypeAt(loc->coords, WITH_OBJECTS);
            printf("x = %d, y = %d, level = %d, tile = %d (%s)\n",
                    loc->coords.x, loc->coords.y, loc->coords.z,
                    xu4.config->usaveIds()->ultimaId( MapTile(tile->id) ),
                    tile->nameStr());
        }
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
