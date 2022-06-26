#include "controller.h"

struct ModuleInfo {
    StringTable modi;
    uint8_t resPathI;
    uint8_t modFileI;
    uint8_t category;
    uint8_t parent;
};

struct TxfHeader;
struct ScreenState;

class GameBrowser : public Controller {
public:
    GameBrowser();
    ~GameBrowser();

    /* controller functions */
    virtual bool present();
    virtual void conclude();
    virtual bool keyPressed(int key);
    virtual bool inputEvent(const InputEvent*);

private:
    StringTable modFiles;
    StringTable modFormat;
    std::vector<ModuleInfo> infoList;
    TxfHeader* txf[3];
    uint32_t   fontTexture;
    int        listenerId;
    uint16_t   sel;
    uint16_t   selMusic;        // 0 = none
    int16_t    listArea[4];
    int16_t    okArea[4];
    int16_t    quitArea[4];
    int16_t    cancelArea[4];

    void selectModule(const int16_t* rect, int y);
    void layout();

    static void renderBrowser(ScreenState* ss, void* data);
    static void displayReset(int, void*, void*);
};
