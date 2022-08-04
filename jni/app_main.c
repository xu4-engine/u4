

#include <unistd.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "glv_activity.h"


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


#if 0
void listAssets( struct android_app* app )
{
    const char* fn;
    AAssetDir* dir = AAssetManager_openDir( app->activity->assetManager, "" );
    if( dir )
    {
        while( (fn = AAssetDir_getNextFileName( dir ) ) )
        {
            LOGI( "asset: %s\n", fn );
        }
        AAssetDir_close( dir );
    }
}


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


UIndex loadScript( struct android_app* app, UThread* ut, const char* filename,
                   UCell* res )
{
    UIndex bufN = 0;
    int len;
    char* script = (char*) loadAsset( app, filename, &len );
    if( script )
    {
        // Comment out any Unix shell interpreter line.
        if( script[0] == '#' && script[1] == '!' )
            script[0] = ';';

        bufN = ur_tokenize( ut, script, script + len, res );
        free( script );
        if( bufN )
            boron_bindDefault( ut, bufN );
    }
    return bufN;
}
#endif


extern int main(int, char**);

static char* dummyArgs[1] = { "xu4" };

void android_main( struct android_app* app )
{
#ifdef REDIRECT_IO
    redirectIO();
#endif

    main(1, dummyArgs);

    // Call finish to get the application window to close.
    ANativeActivity_finish( app->activity );

    // Finally we must handle the activity lifecycle events or Android thinks
    // our program has hung and it will not restart properly.
    android_app_wait_destroy( app );
}
