/*
 * xu4.h
 */

#include "notify.h"

enum NotifySender {
    // Sender Id           Message
    SENDER_LOCATION,    // MoveEvent*
    SENDER_PARTY,       // PartyEvent*
    SENDER_AURA,        // Aura*
    SENDER_MENU,        // MenuEvent*
    SENDER_SETTINGS     // Settings*
};

class Settings;
class Config;
class ImageMgr;
struct Screen;
class Image;
class EventHandler;
struct SaveGame;
class IntroController;
class GameController;

enum XU4GameStage {
    StageExitGame,
    StageIntro,
    StagePlay
};

struct XU4GameServices {
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
    IntroController* intro;
    GameController* game;
    const char* errorMessage;
    int stage;
};

extern XU4GameServices xu4;

#define gs_listen(msk,func,user)    notify_listen(&xu4.notifyBus,msk,func,user)
#define gs_unplug(id)               notify_unplug(&xu4.notifyBus,id)
#define gs_emitMessage(sid,data)    notify_emit(&xu4.notifyBus,sid,data);
