/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

#include "controller.h"
#include "event.h"
#include "map.h"
#include "menu.h"
#include "movement.h"
#include "sound.h"
#include "types.h"

class Map;
struct Portal;
class Creature;
class Ingredients;
class Party;
class PartyMember;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DUNGEON,
    VIEW_DEAD,
    VIEW_CODEX
} ViewMode;

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

/**
 * A controller to read a player number.
 */
class ReadPlayerController : public ReadChoiceController {
public:
    ReadPlayerController();
    virtual bool keyPressed(int key);

    int getPlayer();
    int waitFor();
};

/**
 * Controller for the reagents menu used when mixing spells.  Fills
 * the passed in Ingredients with the selected reagents.
 */
class ReagentsMenuController : public MenuController {
public:
    ReagentsMenuController(Menu *menu, Ingredients *i) : MenuController(menu), ingredients(i) { }

    bool keyPressed(int key);

private:
    Ingredients *ingredients;
};

/**
 * A controller to handle input for commands requiring a letter
 * argument in the range 'a' - lastValidLetter.
 */
class AlphaActionController : public WaitableController<int> {
public:
    AlphaActionController(char letter, const string &p) : lastValidLetter(letter), prompt(p) {}
    bool keyPressed(int key);
    
    static int get(char lastValidLetter, const string &prompt, EventHandler *eh = NULL);

private:
    char lastValidLetter;
    string prompt;
};

/**
 * The main game controller that handles basic game flow and keypresses.
 */
class GameController : public Controller {
public:
    /* controller functions */
    virtual bool keyPressed(int key);
    virtual void timerFired();

    /* main game functions */
    void init(void);    
    static void finishTurn(void);
};

extern GameController *game;

/* key handlers */
bool gameGetFieldTypeKeyHandler(int key, void *data);
bool gameGetPhaseKeyHandler(int key, void *data);
bool gameGetCoordinateKeyHandler(int key, void *data);
bool gameSpellMixMenuKeyHandler(int key, void *data);
bool gameSpecialCmdKeyHandler(int key, void *data);
bool gameZtatsKeyHandler(int key, void *data);

/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);
void gameSetMap(class Map *map, bool saveLocation, const Portal *portal);
int gameExitToParentMap();

/* spell functions */
bool gameCastForPlayer(int player);
void gameResetSpellMixing(void);

/* action functions */
bool gameGetChest(int player);
bool gamePeerCity(int city, void *data);
void peer(bool useGem = true);
bool fireAtCoord(MapCoords coords, int distance, void *data);
int gameDirectionalAction(CoordActionInfo *info);
bool gameGetDirection(int (*handleDirection)(Direction dir));
int useItem(string *itemName);
void readyWeapon(int player = -1, WeaponType w = WEAP_MAX);

/* checking functions */
void gameCheckHullIntegrity(void);

/* creature functions */
bool creatureRangeAttack(MapCoords coords, int distance, void *data);
void gameCreatureCleanup(void);
bool gameSpawnCreature(const class Creature *m);

/* etc */
void gameGetInput(int (*handleBuffer)(string*), string *buffer, int bufferlen = 32);
string gameGetInput(int maxlen = 32);
int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer);
void gameGetPlayerForCommand(bool (*commandFn)(int player), bool canBeDisabled, bool canBeActivePlayer);
void gameDamageParty(int minDamage, int maxDamage);
void gameDamageShip(int minDamage, int maxDamage);
void gameSetActivePlayer(int player);

#endif
