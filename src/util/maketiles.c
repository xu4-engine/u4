/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int unlink(const char *);

#include "u4.h"

int get_halfbyte(FILE *f) {
    static int is_saved = 0;
    static int saved = 0;

    if (is_saved) {
	is_saved = 0;
	return saved & 0xf;
    } else {
	saved = getc(f);
	if (saved == EOF)
	    return EOF;
	is_saved = 1;
	return saved >> 4;
    }
}

void dump_bmp(char *name, char *tmpfn, FILE *out) {
    FILE *convert;
    int c, first, size, status;
    char cmd[256];
    char *convertCmd;

    convertCmd = getenv("CONVERT");
    if (!convertCmd)
        convertCmd = "convert";

    sprintf(cmd, "%s -compress None %s bmp:-", convertCmd, tmpfn);
    convert = popen(cmd, "r");
    if (!convert) {
        fprintf(stderr, "ImageMagick convert failed: %s\n", cmd);
        exit(1);
    }
    
    fprintf(out, "\
/* this file is generated automatically -- DO NOT EDIT!!! */
const unsigned char %s_data[] = {
", name);

    first = 1;
    size = 0;
    while((c = getc(convert)) != EOF) {
        fprintf(out, "%s%d", first ? "\t" : ", ", (unsigned char) c);
        first = 0;
        size++;
    }

    fprintf(out, "\
};

const int %s_size = %d;
", name, size);

    status = pclose(convert);
    if (status != 0) {
        fprintf(stderr, "ImageMagick convert failed: %s\n", cmd);
        exit(1);
    }
}


int main(int argc, char **argv) {
    char *name, *outfn;
    int width, height, n_tiles;
    FILE *in, *tmp, *out;
    int i, j, k;
    char tmpfn[] = "/tmp/maketilesXXXXXX";
    char pixmap_chars[] = " 123456789abcdef";

    if (argc != 4) {
	fprintf(stderr, "usage: %s name infile outfile\n", argv[0]);
	exit(1);
    }

    name = argv[1];

    in = fopen(argv[2], "r");
    if (!in) {
	perror(argv[2]);
	exit(1);
    }

    outfn = argv[3];
    out = fopen(outfn, "w");
    if (!out) {
	perror(argv[3]);
	exit(1);
    }

    if (strcmp(name, "tiles") == 0) {
        width = TILE_WIDTH;
        height = TILE_HEIGHT;
        n_tiles = N_TILES;
    } else if (strcmp(name, "charset") == 0) {
        width = CHAR_WIDTH;
        height = CHAR_HEIGHT;
        n_tiles = N_CHARS;
    } else {
        fprintf(stderr, "unknown name %s\n", name);
        exit(1);
    }
        
    tmp = fdopen(mkstemp(tmpfn), "w");
    if (!tmp) {
        perror(tmpfn);
        exit(1);
    }

    fprintf(tmp, "\
/* XPM */
static char * %s_xpm[] = {
\"%d %d 16 1\",
\" 	c #000000\",
\"1	c #000080\",
\"2	c #008000\",
\"3	c #008080\",
\"4	c #800000\",
\"5	c #008080\",
\"6	c #808000\",
\"7	c #C3C3C3\",
\"8	c #A0A0A0\",
\"9	c #0000FF\",
\"a	c #00FF00\",
\"b	c #FF0000\",
\"c	c #FF0000\",
\"d	c #FF00FF\",
\"e	c #FFFF00\",
\"f	c #FFFFFF\",
", name, width * SCALE, height * n_tiles * SCALE);

    for (i = 0; i < height * n_tiles; i++) {
	char *line = malloc(width * SCALE + 1);
	line[0] = '\0';

	for (j = 0; j < width; j+=2) {
	    int temp = getc(in);
	    for (k = 0; k < SCALE; k++)
		sprintf(line + strlen(line), "%c", pixmap_chars[temp >> 4]);
	    for (k = 0; k < SCALE; k++)
		sprintf(line + strlen(line), "%c", pixmap_chars[temp & 0xf]);
	}
	for (k = 0; k < SCALE; k++) {
	    if (i != 0 || k != 0)
		fprintf(tmp, ",\n");
	    fprintf(tmp, "\"%s\"", line);
	}
	free(line);
    }
    fprintf(tmp, "};\n");

    fclose(in);
    fclose(tmp);

    dump_bmp(name, tmpfn, out);
    unlink(tmpfn);

    fclose(out);

    return 0;
}
