#include <math.h>
#include <cstring>
#include <algorithm>

#include "config.h"
#include "event.h"
#include "image32.h"
#include "intro.h"
#include "gpu.h"
#include "module.h"
#include "settings.h"
#include "screen.h"
#include "sound.h"
#include "u4file.h"
#include "xu4.h"

#include "gamebrowser.h"

extern "C" {
#include "processDir.c"
}


#define ATTR_COUNT  7

enum GuiBufferRegions {
    REGION_PANEL,
    REGION_LIST
};

void GameBrowser::renderBrowser(ScreenState* ss, void* data)
{
    GameBrowser* gb = (GameBrowser*) data;
    WorkBuffer* work = gb->work;

    if (work->dirty)
        gpu_updateWorkBuffer(xu4.gpu, GPU_DLIST_GUI, work);

    gpu_enableGui(xu4.gpu, gb->buttonDown, gb->buttonMode);
    gpu_drawTrisRegion(xu4.gpu, GPU_DLIST_GUI, work->region + REGION_PANEL);

    if (gb->modFormat.used) {
        const GuiArea* area = gb->gbox + WI_LIST;
        int box[4];

        gb->animateScroll();

        // Draw list items.
        box[0] = ss->aspectX + area->x;
        box[1] = ss->aspectY + area->y;
        box[2] = area->x2 - area->x;
        box[3] = area->y2 - area->y;

        gpu_setScissor(box);
        gpu_guiSetOrigin(xu4.gpu, area->x, area->y2 + gb->listScroll);
        gpu_drawTrisRegion(xu4.gpu, GPU_DLIST_GUI, work->region + REGION_LIST);
        gpu_guiSetOrigin(xu4.gpu, 0.0f, 0.0f);
        gpu_setScissor(NULL);
    }
}

GameBrowser::GameBrowser()
{
    int wsize[2];

    sel = selMusic = 0;
    buttonMode = 0;
    buttonDown = WID_NONE;
    listScroll = listScrollTarget = 0.0f;
    psizeList = 20.0f;
    atree = NULL;

    wsize[0] = ATTR_COUNT * 6 * 400;
    wsize[1] = ATTR_COUNT * 6 * 400;
    work = gpu_allocWorkBuffer(wsize, 2);
}

GameBrowser::~GameBrowser()
{
    gpu_freeWorkBuffer(work);
}

void GameBrowser::animateScroll()
{
    if (listScrollTarget != listScroll) {
        listScroll += (listScrollTarget - listScroll) * 0.2f;
        if (fabs(listScrollTarget - listScroll) < 0.5f)
            listScroll = listScrollTarget;
    }
}

float GameBrowser::calcScrollTarget()
{
    const GuiArea* area = gbox + WI_LIST;
    float areaH = area->y2 - area->y;
    float targ = (itemHeightP * sel) - (areaH * 0.5f);
    if (targ < 0.0f) {
        targ = 0.0f;
    } else {
        float endY = (itemHeightP * modFiles.used) - descenderList;
        if (endY < areaH) {
            targ = 0.0f;    // All items fit in view; don't scroll.
        } else {
            endY -= areaH;
            if (targ > endY)
                targ = endY;
        }
    }
    return listScrollTarget = targ;
}

#define NO_PARENT   255

struct ModuleSortContext {
    const StringTable* st;

    bool operator()(const ModuleInfo& a, const ModuleInfo& b) const
    {
        if (a.modFileI == b.parent)
            return true;

        const char* nameA;
        const char* nameB;
        const char* files = sst_strings(st);
        int fi;

        if (a.parent == b.parent) {
            if (a.category != b.category)
                return a.category < b.category;

            // Compare module names.
            nameA = files + sst_start(st, a.modFileI);
            nameB = files + sst_start(st, b.modFileI);
        } else {
            // Compare names of parent modules.
            fi = (a.parent == NO_PARENT) ? a.modFileI : a.parent;
            nameA = files + sst_start(st, fi);
            fi = (b.parent == NO_PARENT) ? b.modFileI : b.parent;
            nameB = files + sst_start(st, fi);
        }
        /*
        printf("KR name %d (%d %d) %s  %d (%d %d) %s\n",
                a.modFileI, a.parent, a.category, nameA,
                b.modFileI, b.parent, b.category, nameB);
        */
        return strcmp(nameA, nameB) < 0;
    }
};

