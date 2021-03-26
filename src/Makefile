#
# $Id$
#

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share

UI=sdl
#UI=allegro

ifeq ($(UI), sdl)
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)
else
UILIBS=-lallegro_acodec -lallegro_audio -lallegro
UIFLAGS=
endif

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCXXFLAGS=-ggdb1 -rdynamic -g -O0 -fno-inline -fno-eliminate-unused-debug-types -gstabs -g3
CXXFLAGS=$(FEATURES) -Wall -I. $(UIFLAGS) $(shell xml2-config --cflags) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\" -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS)
CFLAGS=$(CXXFLAGS)
LIBS=$(UILIBS) $(shell xml2-config --libs) -lpng -lz
INSTALL=install

ifeq ($(STATIC_GCC_LIBS),true)
    LDFLAGS+=-L. -static-libgcc
endif

include Makefile.common

install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4/music
	$(INSTALL) ../mid/*.mid $(libdir)/u4/music
	$(INSTALL) ../mid/*.it $(libdir)/u4/music
	mkdir -p $(libdir)/u4/sound
	$(INSTALL) ../sound/*.ogg $(libdir)/u4/sound
	$(INSTALL) -D coord $(libdir)/u4/coord
	$(INSTALL) -D dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) -D tlkconv $(libdir)/u4/tlkconv
	$(INSTALL) -D u4dec $(libdir)/u4/u4dec
	$(INSTALL) -D u4enc $(libdir)/u4/u4enc
	$(INSTALL) -D u4unpackexe $(libdir)/u4/u4unpackexe
	$(INSTALL) ../conf/*.xml $(libdir)/u4
	mkdir -p $(libdir)/u4/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4/dtd
	mkdir -p $(libdir)/u4/graphics
	mkdir -p $(libdir)/u4/graphics/ega
	mkdir -p $(libdir)/u4/graphics/hires
	mkdir -p $(libdir)/u4/graphics/png
	mkdir -p $(libdir)/u4/graphics/vga
	mkdir -p $(libdir)/u4/graphics/vga2
	mkdir -p $(libdir)/u4/graphics/new
	$(INSTALL) ../graphics/ega/*.png $(libdir)/u4/graphics/ega
	$(INSTALL) ../graphics/hires/*.png $(libdir)/u4/graphics/hires
	$(INSTALL) ../graphics/hires/*.vga $(libdir)/u4/graphics/hires
	$(INSTALL) ../graphics/png/*.png $(libdir)/u4/graphics/png
	$(INSTALL) ../graphics/vga/*.png $(libdir)/u4/graphics/vga
	$(INSTALL) ../graphics/vga2/*.png $(libdir)/u4/graphics/vga2
	$(INSTALL) ../graphics/new/* $(libdir)/u4/graphics/new
	$(INSTALL) -D ../u4.desktop $(datadir)/applications/u4.desktop


# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

# a target to build without dependencies on libstdc++, libgcc_s
all.static_gcc_libs:
	ln -s `$(CXX) -print-file-name=libstdc++.a` .
	$(MAKE) STATIC_GCC_LIBS=true
	rm -f libstdc++.a

-include .depends

