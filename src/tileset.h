/*
 * $Id$
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include "tile.h"

using std::string;

typedef enum {
    TILESET_BASE,
    TILESET_DUNGEON,
    TILESET_GEM,
    TILESET_MAX
} TilesetType;

typedef std::map<TilesetType, class Tileset *> TilesetMap;
typedef std::map<string, class TileRule *> TileRuleMap;

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
    static void load(string filename, TilesetType type);
    static void loadAll(string filename);
    static void unloadAll();
    static Tileset *get(TilesetType type);
    static TilesetType getTypeByStr(string type);
    
    static TilesetMap tilesets;  // A map of tileset types to tilesets
    
    std::map<int, int> indexMap; // Maps indices to tile ids

    TilesetType type;
    TileVector tiles;
    int totalFrames;
    string imageName;
};

#endif
