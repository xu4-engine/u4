/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cctype>
#include <ctime>
#include <map>

#include "u4.h"

#include "game.h"

#include "annotation.h"
#include "armor.h"
#include "camp.h"
#include "cheat.h"
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
#include "intro.h"
#include "item.h"
#include "imagemgr.h"
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
#include "progress_bar.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "sound.h"
#include "spell.h"
#include "stats.h"
#include "tilemap.h"
#include "tileset.h"
#include "utils.h"
#include "script.h"
#include "weapon.h"

using namespace std;

GameController *game = NULL;

/*-----------------*/
/* Functions BEGIN */

/* main game functions */
void gameAdvanceLevel(PartyMember *player);
void gameInnHandler(void);
void gameLostEighth(Virtue virtue);
void gamePartyStarving(void);
long gameTimeSinceLastCommand(void);
int gameSave(void);

/* spell functions */
void gameCastSpell(unsigned int spell, int caster, int param);
bool gameSpellMixHowMany(int spell, int num, Ingredients *ingredients);

void mixReagents();
bool mixReagentsForSpellU4(int spell);
bool mixReagentsForSpellU5(int spell);
void newOrder();

/* conversation functions */
bool talkAt(const Coords &coords);
void talkRunConversation(Conversation &conv, Person *talker, bool showPrompt);

/* action functions */
bool attackAt(const Coords &coords);
bool destroyAt(const Coords &coords);
bool getChestTrapHandler(int player);
bool jimmyAt(const Coords &coords);
bool openAt(const Coords &coords);
void wearArmor(int player = -1);
void ztatsFor(int player = -1);

/* checking functions */
void gameLordBritishCheckLevels(void);

/* creature functions */
void gameDestroyAllCreatures(void);
void gameFixupObjects(Map *map);
void gameCreatureAttack(Creature *obj);

/* Functions END */
/*---------------*/

//extern Object *party[8];
Context *c = NULL;

Debug gameDbg("debug/game.txt", "Game");

MouseArea mouseAreas[] = {
    { 3, { { 8, 8 }, { 8, 184 }, { 96, 96 } }, MC_WEST, { U4_ENTER, 0, U4_LEFT } },
    { 3, { { 8, 8 }, { 184, 8 }, { 96, 96 } }, MC_NORTH, { U4_ENTER, 0, U4_UP }  },
    { 3, { { 184, 8 }, { 184, 184 }, { 96, 96 } }, MC_EAST, { U4_ENTER, 0, U4_RIGHT } },
    { 3, { { 8, 184 }, { 184, 184 }, { 96, 96 } }, MC_SOUTH, { U4_ENTER, 0, U4_DOWN } },
    { 0 }
};

ReadPlayerController::ReadPlayerController() : ReadChoiceController("12345678 \033\n") {}

bool ReadPlayerController::keyPressed(int key) {
    bool valid = ReadChoiceController::keyPressed(key);
    if (valid) {
        if (value < '1' ||
            value > ('0' + c->saveGame->members))
            value = '0';
    } else {
        value = '0';
    }
    return valid;
}

int ReadPlayerController::getPlayer() {
    return value - '1';
}

int ReadPlayerController::waitFor() {
    ReadChoiceController::waitFor();
    return getPlayer();
}

bool AlphaActionController::keyPressed(int key) {
    if (islower(key))
        key = toupper(key);

    if (key >= 'A' && key <= toupper(lastValidLetter)) {
        screenMessage("%c\n", key);
        value = key - 'A';
        doneWaiting();
    } else if (key == U4_SPACE || key == U4_ESC || key == U4_ENTER) {
        screenMessage("\n");
        value = -1;
        doneWaiting();
    } else {
        screenMessage("\n%s", prompt.c_str());
        screenRedrawScreen();
        return KeyHandler::defaultHandler(key, NULL);
    }
    return true;
}
    
