/*
 * $Id$
 */

#ifndef INTRO_H
#define INTRO_H

#include <string>
#include <vector>

#include "controller.h"
#include "menu.h"
#include "observer.h"
#include "savegame.h"
#include "imageview.h"
#include "textview.h"
#include "tileview.h"

#ifdef GPU_RENDER
#include "map.h"
#endif

class IntroObjectState;
class Tile;

/**
 * Binary data loaded from the U4DOS title.exe file.
 */
class IntroBinData {
public:
    const static int INTRO_TEXT_OFFSET;
    const static int INTRO_MAP_OFFSET;
    const static int INTRO_FIXUPDATA_OFFSET;
    const static int INTRO_SCRIPT_TABLE_SIZE;
    const static int INTRO_SCRIPT_TABLE_OFFSET;
    const static int INTRO_BASETILE_TABLE_SIZE;
    const static int INTRO_BASETILE_TABLE_OFFSET;
    const static int BEASTIE1_FRAMES;
    const static int BEASTIE2_FRAMES;
    const static int BEASTIE_FRAME_TABLE_OFFSET;
    const static int BEASTIE1_FRAMES_OFFSET;
    const static int BEASTIE2_FRAMES_OFFSET;

    IntroBinData();
    ~IntroBinData();

    bool load();

#ifdef GPU_RENDER
    Map introMap;
#else
    std::vector<MapTile> introMap;
#endif
    unsigned char *sigData;
    unsigned char *scriptTable;
    const Tile **baseTileTable;
    unsigned char *beastie1FrameTable;
    unsigned char *beastie2FrameTable;
    std::vector<std::string> introText;
    std::vector<std::string> introQuestions;
    std::vector<std::string> introGypsy;

private:
    // disallow assignments, copy contruction
    IntroBinData(const IntroBinData&);
    const IntroBinData &operator=(const IntroBinData&);
};


/**
 * Controls the title animation sequences, including the traditional
 * plotted "Lord British" signature, the pixelized fade-in of the
 * "Ultima IV" game title, as well as the other more simple animated
 * features, followed by the traditional animated map and "Journey
 * Onward" menu, plus the xU4-specific configuration menu.
 *
 * @todo
 * <ul>
 *      <li>make initial menu a Menu too</li>
 *      <li>get rid of mode and switch(mode) statements</li>
 *      <li>get rid global intro instance -- should only need to be accessed from u4.cpp</li>
 * </ul>
 */
class IntroController : public Controller, public Observer<Menu *, MenuEvent &> {
public:
    IntroController();
    ~IntroController();

    bool init();
    bool hasInitiatedNewGame();

    void deleteIntro();
    bool keyPressed(int key);
    unsigned char *getSigData();
    void updateScreen();
    void timerFired();

    void preloadMap();

    void update(Menu *menu, MenuEvent &event);
    void updateConfMenu(MenuEvent &event);
    void updateVideoMenu(MenuEvent &event);
    void updateGfxMenu(MenuEvent &event);
    void updateSoundMenu(MenuEvent &event);
    void updateInputMenu(MenuEvent &event);
    void updateSpeedMenu(MenuEvent &event);
    void updateGameplayMenu(MenuEvent &event);
    void updateInterfaceMenu(MenuEvent &event);

    //
    // Title methods
    //
    void initTitles();
    bool updateTitle();

private:
    void drawMap();
    void drawMapStatic();
    void drawMapAnimated();
    void drawBeasties();
    void drawBeastie(int beast, int vertoffset, int frame);
    void animateTree(Symbol frame);
    void drawCard(int pos, int card);
    void drawAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);

    void initQuestionTree();
    bool doQuestion(int answer);
    void initPlayers(SaveGame *saveGame);
    std::string getQuestion(int v1, int v2);
#ifdef IOS
public:
    void tryTriggerIntroMusic();
#endif
    void initiateNewGame();
    void finishInitiateGame(const string &nameBuffer, SexType sex);
    void startQuestions();
    void showStory();
    void journeyOnward();
    void about();
#ifdef GPU_RENDER
    void enableMap();
