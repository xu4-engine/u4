#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#define EGA_WIDTH 320
#define EGA_HEIGHT 200

int writePngFromEga(unsigned char *data, const char *fname) {
    FILE *fp;
    unsigned char *p;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_color palette[16];
    png_byte *row_pointers[200];
    int i, j;

    for (i = 0; i < EGA_HEIGHT; i++) {
        row_pointers[i] = (png_byte *) malloc(EGA_WIDTH * sizeof (png_byte));
    }

    p = data;
    for (i = 0; i < EGA_HEIGHT; i++) {
        for (j = 0; j < EGA_WIDTH / 2; j++) {
            row_pointers[i][j] = *p++;
        }
    }

    #define setpalentry(i, r, g, b) \
        palette[i].red = r; \
        palette[i].green = g; \
        palette[i].blue = b;
    setpalentry(0, 0x00, 0x00, 0x00);
    setpalentry(1, 0x00, 0x00, 0x80);
    setpalentry(2, 0x00, 0x80, 0x00);
    setpalentry(3, 0x00, 0x80, 0x80);
    setpalentry(4, 0x80, 0x00, 0x00);
    setpalentry(5, 0x80, 0x00, 0x80);
    setpalentry(6, 0x80, 0x80, 0x00);
    setpalentry(7, 0xc3, 0xc3, 0xc3);
    setpalentry(8, 0xa0, 0xa0, 0xa0);
    setpalentry(9, 0x00, 0x00, 0xFF);
    setpalentry(10, 0x00, 0xFF, 0x00);
    setpalentry(11, 0x00, 0xFF, 0xFF);
    setpalentry(12, 0xFF, 0x00, 0x00);
    setpalentry(13, 0xFF, 0x00, 0xFF);
    setpalentry(14, 0xFF, 0xFF, 0x00);
    setpalentry(15, 0xFF, 0xFF, 0xFF);
    #undef setpalentry

    fp = fopen(fname, "wb");
    if (!fp) {
        perror(fname);
        exit(1);
    }
    
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_write_struct error\n");
        exit(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        fprintf(stderr, "longjump error\n");
        exit(1);
    }

    png_init_io(png_ptr, fp);

    width = EGA_WIDTH;
    height = EGA_HEIGHT;
    bit_depth = 4;
    color_type = PNG_COLOR_TYPE_PALETTE;
    interlace_type = PNG_INTERLACE_NONE;
    compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
    filter_method = PNG_FILTER_TYPE_DEFAULT;
    png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, interlace_type, compression_type, filter_method);

    png_set_PLTE(png_ptr, info_ptr, palette, 16);
    png_set_rows(png_ptr, info_ptr, row_pointers);

    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    fclose(fp);

    return 0;
}

int readEgaFromPng(unsigned char **data, const char *fname) {
    FILE *fp;
    unsigned char *p;
    char header[8];
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;
    png_byte **row_pointers;
    int i, j;

    fp = fopen(fname, "rb");
    if (!fp) {
        perror(fname);
        exit(1);
    }
    fread(header, 1, sizeof(header), fp);
    if (png_sig_cmp(header, 0, sizeof(header)) != 0) {
        fprintf(stderr, "not a PNG\n");
        exit(1);
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fprintf(stderr, "png_create_read_struct error\n");
        exit(1);
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }

    end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fprintf(stderr, "png_create_info_struct error\n");
        exit(1);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        fprintf(stderr, "longjump error\n");
        exit(1);
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sizeof(header));

    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
    
    png_get_IHDR(png_ptr, info_ptr, &width, &height,
       &bit_depth, &color_type, &interlace_type,
       &compression_type, &filter_method);

    if (width != EGA_WIDTH ||
        height != EGA_HEIGHT ||
        bit_depth != 4) {
        fprintf(stderr, "PNG must be %dx%d with 4 bits per pixel\n", EGA_WIDTH, EGA_HEIGHT);
        exit(1);
    }

    row_pointers = png_get_rows(png_ptr, info_ptr);

    *data = (unsigned char *) malloc(EGA_WIDTH * (EGA_HEIGHT / 2));

    p = *data;
    for (i = 0; i < EGA_HEIGHT; i++) {
        for (j = 0; j < EGA_WIDTH / 2; j++) {
            *p++ = row_pointers[i][j];
        }
    }

    fclose(fp);

    return 0;
}
