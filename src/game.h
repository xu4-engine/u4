/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

struct _Context;
struct _Map;
struct _Portal;
struct _Monster;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DUNGEON,
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
    int dir;                /* mask of the direction the action is taken
                               (so that (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST)) makes a diagonal) */
    int (*blockedPredicate)(unsigned char tile);
    int blockBefore;        /* try the action first, or test to see if it was blocked first? */
    int firstValidDistance; /* the first distance at which the action will function correctly */
} CoordActionInfo;

typedef struct GetPlayerInfo {
    int canBeDisabled;
    int (*command)(int player);
} GetPlayerInfo;

/* main game functions */
void gameInit(void);
void gameTimer(void *data);
void gameFinishTurn(void);
void gameCleanup(void);

/* key handlers */
int gameBaseKeyHandler(int key, void *data);
int gameGetPlayerNoKeyHandler(int key, void *data);
int gameGetAlphaChoiceKeyHandler(int key, void *data);
int gameGetDirectionKeyHandler(int key, void *data);
int gameGetFieldTypeKeyHandler(int key, void *data);
int gameGetPhaseKeyHandler(int key, void *data);
int gameGetCoordinateKeyHandler(int key, void *data);
int gameSpellMixMenuKeyHandler(int key, void *data);
int gameSpecialCmdKeyHandler(int key, void *data);
int gameZtatsKeyHandler(int key, void *data);

/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);
void gameSetMap(struct _Context *ct, struct _Map *map, int saveLocation, const struct _Portal *portal);
int gameExitToParentMap(struct _Context *ct);
void gameSetTransport(unsigned char tile);

/* spell functions */
int gameCastForPlayer(int player);
void gameResetSpellMixing(void);

/* action functions */
int gameGetChest(int player);
int gamePeerCity(int city, void *data);
void gamePeerGem(void);
int fireAtCoord(int x, int y, int distance, void *data);
int gameDirectionalAction(CoordActionInfo *info);
int useItem(const char *itemName);
int readyForPlayer2(int weapon, void *data);

/* checking functions */
void gameCheckHullIntegrity(void);

/* monster functions */
int monsterRangeAttack(int x, int y, int distance, void *data);
void gameMonsterCleanup(void);
void gameSpawnMonster(const struct _Monster *m);

/* etc */
void gameGetPlayerForCommand(int (*commandFn)(int player), int canBeDisabled);
void gameDamageParty(int minDamage, int maxDamage);
void gameDamageShip(int minDamage, int maxDamage);

#endif
