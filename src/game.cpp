/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <map>

#include <ctime>
#include "u4.h"

#include "game.h"

#include "annotation.h"
#include "armor.h"
#include "camp.h"
#include "city.h"
#include "dungeon.h"
#include "combat.h"
#include "context.h"
#include "death.h"
#include "debug.h"
#include "direction.h"
#include "error.h"
#include "event.h"
#include "item.h"
#include "location.h"
#include "mapmgr.h"
#include "menu.h"
#include "monster.h"
#include "moongate.h"
#include "movement.h"
#include "music.h"
#include "names.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "spell.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "script.h"
#include "weapon.h"

/*-----------------*/
/* Functions BEGIN */

/* main game functions */
void gameAdvanceLevel(const SaveGamePlayerRecord *player);
void gameInitMoons();
void gameInnHandler(void);
void gameLostEighth(Virtue virtue);
void gamePartyStarving(void);
long gameTimeSinceLastCommand(void);
int gameSave(void);

/* key handlers */
bool talkHandleAnyKey(int key, void *data);
bool cmdHandleAnyKey(int key, void *data);
bool windCmdKeyHandler(int key, void *data);
bool helpPage2KeyHandler(int key, void *data);
bool helpPage3KeyHandler(int key, void *data);

/* map and screen functions */
void gameUpdateMoons(int showmoongates);
int gemHandleChoice(int choice);
int peerCityHandleChoice(int choice);

/* spell functions */
bool castForPlayerGetDestPlayer(int player);
int castForPlayerGetDestDir(Direction dir);
int castForPlayerGetPhase(int phase);
int castForPlayerGetEnergyType(int fieldType);
int castForPlayerGetEnergyDir(Direction dir);
bool castForPlayer2(int spell, void *data);
void gameCastSpell(unsigned int spell, int caster, int param);
void gameSpellEffect(int spell, int player, Sound sound);
int gameSpellMixHowMany(string *message);
bool mixReagentsForSpell(int spell, void *data);
int mixReagentsForSpell2(int choice);

/* conversation functions */
bool talkAtCoord(MapCoords coords, int distance, void *data);
int talkHandleBuffer(string *message);
int talkHandleChoice(int choice);
void talkShowReply(int showPrompt);

/* action functions */
bool attackAtCoord(MapCoords coords, int distance, void *data);
bool destroyAtCoord(MapCoords coords, int distance, void *data);
MoveReturnValue gameMoveAvatar(Direction dir, int userEvent);
MoveReturnValue gameMoveAvatarInDungeon(Direction dir, int userEvent);
bool getChestTrapHandler(int player);
bool jimmyAtCoord(MapCoords coords, int distance, void *data);
bool newOrderForPlayer(int player);
bool newOrderForPlayer2(int player2);
bool openAtCoord(MapCoords coords, int distance, void *data);
bool readyForPlayer(int player);
bool wearForPlayer(int player);
bool wearForPlayer2(int armor, void *data);
bool ztatsFor(int player);

/* checking functions */
void gameCheckBridgeTrolls(void);
int gameCheckMoongates(void);
int gameCheckPlayerDisabled(int player);
void gameCheckRandomMonsters(void);
void gameCheckSpecialMonsters(Direction dir);
void gameLordBritishCheckLevels(void);

/* monster functions */
void gameAlertTheGuards(Map *map);
void gameDestroyAllMonsters(void);
void gameFixupMonsters(Map *map);
void gameMonsterAttack(Monster *obj);
int gameSummonMonster(string *monsterName);

/* etc */
bool gameCreateBalloon(Map *map);

/* Functions END */
/*---------------*/

extern Object *party[8];
Context *c = NULL;
int windLock = 0;
string itemNameBuffer;
string monsterNameBuffer;
string howmany;
string destination;
int paused = 0;
int pausedTimer = 0;
int castPlayer;
unsigned int castSpell;
EnergyFieldType fieldType;

/* FIXME */
Mixture *mix;
int mixSpell;
Menu spellMixMenu;

MouseArea mouseAreas[] = {
    { 3, { { 8, 8 }, { 8, 184 }, { 96, 96 } }, MC_WEST, { U4_ENTER, 0, U4_LEFT } },
    { 3, { { 8, 8 }, { 184, 8 }, { 96, 96 } }, MC_NORTH, { U4_ENTER, 0, U4_UP }  },
    { 3, { { 184, 8 }, { 184, 184 }, { 96, 96 } }, MC_EAST, { U4_ENTER, 0, U4_RIGHT } },
    { 3, { { 8, 184 }, { 184, 184 }, { 96, 96 } }, MC_SOUTH, { U4_ENTER, 0, U4_DOWN } },
    { 0 }
};

