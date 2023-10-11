#include <stdint.h>

enum GpuDrawList {
    GPU_DLIST_GUI,
    GPU_DLIST_HUD,
    GPU_DLIST_VIEW_OBJ,
    GPU_DLIST_VIEW_FX
};

enum GpuClutValues {
    COL_BLACK    = 0,
    COL_WHITE    = 1,
    COL_TX_BLACK = 2,
    COL_SD_BROWN = 4,
    COL_DK_BROWN = 5,
    COL_BEIGE    = 9,
    COL_BROWN    = 17,
    COL_YELLOW   = 19,
    COL_LT_GREEN = 33,
    COL_LT_BLUE  = 44,
    COL_LT_GRAY  = 63,
    COL_DK_GRAY  = 66,
    COL_TRANS    = 128
};

#define WID_NONE    -1

struct WorkRegion {
    uint32_t start;
    uint32_t avail;
    uint32_t used;
};

struct WorkBuffer {
    float*   attr;
    uint32_t dirty;         // LIMIT: 32 regions.
    uint16_t regionCount;
    WorkRegion region[2];
};

struct BlockingGroups;
class Map;
class TileView;

WorkBuffer* gpu_allocWorkBuffer(const int* regionSizes, int regionCount);
void        gpu_freeWorkBuffer(WorkBuffer*);
float*      gpu_beginRegion(WorkBuffer*, int regionN);
void        gpu_endRegion(WorkBuffer* work, int regionN, float* attr);

const char* gpu_init(void* res, int w, int h, int scale, int filter);
void     gpu_free(void* res);
void     gpu_viewport(int x, int y, int w, int h);
uint32_t gpu_makeTexture(const Image32* img);
void     gpu_blitTexture(uint32_t tex, int x, int y, const Image32* img);
void     gpu_freeTexture(uint32_t id);
uint32_t gpu_screenTexture(void* res);
void     gpu_setTilesTexture(void* res, uint32_t tex, uint32_t mat, float vDim);
void     gpu_drawTextureScaled(void* res, uint32_t tex);
void     gpu_clear(void* res, const float* color);
void     gpu_invertColors(void* res);
void     gpu_setScissor(int* box);
void     gpu_updateWorkBuffer(void* res, int list, WorkBuffer*);
void     gpu_drawTrisRegion(void* res, int list, const WorkRegion*);
float*   gpu_beginTris(void* res, int list);
void     gpu_endTris(void* res, int list, float* attr);
void     gpu_clearTris(void* res, int list);
void     gpu_drawTris(void* res, int list);
void     gpu_enableGui(void* res, int wid, int mode);
void     gpu_drawGui(void* res, int list, int wid, int mode);
void     gpu_guiClutUV(void* res, float* uv, float colorIndex);
void     gpu_guiSetOrigin(void* res, float x, float y);
float*   gpu_emitQuad(float* attr, const float* drawRect, const float* uvRect);
float*   gpu_emitQuadPq(float* attr, const float* drawRect, const float* uvRect,
                        float texP, float texQ);
void     gpu_resetMap(void* res, const Map* map);
void     gpu_drawMap(void* res, const TileView* view, const float* tileUVs,
                     const BlockingGroups* blocks,
                     int cx, int cy, float scale);
