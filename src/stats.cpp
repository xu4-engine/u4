/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "u4.h"

#include "stats.h"

#include "armor.h"
#include "context.h"
#include "debug.h"
#include "menu.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "tile.h"
#include "weapon.h"

extern bool verbose;

/**
 * StatsArea class implementation
 */
StatsArea::StatsArea() : view(STATS_PARTY_OVERVIEW) {}
 
/**
 * Sets the stats item to the previous in sequence.
 */
void StatsArea::prevItem() {
    view = (StatsView)(view - 1);
    if (view < STATS_CHAR1)
        view = STATS_MIXTURES;
    if (view <= STATS_CHAR8 && (view - STATS_CHAR1 + 1) > c->party->size())
        view = (StatsView) (STATS_CHAR1 - 1 + c->party->size());
    update();
}

/**
 * Sets the stats item to the next in sequence.
 */
void StatsArea::nextItem() {
    view = (StatsView)(view + 1);    
    if (view > STATS_MIXTURES)
        view = STATS_CHAR1;
    if (view <= STATS_CHAR8 && (view - STATS_CHAR1 + 1) > c->party->size())
        view = STATS_WEAPONS;
    update();
}

/**
 * Update the stats (ztats) box on the upper right of the screen.
 */
void StatsArea::update(Observable<string> *o, string arg) {
    int i;
    unsigned char mask;

    if (!arg.empty() && verbose)
        fprintf(stdout, "Observer updated: Stats area redrawn from function %s()\n", arg.c_str());

    clear();

    /*
     * update the upper stats box
     */
    switch(view) {
    case STATS_PARTY_OVERVIEW:
        showPartyView();
        break;
    case STATS_CHAR1:
    case STATS_CHAR2:
    case STATS_CHAR3:
    case STATS_CHAR4:
    case STATS_CHAR5:
    case STATS_CHAR6:
    case STATS_CHAR7:
    case STATS_CHAR8:
        showPlayerDetails(view - STATS_CHAR1);
        break;
    case STATS_WEAPONS:
        showWeapons();
        break;
    case STATS_ARMOR:
        showArmor();
        break;
    case STATS_EQUIPMENT:
        showEquipment();
        break;
    case STATS_ITEMS:
        showItems();
        break;
    case STATS_REAGENTS:
        showReagents();
        break;
    case STATS_MIXTURES:
        showMixtures();
        break;
    }

    /*
     * update the lower stats box (food, gold, etc.)
     */
    if (c->transportContext == TRANSPORT_SHIP)
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+STATS_AREA_HEIGHT+1, "F:%04d   SHP:%02d", c->saveGame->food / 100, c->saveGame->shiphull);
    else
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+STATS_AREA_HEIGHT+1, "F:%04d   G:%04d", c->saveGame->food / 100, c->saveGame->gold);

    mask = 0xff;
    for (i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] == 0)
            mask &= ~(1 << i);
    }

    switch (c->aura->getType()) {
    case AURA_NONE:
        screenShowCharMasked(0, STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1, mask);
        break;
    case AURA_HORN:
        screenShowChar(1, STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1);
        break;
    case AURA_JINX:
        screenShowChar('J', STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1);
        break;
    case AURA_NEGATE:
        screenShowChar('N', STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1);
        break;
    case AURA_PROTECTION:
        screenShowChar('P', STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1);
        break;
    case AURA_QUICKNESS:
        screenShowChar('Q', STATS_AREA_X + STATS_AREA_WIDTH/2, STATS_AREA_Y+STATS_AREA_HEIGHT+1);
        break;
    }    

    redraw();
}

void StatsArea::highlightPlayer(int player) {
    ASSERT(player < c->party->size(), "player number out of range: %d", player);
    screenInvertRect(STATS_AREA_X * CHAR_WIDTH, (STATS_AREA_Y + player) * CHAR_HEIGHT, 
                     STATS_AREA_WIDTH * CHAR_WIDTH, CHAR_HEIGHT);
}

void StatsArea::clear() {
    int i;

    for (i = 0; i < STATS_AREA_WIDTH; i++)
        screenTextAt(STATS_AREA_X + i, 0, "%c", 13);

    screenEraseTextArea(STATS_AREA_X, STATS_AREA_Y, STATS_AREA_WIDTH, STATS_AREA_HEIGHT);
}

/**
 * Redraws the entire stats area
 */
void StatsArea::redraw() {
    screenRedrawTextArea(STATS_AREA_X-1, STATS_AREA_Y-1, STATS_AREA_WIDTH+2, STATS_AREA_HEIGHT+3);
}

/**
 * Sets the title of the stats area.
 */
void StatsArea::setTitle(string title) {
    int titleStart;

    titleStart = (STATS_AREA_WIDTH / 2) - ((strlen(title.c_str()) + 2) / 2);
    screenTextAt(STATS_AREA_X + titleStart, 0, "%c%s%c", 16, title.c_str(), 17);
}

/**
 * The basic party view.
 */
