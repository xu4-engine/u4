/*
 * $Id$
 */

#ifndef VIEW_H
#define VIEW_H

#define SCALED(n) ((n) * settings.scale)

class Image;

/**
 * Generic base class for reflecting the state of a game object onto
 * the screen.
 */
class View {
public:
    View(int x, int y, int width, int height);
    virtual ~View() {}

    virtual void clear();
    virtual void update();
    virtual void update(int x, int y, int width, int height);
    virtual void highlight(int x, int y, int width, int height);

protected:
    int x, y, width, height;
    static Image *screen;
};

#endif /* VIEW_H */
