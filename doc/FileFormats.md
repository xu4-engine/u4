Ultima 4 File Structures
========================

Please send additions, corrections and feedback to
<andrewtaylor@users.sourceforge.net>!


SHAPES.EGA
----------

This contains the bitmaps for all 256 tiles.  Each tile is 16x16 pixels and
each byte represents two pixels.  This means that the first 8 bytes represent
the 16 pixels on the first row of the first tile.  Each tile is 128 bytes.

Offset | Tile  | Description
-------|:-----:|----------------
0x0000 |   0   | Deep Water
0x0080 |   1   | Medium Water
0x0100 |   2   | Shallow Water
0x0180 |   3   | Swamp
0x0200 |   4   | Grasslands
0x0280 |   5   | Scrubland
0x0300 |   6   | Forest
0x0380 |   7   | Hills
0x0400 |   8   | Mountains
0x0480 |   9   | Dungeon Entrance
0x0500 |  10   | Town
0x0580 |  11   | Castle
0x0600 |  12   | Village
0x0680 |  13   | Lord British's Castle West
0x0700 |  14   | Lord British's Castle Entrance
0x0780 |  15   | Lord British's Castle East
0x0800 |  16   | Ship West
0x0880 |  17   | Ship North
0x0900 |  18   | Ship East
0x0980 |  19   | Ship South
0x0a00 |  20   | Horse West
0x0a80 |  21   | Horse East
0x0b00 |  22   | Tile Floor
0x0b80 |  23   | Bridge
0x0c00 |  24   | Balloon
0x0c80 |  25   | Bridge North
0x0d00 |  26   | Bridge South
0x0d80 |  27   | Ladder Up
0x0e00 |  28   | Ladder Down
0x0e80 |  29   | Ruins
0x0f00 |  30   | Shrine
0x0f80 |  31   | Avatar
0x1000 | 32-33 | Mage (2)
0x1100 | 34-35 | Bard (2)
0x1200 | 36-37 | Fighter (2)
0x1300 | 38-39 | Druid (2)
0x1400 | 40-41 | Tinker (2)
0x1500 | 42-43 | Paladin (2)
0x1600 | 44-45 | Ranger (2)
0x1700 | 46-47 | Shepherd (2)
0x1800 |  48   | Column
0x1880 |  49   | White SW
0x1900 |  50   | White SE
0x1980 |  51   | White NW
0x1a00 |  52   | White NE
0x1a80 |  53   | Mast
0x1b00 |  54   | Ship's Wheel
0x1b80 |  55   | Rocks
0x1c00 |  56   | Lyin Down
0x1c80 |  57   | Stone Wall
0x1d00 |  58   | Locked Door
0x1d80 |  59   | Unlocked Door
0x1e00 |  60   | Chest
0x1e80 |  61   | Ankh
0x1f00 |  62   | Brick Floor
0x1f80 |  63   | Wooden Planks
0x2000 | 64-66 | Moongate Opening (3)
0x2180 |  67   | Moongate Open
0x2200 |  68   | Poison Field
0x2280 |  69   | Energy Field
0x2300 |  70   | Fire Field
0x2380 |  71   | Sleep Field
0x2400 |  72   | Solid Barrier
0x2480 |  73   | Hidden Passage
0x2500 |  74   | Altar
0x2580 |  75   | Spit
0x2600 |  76   | Lava Flow
0x2680 |  77   | Missile
0x2700 |  78   | Magic Sphere
0x2780 |  79   | Attack Flash
0x2800 | 80-81 | Guard (2)
0x2900 | 82-83 | Citizen (2)
0x2a00 | 84-85 | Singing Bard (2)
0x2b00 | 86-87 | Jester (2)
0x2c00 | 88-89 | Beggar (2)
0x2d00 | 90-91 | Child (2)
0x2e00 | 92-93 | Bull (2)
0x2f00 | 94-95 | Lord British (2)
0x3000 | 96-121| Letters A-Z
0x3d00 | 122   | Space
0x3d80 | 123   | Right
0x3e00 | 124   | Left
0x3e80 | 125   | Window
0x3f00 | 126   | Blank
0x3f80 | 127   | Brick Wall
0x4000 | 128   | Pirate Ship West
0x4080 | 129   | Pirate Ship North
0x4100 | 130   | Pirate Ship East
0x4180 | 131   | Pirate Ship South
0x4200 |132-133| Nixie (2)
0x4300 |134-135| Giant Squid (2)
0x4400 |136-137| Sea Serpent (2)
0x4500 |138-139| Seahorse (2)
0x4600 |140-141| Whirlpool (2)
0x4700 |142-143| Storm (2)
0x4800 |144-147| Rat (4)
0x4a00 |148-151| Bat (4)
0x4c00 |152-156| Giant Spider (4)
0x4e00 |156-159| Ghost (4)
0x5000 |160-163| Slime (4)
0x5200 |164-167| Troll (4)
0x5400 |168-171| Gremlin (4)
0x5600 |172-175| Mimic (4)
0x5800 |176-179| Reaper (4)
0x5a00 |180-183| Insect Swarm (4)
0x5c00 |184-187| Gazer (4)
0x5e00 |188-191| Phantom (4)
0x6000 |192-195| Orc (4)
0x6200 |196-199| Skeleton (4)
0x6400 |200-203| Rogue (4)
0x6600 |204-207| Python (4)
0x6800 |208-211| Ettin (4)
0x6a00 |212-215| Headless (4)
0x6c00 |216-219| Cyclops (4)
0x6e00 |220-223| Wisp (4)
0x7000 |224-227| Evil Mage (4)
0x7200 |228-231| Lich (4)
0x7400 |232-235| Lava Lizard (4)
0x7600 |236-239| Zorn (4)
0x7800 |240-243| Daemon (4)
0x7a00 |244-247| Hydra (4)
0x7c00 |248-251| Dragon (4)
0x7e00 |252-255| Balron (4)


