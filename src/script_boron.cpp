//#include <stdio.h>
//#include <boron/boron.h>
#include "camp.h"
#include "spell.h"
#include "stats.h"
#include "utils.h"
#include "u4.h"

// These match vendors item-id.
#define SCRIPT_ITEM_CLASS(id)   (id >> 8)
#define SCRIPT_ITEM_INDEX(id)   (id & 0xff)

enum ItemClass {
    IC_MISC,
    IC_ARMOR,
    IC_WEAPON,
    IC_REAGENT,
    IC_PARTY
};

enum ItemMiscId {
    ID_FOOD,
    ID_GOLD,
    ID_TORCH,
    ID_GEM,
    ID_KEY,
    ID_SEXTANT
};

enum ItemParyId {
    ID_MEMBERS,
    ID_TRANSPORT
};

static void script_reportError(UThread* ut, const UCell* ex)
{
    UBuffer str;

    ur_strInit(&str, UR_ENC_UTF8, 0);
    ur_toText(ut, ex, &str);
    ur_strAppendChar(&str, '\n');
    ur_strTermNull(&str);

    fputs(str.ptr.c, stderr);

    ur_strFree(&str);
}

static const UCell* script_eval(UThread* ut, char* script, int len)
{
    UCell* res = boron_evalUtf8(ut, script, len);
    if (! res) {
        const UCell* ex = ur_exception(ut);
        if (ur_is(ex, UT_ERROR))
            script_reportError(ut, ex);
        boron_reset(ut);
    }
    return res;
}

