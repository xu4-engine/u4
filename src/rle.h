/*
 * $Id$
 */

#ifndef RLE_H
#define RLE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RLE_RUNSTART 02

long rleDecompressFile(FILE *in, long inlen, void **out);
long rleDecompressMemory(void *in, long inlen, void **out);
long rleGetDecompressedSize(unsigned char *indata, long inlen);
long rleDecompress(unsigned char *indata, long inlen, unsigned char *outdata, long outlen);

#ifdef __cplusplus
}
#endif

#endif
