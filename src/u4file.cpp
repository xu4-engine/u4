/*
 * u4file.cpp
 */

#include <cctype>
#include <cstring>
#include <cstdlib>
#include <map>

#include "u4file.h"
#include "unzip.h"
#include "debug.h"
#include "xu4.h"

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
 * Represents zip files that game resources can be loaded from.
 */
class U4ZipPackage {
public:
    U4ZipPackage(const string &name, const string &path);
    void addTranslation(const string &value, const string &translation);

    const string &getFilename() const { return name; }
    const string &getInternalPath() const { return path; }
    const string &translate(const string &name) const;

private:
    string name;                /**< filename */
    string path;                /**< the path within the zipfile where resources are located */
    std::map<string, string> translations; /**< mapping from standard resource names to internal names */
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

enum UpgradeFlags {
    UPG_PRESENT = 1,
    UPG_INST    = 2
};
static int upgradeFlags = 0;

static U4PATH * u4path_instance = NULL;
static U4ZipPackage * u4zip_orig = NULL;
static U4ZipPackage * u4zip_upgrad = NULL;

U4PATH * U4PATH::getInstance() {
    return u4path_instance;
}

static void u4zip_setup();

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

    u4zip_setup();

    U4FILE* uf;
    if ((uf = u4fopen("AVATAR.EXE"))) {
        u4fclose(uf);
    } else {
        u4fcleanup();
        return false;
    }