void gameInit() {
    FILE *saveGameFile, *monstersFile;    

    /* initialize the global game context */
    c = new Context;
    c->saveGame = new SaveGame;    

    /* initialize conversation and game state variables */
    c->conversation.talker = NULL;
    c->conversation.state = 0;    
    c->conversation.reply = NULL;
    c->conversation.script = new Script();
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->statsView = STATS_PARTY_OVERVIEW;
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->aura = AURA_NONE;
    c->auraDuration = 0;
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = time(NULL);
    c->lastShip = NULL;

    /* load in the save game */
    saveGameFile = saveGameOpenForReading();
    if (saveGameFile) {
        saveGameRead(c->saveGame, saveGameFile);
        fclose(saveGameFile);
    } else
        errorFatal("no savegame found!");

    /* initialize our start location */
    Map *map = mapMgrGetById(c->saveGame->location);
    
    /* if our map is not the world map, then load the world map first */
    if (map->type != MAPTYPE_WORLD) {
        gameSetMap(mapMgrGetById(MAP_WORLD), 0, NULL);

        /* initialize the moons (must be done from the world map) */
        gameInitMoons();

        gameSetMap(map, 1, NULL);
    }
    else {
        gameSetMap(map, 0, NULL);
        
        /* initialize the moons (must be done from the world map) */
        gameInitMoons();
    }

    /**
     * Translate info from the savegame to something we can use
     */ 
    memcpy(c->players, c->saveGame->players, sizeof(c->players));    
    if (c->location->prev) {
        c->location->coords = MapCoords(c->saveGame->x, c->saveGame->y, c->saveGame->dnglevel);
        c->location->prev->coords = MapCoords(c->saveGame->dngx, c->saveGame->dngy);    
    }
    else c->location->coords = MapCoords(c->saveGame->x, c->saveGame->y, (int)c->saveGame->dnglevel);
    c->saveGame->orientation = (Direction)(c->saveGame->orientation + DIR_WEST);

    /**
     * Fix the coordinates if they're out of bounds.  This happens every
     * time on the world map because (z == -1) is no longer valid.
     * To maintain compatibility with u4dos, this value gets translated
     * when the game is saved and loaded
     */
    if (MAP_IS_OOB(c->location->map, c->location->coords))
        c->location->coords.putInBounds(c->location->map);    

    /* load in monsters.sav */
    monstersFile = saveGameMonstersOpenForReading(MONSTERS_SAV_BASE_FILENAME);
    if (monstersFile) {
        saveGameMonstersRead(&c->location->map->objects, monstersFile);
        fclose(monstersFile);
    }
    gameFixupMonsters(c->location->map);

    /* we have previous monster information as well, load it! */
    if (c->location->prev) {
        monstersFile = saveGameMonstersOpenForReading(OUTMONST_SAV_BASE_FILENAME);
        if (monstersFile) {
            saveGameMonstersRead(&c->location->prev->map->objects, monstersFile);
            fclose(monstersFile);
        }
        gameFixupMonsters(c->location->prev->map);
    }

    /* setup transport context */
    gameSetTransport((MapTile)c->saveGame->transport);

    playerSetLostEighthCallback(&gameLostEighth);
    playerSetAdvanceLevelCallback(&gameAdvanceLevel);
    playerSetItemStatsChangedCallback(&statsUpdate);
    playerSetSpellEffectCallback(&gameSpellEffect);
    playerSetPartyStarvingCallback(&gamePartyStarving);
    playerSetSetTransportCallback(&gameSetTransport);
    itemSetDestroyAllMonstersCallback(&gameDestroyAllMonsters);

    musicPlay();
    screenDrawImage(BKGD_BORDERS);
    statsUpdate();
    screenMessage("Press Alt-h for help\n");
    screenPrompt();    

    /* reagents menu */    
    spellMixMenu.add(0, getReagentName((Reagent)0), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(1, getReagentName((Reagent)1), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(2, getReagentName((Reagent)2), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(3, getReagentName((Reagent)3), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(4, getReagentName((Reagent)4), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(5, getReagentName((Reagent)5), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(6, getReagentName((Reagent)6), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    spellMixMenu.add(7, getReagentName((Reagent)7), STATS_AREA_X+2, 0, NULL, ACTIVATE_NORMAL);
    gameResetSpellMixing();

    eventHandlerPushMouseAreaSet(mouseAreas);
}

/**
 * Saves the game state into party.sav and monsters.sav.
 */
int gameSave() {
    FILE *saveGameFile, *monstersFile, *dngMapFile;
    SaveGame save;

    /*************************************************/
    /* Make sure the savegame struct is accurate now */

    memcpy(save.armor, c->saveGame->armor, sizeof(save.armor));    
    save.balloonstate = c->saveGame->balloonstate;
    if (c->location->prev) {
        save.x = c->location->coords.x;
        save.y = c->location->coords.y;
        save.dnglevel = c->location->coords.z;
        save.dngx = c->location->prev->coords.x;
        save.dngy = c->location->prev->coords.y;
    }
    else {
        save.x = c->location->coords.x;
        save.y = c->location->coords.y;
        save.dnglevel = c->location->coords.z;
        save.dngx = c->saveGame->dngx;
        save.dngy = c->saveGame->dngy;
    }
    save.feluccaphase = c->saveGame->feluccaphase;
    save.food = c->saveGame->food;
    save.gems = c->saveGame->gems;
    save.gold = c->saveGame->gold;
    save.items = c->saveGame->items;
    memcpy(save.karma, c->saveGame->karma, sizeof(save.karma));
    save.keys = c->saveGame->keys;
    save.lastcamp = c->saveGame->lastcamp;
    save.lastmeditation = c->saveGame->lastmeditation;
    save.lastreagent = c->saveGame->lastreagent;
    save.lastvirtue = c->saveGame->lastvirtue;
    save.lbintro = c->saveGame->lbintro;
    save.location = c->location->map->id;
    save.members = c->saveGame->members;
    memcpy(save.mixtures, c->saveGame->mixtures, sizeof(save.mixtures));
    save.moves = c->saveGame->moves;
    save.orientation = (Direction)(c->saveGame->orientation - DIR_WEST);
    memcpy(save.players, c->players, sizeof(save.players));
    memcpy(save.reagents, c->saveGame->reagents, sizeof(save.reagents));
    save.runes = c->saveGame->runes;
    save.sextants = c->saveGame->sextants;
    save.shiphull = c->saveGame->shiphull;
    save.stones = c->saveGame->stones;
    save.torchduration = c->saveGame->torchduration;
    save.torches = c->saveGame->torches;
    save.trammelphase = c->saveGame->trammelphase;
    save.transport = c->saveGame->transport;
    memcpy(save.weapons, c->saveGame->weapons, sizeof(save.weapons));    

    /* Done making sure the savegame struct is accurate */
    /****************************************************/

    saveGameFile = saveGameOpenForWriting();
    if (!saveGameFile) {
        screenMessage("Error opening party.sav\n");
        return 0;
    }

    if (!saveGameWrite(&save, saveGameFile)) {
        screenMessage("Error writing to party.sav\n");
        fclose(saveGameFile);
        return 0;
    }
    fclose(saveGameFile);

    monstersFile = saveGameMonstersOpenForWriting(MONSTERS_SAV_BASE_FILENAME);
    if (!monstersFile) {
        screenMessage("Error opening %s\n", MONSTERS_SAV_BASE_FILENAME);
        return 0;
    }

    /* fix monster animations so they are compatible with u4dos */
    c->location->map->resetObjectAnimations();

    if (!saveGameMonstersWrite(c->location->map->objects, monstersFile)) {
        screenMessage("Error opening monsters.sav\n");
        fclose(monstersFile);
        return 0;
    }
    fclose(monstersFile);

    /**
     * Write dungeon info
     */ 
    if (c->location->context & CTX_DUNGEON) {
        unsigned int x, y, z;

        typedef std::map<const Monster*, int, std::less<const Monster*> > DngMonsterIdMap;
        static DngMonsterIdMap id_map;        

        /**
         * Map monsters to u4dos dungeon monster Ids
         */ 
        if (id_map.size() == 0) {
            id_map[monsters.getById(RAT_ID)]             = 1;
            id_map[monsters.getById(BAT_ID)]             = 2;
            id_map[monsters.getById(GIANT_SPIDER_ID)]    = 3;
            id_map[monsters.getById(GHOST_ID)]           = 4;
            id_map[monsters.getById(SLIME_ID)]           = 5;
            id_map[monsters.getById(TROLL_ID)]           = 6;
            id_map[monsters.getById(GREMLIN_ID)]         = 7;
            id_map[monsters.getById(MIMIC_ID)]           = 8;
            id_map[monsters.getById(REAPER_ID)]          = 9;
            id_map[monsters.getById(INSECT_SWARM_ID)]    = 10;
            id_map[monsters.getById(GAZER_ID)]           = 11;
            id_map[monsters.getById(PHANTOM_ID)]         = 12;
            id_map[monsters.getById(ORC_ID)]             = 13;
            id_map[monsters.getById(SKELETON_ID)]        = 14;
            id_map[monsters.getById(ROGUE_ID)]           = 15;
        }

        dngMapFile = fopen("dngmap.sav", "wb");
        if (!dngMapFile) {
            screenMessage("Error opening dngmap.sav\n");
            return 0;
        }

        for (z = 0; z < c->location->map->levels; z++) {
            for (y = 0; y < c->location->map->height; y++) {
                for (x = 0; x < c->location->map->width; x++) {
                    MapTile tile = c->location->map->getTileFromData(MapCoords(x, y, z));
                    Object *obj = c->location->map->objectAt(MapCoords(x, y, z));

                    /**
                     * Add the monster to the tile
                     */ 
                    if (obj && obj->getType() == OBJECT_MONSTER) {
                        const Monster *m = dynamic_cast<Monster*>(obj);
                        DngMonsterIdMap::iterator m_id = id_map.find(m);
                        if (m_id != id_map.end())
                            tile |= m_id->second;                        
                    }

                    // Write the tile
                    fputc(tile, dngMapFile);
                }
            }
        }

        fclose(dngMapFile);

        /**
         * Write outmonst.sav
         */ 

        monstersFile = saveGameMonstersOpenForWriting(OUTMONST_SAV_BASE_FILENAME);
        if (!monstersFile) {
            screenMessage("Error opening %s\n", OUTMONST_SAV_BASE_FILENAME);
            return 0;
        }
        
        /* fix monster animations so they are compatible with u4dos */
        c->location->prev->map->resetObjectAnimations();

        if (!saveGameMonstersWrite(c->location->prev->map->objects, monstersFile)) {
            screenMessage("Error opening %s\n", OUTMONST_SAV_BASE_FILENAME);
            fclose(monstersFile);
            return 0;
        }
        fclose(monstersFile);
    }

    return 1;
}

/**
 * Sets the view mode.
 */
void gameSetViewMode(ViewMode newMode) {
    c->location->viewMode = newMode;
}

void gameUpdateScreen() {
    switch (c->location->viewMode) {
    case VIEW_NORMAL:
        screenUpdate(1, 0);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_RUNE:
        screenUpdate(0, 0);
        break;
    case VIEW_DUNGEON:
        screenUpdate(1, 0);
        break;
    case VIEW_DEAD:
        screenUpdate(1, 1);
        break;
    case VIEW_CODEX: /* the screen updates will be handled elsewhere */
        break;
    default:
        ASSERT(0, "invalid view mode: %d", c->location->viewMode);
    }
}

void gameSetMap(Map *map, int saveLocation, const Portal *portal) {
    int viewMode;
    LocationContext context;
    FinishTurnCallback finishTurn = &gameFinishTurn;
    MoveCallback move = &gameMoveAvatar;
    Tileset *tileset = tilesetGetByType(TILESET_BASE);
    int activePlayer = (c->location) ? c->location->activePlayer : -1;
    MapCoords coords;

    if (portal)
        coords = portal->start;
    else
        coords = MapCoords(map->width / 2, map->height / 2);
    
    /* If we don't want to save the location, then just return to the previous location,
       as there may still be ones in the stack we want to keep */
    if (!saveLocation)
        gameExitToParentMap();
    
    switch (map->type) {
    case MAPTYPE_WORLD:
        context = CTX_WORLDMAP;
        viewMode = VIEW_NORMAL;
        break;
    case MAPTYPE_DUNGEON:
        context = CTX_DUNGEON;
        viewMode = VIEW_DUNGEON;
        if (portal)
            c->saveGame->orientation = DIR_EAST;
        move = &gameMoveAvatarInDungeon;
        tileset = tilesetGetByType(TILESET_DUNGEON);        
        break;
    case MAPTYPE_COMBAT:
        coords = MapCoords(-1, -1); /* set these to -1 just to be safe; we don't need them */
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        finishTurn = &combatFinishTurn;
        move = &combatMovePartyMember;
        activePlayer = -1; /* different active player for combat, defaults to 'None' */
        break;
    case MAPTYPE_CITY:    
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    }
    
    c->location = locationNew(coords, map, viewMode, context, finishTurn, move, tileset, c->location);    
    c->location->activePlayer = activePlayer;

    if (isCity(map)) {
        City *city = dynamic_cast<City*>(map);
        city->addPeople();        
    }
}

/**
 * Exits the current map and location and returns to its parent location
 * This restores all relevant information from the previous location,
 * such as the map, map position, etc. (such as exiting a city)
 **/

int gameExitToParentMap() {
    if (!c->location)
        return 0;

    if (c->location->prev != NULL) {
        /* Create the balloon for Hythloth */
        if (c->location->map->id == MAP_HYTHLOTH)
            gameCreateBalloon(c->location->prev->map);            

        /* free map info only if previous location was on a different map */
        if (c->location->prev->map != c->location->map) {
            c->location->map->annotations->clear();
            c->location->map->clearObjects();
        }
        locationFree(&c->location);       
        
        return 1;
    }
    return 0;
}

/**
 * Terminates a game turn.  This performs the post-turn housekeeping
 * tasks like adjusting the party's food, incrementing the number of
 * moves, etc.
 */
void gameFinishTurn() {
    Monster *attacker = NULL;    

    while (1) {
        /* adjust food and moves */
        playerEndTurn();

        /* check if aura has expired */
        if (c->auraDuration > 0) {
            if (--c->auraDuration == 0)
                c->aura = AURA_NONE;
        }

        gameCheckHullIntegrity();

        /* update party stats */
        c->statsView = STATS_PARTY_OVERVIEW;
        statsUpdate();

        /* Monsters cannot spawn, move or attack while the avatar is on the balloon */
        /* FIXME: balloonstate is causing problems when mixed with torchduration --
           needs to be separated during gameplay and then put into savegame structure
           when saving */
        if (c->location->context == CTX_DUNGEON || (!c->saveGame->balloonstate)) {

            // apply effects from tile avatar is standing on 
            playerApplyEffect(tileGetEffect(c->location->map->tileAt(c->location->coords, WITH_GROUND_OBJECTS)), ALL_PLAYERS);

            // Move monsters and see if something is attacking the avatar
            attacker = c->location->map->moveObjects(c->location->coords);        

            // Something's attacking!  Start combat!
            if (attacker) {
                gameMonsterAttack(attacker);
                return;
            }       

            // Spawn new monsters
            gameCheckRandomMonsters();            
            gameCheckBridgeTrolls();
        }

        /* update map annotations */
        c->location->map->annotations->passTurn();

        if (!playerPartyImmobilized())
            break;

        if (playerPartyDead()) {
            deathStart(0);
            return;
        } else {            
            screenMessage("Zzzzzz\n");
        }
    }

    if (c->location->context == CTX_DUNGEON) {
        if (c->saveGame->torchduration <= 0)
            screenMessage("It's Dark!\n");
        else c->saveGame->torchduration--;

        /* handle dungeon traps */
        if (dungeonCurrentToken() == DUNGEON_TRAP)
            dungeonHandleTrap((TrapType)dungeonCurrentSubToken());
    }
    /* since torchduration and balloon state share the same variable, make sure our torch
       isn't still lit (screwing all sorts of things up) */
    else if (c->transportContext != TRANSPORT_BALLOON && c->saveGame->balloonstate)
        c->saveGame->balloonstate = 0;

    /* draw a prompt */
    screenPrompt();
    screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);

    c->lastCommandTime = time(NULL);
}

/**
 * Inform a player he has lost zero or more eighths of avatarhood.
 */
void gameLostEighth(Virtue virtue) {
    screenMessage("\n Thou hast lost\n  an eighth!\n");
    statsUpdate();
}

void gameAdvanceLevel(const SaveGamePlayerRecord *player) {
    screenMessage("\n%s\nThou art now Level %d\n", player->name, playerGetRealLevel(player));

    (*spellEffectCallback)('r', -1, SOUND_MAGIC); // Same as resurrect spell
}

void gamePartyStarving(void) {
    int i;
    
    screenMessage("\nStarving!!!\n");
    /* FIXME: add sound effect here */

    /* Do 2 damage to each party member for starving! */
    for (i = 0; i < c->saveGame->members; i++)
        playerApplyDamage(&c->players[i], 2);    
}

void gameSpellEffect(int spell, int player, Sound sound) {
    int time;
    SpellEffect effect = SPELLEFFECT_INVERT;
        
    if (player >= 0)
        statsHighlightCharacter(player);

    /* recalculate spell speed - based on 5/sec */
    time = settings.spellEffectSpeed * 200;

    soundPlay(sound);

    switch(spell)
    {
    case 'g': /* gate */
    case 'r': /* resurrection */
        time = (time * 3) / 2;
        break;
    case 't': /* tremor */
        time = (time * 3) / 2;
        effect = SPELLEFFECT_TREMOR;        
        break;
    default:
        /* default spell effect */        
        break;
    }

    /* pause the game for enough time to complete the spell effect */
    if (!paused) {
        paused = 1;
        pausedTimer = ((time * settings.gameCyclesPerSecond) / 1000) + 1;
    }

    switch(effect)
    {
    case SPELLEFFECT_NONE: 
        break;
    case SPELLEFFECT_TREMOR:
    case SPELLEFFECT_INVERT:
        gameUpdateScreen();
        screenInvertRect(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT);
        screenRedrawScreen();
        
        eventHandlerSleep(time);

        if (effect == SPELLEFFECT_TREMOR) {
            gameUpdateScreen();
            screenShake(10);            
        }

        break;
    }
    
    statsUpdate();
}

void gameCastSpell(unsigned int spell, int caster, int param) {
    SpellCastError spellError;
    string msg;
    
    if (!spellCast(spell, caster, param, &spellError, 1)) {        
        msg = spellGetErrorMessage(spell, spellError);
        if (!msg.empty())
            screenMessage(msg.c_str());
    }    
}

int gameCheckPlayerDisabled(int player) {
    ASSERT(player < c->saveGame->members, "player %d, but only %d members\n", player, c->saveGame->members);

    if (c->players[player].status == STAT_DEAD ||
        c->players[player].status == STAT_SLEEPING) {
        screenMessage("Disabled!\n");
        return 1;
    }

    return 0;
}


/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
bool gameBaseKeyHandler(int key, void *data) {
    bool valid = true;
    int endTurn = 1;
    Object *obj;
    CoordActionInfo *info;    
    AlphaActionInfo *alphaInfo;
    const ItemLocation *item;
    MapTile tile;

    /* Translate context-sensitive action key into a useful command */
    if (key == U4_ENTER && settings.enhancements && settings.enhancementsOptions.smartEnterKey) {
        /* Attempt to guess based on the character's surroundings etc, what
           action they want */        
        
        /* Do they want to board something? */
        if (c->transportContext == TRANSPORT_FOOT) {
            obj = c->location->map->objectAt(c->location->coords);
            if (obj && (tileIsShip(obj->getTile()) || tileIsHorse(obj->getTile()) ||
                tileIsBalloon(obj->getTile()))) key = 'b';
        }
        /* Klimb/Descend Balloon */
        else if (c->transportContext == TRANSPORT_BALLOON) {            
            if (c->saveGame->balloonstate == 1)
                key = 'd';
            else key = 'k';
        }
        /* X-it transport */
        else key = 'x';        
        
        /* Klimb? */
        if ((c->location->map->portalAt(c->location->coords, ACTION_KLIMB) != NULL) || 
                (c->location->context == CTX_DUNGEON &&
                dungeonLadderUpAt(c->location->map, c->location->coords)))
            key = 'k';
        /* Descend? */
        else if ((c->location->map->portalAt(c->location->coords, ACTION_DESCEND) != NULL) ||
                (c->location->context == CTX_DUNGEON &&
                dungeonLadderDownAt(c->location->map, c->location->coords)))
            key = 'd';        
        /* Enter? */
        else if (c->location->map->portalAt(c->location->coords, ACTION_ENTER) != NULL)
            key = 'e';
        
        /* Get Chest? */
        if ((c->location->context == CTX_DUNGEON) || 
            (!c->saveGame->balloonstate)) {
            tile = c->location->map->tileAt(c->location->coords, WITH_GROUND_OBJECTS);
    
            if (tileIsChest(tile)) key = 'g';
        }
        
        /* None of these? Default to search */
        if (key == U4_ENTER) key = 's';
    }
    
    switch (key) {

    case U4_UP:
    case U4_DOWN:
    case U4_LEFT:
    case U4_RIGHT:        
        {
            /* move the avatar */
            MoveReturnValue retval = (*c->location->move)(keyToDirection(key), 1);
        
            /* horse doubles speed (make sure we're on the same map as the previous move first) */
            if (retval & (MOVE_SUCCEEDED | MOVE_SLOWED) && 
                (c->transportContext == TRANSPORT_HORSE) && c->horseSpeed) {
                gameUpdateScreen(); /* to give it a smooth look of movement */
                (*c->location->move)(keyToDirection(key), 0);
            }

            endTurn = (retval & MOVE_END_TURN); /* let the movement handler decide to end the turn */
        }
        
        break;    

    case U4_FKEY:
    case U4_FKEY+1:
    case U4_FKEY+2:
    case U4_FKEY+3:
    case U4_FKEY+4:
    case U4_FKEY+5:
    case U4_FKEY+6:
    case U4_FKEY+7:
        /* teleport to dungeon entrances! */
        if (settings.debug && (c->location->context & CTX_WORLDMAP) && (c->transportContext & TRANSPORT_FOOT_OR_HORSE))
        {
            int portal = 16 + (key - U4_FKEY); /* find dungeon portal */
            c->location->coords = c->location->map->portals[portal]->coords;
        }
        else valid = false;
        break;

    case U4_FKEY+8:
        if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
            gameSetMap(mapMgrGetById(MAP_DECEIT), 1, NULL);
            c->location->coords = MapCoords(1, 0, 7);            
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+9:
        if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
            gameSetMap(mapMgrGetById(MAP_DESPISE), 1, NULL);
            c->location->coords = MapCoords(3, 2, 7);
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+10:
        if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
            gameSetMap(mapMgrGetById(MAP_DESTARD), 1, NULL);
            c->location->coords = MapCoords(7, 6, 7);            
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+11:
        if (settings.debug) {
            screenMessage("Torch: %d\n", c->saveGame->torchduration);
            screenPrompt();
        }
        else valid = false;
        break;

    case 3:                     /* ctrl-C */
        if (settings.debug) {
            screenMessage("Cmd (h = help):");
            eventHandlerPushKeyHandler(&gameSpecialCmdKeyHandler);            
        }
        else valid = false;
        break;

    case 4:                     /* ctrl-D */
        if (settings.debug) {
            info = new CoordActionInfo;
            info->handleAtCoord = &destroyAtCoord;
            info->origin = c->location->coords;            
            info->prev = MapCoords(-1, -1);
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Destroy Object\nDir: ");
        }
        else valid = false;
        break;    

    case 8:                     /* ctrl-H */
        if (settings.debug) {
            screenMessage("Help!\n");
            screenPrompt();
            
            /* Help! send me to Lord British (who conveniently is right around where you are)! */
            gameSetMap(mapMgrGetById(100), 1, NULL);
            c->location->coords.x = 19;
            c->location->coords.y = 8;
            c->location->coords.z = 0;
        }
        else valid = false;
        break;    

    case 22:                    /* ctrl-V */
        {
            extern int screen3dDungeonView;
            if (settings.debug && c->location->context == CTX_DUNGEON) {
                screen3dDungeonView = screen3dDungeonView ? 0 : 1;
                screenMessage("3-D view %s\n", screen3dDungeonView ? "on" : "off");
                endTurn = 0;
            }
            else valid = false;
        }
        break;    

    case ' ':
        screenMessage("Pass\n");        
        break;

    case 'a':
        screenMessage("Attack: ");

        if (c->saveGame->balloonstate)
            screenMessage("\nDrift only!\n");
        else {
            info = new CoordActionInfo;
            info->handleAtCoord = &attackAtCoord;
            info->origin = c->location->coords;            
            info->prev = MapCoords(-1, -1);
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);            
        }
        break;

    case 'b':

        obj = c->location->map->objectAt(c->location->coords);

        if (c->transportContext != TRANSPORT_FOOT)
            screenMessage("Board: Can't!\n");
        else if (obj) {
            int validTransport = 1;
            
            if (tileIsShip(obj->getTile())) {
                screenMessage("Board Frigate!\n");
                if (c->lastShip != obj)
                    c->saveGame->shiphull = 50;
            }
            else if (tileIsHorse(obj->getTile()))
                screenMessage("Mount Horse!\n");
            else if (tileIsBalloon(obj->getTile()))
                screenMessage("Board Balloon!\n");
            else validTransport = 0;

            if (validTransport) {
                gameSetTransport(obj->getTile());
                c->location->map->removeObject(obj);
            }
            else screenMessage("Board What?\n");
        }
        else screenMessage("Board What?\n");
        break;

    case 'c':
        screenMessage("Cast Spell!\nPlayer: ");
        gameGetPlayerForCommand(&gameCastForPlayer, 0, 1);
        break;

    case 'd':        
        if (!usePortalAt(c->location, c->location->coords, ACTION_DESCEND)) {
            if (c->transportContext == TRANSPORT_BALLOON) {
                screenMessage("Land Balloon\n");
                if (c->saveGame->balloonstate == 0)
                    screenMessage("Already Landed!\n");
                else if (tileCanLandBalloon(c->location->map->tileAt(c->location->coords, WITH_OBJECTS))) {
                    c->saveGame->balloonstate = 0;
                    c->opacity = 1;
                }
                else screenMessage("Not Here!\n");
            }
            else screenMessage("Descend what?\n");
        }        

        break;

    case 'e':
        if (!usePortalAt(c->location, c->location->coords, ACTION_ENTER)) {
            if (!c->location->map->portalAt(c->location->coords, ACTION_ENTER))
                screenMessage("Enter what?\n");
        }
        else endTurn = 0; /* entering a portal doesn't end the turn */
        break;

    case 'f':
        if (c->transportContext == TRANSPORT_SHIP) {
            int broadsidesDirs = dirGetBroadsidesDirs(tileGetDirection((MapTile)c->saveGame->transport));            

            info = new CoordActionInfo;
            info->handleAtCoord = &fireAtCoord;
            info->origin = c->location->coords;            
            info->prev = MapCoords(-1, -1);
            info->range = 3;
            info->validDirections = broadsidesDirs; /* can only fire broadsides! */
            info->player = -1;
            info->blockedPredicate = NULL; /* nothing (not even mountains!) can block cannonballs */
            info->blockBefore = 1;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);
            
            screenMessage("Fire Cannon!\nDir: ");
        }
        else
            screenMessage("Fire What?\n");
        break;

    case 'g':
        screenMessage("Get Chest!\n");
        
        if ((c->location->context != CTX_DUNGEON) && c->saveGame->balloonstate)        
            screenMessage("Drift only!\n");
        else {
            tile = c->location->map->tileAt(c->location->coords, WITH_GROUND_OBJECTS);
    
            if (tileIsChest(tile))
            {
                screenMessage("Who opens? ");
                gameGetPlayerForCommand(&gameGetChest, 0, 1);
            }
            else
                screenMessage("Not here!\n");
        }
        
        break;

    case 'h':
        if (!(c->location->context & (CTX_WORLDMAP | CTX_DUNGEON))) {
            screenMessage("Hole up & Camp\nNot here!\n");
            break;
        }
        if (c->transportContext != TRANSPORT_FOOT) {
            screenMessage("Hole up & Camp\nOnly on foot!\n");
            break;
        }
        screenMessage("Hole up & Camp!\n");
        campBegin();
        break;

    case 'i':
        screenMessage("Ignite torch!\n");
        if (c->location->context == CTX_DUNGEON)            
            c->saveGame->torchduration += 100;
        else screenMessage("Not here!\n");
        break;

    case 'j':
        info = new CoordActionInfo;
        info->handleAtCoord = &jimmyAtCoord;
        info->origin = c->location->coords;        
        info->prev = MapCoords(-1, -1);
        info->range = 1;
        info->validDirections = MASK_DIR_ALL;
        info->player = -1;
        info->blockedPredicate = NULL;
        info->blockBefore = 0;
        info->firstValidDistance = 1;
        eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);
        screenMessage("Jimmy\nDir: ");
        break;

    case 'k':        
        if (!usePortalAt(c->location, c->location->coords, ACTION_KLIMB)) {
            if (c->transportContext == TRANSPORT_BALLOON) {
                c->saveGame->balloonstate = 1;
                c->opacity = 0;
                screenMessage("Klimb altitude\n");            
            } else
                screenMessage("Klimb what?\n");
        }
        break;

    case 'l':
        /* can't use sextant in dungeon or in combat */
        if (c->location->context & ~(CTX_DUNGEON | CTX_COMBAT)) {
            if (c->saveGame->sextants >= 1)
                screenMessage("Locate position\nwith sextant\n Latitude: %c'%c\"\nLongitude: %c'%c\"\n",
                              c->location->coords.y / 16 + 'A', c->location->coords.y % 16 + 'A',
                              c->location->coords.x / 16 + 'A', c->location->coords.x % 16 + 'A');
            else
                screenMessage("Locate position with What?\n");
        }
        else screenMessage("Not here!\n");
        break;

    case 'm':
        screenMessage("Mix reagents\n");
        
        alphaInfo = new AlphaActionInfo;
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "For Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt.c_str());
        eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, alphaInfo);        

        c->statsView = STATS_MIXTURES;
        statsUpdate();
        break;

    case 'n':
        screenMessage("New Order!\nExchange # ");
        gameGetPlayerForCommand(&newOrderForPlayer, 1, 0);
        break;

    case 'o':
        if (c->saveGame->balloonstate)
            screenMessage("Open; Not Here!\n");
        else {
            info = new CoordActionInfo;
            info->handleAtCoord = &openAtCoord;
            info->origin = c->location->coords;
            info->prev = MapCoords(-1, -1);
            info->range = 1;
            info->validDirections = MASK_DIR_ALL;
            info->player = -1;
            info->blockedPredicate = NULL;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Open\nDir: ");
        }
        break;

    case 'p':
        if (c->saveGame->gems) {
            c->saveGame->gems--;
            
            gamePeerGem();            
            screenMessage("Peer at a Gem!\n");
        } else
            screenMessage("Peer at What?\n");
        break;

    case 'q':        
        screenMessage("Quit & Save...\n%d moves\n", c->saveGame->moves);
        if (c->location->context & CTX_CAN_SAVE_GAME) {        
            gameSave();
            screenMessage("Press Alt-x to quit\n");
        }
        else screenMessage("Not here!\n");
        
        break;

    case 'r':
        screenMessage("Ready a weapon\nfor: ");
        gameGetPlayerForCommand(&readyForPlayer, 1, 0);
        break;

    case 's':
        if (c->location->context == CTX_DUNGEON)
            dungeonSearch();
        else if (c->saveGame->balloonstate)
            screenMessage("Searching...\nDrift only!\n");
        else {
            screenMessage("Searching...\n");

            item = itemAtLocation(c->location->map, c->location->coords);
            if (item) {
                if (*item->isItemInInventory != NULL && (*item->isItemInInventory)(item->data))
                    screenMessage("Nothing Here!\n");
                else {                
                    if (item->name)
                        screenMessage("You find...\n%s!\n", item->name);
                    (*item->putItemInInventory)(item->data);
                }
            } else
                screenMessage("Nothing Here!\n");
        }

        break;

    case 't':
        if (c->saveGame->balloonstate)
            screenMessage("Talk\nDrift only!\n");
        else {
            info = new CoordActionInfo;
            info->handleAtCoord = &talkAtCoord;
            info->origin = c->location->coords;            
            info->prev = MapCoords(-1, -1);
            info->range = 2;
            info->validDirections = MASK_DIR_ALL;
            info->player = -1;
            info->blockedPredicate = &tileCanTalkOver;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandlerPushKeyHandlerWithData(&gameGetCoordinateKeyHandler, info);
            screenMessage("Talk\nDir: ");
        }
        break;

    case 'u':
        screenMessage("Use which item:\n");
        gameGetInput(&useItem, &itemNameBuffer);

        if (settings.enhancements) {
            /* a little xu4 enhancement: show items in inventory when prompted for an item to use */
            c->statsView = STATS_ITEMS;
            statsUpdate();
        }
        break;

    case 'v':
        if (musicToggle())
            screenMessage("Volume On!\n");
        else
            screenMessage("Volume Off!\n");
        break;

    case 'w':
        screenMessage("Wear Armour\nfor: ");
        gameGetPlayerForCommand(&wearForPlayer, 1, 0);
        break;

    case 'x':
        if ((c->transportContext != TRANSPORT_FOOT) && c->saveGame->balloonstate == 0) {
            Object *obj = c->location->map->addObject((MapTile)c->saveGame->transport, (MapTile)c->saveGame->transport, c->location->coords);
            if (c->transportContext == TRANSPORT_SHIP)
                c->lastShip = obj;

            gameSetTransport(AVATAR_TILE);
            c->horseSpeed = 0;
            screenMessage("X-it\n");
        } else
            screenMessage("X-it What?\n");
        break;

    case 'y':
        screenMessage("Yell ");
        if (c->transportContext == TRANSPORT_HORSE) {
            if (c->horseSpeed == 0) {
                screenMessage("Giddyup!\n");
                c->horseSpeed = 1;
            } else {
                screenMessage("Whoa!\n");
                c->horseSpeed = 0;
            }
        } else
            screenMessage("what?\n");
        break;

    case 'z':        
        screenMessage("Ztats for: ");
        gameGetPlayerForCommand(&ztatsFor, 1, 0);
        break;

    case 'c' + U4_ALT:
        if (settings.debug && c->location->map->isWorldMap()) {
            /* first teleport to the abyss */
            c->location->coords.x = 0xe9;
            c->location->coords.y = 0xe9;
            gameSetMap(mapMgrGetById(MAP_ABYSS), 1, NULL);
            /* then to the final altar */
            c->location->coords.x = 7;
            c->location->coords.y = 7;
            c->location->coords.z = 7;            
        }
        break;
    
    case 'h' + U4_ALT:
        screenMessage("Key Reference:\n"
                      "Arrow Keys: Move\n"
                      "a: Attack\n"
                      "b: Board\n"
                      "c: Cast Spell\n"
                      "d: Descend\n"
                      "e: Enter\n"
                      "f: Fire\n"
                      "g: Get Chest\n"
                      "h: Hole up\n"
                      "i: Ignite torch\n"
                      "(more)");
        eventHandlerPushKeyHandler(&helpPage2KeyHandler);
        break;

    case 'q' + U4_ALT:
        {
            /* FIXME: return to main menu */            
        }
        break;

    case 'v' + U4_ALT:
        screenMessage("XU4 %s\n", VERSION);        
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
        else screenMessage("Bad command!\n");

        endTurn = 0;
        break;
        
    default:
        valid = false;
        break;
    }

    if (valid && endTurn) {
        if (eventHandlerGetKeyHandler() == &gameBaseKeyHandler &&
            c->location->finishTurn == &gameFinishTurn)
            (*c->location->finishTurn)();
    }
    else if (!endTurn) {
        /* if our turn did not end, then manually redraw the text prompt */    
        screenPrompt();
        screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);    
        statsUpdate();
    }

    return valid || keyHandlerDefault(key, NULL);
}

void gameGetInput(int (*handleBuffer)(string*), string *buffer, int bufferlen) {
    ReadBufferActionInfo *readBufferInfo;

    screenEnableCursor();
    screenShowCursor();

    if (!buffer)
        errorFatal("Error: call to gameGetInput() with an invalid input buffer.");

    /* clear out the input buffer */
    buffer->erase();

    readBufferInfo = new ReadBufferActionInfo;
    readBufferInfo->handleBuffer = handleBuffer; 
    readBufferInfo->buffer = buffer;    
    readBufferInfo->bufferLen = bufferlen+1;
    readBufferInfo->screenX = TEXT_AREA_X + c->col;
    readBufferInfo->screenY = TEXT_AREA_Y + c->line;

    eventHandlerPushKeyHandlerWithData(&keyHandlerReadBuffer, readBufferInfo);
}

void gameGetPlayerForCommand(bool (*commandFn)(int player), int canBeDisabled, int canBeActivePlayer) {
    if (c->saveGame->members <= 1) {
        screenMessage("1\n");
        (*commandFn)(0);
    }
    else {
        GetPlayerInfo *info = new GetPlayerInfo;
        info->canBeDisabled = canBeDisabled;        
        info->command = commandFn;

        eventHandlerPushKeyHandlerWithData(&gameGetPlayerNoKeyHandler, (void *)info);
        if (canBeActivePlayer && (c->location->activePlayer >= 0))
            gameGetPlayerNoKeyHandler(c->location->activePlayer + '1', (void *)info);        
    }
}

/**
 * Handles key presses for a command requiring a player number
 * argument.  Once a number key is pressed, control is handed off to a
 * command specific routine.
 */
bool gameGetPlayerNoKeyHandler(int key, void *data) {
    GetPlayerInfo *info = (GetPlayerInfo*)data;    
    bool valid = true;

    eventHandlerPopKeyHandler();

    if (key >= '1' &&
        key <= ('0' + c->saveGame->members)) {
        screenMessage("%c\n", key);
        if (!info->canBeDisabled && playerIsDisabled(key - '1'))
            screenMessage("\nDisabled!\n");
        else (*info->command)(key - '1');
    } else {
        screenMessage("None\n");
        (*c->location->finishTurn)();
        valid = false;
    }

    eventHandlerPopKeyHandlerData();
    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a letter argument.
 * Once a valid key is pressed, control is handed off to a command
 * specific routine.
 */
bool gameGetAlphaChoiceKeyHandler(int key, void *data) {
    AlphaActionInfo *info = (AlphaActionInfo *) data;
    bool valid = true;

    if (islower(key))
        key = toupper(key);

    if (key >= 'A' && key <= toupper(info->lastValidLetter)) {
        screenMessage("%c\n", key);
        eventHandlerPopKeyHandler();
        (*(info->handleAlpha))(key - 'A', info->data);
        eventHandlerPopKeyHandlerData();
    } else if (key == U4_SPACE || key == U4_ESC || key == U4_ENTER) {
        screenMessage("\n");
        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();
        (*c->location->finishTurn)();
    } else {
        valid = false;
        screenMessage("\n%s", info->prompt.c_str());
        screenRedrawScreen();
    }

    return valid || keyHandlerDefault(key, NULL);
}

bool gameGetDirectionKeyHandler(int key, void *data) {
    int (*handleDirection)(Direction dir) = (int(*)(Direction))data;    
    Direction dir = keyToDirection(key);    
    bool valid = (dir != DIR_NONE) ? true : false;
    
    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();

        screenMessage("\n");
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandlerPopKeyHandler();

            screenMessage("%s\n", getDirectionName(dir));
            (*handleDirection)(dir);
            eventHandlerPopKeyHandlerData();
        }
        break;
    }    

    return valid || keyHandlerDefault(key, NULL);
}

