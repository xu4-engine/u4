/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>

#include "weapon.h"

#include "config.h"
#include "error.h"
#include "names.h"
#include "tile.h"

using std::string;
using std::vector;

#define MASK_LOSE                   0x0001
#define MASK_LOSEWHENRANGED         0x0002
#define MASK_CHOOSEDISTANCE         0x0004
#define MASK_ALWAYSHITS             0x0008
#define MASK_MAGIC                  0x0010
#define MASK_LEAVETILE              0x0020
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

void Weapon::loadConf() {
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

    static const struct {
        const char *name;
        unsigned int tile;
    } tiles[] = {
        { "fire", FIREFIELD_TILE },
        { "poison", POISONFIELD_TILE },
        { "lightning", LIGHTNINGFIELD_TILE },
        { "magic", MAGICFLASH_TILE }
    };

    if (!confLoaded)
        confLoaded = true;
    else
        return;

    const Config *config = Config::getInstance();

    vector<ConfigElement> weaponConfs = config->getElement("/config/weapons").getChildren();
    for (std::vector<ConfigElement>::iterator i = weaponConfs.begin(); i != weaponConfs.end(); i++) {
        if (i->getName() != "weapon")
            continue;

        Weapon *weapon = new Weapon;
        weapon->type = static_cast<WeaponType>(weapons.size());
        weapon->name = i->getString("name");
        weapon->abbr = i->getString("abbr");
        weapon->canuse = 0xFF;
        weapon->damage = i->getInt("damage");
        weapon->hittile = HITFLASH_TILE;
        weapon->misstile = MISSFLASH_TILE;
        weapon->leavetile = 0;
        weapon->mask = 0;

        /* Get the range of the weapon, whether it is absolute or normal range */
        string range = i->getString("range");
        if (range.empty()) {
            range = i->getString("absolute_range");
            if (!range.empty())
                weapon->mask |= MASK_ABSOLUTERANGE;
        }
        if (range.empty())
            errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", weapon->name.c_str());

        weapon->range = atoi(range.c_str());

        /* Load weapon attributes */
        for (unsigned at = 0; at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); at++) {
            if (i->getBool(booleanAttributes[at].name)) {
                weapon->mask |= booleanAttributes[at].mask;
            }
        }

        /* Load hit tiles */
        for (unsigned ht = 0; ht < sizeof(tiles) / sizeof(tiles[0]); ht++) {
            if (i->getString("hittile") == tiles[ht].name) {
                weapon->hittile = tiles[ht].tile;
                break;
            }
        }

        /* Load miss tiles */
        for (unsigned mt = 0; mt < sizeof(tiles) / sizeof(tiles[0]); mt++) {
            if (i->getString("misstile") == tiles[mt].name) {
                weapon->misstile = tiles[mt].tile;
                break;
            }
        }

        /* Load leave tiles */
        for (unsigned lt = 0; lt < sizeof(tiles) / sizeof(tiles[0]); lt++) {
            if (i->getString("leavetile") == tiles[lt].name) {
                weapon->mask |= MASK_LEAVETILE;
                weapon->leavetile = tiles[lt].tile;
                break;
            }
        }

        vector<ConfigElement> contraintConfs = i->getChildren();
        for (std::vector<ConfigElement>::iterator j = contraintConfs.begin(); j != contraintConfs.end(); j++) {
            unsigned char mask = 0;

            if (j->getName() != "constraint")
                continue;

            for (int cl = 0; cl < 8; cl++) {
                if (strcasecmp(j->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                    mask = (1 << cl);
            }
            if (mask == 0 && strcasecmp(j->getString("class").c_str(), "all") == 0)
                mask = 0xFF;
            if (mask == 0) {
                errorFatal("malformed weapons.xml file: constraint has unknown class %s", 
                           j->getString("class").c_str());
            }
            if (j->getBool("canuse"))
                weapon->canuse |= mask;
            else
                weapon->canuse &= ~mask;
        }

        weapons.push_back(weapon);
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
int Weapon::getHitTile() const {
    return hittile;
}

/**
 * Return the miss tile for the specified weapon
 */
int Weapon::getMissTile() const {
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
unsigned char Weapon::leavesTile() const {
    return (mask & MASK_LEAVETILE) ? leavetile : 0;
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
