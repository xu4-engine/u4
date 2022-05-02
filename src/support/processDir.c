#include "processDir.h"

#ifdef _WIN32
#include <io.h>

int processDir( const char* path, ProcessDirFunc func, void* user )
{
    char filespec[ _MAX_PATH ];
    struct _finddata_t fileinfo;
    intptr_t handle;
    int d_type;

    // Look for all files.  We ensure there is a slash before the wildcard.
    // It's OK if the path is already terminated with a slash - multiple
    // slashes are filtered by _findfirst.

    strcpy( filespec, path );
    strcat( filespec, "\\*.*" );

    handle = _findfirst( filespec, &fileinfo );
    if( handle == -1 )
        return 0;

	do
	{
		const char* cp = fileinfo.name;
		if( cp[0] == '.' && (cp[1] == '.' || cp[1] == '\0') )
			continue;

        // Set type to Linux DT_DIR (4) or DT_REG (8).
        d_type = (fileinfo.attrib & _A_SUBDIR) ? 4 : 8;

        if( ! func( cp, d_type, user ) )
            break;
	}
	while( _findnext( handle, &fileinfo ) != -1 );

	_findclose( handle );
	return 1;
}

#else

#include <sys/types.h>
#include <dirent.h>

int processDir( const char* path, ProcessDirFunc func, void* user )
{
    DIR* dir;
    struct dirent* entry;

    dir = opendir( path );
    if( ! dir )
        return 0;

    while( (entry = readdir( dir )) )
    {
        const char* cp = entry->d_name;
        if( cp[0] == '.' && (cp[1] == '.' || cp[1] == '\0') )
            continue;
        if( ! func( cp, entry->d_type, user ) )
            break;
    }
    closedir( dir );
    return 1;
}
#endif