CHARSET.EGA
-----------

This contains the bitmaps for each character in the font.  Each
character is 8x8 pixels, and each byte represents two pixels.  In
other words, the first 4 bytes represent the 8 pixels on the first row
of the first character.  Each glyph is 32 bytes.

The order is the standard ASCII mapping; 'A' is character 65, 'z' is
character 122.  The lowest 32 characters have the following glyphs:

Num | Description
:--:|------------------------
 0  | Ankh
 1  | Symbol for horn effect
 2  | Brick wall 1
 3  | Brick wall 2
 4  | Updown arrow
 5  | Down arrow
 6  | Up arrow
 7  | Partial ankh
 8  | Filled circle
 9  | Copyright
10  | Registered trademark
11  | Male symbol
12  | Female symbol
13  | Border bar
14  | Hollow square
15  | Blue dot
16  | Border endpiece (right)
17  | Border endpiece (left)
18  | (empty)
19  | Ellipsis
20  | Moon phase 0
21  | Moon phase 1
22  | Moon phase 2
23  | Moon phase 3
24  | Moon phase 4
25  | Moon phase 5
26  | Moon phase 6
27  | Moon phase 7
28  | Cursor image 1
29  | Cursor image 2
30  | Cursor image 3
31  | Cursor image 4
... | (standard ASCII)
127 | Up Indicator?
128 | Mini palette?


.EGA files (RLE encoded)
------------------------

The following .EGA files are RLE encoded: START.EGA, KEY7.EGA,
RUNE_x.EGA (where x is between 0 and 5), STONCRCL.EGA, HONESTY.EGA,
COMPASSN.EGA, VALOR.EGA, JUSTICE.EGA, SACRIFIC.EGA, HONOR.EGA,
SPIRIT.EGA, HUMILITY.EGA, TRUTH.EGA, LOVE.EGA, and COURAGE.EGA.  Each
of these files represents a 16 color 320x200 bitmap.  Each pixel is
encoded in 4 bits, like SHAPES.EGA and CHARSET.EGA.  However, runs of
identical *bytes* (not pixels) are encoded with a special run
indicator value (0x02) followed by a one byte count and a one byte
value.  This allows a single run of up to 255 bytes (510 pixels) to be
represented as three bytes.  For example, the byte values "28 28 28
28" could be represented as "02 04 28".  If the value 0x02 needs to be
represented, it *must* be encoded in a run ("02 01 02").

File         | Purpose
-------------|----------------------------------
START.EGA    | The basic game borders
KEY7.EGA     | Shown when the key is used
RUNE_0.EGA   | Vision shown after meditating on valor
RUNE_1.EGA   | Vision shown after meditating on honesty, justice, and honor
RUNE_2.EGA   | Vision shown after meditating on compassion and sacrifice
RUNE_3.EGA   | Vision shown after meditating on spirituality
RUNE_4.EGA   | Vision shown after meditating on humility
RUNE_5.EGA   | Vision after answering the final riddle in the Stygian Abyss
STONCRCL.EGA | Shown when game completed
HONESTY.EGA  | Shown during questioning in abyss
COMPASSN.EGA | Shown during questioning in abyss
VALOR.EGA    | Shown during questioning in abyss
JUSTICE.EGA  | Shown during questioning in abyss
SACRIFIC.EGA | Shown during questioning in abyss
HONOR.EGA    | Shown during questioning in abyss
SPIRIT.EGA   | Shown during questioning in abyss
HUMILITY.EGA | Shown during questioning in abyss
TRUTH.EGA    | Shown during questioning in abyss
LOVE.EGA     | Shown during questioning in abyss
COURAGE.EGA  | Shown during questioning in abyss


.EGA files (LZW encoded)
------------------------

The remaining .EGA files are LZW encoded: ABACUS.EGA, ANIMATE.EGA,
GYPSY.EGA, INSIDE.EGA, OUTSIDE.EGA, PORTAL.EGA, TREE.EGA, WAGON.EGA,
HONCOM.EGA, SACHONOR.EGA, SPIRHUM.EGA, VALJUS.EGA, and TITLE.EGA.
Also, the file SHAPES.EGZ is a LZW compressed version of SHAPES.EGA.
Like the RLE encoded files, each of these files represents a 16 color
320x200 bitmap.  LZW encoded files only seem to be used in the
introduction sequence (TITLE.EXE); the images used by AVATAR.EXE are
RLE encoded instead.

For more details on the LZW algorithm see [Lempel-Ziv-Welch].

