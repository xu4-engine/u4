/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include <map>

#include "direction.h"
#include "map.h"
#include "monster.h"
#include "movement.h"
#include "object.h"
#include "player.h"
#include "savegame.h"
#include "types.h"

#define AREA_MONSTERS   16
#define AREA_PLAYERS    8

class Object;
class Monster;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT    
} CombatAction;

/**
 * CombatController class
 */ 
class CombatController {
public:
    CombatController();
    CombatController(class CombatMap *m);
    CombatController(MapId id);

    // Accessor Methods    
    bool          isCamping() const;
    bool          isInn() const;    
    bool          isWinOrLose() const;
    Direction     getExitDir() const;
    unsigned char getFocus() const;
    CombatMap *   getMap() const;
    Monster *     getMonster() const;
    PartyMemberVector* getParty();
    PartyMember*  getCurrentPlayer();
    
    void setExitDir(Direction d);
    void setInn(bool i = true);
    void setMonster(Monster *);
    void setWinOrLose(bool worl = true);
    void showCombatMessage(bool show = true);

    // Methods
    void init(class Monster *m);
    void initCamping();
    void initDungeonRoom(int room, Direction from);
    
    void applyMonsterTileEffects();
    void begin();
    void end(bool adjustKarma);
    void fillMonsterTable(const Monster *monster);
    int  initialNumberOfMonsters(const class Monster *monster) const;
    bool isWon() const;
    bool isLost() const;
    void moveMonsters();
    void placeMonsters();
    void placePartyMembers();
    bool setActivePlayer(int player);

    /** 
     * Static member functions
     */
    // Directional actions
    static bool attackAtCoord(MapCoords coords, int distance, void *data);
    static bool rangedAttack(MapCoords coords, int distance, void *data);
    static bool returnWeaponToOwner(MapCoords coords, int distance, void *data);

    static void attackFlash(Coords coords, MapTile tile, int timeFactor);
    static void finishTurn(void);
    static MoveReturnValue movePartyMember(Direction dir, int userEvent);

    // Key handlers
    static bool baseKeyHandler(int key, void *data);
    static bool chooseWeaponRange(int key, void *data);
    static bool chooseWeaponDir(int key, void *data);

    // Properties
protected:
    class CombatMap *map;
    
    PartyMemberVector party;
    unsigned char focus;

    const class Monster *monsterTable[AREA_MONSTERS];
    class Monster *monster;    

    bool camping;
    bool inn;
    bool placePartyOnMap;
    bool placeMonstersOnMap;
    bool winOrLose;
    bool showMessage;
    Direction exitDir;
};

/**
 * CombatMap class
 */
class CombatMap : public Map {
public:
    CombatMap();
    CombatMap(MapId id);
        
    MonsterVector getMonsters();
    PartyMemberVector getPartyMembers();
    PartyMember* partyMemberAt(Coords coords);    
    Monster* monsterAt(Coords coords);    
    
    static MapId mapForTile(MapTile ground, MapTile transport, class Object *obj);

    bool isDungeonRoom() const;
    bool isAltarRoom() const;
    
    BaseVirtue getAltarRoom() const;
    void setAltarRoom(BaseVirtue ar);
    void setDungeonRoom(bool d);    
    
    // Properties
protected:
    bool dungeonRoom;
    BaseVirtue altarRoom;

public:
    Coords monster_start[AREA_MONSTERS];
    Coords player_start[AREA_PLAYERS];
};

bool isCombatMap(Map *punknown);
CombatMap *getCombatMap(Map *punknown = NULL);

#endif
