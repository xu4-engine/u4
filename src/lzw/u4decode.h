#ifndef U4DECODE_H
#define U4DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

long decompress_u4_file(FILE *in, long filesize, void **out);
long getFilesize(FILE *input_file);
unsigned char mightBeValidCompressedFile(FILE *compressed_file);
long decompress_u4_memory(void *in, long inlen, void **out);

#ifdef __cplusplus
}
#endif

#endif
