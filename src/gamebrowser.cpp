#include <cstring>
#include "config.h"
#include "event.h"
#include "image32.h"
#include "gpu.h"
#include "gui.h"
#include "settings.h"
#include "screen.h"
#include "txf_draw.h"
#include "u4file.h"
#include "xu4.h"

#include "gamebrowser.h"

extern "C" {
#include "processDir.c"
}


#define GUI_LIST    0
#define PSIZE_LIST  20

void GameBrowser::renderBrowser(ScreenState* ss, void* data)
{
    GameBrowser* gb = (GameBrowser*) data;

    //gpu_viewport(0, 0, ss->displayW, ss->displayH);
    gpu_drawGui(xu4.gpu, GUI_LIST, gb->fontTexture);

    if (gb->modList.used) {
        int box[4];
        float selY = gb->txf[0]->lineHeight * PSIZE_LIST * (gb->sel + 1.0f);

        box[0] = gb->listArea[0];
        box[1] = gb->listArea[1] + gb->listArea[3] - 1 - int(selY);
        box[0] += (ss->displayW - ss->aspectW) / 2;
        box[1] += (ss->displayH - ss->aspectH) / 2;

        box[2] = gb->listArea[2];
        box[3] = PSIZE_LIST + 2;
        gpu_setScissor(box);
        gpu_invertColors(xu4.gpu);
        gpu_setScissor(NULL);
    }
}

/*
 * Load textured font metrics.
 *
 * Return zero if any files failed to load.
 */
static int loadFonts(const char** files, int txfCount, TxfHeader** txfArr)
{
    int i;
    for (i = 0; i < txfCount; ++i) {
        txfArr[i] = (TxfHeader*) xu4.config->loadFile(*files++);
        if (! txfArr[i]) {
            int fn;
            for (fn = 0; fn < i; ++fn) {
                free(txfArr[fn]);
                txfArr[fn] = NULL;
            }
            return 0;
        }
    }
    return txfCount;
}

static const uint8_t clut[6*4*2] = {
    // black  white  chocolate4  burlywoord4
    0,0,0,255, 255,255,255,255, 139,69,19,255, 139,115,85,255,
    // royal-blue1  dodger-blue1
    72,118,255,255, 24,116,205,255,

    // semi-transparent
    0,0,0,128, 255,255,255,128, 139,69,19,128, 139,115,85,128,
    72,118,255,128, 24,116,205,128
};

static const char* fontFiles[] = {
    "cfont.png",
    "cfont-comfortaa.txf",
    "cfont-avatar.txf"
};

/*
 * Reload any GPU data.  As this notification should not occur when the
 * browser is open, we don't handle GUI layout here.
 */
void GameBrowser::displayReset(int sender, void* eventData, void* user)
{
    GameBrowser* gb = (GameBrowser*) user;
    //ScreenState* ss = (ScreenState*) eventData;

    if (eventData) {
        gb->fontTexture = gpu_loadTexture(fontFiles[0], 1);
        if (gb->fontTexture) {
            Image32 cimg;
            cimg.pixels = (uint32_t*) clut;
            cimg.w = 6*2;
            cimg.h = 1;
            gpu_blitTexture(gb->fontTexture, 0, 0, &cimg);
        }
    } else {
        gpu_freeTexture(gb->fontTexture);
    }
}

GameBrowser::GameBrowser()
{
    txf[0] = NULL;
    fontTexture = 0;
    sel = 0;

    if (! loadFonts(fontFiles+1, 2, txf))
        return;

    displayReset(SENDER_DISPLAY, (void*) screenState(), this);
    listenerId = gs_listen(1<<SENDER_DISPLAY, displayReset, this);
}

GameBrowser::~GameBrowser()
{
    if (fontTexture)
        gpu_freeTexture(fontTexture);

    if (txf[0]) {
        gs_unplug(listenerId);
        free(txf[0]);
        free(txf[1]);
    }
}

// Return position of ".mod" extension or 0 if none.
static int modExtension(const char* name, int* slen)
{
    int len = strlen(name);
    *slen = len;
    if (len > 4 && strcmp(name + len - 4, ".mod") == 0)
        return len - 4;
    return 0;
}

