/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "u4.h"
#include "event.h"
#include "context.h"
#include "location.h"
#include "savegame.h"
#include "screen.h"
#include "debug.h"

int eventTimerGranularity = 200;

typedef struct TimerCallbackNode {
    TimerCallback callback;
    void *data;
    int interval;
    int current;
    struct TimerCallbackNode *next;
} TimerCallbackNode;

TimerCallbackNode *timerCallbackHead = NULL;
KeyHandlerNode *keyHandlerHead = NULL;
int eventExitFlag = 0;
int timerCallbackListLocked = 0;
TimerCallbackNode *deferedTimerCallbackRemoval = NULL;

/**
 * Set flag to end eventHandlerMain loop.
 */
void eventHandlerSetExitFlag(int flag) {
    eventExitFlag = flag;
}

/**
 * Get flag that controls eventHandlerMain loop.
 */
int eventHandlerGetExitFlag() {
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
    TimerCallbackNode *n = (TimerCallbackNode *) malloc(sizeof(TimerCallbackNode));

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
        n = (TimerCallbackNode *) malloc(sizeof(TimerCallbackNode));
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
        free(n);
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
        free(n);
    }
}

/**
 * Push a key handler onto the top of the keyhandler stack.
 */
void eventHandlerPushKeyHandler(KeyHandler kh) {
    KeyHandlerNode *n = (KeyHandlerNode *) malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = NULL;
        n->next = keyHandlerHead;
        keyHandlerHead = n;
    }
}

/**
 * Push a key handler onto the top of the key handler stack, and
 * provide specific data to pass to the handler.
 */
void eventHandlerPushKeyHandlerData(KeyHandler kh, void *data) {
    KeyHandlerNode *n = (KeyHandlerNode *) malloc(sizeof(KeyHandlerNode));
    if (n) {
        n->kh = kh;
        n->data = data;
        n->next = keyHandlerHead;
        keyHandlerHead = n;
    }
}

/**
 * Pop the top key handler off.
 */
void eventHandlerPopKeyHandler() {
    KeyHandlerNode *n = keyHandlerHead;
    if (n) {
        keyHandlerHead = n->next;
        free(n);
    }
}

/**
 * Eliminate all key handlers and begin stack with new handler
 */
void eventHandlerSetKeyHandler(KeyHandler kh) {
    while (keyHandlerHead)
        eventHandlerPopKeyHandler();

    eventHandlerPushKeyHandler(kh);
}

/**
 * Get the currently active key handler of the top of the key handler
 * stack.
 */
KeyHandler eventHandlerGetKeyHandler() {
    return keyHandlerHead->kh;
}

/**
 * Get the call data associated with the currently active key handler.
 */
void *eventHandlerGetKeyHandlerData() {
    return keyHandlerHead->data;
}

/**
 * A base key handler that should be valid everywhere.
 */
int keyHandlerDefault(int key, void *data) {
    int valid = 1;

    switch (key) {
    case '`':
        if (c && c->location)
            printf("x = %d, y = %d, level = %d, tile = %d\n", c->location->x, c->location->y, c->location->z, mapTileAt(c->location->map, c->location->x, c->location->y, c->location->z));
        break;
    default:
        valid = 0;
        break;
    }

    return valid;
}

/**
 * Generic handler for discarding all keyboard input.
 */
int keyHandlerIgnoreKeys(int key, void *data) {
    return 1;
}

/**
 * Generic handler for reading a single character choice from a set of
 * valid characters.
 */
int keyHandlerGetChoice(int key, void *data) {
    GetChoiceActionInfo *info = (GetChoiceActionInfo *) data;

    if (isupper(key))
        key = tolower(key);

    if (strchr(info->choices, key)) {
        if ((*info->handleChoice)(key))
            free(info);

        return 1;
    }    
    return 0;
}

/**
 * Generic handler for reading a buffer.  Handles key presses when a
 * buffer is being read, such as when a conversation is active.  The
 * keystrokes are buffered up into a word until enter is pressed.
 * Control is then passed to a seperate handler.
 */
int keyHandlerReadBuffer(int key, void *data) {
    ReadBufferActionInfo *info = (ReadBufferActionInfo *) data;
    int valid = 1;

    if ((key >= 'a' && key <= 'z') || 
        (key >= 'A' && key <= 'Z') ||
        (key >= '0' && key <= '9') ||
        key == ' ') {
        int len;

        len = strlen(info->buffer);
        if (len < info->bufferLen - 1) {
            screenHideCursor();
            screenTextAt(info->screenX + len, info->screenY, "%c", key);
            screenSetCursorPos(info->screenX + len + 1, info->screenY);
            screenShowCursor();
            info->buffer[len] = key;
            info->buffer[len + 1] = '\0';
        }

    } else if (key == U4_BACKSPACE) {
        int len;

        len = strlen(info->buffer);
        if (len > 0) {
            screenHideCursor();
            screenTextAt(info->screenX + len - 1, info->screenY, " ");
            screenSetCursorPos(info->screenX + len - 1, info->screenY);
            screenShowCursor();
            info->buffer[len - 1] = '\0';
        }

    } else if (key == '\n' || key == '\r') {
        if ((*info->handleBuffer)(info->buffer))
            free(info);
        else
            info->screenX -= strlen(info->buffer);

    } else {
        valid = 0;
    }

    return valid || keyHandlerDefault(key, NULL);
}
