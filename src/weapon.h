/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#define MAX_WEAPONS     128

typedef struct _Weapon {
    const char *name;
    const char *abbr;
    const char *canready;
    const char *cantready;
    int range;
    int damage;
    int hittile;
    int misstile;
    int leavetile;
    unsigned short mask;
} Weapon;

char *weaponGetName(int weapon);
char *weaponGetAbbrev(int weapon);
int weaponGetRange(int weapon);
int weaponGetDamage(int weapon);
int weaponGetHitTile(int weapon);
int weaponGetMissTile(int weapon);
int weaponAlwaysHits(int weapon);
unsigned char weaponLeavesTile(int weapon);
int weaponCanReady(int weapon, const char *className);
int weaponCanAttackThroughObjects(int weapon);
int weaponRangeAbsolute(int weapon);
int weaponReturns(int weapon);
int weaponLoseWhenUsed(int weapon);
int weaponLoseWhenRanged(int weapon);
int weaponCanChooseDistance(int weapon);
int weaponIsMagic(int weapon);
int weaponShowTravel(int weapon);

#endif
