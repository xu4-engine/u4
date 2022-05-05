#include <cstring>
#include "config.h"
#include "event.h"
#include "image32.h"
#include "gpu.h"
#include "gui.h"
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

    gpu_viewport(0, 0, ss->displayW, ss->displayH);
    gpu_drawGui(xu4.gpu, GUI_LIST, gb->fontTexture);

    if (gb->modList.used) {
        int box[4];
        box[0] = gb->listArea[0];
        box[1] = gb->listArea[1] + gb->listArea[3] - 1
                    - int(gb->txf->lineHeight * PSIZE_LIST * (gb->sel + 1.0f));
        box[2] = gb->listArea[2];
        box[3] = PSIZE_LIST + 2;
        gpu_setScissor(box);
        gpu_invertColors(xu4.gpu);
        gpu_setScissor(NULL);
    }
}

GameBrowser::GameBrowser()
{
    txf = (TxfHeader*) xu4.config->loadFile("cfont-comfortaa.txf");
    if (txf) {
        fontTexture = gpu_loadTexture("cfont.png", 1);

        static const uint8_t clut[6*4*2] = {
            // black  white  chocolate4  burlywoord4
            0,0,0,255, 255,255,255,255, 139,69,19,255, 139,115,85,255,
            // royal-blue1  dodger-blue1
            72,118,255,255, 24,116,205,255,

            // semi-transparent
            0,0,0,128, 255,255,255,128, 139,69,19,128, 139,115,85,128,
            72,118,255,128, 24,116,205,128
        };
        Image32 cimg;
        cimg.pixels = (uint32_t*) clut;
        cimg.w = 6*2;
        cimg.h = 1;
        gpu_blitTexture(fontTexture, 0, 0, &cimg);
    } else
        fontTexture = 0;
    sel = 0;
}

GameBrowser::~GameBrowser()
{
    if (txf) {
        free(txf);
        gpu_freeTexture(fontTexture);
    }
}

static int collectModFiles(const char* name, int type, void* user)
{
    if (type == PDIR_FILE || type == PDIR_LINK) {
        int len = strlen(name);
        if (len > 4 && strcmp(name + len - 4, ".mod") == 0)
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
    static const uint8_t browserGui[] = {
        LAYOUT_V, BG_COLOR_CI, 6,
        MARGIN_V_PER, 10, MARGIN_H_PER, 16, SPACING_PER, 12,
        BG_COLOR_CI, 2,
            FONT_SIZE, 40, LABEL_DT_S,
            FONT_SIZE, PSIZE_LIST, LIST_DT_ST, STORE_DT_AREA,
            LAYOUT_H, MARGIN_H_PER, 25, FIX_WIDTH_PER, 36,
                FONT_SIZE, 24,
                BUTTON_DT_S, STORE_DT_AREA,
                FROM_RIGHT,
                BUTTON_DT_S, STORE_DT_AREA,
            LAYOUT_END,
        LAYOUT_END
    };
    const void* guiData[8];
    const void** data = guiData;

    if (! txf)
        return false;

    sst_init(&modList, 8, 128);
    readModuleList(&modList);

    *data++ = "xu4 | Game Modules";
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

bool GameBrowser::keyPressed(int key)
{
    switch (key) {
        case U4_ENTER:
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
