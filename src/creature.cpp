/**
 * creature.cpp
 */

#include "creature.h"

#include <cstring>
#include "combat.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "screen.h"
#include "settings.h"
#include "textcolor.h"  // required to change the color of screen message text
#include "utils.h"
#include "xu4.h"

#define CSTR(ID)    xu4.config->confString(ID)

enum StatusBits {
    StatPoisoned = 0x01,
    StatSleeping = 0x02,
    StatDead     = 0x04
};

bool isCreature(Object *punknown) {
    Creature *m;
    if ((m = dynamic_cast<Creature*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Creature class implementation
 */
Creature::Creature() : Object(Object::CREATURE) {
    rangedhittile  =
    rangedmisstile =
    camouflageTile = SYM_UNSET;
    status = 0;
}

Creature::Creature(const Creature* cproto) : Object(Object::CREATURE) {
    *this = *cproto;
    onMaps = 0;
    animId = ANIM_UNUSED;
}

std::string Creature::getName() const {
    return CSTR(name);
}

bool Creature::isAttackable() const  {
    if (mattr & MATTR_NONATTACKABLE)
        return false;
    /* can't attack horse transport */
    if (tile.getTileType()->isHorse() && movement == MOVEMENT_FIXED)
        return false;
    return true;
}

int Creature::getDamage() const {
    int x = xu4_random(basehp >> 2);
    return (x >> 4) * 10 + (x % 10);
}

int Creature::setInitialHp(int points) {
    if (points < 0)
        hp = xu4_random(basehp) | (basehp / 2);
    else
        hp = points;

    /* make sure the creature doesn't flee initially */
    if (hp < 24) hp = 24;

    return hp;
}

void Creature::setRandomRanged() {
    rangedhittile = rangedmisstile = Tile::sym.fields[ xu4_random(4) ];
}

CreatureStatus Creature::getState() const {
    int heavy_threshold, light_threshold, crit_threshold;

    crit_threshold = basehp >> 2;  /* (basehp / 4) */
    heavy_threshold = basehp >> 1; /* (basehp / 2) */
    light_threshold = crit_threshold + heavy_threshold;

    if (hp <= 0)
        return MSTAT_DEAD;
    else if (hp < 24)
        return MSTAT_FLEEING;
    else if (hp < crit_threshold)
        return MSTAT_CRITICAL;
    else if (hp < heavy_threshold)
        return MSTAT_HEAVILYWOUNDED;
    else if (hp < light_threshold)
        return MSTAT_LIGHTLYWOUNDED;
    else
        return MSTAT_BARELYWOUNDED;

}

/**
 * Performs a special action for the creature
 * Returns true if the action takes up the creatures
 * whole turn (i.e. it cant move afterwords)
 */
bool Creature::specialAction() {
    const Location* loc = c->location;
    bool retval = false;

    int dx = abs(loc->coords.x - coords.x);
    int dy = abs(loc->coords.y - coords.y);
    int mapdist = map_distance(loc->coords, coords, loc->map);

    /* find out which direction the avatar is in relation to the creature */
    int dir = map_getRelativeDirection(coords, loc->coords, loc->map);

    //Init outside of switch
    int broadsidesDirs = 0;

    switch(id) {

    case LAVA_LIZARD_ID:
    case SEA_SERPENT_ID:
    case HYDRA_ID:
    case DRAGON_ID:

        /* A 50/50 chance they try to range attack when you're close enough
           and not in a city
           Note: Monsters in settlements in U3 do fire on party
        */
        if (mapdist <= 3 && xu4_random(2) == 0 && (loc->context & CTX_CITY) == 0) {
            vector<Coords> path = gameGetDirectionalActionPath(dir, MASK_DIR_ALL, coords,
                                                               1, 3, NULL, false);
            for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
                if (creatureRangeAttack(*i, this))
                    break;
            }
        }

        break;

    case PIRATE_ID:

        /* Fire cannon: Pirates only fire broadsides and only when they can hit you :) */
        retval = true;
        broadsidesDirs = dirGetBroadsidesDirs(tile.getDirection());

        if ((((dx == 0) && (dy <= 3)) ||          /* avatar is close enough and on the same column, OR */
             ((dy == 0) && (dx <= 3))) &&         /* avatar is close enough and on the same row, AND */
            ((broadsidesDirs & dir) > 0)) { /* pirate ship is firing broadsides */

            // nothing (not even mountains!) can block cannonballs
            vector<Coords> path = gameGetDirectionalActionPath(dir, broadsidesDirs, coords,
                                                               1, 3, NULL, false);
            for (vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
                if (fireAt(*i, false))
                    break;
            }
        }
        else
            retval = false;

        break;

    default:
        // Handle pausing after speaking with player.
        if (movement == MOVEMENT_FOLLOW_PAUSE) {
            movement = MOVEMENT_FOLLOW_AVATAR;
            retval = true;
        }
        break;
    }

    return retval;
}

/**
 * Performs a special effect for the creature
 * Returns true if something special happened,
 * or false if nothing happened
 */
bool Creature::specialEffect() {
    Object *obj;
    bool retval = false;

    switch(id) {

    case STORM_ID:
        {
            ObjectDeque::iterator i;

            if (coords == c->location->coords) {

                /* damage the ship */
                if (c->transportContext == TRANSPORT_SHIP) {
                    /* FIXME: Check actual damage from u4dos */
                    gameDamageShip(10, 30);
                }
                /* anything else but balloon damages the party */
                else if (c->transportContext != TRANSPORT_BALLOON) {
                    /* FIXME: formula for twister damage is guesstimated from u4dos */
                    gameDamageParty(0, 75);
                }
                return true;
            }

            /* See if the storm is on top of any objects and destroy them! */
            for (i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();) {

                obj = *i;
                if (this != obj && obj->coords == coords) {
                    /* Converged with an object, destroy the object! */
                    i = c->location->map->removeObject(i);
                    retval = true;
                }
                else i++;
            }
        }
        break;

    case WHIRLPOOL_ID:
        {
            ObjectDeque::iterator i;

            if (coords == c->location->coords && (c->transportContext == TRANSPORT_SHIP)) {

                /* Deal 10 damage to the ship */
                gameDamageShip(-1, 10);

                /* Send the party to Locke Lake */
                const Coords* dest =
                    c->location->map->getLabel(Tile::sym.lockelake);
                if (dest)
                    c->location->coords = *dest;

                /* Teleport the whirlpool that sent you there far away from lockelake */
                this->updateCoords(Coords(0,0,0));
                retval = true;
                break;
            }

            /* See if the whirlpool is on top of any objects and destroy them! */
            for (i = c->location->map->objects.begin();
                 i != c->location->map->objects.end();) {

                obj = *i;

                if (this != obj && obj->coords == coords) {

                    Creature *m = dynamic_cast<Creature*>(obj);

                    /* Make sure the object isn't a flying creature or object */
                    if (!m || (m && (m->swims() || m->sails()) && !m->flies())) {
                        /* Destroy the object it met with */
                        i = c->location->map->removeObject(i);
                        retval = true;
                    }
                    else {
                        i++;
                    }
                }
                else i++;
            }
        }

    default: break;
    }

    return retval;
}

void Creature::act(CombatController *controller) {
    int dist;
    CombatAction action;
    Creature *target;
    CombatMap* map = controller->getMap();

    /* see if creature wakes up if it is asleep */
    if ((getStatus() == STAT_SLEEPING) && (xu4_random(8) == 0))
        wakeUp();

    /* if the creature is still asleep, then do nothing */
    if (getStatus() == STAT_SLEEPING)
        return;

    if (negates())
        c->aura.set(Aura::NEGATE, 2);

    /*
     * figure out what to do
     */

    // creatures who teleport do so 1/8 of the time
    if (teleports() && xu4_random(8) == 0)
        action = CA_TELEPORT;
    // creatures who ranged attack do so 1/4 of the time.  Make sure
    // their ranged attack is not negated!
    else if (ranged != 0 && xu4_random(4) == 0 &&
             (rangedhittile != Tile::sym.magicFlash || (c->aura.getType() != Aura::NEGATE)))
        action = CA_RANGED;
    // creatures who cast sleep do so 1/4 of the time they don't ranged attack
    else if (castsSleep() && (c->aura.getType() != Aura::NEGATE) && (xu4_random(4) == 0))
        action = CA_CAST_SLEEP;
    else if (getState() == MSTAT_FLEEING)
        action = CA_FLEE;
    // default action: attack (or move towards) closest target
    else
        action = CA_ATTACK;

    /*
     * now find out who to do it to
     */

    target = nearestOpponent(map, &dist, action == CA_RANGED);
    if (target == NULL)
        return;

    if (action == CA_ATTACK && dist > 1)
        action = CA_ADVANCE;

    /* let's see if the creature blends into the background, or if he appears... */
    if (camouflages() && !hideOrShow(map))
        return; /* creature is hidden -- no action! */

    switch(action) {
    case CA_ATTACK:
        soundPlay(SOUND_NPC_ATTACK, false);                                    // NPC_ATTACK, melee

        if (controller->attackHit(this, target)) {
            soundPlay(SOUND_PC_STRUCK, false);                                 // PC_STRUCK, melee and ranged
            GameController::flashTile(target->coords, Tile::sym.hitFlash, 4);


            if (!dealDamage(map, target, getDamage()))
                target = NULL;

            if (target && isPartyMember(target)) {
                /* steal gold if the creature steals gold */
                if (stealsGold() && xu4_random(4) == 0) {
                    soundPlay(SOUND_ITEM_STOLEN, false);                       // ITEM_STOLEN, gold
                    c->party->adjustGold(-(xu4_random(0x3f)));
                }

                /* steal food if the creature steals food */
                if (stealsFood()) {
                    soundPlay(SOUND_ITEM_STOLEN, false);                       // ITEM_STOLEN, food
                    c->party->adjustFood(-2500);
                }
            }
        } else {
            GameController::flashTile(target->coords, Tile::sym.missFlash, 1);
        }
        break;

    case CA_CAST_SLEEP: {
        screenMessage("\nSleep!\n");

        gameSpellEffect('s', -1, static_cast<Sound>(SOUND_MAGIC)); /* show the sleep spell effect */

        /* Apply the sleep spell to party members still in combat */
        if (!isPartyMember(this)) {
            PartyMemberVector party = map->getPartyMembers();
            PartyMemberVector::iterator j;

            for (j = party.begin(); j != party.end(); j++) {
                if (xu4_random(2) == 0)
                    (*j)->putToSleep();
            }
        }
        break;
    }

    case CA_TELEPORT: {
        Coords new_c;
        bool valid = false;
        bool firstTry = true;

        while (!valid) {
            new_c = Coords(xu4_random(map->width), xu4_random(map->height), c->location->coords.z);

            const Tile *tile = map->tileTypeAt(new_c, WITH_OBJECTS);

            if (tile->isCreatureWalkable()) {
                /* If the tile would slow me down, try again! */
                if (firstTry && tile->getSpeed() != FAST)
                    firstTry = false;
                /* OK, good enough! */
                else
                    valid = true;
            }
        }

        /* Teleport! */
        updateCoords(new_c);
        break;
    }

    case CA_RANGED: {
        // if the creature has a random tile for a ranged weapon,
        // let's switch it now!
        if (hasRandomRanged())
            setRandomRanged();

        soundPlay(SOUND_NPC_ATTACK, false);

        // figure out which direction to fire the weapon
        int dir = map_getRelativeDirection(coords, target->coords);

        controller->creatureRangedAttack(this, dir);
        break;
    }

    case CA_FLEE:
    case CA_ADVANCE: {
        if (moveCombatObject(action, map, this, target->coords)) {
            if (MAP_IS_OOB(map, coords)) {
                screenMessage("\n%c%s Flees!%c\n", FG_YELLOW, CSTR(name), FG_WHITE);

                /* Congrats, you have a heart! */
                if (isGood())
                    c->party->adjustKarma(KA_SPARED_GOOD);

                map->removeObject(this);
                return;     // Avoid calling animateMovement below.
            }
        }
        break;
    }
    }
    this->animateMovement();
}

void Creature::applyTileEffect(Map* map, TileEffect effect) {
    if (effect != EFFECT_NONE) {
        gameUpdateScreen();

        switch(effect) {
        case EFFECT_SLEEP:
            /* creature fell asleep! */
            if ((resists != EFFECT_SLEEP) &&
                (xu4_random(0xFF) >= hp))
                putToSleep();
            break;

        case EFFECT_LAVA:
        case EFFECT_FIRE:
            /* deal 0 - 127 damage to the creature if it is not immune to fire damage */
            if ((resists != EFFECT_FIRE) && (resists != EFFECT_LAVA))
                applyDamage(map, xu4_random(0x7F), false);
            break;

        case EFFECT_POISONFIELD:
            /* deal 0 - 127 damage to the creature if it is not immune to poison field damage */
            if (resists != EFFECT_POISONFIELD)
                applyDamage(map, xu4_random(0x7F), false);
            break;

        case EFFECT_POISON:
        default: break;
        }
    }
}

int Creature::getAttackBonus() const {
    return 0;
}

int Creature::getDefense() const {
    return 128;
}

bool Creature::divide(Map* map) {
    int dirmask = map->getValidMoves(coords, tile);
    Direction d = dirRandomDir(dirmask);

    /* this is a game enhancement, make sure it's turned on! */
    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.slimeDivides)
        return false;

    /* make sure there's a place to put the divided creature! */
    if (d != DIR_NONE) {
        Coords new_c(coords);

        screenMessage("%s Divides!\n", CSTR(name));

        /* find a spot to put our new creature */
        map_move(new_c, d, map);

        /* create our new creature! */
        Creature * addedCreature = map->addCreature(this, new_c);
        int dividedHp = (this->hp + 1) / 2;
        addedCreature->hp = dividedHp;
        this->hp = dividedHp;
        return true;
    }
    return false;
}

bool Creature::spawnOnDeath(Map* map) {
    /* this is a game enhancement, make sure it's turned on! */
    if (!xu4.settings->enhancements ||
        !xu4.settings->enhancementsOptions.gazerSpawnsInsects)
        return false;

    /* create our new creature! */
    map->addCreature(xu4.config->creature(spawn), coords);
    return true;
}

/**
 * Hides or shows a camouflaged creature, depending on its distance from
 * the nearest opponent
 */
bool Creature::hideOrShow(Map* map) {
    /* find the nearest opponent */
    int dist;

    /* ok, now we've got the nearest party member.  Now, see if they're close enough */
    if (nearestOpponent(map, &dist, false) != NULL) {
        if ((dist < 5) && ! visible)
            visible = true;     /* show yourself */
        else if (dist >= 5)
            visible = false;    /* hide and take no action! */
    }

    return visible;
}

Creature *Creature::nearestOpponent(Map* map, int *dist, bool ranged) const {
    Creature *opponent = NULL;
    int d, leastDist = 0xFFFF;
    ObjectDeque::iterator i;
    bool jinx = (c->aura.getType() == Aura::JINX);

    for (i = map->objects.begin(); i < map->objects.end(); i++) {
        if (!isCreature(*i))
            continue;

        bool amPlayer = isPartyMember(this);
        bool fightingPlayer = isPartyMember(*i);

        /* if a party member, find a creature. If a creature, find a party member */
        /* if jinxed is false, find anything that isn't self */
        if ((amPlayer != fightingPlayer) ||
            (jinx && !amPlayer && *i != this)) {
            Coords objCoords = (*i)->coords;

            /* if ranged, get the distance using diagonals, otherwise get movement distance */
            if (ranged)
                d = map_distance(objCoords, coords);
            else
                d = map_movementDistance(objCoords, coords);

            /* skip target 50% of time if same distance */
            if (d < leastDist || (d == leastDist && xu4_random(2) == 0)) {
                opponent = dynamic_cast<Creature*>(*i);
                leastDist = d;
            }
        }
    }

    if (opponent)
        *dist = leastDist;

    return opponent;
}

void Creature::putToSleep() {
    if (getStatus() != STAT_DEAD) {
        addStatus(STAT_SLEEPING);
        animated = false;   /* freeze creature */
    }
}

/**
 * Add status effects to the creature.
 */
void Creature::addStatus(StatusType s) {
    switch (s) {
        case STAT_POISONED:
            status |= StatPoisoned;
            break;
        case STAT_SLEEPING:
            status |= StatSleeping;
            break;
        case STAT_DEAD:
            status |= StatDead;
            break;
        default:
            break;
    }
}

void Creature::removeStatus(StatusType s) {
    switch (s) {
        case STAT_POISONED:
            status &= ~StatPoisoned;
            break;
        case STAT_SLEEPING:
            status &= ~StatSleeping;
            break;
        case STAT_DEAD:
            status &= ~StatDead;
            break;
        default:
            break;
    }
}

void Creature::setStatus(StatusType s) {
    status = 0;
    addStatus(s);
}

/*
 * Return status (of highest importance).
 */
StatusType Creature::getStatus() const {
    if (status & StatDead)
        return STAT_DEAD;
    if (status & StatSleeping)
        return STAT_SLEEPING;
    if (status & StatPoisoned)
        return STAT_POISONED;
    return STAT_GOOD;
}

bool Creature::isAsleep() const {
    return status & StatSleeping;
}

bool Creature::isDead() const {
    return status & StatDead;
}

bool Creature::isDisabled() const {
    return status & (StatSleeping | StatDead);
}

void Creature::wakeUp() {
    status &= ~StatSleeping;
    animated = true;    /* reanimate creature */
}

/**
 * Applies damage to the creature.
 * Returns true if the creature still exists after the damage has been applied
 * or false, if the creature was destroyed
 *
 * If byplayer is false (when a monster is killed by walking through
 * fire or poison, or as a result of jinx) we don't report experience
 * on death
 */
bool Creature::applyDamage(Map* map, int damage, bool byplayer) {
    /* deal the damage */
    if (id != LORDBRITISH_ID)
        AdjustValueMin(hp, -damage, 0);

    const char* nameStr = CSTR(name);

    switch (getState()) {

    case MSTAT_DEAD:
        if (byplayer)
            screenMessage("%c%s Killed!%c\nExp. %d\n", FG_RED, nameStr, FG_WHITE, xp);
        else
            screenMessage("%c%s Killed!%c\n", FG_RED, nameStr, FG_WHITE);

        /*
         * the creature is dead; let it spawns something else on
         * death (e.g. a gazer that spawns insects like in u5)
         * then remove it
         */
        if (spawnsOnDeath())
            spawnOnDeath(map);

        // Remove yourself from the map
        removeFromMaps();
        return false;

    case MSTAT_FLEEING:
        screenMessage("%c%s Fleeing!%c\n", FG_YELLOW, nameStr, FG_WHITE);
        break;

    case MSTAT_CRITICAL:
        screenMessage("%s Critical!\n", nameStr);
        break;

    case MSTAT_HEAVILYWOUNDED:
        screenMessage("%s Heavily Wounded!\n", nameStr);
        break;

    case MSTAT_LIGHTLYWOUNDED:
        screenMessage("%s Lightly Wounded!\n", nameStr);
        break;

    case MSTAT_BARELYWOUNDED:
        screenMessage("%s Barely Wounded!\n", nameStr);
        break;
    }

    /* creature is still alive and has the chance to divide - xu4 enhancement */
    if (divides() && xu4_random(2) == 0)
        divide(map);

    return true;
}

bool Creature::dealDamage(Map* map, Creature *m, int damage) {
    return m->applyDamage(map, damage, isPartyMember(this));
}

//--------------------------------------
// Static Functions

/**
 * Returns a creature using a tile to find which one to create
 * or NULL if a creature with that tile cannot be found
 */
const Creature* Creature::getByTile(const MapTile& tile) {
    return xu4.config->creatureOfTile(tile.id);
}

/**
 * Returns the creature with the corresponding name (case insensitive) or
 * NULL if none is found.
 */
const Creature* Creature::getByName(const char* name) {
    uint32_t i;
    uint32_t count;
    const Creature* const* cp = xu4.config->creatureTable(&count);

    for (i = 0; i < count; ++i) {
        if (strcasecmp(CSTR(cp[i]->name), name) == 0)
            return cp[i];
    }
    return NULL;
}

/**
 * Creates a random creature based on the tile given
 */
const Creature* Creature::randomForTile(const Tile *tile) {
    /* FIXME: this is too dependent on the tile system, and easily
       broken when tileset changes are made.  Methinks the logic
       behind this should be moved to monsters.xml or another conf
       file */

    int era;
    TileId randTile;

    if (tile->isSailable()) {
        randTile = xu4.config->creature(PIRATE_ID)->tile.getId();
        randTile += xu4_random(7);
        return getByTile(randTile);
    }
    else if (tile->isSwimable()) {
        randTile = xu4.config->creature(NIXIE_ID)->tile.getId();
        randTile += xu4_random(5);
        return getByTile(randTile);
    }

    if (!tile->isCreatureWalkable())
        return NULL;

    //if (c->saveGame->moves > 100000) // FIXME: what's 100,000 moves all about (if anything)?
    if (c->saveGame->moves > 30000)
        era = 0x0f;
    else if (c->saveGame->moves > 20000)
        era = 0x07;
    else
        era = 0x03;

    randTile = xu4.config->creature(ORC_ID)->tile.getId();
    randTile += era & xu4_random(0x10) & xu4_random(0x10);
    return getByTile(randTile);
}


/**
 * Creates a random creature based on the dungeon level given
 */
const Creature* Creature::randomForDungeon(int dngLevel) {
    // Based on u4dos observations, see:
    //  https://sourceforge.net/p/xu4/patches/37/
    int adjustedDngLevel = dngLevel + 1;
    size_t range = adjustedDngLevel < 5 ? 3 : 4;
    CreatureId monster = STORM_ID + adjustedDngLevel + xu4_random(range);
    if(monster >= MIMIC_ID)
        ++monster;

    return xu4.config->creature(monster);
}


/**
 * Creates a random ambushing creature
 */
const Creature* Creature::randomAmbushing() {
    int numAmbushingCreatures = 0;
    int randCreature;
    uint32_t i;
    uint32_t count;
    const Creature* const* creatures = xu4.config->creatureTable(&count);
    const Creature* cp;

    /* first, find out how many creatures exist that might ambush you */
    for (i = 0; i < count; ++i) {
        if (creatures[i]->ambushes())
            numAmbushingCreatures++;
    }

    if (numAmbushingCreatures > 0) {
        /* now, randomely select one of them */
        randCreature = xu4_random(numAmbushingCreatures);
        numAmbushingCreatures = 0;

        /* now, find the one we selected */
        for (i = 0; i < count; ++i) {
            cp = creatures[i];
            if (cp->ambushes()) {
                /* found the creature - return it! */
                if (numAmbushingCreatures == randCreature)
                    return cp;
                else
                    /* move on to the next creature */
                    numAmbushingCreatures++;
            }
        }
    }

    ASSERT(0, "failed to find an ambushing creature");
    return NULL;
}
