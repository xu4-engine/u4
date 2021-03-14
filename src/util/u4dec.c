/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lzw/lzw.h"
#include "rle.h"
#include "util/pngconv.h"

int isPowerOfTwo(int n);

/**
 * A simple command line interface to the U4 RLE and LZW decompressors.
 */
int main(int argc, char *argv[]) {
    FILE *infile;
    unsigned char *indata, *outdata;
    long inlen, outlen;
    char *alg, *infname, *outfname;
    int width, height;
    int cond1, cond2;

    if (argc != 4 && argc != 6) {
        fprintf(stderr, "usage: u4dec rle|lzw|raw infile outfile [width height]\n");
        exit(1);
    }

    alg = argv[1];
    infname = argv[2];
    outfname = argv[3];

    if (argc > 4) {
        width = strtoul(argv[4], NULL, 0);
        height = strtoul(argv[5], NULL, 0);
    } else {
        width = 320;
        height = 200;
    }

    printf("decoding %s image of size %dx%d\n", alg, width, height);

    infile = fopen(infname, "rb");
    if (!infile) {
        perror(infname);
        exit(1);
    }

    if (fseek(infile, 0L, SEEK_END)) {
        perror(infname);
        exit(1);
    }
    inlen = ftell(infile);
    fseek(infile, 0L, SEEK_SET);

    indata = (unsigned char *)malloc(inlen);
    if (!indata) {
        perror("malloc");
        exit(1);
    }

    fread(indata, 1, inlen, infile);

    if (strcmp(alg, "lzw") == 0) {
        outlen = lzwGetDecompressedSize(indata, inlen);

        outdata = (unsigned char *)malloc(outlen);
        if (!outdata) {
            perror("malloc");
            exit(1);
        }

        lzwDecompress(indata, outdata, inlen);
    }

    else if (strcmp(alg, "rle") == 0) {
        outlen = rleGetDecompressedSize(indata, inlen);

        cond1 = (outlen*8) % (width*height) == 0;
        cond2 = isPowerOfTwo((outlen*8) / (width*height));
        if (!cond1 || !cond2) {
            printf("Invalid width or height.\n");
            return(EXIT_FAILURE);
        }

        outdata = (unsigned char *)malloc(outlen);
        if (!outdata) {
            printf("Couldn't allocate outdata.\n");
            return(EXIT_FAILURE);
        }

        rleDecompress(indata, inlen, outdata, outlen);
    }

    else if (strcmp(alg, "raw") == 0) {
        outlen = inlen;
        outdata = indata;
    }

    else {
        fprintf(stderr, "unknown algorithm %s\n", alg);
        exit(1);
    }

    writePngFromEga(outdata, height, width, outlen * 8 / (height * width), outfname);

    return 0;
}

int isPowerOfTwo(int n) {
    int tmp;

    if (n <= 0)
        return(0);
    else {
        tmp = n;
        while (tmp % 2 == 0)
            tmp = tmp >> 1;
        if (tmp == 1) return(1);
        else return(0);
    }
}