static int collectModFiles(const char* name, int type, void* user)
{
    if (type == PDIR_FILE || type == PDIR_LINK) {
        int len;
        if (mod_extension(name, &len))
            sst_append((StringTable*) user, name, len);
    }
    return PDIR_CONTINUE;
}

static bool isExtensionOf(const char* name, const char* /*version*/,
                          const StringTable* childModi)
{
    int len;
    const char* rules = sst_stringL(childModi, MI_RULES, &len);
    const char* pver = (const char*) memchr(rules, '/', len);
    return (pver && memcmp(name, rules, pver - rules) == 0);
}

//#define TEST_LIST
#ifdef TEST_LIST
#define TEST_COUNT 6
static const char* testFile[TEST_COUNT] = {
    "Dark_Magic.mod",
    "Dark_Magic-Onward!.mod",
    "DM-SomeSoundtrack.mod",
    "DM-Music_1.mod",
    "DM-Music_2.mod",
    "Light_Magic.mod"
};
static const char testCategory[TEST_COUNT] = {
    MOD_BASE,
    MOD_EXTENSION,
    MOD_SOUNDTRACK,
    MOD_SOUNDTRACK,
    MOD_SOUNDTRACK,
    MOD_BASE
};
static const char* testInfo[TEST_COUNT*4] = {
    // about, author, rules, version
    "Dark Magic the adventure.\nIn the mists of time...",
    "xu4 developers", "Dark-Magic", "1.0",

    "The Dark Magic adventure continues...",
    "xu4 developers", "Dark_Magic/1.0", "1.0",

    "Dark Magic alternative music by Some Composer",
    "Some Composer", "Dark_Magic/1.0", "1.0",

    "Dark Magic alternative music by Another Composer",
    "Another Composer", "Dark_Magic/1.0", "1.0",

    "Dark Magic alternative music by Another Composer",
    "Another Composer", "Dark_Magic/1.0", "2.0",

    "Light Magic the adventure.\nIn the mists of time...",
    "xu4 developers", "Dark-Magic", "1.0-beta"
};

static int test_query(int n, StringTable* modInfo)
{
    int i = n * 4;
    int end = i + 4;
    for (; i < end; ++i)
        sst_append(modInfo, testInfo[i], -1);
    return testCategory[n];
}
#endif

/*
 * Fill modFiles with module names and infoList with sorted information.
 * The modFormat strings match the infoList order and are edited for display
 * in the list widget.
 */
static void readModuleList(StringTable* modFiles, StringTable* modFormat,
                           std::vector<ModuleInfo>& infoList)
{
    char modulePath[256];
    ModuleInfo info;
    const StringTable* rp = &xu4.resourcePaths;
    const char* rpath;
    const char* files;
    uint32_t i, m;
#ifdef TEST_LIST
    uint32_t testM;
#endif
    int len;

    // Collect .mod files from resourcePaths.

    for (i = 0; i < rp->used; ++i) {
        m = modFiles->used;
        rpath = sst_stringL(rp, i, &len);
        processDir(rpath, collectModFiles, modFiles);

#ifdef TEST_LIST
        testM = modFiles->used;
        if (i == 0) {
            for (int t = 0; t < TEST_COUNT; ++t)
                sst_append(modFiles, testFile[t], -1);
        }
#endif

        memcpy(modulePath, rpath, len);
        modulePath[len] = '/';
        files = sst_strings(modFiles);
        for (; m < modFiles->used; ++m) {
            strcpy(modulePath + len + 1, files + sst_start(modFiles, m));

            sst_init(&info.modi, 4, 80);
            info.resPathI = i;
            info.modFileI = m;
#ifdef TEST_LIST
            if (m >= testM)
                info.category = test_query(m - testM, &info.modi);
            else
#endif
            info.category = mod_query(modulePath, &info.modi);
            info.parent   = NO_PARENT;

            if (info.category == MOD_UNKNOWN)
                sst_free(&info.modi);
            else
                infoList.push_back(info);
        }
    }

    // Assign parents.

    files = sst_strings(modFiles);
    for (const auto& it : infoList) {
        if (it.category == MOD_BASE) {
            const char* name = files + sst_start(modFiles, it.modFileI);
            //const char* version = sst_stringL(it.modi, MI_VERSION, &len);

            for (auto& child : infoList) {
                if (child.category == MOD_BASE)
                    continue;
                if (isExtensionOf(name, NULL, &child.modi))
                    child.parent = it.modFileI;
            }
        }
    }

    // Build infoList with children sorted in alphabetical order under their
    // parents.

    {
    ModuleSortContext sortCtx;
    sortCtx.st = modFiles;
    sort(infoList.begin(), infoList.end(), sortCtx);
    }

    // Create modFormat strings from infoList.  The .mod suffixes are removed
    // and child names are indented.

    const int indentLen = 4;
    strcpy(modulePath, "    ");

    for (const auto& it : infoList) {
        //printf("KR module %d %d/%d %s\n", it.category, it.modFileI, it.parent,
        //       files + sst_start(modFiles, it.modFileI));

        rpath = files + sst_start(modFiles, it.modFileI);

        // Strip .mod suffix.
        len = sst_len(modFiles, it.modFileI) - 4;

        memcpy(modulePath + indentLen, rpath, len);
        char* itemText;

        if (it.category == MOD_BASE) {
            itemText = modulePath + indentLen;
        } else {
            // Indent child modules.
            itemText = modulePath;
            len += indentLen;
            if (it.category == MOD_SOUNDTRACK) {
#if 1
                // Green musical note symbol.
                memcpy(modulePath + len,
                       " \x12\x02\x13\x21N\x12\x00\x13\x00", 10);
                len += 10;
#else
                memcpy(modulePath + len, " (music)", 8);
                len += 8;
#endif
            }
        }

        // Add module version number.
        {
        int vlen;
        const char* version = sst_stringL(&it.modi, MI_VERSION, &vlen);
        char* vp = itemText + len;
        *vp++ = '\t';
        *vp++ = 'v';
        memcpy(vp, version, vlen);
        len += vlen + 2;
        }

        sst_append(modFormat, itemText, len);
    }
}

