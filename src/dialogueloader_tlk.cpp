/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "conversation.h"
#include "dialogueloader_tlk.h"
#include "u4file.h"

DialogueLoader* U4TlkDialogueLoader::instance = DialogueLoader::registerLoader(new U4TlkDialogueLoader, "application/x-u4tlk");

/**
 * A dialogue loader for standard u4dos .tlk files.
 */
Dialogue* U4TlkDialogueLoader::load(void *source) {
    U4FILE *file = static_cast<U4FILE*>(source);
    char tlk_buffer[288];
    Dialogue *dlg;
    unsigned char prob;
    char *ptr;
    Dialogue::Question *question = NULL;
    Dialogue::Keyword *job, *health, *kw1, *kw2;
    string resp1, resp2, q, y, n;
    
    enum QTrigger {
        NONE = 0,
        JOB = 3,
        HEALTH = 4,
        KEYWORD1 = 5,
        KEYWORD2 = 6
    };
    
    QTrigger qtrigger;
    Dialogue::Question::Type qtype;
    
    /* there's no dialogues left in the file */
    if (u4fread(tlk_buffer, 1, sizeof(tlk_buffer), file) != sizeof(tlk_buffer))
        return NULL;
    
    dlg = new Dialogue();
    prob = tlk_buffer[2];
    qtrigger = QTrigger(tlk_buffer[0]);
    qtype = Dialogue::Question::Type(tlk_buffer[1]);

    dlg->setTurnAwayProb(prob);

    // Get the name, pronoun, and description of the talker
    ptr = &tlk_buffer[3];    
    dlg->setName(ptr);      ptr += strlen(ptr) + 1;
    dlg->setPronoun(ptr);   ptr += strlen(ptr) + 1;
    dlg->setDesc(ptr);      ptr += strlen(ptr) + 1;
    
    // Form keywords and questions with the remaining dialogue text.
    job = new Dialogue::Keyword("job", ptr);                    ptr += strlen(ptr) + 1;
    health = new Dialogue::Keyword("health", ptr);              ptr += strlen(ptr) + 1;
    
    resp1 = ptr;    ptr += strlen(ptr) + 1;
    resp2 = ptr;    ptr += strlen(ptr) + 1;
    q = ptr;        ptr += strlen(ptr) + 1;
    y = ptr;        ptr += strlen(ptr) + 1;
    n = ptr;        ptr += strlen(ptr) + 1;
    question = new Dialogue::Question(q, y, n, qtype);

    kw1 = new Dialogue::Keyword(ptr, resp1);                    ptr += strlen(ptr) + 1;
    kw2 = new Dialogue::Keyword(ptr, resp2);
    
    switch(qtrigger) {
    case JOB:       job->setQuestion(question); break;
    case HEALTH:    health->setQuestion(question); break;
    case KEYWORD1:  kw1->setQuestion(question); break;
    case KEYWORD2:  kw2->setQuestion(question); break;
    case NONE:
    default:
        break;
    }

    dlg->addKeyword(job);
    dlg->addKeyword(health);
    dlg->addKeyword(kw1);
    dlg->addKeyword(kw2);

    return dlg;
}
