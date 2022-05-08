#include <cstring>
#include "config.h"
#include "event.h"
#include "image32.h"
#include "gpu.h"
#include "screen.h"
#include "txf_draw.h"
#include "u4file.h"
#include "xu4.h"

#include "gamebrowser.h"

extern "C" {
#include "processDir.c"
}


#define GUI_LIST    0
#define ATTR_COUNT  7

void GameBrowser::renderBrowser(ScreenState* ss, void* data)
{
    GameBrowser* gb = (GameBrowser*) data;

    gpu_viewport(0, 0, ss->displayW, ss->displayH);
    gpu_drawGui(xu4.gpu, GUI_LIST, gb->fontTexture);

    if (gb->modList.used) {
        int box[4];
        box[0] = 40;
        box[1] = 320 - int(gb->txf->lineHeight * 20.0f) * (gb->sel + 1)
                     + int(gb->txf->descender * 20.0f);
        box[2] = 240;
        box[3] = 20;
        gpu_setScissor(box);
        gpu_invertColors(xu4.gpu);
        gpu_setScissor(NULL);
    }
}

GameBrowser::GameBrowser()
{
    txf = (TxfHeader*) xu4.config->loadFile("cfont-comfortaa.txf");
    if (txf)
        fontTexture = gpu_loadTexture("cfont.png", 1);
    else
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
    if (! txf)
        return false;

    sst_init(&modList, 8, 128);
    readModuleList(&modList);

    TxfDrawState ds;
    float* attr;
    int quadCount;

    attr = gpu_beginTris(xu4.gpu, GUI_LIST);

    txf_begin(&ds, txf, 60.0f, 20.0f, 20.0f);
    quadCount = txf_genText(&ds, attr + 3, attr, ATTR_COUNT,
                            (const uint8_t*) "xu4 | Ultima 4", 14);
    attr += quadCount * 6 * ATTR_COUNT;

    ds.x = 20.0f;
    ds.y = 340.0f;
    ds.psize = 32.0f;
    quadCount = txf_genText(&ds, attr + 3, attr, ATTR_COUNT,
                            (const uint8_t*) "Game Modules", 12);
    attr += quadCount * 6 * ATTR_COUNT;

    ds.y = 320.0f;
    ds.psize = 20.0f;
    const char* modStrings = sst_strings(&modList);
    StringEntry* it  = modList.table;
    StringEntry* end = it + modList.used;
    for (; it != end; ++it) {
        ds.x = 40.0f;
        ds.y -= txf->lineHeight * ds.psize;
        quadCount = txf_genText(&ds, attr + 3, attr, ATTR_COUNT,
                                (const uint8_t*) modStrings + it->start,
                                it->len);
        attr += quadCount * 6 * ATTR_COUNT;
    }

    gpu_endTris(xu4.gpu, GUI_LIST, attr);

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
