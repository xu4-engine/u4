#ifndef SCALE_H
#define SCALE_H

class Image;

Image *scaleUp(Image *src, int scale, int n, int filter);
Image *scaleDown(Image *src, int scale);

#endif /* SCALE_H */
