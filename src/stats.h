/*
 * stats.h
 */

#ifndef STATS_H
#define STATS_H

#include "menu.h"
#include "textview.h"

class Ingredients;

#define STATS_AREA_WIDTH 15
#define STATS_AREA_HEIGHT 8
#define STATS_AREA_X TEXT_AREA_X
#define STATS_AREA_Y 1

enum StatsView {
    STATS_PARTY_OVERVIEW,
    STATS_CHAR1,
    STATS_CHAR2,
    STATS_CHAR3,
    STATS_CHAR4,
    STATS_CHAR5,
    STATS_CHAR6,
    STATS_CHAR7,
    STATS_CHAR8,
    STATS_WEAPONS,
    STATS_ARMOR,
    STATS_EQUIPMENT,
    STATS_ITEMS,
    STATS_REAGENTS,
    STATS_MIXTURES,
    MIX_REAGENTS
};

class StatsArea {
public:
    StatsArea();
    ~StatsArea();

    void setView(StatsView view);

    void prevItem();
    void nextItem();
    void update();
    void redraw();
    void highlightPlayer(int player);
    void flashPlayers(int playerMask);
    void showAvatarOnly(bool enable) { avatarOnly = enable; }

    TextView *getMainArea() { return &mainArea; }

    void resetReagentsMenu();
    Menu *getReagentsMenu() { return &reagentsMixMenu; }

private:
    static void statsNotice(int, void*, void*);
    void showPartyView();
    void showPlayerDetails();
    void showWeapons();
    void showArmor();
    void showEquipment();
    void showItems();
    void showReagents(bool active = false);
    void showMixtures();
    void setTitle(const char* s);
    void redrawAura();

    TextView title;
    TextView mainArea;
    TextView summary;

    StatsView view;

    Menu reagentsMixMenu;
    int listenerId;

    int16_t  focusPlayer;
    uint16_t redrawMode;
    uint16_t flashMask;
    uint16_t flashCycle;
    bool     avatarOnly;
};

/**
 * Controller for the reagents menu used when mixing spells.  Fills
 * the passed in Ingredients with the selected reagents.
 */
class ReagentsMenuController : public MenuController {
public:
    ReagentsMenuController(Menu *menu, Ingredients *i, TextView *view) : MenuController(menu, view), ingredients(i) { }

    bool keyPressed(int key);

private:
    Ingredients *ingredients;
};

#endif
