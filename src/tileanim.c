/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "list.h"
#include "tileanim.h"
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
    static const char *transformTypeEnumStrings[] = { "invert", NULL };

    transform = (TileAnimTransform *) malloc(sizeof(TileAnimTransform));
    memset(transform, 0, sizeof(TileAnimTransform));
    transform->type = xmlGetPropAsEnum(node, "type", transformTypeEnumStrings);
    transform->x = xmlGetPropAsInt(node, "x");
    transform->y = xmlGetPropAsInt(node, "y");
    transform->w = xmlGetPropAsInt(node, "width");
    transform->h = xmlGetPropAsInt(node, "height");

    return transform;
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
    ListNode *node;

    if (xu4_random(2) == 0)
        return;

    for (node = anim->transforms; node; node = node->next) {
        TileAnimTransform *transform = (TileAnimTransform *) node->data;

        switch (transform->type) {
        case TRANSFORM_INVERT:
            imageDrawSubRectInverted(tiles, x + scale * transform->x, y + scale * transform->y,
                                     scale * transform->x, tile * (tiles->h / N_TILES) + scale * transform->y, 
                                     scale * transform->w, scale * transform->h);
        }
    }
}
