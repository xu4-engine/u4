/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

#include "config.h"
#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "u4.h"
#include "utils.h"

using std::string;
using std::vector;

TileAnimTransform *TileAnimTransform::create(const ConfigElement &conf) {
    TileAnimTransform *transform;
    static const char *transformTypeEnumStrings[] = { "invert", "pixel", "scroll", "frame", "pixel_color", NULL };

    int type = conf.getEnum("type", transformTypeEnumStrings);    

    switch (type) {
    case 0:
        transform = new TileAnimInvertTransform(conf.getInt("x"),
                                                conf.getInt("y"),
                                                conf.getInt("width"),
                                                conf.getInt("height"));
        break;

    case 1: 
        {
            transform = new TileAnimPixelTransform(conf.getInt("x"),
                                                   conf.getInt("y"));

            vector<ConfigElement> children = conf.getChildren();
            for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
                if (i->getName() == "color") {
                    RGBA *rgba = loadColorFromConf(*i);
                    ((TileAnimPixelTransform *)transform)->colors.push_back(rgba);
                }
            }
        }

        break;
    
    case 2:
        transform = new TileAnimScrollTransform();
        break;

    case 3:
        transform = new TileAnimFrameTransform();
        break;

    case 4:
        {
            transform = new TileAnimPixelColorTransform(conf.getInt("x"),
                                                        conf.getInt("y"),
                                                        conf.getInt("width"),
                                                        conf.getInt("height"));

            vector<ConfigElement> children = conf.getChildren();
            for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
                if (i->getName() == "color") {
                    RGBA *rgba = loadColorFromConf(*i);
                    if (i == children.begin())
                        ((TileAnimPixelColorTransform *)transform)->start = rgba;
                    else ((TileAnimPixelColorTransform *)transform)->end = rgba;
                }
            }
        }
    }

    /**
     * See if the transform is performed randomely
     */ 
    if (conf.exists("random"))
        transform->random = conf.getBool("random");
    else transform->random = false;

    return transform;
}

/**
 * Loads a color from a config element
 */ 
RGBA *TileAnimTransform::loadColorFromConf(const ConfigElement &conf) {
    RGBA *rgba;
    
    rgba = new RGBA;
    rgba->r = conf.getInt("red");
    rgba->g = conf.getInt("green");
    rgba->b = conf.getInt("blue");
    rgba->a = IM_OPAQUE;

    return rgba;
}

TileAnimInvertTransform::TileAnimInvertTransform(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

int TileAnimInvertTransform::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {    
    int tx = (x * tile->w) + (BORDER_WIDTH * scale),
        ty = (y * tile->h) + (BORDER_HEIGHT * scale);

    tile->getImage()->drawSubRectInverted(tx + (this->x * scale), ty + (scale * this->y),
                               scale * this->x, (tile->h * mapTile->frame) + (scale * this->y), 
                               w * scale, h * scale);

    return mapTile->frame;
}

TileAnimPixelTransform::TileAnimPixelTransform(int x, int y) {
    this->x = x;
    this->y = y;
}

int TileAnimPixelTransform::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {
    RGBA *color = colors[xu4_random(colors.size())];
    /* unscaled coords */
    int tx = x * (tile->w / scale) + BORDER_WIDTH,
        ty = y * (tile->h / scale) + BORDER_HEIGHT;

    screenFillRect(tx + this->x, ty + this->y, 1, 1, color->r, color->g, color->b);

    return mapTile->frame;
}

int TileAnimScrollTransform::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {
    int offset = screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND * scale;
    Image *image = tile->getImage();

    if (!image) {
        tile->draw(x, y, mapTile->frame);
        image = tile->getImage();
    }

    image->drawSubRect(x * tile->w + (BORDER_WIDTH * scale),
        y * tile->h + (BORDER_HEIGHT * scale) + offset,
        0, mapTile->frame * tile->h, tile->w, tile->h - offset);
    
    if (offset != 0) {
        image->drawSubRect(x * tile->w + (BORDER_WIDTH * scale),
            y * tile->h + (BORDER_HEIGHT * scale),
            0, (mapTile->frame + 1) * tile->h - offset, tile->w, offset);
    }

    return mapTile->frame;
}

/**
 * Advance the frame by one and draw it!
 */ 
int TileAnimFrameTransform::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {
    int newFrame = (mapTile->frame >= tile->frames-1) ? 0 : mapTile->frame + 1;

    tile->draw(x, y, newFrame);
    return newFrame;
}

TileAnimPixelColorTransform::TileAnimPixelColorTransform(int x, int y, int w, int h) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
}

