#
# $Id$
#

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share

UI=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCXXFLAGS=-ggdb -rdynamic
CXXFLAGS=$(FEATURES) -Wall -I. $(UIFLAGS) $(shell xml2-config --cflags) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\" -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS)
LIBS=$(UILIBS) $(shell xml2-config --libs)
INSTALL=install

include Makefile.common

install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4/music
	$(INSTALL) ../mid/*.mid $(libdir)/u4/music
	mkdir -p $(libdir)/u4/sound
	$(INSTALL) ../sound/*.ogg $(libdir)/u4/sound
	$(INSTALL) ../sound/*.wav $(libdir)/u4/sound
	$(INSTALL) -D dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) -D u4enc $(libdir)/u4/u4enc
	$(INSTALL) -D u4dec $(libdir)/u4/u4dec
	$(INSTALL) -D tlkconv $(libdir)/u4/tlkconv
	$(INSTALL) ../conf/*.xml $(libdir)/u4
	mkdir -p $(libdir)/u4/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4/dtd
	mkdir -p $(libdir)/u4/graphics
	mkdir -p $(libdir)/u4/graphics/ega
	mkdir -p $(libdir)/u4/graphics/vga
	$(INSTALL) ../graphics/ega/*.rle $(libdir)/u4/graphics/ega
	$(INSTALL) ../graphics/vga/*.rle $(libdir)/u4/graphics/vga
	$(INSTALL) -D ../u4.desktop $(datadir)/applications/u4.desktop

# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

-include .depends