int AlphaActionController::get(char lastValidLetter, const string &prompt, EventHandler *eh) {
    if (!eh)
        eh = eventHandler;

    AlphaActionController ctrl(lastValidLetter, prompt);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

GameController::GameController() : mapArea(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W, VIEWPORT_H), paused(false), pausedTimer(0) {
}

void GameController::init() {
    FILE *saveGameFile, *monstersFile;    
    Image *screen = imageMgr->get("screen")->image;

    TRACE(gameDbg, "gameInit() running.");

    screen->fillRect(0, 0, screen->width(), screen->height(), 0, 0, 0);
    screenTextAt(13, 11, "Loading Game...");    
    screenRedrawScreen();    
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 4);
    pb.setBorderColor(240, 240, 240);
    pb.setBorderWidth(1);
    pb.setColor(0, 0, 128);

    /* initialize the global game context */
    c = new Context;
    c->saveGame = new SaveGame;

    TRACE_LOCAL(gameDbg, "Global context initialized.");

    /* initialize conversation and game state variables */    
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->stats = new StatsArea();
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->windLock = false;
    c->aura = new Aura();    
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = time(NULL);
    c->lastShip = NULL;

    /* load in the save game */
    saveGameFile = fopen((settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "rb");
    if (saveGameFile) {
        c->saveGame->read(saveGameFile);
        fclose(saveGameFile);
    } else
        errorFatal("no savegame found!");

    TRACE_LOCAL(gameDbg, "Save game loaded."); ++pb;

    /* initialize our party */
    c->party = new Party(c->saveGame);
    c->party->addObserver(this);

    /* set the map to the world map by default */
    setMap(mapMgr->get(MAP_WORLD), 0, NULL);  
    c->location->map->clearObjects();

    TRACE_LOCAL(gameDbg, "World map set."); ++pb;

    /* initialize our start location */
    Map *map = mapMgr->get(MapId(c->saveGame->location));
    TRACE_LOCAL(gameDbg, "Initializing start location.");

    /* initialize the moons (must be done from the world map) */
    initMoons();
    
    /* if our map is not the world map, then load our map */
    if (map->type != Map::WORLD)
        setMap(map, 1, NULL);    

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

    TRACE_LOCAL(gameDbg, "Loading monsters."); ++pb;

    /* load in creatures.sav */
    monstersFile = fopen((settings.getUserPath() + MONSTERS_SAV_BASE_FILENAME).c_str(), "rb");
    if (monstersFile) {
        saveGameMonstersRead(c->location->map->monsterTable, monstersFile);
        fclose(monstersFile);
    }
    gameFixupObjects(c->location->map);

    /* we have previous creature information as well, load it! */
    if (c->location->prev) {
        monstersFile = fopen((settings.getUserPath() + OUTMONST_SAV_BASE_FILENAME).c_str(), "rb");
        if (monstersFile) {
            saveGameMonstersRead(c->location->prev->map->monsterTable, monstersFile);
            fclose(monstersFile);
        }
        gameFixupObjects(c->location->prev->map);
    }

    spellSetEffectCallback(&gameSpellEffect);
    itemSetDestroyAllCreaturesCallback(&gameDestroyAllCreatures);

    ++pb;

    musicMgr->play();
    imageMgr->get(BKGD_BORDERS)->image->draw(0, 0);
    c->stats->update(); /* draw the party stats */

    screenMessage("Press Alt-h for help\n");    
    screenPrompt();    

    TRACE_LOCAL(gameDbg, "Settings up reagent menu."); 
    c->stats->resetReagentsMenu();

    eventHandler->pushMouseAreaSet(mouseAreas); 
    
    /* add some observers */
    c->aura->addObserver(c->stats);
    c->party->addObserver(c->stats);
    
    eventHandler->setScreenUpdate(&gameUpdateScreen);

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

    saveGameFile = fopen((settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "wb");
    if (!saveGameFile) {
        screenMessage("Error opening " PARTY_SAV_BASE_FILENAME "\n");
        return 0;
    }

    if (!save.write(saveGameFile)) {
        screenMessage("Error writing to " PARTY_SAV_BASE_FILENAME "\n");
        fclose(saveGameFile);
        return 0;
    }
    fclose(saveGameFile);

    monstersFile = fopen((settings.getUserPath() + MONSTERS_SAV_BASE_FILENAME).c_str(), "wb");
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
            id_map[creatureMgr->getById(RAT_ID)]          = 1;
            id_map[creatureMgr->getById(BAT_ID)]          = 2;
            id_map[creatureMgr->getById(GIANT_SPIDER_ID)] = 3;
            id_map[creatureMgr->getById(GHOST_ID)]        = 4;
            id_map[creatureMgr->getById(SLIME_ID)]        = 5;
            id_map[creatureMgr->getById(TROLL_ID)]        = 6;
            id_map[creatureMgr->getById(GREMLIN_ID)]      = 7;
            id_map[creatureMgr->getById(MIMIC_ID)]        = 8;
            id_map[creatureMgr->getById(REAPER_ID)]       = 9;
            id_map[creatureMgr->getById(INSECT_SWARM_ID)] = 10;
            id_map[creatureMgr->getById(GAZER_ID)]        = 11;
            id_map[creatureMgr->getById(PHANTOM_ID)]      = 12;
            id_map[creatureMgr->getById(ORC_ID)]          = 13;
            id_map[creatureMgr->getById(SKELETON_ID)]     = 14;
            id_map[creatureMgr->getById(ROGUE_ID)]        = 15;
        }

        dngMapFile = fopen((settings.getUserPath() + "dngmap.sav").c_str(), "wb");
        if (!dngMapFile) {
            screenMessage("Error opening dngmap.sav\n");
            return 0;
        }

        for (z = 0; z < c->location->map->levels; z++) {
            for (y = 0; y < c->location->map->height; y++) {
                for (x = 0; x < c->location->map->width; x++) {
                    unsigned char tile = c->location->map->translateToRawTileIndex(*c->location->map->getTileFromData(MapCoords(x, y, z)));
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

        monstersFile = fopen((settings.getUserPath() + OUTMONST_SAV_BASE_FILENAME).c_str(), "wb");
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
        screenUpdate(&game->mapArea, true, false);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_RUNE:
        screenUpdate(&game->mapArea, false, false);
        break;
    case VIEW_DUNGEON:
        screenUpdate(&game->mapArea, true, false);
        break;
    case VIEW_DEAD:
        screenUpdate(&game->mapArea, true, true);
        break;
    case VIEW_CODEX: /* the screen updates will be handled elsewhere */
        break;
    default:
        ASSERT(0, "invalid view mode: %d", c->location->viewMode);
    }
}

void GameController::setMap(Map *map, bool saveLocation, const Portal *portal, TurnCompleter *turnCompleter) {
    int viewMode;
    LocationContext context;
    int activePlayer = c->party->getActivePlayer();
    MapCoords coords;

    if (!turnCompleter)
        turnCompleter = this;

    if (portal)
        coords = portal->start;
    else
        coords = MapCoords(map->width / 2, map->height / 2);
    
    /* If we don't want to save the location, then just return to the previous location,
       as there may still be ones in the stack we want to keep */
    if (!saveLocation)
        exitToParentMap();
    
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
        break;
    case Map::COMBAT:
        coords = MapCoords(-1, -1); /* set these to -1 just to be safe; we don't need them */
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        activePlayer = -1; /* different active player for combat, defaults to 'None' */
        break;
    case Map::CITY:    
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    }    
    
    c->location = new Location(coords, map, viewMode, context, turnCompleter, c->location);
    c->location->addObserver(this);
    c->party->setActivePlayer(activePlayer);

    /* now, actually set our new tileset */
    mapArea.setTileset(map->tileset);

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

int GameController::exitToParentMap() {
    if (!c->location)
        return 0;

    if (c->location->prev != NULL) {
        // Create the balloon for Hythloth
        if (c->location->map->id == MAP_HYTHLOTH)
            createBalloon(c->location->prev->map);            

        // free map info only if previous location was on a different map
        if (c->location->prev->map != c->location->map) {
            c->location->map->annotations->clear();
            c->location->map->clearObjects();
            
            /* quench the torch of we're on the world map */
            if (c->location->prev->map->isWorldMap())
                c->party->quenchTorch();
        }
        locationFree(&c->location);

        // restore the tileset to the one the current map uses
        mapArea.setTileset(c->location->map->tileset);
        
        return 1;
    }
    return 0;
}

/**
 * Terminates a game turn.  This performs the post-turn housekeeping
 * tasks like adjusting the party's food, incrementing the number of
 * moves, etc.
 */
void GameController::finishTurn() {
    Creature *attacker = NULL;    

    while (1) {
        /* adjust food and moves */
        c->party->endTurn();

        /* count down the aura, if there is one */
        c->aura->passTurn();        

        gameCheckHullIntegrity();

        /* update party stats */
        c->stats->setView(STATS_PARTY_OVERVIEW);

        /* Creatures cannot spawn, move or attack while the avatar is on the balloon */        
        if (!c->party->isFlying()) {

            // apply effects from tile avatar is standing on 
            c->party->applyEffect(c->location->map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS)->getEffect());

            // Move creatures and see if something is attacking the avatar
            attacker = c->location->map->moveObjects(c->location->coords);        

            // Something's attacking!  Start combat!
            if (attacker) {
                gameCreatureAttack(attacker);
                return;
            }       

            // cleanup old creatures and spawn new ones
            creatureCleanup();
            checkRandomCreatures();            
            checkBridgeTrolls();
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
        Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
        if (c->party->getTorchDuration() <= 0)
            screenMessage("It's Dark!\n");
        else c->party->burnTorch();

        /* handle dungeon traps */
        if (dungeon->currentToken() == DUNGEON_TRAP) {
            dungeonHandleTrap((TrapType)dungeon->currentSubToken());
            // a little kludgey to have a second test for this
            // right here.  But without it you can survive an
            // extra turn after party death and do some things
            // that could cause a crash, like Hole up and Camp.
            if (c->party->isDead()) {
              deathStart(0);
              return;
            }
        }
    }
    
    /* draw a prompt */
    screenPrompt();
    screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);

    c->lastCommandTime = time(NULL);
}

/**
 * Provide feedback to user after a party event happens.
 */
void GameController::update(Party *party, PartyEvent &event) {
    int i;
    
    switch (event.type) {
    case PartyEvent::LOST_EIGHTH:
        // inform a player he has lost zero or more eighths of avatarhood.
        screenMessage("\n Thou hast lost\n  an eighth!\n");
        break;
    case PartyEvent::ADVANCED_LEVEL:
        screenMessage("\n%s\nThou art now Level %d\n", event.player->getName().c_str(), event.player->getRealLevel());
        gameSpellEffect('r', -1, SOUND_MAGIC); // Same as resurrect spell
        break;
    case PartyEvent::STARVING:
        screenMessage("\nStarving!!!\n");
        /* FIXME: add sound effect here */

        // 2 damage to each party member for starving!
        for (i = 0; i < c->saveGame->members; i++)
            c->party->member(i)->applyDamage(2);
        break;
    default:
        break;
    }
}

/**
 * Provide feedback to user after a movement event happens.
 */
void GameController::update(Location *location, MoveEvent &event) {
    switch (location->map->type) {
    case Map::DUNGEON:
        avatarMovedInDungeon(event);
        break;
    case Map::COMBAT:
        // FIXME: let the combat controller handle it
        dynamic_cast<CombatController *>(eventHandler->getController())->movePartyMember(event);
        break;
    default:
        avatarMoved(event);
        break;
    }    
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
    if (!game->paused) {
        game->paused = true;
        game->pausedTimer = ((time * settings.gameCyclesPerSecond) / 1000) + 1;
    }

    switch(effect)
    {
    case Spell::SFX_NONE:
        break;
    case Spell::SFX_TREMOR:
    case Spell::SFX_INVERT:
        gameUpdateScreen();
        game->mapArea.highlight(0, 0, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT);
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
    
    if (!spellCast(spell, caster, param, &spellError, true)) {
        msg = spellGetErrorMessage(spell, spellError);
        if (!msg.empty())
            screenMessage(msg.c_str());
    }    
}

/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
bool GameController::keyPressed(int key) {
    bool valid = true;
    int endTurn = 1;
    Object *obj;
    MapTile *tile;

    /* Translate context-sensitive action key into a useful command */
    if (key == U4_ENTER && settings.enhancements && settings.enhancementsOptions.smartEnterKey) {
        /* Attempt to guess based on the character's surroundings etc, what
           action they want */        
        
        /* Do they want to board something? */
        if (c->transportContext == TRANSPORT_FOOT) {
            obj = c->location->map->objectAt(c->location->coords);
            if (obj && (obj->getTile().getTileType()->isShip() || 
                        obj->getTile().getTileType()->isHorse() || 
                        obj->getTile().getTileType()->isBalloon()))
                key = 'b';
        }
        /* Klimb/Descend Balloon */
        else if (c->transportContext == TRANSPORT_BALLOON) {            
            if (c->party->isFlying())
                key = 'd';
            else key = 'k';
        }
        /* X-it transport */
        else key = 'x';        
        
        /* Klimb? */
        if ((c->location->map->portalAt(c->location->coords, ACTION_KLIMB) != NULL) || 
            (c->location->context == CTX_DUNGEON &&
             dynamic_cast<Dungeon *>(c->location->map)->ladderUpAt(c->location->coords)))
            key = 'k';
        /* Descend? */
        else if ((c->location->map->portalAt(c->location->coords, ACTION_DESCEND) != NULL) ||
                 (c->location->context == CTX_DUNGEON &&
                  dynamic_cast<Dungeon *>(c->location->map)->ladderDownAt(c->location->coords)))
            key = 'd';        
        /* Enter? */
        else if (c->location->map->portalAt(c->location->coords, ACTION_ENTER) != NULL)
            key = 'e';
        
        /* Get Chest? */
        if (!c->party->isFlying()) {
            tile = c->location->map->tileAt(c->location->coords, WITH_GROUND_OBJECTS);
    
            if (tile->getTileType()->isChest()) key = 'g';
        }
        
        /* None of these? Default to search */
        if (key == U4_ENTER) key = 's';
    }

    if ((c->location->context & CTX_DUNGEON) && strchr("abefjlotxy", key))
        screenMessage("Not here!\n");
    else 
        switch (key) {

        case U4_UP:
        case U4_DOWN:
        case U4_LEFT:
        case U4_RIGHT:        
            {
                /* move the avatar */
                MoveResult retval = c->location->move(keyToDirection(key), true);
            
                /* horse doubles speed (make sure we're on the same map as the previous move first) */
                if (retval & (MOVE_SUCCEEDED | MOVE_SLOWED) && 
                    (c->transportContext == TRANSPORT_HORSE) && c->horseSpeed) {
                    gameUpdateScreen(); /* to give it a smooth look of movement */
                    c->location->move(keyToDirection(key), false);
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
                setMap(mapMgr->get(MAP_DECEIT), 1, NULL);
                c->location->coords = MapCoords(1, 0, 7);            
                c->saveGame->orientation = DIR_SOUTH;
            }
            else valid = false;
            break;

        case U4_FKEY+9:
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(mapMgr->get(MAP_DESPISE), 1, NULL);
                c->location->coords = MapCoords(3, 2, 7);
                c->saveGame->orientation = DIR_SOUTH;
            }
            else valid = false;
            break;

        case U4_FKEY+10:
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(mapMgr->get(MAP_DESTARD), 1, NULL);
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
                CheatMenuController cheatMenuController(this);
                eventHandler->pushController(&cheatMenuController);
                cheatMenuController.waitFor();
            }
            else valid = false;
            break;

        case 4:                     /* ctrl-D */
            if (settings.debug) {
                destroy();
            }
            else valid = false;
            break;    

        case 8:                     /* ctrl-H */
            if (settings.debug) {
                screenMessage("Help!\n");
                screenPrompt();
                
                /* Help! send me to Lord British (who conveniently is right around where you are)! */
                setMap(mapMgr->get(100), 1, NULL);
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
                    eventHandler->getTimer()->reset(eventTimerGranularity);                
            
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

        /* handle music volume adjustments */
        case ',':
            // decrease the volume if possible
            screenMessage("Music: %d%s\n", musicMgr->decreaseMusicVolume(), "%");
            endTurn = false;
            break;
        case '.':
            // increase the volume if possible
            screenMessage("Music: %d%s\n", musicMgr->increaseMusicVolume(), "%");
            endTurn = false;
            break;

        /* handle sound volume adjustments */
        case '<':
            // decrease the volume if possible
            screenMessage("Sound: %d%s\n", musicMgr->decreaseSoundVolume(), "%");
            soundPlay(SOUND_FLEE);
            endTurn = false;
            break;
        case '>':
            // increase the volume if possible
            screenMessage("Sound: %d%s\n", musicMgr->increaseSoundVolume(), "%");
            soundPlay(SOUND_FLEE);
            endTurn = false;
            break;

        case 'a':
            attack();
            break;

        case 'b':
            board();
            break;

        case 'c':
            castSpell();
            break;

        case 'd':        
            if (!usePortalAt(c->location, c->location->coords, ACTION_DESCEND)) {
                if (c->transportContext == TRANSPORT_BALLOON) {
                    screenMessage("Land Balloon\n");
                    if (!c->party->isFlying())
                        screenMessage("Already Landed!\n");
                    else if (c->location->map->tileTypeAt(c->location->coords, WITH_OBJECTS)->canLandBalloon()) {
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
            fire();
            break;

        case 'g':
            getChest();
            break;

        case 'h':
            holeUp();
            break;

        case 'i':
            screenMessage("Ignite torch!\n");
            if (c->location->context == CTX_DUNGEON) {
                if (!c->party->lightTorch())
                    screenMessage("None left!\n");
            }
            else screenMessage("Not here!\n");
            break;

        case 'j':
            jimmy();
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
            mixReagents();
            break;

        case 'n':
            newOrder();
            break;

        case 'o':
            opendoor();
            break;

        case 'p':
            peer();
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
            readyWeapon();
            break;

        case 's':
            if (c->location->context == CTX_DUNGEON)
                dungeonSearch();
            else if (c->party->isFlying())
                screenMessage("Searching...\nDrift only!\n");
            else {
                screenMessage("Searching...\n");

                const ItemLocation *item = itemAtLocation(c->location->map, c->location->coords);
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
            talk();
            break;

        case 'u':
            screenMessage("Use which item:\n");
            if (settings.enhancements) {
                /* a little xu4 enhancement: show items in inventory when prompted for an item to use */
                c->stats->setView(STATS_ITEMS);
            }
            itemUse(gameGetInput().c_str());
            break;

        case 'v':
            if (musicMgr->toggle())
                screenMessage("Volume On!\n");
            else
                screenMessage("Volume Off!\n");
            endTurn = false;
            break;

        case 'w':
            wearArmor();
            break;

        case 'x':
            if ((c->transportContext != TRANSPORT_FOOT) && !c->party->isFlying()) {
                Object *obj = c->location->map->addObject(c->party->getTransport(), c->party->getTransport(), c->location->coords);
                if (c->transportContext == TRANSPORT_SHIP)
                    c->lastShip = obj;

                Tile *avatar = c->location->map->tileset->getByName("avatar");
                ASSERT(avatar, "no avatar tile found in tileset");
                c->party->setTransport(avatar->id);
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
            ztatsFor();
            break;

        case 'c' + U4_ALT:
            if (settings.debug && c->location->map->isWorldMap()) {
                /* first teleport to the abyss */
                c->location->coords.x = 0xe9;
                c->location->coords.y = 0xe9;
                setMap(mapMgr->get(MAP_ABYSS), 1, NULL);
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

            eventHandler->pushController(&pauseController);
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

            eventHandler->pushController(&pauseController);
            pauseController.waitFor();

            screenMessage("\n"
                          "u: Use Item\n"
                          "v: Volume On/Off\n"
                          "w: Wear armour\n"
                          "x: eXit\n"
                          "y: Yell\n"
                          "z: Ztats\n"
                          "Space: Pass\n"
                          ",: - Music Vol\n"
                          ".: + Music Vol\n"
                          "<: - Sound Vol\n"
                          ">: + Sound Vol\n"
                          "(more)");

            eventHandler->pushController(&pauseController);
            pauseController.waitFor();

            screenMessage("\n"
                          "Alt-Q: Main Menu\n"
                          "Alt-V: Version\n"
                          "Alt-X: Quit\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          "\n"
                          );
            screenPrompt();
            break;
        }

        case 'q' + U4_ALT:
            {             
                // TODO - implement loop in main() and let quit fall back to there
                // Quit to the main menu
                extern bool quit;
                endTurn = false;

                screenMessage("Quit to menu?");            
                char choice = ReadChoiceController::get("yn \n\033");
                screenMessage("%c", choice);
                if (choice != 'y') {
                    screenMessage("\n");
                    break;
                }
                
                eventHandler->setScreenUpdate(NULL);
                eventHandler->popController();
                
                eventHandler->pushController(intro);
                intro->init();
                eventHandler->run();
                intro->deleteIntro();
                if (!quit) {
                    eventHandler->setControllerDone(false);
                    eventHandler->popController();
                    eventHandler->pushController(this);
                    init();
                    eventHandler->run();                
                }
            }
            break;

        case 'v' + U4_ALT:
            screenMessage("XU4 %s\n", VERSION);        
            endTurn = false;
            break;

        // Turn sound effects on/off    
        case 's' + U4_ALT:
            // FIXME: there's probably a more intuitive key combination for this
            settings.soundVol = !settings.soundVol;
            screenMessage("Sound FX %s!\n", settings.soundVol ? "on" : "off");        
            endTurn = false;
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
        if (eventHandler->getController() == game)
            c->location->turnCompleter->finishTurn();
    }
    else if (!endTurn) {
        /* if our turn did not end, then manually redraw the text prompt */    
        screenPrompt();        
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

string gameGetInput(int maxlen) {
    screenEnableCursor();
    screenShowCursor();

    return ReadStringController::get(maxlen, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
}

int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer) {
    int player;
    if (c->saveGame->members <= 1) {
        screenMessage("1\n");
        player = 0;
    }
    else {
        if (canBeActivePlayer && (c->party->getActivePlayer() >= 0))
            player = c->party->getActivePlayer();
        else {
            ReadPlayerController readPlayerController;
            eventHandler->pushController(&readPlayerController);
            player = readPlayerController.waitFor();
        }

        if (player == -1) {
            screenMessage("None\n");
            return -1;
        }

        screenMessage("\n");
        if (!canBeDisabled && c->party->member(player)->isDisabled()) {
            screenMessage("\nDisabled!\n");
            return -1;
        }
    }

    ASSERT(player < c->party->size(), "player %d, but only %d members\n", player, c->party->size());
    return player;
}

Direction gameGetDirection() {
    ReadDirController dirController;

    eventHandler->pushController(&dirController);
    Direction dir = dirController.waitFor();

    if (dir == DIR_NONE) {
        screenMessage("\n");
        return dir;
    }
    else {
        screenMessage("%s\n", getDirectionName(dir));
        return dir;
    }    
}

bool gameSpellMixHowMany(int spell, int num, Ingredients *ingredients) {
    int i;
    
    /* entered 0 mixtures, don't mix anything! */
    if (num == 0) {
        screenMessage("\nNone mixed!\n");
        ingredients->revert();
        return false;
    }
    
    /* if they ask for more than will give them 99, only use what they need */
    if (num > 99 - c->saveGame->mixtures[spell]) {
        num = 99 - c->saveGame->mixtures[spell];
        screenMessage("\nOnly need %d!", num);
    }
    
    screenMessage("\nMixing %d...\n", num);

    /* see if there's enough reagents to make number of mixtures requested */
    if (!ingredients->checkMultiple(num)) {
        screenMessage("\nYou don't have enough reagents to mix %d spells!\n\n", num);
        ingredients->revert();
        return false;
    }

    screenMessage("\nYou mix the Reagents, and...\n");
    if (spellMix(spell, ingredients)) {
        screenMessage("Success!");
        /* mix the extra spells */
        ingredients->multiply(num);
        for (i = 0; i < num-1; i++)
            spellMix(spell, ingredients);
    }
    else 
        screenMessage("It Fizzles!");

    return true;        
}

bool ZtatsController::keyPressed(int key) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
        c->stats->prevItem();
        return true;
    case U4_DOWN:
    case U4_RIGHT:
        c->stats->nextItem();
        return true;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        if (c->saveGame->members >= key - '0')
            c->stats->setView(StatsView(STATS_CHAR1 + key - '1'));
        return true;
    case '0':
        c->stats->setView(StatsView(STATS_WEAPONS));
        return true;
    case U4_ESC:
    case U4_SPACE:
    case U4_ENTER:
        doneWaiting();
        return true;
    default:
        return KeyHandler::defaultHandler(key, NULL);
    }
}

void destroy() {
    screenMessage("Destroy Object\nDir: ");

    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 
                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (destroyAt(*i))
            return;
    }

    screenMessage("Nothing there!\n");
}

bool destroyAt(const Coords &coords) {
    Object *obj = c->location->map->objectAt(coords);

    if (obj) {
        if (isCreature(obj)) {
            Creature *c = dynamic_cast<Creature*>(obj);
            screenMessage("%s Destroyed!\n", c->getName().c_str());
        }
        else {
            Tile *t = c->location->map->tileset->get(obj->getTile().id);
            screenMessage("%s Destroyed!\n", t->getName().c_str());
        }

        c->location->map->removeObject(obj);
        screenPrompt();
        
        return true;
    }
    
    return false;
}

void attack() {
    screenMessage("Attack: ");

    if (c->party->isFlying()) {
        screenMessage("\nDrift only!\n");
        return;
    }

    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 
                                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (attackAt(*i))
            return;
    }

    screenMessage("Nothing to Attack!\n");
}

/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
bool attackAt(const Coords &coords) {
    Object *under;
    const Tile *ground;    
    Creature *m;

    m = dynamic_cast<Creature*>(c->location->map->objectAt(coords));
    /* nothing attackable: move on to next tile */
    if (m == NULL || !m->isAttackable())
        return false;

    /* attack successful */
    /// TODO: CHEST: Make a user option to not make chests change battlefield
    /// map (1 of 2)
    ground = c->location->map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS);
    if (!ground->isChest()) {
        ground = c->location->map->tileTypeAt(c->location->coords, WITHOUT_OBJECTS);
        if ((under = c->location->map->objectAt(c->location->coords)) && 
            under->getTile().getTileType()->isShip())
            ground = under->getTile().getTileType();
    }

    /* You're attacking a townsperson!  Alert the guards! */
    if ((m->getType() == Object::PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))
        c->location->map->alertGuards();        

    /* not good karma to be killing the innocent.  Bad avatar! */    
    if (m->isGood() || /* attacking a good creature */
        /* attacking a docile (although possibly evil) person in town */
        ((m->getType() == Object::PERSON) && (m->getMovementBehavior() != MOVEMENT_ATTACK_AVATAR))) 
        c->party->adjustKarma(KA_ATTACKED_GOOD);

    CombatController *cc = new CombatController(CombatMap::mapForTile(ground, c->party->getTransport().getTileType(), m));
    cc->init(m);
    cc->begin();    
    return true;
}

void board() {
    if (c->transportContext != TRANSPORT_FOOT) {
        screenMessage("Board: Can't!\n");
        return;
    }

    Object *obj = c->location->map->objectAt(c->location->coords);
    if (!obj) {
        screenMessage("Board What?\n");
        return;
    }

    const Tile *tile = obj->getTile().getTileType();
    if (tile->isShip()) {
        screenMessage("Board Frigate!\n");
        if (c->lastShip != obj)
            c->party->setShipHull(50);
    }
    else if (tile->isHorse())
        screenMessage("Mount Horse!\n");
    else if (tile->isBalloon())
        screenMessage("Board Balloon!\n");
    else {
        screenMessage("Board What?\n");
        return;
    }

    c->party->setTransport(obj->getTile());
    c->location->map->removeObject(obj);
}


void castSpell(int player) {
    if (player == -1) {
        screenMessage("Cast Spell!\nPlayer: ");
        player = gameGetPlayer(false, true);
    }
    if (player == -1)
        return;

    // get the spell to cast
    c->stats->setView(STATS_MIXTURES);
    screenMessage("Spell: ");
    int spell = AlphaActionController::get('z', "Spell: ");
    if (spell == -1)
        return;
    screenMessage("%s!\n", spellGetName(spell));

    c->stats->setView(STATS_PARTY_OVERVIEW);
    
    // if we can't really cast this spell, skip the extra parameters
    if (spellCheckPrerequisites(spell, player) != CASTERR_NOERROR) {
        gameCastSpell(spell, player, 0);
        return;
    }

    // Get the final parameters for the spell
    switch (spellGetParamType(spell)) {
    case Spell::PARAM_NONE:
        gameCastSpell(spell, player, 0);
        break;
    case Spell::PARAM_PHASE: {
        screenMessage("To Phase: ");
        int choice = ReadChoiceController::get("12345678 \033\n");
        if (choice < '1' || choice > '8')
            screenMessage("None\n");
        else {
            screenMessage("%c\n", choice);
            gameCastSpell(spell, player, choice - '1');
        }
        break;
    }
    case Spell::PARAM_PLAYER: {
        screenMessage("Who: ");
        int subject = gameGetPlayer(true, false);
        if (subject != -1)
            gameCastSpell(spell, player, subject);
        break;
    }
    case Spell::PARAM_DIR:
        if (c->location->context == CTX_DUNGEON)
            gameCastSpell(spell, player, c->saveGame->orientation);
        else {
            screenMessage("Dir: ");
            Direction dir = gameGetDirection();
            if (dir != DIR_NONE)
                gameCastSpell(spell, player, (int) dir);
        }
        break;
    case Spell::PARAM_TYPEDIR: {
        screenMessage("Energy type? ");

        EnergyFieldType fieldType = ENERGYFIELD_NONE;
        char key = ReadChoiceController::get("flps \033\n\r");
        switch(key) {
        case 'f': fieldType = ENERGYFIELD_FIRE; break;
        case 'l': fieldType = ENERGYFIELD_LIGHTNING; break;
        case 'p': fieldType = ENERGYFIELD_POISON; break;
        case 's': fieldType = ENERGYFIELD_SLEEP; break;
        default: break;
        }
    
        if (fieldType != ENERGYFIELD_NONE) {
            screenMessage("%c\n", toupper(key));

            Direction dir;
            if (c->location->context == CTX_DUNGEON)
                dir = (Direction)c->saveGame->orientation;
            else {
                screenMessage("Dir: ");
                dir = gameGetDirection();
            }

            if (dir != DIR_NONE) {

                /* Need to pack both dir and fieldType into param */
                int param = fieldType << 4;
                param |= (int) dir;

                gameCastSpell(spell, player, param);
            }
        } 
        else {
            /* Invalid input here = spell failure */
            screenMessage("Failed!\n");

            /* 
             * Confirmed both mixture loss and mp loss in this situation in the 
             * original Ultima IV (at least, in the Amiga version.) 
             */
            //c->saveGame->mixtures[castSpell]--;
            c->party->member(player)->adjustMp(-spellGetRequiredMP(spell));
        }
        break;
    }
    case Spell::PARAM_FROMDIR: {
        screenMessage("From Dir: ");
        Direction dir = gameGetDirection();
        if (dir != DIR_NONE)
            gameCastSpell(spell, player, (int) dir);
        break;
    }
    }    
}

void fire() {
    if (c->transportContext != TRANSPORT_SHIP) {
        screenMessage("Fire What?\n");
        return;
    }

    screenMessage("Fire Cannon!\nDir: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    // can only fire broadsides
    int broadsidesDirs = dirGetBroadsidesDirs(c->party->getDirection());
    if (!DIR_IN_MASK(dir, broadsidesDirs)) {
        screenMessage("Broadsides Only!\n");
        return;
    }

    // nothing (not even mountains!) can block cannonballs
    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), broadsidesDirs, c->location->coords, 
                                                       1, 3, NULL, false);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (fireAt(*i, true))
            return;
    }
}

bool fireAt(const Coords &coords, bool originAvatar) {
    bool validObject = false;
    bool hitsAvatar = false;
    bool objectHit = false;
    
    Object *obj = NULL;

    c->location->map->annotations->add(coords, c->location->map->tileset->getByName("miss_flash")->id, true);
    gameUpdateScreen();

    // based on attack speed setting in setting struct, make a delay
    // for the attack annotation
    int animationDelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    if (animationDelay > 0)
        EventHandler::wait_msecs(animationDelay * 4);

    obj = c->location->map->objectAt(coords);
    Creature *m = dynamic_cast<Creature*>(obj);

    if (obj && obj->getType() == Object::CREATURE && m->isAttackable())
        validObject = true;
    /* See if it's an object to be destroyed (the avatar cannot destroy the balloon) */
    else if (obj && 
             (obj->getType() == Object::UNKNOWN) && 
             !(obj->getTile().getTileType()->isBalloon() && originAvatar))
        validObject = true;

    /* Does the cannon hit the avatar? */
    if (coords == c->location->coords) {
        validObject = true;
        hitsAvatar = true;
    }        

    if (validObject) {
        /* always displays as a 'hit' though the object may not be destroyed */                        
        
        /* Is is a pirate ship firing at US? */
        if (hitsAvatar) {
            CombatController::attackFlash(coords, "hit_flash", 5);

            if (c->transportContext == TRANSPORT_SHIP)
                gameDamageShip(-1, 10);
            else gameDamageParty(10, 25); /* party gets hurt between 10-25 damage */
        }          
        /* inanimate objects get destroyed instantly, while creatures get a chance */
        else if (obj->getType() == Object::UNKNOWN) {
            CombatController::attackFlash(coords, "hit_flash", 5);
            c->location->map->removeObject(obj);
        }
            
        /* only the avatar can hurt other creatures with cannon fire */
        else if (originAvatar) {
            CombatController::attackFlash(coords, "hit_flash", 5);
            if (xu4_random(4) == 0) /* reverse-engineered from u4dos */
                c->location->map->removeObject(obj);
        }
            
        objectHit = true;
    }
        
    c->location->map->annotations->remove(coords, c->location->map->tileset->getByName("miss_flash")->id);

    return objectHit;
}

/**
 * Get the chest at the current x,y of the current context for player 'player'
 */
void getChest(int player) {
    screenMessage("Get Chest!\n");

    if (c->party->isFlying()) {
        screenMessage("Drift only!\n");
        return;
    }
            
    if (player == -1) {
        screenMessage("Who opens? ");
        player = gameGetPlayer(false, true);
    }
    if (player == -1)
        return;

    MapCoords coords;    
    c->location->getCurrentPosition(&coords);
    const Tile *tile = c->location->map->tileTypeAt(coords, WITH_GROUND_OBJECTS);
    MapTile newTile = c->location->getReplacementTile(coords);    

    /* get the object for the chest, if it is indeed an object */
    Object *obj = c->location->map->objectAt(coords);
    if (obj && !obj->getTile().getTileType()->isChest())
        obj = NULL;
    
    if (tile->isChest() || obj) {
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

void holeUp() {
    if (!(c->location->context & (CTX_WORLDMAP | CTX_DUNGEON))) {
        screenMessage("Hole up & Camp\nNot here!\n");
        return;
    }

    if (c->transportContext != TRANSPORT_FOOT) {
        screenMessage("Hole up & Camp\nOnly on foot!\n");
        return;
    }

    screenMessage("Hole up & Camp!\n");

    CombatController *cc = new CampController();
    cc->init(NULL);
    cc->begin();
}

/**
 * Initializes the moon state according to the savegame file. This method of
 * initializing the moons (rather than just setting them directly) is necessary
 * to make sure trammel and felucca stay in sync
 */
void GameController::initMoons()
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
        updateMoons(false);    
}

/**
 * Updates the phases of the moons and shows
 * the visual moongates on the map, if desired
 */
void GameController::updateMoons(bool showmoongates)
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
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x40));
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate)
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x40));
            }
            else if (trammelSubphase == 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x40));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x41));
                }
            }
            else if (trammelSubphase == 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x41));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x42));
                }
            }
            else if (trammelSubphase == 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x42));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x43));
                }
            }
            else if ((trammelSubphase > 3) && (trammelSubphase < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x43));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x43));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x43));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x42));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x42));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x41));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate = moongateGetGateCoordsForPhase(c->saveGame->trammelphase);
                if (gate) {
                    c->location->map->annotations->remove(*gate, c->location->map->translateFromRawTileIndex(0x41));
                    c->location->map->annotations->add(*gate, c->location->map->translateFromRawTileIndex(0x40));
                }
            }
        }
    }
}

