/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "tilemap.h"

#include "debug.h"
#include "error.h"
#include "tileset.h"
#include "xml.h"

Debug dbg("debug_tilemap.txt", "TileMap");

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

    TRACE_LOCAL(dbg, "Unloading all tilemaps");
    unloadAll();

    /* open the filename for the tileset and parse it! */
    TRACE_LOCAL(dbg, string("Parsing ") + filename);
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);    
    
    /* load all of the tilemaps */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"tilemap") != 0)            
            continue;            
     
        /* get filename of the tilemap */
        string tilemapFilename = xmlGetPropAsStr(node, "file");
        
        /* load the tilemap ! */
        TRACE(dbg, string("Loading tilemap: ") + tilemapFilename);
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
    
    /* Clear the map so we don't attempt to delete the memory again
     * next time.
     */
    tileMaps.clear();
}
 
/**
 * Loads a tile map which translates between tile indices and tile names
 * Tile maps are useful to translate from dos tile indices to xu4 tile ids
 */
void TileMap::load(string filename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;    
    
    /* open the filename for the group and parse it! */
    TRACE_LOCAL(dbg, string("Parsing ") + filename);
    doc = xmlParse(filename.c_str());
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilemap") != 0)
        errorFatal("malformed %s", filename.c_str());

    TileIndexMap* tileMap = new TileIndexMap;
    
    string name = xmlGetPropAsStr(root, "name");    
    TRACE_LOCAL(dbg, string("Tilemap name is: ") + name);
    
    int index = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, (xmlChar *)"map") != 0)
            continue;

        /* we assume tiles have already been loaded at this point,
           so let's do some translations! */
        
        int frames = 1;
        string tile = xmlGetPropAsStr(node, "tile");        

        TRACE_LOCAL(dbg, string("\tLoading '") + tile + "'");
        
        /* find the tile this references */
        Tile *t = Tileset::findTileByName(tile);
        if (!t)
            errorFatal("Error: tile '%s' from '%s' was not found in any tileset", tile.c_str(), filename.c_str());
        
        if (xmlPropExists(node, "index"))
            index = xmlGetPropAsInt(node, "index");        
        if (xmlPropExists(node, "frames"))
            frames = xmlGetPropAsInt(node, "frames");        

        /* insert the tile into the tile map */
        for (int i = 0; i < frames; i++) {
            if (i < t->frames)
                (*tileMap)[index+i] = MapTile(t->id, i);
            /* frame fell out of the scope of the tile -- frame is set to 0 */
            else (*tileMap)[index+i] = MapTile(t->id, 0);
        }
        
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
