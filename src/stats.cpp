/*
 * stats.cpp
 */

#include <cassert>
#include <cstring>

#include "stats.h"

#include "config.h"
#include "party.h"
#include "spell.h"
#include "weapon.h"
#include "u4.h"
#include "xu4.h"

enum RedrawMode {
    REDRAW_NONE = 0,
    REDRAW_ALL  = 1,
    REDRAW_AURA = 2
};

/**
 * StatsArea class implementation
 */
StatsArea::StatsArea() :
    title(STATS_AREA_X * CHAR_WIDTH, 0 * CHAR_HEIGHT, STATS_AREA_WIDTH, 1),
    mainArea(STATS_AREA_X * CHAR_WIDTH, STATS_AREA_Y * CHAR_HEIGHT, STATS_AREA_WIDTH, STATS_AREA_HEIGHT),
    summary(STATS_AREA_X * CHAR_WIDTH, (STATS_AREA_Y + STATS_AREA_HEIGHT + 1) * CHAR_HEIGHT, STATS_AREA_WIDTH, 1),
    view(STATS_PARTY_OVERVIEW)
{
    // Generate a formatted string for each menu item,
    // and then add the item to the menu.  The Y value
    // for each menu item will be filled in later.
    for (int count=0; count < 8; count++)
    {
        char outputBuffer[16];
        snprintf(outputBuffer, sizeof(outputBuffer), "-%-11s%%s", getReagentName((Reagent)count));
        reagentsMixMenu.add(count, new IntMenuItem(outputBuffer, 1, 0, -1, (int *)c->party->getReagentPtr((Reagent)count), 0, 99, 1, MENU_OUTPUT_REAGENT));
    }

    listenerId = gs_listen(1<<SENDER_PARTY | 1<<SENDER_AURA | 1<<SENDER_MENU,
                           statsNotice, this);
    focusPlayer = -1;
    redrawMode = REDRAW_NONE;
    flashMask  = 0;
    flashCycle = 0;
    avatarOnly = false;
}

StatsArea::~StatsArea() {
    gs_unplug(listenerId);
}

void StatsArea::setView(StatsView view) {
    this->view = view;
    update();
}

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
 * Queue update for the stats (ztats) box on the upper right of the screen.
 */
void StatsArea::update() {
    redrawMode |= REDRAW_ALL;
}

/**
 * Called once per screen update to redraw the stats area if needed.
 */
void StatsArea::redraw() {
    // Force a redraw when player flash times out.
    if (flashCycle) {
        --flashCycle;
        if (! flashCycle) {
            flashMask = 0;
            if (view == STATS_PARTY_OVERVIEW)
                redrawMode |= REDRAW_ALL;
        }
    }

    if (redrawMode == REDRAW_NONE)
        return;

    if (redrawMode == REDRAW_AURA) {
        redrawMode = REDRAW_NONE;
        redrawAura();
        return;
    }

    // Clear Areas.
    for (int i = 0; i < STATS_AREA_WIDTH; i++)
        title.drawChar(CHARSET_HORIZBAR, i, 0);
    mainArea.clear();
    summary.clear();

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
        showPlayerDetails();
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
    case MIX_REAGENTS:
        showReagents(true);
        break;
    }

    /*
     * update the lower stats box (food, gold, etc.)
     */
    if (c->transportContext == TRANSPORT_SHIP)
        summary.textAtFmt(0, 0, "F:%04d   SHP:%02d",
                          c->saveGame->food / 100, c->saveGame->shiphull);
    else
        summary.textAtFmt(0, 0, "F:%04d   G:%04d",
                          c->saveGame->food / 100, c->saveGame->gold);

    redrawAura();

    title.update();
    mainArea.update();
    summary.update();

    if (flashMask && view == STATS_PARTY_OVERVIEW) {
        int count = c->party->size();
        for (int i = 0; i < count; i++) {
            if (flashMask & (1 << i)) {
                xu4.screenImage->drawHighlight(
                        mainArea.x, mainArea.y + i * CHAR_HEIGHT,
                        STATS_AREA_WIDTH * CHAR_WIDTH, CHAR_HEIGHT);
            }
        }
    }

    redrawMode = REDRAW_NONE;
}

void StatsArea::redrawAura() {
    static const char auraChar[6] = {
        // NONE, HORN, JINX, NEGATE, PROTECTION, QUICKNESS
        CHARSET_ANKH, CHARSET_REDDOT, 'J', 'N', 'P', 'Q'
    };

    int type = c->aura.getType();
    if (type > Aura::NONE && type <= Aura::QUICKNESS) {
        summary.drawChar(auraChar[type], STATS_AREA_WIDTH/2, 0);
    } else {
        uint8_t mask = 0xff;
        for (int i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->karma[i] == 0)
                mask &= ~(1 << i);
        }
        summary.drawCharMasked(CHARSET_ANKH, STATS_AREA_WIDTH/2, 0, mask);
    }
}

