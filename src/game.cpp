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
#include "conversation.h"
#include "debug.h"
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
#include "creature.h"
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
void gameAdvanceLevel(PartyMember *player);
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
void gameCheckRandomCreatures(void);
void gameCheckSpecialCreatures(Direction dir);
void gameLordBritishCheckLevels(void);

/* creature functions */
void gameAlertTheGuards(Map *map);
void gameDestroyAllCreatures(void);
void gameFixupObjects(Map *map);
void gameCreatureAttack(Creature *obj);
int gameSummonCreature(string *creatureName);

/* etc */
bool gameCreateBalloon(Map *map);

/* Functions END */
/*---------------*/

extern Object *party[8];
Context *c = NULL;
int windLock = 0;
string itemNameBuffer;
string creatureNameBuffer;
string howmany;
string destination;
int paused = 0;
int pausedTimer = 0;
int castPlayer;
unsigned int castSpell;
EnergyFieldType fieldType;

Debug gameDbg("debug/game.txt", "Game");

/* FIXME */
Ingredients *mixIngredients;
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

    TRACE(gameDbg, "gameInit() running.");

    /* initialize the global game context */
    c = new Context;
    c->saveGame = new SaveGame;
    c->conversation = new Conversation;

    TRACE_LOCAL(gameDbg, "Global context initialized.");

    /* initialize conversation and game state variables */    
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->stats = new StatsArea();
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->aura = new Aura();    
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = time(NULL);
    c->lastShip = NULL;
    
    /* set the map to the world map by default */
    gameSetMap(mapMgr->get(MAP_WORLD), 0, NULL);    

    TRACE_LOCAL(gameDbg, "World map set.");

    /* load in the save game */
    saveGameFile = saveGameOpenForReading();
    if (saveGameFile) {
        c->saveGame->read(saveGameFile);
        fclose(saveGameFile);
    } else
        errorFatal("no savegame found!");

    TRACE_LOCAL(gameDbg, "Save game loaded.");

    /* initialize our party */
    c->party = new Party(c->saveGame);    

    /* initialize our combat controller */
    c->combat = new CombatController();

    /* initialize our start location */
    Map *map = mapMgr->get(c->saveGame->location);
    TRACE_LOCAL(gameDbg, "Initializing start location.");

    /* initialize the moons (must be done from the world map) */
    gameInitMoons();
    
    /* if our map is not the world map, then load our map */
    if (map->type != Map::WORLD)
        gameSetMap(map, 1, NULL);    

    /**
     * Translate info from the savegame to something we can use
     */     
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

    TRACE_LOCAL(gameDbg, "Loading monsters.");

    /* load in creatures.sav */
    monstersFile = saveGameMonstersOpenForReading(MONSTERS_SAV_BASE_FILENAME);
    if (monstersFile) {
        saveGameMonstersRead(c->location->map->monsterTable, monstersFile);
        fclose(monstersFile);
    }
    gameFixupObjects(c->location->map);

    /* we have previous creature information as well, load it! */
    if (c->location->prev) {
        monstersFile = saveGameMonstersOpenForReading(OUTMONST_SAV_BASE_FILENAME);
        if (monstersFile) {
            saveGameMonstersRead(c->location->prev->map->monsterTable, monstersFile);
            fclose(monstersFile);
        }
        gameFixupObjects(c->location->prev->map);
    }

    /* set the party's transport */
    c->party->setTransport(Tile::translate(c->saveGame->transport));

    playerSetLostEighthCallback(&gameLostEighth);
    playerSetAdvanceLevelCallback(&gameAdvanceLevel);
    playerSetSpellEffectCallback(&gameSpellEffect);
    playerSetPartyStarvingCallback(&gamePartyStarving);    
    itemSetDestroyAllCreaturesCallback(&gameDestroyAllCreatures);

    musicPlay();
    screenDrawImage(BKGD_BORDERS);    
    c->stats->update(); /* draw the party stats */

    screenMessage("Press Alt-h for help\n");    
    screenPrompt();    

    TRACE_LOCAL(gameDbg, "Settings up reagent menu.");

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

    eventHandler.pushMouseAreaSet(mouseAreas); 
    
    /* add some observers */
    c->aura->addObserver(c->stats);
    c->party->addObserver(c->stats);
    spellMixMenu.addObserver(c->stats);

    TRACE(gameDbg, "gameInit() completed successfully.");
}

/**
 * Saves the game state into party.sav and creatures.sav.
 */
