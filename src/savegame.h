/*
 * $Id$
 */

#ifndef SAVEGAME_H
#define SAVEGAME_H

typedef enum {
    WEAP_HANDS,
    WEAP_STAFF, 
    WEAP_DAGGER,
    WEAP_SLING,
    WEAP_MACE,
    WEAP_AXE,
    WEAP_SWORD,
    WEAP_BOW,
    WEAP_CROSSBOW,
    WEAP_OIL,
    WEAP_HALBERD,
    WEAP_MAGICAXE,
    WEAP_MAGICSWORD,
    WEAP_MAGICBOW,
    WEAP_MAGICWAND,
    WEAP_MYSTICSWORD,
    WEAP_MAX
} WeaponType;

typedef enum {
    ARMR_NONE,
    ARMR_CLOTH,
    ARMR_LEATHER,
    ARMR_CHAIN,
    ARMR_PLATE,
    ARMR_MAGICCHAIN,
    ARMR_MAGICPLATE,
    ARMR_MYSTICROBES,
    ARMR_MAX
} ArmorType;

typedef enum {
    SEX_MALE = 0xd,
    SEX_FEMALE = 0xd
} SexType;

typedef enum {
    STAT_GOOD = 'G',
    STAT_POISONED = 'P',
    STAT_SLEEPING = 'S',
    STAT_DEAD = 'D'
} StatusType;

typedef enum {
    VIRT_HONESTY,
    VIRT_COMPASSION,
    VIRT_VALOR,
    VIRT_JUSTICE,
    VIRT_SACRIFICE,
    VIRT_HONOR,
    VIRT_SPIRITUALITY,
    VIRT_HUMILITY,
} Virtue;

typedef enum {
    REAG_ASH,
    REAG_GINSENG,
    REAG_GARLIC,
    REAG_SILK,
    REAG_MOSS,
    REAG_PEARL,
    REAG_NIGHTSHADE,
    REAG_MANDRAKE,
    REAG_MAX
} Reagent;

typedef struct {
    unsigned short hp;
    unsigned short hpMax;
    unsigned short xp;
    unsigned short str, dex, intel;
    unsigned short mp;
    unsigned short unknown;
    WeaponType weapon;
    ArmorType armor;
    char name[16];
    SexType sex;
    unsigned char class;
    StatusType status;
} SaveGamePlayerRecord;


typedef struct {
    unsigned int unknown1;
    unsigned int moves;
    SaveGamePlayerRecord players[8];
    unsigned int food;
    unsigned short gold;
    unsigned short karma[8];
    unsigned short torches;
    unsigned short gems;
    unsigned short keys;
    unsigned short sextants;
    unsigned short armor[ARMR_MAX];
    unsigned short weapons[WEAP_MAX];
    unsigned short reagents[REAG_MAX];
    unsigned short mixtures[26];
    unsigned short unknown2;
    unsigned char x, y;
    char unknown3[32];
} SaveGame;

int saveGameWrite(const SaveGame *save, FILE *f);
int saveGameRead(SaveGame *save, FILE *f);
int saveGamePlayerRecordWrite(const SaveGamePlayerRecord *record, FILE *f);
int saveGamePlayerRecordRead(SaveGamePlayerRecord *record, FILE *f);

#endif
