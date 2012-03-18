/**
 * $Id$
 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <list>
#include <string>

using std::string;
 
/**
 * The following code was taken from the Boost filesystem libraries (http://www.boost.org)
 */
#if !defined( FS_WINDOWS ) && !defined( FS_POSIX )
#   if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#       define FS_WINDOWS
#   else
#       define FS_POSIX
#   endif
#endif

/**
 * Provides cross-platform functionality for representing and validating paths.
 */
class Path {
public:
    Path(const string &p);
    
    bool exists() const;
    bool isFile() const;
    bool isDir() const;
    string getPath() const          {return path; } /**< Returns the full translated path */
    std::list<string>* getDirTree() {return &dirs;} /**< Returns the list of directories for the path */
    string getDir() const;
    string getFilename() const;
    string getBaseFilename() const;
    string getExt() const;

    static bool exists(const string &path);

    // Properties
    static const char delim;

private:
    string path;
    std::list<string> dirs;
    string file, ext;    
};
 
/**
 * Provides cross-platform functionality for file and directory operations.
 * It currently only supports directory creation, but other operations
 * will be added as support is needed.
 */
class FileSystem {
public:
    static FILE *openFile(const string &filepath, const char *mode = "wrb");
    static void createDirectory(Path &path);
    static void createDirectory(const string &filepath);
};
 
#endif