/**
 * Handles feedback after avatar moved during normal 3rd-person view.
 */
void GameController::avatarMoved(MoveEvent &event) {
    if (event.userEvent) {

        // is filterMoveMessages even used?  it doesn't look like the option is hooked up in the configuration menu
        if (!settings.filterMoveMessages) {
            switch (c->transportContext) {
                case TRANSPORT_FOOT:
                case TRANSPORT_HORSE:
                    screenMessage("%s\n", getDirectionName(event.dir));
                    break;
                case TRANSPORT_SHIP:
                    if (event.result & MOVE_TURNED)
                        screenMessage("Turn %s!\n", getDirectionName(event.dir));
                    else if (event.result & MOVE_SLOWED)
                        screenMessage("Slow progress!\n");
                    else
                        screenMessage("Sail %s!\n", getDirectionName(event.dir));    
                    break;
                case TRANSPORT_BALLOON:
                    screenMessage("Drift Only!\n");
                    break;
                default:
                    ASSERT(0, "bad transportContext %d in avatarMoved()", c->transportContext);
            }
        }

        /* movement was blocked */
        if (event.result & MOVE_BLOCKED) {

            /* if shortcuts are enabled, try them! */
            if (settings.shortcutCommands) {
                MapCoords new_coords = c->location->coords;
                MapTile *tile;
                
                new_coords.move(event.dir, c->location->map);
                tile = c->location->map->tileAt(new_coords, WITH_OBJECTS);

                if (tile->getTileType()->isDoor()) {
                    openAt(new_coords);
                    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } else if (tile->getTileType()->isLockedDoor()) {
                    jimmyAt(new_coords);
                    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } /*else if (mapPersonAt(c->location->map, new_coords) != NULL) {
                    talkAtCoord(newx, newy, 1, NULL);
                    event.result = MOVE_SUCCEEDED | MOVE_END_TURN;
                    }*/
            }

            /* if we're still blocked */
            if ((event.result & MOVE_BLOCKED) && !settings.filterMoveMessages) {
                soundPlay(SOUND_BLOCKED, false);
                screenMessage("Blocked!\n");
            }
        }
        else if (c->transportContext == TRANSPORT_FOOT || c->transportContext == TRANSPORT_HORSE) {
            /* movement was slowed */
            if (event.result & MOVE_SLOWED) {
                soundPlay(SOUND_WALK_SLOWED);
                screenMessage("Slow progress!\n");
            }
            else {
                soundPlay(SOUND_WALK_NORMAL);
            }
        }
    }

    /* exited map */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("Leaving...\n");
        exitToParentMap();
        musicMgr->play();
    }

    /* things that happen while not on board the balloon */
    if (c->transportContext & ~TRANSPORT_BALLOON)
        checkSpecialCreatures(event.dir);
    /* things that happen while on foot or horseback */
    if ((c->transportContext & TRANSPORT_FOOT_OR_HORSE) &&
        !(event.result & (MOVE_SLOWED|MOVE_BLOCKED))) {
        if (checkMoongates())
            event.result = (MoveResult)(MOVE_MAP_CHANGE | MOVE_END_TURN);
    }
}

