/*
 * event.h
 */

#ifndef EVENT_H
#define EVENT_H

#include <list>
#include <string>
#include <vector>

#include "anim.h"
#include "controller.h"
#include "types.h"

using std::string;

#define U4_UP           '['
#define U4_DOWN         '/'
#define U4_LEFT         ';'
#define U4_RIGHT        '\''
#define U4_BACKSPACE    8
#define U4_TAB          9
#define U4_SPACE        ' '
#define U4_ESC          27
#define U4_ENTER        13
#define U4_ALT          128
#define U4_KEYPAD_ENTER 271
#define U4_META         323
#define U4_FKEY         282
#define U4_RIGHT_SHIFT  303
#define U4_LEFT_SHIFT   304
#define U4_RIGHT_CTRL   305
#define U4_LEFT_CTRL    306
#define U4_RIGHT_ALT    307
#define U4_LEFT_ALT     308
#define U4_RIGHT_META   309
#define U4_LEFT_META    310

struct _MouseArea;
class EventHandler;
class TextView;

/**
 * A controller that invokes a key handler function.
 */
class KeyHandler : public Controller {
public:
    typedef bool (*Callback)(int, void*);

    /* Static default key handler functions */
    static bool defaultHandler(int key, void *data);
    static bool ignoreKeys(int key, void *data);

    KeyHandler(KeyHandler::Callback func, void* userData = NULL);
    virtual bool keyPressed(int key);

private:
    Callback handler;
    void* data;
};

/**
 * A controller to read a string, terminated by the enter key.
 */
class ReadStringController : public WaitableController<string> {
public:
    ReadStringController(int maxlen, int screenX, int screenY,
                         TextView* view = NULL,
                         const char* accepted_chars = NULL);

    virtual bool keyPressed(int key);

    static string get(int maxlen, int screenX, int screenY, const char *extraChars = NULL);
    static string get(int maxlen, TextView *view, const char *extraChars = NULL);
#ifdef IOS
    void setValue(const string &utf8StringValue) {
        value = utf8StringValue;
    }
#endif

protected:
    int maxlen, screenX, screenY;
    TextView *view;
    uint8_t accepted[16];   // Character bitset.
};

/**
 * A controller to read a integer, terminated by the enter key.
 * Non-numeric keys are ignored.
 */
class ReadIntController : public ReadStringController {
public:
    ReadIntController(int maxlen, int screenX, int screenY);

    static int get(int maxlen);
    int getInt() const;
};

/**
 * A controller to read a single key from a provided list.
 */
class ReadChoiceController : public WaitableController<int> {
public:
    ReadChoiceController(const string &choices);
    virtual bool keyPressed(int key);

    static char get(const string &choices);

protected:
    string choices;
};

/**
 * A controller to read a direction enter with the arrow keys.
 */
class ReadDirController : public WaitableController<Direction> {
public:
    ReadDirController();
    virtual bool keyPressed(int key);
};

class AnyKeyController : public Controller {
public:
    void wait();
    void waitTimeout();
    virtual bool keyPressed(int key);
    virtual void timerFired();
};

/**
 * A class for handling timed events.
 */
class TimedEvent {
public:
    /* Typedefs */
    typedef void (*Callback)(void *);

    /* Constructors */
    TimedEvent(Callback callback, int interval, void *data = NULL);

    /* Member functions */
    Callback getCallback() const;
    void *getData();
    void tick();

    /* Properties */
protected:
    Callback callback;
    void *data;
    int interval;
    int current;
};

#if defined(IOS)
#ifndef __OBJC__
typedef void *TimedManagerHelper;
typedef void *UIEvent;
#else
@class TimedManagerHelper;
@class UIEvent;
#endif
#endif


/**
 * A class for managing timed events
 */
class TimedEventMgr {
public:
    typedef std::list<TimedEvent*> List;

    TimedEventMgr() : locked(false) {}
    ~TimedEventMgr();

    /** Returns true if the event list is locked (in use) */
    bool isLocked() const { return locked; }

    void add(TimedEvent::Callback callback, int interval, void *data = NULL);
    List::iterator remove(List::iterator i);
    void remove(TimedEvent* event);
    void remove(TimedEvent::Callback callback, void *data = NULL);
    void tick();

private:
    /* Properties */
    bool locked;
    List events;
    List deferredRemovals;
};

#define FS_SAMPLES  8

struct FrameSleep {
    uint32_t frameInterval;     // Milliseconds between display updates.
    uint32_t realTime;
    int ftime[FS_SAMPLES];      // Msec elapsed for past frames.
    int ftimeSum;
    uint16_t ftimeIndex;
    uint16_t fsleep;            // Msec to sleep - adjusted slowly.
};

typedef void(*updateScreenCallback)(void);

/**
 * A class for handling game events.
 */
class EventHandler {
public:
    /* Typedefs */
    typedef std::list<_MouseArea*> MouseAreaList;

    /* Constructors */
    EventHandler(int gameCycleDuration, int frameDuration);
    ~EventHandler();

    /* Static functions */
    static bool wait_msecs(unsigned int msecs);
    static bool timerQueueEmpty();
    static int setKeyRepeat(int delay, int interval);
    static bool globalKeyHandler(int key);

    /* Member functions */
    void setTimerInterval(int msecs);
    uint32_t getTimerInterval() const { return timerInterval; }
    TimedEventMgr* getTimer();

    /* Event functions */
    bool run();
    void setScreenUpdate(void (*updateScreen)(void));
#if defined(IOS)
    void handleEvent(UIEvent *);
    static void controllerStopped_helper();
    updateScreenCallback screenCallback() { return updateScreen; }
#endif

    /* Controller functions */
    void runController(Controller*);
    Controller *pushController(Controller *c);
    Controller *popController();
    Controller *getController() const;
    void setController(Controller *c);
    void setControllerDone(bool exit = true);
    bool getControllerDone();
    void quitGame();

    /* Key handler functions */
    void pushKeyHandler(KeyHandler::Callback func, void* data = NULL);
    void popKeyHandler();

    /* Mouse area functions */
    void pushMouseAreaSet(_MouseArea *mouseAreas);
    void popMouseAreaSet();
    _MouseArea* getMouseAreaSet() const;
    _MouseArea* mouseAreaForPoint(int x, int y);

#ifdef DEBUG
    bool beginRecording(const char* file, uint32_t seed);
    void endRecording();
    void recordKey(int key);
    int  recordedKey();
    void recordTick() { ++recordClock; }
    uint32_t replay(const char* file);
#endif

    void advanceFlourishAnim() {
        anim_advance(&flourishAnim, float(timerInterval) * 0.001f);
    }

    Animator flourishAnim;
    Animator fxAnim;

protected:
    void handleInputEvents(Controller*, updateScreenCallback);

    FrameSleep fs;
    uint32_t timerInterval;     // Milliseconds between timedEvents ticks.
    uint32_t runTime;
    int runRecursion;
#ifdef DEBUG
    int recordFP;
    int recordMode;
    int replayKey;
    uint32_t recordClock;
    uint32_t recordLast;
#endif
    bool controllerDone;
    bool ended;
    TimedEventMgr timedEvents;
    std::vector<Controller *> controllers;
    MouseAreaList mouseAreaSets;
    updateScreenCallback updateScreen;
};

#endif