int gameSave() {
    FILE *saveGameFile, *monstersFile, *dngMapFile;
    SaveGame save = *c->saveGame;

    /*************************************************/
    /* Make sure the savegame struct is accurate now */
    
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
    save.location = c->location->map->id;
    save.orientation = (Direction)(c->saveGame->orientation - DIR_WEST);    

    /* Done making sure the savegame struct is accurate */
    /****************************************************/

    saveGameFile = saveGameOpenForWriting();
    if (!saveGameFile) {
        screenMessage("Error opening party.sav\n");
        return 0;
    }

    if (!save.write(saveGameFile)) {
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

    /* fix creature animations so they are compatible with u4dos */
    c->location->map->resetObjectAnimations();
    c->location->map->fillMonsterTable(); /* fill the monster table so we can save it */

    if (!saveGameMonstersWrite(c->location->map->monsterTable, monstersFile)) {
        screenMessage("Error opening creatures.sav\n");
        fclose(monstersFile);
        return 0;
    }
    fclose(monstersFile);

    /**
     * Write dungeon info
     */ 
    if (c->location->context & CTX_DUNGEON) {
        unsigned int x, y, z;

        typedef std::map<const Creature*, int> DngCreatureIdMap;
        static DngCreatureIdMap id_map;        

        /**
         * Map creatures to u4dos dungeon creature Ids
         */ 
        if (id_map.size() == 0) {
            id_map[creatures.getById(RAT_ID)]             = 1;
            id_map[creatures.getById(BAT_ID)]             = 2;
            id_map[creatures.getById(GIANT_SPIDER_ID)]    = 3;
            id_map[creatures.getById(GHOST_ID)]           = 4;
            id_map[creatures.getById(SLIME_ID)]           = 5;
            id_map[creatures.getById(TROLL_ID)]           = 6;
            id_map[creatures.getById(GREMLIN_ID)]         = 7;
            id_map[creatures.getById(MIMIC_ID)]           = 8;
            id_map[creatures.getById(REAPER_ID)]          = 9;
            id_map[creatures.getById(INSECT_SWARM_ID)]    = 10;
            id_map[creatures.getById(GAZER_ID)]           = 11;
            id_map[creatures.getById(PHANTOM_ID)]         = 12;
            id_map[creatures.getById(ORC_ID)]             = 13;
            id_map[creatures.getById(SKELETON_ID)]        = 14;
            id_map[creatures.getById(ROGUE_ID)]           = 15;
        }

        dngMapFile = fopen("dngmap.sav", "wb");
        if (!dngMapFile) {
            screenMessage("Error opening dngmap.sav\n");
            return 0;
        }

        for (z = 0; z < c->location->map->levels; z++) {
            for (y = 0; y < c->location->map->height; y++) {
                for (x = 0; x < c->location->map->width; x++) {
                    unsigned char tile = c->location->map->getTileFromData(MapCoords(x, y, z))->getIndex();
                    Object *obj = c->location->map->objectAt(MapCoords(x, y, z));

                    /**
                     * Add the creature to the tile
                     */ 
                    if (obj && obj->getType() == Object::CREATURE) {
                        const Creature *m = dynamic_cast<Creature*>(obj);
                        DngCreatureIdMap::iterator m_id = id_map.find(m);
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
        
        /* fix creature animations so they are compatible with u4dos */
        c->location->prev->map->resetObjectAnimations();
        c->location->prev->map->fillMonsterTable(); /* fill the monster table so we can save it */

        if (!saveGameMonstersWrite(c->location->prev->map->monsterTable, monstersFile)) {
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

void gameSetMap(Map *map, bool saveLocation, const Portal *portal) {
    int viewMode;
    LocationContext context;
    FinishTurnCallback finishTurn = &gameFinishTurn;
    Tileset *tileset = Tileset::get("base");
    MoveCallback move = &gameMoveAvatar;        
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
    case Map::WORLD:
        context = CTX_WORLDMAP;
        viewMode = VIEW_NORMAL;
        break;
    case Map::DUNGEON:
        context = CTX_DUNGEON;
        viewMode = VIEW_DUNGEON;
        if (portal)
            c->saveGame->orientation = DIR_EAST;
        tileset = Tileset::get("dungeon");
        move = &gameMoveAvatarInDungeon;        
        break;
    case Map::COMBAT:
        coords = MapCoords(-1, -1); /* set these to -1 just to be safe; we don't need them */
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        finishTurn = &CombatController::finishTurn;
        move = &CombatController::movePartyMember;
        activePlayer = -1; /* different active player for combat, defaults to 'None' */
        break;
    case Map::CITY:    
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    }    
    
    c->location = new Location(coords, map, viewMode, context, finishTurn, tileset, move, c->location);    
    c->location->activePlayer = activePlayer;

    /* now, actually set our new tileset */
    Tileset::set(tileset);

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
            
            /* quench the torch of we're on the world map */
            if (c->location->prev->map->isWorldMap())
                c->party->quenchTorch();
        }
        locationFree(&c->location);

        /* restore the tileset to the one the current location uses */
        Tileset::set(c->location->tileset);
        
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
    Creature *attacker = NULL;    

    while (1) {
        /* adjust food and moves */
        c->party->endTurn();

        /* count down the aura, if there is one */
        c->aura->passTurn();        

        gameCheckHullIntegrity();

        /* update party stats */
        c->stats->showPartyView();        

        /* Creatures cannot spawn, move or attack while the avatar is on the balloon */
        /* FIXME: balloonstate is causing problems when mixed with torchduration --
           needs to be separated during gameplay and then put into savegame structure
           when saving */
        if (c->location->context == CTX_DUNGEON || (!c->saveGame->balloonstate)) {

            // apply effects from tile avatar is standing on 
            c->party->applyEffect(c->location->map->tileAt(c->location->coords, WITH_GROUND_OBJECTS)->getEffect());

            // Move creatures and see if something is attacking the avatar
            attacker = c->location->map->moveObjects(c->location->coords);        

            // Something's attacking!  Start combat!
            if (attacker) {
                gameCreatureAttack(attacker);
                return;
            }       

            // Spawn new creatures
            gameCheckRandomCreatures();            
            gameCheckBridgeTrolls();
        }

        /* update map annotations */
        c->location->map->annotations->passTurn();

        if (!c->party->isImmobilized())
            break;

        if (c->party->isDead()) {
            deathStart(0);
            return;
        } else {            
            screenMessage("Zzzzzz\n");
        }
    }

    if (c->location->context == CTX_DUNGEON) {
        if (c->party->getTorchDuration() <= 0)
            screenMessage("It's Dark!\n");
        else c->party->burnTorch();

        /* handle dungeon traps */
        if (dungeonCurrentToken() == DUNGEON_TRAP)
            dungeonHandleTrap((TrapType)dungeonCurrentSubToken());
    }
    
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
}

void gameAdvanceLevel(PartyMember *player) {
    screenMessage("\n%s\nThou art now Level %d\n", player->getName().c_str(), player->getRealLevel());

    (*spellEffectCallback)('r', -1, SOUND_MAGIC); // Same as resurrect spell
}

void gamePartyStarving(void) {
    int i;
    
    screenMessage("\nStarving!!!\n");
    /* FIXME: add sound effect here */

    /* Do 2 damage to each party member for starving! */
    for (i = 0; i < c->saveGame->members; i++)
        c->party->member(i)->applyDamage(2);
}

void gameSpellEffect(int spell, int player, Sound sound) {
    int time;
    Spell::SpecialEffects effect = Spell::SFX_INVERT;
        
    if (player >= 0)
        c->stats->highlightPlayer(player);

    /* recalculate spell speed - based on 5/sec */
    time = settings.spellEffectSpeed * 200;
    
    soundPlay(sound, false);    

    switch(spell)
    {
    case 'g': /* gate */
    case 'r': /* resurrection */
        time = (time * 3) / 2;
        break;
    case 't': /* tremor */
        time = (time * 3) / 2;
        effect = Spell::SFX_TREMOR;        
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
    case Spell::SFX_NONE:
        break;
    case Spell::SFX_TREMOR:
    case Spell::SFX_INVERT:
        gameUpdateScreen();
        screenInvertRect(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT);
        screenRedrawScreen();
        
        EventHandler::sleep(time);

        if (effect == Spell::SFX_TREMOR) {
            gameUpdateScreen();
            screenShake(10);            
        }

        break;
    }
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
    ASSERT(player < c->party->size(), "player %d, but only %d members\n", player, c->party->size());

    return c->party->member(player)->isDisabled();    
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
    MapTile *tile;

    /* Translate context-sensitive action key into a useful command */
    if (key == U4_ENTER && settings.enhancements && settings.enhancementsOptions.smartEnterKey) {
        /* Attempt to guess based on the character's surroundings etc, what
           action they want */        
        
        /* Do they want to board something? */
        if (c->transportContext == TRANSPORT_FOOT) {
            obj = c->location->map->objectAt(c->location->coords);
            if (obj && (obj->getTile().isShip() || obj->getTile().isHorse() || obj->getTile().isBalloon()))
                key = 'b';
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
    
            if (tile->isChest()) key = 'g';
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
            gameSetMap(mapMgr->get(MAP_DECEIT), 1, NULL);
            c->location->coords = MapCoords(1, 0, 7);            
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+9:
        if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
            gameSetMap(mapMgr->get(MAP_DESPISE), 1, NULL);
            c->location->coords = MapCoords(3, 2, 7);
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+10:
        if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
            gameSetMap(mapMgr->get(MAP_DESTARD), 1, NULL);
            c->location->coords = MapCoords(7, 6, 7);            
            c->saveGame->orientation = DIR_SOUTH;
        }
        else valid = false;
        break;

    case U4_FKEY+11:
        if (settings.debug) {
            screenMessage("Torch: %d\n", c->party->getTorchDuration());
            screenPrompt();
        }
        else valid = false;
        break;

    case 3:                     /* ctrl-C */
        if (settings.debug) {
            screenMessage("Cmd (h = help):");
            eventHandler.pushKeyHandler(&gameSpecialCmdKeyHandler);            
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
            eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));
            screenMessage("Destroy Object\nDir: ");
        }
        else valid = false;
        break;    

    case 8:                     /* ctrl-H */
        if (settings.debug) {
            screenMessage("Help!\n");
            screenPrompt();
            
            /* Help! send me to Lord British (who conveniently is right around where you are)! */
            gameSetMap(mapMgr->get(100), 1, NULL);
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

    case '+':
    case '-':
    case U4_KEYPAD_ENTER:
        {
            int old_cycles = settings.gameCyclesPerSecond;
            if (key == '+' && ++settings.gameCyclesPerSecond > MAX_CYCLES_PER_SECOND)
                settings.gameCyclesPerSecond = MAX_CYCLES_PER_SECOND;        
            else if (key == '-' && --settings.gameCyclesPerSecond == 0)
                settings.gameCyclesPerSecond = 1;
            else if (key == U4_KEYPAD_ENTER)
                settings.gameCyclesPerSecond = DEFAULT_CYCLES_PER_SECOND;

            if (old_cycles != settings.gameCyclesPerSecond) {
                eventTimerGranularity = (1000 / settings.gameCyclesPerSecond);
                eventHandler.getTimer()->reset(eventTimerGranularity);                
        
                if (settings.gameCyclesPerSecond == DEFAULT_CYCLES_PER_SECOND)
                    screenMessage("Speed: Normal\n");
                else if (key == '+')
                    screenMessage("Speed Up (%d)\n", settings.gameCyclesPerSecond);
                else screenMessage("Speed Down (%d)\n", settings.gameCyclesPerSecond);                
            }
            else if (settings.gameCyclesPerSecond == DEFAULT_CYCLES_PER_SECOND)
                screenMessage("Speed: Normal\n");
        }        

        endTurn = false;
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
            eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));            
        }
        break;

    case 'b':

        obj = c->location->map->objectAt(c->location->coords);

        if (c->transportContext != TRANSPORT_FOOT)
            screenMessage("Board: Can't!\n");
        else if (obj) {
            int validTransport = 1;
            
            if (obj->getTile().isShip()) {
                screenMessage("Board Frigate!\n");
                if (c->lastShip != obj)
                    c->party->setShipHull(50);                    
            }
            else if (obj->getTile().isHorse())
                screenMessage("Mount Horse!\n");
            else if (obj->getTile().isBalloon())
                screenMessage("Board Balloon!\n");
            else validTransport = 0;

            if (validTransport) {
                c->party->setTransport(obj->getTile());
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
                else if (c->location->map->tileAt(c->location->coords, WITH_OBJECTS)->canLandBalloon()) {
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
            int broadsidesDirs = dirGetBroadsidesDirs(c->party->transport.getDirection());

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
            eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));
            
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
    
            if (tile->isChest())
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
            c->party->lightTorch();            
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
        eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));
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
        eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, alphaInfo));        

        c->stats->showMixtures();
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
            eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));
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
            info->blockedPredicate = &MapTile::canTalkOverTile;
            info->blockBefore = 0;
            info->firstValidDistance = 1;
            eventHandler.pushKeyHandler(KeyHandler(&gameGetCoordinateKeyHandler, info));
            screenMessage("Talk\nDir: ");
        }
        break;

    case 'u':
        screenMessage("Use which item:\n");
        gameGetInput(&useItem, &itemNameBuffer);

        if (settings.enhancements) {
            /* a little xu4 enhancement: show items in inventory when prompted for an item to use */
            c->stats->showItems();
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
            Object *obj = c->location->map->addObject(c->party->transport, c->party->transport, c->location->coords);
            if (c->transportContext == TRANSPORT_SHIP)
                c->lastShip = obj;

            c->party->setTransport(Tileset::findTileByName("avatar")->id);
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
            gameSetMap(mapMgr->get(MAP_ABYSS), 1, NULL);
            /* then to the final altar */
            c->location->coords.x = 7;
            c->location->coords.y = 7;
            c->location->coords.z = 7;            
        }
        break;
    
    case 'h' + U4_ALT: {
        ReadChoiceController pauseController("");

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

        eventHandler.pushController(&pauseController);
        pauseController.waitFor();

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

        eventHandler.pushController(&pauseController);
        pauseController.waitFor();

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
        break;
    }

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
        if (settings.enhancements && settings.enhancementsOptions.activePlayer)
            gameSetActivePlayer(key - '1');        
        else screenMessage("Bad command!\n");

        endTurn = 0;
        break;
        
    default:
        valid = false;
        break;
    }

    if (valid && endTurn) {
        if (*eventHandler.getKeyHandler() == &gameBaseKeyHandler &&
            c->location->finishTurn == &gameFinishTurn)
            (*c->location->finishTurn)();
    }
    else if (!endTurn) {
        /* if our turn did not end, then manually redraw the text prompt */    
        screenPrompt();        
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

void gameGetInput(int (*handleBuffer)(string*), string *buffer, int bufferlen) {
    KeyHandler::ReadBuffer *readBufferInfo;

    screenEnableCursor();
    screenShowCursor();

    if (!buffer)
        errorFatal("Error: call to gameGetInput() with an invalid input buffer.");

    /* clear out the input buffer */
    buffer->erase();

    readBufferInfo = new KeyHandler::ReadBuffer;
    readBufferInfo->handleBuffer = handleBuffer; 
    readBufferInfo->buffer = buffer;    
    readBufferInfo->bufferLen = bufferlen+1;
    readBufferInfo->screenX = TEXT_AREA_X + c->col;
    readBufferInfo->screenY = TEXT_AREA_Y + c->line;

    eventHandler.pushKeyHandler(KeyHandler(&keyHandlerReadBuffer, readBufferInfo));
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

        eventHandler.pushKeyHandler(KeyHandler(&gameGetPlayerNoKeyHandler, (void *)info));
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

    eventHandler.popKeyHandler();

    if (key >= '1' &&
        key <= ('0' + c->saveGame->members)) {
        screenMessage("%c\n", key);
        if (!info->canBeDisabled && c->party->member(key - '1')->isDisabled())
            screenMessage("\nDisabled!\n");
        else (*info->command)(key - '1');
    } else {
        screenMessage("None\n");
        (*c->location->finishTurn)();
        valid = false;
    }

    //eventHandler.popKeyHandlerData();
    return valid || KeyHandler::defaultHandler(key, NULL);
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
        eventHandler.popKeyHandler();
        (*(info->handleAlpha))(key - 'A', info->data);
        //eventHandler.popKeyHandlerData();
    } else if (key == U4_SPACE || key == U4_ESC || key == U4_ENTER) {
        screenMessage("\n");
        eventHandler.popKeyHandler();
        //eventHandler.popKeyHandlerData();
        (*c->location->finishTurn)();
    } else {
        valid = false;
        screenMessage("\n%s", info->prompt.c_str());
        screenRedrawScreen();
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

bool gameGetDirectionKeyHandler(int key, void *data) {
    int (*handleDirection)(Direction dir) = (int(*)(Direction))data;    
    Direction dir = keyToDirection(key);    
    bool valid = (dir != DIR_NONE) ? true : false;
    
    switch(key) {
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        eventHandler.popKeyHandler();
        //eventHandler.popKeyHandlerData();

        screenMessage("\n");
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandler.popKeyHandler();

            screenMessage("%s\n", getDirectionName(dir));
            (*handleDirection)(dir);
            //eventHandler.popKeyHandlerData();
        }
        break;
    }    

    return valid || KeyHandler::defaultHandler(key, NULL);
}

bool gameGetFieldTypeKeyHandler(int key, void *data) {
    int (*handleFieldType)(int field) = (int(*)(int))data;    
    fieldType = ENERGYFIELD_NONE;

    eventHandler.popKeyHandler();
    
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
        //eventHandler.popKeyHandlerData();
        return true;
    } else {
        /* Invalid input here = spell failure */
        screenMessage("Failed!\n");
        /* 
         * Confirmed both mixture loss and mp loss in this situation in the 
         * original Ultima IV (at least, in the Amiga version.) 
         */
        c->saveGame->mixtures[castSpell]--;
        c->party->member(castPlayer)->adjustMp(-spellGetRequiredMP(castSpell));
        (*c->location->finishTurn)();
    }

    //eventHandler.popKeyHandlerData();
    
    return false;
}

