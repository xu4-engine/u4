/*
 * $Id$
 */

#ifndef DIALOGUELOADER_H
#define DIALOGUELOADER_H

#include <string>

#include "conversation.h"

/**
 * The generic dialogue loader interface.  Different dialogue
 * loaders should override the load method to load dialogues from
 * different sources (.tlk files, xml config elements, etc.).  They
 * must also register themselves with registerLoader for one or more
 * source types.  By convention, the source type of load() and 
 * registerLoader() is an xu4-specific mime type.
 * The two main types used are application/x-u4tlk and text/x-u4cfg.
 */
class DialogueLoader {
public:    
    static DialogueLoader *getLoader(const std::string &mimeType);
    virtual Dialogue *load(void *source) = 0;

protected:
    static DialogueLoader *registerLoader(DialogueLoader *loader, const std::string &mimeType);    

private:
    static std::map<std::string, DialogueLoader *> *loaderMap;
};

#endif
