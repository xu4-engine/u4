/*
 * $Id$
 */

#ifndef PNGCONV_H
#define PNGCONV_H

int writePngFromEga(unsigned char *data, int height, int width, int bits, const char *fname);
int readEgaFromPng(unsigned char **data, int *height, int *width, int *bits, const char *fname);

#endif
