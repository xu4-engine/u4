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
#include "names.h"

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
    
    screenEraseTextArea(STATS_AREA_X, 1, 15, 8);
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
    classString = getClassName(c->saveGame->players[charNo].klass);
    classStart = (STATS_AREA_WIDTH / 2) - (strlen(classString) / 2);
    screenTextAt(STATS_AREA_X + classStart, 1, "%s", classString);
    screenTextAt(STATS_AREA_X, 3, " MP:%02d  LV:%d", c->saveGame->players[charNo].mp, c->saveGame->players[charNo].hpMax / 100);
    screenTextAt(STATS_AREA_X, 4, "STR:%02d  HP:%04d", c->saveGame->players[charNo].str, c->saveGame->players[charNo].hp);
    screenTextAt(STATS_AREA_X, 5, "DEX:%02d  HM:%04d", c->saveGame->players[charNo].dex, c->saveGame->players[charNo].hpMax);
    screenTextAt(STATS_AREA_X, 6, "INT:%02d  EX:%04d", c->saveGame->players[charNo].intel, c->saveGame->players[charNo].xp);
    screenTextAt(STATS_AREA_X, 7, "W:%s", getWeaponName(c->saveGame->players[charNo].weapon));
    screenTextAt(STATS_AREA_X, 8, "A:%s", getArmorName(c->saveGame->players[charNo].armor));
}

/**
 * Weapons in inventory.
 */
void statsShowWeapons() {
    int w, line, col;

    statsAreaSetTitle("Weapons");

    line = 1;
    col = 0;
    screenTextAt(STATS_AREA_X, line++, "A-%s", getWeaponName(WEAP_HANDS));
    for (w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            screenTextAt(STATS_AREA_X + col, line++, "%c-%d-%s", w - WEAP_HANDS + 'A', n, getWeaponAbbrev(w));
            if (line >= 9) {
                line = 1;
                col += 8;
            }
        }
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
            screenTextAt(STATS_AREA_X, line++, "%c-%d-%s", a - ARMR_NONE + 'A', c->saveGame->armor[a], getArmorName(a));
    }
}

/**
 * Equipment: touches, gems, keys, and sextants.
 */
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

/**
 * Items: runes, stones, and other miscellaneous quest items.
 */
void statsShowItems() {
    statsAreaSetTitle("Items");
    /* FIXME */
}

/**
 * Unmixed reagents in inventory.
 */
void statsShowReagents() {
    int r, line;

    statsAreaSetTitle("Reagents");

    line = 1;
    for (r = REAG_ASH; r < REAG_MAX; r++) {
        int n = c->saveGame->reagents[r];
        if (n >= 100)
            n = 99;
        if (n >= 10)
            screenTextAt(STATS_AREA_X, line++, "%c%d-%s", r - REAG_ASH + 'A', n, getReagentName(r));
        else if (n >= 1)
            screenTextAt(STATS_AREA_X, line++, "%c-%d-%s", r - REAG_ASH + 'A', n, getReagentName(r));
    }
}

/**
 * Mixed reagents in inventory.
 */
void statsShowMixtures() {
    int s, line, col;

    statsAreaSetTitle("Mixtures");
    
    line = 1;
    col = 0;
    for (s = 0; s < 26; s++) {
        int n = c->saveGame->mixtures[s];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            screenTextAt(STATS_AREA_X + col, line++, "%c-%02d", s + 'A', n);
            if (line >= 9) {
                if (col >= 10)
                    break;
                line = 1;
                col += 5;
            }
        }
    }
}

