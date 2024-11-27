/*
 * TextView.cpp
 */

#include <stdarg.h>

#include "debug.h"
#include "imagemgr.h"
#include "screen.h"
#include "settings.h"
#include "textview.h"
#include "u4.h"
#include "xu4.h"

using std::string;

Image *TextView::charset = NULL;

TextView::TextView(int x, int y, int columns, int rows)
    : View(x, y, columns * CHAR_WIDTH, rows * CHAR_HEIGHT) {
    _cols = columns;
    _rows = rows;
    _cursorX = 0;
    _cursorY = 0;
    colorFG = FONT_COLOR_INDEX(FG_WHITE);
    colorBG = FONT_COLOR_INDEX(BG_NORMAL);
    cursorVisible = false;
    cursorFollowsText = false;

    if (charset == NULL)
        charset = xu4.imageMgr->get(BKGD_CHARSET)->image;
}

void TextView::reinit() {
    View::reinit();
    charset = xu4.imageMgr->get(BKGD_CHARSET)->image;
}

const RGBA fontColor[25] = {
    {153,153,153,255},  // FG_GREY TEXT_FG_PRIMARY   (red 0xFF, 0xAA in EGA)
    {102,102,102,255},  //         TEXT_FG_SECONDARY (red 0xC2)
    { 51, 51, 51,255},  //         TEXT_FG_SHADOW    (red 0x80)

    {102,102,255,255},  // FG_BLUE
    { 51, 51,204,255},
    { 51, 51, 51,255},

    {255,102,255,255},  // FG_PURPLE
    {204, 51,204,255},
    { 51, 51, 51,255},

    {102,255,102,255},  // FG_GREEN
    {  0,153,  0,255},
    { 51, 51, 51,255},

    {255,102,102,255},  // FG_RED
    {204, 51, 51,255},
    { 51, 51, 51,255},

    {255,255, 51,255},  // FG_YELLOW
    {204,153, 51,255},
    { 51, 51, 51,255},

    {255,255,255,255},  // FG_WHITE (Default)
    {204,204,204,255},
    { 68, 68, 68,255},

    {  0,  0,  0,255},  // BG_NORMAL
    {  0,  0,  0,  0},
    {  0,  0,  0,  0},

    {  0,  0,102,255},  // BG_BRIGHT
};

/**
 * Draw a character from the charset onto the view.
 */
void TextView::drawChar(int chr, int x, int y) {
    ASSERT(x < _cols, "x value of %d out of range", x);
    ASSERT(y < _rows, "y value of %d out of range", y);

    charset->drawLetter(this->x + (x * CHAR_WIDTH),
                        this->y + (y * CHAR_HEIGHT),
                        0, chr * CHAR_HEIGHT,
                        CHAR_WIDTH, CHAR_HEIGHT,
                        (chr < ' ') ? NULL : fontColor + colorFG,
                        fontColor + colorBG);
}

/**
 * Draw a character from the charset onto the view, but mask it with
 * horizontal lines.  This is used for the avatar symbol in the
 * statistics area, where a line is masked out for each virtue in
 * which the player is not an avatar.
 */
void TextView::drawCharMasked(int chr, int x, int y, uint8_t mask) {
    drawChar(chr, x, y);
    int mx = this->x + (x * CHAR_WIDTH);
    int my = this->y + (y * CHAR_HEIGHT);
    for (int i = 0; i < 8; i++) {
        if (mask & (1 << i))
            xu4.screenImage->fillRect(mx, my + i, CHAR_WIDTH, 1, 0, 0, 0);
    }
}

/* highlight the selected row using a background color */
void TextView::textSelectedAt(int x, int y, const char *text) {
    if (xu4.settings->enhancements &&
        xu4.settings->enhancementsOptions.textColorization) {
        colorBG = FONT_COLOR_INDEX(BG_BRIGHT);
        for (int i=0; i < _cols-1; i++)
            textAt(x-1+i, y, " ");
        textAt(x, y, text);
        colorBG = FONT_COLOR_INDEX(BG_NORMAL);
    } else {
        textAt(x, y, text);
    }
}

/* depending on the status type, apply colorization to the character */
string TextView::colorizeStatus(char statustype) {
    string output;

    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.textColorization) {
        output = statustype;
        return output;
    }

    switch (statustype) {
        case 'P':  output = FG_GREEN;    break;
        case 'S':  output = FG_PURPLE;   break;
        case 'D':  output = FG_RED;      break;
        default:   output = statustype;  return output;
    }
    output += statustype;
    output += FG_WHITE;
    return output;
}

/* depending on the status type, apply colorization to the character */
string TextView::highlightKey(const string& input, unsigned int keyIndex) {
    if (xu4.settings->enhancements &&
        xu4.settings->enhancementsOptions.textColorization) {
        string output;
        string::size_type length = input.length();
        string::size_type i;

        for (i = 0; i < length; i++) {
            if (i == keyIndex) {
                output += FG_YELLOW;
                output += input[i];
                output += FG_WHITE;
            } else
                output += input[i];
        }

        return output;
    }
    return input;
}

void TextView::textAt(int x, int y, const char *text) {
    int ch;
    while ((ch = *text++)) {
        switch (ch) {
            case FG_GREY:
            case FG_BLUE:
            case FG_PURPLE:
            case FG_GREEN:
            case FG_RED:
            case FG_YELLOW:
            case FG_WHITE:
                colorFG = FONT_COLOR_INDEX(ch);
                break;

            default:
                drawChar(ch, x++, y);
                break;
        }
    }

    if (cursorFollowsText) {
        _cursorX = x;
        _cursorY = y;
        if (cursorVisible)
            syncCursorPos();
    }
}

void TextView::textAtFmt(int x, int y, const char *fmt, ...) {
    char buffer[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    textAt(x, y, buffer);
}

/*
 * Draw text with a single character highlighted.
 */
void TextView::textAtKey(int x, int y, const char *text, int keyIndex) {
    string ctext = highlightKey(text, keyIndex);
    textAt(x, y, ctext.c_str());
}

void TextView::scroll() {
    Image* screen = xu4.screenImage;
    screen->drawSubRectOn(screen, x, y,
                          x, y + CHAR_HEIGHT,
                          width, height - CHAR_HEIGHT);

    screen->fillRect(x, y + (CHAR_HEIGHT * (_rows - 1)),
                     width, CHAR_HEIGHT,
                     0, 0, 0);

    update();
}

void TextView::syncCursorPos()
{
    screenSetCursorPos(x / CHAR_WIDTH + _cursorX, y / CHAR_HEIGHT + _cursorY);
}

/*
 * Position and show cursor.
 */
void TextView::setCursorPos(int x, int y) {
    while (x >= _cols) {
        x -= _cols;
        y++;
    }
    ASSERT(y < _rows, "y value of %d out of range", y);

    _cursorX = x;
    _cursorY = y;
    cursorVisible = true;
    syncCursorPos();
}

/*
 * Translate InputEvent mouse position to view character coordinates.
 */
void TextView::mouseTextPos(int mouseX, int mouseY, int& cx, int& cy) {
    const ScreenState* ss = screenState();
    int scale = ss->aspectH / U4_SCREEN_H;
    cx = (((mouseX - ss->aspectX) / scale) - x) / CHAR_WIDTH;
    cy = (((mouseY - ss->aspectY) / scale) - y) / CHAR_HEIGHT;
}

void TextView::showCursor() {
    cursorVisible = true;
    syncCursorPos();
}

void TextView::hideCursor() {
    cursorVisible = false;
    screenHideCursor();
}
