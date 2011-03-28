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

TileId Tile::nextId = 0;

Tile::Tile(Tileset *tileset) : 
    id(nextId++), tileset(tileset), w(0), h(0), frames(0), scale(1), 
    anim(NULL), opaque(false), rule(NULL), image(NULL), tiledInDungeon(false), animationRule("") {
}

/**
 * Loads tile information.
 */
void Tile::loadProperties(const ConfigElement &conf) {
    if (conf.getName() != "tile")
        return;
            
    name = conf.getString("name"); /* get the name of the tile */

    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation")) {
        animationRule = conf.getString("animation");
    }

    /* see if the tile is opaque */
    opaque = conf.getBool("opaque"); 

    foreground = conf.getBool("usesReplacementTileAsBackground");
    waterForeground = conf.getBool("usesWaterReplacementTileAsBackground");

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

    /* get the name of the image that belongs to this tile */
    if (conf.exists("image"))
        imageName = conf.getString("image");
    else 
        imageName = string("tile_") + name;

    tiledInDungeon = conf.getBool("tiledInDungeon");

    if (conf.exists("directions")) {
        string dirs = conf.getString("directions");
        if (dirs.length() != (unsigned) frames)
            errorFatal("Error: %ld directions for tile but only %d frames", (long) dirs.length(), frames);
        for (unsigned i = 0; i < dirs.length(); i++) {
            if (dirs[i] == 'w')
                directions.push_back(DIR_WEST);
            else if (dirs[i] == 'n')
                directions.push_back(DIR_NORTH);
            else if (dirs[i] == 'e')
                directions.push_back(DIR_EAST);
            else if (dirs[i] == 's')
                directions.push_back(DIR_SOUTH);
            else
                errorFatal("Error: unknown direction specified by %c", dirs[i]);
        }
    }
}

Image *Tile::getImage() {
    if (!image)
        loadImage();
    return image;
}

bool Tile::isTiledInDungeon() const  { return tiledInDungeon; }

/**
 * Loads the tile image
 */ 
void Tile::loadImage() {
    if (!image) {
        scale = settings.scale;

    	SubImage *subimage = NULL;

        ImageInfo *info = imageMgr->get(imageName);
        if (!info) {
            subimage = imageMgr->getSubImage(imageName);
            if (subimage)            
                info = imageMgr->get(subimage->srcImageName);            
        }

        /* FIXME: This is a hack to address the fact that there are 4
           frames for the guard in VGA mode, but only 2 in EGA. Is there
           a better way to handle this? */
        if (name == "guard")
        {
        	if (settings.videoType == "EGA")
        		frames = 2;
        	else
        		frames = 4;
        }

        if (info->image)
        	info->image->alphaOff();

        if (info) {
            w = (subimage ? subimage->width * scale : info->width * scale);
            h = (subimage ? (subimage->height * scale) / frames : (info->height * scale) / frames);
            image = Image::create(w, h * frames, false, Image::HARDWARE);


            //info->image->alphaOff();

            /* draw the tile from the image we found to our tile image */
            if (subimage) {
                Image *tiles = info->image;
                tiles->drawSubRectOn(image, 0, 0, subimage->x * scale, subimage->y * scale, subimage->width * scale, subimage->height * scale);
            }
            else info->image->drawOn(image, 0, 0);
        }

        if (animationRule.size() > 0) {
            extern TileAnimSet *tileanims;

            anim = NULL;
            if (tileanims)
                anim = tileanims->getByName(animationRule);
            if (anim == NULL)
                errorWarning("Warning: animation style '%s' not found", animationRule.c_str());
        }

        /* if we have animations, we always used 'animated' to draw from */
        //if (anim)
        //    image->alphaOff();

        if (image == NULL)
            errorFatal("Error: couldn't load image for tile '%s'", name.c_str());

    	if (Settings::getInstance().enhancements && Settings::getInstance().enhancementsOptions.renderTileTransparency)
    	{
    		int transparency_shadow_size =Settings::getInstance().enhancementsOptions.transparentTileShadowSize;
			for (int f = 0; f < frames; ++f)
				image->setTransparentIndex(0, transparency_shadow_size * scale, frames, f);
    	}
    }
}

void Tile::deleteImage()
{
	delete this->image;
	this->image = NULL;
	this->scale = settings.scale;

}


/**
 * MapTile Class Implementation
 */
