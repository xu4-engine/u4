#ifndef ARMOR_H
#define ARMOR_H

#define MAX_ARMORS     128

typedef struct _Armor {
    const char *name;    
    int defense;
    unsigned short mask;
} Armor;

char *armorGetName(int weapon);
int armorGetDefense(int armor);

#endif