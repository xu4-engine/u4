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
    typedef std::string string;

    static const Armor *get(ArmorType a);
    static const Armor *get(const string &name);

    ArmorType getType() const;
    const string &getName() const;
    int getDefense() const;
    bool canWear(ClassType klass) const;

private:
    Armor(const ConfigElement &conf);

    static void loadConf();
    static bool confLoaded;
    static std::vector<Armor *> armors;

    ArmorType type;
    string name;
    unsigned char canuse;
    int defense;
    unsigned short mask;
};

#endif
