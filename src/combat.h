/*
 * combat.h
 */

#ifndef COMBAT_H
#define COMBAT_H

#include "game.h"
#include "player.h"

#define AREA_CREATURES   16
#define AREA_PLAYERS    8

class CombatMap;
class Creature;
class Dungeon;
class MoveEvent;
class Weapon;

typedef enum {
    CA_ATTACK,
    CA_CAST_SLEEP,
    CA_ADVANCE,
    CA_RANGED,
    CA_FLEE,
    CA_TELEPORT
} CombatAction;

/**
 * CombatController class
 */
class CombatController : public Controller, public Observer<Party *, PartyEvent &>, public TurnCompleter {
public:
    static bool attackHit(const Creature *attacker, const Creature *defender);
    static void engage(MapId mid, const Creature* creatures);
    static void engageDungeon(Dungeon* dng, int room, Direction from);

    CombatController(CombatMap* cmap = NULL);
    virtual ~CombatController();

    // Controller Methods
    virtual bool keyPressed(int key);
    virtual bool isCombatController() const { return true; }

    // TurnCompleter Method
    virtual void finishTurn();

    // Accessor Methods
    bool isCamping()         const { return camping; }
    bool isWinOrLose()       const { return winOrLose; }
    Direction getExitDir()   const { return exitDir; }
    unsigned char getFocus() const { return focus; }
    CombatMap* getMap()      const { return map; }
    const Creature* getCreature() const { return creature; }
    PartyMemberVector* getParty()   { return &party; }
    PartyMember* getCurrentPlayer() { return party[focus]; }

    void setExitDir(Direction d) { exitDir = d; }
    void setCreature(const Creature* m) { creature = m; }

    // Methods
    virtual void begin();
    virtual void end(bool adjustKarma);

    bool creatureRangedAttack(Creature* attacker, int dir);
    void movePartyMember(MoveEvent &event);

protected:
    virtual void awardLoot();
    virtual void update(Party *party, PartyEvent &event);

    void initCreature(const Creature *m);
    void fillCreatureTable(const Creature *creature);
    void placeCreatures();
    void attack();
    void showCombatMessage(bool show = true) { showMessage = show; }

    // Properties
    CombatMap *map;
    PartyMemberVector party;
    unsigned char focus;

    const Creature *creatureTable[AREA_CREATURES];
    const Creature *creature;

    bool camping;
    bool forceStandardEncounterSize;
    bool placePartyOnMap;
    bool placeCreaturesOnMap;
    bool winOrLose;
    bool showMessage;
    Direction exitDir;

private:
    CombatController(const CombatController&);
    const CombatController &operator=(const CombatController&);

    void initDungeonRoom(int room, Direction from);
    void applyCreatureTileEffects();
    int  initialNumberOfCreatures(const Creature *creature) const;
    bool isWon() const;
    bool isLost() const;
    void moveCreatures();
    void placePartyMembers();
    bool setActivePlayer(int player);
    bool attackAt(const Coords &coords, PartyMember *attacker, int dir, int range, int distance);
    bool returnWeaponToOwner(const Coords &coords, int distance, int dir,
                             const Weapon *weapon);
};

typedef std::vector<Creature *> CreatureVector;

/**
 * CombatMap class
 */
class CombatMap : public Map {
public:
    CombatMap();

    CreatureVector getCreatures();
    PartyMemberVector getPartyMembers();
    PartyMember* partyMemberAt(Coords coords);
    Creature* creatureAt(Coords coords);

    static MapId mapForTile(const Tile *ground, const Tile *transport, Object *obj);

    // Getters
    bool isDungeonRoom() const      {return dungeonRoom;}
    bool isAltarRoom() const        {return altarRoom != VIRT_NONE;}
    bool isContextual() const       {return contextual;}
    BaseVirtue getAltarRoom() const {return altarRoom;}

    // Setters
    void setAltarRoom(BaseVirtue ar){altarRoom = ar;}
    void setDungeonRoom(bool d)     {dungeonRoom = d;}
    void setContextual(bool c)      {contextual = c;}

    // Properties
protected:
    bool dungeonRoom;
    BaseVirtue altarRoom;
    bool contextual;

public:
    Coords creature_start[AREA_CREATURES];
    Coords player_start[AREA_PLAYERS];
};

CombatMap *getCombatMap(Map *punknown = NULL);

#endif
