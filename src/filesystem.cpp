/**
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "filesystem.h"

#ifdef FS_WINDOWS
#   include <windows.h>
    const char Path::delim = '\\';
#else
    const char Path::delim = '/';
#endif

/**
 * Creates a path to a directory or file
 */
Path::Path(const string &p) : path(p) {
    struct stat path_stat;
    char src_char, dest_char;
    unsigned int pos;
    bool _exists = false,
        isDir = false;

    /* first, we need to translate the path into something the filesystem will recognize */
    dest_char = delim;
    if (delim == '\\')
        src_char = '/';
    else src_char = '\\';
    
    /* make the path work well with our OS */
    while ((pos = path.find(src_char)) < path.size())
        path[pos] = dest_char;

    /* determine if the path really exists */
    _exists = (stat(path.c_str(), &path_stat) == 0) ? true : false;
    
    /* if so, let's glean more information */
    if (_exists) 
        isDir = (path_stat.st_mode & _S_IFDIR) ? true : false;

    /* find the elements of the path that involve directory structure */
    string dir_path = isDir ? path : path.substr(0, path.find_last_of(dest_char));
            
    /* Add the trailing / or \ to the end, if it doesn't exist */
    if (dir_path[dir_path.size()-1] != dest_char)
        dir_path += dest_char;

    /* Get a list of directories for this path */
    while ((pos = dir_path.find(dest_char)) < dir_path.size()) {
        dirs.push_back(dir_path.substr(0, pos));
        dir_path = dir_path.substr(pos + 1);
    }

    /* If it's for sure a file, get file information! */
    if (path_stat.st_mode & _S_IFREG) {
        file = dirs.size() ? path.substr(path.find_last_of(dest_char)+1) : path;
        if ((pos = file.find_last_of(".")) < file.size()) {
            ext = file.substr(pos + 1);
            file = file.substr(0, pos);
        }
    }    
}
    
/**
 * Returns true if the path exists in the filesystem
 */
bool Path::exists() const {
    struct stat path_stat;
    return (stat(path.c_str(), &path_stat) == 0) ? true : false;
}

/**
 * Returns true if the path points to a file
 */
bool Path::isFile() const {
    struct stat path_stat;
    if ((stat(path.c_str(), &path_stat) == 0) && (path_stat.st_mode & _S_IFREG))
        return true;
    return false;
}

/**
 * Returns true if the path points to a directory
 */
bool Path::isDir() const {
    struct stat path_stat;
    if ((stat(path.c_str(), &path_stat) == 0) && (path_stat.st_mode & _S_IFDIR))
        return true;
    return false;
}

/**
 * Returns the full translated path
 */
string Path::getPath() const {
    return path;
}

/**
 * Returns the list of directories for the path
 */
std::list<string>* Path::getDirTree() {
    return &dirs;
}

/**
 * Returns the directory indicated in the path.
 */
string Path::getDir() const {
    std::list<string>::const_iterator i;
    string dir;

    for (i = dirs.begin(); i != dirs.end(); i++)
        dir += *i + delim;
    return dir;
}

/** Returns the full filename of the file designated in the path */
string Path::getFilename() const        { return (ext.empty()) ? file : file + string(".") + ext; }
string Path::getBaseFilename() const    { return file; }    /**< Returns the base filename of the file */
string Path::getExt() const             { return ext; }     /**< Returns the extension of the file (if it exists) */

/**
 * Returns true if the given path exists in the filesystem
 */
bool Path::exists(const string &path) {
    struct stat path_stat;
    return (stat(path.c_str(), &path_stat) == 0) ? true : false;
}

/**
 * Opens a file and attempts to create the directory structure beneath it before opening the file.
 */
FILE *FileSystem::openFile(const string &filepath, const char *mode) {
    Path path(filepath);
    createDirectory(filepath);
    return fopen(path.getPath().c_str(), mode);
}

/**
 * Create the directory that composes the path.
 * If any directories that make up the path do not exist,
 * they are created.
 */
void FileSystem::createDirectory(Path &path) {
    std::list<string>::iterator i;
    std::list<string>* dirs = path.getDirTree();
    string dir;

    for (i = dirs->begin(); i != dirs->end(); i++) {
        dir += *i;
        
        /* create each directory leading up to our path */
        if (!Path::exists(dir)) {
#           ifdef FS_WINDOWS
                CreateDirectoryA(dir.c_str(), 0);
#           else
                mkdir(dir.c_str(), S_IRWXU|S_IRWXG|S_IRWXO);
#           endif
        }

        dir += Path::delim;
    }
}

/**
 * Create a directory that composes the path
 */
void FileSystem::createDirectory(const string &filepath) {
    Path path(filepath);
    createDirectory(path);
}
