#include "controller.h"

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
    //virtual void timerFired();

private:
    StringTable modList;
    TxfHeader* txf[2];
    uint32_t   fontTexture;
    uint32_t   sel;
    int        listenerId;
    int16_t    listArea[4];
    int16_t    okArea[4];
    int16_t    cancelArea[4];

    static void renderBrowser(ScreenState* ss, void* data);
    static void displayReset(int, void*, void*);
};
