/*
 *  u4decode.c - Command-line decompression utility for Ultima 4 (PC version)
 *
 *  Copyright (C) 2002  Marc Winterrowd
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "lzw.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

long decompress_u4_file(FILE *in, long filesize, void **out);
long getFilesize(FILE *input_file);
unsigned char mightBeValidCompressedFile(FILE *compressed_file);

/*
 * Loads a file, decompresses it (from memory to memory), and writes the decompressed data to another file
 * Returns:
 * -1 if there was an error
 * the decompressed file length, on success
 */
long decompress_u4_file(FILE *in, long filesize, void **out)
{
    unsigned char *compressed_mem, *decompressed_mem;
    long compressed_filesize, decompressed_filesize;
    long errorCode;
    long i;

    /* size of the compressed input file */
    compressed_filesize = filesize;

    /* input file should be longer than 0 bytes */
    if (compressed_filesize == 0)
        return(-1);

    /* check if the input file is _not_ a valid LZW-compressed file */
    if (!mightBeValidCompressedFile(in))
        return(-1);

    /* load compressed file into compressed_mem[] */
    compressed_mem = (unsigned char *) malloc(compressed_filesize);
    fread(compressed_mem, 1, compressed_filesize, in);

    /*
     * determine decompressed file size
     * if lzw_get_decompressed_size() can't determine the decompressed size (i.e. the compressed
     * data is corrupt), it returns -1
     */
    decompressed_filesize = lzwGetDecompressedSize(compressed_mem,compressed_filesize);

    if (decompressed_filesize <= 0) {
        return(-1);
    }

    /* decompress file from compressed_mem[] into decompressed_mem[] */
    decompressed_mem = (unsigned char *) malloc(decompressed_filesize);

    /* testing: clear destination mem */
    for (i = 0; i < decompressed_filesize; i++) {
        decompressed_mem[i] = 0;
    }

    errorCode = lzwDecompress(compressed_mem, decompressed_mem, compressed_filesize);

    free(compressed_mem);

    *out = decompressed_mem;

    return(errorCode);
}

/*
 * Returns the size of a file, and moves the file pointer to the beginning.
 * The file must already be open when this function is called.
 */
long getFilesize(FILE *input_file)
{
    long file_length;
    fseek(input_file, 0, SEEK_END);   /* move file pointer to file end */
    file_length = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);   /* move file pointer to file start */
    return(file_length);
}

/*
 * If the input file is a valid LZW-compressed file, the upper 4 bits of
 * the first byte must be 0, because the first codeword is always a root.
 */
unsigned char mightBeValidCompressedFile(FILE *input_file)
{
    unsigned char firstByte;
    unsigned char c1, c2, c3;   /* booleans */
    long input_filesize;

    /*  check if the input file has a valid size             */
    /*  the compressed file is made up of 12-bit codewords,  */
    /*  so there are either 0 or 4 bits of wasted space      */
    input_filesize = getFilesize(input_file);
    c1 = (input_filesize * 8) % 12 == 0;
    c2 = (input_filesize * 8 - 4) % 12 == 0;

    /* read first byte */
    fseek(input_file, 0, SEEK_SET);   /* move file pointer to file start */
    fread(&firstByte, 1, 1, input_file);
    fseek(input_file, 0, SEEK_SET);   /* move file pointer to file start */
    c3 = (firstByte >> 4) == 0;

    /* check if upper 4 bits are 0 */
    return ((c1 || c2) && c3);
}
