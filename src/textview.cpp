/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <stdarg.h>

#include "debug.h"
#include "event.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "textview.h"

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
        charset = imageMgr->get(BKGD_CHARSET)->image;
    eventHandler->getTimer()->add(&cursorTimer, /*SCR_CYCLE_PER_SECOND*/4, this);
}

TextView::~TextView() {
    eventHandler->getTimer()->remove(&cursorTimer, this);
}

void TextView::drawChar(int chr, int x, int y) {
    charset->drawSubRect(SCALED(this->x + (x * CHAR_WIDTH)),
                         SCALED(this->y + (y * CHAR_HEIGHT)),
                         0, SCALED(chr * CHAR_HEIGHT),
                         SCALED(CHAR_WIDTH),
                         SCALED(CHAR_HEIGHT));
}

void TextView::textAt(int x, int y, const char *fmt, ...) {
    char buffer[1024];
    unsigned int i;

    bool reenableCursor = false;
    if (cursorFollowsText && cursorEnabled) {
        disableCursor();
        reenableCursor = true;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for (i = 0; i < strlen(buffer); i++)
        drawChar(buffer[i], x + i, y);

    if (cursorFollowsText)
        setCursorPos(x + i, y, true);
    if (reenableCursor)
        enableCursor();
}

void TextView::scroll() {
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
