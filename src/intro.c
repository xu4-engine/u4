/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "u4.h"

#include "intro.h"

#include "event.h"
#include "menu.h"
#include "music.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "u4file.h"

extern int quit;

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

/**
 * The states of the intro.
 */
typedef enum {
    INTRO_MAP,                  /* displaying the animated intro map */
    INTRO_MENU,                 /* displaying the main menu: journey onward, etc. */
    INTRO_CONFIG,               /* the configuration screen */
    INTRO_CONFIG_VIDEO,         /* video configuration */
    INTRO_CONFIG_SOUND,         /* sound configuration */
    INTRO_CONFIG_GAMEPLAY,      /* gameplay configuration */
    INTRO_CONFIG_ADVANCED,      /* advanced gameplay config */
    INTRO_CONFIG_KEYBOARD,      /* keyboard config */
    INTRO_CONFIG_SPEED,         /* speed config */
    INTRO_CONFIG_MINOR_OPTIONS, /* minor enhancement options */
    INTRO_CONFIG_MAJOR_OPTIONS, /* major enhancement options */
    INTRO_ABOUT,                /* about xu4 screen */
    INTRO_INIT_NAME,            /* prompting for character name */
    INTRO_INIT_SEX,             /* prompting for character sex */
    INTRO_INIT_STORY,           /* displaying the intro story leading up the gypsy */
    INTRO_INIT_QUESTIONS,       /* prompting for the questions that determine class */
    INTRO_INIT_SEGTOGAME,       /* displaying the text that segues to the game */
    INTRO_DONE
} IntroMode;

typedef struct _IntroObjectState {
    int x, y;
    unsigned char tile;  /* base tile + tile frame */
} IntroObjectState;

#define GYP_PLACES_FIRST 0
#define GYP_PLACES_TWOMORE 1
#define GYP_PLACES_LAST 2
#define GYP_UPON_TABLE 3
#define GYP_SEGUE1 13
#define GYP_SEGUE2 14

/* introduction state */
IntroMode mode;

/* data loaded in from title.exe */
unsigned char *introMap[INTRO_MAP_HEIGHT];
char **introText;
char **introQuestions;
char **introGypsy;
unsigned char *scriptTable;
unsigned char *baseTileTable;
unsigned char *beastie1FrameTable;
unsigned char *beastie2FrameTable;
char *introErrorMessage;

/* additional introduction state data */
char nameBuffer[16];
SexType sex;
int storyInd;
int segueInd;
int answerInd;
int questionRound;
int introAskToggle;
int questionTree[15];
int beastie1Cycle;
int beastie2Cycle;
int beastieOffset;
int sleepCycles;
int scrPos;  /* current position in the script table */
IntroObjectState *objectStateTable;

/* menus */
Menu mainOptions;
Menu videoOptions;
Menu soundOptions;
Menu gameplayOptions;
Menu advancedOptions;
Menu keyboardOptions;
Menu speedOptions;
Menu minorOptions;
Menu majorOptions;

/* temporary place-holder for settings changes */
Settings *settingsChanged;

void introInitiateNewGame(void);
void introDrawMap(void);
void introDrawMapAnimated(void);
void introDrawBeasties(void);
void introStartQuestions(void);
int introHandleName(const char *message);
int introHandleSexChoice(int choice);
void introJourneyOnward(void);
void introShowText(const char *text);
void introInitQuestionTree(void);
const char *introGetQuestion(int v1, int v2);
int introDoQuestion(int answer);
int introHandleQuestionChoice(int choice);
void introInitPlayers(SaveGame *saveGame);

void introMainOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introVideoOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introSoundOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introGameplayOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introAdvancedOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introKeyboardOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introSpeedOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introMinorOptionsMenuItemActivate(Menu menu, ActivateAction action);
void introMajorOptionsMenuItemActivate(Menu menu, ActivateAction action);

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
int introInit() {
    unsigned char screenFixData[533];
    U4FILE *title;
    int i, j;

    mode = INTRO_MAP;
    introAskToggle = 0;
    beastie1Cycle = 0;
    beastie2Cycle = 0;
    beastieOffset = -32;
    introErrorMessage = NULL;

    /* setup our temporary settings structure to hold changes made */
    settingsChanged = (Settings *)malloc(sizeof(Settings));

    title = u4fopen("title.exe");
    if (!title)
        return 0;

    introQuestions = u4read_stringtable(title, INTRO_TEXT_OFFSET, 28);
    introText = u4read_stringtable(title, -1, 24);
    introGypsy = u4read_stringtable(title, -1, 15);

    /* clean up stray newlines at end of strings */
    for (i = 0; i < 15; i++) {
        while (isspace(introGypsy[i][strlen(introGypsy[i]) - 1]))
            introGypsy[i][strlen(introGypsy[i]) - 1] = '\0';
    }


    u4fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    u4fread(screenFixData, 1, sizeof(screenFixData), title);

    u4fseek(title, INTRO_MAP_OFFSET, SEEK_SET);
    introMap[0] = (unsigned char *) malloc(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT);
    for (i = 0; i < INTRO_MAP_HEIGHT; i++) {
        introMap[i] = introMap[0] + INTRO_MAP_WIDTH * i;
        for (j = 0; j < INTRO_MAP_WIDTH; j++) {
            introMap[i][j] = (unsigned char) u4fgetc(title);
        }
    }

    sleepCycles = 0;
    scrPos = 0;

    u4fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = (unsigned char *) malloc(INTRO_SCRIPT_TABLE_SIZE);
    for (i = 0; i < INTRO_SCRIPT_TABLE_SIZE; i++)
        scriptTable[i] = u4fgetc(title);

    u4fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = (unsigned char *) malloc(INTRO_BASETILE_TABLE_SIZE);
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++)
        baseTileTable[i] = u4fgetc(title);

    objectStateTable = (IntroObjectState *) malloc(sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);
    memset(objectStateTable, 0, sizeof(IntroObjectState) * INTRO_BASETILE_TABLE_SIZE);

    /* --------------------------
       load beastie frame table 1
       -------------------------- */
    beastie1FrameTable = (unsigned char *) malloc(sizeof(unsigned char) * BEASTIE1_FRAMES);
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
    beastie2FrameTable = (unsigned char *) malloc(sizeof(unsigned char) * BEASTIE2_FRAMES);
    if (!beastie2FrameTable) {
        u4fclose(title);
        return(0);
    }
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE2_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE2_FRAMES; i++) {
        beastie2FrameTable[i] = u4fgetc(title);
    }

    u4fclose(title);

    screenFixIntroScreen(BKGD_INTRO, screenFixData);

    /* redraw some of the intro screen to allow for additional options */
    screenFixIntroScreen(BKGD_INTRO_EXTENDED, screenFixData);    
    screenFixIntroScreenExtended(BKGD_INTRO_EXTENDED);

    /* load our menus */
    
    mainOptions = menuAddItem(mainOptions, 0, "Video Options", 13, 16, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
    mainOptions = menuAddItem(mainOptions, 1, "Sound Options", 13, 17, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
    mainOptions = menuAddItem(mainOptions, 2, "Gameplay Options", 13, 18, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
    mainOptions = menuAddItem(mainOptions, 0xFF, "Main Menu", 13, 21, &introMainOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
    videoOptions = menuAddItem(videoOptions, 4, "Graphics", 6, 5, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
    videoOptions = menuAddItem(videoOptions, 0, "Scale", 6, 6, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
    videoOptions = menuAddItem(videoOptions, 1, "Mode", 6, 7, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
    videoOptions = menuAddItem(videoOptions, 2, "Filter", 6, 8, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
    videoOptions = menuAddItem(videoOptions, 3, "Screen Shaking", 6, 9, &introVideoOptionsMenuItemActivate, ACTIVATE_ANY);
    videoOptions = menuAddItem(videoOptions, 0xFE, "Use These Settings", 6, 20, &introVideoOptionsMenuItemActivate, ACTIVATE_NORMAL);
    videoOptions = menuAddItem(videoOptions, 0xFF, "Cancel", 6, 21, &introVideoOptionsMenuItemActivate, ACTIVATE_NORMAL);

    soundOptions = menuAddItem(soundOptions, 0, "Volume", 11, 16, &introSoundOptionsMenuItemActivate, ACTIVATE_ANY);
    soundOptions = menuAddItem(soundOptions, 1, "Fading", 11, 17, &introSoundOptionsMenuItemActivate, ACTIVATE_ANY);
    soundOptions = menuAddItem(soundOptions, 0xFE, "Use These Settings", 11, 20, &introSoundOptionsMenuItemActivate, ACTIVATE_NORMAL);
    soundOptions = menuAddItem(soundOptions, 0xFF, "Cancel", 11, 21, &introSoundOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
    gameplayOptions = menuAddItem(gameplayOptions, 0, "Minor Game Enhancements", 6, 5, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);
    gameplayOptions = menuAddItem(gameplayOptions, 3, "Major Game Enhancements", 6, 6, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);    
    gameplayOptions = menuAddItem(gameplayOptions, 1, "Automatic Actions", 6, 8, &introGameplayOptionsMenuItemActivate, ACTIVATE_ANY);    
    gameplayOptions = menuAddItem(gameplayOptions, 2, "\010 Advanced Options", 6, 18, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
    gameplayOptions = menuAddItem(gameplayOptions, 0xFE, "Use These Settings", 6, 20, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
    gameplayOptions = menuAddItem(gameplayOptions, 0xFF, "Cancel", 6, 21, &introGameplayOptionsMenuItemActivate, ACTIVATE_NORMAL);
    
    advancedOptions = menuAddItem(advancedOptions, 4, "\010 Speed Settings", 4, 5, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    advancedOptions = menuAddItem(advancedOptions, 3, "\010 Keyboard Settings", 4, 6, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    advancedOptions = menuAddItem(advancedOptions, 2, "Debug Mode (Cheats)", 4, 8, &introAdvancedOptionsMenuItemActivate, ACTIVATE_ANY);        
    advancedOptions = menuAddItem(advancedOptions, 0, "\010 Minor Enhancement Options", 4, 17, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    advancedOptions = menuAddItem(advancedOptions, 1, "\010 Major Enhancement Options", 4, 18, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);    
    advancedOptions = menuAddItem(advancedOptions, 0xFE, "Use These Settings", 4, 20, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    advancedOptions = menuAddItem(advancedOptions, 0xFF, "Cancel", 4, 21, &introAdvancedOptionsMenuItemActivate, ACTIVATE_NORMAL);

    keyboardOptions = menuAddItem(keyboardOptions, 0, "German Keyboard", 5, 5, &introKeyboardOptionsMenuItemActivate, ACTIVATE_ANY);
    keyboardOptions = menuAddItem(keyboardOptions, 1, "Repeat Delay (in msecs)", 5, 6, &introKeyboardOptionsMenuItemActivate, ACTIVATE_ANY);
    keyboardOptions = menuAddItem(keyboardOptions, 2, "Repeat Interval (in msecs)", 5, 7, &introKeyboardOptionsMenuItemActivate, ACTIVATE_ANY);
    keyboardOptions = menuAddItem(keyboardOptions, 0xFE, "Use These Settings", 5, 20, &introKeyboardOptionsMenuItemActivate, ACTIVATE_NORMAL);
    keyboardOptions = menuAddItem(keyboardOptions, 0xFF, "Cancel", 5, 21, &introKeyboardOptionsMenuItemActivate, ACTIVATE_NORMAL);

    speedOptions = menuAddItem(speedOptions, 0, "Game Cycles Per Second", 4, 5, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 1, "Battle Speed", 4, 6, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 2, "Spell Effect Length", 4, 7, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 3, "Camping length", 4, 8, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 4, "Inn rest length", 4, 9, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 5, "Shrine Meditation length", 4, 10, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 6, "Screen Shake Interval", 4, 11, &introSpeedOptionsMenuItemActivate, ACTIVATE_ANY);
    speedOptions = menuAddItem(speedOptions, 0xFE, "Use These Settings", 4, 20, &introSpeedOptionsMenuItemActivate, ACTIVATE_NORMAL);
    speedOptions = menuAddItem(speedOptions, 0xFF, "Cancel", 4, 21, &introSpeedOptionsMenuItemActivate, ACTIVATE_NORMAL);

    minorOptions = menuAddItem(minorOptions, 4, "Ultima V Spell Mixing", 7, 5, &introMinorOptionsMenuItemActivate, ACTIVATE_ANY);    
    minorOptions = menuAddItem(minorOptions, 0, "Ultima V Shrines", 7, 6, &introMinorOptionsMenuItemActivate, ACTIVATE_ANY);    
    minorOptions = menuAddItem(minorOptions, 1, "Slime Divides", 7, 7, &introMinorOptionsMenuItemActivate, ACTIVATE_ANY);
    minorOptions = menuAddItem(minorOptions, 2, "Fixed Chest Traps", 7, 8, &introMinorOptionsMenuItemActivate, ACTIVATE_ANY);
    minorOptions = menuAddItem(minorOptions, 0xFE, "Use These Settings", 7, 20, &introMinorOptionsMenuItemActivate, ACTIVATE_NORMAL);
    minorOptions = menuAddItem(minorOptions, 0xFF, "Cancel", 7, 21, &introMinorOptionsMenuItemActivate, ACTIVATE_NORMAL);

    //majorOptions = menuAddItem(majorOptions, 0, "Ultima V Combat", 7, 5, &introMajorOptionsMenuItemActivate, ACTIVATE_ANY);
    majorOptions = menuAddItem(majorOptions, 0xFE, "Use These Settings", 7, 20, &introMajorOptionsMenuItemActivate, ACTIVATE_NORMAL);
    majorOptions = menuAddItem(majorOptions, 0xFF, "Cancel", 7, 21, &introMajorOptionsMenuItemActivate, ACTIVATE_NORMAL);

    memcpy(settingsChanged, settings, sizeof(Settings));

    introUpdateScreen();

    musicIntro();

    return 1;
}

/**
 * Frees up data not needed after introduction.
 */
void introDelete() {
    int i;

    free(settingsChanged);

    for (i = 0; i < 28; i++)
        free(introQuestions[i]);
    free(introQuestions);
    introQuestions = NULL;
    for (i = 0; i < 24; i++)
        free(introText[i]);
    free(introText);
    introText = NULL;
    for (i = 0; i < 15; i++)
        free(introGypsy[i]);
    free(introGypsy);
    introGypsy = NULL;

    free(introMap[0]);

    free(scriptTable);
    scriptTable = NULL;
    free(baseTileTable);
    baseTileTable = NULL;
    free(objectStateTable);
    objectStateTable = NULL;

    free(beastie1FrameTable);
    free(beastie2FrameTable);

    screenFreeIntroBackgrounds();

    /* delete our menus */
    menuDelete(&mainOptions);
    menuDelete(&videoOptions);
    menuDelete(&soundOptions);
    menuDelete(&gameplayOptions);
    menuDelete(&advancedOptions);
    menuDelete(&keyboardOptions);
    menuDelete(&speedOptions);
    menuDelete(&minorOptions);
    menuDelete(&majorOptions);
}

/**
 * Handles keystrokes during the introduction.
 */
int introKeyHandler(int key, void *data) {
    int valid = 1;
    GetChoiceActionInfo *info;

    switch (mode) {

    case INTRO_MAP:
    case INTRO_ABOUT:
        mode = INTRO_MENU;
        introUpdateScreen();
        return 1;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            introErrorMessage = NULL;
            introInitiateNewGame();
            break;
        case 'j':
            introJourneyOnward();
            break;
        case 'r':
            introErrorMessage = NULL;
            mode = INTRO_MAP;
            introUpdateScreen();
            break;
        case 'c':
            introErrorMessage = NULL;
            mode = INTRO_CONFIG;
            mainOptions = menuReset(mainOptions);            
            memcpy(settingsChanged, settings, sizeof(Settings));
            introUpdateScreen();
            break;
        case 'a':
            introErrorMessage = NULL;
            mode = INTRO_ABOUT;
            introUpdateScreen();
            break;
        case 'q':
            eventHandlerSetExitFlag(1);
            quit = 1;
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
            valid = 0;
            break;
        }
        break;

    case INTRO_CONFIG:

        if (!introBaseMenuKeyHandler(key, &mainOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'v': mainOptions = menuActivateItem(mainOptions, 0, ACTIVATE_NORMAL); break;
            case 's': mainOptions = menuActivateItem(mainOptions, 1, ACTIVATE_NORMAL); break;                
            case 'g': mainOptions = menuActivateItem(mainOptions, 2, ACTIVATE_NORMAL); break;                
            case 'm': mainOptions = menuActivateItem(mainOptions, 0xFF, ACTIVATE_NORMAL); break;                
            default: break;
            }
        }

        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_VIDEO:
        
        if (!introBaseMenuKeyHandler(key, &videoOptions)) {            
            /* navigate to the item and activate it! */
            switch (key) {
            case 'g': videoOptions = menuActivateItem(videoOptions, 4, ACTIVATE_NORMAL); break;
            case 's': videoOptions = menuActivateItem(videoOptions, 0, ACTIVATE_NORMAL); break;
            case 'm': videoOptions = menuActivateItem(videoOptions, 1, ACTIVATE_NORMAL); break;
            case 'f': videoOptions = menuActivateItem(videoOptions, 2, ACTIVATE_NORMAL); break;
            case 'k': videoOptions = menuActivateItem(videoOptions, 3, ACTIVATE_NORMAL); break;
            default: break;
            }
        }
        
        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_SOUND:
        
        if (!introBaseMenuKeyHandler(key, &soundOptions)) {
            /* navigate to the item and activate it! */
            switch (key) {
            case 'v': soundOptions = menuActivateItem(soundOptions, 0, ACTIVATE_NORMAL); break;
            default: break;
            }
        }
        
        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_GAMEPLAY:
        
        if (!introBaseMenuKeyHandler(key, &gameplayOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'g': gameplayOptions = menuActivateItem(gameplayOptions, 0, ACTIVATE_NORMAL); break;                
            case 'm': gameplayOptions = menuActivateItem(gameplayOptions, 1, ACTIVATE_NORMAL); break;                
            case 'a': gameplayOptions = menuActivateItem(gameplayOptions, 2, ACTIVATE_NORMAL); break;
            default: break;
            }            
        }

        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_ADVANCED:
        if (!introBaseMenuKeyHandler(key, &advancedOptions)) {
            /* navigate to the item and activate it! */
            switch(key) {
            case 'd': advancedOptions = menuActivateItem(advancedOptions, 2, ACTIVATE_NORMAL); break;            
            case 'k': advancedOptions = menuActivateItem(advancedOptions, 3, ACTIVATE_NORMAL); break;
            case 's': advancedOptions = menuActivateItem(advancedOptions, 4, ACTIVATE_NORMAL); break;
            default: break;
            }
        }
        
        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_KEYBOARD:
        introBaseMenuKeyHandler(key, &keyboardOptions);
        
        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_SPEED:
        introBaseMenuKeyHandler(key, &speedOptions);
        
        introUpdateScreen();
        return 1;
        
    case INTRO_CONFIG_MINOR_OPTIONS:
        introBaseMenuKeyHandler(key, &minorOptions);

        introUpdateScreen();
        return 1;

    case INTRO_CONFIG_MAJOR_OPTIONS:
        introBaseMenuKeyHandler(key, &majorOptions);

        introUpdateScreen();
        return 1;

    case INTRO_INIT_NAME:
    case INTRO_INIT_SEX:
        assert(0);              /* shouldn't happen */
        return 1;

    case INTRO_INIT_STORY:
        storyInd++;
        if (storyInd >= 24)
            introStartQuestions();
        introUpdateScreen();
        return 1;

    case INTRO_INIT_QUESTIONS:
        introAskToggle = 1;
        info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        info->choices = "ab";
        info->handleChoice = &introHandleQuestionChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);
        introUpdateScreen();
        return 1;

    case INTRO_INIT_SEGTOGAME:
        segueInd++;
        if (segueInd >= 2) {
            mode = INTRO_DONE;
            eventHandlerSetExitFlag(1);
        }
        else
            introUpdateScreen();
        return 1;

    case INTRO_DONE:
        return 0;
    }

    return valid || keyHandlerDefault(key, NULL);
}

/**
 * Draws the small map on the intro screen.
 */
void introDrawMap() {
    if (sleepCycles > 0) {
        introDrawMapAnimated();
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
                introDrawMapAnimated();

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

void introDrawMapAnimated() {
    int x, y, i;

    /* draw unmodified map */
    for (y = 0; y < INTRO_MAP_HEIGHT; y++) {
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            screenShowTile(introMap[y][x], 0, x, y + 6);
    }

    /* draw animated objects */
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0)
            screenShowTile(objectStateTable[i].tile, 0, objectStateTable[i].x, objectStateTable[i].y + 6);
    }
}

/**
 * Draws the animated beasts in the upper corners of the screen.
 */
void introDrawBeasties() {
    screenShowBeastie(0, beastieOffset, beastie1FrameTable[beastie1Cycle]);
    screenShowBeastie(1, beastieOffset, beastie2FrameTable[beastie2Cycle]);
    if (beastieOffset < 0)
        beastieOffset++;
}

/**
 * Paints the screen.
 */
void introUpdateScreen() {

    screenHideCursor();

    switch (mode) {
    case INTRO_MAP:
        screenDrawBackground(BKGD_INTRO);
        introDrawMap();
        introDrawBeasties();
        break;

    case INTRO_MENU:        
        screenSetCursorPos(24, 16);
        screenShowCursor();

        screenDrawBackground(BKGD_INTRO);
        screenTextAt(2, 14, "In another world, in a time to come.");
        screenTextAt(15, 16, "Options:");
        screenTextAt(11, 17, "Return to the view");
        screenTextAt(11, 18, "Journey Onward");
        screenTextAt(11, 19, "Initiate New Game");
        screenTextAt(11, 20, "Configure");
        screenTextAt(11, 21, "About");
        if (introErrorMessage)
            screenTextAt(11, 22, introErrorMessage);
        introDrawBeasties();
        break;

    case INTRO_CONFIG:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(9, 14, "-- xu4 Configuration --");
        menuShow(menuGetRoot(mainOptions));
        introDrawBeasties();
        break;

    case INTRO_CONFIG_VIDEO:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Video Options:");
        screenTextAt(24, 5, "%s", settingsVideoTypeToString(settingsChanged->videoType));
        screenTextAt(24, 6, "x%d", settingsChanged->scale);
        screenTextAt(24, 7, "%s", settingsChanged->fullscreen ? "Fullscreen" : "Window");
        screenTextAt(24, 8, "%s", settingsFilterToString(settingsChanged->filter));
        screenTextAt(24, 9, "%s", settingsChanged->screenShakes ? "On" : "Off");
        menuShow(menuGetRoot(videoOptions));        
        break;

    case INTRO_CONFIG_SOUND:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(2, 14, "Sound Options:");
        screenTextAt(24, 16, "%s", settingsChanged->vol ? "On" : "Off");        
        screenTextAt(24, 17, "%s", settingsChanged->volumeFades ? "On" : "Off");        
        menuShow(menuGetRoot(soundOptions));
        introDrawBeasties();
        break;

    case INTRO_CONFIG_GAMEPLAY:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Gameplay Options:");
        screenTextAt(32, 5, "%s", settingsChanged->minorEnhancements ? "On" : "Off");
        screenTextAt(32, 6, "%s", settingsChanged->majorEnhancements ? "On" : "Off");
        screenTextAt(6, 9, "  (Open, Jimmy, etc.)     %s", settingsChanged->shortcutCommands ? "On" : "Off");        
        menuShow(menuGetRoot(gameplayOptions));
        break;

    case INTRO_CONFIG_ADVANCED:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3,   "Advanced Options:");
        screenTextAt(34, 8,  "%s", settingsChanged->debug ? "On" : "Off");        
        menuShow(menuGetRoot(advancedOptions));
        break;

    case INTRO_CONFIG_KEYBOARD:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3, "Keyboard Settings:");
        screenTextAt(34, 5, "%s", settingsChanged->germanKbd ? "Yes" : "No"); 
        screenTextAt(34, 6,  "%d", settingsChanged->keydelay);
        screenTextAt(34, 7,  "%d", settingsChanged->keyinterval);
        menuShow(menuGetRoot(keyboardOptions));        
        break;

    case INTRO_CONFIG_SPEED:
        {
            char msg[16] = {0};
            screenDrawBackground(BKGD_INTRO_EXTENDED);
            screenTextAt(2, 3, "Speed Settings:");           

            sprintf(msg, "%d", settingsChanged->gameCyclesPerSecond);
            screenTextAt(33 - strlen(msg), 5, msg);

            sprintf(msg, "%d", settingsChanged->battleSpeed);
            screenTextAt(33 - strlen(msg), 6, msg);
            
            sprintf(msg, "%0.*f sec",
                (settingsChanged->spellEffectSpeed % 5 == 0) ? 0 : 1,
                (double)settingsChanged->spellEffectSpeed / 5);        
            screenTextAt(37 - strlen(msg), 7, msg);

            sprintf(msg, "%d sec", settingsChanged->campTime);
            screenTextAt(37 - strlen(msg), 8, msg);

            sprintf(msg, "%d sec", settingsChanged->innTime);
            screenTextAt(37 - strlen(msg), 9, msg);

            sprintf(msg, "%d sec", settingsChanged->shrineTime);
            screenTextAt(37 - strlen(msg), 10, msg);

            sprintf(msg, "%d msec", settingsChanged->shakeInterval);
            screenTextAt(38 - strlen(msg), 11, msg);

            menuShow(menuGetRoot(speedOptions));
        }
        break;

    case INTRO_CONFIG_MINOR_OPTIONS:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3,   "Minor Game Enhancement Options:");        
        screenTextAt(31, 5,  "%s", settingsChanged->minorEnhancementsOptions.u5spellMixing ? "On" : "Off");
        screenTextAt(31, 6,  "%s", settingsChanged->minorEnhancementsOptions.u5shrines ? "On" : "Off");
        screenTextAt(31, 7,  "%s", settingsChanged->minorEnhancementsOptions.slimeDivides ? "On" : "Off");
        screenTextAt(31, 8,  "%s", settingsChanged->minorEnhancementsOptions.c64chestTraps ? "On" : "Off");
        menuShow(menuGetRoot(minorOptions));
        break;

    case INTRO_CONFIG_MAJOR_OPTIONS:
        screenDrawBackground(BKGD_INTRO_EXTENDED);
        screenTextAt(2, 3,   "Major Game Enhancement Options:");
        //screenTextAt(31, 5,  "%s", settingsChanged->majorEnhancementsOptions.u5combat ? "On" : "Off");
        menuShow(menuGetRoot(majorOptions));
        break;

    case INTRO_ABOUT:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(15, 14, "XU4 %s", VERSION);
        screenTextAt(2, 16, "xu4 is free software; you can redist-");
        screenTextAt(2, 17, "ribute it and/or modify it under the");
        screenTextAt(2, 18, "terms of the GNU GPL as published by");
        screenTextAt(2, 19, "the FSF.  See COPYING.");
        screenTextAt(2, 21, "\011 Copyright 2002-2003 xu4 team");
        screenTextAt(2, 22, "\011 Copyright 1987 Lord British");
        introDrawBeasties();
        break;

    case INTRO_INIT_NAME:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "By what name shalt thou be known");
        screenTextAt(4, 17, "in this world and time?");
        introDrawBeasties();
        break;

    case INTRO_INIT_SEX:
        screenDrawBackground(BKGD_INTRO);
        screenTextAt(4, 16, "Art thou Male or Female?");
        introDrawBeasties();
        break;

    case INTRO_INIT_STORY:
        if (storyInd == 0)
            screenDrawBackground(BKGD_TREE);
        else if (storyInd == 3)
            screenAnimateIntro(0);
        else if (storyInd == 5)
            screenAnimateIntro(1);
        else if (storyInd == 6)
            screenDrawBackground(BKGD_PORTAL);
        else if (storyInd == 11)
            screenDrawBackground(BKGD_TREE);
        else if (storyInd == 15)
            screenDrawBackground(BKGD_OUTSIDE);
        else if (storyInd == 17)
            screenDrawBackground(BKGD_INSIDE);
        else if (storyInd == 20)
            screenDrawBackground(BKGD_WAGON);
        else if (storyInd == 21)
            screenDrawBackground(BKGD_GYPSY);
        else if (storyInd == 23)
            screenDrawBackground(BKGD_ABACUS);
        introShowText(introText[storyInd]);
        break;

    case INTRO_INIT_QUESTIONS:
        if (introAskToggle == 0) {
            if (questionRound == 0)
                screenDrawBackground(BKGD_ABACUS);
            screenShowCard(0, questionTree[questionRound * 2]);
            screenShowCard(1, questionTree[questionRound * 2 + 1]);

            screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);
        
            screenTextAt(0, 19, "%s", introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)]);
            screenTextAt(0, 20, "%s", introGypsy[GYP_UPON_TABLE]);
            screenTextAt(0, 21, "%s and %s.  She says", introGypsy[questionTree[questionRound * 2] + 4], introGypsy[questionTree[questionRound * 2 + 1] + 4]);
            screenTextAt(0, 22, "\"Consider this:\"");
            screenSetCursorPos(16, 22);
        } else
            introShowText(introGetQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));
        break;

    case INTRO_INIT_SEGTOGAME:
        introShowText(introGypsy[GYP_SEGUE1 + segueInd]);
        break;

    case INTRO_DONE:
        break;

    default:
        assert(0);
    }

    screenUpdateCursor();
    screenRedrawScreen();
}

/**
 * Initiate a new savegame by reading the name, sex, then presenting a
 * series of questions to determine the class of the new character.
 */
void introInitiateNewGame() {
    ReadBufferActionInfo *info;

    /* display name  prompt and read name from keyboard */
    mode = INTRO_INIT_NAME;
    introUpdateScreen();    
    screenSetCursorPos(12, 20);
    screenShowCursor();
    screenRedrawScreen();

    info = (ReadBufferActionInfo *) malloc(sizeof(ReadBufferActionInfo));
    info->handleBuffer = &introHandleName;
    info->buffer = nameBuffer;
    info->bufferLen = 12;
    info->screenX = 12;
    info->screenY = 20;
    nameBuffer[0] = '\0';

    eventHandlerPushKeyHandlerData(&keyHandlerReadBuffer, info);
}

/**
 * Starts the gypsys questioning that eventually determines the new
 * characters class.
 */
void introStartQuestions() {
    mode = INTRO_INIT_QUESTIONS;
    questionRound = 0;
    introAskToggle = 0;
    introInitQuestionTree();
}

/**
 * Callback to receive the read character name.
 */
int introHandleName(const char *message) {
    GetChoiceActionInfo *info;

    eventHandlerPopKeyHandler();

    if (message[0] == '\0') {
        mode = INTRO_MENU;

        introUpdateScreen();
    }

    else {
        mode = INTRO_INIT_SEX;

        introUpdateScreen();
        screenSetCursorPos(29, 16);
        screenShowCursor();

        info = (GetChoiceActionInfo *) malloc(sizeof(GetChoiceActionInfo));
        info->choices = "mf";
        info->handleChoice = &introHandleSexChoice;
        eventHandlerPushKeyHandlerData(&keyHandlerGetChoice, info);
    }

    screenRedrawScreen();

    return 1;
}

/**
 * Callback to receive the character sex choice.
 */
int introHandleSexChoice(int choice) {

    if (choice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    eventHandlerPopKeyHandler();
    mode = INTRO_INIT_STORY;
    storyInd = 0;

    introUpdateScreen();
    screenRedrawScreen();

    return 1;
}

/**
 * Get the text for the question giving a choice between virtue v1 and
 * virtue v2 (zero based virtue index, starting at honesty).
 */
const char *introGetQuestion(int v1, int v2) {
    int i = 0;
    int d = 7;

    assert (v1 < v2);

    while (v1 > 0) {
        i += d;
        d--;
        v1--;
        v2--;
    }

    assert((i + v2 - 1) < 28);

    return introQuestions[i + v2 - 1];
}

/**
 * Starts the game.
 */
void introJourneyOnward() {
    FILE *saveGameFile;

    /*
     * ensure a party.sav file exists, otherwise require user to
     * initiate game
     */
    saveGameFile = saveGameOpenForReading();
    if (!saveGameFile) {
        introErrorMessage = "Initiate game first!";
        introUpdateScreen();
        screenRedrawScreen();
        return;
    }

    fclose(saveGameFile);
    eventHandlerSetExitFlag(1);
}

/**
 * Shows text in the lower six lines of the screen.
 */
void introShowText(const char *text) {
    char line[INTRO_TEXT_WIDTH + 1];
    const char *p;
    int len, lineNo = INTRO_TEXT_Y;

    screenEraseTextArea(INTRO_TEXT_X, INTRO_TEXT_Y, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT);

    p = text;
    while (*p) {
        len = strcspn(p, "\n");
        strncpy(line, p, len);
        line[len] = '\0';
        p += len;
        if (*p == '\n')
            p++;
        screenTextAt(0, lineNo, "%s", line);
        lineNo++;
    }
    screenSetCursorPos(strlen(line), lineNo - 1);
}

/**
 * Timer callback for the intro sequence.  Handles animating the intro
 * map, the beasties, etc..
 */
void introTimer(void *data) {
    screenCycle();
    screenUpdateCursor();
    if (mode == INTRO_MAP)
        introDrawMap();
    if (mode == INTRO_MAP || mode == INTRO_MENU || mode == INTRO_CONFIG ||
        mode == INTRO_ABOUT || mode == INTRO_CONFIG_SOUND ||        
        mode == INTRO_INIT_NAME || mode == INTRO_INIT_SEX)
        introDrawBeasties();

    /* 
     * refresh the screen only if the timer queue is empty --
     * i.e. drop a frame if another timer event is about to be fired
     */
    if (eventHandlerTimerQueueEmpty())
        screenRedrawScreen();

    if ((rand() % 2) && ++beastie1Cycle >= BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if ((rand() % 2) && ++beastie2Cycle >= BEASTIE2_FRAMES)
        beastie2Cycle = 0;
}

/**
 * Initializes the question tree.  The tree starts off with the first
 * eight entries set to the numbers 0-7 in a random order.
 */
void introInitQuestionTree() {
    int i, tmp, r;

    for (i = 0; i < 8; i++)
        questionTree[i] = i;

    for (i = 0; i < 8; i++) {
        r = rand() % 8;
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
int introDoQuestion(int answer) {
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
 * Callback to receive question answer choice.
 */
int introHandleQuestionChoice(int choice) {
    FILE *saveGameFile;
    SaveGame saveGame;

    eventHandlerPopKeyHandler();

    if (introDoQuestion(choice == 'a' ? 0 : 1)) {
        mode = INTRO_INIT_SEGTOGAME;
        segueInd = 0;

        saveGameFile = saveGameOpenForWriting();
        if (saveGameFile) {
            SaveGamePlayerRecord avatar;
            saveGamePlayerRecordInit(&avatar);
            saveGameInit(&saveGame, &avatar);
            screenHideCursor();
            introInitPlayers(&saveGame);
            saveGame.food = 30000;
            saveGame.gold = 200;
            saveGame.reagents[REAG_GINSENG] = 3;
            saveGame.reagents[REAG_GARLIC] = 4;
            saveGame.torches = 2;
            saveGameWrite(&saveGame, saveGameFile);
            fclose(saveGameFile);
        }
        saveGameFile = saveGameMonstersOpenForWriting();
        if (saveGameFile) {
            saveGameMonstersWrite(NULL, saveGameFile);
            fclose(saveGameFile);
        }
    } else {
        introAskToggle = 0;
    }

    introUpdateScreen();

    return 1;
}

/**
 * Build the initial avatar player record from the answers to the
 * gypsy's questions.
 */
void introInitPlayers(SaveGame *saveGame) {
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

    strcpy(saveGame->players[0].name, nameBuffer);
    saveGame->players[0].sex = sex;
    saveGame->players[0].klass = (ClassType) questionTree[14];

    assert(saveGame->players[0].klass < 8);

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

    saveGame->players[0].hp = saveGame->players[0].hpMax = playerGetMaxLevel(&saveGame->players[0]) * 100;
    saveGame->players[0].mp = playerGetMaxMp(&saveGame->players[0]);

    p = 1;
    for (i = 0; i < VIRT_MAX; i++) {
        /* Setup the initial virtue levels according to the avatar's class */
        saveGame->karma[i] = initValuesForClass[saveGame->players[0].klass].virtues[i];

        /* Initial setup for party members that aren't in your group yet... */
        if (i != saveGame->players[0].klass) {
            saveGame->players[p].klass = (ClassType) i;
            saveGame->players[p].xp = initValuesForClass[i].xp;
            saveGame->players[p].str = initValuesForNpcClass[i].str;
            saveGame->players[p].dex = initValuesForNpcClass[i].dex;
            saveGame->players[p].intel = initValuesForNpcClass[i].intel;
            saveGame->players[p].weapon = initValuesForClass[i].weapon;
            saveGame->players[p].armor = initValuesForClass[i].armor;
            strcpy(saveGame->players[p].name, initValuesForNpcClass[i].name);
            saveGame->players[p].sex = initValuesForNpcClass[i].sex;
            saveGame->players[p].hp = saveGame->players[p].hpMax = playerGetMaxLevel(&saveGame->players[p]) * 100;
            saveGame->players[p].mp = playerGetMaxMp(&saveGame->players[p]);
            p++;
        }
    }

}

/**
 * The base key handler for the configuration menus
 */
int introBaseMenuKeyHandler(int key, void *data) {
    Menu *menu = (Menu *)data;
    char cancelKey = (mode == INTRO_CONFIG) ? 'm' : 'c';
    char saveKey = (mode == INTRO_CONFIG) ? '\0' : 'u';

    if (key == cancelKey)
        return introBaseMenuKeyHandler(' ', menu);
    else if (key == saveKey)
        return introBaseMenuKeyHandler(0, menu);
    
    switch(key) {
    case U4_UP:
        *menu = menuHighlightNew(*menu, menuGetPreviousItem(*menu));
        break;
    case U4_DOWN:
        *menu = menuHighlightNew(*menu, menuGetNextItem(*menu));
        break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_ENTER:
        {
            MenuItem *menuItem = (MenuItem *)(*menu)->data;
            ActivateAction action = ACTIVATE_NORMAL;
            
            if (menuItem->activateMenuItem) {
                if (key == U4_LEFT)
                    action = ACTIVATE_DECREMENT;
                else if (key == U4_RIGHT)
                    action = ACTIVATE_INCREMENT;
                *menu = menuActivateItem(*menu, -1, action);
            }
        }
        break;
    case ' ':    
        /* activate the 'cancel' menu item */
        menuActivateItem(*menu, 0xFF, ACTIVATE_NORMAL);
        break;    
    case 0:
        /* activate the 'save' menu item */
        menuActivateItem(*menu, 0xFE, ACTIVATE_NORMAL);        
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
void introMainOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;

    if (action != ACTIVATE_NORMAL)
        return;

    switch(menuItem->id) {
    case 0: mode = INTRO_CONFIG_VIDEO; videoOptions = menuReset(videoOptions);break;        
    case 1: mode = INTRO_CONFIG_SOUND; soundOptions = menuReset(soundOptions);break;
    case 2: mode = INTRO_CONFIG_GAMEPLAY; gameplayOptions = menuReset(gameplayOptions); break;
    case 0xFF: mode = INTRO_MENU; break;
    default: break;
    }
}

/* video options menu handler */
void introVideoOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->scale++;
            if (settingsChanged->scale > 5)
                settingsChanged->scale = 1;
        } else {
            settingsChanged->scale--;
            if (settingsChanged->scale <= 0)
                settingsChanged->scale = 5;
        }
        break;
        
    case 1:
        settingsChanged->fullscreen = settingsChanged->fullscreen ? 0 : 1;
        break;

    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->filter++;
            if (settingsChanged->filter == SCL_MAX)
                settingsChanged->filter = (FilterType)(SCL_MIN+1);
        } else {
            settingsChanged->filter--;
            if (settingsChanged->filter == SCL_MIN)
                settingsChanged->filter = (FilterType)(SCL_MAX-1);
        }
        break;

    case 3:
        settingsChanged->screenShakes = settingsChanged->screenShakes ? 0 : 1;
        break;

    case 4:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->videoType++;
            if (settingsChanged->videoType == VIDEO_MAX)
                settingsChanged->videoType = (VideoType)(VIDEO_MIN+1);
        } else {
            settingsChanged->videoType--;
            if (settingsChanged->videoType == VIDEO_MIN)
                settingsChanged->videoType = (VideoType)(VIDEO_MAX-1);
        }
        break;

    case 0xFE:
        /* save settings (if necessary) */
        if (memcmp(settings, settingsChanged, sizeof(Settings)) != 0) {
            memcpy(settings, settingsChanged, sizeof(Settings));
            settingsWrite();

            /* FIXME: resize images, etc. */
            screenReInit();
        }        
    
        mode = INTRO_CONFIG;
        
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG;
        break;
        
    default: break;
    }
}

/* sound options menu handler */
void introSoundOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0: 
        settingsChanged->vol = settingsChanged->vol ? 0 : 1;
        break;
    case 1:
        settingsChanged->volumeFades = settingsChanged->volumeFades ? 0 : 1;
        break;
    case 0xFE:
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();
        
        musicIntro();
    
        mode = INTRO_CONFIG;        
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG;
        break;
    
    default: break;
    }
}

/* gameplay options menu handler */
void introGameplayOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0:
        settingsChanged->minorEnhancements = settingsChanged->minorEnhancements ? 0 : 1;        
        break;
    case 3:
        settingsChanged->majorEnhancements = settingsChanged->majorEnhancements ? 0 : 1;
        break;
    case 1:
        settingsChanged->shortcutCommands = settingsChanged->shortcutCommands ? 0 : 1;
        break;
    case 2:
        mode = INTRO_CONFIG_ADVANCED;        
        advancedOptions = menuReset(advancedOptions);        

        /* show or hide minor/major options if they are enabled/disabled */
        menuItemSetVisible(menuGetItemById(advancedOptions, 0), settingsChanged->minorEnhancements);
        menuItemSetVisible(menuGetItemById(advancedOptions, 1), settingsChanged->majorEnhancements);

        break;    
    case 0xFE:
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();        
    
        mode = INTRO_CONFIG;        
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG;
        break;
    default: break;
    }
}

/* advanced options menu handler */
void introAdvancedOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 2:
        settingsChanged->debug = settingsChanged->debug ? 0 : 1;
        break;
    case 0:
        mode = INTRO_CONFIG_MINOR_OPTIONS;
        minorOptions = menuReset(minorOptions);
        break;
    case 1:
        mode = INTRO_CONFIG_MAJOR_OPTIONS;
        majorOptions = menuReset(majorOptions);
        break;
    case 3:
        mode = INTRO_CONFIG_KEYBOARD;
        keyboardOptions = menuReset(keyboardOptions);
        break;
    case 4:
        mode = INTRO_CONFIG_SPEED;
        speedOptions = menuReset(speedOptions);
        break;    
    case 0xFE:
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();        
    
        mode = INTRO_CONFIG_GAMEPLAY;        
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG_GAMEPLAY;
        break;
    default: break;
    }
}

