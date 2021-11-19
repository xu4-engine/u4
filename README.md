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
Exult.  Linux is the primary development platform but it gets ported
to Windows and MacOS X regularly. It should be trivial to port to any
system with [Allegro] 5.2 or SDL 1.2 support.

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

A major code cleanup began in 2021 after a 5-year hiatus in development
(and 10 years since the last beta release). Recent work includes:

 - Completion of the Allegro 5 platform interface.
 - Reworking the configuration to allow alternative storage backends.
 - Elimination of compiler warnings & memory leaks.
 - Preparing the way for GPU rendering.

Some thoughts for possible improvements:
 - Ultima 5 style aiming in combat (i.e. allow angle shots)
 - more sound effects
 - support for higher-resolution tile sets
 - allow running original game without XML configuration
 - allow the map view to display more of the world
 - menu-based interface, like Sega version
 - improve the scalers:
   + scale entire screen image rather than individual tiles
   + correct for aspect ratio


Compiling
---------

To build on Linux and macOS use these commands:

    ./configure
    make

To see the configure options run:

    ./configure -h

If the required libraries & headers are present, make will create an
executable called `xu4` in the src directory.

The Allegro 5 build requires the allegro, allegro_audio, & allegro_acodec
libraries & headers.

To use SDL 1.2, the SDL & SDL_mixer libraries are required.  TiMidity++ may
be necessary on some platforms, too.

The libxml2 development files are necessary regardless of what platform API
is used.


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
 - the current directory when u4 is run
 - a subdirectory named "ultima4" of the current directory
 - "/usr/lib/u4/ultima4/"
 - "/usr/local/lib/u4/ultima4/"

The zipfile doesn't need to be unpacked, but if it is, xu4 can handle
uppercase or lowercase filenames even on case-sensitive filesystems,
so it doesn't matter whether the files are named AVATAR.EXE or
avater.exe or even Avatar.exe.

At the title screen, a configuration menu can be accessed by pressing
'c'.  Here, the screen scale, filter, volume and other settings can be
modified.  Note: the game must be restarted for the new settings to
take effect.  These settings are stored in the file $HOME/.xu4rc.

xu4 also accepts the following command line options:

    --fullscreen     Fullscreen mode.
    -f

    --filter <str>   Apply a filter on the scaled images. The <str>
                     parameter must be set to one of the following
                     case-sensitive options:
                         point
                         2xBi
                         2xSaI
                         Scale2x

    --skip-intro     Skip the intro, and go directly into the game.
    -i               This option requires the existance of a valid saved
                     game.

    --profile <str>  Activate a specific save game profile.  Using this
    -p <str>         option, you may have multiple saved games at the
                     same time.
                      * Use quotation marks around profile names that
                        include spaces.
                      * All profiles are stored in the "profiles"
                        sub-directory.
                      * The active profile name is shown on the
                        introduction map view off the main menu.

    --quiet          Quiet mode - no sound or music.
    -q

    --scale <n>      Scale the original graphics by a factor of <n>.
    -s <n>           Factor <n> must be 1, 2, 3, 4, or 5.

    --verbose        Verbose output; prints out information useful for
    -v               trouble-shooting.


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

Please send me a mail at andrewtaylor@users.sourceforge.net if you are
interested in helping.


[Allegro]: https://liballeg.org/
