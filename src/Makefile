#
# Makefile for Linux
#

-include ../make.config

PREFIX ?= /usr/local
UI ?= allegro
CONF ?= boron
GPU ?= scale

bindir=$(PREFIX)/bin
libdir=$(PREFIX)/lib
datadir=$(PREFIX)/share

ifeq ($(UI), sdl)
SOUND=sdl
UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)
else
SOUND=faun
ifeq ($(SOUND),faun)
UILIBS=-lallegro -lfaun
else
UILIBS=-lallegro_acodec -lallegro_audio -lallegro
endif
UIFLAGS=
endif

ifeq ($(GPU),scale)
	CONF=boron
	UIFLAGS+=-DUSE_GL
	UILIBS+=-lGL
else ifeq ($(GPU),all)
	CONF=boron
	UIFLAGS+=-DUSE_GL -DGPU_RENDER
	UILIBS+=-lGL
endif

ifeq ($(CONF),xml)
	UIFLAGS+=$(shell xml2-config --cflags)
	UILIBS+=$(shell xml2-config --libs)
else
	UIFLAGS+=-DUSE_BORON -DCONF_MODULE
	UILIBS+=-lboron
endif

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
DEBUGCXXFLAGS=-rdynamic -g
CXXFLAGS=$(FEATURES) -Wall -I. -Isupport $(UIFLAGS) -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS)
CFLAGS=$(CXXFLAGS)
LIBS=$(UILIBS) -lpng -lz
INSTALL=install

ifeq ($(STATIC_GCC_LIBS),true)
    LDFLAGS+=-L. -static-libgcc
endif

include Makefile.common

install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D ../icons/xu4.png $(datadir)/icons/hicolor/48x48/apps/xu4.png
	$(INSTALL) -D ../dist/xu4.desktop $(datadir)/applications/xu4.desktop
	mkdir -p $(libdir)/u4/music
	$(INSTALL) ../mid/*.mid $(libdir)/u4/music
	$(INSTALL) ../mid/*.it $(libdir)/u4/music
	mkdir -p $(libdir)/u4/sound
	$(INSTALL) ../sound/*.ogg $(libdir)/u4/sound
	$(INSTALL) -D coord $(libdir)/u4/coord
	$(INSTALL) -D dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) -D tlkconv $(libdir)/u4/tlkconv
	$(INSTALL) -D u4dec $(libdir)/u4/u4dec
	$(INSTALL) -D u4enc $(libdir)/u4/u4enc
	$(INSTALL) -D u4unpackexe $(libdir)/u4/u4unpackexe
	$(INSTALL) ../conf/*.xml $(libdir)/u4
	mkdir -p $(libdir)/u4/dtd
	$(INSTALL) ../conf/dtd/*.dtd $(libdir)/u4/dtd
	mkdir -p $(libdir)/u4/graphics
	mkdir -p $(libdir)/u4/graphics/ega
	mkdir -p $(libdir)/u4/graphics/hires
	mkdir -p $(libdir)/u4/graphics/png
	mkdir -p $(libdir)/u4/graphics/vga
	mkdir -p $(libdir)/u4/graphics/vga2
	mkdir -p $(libdir)/u4/graphics/new
	$(INSTALL) ../graphics/ega/*.png $(libdir)/u4/graphics/ega
	$(INSTALL) ../graphics/hires/*.png $(libdir)/u4/graphics/hires
	$(INSTALL) ../graphics/hires/*.vga $(libdir)/u4/graphics/hires
	$(INSTALL) ../graphics/png/*.png $(libdir)/u4/graphics/png
	$(INSTALL) ../graphics/vga/*.png $(libdir)/u4/graphics/vga
	$(INSTALL) ../graphics/vga2/*.png $(libdir)/u4/graphics/vga2
	$(INSTALL) ../graphics/new/* $(libdir)/u4/graphics/new


# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

# a target to build without dependencies on libstdc++, libgcc_s
all.static_gcc_libs:
	ln -s `$(CXX) -print-file-name=libstdc++.a` .
	$(MAKE) STATIC_GCC_LIBS=true
	rm -f libstdc++.a

-include .depends

