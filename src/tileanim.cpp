/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "xml.h"
#include "u4.h"
#include "utils.h"

using std::string;

TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node) {
    TileAnimSet *set;
    xmlNodePtr child;

    set = new TileAnimSet;
    set->name = xmlGetPropAsString(node, "name");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "tileanim") == 0) {
            TileAnim *anim = tileAnimLoadFromXml(child);
            set->tileanims[anim->name] = anim;
        }
    }

    return set;
}

TileAnim *tileAnimLoadFromXml(xmlNodePtr node) {
    TileAnim *anim;
    xmlNodePtr child;

    anim = new TileAnim;    
    anim->name = xmlGetPropAsString(node, "name");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "transform") == 0) {
            TileAnimTransform *transform = tileAnimTransformLoadFromXml(child);
            anim->transforms.push_back(transform);            
        }
    }

    return anim;
}

TileAnimTransform *tileAnimTransformLoadFromXml(xmlNodePtr node) {
    TileAnimTransform *transform;
    xmlNodePtr child;
    static const char *transformTypeEnumStrings[] = { "invert", "pixel", NULL };

    int type = xmlGetPropAsEnum(node, "type", transformTypeEnumStrings);

    switch (type) {
    case 0:
        transform = new TileAnimInvertTransform(xmlGetPropAsInt(node, "x"),
                                                xmlGetPropAsInt(node, "y"),
                                                xmlGetPropAsInt(node, "width"),
                                                xmlGetPropAsInt(node, "height"));
        break;

    case 1:
        transform = new TileAnimPixelTransform(xmlGetPropAsInt(node, "x"),
                                               xmlGetPropAsInt(node, "y"));

        for (child = node->xmlChildrenNode; child; child = child->next) {
            if (xmlNodeIsText(child))
                continue;

            if (xmlStrcmp(child->name, (const xmlChar *) "color") == 0) {
                RGBA *rgba = tileAnimColorLoadFromXml(child);
                ((TileAnimPixelTransform *)transform)->colors.push_back(rgba);
            }
        }

        break;
    }

    return transform;
}

RGBA *tileAnimColorLoadFromXml(xmlNodePtr node) {
    RGBA *rgba;
    
    rgba = new RGBA;
    rgba->r = xmlGetPropAsInt(node, "red");
    rgba->g = xmlGetPropAsInt(node, "green");
    rgba->b = xmlGetPropAsInt(node, "blue");
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
                               scale * this->x, tile * (tiles->h / N_TILES) + scale * this->y, 
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
