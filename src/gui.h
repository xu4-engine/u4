/*
  XU4 GUI Layout
  Copyright (C) 2022  Karl Robillard

  This software can be redistributed and/or modified under the terms of
  the GNU General Public License (see gui.cpp).
*/

enum GuiOpcode {
    GUI_END,

    // Position Pen
    PEN_PER,        // percent x, percent y

    // Layout
    LAYOUT_V,
    LAYOUT_H,
    LAYOUT_GRID,    // columns
    LAYOUT_END,
    MARGIN_PER,     // percent
    MARGIN_V_PER,   // percent
    MARGIN_H_PER,   // percent
    SPACING_PER,    // percent
    SPACING_EM,     // font-em-tenth
    FIX_WIDTH_PER,  // percent
    FIX_HEIGHT_PER, // percent
    FROM_BOTTOM,
    FROM_RIGHT,
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_BOTTOM,
    ALIGN_H_CENTER,
    ALIGN_V_CENTER,
    ALIGN_CENTER,
    GAP_PER,        // percent

    // Drawing
    FONT_N,         // font-index
    FONT_SIZE,      // point-size
    BG_COLOR_CI,    // color-index

    // Widgets
    BUTTON_DT_S,    // DATA const char*
    LABEL_DT_S,     // DATA const char*
    LIST_DT_ST,     // DATA StringTable*
    STORE_DT_AREA   // DATA int[4]
};

typedef struct {
    int16_t x, y, w, h;
} GuiRect;

struct TxfHeader;

void gui_layout(int primList, const GuiRect* root, TxfHeader* const*,
                const uint8_t* bytecode, const void** data);
