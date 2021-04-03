/*
 * $Id$
 */

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cctype>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>

#if defined(MACOSX) || defined(IOS)
#include <CoreFoundation/CoreFoundation.h>
#endif

// we rely on xinclude support
#ifndef LIBXML_XINCLUDE_ENABLED
#error "xinclude not available: libxml2 must be compiled with xinclude support"
#endif

#include "config.h"
#include "error.h"
#include "imageloader.h"
#include "imagemgr.h"
#include "settings.h"
#include "sound.h"
#include "u4file.h"
#include "xu4.h"

using namespace std;

extern bool verbose;

#if 0
// For future expansion...
const char** Config::getGames() {
    return &"Ultima IV";
}

void Config::setGame(const char* name) {
}
#endif

//--------------------------------------
// XML Backend

struct XMLConfig
{
    xmlDocPtr doc;
    string sbuf;        // Temporary buffer for const char* return values.
    vector<const char*> sarray;   // Temp. buffer for const char** values.
    vector<string> musicFiles;
    vector<string> soundFiles;
    vector<string> schemeNames;
};

static const char* configXmlPath = "config.xml";
static XMLConfig xcd;

ConfigElement Config::getElement(const string &name) const {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    string path = "/config/" + name;
    context = xmlXPathNewContext(xcd.doc);
    result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(path.c_str()), context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval))
        errorFatal("no match for xpath %s\n", path.c_str());

    xmlXPathFreeContext(context);

    if (result->nodesetval->nodeNr > 1)
        errorWarning("more than one match for xpath %s\n", path.c_str());

    xmlNodePtr node = result->nodesetval->nodeTab[0];
    xmlXPathFreeObject(result);

    return ConfigElement(node);
}

static void *conf_fileOpen(const char *filename) {
    void *result;
    string pathname(u4find_conf(filename));

    if (pathname.empty())
        return NULL;
    result = xmlFileOpen(pathname.c_str());

    if (verbose)
        printf("xml parser opened %s: %s\n", pathname.c_str(), result ? "success" : "failed");

    return result;
}

static void conf_accumError(void *l, const char *fmt, ...) {
    string* errorMessage = static_cast<string *>(l);
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    errorMessage->append(buffer);
}

Config::Config() {
    xmlRegisterInputCallbacks(&xmlFileMatch, &conf_fileOpen, xmlFileRead, xmlFileClose);

    xcd.doc = xmlParseFile(configXmlPath);
    if (!xcd.doc) {
        printf("Failed to read main configuration file '%s'", configXmlPath);
        errorFatal("error parsing main config.");
    }

    xmlXIncludeProcess(xcd.doc);

    if (xu4.settings->validateXml && xcd.doc->intSubset) {
        string errorMessage;
        xmlValidCtxt cvp;

        if (verbose)
            printf("validating config.xml\n");

        cvp.userData = &errorMessage;
        cvp.error = &conf_accumError;

        // Error changed to not fatal due to regression in libxml2
        if (!xmlValidateDocument(&cvp, xcd.doc))
            errorWarning("xml validation error:\n%s", errorMessage.c_str());
    }

    // Load primary elements.

    // musicFile
    {
    xcd.musicFiles.reserve(MUSIC_MAX);
    xcd.musicFiles.push_back("");    // filename for MUSIC_NONE;
    vector<ConfigElement> ce = getElement("music").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "track") {
            xcd.musicFiles.push_back(it->getString("file"));
        }
    }
    }

    // soundFile
    {
    xcd.soundFiles.reserve(SOUND_MAX);
    vector<ConfigElement> ce = getElement("sound").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "track") {
            xcd.soundFiles.push_back(it->getString("file"));
        }
    }
    }

    // schemeNames
    {
    vector<ConfigElement> ce = getElement("graphics").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "imageset") {
            xcd.schemeNames.push_back(it->getString("name"));

            /*
            // register all the images declared in the config files
            ImageSet *set = loadImageSet(ce);
            imageSets[set->name] = set;
            */
        }
    }
    }
}

Config::~Config() {
    xmlFreeDoc(xcd.doc);
}

ConfigElement::ConfigElement(xmlNodePtr xmlNode) :
    node(xmlNode), name(reinterpret_cast<const char *>(xmlNode->name)) {
}

ConfigElement::ConfigElement(const ConfigElement &e) :
    node(e.node), name(e.name) {
}

ConfigElement &ConfigElement::operator=(const ConfigElement &e) {
    if (&e != this) {
        node = e.node;
        name = e.name;
    }
    return *this;
}

/**
 * Returns true if the property exists in the current config element
 */
bool ConfigElement::exists(const std::string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    bool exists = prop != NULL;
    xmlFree(prop);

    return exists;
}

string ConfigElement::getString(const string &name) const {
    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return "";

    string result(reinterpret_cast<const char *>(prop));
    xmlFree(prop);

    return result;
}

int ConfigElement::getInt(const string &name, int defaultValue) const {
    long result;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return defaultValue;

    result = strtol(reinterpret_cast<const char *>(prop), NULL, 0);
    xmlFree(prop);

    return static_cast<int>(result);
}

bool ConfigElement::getBool(const string &name) const {
    int result;

    xmlChar *prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return false;

    if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>("true")) == 0)
        result = true;
    else
        result = false;

    xmlFree(prop);

    return result;
}

int ConfigElement::getEnum(const string &name, const char *enumValues[]) const {
    int result = -1, i;
    xmlChar *prop;

    prop = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
    if (!prop)
        return 0;

    for (i = 0; enumValues[i]; i++) {
        if (xmlStrcmp(prop, reinterpret_cast<const xmlChar *>(enumValues[i])) == 0)
        result = i;
    }

    if (result == -1)
        errorFatal("invalid enum value for %s: %s", name.c_str(), prop);

    xmlFree(prop);

    return result;
}

