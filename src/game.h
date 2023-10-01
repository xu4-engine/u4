/*
 * game.h
 */

#ifndef GAME_H
#define GAME_H

#include <vector>

#include "controller.h"
#include "discourse.h"
#include "event.h"
#include "map.h"
#include "sound.h"
#include "tileview.h"
#include "types.h"

using std::vector;

struct Portal;
class Creature;
class MoveEvent;

typedef enum {
    VIEW_NORMAL,
    VIEW_GEM,
    VIEW_DUNGEON,
    VIEW_CUTSCENE,
    VIEW_CUTSCENE_MAP,
    VIEW_MIXTURES
} ViewMode;

/**
 * Controls interaction while Ztats are being displayed.
 */
class ZtatsController : public WaitableController<void *> {
public:
    bool keyPressed(int key);
};

class TurnController : public Controller {
public:
    TurnController(short timerInterval) : Controller(timerInterval) {}
    virtual void finishTurn() = 0;
};

struct ScreenState;

/**
 * The main game controller that handles basic game flow and keypresses.
 *
 * @todo
 *  <ul>
 *      <li>separate the dungeon specific stuff into another class (subclass?)</li>
 *  </ul>
 */
class GameController : public TurnController {
public:
    GameController();
    ~GameController();

    /* controller functions */
    virtual bool present();
    virtual void conclude();
    virtual bool keyPressed(int key);
    virtual bool inputEvent(const InputEvent*);
    virtual void timerFired();

    /* main game functions */
    void setMap(Map *map, bool saveLocation, const Portal *portal, TurnController *turnCompleter = NULL);
    int exitToParentMap();
    MapId combatMapForTile(const Tile *groundTile, Object *obj);
    virtual void finishTurn();

    bool initContext();
    void updateMoons(bool showmoongates);

    static void flashTile(const Coords &coords, MapTile tile, int timeFactor);
    static void flashTile(const Coords &coords, Symbol tilename, int timeFactor);

    TileView mapArea;
    Discourse vendorDisc;
    Discourse castleDisc;
    bool cutScene;
    std::map<const Tile*, MapId> tileMap;
    std::map<const Tile*, MapId> dungeontileMap;
    void (*spellCastCallback)(int spell, int caster, int subject, int spellMp);

private:
    static void gameNotice(int, void*, void*);
    static void renderHud(ScreenState* ss, void* data);
    void initScreenWithoutReloadingState();
    void initMoons();

    void avatarMoved(MoveEvent &event);
    void avatarMovedInDungeon(MoveEvent &event);

    void creatureCleanup();
    void checkBridgeTrolls();
    void checkRandomCreatures();
    void checkSpecialCreatures(Direction dir);
    bool checkMoongates();

    bool createBalloon(Map *map);

    float* borderAttr;
    int borderAttrLen;
};

/* map and screen functions */
void gameSetViewMode(ViewMode newMode);
void gameUpdateScreen();

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

/* creature functions */
bool creatureRangeAttack(const Coords &coords, Creature *m);
bool gameSpawnCreature(const class Creature *m);
void gameDestroyAllCreatures();

/* etc */
void gameBadCommand();
const char* gameGetInput(int maxlen = 30);
int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer);
void gameGetPlayerForCommand(bool (*commandFn)(int player), bool canBeDisabled, bool canBeActivePlayer);
void gameDamageParty(int minDamage, int maxDamage);
bool gameDamageShip(int minDamage, int maxDamage);
void gameSetActivePlayer(int player);
vector<Coords> gameGetDirectionalActionPath(int dirmask, int validDirections, const Coords &origin, int minDistance, int maxDistance, bool (*blockedPredicate)(const Tile *tile), bool includeBlocked);

#define gameStampCommandTime()      c->lastCommandTime = c->commandTimer
#define gameTimeSinceLastCommand()  ((c->commandTimer - c->lastCommandTime)/1000)

#endif
