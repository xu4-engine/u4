#ifndef WEAPON_H
#define WEAPON_H

#define MAX_WEAPONS     128

typedef struct _Weapon {
    const char *name;
    int range;
    int damage;
    int hittile;
    int misstile;
    int leavetile;
    unsigned short mask;
} Weapon;

char *weaponGetName(int weapon);
int weaponGetRange(int weapon);
int weaponGetDamage(int weapon);
int weaponGetHitTile(int weapon);
int weaponGetMissTile(int weapon);
int weaponAlwaysHit(int weapon);
int weaponLeavesTile(int weapon);

#endif