/*
  XU4 GUI Layout
  Copyright (C) 2022  Karl Robillard

  This software can be redistributed and/or modified under the terms of
  the GNU General Public License (see gui.cpp).
*/

#include "txf_draw.h"

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
    FIX_WIDTH_EM,   // font-em-tenth
    FIX_HEIGHT_EM,  // font-em-tenth
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
    FONT_VSIZE,     // scaled-vga-height (480 pixels)
    FONT_COLOR,     // color-index
    BG_COLOR_CI,    // color-index

    // Widgets
    ARRAY_DT_AREA,  // initial-wid, DATA GuiArea*
    BUTTON_DT_S,    // DATA const char*
    LABEL_DT_S,     // DATA const char*
    LIST_DIM,       // columns, rows
    STORE_DT_AREA,  // DATA int16_t[4]
    STORE_AREA
};

typedef struct {
    int16_t x, y, w, h;
} GuiRect;

// Matches BTree2Box
typedef struct {
    uint16_t x, y;
    uint16_t x2, y2;
    int wid;
} GuiArea;

struct ListCellStyle
{
    float   tabStop;
    float   fontScale;
    uint8_t color;
    uint8_t selColor;
};

struct ListDrawState : public TxfDrawState
{
    float   psizeList;
    uint8_t selected;
    uint8_t tabCount;
    ListCellStyle cell[4];
};

struct TxfDrawState;
struct StringTable;

float* gui_layout(int primList, const GuiRect* root, TxfDrawState*,
                  const uint8_t* bytecode, const void** data);
float* gui_emitListItems(float* attr, ListDrawState*, StringTable* st,
                         int select);
void*  gui_areaTree(const GuiArea* areas, int count);
const GuiArea* gui_pick(const void* tree, const GuiArea* areas,
                        uint16_t x, uint16_t y);
