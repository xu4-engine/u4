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

void statsAreaClear();
void statsShowPartyView();
void statsShowCharDetails(int charNo);
const char *statsClassName(ClassType klass);
const char *statsWeaponName(WeaponType weapon);
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
        break;
    case STATS_ARMOR:
        break;
    case STATS_EQUIPMENT:
        break;
    case STATS_ITEMS:
        break;
    case STATS_REAGENTS:
        break;
    case STATS_MIXTURES:
        break;
    }
}

void statsAreaClear() {
    int i;

    for (i = 0; i < 15; i++)
        screenTextAt(TEXT_AREA_X + i, 0, "%c", 13);
    
    for (i = 1; i < 8; i++)
        screenTextAt(TEXT_AREA_X, i, "               ");
}

/**
 * The basic party view.
 */
void statsShowPartyView() {
    int i;

    assert(c->saveGame->members <= 8);

    for (i = 0; i < c->saveGame->members; i++)
        screenTextAt(TEXT_AREA_X, 1+i, "%d-%-9s%03d%c", i+1, c->saveGame->players[i].name, c->saveGame->players[i].hp, c->saveGame->players[i].status);

    screenTextAt(TEXT_AREA_X, 10, "F:%04d   G:%04d", c->saveGame->food / 100, c->saveGame->gold);
}

/**
 * The individual character view.
 */
void statsShowCharDetails(int charNo) {
    const char *classString;
    int nameStart, classStart;

    nameStart = (15 / 2) - ((strlen(c->saveGame->players[charNo].name) + 2)/ 2);
    screenTextAt(TEXT_AREA_X + nameStart, 0, "%c%s%c", 16, c->saveGame->players[charNo].name, 17);
    screenTextAt(TEXT_AREA_X, 1, "%c             %c", c->saveGame->players[charNo].sex, c->saveGame->players[charNo].status);
    classString = statsClassName(c->saveGame->players[charNo].klass);
    classStart = (15 / 2) - (strlen(classString) / 2);
    screenTextAt(TEXT_AREA_X + classStart, 1, "%s", classString);
    screenTextAt(TEXT_AREA_X, 3, " MP:%02d  LV:%d", c->saveGame->players[charNo].mp, c->saveGame->players[charNo].hpMax / 100);
    screenTextAt(TEXT_AREA_X, 4, "STR:%02d  HP:%04d", c->saveGame->players[charNo].str, c->saveGame->players[charNo].hp);
    screenTextAt(TEXT_AREA_X, 5, "DEX:%02d  HM:%04d", c->saveGame->players[charNo].dex, c->saveGame->players[charNo].hpMax);
    screenTextAt(TEXT_AREA_X, 6, "INT:%02d  EX:%04d", c->saveGame->players[charNo].intel, c->saveGame->players[charNo].xp);
    screenTextAt(TEXT_AREA_X, 7, "W:%s", statsWeaponName(c->saveGame->players[charNo].weapon));
    screenTextAt(TEXT_AREA_X, 8, "A:%s", statsArmorName(c->saveGame->players[charNo].armor));
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
    switch (weapon) {
    case WEAP_HANDS:
        return "Hands";
    case WEAP_STAFF:
        return "Staff";
    case WEAP_DAGGER:
        return "Dagger";
    case WEAP_SLING:
        return "Sling";
    case WEAP_MACE:
        return "Mace";
    case WEAP_AXE:
        return "Axe";
    case WEAP_SWORD:
        return "Sword";
    case WEAP_BOW:
        return "Bow";
    case WEAP_CROSSBOW:
        return "Crossbow";
    case WEAP_OIL:
        return "Oil";
    case WEAP_HALBERD:
        return "Halberd";
    case WEAP_MAGICAXE:
        return "Magic Axe";
    case WEAP_MAGICSWORD:
        return "Magic Sword";
    case WEAP_MAGICBOW:
        return "Magic Bow";
    case WEAP_MAGICWAND:
        return "Magic Wand";
    case WEAP_MYSTICSWORD:
        return "Mystic Sword";
    default:
        return "???";
    }
}

const char *statsArmorName(ArmorType armor) {
    switch (armor) {
    case ARMR_NONE:
        return "Skin";
    case ARMR_CLOTH:
        return "Cloth";
    case ARMR_LEATHER:
        return "Leather";
    case ARMR_CHAIN:
        return "Chain Mail";
    case ARMR_PLATE:
        return "Plate";
    case ARMR_MAGICCHAIN:
        return "Magicchain";
    case ARMR_MAGICPLATE:
        return "Magicplate";
    case ARMR_MYSTICROBES:
        return "Mysticrobes";
    default:
        return "???";
    }
}
