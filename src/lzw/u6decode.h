/*
 * $Id$
 */

#ifndef U6DECODE_H
#define U6DECODE_H

#include <stdio.h>

namespace U6Decode {
    class Stack;
    class Dict;

    unsigned char read1(FILE *f);
    long read4(FILE *f);
    long get_filesize(FILE *input_file);
    bool is_valid_lzw_file(FILE *input_file);
    long get_uncompressed_size(FILE *input_file);
    int get_next_codeword (long& bits_read, unsigned char *source, int codeword_size);
    void output_root(unsigned char root, unsigned char *destination, long& position);
    void get_string(Stack &stack, int codeword);
    int lzw_decompress(unsigned char *source, long source_length, unsigned char *destination, long destination_length);
    int lzw_decompress(FILE *input_file, FILE* output_file);
};

#endif