/**
 * Handles feedback after moving the avatar in the 3-d dungeon view.
 */
void GameController::avatarMovedInDungeon(MoveEvent &event) {
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
    Direction realDir = dirNormalize((Direction)c->saveGame->orientation, event.dir);

    if (!settings.filterMoveMessages) {
        if (event.userEvent) {
            if (event.result & MOVE_TURNED) {
                if (dirRotateCCW((Direction)c->saveGame->orientation) == realDir)
                    screenMessage("Turn Left\n");
                else screenMessage("Turn Right\n");
            }
            /* show 'Advance' or 'Retreat' in dungeons */
            else screenMessage("%s\n", realDir == c->saveGame->orientation ? "Advance" : "Retreat");
        }

        if (event.result & MOVE_BLOCKED)
            screenMessage("Blocked!\n");       
    }

    /* if we're exiting the map, do this */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("Leaving...\n");
        exitToParentMap();
        musicMgr->play();
    }

    /* check to see if we're entering a dungeon room */
    if (event.result & MOVE_SUCCEEDED) {
        if (dungeon->currentToken() == DUNGEON_ROOM) {            
            int room = (int)dungeon->currentSubToken(); /* get room number */
        
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
            CombatController *cc = new CombatController(dng->roomMaps[room]);
            cc->initDungeonRoom(room, dirReverse(realDir));
            cc->begin();
        }
    }
}

