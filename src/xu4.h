/*
 * xu4.h
 */

#include "notify.h"
#include "stringTable.h"

enum NotifySender {
    // Sender Id           Message
    SENDER_LOCATION,    // MoveEvent*
    SENDER_PARTY,       // PartyEvent*
    SENDER_AURA,        // Aura*
    SENDER_MENU,        // MenuEvent*
    SENDER_SETTINGS,    // Settings*
    SENDER_DISPLAY      // NULL or ScreenState*
};

enum DrawLayer {
    LAYER_CPU_BLIT,     // For legacy screenImage rendering.
    LAYER_MAP,          // When GPU_RENDER defined.
    LAYER_HUD,          // Borders and status GUI.
    LAYER_TOP_MENU,     // GameBrowser

    LAYER_COUNT
};

class Settings;
class Config;
class ImageMgr;
struct Screen;
class Image;
class EventHandler;
struct SaveGame;
class GameBrowser;
class IntroController;
class GameController;

enum XU4GameStage {
    StageExitGame,
    StageIntro,
    StagePlay
};

struct XU4GameServices {
    StringTable resourcePaths;
    NotifyBus notifyBus;
    Settings* settings;
    Config* config;
    ImageMgr* imageMgr;
    Screen* screen;
    void* screenSys;
    void* gpu;
    Image* screenImage;
    EventHandler* eventHandler;
    SaveGame* saveGame;
    GameBrowser* gameBrowser;
    IntroController* intro;
    GameController* game;
    const char* errorMessage;
    uint16_t stage;
    uint16_t gameReset;         // Load another game.
    uint32_t randomFx[17];      // Effects random number generator state.
};

extern XU4GameServices xu4;

#define gs_listen(msk,func,user)    notify_listen(&xu4.notifyBus,msk,func,user)
#define gs_unplug(id)               notify_unplug(&xu4.notifyBus,id)
#define gs_emitMessage(sid,data)    notify_emit(&xu4.notifyBus,sid,data);

void xu4_selectGame();
extern "C" int xu4_random(int upperval);
extern "C" int xu4_randomFx(int upperval);
