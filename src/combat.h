/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include <map>

#include "direction.h"
#include "map.h"
#include "movement.h"
#include "savegame.h"
#include "types.h"

#define AREA_MONSTERS   16
#define AREA_PLAYERS    8

class Object;
class Monster;

typedef std::map<int, class Monster *, std::less<int> > CombatObjectMap;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT    
} CombatAction;

class CombatMap : public Map {
public:
    CombatMap();
    CombatMap(MapId id);

    // Methods
    void init(class Monster *m);
    void initCamping();
    void initDungeonRoom(int room, Direction from);
    void begin();
    bool addMonster(const class Monster *m, Coords coords);
    bool setActivePlayer(int player);
    bool putPlayerToSleep(int player);
    int partyMemberAt(Coords coords);    
    int monsterAt(Coords coords);
    void placePartyMembers();
    void fillMonsterTable(const Monster *monster);
    void placeMonsters();
    int initialNumberOfMonsters(const class Monster *monster);
    static MapId mapForTile(MapTile ground, MapTile transport, class Object *obj);
    void applyDamageToMonster(int monster, int damage, int player);
    void applyDamageToPlayer(int player, int damage);    

    int isWon(void);
    int isLost(void);
    void end(bool adjustKarma);
    void moveMonsters(void);
    void applyMonsterTileEffects(void);
        
//protected:
    int findTargetForMonster(Monster *monster, int *distance, int ranged);    
    int divideMonster(Monster *monster);
    int nearestPartyMember(Monster *obj, int *dist);
    int hideOrShowCamouflageMonster(Monster *monster);

    // Properties
public:
    unsigned char focus;

    CombatObjectMap party;
    CombatObjectMap monsters;

//protected:
    
    const class Monster *monsterTable[AREA_MONSTERS];
    class Monster *monster;

    Coords monster_start[AREA_MONSTERS];
    Coords player_start[AREA_PLAYERS];    

    bool dungeonRoom;
    BaseVirtue altarRoom;
    bool camping;
    bool inn;
    bool placePartyOnMap;
    bool placeMonstersOnMap;
    bool winOrLose;
    bool showCombatMessage;
    Direction exitDir;
};

bool isCombatMap(Map *punknown);
CombatMap *getCombatMap(Map *punknown = NULL);

void attackFlash(Coords coords, MapTile tile, int timeFactor);
void combatFinishTurn(void);
MoveReturnValue combatMovePartyMember(Direction dir, int userEvent);

/**
 * Key handlers
 */ 
bool combatBaseKeyHandler(int key, void *data);

#endif
