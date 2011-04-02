
/*
 * $Id$
 * Copyright 2011 Trenton Schulz. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Trenton Schulz.
 */


// iPad version of the screen functions. 
// It's pretty much a straight copy of the screen SDL version. A lot of this stuff could be better abstracted

#include <algorithm>
#include <functional>
#include <vector>
#include <map>

#include "config.h"
#include "context.h"

#include "debug.h"
#include "dungeonview.h"
#include "error.h"
#include "event.h"
#include "image.h"
#include "imagemgr.h"
#include "intro.h"
#include "savegame.h"
#include "settings.h"
#include "scale.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "dngview.h"
#include "u4.h"
#include "u4file.h"
#include "utils.h"
#import "u4view.h"
#import "ios_helpers.h"
#import "U4CFHelper.h"

using std::vector;

int scale;
Scaler filterScaler;

enum LayoutType {
    LAYOUT_STANDARD,
    LAYOUT_GEM,
    LAYOUT_DUNGEONGEM
};

struct Layout {
    string name;
    LayoutType type;
    struct {
        int width, height;
    } tileshape;
    struct {
        int x, y;
        int width, height;
    } viewport;
};

const struct {
    const char *subimage;
    int ega_x2, ega_y2;
    int vga_x2, vga_y2;
    const char *subimage2;
} dngGraphicInfo[] = {
    { "dung0_lft_ew" },
    { "dung0_lft_ns" },
    { "dung0_mid_ew" },
    { "dung0_mid_ns" },
    { "dung0_rgt_ew" },
    { "dung0_rgt_ns" },
    
    { "dung1_lft_ew", 0, 32, 0, 8, "dung1_xxx_ew" },
    { "dung1_lft_ns", 0, 32, 0, 8, "dung1_xxx_ns" },
    { "dung1_mid_ew" },
    { "dung1_mid_ns" },
    { "dung1_rgt_ew", 144, 32, 160, 8, "dung1_xxx_ew" },
    { "dung1_rgt_ns", 144, 32, 160, 8, "dung1_xxx_ns" },
    
    { "dung2_lft_ew", 0, 64, 0, 48, "dung2_xxx_ew" },
    { "dung2_lft_ns", 0, 64, 0, 48, "dung2_xxx_ns" },
    { "dung2_mid_ew" },
    { "dung2_mid_ns" },
    { "dung2_rgt_ew", 112, 64, 128, 48, "dung2_xxx_ew" },
    { "dung2_rgt_ns", 112, 64, 128, 48, "dung2_xxx_ns" },
    
    { "dung3_lft_ew", 0, 80, 48, 72, "dung3_xxx_ew" },
    { "dung3_lft_ns", 0, 80, 48, 72, "dung3_xxx_ns" },
    { "dung3_mid_ew" },
    { "dung3_mid_ns" },
    { "dung3_rgt_ew", 96, 80, 104, 72, "dung3_xxx_ew" },
    { "dung3_rgt_ns", 96, 80, 104, 72, "dung3_xxx_ns" },
    
    { "dung0_lft_ew_door" },
    { "dung0_lft_ns_door" },
    { "dung0_mid_ew_door" },
    { "dung0_mid_ns_door" },
    { "dung0_rgt_ew_door" },
    { "dung0_rgt_ns_door" },
    
    { "dung1_lft_ew_door", 0, 32, 0, 8, "dung1_xxx_ew" },
    { "dung1_lft_ns_door", 0, 32, 0, 8, "dung1_xxx_ns" },
    { "dung1_mid_ew_door" },
    { "dung1_mid_ns_door" },
    { "dung1_rgt_ew_door", 144, 32, 160, 8, "dung1_xxx_ew" },
    { "dung1_rgt_ns_door", 144, 32, 160, 8, "dung1_xxx_ns" },
    
    { "dung2_lft_ew_door", 0, 64, 0, 48, "dung2_xxx_ew" },
    { "dung2_lft_ns_door", 0, 64, 0, 48, "dung2_xxx_ns" },
    { "dung2_mid_ew_door" },
    { "dung2_mid_ns_door" },
    { "dung2_rgt_ew_door", 112, 64, 128, 48, "dung2_xxx_ew" },
    { "dung2_rgt_ns_door", 112, 64, 128, 48, "dung2_xxx_ns" },
    
    { "dung3_lft_ew_door", 0, 80, 48, 72, "dung3_xxx_ew" },
    { "dung3_lft_ns_door", 0, 80, 48, 72, "dung3_xxx_ns" },
    { "dung3_mid_ew_door" },
    { "dung3_mid_ns_door" },
    { "dung3_rgt_ew_door", 96, 80, 104, 72, "dung3_xxx_ew" },
    { "dung3_rgt_ns_door", 96, 80, 104, 72, "dung3_xxx_ns" },
    
    { "dung0_ladderup" },
    { "dung0_ladderup_side" },
    { "dung1_ladderup" },
    { "dung1_ladderup_side" },
    { "dung2_ladderup" },
    { "dung2_ladderup_side" },
    { "dung3_ladderup" },
    { "dung3_ladderup_side" },
    
    { "dung0_ladderdown" },
    { "dung0_ladderdown_side" },
    { "dung1_ladderdown" },
    { "dung1_ladderdown_side" },
    { "dung2_ladderdown" },
    { "dung2_ladderdown_side" },
    { "dung3_ladderdown" },
    { "dung3_ladderdown_side" },
    
    { "dung0_ladderupdown" },
    { "dung0_ladderupdown_side" },
    { "dung1_ladderupdown" },
    { "dung1_ladderupdown_side" },
    { "dung2_ladderupdown" },
    { "dung2_ladderupdown_side" },
    { "dung3_ladderupdown" },
    { "dung3_ladderupdown_side" },
};

