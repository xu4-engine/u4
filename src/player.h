/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <list>
#include <string>
#include <vector>

#include "creature.h"
#include "observable.h"
#include "savegame.h"
#include "script.h"
#include "tile.h"
#include "types.h"

using std::string;

typedef std::vector<class PartyMember *> PartyMemberVector;

#define ALL_PLAYERS -1

typedef enum {
    KA_FOUND_ITEM,
    KA_STOLE_CHEST,    
    KA_GAVE_TO_BEGGAR,
    KA_GAVE_ALL_TO_BEGGAR,
    KA_BRAGGED,
    KA_HUMBLE,
    KA_HAWKWIND,
    KA_MEDITATION,
    KA_BAD_MANTRA,
    KA_ATTACKED_GOOD,
    KA_FLED_EVIL,
    KA_HEALTHY_FLED_EVIL,
    KA_KILLED_EVIL,
    KA_SPARED_GOOD,    
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD,
    KA_CHEAT_REAGENTS,
    KA_DIDNT_CHEAT_REAGENTS,
    KA_USED_SKULL,
    KA_DESTROYED_SKULL
} KarmaAction;

typedef enum {
    HT_NONE,
    HT_CURE,
    HT_FULLHEAL,
    HT_RESURRECT,
    HT_HEAL,
    HT_CAMPHEAL,
    HT_INNHEAL
} HealType;

typedef enum {
    INV_NONE,
    INV_WEAPON,
    INV_ARMOR,
    INV_FOOD,
    INV_REAGENT,
    INV_GUILDITEM,
    INV_HORSE
} InventoryItem;

typedef enum {    
    JOIN_SUCCEEDED,
    JOIN_NOT_EXPERIENCED,
    JOIN_NOT_VIRTUOUS
} CannotJoinError;

/**
 * PartyMember class
 */ 
class PartyMember : public Creature, public Script::Provider {
public:
    PartyMember(class Party *p, SaveGamePlayerRecord *pr);
    virtual ~PartyMember();

    void notifyOfChange();

    // Used to translate script values into something useful
    virtual string translate(std::vector<string>& parts);
    
    // Accessor methods
    int getHp() const;
    int getMaxHp() const;
    int getExp() const;
    int getStr() const;
    int getDex() const;
    int getInt() const;
    int getMp() const;
    int getMaxMp() const;
    WeaponType getWeapon() const;
    ArmorType getArmor() const;
    virtual string getName() const;    
    SexType getSex() const;
    ClassType getClass() const;
    StatusType getStatus() const;
    int getRealLevel() const;
    int getMaxLevel() const;

    virtual void addStatus(StatusType status);
    void adjustMp(int pts);
    void advanceLevel();    
    void applyEffect(TileEffect effect);
    void awardXp(int xp);
    bool heal(HealType type);    
    virtual void removeStatus(StatusType status);
    void setHp(int hp);
    void setMp(int mp);    
    void setArmor(ArmorType a);
    void setWeapon(WeaponType w);    
    
    virtual bool applyDamage(int damage);    
    virtual bool attackHit(Creature *m);
    virtual bool dealDamage(Creature *m, int damage);
    int getDamage();   
    virtual MapTile getHitTile() const;
    virtual MapTile getMissTile() const;    
    virtual bool isHit(int hit_offset = 0);
    bool isDead();
    bool isDisabled();
    int  loseWeapon();
    virtual void putToSleep();
    virtual void wakeUp();

protected:
    SaveGamePlayerRecord *player;
    class Party *party;    
};

/**
 * Party class
 */ 
typedef std::vector<PartyMember *> PartyMemberVector;

class Party : public Observable<Party *>, public Script::Provider {
    friend class PartyMember;
public:
    Party(SaveGame *saveGame);
    virtual ~Party();

    void notifyOfChange();
    
    // Used to translate script values into something useful
    virtual string translate(std::vector<string>& parts);
    
    void adjustFood(int food);
    void adjustGold(int gold);
    void adjustKarma(KarmaAction action);
    void applyEffect(TileEffect effect);
    bool attemptElevation(Virtue virtue);        
    void burnTorch(int turns = 1);
    bool canEnterShrine(Virtue virtue);    
    bool canPersonJoin(string name, Virtue *v);    
    void damageShip(unsigned int pts);
    bool donate(int quantity);
    void endTurn();
    int  getChest();
    int  getTorchDuration() const;    
    void healShip(unsigned int pts);
    bool isFlying() const;
    bool isImmobilized();
    bool isDead();
    bool isPersonJoined(string name);
    CannotJoinError join(string name);
    void lightTorch(int duration = 100, bool loseTorch = true);
    void quenchTorch();
    void reviveParty();
    void setTransport(MapTile transport);
    void setShipHull(int str);
    
    void adjustReagent(int reagent, int amt);
    int reagents(int reagent) const;

    int size() const;
    PartyMember *member(int index) const;    
    
//protected:
public:
    PartyMemberVector members;
    SaveGame *saveGame;
    MapTile transport;
    int torchduration;
};

typedef void (*LostEighthCallback)(Virtue);
typedef void (*AdvanceLevelCallback)(PartyMember *player);
typedef void (*PartyStarvingCallback)(void);
typedef void (*SetTransportCallback)(MapTile tile);

void playerSetLostEighthCallback(LostEighthCallback callback);
void playerSetAdvanceLevelCallback(AdvanceLevelCallback callback);
void playerSetPartyStarvingCallback(PartyStarvingCallback callback);
void playerSetSetTransportCallback(SetTransportCallback callback);

bool isPartyMember(Object *punknown);

#endif
