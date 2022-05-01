#include <stdint.h>

struct BlockingGroups;
class Map;
class TileView;

const char* gpu_init(void* res, int w, int h, int scale, int filter);
void     gpu_free(void* res);
void     gpu_viewport(int x, int y, int w, int h);
uint32_t gpu_loadTexture(const char* file, int linear);
uint32_t gpu_makeTexture(const Image32* img);
void     gpu_blitTexture(uint32_t tex, int x, int y, const Image32* img);
void     gpu_freeTexture(uint32_t id);
uint32_t gpu_screenTexture(void* res);
void     gpu_setTilesTexture(void* res, uint32_t tex, uint32_t mat, float vDim);
void     gpu_drawTextureScaled(void* res, uint32_t tex);
void     gpu_clear(void* res, const float* color);
void     gpu_invertColors(void* res);
void     gpu_setScissor(int* box);
float*   gpu_beginTris(void* res, int list);
void     gpu_endTris(void* res, int list, float* attr);
void     gpu_clearTris(void* res, int list);
void     gpu_drawTris(void* res, int list);
void     gpu_drawGui(void* res, int list, uint32_t tex);
float*   gpu_emitQuad(float* attr, const float* drawRect, const float* uvRect);
//void     gpu_render(void* res, const Image* screen);
void     gpu_resetMap(void* res, const Map* map);
void     gpu_drawMap(void* res, const TileView* view, const float* tileUVs,
                     const BlockingGroups* blocks,
                     int cx, int cy, float scale);
