/*
 * $Id$
 */

#ifndef STATS_H
#define STATS_H

#include <string>
#include "observable.h"
#include "observer.h"
#include "textview.h"

struct SaveGame;
class Aura;
class Menu;
class MenuEvent;
class Party;

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
    STATS_MIXTURES
};

class StatsArea : public Observer<Aura *>, public Observer<Party *>, public Observer<Menu *, MenuEvent &>, public Observable<StatsArea *, string> {
public:
    StatsArea();

    void setView(StatsView view);

    void clear();
    void prevItem();
    void nextItem();
    void update(bool avatarOnly = false);
    virtual void update(Aura *aura);
    virtual void update(Party *party);
    virtual void update(Menu *menu, MenuEvent &event);
    void highlightPlayer(int player);
    void redraw();

    TextView *getMainArea() { return &mainArea; }

private:
    void showPartyView(bool avatarOnly);
    void showPlayerDetails();
    void showWeapons();
    void showArmor();
    void showEquipment();
    void showItems();
    void showReagents();
    void showMixtures();
    void setTitle(const string &s);

    TextView title;
    TextView mainArea;
    TextView summary;
    
    StatsView view;
};

#endif
