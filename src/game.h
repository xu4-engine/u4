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
    int (*handleAtCoord)(int, int, int);
    int origin_x, origin_y;
    int range;
    int validDirections;
    int (*blockedPredicate)(unsigned char tile);
} CoordActionInfo;

void gameInit(void);
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);
void gameSetMap(struct _Context *ct, struct _Map *map, int saveOldPos, const struct _Portal *portal);
int gameExitToParentMap(struct _Context *ct);
int gameBaseKeyHandler(int key, void *data);
int gameGetPlayerNoKeyHandler(int key, void *data);
int gameGetAlphaChoiceKeyHandler(int key, void *data);
int gameGetDirectionKeyHandler(int key, void *data);
int gameGetCoordinateKeyHandler(int key, void *data);
int gameZtatsKeyHandler(int key, void *data);
int gameSpecialCmdKeyHandler(int key, void *data);
void gameTimer(void *data);
void gameFinishTurn(void);
struct _Context *gameCloneContext(struct _Context *ctx);

#endif
