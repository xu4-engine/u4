/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#include "savegame.h"
#include "types.h"

using namespace xu4;

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WEAPONS     128

typedef struct _Weapon {
    string name;
    string abbr;
    unsigned char canuse;
    int range;
    int damage;
    int hittile;
    int misstile;
    int leavetile;
    unsigned short mask;
} Weapon;

string *weaponGetName(int weapon);
string *weaponGetAbbrev(int weapon);
int weaponGetRange(int weapon);
int weaponGetDamage(int weapon);
int weaponGetHitTile(int weapon);
int weaponGetMissTile(int weapon);
int weaponAlwaysHits(int weapon);
unsigned char weaponLeavesTile(int weapon);
int weaponCanReady(int weapon, ClassType klass);
int weaponCanAttackThroughObjects(int weapon);
int weaponRangeAbsolute(int weapon);
int weaponReturns(int weapon);
int weaponLoseWhenUsed(int weapon);
int weaponLoseWhenRanged(int weapon);
int weaponCanChooseDistance(int weapon);
int weaponIsMagic(int weapon);
int weaponShowTravel(int weapon);
int weaponGetByName(const char *name);

#ifdef __cplusplus
}
#endif

#endif
