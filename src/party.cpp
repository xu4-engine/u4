/*
 * $Id$
 */

#include <cstring>

#include "party.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "mapmgr.h"
#include "sound.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "weapon.h"
#include "xu4.h"

#define strequ(A, B)    (strcmp(A, B) == 0)

bool isPartyMember(const Object *punknown) {
    const PartyMember *pm;
    if ((pm = dynamic_cast<const PartyMember*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * PartyMember class implementation
 */
PartyMember::PartyMember(Party *p, SaveGamePlayerRecord *pr) :
    Creature(Creature::getByTile( tileForClass(pr->klass) )),
    player(pr),
    party(p)
{
    // NOTE: Avoid calls which emit notifications during construction!

    /* FIXME: we need to rename movement behaviors */
    movement = MOVEMENT_ATTACK_AVATAR;
    this->ranged = xu4.config->weapon(pr->weapon)->range ? 1 : 0;

    // These lines are the same as setStatus -> PartyMember::addStatus but
    // without notifyOfChange().
    Creature::addStatus(pr->status);
    player->status = pr->status;
}

PartyMember::~PartyMember() {
    // Need to check context as IntroController creates PartyMember objects.
    if (c) {
        removeFromMaps();
    }
}

/**
 * Notify the party that this player has changed somehow
 */
void PartyMember::notifyOfChange() {
    if (party) {
        party->notifyOfChange(this);
    }
}

int PartyMember::getHp() const      { return player->hp; }

/**
 * Determine the most magic points a character could have
 * given his class and intelligence.
 */
int PartyMember::getMaxMp() const {
    int max_mp = -1;

    switch (player->klass) {
    case CLASS_MAGE:            /*  mage: 200% of int */
        max_mp = player->intel * 2;
        break;

    case CLASS_DRUID:           /* druid: 150% of int */
        max_mp = player->intel * 3 / 2;
        break;

    case CLASS_BARD:            /* bard, paladin, ranger: 100% of int */
    case CLASS_PALADIN:
    case CLASS_RANGER:
        max_mp = player->intel;
        break;

    case CLASS_TINKER:          /* tinker: 50% of int */
        max_mp = player->intel / 2;
        break;

    case CLASS_FIGHTER:         /* fighter, shepherd: no mp at all */
    case CLASS_SHEPHERD:
        max_mp = 0;
        break;

    default:
        ASSERT(0, "invalid player class: %d", player->klass);
    }

    /* mp always maxes out at 99 */
    if (max_mp > 99)
        max_mp = 99;

    return max_mp;
}

const Weapon *PartyMember::getWeapon() const {
    return xu4.config->weapon(player->weapon);
}

const Armor *PartyMember::getArmor() const {
    return xu4.config->armor(player->armor);
}

const char* PartyMember::getName() const     { return player->name; }
SexType PartyMember::getSex() const          { return player->sex; }
ClassType PartyMember::getClass() const      { return player->klass; }

CreatureStatus PartyMember::getState() const {
    if (getHp() <= 0)
        return MSTAT_DEAD;
    else if (getHp() < 24)
        return MSTAT_FLEEING;
    else
        return MSTAT_BARELYWOUNDED;
}

/**
 * Determine what level a character has.
 */
int PartyMember::getRealLevel() const {
    return player->hpMax / 100;
}

/**
 * Determine the highest level a character could have with the number
 * of experience points he has.
 */
int PartyMember::getMaxLevel() const {
    int level = 1;
    int next = 100;

    while (player->xp >= next && level < 8) {
        level++;
        next <<= 1;
    }

    return level;
}

/**
 * Adds a status effect to the player
 */
void PartyMember::addStatus(StatusType s) {
    Creature::addStatus(s);
    player->status = getStatus();
    notifyOfChange();
}

/**
 * Adjusts the player's mp by 'pts'
 */
void PartyMember::adjustMp(int pts) {
    AdjustValueMax(player->mp, pts, getMaxMp());
    notifyOfChange();
}

/**
 * Advances the player to the next level if they have enough experience
 */
void PartyMember::advanceLevel() {
    if (getRealLevel() == getMaxLevel())
        return;
    setStatus(STAT_GOOD);
    player->hpMax = getMaxLevel() * 100;
    player->hp = player->hpMax;

    /* improve stats by 1-8 each */
    player->str   += xu4_random(8) + 1;
    player->dex   += xu4_random(8) + 1;
    player->intel += xu4_random(8) + 1;

    if (player->str > 50) player->str = 50;
    if (player->dex > 50) player->dex = 50;
    if (player->intel > 50) player->intel = 50;

    if (party) {
        PartyEvent event(PartyEvent::ADVANCED_LEVEL, this);
        event.player = this;
        gs_emitMessage(SENDER_PARTY, &event);
    }
}

/**
 * Award a player experience points.  Maxs out the players xp at 9999.
 */
void PartyMember::awardXp(int xp) {
    AdjustValueMax(player->xp, xp, 9999);
    notifyOfChange();
}

/**
 * Perform a certain type of healing on the party member
 */
bool PartyMember::heal(HealType type) {
    switch(type) {

    case HT_NONE:
        return true;

    case HT_CURE:
        if (getStatus() != STAT_POISONED)
            return false;
cure:
        removeStatus(STAT_POISONED);
        return true;

    case HT_FULLHEAL:
        if (isDead() || player->hp == player->hpMax)
            return false;
        player->hp = player->hpMax;
        break;

    case HT_RESTORE:
        if (isDead())
            return false;
        if (getStatus() != STAT_POISONED && player->hp == player->hpMax)
            return false;
        player->hp = player->hpMax;
        goto cure;

    case HT_RESURRECT:
        if (! isDead())
            return false;
        setStatus(STAT_GOOD);
        break;

    case HT_HEAL:
        if (isDead() || player->hp == player->hpMax)
            return false;

        player->hp += 75 + (xu4_random(0x100) % 0x19);
        break;

    case HT_CAMPHEAL:
        if (isDead() || player->hp == player->hpMax)
            return false;
        player->hp += 99 + (xu4_random(0x100) & 0x77);
        break;

    case HT_INNHEAL:
        if (isDead() || player->hp == player->hpMax)
            return false;
        player->hp += 100 + (xu4_random(50) * 2);
        break;

    default:
        return false;
    }

    if (player->hp > player->hpMax)
        player->hp = player->hpMax;

    notifyOfChange();
    return true;
}

/**
 * Remove status effects from the party member
 */
void PartyMember::removeStatus(StatusType s) {
    Creature::removeStatus(s);
    player->status = getStatus();
    notifyOfChange();
}

void PartyMember::setHp(int hp) {
    player->hp = hp;
    notifyOfChange();
}

void PartyMember::setMp(int mp) {
    player->mp = mp;
    notifyOfChange();
}

EquipError PartyMember::setArmor(const Armor *a) {
    ArmorType type = (ArmorType) a->type;

    if (type != ARMR_NONE && party->saveGame->armor[type] < 1)
        return EQUIP_NONE_LEFT;
    if (!a->canWear(getClass()))
        return EQUIP_CLASS_RESTRICTED;

    ArmorType oldArmorType = (ArmorType) getArmor()->type;
    if (oldArmorType != ARMR_NONE)
        party->saveGame->armor[oldArmorType]++;
    if (type != ARMR_NONE)
        party->saveGame->armor[type]--;

    player->armor = type;
    notifyOfChange();

    return EQUIP_SUCCEEDED;
}

EquipError PartyMember::setWeapon(const Weapon *w) {
    WeaponType type = (WeaponType) w->type;

    if (type != WEAP_HANDS && party->saveGame->weapons[type] < 1)
        return EQUIP_NONE_LEFT;
    if (!w->canReady(getClass()))
        return EQUIP_CLASS_RESTRICTED;

    WeaponType old = (WeaponType) getWeapon()->type;
    if (old != WEAP_HANDS)
        party->saveGame->weapons[old]++;
    if (type != WEAP_HANDS)
        party->saveGame->weapons[type]--;

    player->weapon = type;
    notifyOfChange();

    return EQUIP_SUCCEEDED;
}

/**
 * Applies damage to a player, and changes status to dead if hit
 * points drop below zero.
 *
 * Byplayer is ignored for now, since it should always be false for U4.  (Is
 * there anything special about being killed by a party member in U5?)  Also
 * keeps interface consistent for virtual base function Creature::applydamage()
 */
bool PartyMember::applyDamage(Map* map, int damage, bool) {
    int newHp = player->hp;

    if (isDead())
        return false;

    newHp -= damage;

    if (newHp < 0) {
        setStatus(STAT_DEAD);
        newHp = 0;
    }

    player->hp = newHp;
    notifyOfChange();

    if (isCombatMap(map) && isDead()) {
        TileId corpseId = Tileset::findTileByName(Tile::sym.corpse)->getId();
        Annotation* ann = map->annotations.add(coords, corpseId);
        ann->ttl = party->size() * 2;

        if (party) {
            PartyEvent event(PartyEvent::PLAYER_KILLED, this);
            event.player = this;
            gs_emitMessage(SENDER_PARTY, &event);
        }

        /* remove yourself from the map */
        removeFromMaps();
        return false;
    }

    return true;
}

int PartyMember::getAttackBonus() const {
    if (xu4.config->weapon(player->weapon)->alwaysHits() || player->dex >= 40)
    return 255;
    return player->dex;
}

int PartyMember::getDefense() const {
    return xu4.config->armor(player->armor)->defense;
}

bool PartyMember::dealDamage(Map* map, Creature *m, int damage) {
    /* we have to record these now, because if we
       kill the target, it gets destroyed */
    int m_xp = m->getXp();

    if (!Creature::dealDamage(map, m, damage)) {
        /* half the time you kill an evil creature you get a karma boost */
        awardXp(m_xp);
        return false;
    }
    return true;
}

/**
 * Calculate damage for an attack.
 */
int PartyMember::getDamage() {
    int maxDamage;

    maxDamage = xu4.config->weapon(player->weapon)->damage;
    maxDamage += player->str;
    if (maxDamage > 255)
        maxDamage = 255;

    return xu4_random(maxDamage);
}

/**
 * Returns the tile that will be displayed when the party
 * member's attack hits
 */
Symbol PartyMember::getHitTile() const {
    return getWeapon()->hitTile;
}

/**
 * Returns the tile that will be displayed when the party
 * member's attack fails
 */
Symbol PartyMember::getMissTile() const {
    return getWeapon()->missTile;
}

/**
 * Lose the equipped weapon for the player (flaming oil, ranged daggers, etc.)
 * Returns the number of weapons left of that type, including the one in
 * the players hand
 */
int PartyMember::loseWeapon() {
    int weapon = player->weapon;

    notifyOfChange();

    if (party->saveGame->weapons[weapon] > 0)
        return (--party->saveGame->weapons[weapon]) + 1;
    else {
        player->weapon = WEAP_HANDS;
        return 0;
    }
}

/**
 * Put the party member to sleep
 */
void PartyMember::putToSleep() {
    if (! isDead()) {
        addStatus(STAT_SLEEPING);
        tile = Tileset::findTileByName(Tile::sym.corpse)->getId();
    }
}

/**
 * Wakes up the party member
 */
void PartyMember::wakeUp() {
    removeStatus(STAT_SLEEPING);
    tile = tileForClass(getClass());
}

MapTile PartyMember::tileForClass(int klass) {
    ASSERT(klass < 8, "invalid class %d in tileForClass", klass);
    Symbol name = Tile::sym.classTiles[klass];
    const Tile *tile = xu4.config->tileset()->getByName(name);
    ASSERT(tile, "no tile found for class %d", klass);
    return tile->getId();
}

/**
 * Party class implementation
 */
Party::Party(SaveGame *s) : saveGame(s), transport(0), torchduration(0), activePlayer(-1) {
    // NOTE: Avoid calls which emit notifications during construction!

    if (MAP_DECEIT <= saveGame->location && saveGame->location <= MAP_ABYSS)
        torchduration = saveGame->torchduration;

    // add the members to the party
    for (int i = 0; i < saveGame->members; i++)
        members.push_back(new PartyMember(this, &saveGame->players[i]));

    // set the party's transport (transport value stored in savegame
    // hardcoded to index into base tilemap)
    initTransport(xu4.config->usaveIds()->moduleId(saveGame->transport));
}

Party::~Party() {
    PartyMemberVector::iterator it;
    foreach (it, members)
        delete *it;
}

/**
 * Notify the party that something about it has changed
 */
void Party::notifyOfChange(PartyMember *pm, PartyEvent::Type eventType) {
    PartyEvent event(eventType, pm);
    gs_emitMessage(SENDER_PARTY, &event);
}

void Party::adjustFood(int food) {
    int oldFood = saveGame->food;
    AdjustValue(saveGame->food, food, 999900, 0);
    if ((saveGame->food / 100) != (oldFood / 100)) {
        notifyOfChange();
    }
}

void Party::adjustGold(int gold) {
    AdjustValue(saveGame->gold, gold, 9999, 0);
    notifyOfChange();
}

/*
 * Check if enough time has passed since last virtue award if a karma
 * action is time limited.
 */
bool Party::virtueIncreaseTimeout() {
    if (((saveGame->moves / 16) >= 0x10000) ||
        (((saveGame->moves / 16) & 0xFFFF) != saveGame->lastvirtue)) {
        saveGame->lastvirtue = (saveGame->moves / 16) & 0xFFFF;
        return true;
    }
    return false;
}

/**
 * Adjusts the avatar's karma level for the given action.  Notify
 * observers with a lost eighth event if the player has lost avatarhood.
 */
void Party::adjustKarma(KarmaAction action) {
#define VMASK_1(a)      (1<<a)
#define VMASK_2(a,b)    ((1<<a) | (1<<b))
#define VMASK_3(a,b,c)  ((1<<a) | (1<<b) | (1<<c))

    switch (action) {
    case KA_FOUND_ITEM:
        adjustVirtues(VMASK_1(VIRT_HONOR), 5);
        break;
    case KA_STOLE_CHEST:
        adjustVirtues(VMASK_3(VIRT_HONESTY, VIRT_JUSTICE, VIRT_HONOR), -1);
        break;
    case KA_GAVE_ALL_TO_BEGGAR:
        //  When donating all, you get +3 HONOR in Apple 2, but not in in U4DOS.
        //  TODO: Make this a configuration option.
        //  adjustVirtues(VMASK_1(VIRT_HONOR), 3);
    case KA_GAVE_TO_BEGGAR:
        //  In U4DOS, we only get +2 COMPASSION, no HONOR or SACRIFICE even if
        //  donating all.
        if (virtueIncreaseTimeout())
            adjustVirtues(VMASK_1(VIRT_COMPASSION), 2);
        break;
    case KA_BRAGGED:
        adjustVirtues(VMASK_1(VIRT_HUMILITY), -5);
        break;
    case KA_HUMBLE:
        if (virtueIncreaseTimeout())
            adjustVirtues(VMASK_1(VIRT_HUMILITY), 10);
        break;
    case KA_HAWKWIND:
    case KA_MEDITATION:
        if (virtueIncreaseTimeout())
            adjustVirtues(VMASK_1(VIRT_SPIRITUALITY), 3);
        break;
    case KA_BAD_MANTRA:
        adjustVirtues(VMASK_1(VIRT_SPIRITUALITY), -3);
        break;
    case KA_ATTACKED_GOOD:
        adjustVirtues(VMASK_3(VIRT_COMPASSION, VIRT_JUSTICE, VIRT_HONOR), -5);
        break;
    case KA_FLED_EVIL:
        adjustVirtues(VMASK_1(VIRT_VALOR), -2);
        break;
    case KA_HEALTHY_FLED_EVIL:
        adjustVirtues(VMASK_2(VIRT_VALOR, VIRT_SACRIFICE), -2);
        break;
    case KA_KILLED_EVIL:
         /* gain one valor half the time, zero the rest */
        if (xu4_random(2))
            adjustVirtues(VMASK_1(VIRT_VALOR), 1);
        break;
    case KA_FLED_GOOD:
        adjustVirtues(VMASK_2(VIRT_COMPASSION, VIRT_JUSTICE), 2);
        break;
    case KA_SPARED_GOOD:
        adjustVirtues(VMASK_2(VIRT_COMPASSION, VIRT_JUSTICE), 1);
        break;
#if 0
    // These are handled by the vendor script.
    case KA_DONATED_BLOOD:
        adjustVirtues(VMASK_1(VIRT_SACRIFICE), 5);
        break;
    case KA_DIDNT_DONATE_BLOOD:
        adjustVirtues(VMASK_1(VIRT_SACRIFICE), -5);
        break;
    case KA_CHEAT_REAGENTS:
        // This doesn't match the penalty in U4DOS, which is based on price.
        adjustVirtues(VMASK_3(VIRT_HONESTY, VIRT_JUSTICE, VIRT_HONOR), -10);
        break;
    case KA_DIDNT_CHEAT_REAGENTS:
        adjustVirtues(VMASK_3(VIRT_HONESTY, VIRT_JUSTICE, VIRT_HONOR), 2);
        break;
#endif
    case KA_USED_SKULL:
        /* using the skull is very, very bad... */
        adjustVirtues(0xff, -5);
        break;
    case KA_DESTROYED_SKULL:
        /* ...but destroying it is very, very good */
        adjustVirtues(0xff, 10);
        break;
    default:
        break;
    }
}

/**
 * Adjusts the avatar's virtue levels by a fixed amount.
 * Notify observers with a lost eighth event if the player has lost avatarhood.
 */
void Party::adjustVirtues(uint32_t virtueMask, int adjust) {
    const int enlightened = 0;  // DOS save game enlightenment value.
    int v, level;
    int modified = 0;

    for (v = 0; v < VIRT_MAX && virtueMask; v++) {
        if (virtueMask & 1) {
            level = saveGame->karma[v];
            if (adjust > 0) {
                if (level != enlightened) {
                    level += adjust;
                    if (level > 99)
                        level = 99;
                    saveGame->karma[v] = level;
                    ++modified;
                }
            } else {
                if (level == enlightened) {
                    level = 99;
                    PartyEvent event(PartyEvent::LOST_EIGHTH, 0);
                    gs_emitMessage(SENDER_PARTY, &event);
                }
                level += adjust;
                if (level < 1)
                    level = 1;
                saveGame->karma[v] = level;
                ++modified;
            }
            //printf("AV %d %d:%d\n", saveGame->moves, v, saveGame->karma[v]);
        }
        virtueMask >>= 1;
    }

    /* something changed */
    if (modified)
        notifyOfChange();
}

/**
 * Apply effects to a player or the entire party.
 */
void Party::applyEffect(int player, Map* map, TileEffect effect) {
    PartyMember* pc;
    int flashMask = 0;
    int damageSound = SOUND_POISON_EFFECT;
    int i, end;
    bool always;

    if (player == ALL_PLAYERS) {
        always = false;
        i = 0;
        end = size();
    } else {
        always = true;
        i = player;
        end = i + 1;
    }

    switch(effect) {
        case EFFECT_NONE:
        case EFFECT_ELECTRICITY:
            return;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
bomb:
            for (; i < end; i++) {
                pc = members[i];
                if (pc->isDead())
                    continue;
                if (always || xu4_random(2) == 0) {
                    pc->applyDamage(map, 16 + xu4_random(32));
                    flashMask |= 1 << i;
                }
            }
            break;

        case EFFECT_SLEEP:
            damageSound = SOUND_SLEEP;
            for (; i < end; i++) {
                pc = members[i];
                if (pc->isDisabled())
                    continue;
                if (always || xu4_random(2) == 0) {
                    // The original game has a single status field so sleeping
                    // removes any poison.  Note it doesn't cure poison when
                    // resting in camp as the status is not changed by Hole up.
                    pc->clearStatus(STAT_POISONED);
                    pc->putToSleep();
                    flashMask |= 1 << i;
                }
            }
            break;

        case EFFECT_POISONFIELD:
        case EFFECT_POISON:
            for (; i < end; i++) {
                pc = members[i];
                if (pc->isDead() || pc->getStatus() == STAT_POISONED)
                    continue;
                if (always || xu4_random(5) == 0) {
                    pc->addStatus(STAT_POISONED);
                    flashMask |= 1 << i;
                }
            }
            break;

        case EFFECT_ROCKS:
            // The DOS game flashes all player stats, even for those not
            // damaged.  Xu4 will only flash those affected.
            soundPlay(SOUND_STONE_FALLING);
            damageSound = SOUND_PARTY_STRUCK;
            // Treat falling rocks and pits like bomb traps.
            goto bomb;
    }

    if (flashMask) {
        soundPlay(Sound(damageSound));
        c->stats->flashPlayers(flashMask);
    }
}

/**
 * Attempt to elevate in the given virtue
 */
bool Party::attemptElevation(Virtue virtue) {
    if (saveGame->karma[virtue] == 99) {
        saveGame->karma[virtue] = 0;
        notifyOfChange();
        return true;
    } else
        return false;
}

/**
 * Burns a torch's duration down a certain number of turns
 * Returns true if the torch is still providing light.
 */
bool Party::burnTorch(int turns) {
    if (torchduration) {
        torchduration -= turns;
        if (torchduration <= 0)
            torchduration = 0;

        saveGame->torchduration = torchduration;
        notifyOfChange();
    }
    return torchduration > 0;
}

/**
 * Returns true if the party can enter the shrine
 */
bool Party::canEnterShrine(Virtue virtue) {
    if (saveGame->runes & (1 << (int) virtue))
        return true;
    else
        return false;
}

/**
 * Returns true if the person can join the party
 */
bool Party::canPersonJoin(const char* name, Virtue *v) {
    int i;

    if (! name)
        return false;

    for (i = 1; i < 8; i++) {
        if (strequ(name, saveGame->players[i].name)) {
            if (v)
                *v = (Virtue) saveGame->players[i].klass;
            return true;
        }
    }
    return false;
}

/**
 * Donates 'quantity' gold. Returns true if the donation succeeded,
 * or false if there was not enough gold to make the donation
 */
bool Party::donate(int quantity) {
    if (quantity > saveGame->gold)
        return false;

    adjustGold(-quantity);
    if (saveGame->gold > 0)
        adjustKarma(KA_GAVE_TO_BEGGAR);
    else adjustKarma(KA_GAVE_ALL_TO_BEGGAR);

    return true;
}

/**
 * Ends the party's turn
 */
void Party::endTurn() {
    Location* loc = c->location;
    int i;
    int nonCombat = (loc->context & CTX_NON_COMBAT) == loc->context;
    int poisonedMask = 0;

    saveGame->moves++;

    for (i = 0; i < size(); i++) {
        PartyMember* pc = members[i];

        /* Handle player status (only for non-combat turns) */
        if (nonCombat) {

            /* party members eat food (also non-combat) */
            if (! pc->isDead())
                adjustFood(-1);

            switch (pc->getStatus()) {
            case STAT_SLEEPING:
                if (xu4_random(5) == 0)
                    pc->wakeUp();
                break;

            case STAT_POISONED:
                if (pc->applyDamage(loc->map, 2))
                    poisonedMask |= 1 << i;
                break;

            default:
                break;
            }
        }

        /* regenerate magic points */
        if (! pc->isDisabled() && pc->getMp() < pc->getMaxMp())
            saveGame->players[i].mp++;
    }

    /* FIXME: Starving uses a game message while poisoning does not.
     * They should be handled in a similar manner.
     */

    if (poisonedMask) {
        /* NOTE: In the DOS version the poisoned sound & status flash occurs
         * for each PC separately.  Xu4 combines them.
         *
         * If the poison damage occurs just before combat begins, these
         * effects may be observed after the combat screen appears.
         */
        soundPlay(SOUND_POISON_DAMAGE);
        c->stats->flashPlayers(poisonedMask);
    }

    /* The party is starving! */
    if ((saveGame->food == 0) && nonCombat) {
        PartyEvent event(PartyEvent::STARVING, 0);
        gs_emitMessage(SENDER_PARTY, &event);
    }

    /* heal ship (25% chance it is healed each turn) */
    if ((loc->context == CTX_WORLDMAP) && (saveGame->shiphull < 50) && xu4_random(4) == 0)
        healShip(1);
}

/**
 * Adds a chest worth of gold to the party's inventory
 */
int Party::getChest() {
    int gold = xu4_random(50) + xu4_random(8) + 10;
    adjustGold(gold);

    return gold;
}

/**
 * Returns the number of turns a currently lit torch will last (or 0 if no torch lit)
 */
int Party::getTorchDuration() const {
    return torchduration;
}

/**
 * Damages the party's ship.
 *
 * Return true if the ship sinks (hull goes below zero).
 */
bool Party::damageShip(unsigned int pts) {
    bool sunk;
    if (pts > saveGame->shiphull) {
        saveGame->shiphull = 0;
        sunk = true;
    } else {
        saveGame->shiphull -= pts;
        sunk = false;
    }

    notifyOfChange();
    return sunk;
}

/**
 * Heals the ship's hull strength by 'pts' points
 */
void Party::healShip(unsigned int pts) {
    saveGame->shiphull += pts;
    if (saveGame->shiphull > 50)
        saveGame->shiphull = 50;

    notifyOfChange();
}

/**
 * Returns true if the balloon is currently in the air
 */
bool Party::isFlying() const {
    return (saveGame->balloonstate && torchduration <= 0);
}

/**
 * Whether or not the party can make an action.
 */
bool Party::isImmobilized() {
    int i;
    bool immobile = true;

    for (i = 0; i < saveGame->members; i++) {
        if (!members[i]->isDisabled())
            immobile = false;
    }

    return immobile;
}

/**
 * Whether or not all the party members are dead.
 */
bool Party::isDead() {
    for (int i = 0; i < saveGame->members; i++) {
        if (! members[i]->isDead())
            return false;
    }
    return true;
}

/**
 * Returns true if the person with that name
 * is already in the party
 */
bool Party::isPersonJoined(const char* name) {
    int i;

    if (! name)
        return false;

    for (i = 1; i < saveGame->members; i++) {
        if (strequ(name, saveGame->players[i].name))
            return true;
    }
    return false;
}

/**
 * Attempts to add the person to the party.
 * Returns JOIN_SUCCEEDED if successful.
 */
CannotJoinError Party::join(const char* name) {
    int i;
    SaveGamePlayerRecord tmp;

    for (i = saveGame->members; i < 8; i++) {
        if (strequ(name, saveGame->players[i].name)) {

            /* ensure avatar is experienced enough */
            if (saveGame->members + 1 > (saveGame->players[0].hpMax / 100))
                return JOIN_NOT_EXPERIENCED;

            /* ensure character has enough karma */
            if ((saveGame->karma[saveGame->players[i].klass] > 0) &&
                (saveGame->karma[saveGame->players[i].klass] < 40))
                return JOIN_NOT_VIRTUOUS;

            tmp = saveGame->players[saveGame->members];
            saveGame->players[saveGame->members] = saveGame->players[i];
            saveGame->players[i] = tmp;

            members.push_back(new PartyMember(this, &saveGame->players[saveGame->members++]));
            PartyEvent event(PartyEvent::MEMBER_JOINED, members.back());
            gs_emitMessage(SENDER_PARTY, &event);
            return JOIN_SUCCEEDED;
        }
    }

    return JOIN_NOT_EXPERIENCED;
}

/**
 * Lights a torch with a default duration of 100
 */
bool Party::lightTorch(int duration, bool loseTorch) {
    if (loseTorch) {
        if (c->saveGame->torches <= 0)
            return false;
        c->saveGame->torches--;
    }

    torchduration += duration;
    saveGame->torchduration = torchduration;

    soundPlay(SOUND_IGNITE);
    notifyOfChange();

    return true;
}

/**
 * Extinguishes a torch
 */
void Party::quenchTorch() {
    torchduration = saveGame->torchduration = 0;

    notifyOfChange();
}

/**
 * Restore each party member to max MP, and restore some HP.
 * \return True if any member gained HP.
 */
bool Party::applyRest(HealType type) {
    bool healed = false;
    for (int i = 0; i < size(); i++) {
        PartyMember *m = member(i);
        m->setMp(m->getMaxMp());
        if ((m->getHp() < m->getMaxHp()) && m->heal(type))
            healed = true;
    }
    return healed;
}

/**
 * Revives the party after the entire party has been killed
 */
void Party::reviveParty() {
    int i;

    for (i = 0; i < size(); i++) {
        members[i]->wakeUp();
        members[i]->setStatus(STAT_GOOD);
        saveGame->players[i].hp = saveGame->players[i].hpMax;
    }

    for (int i = ARMR_NONE + 1; i < ARMR_MAX; i++)
        saveGame->armor[i] = 0;
    for (int i = WEAP_HANDS + 1; i < WEAP_MAX; i++)
        saveGame->weapons[i] = 0;
    saveGame->food = 20099;
    saveGame->gold = 200;
    setTransport(Tileset::findTileByName(Tile::sym.avatar)->getId());

    PartyEvent event(PartyEvent::PARTY_REVIVED, 0);
    gs_emitMessage(SENDER_PARTY, &event);
}

MapTile Party::getTransport() const {
    return transport;
}

/*
 * This is a version of setTransport used during initialization.
 */
void Party::initTransport(const MapTile& tile) {
    transport = tile;

    if (tile.getTileType()->isHorse())
        c->transportContext = TRANSPORT_HORSE;
    else if (tile.getTileType()->isShip())
        c->transportContext = TRANSPORT_SHIP;
    else if (tile.getTileType()->isBalloon())
        c->transportContext = TRANSPORT_BALLOON;
    else
        c->transportContext = TRANSPORT_FOOT;
}

void Party::setTransport(MapTile tile) {
    // transport value stored in savegame hardcoded to index into base tilemap
    saveGame->transport = xu4.config->usaveIds()->ultimaId(tile);
    ASSERT(saveGame->transport != 0, "could not generate valid savegame transport for tile with id %d\n", tile.id);
    initTransport(tile);
    notifyOfChange();
}

void Party::setShipHull(int str) {
    int newStr = str;
    AdjustValue(newStr, 0, 99, 0);

    if (saveGame->shiphull != newStr) {
        saveGame->shiphull = newStr;
        notifyOfChange();
    }
}

Direction Party::getDirection() const {
    return transport.getDirection();
}

void Party::setDirection(Direction dir) {
    transport.setDirection(dir);
}

void Party::adjustReagent(int reagent, int amt) {
    int oldVal = c->saveGame->reagents[reagent];
    AdjustValue(c->saveGame->reagents[reagent], amt, 99, 0);

    if (oldVal != c->saveGame->reagents[reagent]) {
        notifyOfChange();
    }
}

int Party::getReagent(int reagent) const {
    return c->saveGame->reagents[reagent];
}

short* Party::getReagentPtr(int reagent) const {
    return &c->saveGame->reagents[reagent];
}

void Party::setActivePlayer(int p) {
    activePlayer = p;
    PartyEvent event(PartyEvent::ACTIVE_PLAYER_CHANGED, activePlayer < 0 ? 0 : members[activePlayer] );
    gs_emitMessage(SENDER_PARTY, &event);
}

int Party::getActivePlayer() const {
    return activePlayer;
}

void Party::swapPlayers(int p1, int p2) {
    ASSERT(p1 < saveGame->members, "p1 out of range: %d", p1);
    ASSERT(p2 < saveGame->members, "p2 out of range: %d", p2);

    SaveGamePlayerRecord tmp = saveGame->players[p1];
    saveGame->players[p1] = c->saveGame->players[p2];
    c->saveGame->players[p2] = tmp;

    syncMembers();

    if (p1 == activePlayer)
        activePlayer = p2;
    else if (p2 == activePlayer)
        activePlayer = p1;

    notifyOfChange(0);
}

void Party::syncMembers() {
    members.clear();
    for (int i = 0; i < saveGame->members; i++) {
        // add the members to the party
        members.push_back(new PartyMember(this, &saveGame->players[i]));
    }
}

/**
 * Returns the size of the party
 */
int Party::size() const {
    return members.size();
}

/**
 * Returns a pointer to the party member indicated
 */
PartyMember *Party::member(int index) const {
    return members[index];
}

int Party::memberIndex(Creature* person) const {
    int count = members.size();
    for (int i = 0; i < count; ++i) {
        if (person == members[i])
            return i;
    }
    return -1;
}