bool gameGetFieldTypeKeyHandler(int key, void *data) {
    int (*handleFieldType)(int field) = (int(*)(int))data;    
    fieldType = ENERGYFIELD_NONE;

    eventHandlerPopKeyHandler();
    
    switch(tolower(key)) {
    case 'f': fieldType = ENERGYFIELD_FIRE; break;
    case 'l': fieldType = ENERGYFIELD_LIGHTNING; break;
    case 'p': fieldType = ENERGYFIELD_POISON; break;
    case 's': fieldType = ENERGYFIELD_SLEEP; break;
    default: break;
    }
    
    if (fieldType != ENERGYFIELD_NONE) {
        screenMessage("%c\n", toupper(key));
        (*handleFieldType)((int)fieldType);
        eventHandlerPopKeyHandlerData();
        return true;
    } else {
        /* Invalid input here = spell failure */
        screenMessage("Failed!\n");
        /* 
         * Confirmed both mixture loss and mp loss in this situation in the 
         * original Ultima IV (at least, in the Amiga version.) 
         */
        c->saveGame->mixtures[castSpell]--;
        c->players[castPlayer].mp -= spellGetRequiredMP(castSpell);
        (*c->location->finishTurn)();
    }

    eventHandlerPopKeyHandlerData();
    
    return false;
}

