/**
 * creature.h
 */

#ifndef CREATURE_H
#define CREATURE_H

#include "object.h"
#include "movement.h"
#include "types.h"

class CombatController;
class Tile;

typedef uint16_t CreatureId;

/* Creatures on world map */

#define MAX_CREATURES_ON_MAP 4
#define MAX_CREATURE_DISTANCE 16

/* Creature ids */

typedef enum {
    HORSE1_ID       = 0,
    HORSE2_ID       = 1,

    MAGE_ID         = 2,
    BARD_ID         = 3,
    FIGHTER_ID      = 4,
    DRUID_ID        = 5,
    TINKER_ID       = 6,
    PALADIN_ID      = 7,
    RANGER_ID       = 8,
    SHEPHERD_ID     = 9,

    GUARD_ID        = 10,
    VILLAGER_ID     = 11,
    SINGINGBARD_ID  = 12,
    JESTER_ID       = 13,
    BEGGAR_ID       = 14,
    CHILD_ID        = 15,
    BULL_ID         = 16,
    LORDBRITISH_ID  = 17,

    PIRATE_ID       = 18,
    NIXIE_ID        = 19,
    GIANT_SQUID_ID  = 20,
    SEA_SERPENT_ID  = 21,
    SEAHORSE_ID     = 22,
    WHIRLPOOL_ID    = 23,
    STORM_ID        = 24,
    RAT_ID          = 25,
    BAT_ID          = 26,
    GIANT_SPIDER_ID = 27,
    GHOST_ID        = 28,
    SLIME_ID        = 29,
    TROLL_ID        = 30,
    GREMLIN_ID      = 31,
    MIMIC_ID        = 32,
    REAPER_ID       = 33,
    INSECT_SWARM_ID = 34,
    GAZER_ID        = 35,
    PHANTOM_ID      = 36,
    ORC_ID          = 37,
    SKELETON_ID     = 38,
    ROGUE_ID        = 39,
    PYTHON_ID       = 40,
    ETTIN_ID        = 41,
    HEADLESS_ID     = 42,
    CYCLOPS_ID      = 43,
    WISP_ID         = 44,
    EVILMAGE_ID     = 45,
    LICH_ID         = 46,
    LAVA_LIZARD_ID  = 47,
    ZORN_ID         = 48,
    DAEMON_ID       = 49,
    HYDRA_ID        = 50,
    DRAGON_ID       = 51,
    BALRON_ID       = 52,
} CreatureType;

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
    MATTR_DIVIDES       = 0x4000,
    MATTR_SPAWNSONDEATH = 0x8000,
    MATTR_FORCE_OF_NATURE = 0x10000
} CreatureAttrib;

