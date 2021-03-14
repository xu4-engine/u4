#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
    char *x, *y;
    int n;

    if (argc != 3) {
        fprintf(stderr, "coord: a simple program to convert between numeric and sextant coordinates for Ultima IV\n");
        fprintf(stderr, "usage: coord x y\n");
        fprintf(stderr, "example: coord 34 56  (converts to sextant coordinates)\n");
        fprintf(stderr, "example: coord BE EF (converts to numeric coordinates)\n");
        exit(1);
    }

    x = argv[1];
    y = argv[2];

    if (isdigit(x[0])) {
        n = strtoul(x, NULL, 0);
        printf("%c'%c\"\n", n / 16 + 'A', n % 16 + 'A');
        n = strtoul(y, NULL, 0);
        printf("%c'%c\"\n", n / 16 + 'A', n % 16 + 'A');
    } else {
        n = (toupper(x[0]) - 'A') * 16;
        n += toupper(x[1]) - 'A';
        printf("%d, ", n);
        n = (toupper(y[0]) - 'A') * 16;
        n += toupper(y[1]) - 'A';
        printf("%d\n", n);
    }

    return 0;
}
