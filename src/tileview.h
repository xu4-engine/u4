/*
 * $Id$
 */

#ifndef TILEVIEW_H
#define TILEVIEW_H

#include <vector>

#include "anim.h"
#include "coords.h"
#include "types.h"
#include "view.h"

class Tileset;

#ifdef GPU_RENDER
enum VisualEffectMethod {
    VE_FREE,
    VE_SPRITE,
    VE_SPRITE_MOVE
};

#define VE_MAX  4

struct VisualEffect {
    float pos[2];
    uint16_t method;
    VisualId vid;
    AnimId anim;
};
#endif

/**
 * A view of a grid of tiles.  Used to draw Maps.
 * @todo
 * <ul>
 *      <li>use for gem view</li>
 *      <li>intialize from a Layout?</li>
 * </ul>
 */
class TileView : public View {
public:
    TileView(int x, int y, int columns, int rows);
    virtual ~TileView();

    void reinit();
    void drawTile(const MapTile &mapTile, int x, int y);
    void drawTile(std::vector<MapTile> &tiles, int x, int y);
    void drawFocus(int x, int y);
    void loadTile(const MapTile &mapTile);

#ifdef GPU_RENDER
    int showEffect(const Coords &coords, TileId tile,
                   AnimId moveAnim = ANIM_UNUSED);
    void removeEffect(int id);
    void updateEffects(float cx, float cy, const float* uvTable);
#endif

protected:
    int columns, rows;
    int tileWidth, tileHeight;
    const Tileset *tileset;
    Image *animated;        /**< a scratchpad image for drawing animations */
#ifdef GPU_RENDER
    int effectCount;
    VisualEffect effect[VE_MAX];
#endif
};

#endif /* TILEVIEW_H */
