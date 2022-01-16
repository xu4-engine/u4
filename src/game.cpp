/*
 * game.cpp
 */

#include "game.h"

#include "camp.h"
#include "cheat.h"
#include "city.h"
#include "config.h"
#include "conversation.h"
#include "debug.h"
#include "dungeon.h"
#include "death.h"
#include "debug.h"
#include "error.h"
#include "intro.h"
#include "item.h"
#include "imagemgr.h"
#include "location.h"
#include "mapmgr.h"
#include "portal.h"
#include "progress_bar.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "spell.h"
#include "stats.h"
#include "tileset.h"
#include "u4.h"
#include "weapon.h"
#include "xu4.h"

#ifdef IOS
#include "ios_helpers.h"
#endif

/*-----------------*/
/* Functions BEGIN */

/* spell functions */
void mixReagents();
bool mixReagentsForSpellU4(int spell);
bool mixReagentsForSpellU5(int spell);
void mixReagentsSuper();
void newOrder();

/* conversation functions */
bool talkAt(const Coords &coords);

/* action functions */
bool attackAt(const Coords &coords);
bool destroyAt(const Coords &coords);
bool getChestTrapHandler(int player);
bool jimmyAt(const Coords &coords);
bool openAt(const Coords &coords);
void wearArmor(int player = -1);
void ztatsFor(int player = -1);

/* creature functions */
void gameDestroyAllCreatures(void);
void gameFixupObjects(Map *map, const SaveGameMonsterRecord* table);
void gameCreatureAttack(Creature *obj);

/* Functions END */
/*---------------*/

Context *c = NULL;

MouseArea mouseAreas[] = {
    { 3, { { 8, 8 }, { 8, 184 }, { 96, 96 } }, MC_WEST, { U4_ENTER, 0, U4_LEFT } },
    { 3, { { 8, 8 }, { 184, 8 }, { 96, 96 } }, MC_NORTH, { U4_ENTER, 0, U4_UP }  },
    { 3, { { 184, 8 }, { 184, 184 }, { 96, 96 } }, MC_EAST, { U4_ENTER, 0, U4_RIGHT } },
    { 3, { { 8, 184 }, { 184, 184 }, { 96, 96 } }, MC_SOUTH, { U4_ENTER, 0, U4_DOWN } },
    { 0, {{0,0}, {0,0}, {0,0}}, MC_DEFAULT, {0,0} }
};

ReadPlayerController::ReadPlayerController() : ReadChoiceController("12345678 \033\n") {
#ifdef IOS
    U4IOS::beginCharacterChoiceDialog();
#endif
}

ReadPlayerController::~ReadPlayerController() {
#ifdef IOS
    U4IOS::endCharacterChoiceDialog();
#endif
}

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
        return KeyHandler::defaultHandler(key, NULL);
    }
    return true;
}

int AlphaActionController::get(char lastValidLetter, const string &prompt, EventHandler *eh) {
    if (!eh)
        eh = xu4.eventHandler;

    AlphaActionController ctrl(lastValidLetter, prompt);
    eh->pushController(&ctrl);
    return ctrl.waitFor();
}

GameController::GameController() : TurnController(1),
    mapArea(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W, VIEWPORT_H),
    cutScene(false)
{
    gs_listen(1<<SENDER_LOCATION | 1<<SENDER_PARTY, gameNotice, this);

    // Vendor scripts are shared by all cities.
    discourse_load(&vendorDisc, "vendors");

    // Custom castle dialogue will be loaded on demand.
    discourse_init(&castleDisc);
}

GameController::~GameController() {
    discourse_free(&vendorDisc);
    discourse_free(&castleDisc);

    delete c;
    c = NULL;
}

bool GameController::present() {
    xu4.screenImage->fill(Image::black);

    if (c == NULL || (xu4.intro && xu4.intro->hasInitiatedNewGame()))
        return initContext();   // Loads current savegame

    // Inits screen stuff without renewing game
    initScreenWithoutReloadingState();
    mapArea.reinit();
    return true;
}

void GameController::conclude() {
    mapArea.clear();
    xu4.eventHandler->popMouseAreaSet();
    screenSetMouseCursor(MC_DEFAULT);
}

void GameController::initScreenWithoutReloadingState()
{
    musicPlayLocale();
    xu4.imageMgr->get(BKGD_BORDERS)->image->draw(0, 0);
    c->stats->update(); /* draw the party stats */

    screenMessage("Press Alt-h for help\n");
    screenPrompt();

    xu4.eventHandler->pushMouseAreaSet(mouseAreas);

    xu4.eventHandler->setScreenUpdate(&gameUpdateScreen);
}


class MonstersSav {
public:
    MonstersSav() {
        table = new SaveGameMonsterRecord[MONSTERTABLE_SIZE];
    }
    ~MonstersSav() {
        delete[] table;
    }
    SaveGameMonsterRecord* table;
};

/**
 * Loads the saved game (if not already loaded) and create global game context.
 *
 * Return true if loading is successful.
 */
bool GameController::initContext() {
    Debug gameDbg("debug/game.txt", "Game");
    const Settings& settings = *xu4.settings;

    TRACE(gameDbg, "gameInit() running.");

    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 4);
    pb.setBorderColor(240, 240, 240);
    pb.setBorderWidth(1);
    pb.setColor(0, 0, 128);

    screenTextAt(13, 11, "%s", "Loading Game...");

    /* load in the save game (if not done by intro) */
    if (! xu4.saveGame) {
        if (! saveGameLoad()) {
            xu4.stage = StageIntro;     // Go back to intro.
            return false;
        }
    }
    TRACE_LOCAL(gameDbg, "Save game loaded."); ++pb;

    /* initialize the global game context */
    delete c;
    c = new Context;
    c->saveGame = xu4.saveGame;

    /* initialize conversation and game state variables */
    c->line = TEXT_AREA_H - 1;
    c->col = 0;
    c->stats = new StatsArea();
    c->moonPhase = 0;
    c->windDirection = DIR_NORTH;
    c->windCounter = 0;
    c->windLock = false;
    c->horseSpeed = 0;
    c->opacity = 1;
    c->lastCommandTime = c->commandTimer = 0;
    c->lastShip = NULL;

    TRACE_LOCAL(gameDbg, "Global context initialized.");

    /* initialize our party */
    c->party = new Party(c->saveGame);

    /* set the map to the world map by default */
    setMap(xu4.config->map(MAP_WORLD), 0, NULL);
    c->location->map->clearObjects();

    TRACE_LOCAL(gameDbg, "World map set."); ++pb;

    /* initialize our start location */
    Map *map = xu4.config->restoreMap(MapId(c->saveGame->location));
    TRACE_LOCAL(gameDbg, "Initializing start location.");

    /* if our map is not the world map, then load our map */
    if (map->type != Map::WORLD)
        setMap(map, 1, NULL);
    else
        /* initialize the moons (must be done from the world map) */
        initMoons();


    /**
     * Translate info from the savegame to something we can use
     */
    if (c->location->prev) {
        c->location->coords = Coords(c->saveGame->x, c->saveGame->y, c->saveGame->dnglevel);
        c->location->prev->coords = Coords(c->saveGame->dngx, c->saveGame->dngy);
    }
    else
        c->location->coords = Coords(c->saveGame->x, c->saveGame->y, (int)c->saveGame->dnglevel);

    c->saveGame->orientation = (Direction)(c->saveGame->orientation + DIR_WEST);

    /**
     * Fix the coordinates if they're out of bounds.  This happens every
     * time on the world map because (z == -1) is no longer valid.
     * To maintain compatibility with u4dos, this value gets translated
     * when the game is saved and loaded
     */
    map->putInBounds(c->location->coords);

    TRACE_LOCAL(gameDbg, "Loading monsters."); ++pb;

    /* load in monsters.sav */
    {
    MonstersSav mons;
    FILE* fp = fopen((settings.getUserPath() + MONSTERS_SAV).c_str(), "rb");
    if (fp) {
        saveGameMonstersRead(mons.table, fp);
        fclose(fp);
        gameFixupObjects(map, mons.table);
    }

    /* we have previous creature information as well, load it! */
    if (c->location->prev) {
        fp = fopen((settings.getUserPath() + OUTMONST_SAV).c_str(), "rb");
        if (fp) {
            saveGameMonstersRead(mons.table, fp);
            fclose(fp);
            gameFixupObjects(c->location->prev->map, mons.table);
        }
    }
    }

    spellSetEffectCallback(&gameSpellEffect);
    itemSetDestroyAllCreaturesCallback(&gameDestroyAllCreatures);

    ++pb;

    TRACE_LOCAL(gameDbg, "Settings up reagent menu.");
    c->stats->resetReagentsMenu();

    initScreenWithoutReloadingState();
    TRACE(gameDbg, "gameInit() completed successfully.");
    return true;
}

/**
 * Saves the game state into party.sav and monsters.sav.
 * For dungeons dngmap.sav & outmonst.sav are also created.
 */