bool gameGetPhaseKeyHandler(int key, void *data) {    
    int (*handlePhase)(int) = (int(*)(int))data;
    bool valid = true;

    eventHandlerPopKeyHandler();

    if (key >= '1' && key <= '8') {
        screenMessage("%c\n", key);
        (*handlePhase)(key - '1');
    } else {
        screenMessage("None\n");
        (*c->location->finishTurn)();
        valid = false;
    }

    eventHandlerPopKeyHandlerData();

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles key presses for a command requiring a direction argument.
 * Once an arrow key is pressed, control is handed off to a command
 * specific routine.
 */
bool gameGetCoordinateKeyHandler(int key, void *data) {
    CoordActionInfo *info = (CoordActionInfo *) data;
    Direction dir = keyToDirection(key);
    bool valid = (dir != DIR_NONE) ? true : false;
    info->dir = MASK_DIR(dir);

    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();

        screenMessage("\n");        
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandlerPopKeyHandler();
            screenMessage("%s\n", getDirectionName(dir));
            gameDirectionalAction(info);
            eventHandlerPopKeyHandlerData();
        }        
        break;
    }    

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Handles spell mixing for the Ultima V-style menu-system
 */
bool gameSpellMixMenuKeyHandler(int key, void *data) {
    Menu *menu = (Menu *)data;    
    
    switch(key) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
        /* select the corresponding reagent */
        menu->setCurrent(menu->getById((MenuId)(key-'a')));
        gameSpellMixMenuKeyHandler(U4_SPACE, menu);        
        break;
    case U4_UP:
        menu->prev();
        break;
    case U4_DOWN:
        menu->next();
        break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_SPACE:
        if (menu->isVisible()) {            
            MenuItem *item = &(*menu->getCurrent());
            
            /* change whether or not it's selected */
            item->isSelected = !item->isSelected;
                        
            if (item->isSelected)
                mixtureAddReagent(mix, (Reagent)item->id);
            else mixtureRemoveReagent(mix, (Reagent)item->id);
        }
        break;
    case U4_ENTER:
        /* you have reagents! */
        if (menu->isVisible())
        {
            screenHideCursor();
            eventHandlerPopKeyHandler();
            c->statsView = STATS_MIXTURES;
            statsUpdate();
         
            screenMessage("How many? ");
            
            howmany.erase();
            gameGetInput(&gameSpellMixHowMany, &howmany, 2);
        }
        /* you don't have any reagents */
        else {
            eventHandlerPopKeyHandler();
            eventHandlerPopKeyHandlerData();
            c->statsView = STATS_PARTY_OVERVIEW;
            statsUpdate();
            screenEnableCursor();
            (*c->location->finishTurn)();
        }
        return true;

    case U4_ESC:
        eventHandlerPopKeyHandler();
        eventHandlerPopKeyHandlerData();

        mixtureRevert(mix);
        mixtureDelete(mix);

        screenHideCursor();
        c->statsView = STATS_PARTY_OVERVIEW;
        statsUpdate();
        screenMessage("\n");
        
        screenEnableCursor();
        (*c->location->finishTurn)();
    default:
        return false;
    }
    
    statsUpdate();
    return true;
}

void gameResetSpellMixing(void) {
    Menu::MenuItemList::iterator current;
    int i, row;    

    i = 0;
    row = 0;
    for (current = spellMixMenu.begin(); current != spellMixMenu.end(); current++) {    
        if (c->saveGame->reagents[i++] > 0) {
            current->isVisible = true;
            current->y = STATS_AREA_Y + row;
            row++;
        }
        else current->isVisible = false;
    }

    spellMixMenu.reset();
}

int gameSpellMixHowMany(string *message) {
    int i, num;
    
    eventHandlerPopKeyHandler();

    num = (int) strtol(message->c_str(), NULL, 10);
    
    /* entered 0 mixtures, don't mix anything! */
    if (num == 0) {
        screenMessage("\nNone mixed!\n");
        mixtureRevert(mix);
        mixtureDelete(mix);
        (*c->location->finishTurn)();
        return 0;
    }
    
    /* if they ask for more than will give them 99, only use what they need */
    if (num > 99 - c->saveGame->mixtures[mixSpell]) {
        num = 99 - c->saveGame->mixtures[mixSpell];
        screenMessage("\nOnly need %d!", num);
    }
    
    screenMessage("\nMixing %d...\n", num);

    for (i = 0; i < REAG_MAX; i++) {
        /* see if there's enough reagents to mix (-1 because one is being mixed now) */
        if (mix->reagents[i] > 0 && c->saveGame->reagents[i] < num-1) {
            screenMessage("\nYou don't have enough reagents to mix %d spells!\n\n", num);
            mixtureRevert(mix);
            mixtureDelete(mix);
            (*c->location->finishTurn)();
            return 0;
        }
    }    
       
    screenMessage("\nYou mix the Reagents, and...\n");
    if (spellMix(mixSpell, mix)) {
        screenMessage("Success!\n\n");
        /* mix the extra spells */
        for (i = 0; i < num-1; i++)
            spellMix(mixSpell, mix);
        /* subtract the reagents from inventory */
        for (i = 0; i < REAG_MAX; i++) {
            if (mix->reagents[i] > 0)
                c->saveGame->reagents[i] -= num-1;
        }
    }
    else 
        screenMessage("It Fizzles!\n\n");

    mixtureDelete(mix);

    (*c->location->finishTurn)();
    return 1;        
}

/**
 * Handles key presses while Ztats are being displayed.
 */
bool gameZtatsKeyHandler(int key, void *data) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
        statsPrevItem();
        break;
    case U4_DOWN:
    case U4_RIGHT:
        statsNextItem();
        break;
    default:
        eventHandlerPopKeyHandler();
        (*c->location->finishTurn)();
        break;
    }

    statsUpdate();

    return true;
}

bool gameSpecialCmdKeyHandler(int key, void *data) {
    int i;
    const MapCoords *moongate;
    bool valid = true;

    switch (key) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        screenMessage("Gate %d!\n", key - '0');

        if (c->location->map->isWorldMap()) {
            moongate = moongateGetGateCoordsForPhase(key - '1');
            if (moongate)
                c->location->coords = *moongate;                
        }
        else screenMessage("Not here!\n");
        screenPrompt();
        break;

    case 'a':
        {
            int newTrammelphase = c->saveGame->trammelphase + 1;
            if (newTrammelphase > 7)
                newTrammelphase = 0;

            screenMessage("Advance Moons!\n");
            while (c->saveGame->trammelphase != newTrammelphase)
                gameUpdateMoons(1);

            screenPrompt();
        }
        break;

    case 'c':
        collisionOverride = !collisionOverride;
        screenMessage("Collision detection %s!\n", collisionOverride ? "off" : "on");
        screenPrompt();
        break;

    case 'e':
        {
            extern int numWeapons;            

            screenMessage("Equipment!\n");
            screenPrompt();
            for (i = ARMR_NONE + 1; i < ARMR_MAX; i++)
                c->saveGame->armor[i] = 8;
            for (i = WEAP_HANDS + 1; i < numWeapons; i++) {
                if (weaponLoseWhenUsed(i) || weaponLoseWhenRanged(i))
                    c->saveGame->weapons[i] = 99;
                else c->saveGame->weapons[i] = 8;
            }
        }
        break;

    case 'h':
        screenMessage("Help:\n"
                      "1-8   - Gate\n"
                      "F1-F8 - +Virtue\n"
                      "a - Adv. Moons\n"
                      "c - Collision\n"
                      "e - Equipment\n"
                      "h - Help\n"
                      "i - Items\n"
                      "k - Show Karma\n"
                      "l - Location\n"
                      "m - Mixtures\n"                      
                      "(more)");
        eventHandlerPopKeyHandler();
        eventHandlerPushKeyHandler(&cmdHandleAnyKey);
        return true;

    case 'i':
        screenMessage("Items!\n");
        screenPrompt();
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->sextants = 1;
        c->saveGame->items = ITEM_SKULL | ITEM_CANDLE | ITEM_BOOK | ITEM_BELL | ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T | ITEM_HORN | ITEM_WHEEL;
        c->saveGame->stones = 0xff;
        c->saveGame->runes = 0xff;
        c->saveGame->food = 999900;
        c->saveGame->gold = 9999;
        statsUpdate();
        break;

    case 'k':
        screenMessage("Karma!\n\n");
        for (i = 0; i < 8; i++) {
            unsigned int j;
            screenMessage("%s:", getVirtueName((Virtue)i));
            for (j = 13; j > strlen(getVirtueName((Virtue)i)); j--)
                screenMessage(" ");
            if (c->saveGame->karma[i] > 0)                
                screenMessage("%.2d\n", c->saveGame->karma[i]);            
            else screenMessage("--\n");
        }
        screenPrompt();

        break;

    case 'l':
        if (c->location->map->isWorldMap())
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\n", "World Map", c->location->coords.x, c->location->coords.y);
        else
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\nz: %d\n", c->location->map->getName().c_str(), c->location->coords.x, c->location->coords.y, c->location->coords.z);
        screenPrompt();
        break;

    case 'm':
        screenMessage("Mixtures!\n");
        screenPrompt();
        for (i = 0; i < SPELL_MAX; i++)
            c->saveGame->mixtures[i] = 99;
        break;    

    case 'o':
        c->opacity = !c->opacity;
        screenMessage("Opacity %s!\n", c->opacity ? "on" : "off");
        screenPrompt();
        break;

    case 'p':        
        screenMessage("\nPeer at a Gem!\n");
        eventHandlerPopKeyHandler();
        gamePeerGem();
        return true;

    case 'r':
        screenMessage("Reagents!\n");
        screenPrompt();
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;

    case 's':
        screenMessage("Summon!\n");
        eventHandlerPopKeyHandler();

        screenMessage("What?\n");
        gameGetInput(&gameSummonMonster, &monsterNameBuffer);
        
        return true;

    case 't':
        if (c->location->map->isWorldMap()) {
            c->location->map->addObject(tileGetHorseBase(), tileGetHorseBase(), MapCoords(84, 106));
            c->location->map->addObject(tileGetShipBase(), tileGetShipBase(), MapCoords(88, 109));
            c->location->map->addObject(tileGetBalloonBase(), tileGetBalloonBase(), MapCoords(85, 105));
            screenMessage("Transports: Ship, Horse and Balloon created!\n");
            screenPrompt();
        }
        break;

    case 'v':
        screenMessage("\nFull Virtues!\n");
        for (i = 0; i < 8; i++)
            c->saveGame->karma[i] = 0;
        statsUpdate();
        screenPrompt();
        break;

    case 'w':        
        screenMessage("Wind Dir ('l' to lock):\n");
        eventHandlerPopKeyHandler();
        eventHandlerPushKeyHandler(&windCmdKeyHandler);
        return true;

    case 'x':
        screenMessage("\nX-it!\n");        
        if (!gameExitToParentMap())
            screenMessage("Not Here!\n");
        musicPlay();
        screenPrompt();
        break;

    case 'y':
        screenMessage("Y-up!\n");
        if ((c->location->context & CTX_DUNGEON) && (c->location->coords.z > 0))
            c->location->coords.z--;
        else {
            screenMessage("Leaving...\n");
            gameExitToParentMap();
            musicPlay();
        }
        screenPrompt();
        break;

    case 'z':
        screenMessage("Z-down!\n");
        if ((c->location->context & CTX_DUNGEON) && (c->location->coords.z < 7))
            c->location->coords.z++;
        else screenMessage("Not Here!\n");
        screenPrompt();
        break;

    case U4_FKEY+0:
    case U4_FKEY+1:
    case U4_FKEY+2:
    case U4_FKEY+3:
    case U4_FKEY+4:
    case U4_FKEY+5:
    case U4_FKEY+6:
    case U4_FKEY+7:
        screenMessage("Improve %s!\n", getVirtueName((Virtue)(key - U4_FKEY)));
        if (c->saveGame->karma[key - U4_FKEY] == 99)
            c->saveGame->karma[key - U4_FKEY] = 0;
        else if (c->saveGame->karma[key - U4_FKEY] != 0)
            c->saveGame->karma[key - U4_FKEY] += 10;
        if (c->saveGame->karma[key - U4_FKEY] > 99)
            c->saveGame->karma[key - U4_FKEY] = 99;
        statsUpdate();
        screenPrompt();
        break;

    case U4_ESC:
    case U4_ENTER:
    case U4_SPACE:
        screenMessage("Nothing\n");
        screenPrompt();
        break;

    default:
        valid = false;
        break;
    }

    if (valid)
        eventHandlerPopKeyHandler();

    return valid || keyHandlerDefault(key, NULL);
}

