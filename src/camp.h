/*
 * $Id$
 */

#ifndef CAMP_H
#define CAMP_H

#include "combat.h"

/* Number of moves before camping will heal the party */
#define CAMP_HEAL_INTERVAL  100

class CampController : public CombatController {
public:
    CampController();
    virtual void beginCombat();
    virtual void endCombat(bool adjustKarma);
};

class InnController : public CombatController {
public:
    InnController();
    virtual void beginCombat();
    virtual void awardLoot();

private:
    void maybeMeetIsaac();
    void maybeAmbush();
};

#endif