File         | Purpose
-------------|----------------------------------------
TITLE.EGA    | The background for the inital screen, including the large Ultima IV and the border for the map/menu
ANIMATE.EGA  | The frames for the little critters in the upper corners of the intro screen
TREE.EGA     | Initial graphic for character creation sequence (has animated moongate)
PORTAL.EGA   | Closeup of the moongate
OUTSIDE.EGA  | The faire from outside
INSIDE.EGA   | Inside the faire
WAGON.EGA    | The gyspy's wagon
GYPSY.EGA    | The gypsy herself
ABACUS.EGA   | The abacus that serves as the background while the questions are asked
HONCOM.EGA   | The cards for honor and compassion
VALJUS.EGA   | The cards for valor and justice
SACHONOR.EGA | The cards for sacrifice and honor
SPIRHUM.EGA  | The cards for spirituality and humility


WORLD.MAP
---------

This is the map of Britannia. It is 256x256 tiles in total, broken up
into 64 32x32 chunks.  The first chunk is in the top left corner, the
next just below, and so on, until the last in the bottom right corner.
Each tile is stored as a byte that maps to the tiles in SHAPES.EGA.

The "chunked" layout is an artifact of the limited memory on the
original machines that ran Ultima 4.  The whole map would take 64k,
too much for a C64 or an Apple II, so the game would keep a limited
number of 1k chunks in memory at a time.  As the player moved around,
old chunks were thrown out as new ones were swapped in.

Offset | Bytes | Purpose
-------|-------|------------------------------
0x0000 | 1024  | 32x32 map matrix for chunk 0
0x0400 | 1024  | 32x32 map matrix for chunk 1
...    |       |
0xFC00 | 1024  | 32x32 map matrix for chunk 63


.ULT files
----------

These contain town information.  Specifically, a 32x32 map, plus the
starting position, movement behavior, and conversation index of each
NPC.  The conversation index gives the starting position of the NPC's
dialog block in the corresponding .TLK file.  I'm not sure why some of
the data is duplicated.

Offset | Bytes | Purpose
-------|:-----:|------------------------------
0x0000 | 1024  | 32x32 town map matrix
0x0400 |   32  | Tile for NPCs 0-31
0x0420 |   32  | Start_x for NPCs 0-31
0x0440 |   32  | Start_y for NPCs 0-31
0x0460 |   32  | Repetition of 0x400-0x41F
0x0480 |   32  | Repetition of 0x420-0x43F
0x04A0 |   32  | Repetition of 0x440-0x45F
0x04C0 |   32  | Movement_behavior for NPCs 0-31 (0x0=Fixed, 0x1=Wander, 0x80=Follow, 0xFF=Attack)
0x04E0 |   32  | Conversion index (in corresponding .TLK file) for NPCs 0-31


.TLK files
----------

These contain conversation information for up to 16 NPCs.
Each NPC has a 288 (0x120) byte block containing his or her dialogues, plus
the words that trigger his or her responses (keywords).  Most of the fields
are variable length strings terminated by a single zero byte and the following
field starts immediately after.  The blocks are padded with zero bytes to
288 bytes.

Offset | Bytes  | Purpose
-------|:------:|-------------------------------
0x0    |   1    | Question Trigger (0=None, 3=Job, 4=Health, 5=Keyword 1, 6=Keyword 2)
0x1    |   1    | Does Response Affect Humility? (0=No, 1=Yes)
0x2    |   1    | Probability of Turning Away (out of 256)
0x3    | Varies | Name
Varies | Varies | Pronoun
Varies | Varies | Description
Varies | Varies | Job response
Varies | Varies | Health response
Varies | Varies | Response to Keyword 1
Varies | Varies | Response to Keyword 2
Varies | Varies | Yes/No Question
Varies | Varies | Yes response
Varies | Varies | No response
Varies |   5    | Keyword 1
Varies |   5    | Keyword 2
Varies | Varies | Zero padding
0x120  | 288    | Next NPC dialogue...


.CON files
----------

These files contain the 11x11 battleground maps shown when combat starts.
It has the map itself plus starting positions for up to 16 monsters and
8 party members.

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    |  16   | start_x for monsters 0-15
0x10   |  16   | start_y for monsters 0-15
0x20   |   8   | start_x for party members 0-7
0x28   |   8   | start_y for party members 0-7
0x30   |  16   | ??? constant (08AD 83C0 AD83 C0AD 83C0 A000 B9A6 08F0)
0x40   | 121   | 11x11 map matrix
0xB9   |   7   | ??? constant (8D 0000 0000 B709)


.DNG files
----------

These files contain dungeon information.  The first 512 bytes are the
8x8 maps for each of the 8 levels.  These are used for the dungeons 3D
mode.  The rest of the file is a set of 16 (64 in the case of the
ABYSS.DNG) 256 byte blocks that define the dungeon rooms.

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    |   64  | Level 1 8x8 map matrix
0x40   |   64  | Level 2 8x8 map matrix
0x80   |   64  | Level 3 8x8 map matrix
0xC0   |   64  | Level 4 8x8 map matrix
0x100  |   64  | Level 5 8x8 map matrix
0x140  |   64  | Level 6 8x8 map matrix
0x180  |   64  | Level 7 8x8 map matrix
0x1C0  |   64  | Level 8 8x8 map matrix
0x200  |  256  | Room 0 data
0x300  |  256  | Room 1 data
0x400  |  256  | Room 2 data
...    |       |
0x1100 |  256  | Room 15 data

For ABYSS.DNG only:

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x1200 |  256  | Room 16 data
0x1300 |  256  | Room 17 data
...    |       |
0x4100 |  256  | Room 63 data

