/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

#include <string>
#include "types.h"

using std::string;

#ifdef __cplusplus
extern "C" {
#endif

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

extern int eventTimerGranularity;

struct _MouseArea;

typedef bool (*KeyHandler)(int, void *);
typedef void (*TimerCallback)(void *);

/** Additional information to be passed as data param for read buffer key handler */
typedef struct ReadBufferActionInfo {
    int (*handleBuffer)(string*);
    string *buffer;
    int bufferLen;
    int screenX, screenY;
} ReadBufferActionInfo;

/** Additional information to be passed as data param for get choice key handler */
typedef struct GetChoiceActionInfo {
    string choices;
    int (*handleChoice)(int);
} GetChoiceActionInfo;

void eventHandlerInit(void);
void eventHandlerDelete(void);
void eventHandlerSleep(int usec);
void eventHandlerMain(void (*updateScreen)(void));
bool eventHandlerTimerQueueEmpty(void);
void eventHandlerSetExitFlag(bool flag);
bool eventHandlerGetExitFlag();
void eventHandlerAddTimerCallback(TimerCallback callback, int interval);
void eventHandlerAddTimerCallbackData(TimerCallback callback, void *data, int interval);
void eventHandlerRemoveTimerCallback(TimerCallback callback);
void eventHandlerRemoveTimerCallbackData(TimerCallback callback, void *data);
void eventHandlerCallTimerCallbacks();
void eventHandlerResetTimerCallbacks();
void eventHandlerPushKeyHandler(KeyHandler kh);
void eventHandlerPushKeyHandlerWithData(KeyHandler kh, void *data);
void eventHandlerPopKeyHandler();
void eventHandlerPopKeyHandlerData();
void eventHandlerSetKeyHandler(KeyHandler kh);
KeyHandler eventHandlerGetKeyHandler();
void *eventHandlerGetKeyHandlerData();
bool eventHandlerIsKeyIgnored(int key);
bool eventHandlerUniversalKeyHandler(int key);
int eventKeyboardSetKeyRepeat(int delay, int interval);

bool keyHandlerDefault(int key, void *data);
bool keyHandlerIgnoreKeys(int key, void *data);
bool keyHandlerReadBuffer(int key, void *data);
bool keyHandlerGetChoice(int key, void *data);

void eventHandlerPushMouseAreaSet(struct _MouseArea *mouseAreas);
void eventHandlerPopMouseAreaSet(void);
struct _MouseArea *eventHandlerGetMouseAreaSet(void);

#ifdef __cplusplus
}
#endif

#endif