/* keyboard options menu handler */
void introKeyboardOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0:
        settingsChanged->germanKbd = settingsChanged->germanKbd ? 0 : 1;
        break;
    case 1:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->keydelay += 100;
            if (settingsChanged->keydelay > MAX_KEY_DELAY)
                settingsChanged->keydelay = 100;
        } else {
            settingsChanged->keydelay -= 100;
            if (settingsChanged->keydelay < 100)
                settingsChanged->keydelay = MAX_KEY_DELAY;
        }
        break;
    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->keyinterval += 10;
            if (settingsChanged->keyinterval > MAX_KEY_INTERVAL)
                settingsChanged->keyinterval = 10;
        } else {
            settingsChanged->keyinterval -= 10;
            if (settingsChanged->keyinterval < 10)
                settingsChanged->keyinterval = MAX_KEY_INTERVAL;
        }
        break;
    case 0xFE:
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();        

        /* re-initialize keyboard */
        eventKeyboardSetKeyRepeat(settingsChanged->keydelay, settingsChanged->keyinterval);
    
        mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG_ADVANCED;
        break;
    default: break;
    }    
}

/* speed options menu handler */
void introSpeedOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->gameCyclesPerSecond++;
            if (settingsChanged->gameCyclesPerSecond > MAX_CYCLES_PER_SECOND)
                settingsChanged->gameCyclesPerSecond = 1;
        } else {
            settingsChanged->gameCyclesPerSecond--;
            if (settingsChanged->gameCyclesPerSecond < 1)
                settingsChanged->gameCyclesPerSecond = MAX_CYCLES_PER_SECOND;
        }
        break;
    case 1:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->battleSpeed++;
            if (settingsChanged->battleSpeed > MAX_BATTLE_SPEED)
                settingsChanged->battleSpeed = 1;
        } else {
            settingsChanged->battleSpeed--;
            if (settingsChanged->battleSpeed < 1)
                settingsChanged->battleSpeed = MAX_BATTLE_SPEED;
        }
        break;
    case 2:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->spellEffectSpeed++;
            if (settingsChanged->spellEffectSpeed > MAX_SPELL_EFFECT_SPEED)
                settingsChanged->spellEffectSpeed = 1;
        } else {
            settingsChanged->spellEffectSpeed--;
            if (settingsChanged->spellEffectSpeed < 1)
                settingsChanged->spellEffectSpeed = MAX_SPELL_EFFECT_SPEED;
        }
        break;
    case 3:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->campTime++;
            if (settingsChanged->campTime > MAX_CAMP_TIME)
                settingsChanged->campTime = 1;
        } else {
            settingsChanged->campTime--;
            if (settingsChanged->campTime < 1)
                settingsChanged->campTime = MAX_CAMP_TIME;
        }
        break;
    case 4:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->innTime++;
            if (settingsChanged->innTime > MAX_INN_TIME)
                settingsChanged->innTime = 1;
        } else {
            settingsChanged->innTime--;
            if (settingsChanged->innTime < 1)
                settingsChanged->innTime = MAX_INN_TIME;
        }
        break;
    case 5:
        /* make sure that the setting we're trying for is even possible */
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->shrineTime++;
            if (settingsChanged->shrineTime > MAX_SHRINE_TIME)
                settingsChanged->shrineTime = MEDITATION_MANTRAS_PER_CYCLE / settingsChanged->gameCyclesPerSecond;
        } else {
            settingsChanged->shrineTime--;
            if (settingsChanged->shrineTime < (MEDITATION_MANTRAS_PER_CYCLE / settingsChanged->gameCyclesPerSecond))
                settingsChanged->shrineTime = MAX_SHRINE_TIME;
        }
        break;
    case 6:
        if (action != ACTIVATE_DECREMENT) {
            settingsChanged->shakeInterval += 10;
            if (settingsChanged->shakeInterval > MAX_SHAKE_INTERVAL)
                settingsChanged->shakeInterval = MIN_SHAKE_INTERVAL;
        } else {
            settingsChanged->shakeInterval -= 10;
            if (settingsChanged->shakeInterval < MIN_SHAKE_INTERVAL)
                settingsChanged->shakeInterval = MAX_SHAKE_INTERVAL;
        }
        break;

    case 0xFE:
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();        
    
        /* re-initialize events */
        eventHandlerResetTimerCallbacks();
        
        mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG_ADVANCED;
        break;
    default: break;
    }
}

