/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "tile.h"

#include "config.h"
#include "context.h"
#include "creature.h"
#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "location.h"
#include "settings.h"
#include "tileanim.h"
#include "tilemap.h"
#include "tileset.h"
#include "utils.h"

/**
 * Loads tile information.
 */
void Tile::loadProperties(const ConfigElement &conf) {
    if (conf.getName() != "tile")
        return;
            
    name = conf.getString("name"); /* get the name of the tile */

    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation")) {
        extern TileAnimSet *tileanims;
        string animation = conf.getString("animation");

        anim = tileanims->getByName(animation);
        if (anim == NULL)
            errorWarning("Warning: animation style '%s' not found", animation.c_str());        
    }
    else
        anim = NULL;    

    /* see if the tile is opaque */
    opaque = conf.getBool("opaque"); 

    /* find the rule that applies to the current tile, if there is one.
       if there is no rule specified, it defaults to the "default" rule */
    if (conf.exists("rule")) {
        rule = TileRule::findByName(conf.getString("rule"));
        if (rule == NULL)
            rule = TileRule::findByName("default");
    }
    else rule = TileRule::findByName("default");

    /* get the number of frames the tile has */    
    frames = conf.getInt("frames", 1);
    
    /* FIXME: This is a hack to address the fact that there are 4
       frames for the guard in VGA mode, but only 2 in EGA. Is there
       a better way to handle this? */
    if (name == "guard" && settings.videoType == "EGA")
        frames = 2;    

    /* get the name of the image that belongs to this tile */
    if (conf.exists("image"))
        imageName = conf.getString("image");
    else 
        imageName = string("tile_") + name;

    /* get the index, if it is provided.  Otherwise, it is implied */
    index = conf.getInt("index", -1);

    large = conf.getBool("large");
}

/**
 * Returns the tile at the corresponding index of the current tileset
 */ 
MapTile Tile::translate(int index, string tileMap) {    
    TileMap *im = TileMap::get(tileMap);
    if (im)
        return im->translate(index);

    return MapTile();
}

int Tile::getIndex() const {
    return index + frame;
}

Image *Tile::getImage() { 
    if (!image)
        loadImage();
    return image;
}

bool Tile::isLarge() const  { return large; }

/**
 * Loads the tile image
 */ 
void Tile::loadImage() {
    if (!image) {
        SubImage *subimage = NULL;        

        ImageInfo *info = imageMgr->get(imageName);
        if (!info) {
            subimage = imageMgr->getSubImage(imageName);
            if (subimage)            
                info = imageMgr->get(subimage->srcImageName);            
        }

        scale = settings.scale;

        if (info) {
            w = (subimage ? subimage->width * scale : info->width * scale);
            h = (subimage ? (subimage->height * scale) / frames : (info->height * scale) / frames);
            image = Image::create(w, h * frames, false, Image::HARDWARE);

            info->image->alphaOff();

            /* draw the tile from the image we found to our tile image */
            if (subimage) {
                Image *tiles = info->image;
                tiles->drawSubRectOn(image, 0, 0, subimage->x * scale, subimage->y * scale, subimage->width * scale, subimage->height * scale);
            }
            else info->image->drawOn(image, 0, 0);                
        }

        /* if we have animations, we always used 'animated' to draw from */
        if (anim)
            image->alphaOff();

        if (image == NULL)
            errorFatal("Error: not all tile images loaded correctly, aborting...");
    }
}

/**
 * MapTile Class Implementation
 */
Direction MapTile::getDirection() const {
    if (isShip() || isPirateShip())
        return (Direction) (frame + DIR_WEST);
    else if (isHorse())
        return (Direction) ((frame << 1) + DIR_WEST);
    else
        return DIR_WEST;        /* some random default */
}

