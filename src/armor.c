#include <stddef.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "ttype.h"
#include "u4file.h"
#include "armor.h"
#include "error.h"

int armorInfoLoaded = 0;
Armor armors[MAX_ARMORS];

void armorLoadInfoFromXml() {
    char *fname;
    xmlDocPtr doc;
    xmlNodePtr root, node;
    int armor, i;
    
    if (!armorInfoLoaded)
        armorInfoLoaded = 1;
    else return;

    fname = u4find_conf("armors.xml");
    if (!fname)
        errorFatal("unable to open file armors.xml");
    doc = xmlParseFile(fname);
    if (!doc)
        errorFatal("error parsing armors.xml");

    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "armors") != 0)
        errorFatal("malformed armors.xml");

    armor = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "armor") != 0)
            continue;

        armors[armor].name = (char *)xmlGetProp(node, (const xmlChar *)"name");
        armors[armor].canwear = (char *)xmlGetProp(node, (const xmlChar *)"canwear");
        armors[armor].cantwear = (char *)xmlGetProp(node, (const xmlChar *)"cantwear");
        armors[armor].defense = atoi(xmlGetProp(node, (const xmlChar *)"defense"));
        armors[armor].mask = 0;

        /* Load armor attributes, if any 
        for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
            if (xmlStrcmp(xmlGetProp(node, (const xmlChar *) booleanAttributes[i].name), 
                          (const xmlChar *) "true") == 0) {
                weapons[weapon].mask |= booleanAttributes[i].mask;
            }
        } */

        armor++;
    }

    xmlFreeDoc(doc);
}

/**
 * Returns the name of the armor
 */

char *armorGetName(int armor)
{
    armorLoadInfoFromXml();

    return (char *)armors[armor].name;
}

/**
 * Returns the defense value of the armor
 */ 

int armorGetDefense(int armor)
{
    armorLoadInfoFromXml();

    return armors[armor].defense;
}

/**
 * Returns true if the class given can wear the armor
 */

int armorCanWear(int armor, const char *className)
{
    char *klass;
    int allCanWear = 1;
    int retval = 0;

    klass = (char *)strlwr(strdup(className));
    
    // Load in XML if it hasn't been already
    armorLoadInfoFromXml();

    if (armors[armor].canwear)
        allCanWear = 0;
    
    if (allCanWear)
    {
        if (!(armors[armor].cantwear && strstr(armors[armor].cantwear, klass)))
            retval = 1;
    }
    else if (armors[armor].canwear && strstr(armors[armor].canwear, klass))
        retval = 1;

    free(klass);
    return retval;
}