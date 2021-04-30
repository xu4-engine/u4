/*
 * $Id$
 */

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
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
#include "names.h"
#include "savegame.h"
#include "settings.h"
#include "sound.h"
#include "tile.h"
#include "tileset.h"
#include "weapon.h"
#include "u4file.h"
#include "xu4.h"
#include "support/SymbolTable.h"

using namespace std;

extern bool verbose;

Config::~Config() {}

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
    SymbolTable sym;
    xmlDocPtr doc;
    string sbuf;        // Temporary buffer for const char* return values.
    vector<const char*> sarray;   // Temp. buffer for const char** values.
    RGBA* egaColors;
    vector<string> musicFiles;
    vector<string> soundFiles;
    vector<string> schemeNames;
    vector<Armor*> armors;
    vector<Weapon*> weapons;

    TileRule* tileRules;
    uint16_t tileRuleCount;
    int16_t  tileRuleDefault;
    Tileset* tileset;
    UltimaSaveIds usaveIds;
};

struct ConfigXML : public Config {
    ConfigXML();
    ~ConfigXML();

    XMLConfig xcd;
};

#define CB  static_cast<XMLConfig*>(backend)

static const char* configXmlPath = "config.xml";

ConfigElement Config::getElement(const string &name) const {
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    string path = "/config/" + name;
    context = xmlXPathNewContext(CB->doc);
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

static Symbol propertySymbol(SymbolTable& sym, const ConfigElement& ce, const char* name) {
    Symbol ns;
    xmlChar* prop = xmlGetProp(ce.getNode(), (const xmlChar*) name);
    if (! prop)
        return SYM_UNSET;
    ns = sym.intern((const char*) prop);
    xmlFree(prop);
    return ns;
}

Symbol Config::propSymbol(const ConfigElement& ce, const char* name) const {
    return propertySymbol(CB->sym, ce, name);
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

//--------------------------------------
// Tiles

static void conf_tileRule(SymbolTable& sym, TileRule* rule, const ConfigElement &conf) {
    unsigned int i;

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
        { "replacement", MASK_REPLACEMENT },
        { "foreground", MASK_FOREGROUND },
        { "onWaterOnlyReplacement", MASK_WATER_REPLACEMENT},
        { "livingthing", MASK_LIVING_THING }

    };

    static const struct {
        const char *name;
        unsigned int mask;
    } movementBooleanAttr[] = {
        { "swimable", MASK_SWIMABLE },
        { "sailable", MASK_SAILABLE },
        { "unflyable", MASK_UNFLYABLE },
        { "creatureunwalkable", MASK_CREATURE_UNWALKABLE }
    };
    static const char *speedEnumStrings[] = { "fast", "slow", "vslow", "vvslow", NULL };
    static const char *effectsEnumStrings[] = { "none", "fire", "sleep", "poison", "poisonField", "electricity", "lava", NULL };

    rule->mask = 0;
    rule->movementMask = 0;
    rule->speed = FAST;
    rule->effect = EFFECT_NONE;
    rule->walkonDirs = MASK_DIR_ALL;
    rule->walkoffDirs = MASK_DIR_ALL;
    rule->name = propertySymbol(sym, conf, "name");

    for (i = 0; i < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); i++) {
        if (conf.getBool(booleanAttributes[i].name))
            rule->mask |= booleanAttributes[i].mask;
    }

    for (i = 0; i < sizeof(movementBooleanAttr) / sizeof(movementBooleanAttr[0]); i++) {
        if (conf.getBool(movementBooleanAttr[i].name))
            rule->movementMask |= movementBooleanAttr[i].mask;
    }

    string cantwalkon = conf.getString("cantwalkon");
    if (cantwalkon == "all")
        rule->walkonDirs = 0;
    else if (cantwalkon == "west")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkonDirs);
    else if (cantwalkon == "north")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkonDirs);
    else if (cantwalkon == "east")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkonDirs);
    else if (cantwalkon == "south")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkonDirs);
    else if (cantwalkon == "advance")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkonDirs);
    else if (cantwalkon == "retreat")
        rule->walkonDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkonDirs);

    string cantwalkoff = conf.getString("cantwalkoff");
    if (cantwalkoff == "all")
        rule->walkoffDirs = 0;
    else if (cantwalkoff == "west")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_WEST, rule->walkoffDirs);
    else if (cantwalkoff == "north")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, rule->walkoffDirs);
    else if (cantwalkoff == "east")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_EAST, rule->walkoffDirs);
    else if (cantwalkoff == "south")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, rule->walkoffDirs);
    else if (cantwalkoff == "advance")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_ADVANCE, rule->walkoffDirs);
    else if (cantwalkoff == "retreat")
        rule->walkoffDirs = DIR_REMOVE_FROM_MASK(DIR_RETREAT, rule->walkoffDirs);

    rule->speed = static_cast<TileSpeed>(conf.getEnum("speed", speedEnumStrings));
    rule->effect = static_cast<TileEffect>(conf.getEnum("effect", effectsEnumStrings));
}

