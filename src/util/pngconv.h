/*
 * $Id$
 */

#ifndef PNGCONV_H
#define PNGCONV_H

int writePngFromEga(unsigned char *data, const char *fname);
int readEgaFromPng(unsigned char **data, const char *fname);

#endif