void StatsArea::statsNotice(int sender, void* eventData, void* user) {
    StatsArea* sa = (StatsArea*) user;

    if (sender == SENDER_PARTY) {
        sa->update();   /* do a full update */
    }
    else if (sender == SENDER_AURA) {
        sa->redrawMode |= REDRAW_AURA;
    }
    else if (sender == SENDER_MENU) {
        const Menu* menu = ((MenuEvent*) eventData)->menu;
        if (menu == &sa->reagentsMixMenu)
            sa->update();   /* do a full update */
    }
}

/*
 * Highlight the active player.  Pass -1 to disable the highlight.
 */
void StatsArea::highlightPlayer(int player) {
    assert(player < c->party->size());
    if (focusPlayer != player) {
        focusPlayer = player;
        redrawMode |= REDRAW_ALL;
    }
}

void StatsArea::flashPlayers(int playerMask) {
    if (view == STATS_PARTY_OVERVIEW) {
        // Using OR operator to handle multiple flashPlayers calls per frame.
        flashMask |= playerMask;
        flashCycle = 3;
        redrawMode |= REDRAW_ALL;
    }
}

/**
 * Sets the title of the stats area.
 */
void StatsArea::setTitle(const char* s) {
    int titleStart = (STATS_AREA_WIDTH / 2) - ((strlen(s) + 2) / 2);
    title.textAtFmt(titleStart, 0, "%c%s%c", 16, s, 17);
}

/**
 * The basic party view.
 */
void StatsArea::showPartyView() {
    const char *format = "%d%c%-9.8s%3d%s";
    PartyMember *p;
    int activePlayer = c->party->getActivePlayer();
    int count = avatarOnly ? 1 : c->party->size();

    //printf("KR showPartyView %d\n", focusPlayer);
    assert(count <= 8);

    for (int i = 0; i < count; i++) {
        p = c->party->member(i);
        mainArea.textAtFmt(0, i, format, i+1,
                           (i==activePlayer) ? CHARSET_BULLET : '-',
                           p->getName(), p->getHp(),
                           mainArea.colorizeStatus(p->getStatus()).c_str());
    }

    if (focusPlayer >= 0) {
        mainArea.setHighlight(0, focusPlayer * CHAR_HEIGHT,
                              STATS_AREA_WIDTH * CHAR_WIDTH, CHAR_HEIGHT);
    }
}

/**
 * The individual character view.
 */
void StatsArea::showPlayerDetails() {
    int player = view - STATS_CHAR1;

    assert(player < 8);

    PartyMember *p = c->party->member(player);
    setTitle(p->getName());
    mainArea.textAtFmt(0, 0, "%c             %c", p->getSex(), p->getStatus());
    const char* classStr = getClassName(p->getClass());
    int classStart = (STATS_AREA_WIDTH / 2) - (strlen(classStr) / 2);
    mainArea.textAt(classStart, 0, classStr);
    mainArea.textAtFmt(0, 2, " MP:%02d  LV:%d", p->getMp(), p->getRealLevel());
    mainArea.textAtFmt(0, 3, "STR:%02d  HP:%04d", p->getStr(), p->getHp());
    mainArea.textAtFmt(0, 4, "DEX:%02d  HM:%04d", p->getDex(), p->getMaxHp());
    mainArea.textAtFmt(0, 5, "INT:%02d  EX:%04d", p->getInt(), p->getExp());
    mainArea.textAtFmt(0, 6, "W:%s", p->getWeapon()->getName());
    mainArea.textAtFmt(0, 7, "A:%s", p->getArmor()->getName());
}

/**
 * Weapons in inventory.
 */
void StatsArea::showWeapons() {
    setTitle("Weapons");

    int line = 0;
    int col = 0;
    mainArea.textAtFmt(0, line++, "A-%s", xu4.config->weapon(WEAP_HANDS)->getName());
    for (int w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            const char *format = (n >= 10) ? "%c%d-%s" : "%c-%d-%s";

            mainArea.textAtFmt(col, line++, format, w - WEAP_HANDS + 'A', n, xu4.config->weapon((WeaponType) w)->getAbbrev());
            if (line >= (STATS_AREA_HEIGHT)) {
                line = 0;
                col += 8;
            }
        }
    }
}

/**
 * Armor in inventory.
 */
void StatsArea::showArmor() {
    setTitle("Armour");

    int line = 0;
    mainArea.textAt(0, line++, "A  -No Armour");
    for (int a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0) {
            const char *format = (c->saveGame->armor[a] >= 10) ? "%c%d-%s"
                                                               : "%c-%d-%s";

            mainArea.textAtFmt(0, line++, format, a - ARMR_NONE + 'A',
                    c->saveGame->armor[a],
                    xu4.config->armor((ArmorType) a)->getName());
        }
    }
}

