/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "tilemap.h"

#include "error.h"
#include "xml.h"

/**
 * Static variables
 */
TileMap::TileIndexMapMap TileMap::tileMaps;

/**
 * Load all tilemaps from the specified xml file
 */
void TileMap::loadAll(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;

    unloadAll();

    /* open the filename for the tileset and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);    
    
    /* load all of the tilemaps */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tilemap") != 0)            
            continue;            
     
        /* get filename of the tilemap */
        string tilemapFilename = xmlGetPropAsStr(node, "file");
        /* load the tilemap ! */
        load(tilemapFilename);        
    }
}
 
/**
 * Delete all tilemaps
 */
void TileMap::unloadAll() {    
    TileIndexMapMap::iterator map;       
        
    /* free all the memory for the tile maps */
    for (map = tileMaps.begin(); map != tileMaps.end(); map++)
        delete map->second;
}
 
/**
 * Loads a tile map which translates between tile indices and tile names
 * Tile maps are useful to translate from dos tile indices to xu4 tile ids
 */
void TileMap::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;        
    
    /* open the filename for the group and parse it! */
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilemap") != 0)
        errorFatal("malformed %s", filename.c_str());

    TileIndexMap* tileMap = new TileIndexMap;
    
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
 * Returns the Tile index map with the specified name
 */
TileIndexMap* TileMap::get(string name) {
    if (tileMaps.find(name) != tileMaps.end())
        return tileMaps[name];
    else return NULL;    
}

/**
 * Returns the number of tile maps currently loaded
 */
int TileMap::size() {
    return tileMaps.size();
}

/**
 * Translates a tile into a maptile 
 */
const string& TileMap::getName(unsigned int tile, string map) {
    TileIndexMap* im = get(map);
    static string empty;

    if (im)
        return (*im)[tile];
    else
        return empty;;
}
