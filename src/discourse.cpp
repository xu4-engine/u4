/*
 * discourse.cpp
 */

#include <algorithm>
#include <cstring>
#include <string>

#include "context.h"
#include "conversation.h"
#include "discourse.h"
#include "event.h"
#include "u4file.h"

#ifdef CONF_MODULE
#include "config.h"
#include "xu4.h"
#endif

#include "discourse_tlk.cpp"

void discourse_init(Discourse* dis)
{
    memset(dis, 0, sizeof(Discourse));
}

extern Dialogue* U4LordBritish_load();
extern Dialogue* U4Hawkwind_load();
extern Dialogue* U4Tlk_load(U4FILE* file);
extern void talkRunConversation(Person*, const Dialogue*);

// These are ordered to match the VENDOR PersonNpcType.
static const char* vendorGoods[] = {
    "weapons", "armor", "food", "tavern", "reagents",
    "healer", "inn", "guild", "stable"
};

/*
 * Return error message or NULL if successful.
 */
const char* discourse_load(Discourse* dis, const char* resource)
{
#ifdef CONF_MODULE
    if (resource[0] == 'N' && strlen(resource) == 4) {
        const uint8_t* ub = (const uint8_t*) resource;
        dis->conv.id = CDI32(ub[0], ub[1], ub[2], ub[3]);

        int32_t n = xu4.config->npcTalk(dis->conv.id);
        if (! n)
            return "Unable to load NPC talk chunk";

        dis->system = DISCOURSE_XU4_TALK;
        //UThread* ut = xu4.config->boronThread();
        //dis->convCount = ur_buffer(n)->used / 5;
    } else
#endif
    if (strcmp(resource, "vendors") == 0) {
        dis->system = DISCOURSE_VENDOR;
        dis->conv.table = vendorGoods;
        dis->convCount = sizeof(vendorGoods) / sizeof(char*);
    }
    else if (strcmp(resource, "castle") == 0) {
        dis->system = DISCOURSE_CONV;
        dis->conv.table = calloc(2, sizeof(void*));
        dis->convCount = 2;

        Dialogue** dtable = (Dialogue**) dis->conv.table;
        dtable[0] = U4LordBritish_load();
        dtable[1] = U4Hawkwind_load();
    } else {
        // Ultima 4 .TLK files only have 16 conversations.
        const int convLimit = 16;
        int i;
        U4FILE* fh = u4fopen(resource);
        if (! fh)
            return "Unable to open .TLK file";

#if 1
        dis->system = DISCOURSE_U4_TLK;
        dis->conv.table = malloc(convLimit * (sizeof(U4Talk) + 288));
        U4Talk* tlk = (U4Talk*) dis->conv.table;
        char* strings = (char*) (tlk + convLimit);

        for (i = 0; i < convLimit; ++tlk, ++i) {
            if (! U4Talk_load(fh, tlk, strings))
                break;
            strings += 288;
        }
#else
        dis->system = DISCOURSE_CONV;
        dis->conv.table = calloc(convLimit, sizeof(void*));
        Dialogue** dtable = (Dialogue**) dis->conv.table;

        for (i = 0; i < convLimit; i++) {
            dtable[i] = U4Tlk_load(fh);
            if (! dtable[i])
                break;
        }
#endif
        dis->convCount = i;

        u4fclose(fh);
    }

    return NULL;
}

void discourse_free(Discourse* dis)
{
    switch (dis->system) {
        case DISCOURSE_CONV:
        {
            Dialogue** dtable = (Dialogue**) dis->conv.table;
            int i;
            for (i = 0; i < dis->convCount; i++)
                delete dtable[i];
            free(dis->conv.table);
        }
            break;

        case DISCOURSE_U4_TLK:
            free(dis->conv.table);
            break;
    }

    dis->system = DISCOURSE_UNSET;
    dis->conv.table = NULL;
    dis->convCount = 0;
}

extern void talkRunU4Tlk(const Discourse*, int, Person*);

bool discourse_run(const Discourse* dis, uint16_t entry, Person* npc)
{
    if (entry >= dis->convCount)
        return false;

    switch (dis->system) {
    case DISCOURSE_CONV:
        talkRunConversation(npc, ((const Dialogue**) dis->conv.table)[entry]);
        return true;

    case DISCOURSE_U4_TLK:
        talkRunU4Tlk(dis, entry, npc);
        return true;

#ifdef CONF_MODULE
    case DISCOURSE_XU4_TALK:
    {
        int32_t blkN = xu4.config->npcTalk(dis->conv.id);
        if (blkN) {
            talkRunBoron(dis, entry, npc);
            return true;
        }
    }
        return false;
#endif

    case DISCOURSE_VENDOR:
    {
        const char** goods = (const char**) dis->conv.table;
        Controller noTurns;

        xu4.eventHandler->pushController(&noTurns);

#ifdef USE_BORON
        // Make a valid Boron word! from names with spaces.
        string word(c->location->map->getName());
        replace(word.begin(), word.end(), ' ', '-');

        xu4.config->scriptEvalArg("talk-to %s '%s", goods[entry], word.c_str());
#else
        // Load and run the appropriate script.
        string ugood(goods[entry]);
        ugood[0] = toupper(ugood[0]);   // Script baseId is case sensitive.

        Script* script = new Script();
        script->talkToVendor(c->location->map->getName(), ugood);
        script->unload();
        delete script;
#endif

        // Pause follow.
        if (npc->movement == MOVEMENT_FOLLOW_AVATAR)
            npc->movement = MOVEMENT_FOLLOW_PAUSE;

        xu4.eventHandler->popController();
    }
        return true;
    }
    return false;
}

/*
 * Return the conversation index for a character or -1 if the name is not found.
 */
int discourse_findName(const Discourse* dis, const char* name)
{
    int i;
    if (dis->system == DISCOURSE_U4_TLK) {
        const U4Talk* tlk = (const U4Talk*) dis->conv.table;
        for (i = 0; i < dis->convCount; ++i) {
            if (strcmp(tlk->strings + tlk->name, name) == 0)
                return i;
            ++tlk;
        }
    } else if (dis->system == DISCOURSE_CONV) {
        string str(name);
        Dialogue** dtable = (Dialogue**) dis->conv.table;
        for (i = 0; i < dis->convCount; ++i) {
            if (dtable[i] && dtable[i]->getName() == str)
                return i;
        }
    }
    return -1;
}