void screenLoadGraphicsFromConf(void);
Layout *screenLoadLayoutFromConf(const ConfigElement &conf);
void screenShowGemTile(Layout *layout, Map *map, MapTile &t, bool focus, int x, int y);

vector<Layout *> layouts;
vector<TileAnimSet *> tileanimSets;
vector<string> gemLayoutNames;
vector<string> filterNames;
vector<string> lineOfSightStyles;
Layout *gemlayout = NULL;
TileAnimSet *tileanims = NULL;
ImageInfo *charsetInfo = NULL;
ImageInfo *gemTilesInfo = NULL;
std::map<string, int> dungeonTileChars;

extern bool verbose;

void screenInit() {
    filterNames.clear();
    filterNames.push_back("point");
    filterNames.push_back("2xBi");
    filterNames.push_back("2xSaI");
    filterNames.push_back("Scale2x");
    
    lineOfSightStyles.clear();
    lineOfSightStyles.push_back("DOS");
    lineOfSightStyles.push_back("Enhanced");
    
    charsetInfo = NULL;
    gemTilesInfo = NULL;
    
    screenLoadGraphicsFromConf();
    
    /* set up scaling parameters */
    scale = settings.scale;
    filterScaler = scalerGet(settings.filter);
    if (!filterScaler)
        errorFatal("%s is not a valid filter", settings.filter.c_str());
    if (verbose)
        printf("using %s scaler\n", settings.filter.c_str());
    
    if (scale < 1 || scale > 5)
        scale = 2;
    
//    U4View *widget = [U4View instance];
//    [[widget window] setWindowTitle:@"Ultima 4"];
    /* if we can't use vga, reset to default:ega */
    if (!u4isUpgradeAvailable() && settings.videoType == "VGA")
        settings.videoType = "EGA";

    
    /* find the tile animations for our tileset */
    tileanims = NULL;
    for (std::vector<TileAnimSet *>::const_iterator i = tileanimSets.begin(); i != tileanimSets.end(); i++) {
        TileAnimSet *set = *i;
        if (set->name == settings.videoType)
            tileanims = set;
    }
    if (!tileanims)
        errorFatal("unable to find tile animations for \"%s\" video mode in graphics.xml", settings.videoType.c_str());
    
    dungeonTileChars.clear();
    dungeonTileChars["brick_floor"] = CHARSET_FLOOR;
    dungeonTileChars["up_ladder"] = CHARSET_LADDER_UP;
    dungeonTileChars["down_ladder"] = CHARSET_LADDER_DOWN;
    dungeonTileChars["up_down_ladder"] = CHARSET_LADDER_UPDOWN;
    dungeonTileChars["chest"] = '$';
    dungeonTileChars["ceiling_hole"] = CHARSET_FLOOR;
    dungeonTileChars["floor_hole"] = CHARSET_FLOOR;
    dungeonTileChars["magic_orb"] = CHARSET_ORB;
    dungeonTileChars["ceiling_hole"] = 'T';
    dungeonTileChars["fountain"] = 'F';
    dungeonTileChars["secret_door"] = CHARSET_SDOOR;
    dungeonTileChars["brick_wall"] = CHARSET_WALL;
    dungeonTileChars["dungeon_door"] = CHARSET_ROOM;
    dungeonTileChars["avatar"] = CHARSET_REDDOT;
}

