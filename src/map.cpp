/*
 * map.cpp
 */

#include "map.h"

#include "config.h"
#include "debug.h"
#include "event.h"
#include "player.h"
#include "portal.h"
#include "tileset.h"
#include "xu4.h"


/**
 * Map Coords functions
 */

void map_wrap(Coords& c, const Map *map) {
    if (map && map->border_behavior == Map::BORDER_WRAP) {
        while (c.x < 0)
            c.x += map->width;
        while (c.y < 0)
            c.y += map->height;
        while (c.x >= (int)map->width)
            c.x -= map->width;
        while (c.y >= (int)map->height)
            c.y -= map->height;
    }
}

void map_move(Coords& c, Direction d, const Map *map) {
    switch(d) {
        case DIR_NORTH: c.y--; break;
        case DIR_EAST:  c.x++; break;
        case DIR_SOUTH: c.y++; break;
        case DIR_WEST:  c.x--; break;
        default: break;
    }

    // Wrap the coordinates if necessary
    map_wrap(c, map);
}

void map_move(Coords& c, int dx, int dy, const Map *map) {
    c.x += dx;
    c.y += dy;

    // Wrap the coordinates if necessary
    map_wrap(c, map);
}

/**
 * Returns a mask of directions that indicate where one point is relative
 * to another.  For instance, if the object at (x, y) is
 * northeast of (c.x, c.y), then this function returns
 * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
 * This function also takes into account map boundaries and adjusts
 * itself accordingly. If the two coordinates are not on the same z-plane,
 * then this function return DIR_NONE.
 */
