/*
 * $Id$
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <libxml/xmlmemory.h>

#include "sound.h"
#include "error.h"
#include "settings.h"
#include "u4file.h"
#include "xml.h"

char *soundFilenames[SOUND_MAX];
Mix_Chunk *soundChunk[SOUND_MAX];

int soundInit() {
    Sound soundTrack;
    xmlDocPtr doc;
    xmlNodePtr root, node;

    /*
     * load sound track filenames from xml config file
     */

    doc = xmlParse("sound.xml");
    root = xmlDocGetRootElement(doc);
    if (xmlStrcmp(root->name, (const xmlChar *) "sound") != 0)
        errorFatal("malformed sound.xml");

    soundTrack = 0;
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (soundTrack >= SOUND_MAX)
            break;

        if (xmlNodeIsText(node) ||
            xmlStrcmp(node->name, (const xmlChar *) "track") != 0)
            continue;

        soundFilenames[soundTrack] = (char *)xmlGetProp(node, (const xmlChar *) "file");
        soundTrack++;
    }
    xmlFreeDoc(doc);

    return 1;
}

void soundDelete() {
}

void soundPlay(Sound sound) {
    char *pathname;

    assert(sound < SOUND_MAX);

    /* FIXME: control music seperate from sounds
    if (!settings->vol)
      return;*/

    if (soundChunk[sound] == NULL) {
        pathname = u4find_sound(soundFilenames[sound]);
        if (pathname) {
            soundChunk[sound] = Mix_LoadWAV(pathname);
            free(pathname);
            if (!soundChunk[sound]) {
                errorWarning("unable to load sound effect file %s: %s", soundFilenames[sound], Mix_GetError());
                return;
            }
        }
    }
    if (Mix_PlayChannel(-1, soundChunk[sound], 0) == -1)
        errorWarning("error playing sound: %s", Mix_GetError());
}
