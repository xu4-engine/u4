/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * LZW "encode" a file.  This routine doesn't actually compress the
 * data (the algorithm is patented), but it creates a valid input file
 * for the decompressor.  It simply rewrites each 8-bit character into
 * a 12-bit character.  The output file size will be 1.5x the input
 * file size.
 */
int main(int argc, char *argv[]) {
    FILE *in, *out;
    int flip = 1;
    int c;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: lzwenc infile [outfile]\n");
        exit(1);
    }

    if (!(in = fopen(argv[1], "rb"))) {
        perror(argv[1]);
        exit(1);
    }

    if (argc < 3)
        out = stdout;
    else
        out = fopen(argv[2], "wb");
    if (!out) {
        perror(argc < 3 ? "(stdout)" : argv[2]);
        exit(1);
    }

    while ((c = getc(in)) != EOF) {
        if (flip) {
            putc(c >> 4, out);
            putc((c << 4) & 0xff, out);
        } else
            putc(c, out);
        flip = !flip;
    }

    return 0;
}