static int collectModFiles(const char* name, int type, void* user)
{
    if (type == PDIR_FILE || type == PDIR_LINK) {
        int len;
        if (modExtension(name, &len))
            sst_append((StringTable*) user, name, -1);
    }
    return PDIR_CONTINUE;
}

static void readModuleList(StringTable* modList)
{
    const StringTable* st = &xu4.resourcePaths;
    const char* paths = sst_strings(st);
    for (uint32_t i = 0; i < st->used; ++i)
        processDir(paths + sst_start(st, i), collectModFiles, modList);
}

bool GameBrowser::present()
{
    static uint8_t browserGui[] = {
        LAYOUT_V, BG_COLOR_CI, 6,
        MARGIN_V_PER, 10, MARGIN_H_PER, 16, SPACING_PER, 12,
        BG_COLOR_CI, 2,
        MARGIN_V_PER, 6,
            LAYOUT_H,
                FONT_SIZE, 40, LABEL_DT_S,
                FONT_N, 1,     LABEL_DT_S,
            LAYOUT_END,
            FONT_N, 0, FONT_SIZE, PSIZE_LIST, LIST_DT_ST, STORE_DT_AREA,
            FROM_BOTTOM,
            FONT_N, 1, FONT_SIZE, 24,
            LAYOUT_H, SPACING_PER, 20, FIX_WIDTH_EM, 50,
                BUTTON_DT_S, STORE_DT_AREA,
                BUTTON_DT_S, STORE_DT_AREA,
            LAYOUT_END,
        LAYOUT_END
    };
    const void* guiData[8];
    const void** data = guiData;

    if (! fontTexture)
        return false;

    sst_init(&modList, 8, 128);
    readModuleList(&modList);

    browserGui[15] = 16 * xu4.settings->scale;

    *data++ = "xu4 | ";
    *data++ = "Game Modules";
    *data++ = &modList;
    *data++ = listArea;
    *data++ = "Play";
    *data++ = okArea;
    *data++ = "Cancel";
    *data   = cancelArea;
    gui_layout(GUI_LIST, NULL, txf, browserGui, guiData);

    screenSetLayer(LAYER_TOP_MENU, renderBrowser, this);
    return true;
}

void GameBrowser::conclude()
{
    screenSetLayer(LAYER_TOP_MENU, NULL, NULL);
    sst_free(&modList);
}

// Compare names ignoring any ".mod" extension.
static bool equalGameName(const char* a, const char* b)
{
    int lenA, lenB;
    int modA = modExtension(a, &lenA);
    int modB = modExtension(b, &lenB);
    if (modA)
        lenA = modA;
    if (modB)
        lenB = modB;
    return ((lenA == lenB) && strncmp(a, b, lenA) == 0);
}

bool GameBrowser::keyPressed(int key)
{
    switch (key) {
        case U4_ENTER:
        {
            int len;
            const char* game = sst_stringL(&modList, sel, &len);
            if (equalGameName(xu4.settings->game, game)) {
                xu4.eventHandler->setControllerDone(true);
            } else {
                xu4.settings->setGame(game);
                xu4.settings->write();
                xu4.eventHandler->quitGame();
                xu4.gameReset = 1;
            }
        }
            return true;

        case U4_UP:
            if (sel > 0)
                --sel;
            return true;

        case U4_DOWN:
            if (sel < modList.used - 1)
                ++sel;
            return true;

        case U4_ESC:
            xu4.eventHandler->setControllerDone(true);
            return true;
    }
    return false;
}

static bool insideArea(const int16_t* rect, int x, int y)
{
    if (x < rect[0] || y < rect[1])
        return false;
    return (x < (rect[0] + rect[2]) && y < (rect[1] + rect[3]));
}

bool GameBrowser::inputEvent(const InputEvent* ev)
{
    switch (ev->type) {
        case CIE_MOUSE_PRESS:
            if (ev->n == CMOUSE_LEFT) {
                int y = screenState()->displayH - ev->y;
                if (insideArea(cancelArea, ev->x, y))
                    keyPressed(U4_ESC);
                else if (insideArea(okArea, ev->x, y))
                    keyPressed(U4_ENTER);
            }
            break;

        case CIE_MOUSE_WHEEL:
            if (ev->y < 0)
                keyPressed(U4_DOWN);
            else if (ev->y > 0)
                keyPressed(U4_UP);
            break;
    }
    return true;
}
