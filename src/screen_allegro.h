#include <allegro5/allegro5.h>

struct ScreenAllegro {
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_DISPLAY* disp;
    ALLEGRO_TIMER* refreshTimer;
    ALLEGRO_MOUSE_CURSOR* cursors[5];
    double refreshRate;
    int currentCursor;
    int runRecursion;
};

#define SA  ((ScreenAllegro*) xu4.screen)
