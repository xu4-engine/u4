/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "tileanim.h"
#include "xml.h"
#include "u4.h"
#include "utils.h"

TileAnimSet *tileAnimSetLoadFromXml(xmlNodePtr node) {
    TileAnimSet *set;
    xmlNodePtr child;

    set = new TileAnimSet;    
    set->name = xmlGetPropAsStr(node, "name");

    for (child = node->xmlChildrenNode; child; child = child->next) {
        if (xmlNodeIsText(child))
            continue;

        if (xmlStrcmp(child->name, (const xmlChar *) "tileanim") == 0) {
            TileAnim *anim = tileAnimLoadFromXml(child);
            set->tileanims.insert(TileAnimSetAnimMap::value_type(anim->name, anim));            
        }
    }

    return set;
}

TileAnim *tileAnimLoadFromXml(xmlNodePtr node) {
    TileAnim *anim;
    xmlNodePtr child;

    anim = new TileAnim;    
    anim->name = xmlGetPropAsStr(node, "name");

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
    static const char *transformTypeEnumStrings[] = { "invert", NULL };

    transform = new TileAnimTransform;
    memset(transform, 0, sizeof(TileAnimTransform));
    transform->type = (TileAnimTransformType)xmlGetPropAsEnum(node, "type", transformTypeEnumStrings);
    transform->x = xmlGetPropAsInt(node, "x");
    transform->y = xmlGetPropAsInt(node, "y");
    transform->w = xmlGetPropAsInt(node, "width");
    transform->h = xmlGetPropAsInt(node, "height");

    return transform;
}

TileAnim *tileAnimSetGetAnimByName(TileAnimSet *set, string name) {
    TileAnimSetAnimMap::iterator found = set->tileanims.find(name);
    if (found != set->tileanims.end())
        return found->second;
    return NULL;
}

void tileAnimDraw(TileAnim *anim, Image *tiles, int tile, int scale, int x, int y) {
    TileAnimTransformList::iterator i;
    
    if (xu4_random(2) == 0)
        return;

    for (i = anim->transforms.begin(); i != anim->transforms.end(); i++) {
        TileAnimTransform *transform = *i;
        
        switch (transform->type) {
        case TRANSFORM_INVERT:
            imageDrawSubRectInverted(tiles, x + scale * transform->x, y + scale * transform->y,
                                     scale * transform->x, tile * (tiles->h / N_TILES) + scale * transform->y, 
                                     scale * transform->w, scale * transform->h);
        }
    }
}
