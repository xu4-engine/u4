/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#include "area.h"
#include "direction.h"
#include "map.h"
#include "movement.h"
#include "savegame.h"

#define FOCUS   combatInfo.partyFocus

class Object;
struct _Monster;
struct _Map;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT    
} CombatAction;

typedef struct _MonsterCombatInfo {
    class Object *obj;
    short hp;
    StatusType status;
} MonsterCombatInfo;

typedef struct _PartyCombatInfo {
    class Object *obj;
    StatusType status;
    SaveGamePlayerRecord *player;
} PartyCombatInfo;

typedef struct _CombatInfo {
    PartyCombatInfo party[AREA_PLAYERS];
    MonsterCombatInfo monsters[AREA_MONSTERS];

    unsigned char partyFocus;
    const struct _Monster *monsterTable[AREA_MONSTERS];
    const struct _Monster *monster;
    class Object *monsterObj;
    
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
    struct _Map *newCombatMap;
} CombatInfo;

void attackFlash(MapCoords coords, MapTile tile, int timeFactor);
void combatInit(const struct _Monster *m, class Object *monsterObj, MapId mapid);
void combatInitCamping(void);
void combatInitDungeonRoom(int room, Direction from);
void combatBegin();
int combatAddMonster(const struct _Monster *monster, MapCoords coords);
void combatFillMonsterTable(const struct _Monster *monster);
int combatSetActivePlayer(int player);
int combatPutPlayerToSleep(int player);
int combatPartyMemberAt(MapCoords coords);
int combatMonsterAt(MapCoords coords);
void combatPlacePartyMembers(void);
void combatPlaceMonsters(void);
void combatFinishTurn(void);
int combatInitialNumberOfMonsters(const struct _Monster *monster);
MapId combatMapForTile(MapTile groundTile, MapTile transport, class Object *obj);
void combatApplyDamageToMonster(int monster, int damage, int player);
MoveReturnValue combatMovePartyMember(Direction dir, int userEvent);

/**
 * Key handlers
 */ 
bool combatBaseKeyHandler(int key, void *data);

extern CombatInfo combatInfo;

#endif
