#
# $Id$
#

bindir=/usr/bin
libdir=/usr/lib
datadir=/usr/share
desktopdir=/etc/X11/applnk

UI=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)

CFLAGS=-Wall -I. $(UIFLAGS) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\"
LIBS=$(UILIBS)
MAIN=u4

SRCS=u4.c map.c ttype.c screen.c screen_$(UI).c event.c event_$(UI).c person.c vendor.c annotation.c io.c savegame.c u4file.c mapinit.c stats.c spell.c game.c intro.c player.c names.c music.c maps/cities.c maps/castles.c maps/worldmap.c maps/areas.c lzw/lzw.c lzw/u4decode.c
OBJS=$(SRCS:.c=.o)

all:: $(MAIN) dumpsavegame

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

dumpsavegame: util/dumpsavegame.o savegame.o io.o
	$(CC) $(CFLAGS) -o dumpsavegame util/dumpsavegame.o savegame.o io.o

clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

install::
	install -D $(MAIN) $(bindir)/$(MAIN)
	install -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	install -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	install -D ../mid/Castles.mid $(libdir)/u4/music/Castles.mid
	install -D ../mid/Combat.mid $(libdir)/u4/music/Combat.mid
	install -D ../mid/Dungeon.mid $(libdir)/u4/music/Dungeon.mid
	install -D ../mid/Fanfare_Of_Lord_British.mid $(libdir)/u4/music/Fanfare_Of_Lord_British.mid
	install -D ../mid/Shopping.mid $(libdir)/u4/music/Shopping.mid
	install -D ../mid/Shrines.mid $(libdir)/u4/music/Shrines.mid
	install -D ../mid/Towns.mid $(libdir)/u4/music/Towns.mid
	install -D ../mid/Wanderer.mid $(libdir)/u4/music/Wanderer.mid
	install -D dumpsavegame $(libdir)/u4/dumpsavegame
	install -D ../u4.desktop $(desktopdir)/Games/u4.desktop

-include .depends
