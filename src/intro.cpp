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
    unsigned char tile;  /* base tile + tile frame */
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

IntroController::IntroController() : Controller(1) {
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
int IntroController::init() {
    U4FILE *title;
    int i, j;

    mode = INTRO_MAP;
    beastie1Cycle = 0;
    beastie2Cycle = 0;
    beastieOffset = -32;    

    title = u4fopen("title.exe");
    if (!title)
        return 0;

    introQuestions = u4read_stringtable(title, INTRO_TEXT_OFFSET, 28);
    introText = u4read_stringtable(title, -1, 24);
    introGypsy = u4read_stringtable(title, -1, 15);

    /* clean up stray newlines at end of strings */
    for (i = 0; i < 15; i++) {
        int len;
        while ((len = introGypsy[i].length()) > 0 && 
               isspace(introGypsy[i][len - 1]))
            introGypsy[i][len - 1] = '\0';
    }

    if (sigData)
        delete sigData;
    sigData = new unsigned char[533];
    u4fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    u4fread(sigData, 1, 533, title);

    u4fseek(title, INTRO_MAP_OFFSET, SEEK_SET);
    introMap = new unsigned char *[INTRO_MAP_HEIGHT];
    introMap[0] = new unsigned char[INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT];
    for (i = 0; i < INTRO_MAP_HEIGHT; i++) {
        introMap[i] = introMap[0] + INTRO_MAP_WIDTH * i;
        for (j = 0; j < INTRO_MAP_WIDTH; j++) {
            introMap[i][j] = static_cast<unsigned char>(u4fgetc(title));
        }
    }

    sleepCycles = 0;
    scrPos = 0;

    u4fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = new unsigned char[INTRO_SCRIPT_TABLE_SIZE];
    for (i = 0; i < INTRO_SCRIPT_TABLE_SIZE; i++)
        scriptTable[i] = u4fgetc(title);

    u4fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = new unsigned char[INTRO_BASETILE_TABLE_SIZE];
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++)
        baseTileTable[i] = u4fgetc(title);

    objectStateTable = new IntroObjectState[INTRO_BASETILE_TABLE_SIZE];
    memset(objectStateTable, 0, sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);

    /* --------------------------
       load beastie frame table 1
       -------------------------- */
    beastie1FrameTable = new unsigned char[BEASTIE1_FRAMES];
    if (!beastie1FrameTable) {
        u4fclose(title);
        return(0);
    }
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE1_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE1_FRAMES; i++) {
        beastie1FrameTable[i] = u4fgetc(title);
    }

    /* --------------------------
       load beastie frame table 2
       -------------------------- */
    beastie2FrameTable = new unsigned char[BEASTIE2_FRAMES];
    if (!beastie2FrameTable) {
        u4fclose(title);
        return(0);
    }
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE2_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE2_FRAMES; i++) {
        beastie2FrameTable[i] = u4fgetc(title);
    }

    u4fclose(title);

    /* load our menus, checking to see if they're already loaded first */
    if (!menusLoaded) {
        mainOptions.add(0, "Video Options", 13, 16, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
        mainOptions.add(1, "Sound Options", 13, 17, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
        mainOptions.add(2, "Gameplay Options", 13, 18, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
        mainOptions.add(0xFF, "Main Menu", 13, 21, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        videoOptions.add(4, "Graphics", 6, 5, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(5, "Gem Layout", 6, 6, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(0, "Scale", 6, 7, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(1, "Mode", 6, 8, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(2, "Filter", 6, 9, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(3, "Screen Shaking", 6, 10, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
        videoOptions.add(0xFE, "Use These Settings", 6, 20, &introVideoOptionsMenuItemActivate, ACTIVATE_NORMAL);
        videoOptions.add(0xFF, "Cancel", 6, 21, &introVideoOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        soundOptions.add(0, "Volume", 11, 16, &introSoundOptionsMenuItemActivate, ACTIVATE_ANY);
        soundOptions.add(1, "Sound Effects", 11, 17, &introSoundOptionsMenuItemActivate, ACTIVATE_ANY);
        soundOptions.add(2, "Fading", 11, 18, &introSoundOptionsMenuItemActivate, ACTIVATE_ANY);
        soundOptions.add(0xFE, "Use These Settings", 11, 20, &introSoundOptionsMenuItemActivate, ACTIVATE_NORMAL);
        soundOptions.add(0xFF, "Cancel", 11, 21, &introSoundOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        gameplayOptions.add(0, "Game Enhancements", 6, 5, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);    
        gameplayOptions.add(1, "Automatic Actions", 6, 7, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);    
        gameplayOptions.add(3, "Battle Difficulty", 6, 10, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);
        gameplayOptions.add(4, "Mouse", 6, 12, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);
        gameplayOptions.add(2, "\010 Advanced Options", 6, 18, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
        gameplayOptions.add(0xFE, "Use These Settings", 6, 20, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
        gameplayOptions.add(0xFF, "Cancel", 6, 21, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        advancedOptions.add(3, "\010 Speed Settings", 4, 5, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
        advancedOptions.add(2, "\010 Keyboard Settings", 4, 6, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
        advancedOptions.add(1, "Debug Mode (Cheats)", 4, 8, &introAdvancedOptionsMenuItemActivate, ACTIVATE_ANY);        
        advancedOptions.add(0, "\010 Game Enhancement Options", 4, 18, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);    
        advancedOptions.add(0xFE, "Use These Settings", 4, 20, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
        advancedOptions.add(0xFF, "Cancel", 4, 21, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        keyboardOptions.add(1, "Repeat Delay (in msecs)", 5, 5, &introKeyboardOptionsMenuItemActivate, ACTIVATE_ANY);
        keyboardOptions.add(2, "Repeat Interval (in msecs)", 5, 6, &introKeyboardOptionsMenuItemActivate, ACTIVATE_ANY);
        keyboardOptions.add(0xFE, "Use These Settings", 5, 20, &introKeyboardOptionsMenuItemActivate, ACTIVATE_NORMAL);
        keyboardOptions.add(0xFF, "Cancel", 5, 21, &introKeyboardOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        speedOptions.add(0, "Game Cycles Per Second", 4, 5, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(1, "Battle Speed", 4, 6, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(2, "Spell Effect Length", 4, 7, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(3, "Camping length", 4, 8, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(4, "Inn rest length", 4, 9, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(5, "Shrine Meditation length", 4, 10, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(6, "Screen Shake Interval", 4, 11, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
        speedOptions.add(0xFE, "Use These Settings", 4, 20, &introSpeedOptionsMenuItemActivate, ACTIVATE_NORMAL);
        speedOptions.add(0xFF, "Cancel", 4, 21, &introSpeedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
        enhancementOptions.add(6, "Set Active Player", 7, 5, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(4, "Ultima V Spell Mixing", 7, 6, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(0, "Ultima V Shrines", 7, 7, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);    
        enhancementOptions.add(1, "Slime Divides", 7, 8, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(2, "Fixed Chest Traps", 7, 9, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(5, "Smart 'Enter' Key", 7, 10, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(7, "Gem View Shows Objects", 7, 11, &introEnhancementOptionsMenuItemActivate, ACTIVATE_ANY);
        enhancementOptions.add(0xFE, "Use These Settings", 7, 20, &introEnhancementOptionsMenuItemActivate, ACTIVATE_NORMAL);
        enhancementOptions.add(0xFF, "Cancel", 7, 21, &introEnhancementOptionsMenuItemActivate, ACTIVATE_NORMAL);

        menusLoaded = true;
    }

    /* Make a copy of our settings so we can change them */
    settingsChanged = settings;

    updateScreen();

    musicIntro();

    return 1;
}

/**
 * Frees up data not needed after introduction.
 */
void IntroController::deleteIntro() {
    delete introMap[0];

    introQuestions.clear();
    introText.clear();
    introGypsy.clear();

    delete scriptTable;
    scriptTable = NULL;
    delete baseTileTable;
    baseTileTable = NULL;
    delete objectStateTable;
    objectStateTable = NULL;

    delete beastie1FrameTable;
    delete beastie2FrameTable;

    screenFreeIntroBackgrounds();    
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
        case 'c':
            errorMessage.erase();
            mode = INTRO_CONFIG;
            mainOptions.reset();
            settingsChanged = settings;
            updateScreen();
            break;
        case 'a':
            errorMessage.erase();
            about();
            break;
        case 'q':
            EventHandler::setExitFlag();
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
            musicIntroSwitch(key - '0');
            break;
        default:
            valid = false;
            break;
        }
        break;

    case INTRO_CONFIG:

        if (!baseMenuKeyHandler(key, &mainOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'v': mainOptions.activateItem(0, ACTIVATE_NORMAL); break;
            case 's': mainOptions.activateItem(1, ACTIVATE_NORMAL); break;                
            case 'g': mainOptions.activateItem(2, ACTIVATE_NORMAL); break;                
            case 'm': mainOptions.activateItem(0xFF, ACTIVATE_NORMAL); break;                
            default: break;
            }
        }

        updateScreen();
        return true;

    case INTRO_CONFIG_VIDEO:
        
        if (!baseMenuKeyHandler(key, &videoOptions)) {
            /* navigate to the item and activate it! */
            switch (key) {
            case 'g': videoOptions.activateItem(4, ACTIVATE_NORMAL); break;
            case 's': videoOptions.activateItem(0, ACTIVATE_NORMAL); break;
            case 'm': videoOptions.activateItem(1, ACTIVATE_NORMAL); break;
            case 'f': videoOptions.activateItem(2, ACTIVATE_NORMAL); break;
            case 'k': videoOptions.activateItem(3, ACTIVATE_NORMAL); break;
            default: break;
            }
        }
        
        updateScreen();
        return true;

    case INTRO_CONFIG_SOUND:
        
        if (!baseMenuKeyHandler(key, &soundOptions)) {
            /* navigate to the item and activate it! */
            switch (key) {
            case 'v': soundOptions.activateItem(0, ACTIVATE_NORMAL); break;
            case 's': soundOptions.activateItem(1, ACTIVATE_NORMAL); break;
            case 'f': soundOptions.activateItem(2, ACTIVATE_NORMAL); break;
            default: break;
            }
        }
        
        updateScreen();
        return true;

    case INTRO_CONFIG_GAMEPLAY:
        
        if (!baseMenuKeyHandler(key, &gameplayOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'g': gameplayOptions.activateItem(0, ACTIVATE_NORMAL); break;                
            case 'a': gameplayOptions.activateItem(1, ACTIVATE_NORMAL); break;                
            case 'o': gameplayOptions.activateItem(2, ACTIVATE_NORMAL); break;
            case 'b': gameplayOptions.activateItem(3, ACTIVATE_NORMAL); break;
            default: break;
            }            
        }

        updateScreen();
        return true;

    case INTRO_CONFIG_ADVANCED:
        if (!baseMenuKeyHandler(key, &advancedOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'd': advancedOptions.activateItem(1, ACTIVATE_NORMAL); break;            
            case 'k': advancedOptions.activateItem(2, ACTIVATE_NORMAL); break;
            case 's': advancedOptions.activateItem(3, ACTIVATE_NORMAL); break;
            case 'g': advancedOptions.activateItem(0, ACTIVATE_NORMAL); break;            
            default: break;
            }
        }
        
        updateScreen();
        return true;

    case INTRO_CONFIG_KEYBOARD:
        baseMenuKeyHandler(key, &keyboardOptions);
        
        updateScreen();
        return true;

    case INTRO_CONFIG_SPEED:
        baseMenuKeyHandler(key, &speedOptions);
        
        updateScreen();
        return true;
        
    case INTRO_CONFIG_ENHANCEMENT_OPTIONS:
        baseMenuKeyHandler(key, &enhancementOptions);

        updateScreen();
        return true;

    case INTRO_ABOUT:
    case INTRO_INIT:
    case INTRO_INIT_STORY:
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
                objectStateTable[dataNibble].tile = baseTileTable[dataNibble] + (scriptTable[scrPos+1] >> 5);
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
    for (y = 0; y < INTRO_MAP_HEIGHT; y++) {
        for (x = 0; x < INTRO_MAP_WIDTH; x++) {
            MapTile tile = Tile::translate(introMap[y][x]);
            screenShowTile(&tile, false, x, y + 6);
        }
    }

    /* draw animated objects */
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0) {
            MapTile tile = Tile::translate(objectStateTable[i].tile);
            screenShowTile(&tile, false, objectStateTable[i].x, objectStateTable[i].y + 6);
        }
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
        screenTextAt(2, 14, "In another world, in a time to come.");
        screenTextAt(15, 16, "Options:");
        screenTextAt(11, 17, "Return to the view");
        screenTextAt(11, 18, "Journey Onward");
        screenTextAt(11, 19, "Initiate New Game");
        screenTextAt(11, 20, "Configure");
        screenTextAt(11, 21, "About");
        if (!errorMessage.empty())
            screenTextAt(11, 22, errorMessage.c_str());
        drawBeasties();
        break;

    case INTRO_CONFIG:
        screenDrawImage(BKGD_INTRO);
        screenTextAt(9, 14, "-- xu4 Configuration --");
        mainOptions.show();
        drawBeasties();
        break;

    case INTRO_CONFIG_VIDEO:
        screenDrawImage(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Video Options:");
        screenTextAt(24, 5, "%s", settingsChanged.videoType.c_str());
        screenTextAt(24, 6, "%s", settingsChanged.gemLayout.c_str());
        screenTextAt(24, 7, "x%d", settingsChanged.scale);
        screenTextAt(24, 8, "%s", settingsChanged.fullscreen ? "Fullscreen" : "Window");
        screenTextAt(24, 9, "%s", settings.filters.getName(settingsChanged.filter).c_str());
        screenTextAt(24, 10, "%s", settingsChanged.screenShakes ? "On" : "Off");
        videoOptions.show();        
        break;

    case INTRO_CONFIG_SOUND:
        screenDrawImage(BKGD_INTRO);
        screenTextAt(2, 14, "Sound Options:");
        screenTextAt(26, 16, "%s", settingsChanged.musicVol ? "On" : "Off");
        screenTextAt(26, 17, "%s", settingsChanged.soundVol ? "On" : "Off");
        screenTextAt(26, 18, "%s", settingsChanged.volumeFades ? "On" : "Off");        
        soundOptions.show();
        drawBeasties();
        break;

    case INTRO_CONFIG_GAMEPLAY:
        screenDrawImage(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Gameplay Options:");
        screenTextAt(32, 5, "%s", settingsChanged.enhancements ? "On" : "Off");        
        screenTextAt(6, 8, "  (Open, Jimmy, etc.)     %s", settingsChanged.shortcutCommands ? "On" : "Off");        
        screenTextAt(32, 10, "%s", settings.battleDiffs.getName(settingsChanged.battleDiff).c_str());
        screenTextAt(32, 12, "%s", settingsChanged.mouseOptions.enabled ? "On" : "Off");
        gameplayOptions.show();
        break;

    case INTRO_CONFIG_ADVANCED:
        screenDrawImage(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3,   "Advanced Options:");
        screenTextAt(34, 8,  "%s", settingsChanged.debug ? "On" : "Off");        
        advancedOptions.show();
        break;

    case INTRO_CONFIG_KEYBOARD:
        screenDrawImage(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Keyboard Settings:");
        screenTextAt(34, 5,  "%d", settingsChanged.keydelay);
        screenTextAt(34, 6,  "%d", settingsChanged.keyinterval);
        keyboardOptions.show();        
        break;

    case INTRO_CONFIG_SPEED:
        {
            char msg[16] = {0};
            screenDrawImage(BKGD_INTRO_EXTENDED);
            screenTextAt(2, 3, "Speed Settings:");           

            sprintf(msg, "%d", settingsChanged.gameCyclesPerSecond);
            screenTextAt(33 - strlen(msg), 5, msg);

            sprintf(msg, "%d", settingsChanged.battleSpeed);
            screenTextAt(33 - strlen(msg), 6, msg);
            
            sprintf(msg, "%0.*f sec",
                (settingsChanged.spellEffectSpeed % 5 == 0) ? 0 : 1,
                static_cast<double>(settingsChanged.spellEffectSpeed) / 5);        
            screenTextAt(37 - strlen(msg), 7, msg);

            sprintf(msg, "%d sec", settingsChanged.campTime);
            screenTextAt(37 - strlen(msg), 8, msg);

            sprintf(msg, "%d sec", settingsChanged.innTime);
            screenTextAt(37 - strlen(msg), 9, msg);

            sprintf(msg, "%d sec", settingsChanged.shrineTime);
            screenTextAt(37 - strlen(msg), 10, msg);

            sprintf(msg, "%d msec", settingsChanged.shakeInterval);
            screenTextAt(38 - strlen(msg), 11, msg);

            speedOptions.show();
        }
        break;

    case INTRO_CONFIG_ENHANCEMENT_OPTIONS:
        screenDrawImage(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3,   "Game Enhancement Options:");        
        screenTextAt(31, 5,  "%s", settingsChanged.enhancementsOptions.activePlayer ? "On" : "Off");
        screenTextAt(31, 6,  "%s", settingsChanged.enhancementsOptions.u5spellMixing ? "On" : "Off");
        screenTextAt(31, 7,  "%s", settingsChanged.enhancementsOptions.u5shrines ? "On" : "Off");
        screenTextAt(31, 8,  "%s", settingsChanged.enhancementsOptions.slimeDivides ? "On" : "Off");
        screenTextAt(31, 9,  "%s", settingsChanged.enhancementsOptions.c64chestTraps ? "On" : "Off");
        screenTextAt(31, 10,  "%s", settingsChanged.enhancementsOptions.smartEnterKey ? "On" : "Off");
        screenTextAt(31, 11,  "%s", settingsChanged.enhancementsOptions.peerShowsObjects ? "On" : "Off");
        enhancementOptions.show();
        break;    

    case INTRO_ABOUT:
    case INTRO_INIT:
    case INTRO_INIT_STORY:
        break;

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

    // display name prompt and read name from keyboard
    screenDrawImage(BKGD_INTRO);
    screenTextAt(4, 16, "By what name shalt thou be known");
    screenTextAt(4, 17, "in this world and time?");
    drawBeasties();
    screenSetCursorPos(12, 20);
    screenShowCursor();
    screenRedrawScreen();

    ReadStringController nameCont(12, 12, 20);
    eventHandler.pushController(&nameCont);
    nameBuffer = nameCont.waitFor();

    if (nameBuffer[0] == '\0') {
        mode = INTRO_MENU;
        updateScreen();
        return;
    }

    // display sex prompt and read sex from keyboard
    screenDrawImage(BKGD_INTRO);
    screenTextAt(4, 16, "Art thou Male or Female?");
    drawBeasties();
    screenSetCursorPos(29, 16);
    screenShowCursor();

    ReadChoiceController sexCont("mf");
    eventHandler.pushController(&sexCont);
    int sexChoice = sexCont.waitFor();

    if (sexChoice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    // show the lead up story
    showStory();
}

void IntroController::showStory() {
    ReadChoiceController pauseController("");

    mode = INTRO_INIT_STORY;

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
    
        eventHandler.pushController(&pauseController);
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

        // draw the cards
        screenShowCard(0, questionTree[questionRound * 2]);
        screenShowCard(1, questionTree[questionRound * 2 + 1]);

        screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);
        
        screenTextAt(0, 19, "%s", introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)].c_str());
        screenTextAt(0, 20, "%s", introGypsy[GYP_UPON_TABLE].c_str());
        screenTextAt(0, 21, "%s and %s.  She says", introGypsy[questionTree[questionRound * 2] + 4].c_str(), introGypsy[questionTree[questionRound * 2 + 1] + 4].c_str());
        screenTextAt(0, 22, "\"Consider this:\"");
        screenSetCursorPos(16, 22);

        eventHandler.pushController(&pauseController);
        pauseController.waitFor();

        showText(getQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));

        eventHandler.pushController(&questionController);
        int choice = questionController.waitFor();

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

            eventHandler.pushController(&pauseController);
            pauseController.waitFor();

            showText(introGypsy[GYP_SEGUE2]);

            eventHandler.pushController(&pauseController);
            pauseController.waitFor();

            // done: exit intro and let game begin
            EventHandler::setExitFlag();

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

    /*
     * ensure a party.sav file exists, otherwise require user to
     * initiate game
     */
    saveGameFile = saveGameOpenForReading();
    if (!saveGameFile) {
        errorMessage = "Initiate game first!";
        updateScreen();
        screenRedrawScreen();
        return;
    }

    fclose(saveGameFile);
    EventHandler::setExitFlag();
}

void IntroController::about() {
    mode = INTRO_ABOUT;

    screenDrawImage(BKGD_INTRO);
    screenTextAt(15, 14, "XU4 %s", VERSION);
    screenTextAt(2, 16, "xu4 is free software; you can redist-");
    screenTextAt(2, 17, "ribute it and/or modify it under the");
    screenTextAt(2, 18, "terms of the GNU GPL as published by");
    screenTextAt(2, 19, "the FSF.  See COPYING.");
    screenTextAt(2, 21, "\011 Copyright 2002-2003 xu4 team");
    screenTextAt(2, 22, "\011 Copyright 1987 Lord British");
    drawBeasties();
    screenHideCursor();

    ReadChoiceController pauseController("");
    eventHandler.pushController(&pauseController);
    pauseController.waitFor();

    mode = INTRO_MENU;
    screenShowCursor();
    updateScreen();
}

/**
 * Shows text in the lower six lines of the screen.
 */
void IntroController::showText(const string &text) {
    string current = text;
    int lineNo = INTRO_TEXT_Y;
    unsigned long pos;

    screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);    
    
    pos = current.find("\n");
    while (pos < current.length()) {
        screenTextAt(0, lineNo++, "%s", current.substr(0, pos).c_str());
        current = current.substr(pos+1);
        pos = current.find("\n");
    }
    
    /* write the last line (possibly only line) */
    screenTextAt(0, lineNo++, "%s", current.substr(0, pos).c_str());

    screenSetCursorPos(current.length(), lineNo - 1);
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
    if (mode == INTRO_MAP || mode == INTRO_MENU || mode == INTRO_CONFIG ||
        mode == INTRO_ABOUT || mode == INTRO_CONFIG_SOUND ||        
        mode == INTRO_INIT)
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
 */
int IntroController::doQuestion(int answer) {
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
        return 1;

    if (questionTree[questionRound * 2] > questionTree[questionRound * 2 + 1]) {
        tmp = questionTree[questionRound * 2];
        questionTree[questionRound * 2] = questionTree[questionRound * 2 + 1];
        questionTree[questionRound * 2 + 1] = tmp;
    }

    return 0;
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

/**
 * The base key handler for the configuration menus
 */
int IntroController::baseMenuKeyHandler(int key, void *data) {
    Menu *menu = static_cast<Menu *>(data);
    char cancelKey = (mode == INTRO_CONFIG) ? 'm' : 'c';
    char saveKey = (mode == INTRO_CONFIG) ? '\0' : 'u';

    if (key == cancelKey)
        return baseMenuKeyHandler(' ', menu);
    else if (key == saveKey)
        return baseMenuKeyHandler(0, menu);
    
    switch(key) {
    case U4_UP:
        menu->prev();        
        break;
    case U4_DOWN:
        menu->next();
        break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_ENTER:
        {
            MenuItem *menuItem = &(*menu->getCurrent());
            ActivateAction action = ACTIVATE_NORMAL;
            
            if (menuItem->getActivateFunc()) {
                if (key == U4_LEFT)
                    action = ACTIVATE_DECREMENT;
                else if (key == U4_RIGHT)
                    action = ACTIVATE_INCREMENT;
                menu->activateItem(-1, action);
            }
        }
        break;
    case ' ':    
        /* activate the 'cancel' menu item */
        menu->activateItem(0xFF, ACTIVATE_NORMAL);
        break;    
    case 0:
        /* activate the 'save' menu item */
        menu->activateItem(0xFE, ACTIVATE_NORMAL);        
        break;
    case U4_ESC:
        /* go back to the main screen */
        mode = INTRO_MENU; break;
    default:
        return 0;
    }    

    return 1;
}

/* main options menu handler */
void IntroController::introMainOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {
    if (action != ACTIVATE_NORMAL)
        return;

    switch(menuItem->getId()) {
    case 0: intro->mode = INTRO_CONFIG_VIDEO; videoOptions.reset(); break;        
    case 1: intro->mode = INTRO_CONFIG_SOUND; soundOptions.reset(); break;
    case 2: intro->mode = INTRO_CONFIG_GAMEPLAY; gameplayOptions.reset(); break;
    case 0xFF: intro->mode = INTRO_MENU; break;
    default: break;
    }
}

/* video options menu handler */
void IntroController::introVideoOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 0:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.scale++;
            if (settingsChanged.scale > 5)
                settingsChanged.scale = 1;
        } else {
            settingsChanged.scale--;
            if (settingsChanged.scale <= 0)
                settingsChanged.scale = 5;
        }
        break;
        
    case 1:
        settingsChanged.fullscreen = settingsChanged.fullscreen ? 0 : 1;
        break;

    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.filter = static_cast<FilterType>(settingsChanged.filter + 1);
            if (settingsChanged.filter == SCL_MAX)
                settingsChanged.filter = static_cast<FilterType>(SCL_MIN+1);
        } else {
            settingsChanged.filter = static_cast<FilterType>(settingsChanged.filter - 1);
            if (settingsChanged.filter == SCL_MIN)
                settingsChanged.filter = static_cast<FilterType>(SCL_MAX-1);
        }
        break;

    case 3:
        settingsChanged.screenShakes = settingsChanged.screenShakes ? 0 : 1;
        break;

    case 4:
        {   
            const vector<string> &imageSetNames = screenGetImageSetNames();
            vector<string>::const_iterator set = find(imageSetNames.begin(), imageSetNames.end(), settingsChanged.videoType);
            if (set == imageSetNames.end())
                errorFatal("Error: image set '%s' not found", settingsChanged.videoType.c_str());
            
            if (action != ACTIVATE_DECREMENT) {
                /* move to the next set, wrapping if necessary */
                set++;
                if (set == imageSetNames.end())
                    set = imageSetNames.begin();    
                
            } else {
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
       
            if (action != ACTIVATE_DECREMENT) {
                /* move to the next layout, wrapping if necessary */
                layout++;
                if (layout == layoutNames.end())
                    layout = layoutNames.begin();
            
            } else {
                /* move back one, wrapping if necessary */
                if (layout == layoutNames.begin())
                    layout = layoutNames.end();
                layout--;
            }

            settingsChanged.gemLayout = *layout;
        }
        break;        

    case 0xFE:
        /* save settings (if necessary) */
        if (settings != settingsChanged) {
            settings.setData(settingsChanged);
            settings.write();

            /* FIXME: resize images, etc. */
            screenReInit();
        }        
    
        intro->mode = INTRO_CONFIG;
        
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG;
        break;
        
    default: break;
    }
}

/* sound options menu handler */
void IntroController::introSoundOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 0: 
        settingsChanged.musicVol = settingsChanged.musicVol ? 0 : 1;
        break;
    case 1:
        settingsChanged.soundVol = settingsChanged.soundVol ? 0 : 1;
        break;
    case 2:
        settingsChanged.volumeFades = settingsChanged.volumeFades ? 0 : 1;
        break;
    case 0xFE:
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();
        
        musicIntro();
    
        intro->mode = INTRO_CONFIG;
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG;
        break;
    
    default: break;
    }
}

/* gameplay options menu handler */
void IntroController::introGameplayOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 0:
        settingsChanged.enhancements = settingsChanged.enhancements ? 0 : 1;        
        break;    
    case 1:
        settingsChanged.shortcutCommands = settingsChanged.shortcutCommands ? 0 : 1;
        break;
    case 3:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.battleDiff = static_cast<BattleDifficulty>(settingsChanged.battleDiff + 1);
            if (settingsChanged.battleDiff == DIFF_MAX)
                settingsChanged.battleDiff = static_cast<BattleDifficulty>(DIFF_MIN+1);
        } else {
            settingsChanged.battleDiff = static_cast<BattleDifficulty>(settingsChanged.battleDiff - 1);
            if (settingsChanged.battleDiff == DIFF_MIN)
                settingsChanged.battleDiff = static_cast<BattleDifficulty>(DIFF_MAX-1);
        }
        break;
    case 2:
        intro->mode = INTRO_CONFIG_ADVANCED;
        advancedOptions.reset();

        /* show or hide game enhancement options if enhancements are enabled/disabled */
        advancedOptions.getItemById(0)->setVisible(settingsChanged.enhancements);

        break;    

    case 4:
        settingsChanged.mouseOptions.enabled = !settingsChanged.mouseOptions.enabled;
        break;

    case 0xFE:
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();
    
        intro->mode = INTRO_CONFIG;
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG;
        break;
    default: break;
    }
}

/* advanced options menu handler */
void IntroController::introAdvancedOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 1:
        settingsChanged.debug = settingsChanged.debug ? 0 : 1;
        break;
    case 0:
        intro->mode = INTRO_CONFIG_ENHANCEMENT_OPTIONS;
        enhancementOptions.reset();
        break;    
    case 2:
        intro->mode = INTRO_CONFIG_KEYBOARD;
        keyboardOptions.reset();
        break;
    case 3:
        intro->mode = INTRO_CONFIG_SPEED;
        speedOptions.reset();
        break;    
    case 0xFE:
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();
    
        intro->mode = INTRO_CONFIG_GAMEPLAY;
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG_GAMEPLAY;
        break;
    default: break;
    }
}

/* keyboard options menu handler */
void IntroController::introKeyboardOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 1:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.keydelay += 100;
            if (settingsChanged.keydelay > MAX_KEY_DELAY)
                settingsChanged.keydelay = 100;
        } else {
            settingsChanged.keydelay -= 100;
            if (settingsChanged.keydelay < 100)
                settingsChanged.keydelay = MAX_KEY_DELAY;
        }
        break;
    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.keyinterval += 10;
            if (settingsChanged.keyinterval > MAX_KEY_INTERVAL)
                settingsChanged.keyinterval = 10;
        } else {
            settingsChanged.keyinterval -= 10;
            if (settingsChanged.keyinterval < 10)
                settingsChanged.keyinterval = MAX_KEY_INTERVAL;
        }
        break;
    case 0xFE:
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();

        /* re-initialize keyboard */
        KeyHandler::setKeyRepeat(settingsChanged.keydelay, settingsChanged.keyinterval);
    
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    default: break;
    }    
}

/* speed options menu handler */
void IntroController::introSpeedOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {    
    switch(menuItem->getId()) {
    case 0:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.gameCyclesPerSecond++;
            if (settingsChanged.gameCyclesPerSecond > MAX_CYCLES_PER_SECOND)
                settingsChanged.gameCyclesPerSecond = 1;
        } else {
            settingsChanged.gameCyclesPerSecond--;
            if (settingsChanged.gameCyclesPerSecond < 1)
                settingsChanged.gameCyclesPerSecond = MAX_CYCLES_PER_SECOND;
        }
        break;
    case 1:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.battleSpeed++;
            if (settingsChanged.battleSpeed > MAX_BATTLE_SPEED)
                settingsChanged.battleSpeed = 1;
        } else {
            settingsChanged.battleSpeed--;
            if (settingsChanged.battleSpeed < 1)
                settingsChanged.battleSpeed = MAX_BATTLE_SPEED;
        }
        break;
    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.spellEffectSpeed++;
            if (settingsChanged.spellEffectSpeed > MAX_SPELL_EFFECT_SPEED)
                settingsChanged.spellEffectSpeed = 1;
        } else {
            settingsChanged.spellEffectSpeed--;
            if (settingsChanged.spellEffectSpeed < 1)
                settingsChanged.spellEffectSpeed = MAX_SPELL_EFFECT_SPEED;
        }
        break;
    case 3:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.campTime++;
            if (settingsChanged.campTime > MAX_CAMP_TIME)
                settingsChanged.campTime = 1;
        } else {
            settingsChanged.campTime--;
            if (settingsChanged.campTime < 1)
                settingsChanged.campTime = MAX_CAMP_TIME;
        }
        break;
    case 4:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.innTime++;
            if (settingsChanged.innTime > MAX_INN_TIME)
                settingsChanged.innTime = 1;
        } else {
            settingsChanged.innTime--;
            if (settingsChanged.innTime < 1)
                settingsChanged.innTime = MAX_INN_TIME;
        }
        break;
    case 5:
        /* make sure that the setting we're trying for is even possible */
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.shrineTime++;
            if (settingsChanged.shrineTime > MAX_SHRINE_TIME)
                settingsChanged.shrineTime = MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond;
        } else {
            settingsChanged.shrineTime--;
            if (settingsChanged.shrineTime < (MEDITATION_MANTRAS_PER_CYCLE / settingsChanged.gameCyclesPerSecond))
                settingsChanged.shrineTime = MAX_SHRINE_TIME;
        }
        break;
    case 6:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged.shakeInterval += 10;
            if (settingsChanged.shakeInterval > MAX_SHAKE_INTERVAL)
                settingsChanged.shakeInterval = MIN_SHAKE_INTERVAL;
        } else {
            settingsChanged.shakeInterval -= 10;
            if (settingsChanged.shakeInterval < MIN_SHAKE_INTERVAL)
                settingsChanged.shakeInterval = MAX_SHAKE_INTERVAL;
        }
        break;

    case 0xFE:
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();
    
        /* re-initialize events */
        eventTimerGranularity = (1000 / settings.gameCyclesPerSecond);
        eventHandler.getTimer()->reset(eventTimerGranularity);            
        
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    default: break;
    }
}

/* minor enhancement options menu handler */
void IntroController::introEnhancementOptionsMenuItemActivate(MenuItem *menuItem, ActivateAction action) {
    switch(menuItem->getId()) {
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
    case 0xFE:        
        /* save settings */
        settings.setData(settingsChanged);
        settings.write();        
    
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:        
        /* discard settings */
        settingsChanged = settings;
        intro->mode = INTRO_CONFIG_ADVANCED;
        break;
    
    default: break;
    }
}
