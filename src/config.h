/*
 * config.h
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "types.h"

#ifdef CONF_MODULE
#include "cdi.h"
#endif

class ImageSet;
struct AtlasSubImage;
struct Layout;
struct Armor;
struct Weapon;
class Coords;
class Creature;
class Map;
struct TileRule;
class Tileset;
class TileAnimSet;
class ConfigElement;
struct RGBA;
struct UltimaSaveIds;

#ifdef USE_BORON
struct UThread;
#endif

/**
 * Config is a singleton data provider interface which hides the storage
 * format of the game configuration.
 * It provides data in a form easily digestible for game engine modules.
 */
class Config {
public:
    virtual ~Config();

    char* gameTitle(char* buf) const;
    const char* symbolName( Symbol s ) const;
    Symbol intern( const char* name );
    void internSymbols( Symbol* table, uint16_t count, const char* name );
    const char* confString( StringId id ) const;

    // Primary configurable elements.
    const RGBA* egaPalette();
    const Layout* layouts( uint32_t* plen ) const;
#ifdef CONF_MODULE
#ifdef USE_BORON
    UThread* boronThread() const;
    int scriptItemId(Symbol name);
    const void* scriptEvalArg(const char* fmt, ...);
    int32_t npcTalk(uint32_t appId);
#endif
    void* loadFile(const char* sourceFilename) const;
    const char* modulePath(const CDIEntry*) const;
    const CDIEntry* fileEntry( const char* sourceFilename ) const;
    const CDIEntry* imageFile( const char* id ) const;
    const CDIEntry* mapFile( uint32_t id ) const;
    const CDIEntry* musicFile( uint32_t id ) const;
    const CDIEntry* soundFile( uint32_t id ) const;
    const float*    voiceParts( uint32_t id ) const;
    int atlasImages(StringId spec, AtlasSubImage* images, int max);
    void changeSoundtrack(const char* modPath);
#else
    const char* musicFile( uint32_t id );
    const char* soundFile( uint32_t id );
#endif
    ImageSet* newImageSet() const;
    TileAnimSet* newTileAnims() const;
    float* newDrawList(Symbol name, int* plen) const;
    const Armor*  armor( uint32_t id );
    const Weapon* weapon( uint32_t id );
    int armorType( const char* name );
    int weaponType( const char* name );
    const Creature* creature( uint32_t id ) const;
    const Creature* creatureOfTile( TileId id ) const;
    const Creature* const* creatureTable( uint32_t* plen ) const;
    const TileRule* tileRule( Symbol name ) const;
    const Tileset* tileset() const;
    const UltimaSaveIds* usaveIds() const;
    Map* map(uint32_t id);
    Map* restoreMap(uint32_t id);
    const Coords* moongateCoords(int phase) const;

protected:
    void* backend;
};

extern Config* configInit(const char* module, const char* soundtrack);
extern void    configFree(Config*);

#define foreach(it, col)    for(it = col.begin(); it != col.end(); ++it)

#endif /* CONFIG_H */
