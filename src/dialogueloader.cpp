/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "debug.h"
#include "dialogueloader.h"

std::map<std::string, DialogueLoader *> *DialogueLoader::loaderMap = NULL;

DialogueLoader *DialogueLoader::getLoader(const std::string &mimeType) {
    ASSERT(loaderMap != NULL, "loaderMap not initialized");
    if (loaderMap->find(mimeType) == loaderMap->end())
        return NULL;
    return (*loaderMap)[mimeType];
}

DialogueLoader *DialogueLoader::registerLoader(DialogueLoader *loader, const std::string &mimeType) {
    if (loaderMap == NULL) {
        loaderMap = new std::map<std::string, DialogueLoader *>;
    }
    (*loaderMap)[mimeType] = loader;
    return loader;
}
