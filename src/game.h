/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

#include "map.h"
#include "types.h"

class Map;
struct _Portal;
class Monster;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DUNGEON,
    VIEW_DEAD,
    VIEW_CODEX
} ViewMode;

typedef struct AlphaActionInfo {
    char lastValidLetter;
    bool (*handleAlpha)(int, void *);
    string prompt;
    void *data;
} AlphaActionInfo;

typedef struct CoordActionInfo {
    bool (*handleAtCoord)(MapCoords, int, void*);
    MapCoords origin, prev;    
    int range;
    int validDirections;
    class Object *obj;
    int player;
    int dir;                /* mask of the direction the action is taken
                               (so that (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST)) makes a diagonal) */
    bool (*blockedPredicate)(MapTile tile);
    int blockBefore;        /* try the action first, or test to see if it was blocked first? */
    int firstValidDistance; /* the first distance at which the action will function correctly */
} CoordActionInfo;

typedef struct GetPlayerInfo {
    int canBeDisabled;    
    bool (*command)(int player);
} GetPlayerInfo;

/* main game functions */
void gameInit(void);
void gameTimer(void *data);
void gameFinishTurn(void);

/* key handlers */
bool gameBaseKeyHandler(int key, void *data);
bool gameGetPlayerNoKeyHandler(int key, void *data);
bool gameGetAlphaChoiceKeyHandler(int key, void *data);
bool gameGetDirectionKeyHandler(int key, void *data);
bool gameGetFieldTypeKeyHandler(int key, void *data);
bool gameGetPhaseKeyHandler(int key, void *data);
bool gameGetCoordinateKeyHandler(int key, void *data);
bool gameSpellMixMenuKeyHandler(int key, void *data);
bool gameSpecialCmdKeyHandler(int key, void *data);
bool gameZtatsKeyHandler(int key, void *data);

/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);
void gameSetMap(class Map *map, bool saveLocation, const struct _Portal *portal);
int gameExitToParentMap();
void gameSetTransport(MapTile tile);

/* spell functions */
bool gameCastForPlayer(int player);
void gameResetSpellMixing(void);

/* action functions */
bool gameGetChest(int player);
bool gamePeerCity(int city, void *data);
void gamePeerGem(void);
bool fireAtCoord(MapCoords coords, int distance, void *data);
int gameDirectionalAction(CoordActionInfo *info);
int useItem(string *itemName);
bool readyForPlayer2(int weapon, void *data);

/* checking functions */
void gameCheckHullIntegrity(void);

/* monster functions */
bool monsterRangeAttack(MapCoords coords, int distance, void *data);
void gameMonsterCleanup(void);
void gameSpawnMonster(const class Monster *m);

/* etc */
void gameGetInput(int (*handleBuffer)(string*), string *buffer, int bufferlen = 32);
void gameGetPlayerForCommand(bool (*commandFn)(int player), int canBeDisabled, int canBeActivePlayer);
void gameDamageParty(int minDamage, int maxDamage);
void gameDamageShip(int minDamage, int maxDamage);

#endif
