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

class ConfigElement;

typedef std::map<string, class TileRule *> TileRuleMap;

/**
 * TileRule class
 */
class TileRule {
public:    
    static TileRule *findByName(string name);
    static void load();
    static TileRuleMap rules;   // A map of rule names to rules

    bool initFromConf(const ConfigElement &tileRuleConf);

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
    typedef std::map<string, Tileset*> TilesetMap;
    typedef std::map<TileId, Tile*> TileIdMap;
    typedef std::map<string, Tile*> TileStrMap;

    static void loadAll();
    static void unloadAll();
    static Tileset* get(string name);
    static TileId getNextTileId();
    static Tile* findTileByName(string name);        
    static Tileset* get(void);
    static void set(Tileset*);    

public:
    void load(string filename);
    void unload();
    Tile* get(TileId id);
    Tile* getByName(string name);
    string getImageName() const;
    unsigned int numTiles() const;
    unsigned int numFrames() const;    
    
private:
    static TilesetMap tilesets;
    static TileId currentId;    
    static Tileset* current;    

    string name;
    TileIdMap tiles;
    unsigned int totalFrames;
    string imageName;
    Tileset* extends;

    TileStrMap nameMap;
};

#endif
