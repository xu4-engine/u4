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
extern Map deceit_map;
extern Map despise_map;
extern Map destard_map;
extern Map wrong_map;
extern Map covetous_map;
extern Map shame_map;
extern Map hythloth_map;
extern Map abyss_map;

const Portal world_portals[] = {
    { 86, 107, -1, &lcb_1_map, ACTION_ENTER },
    { 218, 107, -1, &lycaeum_map, ACTION_ENTER },
    { 28, 50, -1, &empath_map, ACTION_ENTER },
    { 146, 241, -1, &serpent_map, ACTION_ENTER },
    { 232, 135, -1, &moonglow_map, ACTION_ENTER },
    { 82, 106, -1, &britain_map, ACTION_ENTER },
    { 36, 222, -1, &jhelom_map, ACTION_ENTER },
    { 58, 43, -1, &yew_map, ACTION_ENTER },
    { 159, 20, -1, &minoc_map, ACTION_ENTER },
    { 106, 184, -1, &trinsic_map, ACTION_ENTER },
    { 22, 128, -1, &skara_map, ACTION_ENTER },
    { 187, 169, -1, &magincia_map, ACTION_ENTER },
    { 98, 145, -1, &paws_map, ACTION_ENTER },
    { 136, 158, -1, &den_map, ACTION_ENTER },
    { 201, 59, -1, &vesper_map, ACTION_ENTER },
    { 136, 90, -1, &cove_map, ACTION_ENTER },
    { 233, 66, -1, &shrine_honesty_map, ACTION_ENTER },
    { 128, 92, -1, &shrine_compassion_map, ACTION_ENTER },
    { 36, 229, -1, &shrine_valor_map, ACTION_ENTER },
    { 73, 11, -1, &shrine_justice_map, ACTION_ENTER },
    { 205, 45, -1, &shrine_sacrifice_map, ACTION_ENTER },
    { 81, 207, -1, &shrine_honor_map, ACTION_ENTER },
    { 231, 216, -1, &shrine_humility_map, ACTION_ENTER },
    { 240, 73, -1, &deceit_map, ACTION_ENTER },
    { 91, 67, -1, &despise_map, ACTION_ENTER },
    { 72, 168, -1, &destard_map, ACTION_ENTER },
    { 126, 20, -1, &wrong_map, ACTION_ENTER },
    { 156, 27, -1, &covetous_map, ACTION_ENTER },
    { 58, 102, -1, &shame_map, ACTION_ENTER },
    { 239, 240, -1, &hythloth_map, ACTION_ENTER },
    { 233, 233, -1, &abyss_map, ACTION_ENTER }
};

Map world_map = {
    0,
    NULL, /* fname */
    MAP_WORLD, /* type */
    MAP_WIDTH, MAP_HEIGHT, 1, /* width, height, levels */
    86, 108, -1, /* startx, starty, startlevel */
    BORDER_WRAP, /* border_behavior */
    sizeof(world_portals) / sizeof(world_portals[0]), /* n_portals */
    world_portals, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_OUTSIDE, /* music */
    NULL, /* data */
    { NULL } /* city */
};
