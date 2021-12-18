/*
 * $Id$
 */

#ifndef SOUND_H
#define SOUND_H

enum Sound {
    SOUND_TITLE_FADE,       // the intro title fade
    SOUND_WALK_NORMAL,      // walk, world and town
    SOUND_WALK_SLOWED,      // walk, slow progress
    SOUND_WALK_COMBAT,      // walk, combat
    SOUND_BLOCKED,          // location blocked
    SOUND_ERROR,            // error/bad command
    SOUND_PC_ATTACK,        // PC attacks
    SOUND_PC_STRUCK,        // PC damaged
    SOUND_NPC_ATTACK,       // NPC attacks
    SOUND_NPC_STRUCK,       // NPC damaged
    SOUND_ACID,             // effect, acid damage
    SOUND_SLEEP,            // effect, sleep
    SOUND_POISON_EFFECT,    // effect, poison
    SOUND_POISON_DAMAGE,    // damage, poison
    SOUND_EVADE,            // trap evaded
    SOUND_FLEE,             // flee combat
    SOUND_ITEM_STOLEN,      // item was stolen from a PC, food or gold
    SOUND_LBHEAL,           // LB heals party
    SOUND_LEVELUP,          // PC level up
    SOUND_MOONGATE,         // moongate used

    SOUND_CANNON,
    SOUND_RUMBLE,
    SOUND_PREMAGIC_MANA_JUMBLE,
    SOUND_MAGIC,
    SOUND_WHIRLPOOL,
    SOUND_STORM,
    SOUND_GATE_OPEN,        // Intro moongate opening

    //    SOUND_MISSED,
    //    SOUND_CREATUREATTACK,
    //    SOUND_PLAYERHIT,
    SOUND_MAX
};

enum MusicTrack {
    MUSIC_NONE,
    MUSIC_OUTSIDE,
    MUSIC_TOWNS,
    MUSIC_SHRINES,
    MUSIC_SHOPPING,
    MUSIC_RULEBRIT,
    MUSIC_FANFARE,
    MUSIC_DUNGEON,
    MUSIC_COMBAT,
    MUSIC_CASTLES,

    MUSIC_MAX
};

int soundInit(void);
void soundDelete(void);

void soundPlay(Sound sound, bool onlyOnce = true, int specificDurationInTicks = -1);

void soundStop();
void soundSetVolume(int);
int  soundVolumeDec();
int  soundVolumeInc();

void musicPlay(int);
void musicPlayLocale();
void musicStop();
void musicFadeOut(int);
void musicFadeIn(int, bool);
void musicSetVolume(int);
int  musicVolumeDec();
int  musicVolumeInc();
bool musicToggle();

#endif /* SOUND_H */
