/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "screen.h"
#include "tileanim.h"
#include "xml.h"
#include "u4.h"
#include "utils.h"

using std::map;
using std::vector;

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

    transform = new TileAnimTransform;
    transform->type = (TileAnimTransform::TileAnimTransformType) xmlGetPropAsEnum(node, "type", transformTypeEnumStrings);

    switch (transform->type) {
    case TileAnimTransform::TRANSFORM_INVERT:
        transform->invert.x = xmlGetPropAsInt(node, "x");
        transform->invert.y = xmlGetPropAsInt(node, "y");
        transform->invert.w = xmlGetPropAsInt(node, "width");
        transform->invert.h = xmlGetPropAsInt(node, "height");
        break;

    case TileAnimTransform::TRANSFORM_PIXEL:
        transform->pixel.x = xmlGetPropAsInt(node, "x");
        transform->pixel.y = xmlGetPropAsInt(node, "y");

        for (child = node->xmlChildrenNode; child; child = child->next) {
            if (xmlNodeIsText(child))
                continue;

            if (xmlStrcmp(child->name, (const xmlChar *) "color") == 0) {
                RGBA *rgba = tileAnimColorLoadFromXml(child);
                transform->pixel.colors.push_back(rgba);
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

TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, const std::string &name) {
    map<std::string, TileAnim *>::iterator i = set->tileanims.find(name);
    if (i == set->tileanims.end())
        return NULL;
    return i->second;
}

void tileAnimDraw(TileAnim *anim, Image *tiles, int tile, int scale, int x, int y) {
    RGBA *color;
    
    if (xu4_random(2) == 0)
        return;

    for (vector<TileAnimTransform *>::const_iterator i = anim->transforms.begin(); i != anim->transforms.end(); i++) {
        TileAnimTransform *transform = *i;
        
        switch (transform->type) {
        case TileAnimTransform::TRANSFORM_INVERT:
            imageDrawSubRectInverted(tiles, scale * (x + transform->invert.x), scale * (y + transform->invert.y),
                                     scale * transform->invert.x, tile * (tiles->h / N_TILES) + scale * transform->invert.y, 
                                     scale * transform->invert.w, scale * transform->invert.h);
            break;

        case TileAnimTransform::TRANSFORM_PIXEL:
            color = (RGBA *) transform->pixel.colors[xu4_random(transform->pixel.colors.size())];
            screenFillRect(transform->pixel.x + x, transform->pixel.y + y, 1, 1, color->r, color->g, color->b);
            break;
        }
    }
}
