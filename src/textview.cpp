/*
 * $Id$
 */

#include <stdarg.h>
#include <cstring>

#include "debug.h"
#include "event.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "textview.h"
#include "xu4.h"

Image *TextView::charset = NULL;

TextView::TextView(int x, int y, int columns, int rows) : View(x, y, columns * CHAR_WIDTH, rows * CHAR_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->cursorEnabled = false;
    this->cursorFollowsText = false;
    this->cursorX = 0;
    this->cursorY = 0;
    this->cursorPhase = 0;
    if (charset == NULL)
        charset = xu4.imageMgr->get(BKGD_CHARSET)->image;
    xu4.eventHandler->getTimer()->add(&cursorTimer, /*SCR_CYCLE_PER_SECOND*/4, this);
}

TextView::~TextView() {
    xu4.eventHandler->getTimer()->remove(&cursorTimer, this);
}

void TextView::reinit() {
    View::reinit();
    charset = xu4.imageMgr->get(BKGD_CHARSET)->image;
}

/**
 * Draw a character from the charset onto the view.
 */
void TextView::drawChar(int chr, int x, int y) {
    ASSERT(x < columns, "x value of %d out of range", x);
    ASSERT(y < rows, "y value of %d out of range", y);

    SCALED_VAR
    charset->drawSubRect(SCALED(this->x + (x * CHAR_WIDTH)),
                         SCALED(this->y + (y * CHAR_HEIGHT)),
                         0, SCALED(chr * CHAR_HEIGHT),
                         SCALED(CHAR_WIDTH),
                         SCALED(CHAR_HEIGHT));
}

/**
 * Draw a character from the charset onto the view, but mask it with
 * horizontal lines.  This is used for the avatar symbol in the
 * statistics area, where a line is masked out for each virtue in
 * which the player is not an avatar.
 */
void TextView::drawCharMasked(int chr, int x, int y, unsigned char mask) {
    SCALED_VAR
    drawChar(chr, x, y);
    for (int i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            xu4.screenImage->fillRect(SCALED(this->x + (x * CHAR_WIDTH)),
                                      SCALED(this->y + (y * CHAR_HEIGHT) + i),
                                      SCALED(CHAR_WIDTH), SCALED(1),
                                      0, 0, 0);
        }
    }
}

/* highlight the selected row using a background color */
void TextView::textSelectedAt(int x, int y, const char *text) {
    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.textColorization) {
        this->textAt(x, y, "%s", text);
        return;
    }

    this->setFontColorBG(BG_BRIGHT);
    for (int i=0; i < this->getWidth()-1; i++)
        this->textAt(x-1+i, y, " ");
    this->textAt(x, y, "%s", text);
    this->setFontColorBG(BG_NORMAL);
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
string TextView::colorizeString(string input, ColorFG color, unsigned int colorstart, unsigned int colorlength) {
    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.textColorization)
        return input;

    string output = "";
    string::size_type length = input.length();
    string::size_type i;
    bool colorization = false;

    // loop through the entire string and
    for (i = 0; i < length; i++) {
        if (i == colorstart) {
            output += color;
            colorization = true;
        }
        output += input[i];
        if (colorization) {
           colorlength--;
            if (colorlength == 0) {
                output += FG_WHITE;
                colorization = false;
            }
        }
    }

    // if we reached the end of the string without
    // resetting the color to white, do it now
    if (colorization)
        output += FG_WHITE;

    return output;
}

void TextView::setFontColor(ColorFG fg, ColorBG bg) {
    charset->setFontColorFG(fg);
    charset->setFontColorBG(bg);
}

void TextView::setFontColorFG(ColorFG fg) {
    charset->setFontColorFG(fg);
}
void TextView::setFontColorBG(ColorBG bg) {
    charset->setFontColorBG(bg);
}

void TextView::textAt(int x, int y, const char *fmt, ...) {
    char buffer[1024];
    unsigned int i;
    unsigned int offset = 0;

    bool reenableCursor = false;
    if (cursorFollowsText && cursorEnabled) {
        disableCursor();
        reenableCursor = true;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (i = 0; i < strlen(buffer); i++) {
        switch (buffer[i]) {
            case FG_GREY:
            case FG_BLUE:
            case FG_PURPLE:
            case FG_GREEN:
            case FG_RED:
            case FG_YELLOW:
            case FG_WHITE:
                setFontColorFG((ColorFG)buffer[i]);
                offset++;
                break;
            default:
                drawChar(buffer[i], x+(i-offset), y);
        }
    }

    if (cursorFollowsText)
        setCursorPos(x + i, y, true);
    if (reenableCursor)
        enableCursor();
}

void TextView::scroll() {
    Image* screen = xu4.screenImage;
    SCALED_VAR
    screen->drawSubRectOn(screen,
                          SCALED(x),
                          SCALED(y),
                          SCALED(x),
                          SCALED(y) + SCALED(CHAR_HEIGHT),
                          SCALED(width),
                          SCALED(height) - SCALED(CHAR_HEIGHT));

    screen->fillRect(SCALED(x),
                     SCALED(y + (CHAR_HEIGHT * (rows - 1))),
                     SCALED(width),
                     SCALED(CHAR_HEIGHT),
                     0, 0, 0);

    update();
}

void TextView::setCursorPos(int x, int y, bool clearOld) {
    while (x >= columns) {
        x -= columns;
        y++;
    }
    ASSERT(y < rows, "y value of %d out of range", y);

    if (clearOld && cursorEnabled) {
        drawChar(' ', cursorX, cursorY);
        update(cursorX * CHAR_WIDTH, cursorY * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT);
    }

    cursorX = x;
    cursorY = y;

    drawCursor();
}

void TextView::enableCursor() {
    cursorEnabled = true;
    drawCursor();
}

void TextView::disableCursor() {
    cursorEnabled = false;
    drawChar(' ', cursorX, cursorY);
    update(cursorX * CHAR_WIDTH, cursorY * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT);
}

void TextView::drawCursor() {
    ASSERT(cursorPhase >= 0 && cursorPhase < 4, "invalid cursor phase: %d", cursorPhase);

    if (!cursorEnabled)
        return;

    drawChar(31 - cursorPhase, cursorX, cursorY);
    update(cursorX * CHAR_WIDTH, cursorY * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT);
}

void TextView::cursorTimer(void *data) {
    TextView *thiz = static_cast<TextView *>(data);
    thiz->cursorPhase = (thiz->cursorPhase + 1) % 4;
    thiz->drawCursor();
}
