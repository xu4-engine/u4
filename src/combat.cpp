/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <ctime>
#include "u4.h"

#include "combat.h"

#include "annotation.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "dungeon.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "monster.h"
#include "movement.h"
#include "names.h"
#include "object.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "tile.h"
#include "utils.h"
#include "weapon.h"

bool combatAttackAtCoord(MapCoords coords, int distance, void *data);
bool combatMonsterRangedAttack(MapCoords coords, int distance, void *data);
bool combatReturnWeaponToOwner(MapCoords coords, int distance, void *data);

/**
 * Key handlers
 */ 
bool combatChooseWeaponRange(int key, void *data);
bool combatChooseWeaponDir(int key, void *data);

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
 * CombatMap class implementation
 */ 
CombatMap::CombatMap() : Map() {}
CombatMap::CombatMap(MapId id) : Map(id) {}

/**
 * Initializes the Combat Map with combat information
 */
void CombatMap::init(Monster *m) {
    int i;
    
    monster = m;	
    placeMonstersOnMap = (m == NULL) ? false : true;
	placePartyOnMap = true;	
    winOrLose = true;
    dungeonRoom = false;
    altarRoom = VIRT_NONE;
    showCombatMessage = false;
    camping = false;
    inn = false;

    /* initialize monster info */
    for (i = 0; i < AREA_MONSTERS; i++)       
        monsterTable[i] = NULL;    

    /* fill the monster table if a monster was provided to create */    
    fillMonsterTable(m);

    /* initialize focus */
    focus = 0;    
}

/**
 * Initializes information for camping
 */
void CombatMap::initCamping(void) {
    init(NULL);
    camping = true;    
}

/**
 * Initializes dungeon room combat
 */
void CombatMap::initDungeonRoom(int room, Direction from) {
    int offset, i;
    init(NULL);

    ASSERT(c->location->prev->context & CTX_DUNGEON, "Error: called combatInitDungeonRoom from non-dungeon context");
    {
        Dungeon *dng = dynamic_cast<Dungeon*>(c->location->prev->map);
        unsigned char
            *party_x = &dng->rooms[room].party_north_start_x[0], 
            *party_y = &dng->rooms[room].party_north_start_y[0];

        /* load the dungeon room properties */
        winOrLose = false;
        dungeonRoom = true;
        exitDir = DIR_NONE;
        
        /* FIXME: this probably isn't right way to see if you're entering an altar room... but maybe it is */
        if ((c->location->map->id != MAP_ABYSS) && (room == 0xF)) {            
            /* figure out which dungeon room they're entering */
            if (c->location->coords.x == 3)
                altarRoom = VIRT_LOVE;
            else if (c->location->coords.x <= 2)
                altarRoom = VIRT_TRUTH;
            else altarRoom = VIRT_COURAGE;            
        }        
        
        /* load in monsters and monster start coordinates */
        for (i = 0; i < AREA_MONSTERS; i++) {
            if (dng->rooms[room].monster_tiles[i] > 0) {
				placeMonstersOnMap = true;
                monsterTable[i] = ::monsters.getByTile(dng->rooms[room].monster_tiles[i]);
			}
            monster_start[i].x = dng->rooms[room].monster_start_x[i];
            monster_start[i].y = dng->rooms[room].monster_start_y[i];            
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
            ASSERT(0, "Invalid 'from' direction passed to combatInitDungeonRoom()");
        }

        for (i = 0; i < AREA_PLAYERS; i++) {
            player_start[i].x = *(party_x + (offset * AREA_PLAYERS * 2) + i);
            player_start[i].y = *(party_y + (offset * AREA_PLAYERS * 2) + i);
        }
    }
}

/**
 * Begin combat
 */
