/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

typedef enum _State {
    STATE_NORMAL,
    STATE_TALK,
    STATE_TALKING,
    STATE_QUIT
} State;

typedef int (*KeyHandler)(int);

typedef struct KeyHandlerNode {
    KeyHandler *kh;
    struct KeyHandlerNode *next;
} KeyHandlerNode;

KeyHandlerNode *head;

void eventHandlerMain();
void eventHandlerPushKeyHandler(KeyHandler *kh);
void eventHandlerPopKeyHandler();
KeyHandler *eventHandlerGetKeyHandler();

int keyHandlerDefault(int key);
int keyHandlerNormal(int key);
int keyHandlerTalk(int key);
int keyHandlerTalking(int key);
int keyHandlerQuit(int key);

#endif
