#include <allegro5/allegro5.h>

#include "gpu_opengl.h"

struct ScreenAllegro {
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_DISPLAY* disp;
    ALLEGRO_MOUSE_CURSOR* cursors[5];
    double refreshRate;
    int currentCursor;
    OpenGLResources gpu;
};

#define SA  ((ScreenAllegro*) xu4.screenSys)
