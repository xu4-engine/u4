/*
 * $Id$
 */

#include <stdio.h>

#include "lzw.h"

/**
 * A simple command line interface to the U4 LZW decompressor.
 */
int main(int argc, char *argv[]) {
    FILE *in, *out;
    long inlen, outlen;
    void *outdata;

    if (argc != 2 && argc != 3) {
        fprintf(stderr, "usage: lzwdec infile [outfile]\n");
        exit(1);
    }

    in = fopen(argv[1], "rb");
    if (!in) {
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

    if (fseek(in, 0L, SEEK_END)) {
        perror(argv[1]);
        exit(1);
    }
    inlen = ftell(in);
    fseek(in, 0L, SEEK_SET);

    outlen = decompress_u4_file(in, inlen, &outdata);
    
    fwrite(outdata, 1, outlen, out);

    return 0;
}
