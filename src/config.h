/*
 * config.h
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <libxml/xmlmemory.h>

class ImageSet;
class Armor;
class Weapon;
class ConfigElement;

/**
 * Config is a singleton data provider interface which hides the storage
 * format of the game configuration.
 * It provides data in a form easily digestible for game engine modules.
 */
class Config {
public:
    Config();
    ~Config();

    //const char** getGames();
    //void setGame(const char* name);

    // Primary configurable elements.
    const char* musicFile( uint32_t id );
    const char* soundFile( uint32_t id );
    const char** schemeNames();
    ImageSet* newScheme( uint32_t id );
    const Armor*  armor( uint32_t id );
    const Weapon* weapon( uint32_t id );
    int armorType( const char* name );
    int weaponType( const char* name );
    // More to be added...

    // Deprecated methods for manually parsing a tree.
    ConfigElement getElement(const std::string &name) const;
};

/**
 * NOTE: This class is deprecated and will be eliminated over time.
 *
 * A single configuration element in the config tree.
 */
class ConfigElement {
public:
    ConfigElement(xmlNodePtr xmlNode);
    ConfigElement(const ConfigElement &e);

    ConfigElement &operator=(const ConfigElement &e);

    const std::string getName() const { return name; }

    bool exists(const std::string &name) const;
    std::string getString(const std::string &name) const;
    int getInt(const std::string &name, int defaultValue = 0) const;
    bool getBool(const std::string &name) const;
    int getEnum(const std::string &name, const char *enumValues[]) const;

    std::vector<ConfigElement> getChildren() const;

    xmlNodePtr getNode() const { return node; }

private:
    xmlNodePtr node;
    std::string name;
};

extern Config* configInit();
extern void    configFree(Config*);

#define foreach(it, col)    for(it = col.begin(); it != col.end(); ++it)

#endif /* CONFIG_H */
