/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u4.h"
#include "stats.h"
#include "screen.h"
#include "context.h"
#include "savegame.h"

#define STATS_AREA_WIDTH 15
#define STATS_AREA_X TEXT_AREA_X

void statsAreaClear();
void statsAreaSetTitle(const char *title);
void statsShowPartyView();
void statsShowCharDetails(int charNo);
void statsShowWeapons();
void statsShowArmor();
void statsShowEquipment();
void statsShowItems();
void statsShowReagents();
void statsShowMixtures();
const char *statsClassName(ClassType klass);
const char *statsWeaponName(WeaponType weapon);
const char *statsWeaponAbbrev(WeaponType weapon);
const char *statsArmorName(ArmorType armor);

/**
 * Update the stats (ztats) box on the upper right of the screen.
 */
void statsUpdate() {

    statsAreaClear();

    switch(c->statsItem) {
    case STATS_PARTY_OVERVIEW:
        statsShowPartyView();
        break;
    case STATS_CHAR1:
    case STATS_CHAR2:
    case STATS_CHAR3:
    case STATS_CHAR4:
    case STATS_CHAR5:
    case STATS_CHAR6:
    case STATS_CHAR7:
    case STATS_CHAR8:
        statsShowCharDetails(c->statsItem - STATS_CHAR1);
        break;
    case STATS_WEAPONS:
        statsShowWeapons();
        break;
    case STATS_ARMOR:
        statsShowArmor();
        break;
    case STATS_EQUIPMENT:
        statsShowEquipment();
        break;
    case STATS_ITEMS:
        statsShowItems();
        break;
    case STATS_REAGENTS:
        statsShowReagents();
        break;
    case STATS_MIXTURES:
        statsShowMixtures();
        break;
    }
}

void statsAreaClear() {
    int i;

    for (i = 0; i < STATS_AREA_WIDTH; i++)
        screenTextAt(STATS_AREA_X + i, 0, "%c", 13);
    
    for (i = 1; i < 9; i++)
        screenTextAt(STATS_AREA_X, i, "               ");
}

/**
 * Sets the title of the stats area.
 */
void statsAreaSetTitle(const char *title) {
    int titleStart;

    titleStart = (STATS_AREA_WIDTH / 2) - ((strlen(title) + 2) / 2);
    screenTextAt(STATS_AREA_X + titleStart, 0, "%c%s%c", 16, title, 17);
}

/**
 * The basic party view.
 */
void statsShowPartyView() {
    int i;

    assert(c->saveGame->members <= 8);

    for (i = 0; i < c->saveGame->members; i++)
        screenTextAt(STATS_AREA_X, 1+i, "%d-%-9s%03d%c", i+1, c->saveGame->players[i].name, c->saveGame->players[i].hp, c->saveGame->players[i].status);

    screenTextAt(STATS_AREA_X, 10, "F:%04d   G:%04d", c->saveGame->food / 100, c->saveGame->gold);
}

/**
 * The individual character view.
 */
void statsShowCharDetails(int charNo) {
    const char *classString;
    int classStart;

    assert(charNo < 8);

    statsAreaSetTitle(c->saveGame->players[charNo].name);
    screenTextAt(STATS_AREA_X, 1, "%c             %c", c->saveGame->players[charNo].sex, c->saveGame->players[charNo].status);
    classString = statsClassName(c->saveGame->players[charNo].klass);
    classStart = (STATS_AREA_WIDTH / 2) - (strlen(classString) / 2);
    screenTextAt(STATS_AREA_X + classStart, 1, "%s", classString);
    screenTextAt(STATS_AREA_X, 3, " MP:%02d  LV:%d", c->saveGame->players[charNo].mp, c->saveGame->players[charNo].hpMax / 100);
    screenTextAt(STATS_AREA_X, 4, "STR:%02d  HP:%04d", c->saveGame->players[charNo].str, c->saveGame->players[charNo].hp);
    screenTextAt(STATS_AREA_X, 5, "DEX:%02d  HM:%04d", c->saveGame->players[charNo].dex, c->saveGame->players[charNo].hpMax);
    screenTextAt(STATS_AREA_X, 6, "INT:%02d  EX:%04d", c->saveGame->players[charNo].intel, c->saveGame->players[charNo].xp);
    screenTextAt(STATS_AREA_X, 7, "W:%s", statsWeaponName(c->saveGame->players[charNo].weapon));
    screenTextAt(STATS_AREA_X, 8, "A:%s", statsArmorName(c->saveGame->players[charNo].armor));
}