Dungeon Room format:

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    |   16  | Floor triggers (4 bytes each X 4 triggers possible)
0x10   |   16  | Tile for monsters 0-15 (0 means no monster and 0's come FIRST)
0x20   |   16  | start_x for monsters 0-15
0x30   |   16  | start_y for monsters 0-15
0x40   |    8  | start_x for party member 0-7 (north entry)
0x48   |    8  | start_y for party member 0-7 (north entry)
0x50   |    8  | start_x for party member 0-7 (east entry)
0x58   |    8  | start_y for party member 0-7 (east entry)
0x60   |    8  | start_x for party member 0-7 (south entry)
0x68   |    8  | start_y for party member 0-7 (south entry)
0x70   |    8  | start_x for party member 0-7 (west entry)
0x78   |    8  | start_y for party member 0-7 (west entry)
0x80   |  121  | 11x11 map matrix for room
0xF9   |    7  | Buffer

Trigger format:

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    |  1    | Tile to be placed (0 means no trigger and 0's come LAST)
0x1    |  1    | 2 nibbles indicating the (x,y) coords of trigger
0x2    |  1    | 2 nibbles indicating the (x,y) coords of 1st tile to change
0x3    |  1    | 2 nibbles indicating the (x,y) coords of 2nd tile to change

TRIGGER EXAMPLE:
46 85 75 65 - Places a Fire Field at (7, 5) and (6, 5) when you step on (8, 5).


PARTY.SAV
---------

This file stores the information that is stored when the game is
saved.  It includes party-wide information like gold and food, plus a
39 byte record for each character.

```
Offset Len  Notes
0x0    4    counter (incremented regularly, probably every screen update)
0x4    2    moves (low word)
0x6    2    moves (high word)
0x8    39   player 0 record
0x2F   39   player 1 record
0x56   39   player 2 record
0x7D   39   player 3 record
0xA4   39   player 4 record
0xCB   39   player 5 record
0xF2   39   player 6 record
0x119  39   player 7 record
0x140  4    food (in units of hundredths)
0x144  2    gold
0x146  16   karma (2 bytes each for honesty, compassion, valor, justice,
            sacrifice, honor, spirituality and humility; 0-partial avatar)
0x156  2    torches
0x158  2    gems
0x15A  2    keys
0x15C  2    sextants
0x15E  16   armor (2 bytes each for none (unused), cloth, leather, chain,
            plate, magic chain, magic plate, mystic robes)
0x16E  32   weapons (2 bytes each for none (unused), staves, daggers,
            slings, maces, axes, swords, bows, crossbows, oil, halberds,
            magic axes, magic swords, magic bows, magic wands, mystic
            swords)
0x18E  16   reagents (2 bytes each for sulfurous ash, ginseng, garlic,
            spider silk, blood moss, black pearl, nightshade, mandrake)
0x19E  52   mixtures (2 bytes each for awaken, blink, cure, dispel,
            energy, fireball, gate, heal, iceball, jinx, kill, light,
            magic missile, negate, open, protection, quickness,
            resurrect, sleep, tremor, undead, view, winds, x-it, y-up,
            z-down)
0x1D2  1    items (1 bit each for skull, skull destroyed, candle, book,
            bell, key part C, key part L, key part T)
0x1D3  1    items (1 bit each for horn, wheel, candle used at abyss 
            entrance, book used at abyss entrance, bell used at abyss
            entrance, (3 bits unused))
0x1D4  1    x coord
0x1D5  1    y coord
0x1D6  1    stones (1 bit each for BYRGOPWB)
0x1D7  1    runes (1 bit each for HCVJSHSH)
0x1D8  2    number of characters in party
0x1DA  2    transport (0x10-ship facing west, 0x11-ship facing north,
            0x12-ship facing east, 0x13-ship facing south, 0x14-horse
            facing west, 0x15-horse facing east, 0x18-balloon, 0x1f-on
            foot)
0x1DC  2    on surface: balloon status (0-on ground, 1-flying)
            in dungeon: current torch duration
0x1DE  2    current phase of left moon (Trammel); range = 0-7
0x1E0  2    current phase of right moon (Felucca); range = 0-7
0x1E2  2    hull integrity of currently boarded ship
            if you x-it a ship, save, restart the game, and board the
            ship again, you will find that all damage to the ship has
            been mysteriously repaired.
0x1E4  2    introduced to Lord British (0-haven't been introduced, 1-introduced)
0x1E6  2    time of last successful hole up & camp
            The game stores ((moves / 0x64) & 0xffff) in this field when
            you successfully hole up & camp.
            If (moves / 0x64) < 0x10000:
            You can only hole up & camp successfully if the current
            ((moves / 0x64) & 0xffff) is different from the value in this
            field.
            If (moves / 0x64) >= 0x10000:
            Hole up & camp is always successful.
0x1E8  2    time of last mandrake/nightshade find
            The game stores (moves & 0xf0) in this field when you find
            mandrake/nightshade. You can only find mandrake/nightshade
            if the current (moves & 0xf0) is different from the value
            in this field.
0x1EA  2    time of last successful meditation at a shrine
            The game stores ((moves / 0x64) & 0xffff) in this field when
            you successfully meditate at a shrine.
            If (moves / 0x64) < 0x10000:
            You can only meditate successfully if the current
            ((moves / 0x64) & 0xffff) is different from the value in this
            field.
            If (moves / 0x64) >= 0x10000:
            Meditating at a shrine is always successful.
0x1EC  2    time of last virtue-related conversation
            The game stores ((moves / 0x10) & 0xffff) in this field when
            one of the following happens:
            1) you give gold to a beggar
            2) you answer a yes/no question from an NPC (doesn't have to
               be a question where the answer affects your virtue)
            If ((moves / 0x10) & 0xffff) < 0x10000:
            Giving gold to a beggar or answering a virtue-related yes/no
            question only increases your karma if ((moves / 0x10) & 0xffff)
            is different from the value in this field.
            If ((moves / 0x10) & 0xffff) >= 0x10000:
            Giving gold or answering a virtue question always increases
            your karma.
0x1EE  2    if the party is in a dungeon: coordinates of this dungeon
            if not: coordinates of the dungeon last entered by the party
0x1F0  2    orientation in dungeon (0-west, 1-north, 2-east, 3-south)
0x1F2  2    dungeon level (starting at zero, 0xffff = surface)
0x1F4  2    current party location (see below)
```

The character record format:

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    |   2   | Hit points
0x2    |   2   | Hit points maximum
0x4    |   2   | Experience points
0x6    |   2   | Strength
0x8    |   2   | Dexterity
0xA    |   2   | Intelligence
0xC    |   2   | Magic points
0xE    |   2   | ???
0x10   |   2   | Weapon
0x12   |   2   | Armor
0x14   |  16   | Name
0x24   |   1   | Sex (0xB=Male, 0xC=Female)
0x25   |   1   | Class
0x26   |   1   | Status ('G'=Good, 'P'=Poisoned, 'S'=Sleeping, 'D'=Dead)

Party location:
Since you can only save on the surface or in a dungeon, the only valid
party locations are 0 and 0x11-0x18.

Value | Location
-----:|----------------
 0x0  | Surface
 0x1  | Lord British's castle
 0x2  | The Lycaeum
 0x3  | Empath Abbey
 0x4  | Serpent's Hold
 0x5  | Moonglow
 0x6  | Britain
 0x7  | Jhelom
 0x8  | Yew
 0x9  | Minoc
 0xA  | Trinsic
 0xB  | Skara Brae
 0xC  | Magincia
 0xD  | Paws
 0xE  | Buccaneer's Den
 0xF  | Vesper
0x10  | Cove
0x11  | Deceit
0x12  | Despise
0x13  | Destard
0x14  | Wrong
0x15  | Covetous
0x16  | Shame
0x17  | Hythloth
0x18  | Abyss
0x19  | Shrine of Honesty
0x1A  | Shrine of Compassion
0x1B  | Shrine of Valor
0x1C  | Shrine of Justice
0x1D  | Shrine of Sacrifice
0x1E  | Shrine of Honor
0x1F  | Shrine of Spirituality
0x20  | Shrine of Humility


MONSTERS.SAV
------------

Ultima 4 maintains a structure (in memory) that I've decided to call
the "monster table."
When you Journey Onward, U4 loads MONSTERS.SAV into the monster table.
When you Quit & Save, U4 writes the monster table to MONSTERS.SAV.
The purpose of the monster table fields depends on the current party
location (surface, dungeon, settlement).

Surface:
On the surface, the monster table contains information about monsters
and inanimate objects (chests, horses, ships, balloon).
A table entry is free if (previousTile[i] == 0). However, when u4dos
deletes and entry, it sets both currentTile[i] and previousTile[i] to 0.
Slots 0-7 are for monsters, while slots 8-31 are for inanimate objects.
Non-empty entries don't have to be contiguous.
Whirlpools and twisters are special cases; they must be placed in slot 0-3,
or they won't swallow ships / damage the party.
When you leave Hythloth, u4dos creates the balloon at (233 / 242).
If there is more than one inanimate object on a tile, objects closer to
the beginning of the table are drawn on top of those closer to the end.
Monsters and the party are always drawn on top of inanimate objects.
The current tile and previous tile values are used during runtime.
However, when the game is saved, the previous tile must be set to the
monster's base tile.
The current tile can be any of the monster's tiles, except for pirate
ships, where the current tile determines which direction the pirate ship
is facing.
If the previous tile is not set to the monster's base tile, it won't behave
properly in u4dos: monsters with ranged attacks won't use them in combat,
twisters and whirlpools will attack the party, and pirates will appear as
pirate ships in battle.
For transports with more than one tile (horse, ship), the previous tile
determines which direction the transport is facing, while the current tile
can be any of the transport's tiles.

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    | 32    | Current tile
0x20   | 32    | Current x coordinate
0x40   | 32    | Current y coordinate
0x60   | 32    | Tile for previous turn
0x80   | 32    | X coordinate for previous turn
0xA0   | 32    | Y coordinate for previous turn
0xC0   | 32    | Not used
0xE0   | 32    | Not used

Dungeons:
In a dungeon, the monster table contains information about the monsters
in the dungeon.
A monster stored in the monster table is *also* stored in DNGMAP.SAV.
Inanimate objects are not stored in the monster table; they are stored
*only* in DNGMAP.SAV.

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    | 32    | Not used
0x20   | 32    | Current x coordinate
0x40   | 32    | Current y coordinate
0x60   | 32    | Current tile
0x80   | 32    | X coordinate for previous turn
0xA0   | 32    | Y coordinate for previous turn
0xC0   | 32    | Dungeon level (0-7)
0xE0   | 32    | Not used

Settlements:
In a castle/town/village, the monster table is loaded from the settlement's
.ult file, starting at offset 0x400.
It contains information about the settlement's NPC's.

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    | 32    | Current tile
0x20   | 32    | Current x coordinate
0x40   | 32    | Current y coordinate
0x60   | 32    | Tile for previous turn
0x80   | 32    | X coordinate for previous turn
0xA0   | 32    | Y coordinate for previous turn
0xC0   | 32    | Movement behavior for NPCs 0-31 (0x0=Fixed, 0x1=Wander, 0x80=Follow, 0xFF=Attack)
0xE0   | 32    | Conversation index (.TLK file) for NPCs 0-31


OUTMONST.SAV
------------

OUTMONST.SAV is a backup of the monster table, created when you enter
a dungeon or settlement.
When you leave a dungeon or settlement, the monster table is reloaded
from OUTMONST.SAV.

**Exception:** when you enter Hythloth though the ladder in LB's castle,
the game does not write the current monster table to OUTMONST.SAV,
because it already did that when you entered LB's castle.


DNGMAP.SAV
----------

When you Quit & Save in a dungeon, the game saves objects (orbs,
chests) and monsters in DNGMAP.SAV.
DNGMAP.SAV contains the 8x8 maps for all 8 levels.
Every tile is encoded as a byte.
The upper nibble of a tile determines the tile type (see below).
The lower nibble encodes the monster standing on the tile, except
for tiles 0x80 0x90 0xA0 0xD0 (see below).

Value | Purpose
------|--------------------
0x00  | Nothing
0x10  | Ladder up
0x20  | Ladder down
0x30  | Ladder up/down
0x40  | Chest
0x50  | Hole in ceiling
0x60  | Hole in floor
0x70  | Orb (see below)
0x80  | Trap (magic winds)
0x81  | Trap (falling rocks)
0x8E  | Trap (pit)
0x90  | Fountain (no effect)
0x91  | Fountain (heals all HP)
0x92  | Fountain (100 HP damage)
0x93  | Fountain (cures poison)
0x94  | Fountain (poison and 100 HP damage)
0xA0  | Magic field (poison)
0xA1  | Magic field (energy)
0xA2  | Magic field (fire)
0xA3  | Magic field (sleep)
0xB0  | Altar (stone color depends on dungeon)
0xC0  | Door
0xD0-0xDF | Dungeon room 0-15
0xE0  | Hidden Door
0xF0  | Wall

Monsters:

Value | Purpose
------|--------------------
0x0   | None
0x1   | Rat
0x2   | Bat
0x3   | Spider
0x4   | Ghost
0x5   | Slime
0x6   | Troll
0x7   | Gremlin
0x8   | Mimic
0x9   | Reaper
0xA   | Insects
0xB   | Gazer
0xC   | Phantom
0xD   | Orc
0xE   | Skeleton
0xF   | Rogue

Orbs:
When a PC touches an orb, one or more attributes are raised by 5 points each.
Which attributes are raised depends on the dungeon.

Dungeon  | Attributes
---------|---------------
Deceit   | INT
Despise  | DEX
Destard  | STR
Wrong    | INT, DEX
Covetous | DEX, STR
Shame    | INT, STR
Hythloth | INT, DEX, STR


TITLE.EXE
---------

This executable is launched by ULTIMA.COM to display the introduction
and create new characters.  When "Journey Onward" is selected from the
intro menu, it exits and ULTIMA.COM passes control off to AVATAR.EXE,
the main game executable.  This .exe has lots of the data necessary
for the intro screens embedded in it.

Offset | Bytes | Purpose
-------|------:|----------------------
0x0    | 17445 | ???
0x4425 | 11088 | Text for introduction, gypsy dialog and gyspy questions (zero terminated strings)
0x6f75 |   359 | ???
0x70dc |     8 | Starting X positions for a new game
0x70e4 |     8 | Starting Y positions for a new game
0x70ec |   660 | ???
0x7380 |   184 | List of frames sequences for beasties on intro screen
0x7438 |    54 | ???
0x746e |   532 | 266 X/Y coordinates for the pixels that make up the Lord British signature on the introductory title screen
0x7682 |     1 | Zero terminating byte
0x7683 |    95 | Tile values for the 19x5 map that appears on the introductory title screen
0x76e2 |   548 | Script data for the intro map animations
0x7906 |  ...  | ???


AVATAR.EXE
----------

This is the main game executable, launched from ultima.com (or run on its own
to skip the intro).  Lots of the special case data is embedded in this .exe.

```
Offset  Len   Notes
0x0     63843 ???
0x0f963 1     volume on/off flag
0x0f964 27    13 two-byte code pointers to 13 sound generation routines
              plus one padding byte
0x0f97f 171   .ULT filenames; 16 zero terminated strings
0x0fa2a 48    stone colors; 8 zero terminated strings
0x0fa5a 91    .DNG filenames; 8 zero terminated strings
0x0fab5 28    ???
0x0fad1 8     moongate x-coordinates (one byte for each moongate starting
              at moonglow)
0x0fad9 8     moongate y-coordinates (one byte for each moongate starting
              at moonglow)
0x0fae1 32    16 two-byte offsets to .ULT filenames (value + 62141 gives
              file offset)
0x0fb01 32    area x-coordinates (one byte for each town, city, castle,
              dungeon, shrine)
0x0fb21 32    area y-coordinates (one byte for each town, city, castle,
              dungeon, shrine)
0x0fb41 16    8 two-byte offsets to stone colors (value + 62141 gives
              file offset)
0x0fb51 16    8 two-byte offsets to .DNG filenames (value + 62141 gives
              file offset)
0x0fb61 72    ???
0x0fba9 8     pirates cove ship x coordinates
0x0fbb1 8     pirates cove ship y coordinates
0x0fbb9 8     pirates cove ship tiles
0x0fbc1 37    zero terminated list of tiles that are walkable
0x0fbe6 149   14 zero-terminated navigation strings ("North", "Sail
              North", etc.)
0x0fc7b 539   Chamber of the Codex virtue questions
0x0fe96 78    ???
0x0fee4 647   Chamber of the Codex dialog strings
0x1016b 28    ???
0x10187 509   Chamber of the Codex dialog strings
0x10384 83    ???
0x103d7 1280  abyss question dialog strings
0x108d7 22    11 two-byte offsets to abyss question answers (value +
              62141 gives file offset)
0x108ed 80    ???
0x1093d 169   .TLK filenames; 16 zero terminated strings
0x109e6 13    "OUTMONST.SAV" zero terminated string
0x109f3 32    16 two-byte offsets to .TLK filenames (value + 62139 gives
              file offset)
0x10a13 659   ???
0x10ca6 246   monster names; 36 zero terminated strings
0x10d9c 129   weapon names; 16 zero terminated strings
0x10e1d 77    armor names; 8 zero terminated strings
0x10e6a 64    weapon abbreviations; 16 zero terminated strings
0x10eaa 55    character class names; 8 zero terminated strings
0x10ee1 58    npc type names; 8 zero terminated strings
0x10f1b 81    reagent names; 8 zero terminated strings
0x10f6c 434   spell names; 26 zero terminated strings
0x1101e 292   castle/city/town/dungeon/shrine names; 32 zero terminated
              strings
0x11142 11    ???
0x1114d 72    36 two-byte offsets to monster names (value + 62131 gives
              file offset)
0x11195 32    16 two-byte offsets to weapon names (value + 62131 gives
              file offset)
0x111b5 16    8 two-byte offsets to armor names (value + 62131 gives
              file offset)
0x111c5 32    16 two-byte offsets to weapon abbreviations (value + 62131
              gives file offset)
0x111e5 16    8 two-byte offsets to character class names (value + 62131
              gives file offset)
0x111f5 330   ???
0x1133f 26    mp required for each spell (one byte per spell)
0x11359 654   ???
0x115e7 16    mask of which classes can use each weapon (one byte per
              weapon)
0x115f7 8     mask of which classes can use each type of armor (one byte
              per armor type)
0x115ff 134   ???
0x11685 52    monster hp table
0x116b9 36    monster leader types (one byte per monster type)
0x116dd 36    monster encounter size table (one byte per monster type)
0x11701 2     ???
0x11703 16    weapon value list (one byte per weapon type)
0x11713 8     armor value list (one byte per armor type)
0x1171b 16    weapon type list (one byte per weapon type; 0 = melee, 0xff
              = missile)
0x1172b 154   .CON filenames; 14 zero terminated strings
0x117c5 28    14 two-byte offsets to .CON filenames (value + 62131 gives
              file offset)
0x117e1 64    dungeon .CON filenames; 7 zero terminated strings
0x11821 164   ???
0x118c5 12    altar room exit destinations
              0-3 = truth (north, east, south, west)
              4-7 = love
              8-11 = courage
0x118d1 146   ???
0x11963 8     ambush monster types
0x1196b 190   miscellaneous text strings
0x11a29 26    spell reagent masks (one byte per spell)
0x11a43 364   miscellaneous text strings
0x11baf 16    city/runemask pairs (city id, corresponding rune bitmask)
0x11bbf 12    miscellaneous text string
0x11bcb 120   24 five-byte item location records (see below)
0x11c43 288   ???
0x11d63 250   companion dialog text
0x11e5d 16    8 two-byte offsets to virtue adjectives for companion
              dialogs (value + 62123 gives file offset)
0x11e6d 1404  ???
0x123e9 3486  Hawkwind dialog text; the first 40 zero terminated
              strings are his responses for 5 levels times 8 virtues,
              plus some other miscellaneous dialog strings
0x13187 80    40 two-byte offsets to Hawkwind dialog strings (value +
              62075 gives file offset)
0x131d7 564   reagent vendor dialog text; x zero terminated strings
0x133eb 16    reagent vendor index for each city (one byte per
              castle/city/town)
0x133fb 8     4 two-byte offsets to reagent shop names (value + 62075
              gives file offset)
0x13403 8     4 two-byte offsets to reagent shopkeeper names (value +
              62075 gives file offset)
0x1340b 24    price for each of the first six reagents at Moonglow,
              Skara Brae, Paws, and Buccaneers Den respectively (one
              byte per price)
0x13423 1274  weapon vendor dialog text; 28 zero terminated strings
0x1391d 12    6 two-byte offsets to weapon shop names (value + 62075
              gives file offset)
0x13929 12    6 two-byte offsets to weapon shopkeeper names (value +
              62075 gives file offset)
0x13935 24    which four weapons each of the six weapons vendors can
              sell (one byte per weapon)
0x1394d 32    price of each weapon (two bytes for each price)
0x1396d 16    weapon vendor index for each city (one byte per
              castle/city/town)
0x1397d 28    14 two-byte offsets to weapon descriptions (value + 62075
              gives file offset) for all weapons except hands and
              mystic
0x13999 522   more weapon vendor dialog text; 27 zero terminated strings
0x13ba3 646   armor vendor dialog text; 19 zero terminated strings
0x13e29 10    5 two-byte offsets to armor shop names (value + 62075 gives
              file offset)
0x13933 12    6 two-byte offsets to armor shopkeeper names (value + 62075
              gives file offset)
0x13e3f 20    which four types of armor each of the five armor vendors
              can sell (one byte per armor type)
0x13e53 4     zero padding
0x13e57 16    price of each type of armor (two bytes for each price)
0x13e67 16    armor vendor index for each city (one byte per
              castle/city/town)
0x13e77 12    6 two-byte offsets to armor descriptions (value + 62075
              gives file offset) for all armor types except skin and
              mystic
0x13e83 484   more armor vendor dialog text; 27 zero terminated strings
0x14067 318   horse vendor dialog text; 7 zero terminated strings
0x141a5 622   guild vendor dialog text; x zero terminated strings
0x14413 6     ???
0x14419 8     price per unit of each guild item (two bytes for each price)
0x14521 8     quantity per unit of each guild item (two bytes for each
              quantity)
0x14429 4     2 two-byte offsets to guild shop names (value + 62075 gives
              file offset)
0x1442d 4     2 two-byte offsets to guild shopkeeper names (value + 62075
              gives file offset)
0x14431 8     4 two-byte offsets to guild item inquiry responses (value +
              62075 gives file offset) for torches, gems, keys,
              sextants
0x14439 702   inn dialog text; x zero terminated strings
0x146F7 16    inn index for each city (one byte per castle/city/town)
0x14707 8     inn sleep location x coordinate for seven inns plus zero pad
0x1470f 8     inn sleep location y coordinate for seven inns plus zero pad
0x14717 8     inn prices for seven inns plus zero pad
0x1471f 14    7 two-byte offsets to inn names (value + 62067 gives file
              offset)
0x1472d 14    7 two-byte offsets to inn names (value + 62067 gives file
              offset)
0x1473b 14    7 two-byte offsets to inn price dialogs (value + 62067
              gives file offset)
0x14749 424   more inn dialog text; x zero terminated strings
0x148f1 266   healer dialog text; x zero terminated strings
0x149fb 16    healer index for each city (one byte per castle/city/town)
0x14a0b 20    10 two-byte offsets to healer shopkeeper names (value + 62067
              gives file offset)
0x14a1f 20    10 two-byte offsets to healer shopkeeper names (value + 62067
              gives file offset)
0x14b33 598   more healer dialog text; x zero terminated strings
0x14d89 978   tavern dialog text; x zero terminated strings
0x1515b 16    tavern index for each city (one byte per castle/city/town)
0x1516b 12    minimum price for information at each tavern (two bytes for
              each price)
0x15177 12    6 two-byte offsets to tavern shop names (value + 62067 gives
              file offset)
0x15183 12    6 two-byte offsets to tavern shopkeeper names (value + 62067
              gives file offset)
0x1518f 12    6 two-byte offsets to tavern specialty names (value + 62067
              gives file offset)
0x1519b 12    price for plate of food at each tavern (two bytes for
              each price)
0x151a7 12    7 two-byte offsets to tavern information topics (value +
              62067 gives file offset) (??? why 7, not 6)
0x151b5 12    6 two-byte offsets to tavern information responses (value +
              62067 gives file offset)
0x151c1 598   more tavern dialog text; x zero terminated strings
0x15417 456   food vendor dialog text; x zero terminated strings
0x155df 16    food vendor index for each city (one byte per castle/city/town)
0x155ef 10    price for 25 units of food at each vendor (two bytes for
              each price) for Moonglow, Britan, Yew, Skara Brae, and
              Paws, respectively
0x155f9 10    5 two-byte offsets to food shop names (value + 62067 gives
              file offset)
0x15603 10    5 two-byte offsets to food shopkeeper names (value + 62067
              gives file offset)
0x1560d 189   Lord British dialog keywords; 27 zero terminated strings
0x156ca 2968  Lord British dialog text; 25 zero terminated strings ("He
              says", plus responses for last 24 of the above keywords)
0x16262 104   ???
0x162ca 2037  Lord British "help" response dialog strings
0x16abf ...   ???
```

Each item location record has the following structure: 

Offset | Bytes | Purpose
-------|:-----:|----------------------
0x0    | 1     | Item location (same encoding as party location in PARTY.SAV, e.g. 0 for surface)
0x1    | 1     | Item X coordinate
0x2    | 1     | Item Y coordinate
0x3    | 2     | Pointer to function that handles picking up the item 


Sources
-------

* http://www.geocities.com/xenerkes/
* http://www.moongates.com/u4/Tech.asp
* Marc Winterrowd <nodling at yahoo dot com>

[Lempel-Ziv-Welch]: https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch
