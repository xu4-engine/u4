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

typedef struct TimerCallbackNode {
    TimerCallback callback;
    void *data;
    int interval;
    int current;
    struct TimerCallbackNode *next;
} TimerCallbackNode;

TimerCallbackNode *timerCallbackHead = NULL;
TimerCallbackNode *deferedTimerCallbackRemoval = NULL;
bool timerCallbackListLocked = false;

extern bool quit;
bool eventExitFlag = false;

std::list<KeyHandler>    keyHandlers;    /* key handler <stack> */
std::list<void *>        keyHandlerData; /* key handler data <queue> */
std::list<MouseArea*>    mouseAreaSets;  /* mouse areas <stack> */

/**
 * Set flag to end eventHandlerMain loop.
 */
void eventHandlerSetExitFlag(bool flag) {
    eventExitFlag = flag;
}

/**
 * Get flag that controls eventHandlerMain loop.
 */
bool eventHandlerGetExitFlag() {
    return eventExitFlag;
}

/**
 * Adds a new timer callback to the timer callback list.  The interval
 * value determines how many milliseconds between calls; a value of
 * 250 generates 4 callbacks per second, a value of 1000 generates 1
 * callback per second, etc.
 */
void eventHandlerAddTimerCallback(TimerCallback callback, int interval) {
    eventHandlerAddTimerCallbackData(callback, NULL, interval);
}

void eventHandlerAddTimerCallbackData(TimerCallback callback, void *data, int interval) {
    TimerCallbackNode *n = new TimerCallbackNode;

    ASSERT((interval > 0) && (interval % eventTimerGranularity == 0), "invalid timer interval: %d", interval);
    if (n) {
        n->callback = callback;
        n->interval = interval;
        n->current = interval;
        n->data = data;
        n->next = timerCallbackHead;
        timerCallbackHead = n;
    }
}

/**
 * Removes a timer callback from the timer callback list.
 */
void eventHandlerRemoveTimerCallback(TimerCallback callback) {
    eventHandlerRemoveTimerCallbackData(callback, NULL);
}

void eventHandlerRemoveTimerCallbackData(TimerCallback callback, void *data) {
    TimerCallbackNode *n, *prev;

    if (timerCallbackListLocked) {
        n = new TimerCallbackNode;
        n->callback = callback;
        n->data = data;
        n->next = deferedTimerCallbackRemoval;
        deferedTimerCallbackRemoval = n;
        return;
    }

    n = timerCallbackHead;
    prev = NULL;
    while (n) {
        if (n->callback != callback || n->data != data) {
            prev = n;
            n = n->next;
            continue;
        }

        if (prev)
            prev->next = n->next;
        else
            timerCallbackHead = n->next;
        delete n;
        return;
    }
}

/**
 * Trigger all the timer callbacks.
 */
void eventHandlerCallTimerCallbacks() {
    TimerCallbackNode *n;    

    timerCallbackListLocked = 1;

    for (n = timerCallbackHead; n != NULL; n = n->next) {
        n->current -= eventTimerGranularity;
        if (n->current <= 0) {
            (*n->callback)(n->data);
            n->current = n->interval;
        }
    }

    timerCallbackListLocked = 0;

    while (deferedTimerCallbackRemoval) {
        eventHandlerRemoveTimerCallbackData(deferedTimerCallbackRemoval->callback, deferedTimerCallbackRemoval->data);
        n = deferedTimerCallbackRemoval;
        deferedTimerCallbackRemoval = deferedTimerCallbackRemoval->next;
        delete n;
    }
}

/**
 * Re-initializes each timer callback to a new eventTimerGranularity
 */ 
void eventHandlerResetTimerCallbacks() {
    int baseInterval;
    TimerCallbackNode *n;

    for (n = timerCallbackHead; n != NULL; n = n->next) {
        baseInterval = n->interval / eventTimerGranularity;
        eventTimerGranularity = (1000 / settings.gameCyclesPerSecond);        
        n->interval = baseInterval * eventTimerGranularity;
    }

    /* re-initialize the main event loop */
    eventHandlerDelete();
    eventHandlerInit();
}

