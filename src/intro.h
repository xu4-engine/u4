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

struct IntroObjectState;
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

    MapTile *introMap;
    unsigned char *sigData;
    unsigned char *scriptTable;
    Tile **baseTileTable;
    unsigned char *beastie1FrameTable;
    unsigned char *beastie2FrameTable;
    std::vector<std::string> introText;
    std::vector<std::string> introQuestions;
    std::vector<std::string> introGypsy;
};

/**
 * Controls the intro sequence, including the traditional animated map
 * and "Journey Onward" menu, plus the xu4 specific configuration
 * menu.
 * @todo
 * <ul>
 *      <li>make initial menu a Menu too</li>
 *      <li>get rid of mode and switch(mode) statements</li>
 *      <li>get rid global intro instance -- should only need to be accessed from u4.cpp</li>
 *      <li>animate the lord british signature</li>
 * </ul>
 */
class IntroController : public Controller, public Observer<Menu *, MenuEvent &> {
public:
    IntroController();

    bool init();
    void deleteIntro();
    bool keyPressed(int key);
    unsigned char *getSigData();
    void updateScreen();
    void timerFired();

    void update(Menu *menu, MenuEvent &event);
    void updateConfMenu(MenuEvent &event);
    void updateVideoMenu(MenuEvent &event);
    void updateSoundMenu(MenuEvent &event);
    void updateGameplayMenu(MenuEvent &event);
    void updateAdvancedMenu(MenuEvent &event);
    void updateEnhancementMenu(MenuEvent &event);
    void updateKeyboardMenu(MenuEvent &event);
    void updateSpeedMenu(MenuEvent &event);

private:
    void drawMap();
    void drawMapAnimated();
    void drawBeasties();
    void drawBeastie(int beast, int vertoffset, int frame);
    void animateTree(const string &frame);
    void drawCard(int pos, int card);
    void drawAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);

    void initQuestionTree();
    bool doQuestion(int answer);
    void initPlayers(SaveGame *saveGame);
    std::string getQuestion(int v1, int v2);

    void initiateNewGame();
    void startQuestions();
    void showStory();
    void journeyOnward();
    void about();

    void showText(const string &text);

    void runMenu(Menu *menu, TextView *view, bool withBeasties);

    /**
     * The states of the intro.
     */
    enum {
        INTRO_MAP,                          /* displaying the animated intro map */
        INTRO_MENU                          /* displaying the main menu: journey onward, etc. */
    } mode;

    enum MenuConstants {
        VIDEO_MENU,
        SOUND_MENU,
        GAMEPLAY_MENU,
        ADVANCED_MENU,
        SPEED_MENU,
        KEYBOARD_MENU,
        ENHANCEMENT_MENU,
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
    Menu soundMenu;
    Menu gameplayMenu;
    Menu advancedMenu;
    Menu keyboardMenu;
    Menu speedMenu;
    Menu enhancementMenu;

    // data loaded in from title.exe
    IntroBinData *binData;

    // additional introduction state data
    std::string errorMessage;
    int answerInd;
    int questionRound;
    int questionTree[15];
    int beastie1Cycle;
    int beastie2Cycle;
    int beastieOffset;
    bool beastiesVisible;
    int sleepCycles;
    int scrPos;  /* current position in the script table */
    IntroObjectState *objectStateTable;
};

extern IntroController *intro;

#endif
