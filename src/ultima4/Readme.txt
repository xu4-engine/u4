Congrats! You have the Ultima 4 VGA upgrade patch V1.3 (Music 3.15, VGA 1.15)

What is this?
	Simply this is a combination of a VGA screen driver for Ultima 4 and a remake of my old music patch.  VGA tiles and fonts are distributed with this upgrade.

What does it do?
	Upgrades Ultima IV to 256 color graphics with MIDI support. Replaces the tileset, font, game, and cutscene graphics. Corrects several conversation bugs within the .TLK files.

What you need:  You must have the game ultima 4

Find the latest upgrades, information, and hints at The Moongates Ultima IV Annex:
http://www.moongates.com/u4

Intstallation
This patch has three steps.
    1) unzip the zip file into your ultima 4 directory.
    2) run the program 'SETUP.BAT'.  This will configure sound and set the graphics.
    3) run U4 as you normally would, and enjoy the tunes and the graphics.
    4) If upgrading from Wiltshire's 16-color graphics patch, DO NOT run PARTY.EXE;
       it is no longer is supported and may corrupt the new graphic files. The safest
       solution is a clean install of U4. (Copy the PARTY.SAV file to the new
       install to preserve your game progress.)

If you later want to change what music driver is used, then run 'SETM.EXE'
My upgrade patch is a single unit, but if you want to toggle the patch on or off execute 'SWITCH.BAT'

Compatibility
Should be totally compatible, but no garuntees.

Reporting Bugs/Commenting/Suggestions/Questions
Did you find a bug? do like/hate what I did? is there something I can do to improve it? do you want to tell me anything at all?
--A common bug should be playing music at the wrong time.  I did NOT
        extensively (aka play the entire game from start to finish, asking
        everyone everything) test it.  Should you encounter this bug, please
        tell me, and be as specific as possible.  eg: "I was speaking with
        Joe Shmuckatelli, in the town of Whereveriam, when it started playing 
        Sumtoon, after I asked him about doolahitchies."
Send me an email: draug@u.washington.edu
For art/graphics comments: jcsteele@moongates.com


Known bugs/differences/quirks
 1) This patch uses some the more "advanced" OpCodes.  (this means that
    it won't, or shouldn't, run on old machines like the 8088 or 8086)
    If this is really a problem, complain to me and maybe I'll make a
    more compatible version.  More than likely, you can ignore this one.
 2) The music sounds sort of...well...wrong in some places.  Some
    notes are to quiet when compared to the rest of the notes. (at
    least, it does on my system) this is most notable while shopping.
    If anyone reading this just happens to know how to program the
    midpak.ad file (I believe that it is a midi instrument patch map
    of some sort), please make one!
 3) If you are upgrading from Josh 'Wiltshire' Steele's U4 Graphics
    patch, you need to be aware that the PARTY.EXE program for selecting
    the party icon will no longer function and may in fact corrupt the
    new graphics. Another tile-switching option is being explored.
 4) The music drivers may not work with all computers.  If you have this
    problem, try using either the general midi or soundblaster (original)
    If the setup worked, then it should work in the game.

Features
I also added a few more keys :)
Alt-X:	quit to DOS
Alt-R:	reload saved game
Alt-Q:	Return to title screen (or quit if in the title screen)
Alt-F:  Framelimit toggle
Alt-V:	Sound toggle
Alt-'-':Volume down
Alt-'+':Volume up

Do you like them?

Future of my patch
	Upgrading the dungeons graphics (unless you like the black, blue, white and green)
	Sound FX for U4: Send any suggestions (I do have some, but haven't found the time to code them in yet)
	Palette shifting FX: Send suggestions
	General maintenance

Developement of Music patch
V1.0    The first actual working version, lots of bugs
V1.01   Fixed a few bugs
V2.0    Fixed more bugs, (like the 'what the hell is LB saying' bug)
                for those interested, it was because I was pushed BP when I
                meant to push SP
        Added support for Combat, Shopping, and LB music
        Added Alt-V, Alt-'-', and Alt-'+'
V2.01   Plays correct music upon returning to the main screen
        Fixed bug-sound now stays off if you have turned it off
V2.02   Now plays town music when you enter Magincia
V2.1    Will play correct music if you saved your game in a dungeon
        Now plays music at the title screen again
	Pressing Alt-X or Alt-Q at the title screen will now exit to DOS
        Fixed SETUP.BAT
        Will now change music after leaving an altar room
        Detects death
V3.0	Totally reworked entire method of detecting, should be more
	reliable now.  Death handled gracefully.  Shopping music
	should be played at all shops now.  Inn and Camping are
	detected.  Volume controls not yet implemented.
V3.1	Reworked XMI code, should now play on all supported sound cards
	Added endgame fanfare
V3.15	Re-added Volume control keys (Alt-V, Alt-'-', and Alt-'+') support
	If the midi drivers fail to load, U4 will not crash (so if you are
	tired of the music, rename or delete 'midpak.com' to play without
	music)

Developement of VGA drivers
V1.0	EGA drivers 100% decoded, VGA drivers 100% written.
	about 99% tested.
V1.01	Fixed the following bugs:
	The Balron doesn't flake out any more
	Peering at gems and using the telescope are fixed
V1.1	Corrected some end game code
	Added partial (mostly untested) framelimiters
V1.15	Framelimiter toggle key Alt-F added

Acknowlegdments
KUDOS to SpiritWind, for info on the Atari version, some music, and encouragement,
     and Minstrel, for more and better music,
     and Wiltshire, for the nifty graphics and giving me the idea of adding music,
     and Blackmage, for just plain being interested,
     and The Audio Solution, inc; for their wonderful music interface,
     and Auric, for giving us a new web home (www.moongates.com/u4),
     and everyone who has reported bugs,
     and anyone else that I just happend to forget!


Copyright/disclaimer stuff
First, this software is "Freeware."  This means that this software may be distributed to anyone, anytime, with no restrictions.  Please feel free to make copies and give them to your friends, or add it to your webpage, or whatever.  
Next, MIDPAK.COM and its associated files, are not "Freeware" like mine.  If you intend to use it with a commercial product (not "Freeware" or "Shareware") you must register it with The Audio Solution.  For more information on MIDPAK or The Audio Solution, visit their Web Page.
Finally, Ultima 4 is copyrighted 1987 by Lord British. (And has since been made available freely to the public, I believe)

As usual, I am not responsible for anything this does to you or your computer.


Technical junk
I programmed this entirely in Assembly Language, using Eric Isaacson's
assembler ver 4.05.  And yes, I am a little bit crazy.


Last words
Candy is dandy, but sex won't rot your teeth.  :)
