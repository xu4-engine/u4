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

#define INTRO_TEXT_OFFSET 17445
#define INTRO_MAP_OFFSET 30339
#define INTRO_MAP_HEIGHT 5
#define INTRO_MAP_WIDTH 19
#define INTRO_FIXUPDATA_OFFSET 29806
#define INTRO_SCRIPT_TABLE_SIZE 548
#define INTRO_SCRIPT_TABLE_OFFSET 30434
#define INTRO_BASETILE_TABLE_SIZE 15
#define INTRO_BASETILE_TABLE_OFFSET 16584
#define BEASTIE1_FRAMES 0x80
#define BEASTIE2_FRAMES 0x40
#define BEASTIE_FRAME_TABLE_OFFSET 0x7380
#define BEASTIE1_FRAMES_OFFSET 0
#define BEASTIE2_FRAMES_OFFSET 0x78

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

struct IntroObjectState {
    int x, y;
    MapTile tile; /* base tile + tile frame */    
};

/* menus */
static Menu mainOptions;
static Menu videoOptions;
static Menu soundOptions;
static Menu gameplayOptions;
static Menu advancedOptions;
static Menu keyboardOptions;
static Menu speedOptions;
static Menu enhancementOptions;
bool menusLoaded = false;

/* temporary place-holder for settings changes */
SettingsData settingsChanged;

