#
# $Id$
#

CFLAGS=-Wall -I.

U4=./ultima4
UI=sdl
UILIBS=-lSDL -lpthread
LIBS=$(UILIBS)
MAIN=u4
MAPS=maps/shrine.c
TILES=images/tiles.c images/charset.c
SRCS=u4.c map.c ttype.c screen.c screen_$(UI).c event.c event_$(UI).c person.c annotation.c io.c savegame.c maps/cities.c maps/castles.c maps/worldmap.c $(MAPS) $(TILES)
OBJS=$(SRCS:.c=.o)

all:: images maps u4

############################################################
# IMAGES
############################################################

images:: $(TILES)

cleanimages:
	rm -f util/maketiles $(TILES)

util/maketiles: util/maketiles.c
	$(CC) $(CFLAGS) -o $@ $<

images/tiles.c: util/maketiles
	mkdir -p images
	./util/maketiles tiles $(U4)/shapes.ega images/tiles.c

images/charset.c: util/maketiles
	mkdir -p images
	./util/maketiles charset $(U4)/charset.ega images/charset.c

############################################################
# MAPS
############################################################

maps:: $(MAPS)

cleanmaps:
	rm -f util/makearea $(MAPS)

maps/shrine.c: util/makearea
	mkdir -p maps
	./util/makearea shrine 5 5 $(U4)/shrine.con maps/shrine.c

util/makearea: util/makearea.c
	$(CC) $(CFLAGS) -o $@ $<

############################################################
# MISC
############################################################

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:: cleanimages cleanmaps cleanmisc

cleanmisc:
	rm -rf *~ */*~ $(OBJS)

-include .depends