bool cmdHandleAnyKey(int key, void *data) {
    eventHandlerPopKeyHandler();

    screenMessage("\n"
                  "o - Opacity\n"
                  "p - Peer\n"
                  "r - Reagents\n"
                  "s - Summon\n"
                  "t - Transports\n"
                  "w - Change Wind\n"
                  "x - Exit Map\n"
                  "y - Y-up\n"
                  "z - Z-down\n"
                  );
    screenPrompt();
    return true;
}

bool windCmdKeyHandler(int key, void *data) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
    case U4_DOWN:
    case U4_RIGHT:
        c->windDirection = keyToDirection(key);
        screenMessage("Wind %s!\n", getDirectionName((Direction)c->windDirection));
        break;

    case 'l':
        windLock = !windLock;
        screenMessage("Wind direction is %slocked!\n", windLock ? "" : "un");
        break;
    }

    eventHandlerPopKeyHandler();
    statsUpdate();
    screenPrompt();

    return true;
}

bool helpPage2KeyHandler(int key, void *data) {
    eventHandlerPopKeyHandler();

    screenMessage("\n"
                    "j: Jimmy lock\n"
                    "k: Klimb\n"
                    "l: Locate\n"
                    "m: Mix reagents\n"
                    "n: New Order\n"
                    "o: Open door\n"
                    "p: Peer at Gem\n"
                    "q: Quit & Save\n"
                    "r: Ready weapon\n"
                    "s: Search\n"
                    "t: Talk\n"
                "(more)");
    eventHandlerPushKeyHandler(&helpPage3KeyHandler);
    return true;
}

bool helpPage3KeyHandler(int key, void *data) {
    eventHandlerPopKeyHandler();

    screenMessage("\n"
                    "u: Use Item\n"
                    "v: Volume On/Off\n"
                    "w: Wear armour\n"
                    "x: eXit\n"
                    "y: Yell\n"
                    "z: Ztats\n"
                    "Space: Pass\n"
                    "Alt-V: Version\n"
                    "Alt-X: Quit\n"
                    );
    screenPrompt();
    return true;
}


/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
bool attackAtCoord(MapCoords coords, int distance, void *data) {
    Object *under;
    MapTile ground;    
    Monster *m;

    /* attack failed: finish up */
    if (coords.x == -1 && coords.y == -1) {        
        screenMessage("Nothing to Attack!\n");
        (*c->location->finishTurn)();
        return false;
    }

    m = dynamic_cast<Monster*>(c->location->map->objectAt(coords));
    /* nothing attackable: move on to next tile */
    if ((m == NULL) || 
        /* can't attack horse transport */
        (tileIsHorse(m->getTile()) && m->getMovementBehavior() == MOVEMENT_FIXED)) {
        return false;
    }

    /* attack successful */
    ground = c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS);
    if ((under = c->location->map->objectAt(c->location->coords)) &&
        tileIsShip(under->getTile()))
        ground = under->getTile();

    /* You're attacking a townsperson!  Alert the guards! */
    if ((m->getType() == OBJECT_PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))
        gameAlertTheGuards(c->location->map);        

    /* not good karma to be killing the innocent.  Bad avatar! */    
    if (m->isGood() || /* attacking a good monster */
        /* attacking a docile (although possibly evil) person in town */
        ((m->getType() == OBJECT_PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))) 
        playerAdjustKarma(KA_ATTACKED_GOOD);

    combatInit(m, combatMapForTile(ground, (MapTile)c->saveGame->transport, m));
    combatBegin();
    return true;
}

bool gameCastForPlayer(int player) {
    AlphaActionInfo *info;

    castPlayer = player;

    if (gameCheckPlayerDisabled(player)) {
        (*c->location->finishTurn)();
        return false;
    }

    c->statsView = STATS_MIXTURES;
    statsUpdate();

    info = new AlphaActionInfo;
    info->lastValidLetter = 'z';
    info->handleAlpha = castForPlayer2;
    info->prompt = "Spell: ";
    info->data = NULL;

    screenMessage("%s", info->prompt.c_str());

    eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, info);

    return true;
}

bool castForPlayer2(int spell, void *data) {    
    castSpell = spell;

    screenMessage("%s!\n", spellGetName(spell));    

    c->statsView = STATS_PARTY_OVERVIEW;
    statsUpdate();

    /* If we can't really cast this spell, skip the extra parameters */
    if ((spellGetRequiredMP(spell) > c->players[castPlayer].mp) || /* not enough mp */
        ((spellGetContext(spell) & c->location->context) == 0) ||            /* wrong context */
        (c->saveGame->mixtures[spell] == 0) ||                               /* none mixed! */
        ((spellGetTransportContext(spell) & c->transportContext) == 0)) {    /* invalid transportation for spell */
        
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        return true;
    }

    /* Get the final parameters for the spell */
    switch (spellGetParamType(spell)) {
    case SPELLPRM_NONE:
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        break;
    case SPELLPRM_PHASE:
        screenMessage("To Phase: ");
        eventHandlerPushKeyHandlerWithData(&gameGetPhaseKeyHandler, (void *) &castForPlayerGetPhase);        
        break;
    case SPELLPRM_PLAYER:
        screenMessage("Who: ");
        gameGetPlayerForCommand(&castForPlayerGetDestPlayer, 1, 0);        
        break;
    case SPELLPRM_DIR:
        if (c->location->context == CTX_DUNGEON)
            gameCastSpell(castSpell, castPlayer, c->saveGame->orientation);
        else {
            screenMessage("Dir: ");
            eventHandlerPushKeyHandlerWithData(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir);
        }
        break;
    case SPELLPRM_TYPEDIR:
        screenMessage("Energy type? ");
        eventHandlerPushKeyHandlerWithData(&gameGetFieldTypeKeyHandler, (void *) &castForPlayerGetEnergyType);
        break;
    case SPELLPRM_FROMDIR:
        screenMessage("From Dir: ");
        eventHandlerPushKeyHandlerWithData(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir);
        break;
    }    

    return true;
}

bool castForPlayerGetDestPlayer(int player) {
    gameCastSpell(castSpell, castPlayer, player);
    (*c->location->finishTurn)();
    return true;
}

int castForPlayerGetDestDir(Direction dir) {
    gameCastSpell(castSpell, castPlayer, (int) dir);
    (*c->location->finishTurn)();
    return 1;
}

int castForPlayerGetPhase(int phase) {
    gameCastSpell(castSpell, castPlayer, phase);
    (*c->location->finishTurn)();
    return 1;
}

int castForPlayerGetEnergyType(int fieldType) {
    /* Need a direction */
    if (c->location->context == CTX_DUNGEON)
        castForPlayerGetEnergyDir((Direction)c->saveGame->orientation);
    else {
        screenMessage("Dir: ");
        eventHandlerPushKeyHandlerWithData(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetEnergyDir);
    }
    return 1;
}

int castForPlayerGetEnergyDir(Direction dir) {
    int param;
    
    /* Need to pack both dir and fieldType into param */
    param = fieldType << 4;
    param |= (int) dir;
    
    gameCastSpell(castSpell, castPlayer, param);
    (*c->location->finishTurn)();
    return 1;
}

bool destroyAtCoord(MapCoords coords, int distance, void *data) {
    Object *obj = c->location->map->objectAt(coords);

    screenPrompt();

    if (obj) {
        c->location->map->removeObject(obj);
        return true;
    }
    return false;
}

bool fireAtCoord(MapCoords coords, int distance, void *data) {
    
    CoordActionInfo* info = (CoordActionInfo*)data;
    MapCoords old = info->prev;
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    int validObject = 0;
    int hitsAvatar = 0;
    int originAvatar = (c->location->coords == info->origin);
    
    info->prev = coords;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        c->location->map->annotations->remove(old, MISSFLASH_TILE);
    
    if (coords.x == -1 && coords.y == -1) {
        if (distance == 0)
            screenMessage("Broadsides Only!\n");

        /* The avatar's ship was firing */
        if (originAvatar)
            (*c->location->finishTurn)();

        return true;
    }
    else {
        Object *obj = NULL;

        obj = c->location->map->objectAt(coords);
        Monster *m = dynamic_cast<Monster*>(obj);
                
        /* FIXME: there's got to be a better way make whirlpools and storms impervious to cannon fire */
        if (obj && (obj->getType() == OBJECT_MONSTER) && 
            (m->id != WHIRLPOOL_ID) && (m->id != STORM_ID))
            validObject = 1;        
        /* See if it's an object to be destroyed (the avatar cannot destroy the balloon) */
        else if (obj && (obj->getType() == OBJECT_UNKNOWN) && !(tileIsBalloon(obj->getTile()) && originAvatar))
            validObject = 1;
        
        /* Does the cannon hit the avatar? */
        if (coords == c->location->coords) {
            validObject = 1;
            hitsAvatar = 1;
        }        

        if (validObject)
        {
            /* always displays as a 'hit' though the object may not be destroyed */                        
            
            /* Is is a pirate ship firing at US? */
            if (hitsAvatar) {
                attackFlash(coords, HITFLASH_TILE, 5);

                if (c->transportContext == TRANSPORT_SHIP)
                    gameDamageShip(-1, 10);
                else gameDamageParty(10, 25); /* party gets hurt between 10-25 damage */
            }          
            /* inanimate objects get destroyed instantly, while monsters get a chance */
            else if (obj->getType() == OBJECT_UNKNOWN) {
                attackFlash(coords, HITFLASH_TILE, 5);
                c->location->map->removeObject(obj);
            }
            
            /* only the avatar can hurt other monsters with cannon fire */
            else if (originAvatar) {
                attackFlash(coords, HITFLASH_TILE, 5);
                if (xu4_random(4) == 0) /* reverse-engineered from u4dos */
                    c->location->map->removeObject(obj);
            }
            
            if (originAvatar)
                (*c->location->finishTurn)();

            return true;
        }
        
        c->location->map->annotations->add(coords, MISSFLASH_TILE, true);
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 4);
    }

    return false;
}

/**
 * Get the chest at the current x,y of the current context for player 'player'
 */
bool gameGetChest(int player) {
    Object *obj;
    MapTile tile, newTile;
    MapCoords coords;    
    
    locationGetCurrentPosition(c->location, &coords);
    tile = c->location->map->tileAt(coords, WITH_GROUND_OBJECTS);
    newTile = locationGetReplacementTile(c->location, coords);    
    
    /* get the object for the chest, if it is indeed an object */
    obj = c->location->map->objectAt(coords);
    if (obj && !tileIsChest(obj->getTile()))
        obj = NULL;
    
    if (tileIsChest(tile)) {
        if (obj)
            c->location->map->removeObject(obj);
        else
            c->location->map->annotations->add(coords, newTile);
        
        /* see if the chest is trapped and handle it */
        getChestTrapHandler(player);
        screenMessage("The Chest Holds: %d Gold\n", playerGetChest());

        statsUpdate();
        
        if (isCity(c->location->map) && obj == NULL)
            playerAdjustKarma(KA_STOLE_CHEST);
    }    
    else
        screenMessage("Not Here!\n");
    
    return true;
}

/**
 * Called by gameGetChest() to handle possible traps on chests
 **/
bool getChestTrapHandler(int player) {            
    TileEffect trapType;
    int dex = c->players[player].dex;
    int randNum = xu4_random(4);
    int member = player;
    
    /* Do we use u4dos's way of trap-determination, or the original intended way? */
    int passTest = (settings.enhancements && settings.enhancementsOptions.c64chestTraps) ?
        (xu4_random(2) == 0) : /* xu4-enhanced */
        ((randNum & 1) == 0); /* u4dos original way (only allows even numbers through, so only acid and poison show) */

    /* Chest is trapped! 50/50 chance */
    if (passTest)
    {   
        /* Figure out which trap the chest has */
        switch(randNum & xu4_random(4)) {
        case 0: trapType = EFFECT_FIRE; break;   /* acid trap (56% chance - 9/16) */
        case 1: trapType = EFFECT_SLEEP; break;  /* sleep trap (19% chance - 3/16) */
        case 2: trapType = EFFECT_POISON; break; /* poison trap (19% chance - 3/16) */
        case 3: trapType = EFFECT_LAVA; break;   /* bomb trap (6% chance - 1/16) */
        default: trapType = EFFECT_FIRE; break;
        }

        /* apply the effects from the trap */
        if (trapType == EFFECT_FIRE)
            screenMessage("Acid Trap!\n");            
        else if (trapType == EFFECT_POISON)
            screenMessage("Poison Trap!\n");            
        else if (trapType == EFFECT_SLEEP)
            screenMessage("Sleep Trap!\n");            
        else if (trapType == EFFECT_LAVA) {
            screenMessage("Bomb Trap!\n");
            member = ALL_PLAYERS;            
        }

        /* See if the trap was evaded! */           
        if ((dex + 25) < xu4_random(100) &&         /* test player's dex */            
            (player >= 0)) {                        /* player is < 0 during the 'O'pen spell (immune to traps) */                         
            playerApplyEffect(trapType, member);
            statsUpdate();
        }
        else screenMessage("Evaded!\n");

        return true;
    }

    return false;
}