IntroController::IntroController() : 
    Controller(1), 
    menuArea(1 * CHAR_WIDTH, 13 * CHAR_HEIGHT, 38, 10),
    extendedMenuArea(1 * CHAR_WIDTH, 3 * CHAR_HEIGHT, 38, 22),
    questionArea(INTRO_TEXT_X * CHAR_WIDTH, INTRO_TEXT_Y * CHAR_HEIGHT, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT),
    mapArea(BORDER_WIDTH, (TILE_HEIGHT * 6) + BORDER_HEIGHT, INTRO_MAP_WIDTH, INTRO_MAP_HEIGHT, "base")
{

    beastiesVisible = false;
    introMap = NULL;
    sigData = NULL;
    scriptTable = NULL;
    baseTileTable = NULL;
    beastie1FrameTable = NULL;
    beastie2FrameTable = NULL;
}

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
bool IntroController::init() {
    U4FILE *title;
    int i;

    mode = INTRO_MAP;
    beastie1Cycle = 0;
    beastie2Cycle = 0;
    beastieOffset = -32;    
    beastiesVisible = true;

    title = u4fopen("title.exe");
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
        
    sleepCycles = 0;
    scrPos = 0;

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

    objectStateTable = new IntroObjectState[INTRO_BASETILE_TABLE_SIZE];
    memset(objectStateTable, 0, sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);

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

    /* load our menus, checking to see if they're already loaded first */
    if (!menusLoaded) {
        mainOptions.add(0, "Video Options", 12, 3, 'v');
        mainOptions.add(1, "Sound Options", 12, 4, 's');
        mainOptions.add(2, "Gameplay Options", 12, 5, 'g');
        mainOptions.add(CANCEL, "Main Menu", 12, 8, 'm');
        mainOptions.addShortcutKey(CANCEL, ' ');
        mainOptions.setClosesMenu(CANCEL);
    
        videoOptions.add(4, "Graphics", 5, 2, 'g');
        videoOptions.add(5, "Gem Layout", 5, 3);
        videoOptions.add(0, "Scale", 5, 4, 's');
        videoOptions.add(1, "Mode", 5, 5, 'm');
        videoOptions.add(2, "Filter", 5, 6, 'f');
        videoOptions.add(3, "Screen Shaking", 5, 7, 'k');
        videoOptions.add(USE_SETTINGS, "Use These Settings", 5, 17, 'u');
        videoOptions.add(CANCEL, "Cancel", 5, 18, 'c');
        videoOptions.addShortcutKey(CANCEL, ' ');
        videoOptions.setClosesMenu(USE_SETTINGS);
        videoOptions.setClosesMenu(CANCEL);
    
        soundOptions.add(0, "Volume", 10, 3, 'v');
        soundOptions.add(1, "Sound Effects", 10, 4, 's');
        soundOptions.add(2, "Fading", 10, 5, 'f');
        soundOptions.add(USE_SETTINGS, "Use These Settings", 10, 7, 'u');
        soundOptions.add(CANCEL, "Cancel", 10, 8, 'c');
        soundOptions.addShortcutKey(CANCEL, ' ');
        soundOptions.setClosesMenu(USE_SETTINGS);
        soundOptions.setClosesMenu(CANCEL);
    
        gameplayOptions.add(0, "Game Enhancements", 5, 2, 'g');
        gameplayOptions.add(1, "Automatic Actions", 5, 4, 'a');
        gameplayOptions.add(3, "Battle Difficulty", 5, 7, 'b');
        gameplayOptions.add(4, "Mouse", 5, 9);
        gameplayOptions.add(2, "\010 Advanced Options", 5, 15, 'o');
        gameplayOptions.add(USE_SETTINGS, "Use These Settings", 5, 17, 'u');
        gameplayOptions.add(CANCEL, "Cancel", 5, 18, 'c');
        gameplayOptions.addShortcutKey(CANCEL, ' ');
        gameplayOptions.setClosesMenu(USE_SETTINGS);
        gameplayOptions.setClosesMenu(CANCEL);
    
        advancedOptions.add(3, "\010 Speed Settings", 3, 2, 's');
        advancedOptions.add(2, "\010 Keyboard Settings", 3, 3, 'k');
        advancedOptions.add(1, "Debug Mode (Cheats)", 3, 5, 'd');        
        advancedOptions.add(0, "\010 Game Enhancement Options", 3, 15, 'g');    
        advancedOptions.add(USE_SETTINGS, "Use These Settings", 3, 17, 'u');
        advancedOptions.add(CANCEL, "Cancel", 3, 18, 'c');
        advancedOptions.addShortcutKey(CANCEL, ' ');
        advancedOptions.setClosesMenu(USE_SETTINGS);
        advancedOptions.setClosesMenu(CANCEL);
    
        keyboardOptions.add(1, "Repeat Delay (in msecs)", 4, 2);
        keyboardOptions.add(2, "Repeat Interval (in msecs)", 4, 3);
        keyboardOptions.add(USE_SETTINGS, "Use These Settings", 4, 17, 'u');
        keyboardOptions.add(CANCEL, "Cancel", 4, 18, 'c');
        keyboardOptions.addShortcutKey(CANCEL, ' ');
        keyboardOptions.setClosesMenu(USE_SETTINGS);
        keyboardOptions.setClosesMenu(CANCEL);
    
        speedOptions.add(0, "Game Cycles Per Second", 3, 2);
        speedOptions.add(1, "Battle Speed", 3, 3);
        speedOptions.add(2, "Spell Effect Length", 3, 4);
        speedOptions.add(3, "Camping length", 3, 5);
        speedOptions.add(4, "Inn rest length", 3, 6);
        speedOptions.add(5, "Shrine Meditation length", 3, 7);
        speedOptions.add(6, "Screen Shake Interval", 3, 8);
        speedOptions.add(USE_SETTINGS, "Use These Settings", 3, 17);
        speedOptions.add(CANCEL, "Cancel", 3, 18, 'c');
        speedOptions.addShortcutKey(CANCEL, ' ');
        speedOptions.setClosesMenu(USE_SETTINGS);
        speedOptions.setClosesMenu(CANCEL);
    
        enhancementOptions.add(6, "Set Active Player", 6, 2);
        enhancementOptions.add(4, "Ultima V Spell Mixing", 6, 3);
        enhancementOptions.add(0, "Ultima V Shrines", 6, 4);
        enhancementOptions.add(1, "Slime Divides", 6, 5);
        enhancementOptions.add(2, "Fixed Chest Traps", 6, 6);
        enhancementOptions.add(5, "Smart 'Enter' Key", 6, 7);
        enhancementOptions.add(7, "Gem View Shows Objects", 6, 8);
        enhancementOptions.add(USE_SETTINGS, "Use These Settings", 6, 17, 'u');
        enhancementOptions.add(CANCEL, "Cancel", 6, 18, 'c');
        enhancementOptions.addShortcutKey(CANCEL, ' ');
        enhancementOptions.setClosesMenu(USE_SETTINGS);
        enhancementOptions.setClosesMenu(CANCEL);

        menusLoaded = true;
    }

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
    delete [] introMap;

    introQuestions.clear();
    introText.clear();
    introGypsy.clear();

    delete [] scriptTable;
    scriptTable = NULL;
    delete [] baseTileTable;
    baseTileTable = NULL;
    delete [] objectStateTable;
    objectStateTable = NULL;

    delete [] beastie1FrameTable;
    delete [] beastie2FrameTable;

    imageMgr->freeIntroBackgrounds();
}