void screenDelete() {
    std::vector<Layout *>::const_iterator i;
    for (i = layouts.begin(); i != layouts.end(); i++)
        delete(*i);
    layouts.clear();
    if (verbose)
        printf("screen deleted [screenDelete()]\n");
    
    ImageMgr::destroy();
}

/**
 * Re-initializes the screen and implements any changes made in settings
 */
void screenReInit() {
    intro->deleteIntro();       /* delete intro stuff */
    Tileset::unloadAll(); /* unload tilesets */
    screenDelete(); /* delete screen stuff */
    screenInit();   /* re-init screen stuff (loading new backgrounds, etc.) */
    Tileset::loadAll(); /* re-load tilesets */
    intro->init();    /* re-fix the backgrounds loaded and scale images, etc. */
}

/**
 * Attempts to iconify the screen.
 */
void screenIconify() {
}

const vector<string> &screenGetFilterNames() {
    return filterNames;
}

const vector<string> &screenGetGemLayoutNames() {
    return gemLayoutNames;
}

const vector<string> &screenGetLineOfSightStyles() {
    return lineOfSightStyles;
}

void screenLoadGraphicsFromConf() {
    const Config *config = Config::getInstance();
    
    vector<ConfigElement> graphicsConf = config->getElement("graphics").getChildren();
    for (std::vector<ConfigElement>::iterator conf = graphicsConf.begin(); conf != graphicsConf.end(); conf++) {
        
        if (conf->getName() == "layout")
            layouts.push_back(screenLoadLayoutFromConf(*conf));
        else if (conf->getName() == "tileanimset")
            tileanimSets.push_back(new TileAnimSet(*conf));
    }
    
    gemLayoutNames.clear();
    std::vector<Layout *>::const_iterator i;
    for (i = layouts.begin(); i != layouts.end(); i++) {
        Layout *layout = *i;
        if (layout->type == LAYOUT_GEM) {
            gemLayoutNames.push_back(layout->name);
        }
    }
    
    /*
     * Find gem layout to use.
     */
    for (i = layouts.begin(); i != layouts.end(); i++) {
        Layout *layout = *i;
        
        if (layout->type == LAYOUT_GEM && layout->name == settings.gemLayout) {
            gemlayout = layout;
            break;
        }
    }
    if (!gemlayout)
        errorFatal("no gem layout named %s found!\n", settings.gemLayout.c_str());
}

