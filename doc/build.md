XU4 Build Instructions
======================

This document describes the various ways to build binaries & packages.

The following build systems are provided:

 - [GNU Make] for developers.
 - [Copr] for developers.
 - Cbuild for release packages.

Here is the status of build systems for various target OSes:

| Target  | make   | copr    | cbuild |
| ------- | ------ | ------- | ------ |
| Linux   | OK     | OK      | OK     |
| macOS   | src/Makefile.macosx not up to date | Unknown | Not supported |
| Windows | src/Makefile.mingw not up to date  | OK | OK |


Developer Builds
----------------

### Dependencies

There are two supported platform libraries (Allegro 5 & SDL 1.2) and
two supported configuration libraries (Boron 2 & libxml2):
The official builds use Allegro & Boron.

The Allegro 5 build requires the allegro, allegro_audio, & allegro_acodec
libraries & headers.  Allegro 5.2.7 or later is recommended.

To use SDL 1.2, the SDL & SDL_mixer libraries are required.  TiMidity++ may
be necessary on some platforms, too.

Linux distributions will provide some of these libraries.
Below are example commands for a few flavors of Linux.

Fedora:

    sudo dnf install allegro5-addon-acodec-devel
    sudo dnf install SDL_mixer-devel liblibxml2-devel

Ubuntu:

    sudo add-apt-repository ppa:allegro/5.2
    sudo apt install liballegro-acodec5-dev
    sudo apt install libsdl-mixer1.2-dev libxml2-dev

When using Boron, the interpreter program is needed build game modules.
The static binaries can be downloaded from the
[Boron homepage](http://urlan.sourceforge.net/boron/).

The Boron library can be built from
[source](https://sourceforge.net/p/urlan/boron/code/ci/master/tree/)
or a pre-built SDK can be downloaded from the
[xu4 download](http://xu4.sourceforge.net/download.php#devel) page.
On UNIX systems the following commands will checkout the source using Git,
properly configure it for xu4, build `libboron.a`, and install it:

    make -C dist boron
    make -C dist/boron libboron.a
    sudo make -C dist/boron DESTDIR=/usr install-dev


### configure

The configure script can be used to customize both Make & Copr builds.
The default options match what is used for release builds.
Configure does not need to be run if this is what you want.
To see the configure options run:

    ./configure -h

> **_NOTE:_** Building with SDL currently requires this configuration:
> 
>    ./configure --sdl --gpu none --xml

### GNU Make

The following commands should get you running:

    ./configure
    make download
    make Ultima-IV.mod
    make
    src/xu4

This downloads the original game archives, packs the game module, builds the
binary, and then runs the program.

### Copr

The following commands should get you running:

    ./configure
    make download
    make Ultima-IV.mod
    copr
    ./xu4


Release Builds
--------------

Linux containers can be used to build the Linux & Windows release archives.
The containers & SDKs must be downloaded and then the `tools/cbuild` script
is used to build each target OS archive separately.

This provides the following benefits over server based solutions:

 - No remote account or authorization is needed.
 - No network connection is required after the initial setup.
 - Developers can make sure changes in their working directory will build
   on supported platforms without having to commit any code.
 - Direct access to the containers makes troubleshooting & testing easier.
 - Simple shell scripts & makefiles reduce the learning curve and make
   customization easier.

### Cbuild setup

The heavy lifting is done by [Podman] or [Docker], so those must be
installed first.  Cbuild currently uses [Copr] so that must also be installed.

Run these commands from the project root:

    make -C dist cbuild-images
    make -C dist cbuild-sdks

The `cbuild-images` Makefile target creates the containter images.  For
specifics see the `dist/Dockerfile.*` files.

> **_NOTE:_** dist/Makefile is currently hardcoded to use Podman `buildah`.
> If Docker is being used, uncomment the `docker build` line.

The `cbuild-sdks` Makefile target downloads the required Allegro & Boron SDKs
using `curl`.

### Running Cbuild

After setup is complete all supported targets can be built using using these
commands:

    make Ultima-IV.mod
    tools/cbuild linux
    tools/cbuild windows

This will create xu4 binaries & archives in the `/tmp` directory.

> **_NOTE:_** cbuild is currently hardcoded to use `podman`.
> If Docker is being used then a shell alias for Podman must be set.

Cbuild executes these steps:
 1. The currently checked out code is archived in `project.tar.gz` if it does
    not already exist.
 2. A container is started, the code archive & SDKs are passed to it, and
    a binary is built.
 3. The host retrieves the binary and creates the application archive using
    the current game module.

To rebuild after any code changes, first remove the project.tar.gz file
manually.

If just a binary is required, the application archive step can be skipped by
using the cbuild `-b` option after the target.


[GNU Make]: https://www.gnu.org/software/make/manual/html_node/index.html#toc-Overview-of-make
[Copr]: http://urlan.sourceforge.net/copr.html
[Podman]: https://podman.io/getting-started/installation
[Docker]: https://docs.docker.com/get-docker/
