/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include <map>

#include "direction.h"
#include "map.h"
#include "controller.h"
#include "creature.h"
#include "object.h"
#include "observer.h"
#include "player.h"
#include "savegame.h"
#include "types.h"

#define AREA_CREATURES   16
#define AREA_PLAYERS    8

class CombatMap;
class Creature;
class MoveEvent;
class Weapon;

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
class CombatController : public Controller, public Observer<Party *, PartyEvent &> {
public:
    CombatController();
    CombatController(CombatMap *m);
    CombatController(MapId id);
    virtual ~CombatController();

    // Accessor Methods    
    bool          isCamping() const;
    bool          isInn() const;    
    bool          isWinOrLose() const;
    Direction     getExitDir() const;
    unsigned char getFocus() const;
    CombatMap *   getMap() const;
    Creature *    getCreature() const;
    PartyMemberVector* getParty();
    PartyMember*  getCurrentPlayer();
    
    void setExitDir(Direction d);
    void setInn(bool i = true);
    void setCreature(Creature *);
    void setWinOrLose(bool worl = true);
    void showCombatMessage(bool show = true);

    // Methods
    void init(Creature *m);
    void initCamping();
    void initDungeonRoom(int room, Direction from);
    
    void applyCreatureTileEffects();
    void begin();
    void end(bool adjustKarma);
    void fillCreatureTable(const Creature *creature);
    int  initialNumberOfCreatures(const Creature *creature) const;
    bool isWon() const;
    bool isLost() const;
    void moveCreatures();
    void placeCreatures();
    void placePartyMembers();
    bool setActivePlayer(int player);

    // attack functions
    bool attackAt(const Coords &coords, PartyMember *attacker, int dir, int range, int distance);
    bool rangedAttack(const Coords &coords, Creature *attacker);
    void rangedMiss(const Coords &coords, Creature *attacker);
    bool returnWeaponToOwner(const Coords &coords, int distance, int dir, const Weapon *weapon);

    /** 
     * Static member functions
     */
    static void attackFlash(Coords coords, MapTile tile, int timeFactor);
    static void finishTurn(void);
    static void movePartyMember(MoveEvent &event);

    // Key handlers
    virtual bool keyPressed(int key);
    static bool chooseWeaponDir(int key, void *data);

    virtual void update(Party *party, PartyEvent &event);

    // Properties
protected:
    CombatMap *map;
    
    PartyMemberVector party;
    unsigned char focus;

    const Creature *creatureTable[AREA_CREATURES];
    Creature *creature;    

    bool camping;
    bool inn;
    bool placePartyOnMap;
    bool placeCreaturesOnMap;
    bool winOrLose;
    bool showMessage;
    Direction exitDir;

private:
    CombatController(const CombatController&);
    const CombatController &operator=(const CombatController&);
};

/**
 * CombatMap class
 */
class CombatMap : public Map {
public:
    CombatMap();
        
    CreatureVector getCreatures();
    PartyMemberVector getPartyMembers();
    PartyMember* partyMemberAt(Coords coords);    
    Creature* creatureAt(Coords coords);    
    
    static MapId mapForTile(MapTile ground, MapTile transport, Object *obj);

    bool isDungeonRoom() const;
    bool isAltarRoom() const;
    bool isContextual() const;
    
    BaseVirtue getAltarRoom() const;
    void setAltarRoom(BaseVirtue ar);
    void setDungeonRoom(bool d);    
    void setContextual(bool c = true);
    
    // Properties
protected:
    bool dungeonRoom;
    BaseVirtue altarRoom;
    bool contextual;

public:
    Coords creature_start[AREA_CREATURES];
    Coords player_start[AREA_PLAYERS];
};

bool isCombatMap(Map *punknown);
CombatMap *getCombatMap(Map *punknown = NULL);

#endif
