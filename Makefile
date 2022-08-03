# GNU Makefile for Linux & macOS.

OS := $(shell uname)
ifeq ($(OS), Darwin)
MFILE_OS=Makefile.macosx
else
MFILE_OS=Makefile
endif

# Note: xu4 is hardcoded to look in /usr & /usr/local only!
DESTDIR ?= /usr/local

MODULES=render.pak Ultima-IV.mod U4-Upgrade.mod
REND=module/render


all: src/xu4 $(MODULES)

src/xu4:
	make -C src -f $(MFILE_OS) xu4

render.pak: $(REND)/shader/*.glsl $(REND)/shader/*.png $(REND)/font/cfont.png
	boron -s tools/pack-xu4.b -f $(REND) -o $@

Ultima-IV.mod: module/Ultima-IV/*.b
	boron -s tools/pack-xu4.b module/Ultima-IV

U4-Upgrade.mod: module/U4-Upgrade/*.b
	boron -s tools/pack-xu4.b module/U4-Upgrade

.PHONY: clean download mod snapshot

clean:
	make -C src -f $(MFILE_OS) clean
	rm -f $(MODULES)

mod: $(MODULES)

download: ultima4.zip u4upgrad.zip

ultima4.zip:
	curl -sSL -o $@ http://ultima.thatfleminggent.com/ultima4.zip

u4upgrad.zip:
	curl -sSL -o $@ http://sourceforge.net/projects/xu4/files/Ultima%204%20VGA%20Upgrade/1.3/u4upgrad.zip

install: src/xu4 $(MODULES) u4upgrad.zip
	mkdir -p $(DESTDIR)/bin $(DESTDIR)/share/xu4
	install -m 755 -s src/xu4 $(DESTDIR)/bin/xu4
	install -m 644 -t $(DESTDIR)/share/xu4 $(MODULES) u4upgrad.zip
	install -D -m 644 icons/xu4.png $(DESTDIR)/share/icons/hicolor/48x48/apps/xu4.png
	install -D -m 644 dist/xu4.desktop $(DESTDIR)/share/applications/xu4.desktop

snapshot: $(MODULES)
	@rm -f project.tar.gz
	copr -c
	tools/cbuild windows
	tools/cbuild linux
	scp /tmp/xu4-linux.tar.gz /tmp/xu4-win32.zip $(SF_USER),xu4@web.sourceforge.net:/home/project-web/xu4/htdocs/download