Layout *screenLoadLayoutFromConf(const ConfigElement &conf) {
    Layout *layout;
    static const char *typeEnumStrings[] = { "standard", "gem", "dungeon_gem", NULL };
    
    layout = new Layout;
    layout->name = conf.getString("name");
    layout->type = static_cast<LayoutType>(conf.getEnum("type", typeEnumStrings));
    
    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "tileshape") {
            layout->tileshape.width = i->getInt("width");
            layout->tileshape.height = i->getInt("height");
        }
        else if (i->getName() == "viewport") {
            layout->viewport.x = i->getInt("x");
            layout->viewport.y = i->getInt("y");
            layout->viewport.width = i->getInt("width");
            layout->viewport.height = i->getInt("height");
        }
    }
    
    return layout;
}

/**
 * Draw an image or subimage on the screen.
 */
void screenDrawImage(const string &name, int x, int y) {
    ImageInfo *info = imageMgr->get(name);
    if (info) {
        info->image->draw(x, y);
        return;
    }
    
    SubImage *subimage = imageMgr->getSubImage(name);
    if (subimage) {
        info = imageMgr->get(subimage->srcImageName);
        
        if (info) {
            info->image->drawSubRect(x, y,
                                     subimage->x * (scale / info->prescale),
                                     subimage->y * (scale / info->prescale),
                                     subimage->width * (scale / info->prescale),
                                     subimage->height * (scale / info->prescale));
            return;
        }
    }
    errorFatal("ERROR 1006: Unable to load the image \"%s\".\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", name.c_str(), settings.game.c_str());
}

void screenDrawImageInMapArea(const string &name) {
    ImageInfo *info;
    
    info = imageMgr->get(name);
    if (!info)
        errorFatal("ERROR 1004: Unable to load data files.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", settings.game.c_str());
    
    info->image->drawSubRect(BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             BORDER_WIDTH * scale, BORDER_HEIGHT * scale,
                             VIEWPORT_W * TILE_WIDTH * scale,
                             VIEWPORT_H * TILE_HEIGHT * scale);
}


/**
 * Change the current text color
 */