void StatsArea::showPartyView(int member) {
    int i;
    char *format = "%d%c%-9.8s%3d%c";
    PartyMember *p = NULL;
    view = STATS_PARTY_OVERVIEW;
    int activePlayer = c->location ? c->location->activePlayer : -1;

    clear();

    ASSERT(c->party->size() <= 8, "party members out of range: %d", c->party->size());
    ASSERT(member <= 8, "party member out of range: %d", member);

    if (member == -1) {
        for (i = 0; i < c->party->size(); i++) {
            p = c->party->member(i);
            screenTextAt(STATS_AREA_X, STATS_AREA_Y+i, format, i+1, (i==activePlayer) ? CHARSET_BULLET : '-', p->getName().c_str(), p->getHp(), p->getStatus());
        }
    }
    else {        
        p = c->party->member(member);
        screenTextAt(STATS_AREA_X, STATS_AREA_Y+member, format, member+1, (member==activePlayer) ? CHARSET_BULLET : '-', p->getName().c_str(), p->getHp(), p->getStatus());
    }
}

/**
 * The individual character view.
 */
void StatsArea::showPlayerDetails(int player) {
    string classStr;
    int classStart;
    PartyMember *p;
    view = static_cast<StatsView>(STATS_CHAR1 + player);
    clear();

    ASSERT(player < 8, "character number out of range: %d", player);

    p = c->party->member(player);
    setTitle(p->getName());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+0, "%c             %c", p->getSex(), p->getStatus());
    classStr = getClassName(p->getClass());
    classStart = (STATS_AREA_WIDTH / 2) - (classStr.length() / 2);
    screenTextAt(STATS_AREA_X + classStart, STATS_AREA_Y, "%s", classStr.c_str());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+2, " MP:%02d  LV:%d", p->getMp(), p->getRealLevel());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+3, "STR:%02d  HP:%04d", p->getStr(), p->getHp());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+4, "DEX:%02d  HM:%04d", p->getDex(), p->getMaxHp());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+5, "INT:%02d  EX:%04d", p->getInt(), p->getExp());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+6, "W:%s", Weapon::get(p->getWeapon())->getName().c_str());
    screenTextAt(STATS_AREA_X, STATS_AREA_Y+7, "A:%s", Armor::get(p->getArmor())->getName().c_str());
}

/**
 * Weapons in inventory.
 */
void StatsArea::showWeapons() {
    int w, line, col;
    view = STATS_WEAPONS;

    clear();
    setTitle("Weapons");

    line = STATS_AREA_Y;
    col = 0;
    screenTextAt(STATS_AREA_X, line++, "A-%s", Weapon::get(WEAP_HANDS)->getName().c_str());
    for (w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            const char *format = (n >= 10) ? "%c%d-%s" : "%c-%d-%s";

            screenTextAt(STATS_AREA_X + col, line++, format, w - WEAP_HANDS + 'A', n, Weapon::get((WeaponType) w)->getAbbrev().c_str());
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
void StatsArea::showArmor() {
    int a, line;
    view = STATS_ARMOR;

    clear();
    setTitle("Armour");

    line = STATS_AREA_Y;
    screenTextAt(STATS_AREA_X, line++, "A  -No Armour");
    for (a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0) {
            const char *format = (c->saveGame->armor[a] >= 10) ? "%c%d-%s" : "%c-%d-%s";

            screenTextAt(STATS_AREA_X, line++, format, a - ARMR_NONE + 'A', c->saveGame->armor[a], Armor::get((ArmorType) a)->getName().c_str());
        }
    }
}

/**
 * Equipment: touches, gems, keys, and sextants.
 */
void StatsArea::showEquipment() {
    int line;
    view = STATS_EQUIPMENT;

    clear();
    setTitle("Equipment");

    line = STATS_AREA_Y;
    screenTextAt(STATS_AREA_X, line++, "%2d Torches", c->saveGame->torches);
    screenTextAt(STATS_AREA_X, line++, "%2d Gems", c->saveGame->gems);
    screenTextAt(STATS_AREA_X, line++, "%2d Keys", c->saveGame->keys);
    if (c->saveGame->sextants > 0)
        screenTextAt(STATS_AREA_X, line++, "%2d Sextants", c->saveGame->sextants);    
}

/**
 * Items: runes, stones, and other miscellaneous quest items.
 */
void StatsArea::showItems() {
    int i, j;
    int line;
    char buffer[17];
    view = STATS_ITEMS;

    setTitle("Items");

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
void StatsArea::showReagents() {
    int r, line;    
    extern Menu spellMixMenu;
    view = STATS_REAGENTS;
    
    clear();
    setTitle("Reagents");    

    line = STATS_AREA_Y;
    Menu::MenuItemList::iterator i;
    for (i = spellMixMenu.begin(), r = REAG_ASH; i != spellMixMenu.end(); i++, r++) {
        if (i->isVisible()) {
            /* show the quantity of reagents */
            screenTextAt(STATS_AREA_X, line, "%c-", r+'A');
            screenTextAt(STATS_AREA_X+13, line++, "%2d", c->party->reagents(r - REAG_ASH));
        }
    }
    
    spellMixMenu.show();    
}

/**
 * Mixed reagents in inventory.
 */
void StatsArea::showMixtures() {
    int s, line, col;
    view = STATS_MIXTURES;

    clear();
    setTitle("Mixtures");

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
