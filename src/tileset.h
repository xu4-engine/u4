/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include "tile.h"
#include "types.h"

using std::string;

typedef std::map<string, class TileRule *> TileRuleMap;
typedef std::map<unsigned int, string> TileMap;

/**
 * TileRule class
 */
class TileRule {
public:    
    static TileRule *findByName(string name);
    static void load(string filename);
    static bool loadProperties(TileRule *rule, void *xmlNode);
    static TileRuleMap rules;   // A map of rule names to rules

    string name;
    unsigned short mask;    
    unsigned short movementMask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
};

/**
 * Tileset class
 */
class Tileset {       
public:    
    typedef std::map<string, TileMap*> TileMapMap;

    static void loadTileMap(string filename);
    static void loadGroup(string filename);
    static void load(string filename);
    static void unload();    
    static TileId getNextTileId();
    
    static TileId currentId;    
    static TileVector tiles;
    static int totalFrames;
    static string imageName;        
    
    static TileMapMap tileMaps;
};

#endif
