/**
 * $Id$
 */

#ifndef CREATURE_H
#define CREATURE_H

#include <map>
#include <vector>

#include "object.h"
#include "map.h"
#include "movement.h"
#include "savegame.h"
#include "types.h"

typedef unsigned short CreatureId;
typedef std::map<CreatureId, class Creature*, std::less<CreatureId> > CreatureMap;
typedef std::vector<class Creature *> CreatureVector;

#define MAX_CREATURES 128

/* Creatures on world map */

#define MAX_CREATURES_ON_MAP 4
#define MAX_CREATURE_DISTANCE 10

/* Creature ids */

#define HORSE1_ID       0
#define HORSE2_ID       1

#define MAGE_ID         2
#define BARD_ID         3
#define FIGHTER_ID      4
#define DRUID_ID        5
#define TINKER_ID       6
#define PALADIN_ID      7
#define RANGER_ID       8
#define SHEPHERD_ID     9

#define GUARD_ID        10
#define VILLAGER_ID     11
#define SINGINGBARD_ID  12
#define JESTER_ID       13
#define BEGGAR_ID       14
#define CHILD_ID        15
#define BULL_ID         16
#define LORDBRITISH_ID  17

#define PIRATE_ID       18
#define NIXIE_ID        19
#define GIANT_SQUID_ID  20
#define SEA_SERPENT_ID  21
#define SEAHORSE_ID     22
#define WHIRLPOOL_ID    23
#define STORM_ID        24
#define RAT_ID          25
#define BAT_ID          26
#define GIANT_SPIDER_ID 27
#define GHOST_ID        28
#define SLIME_ID        29
#define TROLL_ID        30
#define GREMLIN_ID      31
#define MIMIC_ID        32
#define REAPER_ID       33
#define INSECT_SWARM_ID 34
#define GAZER_ID        35
#define PHANTOM_ID      36
#define ORC_ID          37
#define SKELETON_ID     38
#define ROGUE_ID        39
#define PYTHON_ID       40
#define ETTIN_ID        41
#define HEADLESS_ID     42
#define CYCLOPS_ID      43
#define WISP_ID         44
#define EVILMAGE_ID     45
#define LICH_ID         46
#define LAVA_LIZARD_ID  47
#define ZORN_ID         48
#define DAEMON_ID       49
#define HYDRA_ID        50
#define DRAGON_ID       51
#define BALRON_ID       52

typedef enum {
    MATTR_STEALFOOD     = 0x1,
    MATTR_STEALGOLD     = 0x2,
    MATTR_CASTS_SLEEP   = 0x4,
    MATTR_UNDEAD        = 0x8,
    MATTR_GOOD          = 0x10,
    MATTR_WATER         = 0x20,
    MATTR_NONATTACKABLE = 0x40,
    MATTR_NEGATE        = 0x80,    
    MATTR_CAMOUFLAGE    = 0x100,    
    MATTR_NOATTACK      = 0x200,    
    MATTR_AMBUSHES      = 0x400,
    MATTR_RANDOMRANGED  = 0x800,
    MATTR_INCORPOREAL   = 0x1000,
    MATTR_NOCHEST       = 0x2000,
    MATTR_DIVIDES       = 0x4000
} CreatureAttrib;

typedef enum {
    MATTR_STATIONARY        = 0x1,
    MATTR_WANDERS           = 0x2,
    MATTR_SWIMS             = 0x4,
    MATTR_SAILS             = 0x8,
    MATTR_FLIES             = 0x10,
    MATTR_TELEPORT          = 0x20,
    MATTR_CANMOVECREATURES  = 0x40,
    MATTR_CANMOVEAVATAR     = 0x80,
    MATTR_CANMOVEON         = 0x100
} CreatureMovementAttrib;

typedef enum {
    MSTAT_DEAD,
    MSTAT_FLEEING,
    MSTAT_CRITICAL,
    MSTAT_HEAVILYWOUNDED,
    MSTAT_LIGHTLYWOUNDED,
    MSTAT_BARELYWOUNDED
} CreatureStatus;

/**
 * Creature Class Definition
 */ 

class Creature : public Object {
    typedef std::list<StatusType> StatusList;

public:
    Creature(MapTile tile = 0);

    // Accessor methods
    virtual string getName() const;
    virtual MapTile getHitTile() const;
    virtual MapTile getMissTile() const;

    void setName(string s);
    void setHitTile(MapTile t);
    void setMissTile(MapTile t);

    // Methods
    bool isGood() const;
    bool isEvil() const;
    bool isUndead() const;
    bool leavesChest() const;
    bool isAquatic() const;
    bool wanders() const;
    bool isStationary() const;
    bool flies() const;
    bool teleports() const;
    bool swims() const;
    bool sails() const;
    bool walks() const;
    bool divides() const;
    bool canMoveOntoCreatures() const;
    bool canMoveOntoPlayer() const;
    bool canMoveOnto() const;
    bool isAttackable() const;
    bool willAttack() const;
    bool stealsGold() const;
    bool stealsFood() const;
    bool negates() const;
    bool camouflages() const;
    bool ambushes() const;
    bool isIncorporeal() const;
    bool hasRandomRanged() const;
    bool leavesTile() const;
    bool castsSleep() const;
    int getDamage() const;    
    MapTile getCamouflageTile() const;    
    void setRandomRanged();
    int setInitialHp(int hp = -1);

    bool specialAction();
    bool specialEffect();

    /* combat methods */
    void act();
    virtual void addStatus(StatusType status);
    void applyTileEffect(TileEffect effect);
    virtual bool attackHit(Creature *m);
    bool divide();
    CreatureStatus getState() const;
    StatusType getStatus() const;
    bool hideOrShow();
    virtual bool isHit(int hit_offset = 0);
    Creature *nearestOpponent(int *dist, bool ranged);
    virtual void putToSleep();
    virtual void removeStatus(StatusType status);
    virtual void setStatus(StatusType status);
    virtual void wakeUp();

    virtual bool applyDamage(int damage);
    virtual bool dealDamage(Creature *m, int damage);

    // Properties
protected:
    string          name;
    MapTile         rangedhittile;
    MapTile         rangedmisstile;

public:    
    CreatureId       id;    
    MapTile         camouflageTile;
    unsigned char   frames;
    CreatureId       leader;
    unsigned short  basehp;
    short           hp;
    StatusList      status;
    unsigned short  xp;
    unsigned char   ranged;
    MapTile         worldrangedtile;    
    bool            leavestile;
    CreatureAttrib   mattr;
    CreatureMovementAttrib movementAttr;
    SlowedType      slowedType;
    unsigned char   encounterSize;
    unsigned char   resists;
};

/**
 * CreatureMgr Class Definition
 */ 
class CreatureMgr {
public:
    CreatureMgr();

    void loadInfoFromXml();

    const Creature *getByTile(MapTile tile) const;
    const Creature *getById(CreatureId id) const;
    const Creature *getByName(string name) const;
    const Creature *randomForTile(MapTile tile) const;
    const Creature *randomForDungeon(int dnglevel) const;
    const Creature *randomAmbushing() const;

private:    
    CreatureMap creatures;    
};

bool isCreature(Object *punknown);

extern CreatureMgr creatures;

#endif