void CombatMap::begin() {
    int i;
    bool partyIsReadyToFight = false;    
    
    /* place party members on the map */	
	if (placePartyOnMap)        
        placePartyMembers();    

    /* place monsters on the map */
    if (placeMonstersOnMap)
        placeMonsters();

    /* camping, make sure everyone's asleep */
    if (camping) {
        for (i = 0; i < c->saveGame->members; i++)
            putPlayerToSleep(i);        
    }

    /* if we entered an altar room, show the name */
    if (altarRoom) {
        screenMessage("\nThe Altar Room of %s\n", getBaseVirtueName(altarRoom));    
        c->location->context = static_cast<LocationContext>(c->location->context | CTX_ALTAR_ROOM);
    }

    /* Use the combat key handler */
    eventHandlerPushKeyHandler(&combatBaseKeyHandler);
 
    /* if there are monsters around, start combat! */    
    if (showCombatMessage && placeMonstersOnMap && winOrLose)
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

/**
 * Sets the active player for combat, showing which weapon they're weilding, etc.
 */
bool CombatMap::setActivePlayer(int player) {    
    CombatObjectMap::iterator i = party.find(player);
    
    if (i != party.end() && !playerIsDisabled(player)) {        
        Monster *p = i->second;
        i = party.find(focus);
        if (i != party.end())
            i->second->setFocus(false);

        p->setFocus();
        focus = player;

        screenMessage("%s with %s\n\020", c->players[focus].name, Weapon::get(c->players[focus].weapon)->getName().c_str());
        statsUpdate(); /* If a character was awakened inbetween world view and combat, this fixes stats info */
        statsHighlightCharacter(focus);
        return true;
    }
    
    return false;
}

/**
 * Puts player 'player' to sleep in combat
 */
bool CombatMap::putPlayerToSleep(int player) {    
    CombatObjectMap::iterator i = party.find(player);
    
    if (i != party.end() && !playerIsDisabled(player)) {
        Monster *p = i->second;
        p->status = STAT_SLEEPING;
        p->setTile(CORPSE_TILE);
        return true;
    }
    return false;
}

bool CombatMap::addMonster(const Monster *m, Coords coords) {
    int i;
    if (m != NULL) { 
        for (i = 0; i < AREA_MONSTERS; i++) {
            /* find a free spot to place the monster */
            if (monsters.find(i) == monsters.end()) {            
                /* place the monster! */
                monsters[i] = c->location->map->addMonster(m, coords);
                return true;
            }
        }
    }
    return false;
}

/**
 * Fills the combat monster table with the monsters that the party will be facing.
 * The monster table only contains *which* monsters will be encountered and
 * *where* they are placed (by position in the table).  Information like
 * hit points and monster status will be created when the monster is actually placed
 */
void CombatMap::fillMonsterTable(const Monster *monster) {
    int i, j;
    
    if (monster != NULL) { 
        const Monster *baseMonster = monster, *current;
        int numMonsters = initialNumberOfMonsters(monster);

        if (baseMonster->id == PIRATE_ID)
            baseMonster = ::monsters.getById(ROGUE_ID);

        for (i = 0; i < numMonsters; i++) {
            current = baseMonster;

            /* find a free spot in the monster table */
            do {j = xu4_random(AREA_MONSTERS) ;} while (monsterTable[j] != NULL);
            
            /* see if monster is a leader or leader's leader */
            if (::monsters.getById(baseMonster->leader) != baseMonster && /* leader is a different monster */
                i != (numMonsters - 1)) { /* must have at least 1 monster of type encountered */
                
                if (xu4_random(32) == 0)       /* leader's leader */
                    current = ::monsters.getById(::monsters.getById(baseMonster->leader)->leader);
                else if (xu4_random(8) == 0)   /* leader */
                    current = ::monsters.getById(baseMonster->leader);
            }

            /* place this monster in the monster table */
            monsterTable[j] = current;
        }
    }
}

/**
 * Places the party members on the map
 */
void CombatMap::placePartyMembers(void) {
    int i;
    party.clear();
    
	for (i = 0; i < c->saveGame->members; i++) {        
        StatusType playerStatus = c->players[i].status;
        MapTile playerTile = tileForClass(c->players[i].klass);

        /* don't place dead party members */
        if (playerStatus != STAT_DEAD) {
            Monster *m;
            
            /* add the party member to the map */
            party[i] = Map::addMonster(::monsters.getByTile(playerTile), player_start[i]);			
            m = party[i];

            /* change the tile for the object to a sleeping person if necessary */
            if (playerStatus == STAT_SLEEPING)
                party[i]->setTile(CORPSE_TILE);
        }
    }
}

/**
 * Places monsters on the map from the monster table and from monsterStart_x and monsterStart_y
 */
void CombatMap::placeMonsters(void) {
    int i;
    monsters.clear();

    for (i = 0; i < AREA_MONSTERS; i++) {        
        const Monster *m = monsterTable[i];
        if (m)
            addMonster(m, monster_start[i]);
    }    
}

int CombatMap::partyMemberAt(Coords coords) {
    int i;    

    for (i = 0; i < AREA_PLAYERS; i++) {
        if ((party.find(i) != party.end()) && (party[i]->getCoords() == coords))
            return i;       
    }
    return -1;
}

int CombatMap::monsterAt(Coords coords) {
    int i;    

    for (i = 0; i < AREA_MONSTERS; i++) {
        if ((monsters.find(i) != monsters.end()) && (monsters[i]->getCoords() == coords))        
            return i;
    }
    return -1;
}

MapId CombatMap::mapForTile(MapTile groundTile, MapTile transport, Object *obj) {
    unsigned int i;
    int fromShip = 0,
        toShip = 0;
    Object *objUnder = c->location->map->objectAt(c->location->coords);

    static const struct {
        MapTile tile;
        MapId mapid;
    } tileToMap[] = {
        { HORSE1_TILE,  MAP_GRASS_CON },
        { HORSE2_TILE,  MAP_GRASS_CON },
        { SWAMP_TILE,   MAP_MARSH_CON },
        { GRASS_TILE,   MAP_GRASS_CON },
        { BRUSH_TILE,   MAP_BRUSH_CON },
        { FOREST_TILE,  MAP_FOREST_CON },
        { HILLS_TILE,   MAP_HILL_CON },
        { DUNGEON_TILE, MAP_HILL_CON },
        { CITY_TILE,    MAP_GRASS_CON },
        { CASTLE_TILE,  MAP_GRASS_CON },
        { TOWN_TILE,    MAP_GRASS_CON },
        { LCB2_TILE,    MAP_GRASS_CON },
        { BRIDGE_TILE,  MAP_BRIDGE_CON },
        { BALLOON_TILE, MAP_GRASS_CON },
        { NORTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SOUTHBRIDGE_TILE, MAP_BRIDGE_CON },
        { SHRINE_TILE,  MAP_GRASS_CON },
        { CHEST_TILE,   MAP_GRASS_CON },
        { BRICKFLOOR_TILE, MAP_BRICK_CON },
        { MOONGATE0_TILE, MAP_GRASS_CON },
        { MOONGATE1_TILE, MAP_GRASS_CON },
        { MOONGATE2_TILE, MAP_GRASS_CON },
        { MOONGATE3_TILE, MAP_GRASS_CON }
    };

    if (tileIsShip(transport) || (objUnder && tileIsShip(objUnder->getTile())))
        fromShip = 1;
    if (tileIsPirateShip(obj->getTile()))
        toShip = 1;

    if (fromShip && toShip)
        return MAP_SHIPSHIP_CON;

    /* We can fight monsters and townsfolk */       
    if (obj->getType() != OBJECT_UNKNOWN) {
        MapTile tileUnderneath = c->location->map->tileAt(obj->getCoords(), WITHOUT_OBJECTS);

        if (toShip)
            return MAP_SHORSHIP_CON;
        else if (fromShip && tileIsWater(tileUnderneath))
            return MAP_SHIPSEA_CON;
        else if (tileIsWater(tileUnderneath))
            return MAP_SHORE_CON;
        else if (fromShip && !tileIsWater(tileUnderneath))
            return MAP_SHIPSHOR_CON;        
    }

    for (i = 0; i < sizeof(tileToMap) / sizeof(tileToMap[0]); i++) {
        if (tileToMap[i].tile == groundTile)
            return tileToMap[i].mapid;
    }    

    return MAP_BRICK_CON;
}

void combatFinishTurn() {
    CombatMap *cm = getCombatMap();
    CombatObjectMap *party = &cm->party;
    int quick;

    /* return to party overview */
    c->statsView = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (cm->isWon() && cm->winOrLose) {
        eventHandlerPopKeyHandler();
        cm->end(true);
        return;
    }
    
    /* make sure the player with the focus is still in battle (hasn't fled or died) */
    if (party->find(cm->focus) != party->end()) {
        /* apply effects from tile player is standing on */
        playerApplyEffect(tileGetEffect(c->location->map->tileAt((*party)[cm->focus]->getCoords(), WITH_GROUND_OBJECTS)), cm->focus);
    }

    quick = (c->aura == AURA_QUICKNESS) && (party->find(cm->focus) != party->end()) && (xu4_random(2) == 0) ? 1 : 0;

    /* check to see if the player gets to go again (and is still alive) */
    if (!quick || (playerIsDisabled(cm->focus))){    

        do {            
            c->location->map->annotations->passTurn();

            /* put a sleeping person in place of the player,
               or restore an awakened member to their original state */            
            if (party->find(cm->focus) != party->end()) {
                /* FIXME: move this to its own function, probably combatTryToWakeUp() or something similar */
                /* wake up! */
                if ((*party)[cm->focus]->status == STAT_SLEEPING && (xu4_random(8) == 0)) {
                    (*party)[cm->focus]->status = STAT_GOOD;
                    statsUpdate();
                }

                /* display a sleeping person or an awake person */                
                if ((*party)[cm->focus]->status == STAT_SLEEPING)
                    (*party)[cm->focus]->setTile(CORPSE_TILE);
                else (*party)[cm->focus]->setTile(tileForClass(c->players[cm->focus].klass));

                /* remove focus from the current party member */
                (*party)[cm->focus]->setFocus(false);

                /* eat some food */
                playerAdjustFood(-1);
            }

            /* put the focus on the next party member */
            cm->focus++;           

            /* move monsters and wrap around at end */
            if (cm->focus >= c->saveGame->members) {   
                
                /* reset the focus to the avatar and start the party's turn over again */
                cm->focus = 0;

                gameUpdateScreen();
                eventHandlerSleep(50); /* give a slight pause in case party members are asleep for awhile */

                /* adjust moves */
                playerEndTurn();

                /* check if aura has expired */
                if (c->auraDuration > 0) {
                    if (--c->auraDuration == 0)
                        c->aura = AURA_NONE;
                }                

                /** 
                 * ====================
                 * HANDLE MONSTER STUFF
                 * ====================
                 */
            
                /* first, move all the monsters */
                cm->moveMonsters();

                /* then, apply tile effects to monsters */
                cm->applyMonsterTileEffects();                

                /* check to see if combat is over */
                if (cm->isLost()) {										
					printf("\n\n%d is the number of party members in battle\n\n", cm->party.size());
                    eventHandlerPopKeyHandler();
                    cm->end(true);
                    return;
                }

                /* end combat immediately if the enemy has fled */
                else if (cm->isWon() && cm->winOrLose) {
                    eventHandlerPopKeyHandler();
                    cm->end(true);
                    return;
                }
            }
        } while (party->find(cm->focus) == party->end() ||    /* dead */
                 (c->players[cm->focus].status == STAT_SLEEPING) || /* sleeping */
                 ((c->location->activePlayer >= 0) && /* active player is set */
                  !playerIsDisabled(c->location->activePlayer) && /* and the active player is not disabled */
                  (party->find(c->location->activePlayer) != party->end()) && /* and the active player is still in combat */
                  (c->location->activePlayer != cm->focus)));
    }
    else c->location->map->annotations->passTurn();

    /* FIXME: there is probably a cleaner way to do this:
       make sure the active player is back to their normal self before acting */
    (*party)[cm->focus]->setTile(tileForClass(c->players[cm->focus].klass));
    cm->setActivePlayer(cm->focus);    
}

bool combatBaseKeyHandler(int key, void *data) {
    CombatMap *cm = getCombatMap();
    bool valid = true;
    CoordActionInfo *info;
    AlphaActionInfo *alphaInfo;
    WeaponType weapon = c->players[cm->focus].weapon;    

    switch (key) {
    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:        
        (*c->location->move)(keyToDirection(key), 1);
        break;

    case U4_ESC:
        if (settings.debug) {
            eventHandlerPopKeyHandler();
            cm->end(false); /* don't adjust karma */
        }
        else screenMessage("Bad command\n");        

        break;
        
    case ' ':
        screenMessage("Pass\n");
        break;

    case U4_FKEY:
        {
            extern void gameDestroyAllMonsters();

            if (settings.debug)
                gameDestroyAllMonsters();
            else valid = false;
            break;
        }

    case 'a':
        info = new CoordActionInfo;
        info->handleAtCoord = &combatAttackAtCoord;
        info->origin = cm->party[cm->focus]->getCoords();
        info->prev = Coords(-1, -1);        
        info->range = Weapon::get(weapon)->getRange();
        info->validDirections = MASK_DIR_ALL;
        info->player = cm->focus;
        info->blockedPredicate = Weapon::get(weapon)->canAttackThroughObjects() ?
            NULL :
            &tileCanAttackOver;
        info->blockBefore = 1;
        info->firstValidDistance = 0;
        
        eventHandlerPushKeyHandlerWithData(&combatChooseWeaponDir, info);        

        screenMessage("Dir: ");
        break;

    case 'c':
        screenMessage("Cast Spell!\n");
        gameCastForPlayer(cm->focus);
        break;

    case 'g':
        screenMessage("Get Chest!\n");
        gameGetChest(cm->focus);
        break;

    case 'l':
        if (settings.debug) {
            Coords coords = cm->party[cm->focus]->getCoords();
            screenMessage("\nLocation:\nx:%d\ny:%d\nz:%d\n", coords.x, coords.y, coords.z);
            screenPrompt();
            valid = false;
            break;            
        }

    case 'r':
        {
            c->statsView = STATS_WEAPONS;
            statsUpdate();

            alphaInfo = new AlphaActionInfo;
            alphaInfo->lastValidLetter = WEAP_MAX + 'a' - 1;
            alphaInfo->handleAlpha = readyForPlayer2;
            alphaInfo->prompt = "Weapon: ";
            alphaInfo->data = reinterpret_cast<void *>(static_cast<int>(cm->focus));

            screenMessage(alphaInfo->prompt.c_str());

            eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, alphaInfo);
        }
        break;

    case 't':
        if (settings.debug && cm->dungeonRoom) {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(c->location->prev->map);
            Trigger *triggers = dungeon->currentRoom->triggers;
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

            c->statsView = STATS_ITEMS;
            statsUpdate();

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
            c->statsView = static_cast<StatsView>(STATS_CHAR1 + cm->focus);
            statsUpdate();

            /* reset the spell mix menu and un-highlight the current item,
               and hide reagents that you don't have */            
            gameResetSpellMixing();

            eventHandlerPushKeyHandler(&gameZtatsKeyHandler);
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
        if (settings.enhancements && settings.enhancementsOptions.activePlayer) {
            if (key == '0') {             
                c->location->activePlayer = -1;
                screenMessage("Set Active Player: None!\n");
            }
            else if (key-'1' < c->saveGame->members) {
                c->location->activePlayer = key - '1';
                screenMessage("Set Active Player: %s!\n", c->players[c->location->activePlayer].name);
            }
        }
        else screenMessage("Bad command\n");

        break;    

    default:
        valid = false;
        break;
    }

    if (valid) {
        c->lastCommandTime = time(NULL);
        if (eventHandlerGetKeyHandler() == &combatBaseKeyHandler &&
            c->location->finishTurn == &combatFinishTurn)
            (*c->location->finishTurn)();
    }

    return valid;
}

bool combatAttackAtCoord(MapCoords coords, int distance, void *data) {
    CombatMap *cm = getCombatMap();
    int monster;
    MapTile hittile, misstile;
    CoordActionInfo* info = static_cast<CoordActionInfo *>(data);
    const Weapon *weapon = Weapon::get(c->players[info->player].weapon);
    int wrongRange = weapon->rangeAbsolute() && (distance != info->range);
    Coords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile groundTile;

    info->prev = coords;    

    hittile = weapon->getHitTile();
    misstile = weapon->getMissTile();

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        cm->annotations->remove(old, misstile);

    /* Missed */
    if (coords.x == -1 && coords.y == -1) {

        /* Check to see if the weapon is lost */
        if ((distance > 1 && weapon->loseWhenRanged()) || weapon->loseWhenUsed()) {
            if (!playerLoseWeapon(info->player))
                screenMessage("Last One!\n");
        }

        /* show the 'miss' tile */
        attackFlash(old, misstile, 3);

        /* This goes here so messages are shown in the original order */
        screenMessage("Missed!\n");
    }
    
    /* Check to see if we might hit something */
    else {
        monster = cm->monsterAt(coords);

        /* If we haven't hit a monster, or the weapon's range is absolute
           and we're testing the wrong range, stop now! */
        if (monster == -1 || wrongRange) {
        
            /* If the weapon is shown as it travels, show it now */
            if (weapon->showTravel()) {
                cm->annotations->add(coords, misstile, true);
                gameUpdateScreen();
        
                /* Based on attack speed setting in setting struct, make a delay for
                   the attack annotation */
                if (attackdelay > 0)
                    eventHandlerSleep(attackdelay * 2);
            }       

            return 0;
        }
    
        /* Check to see if the weapon is lost */
        if ((distance > 1 && weapon->loseWhenRanged()) || weapon->loseWhenUsed()) {
            if (!playerLoseWeapon(info->player))
                screenMessage("Last One!\n");
        }
    
        /* Did the weapon miss? */
        if ((cm->id == 24 && !weapon->isMagic()) ||        /* non-magical weapon in the Abyss */
            !playerAttackHit(&c->players[cm->focus])) {         /* player naturally missed */
            screenMessage("Missed!\n");
        
            /* show the 'miss' tile */
            attackFlash(coords, misstile, 3);

        } else { /* The weapon hit! */

            /* show the 'hit' tile */
            attackFlash(coords, hittile, 3);

            /* apply the damage to the monster */
            cm->applyDamageToMonster(monster, playerGetDamage(&c->players[cm->focus]), cm->focus);

            /* monster is still alive and has the chance to divide - xu4 enhancement */
            if (xu4_random(2) == 0 && (cm->monsters.find(monster) != cm->monsters.end()) && cm->monsters[monster]->divides())
                cm->divideMonster(cm->monsters[monster]);
        }
    }

    /* Check to see if the weapon returns to its owner */
    if (weapon->returns())
        combatReturnWeaponToOwner(coords, distance, data);

    /* If the weapon leaves a tile behind, do it here! (flaming oil, etc) */
    groundTile = cm->tileAt(coords, WITHOUT_OBJECTS);
    if (!wrongRange && (weapon->leavesTile() && tileIsWalkable(groundTile)))
        cm->annotations->add(coords, weapon->leavesTile());    
    
    /* only end the turn if we're still in combat */
    if (c->location->finishTurn == &combatFinishTurn)
        (*c->location->finishTurn)();

    return true;
}

bool combatMonsterRangedAttack(MapCoords coords, int distance, void *data) {
    CombatMap *cm = getCombatMap();
    int player;
    Monster *m;
    MapTile hittile, misstile;
    CoordActionInfo* info = static_cast<CoordActionInfo*>(data);
    Coords old = info->prev;    
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;    
    MapTile groundTile;
    TileEffect effect;
    
    info->prev = coords;

    hittile = cm->monsters[info->player]->rangedhittile;
    misstile = cm->monsters[info->player]->rangedmisstile;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        cm->annotations->remove(old, misstile);

    /* Check to see if the monster hit a party member */
    if (coords.x != -1 && coords.y != -1) {   

        player = cm->partyMemberAt(coords);

        /* If we haven't hit a player, stop now */
        if (player == -1) {
            cm->annotations->add(coords, misstile, true);
            gameUpdateScreen();
    
            /* Based on attack speed setting in setting struct, make a delay for
               the attack annotation */
            if (attackdelay > 0)
                eventHandlerSleep(attackdelay * 2);

            return 0;
        }

        /* Get the effects of the tile the monster is using */
        effect = tileGetEffect(hittile);
  
        /* Did the weapon miss? */
        if (!playerIsHitByAttack(&c->players[player])) {
        
            /* show the 'miss' tile */
            attackFlash(coords, misstile, 4);

        } else { /* The weapon hit! */                   

            /* show the 'hit' tile */
            attackFlash(coords, hittile, 4);             

            /* FIXME: Will this ever be used? */

            /* These effects require the player to be hit to affect the player */
            /*switch(effect) {
            } */
        }

        m = cm->monsters[cm->monsterAt(info->origin)];

        /* These effects happen whether or not the player was hit */
        switch(effect) {
        
        case EFFECT_ELECTRICITY:
            /* FIXME: are there any special effects here? */
            screenMessage("\n%s Electrified!\n", c->players[player].name);
            cm->applyDamageToPlayer(player, m->getDamage());
            break;
        
        case EFFECT_POISON:
        case EFFECT_POISONFIELD:
            
            screenMessage("\n%s Poisoned!\n", c->players[player].name);

            /* see if the player is poisoned */
            if ((xu4_random(2) == 0) && (c->players[player].status != STAT_POISONED))
                c->players[player].status = STAT_POISONED;
            else screenMessage("Failed.\n");
            break;
        
        case EFFECT_SLEEP:

            screenMessage("\n%s Slept!\n", c->players[player].name);

            /* see if the player is put to sleep */
            if (xu4_random(2) == 0)
                cm->putPlayerToSleep(player);            
            else screenMessage("Failed.\n");
            break;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* FIXME: are there any special effects here? */            
            screenMessage("\n%s %s Hit!\n", c->players[player].name,
                effect == EFFECT_LAVA ? "Lava" : "Fiery");
            cm->applyDamageToPlayer(player, m->getDamage());
            break;
                
        default: 
            /* show the appropriate 'hit' message */
            if (hittile == MAGICFLASH_TILE)
                screenMessage("\n%s Magical Hit!\n", c->players[player].name);
            else screenMessage("\n%s Hit!\n", c->players[player].name);
            cm->applyDamageToPlayer(player, m->getDamage());
            break;
        }       

    }
    else {
        m = cm->monsters[cm->monsterAt(info->origin)];

        /* If the monster leaves a tile behind, do it here! (lava lizard, etc) */
        groundTile = cm->tileAt(old, WITH_GROUND_OBJECTS);
        if (m->leavesTile() && tileIsWalkable(groundTile))
            cm->annotations->add(old, hittile);
    }

    return true;
}


bool combatReturnWeaponToOwner(MapCoords coords, int distance, void *data) {
    CombatMap *cm = getCombatMap();
    Direction dir;
    int i;
    MapTile misstile;
    CoordActionInfo* info = static_cast<CoordActionInfo*>(data);
    const Weapon *weapon = Weapon::get(c->players[info->player].weapon);
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
            eventHandlerSleep(attackdelay * 2);
        
        cm->annotations->remove(new_coords, misstile);
    }
    gameUpdateScreen();

    return true;
}

