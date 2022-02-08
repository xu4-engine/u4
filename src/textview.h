/*
 * TextView.h
 */

#ifndef TEXTVIEW_H
#define TEXTVIEW_H

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y)
#endif

#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8

#include "view.h"
#include "image.h"

enum TextColor {
    FG_GREY   = '\023',     // Foreground
    FG_BLUE   = '\024',
    FG_PURPLE = '\025',
    FG_GREEN  = '\026',
    FG_RED    = '\027',
    FG_YELLOW = '\030',
    FG_WHITE  = '\031',
    BG_NORMAL = '\032',     // Background
    BG_BRIGHT = '\033'
};

#define FONT_COLOR_INDEX(n)  ((n - 19) * 3)
extern const RGBA fontColor[25];

/**
 * A view of a text area.  Keeps track of the cursor position.
 */
class TextView : public View {
public:
    TextView(int x, int y, int columns, int rows);
    virtual ~TextView();

    void reinit();

    int getCursorX() const { return cursorX; }
    int getCursorY() const { return cursorY; }
    bool getCursorEnabled() const { return cursorEnabled; }
    int getWidth() const { return columns; }

    void drawChar(int chr, int x, int y);
    void drawCharMasked(int chr, int x, int y, unsigned char mask);
    void textAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(4, 5);
    void scroll();

    void setCursorFollowsText(bool follows) { cursorFollowsText = follows; }
    void setCursorPos(int x, int y, bool clearOld = true);
    void enableCursor();
    void disableCursor();

    // functions to add color to strings
    void textSelectedAt(int x, int y, const char *text);
    string colorizeStatus(char statustype);
    string colorizeString(string input, TextColor color, unsigned int colorstart, unsigned int colorlength=0);

protected:
    void drawCursor();
    static void cursorTimer(void *data);

    int columns, rows;          /**< size of the view in character cells  */
    bool cursorEnabled;         /**< whether the cursor is enabled */
    bool cursorFollowsText;     /**< whether the cursor is moved past the last character written */
    int cursorX, cursorY;       /**< current position of cursor */
    int cursorPhase;            /**< the rotation state of the cursor */
    uint8_t colorBG;
    uint8_t colorFG;
    static Image *charset;      /**< image containing font */
};

#endif /* TEXTVIEW_H */
