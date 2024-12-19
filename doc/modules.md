XU4 Module System
=================

Individual adventures are packaged into module files.
There are three types of modules:

* Base
* Extension
* Soundtrack

There is also a single `render.pak` file containing core rendering assets that
are used for all other game modules and has no game configuration chunk.

* A module is distributed as single file with a `.mod` extension.
* A module can extend and/or modify one other module.  Extension modules work
  as an overlay where any resources with the same name in both are taken from
  the extension.
* Each module uses one set of game rules.  The rules are defined in the
  executable program.  Module extensions will use the rules of the base module.
* Each module is developed in a separate Git repository (or repository branch).
* Saved games are specific to a module.


Module Names
------------

The module name comes directly from the package filename and has the following
restrictions:

* It is limited to 39 characters (including the `.mod` suffix).
* It must start with a letter and contains no spaces. The valid characters are
  ASCII letters and numbers, plus, minus, period, underbar, question mark, and
  exclamation point (0-9 A-Z a-z + - . _ ? !).
* For display purposes minus and underbar characters may be replaced with
  spaces.


Module Structure
----------------

Modules use the [CDI Package Format] to store a series of data chunks.

The package header App_ID is `xu4^2` (`78 75 34 02` in hexadecimal).

The following standard identifiers are used:

CDI Format | Contents
-----------|-------------------
0x0001     | GL shaders
0x0006     | MODI & FNAM strings
0x1002     | PNG images
0x4007     | Xu4 talk
0x2006     | WAVE audio
0x2008     | Ogg Vorbis audio
0x2011     | Flac audio
0x2030     | RFX audio

The following application specific identifiers are used:

CDI Format | Contents
-----------|-----------
0x1FC0     | Xu4 map
0x5FC0     | TXF font
0x7FC0     | CONF chunk


Packaging Modules
-----------------

The `tools/pack-xu4.b` script packages assets from a directory into a single
module file.  The tool reads a Boron configuration file named `config.b` from
inside that directory.  All other assets will be referenced from this
configuration file.

A typical package command looks like this:

    ./tools/pack-xu4.b module/Quest-for-the-Grail

This would create a `Quest-for-the-Grail.mod` file in the current directory.

The pack-xu4 `-h` option will print some brief usage information.

> **_NOTE_:** The tools needed to build modules are available together in the
[Module Tools] archive.


Configuration
-------------

### Module Header

The `config.b` file begins with a module entry that looks like this:

```
module [
    author: "John Smith & friends"
    about: {{
        The Avatar must seek the holy grail in order to bring peace
        to the land.
    }}
    version: "1.0b"
    rules: 'Ultima-IV
]
```

The `author` entry names the person (or group) who created the configuration
scripts and packaged the module.  It does not refer to the creators of other
assets (such as image or audio files) that get packaged.

The `about` entry is shown as a tool tip in the game browser so it should
be relatively short.  Try to keep it around 5 lines of 60 characters each.

#### Game Rules

The `rules` entry declares what built-in game rule system to use.
The only existing rule system is `Ultima-IV`.

For extension modules this entry is a string that contains the base
module name and version separated by a slash.  For an extention of the
previous examples the entry would look like this:

    rules: "Quest-for-the-Grail/1.0b"

#### Module Versions

Versions are declared using strings.  A typical major/minor version should
be used (e.g. "1.0").  If it's an Alpha or Beta append a lowercase 'a' or 'b'.
This keeps the version short enough to fit in the game browser list.


### Data Blocks

The following 15 data blocks can be declared: armors, weapons, creatures,
graphics, draw-lists, tileanim, layouts, maps, tile-rules, tileset,
u4-save-ids, music, sound, vendors, ega-palette.

> **_NOTE_:** Most of these aren't documented yet as they are subject to change.


#### music

The music block is an index of audio stream files.

Each stream must be an Ogg Vorbis file containing either stereo or mono
channels at either a 22050 or 44100 Hz sample rate.

A path relative to the module directory can be declared for a number of files
using the `path` keyword followed by a Boron `file!` value.

Each stream is declared with a `file!` value, optionally preceded by an
`int!` identifier value.  Any previously declared path is prepended to the
filename.  If the identifer is not present then it will be set to that of
the previous file plus one.  If no identifier is present the numbering starts
at zero.

The game accesses specific streams by the identifier only, so the path and
file names may be anything the packager desires.

The following example shows how to use path and identifer values:

    music: [
        path %music
           %wander.ogg
           %"Towns (lively).ogg"
        6  %dungeon.ogg
    ]


#### sound

The sound block is an index of audio files.

Each sound file can be encoded as FLAC, Ogg Vorbis, [rFX], or WAVE.
They must contain either stereo or mono channels at either a 22050 or
44100 Hz sample rate.

Paths, identifiers, and filenames are specified as in the [music block](#music).


Soundtrack Modules
------------------

Soundtrack modules contain only a [music block](#music) in `config.b` and
no other data declarations.


[CDI Package Format]: https://urlan.sourceforge.net/cdi-spec.html
[Module Tools]: https://xu4.sourceforge.net/download.php#devel
[rFX]: https://raylibtech.itch.io/rfxgen
