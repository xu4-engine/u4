/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <libxml/xmlmemory.h>

#include "tileset.h"

#include "error.h"
#include "screen.h"
#include "settings.h"
#include "tilemap.h"
#include "xml.h"

/**
 * TileRule Class Implementation
 */
TileRuleMap TileRule::rules;

/**
 * Returns the tile rule with the given name, or NULL if none could be found
 */
TileRule *TileRule::findByName(string name) {
    TileRuleMap::iterator i = rules.find(name);
    if (i != rules.end())
        return i->second;
    return NULL;
}

/**
 * Load tile information from xml.
 */
void TileRule::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tileRules") != 0)
        errorFatal("malformed %s", filename.c_str());

    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"rule") != 0)
            continue;

        TileRule *rule = new TileRule;
        TileRule::loadProperties(rule, reinterpret_cast<void*>(node));
        rules[rule->name] = rule;
    }

    if (TileRule::findByName("default") == NULL)
        errorFatal("no 'default' rule found in tileRules.xml");

    xmlFree(doc);
}

/**
 * Load properties for the current rule node 
 */
bool TileRule::loadProperties(TileRule *rule, void *xmlNode) {
    xmlNodePtr node = reinterpret_cast<xmlNodePtr>(xmlNode);
    unsigned int i;
    
    static const struct {
        const char *name;
        unsigned int mask;        
    } booleanAttributes[] = {        
        { "dispel", MASK_DISPEL },
        { "talkover", MASK_TALKOVER },
        { "door", MASK_DOOR },
        { "lockeddoor", MASK_LOCKEDDOOR },
        { "chest", MASK_CHEST },
        { "ship", MASK_SHIP },
        { "horse", MASK_HORSE },
        { "balloon", MASK_BALLOON },
        { "canattackover", MASK_ATTACKOVER },
        { "canlandballoon", MASK_CANLANDBALLOON },
        { "replacement", MASK_REPLACEMENT }
    };

    static const struct {
        const char *name;
        unsigned int mask;      
    } movementBooleanAttr[] = {        
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },       
        { "unflyable", MASK_UNFLYABLE },       
        { "creatureunwalkable", MASK_CREATURE_UNWALKABLE }        
    };
    static const char *speedEnumStrings[] = { "fast", "slow", "vslow", "vvslow", NULL };
    static const char *effectsEnumStrings[] = { "none", "fire", "sleep", "poison", "poisonField", "electricity", "lava", NULL };

    rule->mask = 0;
    rule->movementMask = 0;
    rule->speed = FAST;
    rule->effect = EFFECT_NONE;
    rule->walkonDirs = MASK_DIR_ALL;
    rule->walkoffDirs = MASK_DIR_ALL;    
    rule->name = xmlGetPropAsStr(node, "name");    

    for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
        if (xmlGetPropAsBool(node, booleanAttributes[i].name))
            rule->mask |= booleanAttributes[i].mask;        
    }

    for (i = 0; i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]); i++) {
        if (xmlGetPropAsBool(node, movementBooleanAttr[i].name))
            rule->movementMask |= movementBooleanAttr[i].mask;
    }

    if (xmlPropCmp(node, "cantwalkon", "all") == 0)
        rule->walkonDirs = 0;
    else if (xmlPropCmp(node, "cantwalkon", "west") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "north") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "east") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "south") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "advance") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkonDirs);
    else if (xmlPropCmp(node, "cantwalkon", "retreat") == 0)
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkonDirs);

    if (xmlPropCmp(node, "cantwalkoff", "all") == 0)
        rule->walkoffDirs = 0;
    else if (xmlPropCmp(node, "cantwalkoff", "west") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "north") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "east") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "south") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "advance") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkoffDirs);
    else if (xmlPropCmp(node, "cantwalkoff", "retreat") == 0)
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkoffDirs);

    rule->speed = (TileSpeed)xmlGetPropAsEnum(node, "speed", speedEnumStrings);
    rule->effect = (TileEffect)xmlGetPropAsEnum(node, "effect", effectsEnumStrings);

    return true;
}


/**
 * Tileset Class Implementation
 */

/* static member variables */
TileId              Tileset::currentId = 0;
Tileset::TilesetMap Tileset::tilesets;    
Tileset*            Tileset::current = NULL;

/**
 * Loads all tilesets using the filename
 * indicated by 'filename' as a definition
 */
