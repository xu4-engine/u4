/*
 * $Id$
 */

#ifndef PLAYER_H
#define PLAYER_H

#include <list>
#include <string>
#include <vector>

#include "creature.h"
#include "direction.h"
#include "observable.h"
#include "savegame.h"
#include "script.h"
#include "tile.h"
#include "types.h"
#ifdef IOS
#include "ios_helpers.h"
#endif

class Armor;
class Party;
class Weapon;

using std::string;

typedef std::vector<class PartyMember *> PartyMemberVector;

#define ALL_PLAYERS -1

enum KarmaAction {
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
    KA_FLED_GOOD,
    KA_HEALTHY_FLED_EVIL,
    KA_KILLED_EVIL,
    KA_SPARED_GOOD,    
    KA_DONATED_BLOOD,
    KA_DIDNT_DONATE_BLOOD,
    KA_CHEAT_REAGENTS,
    KA_DIDNT_CHEAT_REAGENTS,
    KA_USED_SKULL,
    KA_DESTROYED_SKULL
};

enum HealType {
    HT_NONE,
    HT_CURE,
    HT_FULLHEAL,
    HT_RESURRECT,
    HT_HEAL,
    HT_CAMPHEAL,
    HT_INNHEAL
};

enum InventoryItem {
    INV_NONE,
    INV_WEAPON,
    INV_ARMOR,
    INV_FOOD,
    INV_REAGENT,
    INV_GUILDITEM,
    INV_HORSE
};

enum CannotJoinError {    
    JOIN_SUCCEEDED,
    JOIN_NOT_EXPERIENCED,
    JOIN_NOT_VIRTUOUS
};

enum EquipError {
    EQUIP_SUCCEEDED,
    EQUIP_NONE_LEFT,
    EQUIP_CLASS_RESTRICTED
};

/**
 * PartyMember class
 */ 
class PartyMember : public Creature, public Script::Provider {
public:
    PartyMember(Party *p, SaveGamePlayerRecord *pr);
    virtual ~PartyMember();

    void notifyOfChange();

    // Used to translate script values into something useful
    virtual string translate(std::vector<string>& parts);
    
    // Accessor methods
    virtual int getHp() const;
    int getMaxHp() const   { return player->hpMax; }
    int getExp() const     { return player->xp; }
    int getStr() const     { return player->str; }
    int getDex() const     { return player->dex; }
    int getInt() const     { return player->intel; }
    int getMp() const      { return player->mp; }
    int getMaxMp() const;
    const Weapon *getWeapon() const;
    const Armor *getArmor() const;
    virtual string getName() const;    
    SexType getSex() const;
    ClassType getClass() const;
    virtual CreatureStatus getState() const;
    int getRealLevel() const;
    int getMaxLevel() const;

    virtual void addStatus(StatusType status);
    void adjustMp(int pts);
    void advanceLevel();    
    void applyEffect(TileEffect effect);
    void awardXp(int xp);
    bool heal(HealType type);    
    virtual void removeStatus(StatusType status);
    virtual void setHp(int hp);
    void setMp(int mp);    
    EquipError setArmor(const Armor *a);
    EquipError setWeapon(const Weapon *w);    
    
    virtual bool applyDamage(int damage, bool byplayer = false);    
    virtual int getAttackBonus() const;
    virtual int getDefense() const;
    virtual bool dealDamage(Creature *m, int damage);
    int getDamage();   
    virtual const string &getHitTile() const;
    virtual const string &getMissTile() const;    
    bool isDead();
    bool isDisabled();
    int  loseWeapon();
    virtual void putToSleep();
    virtual void wakeUp();

protected:
    static MapTile tileForClass(int klass);

    SaveGamePlayerRecord *player;
    class Party *party;    
};

/**
 * Party class
 */ 
class PartyEvent;
typedef std::vector<PartyMember *> PartyMemberVector;

class Party : public Observable<Party *, PartyEvent &>, public Script::Provider {
    friend class PartyMember;
public:
    Party(SaveGame *saveGame);
    virtual ~Party();

    void notifyOfChange(PartyMember *partyMember = 0);
    
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
    bool lightTorch(int duration = 100, bool loseTorch = true);
    void quenchTorch();
    void reviveParty();
    MapTile getTransport() const;
    void setTransport(MapTile transport);
    void setShipHull(int str);

    Direction getDirection() const;
    void setDirection(Direction dir);

    void adjustReagent(int reagent, int amt);
    int getReagent(int reagent) const;
    short* getReagentPtr(int reagent) const;

    void setActivePlayer(int p);
    int getActivePlayer() const;

    void swapPlayers(int p1, int p2);

    int size() const;
    PartyMember *member(int index) const;    
    
private:
    void syncMembers();
    PartyMemberVector members;
    SaveGame *saveGame;
    MapTile transport;
    int torchduration;
    int activePlayer;
#ifdef IOS
    friend void U4IOS::syncPartyMembersWithSaveGame();
#endif
};

class PartyEvent {
public:
    enum Type {
        GENERIC,
        LOST_EIGHTH,
        ADVANCED_LEVEL,
        STARVING,
        TRANSPORT_CHANGED,
        PLAYER_KILLED,
        ACTIVE_PLAYER_CHANGED,
        MEMBER_JOINED,
        PARTY_REVIVED
    };

    PartyEvent(Type type, PartyMember *partyMember) : type(type), player(partyMember) { }

    Type type;
    PartyMember *player;
};

bool isPartyMember(Object *punknown);

#endif
