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
extern int isAbyssOpened(void);

const Portal world_portals[] = {
    { 86, 107, -1,  &lcb_1_map,   15, 30, 0, ACTION_ENTER, NULL },
    { 218, 107, -1, &lycaeum_map, 15, 30, 0, ACTION_ENTER, NULL },
    { 28, 50, -1,   &empath_map,  15, 30, 0, ACTION_ENTER, NULL },
    { 146, 241, -1, &serpent_map, 15, 30, 0, ACTION_ENTER, NULL },
    { 232, 135, -1, &moonglow_map, 1, 15, 0, ACTION_ENTER, NULL },
    { 82, 106, -1,  &britain_map,  1, 15, 0, ACTION_ENTER, NULL },
    { 36, 222, -1,  &jhelom_map,   1, 15, 0, ACTION_ENTER, NULL },
    { 58, 43, -1,   &yew_map,      1, 15, 0, ACTION_ENTER, NULL },
    { 159, 20, -1,  &minoc_map,    1, 15, 0, ACTION_ENTER, NULL },
    { 106, 184, -1, &trinsic_map,  1, 15, 0, ACTION_ENTER, NULL },
    { 22, 128, -1,  &skara_map,    1, 15, 0, ACTION_ENTER, NULL },
    { 187, 169, -1, &magincia_map, 1, 15, 0, ACTION_ENTER, NULL },
    { 98, 145, -1,  &paws_map,     1, 15, 0, ACTION_ENTER, NULL },
    { 136, 158, -1, &den_map,      1, 15, 0, ACTION_ENTER, NULL },
    { 201, 59, -1,  &vesper_map,   1, 15, 0, ACTION_ENTER, NULL },
    { 136, 90, -1,  &cove_map,     1, 15, 0, ACTION_ENTER, NULL },
    { 233, 66, -1,  &shrine_honesty_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 128, 92, -1,  &shrine_compassion_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 36, 229, -1,  &shrine_valor_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 73, 11, -1,   &shrine_justice_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 205, 45, -1,  &shrine_sacrifice_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 81, 207, -1,  &shrine_honor_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 231, 216, -1, &shrine_humility_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 240, 73, -1,  &deceit_map,   0, 0, 0, ACTION_ENTER, NULL },
    { 91, 67, -1,   &despise_map,  0, 0, 0, ACTION_ENTER, NULL },
    { 72, 168, -1,  &destard_map,  0, 0, 0, ACTION_ENTER, NULL },
    { 126, 20, -1,  &wrong_map,    0, 0, 0, ACTION_ENTER, NULL },
    { 156, 27, -1,  &covetous_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 58, 102, -1,  &shame_map,    0, 0, 0, ACTION_ENTER, NULL },
    { 239, 240, -1, &hythloth_map, 0, 0, 0, ACTION_ENTER, NULL },
    { 233, 233, -1, &abyss_map,    0, 0, 0, ACTION_ENTER, &isAbyssOpened }
};

Map world_map = {
    0,
    NULL, /* fname */
    MAP_WORLD, /* type */
    MAP_WIDTH, MAP_HEIGHT, 1, /* width, height, levels */
    BORDER_WRAP, /* border_behavior */
    sizeof(world_portals) / sizeof(world_portals[0]), /* n_portals */
    world_portals, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_OUTSIDE, /* music */
    NULL, /* data */
    { NULL } /* city */
};