void Tileset::loadAll(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;

    unloadAll();

    /* open the filename for the tileset and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilesets") != 0)
        errorFatal("malformed %s", filename.c_str());

    /* load tile rules from xml */
    if (!TileRule::rules.size())
        TileRule::load("tileRules.xml");

    /* load tile maps from xml */
    if (!TileMap::size())
        TileMap::loadAll(filename);

    /* load all of the tilesets */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tileset") != 0)            
            continue;

        Tileset *tileset = new Tileset;
        
        /* get filename of each tileset */
        string tilesetFilename = xmlGetPropAsStr(node, "file");
        /* load the tileset! */
        tileset->load(tilesetFilename);

        tilesets[tileset->name] = tileset;
    }

    /* make the current tileset the first one encountered */
    set(tilesets.begin()->second);    
}

/**
 * Delete all tilesets
 */
void Tileset::unloadAll() {
    TilesetMap::iterator i;

    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        i->second->unload();
        delete i->second;
    }
    tilesets.clear();

    TileMap::unloadAll();
    currentId = 0;
}

/**
 * Returns the tileset with the given name, if it exists
 */
Tileset* Tileset::get(string name) {
    if (tilesets.find(name) != tilesets.end())
        return tilesets[name];
    else return NULL;
}

/**
 * Returns the next unique id for a tile
 */
TileId Tileset::getNextTileId() {
    return currentId++;
}

/**
 * Returns the tile that has the given name from any tileset, if there is one
 */
Tile* Tileset::findTileByName(string name) {
    TilesetMap::iterator i;
    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        Tile *t = i->second->getByName(name);
        if (t)
            return t;        
    }

    return NULL;
}

/**
 * Returns the current tileset
 */ 
Tileset* Tileset::get(void) {
    return current;
}

/**
 * Sets the current tileset
 */ 
void Tileset::set(Tileset* t) {
    current = t;
}

/**
 * Loads a tileset from the .xml file indicated by 'filename'
 */
void Tileset::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;        
    
    /* open the filename for the group and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tileset") != 0)
        errorFatal("malformed %s", filename.c_str());

    name = xmlGetPropAsStr(root, "name");
    if (xmlPropExists(root, "imageName"))
        imageName = xmlGetPropAsStr(root, "imageName");
    if (xmlPropExists(root, "extends"))
        extends = Tileset::get(xmlGetPropAsStr(root, "extends"));
    else extends = NULL;    

    int index = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tile") != 0)
            continue;
        
        Tile *tile = new Tile;
        Tile::loadProperties(tile, node);

        /* set the base index for the tile (if it isn't already set explicitly) */
        if (tile->index != -1)
            index = tile->index;
        else            
            tile->index = index;        

        /* grab a unique id for the tile */
        tile->id = getNextTileId();
        
        /* assign the tileset this tile belongs to */
        tile->tileset = this;

        /* the tiles will load their own images when needed */
        tile->image = NULL;

        /* add the tile to our tileset */
        tiles[tile->id] = tile;
        nameMap[tile->name] = tile;
        
        index += tile->frames;
    }
    totalFrames = index;   
    
    xmlFree(doc);
}

/**
 * Unload the current tileset
 */
void Tileset::unload() {
    Tileset::TileIdMap::iterator i;    
        
    /* free all the memory for the tiles */
    for (i = tiles.begin(); i != tiles.end(); i++)
        delete i->second;    

    tiles.clear();
    totalFrames = 0;
    imageName.erase();    
}

/**
 * Returns the tile with the given id in the tileset
 */
Tile* Tileset::get(TileId id) {
    if (tiles.find(id) != tiles.end())
        return tiles[id];
    else if (extends)
        return extends->get(id);
    return NULL;    
}

/**
 * Returns the tile with the given name from the tileset, if it exists
 */
Tile* Tileset::getByName(string name) {
    if (nameMap.find(name) != nameMap.end())
        return nameMap[name];
    else if (extends)
        return extends->getByName(name);
    else return NULL;
}

/**
 * Returns the image name for the tileset, if it exists
 */
string Tileset::getImageName() const {
    if (imageName.empty() && extends)
        return extends->getImageName();
    else return imageName;
}

/**
 * Returns the number of tiles in the tileset
 */
unsigned int Tileset::numTiles() const {
    return tiles.size();
}

/**
 * Returns the total number of frames in the tileset
 */ 
unsigned int Tileset::numFrames() const {
    return totalFrames;
}

