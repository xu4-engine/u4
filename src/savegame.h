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

typedef enum {
    ITEM_SKULL  = 0x01,
    ITEM_CANDLE = 0x04,
    ITEM_BOOK   = 0x08,
    ITEM_BELL   = 0x10,
    ITEM_KEY_C  = 0x20,
    ITEM_KEY_L  = 0x40,
    ITEM_KEY_T  = 0x80
} Item;

typedef enum {
    STON_BLUE   = 0x01,
    STON_YELLOW = 0x02,
    STON_RED    = 0x04,
    STON_GREEN  = 0x08,
    STON_ORANGE = 0x10,
    STON_PURPLE = 0x20,
    STON_WHITE  = 0x40,
    STON_BLACK  = 0x80
} Stone;

typedef enum {
    RUNE_HONESTY      = 0x01,
    RUNE_COMPASSION   = 0x02,
    RUNE_VALOR        = 0x04,
    RUNE_JUSTICE      = 0x08,
    RUNE_SACRIFICE    = 0x10,
    RUNE_HONOR        = 0x20,
    RUNE_SPIRITUALITY = 0x40,
    RUNE_BLACK        = 0x80
} Rune;

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


typedef struct _SaveGame {
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
    unsigned short items;
    unsigned char x, y;
    unsigned char stones;
    unsigned char runes;
    unsigned short members;
    unsigned char partytile;
    char unknown2[27];
} SaveGame;

int saveGameWrite(const SaveGame *save, FILE *f);
int saveGameRead(SaveGame *save, FILE *f);
void saveGameInit(SaveGame *save, int x, int y, const SaveGamePlayerRecord *avatarInfo);
int saveGamePlayerRecordWrite(const SaveGamePlayerRecord *record, FILE *f);
int saveGamePlayerRecordRead(SaveGamePlayerRecord *record, FILE *f);
void saveGamePlayerRecordInit(SaveGamePlayerRecord *record);

#endif
