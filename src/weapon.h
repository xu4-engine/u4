/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include <vector>
#include "savegame.h"

class ConfigElement;

class Weapon {
public:
    static const Weapon *get(WeaponType w);
    static const Weapon *get(const std::string &name);

    WeaponType getType() const;
    const std::string &getName() const;
    const std::string &getAbbrev() const;
    int getRange() const;
    int getDamage() const;
    int getHitTile() const;
    int getMissTile() const;
    bool alwaysHits() const;
    unsigned char leavesTile() const;
    bool canReady(ClassType klass) const;
    bool canAttackThroughObjects() const;
    bool rangeAbsolute() const;
    bool returns() const;
    bool loseWhenUsed() const;
    bool loseWhenRanged() const;
    bool canChooseDistance() const;
    bool isMagic() const;
    bool showTravel() const;

private:
    Weapon(const ConfigElement &conf);

    static void loadConf();
    static bool confLoaded;
    static std::vector<Weapon *> weapons;

    WeaponType type;
    std::string name;
    std::string abbr;
    unsigned char canuse;
    int range;
    int damage;
    int hittile;
    int misstile;
    int leavetile;
    unsigned short mask;
};

#endif
