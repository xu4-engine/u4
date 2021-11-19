# GNU Makefile for Linux & macOS.

OS := $(shell uname)
ifeq ($(OS), Darwin)
MFILE_OS=Makefile.macosx
else
MFILE_OS=Makefile
endif

all: src/xu4

src/xu4:
	make -C src -f $(MFILE_OS)

u4.mod: module/u4/*.b module/u4/shader/*.glsl
	boron -s tools/pack-xu4.b -o $@

.PHONY: clean download

clean:
	make -C src -f $(MFILE_OS) clean
	rm -f u4.mod

download: ultima4.zip u4upgrad.zip

ultima4.zip:
	curl -sSL -o $@ http://ultima.thatfleminggent.com/ultima4.zip

u4upgrad.zip:
	curl -sSL -o $@ http://sourceforge.net/projects/xu4/files/Ultima%204%20VGA%20Upgrade/1.3/u4upgrad.zip
