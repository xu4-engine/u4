/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <algorithm>
#include "u4.h"

#include "intro.h"

#include "debug.h"
#include "error.h"
#include "event.h"
#include "imagemgr.h"
#include "menu.h"
#include "music.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"

using namespace std;

extern bool quit;

IntroController *intro = NULL;

#define INTRO_MAP_HEIGHT 5
#define INTRO_MAP_WIDTH 19
#define INTRO_TEXT_X 0
#define INTRO_TEXT_Y 19
#define INTRO_TEXT_WIDTH 40
#define INTRO_TEXT_HEIGHT 6

#define GYP_PLACES_FIRST 0
#define GYP_PLACES_TWOMORE 1
#define GYP_PLACES_LAST 2
#define GYP_UPON_TABLE 3
#define GYP_SEGUE1 13
#define GYP_SEGUE2 14

class IntroObjectState {
public:
    IntroObjectState() : x(0), y(0) {}
    int x, y;
    MapTile tile; /* base tile + tile frame */    
};

/* menus */
static Menu mainMenu;
static Menu videoMenu;
static Menu soundMenu;
static Menu gameplayMenu;
static Menu advancedMenu;
static Menu keyboardMenu;
static Menu speedMenu;
static Menu enhancementMenu;
bool menusLoaded = false;

/* temporary place-holder for settings changes */
SettingsData settingsChanged;

const int IntroBinData::INTRO_TEXT_OFFSET = 17445;
const int IntroBinData::INTRO_MAP_OFFSET = 30339;
const int IntroBinData::INTRO_FIXUPDATA_OFFSET = 29806;
const int IntroBinData::INTRO_SCRIPT_TABLE_SIZE = 548;
const int IntroBinData::INTRO_SCRIPT_TABLE_OFFSET = 30434;
const int IntroBinData::INTRO_BASETILE_TABLE_SIZE = 15;
const int IntroBinData::INTRO_BASETILE_TABLE_OFFSET = 16584;
const int IntroBinData::BEASTIE1_FRAMES = 0x80;
const int IntroBinData::BEASTIE2_FRAMES = 0x40;
const int IntroBinData::BEASTIE_FRAME_TABLE_OFFSET = 0x7380;
const int IntroBinData::BEASTIE1_FRAMES_OFFSET = 0;
const int IntroBinData::BEASTIE2_FRAMES_OFFSET = 0x78;

IntroBinData::IntroBinData() {
    introMap = NULL;
    sigData = NULL;
    scriptTable = NULL;
    baseTileTable = NULL;
    beastie1FrameTable = NULL;
    beastie2FrameTable = NULL;
}

IntroBinData::~IntroBinData() {
    if (introMap)
        delete [] introMap;
    if (sigData)
        delete [] sigData;
    if (scriptTable)
        delete [] scriptTable;
    if (baseTileTable)
        delete [] baseTileTable;
    if (beastie1FrameTable)
        delete [] beastie1FrameTable;
    if (beastie2FrameTable)
        delete [] beastie2FrameTable;

    introQuestions.clear();
    introText.clear();
    introGypsy.clear();
}

bool IntroBinData::load() {
    int i;

    U4FILE *title = u4fopen("title.exe");
    if (!title)
        return false;

    introQuestions = u4read_stringtable(title, INTRO_TEXT_OFFSET, 28);
    introText = u4read_stringtable(title, -1, 24);
    introGypsy = u4read_stringtable(title, -1, 15);

    /* clean up stray newlines at end of strings */
    for (i = 0; i < 15; i++)
        trim(introGypsy[i]);

    if (sigData)
        delete sigData;
    sigData = new unsigned char[533];
    u4fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    u4fread(sigData, 1, 533, title);

    u4fseek(title, INTRO_MAP_OFFSET, SEEK_SET);
    introMap = new MapTile[(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT) + 1];    
    for (i = 0; i < INTRO_MAP_HEIGHT * INTRO_MAP_WIDTH; i++)        
        introMap[i] = Tile::translate(u4fgetc(title));
        
    u4fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = new unsigned char[INTRO_SCRIPT_TABLE_SIZE];
    for (i = 0; i < INTRO_SCRIPT_TABLE_SIZE; i++)
        scriptTable[i] = u4fgetc(title);

    u4fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = new Tile*[INTRO_BASETILE_TABLE_SIZE];
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        MapTile tile = Tile::translate(u4fgetc(title));
        baseTileTable[i] = Tileset::get()->get(tile.id);
    }

    /* --------------------------
       load beastie frame table 1
       -------------------------- */
    beastie1FrameTable = new unsigned char[BEASTIE1_FRAMES];
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE1_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE1_FRAMES; i++) {
        beastie1FrameTable[i] = u4fgetc(title);
    }

    /* --------------------------
       load beastie frame table 2
       -------------------------- */
    beastie2FrameTable = new unsigned char[BEASTIE2_FRAMES];
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE2_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE2_FRAMES; i++) {
        beastie2FrameTable[i] = u4fgetc(title);
    }

    u4fclose(title);

    return true;
}