unsigned char *IntroController::getSigData() {
    ASSERT(sigData != NULL, "intro sig data not loaded");
    return sigData;
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
            runMenu(&mainOptions, &menuArea, true);
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
            commandNibble = scriptTable[scrPos] >> 4;

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
                dataNibble = scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].x = scriptTable[scrPos+1] & 0x1f;
                objectStateTable[dataNibble].y = commandNibble;
                
                // See if the tile id needs to be recalculated 
                if ((scriptTable[scrPos+1] >> 5) >= baseTileTable[dataNibble]->frames) {
                    int frame = (scriptTable[scrPos+1] >> 5) - baseTileTable[dataNibble]->frames;
                    objectStateTable[dataNibble].tile = baseTileTable[dataNibble]->id + 1;
                    objectStateTable[dataNibble].tile.frame = frame;
                }
                else {
                    objectStateTable[dataNibble].tile = baseTileTable[dataNibble]->id;
                    objectStateTable[dataNibble].tile.frame = (scriptTable[scrPos+1] >> 5);
                }
                
                scrPos += 2;
                break;
            case 7:
                /* ---------------
                   Delete object
                   Format: 7i
                   i = table index
                   --------------- */
                dataNibble = scriptTable[scrPos] & 0xf;
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
                sleepCycles = scriptTable[scrPos] & 0xf;
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
            mapArea.drawTile(&introMap[x + (y * INTRO_MAP_WIDTH)], false, x, y);

    /* draw animated objects */
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0)
            mapArea.drawTile(&objectStateTable[i].tile, false, objectStateTable[i].x, objectStateTable[i].y);
    }
}

/**
 * Draws the animated beasts in the upper corners of the screen.
 */
void IntroController::drawBeasties() {
    screenShowBeastie(0, beastieOffset, beastie1FrameTable[beastie1Cycle]);
    screenShowBeastie(1, beastieOffset, beastie2FrameTable[beastie2Cycle]);
    if (beastieOffset < 0)
        beastieOffset++;
}

/**
 * Paints the screen.
 */