void jimmy() {
    screenMessage("Jimmy\nDir: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 
                                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (jimmyAt(*i))
            return;
    }

    screenMessage("Jimmy what?\n");
}

/**
 * Attempts to jimmy a locked door at map coordinates x,y.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
bool jimmyAt(const Coords &coords) {    
    MapTile *tile = c->location->map->tileAt(coords, WITH_OBJECTS);

    if (!tile->getTileType()->isLockedDoor())
        return false;
        
    if (c->saveGame->keys) {
        Tile *door = c->location->map->tileset->getByName("door");
        ASSERT(door, "no door tile found in tileset");
        c->saveGame->keys--;
        c->location->map->annotations->add(coords, door->id);
        screenMessage("\nUnlocked!\n");
    } else
        screenMessage("No keys left!\n");

    return true;
}

void opendoor() {
    ///  XXX: Pressing "o" should close any open door.
    if (c->party->isFlying()) {
        screenMessage("Open; Not Here!\n");
        return;
    }

    screenMessage("Open\nDir: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 
                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (openAt(*i))
            return;
    }

    screenMessage("Not Here!\n");
}

/**
 * Attempts to open a door at map coordinates x,y.  The door is
 * replaced by a temporary annotation of a floor tile for 4 turns.
 */
bool openAt(const Coords &coords) {
    const Tile *tile = c->location->map->tileTypeAt(coords, WITH_OBJECTS);

    if (!tile->isDoor() && 
        !tile->isLockedDoor())
        return false;

    if (tile->isLockedDoor()) {
        screenMessage("Can't!\n");
        return true;
    }
    
    Tile *floor = c->location->map->tileset->getByName("brick_floor");
    ASSERT(floor, "no floor tile found in tileset");
    c->location->map->annotations->add(coords, floor->id)->setTTL(4);    

    screenMessage("\nOpened!\n");

    return true;
}

