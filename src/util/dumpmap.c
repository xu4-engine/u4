// Dump WORLD.MAP
// gcc -o dumpmap dumpmap.c

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main( int argc, char** argv )
{
    const int chunkDim = 32;
    const size_t chunkLen = chunkDim * chunkDim;
    uint16_t hist[256];
    int x, y, i, j;
    size_t n;
    FILE* fp;
    uint8_t* buf;
    uint8_t* cp;

    fp = fopen( argv[1], "rb" );
    if( ! fp ) {
        printf( "Cannot open %s\n", argv[1] );
        return 1;
    }

    memset(hist, 0, sizeof hist);

    buf = (uint8_t*) malloc( chunkLen );
    for( y = 0; y < 8; ++y ) {
        for( x = 0; x < 8; ++x ) {
            printf( "chunk %d,%d\n", x, y );

            n = fread( buf, 1, chunkLen, fp );
            if( n != chunkLen ) {
                printf( "Read error\n" );
                goto cleanup;
            }

            cp = buf;
            for( j = 0; j < chunkDim; ++j ) {
                for( i = 0; i < chunkDim/4; ++i ) {
                    printf("%2x%2x%2x%2x", cp[0], cp[1], cp[2], cp[3]);

                    hist[cp[0]] += 1;
                    hist[cp[1]] += 1;
                    hist[cp[2]] += 1;
                    hist[cp[3]] += 1;

                    cp += 4;
                }
                printf( "\n" );
            }
        }
    }

    printf( "histogram:\n" );
    for( i = 0; i < 256; ++i ) {
        if( hist[i] )
            printf( "  %02x (%2d) %5d\n", i, i, hist[i] );
    }

cleanup:
    free( buf );
    fclose( fp );
    return 0;
}