int gameSave(const char* userPath) {
    FILE *fp;
    string sbuf(userPath);
    size_t userPathLen = sbuf.size();
    const char* basename;
    const Location* loc = c->location;
    const Map* map = loc->map;
    SaveGame save = *c->saveGame;
    MonstersSav mons;

#define openSaveFile(FN) \
    basename = FN; \
    sbuf.erase(userPathLen, string::npos); \
    sbuf.append(basename); \
    fp = fopen(sbuf.c_str(), "wb"); \
    if (!fp) { \
        screenMessage("Error opening %s\n", basename); \
        return 0; \
    }

    /*************************************************/
    /* Make sure the savegame struct is accurate now */

    if (loc->prev) {
        save.x = loc->coords.x;
        save.y = loc->coords.y;
        save.dnglevel = loc->coords.z;
        save.dngx = loc->prev->coords.x;
        save.dngy = loc->prev->coords.y;
    } else {
        save.x = loc->coords.x;
        save.y = loc->coords.y;
        save.dnglevel = loc->coords.z;
        save.dngx = c->saveGame->dngx;
        save.dngy = c->saveGame->dngy;
    }
    save.location = map->id;
    save.orientation = (Direction)(c->saveGame->orientation - DIR_WEST);

    /* Done making sure the savegame struct is accurate */
    /****************************************************/


    openSaveFile(PARTY_SAV);
    if (! save.write(fp))
        goto write_error;
    fclose(fp);


    openSaveFile(MONSTERS_SAV);
    if (map->type == Map::DUNGEON)
        map->fillMonsterTableDungeon(mons.table);
    else
        map->fillMonsterTable(mons.table);
    if (! saveGameMonstersWrite(mons.table, fp))
        goto write_error;
    fclose(fp);


    /**
     * Write dngmap.sav & outmonst.sav
     */
    if (loc->context & CTX_DUNGEON) {
        openSaveFile(DNGMAP_SAV);
        const uint8_t* data = static_cast<Dungeon*>((Map*) map)->fillRawMap();
        size_t dataLen = map->width * map->height * map->levels;
        fwrite(data, 1, dataLen, fp);
        fclose(fp);


        openSaveFile(OUTMONST_SAV);
        loc->prev->map->fillMonsterTable(mons.table);
        if (! saveGameMonstersWrite(mons.table, fp))
            goto write_error;
        fclose(fp);
    }
    return 1;

write_error:
    fclose(fp);
    screenMessage("Error writing to %s\n", basename);
    return 0;
}

/**
 * Sets the view mode.
 */
void gameSetViewMode(ViewMode newMode) {
    switch (newMode) {
        case VIEW_GEM:
        case VIEW_CUTSCENE:
        case VIEW_CUTSCENE_MAP:
            xu4.game->cutScene = true;
            break;
        default:
            xu4.game->cutScene = false;
            break;
    }

    c->location->viewMode = newMode;
}

void gameUpdateScreen() {
    switch (c->location->viewMode) {
    case VIEW_NORMAL:
    case VIEW_CUTSCENE_MAP:
        screenUpdate(&xu4.game->mapArea, true, false);
        break;
    case VIEW_GEM:
        screenGemUpdate();
        break;
    case VIEW_DUNGEON:
        screenUpdate(&xu4.game->mapArea, true, false);
        break;
    case VIEW_CUTSCENE: /* the screen updates will be handled elsewhere */
        break;
    case VIEW_MIXTURES: /* still testing */
        break;
    default:
        ASSERT(0, "invalid view mode: %d", c->location->viewMode);
    }
}

void GameController::setMap(Map *map, bool saveLocation, const Portal *portal, TurnController *turnCompleter) {
    int viewMode;
    LocationContext context;
    int activePlayer = c->party->getActivePlayer();
    Coords coords;

    if (!turnCompleter)
        turnCompleter = this;

    if (portal)
        coords = portal->start;
    else
        coords = Coords(map->width / 2, map->height / 2);

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
        screenMakeDungeonView();
        break;
    case Map::COMBAT:
        context = CTX_COMBAT;
        viewMode = VIEW_NORMAL;
        activePlayer = -1; /* different active player for combat, defaults to 'None' */
        break;
    case Map::SHRINE:
        context = CTX_SHRINE;
        viewMode = VIEW_NORMAL;
        break;
    case Map::CITY:
    default:
        context = CTX_CITY;
        viewMode = VIEW_NORMAL;
        break;
    }
    c->location = new Location(coords, map, viewMode, context, turnCompleter, c->location);
    c->party->setActivePlayer(activePlayer);
#ifdef IOS
    U4IOS::updateGameControllerContext(c->location->context);
#endif

    if (isCity(map)) {
        City *city = dynamic_cast<City*>(map);
        city->addPeople();
    }

    gameStampCommandTime();     // Restart turn Pass timer.
}

/**
 * Exits the current map and location and returns to its parent location
 * This restores all relevant information from the previous location,
 * such as the map, map position, etc. (such as exiting a city)
 */