void GameBrowser::layout()
{
    static uint8_t browserGui[] = {
        LAYOUT_V, BG_COLOR_CI, COL_BLACK + 128,
        MARGIN_V_PER, 10, MARGIN_H_PER, 16, SPACING_PER, 6,
        BG_COLOR_CI, COL_BROWN,
        ARRAY_DT_AREA, WI_LIST,
        MARGIN_V_PER, 6,
            LAYOUT_H,
                FONT_COLOR, COL_BEIGE,
                FONT_VSIZE, 38, LABEL_DT_S,
                FONT_N, 1,      LABEL_DT_S,
            LAYOUT_END,
            FONT_N, 0, FONT_VSIZE, 20, LIST_DIM, 32, 9, STORE_AREA,
            FROM_BOTTOM,
            FONT_N, 1, FONT_VSIZE, 22,
            LAYOUT_H, SPACING_PER, 10, FIX_WIDTH_EM, 50,
                BUTTON_DT_S, STORE_AREA,
                BUTTON_DT_S, STORE_AREA,
                BUTTON_DT_S, STORE_AREA,
            LAYOUT_END,
        LAYOUT_END
    };
    const void* guiData[6];
    const void** data = guiData;
    const ScreenState* ss = screenState();

    // Set psizeList to match list FONT_VSIZE.
    psizeList = 20.0f * ss->aspectH / 480.0f;
    descenderList = ss->fontTable[0]->descender * psizeList;
    itemHeightP = lineHeight * psizeList;

    *data++ = gbox;
    *data++ = "xu4 | ";
    *data++ = "Game Modules";
    *data++ = "Play";
    *data++ = "Quit";
    *data   = "Cancel";

    TxfDrawState ds;
    ds.fontTable = ss->fontTable;
    float* attr = gpu_beginRegion(work, REGION_PANEL);
    attr = gui_layout(attr, NULL, &ds, browserGui, guiData);
    gpu_endRegion(work, REGION_PANEL, attr);

    free(atree);
    atree = gui_areaTree(gbox, WI_COUNT);

    generateListItems();
}

/*
 * Generate primitives for items in the module list.
 * The top of the list is at 0,0 to be placed with gpu_guiSetOrigin().
 */