void screenTextColor(int color) {
    if (charsetInfo == NULL) {
        charsetInfo = imageMgr->get(BKGD_CHARSET);
        if (!charsetInfo)
            errorFatal("ERROR 1003: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_CHARSET, settings.game.c_str());
    }
    
    if (!settings.enhancements || !settings.enhancementsOptions.textColorization) {
        return;
    }
    
    switch (color)
    {
        case FG_GREY:
        case FG_BLUE:
        case FG_PURPLE:
        case FG_GREEN:
        case FG_RED:
        case FG_YELLOW:
        case FG_WHITE:
            charsetInfo->image->setFontColorFG((ColorFG)color);
    }
}

/**
 * Draw a character from the charset onto the screen.
 */
void screenShowChar(int chr, int x, int y) {
    if (charsetInfo == NULL) {
        charsetInfo = imageMgr->get(BKGD_CHARSET);
        if (!charsetInfo)
            errorFatal("ERROR 1001: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_CHARSET, settings.game.c_str());
    }
    
    charsetInfo->image->drawSubRect(x * charsetInfo->image->width(), y * (CHAR_HEIGHT * scale),
                                    0, chr * (CHAR_HEIGHT * scale),
                                    charsetInfo->image->width(), CHAR_HEIGHT * scale);
}

/**
 * Draw a tile graphic on the screen.
 */
void screenShowGemTile(Layout *layout, Map *map, MapTile &t, bool focus, int x, int y) {
    // Make sure we account for tiles that look like other tiles (dungeon tiles, mainly)
    string looks_like = t.getTileType()->getLooksLike();
    if (!looks_like.empty())
        t = map->tileset->getByName(looks_like)->id;
    
    unsigned int tile = map->translateToRawTileIndex(t);
    
    if (map->type == Map::DUNGEON) {
        ASSERT(charsetInfo, "charset not initialized");
        std::map<string, int>::iterator charIndex = dungeonTileChars.find(t.getTileType()->getName());
        if (charIndex != dungeonTileChars.end()) {
            charsetInfo->image->drawSubRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
                                            (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                                            0,
                                            charIndex->second * layout->tileshape.height * scale,
                                            layout->tileshape.width * scale,
                                            layout->tileshape.height * scale);
        }
    }
    else {
        if (gemTilesInfo == NULL) {
            gemTilesInfo = imageMgr->get(BKGD_GEMTILES);
            if (!gemTilesInfo)
                errorFatal("ERROR 1002: Unable to load the \"%s\" data file.\t\n\nIs %s installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_GEMTILES, settings.game.c_str());
        }
        
        if (tile < 128) {
            gemTilesInfo->image->drawSubRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
                                             (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                                             0,
                                             tile * layout->tileshape.height * scale,
                                             layout->tileshape.width * scale,
                                             layout->tileshape.height * scale);
        } else {
            Image *screen = imageMgr->get("screen")->image;
            screen->fillRect((layout->viewport.x + (x * layout->tileshape.width)) * scale,
                             (layout->viewport.y + (y * layout->tileshape.height)) * scale,
                             layout->tileshape.width * scale,
                             layout->tileshape.height * scale,
                             0, 0, 0);
        }
    }
}

/**
 * Scroll the text in the message area up one position.
 */
void screenScrollMessageArea() {
    ASSERT(charsetInfo != NULL && charsetInfo->image != NULL, "charset not initialized!");
    
    Image *screen = imageMgr->get("screen")->image;
    
    screen->drawSubRectOn(screen,
                          TEXT_AREA_X * charsetInfo->image->width(),
                          TEXT_AREA_Y * CHAR_HEIGHT * scale,
                          TEXT_AREA_X * charsetInfo->image->width(),
                          (TEXT_AREA_Y + 1) * CHAR_HEIGHT * scale,
                          TEXT_AREA_W * charsetInfo->image->width(),
                          (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale);
    
    
    screen->fillRect(TEXT_AREA_X * charsetInfo->image->width(),
                     TEXT_AREA_Y * CHAR_HEIGHT * scale + (TEXT_AREA_H - 1) * CHAR_HEIGHT * scale,
                     TEXT_AREA_W * charsetInfo->image->width(),
                     CHAR_HEIGHT * scale,
                     0, 0, 0);
    
    screenRedrawScreen();
}

/**
 * Do the tremor spell effect where the screen shakes.
 */
void screenShake(int iterations) {
    int shakeOffset;
    unsigned short i;
    Image *screen = imageMgr->get("screen")->image;
    Image *bottom;
    
    // the MSVC8 binary was generating a Access Violation when using
    // drawSubRectOn() or drawOn() to draw the screen surface on top
    // of itself.  Occured on settings.scale 2 and 4 only.
    // Therefore, a temporary Image buffer is used to store the area
    // that gets clipped at the bottom.
    
    if (settings.screenShakes) {
        // specify the size of the offset, and create a buffer
        // to store the offset row plus 1
        shakeOffset = 1;
        bottom = Image::create(SCALED(320), SCALED(shakeOffset+1), false, Image::HARDWARE);
        
        for (i = 0; i < iterations; i++) {
            // store the bottom row
            screen->drawOn(bottom, 0, SCALED((shakeOffset+1)-200));
            
            // shift the screen down and make the top row black
            screen->drawSubRectOn(screen, 0, SCALED(shakeOffset), 0, 0, SCALED(320), SCALED(200-(shakeOffset+1)));
            bottom->drawOn(screen, 0, SCALED(200-(shakeOffset)));
            screen->fillRect(0, 0, SCALED(320), SCALED(shakeOffset), 0, 0, 0);
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);
            
            // shift the screen back up, and replace the bottom row
            screen->drawOn(screen, 0, 0-SCALED(shakeOffset));
            bottom->drawOn(screen, 0, SCALED(200-(shakeOffset+1)));
            screenRedrawScreen();
            EventHandler::sleep(settings.shakeInterval);
        }
        // free the bottom row image
        delete bottom;
    }
}

int screenDungeonGraphicIndex(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;
    
    index = 0;
    
    if (type == DNGGRAPHIC_LADDERUP && xoffset == 0)
        return 48 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);
    
    if (type == DNGGRAPHIC_LADDERDOWN && xoffset == 0)
        return 56 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);
    
    if (type == DNGGRAPHIC_LADDERUPDOWN && xoffset == 0)
        return 64 +
        (distance * 2) +
        (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH) ? 1 : 0);
    
    /* FIXME */
    if (type != DNGGRAPHIC_WALL && type != DNGGRAPHIC_DOOR)
        return -1;
    
    if (type == DNGGRAPHIC_DOOR)
        index += 24;
    
    index += (xoffset + 1) * 2;
    
    index += distance * 6;
    
    if (DIR_IN_MASK(orientation, MASK_DIR_SOUTH | MASK_DIR_NORTH))
        index++;
    
    return index;
}