bool gameGetPhaseKeyHandler(int key, void *data) {    
    int (*handlePhase)(int) = (int(*)(int))data;
    bool valid = true;

    eventHandler.popKeyHandler();

    if (key >= '1' && key <= '8') {
        screenMessage("%c\n", key);
        (*handlePhase)(key - '1');
    } else {
        screenMessage("None\n");
        (*c->location->finishTurn)();
        valid = false;
    }

    //eventHandler.popKeyHandlerData();

    return valid || KeyHandler::defaultHandler(key, NULL);
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
        eventHandler.popKeyHandler();
        //eventHandler.popKeyHandlerData();

        screenMessage("\n");        
        (*c->location->finishTurn)();        

    default:
        if (valid) {
            eventHandler.popKeyHandler();
            screenMessage("%s\n", getDirectionName(dir));
            gameDirectionalAction(info);
            //eventHandler.popKeyHandlerData();
        }        
        break;
    }    

    return valid || KeyHandler::defaultHandler(key, NULL);
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
        {
            /* select the corresponding reagent (if visible) */
            Menu::MenuItemList::iterator mi = menu->getById((MenuId)(key-'a'));
            if (mi->isVisible()) {        
                menu->setCurrent(menu->getById((MenuId)(key-'a')));
                gameSpellMixMenuKeyHandler(U4_SPACE, menu);
            }
        } break;
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
            item->setSelected(!item->isSelected());
                        
            if (item->isSelected())
                mixIngredients->addReagent((Reagent)item->getId());
            else
                mixIngredients->removeReagent((Reagent)item->getId());
        }
        break;
    case U4_ENTER:
        /* you have reagents! */
        if (menu->isVisible())
        {
            screenHideCursor();
            eventHandler.popKeyHandler();
            c->stats->showMixtures();
         
            screenMessage("How many? ");
            
            howmany.erase();
            gameGetInput(&gameSpellMixHowMany, &howmany, 2);
        }
        /* you don't have any reagents */
        else {
            eventHandler.popKeyHandler();
            //eventHandler.popKeyHandlerData();
            c->stats->showPartyView();
            screenEnableCursor();
            (*c->location->finishTurn)();
        }
        break;

    case U4_ESC:
        eventHandler.popKeyHandler();
        //eventHandler.popKeyHandlerData();

        mixIngredients->revert();
        delete mixIngredients;

        screenHideCursor();
        c->stats->showPartyView();
        screenMessage("\n");
        
        screenEnableCursor();
        (*c->location->finishTurn)();        
    default:
        return false;
    }

    return true;
}

void gameResetSpellMixing(void) {
    Menu::MenuItemList::iterator current;
    int i, row;    

    i = 0;
    row = 0;
    for (current = spellMixMenu.begin(); current != spellMixMenu.end(); current++) {    
        if (c->saveGame->reagents[i++] > 0) {
            current->setVisible(true);
            current->setY(STATS_AREA_Y + row);
            row++;
        }
        else current->setVisible(false);
    }

    spellMixMenu.reset(false);
}

