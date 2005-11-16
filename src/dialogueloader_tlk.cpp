/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>

#include "conversation.h"
#include "dialogueloader_tlk.h"
#include "u4file.h"

using std::string;

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
    string name(ptr);    ptr += strlen(ptr) + 1;
    string pronoun(ptr); ptr += strlen(ptr) + 1;
    string desc(ptr);    ptr += strlen(ptr) + 1;

    job = new Dialogue::Keyword("job", string("\n") + ptr);        ptr += strlen(ptr) + 1;
    health = new Dialogue::Keyword("health", string("\n") + ptr);  ptr += strlen(ptr) + 1;
    
    string resp1(ptr);   ptr += strlen(ptr) + 1;
    string resp2(ptr);   ptr += strlen(ptr) + 1;
    string q(ptr);       ptr += strlen(ptr) + 1;
    string y(ptr);       ptr += strlen(ptr) + 1;
    string n(ptr);       ptr += strlen(ptr) + 1;

    question = new Dialogue::Question(q, y, n, qtype);

    kw1 = new Dialogue::Keyword(ptr, string("\n") + resp1);        ptr += strlen(ptr) + 1;
    kw2 = new Dialogue::Keyword(ptr, string("\n") + resp2);
    
    switch(qtrigger) {
    case JOB:       job->setQuestion(question); break;
    case HEALTH:    health->setQuestion(question); break;
    case KEYWORD1:  kw1->setQuestion(question); break;
    case KEYWORD2:  kw2->setQuestion(question); break;
    case NONE:
    default:
        break;
    }

    dlg->setName(name);
    dlg->setPronoun(pronoun);
    dlg->setIntro(string("\nYou meet ") + desc + "\n");
    dlg->setLongIntro(dlg->getIntro() + 
                      "\n" + pronoun + " says: I am " + name + "\n");
    dlg->setDefaultAnswer("That I cannot\nhelp thee with.");

    // NOTE: We let the talker's custom keywords override the standard
    // keywords like HEAL and LOOK.  This behavior differs from u4dos,
    // but fixes a couple conversation files which have keywords that
    // conflict with the standard ones (e.g. Calabrini in Moonglow has
    // HEAL for healer, which is unreachable in u4dos, but clearly
    // more useful than "Fine." for health).
    
    string look = string("\nYou see ") + desc;
    if (isupper(look[8]))
        look[8] = tolower(look[8]);
    dlg->addKeyword(new Dialogue::Keyword("look", look));
    dlg->addKeyword(new Dialogue::Keyword("name", string("\n") + pronoun + " says: I am " + name));
    dlg->addKeyword(new Dialogue::Keyword("give", string("\n") + pronoun + " says: I do not need thy gold.  Keep it!"));
    dlg->addKeyword(new Dialogue::Keyword("join", string("\n") + pronoun + " says: I cannot join thee."));

    /* 
     * This little easter egg appeared in the Amiga version of Ultima IV.
     * I've never figured out what the number means.
     * "Banjo" Bob Hardy was the programmer for the Amiga version.
     */
    dlg->addKeyword(new Dialogue::Keyword("ojna", "\nHi Banjo Bob!\nYour secret\nnumber is\n4F4A4E0A"));

    dlg->addKeyword(job);
    dlg->addKeyword(health);
    dlg->addKeyword(kw1);
    dlg->addKeyword(kw2);

    return dlg;
}
