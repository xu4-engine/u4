/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <vector>

#include "conversation.h"
#include "dialogueloader_lb.h"
#include "u4file.h"

using std::string;
using std::vector;

DialogueLoader* U4LBDialogueLoader::instance = DialogueLoader::registerLoader(new U4LBDialogueLoader, "application/x-u4lbtlk");

/**
 * A special case dialogue loader for Lord British.
 */
Dialogue* U4LBDialogueLoader::load(void *source) {
    U4FILE *avatar = u4fopen("avatar.exe");
    if (!avatar)
        return NULL;
    
    vector<string> lbKeywords = u4read_stringtable(avatar, 87581, 24);
    /* There's a \0 in the 19th string so we get a
       spurious 20th entry */
    vector<string> lbText = u4read_stringtable(avatar, 87754, 25);
    for (int i = 20; i < 24; i++)
        lbText[i] = lbText[i+1];
    lbText.pop_back();

    Dialogue *dlg = new Dialogue();
    dlg->setTurnAwayProb(0);

    dlg->setName("Lord British");
    dlg->setPronoun("He");
    dlg->setIntro("");
    dlg->setLongIntro("");
    dlg->setDefaultAnswer("\nHe says: I\ncannot help thee\nwith that.\n");

    for (unsigned i = 0; i < lbKeywords.size(); i++) {
        dlg->addKeyword(new Dialogue::Keyword(lbKeywords[i], lbText[i]));
    }

    /* since the original game files are a bit sketchy on the 'abyss' keyword,
       let's handle it here just to be safe :) */
    dlg->addKeyword(new Dialogue::Keyword("abys", 
                                          "\n\n\n\n\nHe says:\nThe Great Stygian Abyss is the darkest pocket of evil "
                                          "remaining in Britannia!\n\n\n\n\nIt is said that in the deepest recesses of "
                                          "the Abyss is the Chamber of the Codex!\n\n\n\nIt is also said that only one "
                                          "of highest Virtue may enter this Chamber, one such as an Avatar!!!\n"));

    return dlg;
}
