#ifndef VIEW_H
#define VIEW_H

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

    void setHighlight(int x, int y, int width, int height);
    bool highlightActive() const { return highlightW > 0; }

    int screenRect[4];
    int x, y, width, height;

protected:
    int16_t highlightX, highlightY, highlightW, highlightH;

    void drawHighlighted();
};

#endif /* VIEW_H */
