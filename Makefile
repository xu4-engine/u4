# GNU Makefile for Linux & macOS.

OS := $(shell uname)
ifeq ($(OS), Darwin)
MFILE_OS=Makefile.macosx
else
MFILE_OS=Makefile
endif

all: src/xu4 render.pak Ultima-IV.mod

src/xu4:
	make -C src -f $(MFILE_OS)

render.pak: module/render/shader/*.glsl
	boron -s tools/pack-xu4.b -f module/render -o $@

Ultima-IV.mod: module/Ultima-IV/*.b
	boron -s tools/pack-xu4.b -o $@

.PHONY: clean download snapshot

clean:
	make -C src -f $(MFILE_OS) clean
	rm -f Ultima-IV.mod

download: ultima4.zip u4upgrad.zip

ultima4.zip:
	curl -sSL -o $@ http://ultima.thatfleminggent.com/ultima4.zip

u4upgrad.zip:
	curl -sSL -o $@ http://sourceforge.net/projects/xu4/files/Ultima%204%20VGA%20Upgrade/1.3/u4upgrad.zip

snapshot: render.pak Ultima-IV.mod
	@rm -f project.tar.gz
	copr -c
	tools/cbuild windows
	tools/cbuild linux
	scp /tmp/xu4-linux.tar.gz /tmp/xu4-win32.zip $(SF_USER),xu4@web.sourceforge.net:/home/project-web/xu4/web/download