#endif
#ifdef IOS
private:
#endif
    void showText(const string &text);

    void runMenu(Menu *menu, TextView *view, bool withBeasties);

    /**
     * The states of the intro.
     */
    enum {
        INTRO_TITLES,                       // displaying the animated intro titles
        INTRO_MAP,                          // displaying the animated intro map
        INTRO_MENU                          // displaying the main menu: journey onward, etc.
    } mode;

    enum MenuConstants {
        MI_CONF_VIDEO,
        MI_CONF_SOUND,
        MI_CONF_INPUT,
        MI_CONF_SPEED,
        MI_CONF_GAMEPLAY,
        MI_CONF_INTERFACE,
        MI_CONF_01,
        MI_VIDEO_CONF_GFX,
        MI_VIDEO_02,
        MI_VIDEO_03,
        MI_VIDEO_04,
        MI_VIDEO_05,
        MI_VIDEO_06,
        MI_VIDEO_07,
        MI_VIDEO_08,
        MI_GFX_SCHEME,
        MI_GFX_TILE_TRANSPARENCY,
        MI_GFX_TILE_TRANSPARENCY_SHADOW_SIZE,
        MI_GFX_TILE_TRANSPARENCY_SHADOW_OPACITY,
        MI_GFX_RETURN,
        MI_SOUND_01,
        MI_SOUND_02,
        MI_SOUND_03,
        MI_INPUT_01,
        MI_INPUT_02,
        MI_INPUT_03,
        MI_SPEED_01,
        MI_SPEED_02,
        MI_SPEED_03,
        MI_SPEED_04,
        MI_SPEED_05,
        MI_SPEED_06,
        MI_SPEED_07,
        MI_GAMEPLAY_01,
        MI_GAMEPLAY_02,
        MI_GAMEPLAY_03,
        MI_GAMEPLAY_04,
        MI_GAMEPLAY_05,
        MI_GAMEPLAY_06,
        MI_INTERFACE_01,
        MI_INTERFACE_02,
        MI_INTERFACE_03,
        MI_INTERFACE_04,
        MI_INTERFACE_05,
        MI_INTERFACE_06,
        USE_SETTINGS = 0xFE,
        CANCEL = 0xFF
    };

    ImageView backgroundArea;
    TextView menuArea;
    TextView extendedMenuArea;
    TextView questionArea;
    TileView mapArea;

    // menus
    Menu mainMenu;
    Menu confMenu;
    Menu videoMenu;
    Menu gfxMenu;
    Menu soundMenu;
    Menu inputMenu;
    Menu speedMenu;
    Menu gameplayMenu;
    Menu interfaceMenu;

    // data loaded in from title.exe
    IntroBinData *binData;

    // additional introduction state data
    int answerInd;
    int questionRound;
    int questionTree[15];
    int beastie1Cycle;
    int beastie2Cycle;
    int beastieOffset;
    int beastieSub[2];
    bool beastiesVisible;
    int sleepCycles;
    int scrPos;  /* current position in the script table */
#ifdef GPU_RENDER
    int mapScissor[4];
#else
    IntroObjectState *objectStateTable;
#endif
    ImageInfo* beastiesImg;

    bool justInitiatedNewGame;

    //
    // Title defs, structs, methods, and data members
    //
    enum AnimType {
        SIGNATURE,
        AND,
        BAR,
        ORIGIN,
        PRESENT,
        TITLE,
        SUBTITLE,
        MAP
    };

    struct AnimPlot {
        uint8_t x, y;
        uint8_t r, g, b, a;
    };

    struct AnimElement {
        int rx, ry;                         // screen/source x and y
        int rw, rh;                         // source width and height
        AnimType method;                    // render method
        int animStep;                       // tracks animation position
        int animStepMax;
        int timeBase;                       // initial animation time
        int timeDelay;                      // delay before rendering begins
        int timeDuration;                   // total animation time
        Image *srcImage;                    // storage for the source image
        Image *destImage;                   // storage for the animation frame
        std::vector <AnimPlot> plotData;    // plot data
#ifndef USE_GL
        bool prescaled;
#endif
    };

    void addTitle(int x, int y, int w, int h, AnimType method, int delay, int duration);
    void compactTitle();
    void drawTitle();
    void getTitleSourceData();
#ifdef IOS
public:
#endif
    void skipTitles();
#ifdef IOS
private:
#endif
    std::vector<AnimElement> titles;            // list of title elements
    std::vector<AnimElement>::iterator title;   // current title element

    int  introMusic;
    bool bSkipTitles;
};

#endif
