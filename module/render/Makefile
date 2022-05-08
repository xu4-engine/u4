ODIR=font

UDIR=~/.fonts
SHARE=/usr/share/fonts

AOPT =-font $(UDIR)/Avatar.ttf -fontname avatar
COPT =-font $(SHARE)/aajohan-comfortaa/Comfortaa-Bold.ttf -fontname comfortaa
BOPT =-font $(UDIR)/Britannian-Runes-I.ttf -fontname runesI
B2OPT=-font $(UDIR)/Britannian-Runes-II.ttf -fontname runesII
MOPT =-font $(UDIR)/Merriweather-Regular.ttf -fontname merriweather

JOPT=-json $(ODIR)/cfont.json


.PHONY: all

all: $(ODIR)/cfont.png

$(ODIR)/cfont.png:
	msdf-atlas-gen -size 24 -pxrange 4 -coloringstrategy distance -scanline $(COPT) -and $(MOPT) -and $(AOPT) -and $(BOPT) -type mtsdf -format png -imageout $@ -txf $(ODIR)/cfont.txf -dimensions 512 300
