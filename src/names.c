/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "names.h"

const char *getClassName(ClassType klass) {
    switch (klass) {
    case CLASS_MAGE:
        return "Mage";
    case CLASS_BARD:
        return "Bard";
    case CLASS_FIGHTER:
        return "Fighter";
    case CLASS_DRUID:
        return "Druid";
    case CLASS_TINKER:
        return "Tinker";
    case CLASS_PALADIN:
        return "Paladin";
    case CLASS_RANGER:
        return "Ranger";
    case CLASS_SHEPHERD:
        return "Shepherd";
    default:
        return "???";
    }
}

const char *getWeaponName(WeaponType weapon) {
    static const char *weapNames[] = {
        "Hands", "Staff", "Dagger",
        "Sling", "Mace", "Axe",
        "Sword", "Bow", "Crossbow",
        "Flaming Oil", "Halberd", "Magic Axe",
        "Magic Sword", "Magic Bow", "Magic Wand",
        "Mystic Sword"
    };

    if (weapon < WEAP_MAX)
        return weapNames[weapon - WEAP_HANDS];
    else
        return "???";
}

const char *getWeaponAbbrev(WeaponType weapon) {
    static const char *weapAbbrevs[] = {
        "HND", "STF", "DAG",
        "SLN", "MAC", "AXE",
        "SWD", "BOW", "XBO",
        "OIL", "HAL", "+AX",
        "+SW", "+BO", "WND",
        "^SW"
    };

    if (weapon < WEAP_MAX)
        return weapAbbrevs[weapon - WEAP_HANDS];
    else
        return "???";
}

const char *getArmorName(ArmorType armor) {
    static const char *armorNames[] = {
        "Skin", "Cloth", "Leather", 
        "Chain Mail", "Plate Mail", 
        "Magic Chain", "Magic Plate", "Mystic Robe"
    };

    if (armor < ARMR_MAX)
        return armorNames[armor - ARMR_NONE];
    else
        return "???";
}

const char *getReagentName(Reagent reagent) {
    static const char *reagentNames[] = {
        "Sulfur Ash", "Ginseng", "Garlic", 
        "Spider Silk", "Blood Moss", "Black Pearl", 
        "Nightshade", "Mandrake"
    };

    if (reagent < REAG_MAX)
        return reagentNames[reagent - REAG_ASH];
    else
        return "???";
}

const char *getVirtueName(Virtue virtue) {
    static const char *virtueNames[] = {
        "Honesty", "Compassion", "Valor", 
        "Justice", "Sacrifice", "Honor", 
        "Spirituality", "Humility"
    };

    if (virtue < 8)
        return virtueNames[virtue - VIRT_HONESTY];
    else
        return "???";
}

const char *getStoneName(Virtue virtue) {
    static const char *virtueNames[] = {
        "Blue", "Yellow", "Red", 
        "Green", "Orange", "Purple", 
        "White", "Black"
    };

    if (virtue < VIRT_MAX)
        return virtueNames[virtue - VIRT_HONESTY];
    else
        return "???";
}

const char *getItemName(Item item) {
    switch (item) {
    case ITEM_SKULL:
        return "Skull";
    case ITEM_CANDLE:
        return "Candle";
    case ITEM_BOOK:
        return "Book";
    case ITEM_BELL:
        return "Bell";
    case ITEM_KEY_C:
        return "Courage";
    case ITEM_KEY_L:
        return "Love";
    case ITEM_KEY_T:
        return "Truth";
    case ITEM_HORN:
        return "Horn";
    case ITEM_WHEEL:
        return "Wheel";
    default:
        return "???";
    }
}
