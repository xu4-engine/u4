#
# $Id$
#

prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share
desktopdir=/etc/X11/applnk

UI=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCFLAGS=-ggdb -rdynamic
CFLAGS=$(FEATURES) -Wall -I. $(UIFLAGS) $(shell xml2-config --cflags) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\" -DVERSION=\"$(VERSION)\" $(DEBUGCFLAGS)
LIBS=$(UILIBS) $(shell xml2-config --libs)
INSTALL=install

include Makefile.common

install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4/music
	$(INSTALL) ../mid/*.mid $(libdir)/u4/music
	$(INSTALL) -D dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) -D lzwenc $(libdir)/u4/lzwenc
	$(INSTALL) -D lzwdec $(libdir)/u4/lzwdec
	$(INSTALL) -D rleenc $(libdir)/u4/rleenc
	$(INSTALL) -D rledec $(libdir)/u4/rledec
	$(INSTALL) -D tlkconv $(libdir)/u4/tlkconv
	$(INSTALL) ../conf/*.xml $(libdir)/u4
	mkdir -p $(libdir)/u4/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4/dtd
	mkdir -p $(libdir)/u4/graphics
	mkdir -p $(libdir)/u4/graphics/ega
	mkdir -p $(libdir)/u4/graphics/vga
	$(INSTALL) ../graphics/ega/*.rle $(libdir)/u4/graphics/ega
	$(INSTALL) ../graphics/vga/*.rle $(libdir)/u4/graphics/vga
	$(INSTALL) -D ../u4.desktop $(desktopdir)/Games/u4.desktop

-include .depends
