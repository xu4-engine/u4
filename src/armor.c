/*
 * $Id$
 */

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <libxml/xmlmemory.h>

#include "armor.h"

#include "error.h"
#include "names.h"
#include "tile.h"
#include "xml.h"

int armorInfoLoaded = 0;
Armor armors[MAX_ARMORS];

void armorLoadInfoFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node, child;
    int armor, i;

    if (!armorInfoLoaded)
        armorInfoLoaded = 1;
    else return;

    doc = xmlParse("armors.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "armors") != 0)
        errorFatal("malformed armors.xml");

    armor = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "armor") != 0)
            continue;

        armors[armor].name = xmlGetPropAsStr(node, "name");
        armors[armor].canuse = 0xFF;
        armors[armor].defense = xmlGetPropAsInt(node, "defense");
        armors[armor].mask = 0;

        /* Load armor attributes, if any 
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
                weapons[weapon].mask |= booleanAttributes[i].mask;
            }
        } */

        for (child = node->xmlChildrenNode; child; child = child->next) {
            unsigned char mask = 0;

            if (xmlNodeIsText(child) ||
                xmlStrcmp(child->name, "constraint") != 0)
                continue;

            for (i = 0; i < 8; i++) {
                if (xmlPropCaseCmp(child, "class", getClassName((ClassType)i)) == 0)
                    mask = (1 << i);
            }
            if (mask == 0 && xmlPropCaseCmp(child, "class", "all") == 0)
                mask = 0xFF;
            if (mask == 0) {
                errorFatal("malformed armor.xml file: constraint has unknown class %s", 
                           xmlGetPropAsStr(child, "class"));
            }
            if (xmlGetPropAsBool(child, "canuse"))
                armors[armor].canuse |= mask;
            else
                armors[armor].canuse &= ~mask;
        }

        armor++;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns the name of the armor
 */

char *armorGetName(int armor) {
    armorLoadInfoFromXml();

    return (char *)armors[armor].name;
}

/**
 * Returns the defense value of the armor
 */ 

int armorGetDefense(int armor) {
    armorLoadInfoFromXml();

    return armors[armor].defense;
}

/**
 * Returns true if the class given can wear the armor
 */

int armorCanWear(int armor, ClassType klass) {
    // Load in XML if it hasn't been already
    armorLoadInfoFromXml();

    return (armors[armor].canuse & (1 << klass)) != 0;
}
