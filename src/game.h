/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

struct _Context;
struct _Map;
struct _Portal;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DEAD
} ViewMode;

typedef struct AlphaActionInfo {
    char lastValidLetter;
    int (*handleAlpha)(int, void *);
    const char *prompt;
    void *data;
} AlphaActionInfo;

typedef struct CoordActionInfo {
    int (*handleAtCoord)(int, int, int, void*);
    int origin_x, origin_y;
    int prev_x, prev_y;
    int range;
    int validDirections;
    int player;
    int (*blockedPredicate)(unsigned char tile);
    int blockBefore; /* try the action first, or test to see if it was blocked first? */
} CoordActionInfo;

void gameInit(void);
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);
void gameSetMap(struct _Context *ct, struct _Map *map, int saveLocation, const struct _Portal *portal);
int gameExitToParentMap(struct _Context *ct);
int gameBaseKeyHandler(int key, void *data);
int gameGetPlayerNoKeyHandler(int key, void *data);
int gameGetAlphaChoiceKeyHandler(int key, void *data);
int gameGetDirectionKeyHandler(int key, void *data);
int gameGetPhaseKeyHandler(int key, void *data);
int gameGetCoordinateKeyHandler(int key, void *data);
int gamePeerCity(int city, void *data);
int gameSpecialCmdKeyHandler(int key, void *data);
int gameZtatsKeyHandler(int key, void *data);
void gameCheckHullIntegrity(void);
void gameTimer(void *data);
void gameFinishTurn(void);

#endif
