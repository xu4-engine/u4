/*
 * $Id$
 */

#ifndef U4_SDL_H
#define U4_SDL_H

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif


int u4_SDL_InitSubSystem(Uint32 flags);
void u4_SDL_QuitSubSystem(Uint32 flags);

#ifdef __cplusplus
}
#endif

#endif
