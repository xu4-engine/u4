/*
 * savegame.h
 */

#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <stdio.h>
#include <stdint.h>

#define PARTY_SAV           "party.sav"
#define MONSTERS_SAV        "monsters.sav"
#define DNGMAP_SAV          "dngmap.sav"
#define OUTMONST_SAV        "outmonst.sav"

#define MONSTERTABLE_SIZE               32
#define MONSTERTABLE_CREATURES_SIZE     8
#define MONSTERTABLE_OBJECTS_SIZE       (MONSTERTABLE_SIZE - MONSTERTABLE_CREATURES_SIZE)

/**
 * The list of all weapons.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum WeaponType {
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
};

/**
 * The list of all armor types.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum ArmorType {
    ARMR_NONE,
    ARMR_CLOTH,
    ARMR_LEATHER,
    ARMR_CHAIN,
    ARMR_PLATE,
    ARMR_MAGICCHAIN,
    ARMR_MAGICPLATE,
    ARMR_MYSTICROBES,
    ARMR_MAX
};

/**
 * The list of sex values for the savegame character records.  The
 * values match the male and female symbols in the character set.
 */
enum SexType {
    SEX_MALE = 0xb,
    SEX_FEMALE = 0xc
};

/**
 * The list of class types for the savegame character records.
 */
enum ClassType {
    CLASS_MAGE,
    CLASS_BARD,
    CLASS_FIGHTER,
    CLASS_DRUID,
    CLASS_TINKER,
    CLASS_PALADIN,
    CLASS_RANGER,
    CLASS_SHEPHERD
};

/**
 * The list of status values for the savegame character records.  The
 * values match the letter thats appear in the ztats area.
 */
enum StatusType {
    STAT_GOOD = 'G',
    STAT_POISONED = 'P',
    STAT_SLEEPING = 'S',
    STAT_DEAD = 'D'
};

enum Virtue {
    VIRT_HONESTY,
    VIRT_COMPASSION,
    VIRT_VALOR,
    VIRT_JUSTICE,
    VIRT_SACRIFICE,
    VIRT_HONOR,
    VIRT_SPIRITUALITY,
    VIRT_HUMILITY,
    VIRT_MAX
};

enum BaseVirtue {
    VIRT_NONE       = 0x00,
    VIRT_TRUTH      = 0x01,
    VIRT_LOVE       = 0x02,
    VIRT_COURAGE    = 0x04
};

enum Reagent {
    REAG_ASH,
    REAG_GINSENG,
    REAG_GARLIC,
    REAG_SILK,
    REAG_MOSS,
    REAG_PEARL,
    REAG_NIGHTSHADE,
    REAG_MANDRAKE,
    REAG_MAX
};

#define SPELL_MAX 26

enum Item {
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
};

enum Stone {
    STONE_BLUE   = 0x01,
    STONE_YELLOW = 0x02,
    STONE_RED    = 0x04,
    STONE_GREEN  = 0x08,
    STONE_ORANGE = 0x10,
    STONE_PURPLE = 0x20,
    STONE_WHITE  = 0x40,
    STONE_BLACK  = 0x80
};

enum Rune {
    RUNE_HONESTY      = 0x01,
    RUNE_COMPASSION   = 0x02,
    RUNE_VALOR        = 0x04,
    RUNE_JUSTICE      = 0x08,
    RUNE_SACRIFICE    = 0x10,
    RUNE_HONOR        = 0x20,
    RUNE_SPIRITUALITY = 0x40,
    RUNE_HUMILITY     = 0x80
};

/**
 * The Ultima IV savegame player record data.
 */
struct SaveGamePlayerRecord {
    int write(FILE *f) const;
    int read(FILE *f);
    void init();

    uint16_t hp;
    uint16_t hpMax;
    uint16_t xp;
    uint16_t str, dex, intel;
    uint16_t mp;
    uint16_t unknown;
    WeaponType weapon;
    ArmorType armor;
    char name[16];
    SexType sex;
    ClassType klass;
    StatusType status;
};

/**
 * How Ultima IV stores monster information
 */
typedef struct _SaveGameMonsterRecord {
    uint8_t tile;
    uint8_t x;
    uint8_t y;
    uint8_t prevTile;
    uint8_t prevx;
    uint8_t prevy;
    uint8_t level;
    uint8_t unused;
} SaveGameMonsterRecord;

/**
 * Represents the on-disk contents of PARTY.SAV.
 */
struct SaveGame {
    int write(FILE *f) const;
    int read(FILE *f);
    void init(const SaveGamePlayerRecord *avatarInfo);

    uint32_t unknown1;
    uint32_t moves;
    SaveGamePlayerRecord players[8];
    int32_t food;
    int16_t gold;
    int16_t karma[VIRT_MAX];
    int16_t torches;
    int16_t gems;
    int16_t keys;
    int16_t sextants;
    int16_t armor[ARMR_MAX];
    int16_t weapons[WEAP_MAX];
    int16_t reagents[REAG_MAX];
    int16_t mixtures[SPELL_MAX];
    uint16_t items;
    uint8_t x, y;
    uint8_t stones;
    uint8_t runes;
    uint16_t members;
    uint16_t transport;
    union {
        uint16_t balloonstate;
        uint16_t torchduration;
    };
    uint16_t trammelphase;
    uint16_t feluccaphase;
    uint16_t shiphull;
    uint16_t lbintro;
    uint16_t lastcamp;
    uint16_t lastreagent;
    uint16_t lastmeditation;
    uint16_t lastvirtue;
    uint8_t dngx, dngy;
    uint16_t orientation;
    uint16_t dnglevel;
    uint16_t location;
};

int saveGameMonstersWrite(const SaveGameMonsterRecord *monsterTable, FILE *f);
int saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, FILE *f);

#endif
