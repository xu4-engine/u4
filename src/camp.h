/*
 * $Id$
 */

#ifndef CAMP_H
#define CAMP_H

#include "combat.h"

#define CAMP_HEAL_INTERVAL  100   /* Number of moves before camping will heal the party */

class CampController : public CombatController {
public:
    CampController();
    virtual void init(Creature *m);
    virtual void begin();
    virtual void end(bool adjustKarma);

private:
    bool heal();
};

class InnController : public CombatController {
public:
    InnController();

    virtual void begin();
    virtual void awardLoot();

private:
    bool heal();
};

#endif
