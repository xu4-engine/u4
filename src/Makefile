#
# $Id$
#

CFLAGS=-Wall -I.

U4=./ultima4
UI=sdl
UILIBS=-lSDL -lpthread
LIBS=$(UILIBS)
MAIN=u4
SRCS=u4.c map.c ttype.c screen.c screen_$(UI).c event.c event_$(UI).c person.c annotation.c io.c savegame.c maps/cities.c maps/castles.c maps/worldmap.c maps/areas.c
OBJS=$(SRCS:.c=.o)

all:: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

-include .depends
