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

int eventTimerGranularity = 250;
EventHandler eventHandler;

extern bool quit;
bool EventHandler::exit = false;
unsigned int TimedEventMgr::instances = 0;

void EventHandler::setExitFlag(bool xit) { exit = xit; }     /**< Sets the global exit flag for the event handler */
bool EventHandler::getExitFlag()         { return exit; }    /**< Returns the current value of the global exit flag */
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
            //eventHandler.popKeyHandlerData();

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
            ;//eventHandler.popKeyHandlerData();
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

ReadStringController::ReadStringController(int maxlen, int screenX, int screenY) {
    this->maxlen = maxlen;
    this->screenX = screenX;
    this->screenY = screenY;
    exitWhenDone = false;
}

bool ReadStringController::keyPressed(int key) {
    int valid = true,
        len = value.length();    

    if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z') ||
        (key >= '0' && key <= '9') || key == ' ') {        
        if (len < maxlen - 1) {
            /* add a character to the end */
            value += key;

            screenHideCursor();
            screenTextAt(screenX + len, screenY, "%c", key);
            screenSetCursorPos(screenX + len + 1, screenY);
            screenShowCursor();            
        }

    } else if (key == U4_BACKSPACE) {
        if (len > 0) {
            /* remove the last character */
            value.erase(len - 1, 1);

            screenHideCursor();
            screenTextAt(screenX + len - 1, screenY, " ");
            screenSetCursorPos(screenX + len - 1, screenY);
            screenShowCursor();
        }

    } else if (key == '\n' || key == '\r') {
        if (exitWhenDone)
            eventHandler.setExitFlag(true);
    }    
    else {
        valid = false;
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

string ReadStringController::getString() {
    return value;
}

string ReadStringController::waitFor() {
    exitWhenDone = true;
    eventHandler.main();
    eventHandler.setExitFlag(false);
    eventHandler.popController();
    return value;
}

ReadChoiceController::ReadChoiceController(const string &choices) {
    this->choices = choices;
    exitWhenDone = false;
}

bool ReadChoiceController::keyPressed(int key) {
    if (isupper(key))
        key = tolower(key);

    choice = key;

    if (choices.empty() || choices.find_first_of(choice) < choices.length()) {
        if (exitWhenDone)
            eventHandler.setExitFlag(true);
        return true;
    }

    return false;
}

int ReadChoiceController::getChoice() {
    return choice;
}

int ReadChoiceController::waitFor() {
    exitWhenDone = true;
    eventHandler.main();
    eventHandler.setExitFlag(false);
    eventHandler.popController();
    return choice;
}