int gameSpellMixHowMany(string *message) {
    int i, num;
    
    eventHandler.popKeyHandler();

    num = (int) strtol(message->c_str(), NULL, 10);
    
    /* entered 0 mixtures, don't mix anything! */
    if (num == 0) {
        screenMessage("\nNone mixed!\n");
        mixIngredients->revert();
        delete mixIngredients;
        (*c->location->finishTurn)();
        return 0;
    }
    
    /* if they ask for more than will give them 99, only use what they need */
    if (num > 99 - c->saveGame->mixtures[mixSpell]) {
        num = 99 - c->saveGame->mixtures[mixSpell];
        screenMessage("\nOnly need %d!", num);
    }
    
    screenMessage("\nMixing %d...\n", num);

    /* see if there's enough reagents to make number of mixtures requested */
    if (!mixIngredients->checkMultiple(num)) {
        screenMessage("\nYou don't have enough reagents to mix %d spells!\n\n", num);
        mixIngredients->revert();
        delete mixIngredients;
        (*c->location->finishTurn)();
        return 0;
    }

    screenMessage("\nYou mix the Reagents, and...\n");
    if (spellMix(mixSpell, mixIngredients)) {
        screenMessage("Success!\n\n");
        /* mix the extra spells */
        mixIngredients->multiply(num);
        for (i = 0; i < num-1; i++)
            spellMix(mixSpell, mixIngredients);
    }
    else 
        screenMessage("It Fizzles!\n\n");

    delete mixIngredients;

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
        c->stats->prevItem();
        break;
    case U4_DOWN:
    case U4_RIGHT:
        c->stats->nextItem();
        break;
    default:
        eventHandler.popKeyHandler();
        (*c->location->finishTurn)();
        break;
    }

    return true;
}

bool gameSpecialCmdKeyHandler(int key, void *data) {
    int i;
    const Coords *moongate;
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
            screenMessage("Equipment!\n");
            screenPrompt();
            for (i = ARMR_NONE + 1; i < ARMR_MAX; i++)
                c->saveGame->armor[i] = 8;
            for (i = WEAP_HANDS + 1; i < WEAP_MAX; i++) {
                const Weapon *weapon = Weapon::get(static_cast<WeaponType>(i));
                if (weapon->loseWhenUsed() || weapon->loseWhenRanged())
                    c->saveGame->weapons[i] = 99;
                else
                    c->saveGame->weapons[i] = 8;
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
        eventHandler.popKeyHandler();
        eventHandler.pushKeyHandler(&cmdHandleAnyKey);
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
        c->stats->update();
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
        eventHandler.popKeyHandler();
        if (c->location->viewMode == VIEW_NORMAL)
            c->location->viewMode = VIEW_GEM;
        else c->location->viewMode = VIEW_NORMAL;
        
        screenMessage("\nToggle View!\n");
        screenPrompt();
        return true;

    case 'r':
        screenMessage("Reagents!\n");
        screenPrompt();
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;

    case 's':
        screenMessage("Summon!\n");
        eventHandler.popKeyHandler();

        screenMessage("What?\n");
        gameGetInput(&gameSummonCreature, &creatureNameBuffer);
        
        return true;

    case 't':
        if (c->location->map->isWorldMap()) {
            MapTile horse = Tileset::findTileByName("horse")->id,
                ship = Tileset::findTileByName("ship")->id,
                balloon = Tileset::findTileByName("balloon")->id;
            c->location->map->addObject(horse, horse, MapCoords(84, 106));
            c->location->map->addObject(ship, ship, MapCoords(88, 109));
            c->location->map->addObject(balloon, balloon, MapCoords(85, 105));
            screenMessage("Transports: Ship, Horse and Balloon created!\n");
            screenPrompt();
        }
        break;

    case 'v':
        screenMessage("\nFull Virtues!\n");
        for (i = 0; i < 8; i++)
            c->saveGame->karma[i] = 0;        
        c->stats->update();
        screenPrompt();
        break;

    case 'w':        
        screenMessage("Wind Dir ('l' to lock):\n");
        eventHandler.popKeyHandler();
        eventHandler.pushKeyHandler(&windCmdKeyHandler);
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
        c->stats->update();
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
        eventHandler.popKeyHandler();

    return valid || KeyHandler::defaultHandler(key, NULL);
}

bool cmdHandleAnyKey(int key, void *data) {
    eventHandler.popKeyHandler();

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

    eventHandler.popKeyHandler();    
    screenPrompt();

    return true;
}

/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
bool attackAtCoord(MapCoords coords, int distance, void *data) {
    Object *under;
    const MapTile *ground;    
    Creature *m;

    /* attack failed: finish up */
    if (coords.x == -1 && coords.y == -1) {        
        screenMessage("Nothing to Attack!\n");
        (*c->location->finishTurn)();
        return false;
    }

    m = dynamic_cast<Creature*>(c->location->map->objectAt(coords));
    /* nothing attackable: move on to next tile */
    if ((m == NULL) || 
        /* can't attack horse transport */
        (m->getTile().isHorse() && m->getMovementBehavior() == MOVEMENT_FIXED)) {
        return false;
    }

    /* attack successful */
    ground = c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS);
    if ((under = c->location->map->objectAt(c->location->coords)) &&
        under->getTile().isShip())
        ground = &under->getTile();

    /* You're attacking a townsperson!  Alert the guards! */
    if ((m->getType() == Object::PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))
        gameAlertTheGuards(c->location->map);        

    /* not good karma to be killing the innocent.  Bad avatar! */    
    if (m->isGood() || /* attacking a good creature */
        /* attacking a docile (although possibly evil) person in town */
        ((m->getType() == Object::PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))) 
        c->party->adjustKarma(KA_ATTACKED_GOOD);

    delete(c->combat);
    c->combat = new CombatController(CombatMap::mapForTile(*ground, c->party->transport, m));
    c->combat->init(m);
    c->combat->begin();    
    return true;
}

bool gameCastForPlayer(int player) {
    AlphaActionInfo *info;

    castPlayer = player;

    if (gameCheckPlayerDisabled(player)) {
        (*c->location->finishTurn)();
        return false;
    }

    c->stats->showMixtures();

    info = new AlphaActionInfo;
    info->lastValidLetter = 'z';
    info->handleAlpha = castForPlayer2;
    info->prompt = "Spell: ";
    info->data = NULL;

    screenMessage("%s", info->prompt.c_str());

    eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, info));

    return true;
}

bool castForPlayer2(int spell, void *data) {    
    castSpell = spell;

    screenMessage("%s!\n", spellGetName(spell));    

    c->stats->showPartyView();

    /* If we can't really cast this spell, skip the extra parameters */
    if ((spellGetRequiredMP(spell) > c->party->member(castPlayer)->getMp()) || /* not enough mp */
        ((spellGetContext(spell) & c->location->context) == 0) ||            /* wrong context */
        (c->saveGame->mixtures[spell] == 0) ||                               /* none mixed! */
        ((spellGetTransportContext(spell) & c->transportContext) == 0)) {    /* invalid transportation for spell */
        
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        return true;
    }

    /* Get the final parameters for the spell */
    switch (spellGetParamType(spell)) {
    case Spell::PARAM_NONE:
        gameCastSpell(castSpell, castPlayer, 0);
        (*c->location->finishTurn)();
        break;
    case Spell::PARAM_PHASE:
        screenMessage("To Phase: ");
        eventHandler.pushKeyHandler(KeyHandler(&gameGetPhaseKeyHandler, (void *) &castForPlayerGetPhase));        
        break;
    case Spell::PARAM_PLAYER:
        screenMessage("Who: ");
        gameGetPlayerForCommand(&castForPlayerGetDestPlayer, 1, 0);        
        break;
    case Spell::PARAM_DIR:
        if (c->location->context == CTX_DUNGEON)
            gameCastSpell(castSpell, castPlayer, c->saveGame->orientation);
        else {
            screenMessage("Dir: ");
            eventHandler.pushKeyHandler(KeyHandler(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir));
        }
        break;
    case Spell::PARAM_TYPEDIR:
        screenMessage("Energy type? ");
        eventHandler.pushKeyHandler(KeyHandler(&gameGetFieldTypeKeyHandler, (void *) &castForPlayerGetEnergyType));
        break;
    case Spell::PARAM_FROMDIR:
        screenMessage("From Dir: ");
        eventHandler.pushKeyHandler(KeyHandler(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetDestDir));
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
        eventHandler.pushKeyHandler(KeyHandler(&gameGetDirectionKeyHandler, (void *) &castForPlayerGetEnergyDir));
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
        c->location->map->annotations->remove(old, Tileset::findTileByName("miss_flash")->id);
    
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
        Creature *m = dynamic_cast<Creature*>(obj);
                
        /* FIXME: there's got to be a better way make whirlpools and storms impervious to cannon fire */
        if (obj && (obj->getType() == Object::CREATURE) && 
            (m->id != WHIRLPOOL_ID) && (m->id != STORM_ID))
            validObject = 1;        
        /* See if it's an object to be destroyed (the avatar cannot destroy the balloon) */
        else if (obj && (obj->getType() == Object::UNKNOWN) && !(obj->getTile().isBalloon() && originAvatar))
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
                CombatController::attackFlash(coords, Tileset::findTileByName("hit_flash")->id, 5);

                if (c->transportContext == TRANSPORT_SHIP)
                    gameDamageShip(-1, 10);
                else gameDamageParty(10, 25); /* party gets hurt between 10-25 damage */
            }          
            /* inanimate objects get destroyed instantly, while creatures get a chance */
            else if (obj->getType() == Object::UNKNOWN) {
                CombatController::attackFlash(coords, Tileset::findTileByName("hit_flash")->id, 5);
                c->location->map->removeObject(obj);
            }
            
            /* only the avatar can hurt other creatures with cannon fire */
            else if (originAvatar) {
                CombatController::attackFlash(coords, Tileset::findTileByName("hit_flash")->id, 5);
                if (xu4_random(4) == 0) /* reverse-engineered from u4dos */
                    c->location->map->removeObject(obj);
            }
            
            if (originAvatar)
                (*c->location->finishTurn)();

            return true;
        }
        
        c->location->map->annotations->add(coords, Tileset::findTileByName("miss_flash")->id, true);
        gameUpdateScreen();

        /* Based on attack speed setting in setting struct, make a delay for
           the attack annotation */
        if (attackdelay > 0)
            EventHandler::sleep(attackdelay * 4);
    }

    return false;
}

