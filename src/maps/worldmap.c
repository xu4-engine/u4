/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"
#include "../portal.h"

extern Map britain_map;
extern Map yew_map;
extern Map minoc_map;
extern Map trinsic_map;
extern Map jhelom_map;
extern Map skara_map;
extern Map magincia_map;
extern Map moonglow_map;
extern Map paws_map;
extern Map vesper_map;
extern Map den_map;
extern Map cove_map;
extern Map empath_map;
extern Map lycaeum_map;
extern Map serpent_map;
extern Map lcb_1_map;
extern Map shrine_honesty_map;
extern Map shrine_compassion_map;
extern Map shrine_valor_map;
extern Map shrine_justice_map;
extern Map shrine_sacrifice_map;
extern Map shrine_honor_map;
extern Map shrine_humility_map;

const Portal world_portals[] = {
    { 82, 106, &britain_map, ACTION_ENTER },
    { 58, 43, &yew_map, ACTION_ENTER },
    { 159, 20, &minoc_map, ACTION_ENTER },
    { 106, 184, &trinsic_map, ACTION_ENTER },
    { 36, 222, &jhelom_map, ACTION_ENTER },
    { 22, 128, &skara_map, ACTION_ENTER },
    { 187, 169, &magincia_map, ACTION_ENTER },
    { 232, 135, &moonglow_map, ACTION_ENTER },
    { 98, 145, &paws_map, ACTION_ENTER },
    { 201, 59, &vesper_map, ACTION_ENTER },
    { 136, 158, &den_map, ACTION_ENTER },
    { 136, 90, &cove_map, ACTION_ENTER },
    { 28, 50, &empath_map, ACTION_ENTER },
    { 218, 107, &lycaeum_map, ACTION_ENTER },
    { 146, 241, &serpent_map, ACTION_ENTER },
    { 86, 107, &lcb_1_map, ACTION_ENTER },
    { 233, 66, &shrine_honesty_map, ACTION_ENTER },
    { 128, 92, &shrine_compassion_map, ACTION_ENTER },
    { 36, 229, &shrine_valor_map, ACTION_ENTER },
    { 73, 11, &shrine_justice_map, ACTION_ENTER },
    { 205, 45, &shrine_sacrifice_map, ACTION_ENTER },
    { 81, 207, &shrine_honor_map, ACTION_ENTER },
    { 231, 216, &shrine_humility_map, ACTION_ENTER }
};

Map world_map = {
    0,
    NULL, /* fname */
    MAP_WORLD, /* type */
    MAP_WIDTH, MAP_HEIGHT, /* width, height */
    86, 108, /* startx, starty */
    BORDER_WRAP, /* border_behavior */
    sizeof(world_portals) / sizeof(world_portals[0]), /* n_portals */
    world_portals, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_OUTSIDE, /* music */
    NULL, /* data */
    { NULL } /* city */
};