/**
 * Handles moving the avatar during normal 3rd-person view 
 */
MoveReturnValue gameMoveAvatar(Direction dir, int userEvent) {       
    MoveReturnValue retval = moveAvatar(dir, userEvent);  /* move the avatar */

    if (userEvent) {

        if (!settings.filterMoveMessages) {
            if (retval & MOVE_TURNED)
                screenMessage("Turn %s!\n", getDirectionName(dir));
            else if (c->transportContext == TRANSPORT_SHIP)
                screenMessage("Sail %s!\n", getDirectionName(dir));    
            else if (c->transportContext != TRANSPORT_BALLOON)
                screenMessage("%s\n", getDirectionName(dir));    
        }

        /* movement was blocked */
        if (retval & MOVE_BLOCKED) {

            /* if shortcuts are enabled, try them! */
            if (settings.shortcutCommands) {
                MapCoords new_coords = c->location->coords;
                MapTile tile;
                
                new_coords.move(dir, c->location->map);                
                tile = c->location->map->tileAt(new_coords, WITH_OBJECTS);

                if (tileIsDoor(tile)) {
                    openAtCoord(new_coords, 1, NULL);
                    retval = (MoveReturnValue)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } else if (tileIsLockedDoor(tile)) {
                    jimmyAtCoord(new_coords, 1, NULL);
                    retval = (MoveReturnValue)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } /*else if (mapPersonAt(c->location->map, new_coords) != NULL) {
                    talkAtCoord(newx, newy, 1, NULL);
                    retval = MOVE_SUCCEEDED | MOVE_END_TURN;
                    }*/
            }

            /* if we're still blocked */
            if ((retval & MOVE_BLOCKED) && !settings.filterMoveMessages) {
                screenMessage("Blocked!\n");
            }
        }

        /* play an approriate sound effect */
        if (retval & MOVE_BLOCKED)
            soundPlay(SOUND_BLOCKED);
        else if (c->transportContext == TRANSPORT_FOOT || c->transportContext == TRANSPORT_HORSE)
            soundPlay(SOUND_WALK);
    }

    /* movement was slowed */
    if (retval & MOVE_SLOWED)
        screenMessage("Slow progress!\n");        

    /* exited map */
    if (retval & MOVE_EXIT_TO_PARENT) {
        screenMessage("Leaving...\n");
        gameExitToParentMap();
        musicPlay();
    }

    /* things that happen while not on board the balloon */
    if (c->transportContext & ~TRANSPORT_BALLOON)
        gameCheckSpecialMonsters(dir);
    /* things that happen while on foot or horseback */
    if (c->transportContext & TRANSPORT_FOOT_OR_HORSE) {
        if (gameCheckMoongates())
            retval = (MoveReturnValue)(MOVE_MAP_CHANGE | MOVE_END_TURN);
    }

    return retval;
}

/**
 * Handles moving the avatar in the 3-d dungeon view
 */
MoveReturnValue gameMoveAvatarInDungeon(Direction dir, int userEvent) {

    MoveReturnValue retval = moveAvatarInDungeon(dir, userEvent);  /* move the avatar */
    Direction realDir = dirNormalize((Direction)c->saveGame->orientation, dir);

    if (!settings.filterMoveMessages) {
        if (userEvent) {
            if (retval & MOVE_TURNED) {
                if (dirRotateCCW((Direction)c->saveGame->orientation) == realDir)
                    screenMessage("Turn Left\n");
                else screenMessage("Turn Right\n");
            }
            /* show 'Advance' or 'Retreat' in dungeons */
            else screenMessage("%s\n", realDir == c->saveGame->orientation ? "Advance" : "Retreat");
        }

        if (retval & MOVE_BLOCKED)
            screenMessage("Blocked!\n");       
    }

    /* if we're exiting the map, do this */
    if (retval & MOVE_EXIT_TO_PARENT) {
        screenMessage("Leaving...\n");
        gameExitToParentMap();
        musicPlay();
    }

    /* check to see if we're entering a dungeon room */
    if (retval & MOVE_SUCCEEDED) {
        if (dungeonCurrentToken() == DUNGEON_ROOM) {            
            int room = (int)dungeonCurrentSubToken(); /* get room number */
        
            /**
             * recalculate room for the abyss -- there are 16 rooms for every 2 levels, 
             * each room marked with 0xD* where (* == room number 0-15).
             * for levels 1 and 2, there are 16 rooms, levels 3 and 4 there are 16 rooms, etc.
             */
            if (c->location->map->id == MAP_ABYSS)
                room = (0x10 * (c->location->coords.z/2)) + room;

            combatInitDungeonRoom(room, dirReverse(realDir));
            combatBegin();
        }
    }

    return retval;
}

/**
 * Attempts to jimmy a locked door at map coordinates x,y.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
bool jimmyAtCoord(MapCoords coords, int distance, void *data) {    
    MapTile tile;

    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Jimmy what?\n");
        (*c->location->finishTurn)();
        return false;
    }

    tile = c->location->map->tileAt(coords, WITH_OBJECTS);

    if (!tileIsLockedDoor(tile))
        return false;
        
    if (c->saveGame->keys) {
        c->saveGame->keys--;
        c->location->map->annotations->add(coords, 0x3b);
        screenMessage("\nUnlocked!\n");
    } else
        screenMessage("No keys left!\n");

    (*c->location->finishTurn)();

    return true;
}

/**
 * Readies a weapon for the given player.  Prompts the use for a
 * weapon.
 */
bool readyForPlayer(int player) {
    AlphaActionInfo *info;
    extern int numWeapons;

    c->statsView = STATS_WEAPONS;
    statsUpdate();

    info = new AlphaActionInfo;    
    info->lastValidLetter = numWeapons + 'a' - 1;
    info->handleAlpha = readyForPlayer2;
    info->prompt = "Weapon: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt.c_str());

    eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, info);

    return true;
}

bool readyForPlayer2(int w, void *data) {
    int player = (int) data;
    WeaponType weapon = (WeaponType) w, oldWeapon;
    string weaponName = *weaponGetName(weapon);

    // Return view to party overview
    c->statsView = STATS_PARTY_OVERVIEW;
    statsUpdate();

    if (weapon != WEAP_HANDS && c->saveGame->weapons[weapon] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return false;
    }

    if (!weaponCanReady(weapon, c->players[player].klass)) {
        string indef_article;

        switch(tolower(weaponName[0])) {
        case 'a':
        case 'e':
        case 'i':
        case 'o':
        case 'u':
        case 'y':
            indef_article = "an"; break;
        default: indef_article = "a"; break;
        }

        screenMessage("\nA %s may NOT use %s\n%s\n",
            getClassName(c->players[player].klass),
            indef_article.c_str(),
            weaponName.c_str());
        (*c->location->finishTurn)();
        return false;
    }

    oldWeapon = c->players[player].weapon;
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon != WEAP_HANDS)
        c->saveGame->weapons[weapon]--;
    c->players[player].weapon = weapon;

    screenMessage("%s\n", weaponName.c_str());

    (*c->location->finishTurn)();

    return true;
}

/**
 * Mixes reagents for a spell.  Prompts for reagents.
 */
bool mixReagentsForSpell(int spell, void *data) {
    GetChoiceActionInfo *info;    

    mixSpell = spell;
    mix = mixtureNew();    
    
    /* do we use the Ultima V menu system? */
    if (settings.enhancements && settings.enhancementsOptions.u5spellMixing) {
        screenMessage("%s\n", spellGetName(spell));
        screenDisableCursor();
        gameResetSpellMixing();
        spellMixMenu.reset();        
        eventHandlerPushKeyHandlerWithData(&gameSpellMixMenuKeyHandler, &spellMixMenu);
    }
    else {
        screenMessage("%s\nReagent: ", spellGetName(spell));    

        info = new GetChoiceActionInfo;
        info->choices = "abcdefgh\n\r \033";
        info->handleChoice = &mixReagentsForSpell2;
        eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, info);
    }

    c->statsView = STATS_REAGENTS;
    statsUpdate();

    return 0;
}

int mixReagentsForSpell2(int choice) {
    GetChoiceActionInfo *info;
    AlphaActionInfo *alphaInfo;

    eventHandlerPopKeyHandler();

    if (choice == '\n' || choice == '\r' || choice == ' ') {
        screenMessage("\n\nYou mix the Reagents, and...\n");

        if (spellMix(mixSpell, mix))
            screenMessage("Success!\n\n");
        else
            screenMessage("It Fizzles!\n\n");

        mixtureDelete(mix);

        screenMessage("Mix reagents\n");
        alphaInfo = new AlphaActionInfo;
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "For Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt.c_str());
        eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, alphaInfo);

        c->statsView = STATS_MIXTURES;
        statsUpdate();

        return 1;
    }

    else if (choice == '\033') {

        mixtureRevert(mix);
        mixtureDelete(mix);

        screenMessage("\n\n");
        (*c->location->finishTurn)();
        return 1;
    }

    else {
        screenMessage("%c\n", toupper(choice));

        if (mixtureAddReagent(mix, (Reagent)(choice - 'a')))
            statsUpdate();
        else
            screenMessage("None Left!\n");

        screenMessage("Reagent: ");

        info = new GetChoiceActionInfo;
        info->choices = "abcdefgh\n\r \033";
        info->handleChoice = &mixReagentsForSpell2;
        eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, info);


        return 1;
    }
}

/* FIXME: must be a better way.. */
int newOrderTemp;

/**
 * Exchanges the position of two players in the party.  Prompts the
 * use for the second player number.
 */
bool newOrderForPlayer(int player) {
    GetPlayerInfo *playerInfo;

    if (player == 0) {
        screenMessage("%s, You must lead!\n", c->players[0].name);
        (*c->location->finishTurn)();
        return false;
    }

    screenMessage("    with # ");

    newOrderTemp = player;
    playerInfo = new GetPlayerInfo;
    playerInfo->canBeDisabled = 1;
    playerInfo->command = newOrderForPlayer2;
    eventHandlerPushKeyHandlerWithData(&gameGetPlayerNoKeyHandler, playerInfo);

    return true;
}

bool newOrderForPlayer2(int player2) {
    int player1 = newOrderTemp;
    SaveGamePlayerRecord tmp;

    if (player2 == 0) {
        screenMessage("%s, You must lead!\n", c->players[0].name);
        (*c->location->finishTurn)();
        return false;
    } else if (player1 == player2) {
        screenMessage("What?\n");
        (*c->location->finishTurn)();
        return false;
    }

    tmp = c->players[player1];
    c->players[player1] = c->players[player2];
    c->players[player2] = tmp;

    statsUpdate();

    return true;
}

/**
 * Attempts to open a door at map coordinates x,y.  The door is
 * replaced by a temporary annotation of a floor tile for 4 turns.
 */
bool openAtCoord(MapCoords coords, int distance, void *data) {
    MapTile tile;

    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Not Here!\n");
        (*c->location->finishTurn)();
        return false;
    }

    tile = c->location->map->tileAt(coords, WITH_OBJECTS);

    if (!tileIsDoor(tile) && !tileIsLockedDoor(tile))
        return false;

    if (tileIsLockedDoor(tile)) {
        screenMessage("Can't!\n");
        (*c->location->finishTurn)();
        return true;
    }
    
    c->location->map->annotations->add(coords, BRICKFLOOR_TILE)->setTTL(4);    

    screenMessage("\nOpened!\n");
    (*c->location->finishTurn)();

    return true;
}

/**
 * Waits for space bar to return from gem mode.
 */
int gemHandleChoice(int choice) {
    eventHandlerPopKeyHandler();

    screenEnableCursor();    
    
    c->location->viewMode = VIEW_NORMAL;
    (*c->location->finishTurn)();    

    /* unpause the game */
    paused = 0;

    return 1;
}

/**
 * Peers at a city from A-P (Lycaeum telescope) and functions like a gem
 */
bool gamePeerCity(int city, void *data) {
    GetChoiceActionInfo *choiceInfo;    
    Map *peerMap;

    peerMap = mapMgrGetById((MapId)(city+1));

    if (peerMap)
    {
        gameSetMap(peerMap, 1, NULL);
        c->location->viewMode = VIEW_GEM;

        screenDisableCursor();
            
        // Wait for player to hit a key
        choiceInfo = new GetChoiceActionInfo;
        choiceInfo->choices = "\015 \033";
        choiceInfo->handleChoice = &peerCityHandleChoice;
        eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, choiceInfo);
        return true;
    }
    return false;
}

/**
 * Peers at a gem
 */
void gamePeerGem(void) {
    GetChoiceActionInfo *choiceInfo;

    paused = 1;
    pausedTimer = 0;
    screenDisableCursor();
    
    c->location->viewMode = VIEW_GEM;
    choiceInfo = new GetChoiceActionInfo;
    choiceInfo->choices = "\015 \033";
    choiceInfo->handleChoice = &gemHandleChoice;
    eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, choiceInfo);
}

/**
 * Wait for space bar to return from gem mode and returns map to normal
 */
int peerCityHandleChoice(int choice) {
    eventHandlerPopKeyHandler();
    gameExitToParentMap();
    screenEnableCursor();    
    
    (*c->location->finishTurn)();

    return 1;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
bool talkAtCoord(MapCoords coords, int distance, void *data) {
    const Person *talker;
    extern int personIsVendor(const Person *person);
    City *city = dynamic_cast<City*>(c->location->map);

    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Funny, no\nresponse!\n");
        (*c->location->finishTurn)();
        return false;
    }

    c->conversation.talker = city->personAt(coords);

    /* some persons in some towns exists as a 'person' object, but they
       really are not someone you can talk to.  These persons have mostly null fields */
    if (c->conversation.talker == NULL || 
        (c->conversation.talker->name.empty() && c->conversation.talker->npcType <= NPC_TALKER_COMPANION))
        return false;

    /* if we're talking to Lord British and the avatar is dead, LB resurrects them! */
    if (c->conversation.talker->npcType == NPC_LORD_BRITISH &&
        c->players[0].status == STAT_DEAD) {
        screenMessage("%s, Thou shalt live again!\n", c->players[0].name);
        
        c->players[0].status = STAT_GOOD;
        playerHeal(HT_FULLHEAL, 0);
        (*spellEffectCallback)('r', -1, SOUND_LBHEAL);
    }

    talker = c->conversation.talker;
    c->conversation.state = CONV_INTRO;
    c->conversation.reply = personGetConversationText(&c->conversation, "");
    c->conversation.playerInquiryBuffer.erase();

    talkShowReply(0);

    return true;
}