/**
 * Push a key handler onto the top of the keyhandler stack.
 */
void eventHandlerPushKeyHandler(KeyHandler kh) {
    eventHandlerPushKeyHandlerWithData(kh, NULL);
}

/**
 * Push a key handler onto the top of the key handler stack, and
 * provide specific data to pass to the handler.
 */
void eventHandlerPushKeyHandlerWithData(KeyHandler kh, void *data) {
    keyHandlers.push_front(kh);
    keyHandlerData.push_back(data);
}

/**
 * Pop the top key handler off.
 */
void eventHandlerPopKeyHandler() {
    if (keyHandlers.size() > 0)        
        keyHandlers.pop_front();    
}

/**
 * Pop the key handler data off the front of the queue
 */ 
void eventHandlerPopKeyHandlerData() {
    if (keyHandlerData.size() > 0)
        keyHandlerData.pop_front();
}

/**
 * Eliminate all key handlers and begin stack with new handler
 */
void eventHandlerSetKeyHandler(KeyHandler kh) {
    while (keyHandlers.size() > 0)        
        eventHandlerPopKeyHandler();

    eventHandlerPushKeyHandler(kh);
}

/**
 * Get the currently active key handler off the top of the key handler
 * stack.
 */
KeyHandler eventHandlerGetKeyHandler() {
    return keyHandlers.front();
}

/**
 * Get the call data associated with the currently active key handler.
 */
void *eventHandlerGetKeyHandlerData() {
    return keyHandlerData.back();
}

/**
 * Returns true if the key or key combination is always ignored by xu4
 */
bool eventHandlerIsKeyIgnored(int key) {
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

/**
 * A key handler that handles every keypress
 */
bool eventHandlerUniversalKeyHandler(int key) {
    switch(key) {
#if defined(MACOSX)
    case U4_META + 'q': /* Cmd+q */
    case U4_META + 'x': /* Cmd+x */
#endif
    case U4_ALT + 'x': /* Alt+x */
#if defined(WIN32)
	case U4_ALT + U4_FKEY + 3:
#endif
        quit = eventExitFlag = true;
        return true;
    default: return false;
    }
}

/**
 * A base key handler that should be valid everywhere.
 */
bool keyHandlerDefault(int key, void *data) {
    bool valid = true;

    switch (key) {
    case '`':
        if (c && c->location)
            printf("x = %d, y = %d, level = %d, tile = %d\n", c->location->coords.x, c->location->coords.y, c->location->coords.z, c->location->map->tileAt(c->location->coords, WITH_OBJECTS));
        break;
    default:
        valid = false;
        break;
    }

    return valid;
}

/**
 * Generic handler for discarding all keyboard input.
 */
bool keyHandlerIgnoreKeys(int key, void *data) {
    return true;
}

/**
 * Generic handler for reading a single character choice from a set of
 * valid characters.
 */
bool keyHandlerGetChoice(int key, void *data) {
    GetChoiceActionInfo *info = (GetChoiceActionInfo *) data;

    if (isupper(key))
        key = tolower(key);

    if (info->choices.find_first_of(key) < info->choices.length()) {
        if ((*info->handleChoice)(key))
            eventHandlerPopKeyHandlerData();

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
    ReadBufferActionInfo *info = (ReadBufferActionInfo *) data;
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
            eventHandlerPopKeyHandlerData();
        else
            info->screenX -= info->buffer->length();
    }    
    else {
        valid = false;
    }

    return valid || keyHandlerDefault(key, NULL);
}

void eventHandlerPushMouseAreaSet(MouseArea *mouseAreas) {
    mouseAreaSets.push_front(mouseAreas);
}

void eventHandlerPopMouseAreaSet(void) {
    if (mouseAreaSets.size())
        mouseAreaSets.pop_front();    
}

/**
 * Get the currently active mouse area set of the top of the stack.
 */
MouseArea *eventHandlerGetMouseAreaSet() {
    if (mouseAreaSets.size())
        return mouseAreaSets.front();
    else
        return NULL;
}

