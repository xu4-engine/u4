/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <vector>

#include "tileset.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include "tilemap.h"
#include "xml.h"

using std::vector;

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
void TileRule::load() {
    const Config *config = Config::getInstance();
    vector<ConfigElement> rules = config->getElement("/config/tileRules").getChildren();

    for (std::vector<ConfigElement>::iterator i = rules.begin(); i != rules.end(); i++) {
        TileRule *rule = new TileRule;
        rule->initFromConf(*i);
        TileRule::rules[rule->name] = rule;
    }    
    
    if (TileRule::findByName("default") == NULL)
        errorFatal("no 'default' rule found in tile rules");
}

/**
 * Load properties for the current rule node 
 */
bool TileRule::initFromConf(const ConfigElement &conf) {    
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

    this->mask = 0;
    this->movementMask = 0;
    this->speed = FAST;
    this->effect = EFFECT_NONE;
    this->walkonDirs = MASK_DIR_ALL;
    this->walkoffDirs = MASK_DIR_ALL;    
    this->name = conf.getString("name");

    for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
        if (conf.getBool(booleanAttributes[i].name))
            this->mask |= booleanAttributes[i].mask;        
    }

    for (i = 0; i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]); i++) {
        if (conf.getBool(movementBooleanAttr[i].name))
            this->movementMask |= movementBooleanAttr[i].mask;
    }

    string cantwalkon = conf.getString("cantwalkon");
    if (cantwalkon == "all")
        this->walkonDirs = 0;
    else if (cantwalkon == "west")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkonDirs);
    else if (cantwalkon == "north")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkonDirs);
    else if (cantwalkon == "east")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkonDirs);
    else if (cantwalkon == "south")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkonDirs);
    else if (cantwalkon == "advance")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkonDirs);
    else if (cantwalkon == "retreat")
        this->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkonDirs);

    string cantwalkoff = conf.getString("cantwalkoff");
    if (cantwalkoff == "all")
        this->walkoffDirs = 0;
    else if (cantwalkoff == "west")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, this->walkoffDirs);
    else if (cantwalkoff == "north")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, this->walkoffDirs);
    else if (cantwalkoff == "east")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, this->walkoffDirs);
    else if (cantwalkoff == "south")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, this->walkoffDirs);
    else if (cantwalkoff == "advance")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, this->walkoffDirs);
    else if (cantwalkoff == "retreat")
        this->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, this->walkoffDirs);

    this->speed = static_cast<TileSpeed>(conf.getEnum("speed", speedEnumStrings));
    this->effect = static_cast<TileEffect>(conf.getEnum("effect", effectsEnumStrings));

    return true;
}


/**
 * Tileset Class Implementation
 */

/* static member variables */
TileId              Tileset::currentId = 0;
Tileset::TilesetMap Tileset::tilesets;    
Tileset*            Tileset::current = NULL;
ImageInfo*          Tileset::tilesInfo = NULL;

/**
 * Loads all tilesets using the filename
 * indicated by 'filename' as a definition
 */
void Tileset::loadAll(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    Debug dbg("debug/tileset.txt", "Tileset");

    TRACE(dbg, "Unloading all tilesets");    
    unloadAll();

    /* open the filename for the tileset and parse it! */
    TRACE_LOCAL(dbg, string("Parsing ") + filename);
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilesets") != 0)
        errorFatal("malformed %s", filename.c_str());

    /* load tile rules from xml */
    TRACE_LOCAL(dbg, "Loading tile rules");
    if (!TileRule::rules.size())
        TileRule::load();

    /* load all of the tilesets */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tileset") != 0)            
            continue;

        Tileset *tileset = new Tileset;
        
        /* get filename of each tileset */
        string tilesetFilename = xmlGetPropAsStr(node, "file");
        /* load the tileset! */

        TRACE_LOCAL(dbg, string("Loading tileset: ") + tilesetFilename);
        tileset->load(tilesetFilename);

        tilesets[tileset->name] = tileset;
    }

    /* make the current tileset the first one encountered */
    TRACE_LOCAL(dbg, "Setting default tileset");
    set(tilesets.begin()->second);

    /* load tile maps from xml, including translations from index to id */
    TRACE_LOCAL(dbg, "Loading tilemaps");
    if (!TileMap::size())
        TileMap::loadAll(filename);

    TRACE(dbg, "Successfully Loaded Tilesets");
}

/**
 * Delete all tilesets
 */
void Tileset::unloadAll() {
    TilesetMap::iterator i;
    
    // unload all tilemaps
    TileMap::unloadAll();
    
    // This doesn't unload the tiles info -- that happens elsewhere
    // This simply ensures that we have a valid pointer if the game images
    // are reloaded for some reason.
    tilesInfo = NULL;

    for (i = tilesets.begin(); i != tilesets.end(); i++) {
        i->second->unload();
        delete i->second;
    }
    tilesets.clear();
    
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
 * Returns a pointer to the current image info
 */ 
ImageInfo* Tileset::getImageInfo() {
    return tilesInfo;
}

/**
 * Loads a tileset from the .xml file indicated by 'filename'
 */
void Tileset::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    Debug dbg("debug/tileset.txt", "Tileset", true);
    
    TRACE_LOCAL(dbg, string("\tParsing ") + filename);
    
    /* open the filename for the group and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tileset") != 0)
        errorFatal("malformed %s", filename.c_str());

    TRACE_LOCAL(dbg, string("\tRetreiving global tileset info...") + filename);

    name = xmlGetPropAsStr(root, "name");
    if (xmlPropExists(root, "imageName"))
        imageName = xmlGetPropAsStr(root, "imageName");
    if (xmlPropExists(root, "extends"))
        extends = Tileset::get(xmlGetPropAsStr(root, "extends"));
    else extends = NULL;

    TRACE_LOCAL(dbg, "\tLoading Tiles...");

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

        TRACE_LOCAL(dbg, string("\t\tLoaded '") + tile->name + "'");

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