/**
 * Handles a query while talking to an NPC.
 */
int talkHandleBuffer(string *message) {
    eventHandlerPopKeyHandler();

    c->conversation.reply = personGetConversationText(&c->conversation, message->c_str());
    c->conversation.playerInquiryBuffer.erase();

    talkShowReply(1);

    return 1;
}

int talkHandleChoice(int choice) {
    char message[2];

    eventHandlerPopKeyHandler();

    message[0] = choice;
    message[1] = '\0';

    c->conversation.reply = personGetConversationText(&c->conversation, message);
    c->conversation.playerInquiryBuffer.erase();

    talkShowReply(1);

    return 1;
}

bool talkHandleAnyKey(int key, void *data) {
    int showPrompt = (int) data;

    eventHandlerPopKeyHandler();

    talkShowReply(showPrompt);

    return true;
}

/**
 * Shows the conversation reply and sets up a key handler to handle
 * the current conversation state.
 */
void talkShowReply(int showPrompt) {
    string prompt;
    GetChoiceActionInfo *gcInfo;
    int bufferlen;
    
    screenMessage("%s", c->conversation.reply->front());
    int size = c->conversation.reply->size();
    c->conversation.reply->pop_front();

    /* if all chunks haven't been shown, wait for a key and process next chunk*/    
    size = c->conversation.reply->size();
    if (size > 0) {    
        eventHandlerPushKeyHandlerWithData(&talkHandleAnyKey, (void *) showPrompt);
        return;
    }

    /* otherwise, free current reply and proceed based on conversation state */
    replyDelete(c->conversation.reply);
    c->conversation.reply = NULL;

    if (c->conversation.state == CONV_DONE) {
        (*c->location->finishTurn)();
        return;
    }
    
    /* When Lord British heals the party */
    else if (c->conversation.state == CONV_FULLHEAL) {
        int i;
        
        for (i = 0; i < c->saveGame->members; i++) {
            playerHeal(HT_CURE, i);        // cure the party
            playerHeal(HT_FULLHEAL, i);    // heal the party
        }        
        (*spellEffectCallback)('r', -1, SOUND_MAGIC); // same spell effect as 'r'esurrect

        statsUpdate();
        c->conversation.state = CONV_TALK;
    }
    /* When Lord British checks and advances each party member's level */
    else if (c->conversation.state == CONV_ADVANCELEVELS) {
        gameLordBritishCheckLevels();
        c->conversation.state = CONV_TALK;
    }

    if (showPrompt) {        
        prompt = personGetPrompt(&c->conversation);
        if (!prompt.empty())
            screenMessage("%s", prompt.c_str());        
    }

    switch (personGetInputRequired(&c->conversation, &bufferlen)) {
    case CONVINPUT_STRING:
        gameGetInput(&talkHandleBuffer, &c->conversation.playerInquiryBuffer, bufferlen);
        break;

    case CONVINPUT_CHARACTER:
        gcInfo = new GetChoiceActionInfo;
        gcInfo->choices = personGetChoices(&c->conversation);
        gcInfo->handleChoice = &talkHandleChoice;
        eventHandlerPushKeyHandlerWithData(&keyHandlerGetChoice, gcInfo);
        break;

    case CONVINPUT_NONE:
        /* no handler: conversation done! */
        break;
    }
}

int useItem(string *itemName) {
    eventHandlerPopKeyHandler();

    itemUse(itemName->c_str());

    if (eventHandlerGetKeyHandler() == &gameBaseKeyHandler ||
        eventHandlerGetKeyHandler() == &combatBaseKeyHandler)
        (*c->location->finishTurn)();

    return 1;
}

/**
 * Changes armor for the given player.  Prompts the use for the armor.
 */
bool wearForPlayer(int player) {
    AlphaActionInfo *info;

    c->statsView = STATS_ARMOR;
    statsUpdate();

    info = new AlphaActionInfo;
    info->lastValidLetter = ARMR_MAX + 'a' - 1;
    info->handleAlpha = wearForPlayer2;
    info->prompt = "Armour: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt.c_str());

    eventHandlerPushKeyHandlerWithData(&gameGetAlphaChoiceKeyHandler, info);

    return true;
}

bool wearForPlayer2(int a, void *data) {
    int player = (int) data;
    ArmorType armor = (ArmorType) a, oldArmor;

    if (armor != ARMR_NONE && c->saveGame->armor[armor] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return false;
    }

    if (!armorCanWear(armor, c->players[player].klass)) {
        screenMessage("\nA %s may NOT use\n%s\n", getClassName(c->players[player].klass), armorGetName(armor)->c_str());
        (*c->location->finishTurn)();
        return false;
    }

    oldArmor = c->players[player].armor;
    if (oldArmor != ARMR_NONE)
        c->saveGame->armor[oldArmor]++;
    if (armor != ARMR_NONE)
        c->saveGame->armor[armor]--;
    c->players[player].armor = armor;

    screenMessage("%s\n", armorGetName(armor)->c_str());

    (*c->location->finishTurn)();

    return true;
}

/**
 * Called when the player selects a party member for ztats
 */
bool ztatsFor(int player) {
    /* reset the spell mix menu and un-highlight the current item,
       and hide reagents that you don't have */
    gameResetSpellMixing();

    c->statsView = (StatsView) (STATS_CHAR1 + player);
    statsUpdate();    

    eventHandlerPushKeyHandler(&gameZtatsKeyHandler);    
    return true;
}

/**
 * This function is called every quarter second.
 */
void gameTimer(void *data) {

    if (pausedTimer > 0) {
        pausedTimer--;
        if (pausedTimer <= 0) {
            pausedTimer = 0;
            paused = 0; /* unpause the game */
        }
    }
    
    if (!paused && !pausedTimer) {
        Direction dir = DIR_WEST;  

        if (++c->windCounter >= MOON_SECONDS_PER_PHASE * 4) {
            if (xu4_random(4) == 1 && !windLock)
                c->windDirection = dirRandomDir(MASK_DIR_ALL);
            c->windCounter = 0;        
        }

        /* balloon moves about 4 times per second */
        if ((c->transportContext == TRANSPORT_BALLOON) &&
            c->saveGame->balloonstate) {
            dir = dirReverse((Direction) c->windDirection);
            gameMoveAvatar(dir, 0);            
        }

        gameUpdateMoons(1);

        c->location->map->animateObjects();

        screenCycle();

        /*
         * refresh the screen only if the timer queue is empty --
         * i.e. drop a frame if another timer event is about to be fired
         */
        if (eventHandlerTimerQueueEmpty())
            gameUpdateScreen();

        /*
         * force pass if no commands within last 20 seconds
         */
        if ((eventHandlerGetKeyHandler() == &gameBaseKeyHandler ||
             eventHandlerGetKeyHandler() == &combatBaseKeyHandler) &&
             gameTimeSinceLastCommand() > 20) {
         
            /* pass the turn, and redraw the text area so the prompt is shown */
            (*eventHandlerGetKeyHandler())(' ', NULL);
            screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);
        }
    }

}

/**
 * Updates the phases of the moons and shows
 * the visual moongates on the map, if desired
 */
void gameUpdateMoons(int showmoongates)
{
    int realMoonPhase,
        oldTrammel,
        trammelSubphase;        
    const MapCoords *gate;

    if (c->location->map->isWorldMap() && c->location->viewMode == VIEW_NORMAL) {        
        oldTrammel = c->saveGame->trammelphase;

        if (++c->moonPhase >= MOON_PHASES * MOON_SECONDS_PER_PHASE * 4)
            c->moonPhase = 0;
        
        trammelSubphase = c->moonPhase % (MOON_SECONDS_PER_PHASE * 4 * 3);
        realMoonPhase = (c->moonPhase / (4 * MOON_SECONDS_PER_PHASE));

        c->saveGame->trammelphase = realMoonPhase / 3;
        c->saveGame->feluccaphase = realMoonPhase % 8;

        if (c->saveGame->trammelphase > 7)
            c->saveGame->trammelphase = 7;        
        
        if (showmoongates)
        {
            /* update the moongates if trammel changed */
            if (trammelSubphase == 0) {
                gate = moongateGetGateCoordsForPhase(oldTrammel);
                if (gate)
                    c->location->map->annotations->remove(*gate, MOONGATE0_TILE);
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate)
                    c->location->map->annotations->add(*gate, MOONGATE0_TILE);
            }
            else if (trammelSubphase == 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE0_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE1_TILE);
                }
            }
            else if (trammelSubphase == 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE1_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE2_TILE);
                }
            }
            else if (trammelSubphase == 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE2_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE3_TILE);
                }
            }
            else if ((trammelSubphase > 3) && (trammelSubphase < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE3_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE3_TILE);
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE3_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE2_TILE);
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE2_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE1_TILE);
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, MOONGATE1_TILE);
                    c->location->map->annotations->add(*gate, MOONGATE0_TILE);
                }
            }
        }
    }
}

/**
 * Initializes the moon state according to the savegame file. This method of
 * initializing the moons (rather than just setting them directly) is necessary
 * to make sure trammel and felucca stay in sync
 */
void gameInitMoons()
{
    int trammelphase = c->saveGame->trammelphase,
        feluccaphase = c->saveGame->feluccaphase;        

    ASSERT(c != NULL, "Game context doesn't exist!");
    ASSERT(c->saveGame != NULL, "Savegame doesn't exist!");
    //ASSERT(mapIsWorldMap(c->location->map) && c->location->viewMode == VIEW_NORMAL, "Can only call gameInitMoons() from the world map!");

    c->saveGame->trammelphase = c->saveGame->feluccaphase = 0;
    c->moonPhase = 0;

    while ((c->saveGame->trammelphase != trammelphase) ||
           (c->saveGame->feluccaphase != feluccaphase))
        gameUpdateMoons(0);    
}

/**
 * Handles trolls under bridges
 */
void gameCheckBridgeTrolls() {
    Monster *m;

    if (!c->location->map->isWorldMap() ||
        c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS) != BRIDGE_TILE ||
        xu4_random(8) != 0)
        return;

    screenMessage("\nBridge Trolls!\n");
    
    m = c->location->map->addMonster(monsters.getById(TROLL_ID), c->location->coords);
    combatInit(m, MAP_BRIDGE_CON);
    combatBegin();
}

/**
 * Checks the hull integrity of the ship and handles
 * the ship sinking, if necessary
 */
void gameCheckHullIntegrity() {
    int i;

    /* see if the ship has sunk */
    if ((c->transportContext == TRANSPORT_SHIP) && c->saveGame->shiphull <= 0)
    {
        screenMessage("\nThy ship sinks!\n\n");        

        for (i = 0; i < c->saveGame->members; i++)
        {
            c->players[i].hp = 0;
            c->players[i].status = STAT_DEAD;
        }
        statsUpdate();   

        screenRedrawScreen();        
        deathStart(5);
    }
}

/**
 * Checks for valid conditions and handles
 * special monsters guarding the entrance to the
 * abyss and to the shrine of spirituality
 */
void gameCheckSpecialMonsters(Direction dir) {
    int i;
    Object *obj;    
    static const struct {
        int x, y;
        Direction dir;
    } pirateInfo[] = {
        { 224, 220, DIR_EAST }, /* N'M" O'A" */
        { 224, 228, DIR_EAST }, /* O'E" O'A" */
        { 226, 220, DIR_EAST }, /* O'E" O'C" */
        { 227, 228, DIR_EAST }, /* O'E" O'D" */
        { 228, 227, DIR_SOUTH }, /* O'D" O'E" */
        { 229, 225, DIR_SOUTH }, /* O'B" O'F" */
        { 229, 223, DIR_NORTH }, /* N'P" O'F" */
        { 228, 222, DIR_NORTH } /* N'O" O'E" */
    };

    /*
     * if heading east into pirates cove (O'A" N'N"), generate pirate
     * ships
     */
    if (dir == DIR_EAST &&
        c->location->coords.x == 0xdd &&
        c->location->coords.y == 0xe0) {
        for (i = 0; i < 8; i++) {        
            obj = c->location->map->addMonster(monsters.getById(PIRATE_ID), MapCoords(pirateInfo[i].x, pirateInfo[i].y));
            obj->setDirection(pirateInfo[i].dir);            
        }
    }

    /*
     * if heading south towards the shrine of humility, generate
     * daemons unless horn has been blown
     */    
    if (dir == DIR_SOUTH &&
        c->location->coords.x >= 229 &&
        c->location->coords.x < 234 &&
        c->location->coords.y >= 212 &&
        c->location->coords.y < 217 &&
        c->aura != AURA_HORN) {
        for (i = 0; i < 8; i++)            
            obj = c->location->map->addMonster(monsters.getById(DAEMON_ID), MapCoords(231, c->location->coords.y + 1, c->location->coords.z));                    
    }
}

/**
 * Checks for and handles when the avatar steps on a moongate
 */
int gameCheckMoongates(void) {
    MapCoords dest;
    
    if (moongateFindActiveGateAt(c->saveGame->trammelphase, c->saveGame->feluccaphase, c->location->coords, &dest)) {

        (*spellEffectCallback)(-1, -1, SOUND_MOONGATE); // Default spell effect (screen inversion without 'spell' sound effects)
        
        if (c->location->coords != dest) {
            c->location->coords = dest;            
            (*spellEffectCallback)(-1, -1, SOUND_MOONGATE); // Again, after arriving
        }

        if (moongateIsEntryToShrineOfSpirituality(c->saveGame->trammelphase, c->saveGame->feluccaphase)) {
            Shrine *shrine_spirituality;

            shrine_spirituality = dynamic_cast<Shrine*>(mapMgrGetById(MAP_SHRINE_SPIRITUALITY));

            if (!playerCanEnterShrine(VIRT_SPIRITUALITY))
                return 1;
            
            gameSetMap(shrine_spirituality, 1, NULL);
            musicPlay();

            shrine_spirituality->enter();
        }

        return 1;
    }

    return 0;
}