int map_getRelativeDirection(const Coords& a, const Coords& b, const Map *map) {
    int dx, dy, dirmask;

    dirmask = DIR_NONE;
    if (a.z != b.z)
        return dirmask;

    /* adjust our coordinates to find the closest path */
    if (map && map->border_behavior == Map::BORDER_WRAP) {
        Coords c = a;

        if (abs(int(c.x - b.x)) > abs(int(c.x + map->width - b.x)))
            c.x += map->width;
        else if (abs(int(c.x - b.x)) > abs(int(c.x - map->width - b.x)))
            c.x -= map->width;

        if (abs(int(c.y - b.y)) > abs(int(c.y + map->width - b.y)))
            c.y += map->height;
        else if (abs(int(c.y - b.y)) > abs(int(c.y - map->width - b.y)))
            c.y -= map->height;

        dx = c.x - b.x;
        dy = c.y - b.y;
    }
    else {
        dx = a.x - b.x;
        dy = a.y - b.y;
    }

    /* add x directions that lead towards to_x to the mask */
    if (dx < 0)         dirmask |= MASK_DIR(DIR_EAST);
    else if (dx > 0)    dirmask |= MASK_DIR(DIR_WEST);

    /* add y directions that lead towards to_y to the mask */
    if (dy < 0)         dirmask |= MASK_DIR(DIR_SOUTH);
    else if (dy > 0)    dirmask |= MASK_DIR(DIR_NORTH);

    /* return the result */
    return dirmask;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 * This function also takes into account map boundaries and adjusts
 * itself accordingly, provided the 'map' parameter is passed
 */
Direction map_pathTo(const Coords& a, const Coords &b, int valid_directions, bool towards, const Map *map) {
    int directionsToObject;

    /* find the directions that lead [to/away from] our target */
    directionsToObject = map_getRelativeDirection(a, b, map);
    if (! towards)
        directionsToObject = ~directionsToObject;

    /* make sure we eliminate impossible options */
    directionsToObject &= valid_directions;

    /* get the new direction to move */
    if (directionsToObject > DIR_NONE)
        return dirRandomDir(directionsToObject);

    /* there are no valid directions that lead to our target, just move wherever we can! */
    return dirRandomDir(valid_directions);
}

/**
 * Finds the appropriate direction to travel to move away from one point
 */
Direction map_pathAway(const Coords& a, const Coords &b, int valid_directions) {
    return map_pathTo(a, b, valid_directions, false);
}

/**
 * Finds the movement distance (not using diagonals) from point a to point b
 * on a map, taking into account map boundaries and such.  If the two coords
 * are not on the same z-plane, then this function returns -1;
 */
int map_movementDistance(const Coords& a, const Coords& b, const Map *map) {
    Coords c;
    int dist;
    int dirmask;
    int dx, dy;

    if (a.z != b.z)
        return -1;

    /* get the direction(s) to the coordinates */
    dirmask = map_getRelativeDirection(a, b, map);
    dx = (dirmask & MASK_DIR_WEST) ? -1 : 1;
    dy = (dirmask & MASK_DIR_NORTH) ? -1 : 1;

    dist = 0;
    c = a;

    while ((c.x != b.x) || (c.y != b.y)) {
        if (c.x != b.x) {
            c.x += dx;
            dist++;
        }
        if (c.y != b.y) {
            c.y += dy;
            dist++;
        }
        map_wrap(c, map);
    }

    return dist;
}

/**
 * Finds the distance (using diagonals) from point a to point b on a map
 * If the two coordinates are not on the same z-plane, then this function
 * returns -1. This function also takes into account map boundaries.
 */
int map_distance(const Coords& a, const Coords& b, const Map *map) {
    int dist = map_movementDistance(a, b, map);
    if (dist <= 0)
        return dist;

    /* calculate how many fewer movements there would have been */
    int absDx = abs(a.x - b.x);
    int absDy = abs(a.y - b.y);
    dist -= (absDx < absDy) ? absDx : absDy;
    return dist;
}

bool map_outOfBounds(const Map* map, const Coords& c) {
    return (c.x < 0 || c.x >= (int) map->boundMaxX ||
            c.y < 0 || c.y >= (int) map->boundMaxY ||
            c.z < 0 || c.z >= (int) map->levels);
}

/**
 * Map Class Implementation
 */

Map::Map() {
    _pad = 0;
    width = 0;
    height = 0;
    levels = 1;
    chunk_width = 0;
    chunk_height = 0;
    boundMaxX = boundMaxY = 0;
    flags = 0;
    offset = 0;
    id = 0;
    data = NULL;
    tileset = NULL;
}

Map::~Map() {
    for (PortalList::iterator i = portals.begin(); i != portals.end(); i++) {
        delete (*i)->retroActiveDest;
        delete *i;
    }
    clearObjects();
    delete[] data;
}

const char* Map::getName() const {
    return xu4.config->confString(fname);
}

/*
 * Build BlockingGroups for use by the shadow casting shader.
 */
void Map::queryBlocking(BlockingGroups* bg, int sx, int sy, int vw, int vh) const {
    const Tile* tile;
    int centerX, leftEndX, maxX;
    int centerY, maxY;
    int x, y, di;
    int count;
    float* pos = bg->tilePos;
    float* posEnd = pos + BLOCKING_POS_SIZE;

    centerX = sx + vw / 2;
    centerY = sy + vw / 2;

    // Initialize counts in case the function aborts (buffer_full).
    bg->left = bg->center = bg->right = 0;

    // Handle negative start positions.
    if (sx < 0) {
        vw += sx;   // Subtracts sx.
        sx = 0;
    }
    if (sy < 0) {
        vh += sy;   // Subtracts sy.
        sy = 0;
    }

    maxX = sx + vw;
    if (maxX > width)
        maxX = width;
    maxY = sy + vh;
    if (maxY > height)
        maxY = height;

#define BLOCKING_COLUMN \
    for (di = sy * width + x, y = sy; y < maxY; di += width, ++y) { \
        tile = tileset->get(data[di]); \
        if (tile->opaque) { \
            if (pos == posEnd) \
                goto buffer_full; \
            *pos++ = (float) (x - centerX); \
            *pos++ = (float) (y - centerY); \
            *pos++ = (float) tile->opaque; \
            ++count; \
        } \
    }

    // Gather blocking tiles in column left to right order.

    leftEndX = (centerX < width) ? centerX : width;
    count = 0;
    for (x = sx; x < leftEndX; ++x) {
        BLOCKING_COLUMN
    }
    bg->left = count;

    count = 0;
    if (centerX < width) {
        BLOCKING_COLUMN
    }
    bg->center = count;

    count = 0;
    for (++x; x < maxX; ++x) {
        BLOCKING_COLUMN
    }
    bg->right = count;
    return;

buffer_full:
    fprintf(stderr, "Map::queryBlocking pos buffer full!\n" );
}

/*
 * Call a function for each entity (Annotations & Objects) near a coordinate.
 *
 * \param center    Center of area to process.
 * \param radius    Number of tiles away from center.
 * \param func      Callback function.
 * \param user      User data pointer passed to callback.
 */
void Map::queryVisible(const Coords& center, int radius,
                       void (*func)(const Coords*, VisualId, void*),
                       void* user, const Object** focusPtr) const {
    int minX, minY, maxX, maxY;
    const Coords* cp;
    const TileRenderData* rd = tileset->render;
    VisualId vid;

    *focusPtr = NULL;

#define OUTSIDE(C) (C->x < minX || C->x > maxX || C->y < minY || C->y > maxY)

    minX = center.x - radius;
    minY = center.y - radius;
    maxX = center.x + radius;
    maxY = center.y + radius;

    AnnotationList::const_iterator ait;
    for(ait = annotations.begin(); ait != annotations.end(); ait++) {
        const Annotation& ann = *ait;
        cp = &ann.coords;
        if (OUTSIDE(cp))
            continue;
        //printf("KR ann %d %d %d,%d\n",
        //        ann.tile.id, ann.tile.frame, cp->x, cp->y);
        vid = rd[ann.tile.id].vid;
        func(cp, vid, user);
    }

    const Animator* animator = &xu4.eventHandler->flourishAnim;
    ObjectDeque::const_iterator it;
    for(it = objects.begin(); it != objects.end(); it++) {
        Object* obj = *it;
        cp = &obj->coords;
        if (OUTSIDE(cp))
            continue;
        if (obj->hasFocus())
            *focusPtr = obj;
        //printf("KR obj %d %d %d,%d\n",
        //        obj->tile.id, obj->tile.frame, cp->x, cp->y);
        if (obj->animId != ANIM_UNUSED) {
            obj->tile.frame = anim_valueI(animator, obj->animId);
        }
        vid = rd[obj->tile.id].vid + obj->tile.frame;
        func(cp, vid, user);
    }

    if (flags & SHOW_AVATAR) {
        cp = &c->location->coords;
        if (! OUTSIDE(cp)) {
            MapTile trans = c->party->getTransport();
            vid = rd[trans.id].vid + trans.frame;
            func(cp, vid, user);
        }
    }
}

/*
 * Call a function for each Annotation at a given coordinate.
 * The callback must return Map::QueryDone or Map::QueryContinue.
 */
void Map::queryAnnotations(const Coords& pos,
                           int (*func)(const Annotation*, void*),
                           void* user) const {
    AnnotationList::const_iterator ait;
	for (ait = annotations.begin(); ait != annotations.end(); ++ait) {
        const Annotation& ann = *ait;
        if (ann.coords == pos) {
            if (func(&ann, user) == Map::QueryDone)
                break;
        }
    }
}

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
const Object *Map::objectAt(const Coords &coords) const {
    /* FIXME: return a list instead of one object */
    ObjectDeque::const_iterator i;
    const Object *objAt = NULL;

    for(i = objects.begin(); i != objects.end(); i++) {
        const Object *obj = *i;

        if (obj->getCoords() == coords) {
            /* get the most visible object */
            if (objAt && (objAt->getType() == Object::UNKNOWN) && (obj->getType() != Object::UNKNOWN))
                objAt = obj;
            /* give priority to objects that have the focus */
            else if (objAt && (!objAt->hasFocus()) && (obj->hasFocus()))
                objAt = obj;
            else if (!objAt)
                objAt = obj;
        }
    }
    return objAt;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *Map::portalAt(const Coords &coords, int actionFlags) {
    PortalList::const_iterator i;

    for(i = portals.begin(); i != portals.end(); i++) {
        if (((*i)->coords == coords) &&
            ((*i)->trigger_action & actionFlags))
            return *i;
    }
    return NULL;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
TileId Map::getTileFromData(const Coords &coords) const {
    if (MAP_IS_OOB(this, coords))
        return 0;

    int index = coords.x + (coords.y * width) + (width * height * coords.z);
    return data[index];
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
const Tile* Map::tileTypeAt(const Coords &coords, int withObjects) const {
    /* FIXME: this should return a list of tiles, with the most visible at the front */
    /* FIXME: this only returns the first valid annotation it can find */
    AnnotationList::const_iterator ait;
    for(ait = annotations.begin(); ait != annotations.end(); ait++) {
        const Annotation& ann = *ait;
        if (ann.coords == coords && ! ann.visualOnly)
            return tileset->get( ann.tile.id );
    }

    TileId tid = 0;
    if (withObjects) {
        const Object* obj = objectAt(coords);
        if (obj) {
            if (withObjects == WITH_OBJECTS)
                tid = obj->getTile().id;
            else if (withObjects == WITH_GROUND_OBJECTS &&
                obj->getTile().getTileType()->isWalkable())
                tid = obj->getTile().id;
        }
    }
    if (! tid)
        tid = getTileFromData(coords);

    return tileset->get(tid);
}

void Map::setTileAt(const Coords& coords, TileId tid) {
    int i = (coords.z * width * height) + (coords.y * width) + coords.x;
    data[i] = tid;
}

/**
 * Returns true if the given map is the world map
 */
bool Map::isWorldMap() const {
    return type == WORLD;
}

/**
 * Returns true if the map is enclosed (to see if gem layouts should cut themselves off)
 */
bool Map::isEnclosed(const Coords &party) {
    unsigned int x, y;
    int *path_data;

    if (border_behavior != BORDER_WRAP)
        return true;

    path_data = new int[width * height];
    memset(path_data, -1, sizeof(int) * width * height);

    // Determine what's walkable (1), and what's border-walkable (2)
    findWalkability(party, path_data);

    // Find two connecting pathways where the avatar can reach both without wrapping
    for (x = 0; x < width; x++) {
        int index = x;
        if (path_data[index] == 2 && path_data[index + ((height-1)*width)] == 2)
            return false;
    }

    for (y = 0; y < width; y++) {
        int index = (y * width);
        if (path_data[index] == 2 && path_data[index + width - 1] == 2)
            return false;
    }

    return true;
}

void Map::findWalkability(Coords coords, int *path_data) {
    const Tile *mt = tileTypeAt(coords, WITHOUT_OBJECTS);
    int index = coords.x + (coords.y * width);

    if (mt->isWalkable()) {
        bool isBorderTile = (coords.x == 0) || (coords.x == signed(width-1)) || (coords.y == 0) || (coords.y == signed(height-1));
        path_data[index] = isBorderTile ? 2 : 1;

        if ((coords.x > 0) && path_data[coords.x - 1 + (coords.y * width)] < 0)
            findWalkability(Coords(coords.x - 1, coords.y, coords.z), path_data);
        if ((coords.x < signed(width-1)) && path_data[coords.x + 1 + (coords.y * width)] < 0)
            findWalkability(Coords(coords.x + 1, coords.y, coords.z), path_data);
        if ((coords.y > 0) && path_data[coords.x + ((coords.y - 1) * width)] < 0)
            findWalkability(Coords(coords.x, coords.y - 1, coords.z), path_data);
        if ((coords.y < signed(height-1)) && path_data[coords.x + ((coords.y + 1) * width)] < 0)
            findWalkability(Coords(coords.x, coords.y + 1, coords.z), path_data);
    }
    else path_data[index] = 0;
}

/**
 * Adds a creature object to the given map
 */
Creature *Map::addCreature(const Creature *creature, const Coords& coords) {
    Creature *m = new Creature(creature);

    m->setInitialHp();
    m->setStatus(STAT_GOOD);
    m->placeOnMap(this, coords);

    /* initialize the creature before placing it */
    if (m->wanders())
        m->setMovementBehavior(MOVEMENT_WANDER);
    else if (m->isStationary())
        m->setMovementBehavior(MOVEMENT_FIXED);
    else m->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);

    /* hide camouflaged creatures from view during combat */
    if (m->camouflages() && (type == COMBAT))
        m->setVisible(false);

    /* place the creature on the map */
    objects.push_back(m);
    return m;
}

/**
 * Adds an object to the given map
 */
Object *Map::addObject(Object *obj, Coords coords) {
    objects.push_front(obj);
    return obj;
}

Object *Map::addObject(MapTile tile, MapTile prevtile, const Coords& coords) {
    Object *obj = new Object;

    obj->setTile(tile);
    obj->setPrevTile(prevtile);
    obj->setPrevCoords(coords);
    obj->placeOnMap(this, coords);

    objects.push_front(obj);

    return obj;
}

/**
 * Removes an object from the map
 */

// This function should only be used when not iterating through an
// ObjectDeque, as the iterator will be invalidated and the
// results will be unpredictable.  Instead, use the function
// below.
void Map::removeObject(const Object *rem, bool deleteObject) {
    ObjectDeque::iterator i;
    for (i = objects.begin(); i != objects.end(); i++) {
        if (*i == rem) {
            /* Party members persist through different maps, so don't delete them! */
            if (!isPartyMember(*i) && deleteObject)
                delete (*i);
            objects.erase(i);
            return;
        }
    }
}

ObjectDeque::iterator Map::removeObject(ObjectDeque::iterator rem, bool deleteObject) {
    /* Party members persist through different maps, so don't delete them! */
    if (!isPartyMember(*rem) && deleteObject)
        delete (*rem);
    return objects.erase(rem);
}

/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a creature attacking.
 * Also performs special creature actions and creature effects.
 */
Creature *Map::moveObjects(const Coords& avatar) {
    Creature *attacker = NULL;

    for (unsigned int i = 0; i < objects.size(); i++) {
        Creature *m = dynamic_cast<Creature*>(objects[i]);

        if (m) {
            /* check if the object is an attacking creature and not
               just a normal, docile person in town or an inanimate object */
            if ((m->getType() == Object::PERSON && m->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR) ||
                (m->getType() == Object::CREATURE && m->willAttack())) {
                Coords o_coords = m->getCoords();

                /* don't move objects that aren't on the same level as us */
                if (o_coords.z != avatar.z)
                    continue;

                if (map_movementDistance(o_coords, avatar, this) <= 1) {
                    attacker = m;
                    continue;
                }
            }

            /* Before moving, Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
            m->specialEffect();


            /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
            if (!m->specialAction())
            {
                if  (moveObject(this, m, avatar))
                {
                    m->animateMovement();
                    /* After moving, Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
                    m->specialEffect();
                }
            }
        }
    }

    return attacker;
}

/**
 * Removes all objects from the given map
 */
void Map::clearObjects() {
    for (ObjectDeque::iterator o = objects.begin(); o != objects.end(); o++) {
        if (! isPartyMember(*o))
            delete *o;
    }
    objects.clear();
}

/**
 * Returns the number of creatures on the given map
 */
int Map::getNumberOfCreatures() {
    ObjectDeque::const_iterator i;
    int n = 0;

    for (i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;

        if (obj->getType() == Object::CREATURE)
            n++;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int Map::getValidMoves(const Coords& from, MapTile transport) {
    int retval;
    Direction d;
    Object *obj;
    const Creature *m, *to_m;
    const Tile* prev_tile;
    const Tile* tile;
    int ontoAvatar, ontoCreature;
    Coords testCoord;

    // get the creature object, if it exists (the one that's moving)
    m = Creature::getByTile(transport);

    bool isAvatar = (type != COMBAT) && (from == c->location->coords);
    if (m && m->canMoveOntoPlayer())
        isAvatar = false;

    prev_tile = tileTypeAt(from, WITHOUT_OBJECTS);

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        ontoAvatar = 0;
        ontoCreature = 0;

        // Move the coordinates in the current direction and test it
        testCoord = from;
        map_move(testCoord, d, this);

        // you can always walk off the edge of the map
        if (MAP_IS_OOB(this, testCoord)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        obj = objectAt(testCoord);

        // see if it's trying to move onto the avatar
        if ((flags & SHOW_AVATAR) && (testCoord == c->location->coords))
            ontoAvatar = 1;

        // see if it's trying to move onto a person or creature
        else if (obj && (obj->getType() != Object::UNKNOWN))
            ontoCreature = 1;

        // get the destination tile
        if (ontoAvatar)
            tile = c->party->getTransport().getTileType();
        else if (ontoCreature)
            tile = obj->getTile().getTileType();
        else
            tile = tileTypeAt(testCoord, WITH_OBJECTS);

        // get the other creature object, if it exists (the one that's being moved onto)
        to_m = dynamic_cast<Creature*>(obj);

        // move on if unable to move onto the avatar or another creature
        if (m && !isAvatar) { // some creatures/persons have the same tile as the avatar, so we have to adjust
            // If moving onto the avatar, the creature must be able to move onto the player
            // If moving onto another creature, it must be able to move onto other creatures,
            // and the creature must be able to have others move onto it.  If either of
            // these conditions are not met, the creature cannot move onto another.

            if ((ontoAvatar && m->canMoveOntoPlayer()) || (ontoCreature && m->canMoveOntoCreatures()))
                tile = tileTypeAt(testCoord, WITHOUT_OBJECTS); //Ignore all objects, and just consider terrain
              if ((ontoAvatar && !m->canMoveOntoPlayer())
                ||  (
                        ontoCreature &&
                        (
                            (!m->canMoveOntoCreatures() && !to_m->canMoveOntoCreatures())
                            || (m->isForceOfNature() && to_m->isForceOfNature())
                        )
                    )
                )
                continue;
        }

        // avatar movement
        if (isAvatar) {
            // if the transport is a ship, check sailable
            // if it is a balloon, check flyable
            // avatar or horseback: check walkable

            const Tile* transTile = transport.getTileType();
            if (transTile->isShip() && tile->isSailable())
                retval = DIR_ADD_TO_MASK(d, retval);
            else if (transTile->isBalloon() && tile->isFlyable())
                retval = DIR_ADD_TO_MASK(d, retval);
            else if (transTile->name == Tile::sym.avatar || transTile->isHorse()) {
                if (tile->canWalkOn(d) &&
                    (!transTile->isHorse() || tile->isCreatureWalkable()) &&
                    prev_tile->canWalkOff(d))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
//            else if (ontoCreature && to_m->canMoveOntoPlayer()) {
//              retval = DIR_ADD_TO_MASK(d, retval);
//            }
        }

        // creature movement
        else if (m) {
            // flying creatures
            if (tile->isFlyable() && m->flies()) {
                // FIXME: flying creatures behave differently on the world map?
                if (isWorldMap())
                    retval = DIR_ADD_TO_MASK(d, retval);
                else if (tile->isWalkable() ||
                         tile->isSwimable() ||
                         tile->isSailable())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // swimming creatures and sailing creatures
            else if (tile->isSwimable() ||
                     tile->isSailable() ||
                     tile->isShip()) {
                if (m->swims() && tile->isSwimable())
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (m->sails() && tile->isSailable())
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (m->canMoveOntoPlayer() && tile->isShip())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // ghosts and other incorporeal creatures
            else if (m->isIncorporeal()) {
                // can move anywhere but onto water, unless of course the creature can swim
                if (!(tile->isSwimable() ||
                      tile->isSailable()))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // walking creatures
            else if (m->walks()) {
                if (tile->canWalkOn(d) &&
                    prev_tile->canWalkOff(d) &&
                    tile->isCreatureWalkable())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // Creatures that can move onto player
            else if (ontoAvatar && m->canMoveOntoPlayer())
            {
                //tile should be transport
                if (tile->isShip() && m->swims())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
        }
    }

    return retval;
}

bool Map::move(Object *obj, Direction d) {
    Coords new_coords = obj->getCoords();
    map_move(new_coords, d);
    if (new_coords != obj->getCoords()) {
        obj->setCoords(new_coords);
        return true;
    }
    return false;
}

/**
 * Alerts the guards that the avatar is doing something bad
 */
void Map::alertGuards() {
    ObjectDeque::iterator i;
    const Creature *m;

    /* switch all the guards to attack mode */
    for (i = objects.begin(); i != objects.end(); i++) {
        m = Creature::getByTile((*i)->getTile());
        if (m && (m->getId() == GUARD_ID || m->getId() == LORDBRITISH_ID))
            (*i)->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
}

const Coords* Map::getLabel(Symbol name) const {
    std::map<Symbol, Coords>::const_iterator i = labels.find(name);
    if (i == labels.end())
        return NULL;
    return &i->second;
}

const char* Map::labelAt(const Coords& pos) const {
    std::map<Symbol, Coords>::const_iterator it;
    for (it = labels.begin(); it != labels.end(); ++it) {
        if (it->second == pos)
            return xu4.config->symbolName(it->first);
    }
    return NULL;
}

void Map::putInBounds(Coords& c) const {
    if (c.x < 0)
        c.x = 0;
    else if (c.x >= (int) width)
        c.x = width - 1;

    if (c.y < 0)
        c.y = 0;
    else if (c.y >= (int) height)
        c.y = height - 1;

    if (c.z < 0)
        c.z = 0;
    else if (c.z >= (int) levels)
        c.z = levels - 1;
}

void Map::fillMonsterTable(SaveGameMonsterRecord* table) const {
    ObjectDeque::const_iterator current;
    const Object *obj;
    CObjectDeque monsters;
    CObjectDeque other_creatures;
    CObjectDeque inanimate_objects;
    Object empty;

    int nCreatures = 0;
    int nObjects = 0;
    int i;


    /**
     * First, categorize all the objects we have
     */
    for (current = objects.begin(); current != objects.end(); current++) {
        obj = *current;

        /* moving objects first */
        if ((obj->getType() == Object::CREATURE) &&
            (obj->getMovementBehavior() != MOVEMENT_FIXED)) {
            const Creature *c = dynamic_cast<const Creature*>(obj);
            /* whirlpools and storms are separated from other moving objects */
            if (c->getId() == WHIRLPOOL_ID || c->getId() == STORM_ID)
                monsters.push_back(obj);
            else
                other_creatures.push_back(obj);
        } else
            inanimate_objects.push_back(obj);
    }

    /**
     * Add other monsters to our whirlpools and storms
     */
    while (other_creatures.size() && nCreatures < MONSTERTABLE_CREATURES_SIZE) {
        monsters.push_back(other_creatures.front());
        other_creatures.pop_front();
    }

    /**
     * Add empty objects to our list to fill things up
     */
    while (monsters.size() < MONSTERTABLE_CREATURES_SIZE)
        monsters.push_back(&empty);

    /**
     * Finally, add inanimate objects
     */
    while (inanimate_objects.size() && nObjects < MONSTERTABLE_OBJECTS_SIZE) {
        monsters.push_back(inanimate_objects.front());
        inanimate_objects.pop_front();
    }

    /**
     * Fill in the blanks
     */
    while (monsters.size() < MONSTERTABLE_SIZE)
        monsters.push_back(&empty);

    /**
     * Fill in our monster table
     */
    MapTile prevTile;
    const UltimaSaveIds* saveIds = xu4.config->usaveIds();
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        obj = monsters[i];
        Coords c = obj->getCoords(),
           prevc = obj->getPrevCoords();

        // Reset animation to a value that is savegame compatible with u4dos.
        if (obj->getType() == Object::CREATURE) {
            prevTile = obj->getTile();
            prevTile.frame = 0;
        } else
            prevTile = obj->getPrevTile();

        table->tile = saveIds->ultimaId(obj->getTile());
        table->x = c.x;
        table->y = c.y;
        table->prevTile = saveIds->ultimaId(prevTile);
        table->prevx = prevc.x;
        table->prevy = prevc.y;
        table->level =
        table->unused = 0;
        ++table;
    }
}

void Map::fillMonsterTableDungeon(SaveGameMonsterRecord* table) const {
    MapTile prevTile;
    ObjectDeque::const_iterator it;
    SaveGameMonsterRecord* end = table + MONSTERTABLE_SIZE;
    const UltimaSaveIds* saveIds = xu4.config->usaveIds();
    const Object *obj;

    for (it = objects.begin(); it != objects.end(); ++it) {
        obj = *it;
        if (obj->getType() == Object::CREATURE) {
            const Coords& c = obj->getCoords();
            const Coords& prevc = obj->getPrevCoords();

            // Reset animation to a value that is savegame compatible with u4dos.
            prevTile = obj->getTile();
            prevTile.frame = 0;

            table->tile = 0;
            table->x = c.x;
            table->y = c.y;
            table->prevTile = saveIds->ultimaId(prevTile);
            table->prevx = prevc.x;
            table->prevy = prevc.y;
            table->level = c.z;
            table->unused = 0;

            if (++table == end)
                return;     // Table full.
        }
    }

    while (table != end) {
        memset(table, 0, sizeof(SaveGameMonsterRecord));
        ++table;
    }
}
