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
#define FOCUS           combatInfo.partyFocus

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
    CombatMap() {}

    Coords monster_start[AREA_MONSTERS];
    Coords player_start[AREA_PLAYERS];
};

typedef struct _CombatInfo {
    unsigned char partyFocus;
    const class Monster *monsterTable[AREA_MONSTERS];
    class Monster *monster;

    CombatObjectMap party;
    CombatObjectMap monsters;
    
    MapCoords partyStartCoords[AREA_PLAYERS];
    MapCoords monsterStartCoords[AREA_MONSTERS];

    unsigned char dungeonRoom;
    unsigned char altarRoom;
    unsigned char camping;
    unsigned char inn;    
    unsigned char placeParty;
    unsigned char placeMonsters;    
    unsigned char winOrLose;
    unsigned char showCombatMessage;
    Direction exitDir;
    CombatMap *newCombatMap;
} CombatInfo;

void attackFlash(Coords coords, MapTile tile, int timeFactor);
void combatInit(class Monster *m, MapId mapid);
void combatInitCamping(void);
void combatInitDungeonRoom(int room, Direction from);
void combatBegin();
int combatAddMonster(const class Monster *monster, Coords coords);
void combatFillMonsterTable(const class Monster *monster);
int combatSetActivePlayer(int player);
int combatPutPlayerToSleep(int player);
int combatPartyMemberAt(Coords coords);
int combatMonsterAt(Coords coords);
void combatPlacePartyMembers(void);
void combatPlaceMonsters(void);
void combatFinishTurn(void);
int combatInitialNumberOfMonsters(const class Monster *monster);
MapId combatMapForTile(MapTile groundTile, MapTile transport, class Object *obj);
void combatApplyDamageToMonster(int monster, int damage, int player);
void combatApplyDamageToPlayer(int player, int damage);
MoveReturnValue combatMovePartyMember(Direction dir, int userEvent);

/**
 * Key handlers
 */ 
bool combatBaseKeyHandler(int key, void *data);

extern CombatInfo combatInfo;

bool isCombatMap(Map *punknown);

#endif