/**
 * Checks monster conditions and spawns new monsters if necessary
 */
void gameCheckRandomMonsters() {
    int canSpawnHere = c->location->map->isWorldMap() || c->location->context & CTX_DUNGEON;
    int spawnDivisor = c->location->context & CTX_DUNGEON ? (32 - (c->location->coords.z << 2)) : 32;

    /* remove monsters that are too far away from the avatar */
    gameMonsterCleanup();
    
    /* If there are too many monsters already,
       or we're not on the world map, don't worry about it! */
    if (!canSpawnHere ||
        c->location->map->getNumberOfMonsters() >= MAX_MONSTERS_ON_MAP ||
        xu4_random(spawnDivisor) != 0)
        return;
    
    gameSpawnMonster(NULL);
}

/**
 * Fixes objects initially loaded by saveGameMonstersRead,
 * and alters movement behavior accordingly to match the monster
 */
void gameFixupMonsters(Map *map) {
    ObjectList::iterator i;
    Object *obj;

    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        obj = *i;

        /* translate unknown objects into monster objects if necessary */
        if (obj->getType() == OBJECT_UNKNOWN && monsters.getByTile(obj->getTile()) != NULL &&
            obj->getMovementBehavior() != MOVEMENT_FIXED) {
            /* replace the object with a monster object */
            map->addMonster(monsters.getByTile(obj->getTile()), obj->getCoords());            
            i = map->removeObject(i);
        }
    }    
}

long gameTimeSinceLastCommand() {
    return time(NULL) - c->lastCommandTime;
}

/**
 * Handles what happens when a monster attacks you
 */
void gameMonsterAttack(Monster *m) {
    Object *under;
    MapTile ground;    
    
    screenMessage("\nAttacked by %s\n", m->name.c_str());

    ground = c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS);
    if ((under = c->location->map->objectAt(c->location->coords)) &&
        tileIsShip(under->getTile()))
        ground = under->getTile();
    
    combatInit(m, combatMapForTile(ground, (MapTile)c->saveGame->transport, m));
    combatBegin();
}

/**
 * Performs a ranged attack for the monster at x,y on the world map
 */
bool monsterRangeAttack(MapCoords coords, int distance, void *data) {
    CoordActionInfo* info = (CoordActionInfo*)data;
    MapCoords old = info->prev;
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;       
    Monster *m;
    MapTile tile;

    info->prev = coords;

    /* Find the monster that made the range attack */
    m = dynamic_cast<Monster*>(c->location->map->objectAt(info->origin));    

    /* Figure out what the ranged attack should look like */
    tile = (m && (m->worldrangedtile > 0)) ? m->worldrangedtile : HITFLASH_TILE;

    /* Remove the last weapon annotation left behind */
    if ((distance > 0) && (old.x >= 0) && (old.y >= 0))
        c->location->map->annotations->remove(old, tile);
    
    /* Attack missed, stop now */
    if (coords.x == -1 && coords.y == -1) {
        return true;
    }
    
    /* See if the attack hits the avatar */
    else {
        Object *obj = NULL;

        obj = c->location->map->objectAt(coords);        
        m = dynamic_cast<Monster*>(obj);
        
        /* Does the attack hit the avatar? */
        if (coords == c->location->coords) {
            /* always displays as a 'hit' */
            attackFlash(coords, tile, 3);

            /* FIXME: check actual damage from u4dos -- values here are guessed */
            if (c->transportContext == TRANSPORT_SHIP)
                gameDamageShip(-1, 10);
            else gameDamageParty(10, 25);

            return true;
        }
        /* Destroy objects that were hit */
        else if (obj) {
            if (((obj->getType() == OBJECT_MONSTER) &&
                (m->id != WHIRLPOOL_ID) && (m->id != STORM_ID)) ||
                obj->getType() == OBJECT_UNKNOWN) {
                
                attackFlash(coords, tile, 3);
                c->location->map->removeObject(obj);

                return true;
            }            
        }
        
        /* Show the attack annotation */
        c->location->map->annotations->add(coords, tile, true);
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            eventHandlerSleep(attackdelay * 4);
    }

    return false;    
}

/**
 * Perform an action in the given direction, using the 'handleAtCoord'
 * function of the CoordActionInfo struct.  The 'blockedPredicate'
 * function is used to determine whether or not the action is blocked
 * by the tile it passes over.
 */
int gameDirectionalAction(CoordActionInfo *info) {
    int distance = 0,
        succeeded = 0;
    MapCoords t_c = info->origin;
    Direction dirx = DIR_NONE,
              diry = DIR_NONE;
    MapTile tile;

    /* Figure out which direction the action is going */
    if (DIR_IN_MASK(DIR_WEST, info->dir)) dirx = DIR_WEST;
    else if (DIR_IN_MASK(DIR_EAST, info->dir)) dirx = DIR_EAST;
    if (DIR_IN_MASK(DIR_NORTH, info->dir)) diry = DIR_NORTH;
    else if (DIR_IN_MASK(DIR_SOUTH, info->dir)) diry = DIR_SOUTH;

    /*
     * try every tile in the given direction, up to the given range.
     * Stop when the command handler succeeds, the range is exceeded,
     * or the action is blocked.
     */
    
    if ((dirx <= 0 || DIR_IN_MASK(dirx, info->validDirections)) && 
        (diry <= 0 || DIR_IN_MASK(diry, info->validDirections))) {
        for (distance = 0; distance <= info->range;
             distance++, t_c.move(dirx, c->location->map), t_c.move(diry, c->location->map)) {
            if (distance >= info->firstValidDistance) {                
            
                /* make sure our action isn't taking us off the map */
                if (MAP_IS_OOB(c->location->map, t_c))
                    break;

                tile = c->location->map->tileAt(t_c, WITH_GROUND_OBJECTS);

                /* should we see if the action is blocked before trying it? */
                if (info->blockBefore && info->blockedPredicate &&
                    !(*(info->blockedPredicate))(tile))
                    break;

                if ((*(info->handleAtCoord))(t_c, distance, info)) {
                    succeeded = 1;
                    break;
                }                

                /* see if the action was blocked only if it did not succeed */
                if (!info->blockBefore && info->blockedPredicate &&
                    !(*(info->blockedPredicate))(tile))
                    break;
            }
        }
    }

    if (!succeeded)
        (*info->handleAtCoord)(MapCoords(-1, -1), distance, info);

    return 0;
}

/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to each party member, with a 50% chance for each member to 
 * avoid the damage.  If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to each member.
 */
void gameDamageParty(int minDamage, int maxDamage) {
    int i;
    int damage;

    for (i = 0; i < c->saveGame->members; i++) {
        if (xu4_random(2) == 0) {
            damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
                xu4_random((maxDamage + 1) - minDamage) + minDamage :
                maxDamage;            
            playerApplyDamage(&c->players[i], damage);
            statsHighlightCharacter(i);            
        }
    }
    
    eventHandlerSleep(100);
    statsUpdate();    
    screenShake(1);
}

/**
 * Deals an amount of damage between 'minDamage' and 'maxDamage'
 * to the ship.  If (minDamage == -1) or (minDamage >= maxDamage),
 * deals 'maxDamage' damage to the ship.
 */
void gameDamageShip(int minDamage, int maxDamage) {
    int damage;

    if (c->transportContext == TRANSPORT_SHIP) {
        damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
            xu4_random((maxDamage + 1) - minDamage) + minDamage :
            maxDamage;

        screenShake(1);

        c->saveGame->shiphull -= damage;
        if ((short)c->saveGame->shiphull < 0)
            c->saveGame->shiphull = 0;
        statsUpdate();
        gameCheckHullIntegrity();        
    }
}

/**
 * Removes monsters from the current map if they are too far away from the avatar
 */
void gameMonsterCleanup(void) {
    ObjectList::iterator i;
    Map *map = c->location->map;
    Object *obj;
    
    for (i = map->objects.begin(); i != map->objects.end();) {
        obj = *i;
        MapCoords o_coords = obj->getCoords();

        if ((obj->getType() == OBJECT_MONSTER) && (o_coords.z == c->location->coords.z) &&
             o_coords.distance(c->location->coords, c->location->map) > MAX_MONSTER_DISTANCE) {
            
            /* delete the object and remove it from the map */
            i = map->removeObject(i);            
        }
        else i++;
    }
}

/**
 * Sets the transport for the avatar
 */
void gameSetTransport(MapTile tile) {       
    
    if (tileIsHorse(tile))
        c->transportContext = TRANSPORT_HORSE;
    else if (tileIsShip(tile))
        c->transportContext = TRANSPORT_SHIP;
    else if (tileIsBalloon(tile))
        c->transportContext = TRANSPORT_BALLOON;
    else c->transportContext = TRANSPORT_FOOT;

    c->saveGame->transport = tile;
}

/**
 * Check the levels of each party member while talking to Lord British
 */
void gameLordBritishCheckLevels(void) {
    int i;
    int levelsRaised = 0;    

    for (i = 0; i < c->saveGame->members; i++) {
        if (playerGetRealLevel(&c->players[i]) <
            playerGetMaxLevel(&c->players[i]))

            if (!levelsRaised) {
                /* give an extra space to separate these messages */
                screenMessage("\n");
                levelsRaised = 1;
            }

            playerAdvanceLevel(&c->players[i]);
    }
 
    screenMessage("\nWhat would thou\nask of me?\n");
}

/**
 * Summons a monster given by 'monsterName'. This can either be given
 * as the monster's name, or the monster's id.  Once it finds the
 * monster to be summoned, it calls gameSpawnMonster() to spawn it.
 */
int gameSummonMonster(string *monsterName) {    
    unsigned int id;
    const Monster *m;

    eventHandlerPopKeyHandler();

    if (monsterName->empty()) {
        screenPrompt();
        return 0;
    }
    
    /* find the monster by its id and spawn it */
    id = atoi(monsterName->c_str());
    m = monsters.getById(id);
    if (!m)
        m = monsters.getByName(*monsterName);

    if (m) {
        screenMessage("\n%s summoned!\n", m->name.c_str());
        screenPrompt();
        gameSpawnMonster(m);
        return 1;
    }
    
    screenMessage("\n%s not found\n", monsterName->c_str());
    screenPrompt();
    return 0;
}

/**
 * Spawns a monster (m) just offscreen of the avatar.
 * If (m==NULL) then it finds its own monster to spawn and spawns it.
 */
void gameSpawnMonster(const Monster *m) {
    int dx, dy, t, i;
    const Monster *monster;
    MapCoords coords = c->location->coords;

    if (c->location->context & CTX_DUNGEON) {
        MapTile tile;
        int found = 0;        
        
        for (i = 0; i < 0x20; i++) {
            coords = MapCoords(xu4_random(c->location->map->width), xu4_random(c->location->map->height), c->location->coords.z);
            tile = c->location->map->tileAt(coords, WITH_OBJECTS);
            if (tileIsMonsterWalkable(tile)) {
                found = 1;
                break;
            }
        }

        if (!found)
            return;
        
        dx = coords.x - c->location->coords.x;
        dy = coords.y - c->location->coords.y;
    }    
    else {    
        dx = 7;
        dy = xu4_random(7);

        if (xu4_random(2))
            dx = -dx;
        if (xu4_random(2))
            dy = -dy;
        if (xu4_random(2)) {
            t = dx;
            dx = dy;
            dy = t;
        }
    }

    coords.move(dx, dy, c->location->map);   
    
    /* figure out what monster to spawn */
    if (m)
        monster = m;
    else if (c->location->context & CTX_DUNGEON)
        monster = monsters.randomForDungeon(c->location->coords.z);
    else
        monster = monsters.randomForTile(c->location->map->tileAt(coords, WITHOUT_OBJECTS));

    if (monster) c->location->map->addMonster(monster, coords);    
}

/**
 * Alerts the guards that the avatar is doing something bad
 */ 
void gameAlertTheGuards(Map *map) {
    ObjectList::iterator i;    
    const Monster *m;

    /* switch all the guards to attack mode */
    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        m = monsters.getByTile((*i)->getTile());
        if (m && (m->id == GUARD_ID || m->id == LORDBRITISH_ID))
            (*i)->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
}

/**
 * Destroys all creatures on the current map.
 */
void gameDestroyAllMonsters(void) {
    int i;
    
    (*spellEffectCallback)('t', -1, SOUND_MAGIC); /* same effect as tremor */
    
    if (c->location->context & CTX_COMBAT) {
        /* destroy all monsters in combat */
        for (i = 0; i < AREA_MONSTERS; i++) {
            CombatObjectMap::iterator obj;            
            for (obj = combatInfo.monsters.begin(); obj != combatInfo.monsters.end();) {
                if (obj->second->id != LORDBRITISH_ID) {
                    /* FIXME: This is a crappy way of doing things, but
                       when the combat is setup the way it should be, this 
                       should work out */
                    c->location->map->removeObject(obj->second);
                    /* this, however, is a BAD way of doing it, but necessary for now */
                    combatInfo.monsters.clear();
                }
                else obj++;
            }            
        }
    }    
    else {
        /* destroy all monsters on the map */
        ObjectList::iterator current;
        Map *map = c->location->map;
        
        for (current = map->objects.begin(); current != map->objects.end();) {
            Monster *m = dynamic_cast<Monster*>(*current);

            if (m) {                
                /* the skull does not destroy Lord British */
                if (m->id != LORDBRITISH_ID)
                    current = map->removeObject(current);                
                else current++;
            }
            else current++;
        }
    }

    /* alert the guards! Really, the only one left should be LB himself :) */
    gameAlertTheGuards(c->location->map);
}

/**
 * Creates the balloon near Hythloth, but only if the balloon doesn't already exists somewhere
 */
bool gameCreateBalloon(Map *map) {
    ObjectList::iterator i;    

    /* see if the balloon has already been created (and not destroyed) */
    for (i = map->objects.begin(); i != map->objects.end(); i++)
        if (tileIsBalloon((*i)->getTile()))
            return false;

    map->addObject(BALLOON_TILE, BALLOON_TILE, MapCoords(233, 242, -1));
    return true;
}
