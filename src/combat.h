/*
 * $Id$
 */

#ifndef COMBAT_H
#define COMBAT_H

#define FOCUS   combatInfo.partyFocus

#include "area.h"
#include "direction.h"
#include "savegame.h"

struct _Object;
struct _Monster;
struct _Map;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT,
    CA_SHOW,
    CA_HIDE
} CombatAction;

typedef struct _MonsterCombatInfo {
    struct _Object *obj;
    short hp;
    StatusType status;
} MonsterCombatInfo;

typedef struct _PartyCombatInfo {
    struct _Object *obj;
    StatusType status;
    SaveGamePlayerRecord *player;
} PartyCombatInfo;

typedef struct _CombatInfo {
    PartyCombatInfo party[AREA_PLAYERS];
    MonsterCombatInfo monsters[AREA_MONSTERS];

    unsigned char partyFocus;
    const struct _Monster *monsterTable[AREA_MONSTERS];
    const struct _Monster *monster;
    struct _Object *monsterObj;
    
    struct { unsigned char x, y; } partyStartCoords[AREA_PLAYERS];
    struct { unsigned char x, y; } monsterStartCoords[AREA_MONSTERS];

    unsigned char camping;
    unsigned char placeParty;
    unsigned char placeMonsters;    
    unsigned char winOrLose;
    struct _Map *newCombatMap;
} CombatInfo;

void attackFlash(int x, int y, unsigned char tile, int timeFactor);
void combatInit(const struct _Monster *m, struct _Object *monsterObj, unsigned char mapid, unsigned char camping);
void combatInitCamping(void);
void combatInitDungeonRoom(int room, Direction from);
void combatBegin();
void combatFillMonsterTable(const struct _Monster *monster);
int combatSetActivePlayer(int player);
int combatPutPlayerToSleep(int player);
int combatPartyMemberAt(int x, int y, int z);
int combatMonsterAt(int x, int y, int z);
void combatPlacePartyMembers(void);
void combatPlaceMonsters(void);
void combatFinishTurn(void);
int combatInitialNumberOfMonsters(const struct _Monster *monster);
unsigned char combatMapForTile(unsigned char groundTile, unsigned char transport, struct _Object *obj);
void combatApplyDamageToMonster(int monster, int damage, int player);
int combatBaseKeyHandler(int key, void *data);
int movePartyMember(Direction dir, int userEvent);

extern CombatInfo combatInfo;

#endif
