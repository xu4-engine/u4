/*
 * $Id$
 */


#include <algorithm>
#include <cstring>
#include "u4.h"

#include "intro.h"

#include "debug.h"
#include "error.h"
#include "imagemgr.h"
#include "sound.h"
#include "player.h"
#include "screen.h"
#include "settings.h"
#include "tileset.h"
#include "u4file.h"
#include "utils.h"
#include "xu4.h"

#ifdef USE_GL
#ifdef GPU_RENDER
extern bool loadMapData(Map *map, U4FILE *uf, Symbol borderTile);
#endif
#include "gpu.h"
#endif

#ifdef IOS
#include "ios_helpers.h"
#endif

using namespace std;

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

#ifndef GPU_RENDER
class IntroObjectState {
public:
    IntroObjectState() : x(0), y(0), tile(0) {}
    int x, y;
    MapTile tile; /* base tile + tile frame */
};
#endif

/* temporary place-holder for settings changes */
SettingsData settingsChanged;

const int IntroBinData::INTRO_TEXT_OFFSET = 17445 - 1;  // (start at zero)
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

IntroBinData::IntroBinData() :
    sigData(NULL),
    scriptTable(NULL),
    baseTileTable(NULL),
    beastie1FrameTable(NULL),
    beastie2FrameTable(NULL) {
}

IntroBinData::~IntroBinData() {
    delete [] sigData;
    delete [] scriptTable;
    delete [] baseTileTable;
    delete [] beastie1FrameTable;
    delete [] beastie2FrameTable;

    introQuestions.clear();
    introText.clear();
    introGypsy.clear();
}

