#
# $Id$
#

prefix=/usr
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share
desktopdir=/etc/X11/applnk

UI=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)

CFLAGS=-Wall -I. $(UIFLAGS) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\"
LIBS=$(UILIBS)
INSTALL=install -D

include Makefile.common

install::
	$(INSTALL) $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	$(INSTALL) ../icons/u4.png $(datadir)/pixmaps/u4.png
	$(INSTALL) ../mid/Castles.mid $(libdir)/u4/music/Castles.mid
	$(INSTALL) ../mid/Combat.mid $(libdir)/u4/music/Combat.mid
	$(INSTALL) ../mid/Dungeon.mid $(libdir)/u4/music/Dungeon.mid
	$(INSTALL) ../mid/Fanfare_Of_Lord_British.mid $(libdir)/u4/music/Fanfare_Of_Lord_British.mid
	$(INSTALL) ../mid/Rule_Britannia.mid $(libdir)/u4/music/Rule_Britannia.mid
	$(INSTALL) ../mid/Shopping.mid $(libdir)/u4/music/Shopping.mid
	$(INSTALL) ../mid/Shrines.mid $(libdir)/u4/music/Shrines.mid
	$(INSTALL) ../mid/Towns.mid $(libdir)/u4/music/Towns.mid
	$(INSTALL) ../mid/Wanderer.mid $(libdir)/u4/music/Wanderer.mid
	$(INSTALL) dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) ../u4.desktop $(desktopdir)/Games/u4.desktop

-include .depends
