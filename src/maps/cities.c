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
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &britain_city /* city */
};

City britain_city  = {
    "Britain",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "britain.tlk", /* tlk_fname */
    &britain_map
};

City cove_city;

Map cove_map = {
    "cove.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &cove_city /* city */
};

City cove_city  = {
    "Cove",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "cove.tlk", /* tlk_fname */
    &cove_map
};

City den_city;

Map den_map = {
    "den.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &den_city /* city */
};

City den_city  = {
    "Buccaneers Den",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "den.tlk", /* tlk_fname */
    &den_map
};

City jhelom_city;

Map jhelom_map = {
    "jhelom.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &jhelom_city /* city */
};

City jhelom_city  = {
    "Jhelom",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "jhelom.tlk", /* tlk_fname */
    &jhelom_map
};

City magincia_city;

Map magincia_map = {
    "magincia.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &magincia_city /* city */
};

City magincia_city  = {
    "Magincia",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "magincia.tlk", /* tlk_fname */
    &magincia_map
};

City minoc_city;

Map minoc_map = {
    "minoc.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &minoc_city /* city */
};

City minoc_city  = {
    "Minoc",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "minoc.tlk", /* tlk_fname */
    &minoc_map
};

City moonglow_city;

Map moonglow_map = {
    "moonglow.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &moonglow_city /* city */
};

City moonglow_city  = {
    "Moonglow",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "moonglow.tlk", /* tlk_fname */
    &moonglow_map
};

City paws_city;

Map paws_map = {
    "paws.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &paws_city /* city */
};

City paws_city  = {
    "Paws",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "paws.tlk", /* tlk_fname */
    &paws_map
};

City skara_city;

Map skara_map = {
    "skara.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &skara_city /* city */
};

City skara_city  = {
    "Skara Brae",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "skara.tlk", /* tlk_fname */
    &skara_map
};

City trinsic_city;

Map trinsic_map = {
    "trinsic.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &trinsic_city /* city */
};

City trinsic_city  = {
    "Trinsic",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "trinsic.tlk", /* tlk_fname */
    &trinsic_map
};

City vesper_city;

Map vesper_map = {
    "vesper.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &vesper_city /* city */
};

City vesper_city  = {
    "Vesper",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "vesper.tlk", /* tlk_fname */
    &vesper_map
};

City yew_city;

Map yew_map = {
    "yew.ult", /* fname */
    CITY_WIDTH, CITY_HEIGHT, /* width, height */
    0, 15, /* startx, starty */
    BORDER_EXIT2PARENT, /* border_behavior */
    0, /* n_portals */
    NULL, /* portals */
    SHOW_AVATAR, /* flags */
    NULL, /* data */
    &yew_city /* city */
};

City yew_city  = {
    "Yew",
    0,
    NULL,
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* person_types */
    "yew.tlk", /* tlk_fname */
    &yew_map
};