void screenDungeonDrawTile(Tile *tile, int xoffset, int distance, Direction orientation) {
    DungeonView view(BORDER_WIDTH, BORDER_HEIGHT, VIEWPORT_W, VIEWPORT_H);
    
    // Draw the tile to the screen
    view.drawInDungeon(tile, xoffset, distance, orientation, tile->isTiledInDungeon());
}

void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type) {
    int index;
    
    index = screenDungeonGraphicIndex(xoffset, distance, orientation, type);
    if (index == -1)
        return;
    
    int x = 0, y = 0;
    SubImage *subimage = imageMgr->getSubImage(dngGraphicInfo[index].subimage);
    if (subimage) {
        x = subimage->x;
        y = subimage->y;
    }
    
    screenDrawImage(dngGraphicInfo[index].subimage, (BORDER_WIDTH + x) * scale, (BORDER_HEIGHT + y) * scale);
    if (dngGraphicInfo[index].subimage2 != NULL) {
        // FIXME: subimage2 is a horrible hack, needs to be cleaned up
        if (settings.videoType == "EGA")
            screenDrawImage(dngGraphicInfo[index].subimage2, (8 + dngGraphicInfo[index].ega_x2) * scale, (8 + dngGraphicInfo[index].ega_y2) * scale);
        else
            screenDrawImage(dngGraphicInfo[index].subimage2, (8 + dngGraphicInfo[index].vga_x2) * scale, (8 + dngGraphicInfo[index].vga_y2) * scale);
    }
}

/**
 * Force a redraw.
 */
void screenRedrawScreen() {
    [U4IOS::frontU4View() setNeedsDisplay];
}

void screenRedrawMapArea() {
    [U4IOS::frontU4View() setNeedsDisplayInRect:CGRectMake(BORDER_WIDTH * scale, 
                                                        BORDER_HEIGHT * scale, 
                                                        VIEWPORT_W * TILE_WIDTH * scale,
                                                        VIEWPORT_H * TILE_HEIGHT * scale)];
}

void screenEraseMapArea() {
    Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(BORDER_WIDTH * scale,
                     BORDER_WIDTH * scale,
                     VIEWPORT_W * TILE_WIDTH * scale,
                     VIEWPORT_H * TILE_HEIGHT * scale,
                     0, 0, 0);
}

void screenEraseTextArea(int x, int y, int width, int height) {
    Image *screen = imageMgr->get("screen")->image;
    screen->fillRect(x * CHAR_WIDTH * scale,
                     y * CHAR_HEIGHT * scale,
                     width * CHAR_WIDTH * scale,
                     height * CHAR_HEIGHT * scale,
                     0, 0, 0);
}

void screenRedrawTextArea(int x, int y, int width, int height) {
    [U4IOS::frontU4View() setNeedsDisplayInRect:CGRectMake(x * CHAR_WIDTH * scale, 
                                                        y * CHAR_HEIGHT * scale,
                                                        width * CHAR_WIDTH * scale,
                                                        height * CHAR_HEIGHT * scale)];
}

