/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <libxml/xmlmemory.h>

#include "tileset.h"

#include "error.h"
#include "screen.h"
#include "settings.h"
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

TileVector  Tileset::tiles;
int         Tileset::totalFrames = 0;
string      Tileset::imageName;
TileId      Tileset::currentId = 0;
Tileset::TileMapMap Tileset::tileMaps;

/**
 * Loads all tilesets using the filename
 * indicated by 'filename' as a definition
 */
void Tileset::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;

    unload();

    /* open the filename for the tileset and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilesets") != 0)
        errorFatal("malformed %s", filename.c_str());

    /* get the image name for the tileset */
    if (xmlPropExists(root, "imageName"))
        imageName = xmlGetPropAsString(root, "imageName");

    /* load tile rules from xml */
    if (!TileRule::rules.size())
        TileRule::load("tileRules.xml");

    /* load all of the tilesets */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node))            
            continue;
        else if (xmlStrcmp(node->name, (xmlChar *)"tileset") == 0) {        
            /* get filename of each group */
            string groupFilename = xmlGetPropAsStr(node, "file");
            /* load the tile group! */
            loadGroup(groupFilename);
        }
        else if (xmlStrcmp(node->name, (xmlChar *)"tilemap") == 0) {
            /* get filename of the tilemap */
            string tilemapFilename = xmlGetPropAsStr(node, "file");
            /* load the tilemap ! */
            loadTileMap(tilemapFilename);
        }
    }
}

/**
 * Delete all tiles
 */
void Tileset::unload() {
    TileVector::iterator i;
    TileMapMap::iterator map;
        
    /* free all the memory for the tiles */
    for (i = tiles.begin(); i != tiles.end(); i++)
        delete *i;
    
    /* free all the memory for the tile maps */
    for (map = tileMaps.begin(); map != tileMaps.end(); map++)
        delete map->second;

    tiles.clear();
    totalFrames = 0;
    imageName.erase();
    currentId = 0;    
}

/**
 * Loads a tile map which translates between tile indices and tile names
 * Tile maps are useful to translate from dos tile indices to xu4 tile ids
 */
void Tileset::loadTileMap(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;        
    
    /* open the filename for the group and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilemap") != 0)
        errorFatal("malformed %s", filename.c_str());

    TileMap* tileMap = new TileMap;
    
    string name = xmlGetPropAsStr(root, "name");    
    
    int index = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"map") != 0)
            continue;
        
        int frames = 1;
        string tile = xmlGetPropAsStr(node, "tile");
        
        if (xmlPropExists(node, "index"))
            index = xmlGetPropAsInt(node, "index");
        if (xmlPropExists(node, "frames"))
            frames = xmlGetPropAsInt(node, "frames");

        /* insert the tile into the tile map */
        for (int i = 0; i < frames; i++)
            (*tileMap)[index+i] = tile;        

        index += frames;
    }
    
    /* add the tilemap to our list */
    tileMaps[name] = tileMap;
}

/**
 * Loads a tile group from the .xml file indicated by 'filename'
 */
void Tileset::loadGroup(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;        
    
    /* open the filename for the group and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tiles") != 0)
        errorFatal("malformed %s", filename.c_str());

    static int index = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tile") != 0)
            continue;
        
        Tile tile;
        Tile::loadProperties(&tile, node);
        
        /* set the base index for the tile */
        tile.index = index;

        /* grab a unique id for the tile */
        tile.id = getNextTileId();

        /* add the tile to our tileset */
        tiles.push_back(new Tile(tile));        
        
        index += tile.frames;
    }
    totalFrames = index;
    
    xmlFree(doc);
}

TileId Tileset::getNextTileId() {
    return currentId++;
}