bool IntroBinData::load() {
    int i;
    const UltimaSaveIds* usaveIds = xu4.config->usaveIds();
    const Tileset* tileset = xu4.config->tileset();

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
#ifdef GPU_RENDER
    introMap.id     = 255;      // Unique Id required by screenUpdateMap.
    introMap.border_behavior = Map::BORDER_WRAP;
    introMap.width  = INTRO_MAP_WIDTH;
    introMap.height = INTRO_MAP_HEIGHT;
    introMap.flags  = NO_LINE_OF_SIGHT;
    introMap.tileset = xu4.config->tileset();
    loadMapData(&introMap, title, 0);
#else
    introMap.resize(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT, MapTile(0));
    for (i = 0; i < INTRO_MAP_HEIGHT * INTRO_MAP_WIDTH; i++)
        introMap[i] = usaveIds->moduleId(u4fgetc(title));
#endif

    u4fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = new unsigned char[INTRO_SCRIPT_TABLE_SIZE];
    u4fread(scriptTable, 1, INTRO_SCRIPT_TABLE_SIZE, title);

    u4fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = new const Tile*[INTRO_BASETILE_TABLE_SIZE];
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        MapTile tile = usaveIds->moduleId(u4fgetc(title));
        baseTileTable[i] = tileset->get(tile.id);
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
    menuArea(1 * CHAR_WIDTH, 13 * CHAR_HEIGHT, 38, 11),
    extendedMenuArea(2 * CHAR_WIDTH, 10 * CHAR_HEIGHT, 36, 13),
    questionArea(INTRO_TEXT_X * CHAR_WIDTH, INTRO_TEXT_Y * CHAR_HEIGHT, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT),
    mapArea(BORDER_WIDTH, (TILE_HEIGHT * 6) + BORDER_HEIGHT, INTRO_MAP_WIDTH, INTRO_MAP_HEIGHT),
    binData(NULL),
    titles(),                   // element list
    title(titles.begin()),      // element iterator
    bSkipTitles(false)
{
    // initialize menus
    confMenu.setTitle("XU4 Configuration:", 0, 0);
    confMenu.add(MI_CONF_VIDEO,               "\010 Video Options",              2,  2,/*'v'*/  2);
    confMenu.add(MI_CONF_SOUND,               "\010 Sound Options",              2,  3,/*'s'*/  2);
    confMenu.add(MI_CONF_INPUT,               "\010 Input Options",              2,  4,/*'i'*/  2);
    confMenu.add(MI_CONF_SPEED,               "\010 Speed Options",              2,  5,/*'p'*/  3);
    confMenu.add(MI_CONF_01, new BoolMenuItem("Game Enhancements         %s",    2,  7,/*'e'*/  5, &settingsChanged.enhancements));
    confMenu.add(MI_CONF_GAMEPLAY,            "\010 Enhanced Gameplay Options",  2,  9,/*'g'*/ 11);
    confMenu.add(MI_CONF_INTERFACE,           "\010 Enhanced Interface Options", 2, 10,/*'n'*/ 12);
    confMenu.add(CANCEL,                      "\017 Main Menu",                  2, 12,/*'m'*/  2);
    confMenu.addShortcutKey(CANCEL, ' ');
    confMenu.setClosesMenu(CANCEL);

    /* set the default visibility of the two enhancement menus */
    confMenu.getItemById(MI_CONF_GAMEPLAY)->setVisible(xu4.settings->enhancements);
    confMenu.getItemById(MI_CONF_INTERFACE)->setVisible(xu4.settings->enhancements);

    videoMenu.setTitle("Video Options:", 0, 0);
    videoMenu.add(MI_VIDEO_CONF_GFX,              "\010 Game Graphics Options",  2,  2,/*'g'*/  2);
    videoMenu.add(MI_VIDEO_04,    new IntMenuItem("Scale                x%d", 2,  4,/*'s'*/  0, reinterpret_cast<int *>(&settingsChanged.scale), 1, 5, 1));
    videoMenu.add(MI_VIDEO_05,  (new BoolMenuItem("Mode                 %s",  2,  5,/*'m'*/  0, &settingsChanged.fullscreen))->setValueStrings("Fullscreen", "Window"));
    videoMenu.add(MI_VIDEO_06,   new EnumMenuItem("Filter               %s",  2,  6,/*'f'*/  0, &settingsChanged.filter, screenGetFilterNames()));
    videoMenu.add(MI_VIDEO_08,    new IntMenuItem("Gamma                %s",  2,  7,/*'a'*/  1, &settingsChanged.gamma, 50, 150, 10, MENU_OUTPUT_GAMMA));
    videoMenu.add(USE_SETTINGS,                   "\010 Use These Settings",  2, 11,/*'u'*/  2);
    videoMenu.add(CANCEL,                         "\010 Cancel",              2, 12,/*'c'*/  2);
    videoMenu.addShortcutKey(CANCEL, ' ');
    videoMenu.setClosesMenu(USE_SETTINGS);
    videoMenu.setClosesMenu(CANCEL);

    gfxMenu.setTitle("Game Graphics Options", 0,0);
    gfxMenu.add(MI_GFX_SCHEME,                        new StringMenuItem("Graphics Scheme    %s", 2,  2,/*'G'*/ 0, &settingsChanged.videoType, xu4.config->schemeNames()));
    gfxMenu.add(MI_GFX_TILE_TRANSPARENCY,               new BoolMenuItem("Transparency Hack  %s", 2,  4,/*'t'*/ 0, &settingsChanged.enhancementsOptions.u4TileTransparencyHack));
    gfxMenu.add(MI_GFX_TILE_TRANSPARENCY_SHADOW_SIZE,    new IntMenuItem("  Shadow Size:     %d", 2,  5,/*'s'*/ 9, &settingsChanged.enhancementsOptions.u4TrileTransparencyHackShadowBreadth, 0, 16, 1));
    gfxMenu.add(MI_GFX_TILE_TRANSPARENCY_SHADOW_OPACITY, new IntMenuItem("  Shadow Opacity:  %d", 2,  6,/*'o'*/ 9, &settingsChanged.enhancementsOptions.u4TileTransparencyHackPixelShadowOpacity, 8, 256, 8));
    gfxMenu.add(MI_VIDEO_02,                          new StringMenuItem("Gem Layout         %s", 2,  8,/*'e'*/ 1, &settingsChanged.gemLayout, screenGetGemLayoutNames()));
    gfxMenu.add(MI_VIDEO_03,                            new EnumMenuItem("Line Of Sight      %s", 2,  9,/*'l'*/ 0, &settingsChanged.lineOfSight, screenGetLineOfSightStyles()));
    gfxMenu.add(MI_VIDEO_07,                            new BoolMenuItem("Screen Shaking     %s", 2, 10,/*'k'*/ 8, &settingsChanged.screenShakes));
    gfxMenu.add(MI_GFX_RETURN,               "\010 Return to Video Options",              2,  12,/*'r'*/  2);
    gfxMenu.setClosesMenu(MI_GFX_RETURN);


    soundMenu.setTitle("Sound Options:", 0, 0);
    soundMenu.add(MI_SOUND_01,  new IntMenuItem("Music Volume         %s", 2,  2,/*'m'*/  0, &settingsChanged.musicVol, 0, MAX_VOLUME, 1, MENU_OUTPUT_VOLUME));
    soundMenu.add(MI_SOUND_02,  new IntMenuItem("Sound Effect Volume  %s", 2,  3,/*'s'*/  0, &settingsChanged.soundVol, 0, MAX_VOLUME, 1, MENU_OUTPUT_VOLUME));
    soundMenu.add(MI_SOUND_03, new BoolMenuItem("Fading               %s", 2,  4,/*'f'*/  0, &settingsChanged.volumeFades));
    soundMenu.add(USE_SETTINGS,                 "\010 Use These Settings", 2, 11,/*'u'*/  2);
    soundMenu.add(CANCEL,                       "\010 Cancel",             2, 12,/*'c'*/  2);
    soundMenu.addShortcutKey(CANCEL, ' ');
    soundMenu.setClosesMenu(USE_SETTINGS);
    soundMenu.setClosesMenu(CANCEL);

    inputMenu.setTitle("Keyboard Options:", 0, 0);
    inputMenu.add(MI_INPUT_01,  new IntMenuItem("Repeat Delay        %4d msec", 2,  2,/*'d'*/  7, &settingsChanged.keydelay, 100, MAX_KEY_DELAY, 100));
    inputMenu.add(MI_INPUT_02,  new IntMenuItem("Repeat Interval     %4d msec", 2,  3,/*'i'*/  7, &settingsChanged.keyinterval, 10, MAX_KEY_INTERVAL, 10));
    /* "Mouse Options:" is drawn in the updateInputMenu() function */
    inputMenu.add(MI_INPUT_03, new BoolMenuItem("Mouse                %s",      2,  7,/*'m'*/  0, &settingsChanged.mouseOptions.enabled));
    inputMenu.add(USE_SETTINGS,                 "\010 Use These Settings",      2, 11,/*'u'*/  2);
    inputMenu.add(CANCEL,                       "\010 Cancel",                  2, 12,/*'c'*/  2);
    inputMenu.addShortcutKey(CANCEL, ' ');
    inputMenu.setClosesMenu(USE_SETTINGS);
    inputMenu.setClosesMenu(CANCEL);

    speedMenu.setTitle("Speed Options:", 0, 0);
    speedMenu.add(MI_SPEED_01, new IntMenuItem("Game Cycles per Second    %3d",      2,  2,/*'g'*/  0, &settingsChanged.gameCyclesPerSecond, 1, MAX_CYCLES_PER_SECOND, 1));
    speedMenu.add(MI_SPEED_02, new IntMenuItem("Battle Speed              %3d",      2,  3,/*'b'*/  0, &settingsChanged.battleSpeed, 1, MAX_BATTLE_SPEED, 1));
    speedMenu.add(MI_SPEED_03, new IntMenuItem("Spell Effect Length       %s",       2,  4,/*'p'*/  1, &settingsChanged.spellEffectSpeed, 1, MAX_SPELL_EFFECT_SPEED, 1, MENU_OUTPUT_SPELL));
    speedMenu.add(MI_SPEED_04, new IntMenuItem("Camping Length            %3d sec",  2,  5,/*'m'*/  2, &settingsChanged.campTime, 1, MAX_CAMP_TIME, 1));
    speedMenu.add(MI_SPEED_05, new IntMenuItem("Inn Rest Length           %3d sec",  2,  6,/*'i'*/  0, &settingsChanged.innTime, 1, MAX_INN_TIME, 1));
    speedMenu.add(MI_SPEED_06, new IntMenuItem("Shrine Meditation Length  %3d sec",  2,  7,/*'s'*/  0, &settingsChanged.shrineTime, 1, MAX_SHRINE_TIME, 1));
    speedMenu.add(MI_SPEED_07, new IntMenuItem("Screen Shake Interval     %3d msec", 2,  8,/*'r'*/  2, &settingsChanged.shakeInterval, MIN_SHAKE_INTERVAL, MAX_SHAKE_INTERVAL, 10));
    speedMenu.add(USE_SETTINGS,                "\010 Use These Settings",            2, 11,/*'u'*/  2);
    speedMenu.add(CANCEL,                      "\010 Cancel",                        2, 12,/*'c'*/  2);
    speedMenu.addShortcutKey(CANCEL, ' ');
    speedMenu.setClosesMenu(USE_SETTINGS);
    speedMenu.setClosesMenu(CANCEL);

    /* move the BATTLE DIFFICULTY, DEBUG, and AUTOMATIC ACTIONS settings to "enhancementsOptions" */
    gameplayMenu.setTitle                              ("Enhanced Gameplay Options:", 0, 0);
    gameplayMenu.add(MI_GAMEPLAY_01,   new EnumMenuItem("Battle Difficulty          %s", 2,  2,/*'b'*/  0, &settingsChanged.battleDiff, Settings::battleDiffStrings()));
    gameplayMenu.add(MI_GAMEPLAY_02,   new BoolMenuItem("Fixed Chest Traps          %s", 2,  3,/*'t'*/ 12, &settingsChanged.enhancementsOptions.c64chestTraps));
    gameplayMenu.add(MI_GAMEPLAY_03,   new BoolMenuItem("Gazer Spawns Insects       %s", 2,  4,/*'g'*/  0, &settingsChanged.enhancementsOptions.gazerSpawnsInsects));
    gameplayMenu.add(MI_GAMEPLAY_04,   new BoolMenuItem("Gem View Shows Objects     %s", 2,  5,/*'e'*/  1, &settingsChanged.enhancementsOptions.peerShowsObjects));
    gameplayMenu.add(MI_GAMEPLAY_05,   new BoolMenuItem("Slime Divides              %s", 2,  6,/*'s'*/  0, &settingsChanged.enhancementsOptions.slimeDivides));
    gameplayMenu.add(MI_GAMEPLAY_06,   new BoolMenuItem("Debug Mode (Cheats)        %s", 2,  8,/*'d'*/  0, &settingsChanged.debug));
    gameplayMenu.add(USE_SETTINGS,                      "\010 Use These Settings",       2, 11,/*'u'*/  2);
    gameplayMenu.add(CANCEL,                            "\010 Cancel",                   2, 12,/*'c'*/  2);
    gameplayMenu.addShortcutKey(CANCEL, ' ');
    gameplayMenu.setClosesMenu(USE_SETTINGS);
    gameplayMenu.setClosesMenu(CANCEL);

    interfaceMenu.setTitle("Enhanced Interface Options:", 0, 0);
    interfaceMenu.add(MI_INTERFACE_01, new BoolMenuItem("Automatic Actions          %s", 2,  2,/*'a'*/  0, &settingsChanged.shortcutCommands));
    /* "(Open, Jimmy, etc.)" */
    interfaceMenu.add(MI_INTERFACE_02, new BoolMenuItem("Set Active Player          %s", 2,  4,/*'p'*/ 11, &settingsChanged.enhancementsOptions.activePlayer));
    interfaceMenu.add(MI_INTERFACE_03, new BoolMenuItem("Smart 'Enter' Key          %s", 2,  5,/*'e'*/  7, &settingsChanged.enhancementsOptions.smartEnterKey));
    interfaceMenu.add(MI_INTERFACE_04, new BoolMenuItem("Text Colorization          %s", 2,  6,/*'t'*/  0, &settingsChanged.enhancementsOptions.textColorization));
    interfaceMenu.add(MI_INTERFACE_05, new BoolMenuItem("Ultima V Shrines           %s", 2,  7,/*'s'*/  9, &settingsChanged.enhancementsOptions.u5shrines));
    interfaceMenu.add(MI_INTERFACE_06, new BoolMenuItem("Ultima V Spell Mixing      %s", 2,  8,/*'m'*/ 15, &settingsChanged.enhancementsOptions.u5spellMixing));
    interfaceMenu.add(USE_SETTINGS,                     "\010 Use These Settings",       2, 11,/*'u'*/  2);
    interfaceMenu.add(CANCEL,                           "\010 Cancel",                   2, 12,/*'c'*/  2);
    interfaceMenu.addShortcutKey(CANCEL, ' ');
    interfaceMenu.setClosesMenu(USE_SETTINGS);
    interfaceMenu.setClosesMenu(CANCEL);
}

IntroController::~IntroController() {
    for (unsigned i=0; i < titles.size(); i++) {
        delete titles[i].srcImage;
        delete titles[i].destImage;
    }
}

bool IntroController::present() {
    init();
    preloadMap();
    listenerId = gs_listen(1<<SENDER_MENU, introNotice, this);
    return true;
}

void IntroController::conclude() {
    gs_unplug(listenerId);
    deleteIntro();
}

#ifdef GPU_RENDER
void IntroController::enableMap() {
    Coords center(INTRO_MAP_WIDTH / 2, INTRO_MAP_HEIGHT / 2);
    screenUpdateMap(&mapArea, &binData->introMap, center);
}

#define MAP_ENABLE  enableMap()
#define MAP_DISABLE mapArea.clear()
#else
#define MAP_ENABLE
#define MAP_DISABLE
#endif

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
bool IntroController::init() {

    justInitiatedNewGame = false;
    introMusic = MUSIC_TOWNS;

    uint16_t saveGroup = xu4.imageMgr->setResourceGroup(StageIntro);

    // sigData is referenced during Titles initialization
    binData = new IntroBinData();
    binData->load();

    Symbol sym[2];
    xu4.config->internSymbols(sym, 2, "beast0frame00 beast1frame00");
    beastiesImg = xu4.imageMgr->get(BKGD_ANIMATE);  // Assign resource group.
    beastieSub[0] = beastiesImg->subImageIndex[sym[0]];
    beastieSub[1] = beastiesImg->subImageIndex[sym[1]];

    if (xu4.errorMessage)
        bSkipTitles = true;

    if (bSkipTitles)
    {
        // the init() method is called again from within the
        // game via ALT-Q, so return to the menu
        //
#ifndef IOS
        mode = INTRO_MENU;
#else
        mode = INTRO_MAP;
#endif
        beastiesVisible = true;
        beastieOffset = 0;

        musicPlay(introMusic);
    }
    else
    {
        // initialize the titles
        initTitles();
        mode = INTRO_TITLES;
        beastiesVisible = false;
        beastieOffset = -32;
    }

    beastie1Cycle = 0;
    beastie2Cycle = 0;

    sleepCycles = 0;
    scrPos = 0;
#ifndef GPU_RENDER
    objectStateTable = new IntroObjectState[IntroBinData::INTRO_BASETILE_TABLE_SIZE];
#endif

    backgroundArea.reinit();
    menuArea.reinit();
    extendedMenuArea.reinit();
    questionArea.reinit();
    mapArea.reinit();

    // only update the screen if we are returning from game mode
    if (bSkipTitles)
        updateScreen();

    xu4.imageMgr->setResourceGroup(saveGroup);
    return true;
}

bool IntroController::hasInitiatedNewGame()
{
    return this->justInitiatedNewGame;
}

/**
 * Frees up data not needed after introduction.
 */
void IntroController::deleteIntro() {
    delete binData;
    binData = NULL;

#ifdef GPU_RENDER
    // Ensure that any running map animations are freed.
    Animator* fa = &xu4.eventHandler->flourishAnim;
    if (fa->used)
        anim_clear(fa);
#else
    delete [] objectStateTable;
    objectStateTable = NULL;
#endif

    xu4.imageMgr->freeResourceGroup(StageIntro);
    beastiesImg = NULL;
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

    case INTRO_TITLES:
        // the user pressed a key to abort the sequence
        skipTitles();
        break;

    case INTRO_MAP:
        MAP_DISABLE;
        mode = INTRO_MENU;
        updateScreen();
        break;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            initiateNewGame();
            break;
        case 'j':
            journeyOnward();
            break;
        case 'r':
            mode = INTRO_MAP;
            updateScreen();
            MAP_ENABLE;
            break;
        case 'c': {
            // Make a copy of our settings so we can change them
            settingsChanged = *xu4.settings;
            screenDisableCursor();
            runMenu(&confMenu, &extendedMenuArea, true);
            screenEnableCursor();
            updateScreen();
            break;
        }
        case 'a':
            about();
            break;
        case 'q':
            xu4.eventHandler->quitGame();
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
        {
            int n = key - '0';
            if (n > MUSIC_NONE && n < MUSIC_MAX)
                musicPlay(introMusic = n);
        }
            break;
        default:
            valid = false;
            break;
        }
        break;

    default:
        ASSERT(0, "key handler called in wrong mode");
        return true;
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

/**
 * Draws the small map on the intro screen.
 */
void IntroController::drawMap() {
    if (0 && sleepCycles > 0) {
        drawMapStatic();
        drawMapAnimated();
        sleepCycles--;
    }
    else {
        TileId tileId;
        int frame;
        int x, y;
        unsigned char commandNibble;
        unsigned char dataNibble;
        const unsigned char* script = binData->scriptTable;

        do {
            commandNibble = script[scrPos] >> 4;

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
                dataNibble = script[scrPos] & 0xf;
                x = script[scrPos+1] & 0x1f;
                y = commandNibble;

                // See if the tile id needs to be recalculated
                tileId = binData->baseTileTable[dataNibble]->getId();
                frame = script[scrPos+1] >> 5;
                if (frame >= binData->baseTileTable[dataNibble]->getFrames()) {
                    frame -= binData->baseTileTable[dataNibble]->getFrames();
                    tileId += 1;
                }

#ifdef GPU_RENDER
                {
                VisualEffect* fx =
                    mapArea.useEffect(dataNibble, tileId, (float)x, (float)y);
                fx->vid += frame;
                }
#else
                objectStateTable[dataNibble].x = x;
                objectStateTable[dataNibble].y = y;
                objectStateTable[dataNibble].tile = MapTile(tileId);
                objectStateTable[dataNibble].tile.frame = frame;
#endif

                scrPos += 2;
                break;
            case 7:
                /* ---------------
                   Delete object
                   Format: 7i
                   i = table index
                   --------------- */
                dataNibble = script[scrPos] & 0xf;
#ifdef GPU_RENDER
                mapArea.removeEffect(dataNibble);
#else
                objectStateTable[dataNibble].tile = 0;
#endif
                scrPos++;
                break;
            case 8:
                /* ----------------------------------------------
                   Redraw intro map and objects, then go to sleep
                   Format: 8c
                   c = cycles to sleep
                   ---------------------------------------------- */
                drawMapStatic();
                drawMapAnimated();

                /* set sleep cycles */
                sleepCycles = script[scrPos] & 0xf;
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

void IntroController::drawMapStatic() {
#ifndef GPU_RENDER
    int x, y;

    // draw unmodified map
    for (y = 0; y < INTRO_MAP_HEIGHT; y++)
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            mapArea.drawTile(binData->introMap[x + (y * INTRO_MAP_WIDTH)], x, y);
#endif
}

void IntroController::drawMapAnimated() {
#ifndef GPU_RENDER
    int i;

    // draw animated objects
    Image::enableBlend(1);
    for (i = 0; i < IntroBinData::INTRO_BASETILE_TABLE_SIZE; i++) {
        IntroObjectState& state = objectStateTable[i];
        if (state.tile != 0)
            mapArea.drawTile(state.tile, state.x, state.y);
    }
    Image::enableBlend(0);
#endif
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
    ASSERT(beast == 0 || beast == 1, "invalid beast: %d", beast);

    backgroundArea.draw(beastiesImg, beastieSub[beast] + frame,
                        beast ? (320 - 48) : 0,
                        vertoffset);
}

/**
 * Animates the moongate in the tree intro image.  There are two
 * overlays in the part of the image normally covered by the text.  If
 * the frame parameter is "moongate", the moongate overlay is painted
 * over the image.  If frame is "items", the second overlay is
 * painted: the circle without the moongate, but with a small white
 * dot representing the anhk and history book.
 *
 * TODO: Animate the moongate opening & closing to match the actual game.
 */
void IntroController::animateTree(Symbol frame) {
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

    backgroundArea.draw(xu4.config->intern(cardNames[card]),
                        pos ? 218 : 12, 12);
}

/**
 * Draws the beads in the abacus during the character creation sequence
 */
void IntroController::drawAbacusBeads(int row, int selectedVirtue, int rejectedVirtue) {
    ASSERT(row >= 0 && row < 7, "invalid row: %d", row);
    ASSERT(selectedVirtue < 8 && selectedVirtue >= 0, "invalid virtue: %d", selectedVirtue);
    ASSERT(rejectedVirtue < 8 && rejectedVirtue >= 0, "invalid virtue: %d", rejectedVirtue);

    backgroundArea.draw(IMG_WHITEBEAD, 128 + (selectedVirtue * 9), 24 + (row * 15));
    backgroundArea.draw(IMG_BLACKBEAD, 128 + (rejectedVirtue * 9), 24 + (row * 15));
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
        // display the profile name if a local profile is being used
        {
        const string& pname = xu4.settings->profile;
        if (! pname.empty())
            screenTextAt(40-pname.length(), 24, "%s", pname.c_str());
        }
        break;

    case INTRO_MENU:
        // draw the extended background for all option screens
        backgroundArea.draw(BKGD_INTRO);
        backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

        // if there is an error message to display, show it
        if (xu4.errorMessage)
        {
            menuArea.textAt(6, 5, xu4.errorMessage);
            xu4.errorMessage = NULL;

            drawBeasties();
            // wait for a couple seconds
            EventHandler::wait_msecs(3000);
            // clear the screen again
            backgroundArea.draw(BKGD_INTRO);
            backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
        }

        menuArea.textAt(1,  1, "In another world, in a time to come.");
        menuArea.textAt(14, 3, "Options:");
        menuArea.textAt(10, 5, "%s", menuArea.colorizeString("Return to the view", FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 6, "%s", menuArea.colorizeString("Journey Onward",     FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 7, "%s", menuArea.colorizeString("Initiate New Game",  FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 8, "%s", menuArea.colorizeString("Configure",          FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 9, "%s", menuArea.colorizeString("About",              FG_YELLOW, 0, 1).c_str());
        drawBeasties();

        // draw the cursor last
        screenSetCursorPos(24, 16);
        screenShowCursor();
        break;

    default:
        ASSERT(0, "bad mode in updateScreen");
    }

    screenUpdateCursor();
}

/**
 * Initiate a new savegame by reading the name, sex, then presenting a
 * series of questions to determine the class of the new character.
 */
void IntroController::initiateNewGame() {
    // disable the screen cursor because a text cursor will now be used
    screenDisableCursor();

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // display name prompt and read name from keyboard
    menuArea.textAt(3, 3, "By what name shalt thou be known");
    menuArea.textAt(3, 4, "in this world and time?");

    // enable the text cursor after setting it's initial position
    // this will avoid drawing in undesirable areas like 0,0
    menuArea.setCursorPos(11, 7, false);
    menuArea.setCursorFollowsText(true);
    menuArea.enableCursor();

    drawBeasties();

    string nameBuffer = ReadStringController::get(12, &menuArea, "\033");
    if (nameBuffer.length() == 0) {
        // the user didn't enter a name
        menuArea.disableCursor();
        screenEnableCursor();
        updateScreen();
        return;
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // display sex prompt and read sex from keyboard
    menuArea.textAt(3, 3, "Art thou Male or Female?");

    // the cursor is already enabled, just change its position
    menuArea.setCursorPos(28, 3, true);

    drawBeasties();

    SexType sex;
    int sexChoice = ReadChoiceController::get("mf");
    if (sexChoice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    finishInitiateGame(nameBuffer, sex);
}

void IntroController::finishInitiateGame(const string &nameBuffer, SexType sex)
{
#ifdef IOS
    mode = INTRO_MENU; // ensure we are now in the menu mode, (i.e., stop drawing the map).
#endif
    // no more text entry, so disable the text cursor
    menuArea.disableCursor();

    {
    uint16_t saveGroup = xu4.imageMgr->setResourceGroup(StageIntro);

    // show the lead up story
    showStory();
    if (xu4.stage != StageIntro)
        return;

    // ask questions that determine character class
    startQuestions();
    if (xu4.stage != StageIntro)
        return;

    xu4.imageMgr->setResourceGroup(saveGroup);
    }

    // write out save game an segue into game

    delete xu4.saveGame;
    xu4.saveGame = NULL;    // Make GameController::init() reload the game.

    FILE *saveGameFile = fopen((xu4.settings->getUserPath() + PARTY_SAV).c_str(), "wb");
    if (!saveGameFile) {
        questionArea.disableCursor();
        xu4.errorMessage = "Unable to create save game!";
        updateScreen();
        return;
    }

    {
    SaveGame saveGame;
    SaveGamePlayerRecord avatar;

    avatar.init();
    strcpy(avatar.name, nameBuffer.c_str());
    avatar.sex = sex;
    saveGame.init(&avatar);
    screenHideCursor();
    initPlayers(&saveGame);
    saveGame.food = 30000;
    saveGame.gold = 200;
    saveGame.reagents[REAG_GINSENG] = 3;
    saveGame.reagents[REAG_GARLIC] = 4;
    saveGame.torches = 2;
    saveGame.write(saveGameFile);
    }

    fclose(saveGameFile);

    saveGameFile = fopen((xu4.settings->getUserPath() + MONSTERS_SAV).c_str(), "wb");
    if (saveGameFile) {
        saveGameMonstersWrite(NULL, saveGameFile);
        fclose(saveGameFile);
    }
    justInitiatedNewGame = true;

    // show the text thats segues into the main game
    showText(binData->introGypsy[GYP_SEGUE1]);
#ifdef IOS
    U4IOS::switchU4IntroControllerToContinueButton();
#endif
    ReadChoiceController pauseController("");
    xu4.eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    showText(binData->introGypsy[GYP_SEGUE2]);

    xu4.eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    // done: exit intro and let game begin
    questionArea.disableCursor();

    xu4.stage = StagePlay;
    xu4.eventHandler->setControllerDone();
}

void IntroController::showStory() {
    ReadChoiceController pauseController("");

    beastiesVisible = false;

    questionArea.setCursorFollowsText(true);

    for (int storyInd = 0; storyInd < 24; storyInd++) {
        if (storyInd == 0)
            backgroundArea.draw(BKGD_TREE);
        else if (storyInd == 3)
            animateTree(IMG_MOONGATE);
        else if (storyInd == 5)
            animateTree(IMG_ITEMS);
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

        xu4.eventHandler->pushController(&pauseController);
        // enable the cursor here to avoid drawing in undesirable locations
        questionArea.enableCursor();
        pauseController.waitFor();
        if (xu4.stage != StageIntro)
            break;
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

#ifdef IOS
        U4IOS::switchU4IntroControllerToContinueButton();
#endif
        // wait for a key
        xu4.eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenEnableCursor();
        // show the question to choose between virtues
        showText(getQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));

#ifdef IOS
        U4IOS::switchU4IntroControllerToABButtons();
#endif
        // wait for an answer
        xu4.eventHandler->pushController(&questionController);
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
    // Return to a running game or attempt to load a saved one.
    if (xu4.saveGame || saveGameLoad()) {
        xu4.stage = StagePlay;
        xu4.eventHandler->setControllerDone();
    } else {
        updateScreen();     // Shows errorMessage set by saveGameLoad().
    }
}

/**
 * Shows an about box.
 */
void IntroController::about() {
    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    screenHideCursor();
    menuArea.textAt(14, 1, "XU4 %s", VERSION);
    menuArea.textAt(1, 3, "xu4 is free software; you can redist-");
    menuArea.textAt(1, 4, "ribute it and/or modify it under the");
    menuArea.textAt(1, 5, "terms of the GNU GPL as published by");
    menuArea.textAt(1, 6, "the FSF.  See COPYING.");
    menuArea.textAt(4, 8, "Copyright \011 2002-2006, xu4 Team");
    menuArea.textAt(4, 9, "Copyright \011 1987, Lord British");
    drawBeasties();

    ReadChoiceController::get("");

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
    menu->reset();

    // if the menu has an extended height, fill the menu background, otherwise reset the display
    menu->show(view);
    if (withBeasties)
        drawBeasties();

    MenuController menuController(menu, view);
    xu4.eventHandler->pushController(&menuController);
    menuController.waitFor();

    // enable the cursor here, after the menu has been established
    view->enableCursor();
    view->disableCursor();
}

/**
 * Timer callback for the intro sequence.  Handles animating the intro
 * map, the beasties, etc..
 */
void IntroController::timerFired() {
    screenCycle();
    screenUpdateCursor();

    if (mode == INTRO_TITLES)
        if (updateTitle() == false)
        {
            // setup the map screen
            mode = INTRO_MAP;
            beastiesVisible = true;
            musicPlay(introMusic);
            updateScreen();
            MAP_ENABLE;
        }

    if (mode == INTRO_MAP)
        drawMap();

    if (beastiesVisible)
        drawBeasties();

    if (xu4_random(2) && ++beastie1Cycle >= IntroBinData::BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if (xu4_random(2) && ++beastie2Cycle >= IntroBinData::BEASTIE2_FRAMES)
        beastie2Cycle = 0;

#ifdef USE_GL
    gpu_blitTexture(gpu_screenTexture(xu4.gpu), 0, 0, xu4.screenImage);
#endif
}

/**
 * Update the screen when an observed menu is reset or has an item
 * activated.
 * TODO, reduce duped code.
 */
void IntroController::introNotice(int sender, void* eventData, void* user) {
    MenuEvent* event = (MenuEvent*) eventData;
    ((IntroController*) user)->dispatchMenu(event->menu, *event);
}

void IntroController::dispatchMenu(const Menu *menu, MenuEvent &event) {
    if (menu == &confMenu)
        updateConfMenu(event);
    else if (menu == &videoMenu)
        updateVideoMenu(event);
    else if (menu == &gfxMenu)
        updateGfxMenu(event);
    else if (menu == &soundMenu)
        updateSoundMenu(event);
    else if (menu == &inputMenu)
        updateInputMenu(event);
    else if (menu == &speedMenu)
        updateSpeedMenu(event);
    else if (menu == &gameplayMenu)
        updateGameplayMenu(event);
    else if (menu == &interfaceMenu)
        updateInterfaceMenu(event);

    // beasties are always visible on the menus
    drawBeasties();
}

void IntroController::updateConfMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        // show or hide game enhancement options if enhancements are enabled/disabled
        confMenu.getItemById(MI_CONF_GAMEPLAY)->setVisible(settingsChanged.enhancements);
        confMenu.getItemById(MI_CONF_INTERFACE)->setVisible(settingsChanged.enhancements);

        // save settings
        xu4.settings->setData(settingsChanged);
        xu4.settings->write();

        switch(event.item->getId()) {
        case MI_CONF_VIDEO:
            runMenu(&videoMenu, &extendedMenuArea, true);
            break;
        case MI_VIDEO_CONF_GFX:
            runMenu(&gfxMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_SOUND:
            runMenu(&soundMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_INPUT:
            runMenu(&inputMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_SPEED:
            runMenu(&speedMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_GAMEPLAY:
            runMenu(&gameplayMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_INTERFACE:
            runMenu(&interfaceMenu, &extendedMenuArea, true);
            break;
        case CANCEL:
            // discard settings
            settingsChanged = *xu4.settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateVideoMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
        case USE_SETTINGS:
            /* save settings (if necessary) */
            if (*xu4.settings != settingsChanged) {
                xu4.settings->setData(settingsChanged);
                xu4.settings->write();

                /* FIXME: resize images, etc. */
                deleteIntro();  // delete intro stuff
                screenReInit();
                init();         // re-fix the backgrounds and scale images, etc.

                // go back to menu mode
                mode = INTRO_MENU;
            }
            break;
        case MI_VIDEO_CONF_GFX:
            runMenu(&gfxMenu, &extendedMenuArea, true);
            break;
        case CANCEL:
            // discard settings
            settingsChanged = *xu4.settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateGfxMenu(MenuEvent &event)
{
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {


        switch(event.item->getId()) {
        case MI_GFX_RETURN:
            runMenu(&videoMenu, &extendedMenuArea, true);
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateSoundMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
            case MI_SOUND_01:
                musicSetVolume(settingsChanged.musicVol);
                break;
            case MI_SOUND_02:
                soundSetVolume(settingsChanged.soundVol);
                soundPlay(SOUND_FLEE);
                break;
            case USE_SETTINGS:
                // save settings
                xu4.settings->setData(settingsChanged);
                xu4.settings->write();
                musicPlay(introMusic);
                break;
            case CANCEL:
                musicSetVolume(xu4.settings->musicVol);
                soundSetVolume(xu4.settings->soundVol);
                // discard settings
                settingsChanged = *xu4.settings;
                break;
            default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateInputMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
        case USE_SETTINGS:
            // save settings
            xu4.settings->setData(settingsChanged);
            xu4.settings->write();

            // re-initialize keyboard
            EventHandler::setKeyRepeat(settingsChanged.keydelay, settingsChanged.keyinterval);
#ifndef IOS
            screenShowMouseCursor(xu4.settings->mouseOptions.enabled);
#endif
            break;
        case CANCEL:
            // discard settings
            settingsChanged = *xu4.settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // after drawing the menu, extra menu text can be added here
    extendedMenuArea.textAt(0, 5, "Mouse Options:");
}

void IntroController::updateSpeedMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
        case USE_SETTINGS:
            // save settings
            xu4.settings->setData(settingsChanged);
            xu4.settings->write();

            // re-initialize events
            xu4.eventHandler->setTimerInterval(1000 /
                                        xu4.settings->gameCyclesPerSecond);
            break;
        case CANCEL:
            // discard settings
            settingsChanged = *xu4.settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateGameplayMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
        case USE_SETTINGS:
            // save settings
            xu4.settings->setData(settingsChanged);
            xu4.settings->write();
            break;
        case CANCEL:
            // discard settings
            settingsChanged = *xu4.settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateInterfaceMenu(MenuEvent &event) {
    if (event.type == MenuEvent::ACTIVATE ||
        event.type == MenuEvent::INCREMENT ||
        event.type == MenuEvent::DECREMENT) {

        switch(event.item->getId()) {
            case USE_SETTINGS:
                // save settings
                xu4.settings->setData(settingsChanged);
                xu4.settings->write();
                break;
            case CANCEL:
                // discard settings
                settingsChanged = *xu4.settings;
                break;
            default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // after drawing the menu, extra menu text can be added here
    extendedMenuArea.textAt(2, 3, "  (Open, Jimmy, etc.)");
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
    } initValuesForClass[] = {
        { WEAP_STAFF,  ARMR_CLOTH,   2, 125, 231, 136 }, /* CLASS_MAGE */
        { WEAP_SLING,  ARMR_CLOTH,   3, 240,  83, 105 }, /* CLASS_BARD */
        { WEAP_AXE,    ARMR_LEATHER, 3, 205,  35, 221 }, /* CLASS_FIGHTER */
        { WEAP_DAGGER, ARMR_CLOTH,   2, 175,  59,  44 }, /* CLASS_DRUID */
        { WEAP_MACE,   ARMR_LEATHER, 2, 110, 158,  21 }, /* CLASS_TINKER */
        { WEAP_SWORD,  ARMR_CHAIN,   3, 325, 105, 183 }, /* CLASS_PALADIN */
        { WEAP_SWORD,  ARMR_LEATHER, 2, 150,  23, 129 }, /* CLASS_RANGER */
        { WEAP_STAFF,  ARMR_CLOTH,   1,   5, 186, 171 }  /* CLASS_SHEPHERD */
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

    for (i = 0; i < VIRT_MAX; i++)
        saveGame->karma[i] = 50;

    for (i = 8; i < 15; i++) {
        saveGame->karma[questionTree[i]] += 5;
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
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_HONOR:
            saveGame->players[0].intel++;
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


/**
 * Preload map tiles
 */
void IntroController::preloadMap()
{
#ifndef GPU_RENDER
    int x, y, i;

    // draw unmodified map
    for (y = 0; y < INTRO_MAP_HEIGHT; y++)
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            mapArea.loadTile(binData->introMap[x + (y * INTRO_MAP_WIDTH)]);

    // draw animated objects
    for (i = 0; i < IntroBinData::INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0)
            mapArea.loadTile(objectStateTable[i].tile);
    }
#endif
}


//
// Initialize the title elements
//
void IntroController::initTitles()
{
    // add the intro elements
    //          x,  y,   w,  h, method,  delay, duration
    //
    addTitle(  97,  0, 130, 16, SIGNATURE,   1000, 3000 );  // "Lord British"
    addTitle( 148, 17,  24,  4, AND,         1000,  100 );  // "and"
    addTitle(  84, 31, 152,  1, BAR,         1000,  500 );  // <bar>
    addTitle(  86, 21, 148,  9, ORIGIN,      1000,  100 );  // "Origin Systems, Inc."
    addTitle( 133, 33,  54,  5, PRESENT,        0,  100 );  // "present"
    addTitle(  59, 33, 202, 46, TITLE,       1000, 5000 );  // "Ultima IV"
    addTitle(  40, 80, 240, 13, SUBTITLE,    1000,  100 );  // "Quest of the Avatar"
    addTitle(   0, 96, 320, 96, MAP,         1000,  100 );  // the map

    // get the source data for the elements
    getTitleSourceData();

    // reset the iterator
    title = titles.begin();

    // speed up the timer while the intro titles are displayed
    xu4.eventHandler->setTimerInterval(xu4.settings->titleSpeedOther);
}


//
// Add the intro element to the element list
//
void IntroController::addTitle(int x, int y, int w, int h, AnimType method, int delay, int duration)
{
    AnimElement data = {
        x, y,               // source x and y
        w, h,               // source width and height
        method,             // render method
        0,                  // animStep
        0,                  // animStepMax
        0,                  // timeBase
        delay,              // delay before rendering begins
        duration,           // total animation time
        NULL,               // storage for the source image
        NULL,               // storage for the animation frame
        std::vector<AnimPlot>()
#ifndef USE_GL
        , false
#endif
    };
    titles.push_back(data);
}


//
// Get the source data for title elements
// that have already been initialized
//
void IntroController::getTitleSourceData()
{
    unsigned int r, g, b, a;        // color values
    unsigned char *srcData;         // plot data

    // The BKGD_INTRO image is assumed to have not been
    // loaded yet.  The unscaled version will be loaded
    // here, and elements of the image will be stored
    // individually.  Afterward, the BKGD_INTRO image
    // will be scaled appropriately.
    ImageInfo *info = xu4.imageMgr->get(BKGD_INTRO, true);
    if (!info)
        errorLoadImage(BKGD_INTRO);

#ifdef USE_GL
#define ISCALE(n)   n
#else
#define ISCALE(n)   (n * info->prescale)
    if (info->width  / info->prescale != 320 ||
        info->height / info->prescale != 200)
    {
        // the image appears to have been scaled already
        errorWarning("The title image has been scaled too early!");
    }
    SCALED_VAR;
#endif

    // for each element, get the source data
    for (unsigned i=0; i < titles.size(); i++)
    {
        if ((titles[i].method != SIGNATURE)
            && (titles[i].method != BAR))
        {
            // create a place to store the source image
            titles[i].srcImage = Image::create(
                ISCALE(titles[i].rw),
                ISCALE(titles[i].rh));

            // get the source image
            info->image->drawSubRectOn(titles[i].srcImage, 0, 0,
                ISCALE(titles[i].rx),
                ISCALE(titles[i].ry),
                ISCALE(titles[i].rw),
                ISCALE(titles[i].rh));
        }

        // after getting the srcImage
        switch (titles[i].method)
        {
            case SIGNATURE:
            {
                // PLOT: "Lord British"
                srcData = getSigData();

                RGBA color = info->image->setColor(0, 255, 255);    // cyan for EGA
                int blue[16] = {255, 250, 226, 226, 210, 194, 161, 161,
                                129,  97,  97,  64,  64,  32,  32,   0};
                int x = 0;
                int y = 0;

                while (srcData[titles[i].animStepMax] != 0)
                {
                    x = srcData[titles[i].animStepMax] - 0x4C;
                    y = 0xC0 - srcData[titles[i].animStepMax+1];

                    if (xu4.settings->videoType != "EGA")
                    {
                        // yellow gradient
                        color = info->image->setColor(255, (y == 2 ? 250 : 255), blue[y-1]);
                    }
                    AnimPlot plot = {
                        uint8_t(x),
                        uint8_t(y),
                        uint8_t(color.r),
                        uint8_t(color.g),
                        uint8_t(color.b),
                        255};
                    titles[i].plotData.push_back(plot);
                    titles[i].animStepMax += 2;
                }
                titles[i].animStepMax = titles[i].plotData.size();
                break;
            }

            case BAR:
            {
                titles[i].animStepMax = titles[i].rw;  // image width
                break;
            }

            case TITLE:
            {
                for (int y=0; y < titles[i].rh; y++)
                {
                    // Here x is set to exclude PRESENT at top of image.
                    int x = (y < 6) ? 133 : 0;
                    for ( ; x < titles[i].rw ; x++)
                    {
                        titles[i].srcImage->getPixel(ISCALE(x), ISCALE(y), r, g, b, a);
                        if (r || g || b)
                        {
                            AnimPlot plot = {
                                uint8_t(x+1), uint8_t(y+1),
                                uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a)
                            };
                            titles[i].plotData.push_back(plot);
                        }
                    }
                }
                titles[i].animStepMax = titles[i].plotData.size();
                break;
            }

            case MAP:
            {
                // fill the map area with the transparent color
                titles[i].srcImage->fillRect(8, 8, 304, 80, 0, 0, 0, 0);

#ifndef USE_GL
                Image *scaled;      // the scaled and filtered image
                scaled = screenScale(titles[i].srcImage, xu4.settings->scale / info->prescale, 1, 1);

                titles[i].prescaled = true;
                delete titles[i].srcImage;
                titles[i].srcImage = scaled;
#endif

                titles[i].animStepMax = 20;
                break;
            }

            default:
            {
                titles[i].animStepMax = titles[i].rh ;  // image height
                break;
            }
        }

        // create the initial animation frame
        titles[i].destImage = Image::create(
#ifdef USE_GL
            2 + titles[i].rw, 2 + titles[i].rh
#else
            2 + (titles[i].prescaled ? SCALED(titles[i].rw) : titles[i].rw) * info->prescale ,
            2 + (titles[i].prescaled ? SCALED(titles[i].rh) : titles[i].rh) * info->prescale
#endif
            );
        titles[i].destImage->fill(Image::black);
    }

#ifndef USE_GL
    // scale the original image now
    Image *scaled = screenScale(info->image,
                                xu4.settings->scale / info->prescale,
                                1, 1);
    delete info->image;
    info->image = scaled;
#endif
}


#include "support/getTicks.c"


//
// Update the title element, drawing the appropriate frame of animation
//
bool IntroController::updateTitle()
{
    int animStepTarget = 0;

    int timeCurrent = getTicks();
    float timePercent = 0;

    if (title->animStep == 0 && !bSkipTitles)
    {
        if (title->timeBase == 0)
        {
            // reset the base time
            title->timeBase = timeCurrent;
        }
        if (title == titles.begin())
        {
            // clear the screen
            xu4.screenImage->fill(Image::black);
        }
        if (title->method == TITLE)
        {
            // assume this is the first frame of "Ultima IV" and begin sound
            soundPlay(SOUND_TITLE_FADE);
        }
    }

    // abort after processing all elements
    if (title == titles.end())
    {
        return false;
    }

    // delay the drawing of this phase
    if ((timeCurrent - title->timeBase) < title->timeDelay)
    {
        return true;
    }

    // determine how much of the animation should have been drawn up until now
    timePercent = float(timeCurrent - title->timeBase - title->timeDelay) / title->timeDuration;
    if (timePercent > 1 || bSkipTitles)
        timePercent = 1;
    animStepTarget = int(title->animStepMax * timePercent);

    // perform the animation
    switch (title->method)
    {
        case SIGNATURE:
        {
            while (animStepTarget > title->animStep)
            {
                // blit the pixel-pair to the src surface
                title->destImage->fillRect(
                    title->plotData[title->animStep].x,
                    title->plotData[title->animStep].y,
                    2,
                    1,
                    title->plotData[title->animStep].r,
                    title->plotData[title->animStep].g,
                    title->plotData[title->animStep].b);
                title->animStep++;
            }
            break;
        }

        case BAR:
        {
            RGBA color;
            while (animStepTarget > title->animStep)
            {
                title->animStep++;
                color = title->destImage->setColor(128, 0, 0); // dark red for the underline

                // blit bar to the canvas
                title->destImage->fillRect(
                    1,
                    1,
                    title->animStep,
                    1,
                    color.r,
                    color.g,
                    color.b);
            }
            break;
        }

        case AND:
        {
            // blit the entire src to the canvas
            title->srcImage->drawOn(title->destImage, 1, 1);
            title->animStep = title->animStepMax;
            break;
        }

        case ORIGIN:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // blit src to the canvas one row at a time, bottom up
            title->srcImage->drawSubRectOn(
                title->destImage,
                1,
                title->destImage->height() - 1 - title->animStep,
                0,
                0,
                title->srcImage->width(),
                title->animStep);
            break;
        }

        case PRESENT:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // blit src to the canvas one row at a time, top down
            title->srcImage->drawSubRectOn(
                title->destImage,
                1,
                1,
                0,
                title->srcImage->height() - title->animStep,
                title->srcImage->width(),
                title->animStep);
            break;
        }

        case TITLE:
        {
            // blit src to the canvas in a random pixelized manner
            title->animStep = animStepTarget;

            random_shuffle(title->plotData.begin(), title->plotData.end());
            title->destImage->fillRect(1, 1, title->rw, title->rh, 0, 0, 0);

            // @TODO: animStepTarget (for this loop) should not exceed
            // half of animStepMax.  If so, instead draw the entire
            // image, and then black out the necessary pixels.
            // this should speed the loop up at the end
            for (int i=0; i < animStepTarget; ++i)
            {
                title->destImage->putPixel(
                    title->plotData[i].x,
                    title->plotData[i].y,
                    title->plotData[i].r,
                    title->plotData[i].g,
                    title->plotData[i].b,
                    title->plotData[i].a);
            }

            // Re-draw the PRESENT area.
            title->srcImage->drawSubRectOn(title->destImage, 73, 1,
                72, 0, 58, 5);
            break;
        }

        case SUBTITLE:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // Blit src top & bottom halves so it expands horiz. center out.
            Image* src = title->srcImage;
            int h = src->height();
            int drawH = title->animStep;
            if (drawH <= h) {
                int y = int(title->rh / 2) + 2;
                int w = src->width();
                int botH = drawH / 2;
                int topH = drawH - botH;    // If odd, top gets extra row.
                src->drawSubRectOn(title->destImage, 1, y-topH, 0, 0, w, topH);
                src->drawSubRectOn(title->destImage, 1, y, 0, h-botH, w, botH);
            }
        }
            break;

        case MAP:
        {
            SCALED_VAR

            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            int step = (title->animStep == title->animStepMax ? title->animStepMax - 1 : title->animStep);

            // blit src to the canvas one row at a time, center out
            title->srcImage->drawSubRectOn(
                title->destImage,
                SCALED( 153-(step*8) ),
                SCALED( 1 ),
                0,
                0,
                SCALED( (step+1) * 8 ),
                SCALED( title->srcImage->height()) );
            title->srcImage->drawSubRectOn(
                title->destImage,
                SCALED( 161 ),
                SCALED( 1 ),
                SCALED( 312-(step*8) ),
                0,
                SCALED( (step+1) * 8 ),
                SCALED( title->srcImage->height()) );

#ifdef GPU_RENDER
            //printf( "KR reveal %d\n", step );
            if (step >= mapArea.columns)
                mapArea.scissor = NULL;
            else {
                int scale = xu4.settings->scale;
                mapScissor[0] = scale * (160 - step * 8);   // Left
                mapScissor[1] = mapArea.screenRect[1];      // Bottom
                mapScissor[2] = scale * (step * 2 * 8);     // Width
                mapScissor[3] = mapArea.screenRect[3];      // Height
                mapArea.scissor = mapScissor;

                if (step == 1) {
                    MAP_ENABLE;
                }
            }
#else
            // create a destimage for the map tiles
            int newtime = getTicks();
            if (newtime > title->timeDuration + 250/4)
            {
                // draw the updated map display
                drawMapStatic();

                xu4.screenImage->drawSubRectOn(title->srcImage,
                    SCALED(8), SCALED(8),
                    SCALED(8), SCALED(13*8),
                    SCALED(38*8), SCALED(10*8));

                title->timeDuration = newtime + 250/4;
            }

            title->srcImage->drawSubRectOn(
                title->destImage,
                SCALED( 161 - (step * 8) ),
                SCALED( 9 ),
                SCALED( 160 - (step * 8) ),
                SCALED( 8 ),
                SCALED( (step * 2) * 8 ),
                SCALED( (10 * 8) ) );
#endif
            break;
        }
    }

    // draw the titles
    drawTitle();

    // if the animation for this title has completed,
    // move on to the next title
    if (title->animStep >= title->animStepMax)
    {
        // free memory that is no longer needed
        compactTitle();
        title++;

        if (title == titles.end())
        {
            // reset the timer to the pre-titles granularity
            xu4.eventHandler->setTimerInterval(1000 /
                                        xu4.settings->gameCyclesPerSecond);

            // make sure the titles only appear when the app first loads
            bSkipTitles = true;

            return false;
        }

        if (title->method == TITLE)
        {
            // assume this is "Ultima IV" and pre-load sound
//            soundLoad(SOUND_TITLE_FADE);
            xu4.eventHandler->setTimerInterval(xu4.settings->titleSpeedRandom);
        }
        else if (title->method == MAP)
        {
            xu4.eventHandler->setTimerInterval(xu4.settings->titleSpeedOther);
        }
        else
        {
            xu4.eventHandler->setTimerInterval(xu4.settings->titleSpeedOther);
        }
    }

    return true;
}


//
// The title element has finished drawing all frames, so
// delete, remove, or free data that is no longer needed
//
void IntroController::compactTitle()
{
    if (title->srcImage)
    {
        delete title->srcImage;
        title->srcImage = NULL;
    }
    title->plotData.clear();
}


//
// Scale the animation canvas, then draw it to the screen
//
void IntroController::drawTitle()
{
#ifdef USE_GL
    title->destImage->drawSubRect(title->rx, title->ry, 1, 1,
                                  title->rw, title->rh);
#else
    Image *scaled;      // the scaled and filtered image
    SCALED_VAR

    // blit the scaled and filtered surface to the screen
    if (title->prescaled)
        scaled = title->destImage;
    else
        scaled = screenScale(title->destImage, xu4.settings->scale, 1, 1);

    scaled->drawSubRect(
        SCALED(title->rx),    // dest x, y
        SCALED(title->ry),
        SCALED(1),              // src x, y, w, h
        SCALED(1),
        SCALED(title->rw),
        SCALED(title->rh));

    if (!title->prescaled)
    {
        delete scaled;
        scaled = NULL;
    }
#endif
}


//
// skip the remaining titles
//
void IntroController::skipTitles()
{
    bSkipTitles = true;
    soundStop();
}

#ifdef IOS
// Try to put the intro music back at just the correct moment on iOS;
// don't play it at the very beginning.
void IntroController::tryTriggerIntroMusic() {
    if (mode == INTRO_MAP)
        musicPlay(introMusic);
}
#endif
