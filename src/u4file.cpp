/*
 * $Id$
 */

#include <cctype>
#include <cstring>
#include <cstdlib>

#include "u4file.h"
#include "unzip.h"
#include "debug.h"
#ifdef MACOSX
#include <libgen.h>
#elif defined(IOS)
#include "ios_helpers.h"
#endif

using std::map;
using std::string;
using std::vector;

/**
 * A specialization of U4FILE that uses C stdio internally.
 */
class U4FILE_stdio : public U4FILE {
public:
    static U4FILE *open(const char* fname);

    virtual void close();
    virtual int seek(long offset, int whence);
    virtual long tell();
    virtual size_t read(void *ptr, size_t size, size_t nmemb);
    virtual int getc();
    virtual int putc(int c);
    virtual long length();

private:
    FILE *file;
};

/**
 * A specialization of U4FILE that reads files out of zip archives
 * automatically.
 */
class U4FILE_zip : public U4FILE {
public:
    static U4FILE *open(const string &fname, const U4ZipPackage *package);

    virtual void close();
    virtual int seek(long offset, int whence);
    virtual long tell();
    virtual size_t read(void *ptr, size_t size, size_t nmemb);
    virtual int getc();
    virtual int putc(int c);
    virtual long length();

private:
    unzFile zfile;
};

/**
 * Keeps track of available zip packages.
 */
struct U4ZipPackageMgr {
    U4ZipPackageMgr();
    ~U4ZipPackageMgr();

    void add(U4ZipPackage *package) {
        packages.push_back(package);
    }

    std::vector<U4ZipPackage *> packages;
};

extern bool verbose;

enum UpgradeFlags {
    UPG_PRESENT = 1,
    UPG_INST    = 2
};
static int upgradeFlags = 0;

static U4PATH * u4path_instance = NULL;
static U4ZipPackageMgr * u4zip_instance = NULL;

U4PATH * U4PATH::getInstance() {
    return u4path_instance;
}

/**
 * Program initialization for u4 file functions.
 * Returns true if the required data files are present.
 */
bool u4fsetup()
{
    if (u4path_instance)
        return true;
    u4path_instance = new U4PATH();
    u4path_instance->initDefaultPaths();

    u4zip_instance = new U4ZipPackageMgr();

    U4FILE* uf;
    if ((uf = u4fopen("AVATAR.EXE")))
        u4fclose(uf);
    else
        return false;

    // Check if upgrade is present & installed.
    upgradeFlags = 0;
    if ((uf = u4fopen("u4vga.pal"))) {
        upgradeFlags |= UPG_PRESENT;
        u4fclose(uf);
    }
    /* See if (ega.drv > 5k).  If so, the upgrade is installed */
    /* FIXME: Is there a better way to determine this? */
    if ((uf = u4fopen("ega.drv"))) {
        if (uf->length() > (5 * 1024))
            upgradeFlags |= UPG_INST;
        u4fclose(uf);
    }
    return true;
}

void u4fcleanup()
{
    delete u4path_instance;
    u4path_instance = NULL;

    delete u4zip_instance;
    u4zip_instance = NULL;
}