/**
 * Readies a weapon for a player.  Prompts for the player and/or the
 * weapon if not provided.
 */
void readyWeapon(int player) {

    // get the player if not provided
    if (player == -1) {
        screenMessage("Ready a weapon\nfor: ");
        player = gameGetPlayer(true, false);
        if (player == -1)
            return;
    }

    // get the weapon to use
    c->stats->setView(STATS_WEAPONS);
    screenMessage("Weapon: ");
    WeaponType weapon = (WeaponType) AlphaActionController::get(WEAP_MAX + 'a' - 1, "Weapon: ");
    c->stats->setView(STATS_PARTY_OVERVIEW);
    if (weapon == -1)
        return;

    PartyMember *p = c->party->member(player);
    const Weapon *w = Weapon::get(weapon);

    switch (p->setWeapon(w)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", w->getName().c_str());
        break;
    case EQUIP_NONE_LEFT:
        screenMessage("None left!\n");
        break;
    case EQUIP_CLASS_RESTRICTED: {
        string indef_article;

        switch(tolower(w->getName()[0])) {
        case 'a': case 'e': case 'i':
        case 'o': case 'u': case 'y':
            indef_article = "an"; break;
        default: 
            indef_article = "a"; break;
        }

        screenMessage("\nA %s may NOT use %s\n%s\n", getClassName(p->getClass()),
                      indef_article.c_str(), w->getName().c_str());
        break;
    }
    }
}

void talk() {
    if (c->party->isFlying()) {
        screenMessage("Talk\nDrift only!\n");
        return;
    }
    
    screenMessage("Talk\nDir: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords, 
                                                                       1, 2, &Tile::canTalkOverTile, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (talkAt(*i))
            return;
    }

    screenMessage("Funny, no\nresponse!\n");
}

/**
 * Mixes reagents.  Prompts for a spell, then which reagents to
 * include in the mix.
 */
void mixReagents() {
    bool done = false;

    while (!done) {
        screenMessage("Mix reagents\n");
        screenMessage("For Spell: ");
        c->stats->setView(STATS_MIXTURES);

        int choice = ReadChoiceController::get("abcdefghijklmnopqrstuvwxyz \033\n\r");
        if (choice == ' ' || choice == '\033' || choice == '\n' || choice == '\r')
            break;

        int spell = choice - 'a';
        screenMessage("%s\n", spellGetName(spell));

        // ensure the mixtures for the spell isn't already maxed out
        if (c->saveGame->mixtures[spell] == 99) {
            screenMessage("\nYou cannot mix any more of that spell!\n");
            break;
        }

        c->stats->setView(STATS_REAGENTS);
        if (settings.enhancements && settings.enhancementsOptions.u5spellMixing)
            done = mixReagentsForSpellU5(spell);
        else
            done = mixReagentsForSpellU4(spell);
    }

    c->stats->setView(STATS_PARTY_OVERVIEW);
    screenMessage("\n\n");
}

/**
 * Prompts for spell reagents to mix in the traditional Ultima IV
 * style.
 */
bool mixReagentsForSpellU4(int spell) {
    Ingredients ingredients;

    screenMessage("Reagent: ");

    while (1) {
        int choice = ReadChoiceController::get("abcdefgh\n\r \033");
            
        // done selecting reagents? mix it up and prompt to mix
        // another spell
        if (choice == '\n' || choice == '\r' || choice == ' ') {
            screenMessage("\n\nYou mix the Reagents, and...\n");

            if (spellMix(spell, &ingredients))
                screenMessage("Success!\n\n");
            else
                screenMessage("It Fizzles!\n\n");

            return false;
        }

        // escape: put ingredients back and quit mixing
        if (choice == '\033') {
            ingredients.revert();
            return true;
        }

        screenMessage("%c\n", toupper(choice));
        if (!ingredients.addReagent((Reagent)(choice - 'a')))
            screenMessage("None Left!\n");
        screenMessage("Reagent: ");
    }

    return true;
}

/**
 * Prompts for spell reagents to mix with an Ultima V-like menu.
 */