Direction MapTile::getDirection() const {
    return getTileType()->directionForFrame(frame);
}

bool MapTile::setDirection(Direction d) {
    /* if we're already pointing the right direction, do nothing! */
    if (getDirection() == d)
        return false;

    const Tile *type = getTileType();

    int new_frame = type->frameForDirection(d);
    if (new_frame != -1) {
        frame = new_frame;
        return true;
    }
    return false;
}

bool Tile::canWalkOn(Direction d) const {    
    return DIR_IN_MASK(d, rule->walkonDirs) ? true : false;
}

bool Tile::canWalkOff(Direction d) const {        
    return DIR_IN_MASK(d, rule->walkoffDirs) ? true : false;
}

bool Tile::canAttackOver() const {
    /* All tiles that you can walk, swim, or sail on, can be attacked over.
       All others must declare themselves */    
    return isWalkable() || isSwimable() || isSailable() || (rule->mask & MASK_ATTACKOVER); 
}

bool Tile::canLandBalloon() const {    
    return (rule->mask & MASK_CANLANDBALLOON);
}

bool Tile::isReplacement() const {    
    return (rule->mask & MASK_REPLACEMENT);
}

bool Tile::isWaterReplacement() const {
    return (rule->mask & MASK_WATER_REPLACEMENT);
}

bool Tile::isWalkable() const {        
    return rule->walkonDirs > 0;
}

bool Tile::isLivingObject() const {
	return rule->mask & MASK_LIVING_THING;
}

bool Tile::isCreatureWalkable() const {
    return canWalkOn(DIR_ADVANCE) && !(rule->movementMask & MASK_CREATURE_UNWALKABLE);
}

bool Tile::isDungeonFloor() const {
    Tile *floor = tileset->getByName("brick_floor");
    if (id == floor->id)
        return true;
    return false;
}

bool Tile::isSwimable() const {    
    return (rule->movementMask & MASK_SWIMABLE);
}

bool Tile::isSailable() const {    
    return (rule->movementMask & MASK_SAILABLE);
}

bool Tile::isWater() const {
    return (isSwimable() || isSailable());
}

bool Tile::isFlyable() const {    
    return !(rule->movementMask & MASK_UNFLYABLE);
}

bool Tile::isDoor() const {    
    return (rule->mask & MASK_DOOR);
}

bool Tile::isLockedDoor() const {    
    return (rule->mask & MASK_LOCKEDDOOR);
}

bool Tile::isChest() const {    
    return (rule->mask & MASK_CHEST);
}

bool Tile::isShip() const {    
    return (rule->mask & MASK_SHIP);
}

bool Tile::isPirateShip() const {
    return name == "pirate_ship";
}

bool Tile::isHorse() const {    
    return (rule->mask & MASK_HORSE);
}

bool Tile::isBalloon() const {    
    return (rule->mask & MASK_BALLOON);
}

bool Tile::canDispel() const {    
    return (rule->mask & MASK_DISPEL);
}

bool Tile::canTalkOver() const {    
    return (rule->mask & MASK_TALKOVER);
}

TileSpeed Tile::getSpeed() const {     
    return rule->speed;
}

TileEffect Tile::getEffect() const {    
    return rule->effect;
}

bool Tile::isOpaque() const {
    extern Context *c;

    if (c->opacity)
        return opaque;
    else
        return false;
}

bool Tile::isLandForeground() const{
	return this->foreground;
}
bool Tile::isWaterForeground() const{
	return this->waterForeground;
}


/**
 * Is tile a foreground tile (i.e. has transparent parts).
 * Deprecated? Never used in XML. Other mechanisms exist, though this could help?
 */
bool Tile::isForeground() const {
    return (rule->mask & MASK_FOREGROUND);
}

Direction Tile::directionForFrame(int frame) const {
    if (static_cast<unsigned>(frame) >= directions.size())
        return DIR_NONE;
    else
        return directions[frame];
}

int Tile::frameForDirection(Direction d) const {
    for (int i = 0; (unsigned) i < directions.size() && i < frames; i++) {
        if (directions[i] == d)
            return i;
    }
    return -1;
}

bool Tile::canTalkOverTile(const Tile *tile) {
    return tile->canTalkOver();
}
bool Tile::canAttackOverTile(const Tile *tile) {
    return tile->canAttackOver();
}

const Tile *MapTile::getTileType() const {
    return Tileset::findTileById(id);
}
