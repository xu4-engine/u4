/*
 * $Id$
 */

#ifndef DIALOGUELOADER_HW_H
#define DIALOGUELOADER_HW_H

#include "dialogueloader.h"

/**
 * The dialogue loader for Hawkwind.
 */
class U4HWDialogueLoader : public DialogueLoader {
    static DialogueLoader *instance;

public:
    virtual Dialogue *load(void *source);
};

#endif