bool mixReagentsForSpellU5(int spell) {
    Ingredients ingredients;

    screenDisableCursor();

    c->stats->resetReagentsMenu();
    c->stats->getReagentsMenu()->reset(); // reset the menu, highlighting the first item
    ReagentsMenuController getReagentsController(c->stats->getReagentsMenu(), &ingredients, c->stats->getMainArea());
    eventHandler->pushController(&getReagentsController);
    getReagentsController.waitFor();

    c->stats->getMainArea()->disableCursor();
    screenEnableCursor();

    screenMessage("How many? ");

    int howmany = ReadIntController::get(2, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    gameSpellMixHowMany(spell, howmany, &ingredients);

    return true;
}

/**
 * Exchanges the position of two players in the party.  Prompts the
 * user for the player numbers.
 */
void newOrder() {
    screenMessage("New Order!\nExchange # ");

    int player1 = gameGetPlayer(true, false);

    if (player1 == -1)
        return;

    if (player1 == 0) {
        screenMessage("%s, You must lead!\n", c->party->member(0)->getName().c_str());
        return;
    }

    screenMessage("    with # ");

    int player2 = gameGetPlayer(true, false);

    if (player2 == -1)
        return;

    if (player2 == 0) {
        screenMessage("%s, You must lead!\n", c->party->member(0)->getName().c_str());
        return;
    }

    if (player1 == player2) {
        screenMessage("What?\n");
        return;
    }

    c->party->swapPlayers(player1, player2);
}

/**
 * Peers at a city from A-P (Lycaeum telescope) and functions like a gem
 */
bool gamePeerCity(int city, void *data) {
    Map *peerMap;

    peerMap = mapMgr->get((MapId)(city+1));

    if (peerMap != NULL) {
        game->setMap(peerMap, 1, NULL);
        c->location->viewMode = VIEW_GEM;
        game->paused = true;
        game->pausedTimer = 0;

        screenDisableCursor();
            
        ReadChoiceController::get("\015 \033");

        game->exitToParentMap();
        screenEnableCursor();
        game->paused = false;
    
        return true;
    }
    return false;
}

/**
 * Peers at a gem
 */
void peer(bool useGem) {

    if (useGem) {
        if (c->saveGame->gems <= 0) {
            screenMessage("Peer at What?\n");
            return;
        }

        c->saveGame->gems--;
        screenMessage("Peer at a Gem!\n");
    }

    game->paused = true;
    game->pausedTimer = 0;
    screenDisableCursor();
    
    c->location->viewMode = VIEW_GEM;

    ReadChoiceController::get("\015 \033");

    screenEnableCursor();    
    c->location->viewMode = VIEW_NORMAL;
    game->paused = false;
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
bool talkAt(const Coords &coords) {
    extern int personIsVendor(const Person *person);
    City *city;

    /* can't have any conversations outside of town */
    if (!isCity(c->location->map)) {
        screenMessage("Funny, no\nresponse!\n");
        return true;
    }
    
    city = dynamic_cast<City*>(c->location->map);
    Person *talker = city->personAt(coords);

    /* make sure we have someone we can talk with */
    if (!talker || !talker->canConverse())
        return false;

    /* No response from alerted guards... does any monster both
       attack and talk besides Nate the Snake? */
    if  (talker->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR && 
         talker->getId() != PYTHON_ID)
        return false;

    /* if we're talking to Lord British and the avatar is dead, LB resurrects them! */
    if (talker->getNpcType() == NPC_LORD_BRITISH &&
        c->party->member(0)->getStatus() == STAT_DEAD) {
        screenMessage("%s, Thou shalt live again!\n", c->party->member(0)->getName().c_str());

        c->party->member(0)->setStatus(STAT_GOOD);
        c->party->member(0)->heal(HT_FULLHEAL);
        gameSpellEffect('r', -1, SOUND_LBHEAL);
    }
    
    Conversation conv;
    TRACE_LOCAL(gameDbg, "Setting up script information providers.");
    conv.script->addProvider("party", c->party);
    conv.script->addProvider("context", c);

    conv.state = Conversation::INTRO;
    conv.reply = talker->getConversationText(&conv, "");
    conv.playerInput.erase();
    talkRunConversation(conv, talker, false);

    return true;
}

/**
 * Executes the current conversation until it is done.
 */
void talkRunConversation(Conversation &conv, Person *talker, bool showPrompt) {

    while (conv.state != Conversation::DONE) {
        // TODO: instead of calculating linesused again, cache the
        // result in person.cpp somewhere.
        int linesused = linecount(conv.reply.front(), TEXT_AREA_W);
        screenMessage("%s", conv.reply.front().c_str());
        int size = conv.reply.size();
        conv.reply.pop_front();

        /* if all chunks haven't been shown, wait for a key and process next chunk*/    
        size = conv.reply.size();
        if (size > 0) {    
            ReadChoiceController::get("");
            continue;
        }

        /* otherwise, clear current reply and proceed based on conversation state */
        conv.reply.clear();
    
        /* they're attacking you! */
        if (conv.state == Conversation::ATTACK) {
            conv.state = Conversation::DONE;
            talker->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
        }
    
        if (conv.state == Conversation::DONE)
            break;

        /* When Lord British heals the party */
        else if (conv.state == Conversation::FULLHEAL) {
            int i;

            for (i = 0; i < c->party->size(); i++) {
                c->party->member(i)->heal(HT_CURE);        // cure the party
                c->party->member(i)->heal(HT_FULLHEAL);    // heal the party
            }
            gameSpellEffect('r', -1, SOUND_MAGIC); // same spell effect as 'r'esurrect

            conv.state = Conversation::TALK;
        }
        /* When Lord British checks and advances each party member's level */
        else if (conv.state == Conversation::ADVANCELEVELS) {
            gameLordBritishCheckLevels();
            conv.state = Conversation::TALK;
        }

        if (showPrompt) {
            string prompt = talker->getPrompt(&conv);
            if (!prompt.empty()) {
                if (linesused + linecount(prompt, TEXT_AREA_W) > TEXT_AREA_H)
                    ReadChoiceController::get("");
                screenMessage("%s", prompt.c_str());        
            }
        }

        int maxlen;
        switch (conv.getInputRequired(&maxlen)) {
        case Conversation::INPUT_STRING:
            conv.playerInput = gameGetInput(maxlen);
            conv.reply = talker->getConversationText(&conv, conv.playerInput.c_str());
            conv.playerInput.erase();
            showPrompt = true;
            break;

        case Conversation::INPUT_CHARACTER: {
            char message[2];
            int choice = ReadChoiceController::get("");

            message[0] = choice;
            message[1] = '\0';

            conv.reply = talker->getConversationText(&conv, message);
            conv.playerInput.erase();

            showPrompt = true;
            break;
        }

        case Conversation::INPUT_NONE:
            conv.state = Conversation::DONE;
            break;
        }
    }
    if (conv.reply.size() > 0)
        screenMessage("%s", conv.reply.front().c_str());
}

/**
 * Changes a player's armor.  Prompts for the player and/or the armor
 * type if not provided.
 */
void wearArmor(int player) {

    // get the player if not provided
    if (player == -1) {
        screenMessage("Wear Armour\nfor: ");
        player = gameGetPlayer(true, false);
        if (player == -1)
            return;
    }

    c->stats->setView(STATS_ARMOR);
    screenMessage("Armour: ");
    ArmorType armor = (ArmorType) AlphaActionController::get(ARMR_MAX + 'a' - 1, "Armour: ");
    c->stats->setView(STATS_PARTY_OVERVIEW);
    if (armor == -1)
        return;

    const Armor *a = Armor::get(armor);
    PartyMember *p = c->party->member(player);

    switch (p->setArmor(a)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", a->getName().c_str());
        break;
    case EQUIP_NONE_LEFT:
        screenMessage("None left!\n");
        break;
    case EQUIP_CLASS_RESTRICTED:
        screenMessage("\nA %s may NOT use\n%s\n", getClassName(p->getClass()), a->getName().c_str());
        break;
    }
}

/**
 * Called when the player selects a party member for ztats
 */
void ztatsFor(int player) {
    // get the player if not provided
    if (player == -1) {
        screenMessage("Ztats for: ");
        player = gameGetPlayer(true, false);
        if (player == -1)
            return;
    }

    /* reset the spell mix menu and un-highlight the current item,
       and hide reagents that you don't have */
    c->stats->resetReagentsMenu();

    c->stats->setView(StatsView(STATS_CHAR1 + player));

    ZtatsController ctrl;
    eventHandler->pushController(&ctrl);
    ctrl.waitFor();
}

/**
 * This function is called every quarter second.
 */    
void GameController::timerFired() {

    if (pausedTimer > 0) {
        pausedTimer--;
        if (pausedTimer <= 0) {
            pausedTimer = 0;
            paused = false; /* unpause the game */
        }
    }
    
    if (!paused && !pausedTimer) {
        if (++c->windCounter >= MOON_SECONDS_PER_PHASE * 4) {
            if (xu4_random(4) == 1 && !c->windLock)
                c->windDirection = dirRandomDir(MASK_DIR_ALL);
            c->windCounter = 0;        
        }

        /* balloon moves about 4 times per second */
        if ((c->transportContext == TRANSPORT_BALLOON) &&
            c->party->isFlying()) {
            c->location->move(dirReverse((Direction) c->windDirection), false);
        }        
        
        updateMoons(true);

        screenCycle();

        /*
         * refresh the screen only if the timer queue is empty --
         * i.e. drop a frame if another timer event is about to be fired
         */
        if (eventHandler->timerQueueEmpty())
            gameUpdateScreen();

        /*
         * force pass if no commands within last 20 seconds
         */
        Controller *controller = eventHandler->getController();
        if (controller != NULL && (eventHandler->getController() == game || dynamic_cast<CombatController *>(eventHandler->getController()) != NULL) &&
             gameTimeSinceLastCommand() > 20) {
         
            /* pass the turn, and redraw the text area so the prompt is shown */
            controller->keyPressed(U4_SPACE);
            screenRedrawTextArea(TEXT_AREA_X, TEXT_AREA_Y, TEXT_AREA_W, TEXT_AREA_H);
        }
    }

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
void GameController::checkSpecialCreatures(Direction dir) {
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
            obj = c->location->map->addCreature(creatureMgr->getById(PIRATE_ID), MapCoords(pirateInfo[i].x, pirateInfo[i].y));
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
            obj = c->location->map->addCreature(creatureMgr->getById(DAEMON_ID), MapCoords(231, c->location->coords.y + 1, c->location->coords.z));                    
    }
}

/**
 * Checks for and handles when the avatar steps on a moongate
 */
bool GameController::checkMoongates() {
    Coords dest;
    
    if (moongateFindActiveGateAt(c->saveGame->trammelphase, c->saveGame->feluccaphase, c->location->coords, dest)) {

        gameSpellEffect(-1, -1, SOUND_MOONGATE); // Default spell effect (screen inversion without 'spell' sound effects)
        
        if (c->location->coords != dest) {
            c->location->coords = dest;            
            gameSpellEffect(-1, -1, SOUND_MOONGATE); // Again, after arriving
        }

        if (moongateIsEntryToShrineOfSpirituality(c->saveGame->trammelphase, c->saveGame->feluccaphase)) {
            Shrine *shrine_spirituality;

            shrine_spirituality = dynamic_cast<Shrine*>(mapMgr->get(MAP_SHRINE_SPIRITUALITY));

            if (!c->party->canEnterShrine(VIRT_SPIRITUALITY))
                return true;
            
            setMap(shrine_spirituality, 1, NULL);
            musicMgr->play();

            shrine_spirituality->enter();
        }

        return true;
    }

    return false;
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
            Coords coords(monster->x, monster->y);

            // tile values stored in monsters.sav hardcoded to index into base tilemap
            MapTile tile = TileMap::get("base")->translate(monster->tile),
                oldTile = TileMap::get("base")->translate(monster->prevTile);
            
            if (i < MONSTERTABLE_CREATURES_SIZE) {
                const Creature *creature = creatureMgr->getByTile(tile);
                /* make sure we really have a creature */
                if (creature)
                    obj = map->addCreature(creature, coords);
                else {
                    fprintf(stderr, "Error: A non-creature object was found in the creature section of the monster table. (Tile: %s)\n", tile.getTileType()->getName().c_str());
                    obj = map->addObject(tile, oldTile, coords);
                }
            }
            else
                obj = map->addObject(tile, oldTile, coords);

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
    const Tile *ground;
    
    screenMessage("\nAttacked by %s\n", m->getName().c_str());

    /// TODO: CHEST: Make a user option to not make chests change battlefield
    /// map (2 of 2)
    ground = c->location->map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS);
    if (!ground->isChest()) {
        ground = c->location->map->tileTypeAt(c->location->coords, WITHOUT_OBJECTS);
        if ((under = c->location->map->objectAt(c->location->coords)) && 
            under->getTile().getTileType()->isShip())
            ground = under->getTile().getTileType();
    }

    CombatController *cc = new CombatController(CombatMap::mapForTile(ground, c->party->getTransport().getTileType(), m));
    cc->init(m);
    cc->begin();
}

/**
 * Performs a ranged attack for the creature at x,y on the world map
 */
bool creatureRangeAttack(const Coords &coords, Creature *m) {
    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;

    // Figure out what the ranged attack should look like
    MapTile tile(c->location->map->tileset->getByName((m && !m->getWorldrangedtile().empty()) ? 
                                                      m->getWorldrangedtile() : 
                                                      "hit_flash")->id);

    // See if the attack hits the avatar
    Object *obj = c->location->map->objectAt(coords);        
    m = dynamic_cast<Creature*>(obj);
        
    // Does the attack hit the avatar?
    if (coords == c->location->coords) {
        /* always displays as a 'hit' */
        CombatController::attackFlash(coords, tile, 3);

        /* FIXME: check actual damage from u4dos -- values here are guessed */
        if (c->transportContext == TRANSPORT_SHIP)
            gameDamageShip(-1, 10);
        else gameDamageParty(10, 25);

        return true;
    }
    // Destroy objects that were hit
    else if (obj) {
        if ((obj->getType() == Object::CREATURE && m->isAttackable()) ||
            obj->getType() == Object::UNKNOWN) {
                
            CombatController::attackFlash(coords, tile, 3);
            c->location->map->removeObject(obj);

            return true;
        }            
    }
        
    // Show the attack annotation
    c->location->map->annotations->add(coords, tile, true);
    gameUpdateScreen();

    /* Based on attack speed setting in setting struct, make a delay for
       the attack annotation */
    if (attackdelay > 0)
        EventHandler::wait_msecs(attackdelay * 4);

    c->location->map->annotations->remove(coords, tile);

    return false;    
}

/**
 * Gets the path of coordinates for an action.  Each tile in the
 * direction specified by dirmask, between the minimum and maximum
 * distances given, is included in the path, until blockedPredicate
 * fails.  If a tile is blocked, that tile is included in the path
 * only if includeBlocked is true.
 */
vector<Coords> gameGetDirectionalActionPath(int dirmask, int validDirections, const Coords &origin, int minDistance, int maxDistance, bool (*blockedPredicate)(const Tile *tile), bool includeBlocked) {
    vector<Coords> path;
    Direction dirx = DIR_NONE,
              diry = DIR_NONE;

    /* Figure out which direction the action is going */
    if (DIR_IN_MASK(DIR_WEST, dirmask)) 
        dirx = DIR_WEST;
    else if (DIR_IN_MASK(DIR_EAST, dirmask)) 
        dirx = DIR_EAST;
    if (DIR_IN_MASK(DIR_NORTH, dirmask)) 
        diry = DIR_NORTH;
    else if (DIR_IN_MASK(DIR_SOUTH, dirmask)) 
        diry = DIR_SOUTH;

    /*
     * try every tile in the given direction, up to the given range.
     * Stop when the the range is exceeded, or the action is blocked.
     */
    
    MapCoords t_c(origin);
    if ((dirx <= 0 || DIR_IN_MASK(dirx, validDirections)) && 
        (diry <= 0 || DIR_IN_MASK(diry, validDirections))) {
        for (int distance = 0; distance <= maxDistance;
             distance++, t_c.move(dirx, c->location->map), t_c.move(diry, c->location->map)) {

            if (distance >= minDistance) {
                /* make sure our action isn't taking us off the map */
                if (MAP_IS_OOB(c->location->map, t_c))
                    break;

                const Tile *tile = c->location->map->tileTypeAt(t_c, WITH_GROUND_OBJECTS);

                /* should we see if the action is blocked before trying it? */
                if (!includeBlocked && blockedPredicate &&
                    !(*(blockedPredicate))(tile))
                    break;

                path.push_back(t_c);

                /* see if the action was blocked only if it did not succeed */
                if (includeBlocked && blockedPredicate &&
                    !(*(blockedPredicate))(tile))
                    break;
            }
        }
    }

    return path;
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
    int lastdmged = -1;

    for (i = 0; i < c->party->size(); i++) {
        if (xu4_random(2) == 0) {
            damage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
                xu4_random((maxDamage + 1) - minDamage) + minDamage :
                maxDamage;
            c->party->member(i)->applyDamage(damage);
            c->stats->highlightPlayer(i);
            lastdmged = i;
            EventHandler::wait_msecs(100);
        }
    }
    
    screenShake(2);
    
    // Un-highlight the last player
    if (lastdmged != -1) c->stats->highlightPlayer(lastdmged);
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
        c->party->setActivePlayer(-1);
        screenMessage("Set Active Player: None!\n");
    }
    else if (player < c->party->size()) {
        screenMessage("Set Active Player: %s!\n", c->party->member(player)->getName().c_str());
        if (c->party->member(player)->isDisabled())
            screenMessage("Disabled!\n");
        else 
            c->party->setActivePlayer(player);
    }
}

