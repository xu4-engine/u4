/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "list.h"
#include "tileanim.h"
#include "screen.h"
#include "xml.h"
#include "u4.h"
#include "utils.h"

TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node) {
    TileAnimSet *set;
    xmlNodePtr child;

    set = (TileAnimSet *) malloc(sizeof(TileAnimSet));
    memset(set, 0, sizeof(TileAnimSet));
    set->name = xmlGetPropAsStr(node, "name");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "tileanim") == 0) {
            TileAnim *anim = tileAnimLoadFromXml(child);
            set->tileanims = listAppend(set->tileanims, anim);
        }
    }

    return set;
}

TileAnim *tileAnimLoadFromXml(xmlNodePtr node) {
    TileAnim *anim;
    xmlNodePtr child;

    anim = (TileAnim *) malloc(sizeof(TileAnim));
    memset(anim, 0, sizeof(TileAnim));
    anim->name = xmlGetPropAsStr(node, "name");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "transform") == 0) {
            TileAnimTransform *transform = tileAnimTransformLoadFromXml(child);
            anim->transforms = listAppend(anim->transforms, transform);
        }
    }

    return anim;
}

TileAnimTransform *tileAnimTransformLoadFromXml(xmlNodePtr node) {
    TileAnimTransform *transform;
    xmlNodePtr child;
    static const char *transformTypeEnumStrings[] = { "invert", "pixel", NULL };

    transform = (TileAnimTransform *) malloc(sizeof(TileAnimTransform));
    memset(transform, 0, sizeof(TileAnimTransform));
    transform->type = xmlGetPropAsEnum(node, "type", transformTypeEnumStrings);

    switch (transform->type) {
    case TRANSFORM_INVERT:
        transform->invert.x = xmlGetPropAsInt(node, "x");
        transform->invert.y = xmlGetPropAsInt(node, "y");
        transform->invert.w = xmlGetPropAsInt(node, "width");
        transform->invert.h = xmlGetPropAsInt(node, "height");
        break;
    case TRANSFORM_PIXEL:
        transform->pixel.x = xmlGetPropAsInt(node, "x");
        transform->pixel.y = xmlGetPropAsInt(node, "y");

        for (child = node->xmlChildrenNode; child; child = child->next) {
            if (xmlNodeIsText(child))
                continue;

            if (xmlStrcmp(child->name, (const xmlChar *) "color") == 0) {
                RGBA *rgba = tileAnimColorLoadFromXml(child);
                transform->pixel.colors = listAppend(transform->pixel.colors, rgba);
            }
        }

        break;
    }

    return transform;
}

RGBA *tileAnimColorLoadFromXml(xmlNodePtr node) {
    RGBA *rgba;
    
    rgba = (RGBA *) malloc(sizeof(RGBA));
    rgba->r = xmlGetPropAsInt(node, "red");
    rgba->g = xmlGetPropAsInt(node, "green");
    rgba->b = xmlGetPropAsInt(node, "blue");
    rgba->a = IM_OPAQUE;

    return rgba;
}

TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, const char *name) {
    ListNode *node;
    for (node = set->tileanims; node; node = node->next) {
        TileAnim *anim = (TileAnim *) node->data;
        if (strcmp(anim->name, name) == 0) {
            return anim;
        }
    }
    return NULL;
}

void tileAnimDraw(TileAnim *anim, Image *tiles, int tile, int scale, int x, int y) {
    ListNode *node, *colorNode;
    RGBA *color;
    int i;

    if (xu4_random(2) == 0)
        return;

    for (node = anim->transforms; node; node = node->next) {
        TileAnimTransform *transform = (TileAnimTransform *) node->data;

        switch (transform->type) {
        case TRANSFORM_INVERT:
            imageDrawSubRectInverted(tiles, scale * (x + transform->invert.x), scale * (y + transform->invert.y),
                                     scale * transform->invert.x, tile * (tiles->h / N_TILES) + scale * transform->invert.y, 
                                     scale * transform->invert.w, scale * transform->invert.h);
            break;

        case TRANSFORM_PIXEL:
            colorNode = transform->pixel.colors;
            for (i = xu4_random(listLength(transform->pixel.colors)); i > 0; i--)
                colorNode = colorNode->next;
            color = (RGBA *) colorNode->data;

            screenFillRect(transform->pixel.x + x, transform->pixel.y + y, 1, 1, color->r, color->g, color->b);
            break;
        }
    }
}