static void conf_tileLoad(const Config* cfg, Tile* tile, const ConfigElement &conf) {
    tile->name = conf.getString("name"); /* get the name of the tile */
    if (tile->name == "brick_floor")
        Tile::dungeonFloorId = tile->getId();

    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation"))
        tile->animationRule = conf.getString("animation");

    /* see if the tile is opaque */
    tile->opaque = conf.getBool("opaque");

    tile->foreground = conf.getBool("usesReplacementTileAsBackground");
    tile->waterForeground = conf.getBool("usesWaterReplacementTileAsBackground");

    /* Get the rule that applies to the current tile (or "default") */
    Symbol sym = SYM_UNSET;
    if (conf.exists("rule"))
        sym = cfg->propSymbol(conf, "rule");
    tile->rule = cfg->tileRule(sym);

    /* get the number of frames the tile has */
    tile->frames = conf.getInt("frames", 1);

    /* get the name of the image that belongs to this tile */
    if (conf.exists("image"))
        tile->imageName = conf.getString("image");
    else
        tile->imageName = string("tile_") + tile->name;

    tile->tiledInDungeon = conf.getBool("tiledInDungeon");

    /* Fill directions if they are specified. */
    if (conf.exists("directions"))
        tile->setDirections(conf.getString("directions"));
}

static void conf_tilesetLoad(Config* cfg, Tileset* ts, const ConfigElement& conf) {
    //ts->name = conf.getString("name");
    if (conf.exists("imageName"))
        ts->imageName = conf.getString("imageName");

    int index = 0;
    int moduleId = 0;
    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() != "tile")
            continue;

        Tile* tile = new Tile(moduleId++);
        conf_tileLoad(cfg, tile, *it);

        /* add the tile to our tileset */
        ts->tiles.push_back( tile );
        ts->nameMap[tile->getName()] = tile;

        index += tile->getFrames();
    }
    ts->totalFrames = index;
}

static void conf_ultimaSaveIds(UltimaSaveIds* usaveIds, Tileset* ts, const ConfigElement &conf) {
    int frames;
    int uid = 0;

    usaveIds->alloc(256, 135);

    vector<ConfigElement> children = conf.getChildren();
    vector<ConfigElement>::iterator it;
    foreach (it, children) {
        if (it->getName() != "mapping")
            continue;

        /* find the tile this references */
        string tile = it->getString("tile");
        const Tile *t = ts->getByName(tile);
        if (! t)
            errorFatal("Error: tile '%s' was not found in tileset", tile.c_str());

        if (it->exists("index"))
            uid = it->getInt("index");

        if (it->exists("frames"))
            frames = it->getInt("frames");
        else
            frames = 1;

        usaveIds->addId(uid, frames, t->getId());
        uid += frames;
    }

#if 0
    printf( "ultimaIdTable[%d]", usaveIds->miCount);
    for(int i = 0; i < usaveIds->miCount; ++i) {
        if ((i & 3) == 0) printf("\n");
        printf(" %d,", usaveIds->ultimaIdTable[i]);
    }
    printf( "\nmoduleIdTable[%d]", usaveIds->uiCount);
    for(int i = 0; i < usaveIds->uiCount; ++i) {
        if ((i & 3) == 0) printf("\n");
        printf(" %d,", usaveIds->moduleIdTable[i]);
    }
#endif
}

//--------------------------------------

static Armor*  conf_armor(int type, const ConfigElement&);
static Weapon* conf_weapon(int type, const ConfigElement&);

