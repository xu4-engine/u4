/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rle.h"
#include "util/pngconv.h"

/**
 * RLE encode a file.
 */
int main(int argc, char *argv[]) {
    FILE *out;
    char *fname;
    int datalen;
    int i, c, count, val;
    unsigned char *data, *p;
    int bits;
    int threshold;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: rleenc infile [outfile]\n");
        exit(1);
    }

    fname = argv[1];

    if (argc < 3)
        out = stdout;
    else
        out = fopen(argv[2], "wb");
    if (!out) {
        perror(argc < 3 ? "(stdout)" : argv[2]);
        exit(1);
    }

    readEgaFromPng(&data, 200, 320, &bits, fname);

    /* 
     * The original, 4-bit graphics only start a run if the count is 5
     * or more; but the upgrade uses a run whereever it doesn't expand
     * the file (i.e. at a count of 3).  This value is adjusted to
     * reecode the files exactly as they were, if they weren't
     * changed.
     */
    if (bits == 4)
        threshold = 5;
    else
        threshold = 3;

    p = data;
    datalen = 320 * 200 * bits / 8;
    count = 0;
    val = -1;

    while (p < data + datalen) {
        c = *p++;
        if (c == val && count < 255)
            count++;
        else {
            if (count >= threshold || val == RLE_RUNSTART) {
                putc(RLE_RUNSTART, out);
                putc(count, out);
                putc(val, out);
            } else {
                for (i = 0; i < count; i++)
                    putc(val, out);
            }
            val = c;
            count = 1;
        }
    }
    if (count >= threshold) {
        putc(RLE_RUNSTART, out);
        putc(count, out);
        putc(val, out);
    } else {
        for (i = 0; i < count; i++)
            putc(val, out);
    }

    return 0;
}
