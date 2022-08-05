/*
 * Interface between GLV android_app and xu4 main().
 */

#include <unistd.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "glv_activity.h"
#include "error.h"

extern struct android_app* gGlvApp;


#define  LOG_TAG    "xu4"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)


#define REDIRECT_IO 1

#ifdef REDIRECT_IO
static void* stdioThread( void* arg )
{
    char buf[ 256 ];
    FILE* in = fdopen( ((int*) arg)[0], "r" );
    while( 1 )
    {
        fgets( buf, sizeof(buf), in );
        __android_log_write( ANDROID_LOG_VERBOSE, "stderr", buf );
    }
    return NULL;
}

int gStdOutPipe[2];

void redirectIO()
{
    pthread_t thr;
    pthread_attr_t attr;

    pipe( gStdOutPipe );
    //dup2( gStdOutPipe[1], STDOUT_FILENO );    // stdio doesn't seem to work.
    dup2( gStdOutPipe[1], STDERR_FILENO );

    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

    pthread_create( &thr, &attr, stdioThread, gStdOutPipe );
}
#endif


/*
 * Return number of bytes copied or 0 if failed.
 */
static int64_t extractAsset(ANativeActivity* act, const char* file,
                            char* buf, size_t bufSize)
{
    struct stat destStat;
    AAsset* in;
    int64_t copied = 0;
    const char* path = act->internalDataPath;

    if (! path)
        return 0;

    // TODO: Check that file size matches that of the asset.

    sprintf(buf, "%s/%s", path, file);
    if (stat(buf, &destStat) == 0 && destStat.st_size > 0)
        return destStat.st_size;

    in = AAssetManager_open(act->assetManager, file, AASSET_MODE_UNKNOWN);
    if (in) {
        FILE* out = fopen(buf, "w");
        if (out) {
            int n = -1;

            while (1) {
                n = AAsset_read(in, buf, bufSize);
                if (n < 1)
                    break;
                if (fwrite(buf, 1, n, out) != n)
                    break;
                copied += n;
            }
            fclose(out);

            if (n != 0)
                copied = 0;
        } else
            LOGE("Cannot write file (%s)\n", buf);

        AAsset_close(in);
    } else
        LOGE("Cannot open asset (%s)\n", file);

    return copied;
}


/*
 * Copy .mod/.pak assets to the internal data path so that they can be read
 * using normal file handles.
 */
static void extractModules( struct android_app* app )
{
    AAssetDir* dir = AAssetManager_openDir(app->activity->assetManager, "");
    if( dir ) {
        const size_t lenCopy = 4096;
        const char* fn;
        const char* ext;
        size_t len;
        char* buf = malloc(lenCopy);

        while ((fn = AAssetDir_getNextFileName(dir))) {
            len = strlen(fn);
            if (len > 4) {
                ext = fn + len - 4;
                if (strcmp(ext, ".mod") == 0 ||
                    strcmp(ext, ".pak") == 0 ||
                    strcmp(ext, ".zip") == 0) {
                    if (! extractAsset(app->activity, fn, buf, lenCopy)) {
                        errorFatal("Cannot extract module %s", fn);
                        break;
                    }
                }
            }
        }

        free(buf);
        AAssetDir_close( dir );
    }
}


#if 0
// Return pointer to a buffer which the caller must free() or NULL if failed.
void* loadAsset( struct android_app* app, const char* file, int* rlen )
{
    off_t len;
    AAsset* as;
    void* buf = 0;

    as = AAssetManager_open( app->activity->assetManager, file,
                             AASSET_MODE_BUFFER );
    if( ! as )
    {
        LOGE( "Could not open asset %s\n", file );
        return NULL;
    }

    len = AAsset_getLength( as );
    if( len > 0 )
    {
        buf = malloc( len );
        if( buf )
        {
            AAsset_read( as, buf, len );
            *rlen = len;
        }
    }

    AAsset_close( as );
    return buf;
}
#endif


const char* androidInternalData()
{
    return gGlvApp->activity->internalDataPath;
}


/*
  Event sequence initiated by ANativeActivity_finish().
    glv_activity: Pause                 -> APP_CMD_PAUSE
    glv_activity: NativeWindowDestroyed -> APP_CMD_TERM_WINDOW
    glv_activity: Stop                  -> APP_CMD_STOP
    glv_activity: InputQueueDestroyed   -> APP_CMD_INPUT_CHANGED
    glv_activity: Destroy               -> APP_CMD_DESTROY

  Event sequence when Home key pressed:
    glv_activity: WindowFocusChanged
    glv_activity: Pause
    glv_activity: SaveInstanceState     -> APP_CMD_SAVE_STATE
    glv_activity: Stop
    glv_activity: NativeWindowDestroyed

  Event sequence when Overview key pressed:
    glv_activity: Pause
    glv_activity: WindowFocusChanged *  (Sometimes seen before Pause)
    glv_activity: SaveInstanceState     -> APP_CMD_SAVE_STATE
    glv_activity: Stop
    glv_activity: NativeWindowDestroyed
*/

extern int main(int, char**);

jmp_buf gMainJump;

static char* dummyArgs[1] = { "xu4" };

void android_main( struct android_app* app )
{
#ifdef REDIRECT_IO
    redirectIO();
#endif

    // Set return point for errorFatal().
    if (setjmp(gMainJump))
        return;

    extractModules(app);

    main(1, dummyArgs);

    // Call finish to get the application window to close.
    ANativeActivity_finish( app->activity );

    // Finally we must handle the activity lifecycle events or Android thinks
    // our program has hung and it will not restart properly.
    android_app_wait_destroy( app );
}