/**
 * Weapons in inventory.
 */
void statsShowWeapons() {
    int w, line, col;

    statsAreaSetTitle("Weapons");

    line = 1;
    col = 0;
    screenTextAt(STATS_AREA_X, line++, "A-%s", statsWeaponName(WEAP_HANDS));
    for (w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        if (c->saveGame->weapons[w] > 0)
            screenTextAt(STATS_AREA_X + col, line++, "%c-%d-%s", w - WEAP_HANDS + 'A', c->saveGame->weapons[w], statsWeaponAbbrev(w));
    }
}

/**
 * Armor in inventory.
 */
void statsShowArmor() {
    int a, line;

    statsAreaSetTitle("Armour");

    line = 1;
    screenTextAt(STATS_AREA_X, line++, "A  -No Armour");
    for (a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0)
            screenTextAt(STATS_AREA_X, line++, "%c-%d-%s", a - ARMR_NONE + 'A', c->saveGame->armor[a], statsArmorName(a));
    }
}

void statsShowEquipment() {
    int line;

    statsAreaSetTitle("Equipment");
    
    line = 1;
    screenTextAt(STATS_AREA_X, line++, "%2d-Torches", c->saveGame->torches);
    screenTextAt(STATS_AREA_X, line++, "%2d-Gems", c->saveGame->gems);
    screenTextAt(STATS_AREA_X, line++, "%2d-Keys", c->saveGame->keys);
    if (c->saveGame->sextants > 0)
        screenTextAt(STATS_AREA_X, line++, "%2d-Sextants", c->saveGame->sextants);
}

void statsShowItems() {
    statsAreaSetTitle("Items");
    /* FIXME */
}

void statsShowReagents() {
    statsAreaSetTitle("Reagents");
    /* FIXME */
}

void statsShowMixtures() {
    statsAreaSetTitle("Mixtures");
    /* FIXME */
}

const char *statsClassName(ClassType klass) {
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

const char *statsWeaponName(WeaponType weapon) {
    static const char *weapNames[] = {
        "Hands", "Staff", "Dagger",
        "Sling", "Mace", "Axe",
        "Sword", "Bow", "Crossbow",
        "Flaming Oil", "Halberd", "Magic Axe",
        "Magic Sword", "Magic Bow", "Magic Wand",
        "Mystic Sword"
    };

    if (weapon >= WEAP_HANDS && weapon < WEAP_MAX)
        return weapNames[weapon - WEAP_HANDS];
    else
        return "???";
}

const char *statsWeaponAbbrev(WeaponType weapon) {
    static const char *weapAbbrevs[] = {
        "HND", "STF", "DAG",
        "SLN", "MAC", "AXE",
        "SWD", "BOW", "XBO",
        "OIL", "HAL", "+AX",
        "+SW", "+BO", "WND",
        "^SW"
    };

    if (weapon >= WEAP_HANDS && weapon < WEAP_MAX)
        return weapAbbrevs[weapon - WEAP_HANDS];
    else
        return "???";
}

const char *statsArmorName(ArmorType armor) {
    static const char *armorNames[] = {
        "Skin", "Cloth", "Leather", 
        "Chain Mail", "Plate Mail", 
        "Magic Chain", "Magic Plate", "Mystic Robe"
    };

    if (armor >= ARMR_NONE && armor < ARMR_MAX)
        return armorNames[armor - ARMR_NONE];
    else
        return "???";
}