/*-cf-
    game-wait
        msec    int!
    return: unset!

    Waits a given number of milliseconds.
*/
CFUNC(cf_gameWait)
{
    (void) ut;
    screenHideCursor();
    EventHandler::wait_msecs((int) ur_int(a1));
    screenShowCursor();

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    music
        id      none!/int!/word!
    return: unset!

    If id is 'reset then the default map music is played.
    If id is none! the music is stopped.
*/
CFUNC(cf_music)
{
    (void) ut;
    switch (ur_type(a1)) {
        case UT_NONE:
            musicStop();
            break;
        case UT_INT:
            musicPlay(ur_int(a1));
            break;
        case UT_WORD:           // Assuming 'reset
            musicPlayLocale();
            break;
    }
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    vo
        part    int!/coord!
    return: unset!

    Play a line from a voice stream.

    An int! part is relative to the current voice start line.
    That is set by the global 'voice variable, which is a coord! of
    (stream id, start line).

    A coord! part directly specifies the stream & line.
*/
CFUNC(cf_vo)
{
    if (ur_is(a1, UT_INT)) {
        UBuffer* ctx = ur_threadContext(ut);
        int vi = static_cast<const ConfigBoron*>(xu4.config)->voiceI;
        assert(vi >= 0);
        const UCell* voice = ur_ctxCell(ctx, vi);
        if (ur_is(voice, UT_COORD)) {
            int line = voice->coord.n[1] + ur_int(a1);
            soundSpeakLine(voice->coord.n[0] + 1, line);
        }
    }
    else if (ur_is(a1, UT_COORD)) {
        soundSpeakLine(a1->coord.n[0] + 1, a1->coord.n[1]);
    }
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    relocate
        pos coord!
    return: unset!

    Change the party position on the current map and re-center the view.
*/
CFUNC(cf_relocate)
{
    Coords* pc = &c->location->coords;
    (void) ut;

    pc->x = a1->coord.n[0];
    pc->y = a1->coord.n[1];
    if (a1->coord.len > 2)
        pc->z = a1->coord.n[2];

    gameUpdateScreen();

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    damage-pc
        who     int! Player number (1-8)
        hp      int!
    return: unset!

    Subtract hit points from a player character.
*/
CFUNC(cf_damagePc)
{
    int player = ur_int(a1);
    int hp     = ur_int(a1+1);
    (void) ut;

    if (player > 0 && hp > 0) {
        PartyMember* p = c->party->member(player - 1);
        p->applyDamage(c->location->map, hp);
    }

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    karma
        action  int!
    return: unset!

    Adjust avatar's karma level for an action.
*/
CFUNC(cf_karma)
{
    (void) ut;
    c->party->adjustKarma((KarmaAction) ur_int(a1));
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    inn-sleep
    return: unset!

    Puts the party to sleep for the night (at an Inn).
*/
CFUNC(cf_innSleep)
{
    (void) ut;
    (void) a1;
    CombatController* cc = new InnController();
    cc->beginCombat();

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    cursor
        enable  logic!
    return: unset!

    Show or hide the message area keyboard cursor.
*/
CFUNC(cf_cursor)
{
    (void) ut;
    screenShowCursor( ur_logic(a1) );
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    view-stats
        id      int!
    return: unset!

    Change status display.
*/
CFUNC(cf_viewStats)
{
    (void) ut;
    int n = ur_int(a1);
    if (n < 0 || n > STATS_MIXTURES)
        n = 0;
    c->stats->setView(StatsView(n));

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    >>
        message char!/string!
    return: unset!

    Print to the screen message area.
*/
CFUNC(cf_screenMessage)
{
    if (ur_is(a1, UT_STRING)) {
        USeriesIter si;
        ur_seriesSlice(ut, &si, a1);
        ur_strTermNull((UBuffer*) si.buf);
        screenMessage(si.buf->ptr.c + si.it);
    }
    else if (ur_is(a1, UT_CHAR)) {
        screenMessage("%c", (int) ur_char(a1));
    }
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    input-choice
        spec    string!/block!
    return: Value of selected choice or none!

    This function waits for a single character from the user.

    A spec block contains pairs of (char! value) optionally followed by a
    single default value. If the value is a word! then its bound value is
    returned.

    A spec string! contains the valid input characters and one of those will
    be returned.

    After the input is accepted a new line is emitted to the message area.

    Block specification example:

        input-choice ['1' item1 '2' item2 'a' all goodbye]
*/
CFUNC(inputChoiceS)
{
    std::string valid;
    USeriesIter si;
    UIndex start;
    char ch;

    ur_seriesSlice(ut, &si, a1);
    start = si.it;

    if (! ur_strIsUcs2(si.buf)) {
        const uint8_t* cp = si.buf->ptr.b;
        ur_foreach(si)
            valid += cp[si.it];

        valid += " \015\033";   // Space, CR, ESC.
        ch = EventHandler::readChoice(valid.c_str());
        screenCrLf();

        si.it = start;
        ur_foreach(si) {
            if (ch == cp[si.it]) {
                ur_setId(res, UT_CHAR);
                ur_char(res) = ch;
                return UR_OK;
            }
        }
    }

    ur_setId(res, UT_NONE);
    return UR_OK;
}

CFUNC(inputChoiceB)
{
    std::string valid;
    UBlockIt bi;
    const UCell* start;
    char ch;

    ur_blockIt(ut, &bi, a1);
    start = bi.it;

    // Append all char! to valid string.
    ur_foreach(bi) {
        if (ur_is(bi.it, UT_CHAR))
            valid += ur_char(bi.it);
    }

    valid += " \015\033";   // Space, CR, ESC.
    ch = EventHandler::readChoice(valid.c_str());
    screenCrLf();

    bi.it = start;
    ur_foreach(bi) {
        if (ur_is(bi.it, UT_CHAR) && ch == ur_char(bi.it)) {
            ++bi.it;
            if (bi.it == bi.end)
                return ur_error(ut, UR_ERR_SCRIPT,
                                "input-choice expected block after '%c'", ch);
            goto found;
        }
    }

    // No match for input so use default value if present.
    if ((bi.end - start) & 1) {
        bi.it = bi.end - 1;
        goto found;
    }

    ur_setId(res, UT_NONE);
    return UR_OK;

found:
    if (ur_is(bi.it, UT_WORD)) {
        if (! (bi.it = ur_wordCell(ut, bi.it)))
            return UR_THROW;
    }
    *res = *bi.it;
    return UR_OK;
}

CFUNC(cf_inputChoice)
{
    if (ur_is(a1, UT_BLOCK))
        return inputChoiceB(ut, a1, res);
    return inputChoiceS(ut, a1, res);
}

/*-cf-
    input-number
        limit       int!  Maximum number of digits.
    return: none!/int!

    Request integer from user.  If the number is zero or an escape key was
    pressed then none! is returned.

    After the input is accepted a new line is emitted to the message area.
*/
CFUNC(cf_inputNumber)
{
    (void) ut;
    int maxLen = ur_int(a1);
    if (maxLen <= 0)
        maxLen = 7;     //Conversation::BUFFERLEN;
    int val = EventHandler::readInt(maxLen);
    screenCrLf();

    if (val) {
        ur_setId(res, UT_INT);
        ur_int(res) = val;
    }
    else
        ur_setId(res, UT_NONE);
    return UR_OK;
}

/*-cf-
    input-text
        limit       int!  Maximum number of characters
    return: none!/string!
*/
CFUNC(cf_inputText)
{
    int maxLen = ur_int(a1);
    if (maxLen <= 0)
        maxLen = TEXT_AREA_W-2;
    string str = EventHandler::readString(maxLen);
    screenCrLf();

    if (str.empty()) {
        ur_setId(res, UT_NONE);
    } else {
        // TODO: Reuse a single string! buffer.
        UBuffer* buf = ur_makeStringCell(ut, UR_ENC_LATIN1, str.size(), res);
        ur_strAppendCStr(buf, str.c_str());
        ur_strLowercase(buf, 0, buf->used);
    }
    return UR_OK;
}

/*-cf-
    input-player
    return: none!/int!

    Request player id from user.  If the number is zero or an escape key was
    pressed then none! is returned.

    After the input is accepted a new line is emitted to the message area.
*/
CFUNC(cf_inputPlayer)
{
    int player;
    (void) ut;
    (void) a1;

    player = EventHandler::choosePlayer();
    screenCrLf();

    if (player != -1) {
        ur_setId(res, UT_INT);
        ur_int(res) = player + 1;
    }
    else
        ur_setId(res, UT_NONE);
    return UR_OK;
}

/*-cf-
    party
        'attr        word!
    return: Specified value.

    Get a party attribute (food, gold, keys, members, or transport).
*/
CFUNC(cf_party)
{
    int id = xu4.config->scriptItemId(ur_atom(a1));
    int ic = SCRIPT_ITEM_CLASS(id);
    int n = 0;

    id = SCRIPT_ITEM_INDEX(id);

    if (ic == IC_MISC) {
        switch (id) {
            case ID_FOOD:
                n = c->saveGame->food;
                break;
            case ID_GOLD:
                n = c->saveGame->gold;
                break;
            case ID_TORCH:
                n = c->saveGame->torches;
                break;
            case ID_GEM:
                n = c->saveGame->gems;
                break;
            case ID_KEY:
                n = c->saveGame->keys;
                break;
            case ID_SEXTANT:
                n = c->saveGame->sextants;
                break;
        }
    } else if (ic == IC_ARMOR) {
        n = c->saveGame->armor[id];
    } else if (ic == IC_WEAPON) {
        n = c->saveGame->weapons[id];
    } else if (ic == IC_REAGENT) {
        n = c->saveGame->reagents[id];
    } else if (ic == IC_PARTY) {
        switch (id) {
            case ID_MEMBERS:
                n = c->party->size();
                break;
            case ID_TRANSPORT:
            {
                UAtom atom = 0;
                TransportContext tc = c->transportContext;
                if (tc & TRANSPORT_HORSE)
                    atom = Tile::sym.horse;
                else if (tc & TRANSPORT_SHIP)
                    atom = Tile::sym.ship;
                else if (tc & TRANSPORT_BALLOON)
                    atom = Tile::sym.balloon;

                if (atom) {
                    ur_setId(res, UT_WORD);
                    ur_setWordUnbound(res, atom);
                } else
                    ur_setId(res, UT_NONE);     // TRANSPORT_FOOT
            }
                return UR_OK;
            default:
                return ur_error(ut, UR_ERR_SCRIPT,
                                "Invalid party attribute '%s", ur_wordCStr(a1));
        }
    }

    ur_setId(res, UT_INT);
    ur_int(res) = n;
    return UR_OK;
}

/*-cf-
    pc-attr
        who     int! Player number (1-8)
        'attr   word!
    return: Specified value.

    Get a player character attribute (hp, mp, xp, dex, int, str, name).
*/
CFUNC(cf_pcAttr)
{
    int player = ur_int(a1);
    const char* attr = ur_wordCStr(a1+1);
    PartyMember* pm;
    int n;

    if (player < 1) {
        ur_setId(res, UT_NONE);
        return UR_OK;
    }
    pm = c->party->member(player - 1);

    switch (attr[0]) {
        case 'd':
            n = pm->getDex();       // dex
            break;
        case 'h':
            n = pm->getHp();        // hp
            break;
        case 'i':
            n = pm->getInt();       // int
            break;
        case 'm':
            n = pm->getMp();        // mp
            break;
        case 'n':
        {
            string str = pm->getName();     // name
            UBuffer* buf =
                ur_makeStringCell(ut, UR_ENC_LATIN1, str.size(), res);
            ur_strAppendCStr(buf, str.c_str());
        }
            return UR_OK;
        case 's':
            n = pm->getStr();       // str
            break;
        case 'x':
            n = pm->getExp();       // xp
            break;
        default:
            return ur_error(ut, UR_ERR_SCRIPT,
                            "Invalid PC attribute '%s", attr);
    }

    ur_setId(res, UT_INT);
    ur_int(res) = n;
    return UR_OK;
}

/*-cf-
    pc-needs?
        who     int! Player number (1-8)
        need    word! (cure, heal, fullheal, resurrect)
    return: logic!

    Determine if a player character has a specific need.
*/
CFUNC(cf_pcNeedsQ)
{
    int player = ur_int(a1);
    const char* need = ur_wordCStr(a1+1);
    PartyMember* pm;
    int needs = 0;

    if (player < 1) {
        ur_setId(res, UT_NONE);
        return UR_OK;
    }
    pm = c->party->member(player - 1);

    switch (need[0]) {
        case 'c':
            if (pm->getStatus() == STAT_POISONED)   // cure
                needs = 1;
            break;
        case 'f':
        case 'h':
            if (pm->getHp() < pm->getMaxHp())       // fullheal, heal
                needs = 1;
            break;
        case 'r':
            if (pm->isDead())       // resurrect
                needs = 1;
            break;
        default:
            return ur_error(ut, UR_ERR_SCRIPT, "Invalid PC need '%s", need);
    }

    ur_setId(res, UT_LOGIC);
    ur_logic(res) = needs;
    return UR_OK;
}

extern SpellEffectCallback spellEffectCallback;

/*-cf-
    pc-heal
        who         int! Player number (1-8)
        remedy      word! (cure, heal, fullheal, resurrect)
        /magic      Heal by magicial means
    return: unset!

    Heals a player character with a specific remedy.
*/
CFUNC(cf_pcHeal)
{
    int player = ur_int(a1);
    const char* remedy = ur_wordCStr(a1+1);
    HealType type;

    if (player > 0) {
        if (CFUNC_OPTIONS & 1) {
            screenHideCursor();
            (*spellEffectCallback)('r', -1, SOUND_MAGIC);
            screenShowCursor();
       }

        switch (remedy[0]) {
            case 'c': type = HT_CURE;       break;
            case 'f': type = HT_FULLHEAL;   break;
            case 'h': type = HT_HEAL;       break;
            case 'r': type = HT_RESURRECT;  break;
            default:
                return ur_error(ut, UR_ERR_SCRIPT,
                                "Invalid PC remedy '%s", remedy);
        }

        PartyMember* pm = c->party->member(player - 1);
        pm->heal(type);
    }
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    pay
        price       int!
        quant       int!
        purchase    block!
        broke       block!
    return: Result of purchase or broke code.
*/
CFUNC(cf_pay)
{
    const UCell* code;
    int price = ur_int(a1);
    int quant = ur_int(a1+1);

    price *= quant;
    if (price > c->saveGame->gold) {
        code = a1+3;
    } else {
        c->party->adjustGold(-price);
        code = a1+2;
    }
#if BORON_VERSION > 0x020008
    return (UStatus) boron_reframeDoBlock(ut, code->series.buf, res, 0);
#else
    return boron_doBlock(ut, code, res);
#endif
}

/*-cf-
    add-item
        item    word!   (horse)
    return: unset!

    Add an item to the party inventory.
*/
CFUNC(cf_addItem)
{
    Party* p = c->party;
    UAtom atom = ur_atom(a1);
    (void) ut;

    if (atom == Tile::sym.horse)
        p->setTransport(Tileset::findTileByName(atom)->getId());

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    add-items
        item    word!  (food, gold, torch, gem, key, sextant)
        amount  int!
    return: unset!
    see: add-item, remove-items

    Add a quantity of items to the party inventory.
*/
CFUNC(cf_addItems)
{
    int amount = ur_int(a1+1);
    int id = xu4.config->scriptItemId(ur_atom(a1));
    int ic = SCRIPT_ITEM_CLASS(id);
    (void) ut;

    id = SCRIPT_ITEM_INDEX(id);

    if (ic == IC_ARMOR) {
        AdjustValueMax(c->saveGame->armor[id], amount, 99);
    } else if (ic == IC_WEAPON) {
        AdjustValueMax(c->saveGame->weapons[id], amount, 99);
    } else if (ic == IC_REAGENT) {
        AdjustValueMax(c->saveGame->reagents[id], amount, 99);
        c->stats->resetReagentsMenu();
    } else if (ic == IC_MISC) {
        switch (id) {
            case ID_FOOD:
                c->party->adjustFood(amount * 100);
                goto skip_notify;
            case ID_GOLD:
                c->party->adjustGold(amount);
                goto skip_notify;
            case ID_TORCH:
                AdjustValueMax(c->saveGame->torches, amount, 99);
                break;
            case ID_GEM:
                AdjustValueMax(c->saveGame->gems, amount, 99);
                break;
            case ID_KEY:
                AdjustValueMax(c->saveGame->keys, amount, 99);
                break;
            case ID_SEXTANT:
                AdjustValueMax(c->saveGame->sextants, amount, 99);
                break;
            default:
                goto skip_notify;
        }
    }

    c->party->notifyOfChange(0, PartyEvent::INVENTORY_ADDED);

skip_notify:
    ur_setId(res, UT_UNSET);
    return UR_OK;
}

/*-cf-
    remove-items
        item    word!  Gold, armor, weapons & reagents only.
        amount  int!
    return: unset!
    see: add-items

    Remove a quantity of items from the party inventory.
*/
CFUNC(cf_removeItems)
{
    int amount = -ur_int(a1+1);
    int id = xu4.config->scriptItemId(ur_atom(a1));
    int ic = SCRIPT_ITEM_CLASS(id);
    (void) ut;

    id = SCRIPT_ITEM_INDEX(id);

    if (ic == IC_ARMOR)
        AdjustValueMin(c->saveGame->armor[id], amount, 0);
    else if (ic == IC_WEAPON)
        AdjustValueMin(c->saveGame->weapons[id], amount, 0);
    else if (ic == IC_REAGENT)
        AdjustValueMin(c->saveGame->reagents[id], amount, 0);
    else if (ic == IC_MISC) {
        if (id == ID_GOLD)
            c->party->adjustGold(amount);
    }

    ur_setId(res, UT_UNSET);
    return UR_OK;
}

static const BoronCFunc pfFuncs[] = {
    cf_gameWait,
    cf_music,
    cf_vo,
    cf_relocate,
    cf_damagePc,
    cf_karma,
    cf_innSleep,
    cf_cursor,
    cf_viewStats,
    cf_screenMessage,
    cf_inputChoice,
    cf_inputNumber,
    cf_inputText,
    cf_inputPlayer,
    cf_party,
    cf_pcAttr,
    cf_pcNeedsQ,
    cf_pcHeal,
    cf_pay,
    cf_addItem,
    cf_addItems,
    cf_removeItems
};

static const char pfFuncSpecs[] =
    "game-wait n int!\n"
    "music n\n"
    "vo n\n"
    "relocate a coord!\n"
    "damage-pc n int! hp int!\n"
    "karma n int!\n"
    "inn-sleep\n"
    "cursor n logic!\n"
    "view-stats n int!\n"
    ">> text\n"
    "input-choice spec string!/block!\n"
    "input-number limit int!\n"
    "input-text limit int!\n"
    "input-player\n"
    "party a word!\n"
    "pc-attr n int! 'a word!\n"
    "pc-needs? n int! a word!\n"
    "pc-heal n int! a word! /magic\n"
    "pay price int! quant int! a block! b block!\n"
    "add-item item word!\n"
    "add-items item word! n int!\n"
    "remove-items item word! n int!\n";

static void enumItems(UBuffer* ctx, const UAtom* atoms,
                      int itemClass, int itemIndex, int count )
{
    UCell* cell = ctx->ptr.cell + ctx->used;
    int id = (itemClass << 8) | itemIndex;
    while (count) {
        ur_ctxAppendWord(ctx, *atoms++);
        ur_setId(cell, UT_INT);
        ur_int(cell) = id++;
        ++cell;
        --count;
    }
}

/*
 * \param blkC   Vendor scripts block.
 *
 * \return Index of item-id context.
 */
static UIndex script_init(UThread* ut, const UCell* blkC)
{
    boron_defineCFunc(ut, UR_MAIN_CONTEXT, pfFuncs, pfFuncSpecs,
                      sizeof(pfFuncSpecs)-1);

    boron_bindDefault(ut, blkC->series.buf);
#if BORON_VERSION > 0x020008
    if (boron_evalBlock(ut, blkC->series.buf, ur_stackTop(ut)) != UR_OK)
#else
    if (boron_doBlock(ut, blkC, ur_stackTop(ut)) != UR_OK)
#endif
    {
        const UCell* ex = ur_exception(ut);
        if (ur_is(ex, UT_ERROR))
            script_reportError(ut, ex);
        errorFatal("Vendors script error");
    }

    // Create a context which will map item names to internal script ids.
    // This simplifies the implementation of party, add-items & remove-items.

    const int icount = 38;
    UAtom atoms[icount];
    UIndex itemIdN = ur_makeContext(ut, icount);
    ur_hold(itemIdN);   // Hold forever.
    ur_internAtoms(ut,
        "food gold torch gem key\n"
        "sextant\n"

        "cloth leather chain plate magic-chain\n"
        "magic-plate mystic-robe\n"

        "staff dagger sling mace axe\n"
        "sword bow crossbow oil halberd\n"
        "magic-axe magic-sword magic-bow magic-wand mystic-sword\n"

        "ash ginseng garlic silk moss\n"
        "pearl nightshade mandrake\n"

        "members transport\n",
        atoms);

    UBuffer* ctx = ur_buffer(itemIdN);
    enumItems(ctx, atoms,    IC_MISC,    0,  6);
    enumItems(ctx, atoms+6,  IC_ARMOR,   1,  7);
    enumItems(ctx, atoms+13, IC_WEAPON,  1, 15);
    enumItems(ctx, atoms+28, IC_REAGENT, 0,  8);
    enumItems(ctx, atoms+36, IC_PARTY,   0,  2);
    ur_ctxSort(ctx);
    assert(ctx->used == icount);

    return itemIdN;
}