IntroController::IntroController() : 
    Controller(1), 
    backgroundArea(),
    menuArea(1 * CHAR_WIDTH, 13 * CHAR_HEIGHT, 38, 10),
    extendedMenuArea(1 * CHAR_WIDTH, 3 * CHAR_HEIGHT, 38, 22),
    questionArea(INTRO_TEXT_X * CHAR_WIDTH, INTRO_TEXT_Y * CHAR_HEIGHT, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT),
    mapArea(BORDER_WIDTH, (TILE_HEIGHT * 6) + BORDER_HEIGHT, INTRO_MAP_WIDTH, INTRO_MAP_HEIGHT, "base")
{
    binData = NULL;
    beastiesVisible = false;
}

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
bool IntroController::init() {
    mode = INTRO_MAP;
    beastie1Cycle = 0;
    beastie2Cycle = 0;
    beastieOffset = -32;    
    beastiesVisible = true;

    sleepCycles = 0;
    scrPos = 0;
    objectStateTable = new IntroObjectState[IntroBinData::INTRO_BASETILE_TABLE_SIZE];

    binData = new IntroBinData();
    binData->load();

    /* load our menus, checking to see if they're already loaded first */
    if (!menusLoaded) {
        mainMenu.setTitle("-- xu4 Configuration --", 8, 1);
        mainMenu.add(VIDEO_MENU, "Video Options", 12, 3, 'v');
        mainMenu.add(SOUND_MENU, "Sound Options", 12, 4, 's');
        mainMenu.add(GAMEPLAY_MENU, "Gameplay Options", 12, 5, 'g');
        mainMenu.add(CANCEL, "Main Menu", 12, 8, 'm');
        mainMenu.addShortcutKey(CANCEL, ' ');
        mainMenu.setClosesMenu(CANCEL);

        videoMenu.setTitle("Video Options:", 1, 0);
        videoMenu.add(new StringMenuItem("Graphics          %s", 5, 2, 'g', &settingsChanged.videoType, imageMgr->getSetNames()));
        videoMenu.add(new StringMenuItem("Gem Layout        %s", 5, 3, 0, &settingsChanged.gemLayout, screenGetGemLayoutNames()));
        videoMenu.add(new IntMenuItem   ("Scale             x%d", 5, 4, 's', (int *) &settingsChanged.scale, 1, 5, 1));
        videoMenu.add((new BoolMenuItem ("Mode              %s", 5, 5, 'm', &settingsChanged.fullscreen))->
                      setValueStrings("Fullscreen", "Window"));
        videoMenu.add(new StringMenuItem("Filter            %s", 5, 6, 'f', &settingsChanged.filter, screenGetFilterNames()));
        videoMenu.add(new BoolMenuItem  ("Screen Shaking    %s", 5, 7, 'k', &settingsChanged.screenShakes));
        videoMenu.add(USE_SETTINGS, "Use These Settings", 5, 17, 'u');
        videoMenu.add(CANCEL, "Cancel", 5, 18, 'c');
        videoMenu.addShortcutKey(CANCEL, ' ');
        videoMenu.setClosesMenu(USE_SETTINGS);
        videoMenu.setClosesMenu(CANCEL);
    
        soundMenu.setTitle("Sound Options:", 2, 1);
        soundMenu.add(new BoolMenuItem("Volume         %s", 10, 3, 'v', &settingsChanged.musicVol));
        soundMenu.add(new BoolMenuItem("Sound Effects  %s", 10, 4, 's', &settingsChanged.soundVol));
        soundMenu.add(new BoolMenuItem("Fading         %s", 10, 5, 'f', &settingsChanged.volumeFades));
        soundMenu.add(USE_SETTINGS, "Use These Settings", 10, 7, 'u');
        soundMenu.add(CANCEL, "Cancel", 10, 8, 'c');
        soundMenu.addShortcutKey(CANCEL, ' ');
        soundMenu.setClosesMenu(USE_SETTINGS);
        soundMenu.setClosesMenu(CANCEL);

        gameplayMenu.setTitle("Gameplay Options:", 1, 0);
        gameplayMenu.add(new BoolMenuItem  ("Game Enhancements         %s", 5, 2, 'g', &settingsChanged.enhancements));
        gameplayMenu.add(new BoolMenuItem  ("Automatic Actions         %s", 5, 4, 'a', &settingsChanged.shortcutCommands));
        gameplayMenu.add(new StringMenuItem("Battle Difficulty         %s", 5, 7, 'b', &settingsChanged.battleDiff, settings.getBattleDiffs()));
        gameplayMenu.add(new BoolMenuItem("Mouse                     %s", 5, 9, 0, &settingsChanged.mouseOptions.enabled));
        gameplayMenu.add(ADVANCED_MENU, "\010 Advanced Options", 5, 15, 'o');
        gameplayMenu.add(USE_SETTINGS, "Use These Settings", 5, 17, 'u');
        gameplayMenu.add(CANCEL, "Cancel", 5, 18, 'c');
        gameplayMenu.addShortcutKey(CANCEL, ' ');
        gameplayMenu.setClosesMenu(USE_SETTINGS);
        gameplayMenu.setClosesMenu(CANCEL);
    
        advancedMenu.setTitle("Advanced Options:", 1, 0);
        advancedMenu.add(SPEED_MENU, "\010 Speed Settings", 3, 2, 's');
        advancedMenu.add(KEYBOARD_MENU, "\010 Keyboard Settings", 3, 3, 'k');
        advancedMenu.add(new BoolMenuItem("Debug Mode (Cheats)           %s", 3, 5, 'd', &settingsChanged.debug)); 
        advancedMenu.add(ENHANCEMENT_MENU, "\010 Game Enhancement Options", 3, 15, 'g');
        advancedMenu.add(USE_SETTINGS, "Use These Settings", 3, 17, 'u');
        advancedMenu.add(CANCEL, "Cancel", 3, 18, 'c');
        advancedMenu.addShortcutKey(CANCEL, ' ');
        advancedMenu.setClosesMenu(USE_SETTINGS);
        advancedMenu.setClosesMenu(CANCEL);

        keyboardMenu.setTitle("Keyboard Settings:", 1, 0);
        keyboardMenu.add(new IntMenuItem("Repeat Delay (in msecs)      %d", 4, 2, 0, &settingsChanged.keydelay, 100, MAX_KEY_DELAY, 100));
        keyboardMenu.add(new IntMenuItem("Repeat Interval (in msecs)   %d", 4, 3, 0, &settingsChanged.keyinterval, 10, MAX_KEY_INTERVAL, 10));
        keyboardMenu.add(USE_SETTINGS, "Use These Settings", 4, 17, 'u');
        keyboardMenu.add(CANCEL, "Cancel", 4, 18, 'c');
        keyboardMenu.addShortcutKey(CANCEL, ' ');
        keyboardMenu.setClosesMenu(USE_SETTINGS);
        keyboardMenu.setClosesMenu(CANCEL);
    
        speedMenu.setTitle("Speed Settings:", 1, 0);
        speedMenu.add(new IntMenuItem("Game Cycles Per Second    %3d", 3, 2, 0, &settingsChanged.gameCyclesPerSecond, 1, MAX_CYCLES_PER_SECOND, 1));
        speedMenu.add(new IntMenuItem("Battle Speed              %3d", 3, 3, 0, &settingsChanged.battleSpeed, 1, MAX_BATTLE_SPEED, 1));
        speedMenu.add(new IntMenuItem("Spell Effect Length",           3, 4, 0, &settingsChanged.spellEffectSpeed, 1, MAX_SPELL_EFFECT_SPEED, 1));
        speedMenu.add(new IntMenuItem("Camping length            %3d sec", 3, 5, 0, &settingsChanged.campTime, 1, MAX_CAMP_TIME, 1));
        speedMenu.add(new IntMenuItem("Inn rest length           %3d sec", 3, 6, 0, &settingsChanged.innTime, 1, MAX_INN_TIME, 1));
        speedMenu.add(5, "Shrine Meditation length", 3, 7);
        speedMenu.add(new IntMenuItem("Screen Shake Interval     %3d msec", 3, 8, 0, &settingsChanged.shakeInterval, MIN_SHAKE_INTERVAL, MAX_SHAKE_INTERVAL, 10));
        speedMenu.add(USE_SETTINGS, "Use These Settings", 3, 17);
        speedMenu.add(CANCEL, "Cancel", 3, 18, 'c');
        speedMenu.addShortcutKey(CANCEL, ' ');
        speedMenu.setClosesMenu(USE_SETTINGS);
        speedMenu.setClosesMenu(CANCEL);
    
        enhancementMenu.setTitle("Game Enhancement Options:", 1, 0);
        enhancementMenu.add(new BoolMenuItem("Set Active Player       %s", 6, 2, 0, &settingsChanged.enhancementsOptions.activePlayer));
        enhancementMenu.add(new BoolMenuItem("Ultima V Spell Mixing   %s", 6, 3, 0, &settingsChanged.enhancementsOptions.u5spellMixing));
        enhancementMenu.add(new BoolMenuItem("Ultima V Shrines        %s", 6, 4, 0, &settingsChanged.enhancementsOptions.u5shrines));
        enhancementMenu.add(new BoolMenuItem("Slime Divides           %s", 6, 5, 0, &settingsChanged.enhancementsOptions.slimeDivides));
        enhancementMenu.add(new BoolMenuItem("Fixed Chest Traps       %s", 6, 6, 0, &settingsChanged.enhancementsOptions.c64chestTraps));
        enhancementMenu.add(new BoolMenuItem("Smart 'Enter' Key       %s", 6, 7, 0, &settingsChanged.enhancementsOptions.smartEnterKey));
        enhancementMenu.add(new BoolMenuItem("Gem View Shows Objects  %s", 6, 8, 0, &settingsChanged.enhancementsOptions.peerShowsObjects));
        enhancementMenu.add(USE_SETTINGS, "Use These Settings", 6, 17, 'u');
        enhancementMenu.add(CANCEL, "Cancel", 6, 18, 'c');
        enhancementMenu.addShortcutKey(CANCEL, ' ');
        enhancementMenu.setClosesMenu(USE_SETTINGS);
        enhancementMenu.setClosesMenu(CANCEL);
        menusLoaded = true;
    }

    backgroundArea.reinit();
    menuArea.reinit();
    extendedMenuArea.reinit();
    questionArea.reinit();
    mapArea.reinit();

    /* Make a copy of our settings so we can change them */
    settingsChanged = settings;

    updateScreen();

    musicMgr->intro();

    return true;
}

