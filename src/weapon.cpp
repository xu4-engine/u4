/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>

#include <libxml/xmlmemory.h>

using std::string;

#include "weapon.h"

#include "error.h"
#include "names.h"
#include "tile.h"
#include "xml.h"

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

int weaponInfoLoaded = 0;
int numWeapons = 0;
Weapon weapons[MAX_WEAPONS];

void weaponLoadInfoFromXml() {
    char *range;
    xmlDocPtr doc;
    xmlNodePtr root, node, child;
    unsigned int i;
    int weapon;
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

    if (!weaponInfoLoaded)
        weaponInfoLoaded = 1;
    else return;

    doc = xmlParse("weapons.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "weapons") != 0)
        errorFatal("malformed weapons.xml");

    weapon = 0;
    numWeapons = 0;

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "weapon") != 0)
            continue;

        weapons[weapon].name = xmlGetPropAsStr(node, "name");        
        weapons[weapon].abbr = xmlGetPropAsStr(node, "abbr");
        weapons[weapon].canuse = 0xFF;
        weapons[weapon].damage = xmlGetPropAsInt(node, "damage");
        weapons[weapon].hittile = HITFLASH_TILE;
        weapons[weapon].misstile = MISSFLASH_TILE;
        weapons[weapon].leavetile = 0;
        weapons[weapon].mask = 0;

        /* Get the range of the weapon, whether it is absolute or normal range */
        range = xmlGetPropAsStr(node, "range");
        if (range == NULL) {
            range = xmlGetPropAsStr(node, "absolute_range");
            if (range != NULL)
                weapons[weapon].mask |= MASK_ABSOLUTERANGE;
        }
        if (range == NULL)
            errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", weapons[weapon].name.c_str());

        weapons[weapon].range = atoi(range);

        /* Load weapon attributes */
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
                weapons[weapon].mask |= booleanAttributes[i].mask;
            }
        }

        /* Load hit tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "hittile", tiles[i].name) == 0) {
                weapons[weapon].hittile = tiles[i].tile;
            }
        }

        /* Load miss tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "misstile", tiles[i].name) == 0) {
                weapons[weapon].misstile = tiles[i].tile;
            }
        }

        /* Load leave tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlPropCmp(node, "leavetile", tiles[i].name) == 0) {
                weapons[weapon].mask |= MASK_LEAVETILE;
                weapons[weapon].leavetile = tiles[i].tile;
            }
        }

        for (child = node->xmlChildrenNode; child; child = child->next) {
            unsigned char mask = 0;

            if (xmlNodeIsText(child) ||
                xmlStrcmp(child->name, (const xmlChar *) "constraint") != 0)
                continue;

            for (i = 0; i < 8; i++) {
                if (xmlPropCaseCmp(child, "class", getClassName((ClassType)i)) == 0)
                    mask = (1 << i);
            }
            if (mask == 0 && xmlPropCaseCmp(child, "class", "all") == 0)
                mask = 0xFF;
            if (mask == 0) {
                errorFatal("malformed weapons.xml file: constraint has unknown class %s", 
                           xmlGetPropAsStr(child, "class"));
            }
            if (xmlGetPropAsBool(child, "canuse"))
                weapons[weapon].canuse |= mask;
            else
                weapons[weapon].canuse &= ~mask;
        }

        weapon++;
        numWeapons++;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns the name of the weapon
 */
string *weaponGetName(int weapon) {
    weaponLoadInfoFromXml();

    return &weapons[weapon].name;
}

/**
 * Returns the abbreviation for the weapon
 */
string *weaponGetAbbrev(int weapon) {
    weaponLoadInfoFromXml();

    return &weapons[weapon].abbr;
}

/**
 * Return the range of the weapon
 */
int weaponGetRange(int weapon) {
    weaponLoadInfoFromXml();

    return weapons[weapon].range;
}

/**
 * Return the damage for the specified weapon
 */
int weaponGetDamage(int weapon) {
    weaponLoadInfoFromXml();

    return weapons[weapon].damage;
}

/**
 * Return the hit tile for the specified weapon
 */
int weaponGetHitTile(int weapon) {
    weaponLoadInfoFromXml();

    return weapons[weapon].hittile;
}

/**
 * Return the miss tile for the specified weapon
 */
int weaponGetMissTile(int weapon) {
    weaponLoadInfoFromXml();

    return weapons[weapon].misstile;
}

/**
 * Returns true if the weapon always hits it's target
 */
int weaponAlwaysHits(int weapon) {
    weaponLoadInfoFromXml();

    return weapons[weapon].mask & MASK_ALWAYSHITS;
}

/**
 * Returns 0 if the weapon leaves no tile, otherwise it
 * returns the # of the tile the weapon leaves
 */
unsigned char weaponLeavesTile(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_LEAVETILE) ? weapons[weapon].leavetile : 0;
}

/**
 * Returns true if the class given can ready the weapon
 */
int weaponCanReady(int weapon, ClassType klass) {
    // Load in XML if it hasn't been already
    weaponLoadInfoFromXml();

    return (weapons[weapon].canuse & (1 << klass)) != 0;
}

/**
 * Returns true if the weapon can attack through solid objects
 */
int weaponCanAttackThroughObjects(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_ATTACKTHROUGHOBJECTS);
}

/**
 * Returns true if the weapon's range is absolute (only works at specific distance)
 */
int weaponRangeAbsolute(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_ABSOLUTERANGE);
}

/**
 * Returns true if the weapon 'returns' to its user after used/thrown
 */
int weaponReturns(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_RETURNS);
}

/**
 * Return true if the weapon is lost when used
 */
int weaponLoseWhenUsed(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_LOSE);
}

/**
 * Returns true if the weapon is lost if it is a ranged attack
 */
int weaponLoseWhenRanged(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_LOSEWHENRANGED);
}

/**
 * Returns true if the weapon allows you to choose the distance
 */
int weaponCanChooseDistance(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_CHOOSEDISTANCE);
}

/**
 * Returns true if the weapon is magical
 */
int weaponIsMagic(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_MAGIC);
}

/**
 * Returns true if the weapon displays a tile while it travels
 * to give the appearance of 'flying' to its target
 */
int weaponShowTravel(int weapon) {
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_DONTSHOWTRAVEL) ? 0 : 1;
}

/**
 * Returns the weapon that corresponds to the given name
 */ 
int weaponGetByName(const char *name) {
    int i;
    for (i = 0; i < numWeapons; i++) {
        if (strcasecmp(name, weapons[i].name.c_str()) == 0)
            return i;
    }
    return -1;
}

