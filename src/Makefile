#
# $Id$
#

bindir=/usr/bin
datadir=/usr/share
desktopdir=/etc/X11/applnk

UI=sdl
UILIBS=$(shell sdl-config --libs)
UIFLAGS=$(shell sdl-config --cflags)

CFLAGS=-Wall -I. $(UIFLAGS) -DICON_FILE=\"$(datadir)/pixmaps/u4.bmp\"
LIBS=$(UILIBS)
MAIN=u4

SRCS=u4.c map.c ttype.c screen.c screen_$(UI).c event.c event_$(UI).c person.c annotation.c io.c savegame.c u4file.c mapinit.c stats.c spell.c game.c intro.c maps/cities.c maps/castles.c maps/worldmap.c maps/areas.c lzw/lzw.c lzw/u4decode.c
OBJS=$(SRCS:.c=.o)

all:: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

install::
	install -D $(MAIN) $(bindir)/$(MAIN)
	install -D ../icons/u4.bmp $(datadir)/pixmaps/u4.bmp
	install -D ../icons/u4.png $(datadir)/pixmaps/u4.png
	install -D ../u4.desktop $(desktopdir)/Games/u4.desktop

-include .depends
