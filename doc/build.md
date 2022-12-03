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

The following libraries are required for all targets:

 - Boron 2.0.8
 - Faun 0.1.2
    - Vorbis
 - PNG

Allegro 5.2.7 (or later) is also required for Windows.  This is optional
for Linux, which uses the src/glv Git submodule by default.

Linux distributions will provide the Allegro, Vorbis, & PNG libraries.
Below are example install commands for a few flavors of Linux.

Fedora:

    sudo dnf install allegro5-devel libvorbis-devel libpng-devel

Ubuntu:

    sudo add-apt-repository ppa:allegro/5.2
    sudo apt install liballegro5-dev libvorbis-dev libpng-dev

The Boron interpreter program is needed to build game modules.
The static binaries can be downloaded from the
[Boron homepage](http://urlan.sourceforge.net/boron/).

There are three ways the Boron library can be obtained.  It can be built from
[source](https://sourceforge.net/p/urlan/boron/code/ci/master/tree/)
or a pre-built SDK can be downloaded from the
[xu4 download](http://xu4.sourceforge.net/download.php#devel) page.
On UNIX systems the following commands will checkout the source using Git,
properly configure it for xu4, build `libboron.a`, and install it:

    make -C dist boron
    make -C dist/boron libboron.a
    sudo make -C dist/boron DESTDIR=/usr install-dev

Faun SDKs and source can be downloaded from the
[Faun Releases](https://github.com/WickedSmoke/faun/releases) page.  The
source is also available via the src/faun Git submodule.


### configure

The configure script can be used to customize both Make & Copr builds.
The default options match what is used for release builds.
Configure does not need to be run if this is what you want.
To see the configure options run:

    ./configure -h


### GNU Make

The following commands should get you running:

    ./configure
    make download
    make
    src/xu4

This downloads the original game archives, builds the binary, packs the game
modules, and then runs the program.

### Copr

The following commands should get you running:

    ./configure
    make download
    make mod
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

The `cbuild-sdks` Makefile target downloads the required Allegro, Boron, &
Faun SDKs using `curl`.

### Running Cbuild

After setup is complete all supported targets can be built using using these
commands:

    make mod
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
    the current game modules.

To rebuild after any code changes, first remove the project.tar.gz file
manually.

If just a binary is required, the application archive step can be skipped by
using the cbuild `-b` option after the target.


[GNU Make]: https://www.gnu.org/software/make/manual/html_node/index.html#toc-Overview-of-make
[Copr]: http://urlan.sourceforge.net/copr.html
[Podman]: https://podman.io/getting-started/installation
[Docker]: https://docs.docker.com/get-docker/
