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

TileAnim::TileAnim(const ConfigElement &conf) {
    name = conf.getString("name");

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "transform") {
            TileAnimTransform *transform = TileAnimTransform::create(*i);
            transforms.push_back(transform);
        }
    }
}

TileAnimTransform *TileAnimTransform::create(const ConfigElement &conf) {
    TileAnimTransform *transform;
    static const char *transformTypeEnumStrings[] = { "invert", "pixel", NULL };

    int type = conf.getEnum("type", transformTypeEnumStrings);

    switch (type) {
    case 0:
        transform = new TileAnimInvertTransform(conf.getInt("x"),
                                                conf.getInt("y"),
                                                conf.getInt("width"),
                                                conf.getInt("height"));
        break;

    case 1:
        transform = new TileAnimPixelTransform(conf.getInt("x"),
                                               conf.getInt("y"));

        vector<ConfigElement> children = conf.getChildren();
        for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
            if (i->getName() == "color") {
                RGBA *rgba = tileAnimColorLoadFromConf(*i);
                ((TileAnimPixelTransform *)transform)->colors.push_back(rgba);
            }
        }

        break;
    }

    return transform;
}

RGBA *tileAnimColorLoadFromConf(const ConfigElement &conf) {
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

void TileAnimInvertTransform::draw(Image *tiles, int tile, int scale, int x, int y) {
    tiles->drawSubRectInverted(scale * (x + this->x), scale * (y + this->y),
                               scale * this->x, tile * (tiles->height() / N_TILES) + scale * this->y, 
                               scale * w, scale * h);
}

TileAnimPixelTransform::TileAnimPixelTransform(int x, int y) {
    this->x = x;
    this->y = y;
}

void TileAnimPixelTransform::draw(Image *tiles, int tile, int scale, int x, int y) {
    RGBA *color = colors[xu4_random(colors.size())];
    screenFillRect(this->x + x, this->y + y, 1, 1, color->r, color->g, color->b);
}

TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, const string &name) {
    std::map<string, TileAnim *>::iterator i = set->tileanims.find(name);
    if (i == set->tileanims.end())
        return NULL;
    return i->second;
}

void TileAnim::draw(Image *tiles, int tile, int scale, int x, int y) {
    if (xu4_random(2) == 0)
        return;

    for (std::vector<TileAnimTransform *>::const_iterator i = transforms.begin(); i != transforms.end(); i++) {
        TileAnimTransform *transform = *i;
        
        transform->draw(tiles, tile, scale, x, y);

#if 0
        switch (transform->type) {
        case TileAnimTransform::TRANSFORM_INVERT:
            tiles->drawSubRectInverted(scale * (x + transform->invert.x), scale * (y + transform->invert.y),
                                       scale * transform->invert.x, tile * (tiles->h / N_TILES) + scale * transform->invert.y, 
                                       scale * transform->invert.w, scale * transform->invert.h);
            break;

        case TileAnimTransform::TRANSFORM_PIXEL:
            color = (RGBA *) transform->pixel.colors[xu4_random(transform->pixel.colors.size())];
            screenFillRect(transform->pixel.x + x, transform->pixel.y + y, 1, 1, color->r, color->g, color->b);
            break;
        }
#endif
    }
}
