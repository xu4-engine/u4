#
# $Id$
#

U4=./ultima4

UI=sdl
UILIBS=$(shell sdl-config --libs)
UIFLAGS=$(shell sdl-config --cflags)

CFLAGS=-Wall -I. $(UIFLAGS)
LIBS=$(UILIBS)
MAIN=u4

SRCS=u4.c map.c ttype.c screen.c screen_$(UI).c event.c event_$(UI).c person.c annotation.c io.c savegame.c u4file.c mapinit.c stats.c spell.c maps/cities.c maps/castles.c maps/worldmap.c maps/areas.c
OBJS=$(SRCS:.c=.o)

bindir=/usr/bin

all:: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

install::
	install -D $(MAIN) $(bindir)/$(MAIN)

-include .depends
