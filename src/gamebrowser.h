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
    TxfHeader* txf;
    uint32_t   fontTexture;
    uint32_t   sel;

    static void renderBrowser(ScreenState* ss, void* data);
};