void IntroController::updateScreen() {
    screenHideCursor();

    switch (mode) {
    case INTRO_MAP:
        screenDrawImage(BKGD_INTRO);
        drawMap();
        drawBeasties();
        break;

    case INTRO_MENU:
        screenSetCursorPos(24, 16);
        screenShowCursor();

        screenDrawImage(BKGD_INTRO);
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
    screenDrawImage(BKGD_INTRO);
    menuArea.textAt(3, 3, "By what name shalt thou be known");
    menuArea.textAt(3, 4, "in this world and time?");
    menuArea.setCursorPos(11, 7, true);

    drawBeasties();

    screenRedrawScreen();

    nameBuffer = ReadStringController::get(12, &menuArea);
    if (nameBuffer[0] == '\0') {
        mode = INTRO_MENU;
        updateScreen();
        return;
    }

    // display sex prompt and read sex from keyboard
    screenDrawImage(BKGD_INTRO);
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
}

void IntroController::showStory() {
    ReadChoiceController pauseController("");

    beastiesVisible = false;

    questionArea.enableCursor();
    questionArea.setCursorFollowsText(true);

    for (int storyInd = 0; storyInd < 24; storyInd++) {
        if (storyInd == 0)
            screenDrawImage(BKGD_TREE);
        else if (storyInd == 3)
            screenAnimateIntro("moongate");
        else if (storyInd == 5)
            screenAnimateIntro("items");
        else if (storyInd == 6)
            screenDrawImage(BKGD_PORTAL);
        else if (storyInd == 11)
            screenDrawImage(BKGD_TREE);
        else if (storyInd == 15)
            screenDrawImage(BKGD_OUTSIDE);
        else if (storyInd == 17)
            screenDrawImage(BKGD_INSIDE);
        else if (storyInd == 20)
            screenDrawImage(BKGD_WAGON);
        else if (storyInd == 21)
            screenDrawImage(BKGD_GYPSY);
        else if (storyInd == 23)
            screenDrawImage(BKGD_ABACUS);
        showText(introText[storyInd]);
    
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();
    }
    startQuestions();
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
            screenDrawImage(BKGD_ABACUS);

        // draw the cards and show the lead up text
        screenShowCard(0, questionTree[questionRound * 2]);
        screenShowCard(1, questionTree[questionRound * 2 + 1]);

        questionArea.clear();
        questionArea.textAt(0, 0, "%s", introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)].c_str());
        questionArea.textAt(0, 1, "%s", introGypsy[GYP_UPON_TABLE].c_str());
        questionArea.textAt(0, 2, "%s and %s.  She says", introGypsy[questionTree[questionRound * 2] + 4].c_str(), introGypsy[questionTree[questionRound * 2 + 1] + 4].c_str());
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

        // if done, create the savegame and continue on to the game
        if (doQuestion(choice == 'a' ? 0 : 1)) {
            SaveGame saveGame;
            SaveGamePlayerRecord avatar;

            FILE *saveGameFile = saveGameOpenForWriting();
            if (!saveGameFile) {
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

            saveGameFile = saveGameMonstersOpenForWriting(MONSTERS_SAV_BASE_FILENAME);
            if (saveGameFile) {
                saveGameMonstersWrite(NULL, saveGameFile);
                fclose(saveGameFile);
            }

            // show the text thats segues into the main game
            showText(introGypsy[GYP_SEGUE1]);

            eventHandler->pushController(&pauseController);
            pauseController.waitFor();

            showText(introGypsy[GYP_SEGUE2]);

            eventHandler->pushController(&pauseController);
            pauseController.waitFor();

            // done: exit intro and let game begin
            EventHandler::setControllerDone();

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

    return introQuestions[i + v2 - 1];
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
    saveGameFile = saveGameOpenForReading();
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

    screenDrawImage(BKGD_INTRO);
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

    if (xu4_random(2) && ++beastie1Cycle >= BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if (xu4_random(2) && ++beastie2Cycle >= BEASTIE2_FRAMES)
        beastie2Cycle = 0;
}

/**
 * Update the screen when an observed menu is reset or has an item
 * activated.
 */
void IntroController::update(Menu *menu, MenuEvent &event) {
    if (menu == &mainOptions)
        updateMainOptions(event);
    else if (menu == &videoOptions)
        updateVideoOptions(event);
    else if (menu == &soundOptions)
        updateSoundOptions(event);
    else if (menu == &gameplayOptions)
        updateGameplayOptions(event);
    else if (menu == &advancedOptions)
        updateAdvancedOptions(event);
    else if (menu == &enhancementOptions)
        updateEnhancementOptions(event);
    else if (menu == &keyboardOptions)
        updateKeyboardOptions(event);
    else if (menu == &speedOptions)
        updateSpeedOptions(event);

    if (beastiesVisible)
        drawBeasties();
}

void IntroController::updateMainOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0:
            beastiesVisible = false;
            runMenu(&videoOptions, &extendedMenuArea, false);
            beastiesVisible = true;
            break;
        case 1:
            runMenu(&soundOptions, &menuArea, true);
            break;
        case 2:
            beastiesVisible = false;
            runMenu(&gameplayOptions, &extendedMenuArea, false);
            beastiesVisible = true;
            break;
        case CANCEL:
        default:
            break;
        }
    }

    screenDrawImage(BKGD_INTRO);
    menuArea.textAt(8, 1, "-- xu4 Configuration --");
}

