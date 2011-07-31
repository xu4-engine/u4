/*
 * $Id$
 */

#ifndef U4FILE_H
#define U4FILE_H

#include "vc6.h"
#include <map>
#include <string>
#include <vector>
#include <list>


/**
 * Represents zip files that game resources can be loaded from.
 */
class U4ZipPackage {
public:
    typedef std::string string;

    U4ZipPackage(const string &name, const string &path, bool extension);
    void addTranslation(const string &value, const string &translation);

    const string &getFilename() const { return name; }
    const string &getInternalPath() const { return path; }
    bool isExtension() const { return extension; }
    const string &translate(const string &name) const;

private:    
    string name;                /**< filename */
    string path;                /**< the path within the zipfile where resources are located */
    bool extension;             /**< whether this zipfile is an extension with config information */
    std::map<string, string> translations; /**< mapping from standard resource names to internal names */
};

/**
 * Keeps track of available zip packages.
 */
class U4ZipPackageMgr {
public:
    static U4ZipPackageMgr *getInstance();
    static void destroy();
    
    void add(U4ZipPackage *package);
    const std::vector<U4ZipPackage *> &getPackages() const { return packages; }

private:
    U4ZipPackageMgr();
    ~U4ZipPackageMgr();

    static U4ZipPackageMgr *instance;
    std::vector<U4ZipPackage *> packages;
};

#ifdef putc
#undef putc
#endif

/**
 * An abstract interface for file access.
 */
class U4FILE {
public:
    virtual ~U4FILE() {}

    virtual void close() = 0;
    virtual int seek(long offset, int whence) = 0;
    virtual long tell() = 0;
    virtual size_t read(void *ptr, size_t size, size_t nmemb) = 0;
    virtual int getc() = 0;
    virtual int putc(int c) = 0;
    virtual long length() = 0;

    int getshort();
};

/** A replacement class to manage path searching. Very open-concept */
#define u4Path (*U4PATH::getInstance())
class U4PATH {
public:
	U4PATH() : defaultsHaveBeenInitd(false){}
    void initDefaultPaths();

    static U4PATH * instance;
    static U4PATH * getInstance();


	std::list<std::string> rootResourcePaths;

    std::list<std::string> u4ForDOSPaths;
    std::list<std::string> u4ZipPaths;
    std::list<std::string> musicPaths;
    std::list<std::string> soundPaths;
    std::list<std::string> configPaths;
    std::list<std::string> graphicsPaths;

private:
    bool defaultsHaveBeenInitd;

};

bool u4isUpgradeAvailable();
bool u4isUpgradeInstalled();
U4FILE *u4fopen(const std::string &fname);
U4FILE *u4fopen_stdio(const std::string &fname);
U4FILE *u4fopen_zip(const std::string &fname, U4ZipPackage *package);
void u4fclose(U4FILE *f);
int u4fseek(U4FILE *f, long offset, int whence);
long u4ftell(U4FILE *f);
size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f);
int u4fgetc(U4FILE *f);
int u4fgetshort(U4FILE *f);
int u4fputc(int c, U4FILE *f);
long u4flength(U4FILE *f);
std::vector<std::string> u4read_stringtable(U4FILE *f, long offset, int nstrings);

std::string u4find_path(const std::string &fname, std::list<std::string> specificSubPaths);
std::string u4find_music(const std::string &fname);
std::string u4find_sound(const std::string &fname);
std::string u4find_conf(const std::string &fname);
std::string u4find_graphics(const std::string &fname);

#endif
