/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdlib>

#include "rle.h"

/**
 * Decompress an RLE encoded file.
 */
long rleDecompressFile(FILE *in, long inlen, void **out) {
    void *indata;
    long outlen;

    /* input file should be longer than 0 bytes */
    if (inlen <= 0)
        return -1;

    /* load compressed file into memory */
    indata = malloc(inlen);
    fread(indata, 1, inlen, in);

    outlen = rleDecompressMemory(indata, inlen, out);

    free(indata);

    return outlen;
}

long rleDecompressMemory(void *in, long inlen, void **out) {
    unsigned char *indata, *outdata;
    long outlen;

    /* input should be longer than 0 bytes */
    if (inlen <= 0)
        return -1;

    indata = (unsigned char *)in;

    /* determine decompressed file size */
    outlen = rleGetDecompressedSize(indata, inlen);

    if (outlen <= 0)
        return -1;

    /* decompress file from inlen to outlen */
    outdata = (unsigned char *) malloc(outlen);
    rleDecompress(indata, inlen, outdata, outlen);

    *out = outdata;

    return outlen;
}

/**
 * Determine the uncompressed size of RLE compressed data.
 */
long rleGetDecompressedSize(unsigned char *indata, long inlen) {
    unsigned char *p;
    unsigned char ch, count, val;
    long len = 0;

    p = indata;
    while ((p - indata) < inlen) {
        ch = *p++;
        if (ch == RLE_RUNSTART) {
            count = *p++;
            val = *p++;
            len += count;
        } else
            len++;
    }

    return len;
}

/**
 * Decompress a block of RLE encoded memory.
 */
long rleDecompress(unsigned char *indata, long inlen, unsigned char *outdata, long outlen) {
    int i;
    unsigned char *p, *q;
    unsigned char ch, count, val;

    p = indata;
    q = outdata;
    while ((p - indata) < inlen) {
        ch = *p++;
        if (ch == RLE_RUNSTART) {
            count = *p++;
            val = *p++;
            for (i = 0; i < count; i++) {
                *q++ = val;
                if ((q - outdata) >= outlen)
                    break;
            }
        } else {
            *q++ = ch;
            if ((q - outdata) >= outlen)
                break;
        }
    }

    return q - outdata;
}
