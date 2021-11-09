/*
 * $Id$
 */

#ifndef STATS_H
#define STATS_H

#include <string>
#include "observable.h"
#include "menu.h"
#include "textview.h"

class Aura;
class Ingredients;
class Party;
class PartyEvent;

using std::string;

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

class StatsArea : public Observer<Aura *>, public Observer<Party *, PartyEvent &>, public Observer<Menu *, MenuEvent &>, public Observable<StatsArea *, string> {
public:
    StatsArea();

    void setView(StatsView view);

    void clear();
    void prevItem();
    void nextItem();
    void update(bool avatarOnly = false);
    virtual void update(Aura *aura);
    virtual void update(Party *party, PartyEvent &event)    {update(); /* do a full update */}
    virtual void update(Menu *menu, MenuEvent &event)       {update(); /* do a full update */}
    void highlightPlayer(int player);
    void redraw();

    TextView *getMainArea() { return &mainArea; }

    void resetReagentsMenu();
    Menu *getReagentsMenu() { return &reagentsMixMenu; }

private:
    void showPartyView(bool avatarOnly);
    void showPlayerDetails();
    void showWeapons();
    void showArmor();
    void showEquipment();
    void showItems();
    void showReagents(bool active = false);
    void showMixtures();
    void setTitle(const string &s);

    TextView title;
    TextView mainArea;
    TextView summary;

    StatsView view;

    Menu reagentsMixMenu;
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
