/*
 * $Id$
 */

#ifndef GAME_H
#define GAME_H

#include <vector>

#include "controller.h"
#include "event.h"
#include "map.h"
#include "observer.h"
#include "sound.h"
#include "tileview.h"
#include "types.h"

using std::vector;

class Map;
struct Portal;
class Creature;
class Location;
class MoveEvent;
class Party;
class PartyEvent;
class PartyMember;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_RUNE,
    VIEW_DUNGEON,
    VIEW_DEAD,
    VIEW_CODEX,
    VIEW_MIXTURES
} ViewMode;

/**
 * A controller to read a player number.
 */
class ReadPlayerController : public ReadChoiceController {
public:
    ReadPlayerController();
    ~ReadPlayerController();
    virtual bool keyPressed(int key);

    int getPlayer();
    int waitFor();
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
 * Controls interaction while Ztats are being displayed.
 */
class ZtatsController : public WaitableController<void *> {
public:
    bool keyPressed(int key);
};

class TurnCompleter {
public:
    virtual ~TurnCompleter() {}
    virtual void finishTurn() = 0;
};

/**
 * The main game controller that handles basic game flow and keypresses.
 *
 * @todo
 *  <ul>
 *      <li>separate the dungeon specific stuff into another class (subclass?)</li>
 *  </ul>
 */
class GameController : public Controller, public Observer<Party *, PartyEvent &>, public Observer<Location *, MoveEvent &>,
    public TurnCompleter {
public:
    GameController();
    ~GameController();

    /* controller functions */
    virtual bool keyPressed(int key);
    virtual void timerFired();

    /* main game functions */
    bool init();
    void initScreen();
    void initScreenWithoutReloadingState();
    void setMap(Map *map, bool saveLocation, const Portal *portal, TurnCompleter *turnCompleter = NULL);
    int exitToParentMap();
    virtual void finishTurn();

    virtual void update(Party *party, PartyEvent &event);
    virtual void update(Location *location, MoveEvent &event);

    void initMoons();
    void updateMoons(bool showmoongates);

    static void flashTile(const Coords &coords, MapTile tile, int timeFactor);
    static void flashTile(const Coords &coords, Symbol tilename, int timeFactor);

    TileView mapArea;
    bool paused;
    int pausedTimer;

private:
    void avatarMoved(MoveEvent &event);
    void avatarMovedInDungeon(MoveEvent &event);

    void creatureCleanup();
    void checkBridgeTrolls();
    void checkRandomCreatures();
    void checkSpecialCreatures(Direction dir);
    bool checkMoongates();

    bool createBalloon(Map *map);
};

/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen(void);

/* spell functions */
void castSpell(int player = -1);
void gameSpellEffect(int spell, int player, Sound sound);

/* action functions */
void destroy();
void attack();
void board();
void fire();
void getChest(int player = -1);
void holeUp();
void jimmy();
void opendoor();
bool gamePeerCity(int city, void *data);
void peer(bool useGem = true);
void talk();
bool fireAt(const Coords &coords, bool originAvatar);
Direction gameGetDirection();
void readyWeapon(int player = -1);

/* checking functions */
void gameCheckHullIntegrity(void);

/* creature functions */
bool creatureRangeAttack(const Coords &coords, Creature *m);
void gameCreatureCleanup(void);
bool gameSpawnCreature(const class Creature *m);

/* etc */
string gameGetInput(int maxlen = 32);
int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer);
void gameGetPlayerForCommand(bool (*commandFn)(int player), bool canBeDisabled, bool canBeActivePlayer);
void gameDamageParty(int minDamage, int maxDamage);
void gameDamageShip(int minDamage, int maxDamage);
void gameSetActivePlayer(int player);
vector<Coords> gameGetDirectionalActionPath(int dirmask, int validDirections, const Coords &origin, int minDistance, int maxDistance, bool (*blockedPredicate)(const Tile *tile), bool includeBlocked);

#endif
