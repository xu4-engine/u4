/*
 * tile.cpp
 */

#include <string.h>
#include "tile.h"

#include "config.h"
#include "context.h"
#include "creature.h"
#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "location.h"
#include "settings.h"
#include "screen.h"
#include "tileanim.h"
#include "tileset.h"
#include "utils.h"
#include "xu4.h"


TileSymbols Tile::sym;

void Tile::initSymbols(Config* cfg) {
    sym.brickFloor   = cfg->intern("brick_floor");
    sym.dungeonFloor = sym.brickFloor;
    sym.avatar       = cfg->intern("avatar");
    sym.black        = cfg->intern("black");
    sym.beggar       = cfg->intern("beggar");
    sym.bridge       = cfg->intern("bridge");
    sym.chest        = cfg->intern("chest");
    sym.corpse       = cfg->intern("corpse");
    sym.door         = cfg->intern("door");
    sym.guard        = cfg->intern("guard");
    sym.grass        = cfg->intern("grass");
    sym.horse        = cfg->intern("horse");
    sym.balloon      = cfg->intern("balloon");
    sym.ship         = cfg->intern("ship");
    sym.pirateShip   = cfg->intern("pirate_ship");
    sym.wisp         = cfg->intern("wisp");
    sym.moongate     = cfg->intern("moongate");
    sym.whirlpool    = cfg->intern("whirlpool");
    sym.lockelake    = cfg->intern("lockelake");
    sym.hitFlash     = cfg->intern("hit_flash");
    sym.missFlash    = cfg->intern("miss_flash");
    sym.magicFlash   = cfg->intern("magic_flash");

    // These coincide with ClassType.
    cfg->internSymbols(sym.classTiles, 8,
        "mage bard fighter druid\n"
        "tinker paladin ranger shepherd\n");

    cfg->internSymbols(sym.dungeonTiles, 16,
        "brick_floor up_ladder down_ladder up_down_ladder\n"
        "chest unimpl_ceiling_hole unimpl_floor_hole magic_orb\n"
        "ceiling_hole fountain brick_floor dungeon_altar\n"
        "dungeon_door dungeon_room secret_door brick_wall\n");
    sym.dungeonTiles[16] = SYM_UNSET;

    cfg->internSymbols(sym.fields, 4,
        "poison_field energy_field fire_field sleep_field");
    sym.fields[4] = SYM_UNSET;

    // These coincide with dungeonMapId[].
    cfg->internSymbols(sym.dungeonMaps, 6,
        "brick_floor up_ladder down_ladder up_down_ladder\n"
        "dungeon_door secret_door\n");

    // These coincide with combatMapId[].
    cfg->internSymbols(sym.combatMaps, 21,
        "horse swamp grass brush\n"
        "forest hills dungeon city\n"
        "castle town lcb_entrance bridge\n"
        "balloon bridge_n bridge_s shrine\n"
        "chest brick_floor moongate moongate_opening\n"
        "dungeon_floor\n");
}

Tile::Tile(int tid)
    : id(tid)
    , animationRule(0)
    , w(0)
    , h(0)
    , frames(0)
    , scale(1)
    , opaque(false)
    , foreground()
    , waterForeground()
    , tiledInDungeon(false)
    , rule(NULL)
    , image(NULL)
    , anim(NULL)
    , directionCount(0)
{
}

Tile::~Tile() {
    delete image;
}

void Tile::setDirections(const char* dirs) {
    const unsigned maxDir = sizeof(directions);
    directionCount = strlen(dirs);
    if (directionCount != (unsigned) frames)
        errorFatal("Error: %d directions for tile but only %d frames", (int) directionCount, frames);
    if (directionCount > maxDir)
        errorFatal("Error: Number of directions exceeds limit of %d", maxDir);

    unsigned i = 0;
    for (; i < directionCount; i++) {
        switch (dirs[i]) {
        case 'w': directions[i] = DIR_WEST;  break;
        case 'n': directions[i] = DIR_NORTH; break;
        case 'e': directions[i] = DIR_EAST;  break;
        case 's': directions[i] = DIR_SOUTH; break;
        default:
            errorFatal("Error: unknown direction specified by %c", dirs[i]);
        }
    }
    for (; i < maxDir; i++)
        directions[i] = DIR_NONE;
}

const char* Tile::nameStr() const {
    return xu4.config->symbolName(name);
}

/**
 * Loads the tile image
 */
void Tile::loadImage() {
    if (!image) {
        scale = SCALED_BASE;

        const SubImage* subimage = NULL;

        ImageInfo *info = xu4.imageMgr->get(imageName);
        if (!info) {
            subimage = xu4.imageMgr->getSubImage(imageName);
            if (subimage)
                info = xu4.imageMgr->get(subimage->srcImageName);
        }
        if (!info) //IF still no info loaded
        {
            errorWarning("Error: couldn't load image for tile '%s'", nameStr());
            return;
        }

        /* FIXME: This is a hack to address the fact that there are 4
           frames for the guard in VGA mode, but only 2 in EGA.  VGA mode
           uses a separate guard image.  In the future, frames should not
           be stored in the tile, but come from image animation data. */
        if (! subimage)
            frames = info->tiles;
#if 0
        const char* nstr = xu4.config->symbolName(name);
        if (strcmp(nstr, "guard") == 0)
            printf("tile %c frames: %d %s\n",
                   subimage ? 'S' : 'I', frames, nstr);
#endif

        if (info) {
            w = (subimage ? subimage->width * scale : info->width * scale / info->prescale);
            h = (subimage ? (subimage->height * scale) / frames : (info->height * scale / info->prescale) / frames);
            image = Image::create(w, h * frames);

            // NOTE: Blending should be off by default, but TileView::drawTile
            // was loading images on the fly from inside the draw loop.
            // Therefore, we ensure blending is disabled.
            int wasBlending = Image::enableBlend(0);

            /* draw the tile from the image we found to our tile image */
            if (subimage) {
                Image *tiles = info->image;
                tiles->drawSubRectOn(image, 0, 0, subimage->x * scale, subimage->y * scale, subimage->width * scale, subimage->height * scale);
            }
            else info->image->drawOn(image, 0, 0);

            if (wasBlending)
                Image::enableBlend(1);
        }

        if (animationRule) {
            const TileAnimSet* tileanims = screenState()->tileanims;

            anim = NULL;
            if (tileanims)
                anim = tileanims->getByName(animationRule);
            if (anim == NULL)
                errorWarning("animation '%s' not found",
                             xu4.config->symbolName(animationRule));
        }
    }
}

void Tile::deleteImage()
{
    if(image) {
        delete image;
        image = NULL;
    }
    scale = SCALED_BASE;
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

bool Tile::isDungeonFloor() const {
    return (name == sym.dungeonFloor);
}

bool Tile::isOpaque() const {
    extern Context *c;
    return c->opacity ? opaque : false;
}

/**
 * Is tile a foreground tile (i.e. has transparent parts).
 * Deprecated? Never used in XML. Other mechanisms exist, though this could help?
 */
bool Tile::isForeground() const {
    return (rule->mask & MASK_FOREGROUND);
}

Direction Tile::directionForFrame(int frame) const {
    if (frame >= directionCount)
        return DIR_NONE;
    else
        return (Direction) directions[frame];
}

int Tile::frameForDirection(Direction d) const {
    for (int i = 0; i < directionCount; i++) {
        if (directions[i] == d)
            return i;
    }
    return -1;
}


const Tile *MapTile::getTileType() const {
    return Tileset::findTileById(id);
}
