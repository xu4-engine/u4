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
    typedef std::string string;

    static const Weapon *get(WeaponType w);
    static const Weapon *get(const string &name);

    WeaponType getType() const;
    const string &getName() const;
    const string &getAbbrev() const;
    int getRange() const;
    int getDamage() const;
    const string &getHitTile() const;
    const string &getMissTile() const;
    bool alwaysHits() const;
    const string &leavesTile() const;
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
    string name;
    string abbr;
    unsigned char canuse;
    int range;
    int damage;
    string hittile;
    string misstile;
    string leavetile;
    unsigned short mask;
};

#endif
