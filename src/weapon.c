#include <stddef.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "ttype.h"
#include "u4file.h"
#include "weapon.h"
#include "error.h"

#define MASK_LOSE               0x0001
#define MASK_LOSEWHENRANGED     0x0002
#define MASK_CHOOSEDISTANCE     0x0004
#define MASK_ALWAYSHIT          0x0008
#define MASK_MAGIC              0x0010
#define MASK_LEAVETILE          0x0020

int weaponInfoLoaded = 0;
Weapon weapons[MAX_WEAPONS];

void weaponLoadInfoFromXml() {
    char *fname;
    xmlDocPtr doc;
    xmlNodePtr root, node;
    int weapon, i;
    static const struct {
        const char *name;
        unsigned int mask;        
    } booleanAttributes[] = {
        { "lose", MASK_LOSE },
        { "losewhenranged", MASK_LOSEWHENRANGED },
        { "choosedistance", MASK_CHOOSEDISTANCE },
        { "alwayshit", MASK_ALWAYSHIT },
        { "magic", MASK_MAGIC }        
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

    fname = u4find_conf("weapons.xml");
    if (!fname)
        errorFatal("unable to open file weapons.xml");
    doc = xmlParseFile(fname);
    if (!doc)
        errorFatal("error parsing weapons.xml");

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "weapons") != 0)
        errorFatal("malformed weapons.xml");

    weapon = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "weapon") != 0)
            continue;

        weapons[weapon].name = (char *)xmlGetProp(node, (const xmlChar *)"name");        
        weapons[weapon].abbr = (char *)xmlGetProp(node, (const xmlChar *)"abbr");
        weapons[weapon].range = atoi(xmlGetProp(node, (const xmlChar *)"range"));
        weapons[weapon].damage = atoi(xmlGetProp(node, (const xmlChar *)"damage"));
        weapons[weapon].hittile = HITFLASH_TILE;
        weapons[weapon].misstile = MISSFLASH_TILE;
        weapons[weapon].leavetile = 0;
        weapons[weapon].mask = 0;          

        /* Load weapon attributes */
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) booleanAttributes[i].name), 
                          (const xmlChar *) "true") == 0) {
                weapons[weapon].mask |= booleanAttributes[i].mask;
            }
        }

        /* Load hit tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "hittile"),
                          (const xmlChar *) tiles[i].name) == 0) {
                weapons[weapon].hittile = tiles[i].tile;
            }
        }

        /* Load miss tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "misstile"),
                          (const xmlChar *) tiles[i].name) == 0) {
                weapons[weapon].misstile = tiles[i].tile;
            }
        }

        /* Load leave tiles */
        for (i = 0; i < sizeof(tiles) / sizeof(tiles[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) "leavetile"),
                          (const xmlChar *) tiles[i].name) == 0) {
                weapons[weapon].mask |= MASK_LEAVETILE;
                weapons[weapon].leavetile = tiles[i].tile;
            }
        }

        weapon++;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns the name of the weapon
 */

char *weaponGetName(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].name;
}

/**
 * Returns the abbreviation for the weapon
 */

char *weaponGetAbbrev(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].abbr;
}

/**
 * Return the range of the weapon
 */

int weaponGetRange(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].range;
}

/**
 * Return the damage for the specified weapon
 */ 

int weaponGetDamage(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].damage;
}

/**
 * Return the hit tile for the specified weapon
 */ 

int weaponGetHitTile(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].hittile;
}

/**
 * Return the miss tile for the specified weapon
 */ 

int weaponGetMissTile(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].misstile;
}

/**
 * Returns true if the weapon always hits it's target
 */

int weaponAlwaysHit(int weapon)
{
    weaponLoadInfoFromXml();

    return weapons[weapon].mask & MASK_ALWAYSHIT;
}

/**
 * Returns 0 if the weapon leaves no tile, otherwise it
 * returns the # of the tile the weapon leaves
 */

int weaponLeavesTile(int weapon)
{
    weaponLoadInfoFromXml();

    return (weapons[weapon].mask & MASK_LEAVETILE) ? weapons[weapon].leavetile : 0;
}
