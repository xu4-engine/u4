/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <list>

#include "event.h"

#include "u4.h"
#include "context.h"
#include "debug.h"
#include "location.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "textview.h"

int eventTimerGranularity = 250;

extern bool quit;
bool EventHandler::controllerDone = false;
bool EventHandler::ended = false;
unsigned int TimedEventMgr::instances = 0;

EventHandler *EventHandler::instance = NULL;
EventHandler *EventHandler::getInstance() {
    if (instance == NULL) 
        instance = new EventHandler();
    return instance;
}

/**
 * Waits a given number of milliseconds before continuing 
 */ 
void EventHandler::wait_msecs(unsigned int msecs) {
    int msecs_per_cycle = (1000 / settings.gameCyclesPerSecond);
    int cycles = msecs / msecs_per_cycle;

    if (cycles > 0) {        
        WaitController waitCtrl(cycles);
        getInstance()->pushController(&waitCtrl);
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
    getInstance()->pushController(&waitCtrl);
    waitCtrl.wait();
}

void EventHandler::setControllerDone(bool done) { controllerDone = done; }     /**< Sets the controller exit flag for the event handler */
bool EventHandler::getControllerDone()         { return controllerDone; }      /**< Returns the current value of the global exit flag */
void EventHandler::end() { ended = true; }                                     /**< End all event processing */
TimedEventMgr* EventHandler::getTimer()  { return &timer;}

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

/**
 * Generic handler for reading a single character choice from a set of
 * valid characters.
 */
bool keyHandlerGetChoice(int key, void *data) {
    KeyHandler::GetChoice *info = (KeyHandler::GetChoice *) data;

    if (isupper(key))
        key = tolower(key);

    if (info->choices.find_first_of(key) < info->choices.length()) {
        if ((*info->handleChoice)(key))
            //eventHandler->popKeyHandlerData();

        return true;
    }    
    return false;
}

/**
 * Generic handler for reading a buffer.  Handles key presses when a
 * buffer is being read, such as when a conversation is active.  The
 * keystrokes are buffered up into a word until enter is pressed.
 * Control is then passed to a seperate handler.
 */
bool keyHandlerReadBuffer(int key, void *data) {
    KeyHandler::ReadBuffer *info = (KeyHandler::ReadBuffer *) data;
    int valid = true,
        len = info->buffer->length();    

    if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') ||
        (key >= '0' && key <= '9') || key == ' ') {        
        if (len < info->bufferLen - 1) {
            /* add a character to the end */
            *info->buffer += key;

            screenHideCursor();
            screenTextAt(info->screenX + len, info->screenY, "%c", key);
            screenSetCursorPos(info->screenX + len + 1, info->screenY);
            screenShowCursor();            
        }

    } else if (key == U4_BACKSPACE) {
        if (len > 0) {
            /* remove the last character */
            info->buffer->erase(len - 1, 1);

            screenHideCursor();
            screenTextAt(info->screenX + len - 1, info->screenY, " ");
            screenSetCursorPos(info->screenX + len - 1, info->screenY);
            screenShowCursor();
        }

    } else if (key == '\n' || key == '\r') {
        if ((*info->handleBuffer)(info->buffer))
            ;//eventHandler->popKeyHandlerData();
        else
            info->screenX -= info->buffer->length();
    }    
    else {
        valid = false;
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
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
    unsigned int pos = string::npos;
    
    if (key < U4_ALT)
         pos = accepted.find_first_of(key);

    if (pos != string::npos) {
        if (key == U4_BACKSPACE) {
            if (len > 0) {
                /* remove the last character */
                value.erase(len - 1, 1);

                if (view) {
                    view->textAt(screenX + len - 1, screenY, " ");
                } else {
                    screenHideCursor();
                    screenTextAt(screenX + len - 1, screenY, " ");
                    screenSetCursorPos(screenX + len - 1, screenY);
                    screenShowCursor();
                }
            }
        }
        else if (key == '\n' || key == '\r') {            
            //screenMessage("%s", value.c_str());
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
                screenShowCursor();            
            }
        }
    }
    else valid = false;    

    return valid || KeyHandler::defaultHandler(key, NULL);
}

string ReadStringController::get(int maxlen, int screenX, int screenY, EventHandler *eh) {
    if (!eh)
        eh = eventHandler;

    ReadStringController ctrl(maxlen, screenX, screenY);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

string ReadStringController::get(int maxlen, TextView *view, EventHandler *eh) {
    if (!eh)
        eh = eventHandler;

    ReadStringController ctrl(maxlen, view);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

ReadIntController::ReadIntController(int maxlen, int screenX, int screenY) : ReadStringController(maxlen, screenX, screenY, "0123456789 \n\r\010") {}

int ReadIntController::get(int maxlen, int screenX, int screenY, EventHandler *eh) {
    if (!eh)
        eh = eventHandler;

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
    if (isupper(key))
        key = tolower(key);

    value = key;

    if (choices.empty() || choices.find_first_of(value) < choices.length()) {
        // If the value is printable, display it
        //if (!isspace(key))
        //screenMessage("%c", toupper(key));
        doneWaiting();
        return true;
    }

    return false;
}

char ReadChoiceController::get(const string &choices, EventHandler *eh) {
    if (!eh)
        eh = eventHandler;
    
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
        eventHandler->setControllerDone(true);
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
