/*
 * $Id$
 */

#ifndef ARMOR_H
#define ARMOR_H

#include <string>
#include <vector>
#include "savegame.h"

class ConfigElement;

class Armor {
public:
    static const Armor *get(ArmorType a);
    static const Armor *get(const std::string &name);

    ArmorType getType() const;
    const std::string &getName() const;
    int getDefense() const;
    bool canWear(ClassType klass) const;

private:
    Armor(const ConfigElement &conf);

    static void loadConf();
    static bool confLoaded;
    static std::vector<Armor *> armors;

    ArmorType type;
    std::string name;
    unsigned char canuse;
    int defense;
    unsigned short mask;
};

#endif