void IntroController::updateVideoOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.scale++;
                if (settingsChanged.scale > 5)
                    settingsChanged.scale = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.scale--;
                if (settingsChanged.scale <= 0)
                    settingsChanged.scale = 5;
            }
            break;
        
        case 1:
            if (event.getType() == MenuEvent::DECREMENT || event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE)
                settingsChanged.fullscreen = settingsChanged.fullscreen ? 0 : 1;
            break;

        case 2:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.filter = static_cast<FilterType>(settingsChanged.filter + 1);
                if (settingsChanged.filter == SCL_MAX)
                    settingsChanged.filter = static_cast<FilterType>(SCL_MIN+1);
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.filter = static_cast<FilterType>(settingsChanged.filter - 1);
                if (settingsChanged.filter == SCL_MIN)
                    settingsChanged.filter = static_cast<FilterType>(SCL_MAX-1);
            }
            break;

        case 3:
            if (event.getType() == MenuEvent::DECREMENT || event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE)
                settingsChanged.screenShakes = settingsChanged.screenShakes ? 0 : 1;
            break;

        case 4:
            {   
                const vector<string> &imageSetNames = imageMgr->getSetNames();
                vector<string>::const_iterator set = find(imageSetNames.begin(), imageSetNames.end(), settingsChanged.videoType);
                if (set == imageSetNames.end())
                    errorFatal("Error: image set '%s' not found", settingsChanged.videoType.c_str());
            
                if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                    /* move to the next set, wrapping if necessary */
                    set++;
                    if (set == imageSetNames.end())
                        set = imageSetNames.begin();    
                
                } else if (event.getType() == MenuEvent::DECREMENT) {
                    /* move back one, wrapping if necessary */
                    if (set == imageSetNames.begin())
                        set = imageSetNames.end();                
                    set--;
                }

                settingsChanged.videoType = *set;
            }
            break;

        case 5:
            {
                const vector<string> &layoutNames = screenGetGemLayoutNames();
                vector<string>::const_iterator layout = find(layoutNames.begin(), layoutNames.end(), settingsChanged.gemLayout);
                if (layout == layoutNames.end())
                    errorFatal("Error: gem layout '%s' not found", settingsChanged.gemLayout.c_str());
       
                if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                    /* move to the next layout, wrapping if necessary */
                    layout++;
                    if (layout == layoutNames.end())
                        layout = layoutNames.begin();
            
                } else if (event.getType() == MenuEvent::DECREMENT) {
                    /* move back one, wrapping if necessary */
                    if (layout == layoutNames.begin())
                        layout = layoutNames.end();
                    layout--;
                }

                settingsChanged.gemLayout = *layout;
            }
            break;        

        case USE_SETTINGS:
            /* save settings (if necessary) */
            if (settings != settingsChanged) {
                settings.setData(settingsChanged);
                settings.write();

                /* FIXME: resize images, etc. */
                screenReInit();
            
                // Fix the menu since it was obliterated
                mode = INTRO_MENU; 
                runMenu(&mainOptions, &menuArea, true);
            }        
            break;
        case CANCEL:
            /* discard settings */
            settingsChanged = settings;
            break;
        
        default: break;
        }
    }

    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0, "Video Options:");
    extendedMenuArea.textAt(23, 2, "%s", settingsChanged.videoType.c_str());
    extendedMenuArea.textAt(23, 3, "%s", settingsChanged.gemLayout.c_str());
    extendedMenuArea.textAt(23, 4, "x%d", settingsChanged.scale);
    extendedMenuArea.textAt(23, 5, "%s", settingsChanged.fullscreen ? "Fullscreen" : "Window");
    extendedMenuArea.textAt(23, 6, "%s", settings.filters.getName(settingsChanged.filter).c_str());
    extendedMenuArea.textAt(23, 7, "%s", settingsChanged.screenShakes ? "On" : "Off");
}