/**
 * Get the chest at the current x,y of the current context for player 'player'
 */
bool gameGetChest(int player) {
    Object *obj;
    MapTile *tile, newTile;
    MapCoords coords;    
    
    c->location->getCurrentPosition(&coords);
    tile = c->location->map->tileAt(coords, WITH_GROUND_OBJECTS);
    newTile = c->location->getReplacementTile(coords);    
    
    /* get the object for the chest, if it is indeed an object */
    obj = c->location->map->objectAt(coords);
    if (obj && !obj->getTile().isChest())
        obj = NULL;
    
    if (tile->isChest()) {
        if (obj)
            c->location->map->removeObject(obj);
        else
            c->location->map->annotations->add(coords, newTile);
        
        /* see if the chest is trapped and handle it */
        getChestTrapHandler(player);
        screenMessage("The Chest Holds: %d Gold\n", c->party->getChest());

        screenPrompt();
        
        if (isCity(c->location->map) && obj == NULL)
            c->party->adjustKarma(KA_STOLE_CHEST);
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
    int dex = c->saveGame->players[player].dex;
    int randNum = xu4_random(4);    
    
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
        else if (trapType == EFFECT_LAVA)
            screenMessage("Bomb Trap!\n");        

        /* See if the trap was evaded! */           
        if ((dex + 25) < xu4_random(100) &&         /* test player's dex */            
            (player >= 0)) {                        /* player is < 0 during the 'O'pen spell (immune to traps) */
            if (trapType == EFFECT_LAVA) /* bomb trap */
                c->party->applyEffect(trapType);
            else c->party->member(player)->applyEffect(trapType);
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
                MapTile *tile;
                
                new_coords.move(dir, c->location->map);                
                tile = c->location->map->tileAt(new_coords, WITH_OBJECTS);

                if (tile->isDoor()) {
                    openAtCoord(new_coords, 1, NULL);
                    retval = (MoveReturnValue)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } else if (tile->isLockedDoor()) {
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
        gameCheckSpecialCreatures(dir);
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

            Dungeon *dng = dynamic_cast<Dungeon*>(c->location->map);
            dng->currentRoom = room;

            /* set the map and start combat! */
            delete c->combat;
            c->combat = new CombatController(dng->roomMaps[room]);
            c->combat->initDungeonRoom(room, dirReverse(realDir));
            c->combat->begin();
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
    MapTile *tile;

    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Jimmy what?\n");
        (*c->location->finishTurn)();
        return false;
    }

    tile = c->location->map->tileAt(coords, WITH_OBJECTS);

    if (!tile->isLockedDoor())
        return false;
        
    if (c->saveGame->keys) {
        static const MapTile door = *Tileset::get()->getByName("door");
        c->saveGame->keys--;
        c->location->map->annotations->add(coords, door);
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

    c->stats->showWeapons();

    info = new AlphaActionInfo;    
    info->lastValidLetter = WEAP_MAX + 'a' - 1;
    info->handleAlpha = readyForPlayer2;
    info->prompt = "Weapon: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt.c_str());

    eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, info));

    return true;
}

bool readyForPlayer2(int w, void *data) {
    int player = (int) data;
    const Weapon *weapon = Weapon::get((WeaponType) w);
    PartyMember *p = c->party->member(player);

    // Return view to party overview
    c->stats->showPartyView();

    if (weapon->getType() != WEAP_HANDS && c->saveGame->weapons[weapon->getType()] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return false;
    }

    if (!weapon->canReady(p->getClass())) {
        string indef_article;

        switch(tolower(weapon->getName()[0])) {
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
            getClassName(p->getClass()),
            indef_article.c_str(),
            weapon->getName().c_str());
        (*c->location->finishTurn)();
        return false;
    }

    WeaponType oldWeapon = p->getWeapon();
    if (oldWeapon != WEAP_HANDS)
        c->saveGame->weapons[oldWeapon]++;
    if (weapon->getType() != WEAP_HANDS)
        c->saveGame->weapons[weapon->getType()]--;
    p->setWeapon(weapon->getType());

    screenMessage("%s\n", weapon->getName().c_str());

    (*c->location->finishTurn)();

    return true;
}

/**
 * Mixes reagents for a spell.  Prompts for reagents.
 */
bool mixReagentsForSpell(int spell, void *data) {
    KeyHandler::GetChoice *info;    

    mixSpell = spell;
    mixIngredients = new Ingredients();
    
    /* do we use the Ultima V menu system? */
    if (settings.enhancements && settings.enhancementsOptions.u5spellMixing) {
        screenMessage("%s\n", spellGetName(spell));
        screenDisableCursor();
        gameResetSpellMixing();
        spellMixMenu.reset(); /* reset the menu, highlighting the first item */
        eventHandler.pushKeyHandler(KeyHandler(&gameSpellMixMenuKeyHandler, &spellMixMenu));
    }
    else {
        screenMessage("%s\nReagent: ", spellGetName(spell));    

        info = new KeyHandler::GetChoice;
        info->choices = "abcdefgh\n\r \033";
        info->handleChoice = &mixReagentsForSpell2;        
        eventHandler.pushKeyHandler(KeyHandler(&keyHandlerGetChoice, info));
    }

    c->stats->showReagents();

    return 0;
}

int mixReagentsForSpell2(int choice) {
    KeyHandler::GetChoice *info;
    AlphaActionInfo *alphaInfo;

    eventHandler.popKeyHandler();

    if (choice == '\n' || choice == '\r' || choice == ' ') {
        screenMessage("\n\nYou mix the Reagents, and...\n");

        if (spellMix(mixSpell, mixIngredients))
            screenMessage("Success!\n\n");
        else
            screenMessage("It Fizzles!\n\n");

        delete mixIngredients;

        screenMessage("Mix reagents\n");
        alphaInfo = new AlphaActionInfo;
        alphaInfo->lastValidLetter = 'z';
        alphaInfo->handleAlpha = mixReagentsForSpell;
        alphaInfo->prompt = "For Spell: ";
        alphaInfo->data = NULL;

        screenMessage("%s", alphaInfo->prompt.c_str());
        eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, alphaInfo));

        c->stats->showMixtures();

        return 1;
    }

    else if (choice == '\033') {

        mixIngredients->revert();
        delete mixIngredients;

        screenMessage("\n\n");
        (*c->location->finishTurn)();
        return 1;
    }

    else {
        screenMessage("%c\n", toupper(choice));

        if (!mixIngredients->addReagent((Reagent)(choice - 'a')))
            screenMessage("None Left!\n");

        screenMessage("Reagent: ");

        info = new KeyHandler::GetChoice;
        info->choices = "abcdefgh\n\r \033";
        info->handleChoice = &mixReagentsForSpell2;
        eventHandler.pushKeyHandler(KeyHandler(&keyHandlerGetChoice, info));


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
        screenMessage("%s, You must lead!\n", c->party->member(0)->getName().c_str());
        (*c->location->finishTurn)();
        return false;
    }

    screenMessage("    with # ");

    newOrderTemp = player;
    playerInfo = new GetPlayerInfo;
    playerInfo->canBeDisabled = 1;
    playerInfo->command = newOrderForPlayer2;
    eventHandler.pushKeyHandler(KeyHandler(&gameGetPlayerNoKeyHandler, playerInfo));

    return true;
}