typedef enum {
    MATTR_STATIONARY        = 0x1,
    MATTR_WANDERS           = 0x2,
    MATTR_SWIMS             = 0x4,
    MATTR_SAILS             = 0x8,
    MATTR_FLIES             = 0x10,
    MATTR_TELEPORT          = 0x20,
    MATTR_CANMOVECREATURES  = 0x40,
    MATTR_CANMOVEAVATAR     = 0x80
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
 * @todo
 * <ul>
 *      <li>split into a CreatureType (all the settings for a
 *      particular creature e.g. orc) and Creature (a specific
 *      creature instance)</li>
 *      <li>creatures can be looked up by name, ids can probably go away</li>
 * </ul>
 */
class Creature : public Object {
    typedef std::list<StatusType> StatusList;

public:
    static const Creature* getByTile(const MapTile& tile);
    static const Creature* getByName(const string& name);
    static const Creature* randomForTile(const Tile *tile);
    static const Creature* randomForDungeon(int dnglevel);
    static const Creature* randomAmbushing();

    Creature();
    Creature(const Creature* cproto);

    // Accessor methods
    virtual string getName() const              {return name;}
    virtual const string &getHitTile() const    {return rangedhittile;}
    virtual const string &getMissTile() const   {return rangedmisstile;}
    CreatureId getId() const                    {return id;}
    CreatureId getLeader() const                {return leader;}
    virtual int getHp() const                   {return hp;}
    virtual int getXp() const                   {return xp;}
    virtual const string &getWorldrangedtile() const {return worldrangedtile;}
    SlowedType getSlowedType() const            {return slowedType;}
    int getEncounterSize() const                {return encounterSize;}
    unsigned char getResists() const            {return resists;}

    // Setters
    void setName(string s)                      {name = s;}
    void setHitTile(const string &t)            {rangedhittile = t;}
    void setMissTile(const string &t)           {rangedmisstile = t;}
    virtual void setHp(int points)              {hp = points;}

    // Query methods
    bool isGood() const                 {return mattr & MATTR_GOOD;}
    bool isEvil() const                 {return !isGood();}
    bool isUndead() const               {return mattr & MATTR_UNDEAD;}
    bool leavesChest() const            {return !isAquatic() && !(mattr & MATTR_NOCHEST);}
    bool isAquatic() const              {return mattr & MATTR_WATER;}
    bool wanders() const                {return movementAttr & MATTR_WANDERS;}
    bool isStationary() const           {return movementAttr & MATTR_STATIONARY;}
    bool flies() const                  {return movementAttr & MATTR_FLIES;}
    bool teleports() const              {return movementAttr & MATTR_TELEPORT;}
    bool swims() const                  {return movementAttr & MATTR_SWIMS;}
    bool sails() const                  {return movementAttr & MATTR_SAILS;}
    bool walks() const                  {return !(flies() || swims() || sails());}
    bool divides() const                {return mattr & MATTR_DIVIDES;}
    bool spawnsOnDeath() const          {return mattr & MATTR_SPAWNSONDEATH;}
    bool canMoveOntoCreatures() const   {return movementAttr & MATTR_CANMOVECREATURES;}
    bool canMoveOntoPlayer() const      {return movementAttr & MATTR_CANMOVEAVATAR;}
    bool isAttackable() const;
    bool willAttack() const             {return !(mattr & MATTR_NOATTACK);}
    bool stealsGold() const             {return mattr & MATTR_STEALGOLD;}
    bool stealsFood() const             {return mattr & MATTR_STEALFOOD;}
    bool negates() const                {return mattr & MATTR_NEGATE;}
    bool camouflages() const            {return mattr & MATTR_CAMOUFLAGE;}
    bool ambushes() const               {return mattr & MATTR_AMBUSHES;}
    bool isIncorporeal() const          {return mattr & MATTR_INCORPOREAL;}
    bool hasRandomRanged() const        {return mattr & MATTR_RANDOMRANGED;}
    bool leavesTile() const             {return leavestile;}
    bool castsSleep() const             {return mattr & MATTR_CASTS_SLEEP;}
    bool isForceOfNature() const        {return mattr & MATTR_FORCE_OF_NATURE;}
    int getDamage() const;
    const string &getCamouflageTile() const {return camouflageTile;}
    void setRandomRanged();
    int setInitialHp(int hp = -1);

    bool specialAction();
    bool specialEffect();

    /* combat methods */
    void act(CombatController *controller);
    virtual void addStatus(StatusType status);
    void applyTileEffect(TileEffect effect);
    virtual int getAttackBonus() const;
    virtual int getDefense() const;
    bool divide();
    bool spawnOnDeath();
    virtual CreatureStatus getState() const;
    StatusType getStatus() const;
    bool isAsleep() const;
    bool hideOrShow();
    Creature *nearestOpponent(int *dist, bool ranged);
    virtual void putToSleep();
    virtual void removeStatus(StatusType status);
    virtual void setStatus(StatusType status);
    virtual void wakeUp();

    virtual bool applyDamage(int damage, bool byplayer = true);
    virtual bool dealDamage(Creature *m, int damage);

    // Properties
//protected:
    string          name;
    string          rangedhittile;
    string          rangedmisstile;
    CreatureId      id;
    string          camouflageTile;
    CreatureId      leader;
    int             basehp;
    int             hp;
    StatusList      status;
    int             xp;
    unsigned char   ranged;
    string          worldrangedtile;
    bool            leavestile;
    CreatureAttrib  mattr;
    CreatureMovementAttrib movementAttr;
    SlowedType      slowedType;
    int             encounterSize;
    unsigned char   resists;
    CreatureId      spawn;
};

bool isCreature(Object *punknown);

#endif