int GameController::exitToParentMap() {
    Location* loc = c->location;
    if (loc && loc->prev) {
        Map* currentMap = loc->map;
        Map* prevMap = loc->prev->map;

        // Create the balloon for Hythloth
        if (currentMap->id == MAP_HYTHLOTH)
            createBalloon(prevMap);

        // free map info only if previous location was on a different map
        if (prevMap != currentMap) {
            currentMap->annotations.clear();
            currentMap->clearObjects();

            /* quench the torch of we're on the world map */
            if (prevMap->isWorldMap())
                c->party->quenchTorch();
        }

        locationFree(&c->location);
#ifdef IOS
        U4IOS::updateGameControllerContext(c->location->context);
#endif
        gameStampCommandTime();     // Restart turn Pass timer.
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
    gameStampCommandTime();

    while (xu4.stage == StagePlay) {
        Map* map = c->location->map;

        /* adjust food and moves */
        c->party->endTurn();

        /* count down the aura, if there is one */
        c->aura.passTurn();

        gameCheckHullIntegrity();

        /* update party stats */
        //c->stats->setView(STATS_PARTY_OVERVIEW);

        screenUpdate(&this->mapArea, true, false);
        screenWait(1);

        /* Creatures cannot spawn, move or attack while the avatar is on the balloon */
        if (!c->party->isFlying()) {

            // apply effects from tile avatar is standing on
            c->party->applyEffect(map, map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS)->getEffect());

            // Move creatures and see if something is attacking the avatar
            Creature* attacker = map->moveObjects(c->location->coords);

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
        map->annotations.passTurn();

        if (!c->party->isImmobilized())
            break;

        if (c->party->isDead()) {
            deathStart(0);
            return;
        } else {
            screenMessage("Zzzzzz\n");
            EventHandler::wait_msecs(166);  // Four video frames at 24 fps.
        }
    }

    if (c->location->context == CTX_DUNGEON) {
        Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);
        if (! c->party->burnTorch())
            screenMessage("It's Dark!\n");

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
}

/**
 * Show an attack flash at x, y on the current map.
 * This is used for 'being hit' or 'being missed'
 * by weapons, cannon fire, spells, etc.
 */
void GameController::flashTile(const Coords &coords, MapTile tile, int frames) {
#ifdef GPU_RENDER
    int fx = xu4.game->mapArea.showEffect(coords, tile.id);
#else
    Map* map = c->location->map;
    map->annotations.add(coords, tile, true);
    screenTileUpdate(&xu4.game->mapArea, coords);
    screenUploadToGPU();
#endif

    EventHandler::wait_msecs(frames * 1000 /
                             xu4.settings->screenAnimationFramesPerSecond);

#ifdef GPU_RENDER
    xu4.game->mapArea.removeEffect(fx);
#else
    map->annotations.remove(coords, tile);
    screenTileUpdate(&xu4.game->mapArea, coords);
#endif
}

void GameController::flashTile(const Coords &coords, Symbol tilename, int timeFactor) {
    const Tile *tile = Tileset::findTileByName(tilename);
    ASSERT(tile, "no tile named '%s' found in tileset", xu4.config->symbolName(tilename));
    flashTile(coords, MapTile(tile->getId()), timeFactor);
}

void GameController::gameNotice(int sender, void* eventData, void* user) {
    if (sender == SENDER_LOCATION)
    {
        MoveEvent* ev = (MoveEvent*) eventData;
        switch (ev->location->map->type) {
        case Map::DUNGEON:
            ((GameController*) user)->avatarMovedInDungeon(*ev);
            break;

        case Map::COMBAT:
            // FIXME: let the combat controller handle it
            dynamic_cast<CombatController *>(xu4.eventHandler->getController())->movePartyMember(*ev);
            break;

        default:
            ((GameController*) user)->avatarMoved(*ev);
            break;
        }
    }
    else if (sender == SENDER_PARTY)
    {
        // Provide feedback to user after a party event happens.
        PartyEvent* ev = (PartyEvent*) eventData;
        switch (ev->type) {
        case PartyEvent::LOST_EIGHTH:
            // inform a player he has lost zero or more eighths of avatarhood.
            screenMessage("\n %cThou hast lost\n  an eighth!%c\n",
                          FG_YELLOW, FG_WHITE);
            break;

        case PartyEvent::ADVANCED_LEVEL:
            screenMessage("\n%c%s\nThou art now Level %d%c\n", FG_YELLOW,
                    ev->player->getName().c_str(),
                    ev->player->getRealLevel(), FG_WHITE);
            gameSpellEffect('r', -1, SOUND_MAGIC); // Same as resurrect spell
            break;

        case PartyEvent::STARVING:
            screenMessage("\n%cStarving!!!%c\n", FG_YELLOW, FG_WHITE);
            /* FIXME: add sound effect here */

            // 2 damage to each party member for starving!
            for (int i = 0; i < c->saveGame->members; i++)
                c->party->member(i)->applyDamage(c->location->map, 2);
            break;

        default:
            break;
        }
    }
}

void gameSpellEffect(int spell, int player, Sound sound) {

    int time;
    Spell::SpecialEffects effect = Spell::SFX_INVERT;

    if (player >= 0)
        c->stats->highlightPlayer(player);

    time = xu4.settings->spellEffectSpeed * 800 / xu4.settings->gameCyclesPerSecond;
    soundPlay(sound, false, time);

    ///The following effect multipliers are not accurate
    switch(spell)
    {
    case 'g': /* gate */
    case 'r': /* resurrection */
        break;
    case 't': /* tremor */
        effect = Spell::SFX_TREMOR;
        break;
    default:
        /* default spell effect */
        break;
    }

    switch(effect)
    {
    case Spell::SFX_NONE:
        break;
    case Spell::SFX_TREMOR:
    case Spell::SFX_INVERT:
        gameUpdateScreen();
        xu4.game->mapArea.highlight(0, 0, VIEWPORT_W * TILE_WIDTH, VIEWPORT_H * TILE_HEIGHT);
        EventHandler::wait_msecs(time);
        xu4.game->mapArea.unhighlight();

        if (effect == Spell::SFX_TREMOR) {
            gameUpdateScreen();
            soundPlay(SOUND_RUMBLE, false);
            screenShake(8);
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
            screenMessage("%s", msg.c_str());
    }
}

/**
 * The main key handler for the game.  Interpretes each key as a
 * command - 'a' for attack, 't' for talk, etc.
 */
bool GameController::keyPressed(int key) {
    Settings& settings = *xu4.settings;
    bool valid = true;
    int endTurn = 1;

    /* Translate context-sensitive action key into a useful command */
    if (key == U4_ENTER && settings.enhancements && settings.enhancementsOptions.smartEnterKey) {
        /* Attempt to guess based on the character's surroundings etc, what
           action they want */

        /* Do they want to board something? */
        if (c->transportContext == TRANSPORT_FOOT) {
            const Object* obj =
                        c->location->map->objectAt(c->location->coords);
            if (obj) {
                const Tile* tile = obj->tile.getTileType();
                if (tile->isShip() ||
                    tile->isHorse() ||
                    tile->isBalloon())
                key = 'b';
            }
        }
        /* Klimb/Descend Balloon */
        else if (c->transportContext == TRANSPORT_BALLOON) {
            if (c->party->isFlying())
                key = 'd';
            else {
#ifdef IOS
                U4IOS::IOSSuperButtonHelper superHelper;
                key = ReadChoiceController::get("xk \033\n");
#else
                key = 'k';
#endif
            }
        }
        /* X-it transport */
        else key = 'x';

        /* Klimb? */
        if ((c->location->map->portalAt(c->location->coords, ACTION_KLIMB) != NULL))
            key = 'k';
        /* Descend? */
        else if ((c->location->map->portalAt(c->location->coords, ACTION_DESCEND) != NULL))
            key = 'd';

        if (c->location->context == CTX_DUNGEON) {
            Dungeon *dungeon = static_cast<Dungeon *>(c->location->map);
            bool up = dungeon->ladderUpAt(c->location->coords);
            bool down = dungeon->ladderDownAt(c->location->coords);
            if (up && down) {
#ifdef IOS
                U4IOS::IOSClimbHelper climbHelper;
                key = ReadChoiceController::get("kd \033\n");
#else
                key = 'k'; // This is consistent with the previous code. Ideally, I would have a UI here as well.
#endif
            } else if (up) {
                key = 'k';
            } else {
                key = 'd';
            }
        }

        /* Enter? */
        if (c->location->map->portalAt(c->location->coords, ACTION_ENTER) != NULL)
            key = 'e';

        /* Get Chest? */
        if (!c->party->isFlying()) {
            const Tile* tile;
            tile = c->location->map->tileTypeAt(c->location->coords, WITH_GROUND_OBJECTS);
            if (tile->isChest())
                key = 'g';
        }

        /* None of these? Default to search */
        if (key == U4_ENTER) key = 's';
    }

    if ((c->location->context & CTX_DUNGEON) && strchr("abefjlotxy", key))
        screenMessage("%cNot here!%c\n", FG_GREY, FG_WHITE);
    else
        switch (key) {

        case U4_UP:
        case U4_DOWN:
        case U4_LEFT:
        case U4_RIGHT:
            {
                /* move the avatar */
                StringId previous_map = c->location->map->fname;
                MoveResult retval = c->location->move(keyToDirection(key), true);

                /* horse doubles speed (make sure we're on the same map as the previous move first) */
                if (retval & (MOVE_SUCCEEDED | MOVE_SLOWED) &&
                    c->transportContext == TRANSPORT_HORSE &&
                    c->horseSpeed == HORSE_GALLOP) {
                    gameUpdateScreen(); /* to give it a smooth look of movement */
                    if (previous_map == c->location->map->fname) {
                        EventHandler::wait_msecs(166);
                        c->location->move(keyToDirection(key), false);
                    }
                }
                if (c->horseSpeed == HORSE_GALLOP_INTERRUPT)
                    c->horseSpeed = HORSE_GALLOP;

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
#if 1
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(xu4.config->map(MAP_DECEIT), 1, NULL);
                c->location->coords = Coords(1, 0, 7);
                c->saveGame->orientation = DIR_SOUTH;
            }
            else valid = false;
#else
            screenShake(8);
#endif
            break;

        case U4_FKEY+9:
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(xu4.config->map(MAP_DESPISE), 1, NULL);
                c->location->coords = Coords(3, 2, 7);
                c->saveGame->orientation = DIR_SOUTH;
            }
            else valid = false;
            break;

        case U4_FKEY+10:
            if (settings.debug && (c->location->context & CTX_WORLDMAP)) {
                setMap(xu4.config->map(MAP_DESTARD), 1, NULL);
                c->location->coords = Coords(7, 6, 7);
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
                xu4.eventHandler->pushController(&cheatMenuController);
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
                setMap(xu4.config->map(MAP_CASTLE_LB2), 1, NULL);
                c->location->coords.x = 19;
                c->location->coords.y = 8;
                c->location->coords.z = 0;
            }
            else valid = false;
            break;

        case 22:                    /* ctrl-V */
            {
                if (settings.debug && c->location->context == CTX_DUNGEON) {
                    screenMessage("3-D view %s\n", screenToggle3DDungeonView() ? "on" : "off");
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
                    xu4.eventHandler->setTimerInterval(1000 /
                                                settings.gameCyclesPerSecond);

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
            screenMessage("Music: %d%s\n", musicVolumeDec(), "%");
            endTurn = false;
            break;
        case '.':
            // increase the volume if possible
            screenMessage("Music: %d%s\n", musicVolumeInc(), "%");
            endTurn = false;
            break;

        /* handle sound volume adjustments */
        case '<':
            // decrease the volume if possible
            screenMessage("Sound: %d%s\n", soundVolumeDec(), "%");
            soundPlay(SOUND_FLEE);
            endTurn = false;
            break;
        case '>':
            // increase the volume if possible
            screenMessage("Sound: %d%s\n", soundVolumeInc(), "%");
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
                        screenMessage("%cAlready Landed!%c\n", FG_GREY, FG_WHITE);
                    else if (c->location->map->tileTypeAt(c->location->coords, WITH_OBJECTS)->canLandBalloon()) {
                        c->saveGame->balloonstate = 0;
                        c->opacity = 1;
                    }
                    else screenMessage("%cNot Here!%c\n", FG_GREY, FG_WHITE);
                }
                else screenMessage("%cDescend what?%c\n", FG_GREY, FG_WHITE);
            }
            break;

        case 'e':
            if (!usePortalAt(c->location, c->location->coords, ACTION_ENTER)) {
                if (!c->location->map->portalAt(c->location->coords, ACTION_ENTER))
                    screenMessage("%cEnter what?%c\n", FG_GREY, FG_WHITE);
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
            // FIXME: The entire resting scene should not be run inside the
            // key handler.
            holeUp();
            break;

        case 'i':
            screenMessage("Ignite torch!\n");
            if (c->location->context == CTX_DUNGEON) {
                if (!c->party->lightTorch())
                    screenMessage("%cNone left!%c\n", FG_GREY, FG_WHITE);
            }
            else screenMessage("%cNot here!%c\n", FG_GREY, FG_WHITE);
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
                    screenMessage("%cKlimb what?%c\n", FG_GREY, FG_WHITE);
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
                    screenMessage("%cLocate position with what?%c\n", FG_GREY, FG_WHITE);
            }
            else screenMessage("%cNot here!%c\n", FG_GREY, FG_WHITE);
            break;

        case 'm':
            mixReagents();
#ifdef IOS
            // The iOS MixSpell dialog needs control of the event loop, so it is its
            // job to complete the turn.
            endTurn = false;
#endif
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
                gameSave(xu4.settings->getUserPath().c_str());
                screenMessage("Press Alt-x to quit\n");
            }
            else screenMessage("%cNot here!%c\n", FG_GREY, FG_WHITE);

            break;

        case 'r':
            readyWeapon();
            break;

        case 's':
            if (c->location->context == CTX_DUNGEON)
                dungeonSearch();
            else if (c->party->isFlying())
                screenMessage("Searching...\n%cDrift only!%c\n", FG_GREY, FG_WHITE);
            else {
                screenMessage("Searching...\n");

                const ItemLocation *item = itemAtLocation(c->location->map, c->location->coords);
                if (item) {
                    if (*item->isItemInInventory != NULL && (*item->isItemInInventory)(item->data))
                        screenMessage("%cNothing Here!%c\n", FG_GREY, FG_WHITE);
                    else {
                        if (item->name)
                            screenMessage("You find...\n%s!\n", item->name);
                        (*item->putItemInInventory)(item->data);
                    }
                } else
                    screenMessage("%cNothing Here!%c\n", FG_GREY, FG_WHITE);
            }

            break;

        case 't':
            talk();
            break;

        case 'u': {
            screenMessage("Use which item:\n");
            if (settings.enhancements) {
                /* a little xu4 enhancement: show items in inventory when prompted for an item to use */
                c->stats->setView(STATS_ITEMS);
            }
#ifdef IOS
            U4IOS::IOSConversationHelper::setIntroString("Use which item?");
#endif
            itemUse(gameGetInput().c_str());
            break;
        }

        case 'v':
            if (musicToggle())
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

                const Tile *avatar = c->location->map->tileset->getByName(Tile::sym.avatar);
                ASSERT(avatar, "no avatar tile found in tileset");
                c->party->setTransport(avatar->getId());
                c->horseSpeed = 0;
                screenMessage("X-it\n");
            } else
                screenMessage("%cX-it What?%c\n", FG_GREY, FG_WHITE);
            break;

        case 'y':
            screenMessage("Yell ");
            if (c->transportContext == TRANSPORT_HORSE) {
                if (c->horseSpeed == 0) {
                    screenMessage("Giddyup!\n");
                    c->horseSpeed = HORSE_GALLOP;
                } else {
                    screenMessage("Whoa!\n");
                    c->horseSpeed = 0;
                }
            } else
                screenMessage("%cWhat?%c\n", FG_GREY, FG_WHITE);
            break;

        case 'z':
            ztatsFor();
            break;

        case 'c' + U4_ALT:
            if (settings.debug && c->location->map->isWorldMap()) {
                /* first teleport to the abyss */
                c->location->coords.x = 0xe9;
                c->location->coords.y = 0xe9;
                setMap(xu4.config->map(MAP_ABYSS), 1, NULL);
                /* then to the final altar */
                c->location->coords.x = 7;
                c->location->coords.y = 7;
                c->location->coords.z = 7;
            }
            break;

        case 'h' + U4_ALT: {
#ifdef IOS
            U4IOS::IOSHideActionKeysHelper hideActionKeys;
#endif
            ReadChoiceController pauseController("");

            screenMessage("Key Reference:\n"
                          "Arrow Keys: Move\n"
                          "a: Attack\n"
                          "b: Board\n"
                          "c: Cast Spell\n"
                          "d: Descend\n"
                          "e: Enter\n"
                          "f: Fire Cannons\n"
                          "g: Get Chest\n"
                          "h: Hole up\n"
                          "i: Ignite torch\n"
                          "(more)");

            xu4.eventHandler->pushController(&pauseController);
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

            xu4.eventHandler->pushController(&pauseController);
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

            xu4.eventHandler->pushController(&pauseController);
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
                endTurn = false;

                screenMessage("Quit to menu?");
                char choice = ReadChoiceController::get("yn \n\033");
                if (choice != 'y') {
                    screenMessage("\n");
                    break;
                }

                xu4.eventHandler->setScreenUpdate(NULL);

                // Fade out the music and hide the cursor
                // before returning to the menu.
                musicFadeOut(1000);
                screenHideCursor();

                xu4.stage = StageIntro;
                xu4.eventHandler->setControllerDone();
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
            else screenMessage("%cBad command!%c\n", FG_GREY, FG_WHITE);

            endTurn = 0;
            break;

        default:
            valid = false;
            break;
        }

    if (valid && endTurn) {
        if (xu4.eventHandler->getController() == xu4.game)
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
#ifdef IOS
    U4IOS::IOSConversationHelper helper;
    helper.beginConversation(U4IOS::UIKeyboardTypeDefault);
#endif

    return ReadStringController::get(maxlen, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
}

int gameGetPlayer(bool canBeDisabled, bool canBeActivePlayer) {
    int player;
    if (c->saveGame->members <= 1)
    {
        player = 0;
    }
    else
    {
        if (canBeActivePlayer && (c->party->getActivePlayer() >= 0))
        {
            player = c->party->getActivePlayer();
        }
        else
        {
            ReadPlayerController readPlayerController;
            xu4.eventHandler->pushController(&readPlayerController);
            player = readPlayerController.waitFor();
            if (player >= 0)
                c->col--;   // Will display the name in place of the number
        }

        if (player == -1)
        {
            screenMessage("None\n");
            return -1;
        }
    }

    if ((player >= 0) && (player < 8))
    {
        // Write player's name after prompt
        screenMessage("%s\n", c->saveGame->players[player].name);
    }

    if (!canBeDisabled && c->party->member(player)->isDisabled())
    {
        screenMessage("%cDisabled!%c\n", FG_GREY, FG_WHITE);
        return -1;
    }

    ASSERT(player < c->party->size(), "player %d, but only %d members\n", player, c->party->size());
    return player;
}

Direction gameGetDirection() {
    ReadDirController dirController;

    screenMessage("Dir?");
#ifdef IOS
    U4IOS::IOSDirectionHelper directionPopup;
#endif

    xu4.eventHandler->pushController(&dirController);
    Direction dir = dirController.waitFor();

    screenMessage("\b\b\b\b");

    if (dir == DIR_NONE) {
        screenMessage("    \n");
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
        screenMessage("\n%cOnly need %d!%c\n", FG_GREY, num, FG_WHITE);
    }

    screenMessage("\nMixing %d...\n", num);

    /* see if there's enough reagents to make number of mixtures requested */
    if (!ingredients->checkMultiple(num)) {
        screenMessage("\n%cYou don't have enough reagents to mix %d spells!%c\n", FG_GREY, num, FG_WHITE);
        ingredients->revert();
        return false;
    }

    screenMessage("\nYou mix the Reagents, and...\n");
    if (spellMix(spell, ingredients)) {
        screenMessage("Success!\n\n");
        /* mix the extra spells */
        ingredients->multiply(num);
        for (i = 0; i < num-1; i++)
            spellMix(spell, ingredients);
    }
    else
        screenMessage("It Fizzles!\n\n");

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
        c->stats->setView(StatsView(STATS_PARTY_OVERVIEW));
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

    screenMessage("%cNothing there!%c\n", FG_GREY, FG_WHITE);
}

bool destroyAt(const Coords &coords) {
    Object *obj = c->location->map->objectAt(coords);

    if (obj) {
        if (isCreature(obj)) {
            Creature *c = dynamic_cast<Creature*>(obj);
            screenMessage("%s Destroyed!\n", c->getName().c_str());
        }
        else {
            const Tile* tile = obj->tile.getTileType();
            screenMessage("%s Destroyed!\n", tile->nameStr());
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
        screenMessage("\n%cDrift only!%c\n", FG_GREY, FG_WHITE);
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

    screenMessage("%cNothing to Attack!%c\n", FG_GREY, FG_WHITE);
}

static const Tile* battleGround(const Map* map, const Coords& coord) {
    const Tile* ground = map->tileTypeAt(coord, WITH_GROUND_OBJECTS);

    // TODO: CHEST: Make a user option to not make chests change
    //       battlefield map.

    if (! ground->isChest()) {
        ground = map->tileTypeAt(coord, WITHOUT_OBJECTS);
        const Object* under = map->objectAt(coord);
        if (under) {
            const Tile* tile = under->tile.getTileType();
            if (tile->isShip())
                ground = tile;
        }
    }
    return ground;
}

/**
 * Attempts to attack a creature at map coordinates x,y.  If no
 * creature is present at that point, zero is returned.
 */
bool attackAt(const Coords &coords) {
    const Tile *ground;
    Creature *m;
    Map* map = c->location->map;

    m = dynamic_cast<Creature*>(map->objectAt(coords));
    /* nothing attackable: move on to next tile */
    if (m == NULL || !m->isAttackable())
        return false;

    /* attack successful */
    ground = battleGround(map, c->location->coords);

    /* You're attacking a townsperson!  Alert the guards! */
    if ((m->objType == Object::PERSON) &&
        (m->movement != MOVEMENT_ATTACK_AVATAR))
        map->alertGuards();

    /* not good karma to be killing the innocent.  Bad avatar! */
    if (m->isGood() || /* attacking a good creature */
        /* attacking a docile (although possibly evil) person in town */
        ((m->objType == Object::PERSON) &&
         (m->movement != MOVEMENT_ATTACK_AVATAR)))
        c->party->adjustKarma(KA_ATTACKED_GOOD);

    CombatController::engage(CombatMap::mapForTile(ground,
                        c->party->getTransport().getTileType(), m), m);
    return true;
}

void board() {
    if (c->transportContext != TRANSPORT_FOOT) {
        screenMessage("Board: %cCan't!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    Object *obj = c->location->map->objectAt(c->location->coords);
    if (!obj) {
        screenMessage("%cBoard What?%c\n", FG_GREY, FG_WHITE);
        return;
    }

    const Tile *tile = obj->tile.getTileType();
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
        screenMessage("%cBoard What?%c\n", FG_GREY, FG_WHITE);
        return;
    }

    c->party->setTransport(obj->tile);
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
    // ### Put the iPad thing too.
#ifdef IOS
    U4IOS::IOSCastSpellHelper castSpellController;
#endif
    int spell = AlphaActionController::get('z', "Spell: ");
    if (spell == -1)
        return;

    screenMessage("%s!\n", spellGetName(spell)); //Prints spell name at prompt

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
#ifdef IOS
        U4IOS::IOSConversationChoiceHelper choiceController;
        choiceController.fullSizeChoicePanel();
        choiceController.updateGateSpellChoices();
#endif
        int choice = ReadChoiceController::get("12345678 \033\n");
        if (choice < '1' || choice > '8')
            screenMessage("None\n");
        else {
            screenMessage("\n");
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
#ifdef IOS
        U4IOS::IOSConversationChoiceHelper choiceController;
        choiceController.fullSizeChoicePanel();
        choiceController.updateEnergyFieldSpellChoices();
#endif
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
            screenMessage("\n");

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
        screenMessage("%cFire What?%c\n", FG_GREY, FG_WHITE);
        return;
    }

    screenMessage("Fire Cannon!\nDir: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    // can only fire broadsides
    int broadsidesDirs = dirGetBroadsidesDirs(c->party->getDirection());
    if (!DIR_IN_MASK(dir, broadsidesDirs)) {
        screenMessage("%cBroadsides Only!%c\n", FG_GREY, FG_WHITE);
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


    MapTile tile(c->location->map->tileset->getByName(Tile::sym.missFlash)->getId());
    GameController::flashTile(coords, tile, 1);

    obj = c->location->map->objectAt(coords);
    Creature *m = dynamic_cast<Creature*>(obj);

    if (obj && obj->objType == Object::CREATURE && m->isAttackable())
        validObject = true;
    /* See if it's an object to be destroyed (the avatar cannot destroy the balloon) */
    else if (obj && (obj->objType == Object::UNKNOWN) &&
             ! (obj->tile.getTileType()->isBalloon() && originAvatar))
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
            GameController::flashTile(coords, Tile::sym.hitFlash, 4);

            if (c->transportContext == TRANSPORT_SHIP)
                gameDamageShip(-1, 10);
            else gameDamageParty(10, 25); /* party gets hurt between 10-25 damage */
        }
        /* inanimate objects get destroyed instantly, while creatures get a chance */
        else if (obj->objType == Object::UNKNOWN) {
            GameController::flashTile(coords, Tile::sym.hitFlash, 4);
            c->location->map->removeObject(obj);
        }

        /* only the avatar can hurt other creatures with cannon fire */
        else if (originAvatar) {
            GameController::flashTile(coords, Tile::sym.hitFlash, 4);
            if (xu4_random(4) == 0) /* reverse-engineered from u4dos */
                c->location->map->removeObject(obj);
        }

        objectHit = true;
    }

    return objectHit;
}

/**
 * Get the chest at the current x,y of the current context for player 'player'
 */
void getChest(int player)
{
    screenMessage("Get Chest!\n");

    if (c->party->isFlying())
    {
        screenMessage("%cDrift only!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    // first check to see if a chest exists at the current location
    // if one exists, prompt the player for the opener, if necessary
    Coords coords;
    Location* loc = c->location;
    loc->getCurrentPosition(&coords);
    const Tile *tile = loc->map->tileTypeAt(coords, WITH_GROUND_OBJECTS);

    /* get the object for the chest, if it is indeed an object */
    Object *obj = loc->map->objectAt(coords);
    if (obj && ! obj->tile.getTileType()->isChest())
        obj = NULL;

    if (tile->isChest() || obj)
    {
        // if a spell was cast to open this chest,
        // player will equal -2, otherwise player
        // will default to -1 or the defult character
        // number if one was earlier specified
        if (player == -1)
        {
            screenMessage("Who opens? ");
            player = gameGetPlayer(false, true);
        }
        if (player == -1)
            return;

        if (obj) {
            //printf( "KR getChest obj\n" );
            loc->map->removeObject(obj);
        } else {
            //printf( "KR getChest tile\n" );
            loc->map->setTileAt(coords, loc->getReplacementTile(coords, tile));
        }

        // see if the chest is trapped and handle it
        getChestTrapHandler(player);

        screenMessage("The Chest Holds: %d Gold\n", c->party->getChest());

        screenPrompt();

        if (isCity(loc->map) && obj == NULL)
            c->party->adjustKarma(KA_STOLE_CHEST);
    }
    else
    {
        screenMessage("%cNot Here!%c\n", FG_GREY, FG_WHITE);
    }
}

/**
 * Called by getChest() to handle possible traps on chests
 **/
bool getChestTrapHandler(int player) {
    TileEffect trapType;
    int randNum = xu4_random(4);

    /* Do we use u4dos's way of trap-determination, or the original intended way? */
    int passTest = (xu4.settings->enhancements && xu4.settings->enhancementsOptions.c64chestTraps) ?
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
            screenMessage("%cAcid%c Trap!\n", FG_RED, FG_WHITE);
        else if (trapType == EFFECT_POISON)
            screenMessage("%cPoison%c Trap!\n", FG_GREEN, FG_WHITE);
        else if (trapType == EFFECT_SLEEP)
            screenMessage("%cSleep%c Trap!\n", FG_PURPLE, FG_WHITE);
        else if (trapType == EFFECT_LAVA)
            screenMessage("%cBomb%c Trap!\n", FG_RED, FG_WHITE);

        // player is < 0 during the 'O'pen spell (immune to traps)
        //
        // if the chest was opened by a PC, see if the trap was
        // evaded by testing the PC's dex
        //
        if ((player >= 0) &&
            (c->saveGame->players[player].dex + 25 < xu4_random(100)))
        {
            Map* map = c->location->map;

            // Play sound for acid & bomb since applyEffect does not.
            if (trapType == EFFECT_LAVA || trapType == EFFECT_FIRE)
                soundPlay(SOUND_POISON_EFFECT);

            if (trapType == EFFECT_LAVA) /* bomb trap */
                c->party->applyEffect(map, trapType);
            else
                c->party->member(player)->applyEffect(map, trapType);
        } else {
            soundPlay(SOUND_EVADE);
            screenMessage("Evaded!\n");
        }

        return true;
    }

    return false;
}

void holeUp() {
    screenMessage("Hole up & Camp!\n");

    if (!(c->location->context & (CTX_WORLDMAP | CTX_DUNGEON))) {
        screenMessage("%cNot here!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    if (c->transportContext != TRANSPORT_FOOT) {
        screenMessage("%cOnly on foot!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    CombatController *cc = new CampController();
    cc->beginCombat();
}

/**
 * Initializes the moon state according to the savegame file. This method of
 * initializing the moons (rather than just setting them directly) is necessary
 * to make sure trammel and felucca stay in sync
 */
void GameController::initMoons()
{
    SaveGame* saveGame = c->saveGame;
    int trammelphase = saveGame->trammelphase,
        feluccaphase = saveGame->feluccaphase;

    saveGame->trammelphase = saveGame->feluccaphase = 0;
    c->moonPhase = 0;

    while ((saveGame->trammelphase != trammelphase) ||
           (saveGame->feluccaphase != feluccaphase))
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
            AnnotationList* annot = &c->location->map->annotations;
            const UltimaSaveIds* usaveIds = xu4.config->usaveIds();

            /* update the moongates if trammel changed */
            if (trammelSubphase == 0) {
                gate = xu4.config->moongateCoords(oldTrammel);
                if (gate)
                    annot->remove(*gate, usaveIds->moduleId(0x40));
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate)
                    annot->add(*gate, usaveIds->moduleId(0x40));
            }
            else if (trammelSubphase == 1) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x40));
                    annot->add(*gate, usaveIds->moduleId(0x41));
                }
            }
            else if (trammelSubphase == 2) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x41));
                    annot->add(*gate, usaveIds->moduleId(0x42));
                }
            }
            else if (trammelSubphase == 3) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x42));
                    annot->add(*gate, usaveIds->moduleId(0x43));
                }
            }
            else if ((trammelSubphase > 3) && (trammelSubphase < (MOON_SECONDS_PER_PHASE * 4 * 3) - 3)) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x43));
                    annot->add(*gate, usaveIds->moduleId(0x43));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 3) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x43));
                    annot->add(*gate, usaveIds->moduleId(0x42));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 2) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x42));
                    annot->add(*gate, usaveIds->moduleId(0x41));
                }
            }
            else if (trammelSubphase == (MOON_SECONDS_PER_PHASE * 4 * 3) - 1) {
                gate = xu4.config->moongateCoords(c->saveGame->trammelphase);
                if (gate) {
                    annot->remove(*gate, usaveIds->moduleId(0x41));
                    annot->add(*gate, usaveIds->moduleId(0x40));
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
        if (!xu4.settings->filterMoveMessages) {
            switch (c->transportContext) {
                case TRANSPORT_FOOT:
                case TRANSPORT_HORSE:
                    screenMessage("%s\n", getDirectionName(event.dir));
                    break;
                case TRANSPORT_SHIP:
                    if (event.result & MOVE_TURNED)
                        screenMessage("Turn %s!\n", getDirectionName(event.dir));
                    else if (event.result & MOVE_SLOWED)
                        screenMessage("%cSlow progress!%c\n", FG_GREY, FG_WHITE);
                    else
                        screenMessage("Sail %s!\n", getDirectionName(event.dir));
                    break;
                case TRANSPORT_BALLOON:
                    screenMessage("%cDrift Only!%c\n", FG_GREY, FG_WHITE);
                    break;
                default:
                    ASSERT(0, "bad transportContext %d in avatarMoved()", c->transportContext);
            }
        }

horse_moved:
        /* movement was blocked */
        if (event.result & MOVE_BLOCKED) {

            /* if shortcuts are enabled, try them! */
            if (xu4.settings->shortcutCommands) {
                Coords new_coords = c->location->coords;
                const Tile* tile;

                map_move(new_coords, event.dir, c->location->map);
                tile = c->location->map->tileTypeAt(new_coords, WITH_OBJECTS);

                if (tile->isDoor()) {
                    openAt(new_coords);
                    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } else if (tile->isLockedDoor()) {
                    jimmyAt(new_coords);
                    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
                } /*else if (mapPersonAt(c->location->map, new_coords) != NULL) {
                    talkAtCoord(newx, newy, 1, NULL);
                    event.result = MOVE_SUCCEEDED | MOVE_END_TURN;
                    }*/
            }

            /* if we're still blocked */
            if ((event.result & MOVE_BLOCKED) && !xu4.settings->filterMoveMessages) {
                soundPlay(SOUND_BLOCKED, false);
                screenMessage("%cBlocked!%c\n", FG_GREY, FG_WHITE);
            }
        }
        else if (c->transportContext == TRANSPORT_FOOT ||
                 c->transportContext == TRANSPORT_HORSE) {
            /* movement was slowed */
            if (event.result & MOVE_SLOWED) {
                soundPlay(SOUND_WALK_SLOWED);
                screenMessage("%cSlow progress!%c\n", FG_GREY, FG_WHITE);
            }
            else {
                soundPlay(SOUND_WALK_NORMAL);
            }
        }
    } else {
        /* Emit the sound & result message for the horse second gallop move,
         * which is not a userEvent.  It probably should be to avoid this
         * extra check.  That would require another flag to suppress the move
         * message or the move message needs to be done prior to calling
         * avatarMoved.
         */
        if (c->transportContext == TRANSPORT_HORSE)
            goto horse_moved;
    }

    /* exited map */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("%cLeaving...%c\n", FG_GREY, FG_WHITE);
        exitToParentMap();
        musicPlayLocale();
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
    Direction orientation = (Direction) c->saveGame->orientation;
    Direction realDir = dirNormalize(orientation, event.dir);

    if (!xu4.settings->filterMoveMessages) {
        if (event.userEvent) {
            const char* msg;
            if (event.result & MOVE_TURNED) {
                msg = (dirRotateCCW(orientation) == realDir) ? "Turn Left\n"
                                                             : "Turn Right\n";
            } else {
                /* show 'Advance' or 'Retreat' in dungeons */
                msg = (realDir == orientation) ? "Advance\n" : "Retreat\n";
            }
            screenMessage(msg);
        }

        if (event.result & MOVE_BLOCKED)
            screenMessage("%cBlocked!%c\n", FG_GREY, FG_WHITE);
    }

    /* if we're exiting the map, do this */
    if (event.result & MOVE_EXIT_TO_PARENT) {
        screenMessage("%cLeaving...%c\n", FG_GREY, FG_WHITE);
        exitToParentMap();
        musicPlayLocale();
        return;
    }

    if (event.result & (MOVE_SUCCEEDED | MOVE_TURNED))
        screenDetectDungeonTraps();

    if (event.result & MOVE_SUCCEEDED) {
        /* check to see if we're entering a dungeon room */
        if (dungeon->currentToken() == DUNGEON_ROOM) {
            int room = (int)dungeon->currentSubToken(); /* get room number */

            /**
             * recalculate room for the abyss -- there are 16 rooms for every 2 levels,
             * each room marked with 0xD* where (* == room number 0-15).
             * for levels 1 and 2, there are 16 rooms, levels 3 and 4 there are 16 rooms, etc.
             */
            if (c->location->map->id == MAP_ABYSS)
                room = (0x10 * (c->location->coords.z/2)) + room;

            /* set the map room and start combat! */
            Dungeon *dng = dynamic_cast<Dungeon*>(c->location->map);
            CombatController::engageDungeon(dng, room, dirReverse(realDir));
        }
    }
}