/**
 * Frees up data not needed after introduction.
 */
void IntroController::deleteIntro() {
    delete binData;
    binData = NULL;

    delete [] objectStateTable;
    objectStateTable = NULL;

    imageMgr->freeIntroBackgrounds();
}

unsigned char *IntroController::getSigData() {
    ASSERT(binData->sigData != NULL, "intro sig data not loaded");
    return binData->sigData;
}

/**
 * Handles keystrokes during the introduction.
 */
bool IntroController::keyPressed(int key) {
    bool valid = true;

    switch (mode) {

    case INTRO_MAP:
        mode = INTRO_MENU;
        updateScreen();
        return true;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            errorMessage.erase();
            initiateNewGame();
            break;
        case 'j':
            journeyOnward();
            break;
        case 'r':
            errorMessage.erase();
            mode = INTRO_MAP;
            updateScreen();
            break;
        case 'c': {
            errorMessage.erase();
            settingsChanged = settings;
            screenDisableCursor();
            mode = INTRO_CONFIG;
            runMenu(&mainMenu, &menuArea, true);
            mode = INTRO_MENU;
            screenEnableCursor();
            updateScreen();
            break;
        }
        case 'a':
            errorMessage.erase();
            about();
            break;
        case 'q':
            EventHandler::end();
            quit = true;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            musicMgr->introSwitch(key - '0');
            break;
        default:
            valid = false;
            break;
        }
        break;

    case INTRO_ABOUT:
    case INTRO_INIT:
    case INTRO_CONFIG:
        ASSERT(0, "key handler called in wrong mode");
        return true;
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

/**
 * Draws the small map on the intro screen.
 */
void IntroController::drawMap() {
    if (sleepCycles > 0) {
        drawMapAnimated();
        sleepCycles--;
    }
    else {
        unsigned char commandNibble;
        unsigned char dataNibble;

        do {
            commandNibble = binData->scriptTable[scrPos] >> 4;

            switch(commandNibble) {
                /* 0-4 = set object position and tile frame */
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                /* ----------------------------------------------------------
                   Set object position and tile frame
                   Format: yi [t(3); x(5)]
                   i = table index
                   x = x coordinate (5 least significant bits of second byte)
                   y = y coordinate
                   t = tile frame (3 most significant bits of second byte)
                   ---------------------------------------------------------- */
                dataNibble = binData->scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].x = binData->scriptTable[scrPos+1] & 0x1f;
                objectStateTable[dataNibble].y = commandNibble;
                
                // See if the tile id needs to be recalculated 
                if ((binData->scriptTable[scrPos+1] >> 5) >= binData->baseTileTable[dataNibble]->frames) {
                    int frame = (binData->scriptTable[scrPos+1] >> 5) - binData->baseTileTable[dataNibble]->frames;
                    objectStateTable[dataNibble].tile = binData->baseTileTable[dataNibble]->id + 1;
                    objectStateTable[dataNibble].tile.frame = frame;
                }
                else {
                    objectStateTable[dataNibble].tile = binData->baseTileTable[dataNibble]->id;
                    objectStateTable[dataNibble].tile.frame = (binData->scriptTable[scrPos+1] >> 5);
                }
                
                scrPos += 2;
                break;
            case 7:
                /* ---------------
                   Delete object
                   Format: 7i
                   i = table index
                   --------------- */
                dataNibble = binData->scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].tile = 0;
                scrPos++;
                break;
            case 8:
                /* ----------------------------------------------
                   Redraw intro map and objects, then go to sleep
                   Format: 8c
                   c = cycles to sleep
                   ---------------------------------------------- */
                drawMapAnimated();

                /* set sleep cycles */
                sleepCycles = binData->scriptTable[scrPos] & 0xf;
                scrPos++;
                break;
            case 0xf:
                /* -------------------------------------
                   Jump to the start of the script table
                   Format: f?
                   ? = doesn't matter
                   ------------------------------------- */
                scrPos = 0;
                break;
            default:
                /* invalid command */
                scrPos++;
                break;
            }

        } while (commandNibble != 8);
    }
}

