/*
 * $Id$
 */

#ifndef VIEW_H
#define VIEW_H

#ifdef IOS
#include "ios_helpers.h"
#endif

class TileView;

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

    bool highlightActive() const { return highlighted; }
    int screenRect[4];

protected:
    int x, y, width, height;
    int highlightX, highlightY, highlightW, highlightH;
    bool highlighted;

    void drawHighlighted();
#ifdef IOS
    friend void U4IOS::updateScreenView();
#endif
    friend void screenUpdate(TileView*, bool, bool);
};

#endif /* VIEW_H */
