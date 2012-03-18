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

    // Getters
    ArmorType getType() const       {return type;   } /**< Returns the ArmorType of the armor */
    const string &getName() const   {return name;   } /**< Returns the name of the armor */
    int getDefense() const          {return defense;} /**< Returns the defense value of the armor */
                                                      /** Returns true if the class given can wear the armor */
    bool canWear(ClassType klass) const {return canuse & (1 << klass);}

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