void IntroController::drawMapAnimated() {
    int x, y, i;    

    /* draw unmodified map */
    for (y = 0; y < INTRO_MAP_HEIGHT; y++)
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            mapArea.drawTile(&binData->introMap[x + (y * INTRO_MAP_WIDTH)], false, x, y);

    /* draw animated objects */
    for (i = 0; i < IntroBinData::INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0)
            mapArea.drawTile(&objectStateTable[i].tile, false, objectStateTable[i].x, objectStateTable[i].y);
    }
}

/**
 * Draws the animated beasts in the upper corners of the screen.
 */
void IntroController::drawBeasties() {
    drawBeastie(0, beastieOffset, binData->beastie1FrameTable[beastie1Cycle]);
    drawBeastie(1, beastieOffset, binData->beastie2FrameTable[beastie2Cycle]);
    if (beastieOffset < 0)
        beastieOffset++;
}

/**
 * Animates the "beasties".  The animate intro image is made up frames
 * for the two creatures in the top left and top right corners of the
 * screen.  This function draws the frame for the given beastie on the
 * screen.  vertoffset is used lower the creatures down from the top
 * of the screen.
 */
void IntroController::drawBeastie(int beast, int vertoffset, int frame) {
    char buffer[128];
    int destx;

    ASSERT(beast == 0 || beast == 1, "invalid beast: %d", beast);

    sprintf(buffer, "beast%dframe%02d", beast, frame);

    destx = beast ? (320 - 48) : 0;
    backgroundArea.draw(buffer, destx, vertoffset);
}

/**
 * Animates the moongate in the tree intro image.  There are two
 * overlays in the part of the image normally covered by the text.  If
 * the frame parameter is "moongate", the moongate overlay is painted
 * over the image.  If frame is "items", the second overlay is
 * painted: the circle without the moongate, but with a small white
 * dot representing the anhk and history book.
 */
void IntroController::animateTree(const string &frame) {
    backgroundArea.draw(frame, 72, 68);
}

/**
 * Draws the cards in the character creation sequence with the gypsy.
 */
void IntroController::drawCard(int pos, int card) {
    static const char *cardNames[] = { 
        "honestycard", "compassioncard", "valorcard", "justicecard",
        "sacrificecard", "honorcard", "spiritualitycard", "humilitycard" 
    };

    ASSERT(pos == 0 || pos == 1, "invalid pos: %d", pos);
    ASSERT(card < 8, "invalid card: %d", card);

    backgroundArea.draw(cardNames[card], pos ? 218 : 12, 12);
}

/**
 * Draws the beads in the abacus during the character creation sequence
 */
