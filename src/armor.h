/*
 * $Id$
 */

#ifndef ARMOR_H
#define ARMOR_H

#include "savegame.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ARMORS     128

typedef struct _Armor {
    string name;    
    unsigned char canuse;
    int defense;
    unsigned short mask;
} Armor;

string *armorGetName(int weapon);
int armorGetDefense(int armor);
int armorCanWear(int armor, ClassType klass);
int armorGetByName(const char *name);

#ifdef __cplusplus
}
#endif

#endif