bool MapTile::setDirection(Direction d) {
    bool changed = true;
    
    /* if we're already pointing the right direction, do nothing! */
    if (getDirection() == d)
        return false;

    if (isShip() || isPirateShip())
        frame = d - DIR_WEST;
    else if (isHorse())
        frame = d == DIR_WEST ? 0 : 1;
    else
        changed = false;

    return changed;
}

#define TESTBIT(against)     (Tileset::get()->get(id)->rule->mask & (against))
#define TESTMOVEBIT(against) (Tileset::get()->get(id)->rule->movementMask & (against))
#define GETRULE              (Tileset::get()->get(id)->rule)

bool MapTile::canWalkOn(Direction d) const {    
    return DIR_IN_MASK(d, GETRULE->walkonDirs) ? true : false;
}

bool MapTile::canWalkOff(Direction d) const {        
    return DIR_IN_MASK(d, GETRULE->walkoffDirs) ? true : false;
}

bool MapTile::canAttackOver() const {
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */    
    return isWalkable() || isSwimable() || isSailable() || TESTBIT(MASK_ATTACKOVER);        
}

bool MapTile::canLandBalloon() const {    
    return TESTBIT(MASK_CANLANDBALLOON);
}

bool MapTile::isReplacement() const {    
    return TESTBIT(MASK_REPLACEMENT);
}

bool MapTile::isWalkable() const {        
    return GETRULE->walkonDirs > 0;
}

bool MapTile::isCreatureWalkable() const {
    return canWalkOn(DIR_ADVANCE) && !TESTMOVEBIT(MASK_CREATURE_UNWALKABLE);
}

bool MapTile::isDungeonFloor() const {
    Tile *floor = Tileset::findTileByName("brick_floor");
    if (id == floor->id)
        return true;
    return false;
}

bool MapTile::isSwimable() const {    
    return TESTMOVEBIT(MASK_SWIMABLE);
}

bool MapTile::isSailable() const {    
    return TESTMOVEBIT(MASK_SAILABLE);
}

bool MapTile::isWater() const {
    return (isSwimable() || isSailable());
}

bool MapTile::isFlyable() const {    
    return !TESTMOVEBIT(MASK_UNFLYABLE);
}

bool MapTile::isDoor() const {    
    return TESTBIT(MASK_DOOR);
}

bool MapTile::isLockedDoor() const {    
    return TESTBIT(MASK_LOCKEDDOOR);
}

bool MapTile::isChest() const {    
    return TESTBIT(MASK_CHEST);
}

bool MapTile::isShip() const {    
    return TESTBIT(MASK_SHIP);
}

bool MapTile::isPirateShip() const {
    Tile *pirate = Tileset::findTileByName("pirate_ship");
    if (id == pirate->id)
        return true;
    return false;
}

bool MapTile::isHorse() const {    
    return TESTBIT(MASK_HORSE);
}

bool MapTile::isBalloon() const {    
    return TESTBIT(MASK_BALLOON);
}

bool MapTile::canDispel() const {    
    return TESTBIT(MASK_DISPEL);
}

bool MapTile::canTalkOver() const {    
    return TESTBIT(MASK_TALKOVER);
}

TileSpeed MapTile::getSpeed() const {     
    return GETRULE->speed;
}

TileEffect MapTile::getEffect() const {    
    return GETRULE->effect;
}

bool MapTile::isOpaque() const {
    extern Context *c;

    if (c->opacity)
        return Tileset::get()->get(id)->opaque ? 1 : 0;
    else return 0;
}

/**
 * Is tile a foreground tile (i.e. has transparent parts).
 */
bool MapTile::isForeground() const {
    return TESTBIT(MASK_FOREGROUND);
}

bool MapTile::canTalkOverTile(MapTile tile) {
    return tile.canTalkOver();
}
bool MapTile::canAttackOverTile(MapTile tile) {
    return tile.canAttackOver();
}

MapTile MapTile::tileForClass(int klass) {    
    return Tile::translate((klass * 2) + 0x20, "base");
}

#undef TESTBIT
#undef TESTMOVEBIT
#undef GETRULE