/**
 * Removes creatures from the current map if they are too far away from the avatar
 */
void GameController::creatureCleanup() {
    ObjectDeque::iterator i;
    Map *map = c->location->map;
    
    for (i = map->objects.begin(); i != map->objects.end();) {
        Object *obj = *i;
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
 * Checks creature conditions and spawns new creatures if necessary
 */
void GameController::checkRandomCreatures() {
    int canSpawnHere = c->location->map->isWorldMap() || c->location->context & CTX_DUNGEON;
    int spawnDivisor = c->location->context & CTX_DUNGEON ? (32 - (c->location->coords.z << 2)) : 32;

    /* If there are too many creatures already,
       or we're not on the world map, don't worry about it! */
    if (!canSpawnHere ||
        c->location->map->getNumberOfCreatures() >= MAX_CREATURES_ON_MAP ||
        xu4_random(spawnDivisor) != 0)
        return;
    
    gameSpawnCreature(NULL);
}

/**
 * Handles trolls under bridges
 */
void GameController::checkBridgeTrolls() {
    const Tile *bridge = c->location->map->tileset->getByName("bridge");
    if (!bridge)
        return;

    // TODO: CHEST: Make a user option to not make chests block bridge trolls
    if (!c->location->map->isWorldMap() ||
        c->location->map->tileAt(c->location->coords, WITH_OBJECTS)->id != bridge->id ||
        xu4_random(8) != 0)
        return;

    screenMessage("\nBridge Trolls!\n");
    
    Creature *m = c->location->map->addCreature(creatureMgr->getById(TROLL_ID), c->location->coords);
    CombatController *cc = new CombatController(MAP_BRIDGE_CON);    
    cc->init(m);
    cc->begin();
}

/**
 * Check the levels of each party member while talking to Lord British
 */
void gameLordBritishCheckLevels() {
    bool advanced = false;

    for (int i = 0; i < c->party->size(); i++) {
        PartyMember *player = c->party->member(i);
        if (player->getRealLevel() <
            player->getMaxLevel())

            // add an extra space to separate messages
            if (!advanced) {
                screenMessage("\n");
                advanced = true;
            }

            player->advanceLevel();
    }
 
    screenMessage("\nWhat would thou\nask of me?\n");
}

/**
 * Spawns a creature (m) just offscreen of the avatar.
 * If (m==NULL) then it finds its own creature to spawn and spawns it.
 */
bool gameSpawnCreature(const Creature *m) {
    int t, i;
    const Creature *creature;
    MapCoords coords = c->location->coords;

    if (c->location->context & CTX_DUNGEON) {
        /* FIXME: for some reason dungeon monsters aren't spawning correctly */

        bool found = false;
        MapCoords new_coords;
        
        for (i = 0; i < 0x20; i++) {
            new_coords = MapCoords(xu4_random(c->location->map->width), xu4_random(c->location->map->height), coords.z);
            const Tile *tile = c->location->map->tileTypeAt(new_coords, WITH_OBJECTS);
            if (tile->isCreatureWalkable()) {
                found = true;
                break;
            }
        }

        if (!found)
            return false;        
        
        coords = new_coords;
    }    
    else {    
        int dx = 0,
            dy = 0;
        bool ok = false;
        int tries = 0;
        static const int MAX_TRIES = 10;

        while (!ok && (tries < MAX_TRIES)) {
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

            /* make sure we can spawn the creature there */
            if (m) {
                MapCoords new_coords = coords;
                new_coords.move(dx, dy, c->location->map);
            
                const Tile *tile = c->location->map->tileTypeAt(new_coords, WITHOUT_OBJECTS);
                if ((m->sails() && tile->isSailable()) || 
                    (m->swims() && tile->isSwimable()) ||
                    (m->walks() && tile->isCreatureWalkable()) ||
                    (m->flies() && tile->isFlyable()))
                    ok = true;
                else tries++;
            }
            else ok = true;
        }

        if (ok)
            coords.move(dx, dy, c->location->map);
    }

    /* can't spawn creatures on top of the player */
    if (coords == c->location->coords)
        return false;    
    
    /* figure out what creature to spawn */
    if (m)
        creature = m;
    else if (c->location->context & CTX_DUNGEON)
        creature = creatureMgr->randomForDungeon(c->location->coords.z);
    else
        creature = creatureMgr->randomForTile(c->location->map->tileTypeAt(coords, WITHOUT_OBJECTS));

    if (creature)
        c->location->map->addCreature(creature, coords);    
    return true;
}

/**
 * Destroys all creatures on the current map.
 */
void gameDestroyAllCreatures(void) {
    int i;
    
    gameSpellEffect('t', -1, SOUND_MAGIC); /* same effect as tremor */
    
    if (c->location->context & CTX_COMBAT) {
        /* destroy all creatures in combat */
        for (i = 0; i < AREA_CREATURES; i++) {            
            CombatMap *cm = getCombatMap();
            CreatureVector creatures = cm->getCreatures();
            CreatureVector::iterator obj;

            for (obj = creatures.begin(); obj != creatures.end(); obj++) {
                if ((*obj)->getId() != LORDBRITISH_ID)
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
                if (m->getId() != LORDBRITISH_ID)
                    current = map->removeObject(current);                
                else current++;
            }
            else current++;
        }
    }

    /* alert the guards! Really, the only one left should be LB himself :) */
    c->location->map->alertGuards();
}

/**
 * Creates the balloon near Hythloth, but only if the balloon doesn't already exists somewhere
 */
bool GameController::createBalloon(Map *map) {
    ObjectDeque::iterator i;    

    /* see if the balloon has already been created (and not destroyed) */
    for (i = map->objects.begin(); i != map->objects.end(); i++) {
        Object *obj = *i;
        if (obj->getTile().getTileType()->isBalloon())
            return false;
    }
    
    const Tile *balloon = map->tileset->getByName("balloon");
    ASSERT(balloon, "no balloon tile found in tileset");
    map->addObject(balloon->id, balloon->id, map->getLabel("balloon"));
    return true;
}
