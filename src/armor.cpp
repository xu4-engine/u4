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

bool armorInfoLoaded = false;
Armor armors[MAX_ARMORS];
int numArmors;

void armorLoadInfoFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node, child;
    int i;

    if (!armorInfoLoaded)
        armorInfoLoaded = true;
    else return;

    doc = xmlParse("armors.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "armors") != 0)
        errorFatal("malformed armors.xml");

    numArmors = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "armor") != 0)
            continue;

        armors[numArmors].name = xmlGetPropAsStr(node, "name");
        armors[numArmors].canuse = 0xFF;
        armors[numArmors].defense = xmlGetPropAsInt(node, "defense");
        armors[numArmors].mask = 0;

        /* Load armor attributes, if any 
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlGetPropAsBool(node, booleanAttributes[i].name)) {
                weapons[weapon].mask |= booleanAttributes[i].mask;
            }
        } */

        for (child = node->xmlChildrenNode; child; child = child->next) {
            unsigned char mask = 0;

            if (xmlNodeIsText(child) ||
                xmlStrcmp(child->name, (xmlChar *)"constraint") != 0)
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
                armors[numArmors].canuse |= mask;
            else
                armors[numArmors].canuse &= ~mask;
        }

        numArmors++;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns the name of the armor
 */

string *armorGetName(int armor) {
    armorLoadInfoFromXml();

    return &armors[armor].name;
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

/**
 * Returns the armor that has the given name
 */ 

int armorGetByName(const char *name) {
    int i;
    for (i = 0; i < numArmors; i++) {
        if (strcasecmp(name, armors[i].name.c_str()) == 0)
            return i;
    }
    return -1;
}

