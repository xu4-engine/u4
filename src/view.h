/*
 * $Id$
 */

#ifndef VIEW_H
#define VIEW_H

#define SCALED(n) ((n) * settings.scale)

#ifdef IOS
#include "ios_helpers.h"
#endif

class Image;

/**
 * Generic base class for reflecting the state of a game object onto
 * the screen.
 */
class View {
public:
    View(int x, int y, int width, int height);
    virtual ~View() {}

    virtual void reinit();
    virtual void clear();
    virtual void update();
    virtual void update(int x, int y, int width, int height);
    virtual void highlight(int x, int y, int width, int height);
    virtual void unhighlight();

protected:
    const int x, y, width, height;
    bool highlighted;
    int highlightX, highlightY, highlightW, highlightH;
    void drawHighlighted();
#ifdef IOS
    friend void U4IOS::updateScreenView();
#endif
    static Image *screen;
};

#endif /* VIEW_H */
