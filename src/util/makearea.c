/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u4.h"

int main(int argc, char **argv) {
    char *areaname;
    int startx, starty;
    FILE *con, *out;
    char buffer[VIEWPORT_W * VIEWPORT_H];
    int x, y;

    if (argc != 6) {
	fprintf(stderr, "usage: %s areaname startx starty confile outfile\n", argv[0]);
	exit(1);
    }

    areaname = argv[1];

    startx = strtoul(argv[2], NULL, 0);
    starty = strtoul(argv[3], NULL, 0);

    if (strcmp(argv[4], "-") == 0)
	con = stdin;
    else
	con = fopen(argv[4], "r");
    if (!con) {
	perror(argv[4]);
	exit(1);
    }

    if (strcmp(argv[5], "-") == 0)
	out = stdout;
    else
	out = fopen(argv[5], "w");
    if (!out) {
	perror(argv[5]);
	exit(1);
    }

    fread(buffer, 1, sizeof(buffer), con);

    fprintf(out, "\
/* this file is generated automatically -- DO NOT EDIT!!! */
#include \"../map.h\"

const unsigned char %s_data[] = {
\t", areaname);

    for (y = 0; y < VIEWPORT_H; y++) {
	for (x = 0; x < VIEWPORT_W; x++) {
	    fprintf(out, "%d", buffer[y * (VIEWPORT_W) + x]);
	    if (y != VIEWPORT_H - 1 ||
		x != VIEWPORT_W - 1)
		fprintf(out, ",");
	}
	fprintf(out, "\n\t");
    }
    fprintf(out, "\n};\n\n");

    fprintf(out, "\
const Map %s_map = {
\t\"%s\", /* name */
\t%d, /* width */
\t%d, /* height */
\t%d, /* startx */
\t%d, /* starty */
\t%s, /* border_behavior */
\t0, /* n_portals */
\t0, /* portals */
\t0, /* n_persons */
\t0, /* persons */
\t0, /* flags */
\t%s_data /* data */
};\n", areaname, areaname, VIEWPORT_W, VIEWPORT_H, startx, starty, "BORDER_FIXED", areaname);

    fclose(out);

    if (con != stdin)
	fclose(con);

    return 0;
}