void GameBrowser::generateListItems()
{
    ListDrawState ds;
    const GuiArea* area = gbox + WI_LIST;
    float* attr;
    float rect[4], uvs[4];

    ds.fontTable = screenState()->fontTable;
    txf_begin(&ds, 0, psizeList, 0.0f, 0.0f);

    // Highlight for selected item background.
    rect[0] = 0.0f;
    rect[1] = ds.lineSpacing * -(sel + 1) + descenderList;
    rect[2] = area->x2 - area->x;
    rect[3] = ds.lineSpacing;

    gpu_guiClutUV(xu4.gpu, uvs, COL_LT_BLUE);
    uvs[2] = uvs[0];
    uvs[3] = uvs[1];

    ListCellStyle* cell = ds.cell;
    cell->tabStop   = 0.0f;
    cell->fontScale = 1.0f;
    cell->color     = COL_WHITE;
    cell->selColor  = COL_TX_BLACK;
    ++cell;
    cell->tabStop   = rect[2] - (psizeList * 2.0f);
    cell->fontScale = 0.66f;
    cell->color     = COL_LT_GRAY;
    cell->selColor  = COL_TX_BLACK;

    ds.psizeList = psizeList;

    attr = gpu_beginRegion(work, REGION_LIST);
    attr = gpu_emitQuad(attr, rect, uvs);
    attr = gui_emitListItems(attr, &ds, &modFormat, sel);

    if (selMusic) {
        // Draw green checkmark.
        ds.x = 0.0f;
        ds.y = ds.lineSpacing * -(selMusic + 1.0f);

        ds.tf = ds.fontTable[2];
        txf_setFontSize(&ds, psizeList);
        ds.colorIndex = COL_LT_GREEN;

        int quads = txf_genText(&ds, attr + 3, attr, ATTR_COUNT,
                                (const uint8_t*) "c", 1);
        attr += quads * 6 * ATTR_COUNT;
    }
    gpu_endRegion(work, REGION_LIST, attr);
}

bool GameBrowser::present()
{
    lineHeight = screenState()->fontTable[0]->lineHeight;
    screenSetMouseCursor(MC_DEFAULT);
    screenShowMouseCursor(true);

    sst_init(&modFiles, 8, 128);
    sst_init(&modFormat, 8, 50);
    readModuleList(&modFiles, &modFormat, infoList);

    // Select the current modules.
    {
    int len;
    sel = selMusic = 0;

    for (size_t n = 0; n < infoList.size(); ++n) {
        const char* mod = sst_stringL(&modFiles, infoList[n].modFileI, &len);
        if (infoList[n].category == MOD_SOUNDTRACK) {
            if (mod_namesEqual(xu4.settings->soundtrack, mod))
                selMusic = n;
        } else if (mod_namesEqual(xu4.settings->game, mod)) {
            sel = n;
        }
    }
    }

    layout();
    listScroll = calcScrollTarget();
    screenSetLayer(LAYER_TOP_MENU, renderBrowser, this);
    return true;
}

void GameBrowser::conclude()
{
    free(atree);
    atree = NULL;

    screenSetLayer(LAYER_TOP_MENU, NULL, NULL);
    sst_free(&modFiles);
    sst_free(&modFormat);

    for (auto& it : infoList)
        sst_free(&it.modi);
    infoList.clear();

    if (! xu4.settings->mouseOptions.enabled)
        screenShowMouseCursor(false);
}

bool GameBrowser::keyPressed(int key)
{
    switch (key) {
        case U4_ENTER:
        {
            int len;
            const char* game;
            const char* music = "";

            game = sst_stringL(&modFiles, infoList[sel].modFileI, &len);
            if (infoList[sel].category == MOD_SOUNDTRACK) {
                music = game;
                game = sst_stringL(&modFiles, infoList[sel].parent, &len);
            } else if (selMusic) {
                int par = infoList[selMusic].parent;
                if (par == infoList[sel].modFileI ||
                    par == infoList[sel].parent) {
                    music = sst_stringL(&modFiles,
                                        infoList[selMusic].modFileI, &len);
                }
            }
            //printf( "KR Game '%s' '%s'\n", game, music);

            if (mod_namesEqual(xu4.settings->game, game)) {
                if (mod_namesEqual(xu4.settings->soundtrack, music)) {
                    xu4.eventHandler->setControllerDone(true);
                } else {
                    musicStop();
                    xu4.settings->setSoundtrack(music);
                    xu4.settings->write();
                    xu4.config->changeSoundtrack(music);
                    if (xu4.stage == StagePlay)
                        musicPlayLocale();
                    else
                        musicPlay(xu4.intro->selectedMusic());
                    xu4.eventHandler->setControllerDone(true);
                }
            } else {
                xu4.settings->setGame(game);
                xu4.settings->setSoundtrack(music);
                xu4.settings->write();
                xu4.eventHandler->quitGame();
                xu4.gameReset = 1;
            }
        }
            return true;

        case U4_SPACE:
            if (infoList[sel].category == MOD_SOUNDTRACK) {
                selMusic = (selMusic == sel) ? 0 : sel;
                soundPlay(SOUND_UI_CLICK);
                generateListItems();
            }
            return true;

        case U4_UP:
            if (sel > 0) {
                --sel;
                soundPlay(SOUND_UI_TICK);
                calcScrollTarget();
                generateListItems();
            }
            return true;

        case U4_DOWN:
            if (sel < modFormat.used - 1) {
                ++sel;
                soundPlay(SOUND_UI_TICK);
                calcScrollTarget();
                generateListItems();
            }
            return true;

        case U4_ESC:
            xu4.eventHandler->setControllerDone(true);
            return true;
    }
    return false;
}