void jimmy() {
    screenMessage("Jimmy: ");
    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords,
                                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (jimmyAt(*i))
            return;
    }

    screenMessage("%cJimmy what?%c\n", FG_GREY, FG_WHITE);
}

/**
 * Attempts to jimmy a locked door at map coordinates x,y.  The locked
 * door is replaced by a permanent annotation of an unlocked door
 * tile.
 */
bool jimmyAt(const Coords &coords) {
    Map* map = c->location->map;
    const Tile* tile = map->tileTypeAt(coords, WITH_OBJECTS);

    if (! tile->isLockedDoor())
        return false;

    if (c->saveGame->keys) {
        const Tile *door = map->tileset->getByName(Tile::sym.door);
        ASSERT(door, "no door tile found in tileset");
        c->saveGame->keys--;
        map->annotations.add(coords, door->getId());
        screenMessage("\nUnlocked!\n");
    } else
        screenMessage("%cNo keys left!%c\n", FG_GREY, FG_WHITE);

    return true;
}

void opendoor() {
    ///  XXX: Pressing "o" should close any open door.

    screenMessage("Open: ");

    if (c->party->isFlying()) {
        screenMessage("%cNot Here!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords,
                                                       1, 1, NULL, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (openAt(*i))
            return;
    }

    screenMessage("%cNot Here!%c\n", FG_GREY, FG_WHITE);
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
        screenMessage("%cCan't!%c\n", FG_GREY, FG_WHITE);
        return true;
    }

    const Tile *floor = c->location->map->tileset->getByName(Tile::sym.brickFloor);
    ASSERT(floor, "no floor tile found in tileset");
    Annotation* ann =
        c->location->map->annotations.add(coords, floor->getId(), false, true);
    ann->ttl = 4;

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
        screenMessage("Ready a weapon for: ");
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
    const Weapon *w = xu4.config->weapon(weapon);


    if (!w) {
        screenMessage("\n");
        return;
    }
    switch (p->setWeapon(w)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", w->getName());
        break;
    case EQUIP_NONE_LEFT:
        screenMessage("%cNone left!%c\n", FG_GREY, FG_WHITE);
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

        screenMessage("\n%cA %s may NOT use %s %s%c\n", FG_GREY, getClassName(p->getClass()),
                      indef_article.c_str(), w->getName(), FG_WHITE);
        break;
    }
    }
}