void IntroController::drawAbacusBeads(int row, int selectedVirtue, int rejectedVirtue) {
    ASSERT(row >= 0 && row < 7, "invalid row: %d", row);
    ASSERT(selectedVirtue < 8 && selectedVirtue >= 0, "invalid virtue: %d", selectedVirtue);
    ASSERT(rejectedVirtue < 8 && rejectedVirtue >= 0, "invalid virtue: %d", rejectedVirtue);
    
    backgroundArea.draw("whitebead", 128 + (selectedVirtue * 9), 24 + (row * 15));
    backgroundArea.draw("blackbead", 128 + (rejectedVirtue * 9), 24 + (row * 15));
}

/**
 * Paints the screen.
 */
void IntroController::updateScreen() {
    screenHideCursor();

    switch (mode) {
    case INTRO_MAP:
        backgroundArea.draw(BKGD_INTRO);
        drawMap();
        drawBeasties();
        break;

    case INTRO_MENU:
        screenSetCursorPos(24, 16);
        screenShowCursor();

        backgroundArea.draw(BKGD_INTRO);
        menuArea.textAt(1, 1, "In another world, in a time to come.");
        menuArea.textAt(14, 3, "Options:");
        menuArea.textAt(10, 4, "Return to the view");
        menuArea.textAt(10, 5, "Journey Onward");
        menuArea.textAt(10, 6, "Initiate New Game");
        menuArea.textAt(10, 7, "Configure");
        menuArea.textAt(10, 8, "About");
        if (!errorMessage.empty())
            menuArea.textAt(10, 9, errorMessage.c_str());
        drawBeasties();
        break;

    case INTRO_ABOUT:
    case INTRO_INIT:
    case INTRO_CONFIG:
    default:
        ASSERT(0, "bad mode in updateScreen");
    }

    screenUpdateCursor();
    screenRedrawScreen();
}

/**
 * Initiate a new savegame by reading the name, sex, then presenting a
 * series of questions to determine the class of the new character.
 */
void IntroController::initiateNewGame() {
    mode = INTRO_INIT;

    screenDisableCursor();
    menuArea.enableCursor();
    menuArea.setCursorFollowsText(true);

    // display name prompt and read name from keyboard
    backgroundArea.draw(BKGD_INTRO);
    menuArea.textAt(3, 3, "By what name shalt thou be known");
    menuArea.textAt(3, 4, "in this world and time?");
    menuArea.setCursorPos(11, 7, true);

    drawBeasties();

    screenRedrawScreen();

    nameBuffer = ReadStringController::get(12, &menuArea);
    if (nameBuffer[0] == '\0') {
        menuArea.disableCursor();
        mode = INTRO_MENU;
        updateScreen();
        return;
    }

    // display sex prompt and read sex from keyboard
    backgroundArea.draw(BKGD_INTRO);
    menuArea.textAt(3, 3, "Art thou Male or Female?");
    menuArea.setCursorPos(28, 3, true);
    drawBeasties();

    int sexChoice = ReadChoiceController::get("mf");
    if (sexChoice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    menuArea.disableCursor();

    // show the lead up story
    showStory();

    // ask questions that determine character class
    startQuestions();

    // write out save game an segue into game
    SaveGame saveGame;
    SaveGamePlayerRecord avatar;

    FILE *saveGameFile = fopen((settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "wb");
    if (!saveGameFile) {
        questionArea.disableCursor();
        mode = INTRO_MENU;
        errorMessage = "Unable to create save game!";
        updateScreen();
        return;
    }

    avatar.init();
    saveGame.init(&avatar);
    screenHideCursor();
    initPlayers(&saveGame);
    saveGame.food = 30000;
    saveGame.gold = 200;
    saveGame.reagents[REAG_GINSENG] = 3;
    saveGame.reagents[REAG_GARLIC] = 4;
    saveGame.torches = 2;
    saveGame.write(saveGameFile);
    fclose(saveGameFile);

    saveGameFile = fopen((settings.getUserPath() + MONSTERS_SAV_BASE_FILENAME).c_str(), "wb");
    if (saveGameFile) {
        saveGameMonstersWrite(NULL, saveGameFile);
        fclose(saveGameFile);
    }

    // show the text thats segues into the main game
    showText(binData->introGypsy[GYP_SEGUE1]);

    ReadChoiceController pauseController("");
    eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    showText(binData->introGypsy[GYP_SEGUE2]);

    eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    // done: exit intro and let game begin
    EventHandler::setControllerDone();

    return;
}

void IntroController::showStory() {
    ReadChoiceController pauseController("");

    beastiesVisible = false;

    questionArea.enableCursor();
    questionArea.setCursorFollowsText(true);

    for (int storyInd = 0; storyInd < 24; storyInd++) {
        if (storyInd == 0)
            backgroundArea.draw(BKGD_TREE);
        else if (storyInd == 3)
            animateTree("moongate");
        else if (storyInd == 5)
            animateTree("items");
        else if (storyInd == 6)
            backgroundArea.draw(BKGD_PORTAL);
        else if (storyInd == 11)
            backgroundArea.draw(BKGD_TREE);
        else if (storyInd == 15)
            backgroundArea.draw(BKGD_OUTSIDE);
        else if (storyInd == 17)
            backgroundArea.draw(BKGD_INSIDE);
        else if (storyInd == 20)
            backgroundArea.draw(BKGD_WAGON);
        else if (storyInd == 21)
            backgroundArea.draw(BKGD_GYPSY);
        else if (storyInd == 23)
            backgroundArea.draw(BKGD_ABACUS);
        showText(binData->introText[storyInd]);
    
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();
    }
}

/**
 * Starts the gypsys questioning that eventually determines the new
 * characters class.
 */
void IntroController::startQuestions() {
    ReadChoiceController pauseController("");
    ReadChoiceController questionController("ab");

    questionRound = 0;
    initQuestionTree();

    while (1) {
        // draw the abacus background, if necessary
        if (questionRound == 0)
            backgroundArea.draw(BKGD_ABACUS);

        // draw the cards and show the lead up text
        drawCard(0, questionTree[questionRound * 2]);
        drawCard(1, questionTree[questionRound * 2 + 1]);

        questionArea.clear();
        questionArea.textAt(0, 0, "%s", binData->introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)].c_str());
        questionArea.textAt(0, 1, "%s", binData->introGypsy[GYP_UPON_TABLE].c_str());
        questionArea.textAt(0, 2, "%s and %s.  She says", 
                            binData->introGypsy[questionTree[questionRound * 2] + 4].c_str(), 
                            binData->introGypsy[questionTree[questionRound * 2 + 1] + 4].c_str());
        questionArea.textAt(0, 3, "\"Consider this:\"");

        // wait for a key
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenEnableCursor();
        // show the question to choose between virtues
        showText(getQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));

        // wait for an answer
        eventHandler->pushController(&questionController);
        int choice = questionController.waitFor();

        // update the question tree
        if (doQuestion(choice == 'a' ? 0 : 1)) {
            return;
        }
    }
}