/**
 * Generate the number of monsters in a group.
 */
int CombatMap::initialNumberOfMonsters(const Monster *monster) {
    int nmonsters;

    /* if in an unusual combat situation, generally we stick to normal encounter sizes,
       (such as encounters from sleeping in an inn, etc.) */
    if (camping || inn || isWorldMap() || (c->location->context & CTX_DUNGEON)) {
        nmonsters = xu4_random(8) + 1;
        
        if (nmonsters == 1) {
            if (monster && monster->encounterSize > 0)
                nmonsters = xu4_random(monster->encounterSize) + monster->encounterSize + 1;
            else
                nmonsters = 8;
        }

        while (nmonsters > 2 * c->saveGame->members) {
            nmonsters = xu4_random(16) + 1;
        }
    } else {
        if (monster && monster->id == GUARD_ID)
            nmonsters = c->saveGame->members * 2;
        else
            nmonsters = 1;
    }

    return nmonsters;
}

/**
 * Returns true if the player has won.
 */
int CombatMap::isWon() {
    return monsters.size() > 0 ? false : true;    
}

/**
 * Returns true if the player has lost.
 */
int CombatMap::isLost() {
    return party.size() > 0 ? false : true;
}

void CombatMap::end(bool adjustKarma) {
    int i;
    Coords coords;    
    MapTile ground;
    
    gameExitToParentMap();
    musicPlay();    
    
    if (winOrLose) {
        if (isWon()) {
            if (monster) {
                coords = monster->getCoords();
                ground = tileAt(coords, WITHOUT_OBJECTS);

                /* FIXME: move to separate function */
                /* add a chest, if the monster leaves one */
                if (monster->leavesChest() && 
                    tileIsMonsterWalkable(ground) && tileIsWalkable(ground)) {                    
                    c->location->map->addObject(tileGetChestBase(), tileGetChestBase(), coords);
                }
                /* add a ship if you just defeated a pirate ship */
                else if (tileIsPirateShip(monster->getTile())) {
                    MapTile ship = tileGetShipBase();
                    tileSetDirection(&ship, tileGetDirection(monster->getTile()));
                    c->location->map->addObject(ship, ship, coords);
                }
            }

            screenMessage("\nVictory!\n");
        }
        else if (!playerPartyDead()) {
            /* minus points for fleeing from evil creatures */
            if (adjustKarma && monster && monster->isEvil()) {
                screenMessage("Battle is lost!\n");
                playerAdjustKarma(KA_FLED_EVIL);
            }
            else if (adjustKarma && monster && monster->isGood())
                playerAdjustKarma(KA_SPARED_GOOD);
        }
    }

    /* exiting a dungeon room */
    if (dungeonRoom) {
        screenMessage("Leave Room!\n");
        if (altarRoom) {            
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

    /* remove the monster */
    if (monster)
        c->location->map->removeObject(monster);

    /* If we were camping and were ambushed, wake everyone up! */
    if (camping) {
        for (i = 0; i < c->saveGame->members; i++) {
            if (c->players[i].status == STAT_SLEEPING)
                c->players[i].status = STAT_GOOD;
        }
    }

    /* reset our combat variables */
    camping = false;
    inn = false;
    
    if (playerPartyDead())
        deathStart(0);
    else
        (*c->location->finishTurn)();
}

/**
 * Move a party member during combat and display the appropriate messages
 */
MoveReturnValue combatMovePartyMember(Direction dir, int userEvent) {
    CombatMap *cm = getCombatMap();
    MoveReturnValue retval = movePartyMember(dir, userEvent);
    int i;

    /* active player left/fled combat */
    if ((retval & MOVE_EXIT_TO_PARENT) && (c->location->activePlayer == cm->focus)) {
        c->location->activePlayer = -1;
        /* assign active player to next available party member */
        for (i = 0; i < c->saveGame->members; i++) {
            if (cm->party.find(i) != cm->party.end() && !playerIsDisabled(i)) {
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
    else if (cm->winOrLose && (retval & (MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE)))
        soundPlay(SOUND_FLEE);

    return retval;
}

void CombatMap::moveMonsters() {
    int i, target, distance;
    CombatAction action;
    CoordActionInfo *info;
    Monster *m;    

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters.find(i) == monsters.end())
            continue;
        m = monsters[i];

        /* see if monster wakes up if it is asleep */
        if ((m->status == STAT_SLEEPING) && (xu4_random(8) == 0)) {
            m->status = STAT_GOOD;
            m->setAnimated();
        }

        /* if the monster is still asleep, then move on to the next monster */
        if (m->status == STAT_SLEEPING)
            continue;

        if (m->negates()) {
            c->aura = AURA_NEGATE;
            c->auraDuration = 2;
            statsUpdate();
        }

        /* default action */
        action = CA_ATTACK;        

        /* if the monster doesn't have something specific to do yet, let's try to find something! */
        if (action == CA_ATTACK) {
            /* monsters who teleport do so 1/8 of the time */
            if (m->teleports() && xu4_random(8) == 0)
                action = CA_TELEPORT;
            /* monsters who ranged attack do so 1/4 of the time.
               make sure their ranged attack is not negated! */
            else if (m->ranged != 0 && xu4_random(4) == 0 && 
                     ((m->rangedhittile != MAGICFLASH_TILE) || (c->aura != AURA_NEGATE)))
                action = CA_RANGED;
            /* monsters who cast sleep do so 1/4 of the time they don't ranged attack */
            else if (m->castsSleep() && (c->aura != AURA_NEGATE) && (xu4_random(4) == 0))
                action = CA_CAST_SLEEP;
        
            else if (m->getStatus() == MSTAT_FLEEING)
                action = CA_FLEE;
        }
        
        target = findTargetForMonster(m, &distance, action == CA_RANGED);
        if (target == -1 && action == CA_RANGED) {
            action = CA_ADVANCE;
            findTargetForMonster(m, &distance, 0);
        }
        if (target == -1)
            continue;

        if (action == CA_ATTACK && distance > 1)
            action = CA_ADVANCE;

        /* let's see if the monster blends into the background, or if he appears... */
        if (m->camouflages() && !hideOrShowCamouflageMonster(m))
            continue; /* monster is hidden -- no action! */

        switch(action) {
        case CA_ATTACK:
            if (playerIsHitByAttack(&c->players[target])) {
                
                /* steal gold if the monster steals gold */
                if (m->stealsGold() && xu4_random(4) == 0)
                    playerAdjustGold(-(xu4_random(0x3f)));
                
                /* steal food if the monster steals food */
                if (m->stealsFood())
                    playerAdjustFood(-2500);
                               
                attackFlash(party[target]->getCoords(), HITFLASH_TILE, 3);

                applyDamageToPlayer(target, m->getDamage());               
            } else {
                attackFlash(party[target]->getCoords(), MISSFLASH_TILE, 3);
            }
            break;

        case CA_CAST_SLEEP:
            screenMessage("Sleep!\n");

            (*spellEffectCallback)('s', -1, static_cast<Sound>(0)); /* show the sleep spell effect */
            
            /* Apply the sleep spell to everyone still in combat */
            for (i = 0; i < 8; i++) {
                if ((party.find(i) != party.end()) && xu4_random(2) == 0)
                    putPlayerToSleep(i);                
            }
            
            break;

        case CA_TELEPORT: {
                Coords new_c;
                bool valid = false;
                bool firstTry = true;                    
                MapTile tile;                
            
                while (!valid) {
                    new_c = Coords(xu4_random(width), xu4_random(height), c->location->coords.z);
                    
                    tile = tileAt(new_c, WITH_OBJECTS);
                
                    if (tileIsMonsterWalkable(tile) && tileIsWalkable(tile)) {
                        /* If the tile would slow me down, try again! */
                        if (firstTry && tileGetSpeed(tile) != FAST)
                            firstTry = false;
                        /* OK, good enough! */
                        else
                            valid = true;
                    }
                }
            
                /* Teleport! */
                monsters[i]->setCoords(new_c);
            }

            break;

        case CA_RANGED:
            {            
                MapCoords m_coords = monsters[i]->getCoords(),
                          p_coords = party[target]->getCoords();
            
                info = new CoordActionInfo;
                info->handleAtCoord = &combatMonsterRangedAttack;
                info->origin = m_coords;
                info->prev = Coords(-1, -1);
                info->range = 11;
                info->validDirections = MASK_DIR_ALL;
                info->player = i;
                info->blockedPredicate = &tileCanAttackOver;
                info->blockBefore = 1;
                info->firstValidDistance = 0;

                /* if the monster has a random tile for a ranged weapon,
                   let's switch it now! */
                if (monsters[i]->hasRandomRanged())
                    monsters[i]->setRandomRanged();

                /* figure out which direction to fire the weapon */            
                info->dir = m_coords.getRelativeDirection(p_coords);
            
                /* fire! */
                gameDirectionalAction(info);
                delete info;
            } break;

        case CA_FLEE:
        case CA_ADVANCE:
            if (moveCombatObject(action, this, monsters[i], party[target]->getCoords())) {
                Coords coords = monsters[i]->getCoords();

                if (MAP_IS_OOB(this, coords)) {
                    screenMessage("\n%s Flees!\n", m->name.c_str());
                    
                    /* Congrats, you have a heart! */
                    if (monsters[i]->isGood())
                        playerAdjustKarma(KA_SPARED_GOOD);

                    removeObject(monsters[i]);
                    monsters.erase(monsters.find(i));                    
                }
            }
            
            break;
        }
        statsUpdate();
        screenRedrawScreen();
    }
}

int CombatMap::findTargetForMonster(Monster *monster, int *distance, int ranged) {
    int i, curDistance;
    int closest;
    MapCoords m_coords = monster->getCoords();
    
    *distance = 20;
    closest = -1;
    for (i = 0; i < c->saveGame->members; i++) {
        MapCoords p_coords;

        if (party.find(i) == party.end())
            continue;

        p_coords = party[i]->getCoords();

        /* find out how many moves it would take to get to the party member */
        if (ranged) 
            /* ranged attacks can go diagonally, so find the closest using diagonals */
            curDistance = m_coords.distance(p_coords);
        else
            /* normal attacks are n/e/s/w, so find the distance that way */
            curDistance = m_coords.movementDistance(p_coords);

        /* skip target if further than current target */
        if (curDistance > (*distance))
            continue;
        /* skip target 50% of time if same distance */
        if (curDistance == (*distance) && xu4_random(2) == 0)
            continue;
        
        (*distance) = curDistance;
        closest = i;
    }

    return closest;
}

/**
 * Applies 'damage' amount of damage to the monster
 */

void CombatMap::applyDamageToMonster(int monster, int damage, int player) {
    int xp;
    Monster *m = monsters[monster];

    /* deal the damage */
    if (m->id != LORDBRITISH_ID)
        m->hp -= damage;

    switch (m->getStatus()) {

    case MSTAT_DEAD:
        xp = m->xp;
        screenMessage("%s Killed!\nExp. %d\n", m->name.c_str(), xp);
        
        /* if a player killed the creature, then award the XP,
           otherwise, it died on its own */
        if (player >= 0) {
            playerAwardXp(&c->players[player], xp);
            if (m->isEvil())
                playerAdjustKarma(KA_KILLED_EVIL);
        }

        removeObject(m);
        monsters.erase(monsters.find(monster));        
        break;

    case MSTAT_FLEEING:
        screenMessage("%s Fleeing!\n", m->name.c_str());
        break;

    case MSTAT_CRITICAL:
        screenMessage("%s Critical!\n", m->name.c_str());
        break;

    case MSTAT_HEAVILYWOUNDED:
        screenMessage("%s\nHeavily Wounded!\n", m->name.c_str());
        break;

    case MSTAT_LIGHTLYWOUNDED:
        screenMessage("%s\nLightly Wounded!\n", m->name.c_str());
        break;

    case MSTAT_BARELYWOUNDED:
        screenMessage("%s\nBarely Wounded!\n", m->name.c_str());
        break;
    }
}

/**
 * Applies 'damage' amount of damage to the player, updates map if
 * player killed.
 */
 
void CombatMap::applyDamageToPlayer(int player, int damage) {
    playerApplyDamage(&c->players[player], damage);
    
    if (c->players[player].status == STAT_DEAD) {
        Coords p = party[player]->getCoords();                    
        removeObject(party[player]);
        party.erase(party.find(player));                    
        annotations->add(p, CORPSE_TILE)->setTTL(c->saveGame->members);
        screenMessage("%s is Killed!\n", c->players[player].name);
    }  
}

/**
 * Show an attack flash at x, y. This is used for 'being hit' or 'being missed' 
 * by weapons, cannon fire, spells, etc.
 */

void attackFlash(Coords coords, MapTile tile, int timeFactor) {
    int i;
    int divisor = settings.battleSpeed;
    
    c->location->map->annotations->add(coords, tile, true);
    for (i = 0; i < timeFactor; i++) {        
        /* do screen animations while we're pausing */
        if (i % divisor == 1)
            screenCycle();

        gameUpdateScreen();       
        eventHandlerSleep(eventTimerGranularity/divisor);
    }
    c->location->map->annotations->remove(coords, tile);
}

/**
 * Key handler for choosing an attack direction
 */
bool combatChooseWeaponDir(int key, void *data) {
    CoordActionInfo *info = static_cast<CoordActionInfo *>(eventHandlerGetKeyHandlerData());
    Direction dir = keyToDirection(key);
    bool valid = (dir != DIR_NONE) ? true : false;
    const Weapon *weapon = Weapon::get(c->players[info->player].weapon);

    eventHandlerPopKeyHandler();
    info->dir = MASK_DIR(dir);

    if (valid) {
        screenMessage("%s\n", getDirectionName(dir));
        if (weapon->canChooseDistance()) {
            screenMessage("Range: ");
            eventHandlerPushKeyHandlerWithData(&combatChooseWeaponRange, info);
        }
        else gameDirectionalAction(info);        
    }

    eventHandlerPopKeyHandlerData();
    
    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Key handler for choosing the range of a wepaon
 */
bool combatChooseWeaponRange(int key, void *data) {    
    CoordActionInfo *info = static_cast<CoordActionInfo *>(data);

    if ((key >= '0') && (key <= (info->range + '0'))) {
        info->range = key - '0';
        screenMessage("%d\n", info->range);
        gameDirectionalAction(info);

        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();

        return true;
    }
    
    return false;
}

/**
 * Apply tile effects to all monsters depending on what they're standing on
 */
void CombatMap::applyMonsterTileEffects() {
    int i;
    bool affected = false;

    for (i = 0; i < AREA_MONSTERS; i++) {
        if (monsters.find(i) != monsters.end()) {
            TileEffect effect;
            effect = tileGetEffect(tileAt(monsters[i]->getCoords(), WITH_GROUND_OBJECTS));
            if (effect != EFFECT_NONE) {

                /* give a slight pause before enacting the tile effect */
                if (!affected) {
                    gameUpdateScreen();
                    eventHandlerSleep(100);
                    affected = true;
                }

                switch(effect) {
                case EFFECT_SLEEP:
                    /* monster fell asleep! */
                    if ((monsters[i]->resists != EFFECT_SLEEP) &&
                        (xu4_random(0xFF) >= monsters[i]->hp)) {
                        monsters[i]->status = STAT_SLEEPING;
                        monsters[i]->setAnimated(false); /* freeze monster */
                    }
                    break;

                case EFFECT_LAVA:
                case EFFECT_FIRE:
                    /* deal 0 - 127 damage to the monster if it is not immune to fire damage */
                    if (!(monsters[i]->resists & (EFFECT_FIRE | EFFECT_LAVA)))
                        applyDamageToMonster(i, xu4_random(0x7F), -1);
                    break;

                case EFFECT_POISONFIELD:
                    /* deal 0 - 127 damage to the monster if it is not immune to poison field damage */
                    if (monsters[i]->resists != EFFECT_POISONFIELD)
                        applyDamageToMonster(i, xu4_random(0x7F), -1);
                    break;

                case EFFECT_POISON:
                default: break;
                }
            }
        }
    }
}

int CombatMap::divideMonster(Monster *obj) {
    int dirmask = getValidMoves(obj->getCoords(), obj->getTile());
    Direction d = dirRandomDir(dirmask);

    /* this is a game enhancement, make sure it's turned on! */
    if (!settings.enhancements || !settings.enhancementsOptions.slimeDivides)
        return 0;
    
    /* make sure there's a place to put the divided monster! */
    if (d != DIR_NONE) {
        int index;
                            
        /* find the first free slot in the monster table, if there is one */
        for (index = 0; index < AREA_MONSTERS; index++) {
            if (monsters.find(index) == monsters.end()) {
                MapCoords coords;                
                
                screenMessage("%s Divides!\n", obj->name.c_str());

                /* find a spot to put our new monster */
                coords = obj->getCoords();
                coords.move(d, this);                

                /* create our new monster! */
                monsters[index] = Map::addMonster(obj, coords);                
                return 1;
            }
        }        
    }
    return 0;
}

/**
 * Returns the id of the nearest party member (0-8)
 * and fills 'dist' with the distance
 */
int CombatMap::nearestPartyMember(Monster *obj, int *dist) {
    int member, nearest = -1, d, leastDist = 0xFFFF;    
    MapCoords o_coords = obj->getCoords();

    for (member = 0; member < c->saveGame->members; member++) {
        if (party.find(member) != party.end()) {
            MapCoords p_coords = party[member]->getCoords();

            d = o_coords.movementDistance(p_coords);
            if (d < leastDist) {
                nearest = member;
                leastDist = d;
            }
        }
    }

    if (nearest >= 0)
        *dist = leastDist;

    return nearest;
}

/**
 * Hides or shows a camouflaged monster, depending on its distance from
 * the nearest party member
 */
int CombatMap::hideOrShowCamouflageMonster(Monster *monster) {
    /* find the nearest party member */
    int dist;
    int nearestMember = nearestPartyMember(monster, &dist);

    /* ok, now we've got the nearest party member.  Now, see if they're close enough */
    if (nearestMember >= 0) {
        if ((dist < 5) && !monster->isVisible())
            monster->setVisible(); /* show yourself */
        else if (dist >= 5)
            monster->setVisible(false); /* hide and take no action! */
    }

    return monster->isVisible();
}