void U4PATH::initDefaultPaths() {
    if (defaultsHaveBeenInitd)
        return;

    //The first part of the path searched will be one of these root directories

    /*Try to cover all root possibilities. These can be added to by separate modules*/
    rootResourcePaths.push_back(".");
    rootResourcePaths.push_back("./ultima4");
#ifdef _WIN32
    rootResourcePaths.push_back("C:");
    rootResourcePaths.push_back("C:/DOS");
    rootResourcePaths.push_back("C:/GAMES");
#else
    rootResourcePaths.push_back("/usr/lib/u4");
    rootResourcePaths.push_back("/usr/local/lib/u4");
#endif

    //The second (specific) part of the path searched will be these various subdirectories

    /* the possible paths where u4 for DOS can be installed */
    u4ForDOSPaths.push_back(".");
    u4ForDOSPaths.push_back("u4");
    u4ForDOSPaths.push_back("ultima4");

    /* the possible paths where the u4 zipfiles can be installed */
    u4ZipPaths.push_back(".");
    u4ZipPaths.push_back("u4");

#ifndef CONF_MODULE
    /* the possible paths where the u4 music files can be installed */
    musicPaths.push_back(".");
    musicPaths.push_back("mid");
    musicPaths.push_back("../mid");
    musicPaths.push_back("music");
    musicPaths.push_back("../music");

    /* the possible paths where the u4 sound files can be installed */
    soundPaths.push_back(".");
    soundPaths.push_back("./sound");
    soundPaths.push_back("../sound");

    /* the possible paths where the u4 config files can be installed */
    configPaths.push_back(".");
    configPaths.push_back("conf");
    configPaths.push_back("../conf");

    /* the possible paths where the u4 graphics files can be installed */
    graphicsPaths.push_back(".");
    graphicsPaths.push_back("graphics");
    graphicsPaths.push_back("../graphics");
#endif
}

/**
 * Returns true if the upgrade is present.
 */
bool u4isUpgradeAvailable() {
    return upgradeFlags & UPG_PRESENT;
}

/**
 * Returns true if the upgrade is not only present, but is installed
 * (switch.bat or setup.bat has been run)
 */
bool u4isUpgradeInstalled() {
    if (verbose)
        printf("u4isUpgradeInstalled %d\n", (upgradeFlags & UPG_INST) ? 1 : 0);
    return upgradeFlags & UPG_INST;
}

/**
 * Creates a new zip package.
 */
U4ZipPackage::U4ZipPackage(const string &name, const string &path, bool extension) {
    this->name = name;
    this->path = path;
    this->extension = extension;
}

void U4ZipPackage::addTranslation(const string &value, const string &translation) {
    translations[value] = translation;
}

const string &U4ZipPackage::translate(const string &name) const {
    std::map<string, string>::const_iterator i = translations.find(name);
    if (i != translations.end())
        return i->second;
    else
        return name;
}

