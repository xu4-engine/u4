#define MASK_WALKABLE 0x03
#define MASK_POISON   0x04
#define MASK_SAILABLE 0x08
#define MASK_FLYABLE  0x10
#define MASK_OPAQUE   0x20
#define MASK_LAVA     0x40

#define WALKABLE 0x00
#define SLOW 0x01
#define VSLOW 0x02
#define UNWALKABLE 0x03

/* tile values 0-31 */
static const char _ttype_info1[] = {
/* sea,  water, shallows, swamp, grass, brush, forest, hills, mountain, */
   0x0b, 0x0b,  0x0b,     0x00,  0x00,  0x01,  0x02,   0x02,  0x03,
/* dungeon, city, castle, town, lcb1, lcb2, lcb3, wship, nship, eship, sship */
   0x00,    0x00, 0x00,   0x00, 0x03, 0x00, 0x03, 0x00,  0x00,  0x00,  0x00,
/* whorse, ehorse, hex,  bridge, baloon, nbridge, sbridge, uladder, dladder */
   0x00,   0x00,   0x00, 0x00,   0x00,   0x00,    0x00,    0x00,    0x00,
/* ruins, shrine, avatar */
   0x00,  0x00,   0x03
};

/* tile values 56-79 */
static const char _ttype_info2[] = {
/* corpse, stonewall, ldoor, door, chest, anhk, brick, wood */
   0x00,   0x03,      0x03,  0x03, 0x00,  0x03, 0x00,  0x03,
/* mgate0, mgate1, mgate2, mgate3, field0, field1, field2, field3 */
   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
/* solid, sdoor, altar, roast, lava, stone, orb1, orb2 */
   0x03,  0x00,  0x00,  0x03,  0x40, 0x00,  0x00, 0x00
};


int iswalkable(unsigned char tile) {
    if (tile < 32)
	return (_ttype_info1[tile] & MASK_WALKABLE) != 0x03;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) != 0x03;
    return 0;
}

int isslow(unsigned char tile) {
    if (tile < sizeof(_ttype_info1))
	return (_ttype_info1[tile] & MASK_WALKABLE) == 0x01;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) == 0x01;
    return 0;
}

int isvslow(unsigned char tile) {
    if (tile < sizeof(_ttype_info1))
	return (_ttype_info1[tile] & MASK_WALKABLE) == 0x02;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_WALKABLE) == 0x02;
    return 0;
}

int issailable(unsigned char tile) {
    if (tile < 32)
	return (_ttype_info1[tile] & MASK_SAILABLE) != 0;
    else if (tile < 56)
	return 0;
    else if (tile < 79)
	return (_ttype_info2[tile - 56] & MASK_SAILABLE) != 0;
    return 0;
}

int isdoor(unsigned char tile) {
    return tile == 59;
}

int islockeddoor(unsigned char tile) {
    return tile == 58;
}

int canTalkOver(unsigned char tile) {
    return tile >= 96 && tile <= 122;
}
