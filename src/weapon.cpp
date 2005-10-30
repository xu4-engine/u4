/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>

#include "weapon.h"

#include "config.h"
#include "error.h"
#include "names.h"

using std::string;
using std::vector;

#define MASK_LOSE                   0x0001
#define MASK_LOSEWHENRANGED         0x0002
#define MASK_CHOOSEDISTANCE         0x0004
#define MASK_ALWAYSHITS             0x0008
#define MASK_MAGIC                  0x0010
#define MASK_ATTACKTHROUGHOBJECTS   0x0040
#define MASK_ABSOLUTERANGE          0x0080
#define MASK_RETURNS                0x0100
#define MASK_DONTSHOWTRAVEL         0x0200

bool Weapon::confLoaded = false;
vector<Weapon *> Weapon::weapons;

/**
 * Returns weapon by WeaponType.
 */ 
const Weapon *Weapon::get(WeaponType w) {
    // Load in XML if it hasn't been already
    loadConf();

    if (static_cast<unsigned>(w) >= weapons.size())
        return NULL;
    return weapons[w];
}

/**
 * Returns weapon that has the given name
 */ 
const Weapon *Weapon::get(const string &name) {
    // Load in XML if it hasn't been already
    loadConf();

    for (unsigned i = 0; i < weapons.size(); i++) {
        if (strcasecmp(name.c_str(), weapons[i]->name.c_str()) == 0)
            return weapons[i];
    }
    return NULL;
}

Weapon::Weapon(const ConfigElement &conf) {
    static const struct {
        const char *name;
        unsigned int mask;        
    } booleanAttributes[] = {
        { "lose", MASK_LOSE },
        { "losewhenranged", MASK_LOSEWHENRANGED },
        { "choosedistance", MASK_CHOOSEDISTANCE },
        { "alwayshits", MASK_ALWAYSHITS },
        { "magic", MASK_MAGIC },
        { "attackthroughobjects", MASK_ATTACKTHROUGHOBJECTS },
        { "returns", MASK_RETURNS },
        { "dontshowtravel", MASK_DONTSHOWTRAVEL }
    };    

    type = static_cast<WeaponType>(weapons.size());
    name = conf.getString("name");
    abbr = conf.getString("abbr");
    canuse = 0xFF;
    damage = conf.getInt("damage");
    hittile = "hit_flash";
    misstile = "miss_flash";
    leavetile = "";
    mask = 0;

    /* Get the range of the weapon, whether it is absolute or normal range */
    string _range = conf.getString("range");
    if (_range.empty()) {
        _range = conf.getString("absolute_range");
        if (!_range.empty())
            mask |= MASK_ABSOLUTERANGE;
    }
    if (_range.empty())
        errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", name.c_str());

    range = atoi(_range.c_str());

    /* Load weapon attributes */
    for (unsigned at = 0; at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); at++) {
        if (conf.getBool(booleanAttributes[at].name)) {
            mask |= booleanAttributes[at].mask;
        }
    }

    /* Load hit tiles */
    if (conf.exists("hittile"))
        hittile = conf.getString("hittile");
    
    /* Load miss tiles */
    if (conf.exists("misstile"))
        misstile = conf.getString("misstile");
    
    /* Load leave tiles */
    if (conf.exists("leavetile")) {
        leavetile = conf.getString("leavetile");
    }
    
    vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = contraintConfs.begin(); i != contraintConfs.end(); i++) {
        unsigned char mask = 0;

        if (i->getName() != "constraint")
            continue;

        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(i->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(i->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed weapons.xml file: constraint has unknown class %s", 
                       i->getString("class").c_str());
        }
        if (i->getBool("canuse"))
            canuse |= mask;
        else
            canuse &= ~mask;
    }

    
}

void Weapon::loadConf() {
    if (!confLoaded)
        confLoaded = true;
    else
        return;

    const Config *config = Config::getInstance();

    vector<ConfigElement> weaponConfs = config->getElement("weapons").getChildren();
    for (std::vector<ConfigElement>::iterator i = weaponConfs.begin(); i != weaponConfs.end(); i++) {
        if (i->getName() != "weapon")
            continue;

        weapons.push_back(new Weapon(*i));
    }
}

/**
 * Returns the WeaponType of the weapon
 */
WeaponType Weapon::getType() const {
    return type;
}

/**
 * Returns the name of the weapon
 */
const string &Weapon::getName() const {
    return name;
}

/**
 * Returns the abbreviation for the weapon
 */
const string &Weapon::getAbbrev() const {
    return abbr;
}

/**
 * Return the range of the weapon
 */
int Weapon::getRange() const {
    return range;
}

/**
 * Return the damage for the specified weapon
 */
int Weapon::getDamage() const {
    return damage;
}

/**
 * Return the hit tile for the specified weapon
 */
const string &Weapon::getHitTile() const {
    return hittile;
}

/**
 * Return the miss tile for the specified weapon
 */
const string &Weapon::getMissTile() const {
    return misstile;
}

/**
 * Returns true if the weapon always hits it's target
 */
bool Weapon::alwaysHits() const {
    return mask & MASK_ALWAYSHITS;
}

/**
 * Returns 0 if the weapon leaves no tile, otherwise it
 * returns the # of the tile the weapon leaves
 */
const string &Weapon::leavesTile() const {
    return leavetile;
}

/**
 * Returns true if the class given can ready the weapon
 */
bool Weapon::canReady(ClassType klass) const {
    return (canuse & (1 << klass)) != 0;
}

/**
 * Returns true if the weapon can attack through solid objects
 */
bool Weapon::canAttackThroughObjects() const {
    return mask & MASK_ATTACKTHROUGHOBJECTS;
}

/**
 * Returns true if the weapon's range is absolute (only works at specific distance)
 */
bool Weapon::rangeAbsolute() const {
    return mask & MASK_ABSOLUTERANGE;
}

/**
 * Returns true if the weapon 'returns' to its user after used/thrown
 */
bool Weapon::returns() const {
    return mask & MASK_RETURNS;
}

/**
 * Return true if the weapon is lost when used
 */
bool Weapon::loseWhenUsed() const {
    return mask & MASK_LOSE;
}

/**
 * Returns true if the weapon is lost if it is a ranged attack
 */
bool Weapon::loseWhenRanged() const {
    return mask & MASK_LOSEWHENRANGED;
}

/**
 * Returns true if the weapon allows you to choose the distance
 */
bool Weapon::canChooseDistance() const {
    return mask & MASK_CHOOSEDISTANCE;
}

/**
 * Returns true if the weapon is magical
 */
bool Weapon::isMagic() const {
    return mask & MASK_MAGIC;
}

/**
 * Returns true if the weapon displays a tile while it travels
 * to give the appearance of 'flying' to its target
 */
bool Weapon::showTravel() const {
    return (mask & MASK_DONTSHOWTRAVEL) ? false : true;
}
