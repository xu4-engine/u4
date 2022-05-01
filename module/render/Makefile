# Fedora paths
CFONT=/usr/share/fonts/aajohan-comfortaa/Comfortaa-Regular.ttf

.PHONY: all

all: font/cfont.png

font/cfont.png:
	msdf-atlas-gen -font $(CFONT) -format png -imageout $@ -json font/cfont.json -txf font/cfont.txf -size 24 -dimensions 512 96
