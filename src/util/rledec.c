/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "rle.h"
#include "util/pngconv.h"

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 200

int decode_rle(char *infile_name, char *outfile_name, int width, int height);
long getFilesize(FILE *file);
int isPowerOfTwo(int n);

/**
 * A simple command line interface to the U4 RLE decompressor.
 */
int main(int argc, char *argv[]) {
    char *usage = "Usage:\nrledec infile outfile [width height]\n";
    int width, height;

    switch (argc) {
        case 2+1:
            printf("Using default width and height.\n");
            return(decode_rle(argv[1],argv[2],DEFAULT_WIDTH,DEFAULT_HEIGHT));
        case 4+1:
            width = atoi(argv[3]);
            height = atoi(argv[4]);
            if (width <= 0 || height <= 0) {
                if (width <= 0) printf("Invalid width: %d\n",width);
                if (height <= 0) printf("Invalid height: %d\n",height);
                exit(EXIT_FAILURE);
            }
            return(decode_rle(argv[1],argv[2],width,height));            
            break;
        default:
            printf("%s",usage);
            exit(EXIT_FAILURE);
            break;
    }
}

int decode_rle(char *infile_name, char *outfile_name, int width, int height) {
    FILE *infile;
    unsigned char *indata, *outdata;
    long inlen, outlen;
    int cond1, cond2;

    infile = fopen(infile_name, "rb");
    if (!infile) {
        printf("Couldn't open %s for reading.\n", infile_name);
        return(EXIT_FAILURE);
    }
    inlen = getFilesize(infile);

    if (inlen == 0) {
        printf("%s is empty.\n", infile_name);
        return(EXIT_FAILURE);
    }
    indata = (unsigned char*) malloc(inlen);
    if (!indata) {
        printf("Couldn't allocate indata.\n");
        return(EXIT_FAILURE);
    }

    fread(indata, 1, inlen, infile);

    outlen = rleGetDecompressedSize(indata, inlen);

    cond1 = (outlen*8) % (width*height) == 0;
    cond2 = isPowerOfTwo((outlen*8) / (width*height));
    if (!cond1 || !cond2) {
        printf("Invalid width or height.\n");
        return(EXIT_FAILURE);
    }

    outdata = malloc(outlen);
    if (!outdata) {
        printf("Couldn't allocate outdata.\n");
        return(EXIT_FAILURE);
    }

    rleDecompress(indata, inlen, outdata, outlen);

    writePngFromEga(outdata, height, width, outlen * 8 / (height * width), outfile_name);

    return(EXIT_SUCCESS);
}

long getFilesize(FILE *file) {
    long temp, filesize;

    temp = ftell(file);
    fseek(file, 0L, SEEK_END);
    filesize = ftell(file);
    fseek(file, temp, SEEK_SET);
    return(filesize);
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
