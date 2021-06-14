#include <allegro5/allegro5.h>

#ifdef USE_GL
#include "gpu_opengl.h"
#endif

struct ScreenAllegro {
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_DISPLAY* disp;
    ALLEGRO_TIMER* refreshTimer;
    ALLEGRO_MOUSE_CURSOR* cursors[5];
    double refreshRate;
    int currentCursor;
    int runRecursion;
#ifdef USE_GL
    OpenGLResources gpu;
#endif
};

#define SA  ((ScreenAllegro*) xu4.screenSys)