U4ZipPackageMgr::U4ZipPackageMgr() {
    unzFile f;

    string upg_pathname(u4find_path("u4upgrad.zip", u4Path.u4ZipPaths));
    if (!upg_pathname.empty()) {
        /* upgrade zip is present */
        U4ZipPackage *upgrade = new U4ZipPackage(upg_pathname, "", false);
        upgrade->addTranslation("compassn.ega", "compassn.old");
        upgrade->addTranslation("courage.ega", "courage.old");
        upgrade->addTranslation("cove.tlk", "cove.old");
        upgrade->addTranslation("ega.drv", "ega.old"); // not actually used
        upgrade->addTranslation("honesty.ega", "honesty.old");
        upgrade->addTranslation("honor.ega", "honor.old");
        upgrade->addTranslation("humility.ega", "humility.old");
        upgrade->addTranslation("key7.ega", "key7.old");
        upgrade->addTranslation("lcb.tlk", "lcb.old");
        upgrade->addTranslation("love.ega", "love.old");
        upgrade->addTranslation("love.ega", "love.old");
        upgrade->addTranslation("minoc.tlk", "minoc.old");
        upgrade->addTranslation("rune_0.ega", "rune_0.old");
        upgrade->addTranslation("rune_1.ega", "rune_1.old");
        upgrade->addTranslation("rune_2.ega", "rune_2.old");
        upgrade->addTranslation("rune_3.ega", "rune_3.old");
        upgrade->addTranslation("rune_4.ega", "rune_4.old");
        upgrade->addTranslation("rune_5.ega", "rune_5.old");
        upgrade->addTranslation("sacrific.ega", "sacrific.old");
        upgrade->addTranslation("skara.tlk", "skara.old");
        upgrade->addTranslation("spirit.ega", "spirit.old");
        upgrade->addTranslation("start.ega", "start.old");
        upgrade->addTranslation("stoncrcl.ega", "stoncrcl.old");
        upgrade->addTranslation("truth.ega", "truth.old");
        upgrade->addTranslation("ultima.com", "ultima.old"); // not actually used
        upgrade->addTranslation("valor.ega", "valor.old");
        upgrade->addTranslation("yew.tlk", "yew.old");
        add(upgrade);
    }

    // Check for the default zip packages
    int flag = 0;
    string pathname;

    do {
        //Check for the upgraded package once. unlikely it'll be renamed.
        pathname = u4find_path("ultima4-1.01.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        // We check for all manner of generic packages, though.
        pathname = u4find_path("ultima4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("Ultima4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("ULTIMA4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("u4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("U4.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        //search for the ultimaforever.com zip and variations
        pathname = u4find_path("UltimaIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("Ultimaiv.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("ULTIMAIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("ultimaIV.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        pathname = u4find_path("ultimaiv.zip", u4Path.u4ZipPaths);
        if (!pathname.empty()) {
            flag = 1;
            break;
        }

        // If it's not found by this point, give up.
        break;

    } while (flag == 0);

    if (flag) {
        f = unzOpen(pathname.c_str());
        if (!f)
            return;

        //Now we detect the folder structure inside the zipfile.
        if (unzLocateFile(f, "charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "", false));

        } else if (unzLocateFile(f, "ultima4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "ultima4/", false));

        } else if (unzLocateFile(f, "Ultima4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "Ultima4/", false));

        } else if (unzLocateFile(f, "ULTIMA4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "ULTIMA4/", false));

        } else if (unzLocateFile(f, "u4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "u4/", false));

        } else if (unzLocateFile(f, "U4/charset.ega", 2) == UNZ_OK) {
            add(new U4ZipPackage(pathname, "U4/", false));

        }

        unzClose(f);

    }

    /* scan for extensions */
}

U4ZipPackageMgr::~U4ZipPackageMgr() {
    for (std::vector<U4ZipPackage *>::iterator i = packages.begin(); i != packages.end(); i++)
        delete *i;
}

int U4FILE::getshort() {
    int byteLow = getc();
    return byteLow | (getc() << 8);
}

U4FILE *U4FILE_stdio::open(const char* fname) {
    FILE *f = fopen(fname, "rb");
    if (!f)
        return NULL;

    U4FILE_stdio *u4f = new U4FILE_stdio;
    u4f->file = f;
    return u4f;
}

void U4FILE_stdio::close() {
    fclose(file);
}

int U4FILE_stdio::seek(long offset, int whence) {
    return fseek(file, offset, whence);
}

long U4FILE_stdio::tell() {
    return ftell(file);
}

size_t U4FILE_stdio::read(void *ptr, size_t size, size_t nmemb) {
    return fread(ptr, size, nmemb, file);
}

int U4FILE_stdio::getc() {
    return fgetc(file);
}

int U4FILE_stdio::putc(int c) {
    return fputc(c, file);
}

long U4FILE_stdio::length() {
    long curr, len;

    curr = ftell(file);
    fseek(file, 0L, SEEK_END);
    len = ftell(file);
    fseek(file, curr, SEEK_SET);

    return len;
}

/**
 * Opens a file from within a zip archive.
 */
U4FILE *U4FILE_zip::open(const string &fname, const U4ZipPackage *package) {
    U4FILE_zip *u4f;
    unzFile f;

    f = unzOpen(package->getFilename().c_str());
    if (!f)
        return NULL;

    string pathname = package->getInternalPath() + package->translate(fname);

    if (unzLocateFile(f, pathname.c_str(), 2) == UNZ_END_OF_LIST_OF_FILE) {
        unzClose(f);
        return NULL;
    }
    unzOpenCurrentFile(f);

    u4f = new U4FILE_zip;
    u4f->zfile = f;

    return u4f;
}

void U4FILE_zip::close() {
    unzClose(zfile);
}

int U4FILE_zip::seek(long offset, int whence) {
    char *buf;
    long pos;

    ASSERT(whence != SEEK_END, "seeking with whence == SEEK_END not allowed with zipfiles");
    pos = unztell(zfile);
    if (whence == SEEK_CUR)
        offset = pos + offset;
    if (offset == pos)
        return 0;
    if (offset < pos) {
        unzCloseCurrentFile(zfile);
        unzOpenCurrentFile(zfile);
        pos = 0;
    }
    ASSERT(offset - pos > 0, "error in U4FILE_zip::seek");
    buf = new char[offset - pos];
    unzReadCurrentFile(zfile, buf, offset - pos);
    delete [] buf;
    return 0;
}

long U4FILE_zip::tell() {
    return unztell(zfile);
}

size_t U4FILE_zip::read(void *ptr, size_t size, size_t nmemb) {
    size_t retval = unzReadCurrentFile(zfile, ptr, size * nmemb);
    if (retval > 0)
        retval = retval / size;

    return retval;
}

int U4FILE_zip::getc() {
    int retval;
    unsigned char c;

    if (unzReadCurrentFile(zfile, &c, 1) > 0)
        retval = c;
    else
        retval = EOF;

    return retval;
}

int U4FILE_zip::putc(int c) {
    ASSERT(0, "zipfiles must be read-only!");
    return c;
}

long U4FILE_zip::length() {
    unz_file_info fileinfo;

    unzGetCurrentFileInfo(zfile, &fileinfo,
                          NULL, 0,
                          NULL, 0,
                          NULL, 0);
    return fileinfo.uncompressed_size;
}

/**
 * Open a data file from the Ultima 4 for DOS installation.  This
 * function checks the various places where it can be installed, and
 * maps the filenames to uppercase if necessary.  The files are always
 * opened for reading only.
 *
 * First, it looks in the zipfiles.  Next, it tries FILENAME, Filename
 * and filename in up to four paths, meaning up to twelve or more
 * opens per file.  Seems to be ok for performance, but could be
 * getting excessive.  The presence of the zipfiles should probably be
 * cached.
 */
U4FILE *u4fopen(const string &fname) {
    U4FILE *u4f = NULL;
    unsigned int i;

    if (verbose)
        printf("looking for %s\n", fname.c_str());

    /**
     * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
     */
    const vector<U4ZipPackage *> &packages = u4zip_instance->packages;
    for (std::vector<U4ZipPackage *>::const_reverse_iterator j = packages.rbegin(); j != packages.rend(); j++) {
        u4f = U4FILE_zip::open(fname, *j);
        if (u4f) {
            if (verbose) {
                printf("%s found in %s\n", fname.c_str(),
                       (*j)->getFilename().c_str());
            }
            return u4f; /* file was found, return it! */
        }
    }

    /*
     * file not in a zipfile; check if it has been unzipped
     */
    string fname_copy(fname);

    string pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
    if (pathname.empty()) {
        using namespace std;
        if (islower(fname_copy[0])) {
            fname_copy[0] = toupper(fname_copy[0]);
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }

        if (pathname.empty()) {
            for (i = 0; fname_copy[i] != '\0'; i++) {
                if (islower(fname_copy[i]))
                    fname_copy[i] = toupper(fname_copy[i]);
            }
            pathname = u4find_path(fname_copy, u4Path.u4ForDOSPaths);
        }
    }

    if (!pathname.empty()) {
        u4f = U4FILE_stdio::open(pathname.c_str());
        if (verbose && u4f != NULL)
            printf("%s successfully opened\n", pathname.c_str());
    }

    return u4f;
}

/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE.
 */
U4FILE *u4fopen_stdio(const char* fname) {
    return U4FILE_stdio::open(fname);
}

/**
 * Opens a file from a zipfile and wraps it in a U4FILE.
 */
U4FILE *u4fopen_zip(const string &fname, U4ZipPackage *package) {
    return U4FILE_zip::open(fname, package);
}

/**
 * Closes a data file from the Ultima 4 for DOS installation.
 */
void u4fclose(U4FILE *f) {
    f->close();
    delete f;
}

int u4fseek(U4FILE *f, long offset, int whence) {
    return f->seek(offset, whence);
}

long u4ftell(U4FILE *f) {
    return f->tell();
}

size_t u4fread(void *ptr, size_t size, size_t nmemb, U4FILE *f) {
    return f->read(ptr, size, nmemb);
}

int u4fgetc(U4FILE *f) {
    return f->getc();
}

int u4fgetshort(U4FILE *f) {
    return f->getshort();
}

int u4fputc(int c, U4FILE *f) {
    return f->putc(c);
}

/**
 * Returns the length in bytes of a file.
 */
long u4flength(U4FILE *f) {
    return f->length();
}

/**
 * Read a series of zero terminated strings from a file.  The strings
 * are read from the given offset, or the current file position if
 * offset is -1.
 */
vector<string> u4read_stringtable(U4FILE *f, long offset, int nstrings) {
    string buffer;
    int i;
    vector<string> strs;

    ASSERT(offset < u4flength(f), "offset begins beyond end of file");

    if (offset != -1)
        f->seek(offset, SEEK_SET);
    for (i = 0; i < nstrings; i++) {
        char c;
        buffer.erase();

        while ((c = f->getc()) != '\0')
            buffer += c;

        strs.push_back(buffer);
    }

    return strs;
}

string u4find_path(const string &fname, std::list<string> specificSubPaths) {
    FILE *f = NULL;

    // Try absolute first
    char path[2048]; // Sometimes paths get big.

    f = fopen(fname.c_str(), "rb");
    if (f)
        strcpy(path, fname.c_str());

    // Try 'file://' protocol if specified
    if (f == NULL) {
        const string file_url_prefix("file://");

        if (fname.compare(0, file_url_prefix.length(), file_url_prefix) == 0) {
            strcpy(path, fname.substr(file_url_prefix.length()).c_str());
            if (verbose) {
                printf("trying to open %s\n", path);
            }
            f = fopen(path, "rb");
        }
    }

    // Try paths
    if (f == NULL) {
        for (std::list<string>::iterator rootItr = u4Path.rootResourcePaths.begin();
                rootItr!=u4Path.rootResourcePaths.end() && !f;
                ++rootItr) {
            for (std::list<string>::iterator subItr = specificSubPaths.begin();
                    subItr!=specificSubPaths.end() && !f;
                    ++subItr) {

                snprintf(path, sizeof(path), "%s/%s/%s", rootItr->c_str(), subItr->c_str(), fname.c_str());

                if (verbose) {
                    printf("trying to open %s\n", path);
                }
                if ((f = fopen(path, "rb")) != NULL)
                    break;
            }
        }
    }
#if defined(IOS)
    if (f == NULL) {
        string base = fname;
        string ext = "";
        string dir = "";
        // This is VERY dependant on the current layout of the XML files. It will fail in a general case.
        size_t seppos = fname.rfind('/');
        if (seppos != string::npos)
            dir = fname.substr(0, seppos);
        size_t pos = fname.rfind('.');
        if (pos != string::npos) {
            if (seppos != string::npos)
                base = fname.substr(seppos + 1, pos - seppos - 1);
            else
                base = fname.substr(0, pos);
            ext = fname.substr(pos + 1);
        }

        string pathFile = U4IOS::getFileLocation(dir, base, ext);
        strncpy(path, pathFile.c_str(), 2048);
        if (verbose)
            printf("trying to open %s\n", path);

        f = fopen(path, "rb");
    }
#endif

    if (verbose) {
        if (f != NULL)
            printf("%s successfully found\n", path);
        else
            printf("%s not found\n", fname.c_str());
    }

    if (f) {
        fclose(f);
        return path;
    } else
        return "";
}

#ifndef CONF_MODULE
string u4find_music(const string &fname) {
    return u4find_path(fname, u4Path.musicPaths);
}

string u4find_sound(const string &fname) {
    return u4find_path(fname, u4Path.soundPaths);
}

string u4find_conf(const string &fname) {
    return u4find_path(fname, u4Path.configPaths);
}

string u4find_graphics(const string &fname) {
    return u4find_path(fname, u4Path.graphicsPaths);
}
#endif
