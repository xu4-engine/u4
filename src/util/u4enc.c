/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rle.h"
#include "lzw/hash.h"
#include "util/pngconv.h"

#define MAX_DICT_CAPACITY 0xCCC
#define DICT_SIZE 0x1000

int compressing = 1;
int save = -1;
int dictsize = 0;
struct {
    int len;
    unsigned char *data;
    int occupied;
} lzwdict[DICT_SIZE];

/**
 * Outputs a 12 bit word.  If a half byte is left, it is saved until
 * the next time this is called.  flush_12 must be called to flush out
 * any saved data at the end of the stream.
 */
void putc_12(int c, FILE *out) {
    if (save == -1) {
        putc(c >> 4, out);
        save = c & 0x0f;
    } else {
        putc((save << 4) | (c >> 8), out);
        putc(c & 0xff, out);
        save = -1;
    }
}

/**
 * Flushes the last half byte from putc_12.
 */
void flush_12(FILE *out) {
    if (save != -1) {
        putc(save << 4, out);
        save = -1;
    }
}

/**
 *  Initializes the LZW dictionary.
 */
void initdict() {
    int i;

    dictsize = 0;

    for (i = 0; i < 256; i++) {
        lzwdict[i].len = 1;
        lzwdict[i].data = strdup("");
        lzwdict[i].data[0] = i;
        lzwdict[i].occupied = 1;
    }

    for (; i < DICT_SIZE; i++) {
        lzwdict[i].len = 0;
        lzwdict[i].data = NULL;
        lzwdict[i].occupied = 0;
    }
}

/**
 * Gets the 12-bit LZW code for a given string.  -1 is returned if not
 * in the dictionary.
 */
int getcode(unsigned char *str, int len) {
    int prefixcode;
    int hashcode;

    if (len == 1)
        return str[0];

    prefixcode = getcode(str, len - 1);

    hashcode = probe1(str[len - 1], prefixcode);
    if (lzwdict[hashcode].occupied &&
        lzwdict[hashcode].len == len &&
        memcmp(lzwdict[hashcode].data, str, len) == 0)
        return hashcode;
    hashcode = probe2(str[len - 1], prefixcode);
    if (lzwdict[hashcode].occupied &&
        lzwdict[hashcode].len == len &&
        memcmp(lzwdict[hashcode].data, str, len) == 0)
        return hashcode;
    do {
        hashcode = probe3(hashcode);
        if (lzwdict[hashcode].occupied &&
            lzwdict[hashcode].len == len &&
            memcmp(lzwdict[hashcode].data, str, len) == 0)
            return hashcode;
    } while (lzwdict[hashcode].occupied);

    return -1;
}

/**
 * Add a new word to the LZW dictionary.
 */
void addcode(unsigned char *str, int len) {
    int hashcode;

    if (!compressing)
        return;

    hashcode = probe1(str[len - 1], getcode(str, len - 1));
    if (lzwdict[hashcode].occupied) {
        hashcode = probe2(str[len - 1], getcode(str, len - 1));
    }
    if (lzwdict[hashcode].occupied) {
        do {
            hashcode = probe3(hashcode);
        } while (lzwdict[hashcode].occupied);
    }
    lzwdict[hashcode].len = len;
    lzwdict[hashcode].data = (unsigned char *) malloc(len);
    memcpy(lzwdict[hashcode].data, str, len);
    lzwdict[hashcode].occupied = 1;
    dictsize++;
}

/**
 * LZW encode a file.
 */
int main(int argc, char *argv[]) {
    FILE *out;
    char *alg, *infname, *outfname;
    int bits;
    int height = 0, width = 0;
    int datalen, c;
    unsigned char *data, *p;

    if (argc != 4) {
        fprintf(stderr, "usage: u4enc rle|lzw|raw infile outfile\n");
        exit(1);
    }

    alg = argv[1];
    infname = argv[2];
    outfname = argv[3];

    out = fopen(outfname, "wb");
    if (!out) {
        perror(outfname);
        exit(1);
    }

    readEgaFromPng(&data, &height, &width, &bits, infname);
    datalen = width * height * bits / 8;

    fprintf(stderr, "image is %dx%d (%d bits)\n", width, height, bits);

    if (strcmp(alg, "lzw") == 0) {
        unsigned char str[4096];
        int idx;

        initdict();

        p = data;
        idx = 0;
        c = *p++;
        str[idx++] = c;
        while (1) {
            c = *p++;
            if (p > (data + datalen))
                break;
            str[idx++] = c;
            if (getcode(str, idx) == -1) {
                int code = getcode(str, idx-1);
                putc_12(code, out);
                if (idx >= 4095) {
                    fprintf(stderr, "overflow in lzwenc\n");
                    exit(1);
                }
                addcode(str, idx);
                if (dictsize > MAX_DICT_CAPACITY) {
                    initdict();
                    putc_12(c, out);
                    c = *p++;
                    if (p > (data + datalen))
                        break;
                }
                idx = 0;
                str[idx++] = c;
            }
        }
        if (idx != 0)
            putc_12(getcode(str, idx), out);
        flush_12(out);
    }

    else if (strcmp(alg, "rle") == 0) {
        int count, threshold, val, i;

        /* 
         * The original, 4-bit graphics only start a run if the count is 5
         * or more; but the upgrade uses a run whereever it doesn't expand
         * the file (i.e. at a count of 3).  This value is adjusted to
         * reecode the files exactly as they were, if they weren't
         * changed.  A threshold of 3 or 4 gives optimal compression; 5 is
         * slightly worse and it is not clear why the original files used
         * it.
         */
        if (bits == 4)
            threshold = 5;
        else
            threshold = 3;

        p = data;
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
        if (count >= threshold || val == RLE_RUNSTART) {
            putc(RLE_RUNSTART, out);
            putc(count, out);
            putc(val, out);
        } else {
            for (i = 0; i < count; i++)
                putc(val, out);
        }
    }

    else if (strcmp(alg, "raw") == 0) {
        fwrite(data, datalen, 1, out);
    }

    else {
        fprintf(stderr, "unknown algorithm %s\n", alg);
        exit(1);
    }


    return 0;
}
