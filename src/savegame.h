/*
 * $Id$
 */

#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PARTY_SAV_BASE_FILENAME         "party.sav"
#define MONSTERS_SAV_BASE_FILENAME      "monsters.sav"
#define OUTMONST_SAV_BASE_FILENAME      "outmonst.sav"

class Object;

/**
 * The list of all weapons.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
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

/**
 * The list of all armor types.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
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

/**
 * The list of sex values for the savegame character records.  The
 * values match the male and female symbols in the character set.
 */
typedef enum {
    SEX_MALE = 0xb,
    SEX_FEMALE = 0xc
} SexType;

/**
 * The list of class types for the savegame character records.
 */
typedef enum {
    CLASS_MAGE,
    CLASS_BARD,
    CLASS_FIGHTER,
    CLASS_DRUID,
    CLASS_TINKER,
    CLASS_PALADIN,
    CLASS_RANGER,
    CLASS_SHEPHERD
} ClassType;

/**
 * The list of status values for the savegame character records.  The
 * values match the letter thats appear in the ztats area.
 */
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
    VIRT_MAX
} Virtue;

typedef enum {
    VIRT_TRUTH      = 0x01,
    VIRT_LOVE       = 0x02,
    VIRT_COURAGE    = 0x04
} Principle;

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

#define SPELL_MAX 26

typedef enum {
    ITEM_SKULL  = 0x01,
    ITEM_SKULL_DESTROYED = 0x02,
    ITEM_CANDLE = 0x04,
    ITEM_BOOK   = 0x08,
    ITEM_BELL   = 0x10,
    ITEM_KEY_C  = 0x20,
    ITEM_KEY_L  = 0x40,
    ITEM_KEY_T  = 0x80,
    ITEM_HORN   = 0x100,
    ITEM_WHEEL  = 0x200,
    ITEM_CANDLE_USED = 0x400,
    ITEM_BOOK_USED = 0x800,
    ITEM_BELL_USED = 0x1000
} Item;

typedef enum {
    STONE_BLUE   = 0x01,
    STONE_YELLOW = 0x02,
    STONE_RED    = 0x04,
    STONE_GREEN  = 0x08,
    STONE_ORANGE = 0x10,
    STONE_PURPLE = 0x20,
    STONE_WHITE  = 0x40,
    STONE_BLACK  = 0x80
} Stone;

typedef enum {
    RUNE_HONESTY      = 0x01,
    RUNE_COMPASSION   = 0x02,
    RUNE_VALOR        = 0x04,
    RUNE_JUSTICE      = 0x08,
    RUNE_SACRIFICE    = 0x10,
    RUNE_HONOR        = 0x20,
    RUNE_SPIRITUALITY = 0x40,
    RUNE_HUMILITY     = 0x80
} Rune;

/**
 * The for Ultima IV savegame player record data.  
 */
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
    ClassType klass;
    StatusType status;
} SaveGamePlayerRecord;

/**
 * Ultima IV savegame data.  This structure approximately mirrors the
 * disk structure of PARTY.SAV.
 */
typedef struct _SaveGame {
    unsigned int unknown1;
    unsigned int moves;
    SaveGamePlayerRecord players[8];
    int food;
    short gold;
    short karma[VIRT_MAX];
    short torches;
    short gems;
    short keys;
    short sextants;
    short armor[ARMR_MAX];
    short weapons[WEAP_MAX];
    short reagents[REAG_MAX];
    short mixtures[SPELL_MAX];
    unsigned short items;
    unsigned char x, y;
    unsigned char stones;
    unsigned char runes;
    unsigned short members;
    unsigned short transport;
    union {
        unsigned short balloonstate;
        unsigned short torchduration;
    };
    unsigned short trammelphase;
    unsigned short feluccaphase;
    unsigned short shiphull;
    unsigned short lbintro;
    unsigned short lastcamp;
    unsigned short lastreagent;
    unsigned short lastmeditation;
    unsigned short lastvirtue;
    unsigned char dngx, dngy;
    unsigned short orientation;
    unsigned short dnglevel;
    unsigned short location;
} SaveGame;

char *partySavFilename(void);
char *monstersSavFilename(void);
FILE *saveGameOpenForWriting(void);
FILE *saveGameOpenForReading(void);
FILE *saveGameMonstersOpenForWriting(const char *filename);
FILE *saveGameMonstersOpenForReading(const char *filename);
int saveGameWrite(const SaveGame *save, FILE *f);
int saveGameRead(SaveGame *save, FILE *f);
void saveGameInit(SaveGame *save, const SaveGamePlayerRecord *avatarInfo);
int saveGamePlayerRecordWrite(const SaveGamePlayerRecord *record, FILE *f);
int saveGamePlayerRecordRead(SaveGamePlayerRecord *record, FILE *f);
void saveGamePlayerRecordInit(SaveGamePlayerRecord *record);
int saveGameMonstersWrite(const class Object *objs, FILE *f);
int saveGameMonstersRead(class Object **objs, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
