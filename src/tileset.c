/*
 * $Id$
 */

#include <stddef.h>
#include <string.h>
#include <libxml/xmlmemory.h>

#include "tileset.h"

#include "error.h"
#include "list.h"
#include "screen.h"
#include "settings.h"
#include "xml.h"

/* global variables */
ListNode *tilesetList = NULL;
TileRule *tilesetRules = NULL;
int numRules = 0;

/**
 * Function Prototypes
 */
void tilesetLoad(const char *filename, TilesetType type);
void tilesetLoadRulesFromXml();
int tilesetLoadRuleProperties(TileRule *rule, xmlNodePtr node);

/**
 * Loads all tilesets using the filename
 * indicated by 'tilesetFilename' as a definition
 */
void tilesetLoadAllTilesetsFromXml(const char *tilesetFilename) {
    xmlDocPtr doc;
    xmlNodePtr root, node;    

    tilesetDeleteAllTilesets();

    /* open the filename for the tileset and parse it! */
    doc = xmlParse(tilesetFilename);
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tilesets") != 0)
        errorFatal("malformed %s", tilesetFilename);

    /* load tile rules from xml */
    tilesetLoadRulesFromXml();

    /* load all of the tilesets */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        const char *filename;
        TilesetType type;
                
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, "tileset") != 0)
            continue;
        
        /* get filename and type of each tileset */
        filename = xmlGetPropAsStr(node, "file");        
        type = tilesetGetTypeByStr(xmlGetPropAsStr(node, "type"));
    
        /* load the tileset! */
        tilesetLoad(filename, type);
    }
}

/**
 * Delete all tilesets
 */
void tilesetDeleteAllTilesets() {
    ListNode *node = tilesetList;
    Tileset* tileset;
    
    while(node) {
        tileset = (Tileset*)node->data;
        if (tileset && tileset->tiles) {
            free(tileset->tiles);
        }
        free(node->data);
        node = node->next;
    }

    listDelete(tilesetList);
    tilesetList = NULL;
}

/**
 * Loads a tileset from the .xml file indicated by 'filename'
 */
void tilesetLoad(const char *filename, TilesetType type) {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    Tileset *tileset;
    ListNode *list = tilesetList;    
    
    /* make sure we aren't loading the same type of tileset twice */
    while (list) {
        if (type == ((Tileset*)list->data)->type)
            errorFatal("error: tileset of type %d already loaded", type);
        list = list->next;
    }
    
    /* open the filename for the tileset and parse it! */
    doc = xmlParse(filename);
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tiles") != 0)
        errorFatal("malformed %s", filename);

    tileset = (Tileset *)malloc(sizeof(Tileset));
    if (tileset == NULL)
        errorFatal("error allocating memory for tileset");
    
    /* init all values to 0 */
    memset(tileset, 0, sizeof(Tileset));  
    
    tileset->type = type;

    if (xmlPropExists(root, "imageId"))
        tileset->imageId = xmlGetPropAsInt(root, "imageId");
    else
        tileset->imageId = 0;

    /* count how many tiles are in the tileset */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, "tile") != 0)
            continue;
        else tileset->numTiles++;
    }

    if (tileset->numTiles > 0) {
        /* FIXME: eventually, each tile definition won't be duplicated,
           so this will work as it should.  For now, we stick to 256 tiles
        tileset->tiles = (Tile*)malloc(sizeof(Tile) * (tileset->numTiles + 1));
        */
        tileset->tiles = (Tile*)malloc(sizeof(Tile) * 257);

        if (tileset->tiles == NULL)
            errorFatal("error allocating memory for tiles");
        
        for (node = root->xmlChildrenNode; node; node = node->next) {
            if (xmlNodeIsText(node) || xmlStrcmp(node->name, "tile") != 0)
                continue;
            
            tileLoadTileInfo(&tileset->tiles, tileset->totalFrames, node);
            tileset->totalFrames += tileset->tiles[tileset->totalFrames].frames;            
        }
    }
    else errorFatal("Error: no 'tile' nodes defined in %s", filename);    

    tilesetList = listAppend(tilesetList, tileset);
    xmlFree(doc);
}

/**
 * Returns the tileset of the given type, if it is already loaded
 */
Tileset *tilesetGetByType(TilesetType type) {
    ListNode *node;    
    Tileset *tileset;

    node = tilesetList;
    while (node) {
        tileset = (Tileset*)node->data;
        if (tileset->type == type)
            return (Tileset*)node->data;
        node = node->next;
    }
    
    errorFatal("Tileset of type %d not found", type);
    return NULL;
}

/**
 * Given the string representation of a tileset type, returns the
 * corresponding 'TilesetType' version.
 */

TilesetType tilesetGetTypeByStr(const char *type) {
    const char *types[] = { "base", "dungeon", "gem" };
    int i;

    for (i = TILESET_BASE; i < TILESET_MAX; i++) {
        if (strcasecmp(type, types[i]) == 0)
            return (TilesetType)i;
    }

    return TILESET_BASE;
}

/**
 * Load tile information from xml.
 */
void tilesetLoadRulesFromXml() {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    
    numRules = 0;
    if (tilesetRules)
        free(tilesetRules);

    doc = xmlParse("tileRules.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "tileRules") != 0)
        errorFatal("malformed tileRules.xml");

    /* first, we need to count how many rules we are loading */
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (xmlNodeIsText(node) || xmlStrcmp(node->name, "rule") != 0)
            continue;
        else numRules++;        
    }

    if (numRules == 0)
        return;
    else {
        int i = 0;

        tilesetRules = (TileRule *)malloc(sizeof(TileRule) * (numRules + 1));
        if (!tilesetRules)
            errorFatal("Error allocating memory for tile rules");

        for (node = root->xmlChildrenNode; node; node = node->next) {
            if (xmlNodeIsText(node) || xmlStrcmp(node->name, "rule") != 0)
                continue;
            
            tilesetLoadRuleProperties(&tilesetRules[i++], node);
        }
    }

    if (tilesetFindRuleByName("default") == NULL)
        errorFatal("no 'default' rule found in tileRules.xml");

    xmlFree(doc);
}

/**
 * Load properties for the current rule node 
 */
int tilesetLoadRuleProperties(TileRule *rule, xmlNodePtr node) {
    int i;
    
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
        { "monsterunwalkable", MASK_MONSTER_UNWALKABLE }        
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

    rule->speed = xmlGetPropAsEnum(node, "speed", speedEnumStrings);
    rule->effect = xmlGetPropAsEnum(node, "effect", effectsEnumStrings);

    return 1;
}

TileRule *tilesetFindRuleByName(const char *name) {
    int i;
    
    if (!name)
        return NULL;
    
    for (i = 0; i < numRules; i++)
        if (strcasecmp(name, tilesetRules[i].name) == 0)
            return &tilesetRules[i];

    return NULL;
}