/**
 * Equipment: touches, gems, keys, and sextants.
 */
void StatsArea::showEquipment() {
    setTitle("Equipment");

    int line = 0;
    mainArea.textAtFmt(0, line++, "%2d Torches", c->saveGame->torches);
    mainArea.textAtFmt(0, line++, "%2d Gems", c->saveGame->gems);
    mainArea.textAtFmt(0, line++, "%2d Keys", c->saveGame->keys);
    if (c->saveGame->sextants > 0)
        mainArea.textAtFmt(0, line++, "%2d Sextants", c->saveGame->sextants);
}

/**
 * Items: runes, stones, and other miscellaneous quest items.
 */
void StatsArea::showItems() {
    int i, j;
    char buffer[17];

    setTitle("Items");

    int line = 0;
    if (c->saveGame->stones != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->stones & (1 << i))
                buffer[j++] = getStoneName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        mainArea.textAtFmt(0, line++, "Stones:%s", buffer);
    }
    if (c->saveGame->runes != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->runes & (1 << i))
                buffer[j++] = getVirtueName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        mainArea.textAtFmt(0, line++, "Runes:%s", buffer);
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
        mainArea.textAt(0, line++, buffer);
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
        mainArea.textAtFmt(0, line++, "3 Part Key:%s", buffer);
    }
    if (c->saveGame->items & ITEM_HORN)
        mainArea.textAt(0, line++, getItemName(ITEM_HORN));
    if (c->saveGame->items & ITEM_WHEEL)
        mainArea.textAt(0, line++, getItemName(ITEM_WHEEL));
    if (c->saveGame->items & ITEM_SKULL)
        mainArea.textAt(0, line++, getItemName(ITEM_SKULL));
}

/**
 * Unmixed reagents in inventory.
 */
void StatsArea::showReagents(bool active)
{
    setTitle("Reagents");

    Menu::MenuItemList::iterator i;
    int line = 0,
        r = REAG_ASH;
    char shortcut[2];

    shortcut[1] = '\0';

    reagentsMixMenu.show(&mainArea);

    for (i = reagentsMixMenu.begin(); i != reagentsMixMenu.end(); i++, r++)
    {
        if ((*i)->isVisible())
        {
            // Insert the reagent menu item shortcut character
            shortcut[0] = 'A'+r;
            if (active)
                mainArea.textAtKey(0, line++, shortcut, 0);
            else
                mainArea.textAt(0, line++, shortcut);
        }
    }
}

/**
 * Mixed reagents in inventory.
 */
void StatsArea::showMixtures() {
    setTitle("Mixtures");

    int line = 0;
    int col = 0;
    for (int s = 0; s < SPELL_MAX; s++) {
        int n = c->saveGame->mixtures[s];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            mainArea.textAtFmt(col, line++, "%c-%02d", s + 'A', n);
            if (line >= (STATS_AREA_HEIGHT)) {
                if (col >= 10)
                    break;
                line = 0;
                col += 5;
            }
        }
    }
}

void StatsArea::resetReagentsMenu() {
    Menu::MenuItemList::iterator current;
    int i = 0,
        row = 0;

    for (current = reagentsMixMenu.begin(); current != reagentsMixMenu.end(); current++)
    {
        if (c->saveGame->reagents[i++] > 0)
        {
            (*current)->setVisible(true);
            (*current)->setY(row++);
        }
        else (*current)->setVisible(false);
    }

    reagentsMixMenu.reset(false);
}

/**
 * Handles spell mixing for the Ultima V-style menu-system
 */
bool ReagentsMenuController::keyPressed(int key) {
    switch(key) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
        {
            /* select the corresponding reagent (if visible) */
            Menu::MenuItemList::iterator mi = menu->getById(key-'a');
            if ((*mi)->isVisible()) {
                menu->setCurrent(menu->getById(key-'a'));
                keyPressed(U4_SPACE);
            }
        } break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_SPACE:
        if (menu->isVisible()) {
            MenuItem *item = *menu->getCurrent();

            /* change whether or not it's selected */
            item->setSelected(!item->isSelected());

            if (item->isSelected())
                ingredients->addReagent((Reagent)item->getId());
            else
                ingredients->removeReagent((Reagent)item->getId());
        }
        break;
    case U4_ENTER:
        xu4.eventHandler->setControllerDone();
        break;

    case U4_ESC:
        ingredients->revert();
        xu4.eventHandler->setControllerDone();
        break;

    default:
        return MenuController::keyPressed(key);
    }

    return true;
}

