/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u4.h"

int main(int argc, char **argv) {
    FILE *in, *out;
    char buffer[MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH];
    int x, xch, y, ych, i;

    if (argc != 3) {
	fprintf(stderr, "usage: %s infile outfile\n", argv[0]);
	exit(1);
    }

    if (strcmp(argv[1], "-") == 0)
	in = stdin;
    else
	in = fopen(argv[1], "r");
    if (!in) {
	perror(argv[1]);
	exit(1);
    }

    if (strcmp(argv[2], "-") == 0)
	out = stdout;
    else
	out = fopen(argv[2], "w");
    if (!out) {
	perror(argv[2]);
	exit(1);
    }

    xch = 0;
    ych = 0;
    x = 0;
    y = 0;
    for (i = 0; i < (MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH); i++) {
	buffer[x + (y * MAP_CHUNK_WIDTH * MAP_HORIZ_CHUNKS) + (xch * MAP_CHUNK_WIDTH) + (ych * MAP_CHUNK_HEIGHT * MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH)] = getc(in);
	x++;
	if (x >= MAP_CHUNK_WIDTH) {
	    x = 0;
	    y++;
	    if (y >= MAP_CHUNK_HEIGHT) {
		y = 0;
		xch++;
		if (xch >= MAP_HORIZ_CHUNKS) {
		    xch = 0;
		    ych++;
		}
	    }
	}
    }

    fprintf(out, "\
/* this file is generated automatically -- DO NOT EDIT!!! */
#include \"../map.h\"

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
extern Map shrine_map;

const Portal world_portals[] = {
\t{ 82, 106, &britain_map, ACTION_ENTER },
\t{ 58, 43, &yew_map, ACTION_ENTER },
\t{ 159, 20, &minoc_map, ACTION_ENTER },
\t{ 106, 184, &trinsic_map, ACTION_ENTER },
\t{ 36, 222, &jhelom_map, ACTION_ENTER },
\t{ 22, 128, &skara_map, ACTION_ENTER },
\t{ 187, 169, &magincia_map, ACTION_ENTER },
\t{ 232, 135, &moonglow_map, ACTION_ENTER },
\t{ 98, 145, &paws_map, ACTION_ENTER },
\t{ 201, 59, &vesper_map, ACTION_ENTER },
\t{ 136, 158, &den_map, ACTION_ENTER },
\t{ 136, 90, &cove_map, ACTION_ENTER },
\t{ 28, 50, &empath_map, ACTION_ENTER },
\t{ 218, 107, &lycaeum_map, ACTION_ENTER },
\t{ 146, 241, &serpent_map, ACTION_ENTER },
\t{ 86, 107, &lcb_1_map, ACTION_ENTER },
\t{ 128, 92, &shrine_map, ACTION_ENTER },
\t{ 73, 11, &shrine_map, ACTION_ENTER },
\t{ 205, 45, &shrine_map, ACTION_ENTER },
\t{ 233, 66, &shrine_map, ACTION_ENTER },
\t{ 231, 216, &shrine_map, ACTION_ENTER },
\t{ 81, 207, &shrine_map, ACTION_ENTER },
\t{ 36, 229, &shrine_map, ACTION_ENTER }
};

const unsigned char world_data[] = {
\t");

    for (y = 0; y < (MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT); y++) {
	for (x = 0; x < (MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH); x++) {
	    fprintf(out, "%d", buffer[y * (MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH) + x]);
	    if (y != (MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT) - 1 ||
		x != (MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH) - 1)
		fprintf(out, ",");
	}
	fprintf(out, "\n\t");
    }
    fprintf(out, "\n};\n");

    fprintf(out, "
const Map world_map = {
\t\"World\", /* name */
\t%d, /* width */
\t%d, /* height */
\t86, /* startx */
\t108, /* starty */
\t%s, /* border_behavior */
\t%s, /* n_portals */
\tworld_portals, /* portals */
\t0, /* n_persons */
\t0, /* persons */
\tSHOW_AVATAR, /* flags */
\tworld_data /* data */
};\n", MAP_HORIZ_CHUNKS * MAP_CHUNK_WIDTH, MAP_VERT_CHUNKS * MAP_CHUNK_HEIGHT, "BORDER_WRAP", "sizeof(world_portals) / sizeof(world_portals[0])");

    fclose(out);

    if (in != stdin)
	fclose(in);

    return 0;
}