void GameBrowser::selectModule(const GuiArea* area, int screenY)
{
    float y = screenY - listScroll - descenderList;
    float row = (float) (area->y2 - y) / itemHeightP;
    int n = (int) row;
    if (n >= 0 && n < (int) modFormat.used) {
        if (infoList[n].category == MOD_SOUNDTRACK) {
            // Toggle selected soundrack.
            if (selMusic == n) {
                selMusic = 0;
            } else {
                selMusic = n;
                /*
                do {
                    --n;
                } while (n && n != sel && infoList[n].category != MOD_BASE);
                sel = n;
                */
            }
            soundPlay(SOUND_UI_CLICK);
            generateListItems();
        } else if (sel != n) {
            sel = n;
            soundPlay(SOUND_UI_TICK);
            calcScrollTarget();
            generateListItems();
        }
    }
}

// Translate event mouse position to used area of screen.
static void screenMousePos(const InputEvent* ev, int& x, int& y)
{
    const ScreenState* ss = screenState();
    x = ev->x - ss->aspectX;
    y = (ss->displayH - ev->y) - ss->aspectY;
}

bool GameBrowser::inputEvent(const InputEvent* ev)
{
    int x, y;

    switch (ev->type) {
        case IE_MOUSE_PRESS:
        case IE_MOUSE_RELEASE:
            if (ev->n == CMOUSE_LEFT) {
                screenMousePos(ev, x, y);
                const GuiArea* hit = gui_pick(atree, gbox, x, y);
                if (hit) {
                    if (ev->type == IE_MOUSE_PRESS) {
                        buttonDown = hit->wid;
                        buttonMode = 1;

                        if (hit->wid == WI_LIST)
                            selectModule((const GuiArea*) hit, y);
                    } else {
                        if (buttonDown == hit->wid) {
                            // NOTE: Sound is only played on Cancel to avoid
                            //       abrupt cutoff during reset/quit.
                            switch (hit->wid) {
                            case WI_OK:
                                keyPressed(U4_ENTER);
                                break;
                            case WI_CANCEL:
                                soundPlay(SOUND_UI_CLICK);
                                keyPressed(U4_ESC);
                                break;
                            case WI_QUIT:
                                xu4.eventHandler->quitGame();
                                break;
                            }
                        }
                        buttonDown = WID_NONE;
                    }
                } else {
                    buttonDown = WID_NONE;
                }
            }
            break;

        case IE_MOUSE_MOVE:
            if (buttonDown >= WI_OK) {
                const GuiArea* box = gbox + buttonDown;
                screenMousePos(ev, x, y);
                if (x >= box->x && x < box->x2 &&
                    y >= box->y && y < box->y2)
                    buttonMode = 1;
                else
                    buttonMode = 0;
            }
            break;

        case IE_MOUSE_WHEEL:
            if (ev->y < 0) {
                const GuiArea* area = gbox + WI_LIST;
                float areaH = area->y2 - area->y;
                float endY = (itemHeightP * modFiles.used) - descenderList;
                endY -= areaH;
                listScrollTarget += itemHeightP * 2.0f;     // Scroll down.
                if (listScrollTarget > endY)
                    listScrollTarget = endY;
            } else if (ev->y > 0) {
                listScrollTarget -= itemHeightP * 2.0f;     // Scroll up.
                if (listScrollTarget < 0.0f)
                    listScrollTarget = 0.0f;
            }
            break;
    }
    return true;
}
