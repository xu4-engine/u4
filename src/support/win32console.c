#include <windows.h>
#include <fcntl.h>
#include <io.h>

static int isRedirected(DWORD stdNum)
{
    HANDLE fh = GetStdHandle(stdNum);
    if (fh) {
        DWORD type = GetFileType(fh);
        if (type == FILE_TYPE_DISK || type == FILE_TYPE_PIPE)
            return 1;
    }
    return 0;
}

void redirectIOToConsole()
{
    int outRedir, errRedir;
    CONSOLE_SCREEN_BUFFER_INFO conInfo;

    outRedir = isRedirected(STD_OUTPUT_HANDLE);
    errRedir = isRedirected(STD_ERROR_HANDLE);

    if (outRedir && errRedir)
        return;

    if (! AttachConsole(ATTACH_PARENT_PROCESS))
        return;

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);

    conInfo.dwSize.Y = 400;     // MAX_CONSOLE_LINES
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);

    if (! outRedir) {
        // redirect unbuffered STDOUT to the console
        freopen("CONOUT$", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0);
    }

    if (! errRedir) {
        // redirect unbuffered STDERR to the console
        freopen("CONOUT$", "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
    }

#if 0
    // redirect unbuffered STDIN to the console
    intptr_t stdHandle = (intptr_t) GetStdHandle(STD_INPUT_HANDLE);
    int conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    FILE* fp = _fdopen(conHandle, "r");
    *stdin = *fp;
    setvbuf(stdin, NULL, _IONBF, 0);
#endif
}
