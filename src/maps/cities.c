/*
 * $Id$
 */

#include <stddef.h>

#include "../u4.h"
#include "../map.h"
#include "../city.h"

City britain_city;

Map britain_map = {
    "britain.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &britain_city } /* city */
};

City britain_city  = {
    "Britain", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 29, 28, 27, 26, 0, 31, 25, 0, 0, 0, 0 }, /* person_types */
    "britain.tlk", /* tlk_fname */
    &britain_map /* map */
};

City cove_city;

Map cove_map = {
    "cove.ult", /* fname */
    MAP_VILLAGE, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &cove_city } /* city */
};

City cove_city  = {
    "Cove", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0 }, /* person_types */
    "cove.tlk", /* tlk_fname */
    &cove_map /* map */
};

City den_city;

Map den_map = {
    "den.ult", /* fname */
    MAP_VILLAGE, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &den_city } /* city */
};

City den_city  = {
    "Buccaneers Den", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 0, 28, 27, 0, 25, 30, 0, 0, 29, 0, 0, 0 }, /* person_types */
    "den.tlk", /* tlk_fname */
    &den_map /* map */
};

City jhelom_city;

Map jhelom_map = {
    "jhelom.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &jhelom_city } /* city */
};

City jhelom_city  = {
    "Jhelom", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 29, 28, 0, 30, 0, 25, 31, 0, 0, 0, 0 }, /* person_types */
    "jhelom.tlk", /* tlk_fname */
    &jhelom_map /* map */
};

City magincia_city;

Map magincia_map = {
    "magincia.ult", /* fname */
    MAP_RUIN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &magincia_city } /* city */
};

City magincia_city  = {
    "Magincia", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "magincia.tlk", /* tlk_fname */
    &magincia_map /* map */
};

City minoc_city;

Map minoc_map = {
    "minoc.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &minoc_city } /* city */
};

City minoc_city  = {
    "Minoc", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 30, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0 }, /* person_types */
    "minoc.tlk", /* tlk_fname */
    &minoc_map /* map */
};

City moonglow_city;

Map moonglow_map = {
    "moonglow.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &moonglow_city } /* city */
};

City moonglow_city  = {
    "Moonglow", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 0, 0, 26, 24, 25, 30, 0, 0, 0, 0, 0 }, /* person_types */
    "moonglow.tlk", /* tlk_fname */
    &moonglow_map /* map */
};

City paws_city;

Map paws_map = {
    "paws.ult", /* fname */
    MAP_VILLAGE, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &paws_city } /* city */
};

City paws_city  = {
    "Paws", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "paws.tlk", /* tlk_fname */
    &paws_map /* map */
};

City skara_city;

Map skara_map = {
    "skara.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &skara_city } /* city */
};

City skara_city  = {
    "Skara Brae", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 0, 0, 28, 0, 30, 31, 29, 0, 0, 0, 0 }, /* person_types */
    "skara.tlk", /* tlk_fname */
    &skara_map /* map */
};

City trinsic_city;

Map trinsic_map = {
    "trinsic.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &trinsic_city } /* city */
};

City trinsic_city  = {
    "Trinsic", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 29, 28, 0, 31, 0, 0, 29, 0, 0, 0, 0 }, /* person_types */
    "trinsic.tlk", /* tlk_fname */
    &trinsic_map /* map */
};

City vesper_city;

Map vesper_map = {
    "vesper.ult", /* fname */
    MAP_VILLAGE, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &vesper_city } /* city */
};

City vesper_city  = {
    "Vesper", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 0, 25, 0, 0, 23, 0, 0, 26, 23, 0, 0, 0 }, /* person_types */
    "vesper.tlk", /* tlk_fname */
    &vesper_map /* map */
};

City yew_city;

Map yew_map = {
    "yew.ult", /* fname */
    MAP_TOWN, /* type */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    1, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    MUSIC_TOWNS, /* music */
    NULL, /* data */
    { &yew_city } /* city */
};

City yew_city  = {
    "Yew", /* name */
    0, /* n_persons */
    NULL, /* persons */
    { 32, 0, 0, 26, 0, 0, 26, 0, 0, 0, 0, 0 }, /* person_types */
    "yew.tlk", /* tlk_fname */
    &yew_map /* map */
};