vector<ConfigElement> ConfigElement::getChildren() const {
    vector<ConfigElement> result;

    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_ELEMENT_NODE)
            result.push_back(ConfigElement(child));
    }

    return result;
}

/*
 * Return a filename pointer for the given MusicTrack id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::musicFile( uint32_t id ) {
    if (id < xcd.musicFiles.size()) {
        xcd.sbuf = u4find_music(xcd.musicFiles[id]);
        return xcd.sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a filename pointer for the given Sound id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::soundFile( uint32_t id ) {
    if (id < xcd.soundFiles.size()) {
        xcd.sbuf = u4find_sound(xcd.soundFiles[id]);
        return xcd.sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a C string pointer array for the available Image scheme names.
 */
const char** Config::schemeNames() {
    xcd.sarray.clear();
    vector<string>::iterator it;
    for (it = xcd.schemeNames.begin(); it != xcd.schemeNames.end(); ++it)
        xcd.sarray.push_back( (*it).c_str() );
    xcd.sarray.push_back(NULL);
    return &xcd.sarray.front();
}

//--------------------------------------
// Graphics config

static SubImage* loadSubImage(const ImageInfo *info, const ConfigElement &conf) {
    SubImage *subimage;
    static int x = 0,
               y = 0,
               last_width = 0,
               last_height = 0;

    subimage = new SubImage;
    subimage->name = conf.getString("name");
    subimage->width = conf.getInt("width");
    subimage->height = conf.getInt("height");
    subimage->srcImageName = info->name;
    if (conf.exists("x") && conf.exists("y")) {
        x = subimage->x = conf.getInt("x");
        y = subimage->y = conf.getInt("y");
    }
    else {
        // Automatically increment our position through the base image
        x += last_width;
        if (x >= last_width) {
            x = 0;
            y += last_height;
        }

        subimage->x = x;
        subimage->y = y;
    }

    // "remember" the width and height of this subimage
    last_width = subimage->width;
    last_height = subimage->height;

    return subimage;
}

static uint8_t mapFiletype(const string& str, const string& file) {
    if (! str.empty()) {
        if (str == "image/x-u4raw")
            return FTYPE_U4RAW;
        if (str == "image/x-u4rle")
            return FTYPE_U4RLE;
        if (str == "image/x-u4lzw")
            return FTYPE_U4LZW;
        if (str == "image/x-u5lzw")
            return FTYPE_U5LZW;
        if (str == "image/png")
            return FTYPE_PNG;
        if (str == "image/fmtowns")
            return FTYPE_FMTOWNS;
        if (str == "image/fmtowns-pic")
            return FTYPE_FMTOWNS_PIC;
        if (str == "image/fmtowns-tif")
            return FTYPE_FMTOWNS_TIF;
    }

    // Guess at type based on filename.
    size_t length = file.length();
    if (length >= 4 && file.compare(length - 4, 4, ".png") == 0)
        return FTYPE_PNG;

    errorWarning("Unknown image filetype %s", str.c_str());
    return FTYPE_UNKNOWN;
}

static ImageInfo* loadImageInfo(const ConfigElement &conf) {
    ImageInfo *info;
    static const char *fixupEnumStrings[] = { "none", "intro", "abyss", "abacus", "dungns", "blackTransparencyHack", "fmtownsscreen", NULL };

    info = new ImageInfo;
    info->name = conf.getString("name");
    info->filename = conf.getString("filename");
    info->width = conf.getInt("width", -1);
    info->height = conf.getInt("height", -1);
    info->depth = conf.getInt("depth", -1);
    info->prescale = conf.getInt("prescale");
    info->filetype = mapFiletype(conf.getString("filetype"), info->filename);
    info->tiles = conf.getInt("tiles");
    info->introOnly = conf.getBool("introOnly");
    info->transparentIndex = conf.getInt("transparentIndex", -1);

    info->xu4Graphic = conf.getBool("xu4Graphic");
    info->fixup = static_cast<ImageFixup>(conf.getEnum("fixup", fixupEnumStrings));
    info->image = NULL;

    vector<ConfigElement> children = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() == "subimage") {
            SubImage *subimage = loadSubImage(info, *i);
            info->subImages[subimage->name] = subimage;
        }
    }

    return info;
}

static ImageSet* loadImageSet(const ConfigElement &conf) {
    ImageSet *set = new ImageSet;

    set->name    = conf.getString("name");
    set->extends = conf.getString("extends");

    std::vector<ConfigElement>::iterator it;
    std::map<string, ImageInfo *>::iterator dup;
    vector<ConfigElement> children = conf.getChildren();

    foreach (it, children) {
        if (it->getName() == "image") {
            ImageInfo *info = loadImageInfo(*it);
            dup = set->info.find(info->name);
            if (dup != set->info.end()) {
                delete dup->second;
                set->info.erase(dup);
            }
            set->info[info->name] = info;
        }
    }

    return set;
}

/*
 * Return ImageSet pointer which caller must delete.
 */
ImageSet* Config::newScheme( uint32_t id ) {
    uint32_t n = 0;
    vector<ConfigElement> ce = getElement("graphics").getChildren();
    vector<ConfigElement>::const_iterator it  = ce.begin();
    vector<ConfigElement>::const_iterator end = ce.end();
    for (; it != end; ++it) {
        if (it->getName() == "imageset") {
            if( n == id )
                return loadImageSet(*it);
            ++n;
        }
    }
    return NULL;
}

//--------------------------------------
// Config Service API

// Create configService.
Config* configInit() {
    // Here's where we can compile the program with alternate back-ends
    // (e.g. SQL, JSON, ... or something better.)

    return new Config;
}

void configFree(Config* conf) {
    delete conf;
}
