/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <ctime>
#include "u4.h"

#include "combat.h"

#include "annotation.h"
#include "context.h"
#include "creature.h"
#include "death.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "movement.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "weapon.h"

/**
 * Returns true if 'map' points to a Combat Map
 */ 
bool isCombatMap(Map *punknown) {
    CombatMap *ps;
    if ((ps = dynamic_cast<CombatMap*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Returns a CombatMap pointer to the map
 * passed, or a CombatMap pointer to the current map
 * if no arguments were passed.
 *
 * Returns NULL if the map provided (or current map)
 * is not a combat map.
 */
CombatMap *getCombatMap(Map *punknown) {
    Map *m = punknown ? punknown : c->location->map;
    if (!isCombatMap(m))
        return NULL;
    else return dynamic_cast<CombatMap*>(m);
}

/**
 * CombatController class implementation
 */
CombatController::CombatController() {}
CombatController::CombatController(CombatMap *m) {
    map = m;
    gameSetMap(map, true, NULL);
}
CombatController::CombatController(MapId id) {
    map = getCombatMap(mapMgr->get(id));
    gameSetMap(map, true, NULL);
}

// Accessor Methods    
bool CombatController::isCamping() const                    { return camping; }
bool CombatController::isInn() const                        { return inn; }
bool CombatController::isWinOrLose() const                  { return winOrLose; }
Direction CombatController::getExitDir() const              { return exitDir; }
unsigned char CombatController::getFocus() const            { return focus; }
CombatMap *CombatController::getMap() const                 { return map; }
Creature *CombatController::getCreature() const               { return creature; }
PartyMemberVector *CombatController::getParty()             { return &party; }
PartyMember* CombatController::getCurrentPlayer()           { return party[focus]; }

void CombatController::setExitDir(Direction d)              { exitDir = d; }
void CombatController::setInn(bool i)                       { inn = i; }
void CombatController::setCreature(Creature *m)               { creature = m; }
void CombatController::setWinOrLose(bool worl)              { winOrLose = worl; }
void CombatController::showCombatMessage(bool show)         { showMessage = show; }

/**
 * Initializes the combat controller with combat information
 */
void CombatController::init(class Creature *m) {
    int i;
    
    creature = m;    
    placeCreaturesOnMap = (m == NULL) ? false : true;
    placePartyOnMap = true;    
    winOrLose = true;
    map->setDungeonRoom(false);
    map->setAltarRoom(VIRT_NONE);
    showMessage = false;
    camping = false;
    inn = false;

    /* initialize creature info */
    for (i = 0; i < AREA_CREATURES; i++) {
        creatureTable[i] = NULL;        
    }

    for (i = 0; i < AREA_PLAYERS; i++) {
        party.push_back(NULL);
    }

    /* fill the creature table if a creature was provided to create */    
    fillCreatureTable(m);

    /* initialize focus */
    focus = 0; 
}

/**
 * Initializes information for camping
 */
void CombatController::initCamping() {
    init(NULL);
    camping = true;
}

/**
 * Initializes dungeon room combat
 */
void CombatController::initDungeonRoom(int room, Direction from) {
    int offset, i;
    init(NULL);

    ASSERT(c->location->prev->context & CTX_DUNGEON, "Error: called initDungeonRoom from non-dungeon context");
    {
        Dungeon *dng = dynamic_cast<Dungeon*>(c->location->prev->map);
        unsigned char
            *party_x = &dng->rooms[room].party_north_start_x[0], 
            *party_y = &dng->rooms[room].party_north_start_y[0];

        /* load the dungeon room properties */
        winOrLose = false;
        map->setDungeonRoom(true);
        exitDir = DIR_NONE;
        
        /* FIXME: this probably isn't right way to see if you're entering an altar room... but maybe it is */
        if ((c->location->map->id != MAP_ABYSS) && (room == 0xF)) {            
            /* figure out which dungeon room they're entering */
            if (c->location->coords.x == 3)
                map->setAltarRoom(VIRT_LOVE);
            else if (c->location->coords.x <= 2)
                map->setAltarRoom(VIRT_TRUTH);
            else map->setAltarRoom(VIRT_COURAGE);
        }        
        
        /* load in creatures and creature start coordinates */
        for (i = 0; i < AREA_CREATURES; i++) {
            if (dng->rooms[room].creature_tiles[i] > 0) {
                placeCreaturesOnMap = true;
                creatureTable[i] = ::creatures.getByTile(dng->rooms[room].creature_tiles[i]);
            }
            map->creature_start[i].x = dng->rooms[room].creature_start_x[i];
            map->creature_start[i].y = dng->rooms[room].creature_start_y[i];            
        }
        
        /* figure out party start coordinates */
        switch(from) {
        case DIR_WEST: offset = 3; break;
        case DIR_NORTH: offset = 0; break;
        case DIR_EAST: offset = 1; break;
        case DIR_SOUTH: offset = 2; break;
        case DIR_ADVANCE:
        case DIR_RETREAT:
        default: 
            ASSERT(0, "Invalid 'from' direction passed to initDungeonRoom()");
        }

        for (i = 0; i < AREA_PLAYERS; i++) {
            map->player_start[i].x = *(party_x + (offset * AREA_PLAYERS * 2) + i);
            map->player_start[i].y = *(party_y + (offset * AREA_PLAYERS * 2) + i);
        }
    }
}

/**
 * Apply tile effects to all creatures depending on what they're standing on
 */
void CombatController::applyCreatureTileEffects() {
    CreatureVector creatures = map->getCreatures();
    CreatureVector::iterator i;

    for (i = creatures.begin(); i != creatures.end(); i++) {        
        Creature *m = *i;
        TileEffect effect = map->tileAt(m->getCoords(), WITH_GROUND_OBJECTS)->getEffect();
        m->applyTileEffect(effect);
    }
}

/**
 * Begin combat
 */
void CombatController::begin() {
    int i;
    bool partyIsReadyToFight = false;    
    
    /* place party members on the map */    
    if (placePartyOnMap)        
        placePartyMembers();    

    /* place creatures on the map */
    if (placeCreaturesOnMap)
        placeCreatures();

    /* camping, make sure everyone's asleep */
    if (camping) {
        for (i = 0; i < c->party->size(); i++)
            c->party->member(i)->putToSleep();            
    }

    /* if we entered an altar room, show the name */
    if (map->isAltarRoom()) {
        screenMessage("\nThe Altar Room of %s\n", getBaseVirtueName(map->getAltarRoom()));    
        c->location->context = static_cast<LocationContext>(c->location->context | CTX_ALTAR_ROOM);
    }

    /* Use the combat key handler */
    eventHandler.pushKeyHandler(&CombatController::baseKeyHandler);
 
    /* if there are creatures around, start combat! */    
    if (showMessage && placeCreaturesOnMap && winOrLose)
        screenMessage("\n**** COMBAT ****\n\n");
    
    /* FIXME: there should be a better way to accomplish this */
    if (!camping) {
        musicPlay();
    }

    /* Set focus to the first active party member, if there is one */ 
    for (i = 0; i < AREA_PLAYERS; i++) {
        if (setActivePlayer(i)) {
            partyIsReadyToFight = true;
            break;
        }
    }    

    if (!camping && !partyIsReadyToFight)
        (*c->location->finishTurn)();
}

void CombatController::end(bool adjustKarma) {
    int i;
    Coords coords;    
    MapTile *ground;    
    
    /* need to get this here because when we exit to the parent map, all the monsters are cleared */
    bool won = isWon();
    
    gameExitToParentMap();
    musicPlay();
    
    if (winOrLose) {
        if (won) {
            if (creature) {
                coords = creature->getCoords();
                ground = c->location->map->tileAt(coords, WITHOUT_OBJECTS);

                /* FIXME: move to separate function */
                /* add a chest, if the creature leaves one */
                if (!inn && creature->leavesChest() && ground->isCreatureWalkable()) {
                    MapTile chest = Tileset::findTileByName("chest")->id;
                    c->location->map->addObject(chest, chest, coords);
                }
                /* add a ship if you just defeated a pirate ship */
                else if (creature->getTile().isPirateShip()) {
                    MapTile ship = Tileset::findTileByName("ship")->id;
                    ship.setDirection(creature->getTile().getDirection());
                    c->location->map->addObject(ship, ship, coords);
                }
            }
            
            screenMessage("\nVictory!\n");
        }
        else if (!c->party->isDead()) {
            /* minus points for fleeing from evil creatures */
            if (adjustKarma && creature && creature->isEvil()) {
                screenMessage("Battle is lost!\n");
                c->party->adjustKarma(KA_FLED_EVIL);
            }
            else if (adjustKarma && creature && creature->isGood())
                c->party->adjustKarma(KA_SPARED_GOOD);
        }
    }

    /* exiting a dungeon room */
    if (map->isDungeonRoom()) {
        screenMessage("Leave Room!\n");
        if (map->isAltarRoom()) {            
            PortalTriggerAction action = ACTION_NONE;

            /* when exiting altar rooms, you exit to other dungeons.  Here it goes... */
            switch(exitDir) {
            case DIR_NORTH: action = ACTION_EXIT_NORTH; break;
            case DIR_EAST:  action = ACTION_EXIT_EAST; break;
            case DIR_SOUTH: action = ACTION_EXIT_SOUTH; break;
            case DIR_WEST:  action = ACTION_EXIT_WEST; break;            
            case DIR_NONE:  break;
            case DIR_ADVANCE:
            case DIR_RETREAT:
            default: ASSERT(0, "Invalid exit dir %d", exitDir); break;
            }

            if (action != ACTION_NONE)
                usePortalAt(c->location, c->location->coords, action);
        }
        else screenMessage("\n");

        if (exitDir != DIR_NONE) {
            c->saveGame->orientation = exitDir;  /* face the direction exiting the room */
            (*c->location->move)(DIR_NORTH, 0);  /* advance 1 space outside of the room */
        }
    }

    /* remove the creature */
    if (creature)
        c->location->map->removeObject(creature);

    /* If we were camping and were ambushed, wake everyone up! */
    if (camping) {
        for (i = 0; i < c->party->size(); i++)
            c->party->member(i)->wakeUp();        
    }

    /* reset our combat variables */
    camping = false;
    inn = false;
    
    if (c->party->isDead())
        deathStart(0);
    else
        (*c->location->finishTurn)();
}

/**
 * Fills the combat creature table with the creatures that the party will be facing.
 * The creature table only contains *which* creatures will be encountered and
 * *where* they are placed (by position in the table).  Information like
 * hit points and creature status will be created when the creature is actually placed
 */
void CombatController::fillCreatureTable(const Creature *creature) {
    int i, j;
    
    if (creature != NULL) { 
        const Creature *baseCreature = creature, *current;
        int numCreatures = initialNumberOfCreatures(creature);

        if (baseCreature->id == PIRATE_ID)
            baseCreature = creatures.getById(ROGUE_ID);

        for (i = 0; i < numCreatures; i++) {
            current = baseCreature;

            /* find a free spot in the creature table */
            do {j = xu4_random(AREA_CREATURES) ;} while (creatureTable[j] != NULL);
            
            /* see if creature is a leader or leader's leader */
            if (creatures.getById(baseCreature->leader) != baseCreature && /* leader is a different creature */
                i != (numCreatures - 1)) { /* must have at least 1 creature of type encountered */
                
                if (xu4_random(32) == 0)       /* leader's leader */
                    current = creatures.getById(creatures.getById(baseCreature->leader)->leader);
                else if (xu4_random(8) == 0)   /* leader */
                    current = creatures.getById(baseCreature->leader);
            }

            /* place this creature in the creature table */
            creatureTable[j] = current;
        }
    }
}

/**
 * Generate the number of creatures in a group.
 */
int  CombatController::initialNumberOfCreatures(const class Creature *creature) const {
    int ncreatures;
    Map *map = c->location->prev ? c->location->prev->map : c->location->map;

    /* if in an unusual combat situation, generally we stick to normal encounter sizes,
       (such as encounters from sleeping in an inn, etc.) */
    if (camping || inn || map->isWorldMap() || (c->location->prev && c->location->prev->context & CTX_DUNGEON)) {
        ncreatures = xu4_random(8) + 1;
        
        if (ncreatures == 1) {
            if (creature && creature->encounterSize > 0)
                ncreatures = xu4_random(creature->encounterSize) + creature->encounterSize + 1;
            else
                ncreatures = 8;
        }

        while (ncreatures > 2 * c->saveGame->members) {
            ncreatures = xu4_random(16) + 1;
        }
    } else {
        if (creature && creature->id == GUARD_ID)
            ncreatures = c->saveGame->members * 2;
        else
            ncreatures = 1;
    }

    return ncreatures;
}

/**
 * Returns true if the player has won.
 */
bool CombatController::isWon() const {
    CreatureVector creatures = map->getCreatures();    
    if (creatures.size())
        return false;    
    return true;
}

/**
 * Returns true if the player has lost.
 */
bool CombatController::isLost() const {
    PartyMemberVector party = map->getPartyMembers();
    if (party.size())
        return false;
    return true;
}

/**
 * Performs all of the creature's actions
 */
void CombatController::moveCreatures() {
    Creature *m;
    CreatureVector creatures = map->getCreatures();
    CreatureVector::iterator i;
    
    for (i = creatures.begin(); i != creatures.end(); i++) {
        m = *i;
        m->act();
    }
}

/**
 * Places creatures on the map from the creature table and from the creature_start coords
 */
void CombatController::placeCreatures() {
    int i;    

    for (i = 0; i < AREA_CREATURES; i++) {        
        const Creature *m = creatureTable[i];
        if (m)
            map->addCreature(m, map->creature_start[i]);
    }
}

/**
 * Places the party members on the map
 */
void CombatController::placePartyMembers() {
    int i;
    party.clear();
    
    for (i = 0; i < c->party->size(); i++) {
        PartyMember *p = c->party->member(i);        
        p->setFocus(false); // take the focus off of everyone        

        /* don't place dead party members */
        if (p->getStatus() != STAT_DEAD) {
            /* add the party member to the map */
            p->setCoords(map->player_start[i]);
            p->setMap(map);
            map->objects.push_back(p);
            party[i] = p;
        }
    }
}

/**
 * Sets the active player for combat, showing which weapon they're weilding, etc.
 */
bool CombatController::setActivePlayer(int player) {
    PartyMember *p = party[player];
    
    if (p && !p->isDisabled()) {        
        if (party[focus])
            party[focus]->setFocus(false);        

        p->setFocus();
        focus = player;

        screenMessage("%s with %s\n\020", p->getName().c_str(), Weapon::get(p->getWeapon())->getName().c_str());        
        c->stats->highlightPlayer(focus);        
        return true;
    }
    
    return false;
}

/** 
 * Static member functions
 */
// Directional actions
bool CombatController::attackAtCoord(MapCoords coords, int distance, void *data) {
    CombatController *ct = c->combat;
    CombatMap *cm = ct->map;
    MapTile hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    PartyMember *attacker = dynamic_cast<PartyMember*>(info->obj);
    const Weapon *weapon = Weapon::get(attacker->getWeapon());
    bool wrongRange = weapon->rangeAbsolute() && (distance != info->range);
    Coords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile *groundTile;    
    Creature *creature;

    info->prev = coords;    

    hittile = weapon->getHitTile();
    misstile = weapon->getMissTile();;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        cm->annotations->remove(old, misstile);

    /* Missed */
    if (coords.x == -1 && coords.y == -1) {

        /* Check to see if the weapon is lost */
        if ((distance > 1 && weapon->loseWhenRanged()) || weapon->loseWhenUsed()) {
            if (!attacker->loseWeapon())
                screenMessage("Last One!\n");
        }

        /* show the 'miss' tile */
        attackFlash(old, misstile, 3);
        soundPlay(SOUND_MISSED, false);

        /* This goes here so messages are shown in the original order */
        screenMessage("Missed!\n");
    }
    
    /* Check to see if we might hit something */
    else {
        creature = cm->creatureAt(coords);

        /* If we haven't hit a creature, or the weapon's range is absolute
           and we're testing the wrong range, stop now! */
        if (!creature || wrongRange) {
        
            /* If the weapon is shown as it travels, show it now */
            if (weapon->showTravel()) {
                cm->annotations->add(coords, misstile, true);
                gameUpdateScreen();
        
                /* Based on attack speed setting in setting struct, make a delay for
                   the attack annotation */
                if (attackdelay > 0)
                    EventHandler::sleep(attackdelay * 2);
            }       

            return 0;
        }
    
        /* Check to see if the weapon is lost */
        if ((distance > 1 && weapon->loseWhenRanged()) || weapon->loseWhenUsed()) {
            if (!attacker->loseWeapon())
                screenMessage("Last One!\n");
        }
    
        /* Did the weapon miss? */
        if ((cm->id == 24 && !weapon->isMagic()) || /* non-magical weapon in the Abyss */
            !attacker->attackHit(creature)) { /* player naturally missed */
            screenMessage("Missed!\n");
        
            /* show the 'miss' tile */
            attackFlash(coords, misstile, 3);
            soundPlay(SOUND_MISSED, false);

        } else { /* The weapon hit! */

            /* show the 'hit' tile */
            attackFlash(coords, hittile, 3);            

            /* apply the damage to the creature */
            if (!attacker->dealDamage(creature, attacker->getDamage()))
                creature = NULL;

            /* creature is still alive and has the chance to divide - xu4 enhancement */
            if (xu4_random(2) == 0 && creature && creature->divides())
                creature->divide();
        }
    }

    /* Check to see if the weapon returns to its owner */
    if (weapon->returns())
        CombatController::returnWeaponToOwner(coords, distance, data);

    /* If the weapon leaves a tile behind, do it here! (flaming oil, etc) */
    groundTile = cm->tileAt(coords, WITHOUT_OBJECTS);
    if (!wrongRange && (weapon->leavesTile().id > 0 && groundTile->isWalkable()))
        cm->annotations->add(coords, weapon->leavesTile());    
    
    /* only end the turn if we're still in combat */
    if (c->location->finishTurn == &CombatController::finishTurn)
        (*c->location->finishTurn)();

    return true;
}

bool CombatController::rangedAttack(MapCoords coords, int distance, void *data) {
    CombatMap *cm = c->combat->map;    
    MapTile hittile, misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;    
    Coords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile *groundTile;
    TileEffect effect;
    Creature *attacker = dynamic_cast<Creature*>(info->obj),
            *target = NULL;
    
    info->prev = coords;

    hittile = attacker->getHitTile();
    misstile = attacker->getMissTile();

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        cm->annotations->remove(old, misstile);

    /* Check to see if the creature hit an opponent */
    if (coords.x != -1 && coords.y != -1) {
        target = isCreature(attacker) ? cm->partyMemberAt(coords) : cm->creatureAt(coords);

        /* If we haven't hit something valid, stop now */
        if (!target) {            
            cm->annotations->add(coords, misstile, true);            
            gameUpdateScreen();            
    
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                EventHandler::sleep(attackdelay * 2);

            return 0;
        }

        /* Get the effects of the tile the creature is using */
        effect = hittile.getEffect();
  
        /* Monster's ranged attacks never miss */

        /* show the 'hit' tile */
        attackFlash(coords, hittile, 4);        

        /* These effects happen whether or not the opponent was hit */
        switch(effect) {
        
        case EFFECT_ELECTRICITY:
            /* FIXME: are there any special effects here? */
            screenMessage("\n%s Electrified!\n", target->getName().c_str());
            attacker->dealDamage(target, attacker->getDamage());
            break;
        
        case EFFECT_POISON:
        case EFFECT_POISONFIELD:
            
            screenMessage("\n%s Poisoned!\n", target->getName().c_str());

            /* see if the player is poisoned */
            if ((xu4_random(2) == 0) && (target->getStatus() != STAT_POISONED)) {
                target->addStatus(STAT_POISONED);
                soundPlay(SOUND_PLAYERHIT, false);
            }
            else screenMessage("Failed.\n");
            break;
        
        case EFFECT_SLEEP:

            screenMessage("\n%s Slept!\n", target->getName().c_str());
            soundPlay(SOUND_PLAYERHIT, false);

            /* see if the player is put to sleep */
            if (xu4_random(2) == 0)
                target->putToSleep();
            else screenMessage("Failed.\n");
            break;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* FIXME: are there any special effects here? */            
            screenMessage("\n%s %s Hit!\n", target->getName().c_str(),
                effect == EFFECT_LAVA ? "Lava" : "Fiery");
            attacker->dealDamage(target, attacker->getDamage());
            break;
                
        default: 
            /* show the appropriate 'hit' message */
            if (hittile == Tileset::findTileByName("magic_flash")->id)
                screenMessage("\n%s Magical Hit!\n", target->getName().c_str());
            else screenMessage("\n%s Hit!\n", target->getName().c_str());
            attacker->dealDamage(target, attacker->getDamage());
            break;
        }       

    }
    else {
        soundPlay(SOUND_MISSED, false);

        /* If the creature leaves a tile behind, do it here! (lava lizard, etc) */
        groundTile = cm->tileAt(old, WITH_GROUND_OBJECTS);
        if (attacker->leavesTile() && groundTile->isWalkable())
            cm->annotations->add(old, hittile);
    }

    return true;
}

bool CombatController::returnWeaponToOwner(MapCoords coords, int distance, void *data) {
    CombatMap *cm = c->combat->map;
    Direction dir;
    int i;
    MapTile misstile;
    CoordActionInfo* info = (CoordActionInfo*)data;
    const Weapon *weapon = Weapon::get(c->combat->party[info->player]->getWeapon());
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    MapCoords new_coords = coords;
    
    misstile = weapon->getMissTile();

    /* reverse the direction of the weapon */
    dir = dirReverse(dirFromMask(info->dir));

    for (i = distance; i > 1; i--) {
        new_coords.move(dir, cm);        
        
        cm->annotations->add(new_coords, misstile, true);
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            EventHandler::sleep(attackdelay * 2);
        
        cm->annotations->remove(new_coords, misstile);
    }
    gameUpdateScreen();

    return true;
}

/**
 * Show an attack flash at x, y on the current map.
 * This is used for 'being hit' or 'being missed' 
 * by weapons, cannon fire, spells, etc.
 */
void CombatController::attackFlash(Coords coords, MapTile tile, int timeFactor) {
    int i;
    int divisor = settings.battleSpeed;
    
    c->location->map->annotations->add(coords, tile, true);
    for (i = 0; i < timeFactor; i++) {        
        /* do screen animations while we're pausing */
        if (i % divisor == 1)
            screenCycle();

        gameUpdateScreen();       
        EventHandler::sleep(eventTimerGranularity/divisor);
    }
    c->location->map->annotations->remove(coords, tile);
}

void CombatController::finishTurn(void) {
    CombatController *ct = c->combat;    
    PartyMember *player = ct->getCurrentPlayer();
    int quick;

    /* return to party overview */
    c->stats->showPartyView();    

    if (ct->isWon() && ct->winOrLose) {
        eventHandler.popKeyHandler();
        ct->end(true);
        return;
    }
    
    /* make sure the player with the focus is still in battle (hasn't fled or died) */
    if (player) {
        /* apply effects from tile player is standing on */
        player->applyEffect(c->location->map->tileAt(player->getCoords(), WITH_GROUND_OBJECTS)->getEffect());
    }

    quick = (*c->aura == AURA_QUICKNESS) && player && (xu4_random(2) == 0) ? 1 : 0;

    /* check to see if the player gets to go again (and is still alive) */
    if (!quick || player->isDisabled()){    

        do {
            c->location->map->annotations->passTurn();

            /* put a sleeping person in place of the player,
               or restore an awakened member to their original state */            
            if (player) {                
                if (player->getStatus() == STAT_SLEEPING && (xu4_random(8) == 0))
                    player->wakeUp();                

                /* remove focus from the current party member */
                player->setFocus(false);

                /* eat some food */
                c->party->adjustFood(-1);
            }

            /* put the focus on the next party member */
            ct->focus++;
            player = ct->getCurrentPlayer();

            /* move creatures and wrap around at end */
            if (ct->focus >= c->party->size()) {   
                
                /* reset the focus to the avatar and start the party's turn over again */
                ct->focus = 0;
                player = ct->getCurrentPlayer();

                gameUpdateScreen();
                EventHandler::sleep(50); /* give a slight pause in case party members are asleep for awhile */

                /* adjust moves */
                c->party->endTurn();

                /* count down our aura (if we have one) */
                c->aura->passTurn();                

                /** 
                 * ====================
                 * HANDLE CREATURE STUFF
                 * ====================
                 */
            
                /* first, move all the creatures */
                ct->moveCreatures();

                /* then, apply tile effects to creatures */
                ct->applyCreatureTileEffects();                

                /* check to see if combat is over */
                if (ct->isLost()) {                    
                    eventHandler.popKeyHandler();
                    ct->end(true);
                    return;
                }

                /* end combat immediately if the enemy has fled */
                else if (ct->isWon() && ct->winOrLose) {
                    eventHandler.popKeyHandler();
                    ct->end(true);
                    return;
                }
            }
        } while (!player || 
                  player->isDisabled() || /* dead or sleeping */                 
                 ((c->location->activePlayer >= 0) && /* active player is set */
                  !ct->party[c->location->activePlayer]->isDisabled() && /* and the active player is not disabled */
                  (ct->party[c->location->activePlayer]) && /* and the active player is still in combat */
                  (c->location->activePlayer != ct->focus)));
    }
    else c->location->map->annotations->passTurn();
    
#if 0
    if (ct->focus != 0) {
        ct->getCurrentPlayer()->act();
        ct->finishTurn();
    }
    else ct->setActivePlayer(ct->focus);
#else
    /* display info about the current player */
    ct->setActivePlayer(ct->focus);
#endif
}

/**
 * Move a party member during combat and display the appropriate messages
 */
MoveReturnValue CombatController::movePartyMember(Direction dir, int userEvent) {
    CombatController *ct = c->combat;    
    MoveReturnValue retval = ::movePartyMember(dir, userEvent);
    int i;

    /* active player left/fled combat */
    if ((retval & MOVE_EXIT_TO_PARENT) && (c->location->activePlayer == ct->focus)) {
        c->location->activePlayer = -1;
        /* assign active player to next available party member */
        for (i = 0; i < c->party->size(); i++) {
            if (ct->party[i] && !ct->party[i]->isDisabled()) {
                c->location->activePlayer = i;
                break;
            }
        }
    }

    screenMessage("%s\n", getDirectionName(dir));
    if (retval & MOVE_MUST_USE_SAME_EXIT)
        screenMessage("All must use same exit!\n");
    else if (retval & MOVE_BLOCKED)
        screenMessage("Blocked!\n");
    else if (retval & MOVE_SLOWED)
        screenMessage("Slow progress!\n"); 
    else if (ct->winOrLose && (retval & (MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE)))
        soundPlay(SOUND_FLEE);

    return retval;
}

// Key handlers
bool CombatController::baseKeyHandler(int key, void *data) {
    CombatController *ct = c->combat;
    bool valid = true;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo;
    WeaponType weapon = c->combat->getCurrentPlayer()->getWeapon();

    switch (key) {
    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:        
        (*c->location->move)(keyToDirection(key), 1);
        break;

    case U4_ESC:
        if (settings.debug) {
            eventHandler.popKeyHandler();
            ct->end(false); /* don't adjust karma */
        }
        else screenMessage("Bad command\n");        

        break;
        
    case ' ':
        screenMessage("Pass\n");
        break;

    case U4_FKEY:
        {
            extern void gameDestroyAllCreatures();

            if (settings.debug)
                gameDestroyAllCreatures();
            else valid = false;
            break;
        }

    // Change the speed of battle
    case '+':
    case '-':
    case U4_KEYPAD_ENTER:
        {
            int old_speed = settings.battleSpeed;
            if (key == '+' && ++settings.battleSpeed > MAX_BATTLE_SPEED)
                settings.battleSpeed = MAX_BATTLE_SPEED;        
            else if (key == '-' && --settings.battleSpeed == 0)
                settings.battleSpeed = 1;
            else if (key == U4_KEYPAD_ENTER)
                settings.battleSpeed = DEFAULT_BATTLE_SPEED;

            if (old_speed != settings.battleSpeed) {        
                if (settings.battleSpeed == DEFAULT_BATTLE_SPEED)
                    screenMessage("Battle Speed:\nNormal\n");
                else if (key == '+')
                    screenMessage("Battle Speed:\nUp (%d)\n", settings.battleSpeed);
                else screenMessage("Battle Speed:\nDown (%d)\n", settings.battleSpeed);
            }
            else if (settings.battleSpeed == DEFAULT_BATTLE_SPEED)
                screenMessage("Battle Speed:\nNormal\n");
        }        

        valid = false;
        break;

    case 'a':
        info = new CoordActionInfo;
        info->handleAtCoord = &CombatController::attackAtCoord;
        info->origin = ct->getCurrentPlayer()->getCoords();
        info->prev = Coords(-1, -1);        
        info->range = Weapon::get(weapon)->getRange();
        info->validDirections = MASK_DIR_ALL;
        info->obj = ct->getCurrentPlayer();
        info->player = ct->getFocus();
        info->blockedPredicate = Weapon::get(weapon)->canAttackThroughObjects() ?
            NULL :
            &MapTile::canAttackOverTile;
        info->blockBefore = 1;
        info->firstValidDistance = 0;
        
        screenMessage("Dir: ");        
        eventHandler.pushKeyHandler(KeyHandler(&CombatController::chooseWeaponDir, info));
        break;

    case 'c':
        screenMessage("Cast Spell!\n");
        gameCastForPlayer(ct->focus);
        break;

    case 'g':
        screenMessage("Get Chest!\n");
        gameGetChest(ct->focus);
        break;

    case 'l':
        if (settings.debug) {
            Coords coords = ct->getCurrentPlayer()->getCoords();
            screenMessage("\nLocation:\nx:%d\ny:%d\nz:%d\n", coords.x, coords.y, coords.z);
            screenPrompt();
            valid = false;
            break;            
        }

    case 'r':
        {
            c->stats->showWeapons();

            alphaInfo = new AlphaActionInfo;
            alphaInfo->lastValidLetter = WEAP_MAX + 'a' - 1;
            alphaInfo->handleAlpha = readyForPlayer2;
            alphaInfo->prompt = "Weapon: ";
            alphaInfo->data = reinterpret_cast<void *>(static_cast<int>(ct->getFocus()));

            screenMessage(alphaInfo->prompt.c_str());

            eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, alphaInfo));
        }
        break;

    case 't':
        if (settings.debug && ct->map->isDungeonRoom()) {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(c->location->prev->map);
            Trigger *triggers = dungeon->rooms[dungeon->currentRoom].triggers;
            int i;

            screenMessage("Triggers!\n");

            for (i = 0; i < 4; i++) {
                screenMessage("%.1d)xy tile xy xy\n", i+1);
                screenMessage("  %.1X%.1X  %.3d %.1X%.1X %.1X%.1X\n",
                    triggers[i].x, triggers[i].y,
                    triggers[i].tile,
                    triggers[i].change_x1, triggers[i].change_y1,
                    triggers[i].change_x2, triggers[i].change_y2);                
            }
            screenPrompt();
            valid = false;
            
            break;
        }

    case 'u':
        {
            extern string itemNameBuffer;
            screenMessage("Use which item:\n");
            gameGetInput(&useItem, &itemNameBuffer);

            c->stats->showItems();            

            return true;
        }

    case 'v':
        if (musicToggle())
            screenMessage("Volume On!\n");
        else
            screenMessage("Volume Off!\n");
        break;

    case 'v' + U4_ALT:
        screenMessage("XU4 %s\n", VERSION);        
        break;

    case 'z': 
        {            
            c->stats->showPlayerDetails(ct->getFocus());            

            /* reset the spell mix menu and un-highlight the current item,
               and hide reagents that you don't have */            
            gameResetSpellMixing();

            eventHandler.pushKeyHandler(&gameZtatsKeyHandler);
            screenMessage("Ztats\n");        
        }
        break;    

    case 'b':
    case 'e':
    case 'd':
    case 'f':    
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 's':    
    case 'w':
    case 'x':   
    case 'y':
        screenMessage("Not here!\n");
        break;

    case '0':        
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if (settings.enhancements && settings.enhancementsOptions.activePlayer)
            gameSetActivePlayer(key - '1');            
        else screenMessage("Bad command\n");

        break;    

    default:
        valid = false;
        break;
    }

    if (valid) {
        c->lastCommandTime = time(NULL);
        if (*eventHandler.getKeyHandler() == &CombatController::baseKeyHandler &&
            c->location->finishTurn == &CombatController::finishTurn)
            (*c->location->finishTurn)();
    }

    return valid;
}

/**
 * Key handler for choosing the range of a wepaon
 */
bool CombatController::chooseWeaponRange(int key, void *data) {
    CoordActionInfo *info = (CoordActionInfo *) data;    

    if ((key >= '0') && (key <= (info->range + '0'))) {
        info->range = key - '0';
        screenMessage("%d\n", info->range);
        gameDirectionalAction(info);

        eventHandler.popKeyHandler();
        //eventHandler.popKeyHandlerData();

        return true;
    }
    
    return false;
}

/**
 * Key handler for choosing an attack direction
 */
bool CombatController::chooseWeaponDir(int key, void *data) {
    CombatController *ct = c->combat;
    CoordActionInfo *info = (CoordActionInfo *)data;
    Direction dir = keyToDirection(key);
    bool valid = (dir != DIR_NONE) ? true : false;
    WeaponType weapon = ct->party[info->player]->getWeapon();

    eventHandler.popKeyHandler();
    info->dir = MASK_DIR(dir);

    if (valid) {
        screenMessage("%s\n", getDirectionName(dir));
        if (Weapon::get(weapon)->canChooseDistance()) {
            screenMessage("Range: ");
            eventHandler.pushKeyHandler(KeyHandler(&CombatController::chooseWeaponRange, info));
        }
        else gameDirectionalAction(info);        
    }

    //eventHandler.popKeyHandlerData();
    
    return valid || KeyHandler::defaultHandler(key, NULL);
}

/**
 * CombatMap class implementation
 */ 
CombatMap::CombatMap() : Map() {}

/**
 * Returns a vector containing all of the creatures on the map
 */ 
CreatureVector CombatMap::getCreatures() {
    ObjectDeque::iterator i;
    CreatureVector creatures;
    for (i = objects.begin(); i != objects.end(); i++) {
        if (isCreature(*i) && !isPartyMember(*i))
            creatures.push_back(dynamic_cast<Creature*>(*i));
    }
    return creatures;
}

/**
 * Returns a vector containing all of the party members on the map
 */ 
PartyMemberVector CombatMap::getPartyMembers() {
    ObjectDeque::iterator i;
    PartyMemberVector party;
    for (i = objects.begin(); i != objects.end(); i++) {
        if (isPartyMember(*i))
            party.push_back(dynamic_cast<PartyMember*>(*i));
    }
    return party;
}

/**
 * Returns the party member at the given coords, if there is one,
 * NULL if otherwise.
 */ 
PartyMember *CombatMap::partyMemberAt(Coords coords) {
    PartyMemberVector party = getPartyMembers();
    PartyMemberVector::iterator i;
    
    for (i = party.begin(); i != party.end(); i++) {
        if ((*i)->getCoords() == coords)
            return *i;
    }
    return NULL;
}

/**
 * Returns the creature at the given coords, if there is one,
 * NULL if otherwise.
 */ 
Creature *CombatMap::creatureAt(Coords coords) {
    CreatureVector creatures = getCreatures();
    CreatureVector::iterator i;

    for (i = creatures.begin(); i != creatures.end(); i++) {
        if ((*i)->getCoords() == coords)            
            return *i;
    }
    return NULL;
}

/**
 * Returns a valid combat map given the provided information
 */ 
MapId CombatMap::mapForTile(MapTile groundTile, MapTile transport, Object *obj) {
    int fromShip = 0,
        toShip = 0;
    Object *objUnder = c->location->map->objectAt(c->location->coords);

    static std::map<MapTile, MapId> tileMap;
    if (!tileMap.size()) {        
        tileMap[Tileset::findTileByName("horse")->id] = MAP_GRASS_CON;        
        tileMap[Tileset::findTileByName("swamp")->id] = MAP_MARSH_CON;
        tileMap[Tileset::findTileByName("grass")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("brush")->id] = MAP_BRUSH_CON;
        tileMap[Tileset::findTileByName("forest")->id] = MAP_FOREST_CON;
        tileMap[Tileset::findTileByName("hills")->id] = MAP_HILL_CON;
        tileMap[Tileset::findTileByName("dungeon")->id] = MAP_HILL_CON;
        tileMap[Tileset::findTileByName("city")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("castle")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("town")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("lcb_entrance")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("bridge")->id] = MAP_BRIDGE_CON;
        tileMap[Tileset::findTileByName("balloon")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("bridge_pieces")->id] = MAP_BRIDGE_CON;        
        tileMap[Tileset::findTileByName("shrine")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("chest")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("brick_floor")->id] = MAP_BRICK_CON;
        tileMap[Tileset::findTileByName("moongate")->id] = MAP_GRASS_CON;
        tileMap[Tileset::findTileByName("moongate_opening")->id] = MAP_GRASS_CON;        
    }

    if (transport.isShip() || (objUnder && objUnder->getTile().isShip()))
        fromShip = 1;
    if (obj->getTile().isPirateShip())
        toShip = 1;

    if (fromShip && toShip)
        return MAP_SHIPSHIP_CON;

    /* We can fight creatures and townsfolk */       
    if (obj->getType() != OBJECT_UNKNOWN) {
        MapTile *tileUnderneath = c->location->map->tileAt(obj->getCoords(), WITHOUT_OBJECTS);

        if (toShip)
            return MAP_SHORSHIP_CON;
        else if (fromShip && tileUnderneath->isWater())
            return MAP_SHIPSEA_CON;
        else if (tileUnderneath->isWater())
            return MAP_SHORE_CON;
        else if (fromShip && !tileUnderneath->isWater())
            return MAP_SHIPSHOR_CON;        
    }

    if (tileMap.find(groundTile) != tileMap.end())
        return tileMap[groundTile];    

    return MAP_BRICK_CON;
}

bool CombatMap::isDungeonRoom() const {
    return dungeonRoom;
}

bool CombatMap::isAltarRoom() const {
    return (altarRoom != VIRT_NONE) ? true : false;
}

BaseVirtue CombatMap::getAltarRoom() const {
    return altarRoom;
}

void CombatMap::setAltarRoom(BaseVirtue ar) {
    altarRoom = ar;
}

void CombatMap::setDungeonRoom(bool d) {
    dungeonRoom = d;
}