bool newOrderForPlayer2(int player2) {
    int player1 = newOrderTemp;
    SaveGamePlayerRecord tmp;

    if (player2 == 0) {
        screenMessage("%s, You must lead!\n", c->party->member(0)->getName().c_str());
        (*c->location->finishTurn)();
        return false;
    } else if (player1 == player2) {
        screenMessage("What?\n");
        (*c->location->finishTurn)();
        return false;
    }

    tmp = c->saveGame->players[player1];
    c->saveGame->players[player1] = c->saveGame->players[player2];
    c->saveGame->players[player2] = tmp;

    /* re-build the party */
    delete c->party;
    c->party = new Party(c->saveGame);

    return true;
}

/**
 * Attempts to open a door at map coordinates x,y.  The door is
 * replaced by a temporary annotation of a floor tile for 4 turns.
 */
bool openAtCoord(MapCoords coords, int distance, void *data) {
    MapTile *tile;

    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Not Here!\n");
        (*c->location->finishTurn)();
        return false;
    }

    tile = c->location->map->tileAt(coords, WITH_OBJECTS);

    if (!tile->isDoor() && !tile->isLockedDoor())
        return false;

    if (tile->isLockedDoor()) {
        screenMessage("Can't!\n");
        (*c->location->finishTurn)();
        return true;
    }
    
    c->location->map->annotations->add(coords, Tileset::findTileByName("brick_floor")->id)->setTTL(4);    

    screenMessage("\nOpened!\n");
    (*c->location->finishTurn)();

    return true;
}

/**
 * Waits for space bar to return from gem mode.
 */
int gemHandleChoice(int choice) {
    eventHandler.popKeyHandler();

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
    KeyHandler::GetChoice *choiceInfo;    
    Map *peerMap;

    peerMap = mapMgr->get((MapId)(city+1));

    if (peerMap)
    {
        gameSetMap(peerMap, 1, NULL);
        c->location->viewMode = VIEW_GEM;
        paused = 1;
        pausedTimer = 0;

        screenDisableCursor();
            
        // Wait for player to hit a key
        choiceInfo = new KeyHandler::GetChoice;
        choiceInfo->choices = "\015 \033";
        choiceInfo->handleChoice = &peerCityHandleChoice;
        eventHandler.pushKeyHandler(KeyHandler(&keyHandlerGetChoice, choiceInfo));
        return true;
    }
    return false;
}

/**
 * Peers at a gem
 */
void gamePeerGem(void) {
    KeyHandler::GetChoice *choiceInfo;

    paused = 1;
    pausedTimer = 0;
    screenDisableCursor();
    
    c->location->viewMode = VIEW_GEM;
    choiceInfo = new KeyHandler::GetChoice;
    choiceInfo->choices = "\015 \033";
    choiceInfo->handleChoice = &gemHandleChoice;
    eventHandler.pushKeyHandler(KeyHandler(&keyHandlerGetChoice, choiceInfo));
}

/**
 * Wait for space bar to return from gem mode and returns map to normal
 */
int peerCityHandleChoice(int choice) {
    eventHandler.popKeyHandler();
    gameExitToParentMap();
    screenEnableCursor();
    paused = 0;
    
    (*c->location->finishTurn)();

    return 1;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
bool talkAtCoord(MapCoords coords, int distance, void *data) {
    extern int personIsVendor(const Person *person);
    City *city;

    /* can't have any conversations outside of town */
    if (!isCity(c->location->map)) {
        screenMessage("Funny, no\nresponse!\n");
        (*c->location->finishTurn)();
        return true;
    }
    
    if (coords.x == -1 && coords.y == -1) {
        screenMessage("Funny, no\nresponse!\n");
        (*c->location->finishTurn)();
        return false;
    }

    city = dynamic_cast<City*>(c->location->map);
    c->conversation->setTalker(city->personAt(coords));

    /* make sure we have someone we can talk with */
    if (!c->conversation->isValid())
        return false;

    /* if we're talking to Lord British and the avatar is dead, LB resurrects them! */
    if (c->conversation->getTalker()->npcType == NPC_LORD_BRITISH &&
        c->party->member(0)->getStatus() == STAT_DEAD) {
        screenMessage("%s, Thou shalt live again!\n", c->party->member(0)->getName().c_str());
        
        c->party->member(0)->setStatus(STAT_GOOD);
        c->party->member(0)->heal(HT_FULLHEAL);
        (*spellEffectCallback)('r', -1, SOUND_LBHEAL);
    }
    
    c->conversation->state = Conversation::INTRO;
    c->conversation->reply = personGetConversationText(c->conversation, "");
    c->conversation->playerInput.erase();

    talkShowReply(0);

    return true;
}

/**
 * Handles a query while talking to an NPC.
 */
int talkHandleBuffer(string *message) {
    eventHandler.popKeyHandler();

    c->conversation->reply = personGetConversationText(c->conversation, message->c_str());
    c->conversation->playerInput.erase();

    talkShowReply(1);

    return 1;
}

int talkHandleChoice(int choice) {
    char message[2];

    eventHandler.popKeyHandler();

    message[0] = choice;
    message[1] = '\0';

    c->conversation->reply = personGetConversationText(c->conversation, message);
    c->conversation->playerInput.erase();

    talkShowReply(1);

    return 1;
}

bool talkHandleAnyKey(int key, void *data) {
    int showPrompt = (int) data;

    eventHandler.popKeyHandler();

    talkShowReply(showPrompt);

    return true;
}

/**
 * Shows the conversation reply and sets up a key handler to handle
 * the current conversation state.
 */
void talkShowReply(int showPrompt) {
    string prompt;
    KeyHandler::GetChoice *gcInfo;
    int bufferlen;
    
    screenMessage("%s", c->conversation->reply->front());
    int size = c->conversation->reply->size();
    c->conversation->reply->pop_front();

    /* if all chunks haven't been shown, wait for a key and process next chunk*/    
    size = c->conversation->reply->size();
    if (size > 0) {    
        eventHandler.pushKeyHandler(KeyHandler(&talkHandleAnyKey, (void *) showPrompt));
        return;
    }

    /* otherwise, free current reply and proceed based on conversation state */
    replyDelete(c->conversation->reply);
    c->conversation->reply = NULL;
    
    /* they're attacking you! */
    if (c->conversation->state == Conversation::ATTACK) {
        c->conversation->state = Conversation::DONE;
        c->conversation->getTalker()->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
    
    if (c->conversation->state == Conversation::DONE) {
        (*c->location->finishTurn)();
        return;
    }
    
    /* When Lord British heals the party */
    else if (c->conversation->state == Conversation::FULLHEAL) {
        int i;
        
        for (i = 0; i < c->party->size(); i++) {
            c->party->member(i)->heal(HT_CURE);        // cure the party
            c->party->member(i)->heal(HT_FULLHEAL);    // heal the party
        }        
        (*spellEffectCallback)('r', -1, SOUND_MAGIC); // same spell effect as 'r'esurrect

        c->conversation->state = Conversation::TALK;
    }
    /* When Lord British checks and advances each party member's level */
    else if (c->conversation->state == Conversation::ADVANCELEVELS) {
        gameLordBritishCheckLevels();
        c->conversation->state = Conversation::TALK;
    }

    if (showPrompt) {        
        prompt = personGetPrompt(c->conversation);
        if (!prompt.empty())
            screenMessage("%s", prompt.c_str());        
    }

    switch (c->conversation->getInputRequired(&bufferlen)) {
    case Conversation::INPUT_STRING:
        gameGetInput(&talkHandleBuffer, &c->conversation->playerInput, bufferlen);
        break;

    case Conversation::INPUT_CHARACTER:
        gcInfo = new KeyHandler::GetChoice;
        gcInfo->choices = personGetChoices(c->conversation);
        gcInfo->handleChoice = &talkHandleChoice;
        eventHandler.pushKeyHandler(KeyHandler(&keyHandlerGetChoice, gcInfo));
        break;

    case Conversation::INPUT_NONE:
        /* no handler: conversation done! */
        break;
    }
}

int useItem(string *itemName) {
    eventHandler.popKeyHandler();

    itemUse(itemName->c_str());

    if (*eventHandler.getKeyHandler() == &gameBaseKeyHandler ||
        *eventHandler.getKeyHandler() == &CombatController::baseKeyHandler)
        (*c->location->finishTurn)();

    return 1;
}

/**
 * Changes armor for the given player.  Prompts the use for the armor.
 */
bool wearForPlayer(int player) {
    AlphaActionInfo *info;

    c->stats->showArmor();

    info = new AlphaActionInfo;
    info->lastValidLetter = ARMR_MAX + 'a' - 1;
    info->handleAlpha = wearForPlayer2;
    info->prompt = "Armour: ";
    info->data = (void *) player;

    screenMessage("%s", info->prompt.c_str());

    eventHandler.pushKeyHandler(KeyHandler(&gameGetAlphaChoiceKeyHandler, info));

    return true;
}

bool wearForPlayer2(int a, void *data) {
    int player = (int) data;
    const Armor *armor = Armor::get((ArmorType) a);
    PartyMember *p = c->party->member(player);

    if (armor->getType() != ARMR_NONE && c->saveGame->armor[armor->getType()] < 1) {
        screenMessage("None left!\n");
        (*c->location->finishTurn)();
        return false;
    }

    if (!armor->canWear(p->getClass())) {
        screenMessage("\nA %s may NOT use\n%s\n", getClassName(p->getClass()), armor->getName().c_str());
        (*c->location->finishTurn)();
        return false;
    }

    ArmorType oldArmorType = p->getArmor();
    if (oldArmorType != ARMR_NONE)
        c->saveGame->armor[oldArmorType]++;
    if (armor->getType() != ARMR_NONE)
        c->saveGame->armor[armor->getType()]--;
    p->setArmor(armor->getType());

    screenMessage("%s\n", armor->getName().c_str());

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

    c->stats->showPlayerDetails(player);

    eventHandler.pushKeyHandler(&gameZtatsKeyHandler);    
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

        screenCycle();

        /*
         * refresh the screen only if the timer queue is empty --
         * i.e. drop a frame if another timer event is about to be fired
         */
        if (eventHandler.timerQueueEmpty())
            gameUpdateScreen();

        /*
         * force pass if no commands within last 20 seconds
         */
        KeyHandler *keyHandler = eventHandler.getKeyHandler();
        if (keyHandler != NULL && (*keyHandler == &gameBaseKeyHandler || *keyHandler == &CombatController::baseKeyHandler) &&
             gameTimeSinceLastCommand() > 20) {
         
            /* pass the turn, and redraw the text area so the prompt is shown */
            keyHandler->handle(U4_SPACE);
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
    const Coords *gate;

    if (c->location->map->isWorldMap()) {
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
                    c->location->map->annotations->remove(*gate, Tile::translate(0x40));
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate)
                    c->location->map->annotations->add(*gate, Tile::translate(0x40));
            }
            else if (trammelSubphase == 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x40));
                    c->location->map->annotations->add(*gate, Tile::translate(0x41));
                }
            }
            else if (trammelSubphase == 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x41));
                    c->location->map->annotations->add(*gate, Tile::translate(0x42));
                }
            }
            else if (trammelSubphase == 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x42));
                    c->location->map->annotations->add(*gate, Tile::translate(0x43));
                }
            }
            else if ((trammelSubphase > 3) && (trammelSubphase < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x43));
                    c->location->map->annotations->add(*gate, Tile::translate(0x43));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x43));
                    c->location->map->annotations->add(*gate, Tile::translate(0x42));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x42));
                    c->location->map->annotations->add(*gate, Tile::translate(0x41));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, Tile::translate(0x41));
                    c->location->map->annotations->add(*gate, Tile::translate(0x40));
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
    Creature *m;
    static const TileId bridge = Tileset::findTileByName("bridge")->id;

    if (!c->location->map->isWorldMap() ||
        c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS)->id != bridge ||
        xu4_random(8) != 0)
        return;

    screenMessage("\nBridge Trolls!\n");
    
    m = c->location->map->addCreature(creatures.getById(TROLL_ID), c->location->coords);
    delete c->combat;
    c->combat = new CombatController(MAP_BRIDGE_CON);    
    c->combat->init(m);
    c->combat->begin();
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

        for (i = 0; i < c->party->size(); i++)
        {
            c->party->member(i)->setHp(0);
            c->party->member(i)->setStatus(STAT_DEAD);            
        }

        screenRedrawScreen();        
        deathStart(5);
    }
}