int TileAnimPixelColorTransform::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {
    RGBA diff = *end;
    diff.r -= start->r;
    diff.g -= start->g;
    diff.b -= start->b;  

    int tx = (x * tile->w) + (BORDER_WIDTH * scale),
        ty = (y * tile->h) + (BORDER_HEIGHT * scale);

    extern Image *screen;
    Image *tileImage = tile->getImage();

    for (int j = this->y * scale; j < (this->y * scale) + (h * scale); j++) {
        for (int i = this->x * scale; i < (this->x * scale) + (w * scale); i++) {
            RGBA pixelAt;
            
            tileImage->getPixel(i, j, pixelAt.r, pixelAt.g, pixelAt.b, pixelAt.a);
            if (pixelAt.r >= start->r && pixelAt.r <= end->r &&
                pixelAt.g >= start->g && pixelAt.g <= end->g &&
                pixelAt.b >= start->b && pixelAt.b <= end->b) {
                screen->putPixel(tx + i, ty + j, start->r + xu4_random(diff.r), start->g + xu4_random(diff.g), start->b + xu4_random(diff.b), pixelAt.a);
            }
        }
    }    
    
    return mapTile->frame;
}

TileAnimContext* TileAnimContext::create(const ConfigElement &conf) {
    TileAnimContext *context;
    static const char *contextTypeEnumStrings[] = { "frame", NULL };

    TileAnimContext::Type type = (TileAnimContext::Type)conf.getEnum("type", contextTypeEnumStrings);

    switch(type) {
    case FRAME:
        context = new TileAnimFrameContext(conf.getInt("frame"));
        break;
    default:
        context = NULL;
        break;
    }

    /**
     * Add the transforms to the context
     */ 
    if (context) {        
        vector<ConfigElement> children = conf.getChildren();

        for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
            if (i->getName() == "transform") {
                TileAnimTransform *transform = TileAnimTransform::create(*i);            

                context->add(transform);
            }        
        }
    }

    return context;
}

void TileAnimContext::add(TileAnimTransform* transform) {
    animTransforms.push_back(transform);
}

bool TileAnimContext::isInContext(Tile *t, MapTile *mapTile) {
    return false;
}

TileAnimContext::TileAnimTransformList& TileAnimContext::getTransforms() {
    return animTransforms;
}

TileAnimFrameContext::TileAnimFrameContext(int f) : frame(f) {}
bool TileAnimFrameContext::isInContext(Tile *t, MapTile *mapTile) {
    return (mapTile->frame == frame);
}

/**
 * TileAnimSet
 */ 
TileAnimSet::TileAnimSet(const ConfigElement &conf) {
    name = conf.getString("name");

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "tileanim") {
            TileAnim *anim = new TileAnim(*i);
            tileanims[anim->name] = anim;
        }
    }
}

/**
 * Returns the tile animation with the given name from the current set
 */ 
TileAnim *TileAnimSet::getByName(const std::string &name) {    
    TileAnimMap::iterator i = tileanims.find(name);
    if (i == tileanims.end())
        return NULL;
    return i->second;
}

TileAnim::TileAnim(const ConfigElement &conf) : controls(false), random(false) {
    name = conf.getString("name");
    if (conf.exists("controlling"))
        controls = conf.getBool("controlling");
    if (conf.exists("random"))
        random = conf.getBool("random");
       
    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "transform") {
            TileAnimTransform *transform = TileAnimTransform::create(*i);            

            transforms.push_back(transform);
        }
        else if (i->getName() == "context") {
            TileAnimContext *context = TileAnimContext::create(*i);

            contexts.push_back(context);
        }
    }
}

int TileAnim::draw(Tile *tile, MapTile *mapTile, int scale, int x, int y) {
    int newFrame = mapTile->frame;
    std::vector<TileAnimTransform *>::const_iterator t;
    std::vector<TileAnimContext *>::const_iterator c;

    /* load the tile image if it isn't already loaded */
    if (!tile->image)
        tile->draw(x, y, mapTile->frame);    

    if ((this->random && xu4_random(2) == 0) || (!transforms.size() && !contexts.size())) {
        /* if we control drawing the tile, lets make sure its drawn */
        if (isControlling())
            tile->draw(x, y, mapTile->frame);
        return mapTile->frame;
    }
    
    bool drawn = false;
    
    /**
     * Do global transforms
     */ 
    for (t = transforms.begin(); t != transforms.end(); t++) {
        TileAnimTransform *transform = *t;
        
        if (!transform->isRandom() || xu4_random(2) == 0) {
            newFrame = transform->draw(tile, mapTile, scale, x, y);
            drawn = true;
        }
    }

    /**
     * Do contextual transforms
     */ 
    for (c = contexts.begin(); c != contexts.end(); c++) {
        if ((*c)->isInContext(tile, mapTile)) {
            TileAnimContext::TileAnimTransformList ctx_transforms = (*c)->getTransforms();
            for (t = ctx_transforms.begin(); t != ctx_transforms.end(); t++) {
                TileAnimTransform *transform = *t;

                if (!transform->isRandom() || xu4_random(2) == 0) {        
                    newFrame = transform->draw(tile, mapTile, scale, x, y);
                    drawn = true;
                }
            }
        }
    }

    /**
     * Be sure the tile was actually drawn
     */
    if (isControlling() && !drawn)
        tile->draw(x, y, mapTile->frame);
    
    return newFrame;
}

/**
 * Returns true if the tile animation 
 * is in charge of drawing the tile.
 */ 
bool TileAnim::isControlling() const {
    return controls;
}

/**
 * Returns true if the tile animation
 * is only enacted randomely (50%)
 */ 
bool TileAnim::isRandom() const {
    return random;
}
