#include <stdio.h>

#include "context.h"
#include "savegame.h"
#include "direction.h"

unsigned char dungeonGetTile(int x, int y) {
    static unsigned char map[5][5] = {
        { 0, 1, 1, 1, 0 },
        { 0, 0, 1, 0, 0 },
        { 1, 0, 0, 0, 1 },
        { 1, 1, 0, 1, 1 },
        { 1, 1, 0, 1, 1 }
    };
        
    if (x < 0)
        x += 5;
    if (y < 0)
        y += 5;
    if (x >= 5)
        x -= 5;
    if (y >= 5)
        y -= 5;

    return map[y][x];
}


unsigned char dungeonGetVisibleTile(int fwd, int side) {
    switch (c->saveGame->orientation) {
    case DIR_WEST:
        return dungeonGetTile(c->saveGame->x - fwd, c->saveGame->y - side);

    case DIR_NORTH:
        return dungeonGetTile(c->saveGame->x + side, c->saveGame->y - fwd);

    case DIR_EAST:
        return dungeonGetTile(c->saveGame->x + fwd, c->saveGame->y + side);

    case DIR_SOUTH:
        return dungeonGetTile(c->saveGame->x - side, c->saveGame->y + fwd);

    }
}

void dungeonCreateView() {
    int i, j;
    char view[5][3];

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 3; j++) {
            if (dungeonGetVisibleTile(i, j - 1)) {
                view[i][j] = 'X';
            } else
                view[i][j] = '.';
        }
    }

    view[0][1] = 'A';

    printf("%d, %d, %s\n", c->saveGame->x, c->saveGame->y, getDirectionName(c->saveGame->orientation));
    for (i = 4; i >= 0; i--)
        printf("%.3s\n", view[i]);
}

Context context;
SaveGame sg;
Context *c;

int main() {
    char buffer[100];

    c = &context;
    c->saveGame = &sg;
    
    c->saveGame->x = 0;
    c->saveGame->y = 0;
    c->saveGame->orientation = DIR_SOUTH;

    dungeonCreateView();

    while(fgets(buffer, sizeof(buffer), stdin)) {
        switch (buffer[0]) {
        case 'a':
            c->saveGame->orientation = dirRotateCCW(c->saveGame->orientation);
            break;
        case 'd':
            c->saveGame->orientation = dirRotateCW(c->saveGame->orientation);
            break;
        case 'w': {
            int x, y;
            x = c->saveGame->x;
            y = c->saveGame->y;
            
            dirMove(c->saveGame->orientation, &x, &y);
            if (x < 0)
                x += 5;
            if (y < 0)
                y += 5;
            if (x >= 5)
                x -= 5;
            if (y >= 5)
                y -= 5;

            c->saveGame->x = x;
            c->saveGame->y = y;

            break;
        }
        }
        
        dungeonCreateView();
    }
}