    // Check if upgrade is present & installed.
    upgradeFlags = 0;
    if ((uf = u4fopen_upgrade("u4vga.pal"))) {
        upgradeFlags |= UPG_PRESENT;
        u4fclose(uf);
    }
    /* See if (ega.drv > 5k).  If so, the upgrade is installed */
    /* FIXME: Is there a better way to determine this? */
    if ((uf = u4fopen_upgrade("ega.drv"))) {
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

    delete u4zip_orig;
    delete u4zip_upgrad;
    u4zip_orig = u4zip_upgrad = NULL;
}

void U4PATH::initDefaultPaths() {
    if (defaultsHaveBeenInitd)
        return;

    // The first part of the path searched will be one of the xu4.resourcePaths.
    // The second (specific) part of the path searched will be these various
    // subdirectories.

    /* the possible paths where u4 for DOS can be installed */
    u4ForDOSPaths.push_back(".");
    u4ForDOSPaths.push_back("u4");
    u4ForDOSPaths.push_back("ultima4");

    /* the possible paths where the u4 zipfiles can be installed */
    u4ZipPaths.push_back(".");
    u4ZipPaths.push_back("u4");
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
    if (xu4.verbose)
        printf("u4isUpgradeInstalled %d\n", (upgradeFlags & UPG_INST) ? 1 : 0);
    return upgradeFlags & UPG_INST;
}

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

/**
 * Returns true if the file exists and is readable by the user.
 */
static bool u4fexists(const char* fname) {
#ifdef _WIN32
    WIN32_FIND_DATA data;
    HANDLE fh;
    fh = FindFirstFile(fname, &data);
    if (fh == INVALID_HANDLE_VALUE)
        return false;
    FindClose(fh);
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
    return true;
#else
    struct stat buf;
    if (stat(fname, &buf) == -1)
        return false;
    if (S_ISDIR(buf.st_mode))
        return false;
    return (buf.st_mode & S_IRUSR) ? true : false;
#endif
}

/**
 * Creates a new zip package.
 */
U4ZipPackage::U4ZipPackage(const string &name, const string &path) {
    this->name = name;
    this->path = path;
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

static const char* u4ZipFilenames[] = {
    // Check for the upgraded package which is unlikely to be renamed.
    "ultima4-1.01.zip",
    // We check for all manner of generic packages, though.
    "ultima4.zip",
#ifndef _WIN32
    "Ultima4.zip",
    "ULTIMA4.zip",
#endif
    "u4.zip",
    "U4.zip",
    // Search for the ultimaforever.com zip and variations
    "UltimaIV.zip",
#ifndef _WIN32
    "ultimaiv.zip",
    "ULTIMAIV.zip",
    "ultimaIV.zip",
    "Ultimaiv.zip",
#endif
    NULL
};

static void u4zip_setup() {
    u4zip_orig = u4zip_upgrad = NULL;

    string upg_pathname(u4find_path("u4upgrad.zip", &u4Path.u4ZipPaths));
    if (!upg_pathname.empty()) {
        /* upgrade zip is present */
        U4ZipPackage* upgrade = new U4ZipPackage(upg_pathname, "");
        u4zip_upgrad = upgrade;
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
    }

    // Check for the default zip packages
    string pathname;
    const char** zipFile;
    for (zipFile = u4ZipFilenames; *zipFile; ++zipFile) {
        pathname = u4find_path(*zipFile, &u4Path.u4ZipPaths);
        if (! pathname.empty())
            break;
    }
    if (*zipFile) {
        unzFile f = unzOpen(pathname.c_str());
        if (!f)
            return;

        //Now we detect the folder structure inside the zipfile.
        if (unzLocateFile(f, "charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "");
        else if (unzLocateFile(f, "ultima4/charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "ultima4/");
        else if (unzLocateFile(f, "Ultima4/charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "Ultima4/");
        else if (unzLocateFile(f, "ULTIMA4/charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "ULTIMA4/");
        else if (unzLocateFile(f, "u4/charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "u4/");
        else if (unzLocateFile(f, "U4/charset.ega", 2) == UNZ_OK)
            u4zip_orig = new U4ZipPackage(pathname, "U4/");
        unzClose(f);
    }
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
    uint8_t c;

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
 * getting excessive.
 */
static U4FILE *u4fopen_zip(const string &fname, U4ZipPackage* zipPkg) {
    U4FILE *u4f = NULL;
    unsigned int i;

    if (xu4.verbose)
        printf("looking for %s\n", fname.c_str());

    /**
     * search for file within zipfiles (ultima4.zip, u4upgrad.zip, etc.)
     */
    if (zipPkg) {
        u4f = U4FILE_zip::open(fname, zipPkg);
        if (u4f) {
            if (xu4.verbose) {
                printf("%s found in %s\n", fname.c_str(),
                       zipPkg->getFilename().c_str());
            }
            return u4f; /* file was found, return it! */
        }
    }

    /*
     * file not in a zipfile; check if it has been unzipped
     */
    string fname_copy(fname);

    string pathname = u4find_path(fname_copy.c_str(), &u4Path.u4ForDOSPaths);
    if (pathname.empty()) {
        using namespace std;
        if (islower(fname_copy[0])) {
            fname_copy[0] = toupper(fname_copy[0]);
            pathname = u4find_path(fname_copy.c_str(), &u4Path.u4ForDOSPaths);
        }

        if (pathname.empty()) {
            for (i = 0; fname_copy[i] != '\0'; i++) {
                if (islower(fname_copy[i]))
                    fname_copy[i] = toupper(fname_copy[i]);
            }
            pathname = u4find_path(fname_copy.c_str(), &u4Path.u4ForDOSPaths);
        }
    }

    if (!pathname.empty()) {
        u4f = U4FILE_stdio::open(pathname.c_str());
        if (xu4.verbose && u4f != NULL)
            printf("%s successfully opened\n", pathname.c_str());
    }

    return u4f;
}

U4FILE *u4fopen(const string &fname) {
    return u4fopen_zip(fname, u4zip_orig);
}

/**
 * Open a data file from the Ultima 4 upgrade.
 */
U4FILE *u4fopen_upgrade(const string &fname) {
    return u4fopen_zip(fname, u4zip_upgrad);
}

/**
 * Opens a file with the standard C stdio facilities and wrap it in a
 * U4FILE.
 */
U4FILE *u4fopen_stdio(const char* fname) {
    return U4FILE_stdio::open(fname);
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

extern "C" int u4find_pathc(const char* fname, const char* ext,
                            char* path, size_t pathSize) {
    const StringTable* st = &xu4.resourcePaths;
    const char* root = sst_strings(st);
    for (uint32_t i = 0; i < st->used; ++i) {
        snprintf(path, pathSize, "%s/%s%s",
                 root + sst_start(st, i), fname, ext);
        path[pathSize-1] = '\0';
        if (xu4.verbose)
            printf("trying to open %s\n", path);
        if (u4fexists(path))
            return 1;
    }
    return 0;
}

string u4find_path(const char* fname, const std::list<string>* subPaths) {
    bool found;
    char path[2048];    // Sometimes paths get big.

    // Try absolute first
    found = u4fexists(fname);
    if (found) {
        strcpy(path, fname);
        goto done;
    }

#if 0
    // Try 'file://' protocol if specified
    if (strncmp(fname, "file://", 7) == 0) {
        const char* upath = fname + 7;
        if (xu4.verbose)
            printf("trying to open %s\n", upath);
        if ((found = u4fexists(upath))) {
            strcpy(path, upath);
            goto done;
        }
    }
#endif

    // Try resource paths
    {
    const StringTable* st = &xu4.resourcePaths;
    const char* root = sst_strings(st);
    for (uint32_t i = 0; i < st->used; ++i) {
        if (subPaths) {
            std::list<string>::const_iterator sub;
            for (sub = subPaths->begin(); sub != subPaths->end(); ++sub) {
                snprintf(path, sizeof(path), "%s/%s/%s",
                         root + sst_start(st, i), sub->c_str(), fname);
                if (xu4.verbose)
                    printf("trying to open %s\n", path);
                if ((found = u4fexists(path)))
                    goto done;
            }
        } else {
            snprintf(path, sizeof(path), "%s/%s",
                     root + sst_start(st, i), fname);
            if (xu4.verbose)
                printf("trying to open %s\n", path);
            if ((found = u4fexists(path)))
                goto done;
        }
    }
    }

done:
    if (xu4.verbose) {
        if (found)
            printf("%s successfully found\n", path);
        else
            printf("%s not found\n", fname);
    }
    return found ? path : "";
}
