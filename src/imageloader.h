/*
 * $Id$
 */

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <map>
#include <string>

class Image;
class U4FILE;

/**
 * The generic image loader interface.  Image loaders should override
 * the load method to load an image from a U4FILE and register
 * themselves with registerLoader.  By convention, the type parameter
 * of load and registerLoader is the standard mime type of the image
 * file (e.g. image/png) or an xu4 specific mime type
 * (e.g. image/x-u4...).
 */
class ImageLoader {
public:
    ImageLoader();
    void setDimensions(int width, int height, int bpp);
    virtual Image *load(U4FILE *file) = 0;
    static ImageLoader *getLoader(const std::string &fileType);

protected:
    int width, height, bpp;
    static ImageLoader *registerLoader(ImageLoader *loader, const std::string &type);
    static void setFromRawData(Image *image, int width, int height, int bpp, unsigned char *rawData);

private:
    static std::map<std::string, ImageLoader *> *loaderMap;
};

#endif /* IMAGELOADER_H */
