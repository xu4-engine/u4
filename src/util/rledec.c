/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "rle.h"
#include "util/pngconv.h"

#define WIDTH 320
#define HEIGHT 200

/**
 * A simple command line interface to the U4 RLE decompressor.
 */
int main(int argc, char *argv[]) {
    FILE *infile;
    void *indata, *outdata;
    long inlen, outlen;
    char *fname;

    if (argc != 3) {
        fprintf(stderr, "usage: rledec infile outfile\n");
        exit(1);
    }

    infile = fopen(argv[1], "rb");
    if (!infile) {
        perror(argv[1]);
        exit(1);
    }

    fname = argv[2];

    if (fseek(infile, 0L, SEEK_END)) {
        perror(argv[1]);
        exit(1);
    }
    inlen = ftell(infile);
    fseek(infile, 0L, SEEK_SET);
        
    indata = malloc(inlen);
    if (!indata) {
        perror("malloc");
        exit(1);
    }

    fread(indata, 1, inlen, infile);

    outlen = rleGetDecompressedSize(indata, inlen);

    outdata = malloc(outlen);
    if (!outdata) {
        perror("malloc");
        exit(1);
    }

    rleDecompress(indata, inlen, outdata, outlen);

    writePngFromEga(outdata, HEIGHT, WIDTH, outlen * 8 / (HEIGHT * WIDTH), fname);

    return 0;
}
