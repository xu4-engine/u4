/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>
#include <cstring>

#include "armor.h"

#include "config.h"
#include "error.h"
#include "names.h"
#include "tile.h"

using std::vector;
using std::string;

bool Armor::confLoaded = false;
vector<Armor *> Armor::armors;

/**
 * Returns armor by ArmorType.
 */ 
const Armor *Armor::get(ArmorType a) {
    // Load in XML if it hasn't been already
    loadConf();

    if (static_cast<unsigned>(a) >= armors.size())
        return NULL;
    return armors[a];
}

/**
 * Returns armor that has the given name
 */ 
const Armor *Armor::get(const string &name) {
    // Load in XML if it hasn't been already
    loadConf();

    for (unsigned i = 0; i < armors.size(); i++) {
        if (strcasecmp(name.c_str(), armors[i]->name.c_str()) == 0)
            return armors[i];
    }
    return NULL;
}

Armor::Armor(const ConfigElement &conf) {
    type = static_cast<ArmorType>(armors.size());
    name = conf.getString("name");
    canuse = 0xFF;
    defense = conf.getInt("defense");
    mask = 0;

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
            errorFatal("malformed armor.xml file: constraint has unknown class %s", 
                       i->getString("class").c_str());
        }
        if (i->getBool("canuse"))
            canuse |= mask;
        else
            canuse &= ~mask;
    }
}

void Armor::loadConf() {
    if (!confLoaded)
        confLoaded = true;
    else
        return;

    const Config *config = Config::getInstance();

    vector<ConfigElement> armorConfs = config->getElement("armors").getChildren();    
    for (std::vector<ConfigElement>::iterator i = armorConfs.begin(); i != armorConfs.end(); i++) {
        if (i->getName() != "armor")
            continue;

        armors.push_back(new Armor(*i));
    }
}