/**
 * Get the text for the question giving a choice between virtue v1 and
 * virtue v2 (zero based virtue index, starting at honesty).
 */
string IntroController::getQuestion(int v1, int v2) {
    int i = 0;
    int d = 7;

    ASSERT(v1 < v2, "first virtue must be smaller (v1 = %d, v2 = %d)", v1, v2);

    while (v1 > 0) {
        i += d;
        d--;
        v1--;
        v2--;
    }

    ASSERT((i + v2 - 1) < 28, "calculation failed");

    return binData->introQuestions[i + v2 - 1];
}

/**
 * Starts the game.
 */
void IntroController::journeyOnward() {
    FILE *saveGameFile;    
    bool validSave = false;

    /*
     * ensure a party.sav file exists, otherwise require user to
     * initiate game
     */
    saveGameFile = fopen((settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str(), "rb");
    if (saveGameFile) {
        SaveGame *saveGame = new SaveGame;

        // Make sure there are players in party.sav --
        // In the Ultima Collection CD, party.sav exists, but does
        // not contain valid info to journey onward        
        saveGame->read(saveGameFile);        
        if (saveGame->members > 0)
            validSave = true;
        delete saveGame;
    }
    
    if (!validSave) {
        errorMessage = "Initiate game first!";
        updateScreen();
        screenRedrawScreen();
        return;
    }

    fclose(saveGameFile);
    EventHandler::setControllerDone();
}

/**
 * Shows an about box.
 */
void IntroController::about() {
    mode = INTRO_ABOUT;

    backgroundArea.draw(BKGD_INTRO);
    screenHideCursor();
    menuArea.textAt(14, 1, "XU4 %s", VERSION);
    menuArea.textAt(1, 3, "xu4 is free software; you can redist-");
    menuArea.textAt(1, 4, "ribute it and/or modify it under the");
    menuArea.textAt(1, 5, "terms of the GNU GPL as published by");
    menuArea.textAt(1, 6, "the FSF.  See COPYING.");
    menuArea.textAt(1, 8, "\011 Copyright 2002-2003 xu4 team");
    menuArea.textAt(1, 9, "\011 Copyright 1987 Lord British");
    drawBeasties();

    ReadChoiceController::get("");

    mode = INTRO_MENU;
    screenShowCursor();
    updateScreen();
}

/**
 * Shows text in the question area.
 */
void IntroController::showText(const string &text) {
    string current = text;
    int lineNo = 0;

    questionArea.clear();
    
    unsigned long pos = current.find("\n");
    while (pos < current.length()) {
        questionArea.textAt(0, lineNo++, "%s", current.substr(0, pos).c_str());
        current = current.substr(pos+1);
        pos = current.find("\n");
    }
    
    /* write the last line (possibly only line) */
    questionArea.textAt(0, lineNo++, "%s", current.substr(0, pos).c_str());
}

/**
 * Run a menu and return when the menu has been closed.  Screen
 * updates are handled by observing the menu.
 */
void IntroController::runMenu(Menu *menu, TextView *view, bool withBeasties) {
    view->enableCursor();
    menu->addObserver(this);
    menu->reset();
    menu->show(view);
    if (withBeasties)
        drawBeasties();

    MenuController menuController(menu, view);
    eventHandler->pushController(&menuController);
    menuController.waitFor();

    menu->deleteObserver(this);
    view->disableCursor();
}

/**
 * Timer callback for the intro sequence.  Handles animating the intro
 * map, the beasties, etc..
 */
void IntroController::timerFired() {
    screenCycle();
    screenUpdateCursor();
    if (mode == INTRO_MAP)
        drawMap();
    if (beastiesVisible)
        drawBeasties();

    /* 
     * refresh the screen only if the timer queue is empty --
     * i.e. drop a frame if another timer event is about to be fired
     */
    if (EventHandler::timerQueueEmpty())
        screenRedrawScreen();

    if (xu4_random(2) && ++beastie1Cycle >= IntroBinData::BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if (xu4_random(2) && ++beastie2Cycle >= IntroBinData::BEASTIE2_FRAMES)
        beastie2Cycle = 0;
}

/**
 * Update the screen when an observed menu is reset or has an item
 * activated.
 */
void IntroController::update(Menu *menu, MenuEvent &event) {
    if (menu == &mainMenu)
        updateMainMenu(event);
    else if (menu == &videoMenu)
        updateVideoMenu(event);
    else if (menu == &soundMenu)
        updateSoundMenu(event);
    else if (menu == &gameplayMenu)
        updateGameplayMenu(event);
    else if (menu == &advancedMenu)
        updateAdvancedMenu(event);
    else if (menu == &enhancementMenu)
        updateEnhancementMenu(event);
    else if (menu == &keyboardMenu)
        updateKeyboardMenu(event);
    else if (menu == &speedMenu)
        updateSpeedMenu(event);

    if (beastiesVisible)
        drawBeasties();
}

void IntroController::updateMainMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0:
            beastiesVisible = false;
            runMenu(&videoMenu, &extendedMenuArea, false);
            beastiesVisible = true;
            break;
        case 1:
            runMenu(&soundMenu, &menuArea, true);
            break;
        case 2:
            beastiesVisible = false;
            runMenu(&gameplayMenu, &extendedMenuArea, false);
            beastiesVisible = true;
            break;
        case CANCEL:
        default:
            break;
        }
    }

    backgroundArea.draw(BKGD_INTRO);
}