Layout *screenGetGemLayout(const Map *map) {
    if (map->type == Map::DUNGEON) {
        std::vector<Layout *>::const_iterator i;
        for (i = layouts.begin(); i != layouts.end(); i++) {
            Layout *layout = *i;
            
            if (layout->type == LAYOUT_DUNGEONGEM)
                return layout;
        }
        errorFatal("no dungeon gem layout found!\n");
        return NULL;
    }
    else
        return gemlayout;
}

void screenGemUpdate() {
    MapTile tile;
    int x, y;
    Image *screen = imageMgr->get("screen")->image;
    
    screen->fillRect(BORDER_WIDTH * scale,
                     BORDER_HEIGHT * scale,
                     VIEWPORT_W * TILE_WIDTH * scale,
                     VIEWPORT_H * TILE_HEIGHT * scale,
                     0, 0, 0);
    
    Layout *layout = screenGetGemLayout(c->location->map);
    for (x = 0; x < layout->viewport.width; x++) {
        for (y = 0; y < layout->viewport.height; y++) {
            bool focus;
            tile = screenViewportTile(layout->viewport.width, layout->viewport.height, x, y, focus).front();
            screenShowGemTile(layout, c->location->map, tile, focus, x, y);
        }
    }
    screenRedrawMapArea();
    
    screenUpdateCursor();
    screenUpdateMoons();
    screenUpdateWind();
}

static void releaseScaleData(void *info, const void *data, size_t size) {
    delete [] static_cast<const char *>(data);
}

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered
 * seperately. filter determines whether or not to filter the
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    if (scale == 1)
        return Image::duplicate(src);

    src->createCachedImage();
    CGLayerRef srcLayer = src->getSurface();
    CGSize srcSize = CGLayerGetSize(srcLayer);
    int srcWidth = int(srcSize.width);
    int srcHeight = int(srcSize.height);
    const int bytesPerLine = ((srcWidth * 4) + 15) & ~15;
    boost::intrusive_ptr<CGDataProvider> dataProvider = cftypeFromCreateOrCopy(
                                            CGDataProviderCreateWithData(0, src->cachedImageData,
                                                                         srcHeight * bytesPerLine,
                                                                         releaseScaleData));
    boost::intrusive_ptr<CGColorSpace> colorSpace = cftypeFromCreateOrCopy(CGColorSpaceCreateDeviceRGB());
    boost::intrusive_ptr<CGImage> cgimage = cftypeFromCreateOrCopy(
                                                        CGImageCreate(srcWidth, srcHeight,
                                                                      8, 32, bytesPerLine,
                                                                      colorSpace.get(),
                                                                      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault,
                                                                      dataProvider.get(),
                                                                      0, true,
                                                                      kCGRenderingIntentDefault));
    // The CGImage owns the cachedImageData now, so reset it to zero.
    src->cachedImageData = 0;
    
    Image *dest = Image::create(srcWidth * scale, srcHeight * scale, false, Image::HARDWARE);
    CGLayerRef destLayer = dest->getSurface();
    CGContextRef destContext = CGLayerGetContext(destLayer);
    CGInterpolationQuality oldQuality = CGContextGetInterpolationQuality(destContext);
    CGContextSetInterpolationQuality(destContext, kCGInterpolationHigh);
    CGContextDrawImage(destContext, CGRectMake(0., 0., srcWidth * scale, srcHeight * scale), cgimage.get());
    CGContextSetInterpolationQuality(destContext, oldQuality);
    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    bool isTransparent;
    unsigned int transparentIndex;
    bool alpha = src->isAlphaOn();
    
    isTransparent = src->getTransparentIndex(transparentIndex);
    
    src->alphaOff();
    
    dest = Image::create(src->width() / scale, src->height() / scale, src->isIndexed(), Image::HARDWARE);
    if (!dest)
        return NULL;
    
    if (dest->isIndexed())
        dest->setPaletteFromImage(src);
    
    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }
    
    if (isTransparent)
        dest->setTransparentIndex(transparentIndex);
    
    if (alpha)
        src->alphaOn();
    
    return dest;
}
