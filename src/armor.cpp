/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

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

void Armor::loadConf() {
    if (!confLoaded)
        confLoaded = true;
    else
        return;

    const Config *config = Config::getInstance();

    vector<ConfigElement> armorConfs = config->getElement("/config/armors").getChildren();
    for (vector<ConfigElement>::iterator i = armorConfs.begin(); i != armorConfs.end(); i++) {
        if (i->getName() != "armor")
            continue;

        Armor *armor = new Armor;
        armor->type = static_cast<ArmorType>(armors.size());
        armor->name = i->getString("name");
        armor->canuse = 0xFF;
        armor->defense = i->getInt("defense");
        armor->mask = 0;

        vector<ConfigElement> contraintConfs = i->getChildren();
        for (vector<ConfigElement>::iterator j = contraintConfs.begin(); j != contraintConfs.end(); j++) {
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
                errorFatal("malformed armor.xml file: constraint has unknown class %s", 
                           j->getString("class").c_str());
            }
            if (j->getBool("canuse"))
                armor->canuse |= mask;
            else
                armor->canuse &= ~mask;
        }

        armors.push_back(armor);
    }
}

/**
 * Returns the ArmorType of the armor
 */
ArmorType Armor::getType() const {
    return type;
}

/**
 * Returns the name of the armor
 */
const string &Armor::getName() const {
    return name;
}

/**
 * Returns the defense value of the armor
 */ 
int Armor::getDefense() const {
    return defense;
}

/**
 * Returns true if the class given can wear the armor
 */
bool Armor::canWear(ClassType klass) const {
    return (canuse & (1 << klass)) != 0;
}

