/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lzw/hash.h"

#define MAX_DICT_CAPACITY 0xCCC
#define DICT_SIZE 0x1000

int compress = 1;
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

    if (!compress)
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
    FILE *in, *out;
    unsigned char str[4096];
    int idx;
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

    initdict();

    idx = 0;
    c = getc(in);
    str[idx++] = c;
    while (1) {
        c = getc(in);
        if (c == EOF)
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
                c = getc(in);
                if (c == EOF)
                    break;
            }
            idx = 0;
            str[idx++] = c;
        }
    }
    if (idx != 0)
        putc_12(getcode(str, idx), out);
    flush_12(out);

    return 0;
}
