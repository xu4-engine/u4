/*
 * $Id$
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise
#include <string>
#include <vector>
#include <libxml/xmlmemory.h>

class ConfigElement;

/**
 * Singleton class that manages the XML configuration tree.
 */
class Config {
public:
    static const Config *getInstance();

    ConfigElement getElement(const std::string &path) const;

private:
    Config();
    static void *fileOpen(const char *filename);
    static void accumError(void *l, const char *fmt, ...);

    static Config *instance;
    xmlDocPtr doc;
};

/**
 * A single configuration element in the config tree.  Right now, a
 * thin wrapper around the XML DOM element.
 */
class ConfigElement {
public:
    ConfigElement(xmlNodePtr xmlNode);
    ConfigElement(const ConfigElement &e);
    ~ConfigElement();

    ConfigElement &operator=(const ConfigElement &e);

    const std::string getName() const { return name; }

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

#endif /* CONFIG_H */