/* minor enhancement options menu handler */
void introMinorOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0: 
        settingsChanged->minorEnhancementsOptions.u5shrines = settingsChanged->minorEnhancementsOptions.u5shrines ? 0 : 1;
        break;
    case 1: 
        settingsChanged->minorEnhancementsOptions.slimeDivides = settingsChanged->minorEnhancementsOptions.slimeDivides ? 0 : 1;
        break;
    case 2: 
        settingsChanged->minorEnhancementsOptions.c64chestTraps = settingsChanged->minorEnhancementsOptions.c64chestTraps ? 0 : 1;
        break;    
    case 4:
        settingsChanged->minorEnhancementsOptions.u5spellMixing = settingsChanged->minorEnhancementsOptions.u5spellMixing ? 0 : 1;
        break;
    case 0xFE:        
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();        
    
        mode = INTRO_CONFIG_ADVANCED;
        break;
    case 0xFF:        
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG_ADVANCED;
        break;
    
    default: break;
    }
}

/* major enhancement options menu handler */
void introMajorOptionsMenuItemActivate(Menu menu, ActivateAction action) {
    MenuItem *menuItem = (MenuItem *)menu->data;
    switch(menuItem->id) {
    case 0: 
        settingsChanged->majorEnhancementsOptions.u5combat = settingsChanged->majorEnhancementsOptions.u5combat ? 0 : 1;
        break;
    case 0xFE:        
        /* save settings */
        memcpy(settings, settingsChanged, sizeof(Settings));
        settingsWrite();
    
        mode = INTRO_CONFIG_ADVANCED;        
        break;
    case 0xFF:
        /* discard settings */
        memcpy(settingsChanged, settings, sizeof(Settings));
        mode = INTRO_CONFIG_ADVANCED;
        break;
    
    default: break;
    }
}
