/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include <map>

#include "direction.h"
#include "map.h"
#include "creature.h"
#include "movement.h"
#include "object.h"
#include "player.h"
#include "savegame.h"
#include "types.h"

#define AREA_CREATURES   16
#define AREA_PLAYERS    8

class Object;
class Creature;

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
    Creature *     getCreature() const;
    PartyMemberVector* getParty();
    PartyMember*  getCurrentPlayer();
    
    void setExitDir(Direction d);
    void setInn(bool i = true);
    void setCreature(Creature *);
    void setWinOrLose(bool worl = true);
    void showCombatMessage(bool show = true);

    // Methods
    void init(class Creature *m);
    void initCamping();
    void initDungeonRoom(int room, Direction from);
    
    void applyCreatureTileEffects();
    void begin();
    void end(bool adjustKarma);
    void fillCreatureTable(const Creature *creature);
    int  initialNumberOfCreatures(const class Creature *creature) const;
    bool isWon() const;
    bool isLost() const;
    void moveCreatures();
    void placeCreatures();
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

    const class Creature *creatureTable[AREA_CREATURES];
    class Creature *creature;    

    bool camping;
    bool inn;
    bool placePartyOnMap;
    bool placeCreaturesOnMap;
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
        
    CreatureVector getCreatures();
    PartyMemberVector getPartyMembers();
    PartyMember* partyMemberAt(Coords coords);    
    Creature* creatureAt(Coords coords);    
    
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
    Coords creature_start[AREA_CREATURES];
    Coords player_start[AREA_PLAYERS];
};

bool isCombatMap(Map *punknown);
CombatMap *getCombatMap(Map *punknown = NULL);

#endif
