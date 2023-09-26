/*
 * event.cpp
 */

#include <cassert>
#include <cctype>
#include <cstring>
#include <list>

#include "event.h"

#include "config.h"
#include "context.h"
#include "location.h"
#include "savegame.h"
#include "screen.h"
#include "sound.h"
#include "textview.h"
#include "u4.h"
#include "xu4.h"

using std::string;

static void frameSleepInit(FrameSleep* fs, int frameDuration) {
    fs->frameInterval = frameDuration;
    fs->realTime = 0;
    for (int i = 0; i < 8; ++i)
        fs->ftime[i] = frameDuration;
    fs->ftimeSum = frameDuration * 8;
    fs->ftimeIndex = 0;
    fs->fsleep = frameDuration - 6;
}

/**
 * Constructs the event handler object.
 */
EventHandler::EventHandler(int gameCycleDuration, int frameDuration) :
    timerInterval(gameCycleDuration),
    runRecursion(0),
    updateScreen(NULL)
{
    controllerDone = ended = paused = false;
    anim_init(&flourishAnim, 64, NULL, NULL);
    anim_init(&fxAnim, 32, NULL, NULL);
    frameSleepInit(&fs, frameDuration);

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

const MouseArea* EventHandler::mouseAreaForPoint(int x, int y) const {
    int i;
    const MouseArea *areas = getMouseAreaSet();

    if (!areas)
        return NULL;

    screenPointToMouseArea(&x, &y);
    for (i = 0; areas[i].npoints != 0; i++) {
        if (pointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return NULL;
}

void EventHandler::setScreenUpdate(void (*updateFunc)(void)) {
    updateScreen = updateFunc;
}

#define GPU_PAUSE

void EventHandler::togglePause() {
    if (paused) {
        paused = false;
    } else {
#ifdef GPU_PAUSE
        // Don't pause if Game Browser (or other LAYER_TOP_MENU user) is open.
        if (screenLayerUsed(LAYER_TOP_MENU))
            return;
#endif
        paused = true;
        // Set ended to break the run loop.  runPause() resets it so that
        // the game does not end.
        ended = true;
    }
}

void EventHandler::expose() {
    if (paused)
        screenSwapBuffers();
}

#include "support/getTicks.c"

#ifdef GPU_PAUSE
#include "gpu.h"
#include "gui.h"

static void renderPause(ScreenState* ss, void* data)
{
    gpu_drawGui(xu4.gpu, GPU_DLIST_GUI, WID_NONE, 0);
}
#endif

// Return true if game should end.
bool EventHandler::runPause() {
    Controller waitCon;

    soundSuspend(1);
    screenSetMouseCursor(MC_DEFAULT);
#ifdef GPU_PAUSE
    static uint8_t pauseGui[] = {
        LAYOUT_V, BG_COLOR_CI, 128,
        MARGIN_V_PER, 42, FONT_VSIZE, 38, LABEL_DT_S,
        LAYOUT_END
    };
    const void* guiData[1];
    guiData[0] = "\x13\xAPaused";
    TxfDrawState ds;
    ds.fontTable = screenState()->fontTable;
    float* attr = gui_layout(GPU_DLIST_GUI, NULL, &ds, pauseGui, guiData);
    gpu_endTris(xu4.gpu, GPU_DLIST_GUI, attr);

    screenSetLayer(LAYER_TOP_MENU, renderPause, this);
#else
    screenTextAt(16, 12, "( Paused )");
    screenUploadToGPU();
#endif
    screenSwapBuffers();

    ended = false;
    do {
        msecSleep(333);
        handleInputEvents(&waitCon, NULL);
    } while (paused && ! ended);

#ifdef GPU_PAUSE
    screenSetLayer(LAYER_TOP_MENU, NULL, NULL);
#endif
    soundSuspend(0);
    return ended;
}

/*
 * Return non-zero if waitTime has been reached or passed.
 */
static int frameSleep(FrameSleep* fs, uint32_t waitTime) {
    uint32_t now;
    int32_t elapsed, elapsedLimit, frameAdjust;
    int i;

    now = getTicks();
    elapsed = now - fs->realTime;
    fs->realTime = now;

    elapsedLimit = fs->frameInterval * 8;
    if (elapsed > elapsedLimit)
        elapsed = elapsedLimit;

    i = fs->ftimeIndex;
    fs->ftimeSum += elapsed - fs->ftime[i];
    fs->ftime[i] = elapsed;
    fs->ftimeIndex = i ? i - 1 : FS_SAMPLES - 1;

    frameAdjust = int(fs->frameInterval) - fs->ftimeSum / FS_SAMPLES;
#if 0
    // Adjust by 1 msec.
    if (frameAdjust > 0) {
        if (fs->fsleep < fs->frameInterval - 1)
            ++fs->fsleep;
    } else if (frameAdjust < 0) {
        if (fs->fsleep)
            --fs->fsleep;
    }
#else
    // Adjust by 2 msec.
    if (frameAdjust > 0) {
        if (frameAdjust > 2)
            frameAdjust = 2;
        int sa = int(fs->fsleep) + frameAdjust;
        int limit = fs->frameInterval - 1;
        fs->fsleep = (sa > limit) ? limit : sa;
    } else if (frameAdjust < 0) {
        if (frameAdjust < -2)
            frameAdjust = -2;
        int sa = int(fs->fsleep) + frameAdjust;
        fs->fsleep = (sa < 0) ? 0 : sa;
    }
#endif

    if (waitTime && now >= waitTime)
        return 1;
#if 0
    printf("KR fsleep %d ela:%d avg:%d adj:%d\n",
           fs->fsleep, elapsed, fs->ftimeSum / FS_SAMPLES, frameAdjust);
#endif
    if (fs->fsleep)
        msecSleep(fs->fsleep);
    return 0;
}

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
        eh->runTime += eh->fs.frameInterval;

        screenSwapBuffers();
        if (frameSleep(&eh->fs, waitTime))
            break;
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
    if (updateScreen)
        (*updateScreen)();

    if (! runRecursion) {
        runTime = 0;
        fs.realTime = getTicks();
    }
    ++runRecursion;

resume:
    while (! ended && ! controllerDone) {
        handleInputEvents(NULL, updateScreen);
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
        runTime += fs.frameInterval;

        screenSwapBuffers();
        frameSleep(&fs, 0);
    }

    if (paused && ! runPause())
        goto resume;

    --runRecursion;
    return ended;
}

//----------------------------------------------------------------------------

#ifdef DEBUG
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
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

void EventHandler::pushMouseAreaSet(const MouseArea *mouseAreas) {
    mouseAreaSets.push_front(mouseAreas);
}

void EventHandler::popMouseAreaSet() {
    if (mouseAreaSets.size())
        mouseAreaSets.pop_front();
}

/**
 * Get the currently active mouse area set off the top of the stack.
 */
const MouseArea* EventHandler::getMouseAreaSet() const {
    if (mouseAreaSets.size())
        return mouseAreaSets.front();
    else
        return NULL;
}

//----------------------------------------------------------------------------

/**
 * A controller to read a string, terminated by the enter key.
 */
class ReadStringController : public WaitableController<string> {
public:
    ReadStringController(int maxlen, int screenX, int screenY,
                         TextView* view = NULL,
                         const char* accepted_chars = NULL);

    virtual bool keyPressed(int key);

#ifdef IOS
    void setValue(const string &utf8StringValue) {
        value = utf8StringValue;
    }
#endif

protected:
    int maxlen, screenX, screenY;
    TextView *view;
    uint8_t accepted[16];   // Character bitset.

    friend EventHandler;
};

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

static void soundInvalidInput() {
    soundPlay(SOUND_BLOCKED);
}

bool ReadStringController::keyPressed(int key) {
    if (key < MAX_BITS && TEST_BIT(accepted, key)) {
        int len = value.length();

        if (key == U4_BACKSPACE) {
            if (len > 0) {
                /* remove the last character */
                value.erase(len - 1, 1);

                if (view) {
                    view->textAt(screenX + len - 1, screenY, " ");
                    view->setCursorPos(screenX + len - 1, screenY);
                } else {
                    screenHideCursor();
                    screenTextAt(screenX + len - 1, screenY, " ");
                    screenSetCursorPos(screenX + len - 1, screenY);
                    screenShowCursor();
                }
            } else
                soundInvalidInput();
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
                view->textAtFmt(screenX + len, screenY, "%c", key);
                view->setCursorPos(screenX + len + 1, screenY);
            } else {
                screenHideCursor();
                screenTextAt(screenX + len, screenY, "%c", key);
                screenSetCursorPos(screenX + len + 1, screenY);
                c->col = len + 1;
                screenShowCursor();
            }
        }
        else
            soundInvalidInput();
        return true;
    } else {
        bool valid = EventHandler::defaultKeyHandler(key);
        if (! valid)
            soundInvalidInput();
        return valid;
    }
}

//----------------------------------------------------------------------------

/**
 * A controller to read a single key from a provided list.
 */
class ReadChoiceController : public WaitableController<int> {
public:
    ReadChoiceController(const string &choices);
    virtual bool keyPressed(int key);

protected:
    string choices;
};

ReadChoiceController::ReadChoiceController(const string &choices) {
    this->choices = choices;
}

bool ReadChoiceController::keyPressed(int key) {
    // isupper() accepts 1-byte characters, yet the modifier keys
    // (ALT, SHIFT, ETC) produce values beyond 255
    if ((key <= 0x7F) && (isupper(key)))
        key = tolower(key);

    if (choices.empty() || choices.find_first_of(key) < choices.length()) {
        // If the value is printable, display it
        const ScreenState* ss = screenState();
        if (ss->cursorVisible && key > ' ' && key <= 0x7F)
            screenShowChar(toupper(key), ss->cursorX, ss->cursorY);
        value = key;
        doneWaiting();
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------

/**
 * A controller to read a direction enter with the arrow keys.
 */
class ReadDirController : public WaitableController<Direction> {
public:
    ReadDirController();
    virtual bool keyPressed(int key);
};

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

class AnyKeyController : public Controller {
public:
    void wait();
    void waitTimeout();
    virtual bool keyPressed(int key);
    virtual bool inputEvent(const InputEvent*);
    virtual void timerFired();
};

void AnyKeyController::wait() {
    timerInterval = 0;
    xu4.eventHandler->runController(this);
}

// Wait briefly (10 seconds) for a key press.
void AnyKeyController::waitTimeout() {
    timerInterval = 10000 / xu4.eventHandler->getTimerInterval();
    xu4.eventHandler->runController(this);
}

bool AnyKeyController::keyPressed(int key) {
    xu4.eventHandler->setControllerDone();
    return true;
}

bool AnyKeyController::inputEvent(const InputEvent* ev) {
    if (ev->type == IE_MOUSE_PRESS && ev->n == CMOUSE_LEFT)
        xu4.eventHandler->setControllerDone();
    return true;
}

void AnyKeyController::timerFired() {
    xu4.eventHandler->setControllerDone();
}

//----------------------------------------------------------------------------

/**
 * A controller to read a player number.
 */
class ReadPlayerController : public ReadChoiceController {
public:
    ReadPlayerController();
    ~ReadPlayerController();
    virtual bool keyPressed(int key);

    int getPlayer();
    int waitFor();
};

ReadPlayerController::ReadPlayerController() : ReadChoiceController("12345678 \033\n") {
#ifdef IOS
    U4IOS::beginCharacterChoiceDialog();
#endif
}

ReadPlayerController::~ReadPlayerController() {
#ifdef IOS
    U4IOS::endCharacterChoiceDialog();
#endif
}

bool ReadPlayerController::keyPressed(int key) {
    bool valid = ReadChoiceController::keyPressed(key);
    if (valid) {
        if (value < '1' ||
            value > ('0' + c->saveGame->members))
            value = '0';
    } else {
        value = '0';
    }
    return valid;
}

int ReadPlayerController::getPlayer() {
    return value - '1';
}

int ReadPlayerController::waitFor() {
    ReadChoiceController::waitFor();
    return getPlayer();
}

//----------------------------------------------------------------------------

/**
 * A controller to handle input for commands requiring a letter
 * argument in the range 'a' - lastValidLetter.
 */
class AlphaActionController : public WaitableController<int> {
public:
    AlphaActionController(char letter, const string &p) : lastValidLetter(letter), prompt(p) {}
    bool keyPressed(int key);

private:
    char lastValidLetter;
    string prompt;
};

bool AlphaActionController::keyPressed(int key) {
    if (islower(key))
        key = toupper(key);

    if (key >= 'A' && key <= toupper(lastValidLetter)) {
        screenMessage("%c\n", key);
        value = key - 'A';
        doneWaiting();
    } else if (key == U4_SPACE || key == U4_ESC || key == U4_ENTER) {
        screenMessage("\n");
        value = -1;
        doneWaiting();
    } else {
        screenMessage("\n%s", prompt.c_str());
        return EventHandler::defaultKeyHandler(key);
    }
    return true;
}

//----------------------------------------------------------------------------

/*
 * A controller that ignores keypresses
 */
class IgnoreKeysController : public Controller {
public:
    virtual bool keyPressed(int key) {
        EventHandler::globalKeyHandler(key);
        return true;
    }
};

//----------------------------------------------------------------------------

/**
 * Handles any and all keystrokes.
 * Generally used to exit the application, switch applications,
 * minimize, maximize, etc.
 */
bool EventHandler::globalKeyHandler(int key) {
    switch(key) {
    case U4_PAUSE:
    case U4_ALT + 'p':
        xu4.eventHandler->togglePause();
        return true;

#if defined(MACOSX)
    case U4_META + 'q': /* Cmd+q */
    case U4_META + 'x': /* Cmd+x */
#endif
    case U4_ALT + 'x': /* Alt+x */
#if defined(WIN32)
    case U4_ALT + U4_FKEY + 3:
#endif
        xu4.eventHandler->quitGame();
        return true;

    default:
        return false;
    }
}

/**
 * A default key handler that should be valid everywhere
 */
bool EventHandler::defaultKeyHandler(int key) {
    switch (key) {
#ifdef DEBUG
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
#endif

    case U4_ESC:
        xu4_selectGame();
        break;

    default:
        return false;
    }

    return true;
}

//----------------------------------------------------------------------------

/*
 * Read a player number.
 * Return -1 if none is selected.
 */
int EventHandler::choosePlayer()
{
    ReadPlayerController ctrl;
    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

/**
 * Handle input for commands requiring a letter argument in the
 * range 'a' - lastValidLetter.
 */
int EventHandler::readAlphaAction(char lastValidLetter, const char* prompt)
{
    AlphaActionController ctrl(lastValidLetter, prompt);
    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

/*
 * Read a single key from a provided list.
 */
char EventHandler::readChoice(const char* choices)
{
    ReadChoiceController ctrl(choices);
    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

/*
 * Read a direction entered with the arrow keys.
 */
Direction EventHandler::readDir()
{
    ReadDirController ctrl;
#ifdef IOS
    U4IOS::IOSDirectionHelper directionPopup;
#endif
    xu4.eventHandler->pushController(&ctrl);
    return ctrl.waitFor();
}

/**
 * Read an integer, terminated by the enter key.
 * Non-numeric keys are ignored.
 */
int EventHandler::readInt(int maxlen)
{
    ReadStringController ctrl(maxlen, TEXT_AREA_X + c->col,
                                      TEXT_AREA_Y + c->line,
                              NULL, "0123456789 \n\r\010");

    xu4.eventHandler->pushController(&ctrl);
    string s = ctrl.waitFor();
    return static_cast<int>(strtol(s.c_str(), NULL, 10));
}

/*
 * Read a string, terminated by the enter key.
 */
const char* EventHandler::readString(int maxlen, const char *extraChars)
{
    assert(size_t(maxlen) < sizeof(xu4.eventHandler->readStringBuf));
    ReadStringController ctrl(maxlen, TEXT_AREA_X + c->col,
                                      TEXT_AREA_Y + c->line);
    if (extraChars)
        addCharBits(ctrl.accepted, extraChars);

    xu4.eventHandler->pushController(&ctrl);
    ctrl.waitFor();

    char* buf = xu4.eventHandler->readStringBuf;
    strcpy(buf, ctrl.value.c_str());
    return buf;
}

/*
 * Read a string, terminated by the enter key.
 */
const char* EventHandler::readStringView(int maxlen, TextView *view,
                                         const char* extraChars) {
    assert(size_t(maxlen) < sizeof(xu4.eventHandler->readStringBuf));
    ReadStringController ctrl(maxlen, view->cursorX(), view->cursorY(),
                              view);
    if (extraChars)
        addCharBits(ctrl.accepted, extraChars);

    xu4.eventHandler->pushController(&ctrl);
    ctrl.waitFor();

    char* buf = xu4.eventHandler->readStringBuf;
    strcpy(buf, ctrl.value.c_str());
    return buf;
}

/*
 * Wait until a key is pressed.
 */
void EventHandler::waitAnyKey()
{
#if 1
    AnyKeyController ctrl;
    ctrl.wait();
#else
    ReadChoiceController ctrl("");
    xu4.eventHandler->pushController(&ctrl);
    ctrl.waitFor();
#endif
}

/*
 * Wait briefly (10 seconds) for a key press.
 */
void EventHandler::waitAnyKeyTimeout()
{
    AnyKeyController ctrl;
    ctrl.waitTimeout();
}

/*
 * Ignore non-global key & mouse events forever.
 */
void EventHandler::ignoreInput()
{
    IgnoreKeysController ctrl;
    xu4.eventHandler->runController(&ctrl);
}