ConfigXML::ConfigXML() {
    backend = &xcd;

    xcd.tileset = NULL;
    memset(&xcd.usaveIds, 0, sizeof(xcd.usaveIds));

    xcd.sym.intern("unset!");   // Symbol 0 can be used as nil/unset/unknown.

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

    // egaPalette (load on demand)
    xcd.egaColors = NULL;

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

    {
    vector<ConfigElement> ce;
    vector<ConfigElement>::const_iterator it;

    // armors
    ce = getElement("armors").getChildren();
    foreach (it, ce) {
        if (it->getName() == "armor")
            xcd.armors.push_back(conf_armor(xcd.armors.size(), *it));
    }

    // weapons
    ce = getElement("weapons").getChildren();
    foreach (it, ce) {
        if (it->getName() == "weapon")
            xcd.weapons.push_back(conf_weapon(xcd.weapons.size(), *it));
    }

    // tileRules
    {
    TileRule* rule;
    Symbol symDefault = xcd.sym.intern("default");

    ce = getElement("tileRules").getChildren();
    xcd.tileRuleCount = ce.size();
    xcd.tileRuleDefault = -1;
    rule = xcd.tileRules = new TileRule[ xcd.tileRuleCount ];
    foreach (it, ce) {
        conf_tileRule(xcd.sym, rule, *it);
        if (rule->name == symDefault )
            xcd.tileRuleDefault = rule - xcd.tileRules;
        ++rule;
    }
    if (xcd.tileRuleDefault < 0)
        errorFatal("no 'default' rule found in tile rules");
    }

    // tileset (requires tileRules)
    ce = getElement("tilesets").getChildren();
    foreach (it, ce) {
        if (it->getName() == "tileset") {
            xcd.tileset = new Tileset;
            conf_tilesetLoad(this, xcd.tileset, *it);
            break;      // Only support one tileset.
        }
    }

    // usaveIds (requires tileset)
    if (xcd.tileset) {
        foreach (it, ce) {
            if (it->getName() == "tilemap") {
                conf_ultimaSaveIds(&xcd.usaveIds, xcd.tileset, *it);
                break;
            }
        }
    }
    }
}

ConfigXML::~ConfigXML() {
    xmlFreeDoc(xcd.doc);

    delete[] xcd.egaColors;

    vector<Armor *>::iterator ait;
    foreach (ait, xcd.armors)
        delete *ait;

    vector<Weapon *>::iterator wit;
    foreach (wit, xcd.weapons)
        delete *wit;

    delete[] xcd.tileRules;
    delete xcd.tileset;
    xcd.usaveIds.free();
}

//--------------------------------------

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

//--------------------------------------

const char* Config::symbolName( Symbol s ) const {
    return CB->sym.name(s);
}

/*
 * Return pointer to 16 RGBA values.
 */
const RGBA* Config::egaPalette() {
    if (! CB->egaColors) {
        RGBA* col = CB->egaColors = new RGBA[16];
        RGBA* end = col + 16;

        vector<ConfigElement> ce = getElement("egaPalette").getChildren();
        vector<ConfigElement>::iterator it;
        foreach (it, ce) {
            if (it->getName() != "color")
                continue;
            col->r = it->getInt("red");
            col->g = it->getInt("green");
            col->b = it->getInt("blue");
            col->a = 255;
            if (++col == end)
                break;
        }
    }
    return CB->egaColors;
}

/*
 * Return a filename pointer for the given MusicTrack id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::musicFile( uint32_t id ) {
    if (id < CB->musicFiles.size()) {
        CB->sbuf = u4find_music(CB->musicFiles[id]);
        return CB->sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a filename pointer for the given Sound id (see sound.h)
 * The value is a C string as that is what low-level audio APIs require.
 * The pointer is valid until the next musicFile or soundFile call.
 */
const char* Config::soundFile( uint32_t id ) {
    if (id < CB->soundFiles.size()) {
        CB->sbuf = u4find_sound(CB->soundFiles[id]);
        return CB->sbuf.c_str();
    }
    return NULL;
}

/*
 * Return a C string pointer array for the available Image scheme names.
 */
const char** Config::schemeNames() {
    vector<const char*>& sarray = CB->sarray;
    sarray.clear();
    vector<string>::iterator it;
    for (it = CB->schemeNames.begin(); it != CB->schemeNames.end(); ++it)
        sarray.push_back( (*it).c_str() );
    sarray.push_back(NULL);
    return &sarray.front();
}

/*
 * Return an Armor pointer for the given ArmorType id (see savegame.h)
 */
const Armor* Config::armor( uint32_t id ) {
    if (id < CB->armors.size())
        return CB->armors[id];
    return NULL;
}

/*
 * Return a Weapon pointer for the given WeaponType id (see savegame.h)
 */
const Weapon* Config::weapon( uint32_t id ) {
    if (id < CB->weapons.size())
        return CB->weapons[id];
    return NULL;
}

int Config::armorType( const char* name ) {
    vector<Armor*>::const_iterator it;
    foreach (it, CB->armors) {
        if ((*it)->name == name)
            return it - CB->armors.begin();
    }
    return -1;
}

int Config::weaponType( const char* name ) {
    vector<Weapon*>::const_iterator it;
    foreach (it, CB->weapons) {
        if ((*it)->name == name)
            return it - CB->weapons.begin();
    }
    return -1;
}

/*
 * Get rule by name.  If there is no such named rule, then the "default" rule
 * is returned.
 */
const TileRule* Config::tileRule( Symbol name ) const {
    const TileRule* it = CB->tileRules;
    const TileRule* end = it + CB->tileRuleCount;
    for (; it != end; ++it) {
        if (it->name == name)
            return it;
    }
    return CB->tileRules + CB->tileRuleDefault;
}

const Tileset* Config::tileset() const {
    return CB->tileset;

}

