/*
 * $Id$
 */

#ifndef EVENT_H
#define EVENT_H

#define U4_UP '['
#define U4_DOWN '/'
#define U4_LEFT ';'
#define U4_RIGHT '\''
#define U4_BACKSPACE 8

typedef int (*KeyHandler)(int);

typedef struct KeyHandlerNode {
    KeyHandler kh;
    struct KeyHandlerNode *next;
} KeyHandlerNode;

void eventHandlerMain();
void eventHandlerPushKeyHandler(KeyHandler kh);
void eventHandlerPopKeyHandler();
KeyHandler eventHandlerGetKeyHandler();

int keyHandlerDefault(int key);
int keyHandlerNormal(int key);
int keyHandlerTalk(int key);
int keyHandlerTalking(int key);
int keyHandlerQuit(int key);

#endif
