/*
 * $Id$
 */

#ifndef COORDS_H
#define COORDS_H

/**
 * A simple representation of a point in 3D space.
 */
class Coords {
public:
    int x, y, z;

    Coords(int initx = 0, int inity = 0, int initz = 0) : x(initx), y(inity), z(initz) {}

    bool operator==(const Coords &a) const { return x == a.x && y == a.y && z == a.z; }
    bool operator!=(const Coords &a) const { return !operator==(a); }
};

#endif /* COORDS_H */
