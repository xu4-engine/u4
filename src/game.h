/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

typedef struct AlphaActionInfo {
    char lastValidLetter;
    int (*handleAlpha)(int, void *);
    const char *prompt;
    void *data;
} AlphaActionInfo;

typedef struct DirectedActionInfo {
    int (*handleAtCoord)(int, int);
    int range;
    int (*blockedPredicate)(unsigned char tile);
    const char *failedMessage;
} DirectedActionInfo;

int gameBaseKeyHandler(int key, void *data);
int gameGetPlayerNoKeyHandler(int key, void *data);
int gameGetDirectionKeyHandler(int key, void *data);
int gameQuitKeyHandler(int key, void *data);
int gameZtatsKeyHandler(int key, void *data);
int gameZtatsKeyHandler2(int key, void *data);
void gameTimer(void);

#endif