void IntroController::updateVideoMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            /* save settings (if necessary) */
            if (settings != settingsChanged) {
                settings.setData(settingsChanged);
                settings.write();

                /* FIXME: resize images, etc. */
                screenReInit();

                // Fix the menu since it was obliterated
                mode = INTRO_MENU; 
                runMenu(&mainMenu, &menuArea, true);
            }        
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;

        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
}

void IntroController::updateSoundMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();
        
            musicMgr->intro();

            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
    
        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO);
}

void IntroController::updateGameplayMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case ADVANCED_MENU:
            // show or hide game enhancement options if enhancements are enabled/disabled
            advancedMenu.getItemById(ENHANCEMENT_MENU)->setVisible(settingsChanged.enhancements);

            runMenu(&advancedMenu, &extendedMenuArea, false);
            break;
        case USE_SETTINGS:
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(5, 5, "  (Open, Jimmy, etc.)");
}

void IntroController::updateAdvancedMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case ENHANCEMENT_MENU:
            runMenu(&enhancementMenu, &extendedMenuArea, false);
            break;
        case KEYBOARD_MENU:
            runMenu(&keyboardMenu, &extendedMenuArea, false);
            break;
        case SPEED_MENU:
            runMenu(&speedMenu, &extendedMenuArea, false);
            break;
        case USE_SETTINGS:
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
}

void IntroController::updateEnhancementMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:        
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();        
            break;
        case CANCEL:        
            /* discard settings */
            settingsChanged = settings;
            break;
    
        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
}

void IntroController::updateKeyboardMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();

            /* re-initialize keyboard */
            KeyHandler::setKeyRepeat(settingsChanged.keydelay, settingsChanged.keyinterval);
    
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
        default: break;
        }    
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
}

void IntroController::updateSpeedMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 5:
            /* make sure that the setting we're trying for is even possible */
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.shrineTime++;
                if (settingsChanged.shrineTime > MAX_SHRINE_TIME)
                    settingsChanged.shrineTime = MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.shrineTime--;
                if (settingsChanged.shrineTime < (MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond))
                    settingsChanged.shrineTime = MAX_SHRINE_TIME;
            }
            break;
        case USE_SETTINGS:
            /* save settings */
            settings.setData(settingsChanged);
            settings.write();
    
            /* re-initialize events */
            eventTimerGranularity = (1000 / settings.gameCyclesPerSecond);
            eventHandler->getTimer()->reset(eventTimerGranularity);            
        
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    backgroundArea.draw(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(29, 4, "%3g sec", static_cast<double>(settingsChanged.spellEffectSpeed) / 5);
    extendedMenuArea.textAt(29, 7, "%3d sec", settingsChanged.shrineTime);
}


/**
 * Initializes the question tree.  The tree starts off with the first
 * eight entries set to the numbers 0-7 in a random order.
 */
void IntroController::initQuestionTree() {
    int i, tmp, r;

    for (i = 0; i < 8; i++)
        questionTree[i] = i;

    for (i = 0; i < 8; i++) {
        r = xu4_random(8);
        tmp = questionTree[r];
        questionTree[r] = questionTree[i];
        questionTree[i] = tmp;
    }
    answerInd = 8;

    if (questionTree[0] > questionTree[1]) {
        tmp = questionTree[0];
        questionTree[0] = questionTree[1];
        questionTree[1] = tmp;
    }
        
}

/**
 * Updates the question tree with the given answer, and advances to
 * the next round.
 * @return true if all questions have been answered, false otherwise
 */
bool IntroController::doQuestion(int answer) {
    if (!answer)
        questionTree[answerInd] = questionTree[questionRound * 2];
    else
        questionTree[answerInd] = questionTree[questionRound * 2 + 1];
    
    drawAbacusBeads(questionRound, questionTree[answerInd],
        questionTree[questionRound * 2 + ((answer) ? 0 : 1)]);

    answerInd++;
    questionRound++;

    if (questionRound > 6)
        return true;

    if (questionTree[questionRound * 2] > questionTree[questionRound * 2 + 1]) {
        int tmp = questionTree[questionRound * 2];
        questionTree[questionRound * 2] = questionTree[questionRound * 2 + 1];
        questionTree[questionRound * 2 + 1] = tmp;
    }

    return false;
}

/**
 * Build the initial avatar player record from the answers to the
 * gypsy's questions.
 */
