/*
 * $Id$
 */

#ifndef WEAPON_H
#define WEAPON_H

#include <string>
#include "savegame.h"

/**< Flags affecting weapon's behavior. */
enum WeaponFlags {
    WEAP_LOSE                   = 0x0001,   /**< lost when used */
    WEAP_LOSEWHENRANGED         = 0x0002,   /**< lost when used for ranged attack */
    WEAP_CHOOSEDISTANCE         = 0x0004,   /**< allows player to choose attack distance */
    WEAP_ALWAYSHITS             = 0x0008,   /**< always hits it's target */
    WEAP_MAGIC                  = 0x0010,   /**< is magical */
    WEAP_ATTACKTHROUGHOBJECTS   = 0x0040,   /**< can attack through solid objects */
    WEAP_ABSOLUTERANGE          = 0x0080,   /**< range is absolute (only works at specific distance) */
    WEAP_RETURNS                = 0x0100,   /**< returns to user after used/thrown */
    WEAP_DONTSHOWTRAVEL         = 0x0200    /**< do not show animations when attacking */
};

class Weapon {
public:
    WeaponType getType() const          {return type;}
    const std::string &getName() const  {return name;}
    const std::string &getAbbrev() const{return abbr;}
    bool canReady(ClassType klass) const{return (canuse & (1 << klass)) != 0;}
    int getRange() const                {return range;}
    int getDamage() const               {return damage;}
    unsigned short getFlags() const     {return flags;}

    bool loseWhenUsed() const           {return flags & WEAP_LOSE;}
    bool loseWhenRanged() const         {return flags & WEAP_LOSEWHENRANGED;}
    bool canChooseDistance() const      {return flags & WEAP_CHOOSEDISTANCE;}
    bool alwaysHits() const             {return flags & WEAP_ALWAYSHITS;}
    bool isMagic() const                {return flags & WEAP_MAGIC;}
    bool canAttackThroughObjects() const{return flags & WEAP_ATTACKTHROUGHOBJECTS;}
    bool rangeAbsolute() const          {return flags & WEAP_ABSOLUTERANGE;}
    bool returns() const                {return flags & WEAP_RETURNS;}
    bool showTravel() const             {return !(flags & WEAP_DONTSHOWTRAVEL);}


    WeaponType type;
    std::string name;
    std::string abbr;       /**< abbreviation for the weapon */
    Symbol hitTile;         /**< tile to display a hit */
    Symbol missTile;        /**< tile to display a miss */
    Symbol leaveTile;       /**< if the weapon leaves a tile, the tile #, zero otherwise */
    uint16_t canuse;        /**< bitmask of classes that can use weapon */
    uint16_t range;         /**< range of weapon */
    uint16_t damage;        /**< damage of weapon */
    uint16_t flags;
};

class Armor {
public:
    // Getters
    ArmorType getType() const       {return type;   } /**< Returns the ArmorType of the armor */
    const std::string &getName() const   {return name;   } /**< Returns the name of the armor */
    int getDefense() const          {return defense;} /**< Returns the defense value of the armor */
                                                      /** Returns true if the class given can wear the armor */
    bool canWear(ClassType klass) const {return canuse & (1 << klass);}


    ArmorType type;
    std::string name;
    unsigned char canuse;
    int defense;
    unsigned short mask;
};

#endif
