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

    void reinit();

    int cursorX() const { return _cursorX; }
    int cursorY() const { return _cursorY; }
    int columns() const { return _cols; }

    void drawChar(int chr, int x, int y);
    void drawCharMasked(int chr, int x, int y, unsigned char mask);
    void textAt(int x, int y, const char *text);
    void textAtFmt(int x, int y, const char *fmt, ...) PRINTF_LIKE(4, 5);
    void textAtKey(int x, int y, const char *text, int keyIndex);
    void scroll();

    void setCursorFollowsText(bool follows) { cursorFollowsText = follows; }
    void setCursorPos(int x, int y);
    void showCursor();
    void hideCursor();

    void mouseTextPos(int mouseX, int mouseY, int& tx, int& ty);

    // functions to add color to strings
    void textSelectedAt(int x, int y, const char *text);
    string colorizeStatus(char statustype);
    string highlightKey(const string& input, unsigned int keyIndex);

protected:
    void syncCursorPos();

    uint16_t _cols, _rows;      // size of the view in character cells
    int16_t _cursorX, _cursorY; // current position of cursor
    uint8_t colorBG;
    uint8_t colorFG;
    bool cursorVisible;         // whether the cursor is enabled
    bool cursorFollowsText;     // places cursor after last char. written
    static Image *charset;      // image containing font
};

#endif /* TEXTVIEW_H */