/**
 * Checks for valid conditions and handles
 * special creatures guarding the entrance to the
 * abyss and to the shrine of spirituality
 */
void gameCheckSpecialCreatures(Direction dir) {
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
            obj = c->location->map->addCreature(creatures.getById(PIRATE_ID), MapCoords(pirateInfo[i].x, pirateInfo[i].y));
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
        *c->aura != Aura::HORN) {
        for (i = 0; i < 8; i++)            
            obj = c->location->map->addCreature(creatures.getById(DAEMON_ID), MapCoords(231, c->location->coords.y + 1, c->location->coords.z));                    
    }
}

/**
 * Checks for and handles when the avatar steps on a moongate
 */
int gameCheckMoongates(void) {
    Coords dest;
    
    if (moongateFindActiveGateAt(c->saveGame->trammelphase, c->saveGame->feluccaphase, c->location->coords, dest)) {

        (*spellEffectCallback)(-1, -1, SOUND_MOONGATE); // Default spell effect (screen inversion without 'spell' sound effects)
        
        if (c->location->coords != dest) {
            c->location->coords = dest;            
            (*spellEffectCallback)(-1, -1, SOUND_MOONGATE); // Again, after arriving
        }

        if (moongateIsEntryToShrineOfSpirituality(c->saveGame->trammelphase, c->saveGame->feluccaphase)) {
            Shrine *shrine_spirituality;

            shrine_spirituality = dynamic_cast<Shrine*>(mapMgr->get(MAP_SHRINE_SPIRITUALITY));

            if (!c->party->canEnterShrine(VIRT_SPIRITUALITY))
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
 * Checks creature conditions and spawns new creatures if necessary
 */
void gameCheckRandomCreatures() {
    int canSpawnHere = c->location->map->isWorldMap() || c->location->context & CTX_DUNGEON;
    int spawnDivisor = c->location->context & CTX_DUNGEON ? (32 - (c->location->coords.z << 2)) : 32;

    /* remove creatures that are too far away from the avatar */
    gameCreatureCleanup();
    
    /* If there are too many creatures already,
       or we're not on the world map, don't worry about it! */
    if (!canSpawnHere ||
        c->location->map->getNumberOfCreatures() >= MAX_CREATURES_ON_MAP ||
        xu4_random(spawnDivisor) != 0)
        return;
    
    gameSpawnCreature(NULL);
}

/**
 * Fixes objects initially loaded by saveGameMonstersRead,
 * and alters movement behavior accordingly to match the creature
 */
void gameFixupObjects(Map *map) {
    int i;
    Object *obj;

    /* add stuff from the monster table to the map */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        SaveGameMonsterRecord *monster = &map->monsterTable[i];
        if (monster->prevTile != 0) {
            Coords c(monster->x, monster->y);
            MapTile tile = Tile::translate(monster->tile),
                oldTile = Tile::translate(monster->prevTile);
            
            if (i < MONSTERTABLE_CREATURES_SIZE) {
                const Creature *creature = creatures.getByTile(tile);
                /* make sure we really have a creature */
                if (creature)
                    obj = map->addCreature(creature, c);
                else {
                    fprintf(stderr, "Error: A non-creature object was found in the creature section of the monster table. (Tile: %s)\n", ::c->location->tileset->get(tile.id)->name.c_str());
                    obj = map->addObject(tile, oldTile, c);
                }
            }
            else
                obj = map->addObject(tile, oldTile, c);

            /* set the map for our object */
            obj->setMap(map);
        }
    }    
}

long gameTimeSinceLastCommand() {
    return time(NULL) - c->lastCommandTime;
}

/**
 * Handles what happens when a creature attacks you
 */
void gameCreatureAttack(Creature *m) {
    Object *under;
    const MapTile *ground;
    
    screenMessage("\nAttacked by %s\n", m->getName().c_str());

    ground = c->location->map->tileAt(c->location->coords, WITHOUT_OBJECTS);
    if ((under = c->location->map->objectAt(c->location->coords)) &&
        under->getTile().isShip())
        ground = &under->getTile();

    delete c->combat;
    c->combat = new CombatController(CombatMap::mapForTile(*ground, c->party->transport, m));
    c->combat->init(m);
    c->combat->begin();
}

/**
 * Performs a ranged attack for the creature at x,y on the world map
 */
bool creatureRangeAttack(MapCoords coords, int distance, void *data) {
    CoordActionInfo* info = (CoordActionInfo*)data;
    MapCoords old = info->prev;
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;       
    Creature *m;
    MapTile tile;

    info->prev = coords;

    /* Find the creature that made the range attack */
    m = dynamic_cast<Creature*>(c->location->map->objectAt(info->origin));    

    /* Figure out what the ranged attack should look like */
    tile = (m && (m->worldrangedtile.id > 0)) ? m->worldrangedtile : Tileset::findTileByName("hit_flash")->id;

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
        m = dynamic_cast<Creature*>(obj);
        
        /* Does the attack hit the avatar? */
        if (coords == c->location->coords) {
            /* always displays as a 'hit' */
            CombatController::attackFlash(coords, tile, 3);

            /* FIXME: check actual damage from u4dos -- values here are guessed */
            if (c->transportContext == TRANSPORT_SHIP)
                gameDamageShip(-1, 10);
            else gameDamageParty(10, 25);

            return true;
        }
        /* Destroy objects that were hit */
        else if (obj) {
            if (((obj->getType() == Object::CREATURE) &&
                (m->id != WHIRLPOOL_ID) && (m->id != STORM_ID)) ||
                obj->getType() == Object::UNKNOWN) {
                
                CombatController::attackFlash(coords, tile, 3);
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
            EventHandler::sleep(attackdelay * 4);
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
    MapTile *tile;

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
                    !(*(info->blockedPredicate))(*tile))
                    break;

                if ((*(info->handleAtCoord))(t_c, distance, info)) {
                    succeeded = 1;
                    break;
                }                

                /* see if the action was blocked only if it did not succeed */
                if (!info->blockBefore && info->blockedPredicate &&
                    !(*(info->blockedPredicate))(*tile))
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

    for (i = 0; i < c->party->size(); i++) {
        if (xu4_random(2) == 0) {
            damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
                xu4_random((maxDamage + 1) - minDamage) + minDamage :
                maxDamage;
            c->party->member(i)->applyDamage(damage);            
            c->stats->highlightPlayer(i);            
        }
    }
    
    EventHandler::sleep(100);
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
        
        c->party->damageShip(damage);        
        gameCheckHullIntegrity();        
    }
}

/**
 * Sets (or unsets) the active player
 */
void gameSetActivePlayer(int player) {
    if (player == -1) {
        c->location->activePlayer = -1;
        screenMessage("Set Active Player: None!\n");
    }
    else if (player < c->party->size()) {
        screenMessage("Set Active Player: %s!\n", c->party->member(player)->getName().c_str());
        if (c->party->member(player)->isDisabled())
            screenMessage("Disabled!\n");
        else c->location->activePlayer = player;
    }
    // FIXME: we should move the active player into the Party class
    // so it will notify the stats area when a new active player is
    // selected.  For now, this will do:
    c->stats->update(NULL, "::gameSetActivePlayer()");
}

/**
 * Removes creatures from the current map if they are too far away from the avatar
 */
void gameCreatureCleanup(void) {
    ObjectDeque::iterator i;
    Map *map = c->location->map;
    Object *obj;
    
    for (i = map->objects.begin(); i != map->objects.end();) {
        obj = *i;
        MapCoords o_coords = obj->getCoords();

        if ((obj->getType() == Object::CREATURE) && (o_coords.z == c->location->coords.z) &&
             o_coords.distance(c->location->coords, c->location->map) > MAX_CREATURE_DISTANCE) {
            
            /* delete the object and remove it from the map */
            i = map->removeObject(i);            
        }
        else i++;
    }
}

/**
 * Check the levels of each party member while talking to Lord British
 */
void gameLordBritishCheckLevels(void) {
    int i;
    int levelsRaised = 0;    

    for (i = 0; i < c->party->size(); i++) {
        PartyMember *player = c->party->member(i);
        if (player->getRealLevel() <
            player->getMaxLevel())

            if (!levelsRaised) {
                /* give an extra space to separate these messages */
                screenMessage("\n");
                levelsRaised = 1;
            }

            player->advanceLevel();
    }
 
    screenMessage("\nWhat would thou\nask of me?\n");
}

/**
 * Summons a creature given by 'creatureName'. This can either be given
 * as the creature's name, or the creature's id.  Once it finds the
 * creature to be summoned, it calls gameSpawnCreature() to spawn it.
 */
int gameSummonCreature(string *creatureName) {    
    unsigned int id;
    const Creature *m = NULL;

    eventHandler.popKeyHandler();

    if (creatureName->empty()) {
        screenPrompt();
        return 0;
    }
    
    /* find the creature by its id and spawn it */
    id = atoi(creatureName->c_str());
    if (id > 0)
        m = creatures.getById(id);

    if (!m)
        m = creatures.getByName(*creatureName);

    if (m) {
        screenMessage("\n%s summoned!\n", m->getName().c_str());
        screenPrompt();
        gameSpawnCreature(m);
        return 1;
    }
    
    screenMessage("\n%s not found\n", creatureName->c_str());
    screenPrompt();
    return 0;
}

/**
 * Spawns a creature (m) just offscreen of the avatar.
 * If (m==NULL) then it finds its own creature to spawn and spawns it.
 */
void gameSpawnCreature(const Creature *m) {
    int t, i;
    const Creature *creature;
    MapCoords coords = c->location->coords;

    if (c->location->context & CTX_DUNGEON) {
        /* FIXME: for some reason dungeon monsters aren't spawning correctly */

        MapTile *tile;
        bool found = false;
        MapCoords new_coords;
        
        for (i = 0; i < 0x20; i++) {
            new_coords = MapCoords(xu4_random(c->location->map->width), xu4_random(c->location->map->height), coords.z);
            tile = c->location->map->tileAt(new_coords, WITH_OBJECTS);
            if (tile->isCreatureWalkable()) {
                found = true;
                break;
            }
        }

        if (!found)
            return;        
        
        coords = new_coords;
    }    
    else {    
        int dx = 7,
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

        coords.move(dx, dy, c->location->map);
    }       
    
    /* figure out what creature to spawn */
    if (m)
        creature = m;
    else if (c->location->context & CTX_DUNGEON)
        creature = creatures.randomForDungeon(c->location->coords.z);
    else
        creature = creatures.randomForTile(*c->location->map->tileAt(coords, WITHOUT_OBJECTS));

    if (creature)
        c->location->map->addCreature(creature, coords);    
}

/**
 * Alerts the guards that the avatar is doing something bad
 */ 
void gameAlertTheGuards(Map *map) {
    ObjectDeque::iterator i;    
    const Creature *m;

    /* switch all the guards to attack mode */
    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        m = creatures.getByTile((*i)->getTile());
        if (m && (m->id == GUARD_ID || m->id == LORDBRITISH_ID))
            (*i)->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
}

/**
 * Destroys all creatures on the current map.
 */
void gameDestroyAllCreatures(void) {
    int i;
    
    (*spellEffectCallback)('t', -1, SOUND_MAGIC); /* same effect as tremor */
    
    if (c->location->context & CTX_COMBAT) {
        /* destroy all creatures in combat */
        for (i = 0; i < AREA_CREATURES; i++) {            
            CombatMap *cm = getCombatMap();
            CreatureVector creatures = cm->getCreatures();
            CreatureVector::iterator obj;

            for (obj = creatures.begin(); obj != creatures.end(); obj++) {
                if ((*obj)->id != LORDBRITISH_ID)
                    cm->removeObject(*obj);                
            }            
        }
    }    
    else {
        /* destroy all creatures on the map */
        ObjectDeque::iterator current;
        Map *map = c->location->map;
        
        for (current = map->objects.begin(); current != map->objects.end();) {
            Creature *m = dynamic_cast<Creature*>(*current);

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
    ObjectDeque::iterator i;    

    /* see if the balloon has already been created (and not destroyed) */
    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        if ((*i)->getTile().isBalloon())
            return false;
    }
    
    MapTile balloon = Tileset::findTileByName("balloon")->id;
    map->addObject(balloon, balloon, MapCoords(233, 242, -1));
    return true;
}
