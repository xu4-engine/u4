XU4
===

> Prepare yourself for a grand adventure: Ultima IV, sixteen times
> larger than Ultima III, is a milestone in computer gaming.  Lord
> British has produced a game to challenge, not only your physical
> and mental skills, but the true fabric of your character.  The evil
> triad of Mondain, Minax, and the hellspawn Exodus, have been
> vanquished and peace reigns throughout the land of Britannia.  Evil
> yet abounds, but in isolated pockets and in the hearts of men.  A
> new age awaits the coming of one who can conquer evil on all
> frontiers through the mastery of both magic and the use of force.
> Daemons, dragons and long-dead wizards still plague the countryside
> and must be destroyed.  The seeker on the path of the avatar will
> faces hostile groups composed of mixed enemy types and will survive
> such encounters only by strategic use of weapons and terrain.
> Earthly victories over seemingly impossible odds lead to the final
> conflict, where the ultimate challenge -- the self -- awaits...
>   -- Back cover of Ultima IV box

XU4 is a remake of the computer game Ultima IV.  The goal is to make
it easy and convenient to play this classic on modern operating
systems.  XU4 is primarily inspired by the much more ambitious project
Exult.  Linux is the primary development platform but it gets built
for Windows regularly.  It should be trivial to port to any system with
[Allegro] 5.2 or SDL 1.2 support.

XU4 isn't a new game based on the Ultima IV story -- it is a faithful
recreation of the old game, right up to the crappy graphics.  If you
are looking for a game with modern gameplay and graphics, this is not
it -- yet.  New features that improve the gameplay and keep with the
spirit of the original game will be added.

XU4 also tries to maintain strict compatibility with the original for
its savegame files.  You can use a game saved in XU4 with the original
and vice versa, at least in theory.


Status
------

The game is fully playable and can display the original DOS EGA and
upgraded VGA graphics.

Some thoughts for possible improvements:
 - Ultima 5 style aiming in combat (i.e. allow angle shots)
 - More sound effects
 - Support for higher-resolution tile sets
 - Allow the map view to display more of the world
 - Menu-based interface, like Sega version
 - Use GPU for all rendering.
 - Formal modding system to extend the world or create entirely new adventures.


Compiling
---------

To build the binary on Linux and macOS use these commands:

    ./configure
    make

> **_NOTE_:** The macOS build is currently broken.

If the required libraries & headers are present, make will create the
executable `src/xu4`.

For more detailed build instructions see [doc/build.md](doc/build.md).


Running
-------

The actual data files from Ultima 4 are loaded at runtime, which means
that a copy of Ultima 4 for DOS must be present at runtime.
Fortunately, Ultima IV is available as closed-source freeware from
https://www.gog.com/game/ultima_4.

If you have the optional u4upgrad.zip, place it in the same directory as the
u4 executable.  xu4 will read the Ultima IV data files straight out of the
zipfile.

xu4 searches for the zipfiles, or the unpacked contents of the
zipfiles in the following places:
 - The current directory when xu4 is run
 - A subdirectory named `ultima4` of the current directory
 - On Linux: `$HOME/.local/share/xu4`
 - On macOS: `$HOME/Library/Application Support/xu4`
 - On UNIX systems: `/usr/share/xu4` & `/usr/local/share/xu4`
 - On Windows: `%LOCALAPPDATA%\xu4`

The zipfile doesn't need to be unpacked, but if it is, xu4 can handle
uppercase or lowercase filenames even on case-sensitive filesystems,
so it doesn't matter whether the files are named AVATAR.EXE or
avater.exe or even Avatar.exe.

At the title screen, a configuration menu can be accessed by pressing
'c'.  Here, the screen scale, filter, volume and other settings can be
modified.  These settings are stored in `$HOME/.config/xu4/xu4rc` on Linux
and `%APPDATA%\xu4\xu4.cfg` on Windows.

The saved game files are stored in the settings directory.

xu4 also accepts the following command line options:

        --filter <string>   Specify display filtering mode.
                            (point, HQX, xBR-lv2)
    -f, --fullscreen        Run in fullscreen mode.
    -h, --help              Print this message and quit.
    -i, --skip-intro        Skip the intro. and load the last saved game.
    -m, --module <file>     Specify game module (default is Ultima-IV).
    -p, --profile <string>  Use another set of settings and save files.
    -q, --quiet             Disable audio.
    -s, --scale <int>       Specify display scaling factor (1-5).
    -v, --verbose           Enable verbose console output.

### Profiles

Profiles are stored in the `profiles` sub-directory.
Use quotation marks around profile names that include spaces.
The active profile name is shown on the introduction map view off the main menu.


Ultima 4 Documentation
----------------------

Included with Ultima 4 for DOS, as downloaded from one of the above
sites, are electronic copies of the printed documentation from the
original Ultima IV box.  HISTORY.TXT contains the "The History of
Britannia", a general introduction to the world of Ultima IV.
WISDOM.TXT contains "The Book of Mystic Wisdom", which explains the
system of magic and provides descriptions of the spells and reagents.

PDF versions of these books are included in the official download in
the EXTRA folder of the zip file.
As an added bonus this folder also includes a pdf of the offical 
Ultima IV cluebook.

An image of the cloth map from the original Ultima IV box is also
included in the EXTRA folder.


Debug Mode (cheats)
-------------------

xu4 has a very useful debug mode (you can also think of it as a cheat mode).
To enable it:
- press 'c' in the main xu4 menu
- make sure that
  1) Game Enhancements = On
  2) Enhanced Gameplay Options -> Debug Mode (Cheats) = On

Cheat list:
* ctrl-c (cheat menu; press one of the following keys to use a cheat)

        1-8   gate (teleports you to a moongate location)
        F1-F8 virtue +10
        a     advance moons (advances the left moon [Trammel] to its next phase)
        c     collision (lets you walk across water, through mountains, etc.)
        e     equipment (gives the party Armour and Weapons)
        f     full stats (gives all party members 50 str, dex & intel and level 8)
        g     goto (enter a location and xu4 teleports you there)
        h     help (displays list of available cheats)
        i     items (gives the party Items and Equipment)
        j     joined by companions (if eligible)
        k     show karma (shows your virtues)
        l     location (displays current map and coordinates)
        m     mixtures (gives the party 99 mixtures of all spells)
        o     opacity (lets you see through opaque tiles)
        p     peer (switches between normal and gem view)
        r     reagents (gives the party 99 of all reagents)
        s     summon (enter a monster name, and xu4 creates it somewhere nearby)
        t     transports (press b/h/s + arrow key, and xu4 creates a balloon/horse/ship)
        v     full virtues (makes you a full avatar)
        w     change wind (changes or locks the wind direction)
        x     exit map (teleports the party to where it entered the current map)
        y     y-up (like the Y-up spell, but free)
        z     z-down (like the Z-down spell, but free)

* ctrl-d (destroy monster/object; doesn't work on energy fields)
* ctrl-h (teleport to Lord British's throne room)
* ctrl-v (switch between 3-d and 2-d view; dungeons only)
* F1-F8 (teleport to a dungeon entrance; surface only)
* F9-F11 (teleport to the altar room of truth/love/courage; surface only)
* F12 (display torch duration)
* Escape (end combat)

Note:
Except for the Escape key, none of the cheats work during combat.


Misc.
-----

See http://xu4.sourceforge.net/links.html for some other interesting
Ultima IV related links.

Please send an email to wickedsmoke@users.sourceforge.net if you are
interested in helping.


[Allegro]: https://liballeg.org/