void talk() {
    screenMessage("Talk: ");

    if (c->party->isFlying()) {
        screenMessage("%cDrift only!%c\n", FG_GREY, FG_WHITE);
        return;
    }

    Direction dir = gameGetDirection();

    if (dir == DIR_NONE)
        return;

    vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, c->location->coords,
                                                                       1, 2, &Tile::canTalkOverTile, true);
    for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (talkAt(*i))
            return;
    }

    screenMessage("Funny, no response!\n");
}

/**
 * Mixes reagents.  Prompts for a spell, then which reagents to
 * include in the mix.
 */
void mixReagents() {

    /*  uncomment this line to activate new spell mixing code */
    //   return mixReagentsSuper();
    bool done = false;

    while (!done) {
        screenMessage("Mix reagents\n");
#ifdef IOS
        U4IOS::beginMixSpellController();
        return; // Just return, the dialog takes control from here.
#endif

        // Verify that there are reagents remaining in the inventory
        bool found = false;
        for (int i=0; i < 8; i++)
        {
            if (c->saveGame->reagents[i] > 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            screenMessage("%cNone Left!%c", FG_GREY, FG_WHITE);
            done = true;
        }
        else
        {
            screenMessage("For Spell: ");
            c->stats->setView(STATS_MIXTURES);

            int choice = ReadChoiceController::get("abcdefghijklmnopqrstuvwxyz \033\n\r");
            if (choice == ' ' || choice == '\033' || choice == '\n' || choice == '\r')
                break;

            int spell = choice - 'a';
            screenMessage("\b%s\n", spellGetName(spell));

            // ensure the mixtures for the spell isn't already maxed out
            if (c->saveGame->mixtures[spell] == 99) {
                screenMessage("\n%cYou cannot mix any more of that spell!%c\n", FG_GREY, FG_WHITE);
                break;
            }

            // Reset the reagent spell mix menu by removing
            // the menu highlight from the current item, and
            // hiding reagents that you don't have
            c->stats->resetReagentsMenu();

            c->stats->setView(MIX_REAGENTS);
            if (xu4.settings->enhancements && xu4.settings->enhancementsOptions.u5spellMixing)
                done = mixReagentsForSpellU5(spell);
            else
                done = mixReagentsForSpellU4(spell);
        }
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

    while (xu4.stage == StagePlay) {
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

        if (! ingredients.addReagent((Reagent)(choice - 'a')))
            screenMessage("\n%cNone Left!%c\n", FG_GREY, FG_WHITE);
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

    c->stats->getReagentsMenu()->reset(); // reset the menu, highlighting the first item
    ReagentsMenuController getReagentsController(c->stats->getReagentsMenu(), &ingredients, c->stats->getMainArea());
    xu4.eventHandler->pushController(&getReagentsController);
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
        screenMessage("%cWhat?%c\n", FG_GREY, FG_WHITE);
        return;
    }

    c->party->swapPlayers(player1, player2);
}

/**
 * Peers at a city from A-P (Lycaeum telescope) and functions like a gem
 */
bool gamePeerCity(int city, void *data) {
    Map *peerMap;

    peerMap = xu4.config->map((MapId)(city+1));

    if (peerMap != NULL) {
        xu4.game->setMap(peerMap, 1, NULL);
        gameSetViewMode(VIEW_GEM);
        screenDisableCursor();

#ifdef IOS
        U4IOS::IOSConversationChoiceHelper continueHelper;
        continueHelper.updateChoices(" ");
        continueHelper.fullSizeChoicePanel();
#endif
        ReadChoiceController::get("\015 \033");

        xu4.game->exitToParentMap();
        screenEnableCursor();
        gameSetViewMode(VIEW_NORMAL);
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
            screenMessage("%cPeer at What?%c\n", FG_GREY, FG_WHITE);
            return;
        }

        c->saveGame->gems--;
        screenMessage("Peer at a Gem!\n");
    }

    screenDisableCursor();
    gameSetViewMode(VIEW_GEM);

#ifdef IOS
    U4IOS::IOSConversationChoiceHelper continueHelper;
    continueHelper.updateChoices(" ");
    continueHelper.fullSizeChoicePanel();
#endif
    ReadChoiceController::get("\015 \033");

    screenEnableCursor();
    gameSetViewMode(VIEW_NORMAL);
}

/**
 * Begins a conversation with the NPC at map coordinates x,y.  If no
 * NPC is present at that point, zero is returned.
 */
bool talkAt(const Coords &coords) {
    /* can't have any conversations outside of town */
    City* city = static_cast<City*>(c->location->map);
    if (! isCity(city)) {
        screenMessage("Funny, no response!\n");
        return true;
    }

    /* make sure we have someone we can talk with */
    Person* speaker = city->personAt(coords);
    if (! speaker)
        return false;

    PersonNpcType npcType = speaker->getNpcType();
    if (speaker->isVendor()) {
        return discourse_run(&xu4.game->vendorDisc,
                             npcType - NPC_VENDOR_WEAPONS, speaker);
    }

    if (npcType >= NPC_LORD_BRITISH) {
        if (npcType == NPC_LORD_BRITISH) {
            /* If the avatar is dead Lord British resurrects them! */
            PartyMember* p0 = c->party->member(0);
            if (p0->getStatus() == STAT_DEAD) {
                screenMessage("%s, Thou shalt live again!\n",
                              p0->getName().c_str());
                p0->setStatus(STAT_GOOD);
                p0->heal(HT_FULLHEAL);
                gameSpellEffect('r', -1, SOUND_LBHEAL);
            }
        }

        Discourse* dis = &xu4.game->castleDisc;
        if (! dis->convCount)
            discourse_load(dis, "castle");
        return discourse_run(dis, npcType - NPC_LORD_BRITISH, speaker);
    }

    /* No response from alerted guards... does any monster both
       attack and talk besides Nate the Snake? */
    if (speaker->movement == MOVEMENT_ATTACK_AVATAR &&
        speaker->getId() != PYTHON_ID)
        return false;

    return discourse_run(&city->disc, speaker->discourseId(), speaker);
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

    const Armor *a = xu4.config->armor(armor);
    PartyMember *p = c->party->member(player);

    if (!a) {
        screenMessage("\n");
        return;
    }
    switch (p->setArmor(a)) {
    case EQUIP_SUCCEEDED:
        screenMessage("%s\n", a->getName());
        break;
    case EQUIP_NONE_LEFT:
        screenMessage("%cNone left!%c\n", FG_GREY, FG_WHITE);
        break;
    case EQUIP_CLASS_RESTRICTED:
        screenMessage("\n%cA %s may NOT use %s%c\n", FG_GREY, getClassName(p->getClass()), a->getName(), FG_WHITE);
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

    // Reset the reagent spell mix menu by removing
    // the menu highlight from the current item, and
    // hiding reagents that you don't have
    c->stats->resetReagentsMenu();

    c->stats->setView(StatsView(STATS_CHAR1 + player));
#ifdef IOS
    U4IOS::IOSHideActionKeysHelper hideExtraControls;
#endif
    ZtatsController ctrl;
    xu4.eventHandler->pushController(&ctrl);
    ctrl.waitFor();
}

/**
 * This function is called every quarter second.
 */
void GameController::timerFired() {
    if (cutScene) {
        screenCycle();
        screenUpdateCursor();
        screenUploadToGPU();
    } else {
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
        gameUpdateScreen();

        /*
         * force pass if no commands within last 20 seconds
         */
        Controller *controller = xu4.eventHandler->getController();
        if (dynamic_cast<TurnController *>(controller)) {
            c->commandTimer += 1000 / xu4.settings->gameCyclesPerSecond;
            if (gameTimeSinceLastCommand() > 20) {
                /* pass the turn, and redraw the text area prompt */
                controller->keyPressed(U4_SPACE);
            }
        }
    }
}

/**
 * Checks the hull integrity of the ship and handles
 * the ship sinking, if necessary
 */
void gameCheckHullIntegrity() {
    bool killAll = false;

    /* see if the ship has sunk */
    if ((c->transportContext == TRANSPORT_SHIP) && c->saveGame->shiphull <= 0)
    {
        screenMessage("\nThy ship sinks!\n\n");
        killAll = true;
    }

    Location* loc = c->location;
    if (! collisionOverride && c->transportContext == TRANSPORT_FOOT &&
        loc->map->tileTypeAt(loc->coords, WITHOUT_OBJECTS)->isSailable() &&
        ! loc->map->tileTypeAt(loc->coords, WITH_GROUND_OBJECTS)->isShip() &&
        ! loc->map->getValidMoves(loc->coords, c->party->getTransport()))
    {
        screenMessage("\nTrapped at sea without thy ship, thou dost drown!\n\n");
        killAll = true;
    }

    if (killAll)
    {
        for (int i = 0; i < c->party->size(); i++)
        {
            c->party->member(i)->setHp(0);
            c->party->member(i)->setStatus(STAT_DEAD);
        }

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
    const Coords& coords = c->location->coords;

    /*
     * if heading east into pirates cove (O'A" N'N"), generate pirate
     * ships
     */
    if (dir == DIR_EAST &&
        coords.x == 0xdd &&
        coords.y == 0xe0) {
        Object *obj;
        for (i = 0; i < 8; i++) {
            obj = c->location->map->addCreature(xu4.config->creature(PIRATE_ID), Coords(pirateInfo[i].x, pirateInfo[i].y));
            obj->setDirection(pirateInfo[i].dir);
        }
    }

    /*
     * if heading south towards the shrine of humility, generate
     * daemons unless horn has been blown
     */
    if (dir == DIR_SOUTH &&
        coords.x >= 229 && coords.x < 234 &&
        coords.y >= 212 && coords.y < 217 &&
        c->aura.getType() != Aura::HORN) {
        for (i = 0; i < 8; i++)
            c->location->map->addCreature(xu4.config->creature(DAEMON_ID),
                                          Coords(231, coords.y + 1, coords.z));
    }
}

static bool activeMoongateAt(int trammel, int felucca, const Coords& src,
                             Coords& dest) {
    const Coords* mc = xu4.config->moongateCoords(trammel);
    if (mc && (src == *mc)) {
        mc = xu4.config->moongateCoords(felucca);
        if (mc) {
            dest = *mc;
            return true;
        }
    }
    return false;
}

/**
 * Checks for and handles when the avatar steps on a moongate
 */
bool GameController::checkMoongates() {
    Coords dest;

    if (! activeMoongateAt(c->saveGame->trammelphase,
                           c->saveGame->feluccaphase,
                           c->location->coords, dest))
        return false;

    // Default spell effect (screen inversion without 'spell' sound effects)
    gameSpellEffect(-1, -1, SOUND_MOONGATE);

    if (c->location->coords != dest) {
        c->location->coords = dest;
        gameSpellEffect(-1, -1, SOUND_MOONGATE); // Again, after arriving
    }

    // Entry to shrine of Spirituality
    if (c->saveGame->trammelphase == 4 &&
        c->saveGame->feluccaphase == 4 &&
        c->party->canEnterShrine(VIRT_SPIRITUALITY)) {
        Shrine* shrine = static_cast<Shrine*>(xu4.config->map(MAP_SHRINE_SPIRITUALITY));
        setMap(shrine, 1, NULL);
        musicPlayLocale();
        shrine->enter();
    }

    return true;
}

/**
 * Fixes objects initially loaded by saveGameMonstersRead,
 * and alters movement behavior accordingly to match the creature
 */
void gameFixupObjects(Map *map, const SaveGameMonsterRecord* table) {
    int i;
    const SaveGameMonsterRecord *it;
    Object* obj;
    MapTile tile, oldTile;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    int creatureLimit = (map->type == Map::DUNGEON) ? MONSTERTABLE_SIZE
                                          : MONSTERTABLE_CREATURES_SIZE;

    // NOTE: In dungeons it->tile is zero for unused entries and there can
    // be more than MONSTERTABLE_CREATURES_SIZE monsters.

    /* add stuff from the monster table to the map */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        it = table + i;
        if (it->prevTile != 0) {
            Coords coords(it->x, it->y);

            // tile values stored in monsters.sav hardcoded to index into base tilemap
            oldTile = usaveIds->moduleId(it->prevTile);
            if (map->type == Map::DUNGEON) {
                coords.z = it->level;
                tile = oldTile;
            } else
                tile = usaveIds->moduleId(it->tile);

            if (i < creatureLimit) {
                const Creature *creature = Creature::getByTile(tile);
                /* make sure we really have a creature */
                if (creature) {
                    obj = map->addCreature(creature, coords);

                    // Preserve animation & previous state to keep round-trip
                    // load > save > load identical.

                    obj->tile = tile;
                    obj->prevTile = oldTile;
                    obj->prevCoords.x = it->prevx;
                    obj->prevCoords.y = it->prevy;
                } else {
                    fprintf(stderr, "Error: A non-creature object was found in the creature section of the monster table. (Tile: %s)\n", tile.getTileType()->nameStr());
                    obj = map->addObject(tile, oldTile, coords);
                }
            } else {
                obj = map->addObject(tile, oldTile, coords);
            }

            obj->prevCoords.x = it->prevx;
            obj->prevCoords.y = it->prevy;
        }
    }
}

/**
 * Handles what happens when a creature attacks you
 */
void gameCreatureAttack(Creature *m) {
    const Tile *ground;

    screenMessage("\nAttacked by %s\n", m->getName().c_str());

    ground = battleGround(c->location->map, c->location->coords);

    CombatController::engage(CombatMap::mapForTile(ground,
                        c->party->getTransport().getTileType(), m), m);
}

/**
 * Performs a ranged attack for the creature at x,y on the world map
 */
bool creatureRangeAttack(const Coords &coords, Creature *m) {
//    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;

    // Figure out what the ranged attack should look like
    Symbol tname = Tile::sym.hitFlash;
    if (m && m->getWorldrangedtile())
        tname = m->getWorldrangedtile();
    MapTile tile(c->location->map->tileset->getByName(tname)->getId());

    GameController::flashTile(coords, tile, 1);

    // See if the attack hits the avatar
    Object *obj = c->location->map->objectAt(coords);
    m = dynamic_cast<Creature*>(obj);

    // Does the attack hit the avatar?
    if (coords == c->location->coords) {
        /* always displays as a 'hit' */
        GameController::flashTile(coords, tile, 3);

        /* FIXME: check actual damage from u4dos -- values here are guessed */
        if (c->transportContext == TRANSPORT_SHIP)
            gameDamageShip(-1, 10);
        else gameDamageParty(10, 25);

        return true;
    }
    // Destroy objects that were hit
    else if (obj) {
        if ((obj->objType == Object::CREATURE && m->isAttackable()) ||
            obj->objType == Object::UNKNOWN) {

            GameController::flashTile(coords, tile, 3);
            c->location->map->removeObject(obj);

            return true;
        }
    }
    return false;
}

/**
 * Gets the path of coordinates for an action.  Each tile in the
 * direction specified by dirmask, between the minimum and maximum
 * distances given, is included in the path, until blockedPredicate
 * fails.  If a tile is blocked, that tile is included in the path
 * only if includeBlocked is true.
 */
vector<Coords> gameGetDirectionalActionPath(int dirmask, int validDirections,
        const Coords &origin, int minDistance, int maxDistance,
        bool (*blockedPredicate)(const Tile *tile), bool includeBlocked) {
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

    Map* map = c->location->map;
    Coords t_c(origin);
    if ((dirx <= 0 || DIR_IN_MASK(dirx, validDirections)) &&
        (diry <= 0 || DIR_IN_MASK(diry, validDirections))) {
        for (int distance = 0; distance <= maxDistance;
             distance++, map_move(t_c, dirx, map),
                 map_move(t_c, diry, map)) {

            if (distance >= minDistance) {
                /* make sure our action isn't taking us off the map */
                if (MAP_IS_OOB(map, t_c))
                    break;

                const Tile *tile = map->tileTypeAt(t_c, WITH_GROUND_OBJECTS);

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
            c->party->member(i)->applyDamage(c->location->map, damage);
            c->stats->highlightPlayer(i);
            lastdmged = i;
            EventHandler::wait_msecs(50);
        }
    }

    screenShake(1);

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
    Party* party = c->party;

    if (player == -1) {
        party->setActivePlayer(-1);
        screenMessage("Set Active Player: None!\n");
    }
    else if (player < party->size()) {
        screenMessage("Set Active Player: %s!\n",
                      party->member(player)->getName().c_str());
        if (party->member(player)->isDisabled())
            screenMessage("Disabled!\n");
        else
            party->setActivePlayer(player);
    }
}

/**
 * Removes creatures from the current map if they are too far away from the avatar
 */
void GameController::creatureCleanup() {
    ObjectDeque::iterator i;
    Map *map = c->location->map;

    for (i = map->objects.begin(); i != map->objects.end();) {
        const Object *obj = *i;
        const Coords& o_coords = obj->coords;

        if ((obj->objType == Object::CREATURE) &&
            (o_coords.z == c->location->coords.z) &&
            map_distance(o_coords, c->location->coords, c->location->map) > MAX_CREATURE_DISTANCE) {

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
    const Location* loc = c->location;
    int canSpawnHere = loc->map->isWorldMap() || loc->context & CTX_DUNGEON;
#ifdef IOS
    int spawnDivisor = loc->context & CTX_DUNGEON ? (53 - (loc->coords.z << 2)) : 53;
#else
    int spawnDivisor = loc->context & CTX_DUNGEON ? (32 - (loc->coords.z << 2)) : 32;
#endif

    /* If there are too many creatures already,
       or we're not on the world map, don't worry about it! */
    if (!canSpawnHere ||
        loc->map->getNumberOfCreatures() >= MAX_CREATURES_ON_MAP ||
        xu4_random(spawnDivisor) != 0)
        return;

    gameSpawnCreature(NULL);
}

/**
 * Handles trolls under bridges
 */
void GameController::checkBridgeTrolls() {
    Map* map = c->location->map;

    // TODO: CHEST: Make a user option to not make chests block bridge trolls
    if (! map->isWorldMap() ||
        map->tileTypeAt(c->location->coords, WITH_OBJECTS)->name != Tile::sym.bridge ||
        xu4_random(8) != 0)
        return;

    screenMessage("\nBridge Trolls!\n");

    Creature *m = map->addCreature(xu4.config->creature(TROLL_ID),
                                   c->location->coords);
    CombatController::engage(MapId(MAP_BRIDGE_CON), m);
}

/**
 * Spawns a creature (m) just offscreen of the avatar.
 * If (m==NULL) then it finds its own creature to spawn and spawns it.
 */
bool gameSpawnCreature(const Creature *m) {
    int t, i;
    const Creature *creature;
    Coords coords = c->location->coords;

    if (c->location->context & CTX_DUNGEON) {
        /* FIXME: for some reason dungeon monsters aren't spawning correctly */

        bool found = false;
        Coords new_coords;

        for (i = 0; i < 0x20; i++) {
            new_coords = Coords(xu4_random(c->location->map->width), xu4_random(c->location->map->height), coords.z);
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
                Coords new_coords = coords;
                map_move(new_coords, dx, dy, c->location->map);

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
            map_move(coords, dx, dy, c->location->map);
    }

    /* can't spawn creatures on top of the player */
    if (coords == c->location->coords)
        return false;

    /* figure out what creature to spawn */
    if (m)
        creature = m;
    else if (c->location->context & CTX_DUNGEON)
        creature = Creature::randomForDungeon(c->location->coords.z);
    else
        creature = Creature::randomForTile(c->location->map->tileTypeAt(coords, WITHOUT_OBJECTS));

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
        const Object *obj = *i;
        if (obj->tile.getTileType()->isBalloon())
            return false;
    }

    const Tile *balloon = map->tileset->getByName(Tile::sym.balloon);
    ASSERT(balloon, "no balloon tile found in tileset");
    const Coords* coord = map->getLabel(Tile::sym.balloon);
    if (coord) {
        map->addObject(balloon->getId(), balloon->getId(), *coord);
        return true;
    }
    return false;
}

// Colors assigned to reagents based on my best reading of them
// from the book of wisdom.  Maybe we could use BOLD to distinguish
// the two grey and the two red reagents.
const int colors[] = {
  FG_YELLOW, FG_GREY, FG_BLUE, FG_WHITE, FG_RED, FG_GREY, FG_GREEN, FG_RED
};

void
showMixturesSuper(int page = 0) {
  screenTextColor(FG_WHITE);
  for (int i = 0; i < 13; i++) {
    char buf[4];

    const Spell *s = getSpell(i + 13 * page);
    int line = i + 8;
    screenTextAt(2, line, "%s", s->name);

    snprintf(buf, 4, "%3u", c->saveGame->mixtures[i + 13 * page]);
    screenTextAt(6, line, "%s", buf);

    screenShowChar(32, 9, line);
    int comp = s->components;
    for (int j = 0; j < 8; j++) {
      screenTextColor(colors[j]);
      screenShowChar(comp & (1 << j) ? CHARSET_BULLET : ' ', 10 + j, line);
    }
    screenTextColor(FG_WHITE);

    snprintf(buf, 3, "%2d", s->mp);
    screenTextAt(19, line, "%s", buf);
  }
}

void
mixReagentsSuper() {

  screenMessage("Mix reagents\n");

  static int page = 0;

  struct ReagentShop {
    const char *name;
    int price[6];
  };
  ReagentShop shops[] = {
    { "BuccDen", {6, 7, 9, 9, 9, 1} },
    { "Moonglo", {2, 5, 6, 3, 6, 9} },
    { "Paws",    {3, 4, 2, 8, 6, 7} },
    { "SkaraBr", {2, 4, 9, 6, 4, 8} },
  };
  const int shopcount = sizeof (shops) / sizeof (shops[0]);

  int oldlocation = c->location->viewMode;
  c->location->viewMode = VIEW_MIXTURES;
  screenUpdate(&xu4.game->mapArea, true, true);

  screenTextAt(16, 2, "%s", "<-Shops");

  c->stats->setView(StatsView(STATS_REAGENTS));
  screenTextColor(FG_PURPLE);
  screenTextAt(2, 7, "%s", "SPELL # Reagents MP");

  for (int i = 0; i < shopcount; i++) {
    int line = i + 1;
    ReagentShop *s = &shops[i];
    screenTextColor(FG_WHITE);
    screenTextAt(2, line, "%s", s->name);
    for (int j = 0; j < 6; j++) {
      screenTextColor(colors[j]);
      screenShowChar('0' + s->price[j], 10 + j, line);
    }
  }

  for (int i = 0; i < 8; i++) {
    screenTextColor(colors[i]);
    screenShowChar('A' + i, 10 + i, 6);
  }

  bool done = false;
  while (!done) {
    showMixturesSuper(page);
    screenMessage("For Spell: ");

    int spell = ReadChoiceController::get("abcdefghijklmnopqrstuvwxyz \033\n\r");
    if (spell < 'a' || spell > 'z' ) {
      screenMessage("\nDone.\n");
      done = true;
    } else {
      spell -= 'a';
      const Spell *s = getSpell(spell);
      screenMessage("%s\n", s->name);
      page = (spell >= 13);
      showMixturesSuper(page);

      // how many can we mix?
      int mixQty = 99 - c->saveGame->mixtures[spell];
      int ingQty = 99;
      int comp = s->components;
      for (int i = 0; i < 8; i++) {
        if (comp & 1 << i) {
          int reagentQty = c->saveGame->reagents[i];
          if (reagentQty < ingQty)
            ingQty = reagentQty;
        }
      }
      screenMessage("You can make %d.\n", (mixQty > ingQty) ? ingQty : mixQty);
      screenMessage("How many? ");

      int howmany = ReadIntController::get(2, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);

      if (howmany == 0) {
        screenMessage("\nNone mixed!\n");
      } else if (howmany > mixQty) {
        screenMessage("\n%cYou cannot mix that much more of that spell!%c\n", FG_GREY, FG_WHITE);
      } else if (howmany > ingQty) {
        screenMessage("\n%cYou don't have enough reagents to mix %d spells!%c\n", FG_GREY, howmany, FG_WHITE);
      } else {
        c->saveGame->mixtures[spell] += howmany;
        for (int i = 0; i < 8; i++) {
          if (comp & 1 << i) {
            c->saveGame->reagents[i] -= howmany;
          }
        }
        screenMessage("\nSuccess!\n\n");
      }
    }
    c->stats->setView(StatsView(STATS_REAGENTS));
  }

  c->location->viewMode = oldlocation;
  return;
}
