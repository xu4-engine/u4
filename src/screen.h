/*
 * $Id$
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "direction.h"
#include "dngview.h"
#include "map.h"
#include "u4file.h"

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__
#define PRINTF_LIKE(x,y)  __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTF_LIKE(x,y)
#endif

struct _Tileset;
struct _Image;
struct _TileAnimSet;

typedef enum {
    BKGD_SHAPES,
    BKGD_CHARSET,
    BKGD_BORDERS,
    BKGD_INTRO,
    BKGD_INTRO_EXTENDED,
    BKGD_TREE,
    BKGD_PORTAL,
    BKGD_OUTSIDE,
    BKGD_INSIDE,
    BKGD_WAGON,
    BKGD_GYPSY,
    BKGD_ABACUS,
    BKGD_HONCOM,
    BKGD_VALJUS,
    BKGD_SACHONOR,
    BKGD_SPIRHUM,
    BKGD_ANIMATE,
    BKGD_KEY,
    BKGD_HONESTY,
    BKGD_COMPASSN,
    BKGD_VALOR,
    BKGD_JUSTICE,
    BKGD_SACRIFIC,
    BKGD_HONOR,
    BKGD_SPIRIT,
    BKGD_HUMILITY,
    BKGD_TRUTH,
    BKGD_LOVE,
    BKGD_COURAGE,
    BKGD_STONCRCL,
    BKGD_RUNE_INF,
    BKGD_SHRINE_HON,
    BKGD_SHRINE_COM,
    BKGD_SHRINE_VAL,
    BKGD_SHRINE_JUS,
    BKGD_SHRINE_SAC,
    BKGD_SHRINE_HNR,
    BKGD_SHRINE_SPI,
    BKGD_SHRINE_HUM,
    BKGD_GEMTILES,
    BKGD_MAX
} BackgroundType;

typedef enum {
    MC_DEFAULT,
    MC_WEST,
    MC_NORTH,
    MC_EAST,
    MC_SOUTH
} MouseCursor;

typedef struct _MouseArea {
    int npoints;
    struct {
        int x, y;
    } point[4];
    MouseCursor cursor;
    int command[3];
} MouseArea;

typedef enum {
    FIXUP_NONE,
    FIXUP_INTRO,
    FIXUP_INTRO_EXTENDED,
    FIXUP_ABYSS
} ImageFixup;

typedef struct {
    int id;
    const char *filename;
    int width, height, depth;
    int prescale;
    CompressionType filetype;
    int tiles;                  /* used to scale the without bleeding colors between adjacent tiles */
    int introOnly;              /* whether can be freed after the intro */
    int transparentIndex;       /* color index to consider transparent */
    int xu4Graphic;             /* an original xu4 graphic not part of u4dos or the VGA upgrade */
    ImageFixup fixup;           /* a routine to do miscellaneous fixes to the image */
} ImageInfo;

typedef struct {
    const char *name;
    const char *location;
    const char *extends;
    ImageInfo *image[BKGD_MAX];
} ImageSet;

typedef enum {
    LAYOUT_STANDARD,
    LAYOUT_GEM
} LayoutType;

typedef struct {
    char *name;
    LayoutType type;
    struct {
        int width, height;
    } tileshape;
    struct {
        int x, y;
        int width, height;
    } viewport;
} Layout;

typedef xu4_map<string, ImageSet*, std::less<string> >      ImageSetMap;
typedef xu4_map<string, Layout*, std::less<string> >        LayoutMap;
typedef xu4_map<string, struct _TileAnimSet*, std::less<string> >   TileAnimSetMap;

#define SCR_CYCLE_PER_SECOND 4

void screenInit(void);
void screenDelete(void);
void screenReInit(void);

char **screenGetGemLayoutNames(void);

int screenLoadBackground(BackgroundType bkgd);
void screenDrawBackground(BackgroundType bkgd);
void screenDrawBackgroundInMapArea(BackgroundType bkgd);
void screenFreeBackgrounds();

void screenCycle(void);
void screenEraseMapArea(void);
void screenEraseTextArea(int x, int y, int width, int height);
void screenFindLineOfSight(void);
void screenGemUpdate(void);
void screenInvertRect(int x, int y, int w, int h);
void screenMessage(const char *fmt, ...) PRINTF_LIKE(1, 2);
void screenPrompt(void);
void screenRedrawMapArea(void);
void screenRedrawScreen(void);
void screenRedrawTextArea(int x, int y, int width, int height);
void screenScrollMessageArea(void);
void screenShake(int iterations);
void screenShowTile(struct _Tileset* tileset, MapTile tile, int focus, int x, int y);
void screenShowGemTile(MapTile tile, int focus, int x, int y);
void screenShowChar(int chr, int x, int y);
void screenShowCharMasked(int chr, int x, int y, unsigned char mask);
void screenTextAt(int x, int y, const char *fmt, ...) PRINTF_LIKE(3, 4);
void screenUpdate(int showmap, int blackout);
void screenUpdateCursor(void);
void screenUpdateMoons(void);
void screenUpdateWind(void);
MapTile screenViewportTile(unsigned int width, unsigned int height, int x, int y, int *focus);

void screenAnimateIntro(int frame);
void screenFreeIntroAnimations();
void screenFreeIntroBackgrounds();
void screenShowCard(int pos, int card);
void screenShowAbacusBeads(int row, int selectedVirtue, int rejectedVirtue);
void screenShowBeastie(int beast, int vertoffset, int frame);

void screenShowCursor(void);
void screenHideCursor(void);
void screenEnableCursor(void);
void screenDisableCursor(void);
void screenSetCursorPos(int x, int y);

void screenDungeonDrawTile(int distance, MapTile tile);
void screenDungeonDrawWall(int xoffset, int distance, Direction orientation, DungeonGraphicType type);

void screenSetMouseCursor(MouseCursor cursor);
int screenPointInMouseArea(int x, int y, MouseArea *area);

extern int screenCurrentCycle;

#define SCR_CYCLE_MAX 16

#ifdef __cplusplus
}
#endif

#endif