const UltimaSaveIds* Config::usaveIds() const {
    return &CB->usaveIds;
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
    info->resGroup = 0;
    info->width = conf.getInt("width", -1);
    info->height = conf.getInt("height", -1);
    info->depth = conf.getInt("depth", -1);
    info->prescale = conf.getInt("prescale");
    info->filetype = mapFiletype(conf.getString("filetype"), info->filename);
    info->tiles = conf.getInt("tiles");
    info->transparentIndex = conf.getInt("transparentIndex", -1);
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
// Items (weapons & armor)

static Armor* conf_armor(int type, const ConfigElement& conf) {
    Armor* arm = new Armor;

    arm->type    = (ArmorType) type;
    arm->name    = conf.getString("name");
    arm->canuse  = 0xFF;
    arm->defense = conf.getInt("defense");
    arm->mask    = 0;

    vector<ConfigElement> contraintConfs = conf.getChildren();
    std::vector<ConfigElement>::iterator it;
    foreach (it, contraintConfs) {
        if (it->getName() != "constraint")
            continue;

        unsigned char mask = 0;
        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(it->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(it->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed armor.xml file: constraint has unknown class %s",
                       it->getString("class").c_str());
        }
        if (it->getBool("canuse"))
            arm->canuse |= mask;
        else
            arm->canuse &= ~mask;
    }
    return arm;
}

static Weapon* conf_weapon(int type, const ConfigElement& conf) {
    static const struct {
        const char *name;
        unsigned int flag;
    } booleanAttributes[] = {
        { "lose", WEAP_LOSE },
        { "losewhenranged", WEAP_LOSEWHENRANGED },
        { "choosedistance", WEAP_CHOOSEDISTANCE },
        { "alwayshits", WEAP_ALWAYSHITS },
        { "magic", WEAP_MAGIC },
        { "attackthroughobjects", WEAP_ATTACKTHROUGHOBJECTS },
        { "returns", WEAP_RETURNS },
        { "dontshowtravel", WEAP_DONTSHOWTRAVEL }
    };
    Weapon* wpn = new Weapon;

    wpn->type   = (WeaponType) type;
    wpn->name   = conf.getString("name");
    wpn->abbr   = conf.getString("abbr");
    wpn->canuse = 0xFF;
    wpn->range  = 0;
    wpn->damage = conf.getInt("damage");
    wpn->hittile   = "hit_flash";
    wpn->misstile  = "miss_flash";
    //wpn->leavetile = "";
    wpn->flags = 0;


    /* Get the range of the weapon, whether it is absolute or normal range */
    string _range = conf.getString("range");
    if (_range.empty()) {
        _range = conf.getString("absolute_range");
        if (!_range.empty())
            wpn->flags |= WEAP_ABSOLUTERANGE;
    }
    if (_range.empty())
        errorFatal("malformed weapons.xml file: range or absolute_range not found for weapon %s", wpn->name.c_str());

    wpn->range = atoi(_range.c_str());

    /* Load weapon attributes */
    for (unsigned at = 0; at < sizeof(booleanAttributes) / sizeof(booleanAttributes[0]); at++) {
        if (conf.getBool(booleanAttributes[at].name))
            wpn->flags |= booleanAttributes[at].flag;
    }

    /* Load hit tiles */
    if (conf.exists("hittile"))
        wpn->hittile = conf.getString("hittile");

    /* Load miss tiles */
    if (conf.exists("misstile"))
        wpn->misstile = conf.getString("misstile");

    /* Load leave tiles */
    if (conf.exists("leavetile"))
        wpn->leavetile = conf.getString("leavetile");

    vector<ConfigElement> contraintConfs = conf.getChildren();
    for (std::vector<ConfigElement>::iterator i = contraintConfs.begin(); i != contraintConfs.end(); i++) {
        unsigned char mask = 0;

        if (i->getName() != "constraint")
            continue;

        for (int cl = 0; cl < 8; cl++) {
            if (strcasecmp(i->getString("class").c_str(), getClassName(static_cast<ClassType>(cl))) == 0)
                mask = (1 << cl);
        }
        if (mask == 0 && strcasecmp(i->getString("class").c_str(), "all") == 0)
            mask = 0xFF;
        if (mask == 0) {
            errorFatal("malformed weapons.xml file: constraint has unknown class %s",
                       i->getString("class").c_str());
        }
        if (i->getBool("canuse"))
            wpn->canuse |= mask;
        else
            wpn->canuse &= ~mask;
    }
    return wpn;
}

//--------------------------------------
// Config Service API

// Create configService.
Config* configInit() {
    // Here's where we can compile the program with alternate back-ends
    // (e.g. SQL, JSON, ... or something better.)

    return new ConfigXML;
}

void configFree(Config* conf) {
    delete conf;
}