void IntroController::initPlayers(SaveGame *saveGame) {
    int i, p;
    static const struct {
        WeaponType weapon;
        ArmorType armor;
        int level, xp, x, y;
        int virtues[8];
    } initValuesForClass[] = {
        { WEAP_STAFF,  ARMR_CLOTH,   2, 125, 231, 136, { 65, 50, 60, 50, 50, 55, 55, 50 } }, /* CLASS_MAGE */
        { WEAP_SLING,  ARMR_CLOTH,   3, 240,  83, 105, { 50, 65, 55, 55, 50, 60, 50, 50 } }, /* CLASS_BARD */
        { WEAP_AXE,    ARMR_LEATHER, 3, 205,  35, 221, { 50, 55, 65, 50, 55, 50, 60, 50 } }, /* CLASS_FIGHTER */
        { WEAP_DAGGER, ARMR_CLOTH,   2, 175,  59,  44, { 55, 50, 50, 65, 50, 60, 55, 50 } }, /* CLASS_DRUID */
        { WEAP_MACE,   ARMR_LEATHER, 2, 110, 158,  21, { 50, 50, 60, 50, 65, 55, 55, 50 } }, /* CLASS_TINKER */
        { WEAP_SWORD,  ARMR_CHAIN,   3, 325, 105, 183, { 50, 50, 50, 55, 60, 65, 50, 55 } }, /* CLASS_PALADIN */
        { WEAP_SWORD,  ARMR_LEATHER, 2, 150,  23, 129, { 50, 50, 50, 55, 55, 60, 65, 50 } }, /* CLASS_RANGER */
        { WEAP_STAFF,  ARMR_CLOTH,   1,   5, 186, 171, { 50, 50, 60, 50, 50, 55, 55, 65 } }  /* CLASS_SHEPHERD */
    };
    static const struct {
        const char *name;
        int str, dex, intel;
        SexType sex;
    } initValuesForNpcClass[] = {
        { "Mariah",    9, 12, 20, SEX_FEMALE }, /* CLASS_MAGE */
        { "Iolo",     16, 19, 13, SEX_MALE },   /* CLASS_BARD */
        { "Geoffrey", 20, 15, 11, SEX_MALE },   /* CLASS_FIGHTER */
        { "Jaana",    17, 16, 13, SEX_FEMALE }, /* CLASS_DRUID */
        { "Julia",    15, 16, 12, SEX_FEMALE }, /* CLASS_TINKER */
        { "Dupre",    17, 14, 17, SEX_MALE },   /* CLASS_PALADIN */
        { "Shamino",  16, 15, 15, SEX_MALE },   /* CLASS_RANGER */
        { "Katrina",  11, 12, 10, SEX_FEMALE }  /* CLASS_SHEPHERD */
    };

    strcpy(saveGame->players[0].name, nameBuffer.c_str());
    saveGame->players[0].sex = sex;
    saveGame->players[0].klass = static_cast<ClassType>(questionTree[14]);

    ASSERT(saveGame->players[0].klass < 8, "bad class: %d", saveGame->players[0].klass);

    saveGame->players[0].weapon = initValuesForClass[saveGame->players[0].klass].weapon;
    saveGame->players[0].armor = initValuesForClass[saveGame->players[0].klass].armor;
    saveGame->players[0].xp = initValuesForClass[saveGame->players[0].klass].xp;
    saveGame->x = initValuesForClass[saveGame->players[0].klass].x;
    saveGame->y = initValuesForClass[saveGame->players[0].klass].y;

    saveGame->players[0].str = 15;
    saveGame->players[0].dex = 15;
    saveGame->players[0].intel = 15;

    for (i = 8; i < 15; i++) {
        switch (questionTree[i]) {
        case VIRT_HONESTY:
            saveGame->players[0].intel += 3;
            break;
        case VIRT_COMPASSION:
            saveGame->players[0].dex += 3;
            break;
        case VIRT_VALOR:
            saveGame->players[0].str += 3;
            break;
        case VIRT_JUSTICE:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            break;
        case VIRT_SACRIFICE:
            saveGame->players[0].intel++;
            saveGame->players[0].str++;
            break;
        case VIRT_HONOR:
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_SPIRITUALITY:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_HUMILITY:
            /* no stats for you! */
            break;
        }
    }

    PartyMember player(NULL, &saveGame->players[0]);
    saveGame->players[0].hp = saveGame->players[0].hpMax = player.getMaxLevel() * 100;
    saveGame->players[0].mp = player.getMaxMp();

    p = 1;
    for (i = 0; i < VIRT_MAX; i++) {
        /* Setup the initial virtue levels according to the avatar's class */
        saveGame->karma[i] = initValuesForClass[saveGame->players[0].klass].virtues[i];
        player = PartyMember(NULL, &saveGame->players[i]);

        /* Initial setup for party members that aren't in your group yet... */
        if (i != saveGame->players[0].klass) {
            saveGame->players[p].klass = static_cast<ClassType>(i);
            saveGame->players[p].xp = initValuesForClass[i].xp;
            saveGame->players[p].str = initValuesForNpcClass[i].str;
            saveGame->players[p].dex = initValuesForNpcClass[i].dex;
            saveGame->players[p].intel = initValuesForNpcClass[i].intel;
            saveGame->players[p].weapon = initValuesForClass[i].weapon;
            saveGame->players[p].armor = initValuesForClass[i].armor;
            strcpy(saveGame->players[p].name, initValuesForNpcClass[i].name);
            saveGame->players[p].sex = initValuesForNpcClass[i].sex;
            saveGame->players[p].hp = saveGame->players[p].hpMax = initValuesForClass[i].level * 100;
            saveGame->players[p].mp = player.getMaxMp();
            p++;
        }
    }
}
