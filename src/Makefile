#
# $Id$
#

CFLAGS=-Wall -I.

U4=./ultima4
LIBS=-lSDL -lpthread
MAIN=u4
MAPS=maps/worldmap.c maps/britain.c maps/yew.c maps/minoc.c maps/trinsic.c maps/jhelom.c maps/skara.c maps/magincia.c maps/moonglow.c maps/paws.c maps/vesper.c maps/den.c maps/cove.c maps/empath.c maps/lycaeum.c maps/serpent.c maps/lcb_1.c maps/shrine.c
TILES=images/tiles.c images/charset.c
SRCS=u4.c map.c ttype.c screen.c event.c savegame.c $(MAPS) $(TILES)
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
	rm -f util/makeworldmap util/makecity util/makearea $(MAPS)

maps/worldmap.c: util/makeworldmap
	mkdir -p maps
	./util/makeworldmap $(U4)/world.map maps/worldmap.c

maps/britain.c: util/makecity
	mkdir -p maps
	./util/makecity britain 0 15 $(U4)/britain.ult $(U4)/britain.tlk maps/britain.c

maps/yew.c: util/makecity
	mkdir -p maps
	./util/makecity yew 0 15 $(U4)/yew.ult $(U4)/yew.tlk maps/yew.c

maps/minoc.c: util/makecity
	mkdir -p maps
	./util/makecity minoc 0 15 $(U4)/minoc.ult $(U4)/minoc.tlk maps/minoc.c

maps/trinsic.c: util/makecity
	mkdir -p maps
	./util/makecity trinsic 0 15 $(U4)/trinsic.ult $(U4)/trinsic.tlk maps/trinsic.c

maps/jhelom.c: util/makecity
	mkdir -p maps
	./util/makecity jhelom 0 15 $(U4)/jhelom.ult $(U4)/jhelom.tlk maps/jhelom.c

maps/skara.c: util/makecity
	mkdir -p maps
	./util/makecity skara 0 15 $(U4)/skara.ult $(U4)/skara.tlk maps/skara.c

maps/magincia.c: util/makecity
	mkdir -p maps
	./util/makecity magincia 0 15 $(U4)/magincia.ult $(U4)/magincia.tlk maps/magincia.c

maps/moonglow.c: util/makecity
	mkdir -p maps
	./util/makecity moonglow 0 15 $(U4)/moonglow.ult $(U4)/moonglow.tlk maps/moonglow.c

maps/paws.c: util/makecity
	mkdir -p maps
	./util/makecity paws 0 15 $(U4)/paws.ult $(U4)/paws.tlk maps/paws.c

maps/vesper.c: util/makecity
	mkdir -p maps
	./util/makecity vesper 0 15 $(U4)/vesper.ult $(U4)/vesper.tlk maps/vesper.c

maps/den.c: util/makecity
	mkdir -p maps
	./util/makecity den 0 15 $(U4)/den.ult $(U4)/den.tlk maps/den.c

maps/cove.c: util/makecity
	mkdir -p maps
	./util/makecity cove 0 15 $(U4)/cove.ult $(U4)/cove.tlk maps/cove.c

maps/empath.c: util/makecity
	mkdir -p maps
	./util/makecity empath 15 31 $(U4)/empath.ult $(U4)/empath.tlk maps/empath.c

maps/lycaeum.c: util/makecity
	mkdir -p maps
	./util/makecity lycaeum 15 31 $(U4)/lycaeum.ult $(U4)/lycaeum.tlk maps/lycaeum.c

maps/serpent.c: util/makecity
	mkdir -p maps
	./util/makecity serpent 15 31 $(U4)/serpent.ult $(U4)/serpent.tlk maps/serpent.c

maps/lcb_1.c: util/makecity
	mkdir -p maps
	./util/makecity lcb_1 15 31 $(U4)/lcb_1.ult $(U4)/lcb.tlk maps/lcb_1.c

maps/shrine.c: util/makearea
	mkdir -p maps
	./util/makearea shrine 5 5 $(U4)/shrine.con maps/shrine.c

util/makeworldmap: util/makeworldmap.c
	$(CC) $(CFLAGS) -o $@ $<

util/makecity: util/makecity.c
	$(CC) $(CFLAGS) -o $@ $<

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
