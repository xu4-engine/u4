#include <windows.h>
#include <fcntl.h>
#include <io.h>

void redirectIOToConsole()
{
    int conHandle;
    intptr_t stdHandle;
    CONSOLE_SCREEN_BUFFER_INFO conInfo;
    FILE* fp;

    // allocate a console for this app
    AllocConsole();

    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);

    conInfo.dwSize.Y = 400;     // MAX_CONSOLE_LINES
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);

    // redirect unbuffered STDOUT to the console
    stdHandle = (intptr_t) GetStdHandle(STD_OUTPUT_HANDLE);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    fp = _fdopen(conHandle, "w");
    *stdout = *fp;
    setvbuf(stdout, NULL, _IONBF, 0);

    // redirect unbuffered STDIN to the console
    stdHandle = (intptr_t) GetStdHandle(STD_INPUT_HANDLE);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    fp = _fdopen(conHandle, "r");
    *stdin = *fp;
    setvbuf(stdin, NULL, _IONBF, 0);

    // redirect unbuffered STDERR to the console
    stdHandle = (intptr_t) GetStdHandle(STD_ERROR_HANDLE);
    conHandle = _open_osfhandle(stdHandle, _O_TEXT);
    fp = _fdopen(conHandle, "w");
    *stderr = *fp;
    setvbuf(stderr, NULL, _IONBF, 0);
}
