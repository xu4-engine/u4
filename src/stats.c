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
#include "ttype.h"
#include "player.h"

#define STATS_AREA_WIDTH 15
#define STATS_AREA_HEIGHT 8
#define STATS_AREA_X TEXT_AREA_X
#define STATS_AREA_Y 1

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
    int i;
    unsigned char mask;

    statsAreaClear();

    /* 
     * update the upper stats box 
     */
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

    /* 
     * update the lower stats box (food, gold, etc.)
     */
    if (tileIsShip(c->saveGame->transport))
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+STATS_AREA_HEIGHT+1, "F:%04d   SHP:%02d", c->saveGame->food / 100, c->saveGame->shiphull);
    else
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+STATS_AREA_HEIGHT+1, "F:%04d   G:%04d", c->saveGame->food / 100, c->saveGame->gold);

    mask = 0xff;
    for (i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] == 0)
            mask &= ~(1 << i);
    }
    screenShowCharMasked(0, STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1, mask);
}

void statsAreaClear() {
    int i;

    for (i = 0; i < STATS_AREA_WIDTH; i++)
        screenTextAt(STATS_AREA_X + i, 0, "%c", 13);
    
    screenEraseTextArea(STATS_AREA_X, STATS_AREA_Y, STATS_AREA_WIDTH, STATS_AREA_HEIGHT);
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
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+i, "%d-%-9s%3d%c", i+1, c->saveGame->players[i].name, c->saveGame->players[i].hp, c->saveGame->players[i].status);
}

/**
 * The individual character view.
 */
void statsShowCharDetails(int charNo) {
    const char *classString;
    int classStart;

    assert(charNo < 8);

    statsAreaSetTitle(c->saveGame->players[charNo].name);
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+0, "%c             %c", c->saveGame->players[charNo].sex, c->saveGame->players[charNo].status);
    classString = getClassName(c->saveGame->players[charNo].klass);
    classStart = (STATS_AREA_WIDTH / 2) - (strlen(classString) / 2);
    screenTextAt(STATS_AREA_X + classStart, STATS_AREA_Y, "%s", classString);
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+2, " MP:%02d  LV:%d", c->saveGame->players[charNo].mp, playerGetRealLevel(&c->saveGame->players[charNo]));
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+3, "STR:%02d  HP:%04d", c->saveGame->players[charNo].str, c->saveGame->players[charNo].hp);
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+4, "DEX:%02d  HM:%04d", c->saveGame->players[charNo].dex, c->saveGame->players[charNo].hpMax);
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+5, "INT:%02d  EX:%04d", c->saveGame->players[charNo].intel, c->saveGame->players[charNo].xp);
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+6, "W:%s", getWeaponName(c->saveGame->players[charNo].weapon));
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+7, "A:%s", getArmorName(c->saveGame->players[charNo].armor));
}

/**
 * Weapons in inventory.
 */
void statsShowWeapons() {
    int w, line, col;

    statsAreaSetTitle("Weapons");

    line = STATS_AREA_Y;
    col = 0;
    screenTextAt(STATS_AREA_X, line++, "A-%s", getWeaponName(WEAP_HANDS));
    for (w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            screenTextAt(STATS_AREA_X + col, line++, "%c-%d-%s", w - WEAP_HANDS + 'A', n, getWeaponAbbrev((WeaponType) w));
            if (line >= (STATS_AREA_Y+STATS_AREA_HEIGHT)) {
                line = STATS_AREA_Y;
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

    line = STATS_AREA_Y;
    screenTextAt(STATS_AREA_X, line++, "A  -No Armour");
    for (a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0)
            screenTextAt(STATS_AREA_X, line++, "%c-%d-%s", a - ARMR_NONE + 'A', c->saveGame->armor[a], getArmorName((ArmorType) a));
    }
}

/**
 * Equipment: touches, gems, keys, and sextants.
 */
void statsShowEquipment() {
    int line;

    statsAreaSetTitle("Equipment");
    
    line = STATS_AREA_Y;
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
    int i, j;
    int line;
    char buffer[17];

    statsAreaSetTitle("Items");

    line = STATS_AREA_Y;
    if (c->saveGame->stones != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->stones & (1 << i))
                buffer[j++] = getStoneName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        screenTextAt(STATS_AREA_X, line++, "Stones:%s", buffer);
    }
    if (c->saveGame->runes != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->runes & (1 << i))
                buffer[j++] = getVirtueName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        screenTextAt(STATS_AREA_X, line++, "Runes:%s", buffer);
    }
    if (c->saveGame->items & (ITEM_CANDLE | ITEM_BOOK | ITEM_BELL)) {
        buffer[0] = '\0';
        if (c->saveGame->items & ITEM_BELL) {
            strcat(buffer, getItemName(ITEM_BELL));
            strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_BOOK) {
            strcat(buffer, getItemName(ITEM_BOOK));
            strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_CANDLE) {
            strcat(buffer, getItemName(ITEM_CANDLE));
            buffer[15] = '\0';
        }
        screenTextAt(STATS_AREA_X, line++, "%s", buffer);
    }
    if (c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        j = 0;
        if (c->saveGame->items & ITEM_KEY_T)
            buffer[j++] = getItemName(ITEM_KEY_T)[0];
        if (c->saveGame->items & ITEM_KEY_L)
            buffer[j++] = getItemName(ITEM_KEY_L)[0];
        if (c->saveGame->items & ITEM_KEY_C)
            buffer[j++] = getItemName(ITEM_KEY_C)[0];
        buffer[j] = '\0';
        screenTextAt(STATS_AREA_X, line++, "3 Part Key:%s", buffer);
    }
    if (c->saveGame->items & ITEM_HORN)
        screenTextAt(STATS_AREA_X, line++, "%s", getItemName(ITEM_HORN));
    if (c->saveGame->items & ITEM_WHEEL)
        screenTextAt(STATS_AREA_X, line++, "%s", getItemName(ITEM_WHEEL));
    if (c->saveGame->items & ITEM_SKULL)
        screenTextAt(STATS_AREA_X, line++, "%s", getItemName(ITEM_SKULL));
}

/**
 * Unmixed reagents in inventory.
 */
void statsShowReagents() {
    int r, line;

    statsAreaSetTitle("Reagents");

    line = STATS_AREA_Y;
    for (r = REAG_ASH; r < REAG_MAX; r++) {
        int n = c->saveGame->reagents[r];
        if (n >= 100)
            n = 99;
        if (n >= 10)
            screenTextAt(STATS_AREA_X, line++, "%c%d-%s", r - REAG_ASH + 'A', n, getReagentName((Reagent) r));
        else if (n >= 1)
            screenTextAt(STATS_AREA_X, line++, "%c-%d-%s", r - REAG_ASH + 'A', n, getReagentName((Reagent) r));
    }
}

/**
 * Mixed reagents in inventory.
 */
void statsShowMixtures() {
    int s, line, col;

    statsAreaSetTitle("Mixtures");
    
    line = STATS_AREA_Y;
    col = 0;
    for (s = 0; s < SPELL_MAX; s++) {
        int n = c->saveGame->mixtures[s];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            screenTextAt(STATS_AREA_X + col, line++, "%c-%02d", s + 'A', n);
            if (line >= (STATS_AREA_Y+STATS_AREA_HEIGHT)) {
                if (col >= 10)
                    break;
                line = STATS_AREA_Y;
                col += 5;
            }
        }
    }
}