void IntroController::updateSoundOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0: 
            settingsChanged.musicVol = settingsChanged.musicVol ? 0 : 1;
            break;
        case 1:
            settingsChanged.soundVol = settingsChanged.soundVol ? 0 : 1;
            break;
        case 2:
            settingsChanged.volumeFades = settingsChanged.volumeFades ? 0 : 1;
            break;
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

    screenDrawImage(BKGD_INTRO);
    menuArea.textAt(2, 1, "Sound Options:");
    menuArea.textAt(25, 3, "%s", settingsChanged.musicVol ? "On" : "Off");
    menuArea.textAt(25, 4, "%s", settingsChanged.soundVol ? "On" : "Off");
    menuArea.textAt(25, 5, "%s", settingsChanged.volumeFades ? "On" : "Off");
}

void IntroController::updateGameplayOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0:
            settingsChanged.enhancements = settingsChanged.enhancements ? 0 : 1;        
            break;
        case 1:
            settingsChanged.shortcutCommands = settingsChanged.shortcutCommands ? 0 : 1;
            break;
        case 3:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.battleDiff = static_cast<BattleDifficulty>(settingsChanged.battleDiff + 1);
                if (settingsChanged.battleDiff == DIFF_MAX)
                    settingsChanged.battleDiff = static_cast<BattleDifficulty>(DIFF_MIN+1);
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.battleDiff = static_cast<BattleDifficulty>(settingsChanged.battleDiff - 1);
                if (settingsChanged.battleDiff == DIFF_MIN)
                    settingsChanged.battleDiff = static_cast<BattleDifficulty>(DIFF_MAX-1);
            }
            break;
        case 2:
            // show or hide game enhancement options if enhancements are enabled/disabled
            advancedOptions.getItemById(0)->setVisible(settingsChanged.enhancements);

            runMenu(&advancedOptions, &extendedMenuArea, false);
            break;
        case 4:
            settingsChanged.mouseOptions.enabled = !settingsChanged.mouseOptions.enabled;
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

    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0, "Gameplay Options:");
    extendedMenuArea.textAt(31, 2, "%s", settingsChanged.enhancements ? "On" : "Off");        
    extendedMenuArea.textAt(5, 5, "  (Open, Jimmy, etc.)     %s", settingsChanged.shortcutCommands ? "On" : "Off");        
    extendedMenuArea.textAt(31, 7, "%s", settings.battleDiffs.getName(settingsChanged.battleDiff).c_str());
    extendedMenuArea.textAt(31, 9, "%s", settingsChanged.mouseOptions.enabled ? "On" : "Off");
}

void IntroController::updateAdvancedOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 1:
            settingsChanged.debug = settingsChanged.debug ? 0 : 1;
            break;
        case 0:
            runMenu(&enhancementOptions, &extendedMenuArea, false);
            break;
        case 2:
            runMenu(&keyboardOptions, &extendedMenuArea, false);
            break;
        case 3:
            runMenu(&speedOptions, &extendedMenuArea, false);
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

    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0,   "Advanced Options:");
    extendedMenuArea.textAt(33, 5,  "%s", settingsChanged.debug ? "On" : "Off");
}

void IntroController::updateEnhancementOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0: 
            settingsChanged.enhancementsOptions.u5shrines = settingsChanged.enhancementsOptions.u5shrines ? 0 : 1;
            break;
        case 1: 
            settingsChanged.enhancementsOptions.slimeDivides = settingsChanged.enhancementsOptions.slimeDivides ? 0 : 1;
            break;
        case 2: 
            settingsChanged.enhancementsOptions.c64chestTraps = settingsChanged.enhancementsOptions.c64chestTraps ? 0 : 1;
            break;    
        case 4:
            settingsChanged.enhancementsOptions.u5spellMixing = settingsChanged.enhancementsOptions.u5spellMixing ? 0 : 1;
            break;
        case 5:
            settingsChanged.enhancementsOptions.smartEnterKey = settingsChanged.enhancementsOptions.smartEnterKey ? 0 : 1;
            break;
        case 6:
            settingsChanged.enhancementsOptions.activePlayer = settingsChanged.enhancementsOptions.activePlayer ? 0 : 1;
            break;
        case 7:
            settingsChanged.enhancementsOptions.peerShowsObjects = settingsChanged.enhancementsOptions.peerShowsObjects ? 0 : 1;
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

    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0,  "Game Enhancement Options:");        
    extendedMenuArea.textAt(30, 2, "%s", settingsChanged.enhancementsOptions.activePlayer ? "On" : "Off");
    extendedMenuArea.textAt(30, 3, "%s", settingsChanged.enhancementsOptions.u5spellMixing ? "On" : "Off");
    extendedMenuArea.textAt(30, 4, "%s", settingsChanged.enhancementsOptions.u5shrines ? "On" : "Off");
    extendedMenuArea.textAt(30, 5, "%s", settingsChanged.enhancementsOptions.slimeDivides ? "On" : "Off");
    extendedMenuArea.textAt(30, 6, "%s", settingsChanged.enhancementsOptions.c64chestTraps ? "On" : "Off");
    extendedMenuArea.textAt(30, 7, "%s", settingsChanged.enhancementsOptions.smartEnterKey ? "On" : "Off");
    extendedMenuArea.textAt(30, 8, "%s", settingsChanged.enhancementsOptions.peerShowsObjects ? "On" : "Off");
}

void IntroController::updateKeyboardOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 1:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.keydelay += 100;
                if (settingsChanged.keydelay > MAX_KEY_DELAY)
                    settingsChanged.keydelay = 100;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.keydelay -= 100;
                if (settingsChanged.keydelay < 100)
                    settingsChanged.keydelay = MAX_KEY_DELAY;
            }
            break;
        case 2:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.keyinterval += 10;
                if (settingsChanged.keyinterval > MAX_KEY_INTERVAL)
                    settingsChanged.keyinterval = 10;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.keyinterval -= 10;
                if (settingsChanged.keyinterval < 10)
                    settingsChanged.keyinterval = MAX_KEY_INTERVAL;
            }
            break;
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

    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0, "Keyboard Settings:");
    extendedMenuArea.textAt(33, 2,  "%d", settingsChanged.keydelay);
    extendedMenuArea.textAt(33, 3,  "%d", settingsChanged.keyinterval);


}

void IntroController::updateSpeedOptions(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case 0:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.gameCyclesPerSecond++;
                if (settingsChanged.gameCyclesPerSecond > MAX_CYCLES_PER_SECOND)
                    settingsChanged.gameCyclesPerSecond = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.gameCyclesPerSecond--;
                if (settingsChanged.gameCyclesPerSecond < 1)
                    settingsChanged.gameCyclesPerSecond = MAX_CYCLES_PER_SECOND;
            }
            break;
        case 1:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.battleSpeed++;
                if (settingsChanged.battleSpeed > MAX_BATTLE_SPEED)
                    settingsChanged.battleSpeed = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.battleSpeed--;
                if (settingsChanged.battleSpeed < 1)
                    settingsChanged.battleSpeed = MAX_BATTLE_SPEED;
            }
            break;
        case 2:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.spellEffectSpeed++;
                if (settingsChanged.spellEffectSpeed > MAX_SPELL_EFFECT_SPEED)
                    settingsChanged.spellEffectSpeed = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.spellEffectSpeed--;
                if (settingsChanged.spellEffectSpeed < 1)
                    settingsChanged.spellEffectSpeed = MAX_SPELL_EFFECT_SPEED;
            }
            break;
        case 3:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.campTime++;
                if (settingsChanged.campTime > MAX_CAMP_TIME)
                    settingsChanged.campTime = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.campTime--;
                if (settingsChanged.campTime < 1)
                    settingsChanged.campTime = MAX_CAMP_TIME;
            }
            break;
        case 4:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.innTime++;
                if (settingsChanged.innTime > MAX_INN_TIME)
                    settingsChanged.innTime = 1;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.innTime--;
                if (settingsChanged.innTime < 1)
                    settingsChanged.innTime = MAX_INN_TIME;
            }
            break;
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
        case 6:
            if (event.getType() == MenuEvent::INCREMENT || event.getType() == MenuEvent::ACTIVATE) {
                settingsChanged.shakeInterval += 10;
                if (settingsChanged.shakeInterval > MAX_SHAKE_INTERVAL)
                    settingsChanged.shakeInterval = MIN_SHAKE_INTERVAL;
            } else if (event.getType() == MenuEvent::DECREMENT) {
                settingsChanged.shakeInterval -= 10;
                if (settingsChanged.shakeInterval < MIN_SHAKE_INTERVAL)
                    settingsChanged.shakeInterval = MAX_SHAKE_INTERVAL;
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

    char msg[16] = {0};
    screenDrawImage(BKGD_INTRO_EXTENDED);
    extendedMenuArea.textAt(1, 0, "Speed Settings:");           
    extendedMenuArea.textAt(29, 2, "%3d", settingsChanged.gameCyclesPerSecond);

    sprintf(msg, "%d", settingsChanged.battleSpeed);
    extendedMenuArea.textAt(32 - strlen(msg), 3, msg);

    sprintf(msg, "%0.*f sec",
            (settingsChanged.spellEffectSpeed % 5 == 0) ? 0 : 1,
            static_cast<double>(settingsChanged.spellEffectSpeed) / 5);        
    extendedMenuArea.textAt(36 - strlen(msg), 4, msg);

    extendedMenuArea.textAt(29, 5, "%3d sec", settingsChanged.campTime);

    sprintf(msg, "%d sec", settingsChanged.innTime);
    extendedMenuArea.textAt(36 - strlen(msg), 6, msg);

    sprintf(msg, "%d sec", settingsChanged.shrineTime);
    extendedMenuArea.textAt(36 - strlen(msg), 7, msg);

    sprintf(msg, "%d msec", settingsChanged.shakeInterval);
    extendedMenuArea.textAt(37 - strlen(msg), 8, msg);
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
    int tmp;
    
    if (!answer)
        questionTree[answerInd] = questionTree[questionRound * 2];
    else
        questionTree[answerInd] = questionTree[questionRound * 2 + 1];
    
    screenShowAbacusBeads(questionRound, questionTree[answerInd], 
        questionTree[questionRound * 2 + ((answer) ? 0 : 1)]);

    answerInd++;
    questionRound++;

    if (questionRound > 6)
        return true;

    if (questionTree[questionRound * 2] > questionTree[questionRound * 2 + 1]) {
        tmp = questionTree[questionRound * 2];
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
        int xp, x, y;
        int virtues[8];
    } initValuesForClass[] = {
        { WEAP_STAFF,  ARMR_CLOTH,   125, 231, 136, { 65, 50, 60, 50, 50, 55, 55, 50 } }, /* CLASS_MAGE */
        { WEAP_SLING,  ARMR_CLOTH,   240,  83, 105, { 50, 65, 55, 55, 50, 60, 50, 50 } }, /* CLASS_BARD */
        { WEAP_AXE,    ARMR_LEATHER, 205,  35, 221, { 50, 55, 65, 50, 55, 50, 60, 50 } }, /* CLASS_FIGHTER */
        { WEAP_DAGGER, ARMR_CLOTH,   175,  59,  44, { 55, 50, 50, 65, 50, 60, 55, 50 } }, /* CLASS_DRUID */
        { WEAP_MACE,   ARMR_LEATHER, 110, 158,  21, { 50, 50, 60, 50, 65, 55, 55, 50 } }, /* CLASS_TINKER */
        { WEAP_SWORD,  ARMR_CHAIN,   325, 105, 183, { 50, 50, 50, 55, 60, 65, 50, 55 } }, /* CLASS_PALADIN */
        { WEAP_SWORD,  ARMR_LEATHER, 150,  23, 129, { 50, 50, 50, 55, 55, 60, 65, 50 } }, /* CLASS_RANGER */
        { WEAP_STAFF,  ARMR_CLOTH,     5, 186, 171, { 50, 50, 60, 50, 50, 55, 55, 65 } }  /* CLASS_SHEPHERD */
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
            saveGame->players[p].hp = saveGame->players[p].hpMax = player.getMaxLevel() * 100;
            saveGame->players[p].mp = player.getMaxMp();
            p++;
        }
    }
}
